// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include <everest/logging.hpp>
#include <gensio/gensio>
#include <string>

class GensioEVerestLogger : public gensios::Os_Funcs_Log_Handler {
    void log(enum gensios::gensio_log_levels level, const std::string log) override {
        switch (level) {
        case gensios::GENSIO_LOG_FATAL:
            EVLOG_critical << log;
            break;
        case gensios::GENSIO_LOG_ERR:
            EVLOG_error << log;
            break;
        case gensios::GENSIO_LOG_WARNING:
            EVLOG_warning << log;
            break;
        case gensios::GENSIO_LOG_INFO:
            EVLOG_info << log;
            break;
        case gensios::GENSIO_LOG_DEBUG:
            EVLOG_debug << log;
            break;
        }
    }
};
