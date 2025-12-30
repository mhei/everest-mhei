// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#ifndef STRONGLINK_SL032TOKEN_PROVIDER_HPP
#define STRONGLINK_SL032TOKEN_PROVIDER_HPP

//
// AUTO GENERATED - MARKED REGIONS WILL BE KEPT
// template version 2
//

#include "ld-ev.hpp"

// headers for provided interface implementations
#include <generated/interfaces/auth_token_provider/Implementation.hpp>

// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1
// insert your custom include headers here
#include "StronglinkSL032Event.hpp"
#include <gensio/gensio>
#include <memory>
// ev@4bf81b14-a215-475c-a1d3-0a484ae48918:v1

namespace module {

struct Conf {
    std::string device;
    int baudrate;
    std::string gensio;
    int interval_ms;
    bool ignore_echo;
    bool debug;
};

class StronglinkSL032TokenProvider : public Everest::ModuleBase {
public:
    StronglinkSL032TokenProvider() = delete;
    StronglinkSL032TokenProvider(const ModuleInfo& info, std::unique_ptr<auth_token_providerImplBase> p_main,
                                 Conf& config) :
        ModuleBase(info), p_main(std::move(p_main)), config(config){};

    const std::unique_ptr<auth_token_providerImplBase> p_main;
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
    std::unique_ptr<gensios::Os_Funcs> gensio_os_funcs;
    std::unique_ptr<gensios::Waiter> gensio_waiter;
    std::unique_ptr<StronglinkSL032Event> gensio_event;
    std::string gensio_str;

    void on_token(const std::string& id_token);
    // ev@211cfdbe-f69a-4cd6-a4ec-f8aaa3d1b6c8:v1
};

// ev@087e516b-124c-48df-94fb-109508c7cda9:v1
// insert other definitions here
// ev@087e516b-124c-48df-94fb-109508c7cda9:v1

} // namespace module

#endif // STRONGLINK_SL032TOKEN_PROVIDER_HPP
