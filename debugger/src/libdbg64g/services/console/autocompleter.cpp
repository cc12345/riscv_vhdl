/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Auto Completer implementation.
 */

#include <string.h>
#include "autocompleter.h"
#include "coreservices/ithread.h"

namespace debugger {

/** Class registration in the Core */
REGISTER_CLASS(AutoCompleter)

AutoCompleter::AutoCompleter(const char *name) 
    : IService(name) {
    registerInterface(static_cast<IAutoComplete *>(this));
    registerAttribute("SocInfo", &socInfo_);
    registerAttribute("History", &history_);
    registerAttribute("HistorySize", &history_size_);

    socInfo_.make_string("");
    history_.make_list(0);
    history_size_.make_int64(4);
    history_idx_ = 0;

    cmdLine_ = "";
    carretPos_ = 0;
}

AutoCompleter::~AutoCompleter() {
}

void AutoCompleter::postinitService() {
    info_ = static_cast<ISocInfo *>
            (RISCV_get_service_iface(socInfo_.to_string(), IFACE_SOC_INFO));
    history_idx_ = history_.size();
}

bool AutoCompleter::processKey(uint32_t qt_key,
                                AttributeType *cmd,
                                AttributeType *cursor) {
    bool isNewLine = false;
    bool set_history_end = true;
    if (!cursor->is_list() || cursor->size() != 2) {
        cursor->make_list(2);
        (*cursor)[0u].make_int64(0);
        (*cursor)[1].make_int64(0);
    }
    switch (qt_key) {
    case KB_Up:
        set_history_end = false;
        if (history_idx_ == history_.size()) {
            unfinshedLine_ = cmdLine_;
        }
        if (history_idx_ > 0) {
            history_idx_--;
        }
        cmdLine_ = std::string(history_[history_idx_].to_string());
        break;
    case KB_Down:
        set_history_end = false;
        if (history_idx_ == (history_.size() - 1)) {
            history_idx_++;
            cmdLine_ = unfinshedLine_;
        } else if (history_idx_ < (history_.size() - 1)) {
            history_idx_++;
            cmdLine_ = std::string(history_[history_idx_].to_string());
        }
        break;
    case KB_Left:
        if (carretPos_ < cmdLine_.size()) {
            carretPos_++;
        }
        break;
    case KB_Right:
        if (carretPos_ > 0) {
            carretPos_--;
        }
        break;
    case KB_Backspace:// 1. Backspace button:
        if (cmdLine_.size() && carretPos_ == 0) {
            cmdLine_.erase(cmdLine_.size() - 1);
        } else if (cmdLine_.size() && carretPos_ < cmdLine_.size()) {
            std::string t1 = cmdLine_.substr(0, cmdLine_.size() - carretPos_ - 1);
            cmdLine_ = t1 + cmdLine_.substr(cmdLine_.size() - carretPos_, carretPos_);
        }
        break;
    case KB_Return:// 2. Enter button:
        isNewLine = true;
        break;
    case 0:
        break;
    case KB_Shift:
    case KB_Control:
    case KB_Alt:
        break;
    default:
        if (carretPos_ == 0) {
            cmdLine_ += static_cast<uint8_t>(qt_key);
        } else if (carretPos_ == cmdLine_.size()) {
            std::string t1;
            t1 += static_cast<uint8_t>(qt_key);
            cmdLine_ = t1 + cmdLine_;
        } else {
            std::string t1 = cmdLine_.substr(0, cmdLine_.size() - carretPos_);
            t1 += static_cast<uint8_t>(qt_key);
            cmdLine_ = t1 + cmdLine_.substr(cmdLine_.size() - carretPos_, carretPos_);
        }
    }

    cmd->make_string(cmdLine_.c_str());

    if (isNewLine) {
        if (cmdLine_.size()) {
            addToHistory(cmdLine_.c_str());
            cmdLine_.clear();
        }
        carretPos_ = 0;
    }
    (*cursor)[0u].make_int64(carretPos_);
    (*cursor)[1].make_int64(carretPos_);

    if (set_history_end) {
        history_idx_ = history_.size();
    }
    return isNewLine;
}

void AutoCompleter::addToHistory(const char *cmd) {
    unsigned found = history_.size();
    for (unsigned i = 0; i < history_.size(); i++) {
        if (strcmp(cmd, history_[i].to_string()) == 0) {
            found = i;
            break;
        }
    }
    if (found  ==  history_.size()) {
        AttributeType t1;
        t1.make_string(cmd);
        history_.add_to_list(&t1);

        unsigned min_size = static_cast<unsigned>(history_size_.to_int64());
        if (history_.size() >= 2*min_size) {
            history_.trim_list(0, min_size);
        }
    } else if (found < (history_.size() - 1)) {
        history_.swap_list_item(found, history_.size() - 1);
    }
    history_idx_ = history_.size();
}

}  // namespace debugger