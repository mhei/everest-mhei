// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "USSFrame.hpp"

unsigned char USSFrame::calc_checksum(const unsigned char* p, const std::size_t l) {
    if (l == 0)
        return 0;

    unsigned char bcc = p[0];

    for (std::size_t i = 1; i < l; ++i)
        bcc ^= p[i];

    return bcc;
}

USSFrame::USSFrame() : valid(false) {
    this->frame.reserve(MAX_FRAME_LENGTH);
}

USSFrame::USSFrame(unsigned char address, unsigned char service, const std::vector<unsigned char>& data) {
    this->frame.reserve(MIN_FRAME_LENGTH + data.size());
    this->frame.clear();

    this->frame.push_back(STX);
    this->frame.push_back(3 + data.size());
    this->frame.push_back(address);
    this->frame.push_back(service);
    this->frame.insert(frame.end(), data.begin(), data.end());
    this->frame.push_back(this->calc_checksum(this->frame.data(), this->frame.size()));

    this->valid = true;
}

USSFrame::USSFrame(const unsigned char* p, const std::size_t l) : valid(false) {
    this->push_back(p, l);
}

void USSFrame::push_back(const unsigned char* p, const std::size_t l) {
    if (p && l) {
        this->frame.insert(this->frame.end(), p, p + l);
    }

    // if frame is still invalid, here most probably incomplete, let's run the check
    if (!this->valid) {
        this->check_frame();
    }
}

void USSFrame::check_frame() {
    // a frame is at least 5 bytes long
    if (this->frame.size() < MIN_FRAME_LENGTH)
        return;

    // we don't need to check all start positions in the buffer:
    // - due to the minimum frame length we can stop this count before the current buffer length
    // - when reaching <MAX_FRAME_LENGTH> index, then in the first <MAX_FRAME_LENGTH> bytes is probably
    //   garbage, so we can discard this and move trailing data to buffer start
    std::size_t limit_idx = std::min(this->frame.size() - MIN_FRAME_LENGTH + 1, MAX_FRAME_LENGTH + 1);
    std::size_t last_stx_idx = 0;
    std::size_t idx;

    for (idx = 0; idx < limit_idx; ++idx) {
        // first byte should be STX
        if (this->frame[idx] != STX) {
            continue;
        }

        // remember the index
        last_stx_idx = idx;

        // double check to prevent out-of-bound access
        if (idx + OFFSET_LGE >= frame.size())
            break;

        // responses have always an ADR, SVC and BCC field
        // so the length field should have a value of 3 or more
        if (this->frame[idx + OFFSET_LGE] < 3) {
            continue;
        }

        // total frame length as claimed by byte stream (note: might be wrong!)
        std::size_t frame_total_length = this->frame[idx + OFFSET_LGE] + 2;

        // bogus length in byte stream or not yet all data received
        if (frame_total_length > this->frame.size() - idx) {
            continue;
        }

        // validate the checksum
        unsigned char claimed_checksum = this->frame[idx + frame_total_length - 1];
        unsigned char calculated_checksum = this->calc_checksum(this->frame.data() + idx, frame_total_length - 1);

        if (claimed_checksum != calculated_checksum) {
            continue;
        }

        // if we get here, then it looks like a valid frame
        this->valid = true;
        this->frame.erase(this->frame.begin(), this->frame.begin() + idx);
        break;
    }

    // in case all found STX bytes do not look like a valid frame start
    // and we hit the MAX_FRAME_LENGTH limit, then let's discard the garbage
    if (last_stx_idx > 0 and idx > MAX_FRAME_LENGTH) {
        this->frame.erase(this->frame.begin(), this->frame.begin() + last_stx_idx);
    }
}

unsigned char USSFrame::get_address() const {
    return this->frame[OFFSET_ADR];
}

unsigned char USSFrame::get_service() const {
    return this->frame[OFFSET_SVC];
}

std::vector<unsigned char> USSFrame::get_data() const {
    std::size_t data_length = this->frame[OFFSET_LGE] - 3;
    return std::vector<unsigned char>(this->frame.begin() + OFFSET_DATA,
                                      this->frame.begin() + OFFSET_DATA + data_length);
}

void USSFrame::clear() {
    this->valid = false;
    this->frame.clear();
}
