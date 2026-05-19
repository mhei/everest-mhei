// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include <chrono>
#include <condition_variable>
#include <everest/logging.hpp>
#include <generated/types/board_support_common.hpp>
#include <generated/types/ev_board_support.hpp>
#include <gensio/gensio>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <sigslot/signal.hpp>
#include <string>
#include <thread>

using namespace std::chrono_literals;

class MCU : public gensios::Event {
public:
    MCU(gensios::Waiter* w);
    ~MCU();

    // gensio stuff
    void set_gensio(gensios::Gensio* g);
    void enable_tx();
    void disable_tx();
    int get_err();

    // helper
    void enable();
    void disable();
    double get_cached_duty_cycle() const;

    // synchron methods
    void set_cp_state(const types::ev_board_support::EvCpState cp_state);

    // events
    sigslot::signal<types::board_support_common::Event&> on_cp_change;

    enum CPError {
        None,
        PilotFault,
    };
    sigslot::signal<const CPError&> on_cp_error;

    sigslot::signal<const types::board_support_common::Ampacity&> on_pp_change;
    sigslot::signal<const std::string&> on_pp_error;

    sigslot::signal<const float&> on_duty_cycle_change;

private:
    bool termination_requested{false};
    int err{0};
    bool tx_enabled{false};
    gensios::Gensio* io{NULL};
    gensios::Waiter* waiter;
    std::chrono::milliseconds sleep_interval{200ms};
    std::thread query_thread;
    std::atomic_bool is_enabled{false};
    bool reinit{false};
    double duty_cycle{100.0};

    bool cp_error_raised{false};
    bool pp_error_raised{false};

    std::mutex cmd_mtx;

    std::mutex rx_mtx;
    std::string rx_line;
    std::string cmd_response;

    std::condition_variable cv;

    types::board_support_common::Event previous_cp_state{types::board_support_common::Event::PowerOn};
    std::optional<types::board_support_common::Ampacity> previous_ampacity{};

    std::string execute_command(const std::string cmd, const std::chrono::milliseconds timeout = {250ms});

    types::board_support_common::Event ce_voltage_to_bsp_event(const float& v) const;
    void ce_voltage_to_cp_state(const float& v);
    void id_voltage_to_pp_state(const float& v);

    void reset();
    void echo_off();
    void auto_plug();
    void get_voltages();

    gensios::gensiods read(int ierr, const gensios::SimpleUCharVector data, const char* const* auxdata) override;

    void write_ready() override {
    }

    void freed() override {
        this->waiter->wake();
    }
};
