#ifndef _LIBS_UTILS_LOG_H_
#define _LIBS_UTILS_LOG_H_

#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <assert.h>

namespace Mercury {
// ---------------------------------------------------------------------------

#ifdef __cplusplus
extern "C" {
#endif

#ifdef LOG_NDEBUG
#define MP_LOG_DEBUG 0
#else
#define MP_LOG_DEBUG 1
#endif

#ifndef LOG_TAG
#define LOG_TAG "MP_TAG"
#endif

#define CONDITION(cond)     (__builtin_expect(!!(cond), 1))

static char *timeString() {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    struct tm *t = localtime(&ts.tv_sec);
    static char timeStr[20];
    sprintf(timeStr, "%.2d-%.2d %.2d:%.2d:%.2d.%.3ld", t->tm_mon + 1, t->tm_mday, t->tm_hour,
            t->tm_min, t->tm_sec, ts.tv_nsec / 1000000);
    return timeStr;
}

#define CHECK(b) (b ? (void)0 : abort())

#define CHECK_EQ(a, b) CHECK((a) == (b))

#define CHECK_LT(a, b) CHECK((a) < (b))

#define CHECK_GE(a, b) CHECK((a) >= (b))

#define CHECK_LE(a, b) CHECK((a) <= (b))

#if MP_LOG_DEBUG

#define ALOGV(format, ...) \
printf("%s %d %d V %s: [%s:%d] " format"\n", timeString(), (int)getpid(), (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ALOGD(format, ...) \
printf("%s %d %d D %s: [%s:%d] " format"\n", timeString(), (int)getpid(), (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)


#else

#define ALOGV(format, ...)

#define ALOGD(format, ...)

#endif // MP_LOG_DEBUG

//-----------------------------------------------------------------------------------------------

#define ALOGI(format, ...) \
printf("%s %d %d I %s: [%s:%d] " format"\n", timeString(), (int)getpid(), (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ALOGW(format, ...) \
printf("%s %d %d W %s: [%s:%d] " format"\n", timeString(), (int)getpid(), (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define ALOGE(format, ...) \
printf("%s %d %d E %s: [%s:%d] " format"\n", timeString(), (int)getpid(), (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__)


#define LOG_ALWAYS_FATAL(...) do { ALOGE(__VA_ARGS__); assert(0); } while(0)

#define LOG_ALWAYS_FATAL_IF(cond, format, ...)                                                  \
    do {                                                                                        \
        if (!(cond)) {                                                                          \
            printf("%s %d %d E %s: [%s:%d] " format"\n", timeString(), (int)getpid(),           \
                    (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
            assert(0);                                                                          \
        }                                                                                       \
    } while(0)

#define ALOGW_IF(cond, format, ...)                                                             \
    do {                                                                                        \
        if ((cond)) {                                                                           \
            printf("%s %d %d W %s: [%s:%d] " format"\n", timeString(), (int)getpid(),           \
                    (int)syscall(SYS_gettid), LOG_TAG, __FUNCTION__, __LINE__, ##__VA_ARGS__);  \
        }                                                                                       \
    } while(0)

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#define ALOG_ASSERT(cond, str)                                \
    do {                                                      \
        if (!(cond)) {                                        \
            printf(__FILE__":" LITERAL_TO_STRING(__LINE__)    \
                    " CHECK(" #cond ") failed: " str);        \
            assert(0);                                        \
        }                                                     \
    } while(0)

#ifdef __cplusplus
}
#endif

// ---------------------------------------------------------------------------
}; // namespace Mercury


#endif // _LIBS_UTILS_LOG_H_
