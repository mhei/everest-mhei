// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "BaptismData.hpp"

namespace module {

void BaptismData::init() {
    invoke_init(*p_kvs);
}

void BaptismData::ready() {
    invoke_ready(*p_kvs);
}

} // namespace module
