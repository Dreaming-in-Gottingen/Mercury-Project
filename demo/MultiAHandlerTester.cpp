//#define LOG_NDEBUG 0
#define LOG_TAG "MultiAHandler"

#include <Log.h>

#include <AHandler.h>
#include <ALooper.h>
#include <AMessage.h>
#include <ABuffer.h>
#include <ADebug.h>

using namespace Mercury;

// -----------------------------------------------------------------------------
//class AHandlerTester0 : public AHandler {
//public:
struct AHandlerTester0 : public AHandler {
    AHandlerTester0();
    void start();
    void read();
    void stop();

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
    ~AHandlerTester0();

private:
    enum {
        kWhatStart  = 'strt',
        kWhatRead   = 'read',
        kWhatStop   = 'stop',
    };

    sp<ALooper> mLooper;
};


AHandlerTester0::AHandlerTester0()
{
    ALOGD("AHandlerTester0(%p) ctor", this);
}

AHandlerTester0::~AHandlerTester0()
{
    ALOGD("AHandlerTester0(%p) dtor", this);
}

void AHandlerTester0::start()
{
    ALOGD("--------start begin----------");
    //deliver may fail when mLooper's internal thread has not run.
    sp<AMessage> msg = new AMessage(kWhatStart, id());
    msg->setInt64("start_obj", (int64_t)this);
    msg->post();
    ALOGD("--------start end----------");
}

void AHandlerTester0::read()
{
    ALOGD("--------read begin--------");
    sp<AMessage> msg = new AMessage(kWhatRead, id());
    msg->post(2000*1000);
    ALOGD("--------read end---test post(2s)-------");
}

void AHandlerTester0::stop()
{
    ALOGD("--------stop begin-------");
    sp<AMessage> msg = new AMessage(kWhatStop, id());
    msg->post();
    ALOGD("--------stop end----------");
}

void AHandlerTester0::onMessageReceived(const sp<AMessage> &msg)
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
        case kWhatStop:
        {
            ALOGD("onMsg. stop!");
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

// -----------------------------------------------------------------------------
struct AHandlerTester1 : public AHandler {
    AHandlerTester1();
    void write();

protected:
    virtual void onMessageReceived(const sp<AMessage> &msg);
    ~AHandlerTester1();

private:
    enum {
        kWhatWrite   = 'writ',
    };
};


AHandlerTester1::AHandlerTester1()
{
    ALOGD("AHandlerTester1(%p) ctor", this);
}

AHandlerTester1::~AHandlerTester1()
{
    ALOGD("AHandlerTester1(%p) dtor", this);
}

void AHandlerTester1::write()
{
    ALOGD("--------write begin--------");
    sp<AMessage> msg = new AMessage(kWhatWrite, id());
    msg->post();
    ALOGD("--------write end----------");
}

void AHandlerTester1::onMessageReceived(const sp<AMessage> &msg)
{
    switch (msg->what())
    {
        case kWhatWrite:
        {
            ALOGD("onMsg. write!");
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
    ALOGD("----------------------MultiAHandlerTester begin----------------------");

    sp<ALooper> looper = new ALooper();
    looper->setName("MultiAHandler");
    looper->start();

    sp<AHandlerTester0> tester0 = new AHandlerTester0();
    sp<AHandlerTester1> tester1 = new AHandlerTester1();

    looper->registerHandler(tester0.get());
    looper->registerHandler(tester1.get());

    ALOGD("-----------------<tester0. start>---------------");
    tester0->start();
    ALOGD("-----------------<tester0. read>---------------");
    tester0->read();
    ALOGD("-----------------<tester1. write>---------------");
    tester1->write();
    sleep(3);
    ALOGD("-----------------<tester0. stop>---------------");
    tester0->stop();

    ALOGD("-----------------<looper unregisterHandler/stop>---------------");
    //usleep(1000*100); // if not, may deliver(msg:'stop') fail
    looper->unregisterHandler(tester0->id());
    looper->unregisterHandler(tester1->id());
    looper->stop();
    ALOGD("-----------------<end of unregisterHandler/stop>---------------");

    sleep(1);

    tester0.clear(); //equal to tester=NULL;
    tester1.clear();

    ALOGD("----------------------MultiAHandlerTester end----------------------");

    return 0;
}
