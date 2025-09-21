// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#ifndef SIMPLE_EVDEV_TX_STOP_BTN_HPP
#define SIMPLE_EVDEV_TX_STOP_BTN_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 2
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/empty/Implementation.hpp>

// headers for required interface implementations
#include <generated/interfaces/evse_manager/Interface.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
// insert your custom include headers here
#include <libevdev/libevdev.h>
#include <sys/poll.h>
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    std::string device;
    std::string key;
    bool debug;
};

class SimpleEVDevTxStopBtn : public Everest::ModuleBase {
public:
    SimpleEVDevTxStopBtn() = delete;
    SimpleEVDevTxStopBtn(const ModuleInfo& info, std::unique_ptr<emptyImplBase> p_empty,
                         std::unique_ptr<evse_managerIntf> r_evse_manager, Conf& config) :
        ModuleBase(info), p_empty(std::move(p_empty)), r_evse_manager(std::move(r_evse_manager)), config(config){};

    const std::unique_ptr<emptyImplBase> p_empty;
    const std::unique_ptr<evse_managerIntf> r_evse_manager;
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

    // file descriptor
    struct pollfd poll_fd {
        0, POLLIN, 0
    };

    // libevdev handle
    struct libevdev* dev;

    // key code to trigger on
    int event_code;
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// insert other definitions here
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // SIMPLE_EVDEV_TX_STOP_BTN_HPP
