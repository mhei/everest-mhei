// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "KL02USSProtocol.hpp"
#include <cmath>
#include <iomanip>
#include <sstream>

USSFrame KL02USSProtocol::device_test_1() {
    return USSFrame(ADR, DEVICE_TEST1, {});
}

USSFrame KL02USSProtocol::device_test_2() {
    return USSFrame(ADR, DEVICE_TEST2, {});
}

USSFrame KL02USSProtocol::get_pwm() {
    return USSFrame(ADR, GET_PWM, {});
}

USSFrame KL02USSProtocol::set_pwm(double duty_cycle) {
    std::vector<unsigned char> data;
    append_u16(data, 1000); // 1 kHz fixed for our use-case
    append_u16(data, std::lround(duty_cycle * 10.0));
    return USSFrame(ADR, SET_PWM, data);
}

USSFrame KL02USSProtocol::control_pwm(PWMControl control) {
    return USSFrame(ADR, CONTROL_PWM, {control});
}

USSFrame KL02USSProtocol::get_ucp() {
    return USSFrame(ADR, GET_UCP, {});
}

USSFrame KL02USSProtocol::set_ucp(uint8_t value) {
    return USSFrame(ADR, SET_UCP, {value});
}

USSFrame KL02USSProtocol::lock(unsigned int number, LockControl cmd) {
    switch (number) {
    case 0:
        return USSFrame(ADR, LOCK1, {cmd});
    case 1:
        return USSFrame(ADR, LOCK2, {cmd});
    default:
        throw std::runtime_error("Invalid lock");
    }
}

USSFrame KL02USSProtocol::get_lock_fault() {
    return USSFrame(ADR, GET_LOCK_FAULT, {});
}

USSFrame KL02USSProtocol::set_cyclic(uint8_t interval) {
    return USSFrame(ADR, SET_CYCLIC, {interval});
}

USSFrame KL02USSProtocol::reset() {
    return USSFrame(ADR, RESET, {});
}

USSFrame KL02USSProtocol::set_pp_resistor(PPResistor value) {
    return USSFrame(ADR, PP_RESISTOR, {value});
}

USSFrame KL02USSProtocol::enable_pp_pullup(bool enable) {
    return USSFrame(ADR, PP_PULLUP, {enable});
}

USSFrame KL02USSProtocol::get_pp_voltage() {
    return USSFrame(ADR, PP_VOLTAGE, {});
}

KL02USSProtocol::DeviceTest1Response KL02USSProtocol::parse_device_test1(const USSFrame& f) {
    check_service(f, get_service_rsp(DEVICE_TEST1));

    auto d = f.get_data();
    if (d.size() < 3)
        throw std::runtime_error("Invalid payload");

    return {d[0], d[1], d[2]};
}

KL02USSProtocol::DeviceTest2Response KL02USSProtocol::parse_device_test2(const USSFrame& f) {
    check_service(f, get_service_rsp(DEVICE_TEST2));

    auto d = f.get_data();
    if (d.size() < 3)
        throw std::runtime_error("Invalid payload");

    return {d[0], d[1], d[2]};
}

KL02USSProtocol::PWMValues KL02USSProtocol::parse_get_pwm(const USSFrame& f) {
    check_service(f, get_service_rsp(GET_PWM));

    auto d = f.get_data();
    if (d.size() < 4)
        throw std::runtime_error("Invalid payload");

    return {read_u16(&d[0]), read_u16(&d[2])};
}

bool KL02USSProtocol::parse_set_pwm(const USSFrame& f) {
    check_service(f, get_service_rsp(SET_PWM));

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return d[0]; // 0 = no error, 1 = invalid parameter
}

bool KL02USSProtocol::parse_control_pwm(const USSFrame& f) {
    check_service(f, get_service_rsp(CONTROL_PWM));

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return d[0]; // 0 = PWM off, 1 = PWM on
}

KL02USSProtocol::CPVoltage KL02USSProtocol::parse_get_ucp(const USSFrame& f) {
    check_service(f, get_service_rsp(GET_UCP));

    auto d = f.get_data();
    if (d.size() < 4)
        throw std::runtime_error("Invalid payload");

    return {read_i16(&d[0]) * VOLTAGE_LSB, read_i16(&d[2]) * VOLTAGE_LSB};
}

uint8_t KL02USSProtocol::parse_set_ucp(const USSFrame& f) {
    check_service(f, get_service_rsp(SET_UCP));

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return d[0];
}

KL02USSProtocol::LockStatus KL02USSProtocol::parse_lock(const USSFrame& f) {
    uint8_t svc = f.get_service();
    if (svc != get_service_rsp(LOCK1) && svc != get_service_rsp(LOCK2))
        throw std::runtime_error("Invalid service");

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return static_cast<LockStatus>(d[0]);
}

KL02USSProtocol::CyclicData KL02USSProtocol::parse_cyclic_data(const USSFrame& f) {
    check_service(f, get_service_rsp(SET_CYCLIC));

    auto d = f.get_data();
    if (d.size() < 8)
        throw std::runtime_error("Invalid payload");

    return {read_u16(&d[0]), read_i16(&d[2]) * VOLTAGE_LSB, read_i16(&d[4]) * VOLTAGE_LSB,
            static_cast<LockStatus>(d[6]), static_cast<LockStatus>(d[7])};
}

bool KL02USSProtocol::parse_por(const USSFrame& f) {
    check_service(f, get_service_rsp(RESET));

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return d[0];
}

bool KL02USSProtocol::parse_set_pp_resistor(const USSFrame& f) {
    check_service(f, get_service_rsp(PP_RESISTOR));

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return d[0];
}

bool KL02USSProtocol::parse_enable_pp_pullup(const USSFrame& f) {
    check_service(f, get_service_rsp(PP_PULLUP));

    auto d = f.get_data();
    if (d.size() < 1)
        throw std::runtime_error("Invalid payload");

    return d[0];
}

float KL02USSProtocol::parse_get_pp_voltage(const USSFrame& f) {
    check_service(f, get_service_rsp(PP_VOLTAGE));

    auto d = f.get_data();
    if (d.size() < 2)
        throw std::runtime_error("Invalid payload");

    return read_u16(&d[0]) * VOLTAGE_LSB;
}

bool KL02USSProtocol::is_svc_response_frame(const USSFrame& f) {
    return f.get_service() & RSP;
}

KL02USSProtocol::Service KL02USSProtocol::svc_response_to_service(unsigned int svc) {
    return static_cast<KL02USSProtocol::Service>(svc & ~RSP);
}

std::string KL02USSProtocol::service_to_string(KL02USSProtocol::Service service) {
    const char* name = "UNKNOWN";

    switch (service) {
    case DEVICE_TEST1:
        name = "DEVICE_TEST1";
        break;
    case DEVICE_TEST2:
        name = "DEVICE_TEST2";
        break;
    case GET_PWM:
        name = "GET_PWM";
        break;
    case SET_PWM:
        name = "SET_PWM";
        break;
    case CONTROL_PWM:
        name = "CONTROL_PWM";
        break;
    case GET_UCP:
        name = "GET_UCP";
        break;
    case SET_UCP:
        name = "SET_UCP";
        break;
    case LOCK1:
        name = "LOCK1";
        break;
    case LOCK2:
        name = "LOCK2";
        break;
    case GET_LOCK_FAULT:
        name = "GET_LOCK_FAULT";
        break;
    case SET_CYCLIC:
        name = "SET_CYCLIC";
        break;
    case PBSC:
        name = "PBSC";
        break;
    case RESET:
        name = "RESET";
        break;
    case PP_RESISTOR:
        name = "PP_RESISTOR";
        break;
    case PP_PULLUP:
        name = "PP_PULLUP";
        break;
    case PP_VOLTAGE:
        name = "PP_VOLTAGE";
        break;
    }

    std::ostringstream oss;
    oss << name << " (0x" << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
        << static_cast<int>(service) << ")";

    return oss.str();
}

uint8_t KL02USSProtocol::get_service_rsp(Service svc) {
    return static_cast<uint8_t>(svc) | RSP;
}

void KL02USSProtocol::append_u16(std::vector<unsigned char>& v, uint16_t val) {
    // Note: the byte-order seems to be wrong compared to what is received
    v.push_back(val & 0xFF);
    v.push_back((val >> 8) & 0xFF);
}

uint16_t KL02USSProtocol::read_u16(const unsigned char* p) {
    return static_cast<uint16_t>((p[1] << 8) | p[0]);
}

int16_t KL02USSProtocol::read_i16(const unsigned char* p) {
    return static_cast<int16_t>((p[1] << 8) | p[0]);
}

void KL02USSProtocol::check_service(const USSFrame& f, uint8_t expected) {
    if (!f.is_valid())
        throw std::runtime_error("Invalid frame");

    if (f.get_service() != expected)
        throw std::runtime_error("Unexpected service");
}
