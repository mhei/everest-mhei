// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include <KL02USSProtocol.hpp>
#include <USSFrame.hpp>
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

class KL02 : public gensios::Event {
public:
    KL02(gensios::Waiter* w, bool is_ev, bool use_pp, bool use_pp_pullup);
    ~KL02();

    // gensio stuff
    void set_gensio(gensios::Gensio* g);
    void enable_tx();
    void disable_tx();
    int get_err();
    void trace(bool on);

    // helper for both modes
    void enable();
    void disable();
    double get_cached_duty_cycle() const;

    // synchron methods for EVSE mode
    bool pwm_is_enabled();
    void pwm_enable();
    void pwm_disable();
    void set_duty_cycle(const double duty_cycle);

    // synchron methods for EV mode
    void get_duty_cycle();
    void set_cp_state(const types::ev_board_support::EvCpState cp_state);

    // events for both modes EVSE and EV
    sigslot::signal<const types::board_support_common::Event&> on_cp_change;

    enum CPError {
        None,
        DiodeFault,
        PilotFault,
        CPShort,
    };
    sigslot::signal<const CPError&> on_cp_error;
    sigslot::signal<const types::board_support_common::Ampacity&> on_pp_change;
    sigslot::signal<const std::string&> on_pp_error;

    // events for EV mode
    sigslot::signal<const float&> on_duty_cycle_change;
    sigslot::signal<const std::string&> on_duty_cycle_error;

private:
    static constexpr unsigned char AN2295_ACK = 0xfc;
    static constexpr unsigned char AN2295_QUIT = 0x51; // 'Q' = QUIT
    bool termination_requested{false};
    int err{0};
    bool tx_enabled{false};
    gensios::Gensio* io{NULL};
    gensios::Waiter* waiter;
    std::chrono::milliseconds sleep_interval{200ms};
    std::thread query_thread;
    std::atomic_bool is_enabled{false};
    bool reinit{false};
    bool is_ev{false};
    bool use_pp{false};
    bool use_pp_pullup{false};
    double duty_cycle{100.0};
    bool pwm_ctrl_error{false};
    bool pwm_status{false};
    bool cp_error_raised{false};
    bool diode_error_raised{false};
    bool pp_error_raised{false};
    bool duty_cycle_error_raised{false};
    bool trace_pkts{false};
    USSFrame rx_frame;
    std::mutex cmd_mtx;
    std::mutex tx_mtx;
    std::condition_variable cv;
    KL02USSProtocol::Service expected_svc_response{KL02USSProtocol::Service::RESET};
    KL02USSProtocol::Service last_svc_response{KL02USSProtocol::Service::PBSC};
    types::board_support_common::Event previous_cp_state{types::board_support_common::Event::PowerOn};
    std::optional<types::board_support_common::Ampacity> previous_ampacity{};

    void tx_frame(USSFrame frame, const std::chrono::milliseconds timeout = {250ms});

    types::board_support_common::Event cp_voltage_to_cp_state(const float& v);
    void cp_voltages_to_cp_state(const KL02USSProtocol::CPVoltage& v);
    void pp_voltage_to_pp_state(const float& v);
    void filter_duty_cycle(const uint16_t dc);

    void setup_pp_pullup();
    void get_ucp();
    void get_pp_voltage();
    void get_duty_cycle_no_lock();

    void reset();

    gensios::gensiods read(int ierr, const gensios::SimpleUCharVector data, const char* const* auxdata) override;

    template <typename T> void dump_frame(const std::string msg, bool tx, const T& frame) const {
        std::ostringstream oss;
        std::string prefix = tx ? "[" : "<";
        std::string suffix = tx ? "]" : ">";

        for (long unsigned int idx = 0; idx < frame.size(); ++idx) {
            oss << prefix << std::hex << std::setfill('0') << std::setw(2) << static_cast<unsigned int>(frame[idx])
                << suffix << " ";
        }

        std::string f = oss.str();
        if (!f.empty())
            f.pop_back();

        if (this->trace_pkts)
            EVLOG_info << msg << f;
        else
            EVLOG_verbose << msg << f;
    }

    void write_ready() override {
    }

    void freed() override {
        this->waiter->wake();
    }
};
