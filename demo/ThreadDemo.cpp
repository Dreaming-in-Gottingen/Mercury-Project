#define LOG_TAG "ThreadDemo"

#include <Log.h>
#include <Thread.h>

using namespace Mercury;

class TaskThread : public Thread {
public:
    TaskThread(int x, int y);

    //Polymorphic to init internal state when thread Run, even if not call explicitly.
    //This can be not exist to use Thread::readyToRun().
    //When it exist, it must return NO_ERROR to make thread run.
    virtual status_t readyToRun();

    //void requestExit();

protected:
    //protected to forbiden stack obj, only heap obj can be make!
    ~TaskThread();

private:
    //Must derived from Thread::threadLoop, because of pure virtual func.
    bool threadLoop();

    int mCount;
    int mX;
    int mY;
};

TaskThread::TaskThread(int x, int y)
{
    ALOGD("TaskThread ctor!");
    mX = x;
    mY = y;
}

TaskThread::~TaskThread()
{
    ALOGD("TaskThread dtor!");
}

status_t TaskThread::readyToRun()
{
    ALOGD("We pretend to init some internal state");
    mCount = 0;
    return NO_ERROR;
}

bool TaskThread::threadLoop()
{
    sleep(1);
    ALOGD("[%d]th to come in threadLoop", mCount++);

    return true;
}

int main(void)
{
    printf("---------------TaskThreadDemo begin------------------\n");

    sp<TaskThread> task = new TaskThread(123, 321);
    task->run();
    ALOGD("sleep begin");
    sleep(10);
    ALOGD("sleep end, and requestExit begin!");
    task->requestExit();
    ALOGD("requestExit end, and join begin!");
    task->join();
    ALOGD("join end");
    //task->requestExitAndWait(); //equal to requestExit()+join()

    printf("---------------TaskThreadDemo end------------------\n");

    return 0;
}

