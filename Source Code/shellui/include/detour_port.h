// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stddef.h>
#include <stdint.h>
bool PortReadCode(uint64_t address,void* bytes,size_t length);
bool PortWriteCode(uint64_t address,const void* bytes,size_t length);
void* PortDetourFunction(uint64_t address,void* replacement,void(*progress)(const char*)=nullptr);
bool BeginPortDetours();
void EnablePortExternalPublication();
void FinishPortExternalPublication();
bool PortDetoursReady();
bool CommitPortDetours();
bool RollbackPortDetours();
