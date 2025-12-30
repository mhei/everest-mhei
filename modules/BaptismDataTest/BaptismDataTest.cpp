// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include <string>
#include <variant>
#include <vector>
#include "BaptismDataTest.hpp"

namespace module {

void BaptismDataTest::init() {
    invoke_init(*p_empty);
}

void BaptismDataTest::ready() {
    invoke_ready(*p_empty);

    std::vector<std::string> keys = {"serial", "serial#", "serial_no", "board_revision",
        "dak", "product_code", "manufacturer", "manufacturer_url", "vendor",
        "model", "model_url", "model_no", "revision", "firmware_version"};

    for (auto k: keys) {
        if (r_kvs->call_exists(k)) {
            auto v = r_kvs->call_load(k);
            EVLOG_info << "BaptismData: \"" << k << "\" => \"" << std::get<std::string>(v) << "\"";
        } else {
            EVLOG_info << "BaptismData: \"" << k << "\" does not exist.";
        }
    }
}

} // namespace module
