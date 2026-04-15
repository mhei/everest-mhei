// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#include "evse_board_supportImpl.hpp"
#include <atomic>

namespace module {
namespace evse_board_support {

void evse_board_supportImpl::init() {
    // configure hardware capabilities: use user-configurable settings for flexibility
    // but use the same value for import and export - there seems to be no reason for AC
    // that these values differ for import and export
    this->hw_capabilities.min_current_A_import = this->mod->config.min_current_A;
    this->hw_capabilities.max_current_A_import = this->mod->config.max_current_A;
    this->hw_capabilities.min_current_A_export = this->mod->config.min_current_A;
    this->hw_capabilities.max_current_A_export = this->mod->config.max_current_A;

    if (this->mod->config.min_phase_count > this->mod->config.max_phase_count) {
        throw std::runtime_error(fmt::format("Invalid phase count configuration: min_phase_count ({}) must be <= "
                                             "max_phase_count ({})",
                                             this->mod->config.min_phase_count, this->mod->config.max_phase_count));
    }

    this->hw_capabilities.max_phase_count_import = this->mod->config.max_phase_count;
    this->hw_capabilities.min_phase_count_import = this->mod->config.min_phase_count;
    this->hw_capabilities.max_phase_count_export = this->mod->config.max_phase_count;
    this->hw_capabilities.min_phase_count_export = this->mod->config.min_phase_count;

    // on the hardware, there is no native support for contactors (without addon board)
    this->hw_capabilities.supports_changing_phases_during_charging = false;

    this->hw_capabilities.connector_type =
        types::evse_board_support::string_to_connector_type(this->mod->config.connector_type);

    // state E generation is supported
    this->hw_capabilities.supports_cp_state_E = true;

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

        this->pp_ampacity = {new_ampacity};

        // we received a valid measurement update, so clear any possible error raised before
        if (this->pp_fault_reported.exchange(false)) {
            this->clear_error("evse_board_support/MREC23ProximityFault");
        }

        // publish new value
        this->publish_ac_pp_ampacity(this->pp_ampacity.value());

        // publish a ProximityFault when this we detected a plug removal during charging
        if (this->pp_ampacity.value().ampacity == types::board_support_common::Ampacity::None &&
            (this->cp_current_state == types::board_support_common::Event::C ||
             this->cp_current_state == types::board_support_common::Event::D)) {
            if (!this->pp_fault_reported.exchange(true)) {
                Everest::error::Error error_object = this->error_factory->create_error(
                    "evse_board_support/MREC23ProximityFault", "PlugRemoval", "Plug removed from socket during charge",
                    Everest::error::Severity::High);
                this->raise_error(error_object);
            }
        }
    });

    this->mod->controller->on_pp_error.connect([&](const std::string& err_msg) {
        // publish a ProximityFault
        if (!this->pp_fault_reported.exchange(true)) {
            EVLOG_error << err_msg;

            Everest::error::Error error_object = this->error_factory->create_error(
                "evse_board_support/MREC23ProximityFault", "MeasurementError", err_msg, Everest::error::Severity::High);
            this->raise_error(error_object);
        }
    });

    this->mod->controller->on_cp_change.connect([&](const types::board_support_common::Event& event) {
        EVLOG_info << "CP state change from " << this->cp_current_state << " to " << event << ", "
                   << "PWM: " << std::fixed << std::setprecision(1) << this->mod->controller->get_cached_duty_cycle()
                   << "%";

        this->publish_event({event});
        this->cp_current_state = event;

        // clear a possible ProximityFault error on transition to state A
        if (event == types::board_support_common::Event::A && this->pp_fault_reported.exchange(false)) {
            this->clear_error("evse_board_support/MREC23ProximityFault");
        }
    });

    this->mod->controller->on_cp_error.connect([&](const KL02::CPError err) {
        if (err == KL02::CPError::None) {
            if (this->diode_fault_is_active.exchange(false)) {
                this->clear_error("evse_board_support/DiodeFault");
            }
            if (this->pilot_fault_is_active.exchange(false)) {
                this->clear_error("evse_board_support/MREC14PilotFault");
            }
        } else {
            Everest::error::Error error_object;
            std::string err_msg;

            switch (err) {
            case KL02::CPError::DiodeFault:
                err_msg = "Diode fault detected.";
                error_object = this->error_factory->create_error("evse_board_support/DiodeFault", "", err_msg,
                                                                 Everest::error::Severity::High);
                this->diode_fault_is_active = true;
                break;
            case KL02::CPError::CPShort:
                err_msg = "CP short fault detected.";
                error_object = this->error_factory->create_error("evse_board_support/MREC14PilotFault", "CPShortFault",
                                                                 err_msg, Everest::error::Severity::High);
                this->pilot_fault_is_active = true;
                break;
            case KL02::CPError::PilotFault:
                err_msg = "CP voltage is out of range.";
                error_object = this->error_factory->create_error("evse_board_support/MREC14PilotFault", "CPOutOfRange",
                                                                 err_msg, Everest::error::Severity::High);
                this->pilot_fault_is_active = true;
                break;
            default:
                err_msg = "Unknown CP error detected.";
                error_object = this->error_factory->create_error("evse_board_support/MREC14PilotFault", "Unknown",
                                                                 err_msg, Everest::error::Severity::High);
                this->pilot_fault_is_active = true;
            }

            EVLOG_error << err_msg;
            this->raise_error(error_object);
        }
    });
}

void evse_board_supportImpl::ready() {
    // the BSP must publish this variable at least once during start up
    this->publish_capabilities(this->hw_capabilities);
}

void evse_board_supportImpl::handle_enable(bool& value) {
    if (value) {
        auto duty_cycle = this->mod->controller->get_cached_duty_cycle();

        EVLOG_info << "handle_enable: " << (value ? "Applying cached" : "Setting") << " duty cycle of " << std::fixed
                   << std::setprecision(1) << duty_cycle << "%";

        // set_duty_cycle automatically calls pwm_enable()
        this->mod->controller->set_duty_cycle(duty_cycle);

        this->mod->controller->enable();
    } else {
        EVLOG_info << "handle_enable: disable";

        this->mod->controller->set_duty_cycle(0.0);

        this->mod->controller->disable();

        this->cp_current_state = types::board_support_common::Event::PowerOn;
        this->pp_ampacity.reset();
    }
    this->is_enabled = value;
}

void evse_board_supportImpl::handle_pwm_on(double& value) {
    EVLOG_info << "handle_pwm_on: " << (this->is_enabled ? "Setting" : "Caching") << " new duty cycle of " << std::fixed
               << std::setprecision(1) << value << "%";

    this->mod->controller->set_duty_cycle(value);
}

void evse_board_supportImpl::handle_cp_state_X1() {
    EVLOG_info << "handle_cp_state_X1: " << (this->is_enabled ? "Setting" : "Caching") << " new duty cycle of 100.0%";
    this->mod->controller->set_duty_cycle(100.0);
}

void evse_board_supportImpl::handle_cp_state_F() {
    EVLOG_info << "handle_cp_state_F: " << (this->is_enabled ? "Setting" : "Caching") << " new duty cycle of 0.0%";
    this->mod->controller->set_duty_cycle(0.0);
}

void evse_board_supportImpl::handle_cp_state_E() {
    EVLOG_info << "handle_cp_state_E: disable PWM circuit";
    this->mod->controller->pwm_disable();
}

void evse_board_supportImpl::handle_allow_power_on(types::evse_board_support::PowerOnOff& value) {
    // simulate a contactor
    this->publish_event({value.allow_power_on ? types::board_support_common::Event::PowerOn
                                              : types::board_support_common::Event::PowerOff});
}

void evse_board_supportImpl::handle_ac_switch_three_phases_while_charging(bool& value) {
    // your code for cmd ac_switch_three_phases_while_charging goes here
    (void)value;
}

void evse_board_supportImpl::handle_ac_set_overcurrent_limit_A(double& value) {
    // your code for cmd ac_set_overcurrent_limit_A goes here
    (void)value;
}

} // namespace evse_board_support
} // namespace module
