//#define LOG_NDEBUG 0
#define LOG_TAG "ALooperTester"

#include <Log.h>

#include <AHandler.h>
#include <ALooper.h>
#include <AMessage.h>
#include <ABuffer.h>
#include <ADebug.h>

using namespace Mercury;

struct Obj : public RefBase {
    Obj() {
        ALOGD("Obj(%p) ctor!", this);
    }
    ~Obj() {
        ALOGD("Obj(%p) dtor!", this);
    }

    void setName(AString str) {
        name = str;
    }
    AString getName() {
        return name;
    }

    void onFirstRef() {
        ALOGW("Obj onFirstRef!");
    }

    void onLastStrongRef(const void*) {
        ALOGW("Obj onLastStrongRef!");
    }

    void onLastWeakRef(const void*) {
        ALOGW("Obj onLastWeakRef!");
    }

private:
    AString name;

    DISALLOW_EVIL_CONSTRUCTORS(Obj);
};


//class ALooperTester : public AHandler {
//public:
struct ALooperTester : public AHandler {
    ALooperTester();
    void registerHandler(sp<ALooper> &looper);
    void start();
    void read();
    void wait();
    void other();
    void stop();

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
    ~ALooperTester();

private:
    enum {
        kWhatStart  = 'strt',
        kWhatRead   = 'read',
        kWhatWait   = 'wait',
        kWhatOther  = 'othr',
        kWhatStop   = 'stop',
        kWhatNewMsg = 'mesg',
    };

    sp<ALooper> mLooper;
};


ALooperTester::ALooperTester()
{
    //use internal looper
    //mLooper = new ALooper();
    //mLooper->setName("ALooperTester");
    ALOGD("ALooperTester(%p) ctor", this);
}

ALooperTester::~ALooperTester()
{
    mLooper->unregisterHandler(id());
    //mLooper->stop();
    ALOGD("ALooperTester(%p) dtor", this);
}

void ALooperTester::registerHandler(sp<ALooper> &looper)
{
    //use external looper, and register handler to looper
    //we also can register it out of class
    mLooper = looper;
    mLooper->registerHandler(this);
}

void ALooperTester::start()
{
    //registerHandler(this) CAN NOT be done in ctor, because 'this' obj is constructing.
    //mLooper->registerHandler(this);
    //mLooper->start();

    ALOGD("--------start begin----------");
    //deliver may fail when mLooper's internal thread has not run.
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setInt64("start_obj", (int64_t)this);
    msg->post();
    ALOGD("--------start end----------");
}

void ALooperTester::read()
{
    ALOGD("--------read begin--------");
    sp<AMessage> msg = new AMessage(kWhatRead, id());
    msg->post(2000*1000);
    ALOGD("--------read end---test post(2s)-------");
}

void ALooperTester::wait()
{
    ALOGD("--------wait begin--------");
    sp<AMessage> msg = new AMessage(kWhatWait, id());
    msg->setInt32("val", 123456);

    sp<AMessage> response;
    msg->postAndAwaitResponse(&response);
    ALOGD("--postAndAwaitResponse finish--");
    ALOGD("dump response info:");
    ALOGD("%s", response->debugString().c_str());
    ALOGD("--------wait end----------");
}

void ALooperTester::other()
{
    ALOGD("--------other(Object,Buffer,Message,Rect) begin--------");
    sp<AMessage> msg = new AMessage(kWhatOther, id());

    //Object
    sp<Obj> obj = new Obj();
    obj->setName("This is Obj.");
    msg->setObject("Object", obj);

    //ABuffer
    sp<ABuffer> abuf = new ABuffer(64);
    const char *buf = "This is ABuffer.";
    strcpy((char *)abuf->data(), buf);
    abuf->setRange(0, strlen(buf));
    msg->setBuffer("abuf", abuf);

    //AMessage
    sp<AMessage> mm = new AMessage(kWhatNewMsg, id());
    mm->setBuffer("buffer", abuf);
    msg->setMessage("message", mm);
    //msg->setMessage("message", msg); //CAN NOT transfer itself!!!

    //Rect
    msg->setRect("rect", 0, 0, 1920, 1080);

    msg->post();
    ALOGD("--------other(Object,Buffer,Message,Rect) end--------");
}

void ALooperTester::stop()
{
    ALOGD("--------stop begin-------");
    sp<AMessage> msg = new AMessage(kWhatStop, id());
    msg->post();
    // after unregisterHandler, msg->post will cause ALooperRoster do not know where to post
    // so we'd better put unregisterHandler in dtor
    //mLooper->unregisterHandler(id());
    //mLooper->stop();
    ALOGD("--------stop end----------");
}

void ALooperTester::onMessageReceived(const sp<AMessage> &msg)
{
    switch (msg->what())
    {
        case kWhatStart:
        {
            ALOGD("onMsg. start!");
            int64_t ptr;
            bool ret = msg->findInt64("start_obj", &ptr);
            ALOGD("ptr=%#lx, should equal to obj addr", (long)ptr);
            break;
        }
        case kWhatRead:
        {
            ALOGD("onMsg. delay(2s) read!");
            break;
        }
        case kWhatWait:
        {
            ALOGD("onMsg. wait! need response to sender!");
            ALOGD("dump Wait info...");
            ALOGD("-----------------------------------------------------------------");
            ALOGD("%s", msg->debugString().c_str());
            ALOGD("-----------------------------------------------------------------");
            int32_t v;
            bool ret = msg->findInt32("val", &v);
            if (ret) {
                ALOGI("find 'val': %d", v);
            } else {
                ALOGW("not find 'val'!");
            }
            uint32_t replyID;
            msg->senderAwaitsResponse(&replyID);
            msg->setString("replyInfo", "hello,world", 64);
            msg->postReply(replyID);
            break;
        }
        case kWhatOther:
        {
            ALOGD("onMsg. other!");
            ALOGD("dump Other info...");
            ALOGD("-----------------------------------------------------------------");
            ALOGD("%s", msg->debugString().c_str());
            ALOGD("-----------------------------------------------------------------");

            //Object
            sp<RefBase> o;
            const char *name = "Object";
            bool ret = msg->findObject(name, &o);
            sp<Obj> oo = static_cast<Obj*>(o.get());
            //Obj *oo = static_cast<Obj*>(o.get());
            if (ret) {
                ALOGD("find '%s'! content: '%s', addr:%p, ref_cnt:%d(3 or 4)", name, oo->getName().c_str(), oo.get(), oo->getStrongCount());
            } else {
                ALOGD("not find '%s'!", name);
            }

            //ABuffer
            sp<ABuffer> buf;
            name = "abuf";
            ret = msg->findBuffer(name, &buf);
            if (ret) {
                ALOGD("find '%s'! content_sz:%zd, content:%s", name, buf->size(), buf->data());
            } else {
                ALOGD("not find '%s'!", name);
            }

            //AMessage
            name = "message";
            sp<AMessage> mm;
            ret = msg->findMessage(name, &mm);
            if (ret) {
                ALOGD("find '%s'! send it out!", name);
                mm->post(); //next msg will go to default when NO case_kWhatNewMsg!
            } else {
                ALOGD("not find '%s'!", name);
            }

            //Rect
            name = "rect";
            int32_t l,t,r,b;
            ret = msg->findRect(name, &l, &t, &r, &b);
            if (ret) {
                ALOGD("find '%s'! l:%d, t:%d, r:%d, b:%d", name, l, t, r, b);
            } else {
                ALOGD("not find '%s'!", name);
            }
            ALOGD("onMsg(other) end! msg ref_cnt:%d(1 or 2)", msg->getStrongCount());

            break;
        }
        case kWhatStop:
        {
            ALOGD("onMsg. stop!");
            break;
        }
        case kWhatNewMsg:
        {
            ALOGD("onMsg. NewMsg!");
            ALOGD("dump NewMsg...");
            ALOGD("-----------------------------------------------------------------");
            ALOGD("%s", msg->debugString().c_str());
            ALOGD("-----------------------------------------------------------------");
            break;
        }
        default:
        {
            ALOGD("unknown msg! what:%#x, dump msg...", msg->what());
            ALOGD("-----------------------------------------------------------------");
            ALOGD("%s", msg->debugString().c_str());
            ALOGD("-----------------------------------------------------------------");
            break;
        }
    }
}

int main(void)
{
    ALOGD("----------------------ALooperTester begin----------------------");

    // below for testing ALooper/AMessage/AHandler
    sp<ALooper> looper = new ALooper();
    looper->start();

    sp<ALooperTester> tester = new ALooperTester();

    tester->registerHandler(looper);

    ALOGD("-----------------<start>---------------");
    tester->start();
    ALOGD("---------------start->read---------------");
    tester->read();
    ALOGD("---------------read->wait---------------");
    tester->wait();
    ALOGD("---------------wait->other---------------");
    tester->other();
    ALOGD("---------------other->stop---------------");
    tester->stop();
    ALOGD("-----------------<stop>---------------");

    sleep(3);

    tester.clear(); //equal to tester=NULL;

    ALOGD("----------------------ALooperTester end----------------------");

    return 0;
}
