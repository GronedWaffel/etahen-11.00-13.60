// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <onion/fps_types.h>
#include <string.h>
static inline int onion_fps_sample_valid(const struct OnionFpsSample *s,uint64_t now) {
 return s && s->magic==ONION_FPS_MAGIC && s->valid && s->pid>1 &&
        s->unix_ns>0 && now>=s->unix_ns && now-s->unix_ns<=ONION_FPS_STALE_NS &&
        s->fps>=1.0f && s->fps<=240.0f && memchr(s->title_id,0,sizeof(s->title_id)) && s->title_id[0];
}
