// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "experimental_trace.h"
static inline void port_stage(const char* component,const char* stage){experimental_event(component,stage,0,0);}
static inline void port_result(const char* component,const char* operation,long long result,int error){experimental_event(component,operation,result,error);}
