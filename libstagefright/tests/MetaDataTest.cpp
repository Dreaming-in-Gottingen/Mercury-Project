//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "MetaDataTest"
#include <Log.h>

#include <RefBase.h>

#include <MetaData.h>

using namespace Mercury;
int main()
{
    ALOGD("---------------MetaData begin------------------");

    sp<MetaData> sp_md = new MetaData();

    ALOGD("no data, dump...");
    sp_md->dumpToLog();

    // add some data
    sp_md->setCString(kKeyMIMEType, "video/avc");
    sp_md->setInt32(kKeyWidth, 1280);
    sp_md->setInt32(kKeyHeight, 720);
    sp_md->setRect(kKeyCropRect, 0, 0, 320, 240);

    uint8_t csd_data[10] = {0,0,0,1,0x67,1,2,3,4,5};
    sp_md->setData(kKeyAVCC, kTypeAVCC, csd_data, sizeof(csd_data));

    ALOGD("add mime/w/h/rect/csd, dump...");
    sp_md->dumpToLog();

    // find data
    const char *mime_ptr;
    bool csd_exist = sp_md->findCString(kKeyMIMEType, &mime_ptr);
    ALOGD("csd_exit=%d, mime=[%s]", csd_exist, mime_ptr);

    // delete data
    int32_t left, top, right, bottom;
    bool rect_exist = sp_md->findRect(kKeyCropRect, &left, &top, &right, &bottom);
    if (rect_exist) {
        sp_md->remove(kKeyCropRect);
    }
    // find data
    rect_exist = sp_md->findRect(kKeyCropRect, &left, &top, &right, &bottom);
    if (rect_exist == true) {
        ALOGW("rect found!");
    } else {
        ALOGW("rect not found!");
    }

    ALOGD("delete rect, dump...");
    sp_md->dumpToLog();

    ALOGD("---------------MetaData end------------------");
    return 0;
}
