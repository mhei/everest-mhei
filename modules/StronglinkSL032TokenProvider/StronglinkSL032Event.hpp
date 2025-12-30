// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include "StronglinkSL032.hpp"
#include <chrono>
#include <everest/logging.hpp>
#include <gensio/gensio>
#include <iomanip>
#include <iostream>
#include <sigslot/signal.hpp>
#include <string>
#include <thread>

using namespace std::chrono_literals;

class StronglinkSL032Event : public gensios::Event {
public:
    StronglinkSL032Event(gensios::Waiter* w, int interval, bool ignore_echo);

    ~StronglinkSL032Event() {
        this->termination_requested = true;

        // if thread is active wait until it is terminated
        if (this->query_thread.joinable())
            this->query_thread.join();
    }

    void set_gensio(gensios::Gensio* g) {
        this->io = g;
    }

    void enable_tx();
    void disable_tx();

    int get_err() {
        return this->err;
    }

    sigslot::signal<const std::string&> on_token;

private:
    bool termination_requested{false};
    bool ignore_echo{false};
    std::atomic<unsigned int> still_to_discard{0};
    int err{0};
    bool tx_enabled{false};
    gensios::Gensio* io{NULL};
    gensios::Waiter* waiter;
    std::chrono::milliseconds sleep_interval;
    std::thread query_thread;
    StronglinkSL032::RxFrame rx_frame;
    std::string reported_tag;

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

        EVLOG_verbose << msg << f;
    }

    void write_ready() override {
    }

    void freed() override {
        this->waiter->wake();
    }
};
