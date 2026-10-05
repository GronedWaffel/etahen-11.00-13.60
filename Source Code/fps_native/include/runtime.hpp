// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <atomic>
#include <string>
#include <sys/types.h>
extern std::atomic<bool> g_stack_shutting_down;
bool Get_Running_App_TID(std::string &,int &);
pid_t fps_game_pid(int app);
bool onion_proc_is_alive(pid_t pid);
bool fps_startup_ready();
