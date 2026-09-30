// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdint.h>
#define PORT_PUBLISH_MAGIC UINT64_C(0x13605055424c4953)
#define PORT_PUBLISH_OFFSET 0x3400
typedef struct PortPatch {uint64_t address;unsigned char before[14],after[14];} PortPatch;
typedef struct PortPublishRequest {uint64_t magic;uint32_t count;uint32_t state;uint64_t records;} PortPublishRequest;
// state: 0 preparing, 1 request, 2 applied, 3 failed, 4 initialization finished.
