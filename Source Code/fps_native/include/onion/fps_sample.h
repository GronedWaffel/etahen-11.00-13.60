/* Copyright (C) 2026 OnionHEN / LightningMods
 *
 * Seqlock record published by the daemon FPS sampler and read by ShellUI.
 * Header-only: ShellUI must not link libonion_fps (no DMAP / ioctl).
 */
#pragma once

#include <onion/system_tmp.h>
#include <onion/fps_validate.h>

#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <onion/fps_types.h>

static inline uint64_t onion_fps_realtime_ns(void) {
  struct timespec ts;
  if (clock_gettime(CLOCK_REALTIME, &ts) != 0)
    return 0;
  return (uint64_t)ts.tv_sec * 1000000000ull + (uint64_t)ts.tv_nsec;
}

static inline int onion_fps_seqlock_load(const volatile struct OnionFpsSample *src,
                                         struct OnionFpsSample *dst) {
  if (!src || !dst)
    return -1;
  for (int i = 0; i < 8; ++i) {
    const uint32_t s1 = __atomic_load_n(&src->seq, __ATOMIC_ACQUIRE);
    if (s1 & 1u)
      continue;
    memcpy(dst, (const void *)src, sizeof(*dst));
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    const uint32_t s2 = __atomic_load_n(&src->seq, __ATOMIC_ACQUIRE);
    if (s1 == s2 && (s2 & 1u) == 0)
      return 0;
  }
  return -1;
}

static inline struct OnionFpsSample *onion_fps_map_ro(const char *path) {
  const int fd = open(path, O_RDONLY);
  if (fd < 0)
    return (struct OnionFpsSample *)MAP_FAILED;
  struct stat st;
  if(fstat(fd,&st) || st.st_size<ONION_FPS_SAMPLE_BYTES){close(fd);return (struct OnionFpsSample*)MAP_FAILED;}
  void *p = mmap(NULL, ONION_FPS_SAMPLE_BYTES, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);
  return (struct OnionFpsSample *)p;
}

static inline int onion_fps_read_path(const char *path, struct OnionFpsSample **cached, struct OnionFpsSample *out) {
  struct OnionFpsSample *map=*cached;
  if (!out)
    return -1;
  memset(out, 0, sizeof(*out));
  if (map == NULL || map == (struct OnionFpsSample *)MAP_FAILED) {
    map = onion_fps_map_ro(path);
    *cached=map;
    if (map == (struct OnionFpsSample *)MAP_FAILED) {
      map = NULL;
      return -1;
    }
  }
  struct OnionFpsSample local;
  if (onion_fps_seqlock_load(map, &local) != 0)
    return -1;
  const uint64_t now = onion_fps_realtime_ns();
  if (!onion_fps_sample_valid(&local,now)) return -1;
  *out = local;
  return 0;
}

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
static_assert(sizeof(OnionFpsSample) == ONION_FPS_SAMPLE_BYTES,
              "OnionFpsSample must stay 128 bytes");
static_assert(offsetof(OnionFpsSample, unix_ns) == 24,
              "unix_ns must be 8-byte aligned");
#endif
