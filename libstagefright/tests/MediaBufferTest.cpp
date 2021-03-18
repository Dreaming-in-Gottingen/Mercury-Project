//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "MediaBufferTest"
#include <Log.h>

#include <RefBase.h>

#include <ALooper.h>
#include <AHandler.h>
#include <AMessage.h>
#include <ABuffer.h>

#include <MetaData.h>
#include <MediaBuffer.h>

using namespace Mercury;

class MyObserver: public MediaBufferObserver {
public:
    virtual void signalBufferReturned(MediaBuffer *buffer)
    {
        ALOGD("signalBufferReturned");
        // only one way to dtor: setObserver(0)+release
        buffer->setObserver(NULL);
        buffer->release();
    }
};

class MyHandler: public AHandler {
public:
    MyHandler()  { ALOGD("++++MyHandler ctor++++"); }
    ~MyHandler() { ALOGD("----MyHandler dtor----"); }
    void start() {
        ALOGD("MyHandler start!");
        mLooper = new ALooper();
        mLooper->start();
        mLooper->registerHandler(this);

        sp<AMessage> msg = new AMessage(kWhatStart, id());
        msg->post();
    }
    void stop() {
        ALOGD("MyHandler stop!");
        sp<AMessage> msg = new AMessage(kWhatStop, id());
        msg->post();

        // need sleep a while, if not, will happen this:
        // [deliverMessage:132] failed to deliver message. Target handler not registered.
        usleep(10*1000);
        mLooper->unregisterHandler(id());
    }
    void push_buffer(MediaBuffer *buffer) {
        ALOGD("MyHandler push_buffer!");
        ALOGD("dump MediaBuffer.meta_data()...");
        buffer->meta_data()->dumpToLog();

        // not suitable!
        // because other thread can not get MediaBuffer pointer.
        // use scene: one thread add_ref()+setObserver(), other thread release() after have used MediaBuffer.data()
        //buffer->add_ref();
        //buffer->setObserver(new MyObserver());

        // copy MediaBuffer' content(buf+meta) to a new ABuffer
        sp<ABuffer> copy = new ABuffer(buffer->range_length()+1); // careful: +1 logic, if string dump, must '\0' at tail.
        memcpy(copy->data(), buffer->data()+buffer->range_offset(), buffer->range_length());
        copy->data()[copy->size()-1] = 0;
        bool exist;
        const char* ptr;
        exist = buffer->meta_data()->findCString('info', &ptr);
        if (exist) {
            ALOGD("find[\"info\": %s] in MetaData.", ptr);
            copy->meta()->setString("info", ptr, strlen(ptr));
        }
        int64_t timeUs;
        exist = buffer->meta_data()->findInt64(kKeyTime, &timeUs);
        if (exist) {
            ALOGD("find[\"kKeyTime\": %#llx] in MetaData.", timeUs);
            copy->meta()->setInt64("timeUs", timeUs);
        }
        int32_t syncFrm;
        exist = buffer->meta_data()->findInt32(kKeyIsSyncFrame, &syncFrm);
        if (exist) {
            ALOGD("find[\"kKeyIsSyncFrame\": %d] in MetaData.", syncFrm);
            copy->meta()->setInt64("isSync", syncFrm);
        }

        // after copy out ABuffer's content, release MediaBuffer and send new ABuffer to another thread
        buffer->release(); // will dtor MediaBuffer obj.

        //msg->setPointer("mbuf", buffer); // this is not a good method, for we can't visit internal ABuffer.meta() in other thread.
        sp<AMessage> msg = new AMessage(kWhatBuffer, id());
        msg->setBuffer("abuffer", copy);
        msg->post();
    }

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg) {
        switch (msg->what()) {
            case kWhatStart:
                ALOGD("receive msg-start");
                break;
            case kWhatStop:
                ALOGD("receive msg-stop");
                break;
            case kWhatBuffer:
                ALOGD("receive msg-buffer");
                //MediaBuffer *pmb; // can not visit ABuffer! we not use this method
                //bool find = msg->findPointer("mbuf", &pmb);
                sp<ABuffer> abuf;
                bool find = msg->findBuffer("abuffer", &abuf);
                if (find) {
                    ALOGD("ABuffer: data()=(%s), cap=%zu, offset/size=(%zu,%zu)", abuf->data(), abuf->capacity(), abuf->offset(), abuf->size());
                    sp<AMessage> meta = abuf->meta();
                    if (meta != NULL) {
                        AString astr = meta->debugString();
                        ALOGD("meta dump...............");
                        ALOGD("%s", astr.c_str());
                    }
                } else {
                    ALOGD("fatal error! why not find mbuf?!");
                }
                break;
            default:
                ALOGD("receive ??? -> %#x", msg->what());
                break;
        }
    }

private:
    enum {
        kWhatStart  = 'stat',
        kWhatStop   = 'stop',
        kWhatBuffer = 'buff',
    };
    sp<ALooper> mLooper;
};

int main()
{
    ALOGD("---------------MediaBuffer begin------------------");

    size_t len = 128;
    //char *mem = malloc(len);
    // ctor1, mbuf doest not own mem, it just use it.
    //MediaBuffer mbuf1 = new MediaBuffer(mem, len);

    // ctor2

    // ctor3, mbuf doest not own mem(alloc by others-abuf).
    sp<ABuffer> abuf = new ABuffer(len);
    MediaBuffer *mbuf3 = new MediaBuffer(abuf);
    const char* str = "hello, this is to test mbuf3.";
    //memcpy(mbuf3->data(), str, strlen(str));
    strncpy(mbuf3->data(), str, len);
    abuf->setRange(0, strlen(str));
    ALOGD("abuf info: cap=%zu, range_offset/size=(%zu,%zu), refCnt=%d",
        abuf->capacity(), abuf->offset(), abuf->size(), abuf->getStrongCount());

    mbuf3->set_range(abuf->offset(), abuf->size());
    ALOGD("mbuf3 info: refCnt=%d, size=%zu, range_offset/length=(%zu,%zu)",
        mbuf3->refcount(), mbuf3->size(), mbuf3->range_offset(), mbuf3->range_length());
    ALOGD("mbuf3 data(): [%s]", mbuf3->data()+mbuf3->range_offset());

    // we can carry extra data by two methods: MediaBuffer.meta_data() or ABuffer.meta().
    sp<MetaData> meta = mbuf3->meta_data();
    int64_t timeUs = 0x12345678;
    meta->setInt64(kKeyTime, timeUs);
    meta->setInt32(kKeyIsSyncFrame, false);
    meta->setCString('info', "fuck the GFW!");

    sp<MyHandler> handler = new MyHandler();
    handler->start();
    handler->push_buffer(mbuf3);
    handler->stop();

    // use scene: two threads share data -- ABuffer, which need to auto manager by RefBase.
    // abuf is the real thing, which need to be protected!
    // use add_ref + setObserver at the same time before move ownership.
    //mbuf3->add_ref();
    //mbuf3->setObserver(new MyObserver());

    //ALOGD("mbuf3 info: refCnt=%d", mbuf3->refcount());
    //mbuf3->release();

    sleep(5);

    ALOGD("---------------MediaBuffer end------------------");

    return 0;
}
