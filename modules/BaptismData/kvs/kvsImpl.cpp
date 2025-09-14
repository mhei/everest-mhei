// SPDX-License-Identifier: GPL-3.0-only
// Copyright Michael Heimpold

#include <cmath>
#include <cstddef>
#include <cstring>
#include <cstdlib>
#include <string>
#include <variant>
#include "kvsImpl.hpp"
#include <baptismdata/baptismdata.h>

namespace module {
namespace kvs {

void kvsImpl::init() {
    struct baptismdata_ctx *ctx = NULL;
    void *tmp;
    int rv;

    rv = baptismdata_open(&ctx);
    if (rv < 0) {
        EVLOG_error << "baptismdata_open() failed: " << std::strerror(std::abs(rv));
        return;
    }

    /* iterate over all available variables and load them into our map */
    tmp = NULL;
    while ((tmp = baptismdata_iterator(ctx, tmp)) != NULL) {
        std::string k, v;

        k.assign(baptismdata_get_name(tmp));
        v.assign(baptismdata_get_value(tmp));

        this->kv_map.insert({k, v});
    }

    baptismdata_close(ctx);
}

void kvsImpl::ready() {
}

void kvsImpl::handle_store(std::string& key,
                           std::variant<std::nullptr_t, Array, Object, bool, double, int, std::string>& value) {
    // do nothing - baptism data is read-only
    EVLOG_debug << "handle_store(\"" << key << "\") ignored";
}

std::variant<std::nullptr_t, Array, Object, bool, double, int, std::string> kvsImpl::handle_load(std::string& key) {
    auto s = this->kv_map.find(key);

    if (s == this->kv_map.end())
        return nullptr;

    return s->second;
}

void kvsImpl::handle_delete(std::string& key) {
    // do nothing - baptism data is read-only
    EVLOG_debug << "handle_delete(\"" << key << "\") ignored";
}

bool kvsImpl::handle_exists(std::string& key) {
    auto s = this->kv_map.find(key);

    return s != this->kv_map.end();
}

} // namespace kvs
} // namespace module
