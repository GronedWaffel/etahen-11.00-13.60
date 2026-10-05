#pragma once

extern "C" {
	#include <stdint.h>
}

struct Frame {
	Frame *next;
	uintptr_t addr;
};
static auto constexpr CRASH_LOG_PATH = "/data/etaHEN/experimental-daemon-crash.log";
const Frame *getFramePointer();
void printBacktraceForCrash();
void crash_log(const char *fmt, ...);
void printBacktraceForCrash();