// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace StronglinkSL032 {

enum class Preamble : unsigned char {
    Request = 0xBA,
    Response = 0xBD,
};

enum class Command : unsigned char {
    SelectMifareCard = 0x01,
};

enum class Status : unsigned char {
    Success = 0x00,
    NoTag = 0x01,
    Collision = 0x0A,
    ChecksumError = 0xF0,
};

enum class CardType : unsigned char {
    Other = 0x00,
    MFMini_4B = 0x01,
    MFMini_7B = 0x02,
    MF1K_4B__MFPLUS2K_SL1_4B = 0x03,
    MF1K_7B__MFPLUS2K_SL1_7B = 0x04,
    MF4K_4B__MFPLUS4K_SL1_4B = 0x05,
    MF4K_7B__MFPLUS4K_SL1_7B = 0x06,
    MF_Ultralight__Ultralight_C__NTag203 = 0x07,
    MF_DESFire__MF_DESFire_EV1 = 0x09,
    MF_PROX = 0x0B,
    MFPLUS2K_SL2_4B = 0x21,
    MFPLUS4K_SL2_4B = 0x22,
    MFPLUS2K_SL2_7B = 0x23,
    MFPLUS4K_SL2_7B = 0x24,
    MFPLUS2K_SL0_SL3_4B = 0x31,
    MFPLUS4K_SL0_SL3_4B = 0x32,
    MFPLUS2K_SL0_SL3_7B = 0x33,
    MFPLUS4K_SL0_SL3_7B = 0x34,
};

std::string CardType_to_string(CardType type);
std::ostream& operator<<(std::ostream& os, const CardType type);

// 1 byte preamble + 1 byte length + 1 byte command + 1 byte status + 1 byte checksum
constexpr std::size_t MinFrameLength = 5;

// 1 byte preamble + 1 byte length + 255 bytes command...checksum
constexpr std::size_t MaxFrameLength = 257;

unsigned char calc_checksum(const unsigned char* p, const std::size_t l);

class TxFrame {
public:
    TxFrame(const Command cmd, const std::vector<unsigned char>& payload);

    operator std::vector<unsigned char>() const {
        return this->frame;
    }

private:
    std::vector<unsigned char> frame;
};

class RxFrame {
public:
    RxFrame() = default;

    RxFrame(const unsigned char* p, const std::size_t l);

    void push_back(const unsigned char* p, const std::size_t l);

    bool is_valid() {
        return this->valid;
    };
    Command get_cmd();
    Status get_status();

    unsigned char* get_data();
    std::size_t get_data_length();

    std::pair<std::string, std::string> get_card_info();

    unsigned char* get_tail();
    std::size_t get_tail_length();

    void clear();

private:
    unsigned char buffer[2 * MaxFrameLength];
    std::size_t buffer_length{0};
    unsigned char* frame{nullptr};
    unsigned char* tail{nullptr};
    bool valid{false};

    static constexpr std::size_t OFFSET_PREAMBLE = 0;
    static constexpr std::size_t OFFSET_LEN = 1;
    static constexpr std::size_t OFFSET_CMD = 2;
    static constexpr std::size_t OFFSET_STATUS = 3;
    static constexpr std::size_t OFFSET_DATA = 4;

    void check_frame();
};

} // namespace StronglinkSL032
