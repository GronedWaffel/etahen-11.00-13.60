// SPDX-License-Identifier: GPL-3.0-or-later
#include "port_debug_service.hpp"
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>

extern void notify(bool, const char*, ...);
extern void etaHEN_log(const char*, ...);
extern "C" {
extern uint8_t ps5debug_start[];
pid_t elfldr_spawn(const char*, int, uint8_t*, const char*);
}

bool port_start_ps5debug() {
    static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    // Keep this latch even if the child later exits: it may have installed
    // kernel hooks before failing. Repeated presses must never reinject it.
    static bool dispatched = false;
    pthread_mutex_lock(&lock);
    if (dispatched) {
        notify(true, "PS5Debug was already started this session.\nRestart the PS5 before loading it again.");
        pthread_mutex_unlock(&lock);
        return true;
    }

    // Test ownership without connecting to or disturbing an existing debugger.
    // No SO_REUSEADDR: any occupied port (including TIME_WAIT) blocks loading.
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        notify(true, "Cannot check PS5Debug port 744. Payload was not loaded.");
        pthread_mutex_unlock(&lock);
        return false;
    }
    sockaddr_in address{};
    address.sin_len = sizeof(address);
    address.sin_family = AF_INET;
    address.sin_port = htons(744);
    address.sin_addr.s_addr = htonl(INADDR_ANY);
    int result = bind(fd, reinterpret_cast<sockaddr*>(&address), sizeof(address));
    int error = errno;
    close(fd);
    if (result < 0) {
        etaHEN_log("PS5Debug start blocked: port probe errno=%d", error);
        notify(true, error == EADDRINUSE
            ? "Port 744 is already in use.\nPS5Debug may already be running; it was not loaded again."
            : "Cannot check PS5Debug port 744. Payload was not loaded.");
        pthread_mutex_unlock(&lock);
        return error == EADDRINUSE;
    }

    const pid_t pid = elfldr_spawn("/", STDOUT_FILENO, ps5debug_start, "PS5Debug-NG");
    etaHEN_log("PS5Debug-NG spawn result: %d", pid);
    if (pid > 0) dispatched = true;
    notify(true, pid > 0
        ? "PS5Debug-NG is starting on port 744.\nWait for its ready notification. Restart to disable."
        : "Failed to start PS5Debug-NG.");
    pthread_mutex_unlock(&lock);
    return pid > 0;
}
