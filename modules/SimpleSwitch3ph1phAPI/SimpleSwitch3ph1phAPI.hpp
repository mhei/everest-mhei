// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#ifndef SIMPLE_SWITCH3PH1PH_API_HPP
#define SIMPLE_SWITCH3PH1PH_API_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 2
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/empty/Implementation.hpp>

// headers for required interface implementations
#include <generated/interfaces/energy/Interface.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
// insert your custom include headers here
#include <atomic>
#include <mutex>
#include <string>
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    int initial_ac_max_phase_count;
    double initial_ac_max_current_A;
};

class SimpleSwitch3ph1phAPI : public Everest::ModuleBase {
public:
    SimpleSwitch3ph1phAPI() = delete;
    SimpleSwitch3ph1phAPI(const ModuleInfo& info, Everest::MqttProvider& mqtt_provider,
                          std::unique_ptr<emptyImplBase> p_empty, std::unique_ptr<energyIntf> r_energy, Conf& config) :
        ModuleBase(info),
        mqtt(mqtt_provider),
        p_empty(std::move(p_empty)),
        r_energy(std::move(r_energy)),
        config(config){};

    Everest::MqttProvider& mqtt;
    const std::unique_ptr<emptyImplBase> p_empty;
    const std::unique_ptr<energyIntf> r_energy;
    const Conf& config;

    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1
    // insert your public definitions here
    // ev@1fce4c5e-0ab8-41bb-90f7-14277703d2ac:v1

protected:
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1
    // insert your protected definitions here
    // ev@4714b2ab-a24f-4b95-ab81-36439e1478de:v1

private:
    friend class LdEverest;
    void init();
    void ready();

    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
    // insert your private definitions here
    /// @brief The limits structure pushed to linked energy leaf node
    types::energy::EnforcedLimits limits;

    /// @brief Remember whether we have the initial limits pushed already
    bool initial_limits_pushed{false};

    /// @brief Lock to protect the limits structure and the bool
    std::mutex mutex;

    /// @brief Helper to update the schedule/timestamp
    void enforce_limits();
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// insert other definitions here
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // SIMPLE_SWITCH3PH1PH_API_HPP
