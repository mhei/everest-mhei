// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#include "ev_board_supportImpl.hpp"

namespace module {
namespace ev_board_support {

void ev_board_supportImpl::init() {

    this->mod->controller->on_pp_change.connect([&](const types::board_support_common::Ampacity& new_ampacity) {
        if (new_ampacity == types::board_support_common::Ampacity::None and this->pp_ampacity.has_value() and
            this->pp_ampacity.value().ampacity != types::board_support_common::Ampacity::None) {
            EVLOG_info << "PP noticed plug removal from socket";
        } else {
            if (this->pp_ampacity.has_value()) {
                EVLOG_info << "PP ampacity change from " << this->pp_ampacity.value().ampacity << " to "
                           << new_ampacity;
            } else {
                EVLOG_info << "PP ampacity change to " << new_ampacity;
            }
        }

        // remember last measurement value
        this->pp_ampacity = {new_ampacity};

        // update published value
        this->bsp_measurement.proximity_pilot = {new_ampacity};

        // publish new value
        this->publish_bsp_measurement(this->bsp_measurement);
    });

    this->mod->controller->on_cp_change.connect([&](const types::board_support_common::Event& event) {
        EVLOG_info << "CP state change from " << this->cp_current_state << " to " << event << ", "
                   << "PWM: " << std::fixed << std::setprecision(1) << this->mod->controller->get_cached_duty_cycle()
                   << "%";

        this->publish_bsp_event({event});
        this->cp_current_state = event;
    });

    this->mod->controller->on_duty_cycle_change.connect([&](const float& new_duty_cycle) {
        EVLOG_info << "Duty cycle change from " << std::fixed << std::setprecision(1)
                   << this->bsp_measurement.cp_pwm_duty_cycle << "%"
                   << " to " << std::fixed << std::setprecision(1) << new_duty_cycle << "%";

        this->bsp_measurement.cp_pwm_duty_cycle = new_duty_cycle;

        // publish new value
        this->publish_bsp_measurement(this->bsp_measurement);
    });
}

void ev_board_supportImpl::ready() {
}

void ev_board_supportImpl::handle_enable(bool& value) {
    if (value) {
        EVLOG_info << "handle_enable: enable";
        this->mod->controller->set_cp_state(this->requested_cp_state);
        this->mod->controller->enable();
    } else {
        EVLOG_info << "handle_enable: disable";
        this->mod->controller->set_cp_state(types::ev_board_support::EvCpState::A);
        this->mod->controller->disable();
        this->cp_current_state = types::board_support_common::Event::PowerOn;
        this->pp_ampacity.reset();
    }
    this->is_enabled = value;
}

void ev_board_supportImpl::handle_set_cp_state(types::ev_board_support::EvCpState& cp_state) {
    EVLOG_info << "handle_set_cp_state: " << cp_state;
    this->requested_cp_state = cp_state;
    if (this->is_enabled) {
        this->mod->controller->set_cp_state(cp_state);
    }
}

void ev_board_supportImpl::handle_allow_power_on(bool& value) {
    // your code for cmd allow_power_on goes here
    (void)value;
}

void ev_board_supportImpl::handle_diode_fail(bool& value) {
    // your code for cmd diode_fail goes here
    (void)value;
}

void ev_board_supportImpl::handle_set_ac_max_current(double& current) {
    // your code for cmd set_ac_max_current goes here
    (void)current;
}

void ev_board_supportImpl::handle_set_three_phases(bool& three_phases) {
    // your code for cmd set_three_phases goes here
    (void)three_phases;
}

void ev_board_supportImpl::handle_set_rcd_error(double& rcd_current_mA) {
    // your code for cmd set_rcd_error goes here
    (void)rcd_current_mA;
}

} // namespace ev_board_support
} // namespace module
