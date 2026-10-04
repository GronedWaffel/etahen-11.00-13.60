// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#ifdef ETAHEN_TOOLBOX_DIAGNOSTIC
#ifdef __cplusplus
extern "C" {
#endif
void port_diag(const char *operation, unsigned long long address, long long result, int error);
void port_diag_flush(void);
#ifdef __cplusplus
}
#endif
#else
#define port_diag(operation,address,result,error) ((void)0)
#define port_diag_flush() ((void)0)
#endif
