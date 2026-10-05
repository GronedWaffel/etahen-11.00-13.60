/* Copyright (C) 2026 OnionHEN / LightningMods */
#pragma once
#include <stdint.h>
#define ONION_FPS_MAGIC 0x4F465053u /* 'OFPS' */
#define ONION_FPS_SAMPLE_BYTES 128
#define ONION_FPS_STALE_NS 2000000000ull /* 2 s */

enum OnionFpsSource {
  ONION_FPS_SRC_NONE = 0,
  ONION_FPS_SRC_SCANOUT = 1u << 0,
  ONION_FPS_SRC_RING = 1u << 1,
  ONION_FPS_SRC_GLOBAL = 1u << 2,
  ONION_FPS_SRC_HYBRID = 1u << 3,
  ONION_FPS_SRC_MULTIPASS = 1u << 4,
  ONION_FPS_SRC_BC = 1u << 5
};

struct OnionFpsSample {
  uint32_t magic;
  uint32_t seq;
  int32_t pid;
  uint8_t valid;
  uint8_t source;
  uint8_t pad[2];
  float fps;
  uint32_t pad_align;
  uint64_t unix_ns;
  char title_id[16];
  uint8_t reserved[80];
};
