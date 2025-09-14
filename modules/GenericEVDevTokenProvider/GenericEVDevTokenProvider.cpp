// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold
#include "GenericEVDevTokenProvider.hpp"

namespace module {

void GenericEVDevTokenProvider::init() {
    invoke_init(*p_main);
}

void GenericEVDevTokenProvider::ready() {
    invoke_ready(*p_main);
}

} // namespace module
