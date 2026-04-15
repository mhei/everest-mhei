// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include <stdexcept>
#include <vector>

class USSFrame {
public:
    // default ctor
    USSFrame();

    // ctor for building a frame
    USSFrame(unsigned char address, unsigned char service, const std::vector<unsigned char>& data);

    operator std::vector<unsigned char>() const {
        return this->frame;
    }

    unsigned int operator[](std::size_t index) const {
        return this->frame[index];
    }

    std::size_t size() const {
        return this->frame.size();
    }

    // ctor from buffer
    USSFrame(const unsigned char* p, const std::size_t l);

    // streaming interface
    void push_back(const unsigned char* p, const std::size_t l);

    bool is_valid() const {
        return this->valid;
    };

    void clear();

    unsigned char get_address() const;
    unsigned char get_service() const;
    std::vector<unsigned char> get_data() const;

private:
    static constexpr std::size_t MIN_FRAME_LENGTH = 5;   // STX + LGE + ADR + SVC + PAYLOAD[0] + BCC
    static constexpr std::size_t MAX_FRAME_LENGTH = 260; // STX + LGE + ADR + SVC + PAYLOAD[255] + BCC
    static constexpr unsigned char STX = 0x02;

    static constexpr std::size_t OFFSET_LGE = 1;
    static constexpr std::size_t OFFSET_ADR = 2;
    static constexpr std::size_t OFFSET_SVC = 3;
    static constexpr std::size_t OFFSET_DATA = 4;

    std::vector<unsigned char> frame;
    bool valid{false};

    static unsigned char calc_checksum(const unsigned char* p, const std::size_t l);

    void check_frame();
};
