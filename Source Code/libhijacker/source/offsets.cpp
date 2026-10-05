extern "C"
{
#include <stdint.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <stdio.h>
}

static constexpr uint32_t VERSION_MASK = 0xffff0000;

// Existing and new firmware version constants
static constexpr uint32_t V100 = 0x1000000;
static constexpr uint32_t V101 = 0x1010000;
static constexpr uint32_t V102 = 0x1020000;
static constexpr uint32_t V105 = 0x1050000;
static constexpr uint32_t V110 = 0x1100000;
static constexpr uint32_t V111 = 0x1110000;
static constexpr uint32_t V112 = 0x1120000;
static constexpr uint32_t V113 = 0x1130000;
static constexpr uint32_t V114 = 0x1140000;
static constexpr uint32_t V200 = 0x2000000;
static constexpr uint32_t V220 = 0x2200000;
static constexpr uint32_t V225 = 0x2250000;
static constexpr uint32_t V226 = 0x2260000;
static constexpr uint32_t V230 = 0x2300000;
static constexpr uint32_t V250 = 0x2500000;
static constexpr uint32_t V270 = 0x2700000;
static constexpr uint32_t V300 = 0x3000000;
static constexpr uint32_t V310 = 0x3100000;
static constexpr uint32_t V320 = 0x3200000;
static constexpr uint32_t V321 = 0x3210000;
static constexpr uint32_t V400 = 0x4000000;
static constexpr uint32_t V402 = 0x4020000;
static constexpr uint32_t V403 = 0x4030000;
static constexpr uint32_t V450 = 0x4500000;
static constexpr uint32_t V451 = 0x4510000;
static constexpr uint32_t V500 = 0x5000000;
static constexpr uint32_t V502 = 0x5020000;
static constexpr uint32_t V510 = 0x5100000;
static constexpr uint32_t V550 = 0x5500000;
static constexpr uint32_t V600 = 0x6000000;
static constexpr uint32_t V602 = 0x6020000;
static constexpr uint32_t V650 = 0x6500000;
static constexpr uint32_t V700 = 0x7000000;
static constexpr uint32_t V701 = 0x7010000;
static constexpr uint32_t V720 = 0x7200000;
static constexpr uint32_t V740 = 0x7400000;
static constexpr uint32_t V760 = 0x7600000;
static constexpr uint32_t V761 = 0x7610000;
// New firmware versions
static constexpr uint32_t V800 = 0x8000000;
static constexpr uint32_t V820 = 0x8200000;
static constexpr uint32_t V840 = 0x8400000;
static constexpr uint32_t V860 = 0x8600000;
static constexpr uint32_t V900 = 0x9000000;
static constexpr uint32_t V905 = 0x9050000;
static constexpr uint32_t V920 = 0x9200000;
static constexpr uint32_t V940 = 0x9400000;
static constexpr uint32_t V960 = 0x9600000;
static constexpr uint32_t V1000 = 0x10000000;
static constexpr uint32_t V1001 = 0x10010000;
static constexpr uint32_t V1020 = 0x10200000;
static constexpr uint32_t V1040 = 0x10400000;
static constexpr uint32_t V1060 = 0x10600000;
// Verified public SDK v0.43 crt/kernel.c, firmware 13.60 (d9c9519).
static constexpr uint32_t V1360 = 0x13600000;




uint32_t getSystemSwVersion() {
    static uint32_t version;
    if (version != 0) [[likely]] {
        return version;
    }
    size_t size = 4;
    sysctlbyname("kern.sdk_version", &version, &size, nullptr, 0);
    return version;
}
#include "port_firmware.h"
namespace offsets {
static const SnipersFirmwareProfile* profile(){ return snipers_firmware_profile(getSystemSwVersion()); }
size_t allproc(){const auto* p=profile();return p ? p->allproc : static_cast<size_t>(-1);}
size_t security_flags(){const auto* p=profile();return p ? p->security : static_cast<size_t>(-1);}
size_t qa_flags(){const auto* p=profile();return p ? p->security + 0x24 : static_cast<size_t>(-1);}
size_t utoken_flags(){const auto* p=profile();return p ? p->security + 0x8C : static_cast<size_t>(-1);}
size_t root_vnode(){const auto* p=profile();return p ? p->rootvnode : static_cast<size_t>(-1);}
}
