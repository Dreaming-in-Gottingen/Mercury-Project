//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "CallStackDemo"

#include <stdio.h>

#include <Log.h>
#include <CallStack.h>

using namespace Mercury;

void test0()
{
    ALOGD("----begin----");
    CallStack("test");
    ALOGD("----end----");
}

void test1()
{
    ALOGD("----begin----");
    test0();
    ALOGD("----end----");
}

void test2()
{
    ALOGD("----begin----");
    test1();
    ALOGD("----end----");
}

void test3()
{
    ALOGD("----begin----");
    test2();
    ALOGD("----end----");
}

int main(void)
{
    printf("-----------CallStackDemo begin------------\n");

    test3();

    printf("-----------CallStackDemo end------------\n");

    return 0;
}
