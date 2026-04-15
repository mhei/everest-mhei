// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "KL02.hpp"
#include <KL02USSProtocol.hpp>
#include <fmt/core.h>

KL02::KL02(gensios::Waiter* w, bool is_ev, bool use_pp, bool use_pp_pullup) :
    waiter(w), is_ev(is_ev), use_pp(use_pp), use_pp_pullup(use_pp_pullup) {

    this->query_thread = std::thread([this] {
        while (!this->termination_requested) {
            if (this->tx_enabled) {
                std::scoped_lock lock(this->cmd_mtx);

                if (this->reinit) {
                    // reset KL02
                    this->reset();

                    // configure PP pullup if required
                    this->setup_pp_pullup();

                    this->reinit = false;
                }

                // query PP state if in use
                if (this->use_pp) {
                    this->get_pp_voltage();
                }

                // get CP state - for both EVSE and EV roles
                this->get_ucp();

                // determine duty cycle for EV role
                if (this->is_ev) {
                    this->get_duty_cycle_no_lock();
                }
            }
            std::this_thread::sleep_for(this->sleep_interval);
        }
    });
}

KL02::~KL02() {
    this->termination_requested = true;

    // if thread is active wait until it is terminated
    if (this->query_thread.joinable())
        this->query_thread.join();
}

void KL02::set_gensio(gensios::Gensio* g) {
    this->io = g;
    this->reinit = true;
}

int KL02::get_err() {
    return this->err;
}

void KL02::enable_tx() {
    this->tx_enabled = true;
}

void KL02::disable_tx() {
    this->tx_enabled = false;
}

void KL02::trace(bool on) {
    this->trace_pkts = on;
}

void KL02::tx_frame(USSFrame frame, const std::chrono::milliseconds timeout) {
    std::unique_lock<std::mutex> lock(this->tx_mtx);

    try {
        auto c = this->io->write(frame, NULL);

        if (c != frame.size()) {
            EVLOG_warning << "could not sent complete request frame - only " << c << " of "
                          << std::to_string(frame.size()) << " bytes";
            return;
        }

        this->dump_frame("sent: ", true, frame);

    } catch (gensios::gensio_error& e) {
        // error could happen in case gensio is already gone
        EVLOG_debug << e.what();
        return;
    }

    this->expected_svc_response = KL02USSProtocol::svc_response_to_service(frame.get_service());

    // wait until we received the response to this request,or timeout
    if (not this->cv.wait_for(lock, timeout,
                              [this] { return this->last_svc_response == this->expected_svc_response; })) {
        throw std::runtime_error("KL02 did not respond to " +
                                 KL02USSProtocol::service_to_string(this->expected_svc_response) + " request");
    }

    EVLOG_verbose << "Got response for " << KL02USSProtocol::service_to_string(this->expected_svc_response)
                  << " request";
}

void KL02::enable() {
    this->is_enabled.exchange(true);
}

void KL02::disable() {
    if (this->is_enabled.exchange(false)) {
        this->previous_cp_state = types::board_support_common::Event::Disconnected;
        this->previous_ampacity.reset();
    }
}

bool KL02::pwm_is_enabled() {
    std::scoped_lock lock(this->cmd_mtx);

    this->tx_frame(KL02USSProtocol::control_pwm(KL02USSProtocol::PWMControl::PWM_CHECK));
    return this->pwm_status;
}

void KL02::pwm_enable() {
    std::scoped_lock lock(this->cmd_mtx);

    this->tx_frame(KL02USSProtocol::control_pwm(KL02USSProtocol::PWMControl::PWM_ENABLE));
    if (!this->pwm_status) {
        throw std::runtime_error("KL02 could not enable PWM");
    }
}

void KL02::pwm_disable() {
    std::scoped_lock lock(this->cmd_mtx);

    this->tx_frame(KL02USSProtocol::control_pwm(KL02USSProtocol::PWMControl::PWM_DISABLE));
    if (this->pwm_status) {
        throw std::runtime_error("KL02 could not disable PWM");
    }
}

void KL02::set_duty_cycle(const double duty_cycle) {
    std::scoped_lock lock(this->cmd_mtx);

    // setting the duty cycle only works if PWM is enabled, so enable it if not yet done
    if (!this->pwm_status) {
        this->tx_frame(KL02USSProtocol::control_pwm(KL02USSProtocol::PWMControl::PWM_ENABLE));
        if (!this->pwm_status) {
            throw std::runtime_error("KL02 could not enable PWM");
        }
    }

    this->tx_frame(KL02USSProtocol::set_pwm(duty_cycle));
    this->duty_cycle = duty_cycle;
}

void KL02::get_duty_cycle() {
    std::scoped_lock lock(this->cmd_mtx);

    this->get_duty_cycle_no_lock();
}

void KL02::get_duty_cycle_no_lock() {
    this->tx_frame(KL02USSProtocol::get_pwm());
}

double KL02::get_cached_duty_cycle() const {
    return this->duty_cycle;
}

void KL02::setup_pp_pullup() {
    if (this->use_pp) {
        this->tx_frame(KL02USSProtocol::enable_pp_pullup(this->use_pp_pullup));
        // ignore return value, assume it worked
    }
}

void KL02::get_ucp() {
    this->tx_frame(KL02USSProtocol::get_ucp());
}

void KL02::get_pp_voltage() {
    this->tx_frame(KL02USSProtocol::get_pp_voltage());
}

void KL02::reset() {
    this->tx_frame(KL02USSProtocol::reset(), 10000ms);
}

void KL02::set_cp_state(const types::ev_board_support::EvCpState cp_state) {
    std::scoped_lock lock(this->cmd_mtx);

    uint8_t value;

    switch (cp_state) {
    case types::ev_board_support::EvCpState::A:
        value = 0;
        break;
    case types::ev_board_support::EvCpState::B:
        value = KL02USSProtocol::CP_RESISTOR_2K7;
        break;
    case types::ev_board_support::EvCpState::C:
        value = KL02USSProtocol::CP_RESISTOR_2K7 | KL02USSProtocol::CP_RESISTOR_1K3;
        break;
    case types::ev_board_support::EvCpState::D:
        value = KL02USSProtocol::CP_RESISTOR_2K7 | KL02USSProtocol::CP_RESISTOR_270;
        break;
    case types::ev_board_support::EvCpState::E:
        value = 0;
        break;
    default:
        // not possible but ignore
        value = 0;
    }

    this->tx_frame(KL02USSProtocol::set_ucp(value));
    // we don't check the result but expect that our desired resistors are now active
}

// we use Disconnected to signal out-of-range
types::board_support_common::Event KL02::cp_voltage_to_cp_state(const float& v) {
    if (v > 13.0)
        return types::board_support_common::Event::Disconnected;

    if (v >= 11.0)
        return types::board_support_common::Event::A;

    if (v >= 10.0)
        return types::board_support_common::Event::Disconnected;

    if (v >= 8.0)
        return types::board_support_common::Event::B;

    if (v >= 7.0)
        return types::board_support_common::Event::Disconnected;

    if (v >= 5.0)
        return types::board_support_common::Event::C;

    if (v >= 4.0)
        return types::board_support_common::Event::Disconnected;

    if (v >= 2.0)
        return types::board_support_common::Event::D;

    if (v >= 1.5)
        return types::board_support_common::Event::Disconnected;

    if (v >= -1.5)
        return types::board_support_common::Event::E;

    if (v >= -11.0)
        return types::board_support_common::Event::Disconnected;

    if (v >= -13.0)
        return types::board_support_common::Event::F;

    return types::board_support_common::Event::Disconnected;
}

void KL02::cp_voltages_to_cp_state(const KL02USSProtocol::CPVoltage& v) {
    CPError cp_error{None};

    EVLOG_verbose << "New CP voltages: " << std::fixed << std::setprecision(1) << v.positive << " V, " << std::fixed
                  << std::setprecision(1) << v.negative << " V";

    auto pos = cp_voltage_to_cp_state(v.positive);
    auto neg = cp_voltage_to_cp_state(v.negative);

    types::board_support_common::Event new_cp_state = pos;

    if (pos == types::board_support_common::Event::E and neg == types::board_support_common::Event::F) {
        new_cp_state = types::board_support_common::Event::F;
    }

    if (!this->pwm_status) {
        new_cp_state = types::board_support_common::Event::E;
    }

    EVLOG_verbose << "New CP state: " << new_cp_state;

    if (pos == types::board_support_common::Event::Disconnected and
        neg == types::board_support_common::Event::Disconnected) {
        new_cp_state = types::board_support_common::Event::Disconnected;
        cp_error = PilotFault;
    }

    if (types::board_support_common::Event::B <= new_cp_state and
        new_cp_state <= types::board_support_common::Event::D and
        neg == types::board_support_common::Event::Disconnected) {
        cp_error = DiodeFault;
    }

    if (this->pwm_status and pos == types::board_support_common::Event::E and
        neg == types::board_support_common::Event::E) {
        cp_error = CPShort;
    }

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
        this->on_cp_change(new_cp_state);
        this->previous_cp_state = new_cp_state;
    }
}

void KL02::pp_voltage_to_pp_state(const float& v) {
    bool pp_error{false};
    types::board_support_common::Ampacity new_ampacity{types::board_support_common::Ampacity::None};

    if (3.8 < v and v < 4.5)
        new_ampacity = types::board_support_common::Ampacity::A_13;
    if (2.5 < v and v < 3.8)
        new_ampacity = types::board_support_common::Ampacity::A_20;
    if (1.6 < v and v < 2.5)
        new_ampacity = types::board_support_common::Ampacity::A_32;
    if (0.9 < v and v < 1.6)
        new_ampacity = types::board_support_common::Ampacity::A_63_3ph_70_1ph;
    if (v < 0.9)
        pp_error = true;

    if (pp_error) {
        this->previous_ampacity.reset();
        if (!this->pp_error_raised) {
            this->on_pp_error("Measured PP voltage in invalid range");
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

void KL02::filter_duty_cycle(const uint16_t dc) {
    double new_dc = dc / 10.0;
    bool dc_error{false};

    if (new_dc < 3.0)
        new_dc = 0.0;
    if (3.0 <= new_dc and new_dc <= 7.0)
        new_dc = 5.0;
    if (7.0 < new_dc and new_dc < 8.0)
        dc_error = true;
    if (8.0 <= new_dc and new_dc < 10.0)
        new_dc = 10.0;
    if (96.0 < new_dc and new_dc <= 97.0)
        new_dc = 96.0;
    if (97.0 < new_dc and new_dc <= 99.0)
        dc_error = true;
    if (99.0 < new_dc)
        new_dc = 100.0;

    if (dc_error) {
        this->duty_cycle = new_dc;
        if (!this->duty_cycle_error_raised) {
            this->on_duty_cycle_error("Measured Duty Cycle in invalid range");
            this->duty_cycle_error_raised = true;
        }
        return;
    }

    if (this->duty_cycle != new_dc) {
        this->on_duty_cycle_change(new_dc);
        this->duty_cycle = new_dc;
        this->duty_cycle_error_raised = false;
    }
}

gensios::gensiods KL02::read(int ierr, const gensios::SimpleUCharVector data, const char* const* auxdata) {
    (void)auxdata;

    if (ierr) {
        this->err = ierr;
        this->io->set_read_callback_enable(false);
        this->io->set_write_callback_enable(false);
        this->waiter->wake();
        return 0;
    }

    this->dump_frame("received: ", false, data);

    // after reset, there is a bootloader running:
    // https://www.nxp.com/docs/en/application-note/AN2295.pdf
    // let's ACK and QUIT to start the user application faster
    if (this->reinit and (data.size() == 1) and (data[0] == AN2295_ACK)) {
        std::vector<unsigned char> quit_frame;
        quit_frame.push_back(AN2295_ACK);
        quit_frame.push_back(AN2295_QUIT);
        this->io->write(quit_frame, NULL);
        return data.size();
    }

    this->rx_frame.push_back(data.data(), data.size());

    if (this->rx_frame.is_valid()) {
        if (KL02USSProtocol::is_svc_response_frame(this->rx_frame)) {
            auto svc = KL02USSProtocol::svc_response_to_service(this->rx_frame.get_service());

            switch (svc) {
            case KL02USSProtocol::GET_PWM: {
                auto pwm_values = KL02USSProtocol::parse_get_pwm(this->rx_frame);
                if (this->is_enabled) {
                    this->filter_duty_cycle(pwm_values.duty_cycle);
                }
            } break;
            case KL02USSProtocol::SET_PWM:
                this->pwm_ctrl_error = KL02USSProtocol::parse_set_pwm(this->rx_frame);
                break;
            case KL02USSProtocol::CONTROL_PWM:
                this->pwm_status = KL02USSProtocol::parse_control_pwm(this->rx_frame);
                break;
            case KL02USSProtocol::GET_UCP: {
                auto v = KL02USSProtocol::parse_get_ucp(this->rx_frame);
                if (this->is_enabled) {
                    this->cp_voltages_to_cp_state(v);
                }
            } break;
            case KL02USSProtocol::SET_UCP:
                KL02USSProtocol::parse_set_ucp(this->rx_frame);
                break;
            case KL02USSProtocol::RESET:
                KL02USSProtocol::parse_por(this->rx_frame);
                break;
            case KL02USSProtocol::PP_RESISTOR:
                KL02USSProtocol::parse_set_pp_resistor(this->rx_frame);
                break;
            case KL02USSProtocol::PP_PULLUP:
                KL02USSProtocol::parse_enable_pp_pullup(this->rx_frame);
                break;
            case KL02USSProtocol::PP_VOLTAGE: {
                auto v = KL02USSProtocol::parse_get_pp_voltage(this->rx_frame);
                if (this->is_enabled) {
                    this->pp_voltage_to_pp_state(v);
                }
            } break;
            default:
                EVLOG_warning << fmt::format("unexpected service response {:#04x} received",
                                             static_cast<unsigned int>(svc));
            }

            {
                std::lock_guard<std::mutex> lock(this->tx_mtx);
                this->last_svc_response = svc;
            }
            this->cv.notify_one();
        }
        this->rx_frame.clear();
    }

    // consumed data
    return data.size();
}
