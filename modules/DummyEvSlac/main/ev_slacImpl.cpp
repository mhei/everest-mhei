// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#include "ev_slacImpl.hpp"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

namespace module {
namespace main {

void ev_slacImpl::init() {
}

void ev_slacImpl::ready() {
    publish_state(state);

    const unsigned int seconds_until_match = 5;
    unsigned int seconds_until_match_remaining = seconds_until_match;

    while (true) {
        if (state == types::slac::State::MATCHING) {
            if (seconds_until_match_remaining > 0) {
                seconds_until_match_remaining--;
            } else {
                EVLOG_info << "now MATCHED";
                state = types::slac::State::MATCHED;
                publish_state(state);
                publish_dlink_ready(true);
                seconds_until_match_remaining = seconds_until_match;
            }
        }
        std::this_thread::sleep_for(1s);
    }
}

void ev_slacImpl::handle_reset() {
    EVLOG_info << "handle_reset()";

    if (state != types::slac::State::UNMATCHED) {
        state = types::slac::State::UNMATCHED;
        publish_state(state);
        publish_dlink_ready(false);

        EVLOG_info << "handle_reset: finally UNMATCHED";
    }
}

bool ev_slacImpl::handle_trigger_matching() {
    EVLOG_info << "handle_trigger_matching: started MATCHING";

    state = types::slac::State::MATCHING;
    publish_state(state);

    return true;
}

} // namespace main
} // namespace module
