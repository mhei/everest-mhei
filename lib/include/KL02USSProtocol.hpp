// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#pragma once
#include "USSFrame.hpp"
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

class KL02USSProtocol {
public:
    // Service IDs (from datasheet)
    enum Service : uint8_t {
        DEVICE_TEST1 = 0x01,
        DEVICE_TEST2 = 0x04,
        GET_PWM = 0x10,
        SET_PWM = 0x11,
        CONTROL_PWM = 0x12,
        GET_UCP = 0x14,
        SET_UCP = 0x15,
        LOCK1 = 0x17,
        LOCK2 = 0x18,
        GET_LOCK_FAULT = 0x1A,
        SET_CYCLIC = 0x20,
        PBSC = 0x31,
        RESET = 0x33,
        PP_RESISTOR = 0x50,
        PP_PULLUP = 0x51,
        PP_VOLTAGE = 0x52,
    };

    // reset reason flags for DEVICE_TEST1
    static constexpr uint8_t RESET_REASON_POR = 1 << 0;
    static constexpr uint8_t RESET_REASON_EXTERNAL = 1 << 1;
    static constexpr uint8_t RESET_REASON_BROWNOUT = 1 << 2;
    static constexpr uint8_t RESET_REASON_WATCHDOG = 1 << 3;
    static constexpr uint8_t RESET_REASON_JTAG = 1 << 4;

    // reset reason flags for DEVICE_TEST2
    static constexpr uint8_t RESET_REASON_INT_STOP_MODE = 1 << 0;
    static constexpr uint8_t RESET_REASON_INT_CORE_LOCKUP = 1 << 1;
    static constexpr uint8_t RESET_REASON_SW_RESET = 1 << 2;
    static constexpr uint8_t RESET_REASON_LOSS_CLK_RST = 1 << 3;
    static constexpr uint8_t RESET_REASON_WAKEUP_RST = 1 << 4;

    // various data structures
    struct DeviceTest1Response {
        uint8_t sw_version;
        uint8_t hw_version;
        uint8_t reset_reason;
    };

    struct DeviceTest2Response {
        uint8_t buildno_low;
        uint8_t buildno_high;
        uint8_t reset_reason;
    };

    struct PWMValues {
        uint16_t frequency;  // Hz
        uint16_t duty_cycle; // 0.1%
    };

    enum PWMControl : uint8_t {
        PWM_DISABLE = 0,
        PWM_ENABLE = 1,
        PWM_CHECK = 2,
    };

    struct CPVoltage {
        float positive;
        float negative;
    };

    // resistor flags for CP voltage selection
    static constexpr uint8_t CP_RESISTOR_2K7 = 1 << 0;
    static constexpr uint8_t CP_RESISTOR_1K3 = 1 << 1;
    static constexpr uint8_t CP_RESISTOR_270 = 1 << 2;

    enum LockControl : uint8_t {
        UNLOCK = 0,
        LOCK = 1,
        STATUS = 2,
    };

    enum LockStatus : uint8_t {
        OPEN = 0,
        CLOSED = 1,
        NOT_CONNECTED = 2,
    };

    struct CyclicData {
        uint16_t duty_cycle;
        float positive;
        float negative;
        LockStatus lock1_status;
        LockStatus lock2_status;
    };

    enum PPResistor : uint8_t {
        PP_2700 = 0,
        PP_150 = 1,
        PP_487 = 2,
        PP_1500 = 3,
        PP_680 = 4,
        PP_220 = 5,
        PP_100 = 6,
        PP_OFF = 7,
    };

    // request builders
    static USSFrame device_test_1();
    static USSFrame device_test_2();
    static USSFrame get_pwm();
    static USSFrame set_pwm(double duty_cycle);
    static USSFrame control_pwm(PWMControl control);
    static USSFrame get_ucp();
    static USSFrame set_ucp(uint8_t value);
    static USSFrame lock(unsigned int number, LockControl cmd);
    static USSFrame get_lock_fault();
    static USSFrame set_cyclic(uint8_t interval);
    static USSFrame reset();
    static USSFrame set_pp_resistor(PPResistor value);
    static USSFrame enable_pp_pullup(bool enable);
    static USSFrame get_pp_voltage();

    // response parsers
    static DeviceTest1Response parse_device_test1(const USSFrame& f);
    static DeviceTest2Response parse_device_test2(const USSFrame& f);
    static PWMValues parse_get_pwm(const USSFrame& f);
    static bool parse_set_pwm(const USSFrame& f);
    static bool parse_control_pwm(const USSFrame& f);
    static CPVoltage parse_get_ucp(const USSFrame& f);
    static uint8_t parse_set_ucp(const USSFrame& f);
    static LockStatus parse_lock(const USSFrame& f);
    static CyclicData parse_cyclic_data(const USSFrame& f);
    static bool parse_por(const USSFrame& f);
    static bool parse_set_pp_resistor(const USSFrame& f);
    static bool parse_enable_pp_pullup(const USSFrame& f);
    static float parse_get_pp_voltage(const USSFrame& f);

    static bool is_svc_response_frame(const USSFrame& f);
    static Service svc_response_to_service(unsigned int svc);

    static std::string service_to_string(Service service);

private:
    // KL02 uses static address
    static constexpr uint8_t ADR = 0x00;

    // response flag
    static constexpr uint8_t RSP = 0x80;

    // datasheet states 29 mV as LSB
    static constexpr float VOLTAGE_LSB = 0.029f;

    static uint8_t get_service_rsp(Service svc);
    static void append_u16(std::vector<unsigned char>& v, uint16_t val);
    static uint16_t read_u16(const unsigned char* p);
    static int16_t read_i16(const unsigned char* p);
    static void check_service(const USSFrame& f, uint8_t expected);
};
