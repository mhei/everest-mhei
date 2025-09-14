// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#include "auth_token_providerImpl.hpp"
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <everest/helpers/helpers.hpp>
#include <fcntl.h>
#include <iostream>
#include <libevdev/libevdev.h>
#include <linux/input-event-codes.h>
#include <string>
#include <sys/poll.h>
#include <unistd.h>
#include <unordered_map>

namespace module {
namespace main {

void auth_token_providerImpl::init() {
    int rv;

    // open the configured device
    this->poll_fd.fd = open(this->mod->config.device.c_str(), O_RDONLY | O_NONBLOCK);

    if (this->poll_fd.fd < 0) {
        EVLOG_error << "Failed to open '" << this->mod->config.device << "': " << std::strerror(errno);
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
}

void auth_token_providerImpl::ready() {
    std::string id_token;

    while (this->poll_fd.fd != -1) {
        int rv;

        rv = poll(&this->poll_fd, 1, -1);
        if (rv < 0) {
            this->raise_error(std::string("poll() failed: ") + std::strerror(errno));
            break;
        }

        if (this->poll_fd.revents & POLLIN) {
            struct input_event ev;

            while ((rv = libevdev_next_event(this->dev, LIBEVDEV_READ_FLAG_NORMAL, &ev)) == 0) {
                if (ev.type == EV_KEY && ev.value == 0) {
                    if (ev.code == KEY_ENTER) {
                        types::authorization::ProvidedIdToken token;

                        // guess the type according to the string length:
                        // ISO14443 UIDs are either 4 bytes, 7 bytes or 10 bytes
                        // ISO15693 UIDs are only 8 bytes
                        switch (id_token.length()) {
                        case 10:
                            // the reader I own has either firmware bug (for 4 bytes cards it
                            // reports 10 chars with leading zeros instead of expected 8 chars),
                            // or I'm missing some point; let's trim it away
                            id_token.erase(0, 2);
                            [[fallthrough]];
                        case 8:
                            [[fallthrough]];
                        case 14:
                            [[fallthrough]];
                        case 20:
                            token.id_token.type = types::authorization::IdTokenType::ISO14443;
                            break;
                        case 16:
                            token.id_token.type = types::authorization::IdTokenType::ISO15693;
                            break;
                        default:
                            token.id_token.type = types::authorization::IdTokenType::Local;
                        }

                        token.id_token.value = id_token;
                        token.authorization_type = types::authorization::AuthorizationType::RFID;

                        if (this->mod->config.debug) {
                            EVLOG_info << "Publishing new RFID/NFC token: " << everest::helpers::redact(token);
                        } else {
                            EVLOG_debug << "Publishing new RFID/NFC token: " << everest::helpers::redact(token);
                        }

                        this->publish_provided_token(token);

                        id_token.clear();
                    } else {
                        char c = this->map_key_code_to_char(ev.code);

                        if (c) {
                            id_token += c;
                        }
                    }
                }
            }

            if (rv != -EAGAIN && rv != LIBEVDEV_READ_STATUS_SYNC) {
                this->raise_error(std::string("libevdev_next_event() failed: ") + std::strerror(-rv));
                break;
            }
        }
    }
}

char auth_token_providerImpl::map_key_code_to_char(int keycode) {
    // clang-format off
    static const std::unordered_map<int, char> key_map = {
        {KEY_A, 'a'}, {KEY_B, 'b'}, {KEY_C, 'c'}, {KEY_D, 'd'}, {KEY_E, 'e'}, {KEY_F, 'f'},
        {KEY_G, 'g'}, {KEY_H, 'h'}, {KEY_I, 'i'}, {KEY_J, 'j'}, {KEY_K, 'k'}, {KEY_L, 'l'},
        {KEY_M, 'm'}, {KEY_N, 'n'}, {KEY_O, 'o'}, {KEY_P, 'p'}, {KEY_Q, 'q'}, {KEY_R, 'r'},
        {KEY_S, 's'}, {KEY_T, 't'}, {KEY_U, 'u'}, {KEY_V, 'v'}, {KEY_W, 'w'}, {KEY_X, 'x'},
        {KEY_Y, 'y'}, {KEY_Z, 'z'},

        {KEY_1, '1'}, {KEY_2, '2'}, {KEY_3, '3'}, {KEY_4, '4'}, {KEY_5, '5'},
        {KEY_6, '6'}, {KEY_7, '7'}, {KEY_8, '8'}, {KEY_9, '9'}, {KEY_0, '0'},
    };
    // clang-format on

    auto it = key_map.find(keycode);
    return (it != key_map.end()) ? it->second : '\0';
}

void auth_token_providerImpl::raise_error(const std::string error_message) {
    auto error = this->error_factory->create_error("generic/CommunicationFault", "Communication lost", error_message);
    EVLOG_error << error_message;
    this->raise_error(error);
}

} // namespace main
} // namespace module
