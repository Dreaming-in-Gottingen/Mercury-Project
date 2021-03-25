//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "UtilsTest"
#include <Log.h>

#include <RefBase.h>

#include <ABuffer.h>
#include <AMessage.h>

#include <MetaData.h>
#include <Utils.h>

using namespace Mercury;

int main()
{
    ALOGI("---------------Utils begin------------------");

    uint32_t u32 = 0x1234abcd;
    uint8_t *ptr_u32 = (uint8_t *)&u32;
    ALOGD("raw_data=%#x; U32_AT()=%#x", u32, U32_AT(ptr_u32));

    uint64_t u64 = 0x1234567890abcdef;
    uint8_t *ptr_u64 = (uint8_t *)&u64;
    ALOGD("raw_data=%#llx; U64_AT()=%#llx", u64, U64_AT(ptr_u64));
    ALOGD("raw_data=%#llx; (may won't work)hton64()=%#llx", u64, hton64(ptr_u64));

    sp<AMessage> msg = new AMessage();
    msg->setString("mime", "video/avc");
    msg->setInt32("track-id", 1);
    msg->setInt32("rotation-degrees", 90);
    msg->setInt32("width", 1920);
    msg->setInt32("height", 1080);
    msg->setInt64("durationUs", 30*3600*1000000);
    msg->setInt32("frame-rate", 30);
    msg->setInt32("color-range", 2);
    msg->setInt32("color-standard", 1);
    msg->setInt32("max-input-size", 153401);
    sp<ABuffer> csd0_buf = new ABuffer(64);
    sp<ABuffer> csd1_buf = new ABuffer(64);
    uint8_t csd0[] = { 00, 00, 00, 01, 0x67, 0x42, 0xC0, 0x14, 0xE9, 0x01, 0x30, 0x76, 0xCB, 0x11, 0x00, 0xB0, 0x88, 0x46, 0xA0 };
    uint8_t csd1[] = { 00, 00, 00, 01, 0x68, 0xCE, 0x3C, 0x80 };
    memcpy(csd0_buf->data(), csd0, sizeof(csd0));
    memcpy(csd1_buf->data(), csd1, sizeof(csd1));
    csd0_buf->setRange(0, sizeof(csd0));
    csd1_buf->setRange(0, sizeof(csd1));
    msg->setBuffer("csd-0", csd0_buf);
    msg->setBuffer("csd-1", csd1_buf);
    ALOGD("msg dump...");
    ALOGD("%s", msg->debugString().c_str());

    ALOGD("--------AMessage -> MetaData-----------");
    sp<MetaData> msg2meta = new MetaData();
    convertMessageToMetaData(msg, msg2meta);
    msg2meta->dumpToLog();

    ALOGD("--------MetaData -> AMessage-----------");
    sp<AMessage> meta2msg = new AMessage();
    convertMetaDataToMessage(msg2meta, &meta2msg);
    ALOGD("%s", meta2msg->debugString().c_str());

    ALOGI("---------------Utils end------------------");

    return 0;
}
