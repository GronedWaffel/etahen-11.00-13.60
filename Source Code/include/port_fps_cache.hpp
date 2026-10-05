// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <onion/fps_validate.h>

// A render callback must never wait for a sampler, filesystem or app service.
// Deadline and fixed-point value are published together in one lock-free word.
class PortFpsCache {
    std::atomic<uint64_t> word{0};
public:
    static_assert(std::atomic<uint64_t>::is_always_lock_free);
    void clear() { word.store(0, std::memory_order_release); }
    void publish(const OnionFpsSample* sample, uint64_t realtime_ns, uint64_t mono_ms) {
        if(!onion_fps_sample_valid(sample,realtime_ns)){clear();return;}
        uint64_t remaining=(ONION_FPS_STALE_NS-(realtime_ns-sample->unix_ns))/1000000;
        unsigned tenths=(unsigned)(sample->fps*10.0f+0.5f);
        if(!remaining || mono_ms+remaining>=(UINT64_C(1)<<48)){clear();return;}
        word.store(((mono_ms+remaining)<<16)|tenths,std::memory_order_release);
    }
    unsigned read(uint64_t mono_ms) const {
        uint64_t value=word.load(std::memory_order_acquire);
        return mono_ms<(value>>16)?(unsigned)(value&65535):0;
    }
};
inline bool port_fps_record_current(const OnionFpsSample& s,uint64_t now) {
    return s.magic==ONION_FPS_MAGIC && !(s.seq&1) && s.pid>1 && s.unix_ns &&
        now>=s.unix_ns && now-s.unix_ns<=ONION_FPS_STALE_NS &&
        memchr(s.title_id,0,sizeof(s.title_id)) && s.title_id[0];
}
// A newer identified game with no usable counter supersedes the old game too.
inline const OnionFpsSample* port_fps_choose(const OnionFpsSample& native,
                                           const OnionFpsSample& bc,uint64_t now) {
    bool n=port_fps_record_current(native,now);
    bool b=port_fps_record_current(bc,now);
    if(n && (!b || native.unix_ns>=bc.unix_ns))return &native;
    return b?&bc:nullptr;
}
