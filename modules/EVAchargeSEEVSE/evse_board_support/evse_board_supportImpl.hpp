// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#ifndef EVSE_BOARD_SUPPORT_EVSE_BOARD_SUPPORT_IMPL_HPP
#define EVSE_BOARD_SUPPORT_EVSE_BOARD_SUPPORT_IMPL_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 3
//

#include <generated/interfaces/evse_board_support/Implementation.hpp>

#include "../EVAchargeSEEVSE.hpp"

// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1
// insert your custom include headers here
#include <atomic>
// ev@75ac1216-19eb-4182-a85c-820f1fc2c091:v1

namespace module {
namespace evse_board_support {

struct Conf {};

class evse_board_supportImpl : public evse_board_supportImplBase {
public:
    evse_board_supportImpl() = delete;
    evse_board_supportImpl(Everest::ModuleAdapter* ev, const Everest::PtrContainer<EVAchargeSEEVSE>& mod,
                           Conf& config) :
        evse_board_supportImplBase(ev, "evse_board_support"), mod(mod), config(config){};

    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1
    // insert your public definitions here
    // ev@8ea32d28-373f-4c90-ae5e-b4fcc74e2a61:v1

protected:
    // command handler functions (virtual)
    virtual void handle_enable(bool& value) override;
    virtual void handle_pwm_on(double& value) override;
    virtual void handle_cp_state_X1() override;
    virtual void handle_cp_state_F() override;
    virtual void handle_cp_state_E() override;
    virtual void handle_allow_power_on(types::evse_board_support::PowerOnOff& value) override;
    virtual void handle_ac_switch_three_phases_while_charging(bool& value) override;
    virtual void handle_ac_set_overcurrent_limit_A(double& value) override;

    // ev@d2d1847a-7b88-41dd-ad07-92785f06f5c4:v1
    // insert your protected definitions here
    // ev@d2d1847a-7b88-41dd-ad07-92785f06f5c4:v1

private:
    const Everest::PtrContainer<EVAchargeSEEVSE>& mod;
    const Conf& config;

    virtual void init() override;
    virtual void ready() override;

    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
    // insert your private definitions here

    /// @brief Hardware Capabilities
    types::evse_board_support::HardwareCapabilities hw_capabilities;

    /// @brief Tracks whether this EVSE is enabled or not.
    std::atomic_bool is_enabled{false};

    /// @brief Tracks the last published CP state.
    types::board_support_common::Event cp_current_state{types::board_support_common::Event::PowerOn};

    /// @brief Last published/detected ampacity
    std::optional<types::board_support_common::ProximityPilot> pp_ampacity;

    /// @brief Flag to remember whether we already published a proximity error
    std::atomic_bool pp_fault_reported{false};

    /// @brief Flag to remember whether we already published a diode fault error
    std::atomic_bool diode_fault_is_active{false};

    /// @brief Flag to remember whether we already published a pilot fault error
    std::atomic_bool pilot_fault_is_active{false};

    // ev@3370e4dd-95f4-47a9-aaec-ea76f34a66c9:v1
};

// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1
// insert other definitions here
// ev@3d7da0ad-02c2-493d-9920-0bbbd56b9876:v1

} // namespace evse_board_support
} // namespace module

#endif // EVSE_BOARD_SUPPORT_EVSE_BOARD_SUPPORT_IMPL_HPP
