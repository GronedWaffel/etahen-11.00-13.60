// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#include <stddef.h>
// PS5 13.60 Mono metadata: Nullable<Int64>, size 16, alignment 8.
// Value-type offsets exclude the 16-byte boxed-object header.
struct PortNullableInt64 {
    uint8_t hasValue;
    uint8_t padding[7];
    int64_t value;
};
static_assert(sizeof(PortNullableInt64)==16);
static_assert(alignof(PortNullableInt64)==8);
static_assert(offsetof(PortNullableInt64,value)==8);
