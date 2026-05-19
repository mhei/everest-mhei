// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "MCU.hpp"
#include <fmt/core.h>
#include <sstream>

MCU::MCU(gensios::Waiter* w) : waiter(w) {

    this->query_thread = std::thread([this] {
        while (!this->termination_requested) {
            if (this->tx_enabled) {
                std::scoped_lock lock(this->cmd_mtx);

                if (this->reinit) {
                    // reset MCU state
                    this->reset();
                    this->echo_off();
                    this->auto_plug();

                    this->reinit = false;
                }

                this->get_voltages();
            }
            std::this_thread::sleep_for(this->sleep_interval);
        }
    });
}

MCU::~MCU() {
    this->termination_requested = true;

    // if thread is active wait until it is terminated
    if (this->query_thread.joinable())
        this->query_thread.join();
}

void MCU::set_gensio(gensios::Gensio* g) {
    this->io = g;
    this->reinit = true;
}

int MCU::get_err() {
    return this->err;
}

void MCU::enable_tx() {
    this->tx_enabled = true;
}

void MCU::disable_tx() {
    this->tx_enabled = false;
}

std::string MCU::execute_command(const std::string cmd, const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(this->rx_mtx);
    std::string cmdline = cmd + "\r\n";

    try {
        auto c = this->io->write(cmdline.data(), cmdline.size(), NULL);

        if (c != cmdline.size()) {
            EVLOG_warning << "could not sent complete command - only " << c << " of " << std::to_string(cmdline.size())
                          << " bytes (" << cmd << ")";
            return "";
        }

        EVLOG_verbose << "sent: " << cmd;

    } catch (gensios::gensio_error& e) {
        // error could happen in case gensio is already gone
        EVLOG_debug << e.what();
        return "";
    }

    // wait until we received the response to this request, or timeout
    if (not this->cv.wait_for(lock, timeout, [this] { return this->cmd_response.size() > 0; })) {
        throw std::runtime_error("MCU didn't respond to command: " + cmd);
    }

    std::string rsp = this->cmd_response;

    EVLOG_verbose << "recv: " << rsp;

    this->cmd_response.clear();

    if (rsp.compare(0, 3, "ERR") == 0) {
        throw std::runtime_error("MCU failed to execute \"" + cmd + "\": " + rsp.substr(4));
    }

    return rsp;
}

void MCU::enable() {
    this->is_enabled.exchange(true);
}

void MCU::disable() {
    if (this->is_enabled.exchange(false)) {
        this->previous_cp_state = types::board_support_common::Event::Disconnected;
        this->previous_ampacity.reset();
    }
}

double MCU::get_cached_duty_cycle() const {
    return this->duty_cycle;
}

void MCU::get_voltages() {
    auto rsp = this->execute_command("get_voltages");

    // Expected format:
    // "OK ID:5.0 V, CE:0.03 V"

    float id_voltage, ce_voltage;

    size_t id_pos = rsp.find("ID:");
    size_t ce_pos = rsp.find("CE:");

    if (id_pos == std::string::npos || ce_pos == std::string::npos)
        return;

    try {
        // extract ID float
        id_pos += 3; // skip "ID:"
        size_t id_end = rsp.find(" V", id_pos);
        id_voltage = std::stof(rsp.substr(id_pos, id_end - id_pos));

        // extract CE float
        ce_pos += 3; // skip "CE:"
        size_t ce_end = rsp.find(" V", ce_pos);
        ce_voltage = std::stof(rsp.substr(ce_pos, ce_end - ce_pos));
    } catch (...) {
        return;
    }

    if (this->is_enabled) {
        this->id_voltage_to_pp_state(id_voltage);
        this->ce_voltage_to_cp_state(ce_voltage);
    }
}

void MCU::reset() {
    this->execute_command("reset");
}

void MCU::echo_off() {
    this->execute_command("echo off");
}

void MCU::auto_plug() {
    this->execute_command("auto_plug on");
}

void MCU::set_cp_state(const types::ev_board_support::EvCpState cp_state) {
    std::scoped_lock lock(this->cmd_mtx);

    this->execute_command("set_ce_state " + types::ev_board_support::ev_cp_state_to_string(cp_state));
}

// we use PowerOn to signal out-of-range and D for B0
types::board_support_common::Event MCU::ce_voltage_to_bsp_event(const float& v) const {
    if (3.38f <= v and v <= 4.04f)
        return types::board_support_common::Event::B;

    if (2.38f <= v and v <= 3.16f)
        return types::board_support_common::Event::D;

    if (1.02f <= v and v <= 1.4f)
        return types::board_support_common::Event::C;

    if (0.53f <= v and v <= 0.74f)
        return types::board_support_common::Event::F;

    if (-0.2f <= v and v <= 0.2f) {
        if (this->previous_ampacity != types::board_support_common::Ampacity::None) {
            return types::board_support_common::Event::A;
        } else {
            return types::board_support_common::Event::Disconnected;
        }
    }
    return types::board_support_common::Event::A;

    return types::board_support_common::Event::PowerOn;
}

void MCU::ce_voltage_to_cp_state(const float& v) {
    CPError cp_error{None};
    auto previous_dc = this->duty_cycle;

    EVLOG_verbose << "New CE voltage: " << std::fixed << std::setprecision(1) << v << " V";

    types::board_support_common::Event new_cp_state = this->ce_voltage_to_bsp_event(v);

    // set simulated duty cycle and fixup B0
    switch (new_cp_state) {
    case types::board_support_common::Event::A:
    case types::board_support_common::Event::E:
        // don't change simulated duty cycle here
        break;
    case types::board_support_common::Event::B:
        this->duty_cycle = 5.0;
        break;
    case types::board_support_common::Event::C:
        this->duty_cycle = 5.0;
        break;
    case types::board_support_common::Event::D:
        new_cp_state = types::board_support_common::Event::B;
        this->duty_cycle = 100.0;
        break;
    case types::board_support_common::Event::F:
        this->duty_cycle = 0.0;
        break;
    case types::board_support_common::Event::Disconnected:
        this->duty_cycle = 100.0;
        break;
    case types::board_support_common::Event::PowerOn:
        this->duty_cycle = 100.0;
        cp_error = PilotFault;
        break;
    // cannot happen - just to avoid compiler warning
    case types::board_support_common::Event::PowerOff:
        break;
    }

    EVLOG_verbose << "New CP state: " << new_cp_state;

    if (cp_error != None) {
        this->previous_cp_state = new_cp_state;
        if (!this->cp_error_raised) {
            this->on_cp_error(cp_error);
            this->cp_error_raised = true;
        }
        return;
    } else {
        if (this->cp_error_raised) {
            this->on_cp_error(cp_error);
            this->cp_error_raised = false;
        }
    }

    if (this->previous_cp_state != new_cp_state) {
        // update previous cp state early since new_cp_state is reference and can
        // be changed by callee
        this->previous_cp_state = new_cp_state;
        this->on_cp_change(new_cp_state);
    }

    if (this->duty_cycle != previous_dc) {
        this->on_duty_cycle_change(this->duty_cycle);
    }
}

void MCU::id_voltage_to_pp_state(const float& v) {
    bool pp_error{false};
    types::board_support_common::Ampacity new_ampacity{types::board_support_common::Ampacity::None};

    // inlet not present
#if 0
    // commented due to hw design issue in v1
    if (4.5 < v and v < 5.5)
        pp_error = true;
#endif
    // Mated_EVSE
    if (0.74 < v and v < 1.37)
        new_ampacity = types::board_support_common::Ampacity::A_13;

    if (pp_error) {
        this->previous_ampacity.reset();
        if (!this->pp_error_raised) {
            this->on_pp_error("Measured ID voltage in invalid range (inlet present?)");
            this->pp_error_raised = true;
        }
        return;
    }

    if (not this->previous_ampacity.has_value() or this->previous_ampacity.value() != new_ampacity) {
        this->on_pp_change(new_ampacity);
        this->previous_ampacity = new_ampacity;
        this->pp_error_raised = false;
    }
}

gensios::gensiods MCU::read(int ierr, const gensios::SimpleUCharVector data, const char* const* auxdata) {
    (void)auxdata;

    if (ierr) {
        this->err = ierr;
        this->io->set_read_callback_enable(false);
        this->io->set_write_callback_enable(false);
        this->waiter->wake();
        return 0;
    }

    std::string rx(reinterpret_cast<const char*>(data.data()), data.size());

    EVLOG_verbose << "received: " << rx;

    this->rx_line += rx;

    size_t lf_pos = this->rx_line.find("\n");

    while (lf_pos != std::string::npos) {
        std::lock_guard<std::mutex> lock(this->rx_mtx);

        this->cmd_response = this->rx_line.substr(0, lf_pos);
        if (!this->cmd_response.empty() && this->cmd_response.back() == '\r') {
            this->cmd_response.pop_back();
        }

        this->rx_line.erase(0, lf_pos + 1);

        this->cv.notify_one();

        lf_pos = this->rx_line.find("\n");
    }

    // consumed data
    return data.size();
}
