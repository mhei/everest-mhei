// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "SimpleSwitch3ph1phAPI.hpp"
#include "configuration.h"
#include <chrono>
#include <date/date.h>
#include <date/tz.h>
#include <mutex>
#include <nlohmann/json.hpp>
#include <thread>
#include <utils/date.hpp>
#include <vector>

using nlohmann::json;
using namespace std::chrono_literals;

namespace module {

void SimpleSwitch3ph1phAPI::enforce_limits() {
    auto tp = date::utc_clock::now();

    this->limits.valid_until = Everest::Date::to_rfc3339(tp + 3600s);

    this->r_energy->call_enforce_limits(this->limits);
}

void SimpleSwitch3ph1phAPI::init() {
    std::scoped_lock lock(this->mutex);
    types::energy::LimitsRes limits_res;

    limits_res.ac_max_current_A = this->config.initial_ac_max_current_A;
    limits_res.ac_max_phase_count = this->config.initial_ac_max_phase_count;

    this->limits.limits_root_side = limits_res;

    this->r_energy->subscribe_energy_flow_request([this](types::energy::EnergyFlowRequest e) {
        std::scoped_lock lock(this->mutex);

        // store/update the received uuid
        this->limits.uuid = e.uuid;

        // publish the limits at least once if not yet done
        if (not this->initial_limits_pushed) {
            json l(this->limits.limits_root_side.value());
            EVLOG_debug << "Pushing initial limits: " << l.dump();
            this->enforce_limits();
            this->initial_limits_pushed = true;
        }
    });

    this->mqtt.subscribe("everest_api/switch3ph1ph/cmd/set_limit_amps_and_phases", [this](const std::string& data) {
        if (!data.empty()) {
            try {
                std::scoped_lock lock(this->mutex);
                auto arg = json::parse(data);

                this->limits.limits_root_side = arg;

                // we can push the limits only if we already received the uuid of our consumer
                if (this->initial_limits_pushed) {
                    EVLOG_debug << "set_limit_amps_and_phases: " << data;
                    this->enforce_limits();
                    this->r_energy->call_enforce_limits(this->limits);
                } else {
                    EVLOG_warning << "set_limit_amps_and_phases: " << data << " (delayed)";
                }
            } catch (const std::exception& e) {
                EVLOG_error << "set_limit_amps_and_phases: Cannot parse argument, command ignored: " << e.what();
            }
        } else {
            EVLOG_error << "set_limit_amps_and_phases: No argument specified, ignoring command";
        }
    });

    invoke_init(*p_empty);

    EVLOG_info << MODULE_DESCRIPTION << " (version: " << PROJECT_VERSION << ")";
}

void SimpleSwitch3ph1phAPI::ready() {
    invoke_ready(*p_empty);

    while (true) {
        std::unique_lock lock(this->mutex);

        if (this->initial_limits_pushed)
            this->enforce_limits();

        lock.unlock();
        std::this_thread::sleep_for(10s);
    }
}

} // namespace module
