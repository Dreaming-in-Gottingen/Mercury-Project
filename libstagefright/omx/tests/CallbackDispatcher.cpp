#define LOG_LEVEL 3
#define LOG_TAG "CallbackDispatcher"
#include <Log.h>

#include <RefBase.h>

#include <Thread.h>
#include <Mutex.h>
#include <Condition.h>

#include <list>

using namespace Mercury;

/*
 * copy from OMXNodeInstance, simplify it's process logic.
 */

typedef struct OMX_CALLBACKTYPE
{
    int (*EventHandler)(
        void* hComponent,
        void* pAppData,
        uint32_t eEvent,
        int32_t nData1,
        int32_t nData2,
        void* pEventData);
    int (*EmptyBufferDone)(
        void* hComponent,
        void* pAppData,
        void* pBuffer);
    int (*FillBufferDone)(
        void* hComponent,
        void* pAppData,
        void* pBuffer);
} OMX_CALLBACKTYPE;

struct omx_message {
    enum {
        EVENT,
        EMPTY_BUFFER_DONE,
        FILL_BUFFER_DONE,
        FRAME_RENDERED,
    } type;

    int fenceFd; // used for EMPTY_BUFFER_DONE and FILL_BUFFER_DONE; client must close this

    union {
        // if type == EVENT
        struct {
            uint32_t event;
            int32_t data1;
            int32_t data2;
            int32_t data3;
            int32_t data4;
        } event_data;

        // if type == EMPTY_BUFFER_DONE
        struct {
            uint32_t buffer;
        } buffer_data;

        // if type == FILL_BUFFER_DONE
        struct {
            uint32_t buffer;
            uint32_t range_offset;
            uint32_t range_length;
            uint32_t flags;
            uint32_t timestamp;
        } extended_buffer_data;

        // if type == FRAME_RENDERED
        struct {
            uint32_t timestamp;
            uint64_t nanoTime;
        } render_data;
    } u;
};

////////////////////////////////////////////////////////////////////////////////
struct TaskInstance : public RefBase {
	TaskInstance() {
        ALOGI("TaskInstance(%p) ctor!", this);
    }

    void init();
    int freeNode();

    // handles messages and removes them from the list
    void onMessages(std::list<omx_message> &messages);

    void startInternalComponent();
    void stopInternalComponent();

    static OMX_CALLBACKTYPE kCallbacks;

private:
	~TaskInstance() {
        ALOGI("TaskInstance(%p) dtor!", this);
    }

    static int OnEvent(
            void* hComponent,
            void* pAppData,
            uint32_t eEvent,
            int32_t nData1,
            int32_t nData2,
            void* pEventData);

    static int OnEmptyBufferDone(
            void* hComponent,
            void* pAppData,
            void* pBuffer);

    static int OnFillBufferDone(
            void* hComponent,
            void* pAppData,
            void* pBuffer);

    struct AVCodecCompThread;
    sp<AVCodecCompThread> mCompWorker;

    // Handles |msg|, and may modify it. Returns true if completely handled it and
    // |msg| does not need to be sent to the event listener.
    bool handleMessage(omx_message &msg);

    struct CallbackDispatcher;
    struct CallbackDispatcherThread;
    sp<CallbackDispatcher> mDispatcher;
};


////////////////////////////////////////////////////////////////////////////////
struct TaskInstance::CallbackDispatcherThread : public Thread {
    explicit CallbackDispatcherThread(CallbackDispatcher *dispatcher)
        : mDispatcher(dispatcher) {
    }

private:
    CallbackDispatcher *mDispatcher;

    bool threadLoop();

    CallbackDispatcherThread(const CallbackDispatcherThread &);
    CallbackDispatcherThread &operator=(const CallbackDispatcherThread &);
};


struct TaskInstance::CallbackDispatcher : public RefBase {
    explicit CallbackDispatcher(const sp<TaskInstance> &owner);

    // Posts |msg| to the listener's queue. If |realTime| is true, the listener thread is notified
    // that a new message is available on the queue. Otherwise, the message stays on the queue, but
    // the listener is not notified of it. It will process this message when a subsequent message
    // is posted with |realTime| set to true.
    void post(const omx_message &msg, bool realTime = true);

    bool loop();

protected:
    virtual ~CallbackDispatcher();

private:
    enum {
        // This is used for frame_rendered message batching, which will eventually end up in a
        // single AMessage in MediaCodec when it is signaled to the app. AMessage can contain
        // up-to 64 key-value pairs, and each frame_rendered message uses 2 keys, so the max
        // value for this would be 32. Nonetheless, limit this to 12 to which gives at least 10
        // mseconds of batching at 120Hz.
        kMaxQueueSize = 12,
    };

    Mutex mLock;

    sp<TaskInstance> const mOwner;
    bool mDone;
    Condition mQueueChanged;
    std::list<omx_message> mQueue;

    sp<CallbackDispatcherThread> mThread;

    void dispatch(std::list<omx_message> &messages);

    CallbackDispatcher(const CallbackDispatcher &);
    CallbackDispatcher &operator=(const CallbackDispatcher &);
};

TaskInstance::CallbackDispatcher::CallbackDispatcher(const sp<TaskInstance> &owner)
    : mOwner(owner),
      mDone(false) {
    ALOGD("CallbackDispatcher(%p) ctor!", this);
    mThread = new CallbackDispatcherThread(this);
    mThread->run("TaskCallbackDispatcher"/*, ANDROID_PRIORITY_FOREGROUND*/);
}

TaskInstance::CallbackDispatcher::~CallbackDispatcher() {
    ALOGD("CallbackDispatcher(%p) dtor begin!", this);
    {
        Mutex::Autolock autoLock(mLock);

        mDone = true;
        mQueueChanged.signal();
    }

    // A join on self can happen if the last ref to CallbackDispatcher
    // is released within the CallbackDispatcherThread loop
    status_t status = mThread->join();
    if (status != WOULD_BLOCK) {
        // Other than join to self, the only other error return codes are
        // whatever readyToRun() returns, and we don't override that
        CHECK_EQ(status, (status_t)NO_ERROR);
    }
    ALOGD("CallbackDispatcher(%p) dtor end!", this);
}

void TaskInstance::CallbackDispatcher::post(const omx_message &msg, bool realTime) {
    Mutex::Autolock autoLock(mLock);

    mQueue.push_back(msg);
    if (realTime || mQueue.size() >= kMaxQueueSize) {
        mQueueChanged.signal();
    }
}

void TaskInstance::CallbackDispatcher::dispatch(std::list<omx_message> &messages) {
    if (mOwner == NULL) {
        ALOGW("Would have dispatched a message to a node that's already gone.");
        return;
    }
    mOwner->onMessages(messages);
}

bool TaskInstance::CallbackDispatcher::loop() {
    for (;;) {
        std::list<omx_message> messages;

        {
            Mutex::Autolock autoLock(mLock);
            while (!mDone && mQueue.empty()) {
                mQueueChanged.wait(mLock);
            }

            if (mDone) {
                ALOGW("mDone change to TRUE! exit loop!!!");
                break;
            }

            messages.swap(mQueue);
        }

        dispatch(messages);
    }

    return false;
}

////////////////////////////////////////////////////////////////////////////////
bool TaskInstance::CallbackDispatcherThread::threadLoop() {
    return mDispatcher->loop();
}

////////////////////////////////////////////////////////////////////////////////
struct TaskInstance::AVCodecCompThread : public Thread {
	AVCodecCompThread(void* pApp) {
        ALOGD("AVCodecCompThread(%p) ctor!", this);
        mpApp = pApp;
    }

    void registerCallback(OMX_CALLBACKTYPE *pCb) {
        mpCb = pCb;
    }
    void startComp() {
        mbDone = false;
        run();
    }
    void stopComp() {
        mbDone = true;
        requestExit();
        join();
    }

private:
	~AVCodecCompThread() {
        ALOGD("AVCodecCompThread(%p) dtor!", this);
    }

    bool threadLoop() {
        mpCb->EventHandler(this, mpApp, 123, 1, 2, NULL);
        mpCb->EmptyBufferDone(this, mpApp, NULL);
        mpCb->FillBufferDone(this, mpApp, NULL);
        sleep(1);
        if (mbDone) {
            ALOGW("mbDone change to TRUE, need exit component!");
            return false;
        }
        return true;
    }

    OMX_CALLBACKTYPE* mpCb;
    void* mpApp;
    bool mbDone;
};

////////////////////////////////////////////////////////////////////////////////
void TaskInstance::onMessages(std::list<omx_message> &messages)
{
    for (std::list<omx_message>::iterator it = messages.begin(); it != messages.end(); ) {
        if (handleMessage(*it)) {
            messages.erase(it++);
        } else {
            ++it;
        }
    }
}

bool TaskInstance::handleMessage(omx_message &msg)
{
    switch (msg.type) {
    case omx_message::FILL_BUFFER_DONE:
        ALOGD("pretend to handle msg(FILL_BUFFER_DONE)");
        break;
    case omx_message::EMPTY_BUFFER_DONE:
        ALOGD("pretend to handle msg(EMPTY_BUFFER_DONE)");
        break;
    case omx_message::EVENT:
        ALOGD("pretend to handle msg(EVENT)");
        break;
    default:
        ALOGE("unknown msg(%d)", msg.type);
        break;
    }
    return true;
}

int TaskInstance::OnEvent(
        void* hComponent,
        void* pAppData,
        uint32_t eEvent,
        int32_t nData1,
        int32_t nData2,
        void* pEventData)
{
    TaskInstance *instance = static_cast<TaskInstance *>(pAppData);
    omx_message msg;
    msg.type = omx_message::EVENT;
    msg.fenceFd = -1;
    msg.u.event_data.event = eEvent;
    msg.u.event_data.data1 = nData1;
    msg.u.event_data.data2 = nData2;
    instance->mDispatcher->post(msg, true /* realTime */);
    return 0;
}

int TaskInstance::OnEmptyBufferDone(
        void* hComponent,
        void* pAppData,
        void* pBuffer)
{
    TaskInstance *instance = static_cast<TaskInstance *>(pAppData);
    omx_message msg;
    msg.type = omx_message::EMPTY_BUFFER_DONE;
    msg.fenceFd = -1;
    msg.u.buffer_data.buffer = (uint32_t)pBuffer;
    instance->mDispatcher->post(msg, true /* realTime */);
    return 0;
}

int TaskInstance::OnFillBufferDone(
        void* hComponent,
        void* pAppData,
        void* pBuffer)
{
    TaskInstance *instance = static_cast<TaskInstance *>(pAppData);
    omx_message msg;
    msg.type = omx_message::FILL_BUFFER_DONE;
    msg.fenceFd = -1;
    msg.u.extended_buffer_data.buffer = (uint32_t)pBuffer;
    instance->mDispatcher->post(msg, true /* realTime */);
    return 0;
}

OMX_CALLBACKTYPE TaskInstance::kCallbacks = {
    &OnEvent, &OnEmptyBufferDone, &OnFillBufferDone
};

void TaskInstance::startInternalComponent()
{
    mCompWorker = new AVCodecCompThread(this);
    mCompWorker->registerCallback(&kCallbacks);
    mCompWorker->startComp();
}

void TaskInstance::stopInternalComponent()
{
    mCompWorker->stopComp();
}

void TaskInstance::init()
{
    mDispatcher = new CallbackDispatcher(this);
}

int TaskInstance::freeNode()
{
    mCompWorker.clear();
    mDispatcher.clear();
    return 0;
}

int main()
{
    ALOGD("--------------------CallbackDispatcher begin---------------------------");

    sp<TaskInstance> instance = new TaskInstance();
    ALOGD("----------init------------");
    instance->init();
    ALOGD("----------init->startInternalComponent------------");
    instance->startInternalComponent();
    ALOGD("----------sleep 5s------------");
    sleep(5);
    ALOGD("----------sleep->stopInternalComponent------------");
    instance->stopInternalComponent();
    ALOGD("----------stopInternalComponent->freeNode------------");
    instance->freeNode();

    ALOGD("--------------------CallbackDispatcher end---------------------------");

    return 0;
}

