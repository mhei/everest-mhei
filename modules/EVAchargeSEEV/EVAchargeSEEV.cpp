// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "EVAchargeSEEV.hpp"
#include <GensioEVerestLogger.hpp>
#include <KL02.hpp>
#include <memory>

namespace module {

void EVAchargeSEEV::init() {
    try {
        this->gensio_os_funcs = std::make_unique<gensios::Os_Funcs>(-1, new GensioEVerestLogger);
        this->gensio_os_funcs->proc_setup();

        this->gensio_waiter = std::make_unique<gensios::Waiter>(*this->gensio_os_funcs);

        this->controller = std::make_unique<KL02>(&(*this->gensio_waiter), true, true, true);
    } catch (gensios::gensio_error& e) {
        EVLOG_error << e.what();
        return;
    }

    invoke_init(*p_ev_board_support);
}

void EVAchargeSEEV::ready() {
    invoke_ready(*p_ev_board_support);

    while (true) {
        try {
            // a gensio cannot be re-used, we have to create always a new instance
            gensios::GensioW gensio(this->config.kl02_gensio, *this->gensio_os_funcs, &(*this->controller));
            this->controller->set_gensio(&gensio);

            EVLOG_debug << "opening gensio: " << this->config.kl02_gensio;
            gensio->open_s();
            gensio->set_read_callback_enable(true);

            this->controller->enable_tx();

            this->gensio_waiter->wait(1);

            this->controller->disable_tx();

            int err = this->controller->get_err();
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

} // namespace module
