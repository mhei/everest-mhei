// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "StronglinkSL032.hpp"
#include <algorithm>
#include <cstring>
#include <everest/logging.hpp>
#include <fmt/core.h>

namespace StronglinkSL032 {

std::string CardType_to_string(CardType type) {
    switch (type) {
    case CardType::Other:
        return "Other";
    case CardType::MFMini_4B:
        return "MFMini_4B";
    case CardType::MFMini_7B:
        return "MFMini_7B";
    case CardType::MF1K_4B__MFPLUS2K_SL1_4B:
        return "MF1K_4B__MFPLUS2K_SL1_4B";
    case CardType::MF1K_7B__MFPLUS2K_SL1_7B:
        return "MF1K_7B__MFPLUS2K_SL1_7B";
    case CardType::MF4K_4B__MFPLUS4K_SL1_4B:
        return "MF4K_4B__MFPLUS4K_SL1_4B";
    case CardType::MF4K_7B__MFPLUS4K_SL1_7B:
        return "MF4K_7B__MFPLUS4K_SL1_7B";
    case CardType::MF_Ultralight__Ultralight_C__NTag203:
        return "MF_Ultralight__Ultralight_C__NTag203";
    case CardType::MF_DESFire__MF_DESFire_EV1:
        return "MF_DESFire__MF_DESFire_EV1";
    case CardType::MF_PROX:
        return "MF_PROX";
    case CardType::MFPLUS2K_SL2_4B:
        return "MFPLUS2K_SL2_4B";
    case CardType::MFPLUS4K_SL2_4B:
        return "MFPLUS4K_SL2_4B";
    case CardType::MFPLUS2K_SL2_7B:
        return "MFPLUS2K_SL2_7B";
    case CardType::MFPLUS4K_SL2_7B:
        return "MFPLUS4K_SL2_7B";
    case CardType::MFPLUS2K_SL0_SL3_4B:
        return "MFPLUS2K_SL0_SL3_4B";
    case CardType::MFPLUS4K_SL0_SL3_4B:
        return "MFPLUS4K_SL0_SL3_4B";
    case CardType::MFPLUS2K_SL0_SL3_7B:
        return "MFPLUS2K_SL0_SL3_7B";
    case CardType::MFPLUS4K_SL0_SL3_7B:
        return "MFPLUS4K_SL0_SL3_7B";
    default:
        return "Unknown Type";
    }
}

std::ostream& operator<<(std::ostream& os, const CardType type) {
    os << CardType_to_string(type);
    return os;
}

unsigned char calc_checksum(const unsigned char* p, const std::size_t l) {
    unsigned char checksum;

    if (l == 0)
        return 0;

    checksum = p[0];

    for (std::size_t i = 1; i < l; ++i)
        checksum ^= p[i];

    return checksum;
}

TxFrame::TxFrame(const Command cmd, const std::vector<unsigned char>& payload) {
    this->frame.reserve(4 + payload.size());

    this->frame.push_back(static_cast<unsigned char>(Preamble::Request));
    this->frame.push_back(2 + payload.size());
    this->frame.push_back(static_cast<unsigned char>(cmd));

    this->frame.insert(this->frame.end(), payload.begin(), payload.end());

    this->frame.push_back(calc_checksum(this->frame.data(), this->frame.size()));
}

RxFrame::RxFrame(const unsigned char* p, const std::size_t l) {
    this->push_back(p, l);
}

void RxFrame::push_back(const unsigned char* p, const std::size_t l) {
    if (p && l) {
        memcpy(&this->buffer[this->buffer_length], p, l);
        this->buffer_length += l;
    }

    // if frame is still invalid, here most probably incomplete, let's run the check
    if (!this->valid) {
        this->check_frame();
    }
}

void RxFrame::check_frame() {
    // a response frame is at least 5 bytes long
    if (this->buffer_length < MinFrameLength)
        return;

    // we don't need to check all start positions in the buffer:
    // - due to the minimum frame length we can stop this count before the current buffer length
    // - when reaching <MaxFrameLength> index, then in the first <MaxFrameLength> bytes is probably
    //   garbage, so we can discard this and move trailing data to buffer start
    std::size_t limit_idx = std::min(this->buffer_length - MinFrameLength + 1, MaxFrameLength + 1);
    std::size_t idx;
    this->frame = nullptr;
    this->tail = nullptr;

    for (idx = 0; idx < limit_idx; ++idx) {
        // first byte should match expected preamble for a response
        if (static_cast<Preamble>(this->buffer[idx]) != Preamble::Response) {
            EVLOG_verbose << fmt::format("skipped at idx={}, {:#04x} is not {:#04x}", idx,
                                         static_cast<unsigned int>(this->buffer[idx]),
                                         static_cast<unsigned int>(Preamble::Response));
            continue;
        }

        // we found one, let's assume good case
        this->frame = &this->buffer[idx];

        // responses have always a command, a status and the checksum field
        // so the length field should have a value of 3 or more
        if (this->frame[OFFSET_LEN] < 3) {
            EVLOG_verbose << "claimed frame length < 3";
            continue;
        }

        // total frame length as claimed by byte stream (note: might be wrong!)
        std::size_t frame_total_length = this->frame[OFFSET_LEN] + 2;

        // bogus length in byte stream or not yet all data received
        if (frame_total_length > this->buffer_length - idx) {
            EVLOG_verbose << "frame_total_length=" << frame_total_length
                          << ", buffer_length - idx=" << (this->buffer_length - idx);
            continue;
        }

        // validate the checksum
        unsigned char claimed_checksum = this->frame[frame_total_length - 1];
        unsigned char calculated_checksum = calc_checksum(this->frame, frame_total_length - 1);

        if (claimed_checksum != calculated_checksum) {
            EVLOG_verbose << fmt::format("CRC mismatch: expected {:#04x} but received {:#04x}", calculated_checksum,
                                         claimed_checksum);
            continue;
        }

        // if we get here, then it looks like a valid frame
        this->valid = true;
        this->tail = &this->frame[frame_total_length];
        break;
    }

    // in case all found preamble bytes do not look like a valid frame start
    // and we hit the MaxFrameLength limit, then let's discard the garbage;
    // the frame pointer is still pointing to the last found preamble byte (if any)
    if (this->frame and idx == MaxFrameLength + 1) {
        std::size_t discarded = this->frame - this->buffer;
        this->buffer_length -= discarded;
        memmove(this->buffer, this->frame, this->buffer_length);
        EVLOG_verbose << "discarded " << discarded << " bytes";
    }
}

void RxFrame::clear() {
    this->valid = false;
    if (this->tail) {
        std::size_t discarded = this->tail - this->buffer;
        this->buffer_length -= discarded;
        memmove(this->buffer, this->tail, this->buffer_length);
        EVLOG_verbose << "cleared " << discarded << " bytes";
    } else {
        // this case should not happen since clear() should only be called on valid frames
        // and in this case tail should be set
        this->buffer_length = 0;
    }
}

Command RxFrame::get_cmd() {
    return static_cast<Command>(this->frame[OFFSET_CMD]);
}

Status RxFrame::get_status() {
    return static_cast<Status>(this->frame[OFFSET_STATUS]);
}

unsigned char* RxFrame::get_data() {
    return &this->frame[OFFSET_DATA];
}

std::size_t RxFrame::get_data_length() {
    // length - 1 byte for command - 1 byte for status - 1 byte for checksum
    return this->frame[OFFSET_LEN] - 3;
}

std::pair<std::string, std::string> RxFrame::get_card_info() {
    auto data = this->get_data();
    auto length = this->get_data_length() - 1;
    std::ostringstream oss;

    for (std::size_t i = 0; i < length; ++i) {
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(data[i]);
    }

    return {oss.str(), CardType_to_string(static_cast<CardType>(data[length]))};
}

unsigned char* RxFrame::get_tail() {
    return this->tail;
}

std::size_t RxFrame::get_tail_length() {
    return &this->buffer[buffer_length] - this->tail;
}

} // namespace StronglinkSL032
