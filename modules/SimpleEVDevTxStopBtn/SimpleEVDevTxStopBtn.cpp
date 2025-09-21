// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "SimpleEVDevTxStopBtn.hpp"
#include <cerrno>
#include <cstdlib>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <string>
#include <sys/poll.h>
#include <unistd.h>

namespace module {

void SimpleEVDevTxStopBtn::init() {
    int rv;

    // open the configured device
    this->poll_fd.fd = open(this->config.device.c_str(), O_RDONLY | O_NONBLOCK);

    if (this->poll_fd.fd < 0) {
        EVLOG_error << "Failed to open '" << this->config.device << "': " << std::strerror(errno);
        return;
    }

    rv = libevdev_new_from_fd(this->poll_fd.fd, &this->dev);
    if (rv < 0) {
        EVLOG_error << "Failed to init libevdev: " << strerror(-rv);
        return;
    }

    EVLOG_info << "Input device name: " << libevdev_get_name(this->dev);

    if (!libevdev_has_event_type(this->dev, EV_KEY)) {
        EVLOG_error << "Device does not support key events.";
        libevdev_free(this->dev);
        close(this->poll_fd.fd);
        this->poll_fd.fd = -1;
        return;
    }

    // translate key name into code
    this->event_code = libevdev_event_code_from_name(EV_KEY, this->config.key.c_str());
    if (this->event_code == -1) {
        EVLOG_error << "Key name '" << this->config.key << "' is not recognized.";
        libevdev_free(this->dev);
        close(this->poll_fd.fd);
        this->poll_fd.fd = -1;
        return;
    }

    invoke_init(*p_empty);
}

void SimpleEVDevTxStopBtn::ready() {
    invoke_ready(*p_empty);

    while (this->poll_fd.fd != -1) {
        int rv;

        rv = poll(&this->poll_fd, 1, -1);
        if (rv < 0) {
            EVLOG_error << "poll() failed: " << std::strerror(errno);
            break;
        }

        if (this->poll_fd.revents & POLLIN) {
            struct input_event ev;

            while ((rv = libevdev_next_event(this->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev)) == 0) {
                if (ev.type == EV_KEY && ev.value == 0 && ev.code == this->event_code) {
                    types::evse_manager::StopTransactionRequest req;
                    req.reason = types::evse_manager::StopTransactionReason::Local;
                    this->r_evse_manager->call_stop_transaction(req);
                }
            }

            if (rv != -EAGAIN && rv != LIBEVDEV_READ_STATUS_SYNC) {
                EVLOG_error << "libevdev_next_event() failed: " << std::strerror(-rv);
                break;
            }
        }
    }
}

} // namespace module
