// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "StronglinkSL032TokenProvider.hpp"
#include <GensioEVerestLogger.hpp>
#include <chrono>
#include <fmt/core.h>
#include <gensio/gensio>
#include <iomanip>
#include <iostream>
#include <string>
#if !DISABLE_EDM
#include <everest/helpers/helpers.hpp>
#endif

using namespace std::chrono_literals;

#if DISABLE_EDM
namespace everest {
namespace helpers {

types::authorization::ProvidedIdToken& redact(types::authorization::ProvidedIdToken& token) {
    return token;
}

} // namespace helpers
} // namespace everest
#endif

namespace module {

void StronglinkSL032TokenProvider::init() {

    this->gensio_str = fmt::format(this->config.gensio, fmt::arg("device", this->config.device),
                                   fmt::arg("baudrate", this->config.baudrate));

    try {
        this->gensio_os_funcs = std::make_unique<gensios::Os_Funcs>(-1, new GensioEVerestLogger);
        this->gensio_os_funcs->proc_setup();

        this->gensio_waiter = std::make_unique<gensios::Waiter>(*this->gensio_os_funcs);

        this->gensio_event = std::make_unique<StronglinkSL032Event>(&(*this->gensio_waiter), this->config.interval_ms,
                                                                    this->config.ignore_echo);
    } catch (gensios::gensio_error& e) {
        EVLOG_error << e.what();
        return;
    }

    this->gensio_event->on_token.connect(&StronglinkSL032TokenProvider::on_token, this);

    invoke_init(*p_main);
}

void StronglinkSL032TokenProvider::ready() {
    invoke_ready(*p_main);

    while (true) {
        try {
            // a gensio cannot be re-used, we have to create always a new instance
            gensios::GensioW gensio(this->gensio_str, *this->gensio_os_funcs, &(*this->gensio_event));
            this->gensio_event->set_gensio(&gensio);

            EVLOG_debug << "opening gensio: " << this->gensio_str;
            gensio->open_s();
            gensio->set_read_callback_enable(true);

            this->gensio_event->enable_tx();

            this->gensio_waiter->wait(1);

            this->gensio_event->disable_tx();

            int err = this->gensio_event->get_err();
            if (err) {
                EVLOG_error << gensios::err_to_string(err);
            }

            // it's better to close it before it is destroyed, but
            // the close must complete before the destruction.
            EVLOG_debug << "closing gensio";
            gensio->close_s();

        } catch (gensios::gensio_error& e) {
            if (e.get_error() == GE_NOTFOUND) {
                EVLOG_error << "Failed to open/connect the gensio: device does not exists or connection not possible.";
            } else {
                EVLOG_error << e.what();
            }
        }

        // in case of error, just sleep shortly, then try to re-establish the
        // connection to the reader
        std::this_thread::sleep_for(5s);
    }
}

void StronglinkSL032TokenProvider::on_token(const std::string& id_token) {
    types::authorization::ProvidedIdToken token;

    // guess the type according to the string length:
    // ISO14443 UIDs are either 4 bytes, 7 bytes or 10 bytes
    // ISO15693 UIDs are only 8 bytes
    switch (id_token.length()) {
    case 8:
        [[fallthrough]];
    case 14:
        [[fallthrough]];
    case 20:
        token.id_token.type = types::authorization::IdTokenType::ISO14443;
        break;
    case 16:
        token.id_token.type = types::authorization::IdTokenType::ISO15693;
        break;
    default:
        token.id_token.type = types::authorization::IdTokenType::Local;
    }

    token.id_token.value = id_token;
    token.authorization_type = types::authorization::AuthorizationType::RFID;

    if (this->config.debug) {
        EVLOG_info << "Publishing new RFID/NFC token: " << everest::helpers::redact(token);
    } else {
        EVLOG_debug << "Publishing new RFID/NFC token: " << everest::helpers::redact(token);
    }

    this->p_main->publish_provided_token(token);
}

} // namespace module
