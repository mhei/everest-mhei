// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "DummyEvSlac.hpp"

namespace module {

void DummyEvSlac::init() {
    invoke_init(*p_main);
}

void DummyEvSlac::ready() {
    invoke_ready(*p_main);
}

} // namespace module
