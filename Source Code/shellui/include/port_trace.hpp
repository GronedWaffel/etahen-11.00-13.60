// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#if defined(ETAHEN_SHELLUI_TRACE) || defined(ETAHEN_EXPERIMENTAL_DIAGNOSTICS)
#include <stdio.h>
extern void PortStatus(const char*);
// Shared-memory breadcrumbs only. Never add file or socket I/O to ShellUI.
static inline void PortTrace(const char* phase, const char* detail="") {
 char text[192];
 snprintf(text,sizeof(text),"trace: %s %s",phase,detail);
 PortStatus(text);
}
#else
static inline void PortTrace(const char*,const char* = "") {}
#endif
