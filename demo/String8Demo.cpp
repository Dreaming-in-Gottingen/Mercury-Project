//#define LOG_NDEBUG
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
    ALOGD("     size=%d", file.size());
    ALOGD("     length=%d", file.length());
    ALOGD("     bytes=%d", file.bytes());
    ALOGD("     isEmpty=%d", file.isEmpty());
    ALOGD("     getPathLeaf=%s", file.getPathLeaf().string());
    ALOGD("     getPathExtension=%s", file.getPathExtension().string());

    printf("-----------String8Demo end------------\n");

    return 0;
}
