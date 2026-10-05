// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <stdio.h>
#include <stdarg.h>
#include <pthread.h>
static inline void fps_log(const char *format,...) {
 char text[512];va_list args;va_start(args,format);vsnprintf(text,sizeof text,format,args);va_end(args);
 static pthread_mutex_t lock=PTHREAD_MUTEX_INITIALIZER;
 pthread_mutex_lock(&lock);
 FILE *f=fopen("/data/etaHEN/fps.log","a");
 if(f){fseek(f,0,SEEK_END);long size=ftell(f);if(size>=0 && size<1024*1024)fprintf(f,"%s\n",text);fclose(f);}
 pthread_mutex_unlock(&lock);
}
#define LOG_INFO(...) fps_log(__VA_ARGS__)
#define LOG_WARN(...) fps_log(__VA_ARGS__)
#define LOG_ERROR(...) fps_log(__VA_ARGS__)
#define LOG_DEBUG(...) ((void)0)
#define LOG_TRACE(...) ((void)0)
