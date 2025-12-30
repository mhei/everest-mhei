// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "StronglinkSL032Event.hpp"
#include <fmt/core.h>

using namespace StronglinkSL032;

StronglinkSL032Event::StronglinkSL032Event(gensios::Waiter* w, int interval, bool ignore_echo) :
    ignore_echo(ignore_echo), waiter(w) {
    this->sleep_interval = std::chrono::milliseconds(interval);

    this->query_thread = std::thread([this] {
        std::vector<unsigned char> frame = TxFrame(StronglinkSL032::Command::SelectMifareCard, {});

        while (!this->termination_requested) {
            if (this->tx_enabled) {
                if (this->ignore_echo) {
                    this->still_to_discard += frame.size();
                }

                try {
                    auto c = this->io->write(frame, NULL);
                    this->dump_frame("sent: ", true, frame);

                    if (c != frame.size()) {
                        EVLOG_warning << "could not sent complete request frame - only " << c << " of "
                                      << std::to_string(frame.size()) << " bytes";
                    }
                } catch (gensios::gensio_error& e) {
                    // error could happen in case gensio is already gone
                    EVLOG_verbose << e.what();
                }
            }
            std::this_thread::sleep_for(this->sleep_interval);
        }
    });
}

void StronglinkSL032Event::enable_tx() {
    this->tx_enabled = true;
}

void StronglinkSL032Event::disable_tx() {
    this->tx_enabled = false;
}

gensios::gensiods StronglinkSL032Event::read(int ierr, const gensios::SimpleUCharVector data,
                                             const char* const* auxdata) {
    (void)auxdata;

    if (ierr) {
        this->err = ierr;
        this->io->set_read_callback_enable(false);
        this->io->set_write_callback_enable(false);
        this->waiter->wake();
        return 0;
    }

    this->dump_frame("received: ", false, data);

    // read the value (note: the other thread is only increasing it)
    unsigned int to_discard = this->still_to_discard.exchange(0);

    if (to_discard >= data.size()) {
        // exact match or still more to discard
        to_discard -= data.size();
        // remember remaining part to still discard
        this->still_to_discard += to_discard;
        // tell that we processed the data
        return data.size();
    }

    this->rx_frame.push_back(data.data(), data.size());

    if (this->rx_frame.is_valid()) {
        auto cmd = this->rx_frame.get_cmd();
        auto status = this->rx_frame.get_status();
        std::string uid;
        std::string type;

        switch (cmd) {
        case Command::SelectMifareCard:
            switch (status) {
            case Status::Success:
                std::tie(uid, type) = this->rx_frame.get_card_info();
                EVLOG_verbose << "Tag recognized: uid: " << uid << ", type: " << type;

                if (uid != this->reported_tag) {
                    this->on_token(uid);
                    this->reported_tag = uid;
                }
                break;

            case Status::NoTag:
                EVLOG_verbose << "No Tag";
                this->reported_tag = "";
                break;

            default:
                // ignore
                ;
            }
            break;

        default:
            EVLOG_warning << fmt::format("response for command {:#04x} received", static_cast<unsigned int>(cmd));
        }

        this->rx_frame.clear();
    }

    // consumed data
    return data.size();
}
