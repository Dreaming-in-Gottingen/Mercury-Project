//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "String8Demo"
#include <Log.h>

#include <stdio.h>

#include <String8.h>

using namespace Mercury;

int main(void)
{
    printf("-----------String8Demo begin------------\n");

    String8 file("/mnt/sdcard/video.mp4");
    ALOGD("file string=%s", file.string());
    ALOGD("     size=%zd", file.size());
    ALOGD("     length=%zd", file.length());
    ALOGD("     bytes=%zd", file.bytes());
    ALOGD("     isEmpty=%d", file.isEmpty());
    ALOGD("     getPathLeaf=%s", file.getPathLeaf().string());
    ALOGD("     getPathExtension=%s", file.getPathExtension().string());

    const char *ptr = "hello,world";
    String8 fmt = String8::format("(char*) %s", ptr);
    ALOGD("fmt: [%s]", fmt.string());

    printf("-----------String8Demo end------------\n");

    return 0;
}
