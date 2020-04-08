#ifndef _UTIL_H_
#define _UTIL_H_
#include <android/log.h>
#include <unistd.h>
#define LOG_TAG "TGRAT"

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__))
//#define LOGD(...)
//#define LOGI(...)
//#define LOGW(...)

#ifdef DEBUG_TRACE
void _trace(const char* format, ...);
#endif

#ifdef DEBUG_TRACE
#define TRACE(X) _trace X;
#else /*DEBUG_TRACE*/
#define TRACE(X) ((void)__android_log_print(ANDROID_LOG_WARN, LOG_TAG, X))
#endif /*DEBUG_TRACE*/

#endif //_UTIL_H_

