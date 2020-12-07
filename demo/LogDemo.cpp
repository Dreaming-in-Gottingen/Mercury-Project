//#define LOG_NDEBUG
#define LOG_TAG "LogDemo"

#include <stdio.h>

#include <Log.h>

using namespace Mercury;

int main(void)
{
    printf("-----------LogDemo begin------------\n");

    ALOGV("this is verbose!");
    ALOGD("this is debug!");
    ALOGI("this is info!");
    ALOGW("this is warning!");
    ALOGE("this is error!");

    ALOG_ASSERT(2>1, "test ALOG_ASSERT!");
    //ALOG_ASSERT(2<1, "test ALOG_ASSERT!");

    //LOG_FATAL_IF(2<1, "line:%d", __LINE__);
    //LOG_FATAL_IF(2>1, "line:%d", __LINE__);

    //LOG_ALWAYS_FATAL("line:%d", __LINE__);
    //LOG_ALWAYS_FATAL("test LOG_ALWAYS_FATAL!");

    //LOG_ALWAYS_FATAL_IF(2<1, "line:%d", __LINE__);
    //LOG_ALWAYS_FATAL_IF(2>1, "line:%d", __LINE__);

    //ALOGW_IF(2>1, "line:%d", __LINE__);
    //ALOGW_IF(2<1, "test ALOGW_IF");

    printf("-----------LogDemo end------------\n");

    return 0;
}
