//#define LOG_NDEBUG
#define LOG_TAG "LogDemo"

#include <stdio.h>

#include <Log.h>

extern int test();

int main(void)
{
    printf("-----------LogDemo begin------------\n");

    ALOGV("this is verbose!");
    ALOGD("this is debug!");
    ALOGI("this is info!");
    ALOGW("this is warning!");
    ALOGE("this is error!");

    LOG_ALWAYS_FATAL_IF(2>1, "line:%d", __LINE__);
    ALOGW_IF(2>1, "line:%d", __LINE__);
    ALOG_ASSERT(2<1, "line:%d", __LINE__);

    printf("-----------LogDemo end------------\n");

    return 0;
}
