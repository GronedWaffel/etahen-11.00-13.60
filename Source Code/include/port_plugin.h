/* SPDX-License-Identifier: GPL-3.0-or-later
 * Shared etaHEN plugin validation and identity. No console APIs: also used by
 * host tests and the Toolbox directory reader. */
#ifndef ETAHEN_PORT_PLUGIN_H
#define ETAHEN_PORT_PLUGIN_H
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define PORT_PLUGIN_MAX_SIZE (64u * 1024u * 1024u)
#define PORT_PLUGIN_HEADER_SIZE 29u
typedef struct {
    char identity[16];
    char version[5];
    size_t elf_offset;
} PortPlugin;

static inline int port_plugin_suffix(const char *path, const char *suffix) {
    if (!path || !suffix) return 0;
    size_t a = strlen(path), b = strlen(suffix);
    return a >= b && !strcmp(path + a - b, suffix);
}

/* ShellUI sees /user/data and /usbN; services see /data and /mnt/usbN.
 * Hash the same spelling in both processes. Never use raw ELF bytes as a name. */
static inline void port_plugin_identity(const char *path, char out[16]) {
    if (!strncmp(path, "/user/data/", 11)) path += 5;
    uint64_t h = UINT64_C(14695981039346656037);
    if (!strncmp(path, "/usb", 4)) {
        const char *p = "/mnt";
        while (*p) { h ^= (unsigned char)*p++; h *= UINT64_C(1099511628211); }
    }
    while (*path) { h ^= (unsigned char)*path++; h *= UINT64_C(1099511628211); }
    snprintf(out, 16, "ep%013llx", (unsigned long long)(h & UINT64_C(0xfffffffffffff)));
}

static inline uint16_t port_plugin_u16(const unsigned char *p) {
    return (uint16_t)(p[0] | ((uint16_t)p[1] << 8));
}
static inline uint64_t port_plugin_u64(const unsigned char *p) {
    uint64_t v = 0;
    for (unsigned i = 0; i < 8; ++i) v |= (uint64_t)p[i] << (8*i);
    return v;
}

/* available may be only the header prefix when browsing. File bounds are still
 * checked against total; executable segment bounds are checked by the loader. */
static inline int port_plugin_parse(const char *path, const void *data,
                                    size_t available, size_t total, PortPlugin *out) {
    if (!path || !data || !out || available > total || total > PORT_PLUGIN_MAX_SIZE || total < 64) return 0;
    memset(out, 0, sizeof(*out));
    const unsigned char *p = (const unsigned char *)data;
    size_t off = 0;
    if (port_plugin_suffix(path, ".plugin")) {
        if (available < PORT_PLUGIN_HEADER_SIZE || total < PORT_PLUGIN_HEADER_SIZE + 64 ||
            memcmp(p, "etaHEN_PLUGIN", 14) || p[23] || p[28]) return 0;
        for (unsigned i = 14; i < 23; ++i) {
            if (i < 18 ? (p[i] < 'A' || p[i] > 'Z') : (p[i] < '0' || p[i] > '9')) return 0;
        }
        for (unsigned i = 24; i < 28 && p[i]; ++i)
            if (p[i] < 32 || p[i] > 126) return 0;
        memcpy(out->identity, p + 14, 9);
        memcpy(out->version, p + 24, 4);
        off = PORT_PLUGIN_HEADER_SIZE;
    } else if (port_plugin_suffix(path, ".elf")) {
        port_plugin_identity(path, out->identity);
    } else return 0;
    if (available < off + 64) return 0;
    p += off;
    if (memcmp(p, "\177ELF", 4) || p[4] != 2 || p[5] != 1 || p[6] != 1 ||
        port_plugin_u16(p+18) != 62 || port_plugin_u16(p+52) != 64 ||
        port_plugin_u16(p+54) != 56) return 0;
    const uint16_t type = port_plugin_u16(p+16), count = port_plugin_u16(p+56);
    if ((type != 2 && type != 3) || !count || count > 128) return 0;
    const uint64_t ph = port_plugin_u64(p+32), bytes = total - off;
    if (ph < 64 || ph > bytes || (uint64_t)count * 56 > bytes - ph) return 0;
    if (available == total) {
        int executable = 0;
        const uint64_t entry = port_plugin_u64(p+24);
        for (unsigned i = 0; i < count; ++i) {
            const unsigned char *seg = p + ph + i*56;
            uint64_t start = port_plugin_u64(seg+8), filesz = port_plugin_u64(seg+32);
            uint64_t memsz = port_plugin_u64(seg+40);
            if (start > bytes || filesz > bytes-start) return 0;
            if (seg[0] == 1 && seg[1] == 0 && seg[2] == 0 && seg[3] == 0) {
                uint64_t address=port_plugin_u64(seg+16), align=port_plugin_u64(seg+48);
                if(filesz>memsz || memsz>UINT64_MAX-address) return 0;
                if(align>1 && ((align&(align-1)) || (address%align)!=(start%align))) return 0;
                if((seg[4]&1) && entry>=address && entry-address<filesz) executable=1;
            }
        }
        if (!executable) return 0;
    }
    out->elf_offset = off;
    return 1;
}
#endif
