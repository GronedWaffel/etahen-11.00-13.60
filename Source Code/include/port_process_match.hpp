// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <string.h>

// URL loading gives the bootstrap the downloaded filename, which can itself
// contain "etaHEN". Only a different live process can be a resident instance.
inline bool port_other_process_matches(int pid, int self, const char* comm,
                                       size_t capacity, const char* needle) {
    if (pid <= 0 || pid == self || !comm || !needle || !*needle) return false;
    size_t length = 0;
    while (length < capacity && comm[length]) ++length;
    const size_t wanted = strlen(needle);
    if (wanted > length) return false;
    for (size_t i = 0; i <= length - wanted; ++i)
        if (!memcmp(comm + i, needle, wanted)) return true;
    return false;
}
