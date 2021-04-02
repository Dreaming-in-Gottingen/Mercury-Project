//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "MPEG2TSWriterTest"
#include <Log.h>

#include <RefBase.h>
#include <Errors.h>

#include <MetaData.h>
#include <MediaBuffer.h>
#include <MediaDefs.h>
#include <MediaSource.h>
#include <MPEG2TSWriter.h>

#include <errno.h>
#include <string.h>

using namespace Mercury;

/*
 * for Android4.4
 * Muxer(MPEG2TSWriter:SourceInfo) have two type(audio+video) of MediaSouces, which is OMXCodec
 * OMXCodec's MediaSource is CameraSource or AudioSource
 * CameraSource <-> OMXCodec <-> Muxer
 * now, I replace OMXCodec with local bitstream file to simulate it's behaviour,
 * for the reason that OMXCodec has not porting OK and reduce complexity.
 */
class FakeMediaSource: public MediaSource {
public:
    FakeMediaSource() {
        ALOGI("FakeMediaSource ctor! this=%p", this);
    }

    // below pure virtual api must be implemented because of derived from MediaSource
    status_t start(MetaData *params = NULL);
    status_t stop();
    sp<MetaData> getFormat();
    status_t read(MediaBuffer **buffer, const ReadOptions *options = NULL);

    status_t openBsFile(const char *path, uint8_t type);

protected:
    virtual ~FakeMediaSource() {
        ALOGI("FakeMediaSource dtor! this=%p", this);
    }

private:
    uint8_t mBsType; // 0-h264; 1-aac
    FILE *mpBsFp;

    uint8_t *mpBuf;
    uint8_t *mpHeader;

    bool mbCsdSend;
    uint8_t *mpCsdData;
    uint8_t mCsdLen;

    MediaBuffer *mpMediaBuffer;
};

status_t FakeMediaSource::start(MetaData *params)
{
    int size0, size1;
    int idx, flag, dura;

#define MAX_BS_BUF 1024*1024 // should make sure: one bitstream len <= 1MB

    mpBuf = (uint8_t *)malloc(MAX_BS_BUF);
    mpHeader = (uint8_t *)malloc(64);

    // header format: $$AV$$csd%d,%d:
    // first data block is video csd
    memset(mpHeader, 0, 64);

    // video csd extract
    fread(mpHeader, 1, 15, mpBsFp);
    int ret = sscanf(mpHeader, "$$AV$$csd%d,%d:", &idx, &size0);
    ALOGD("ret:%d, idx:%d, vcsd_len:%d", ret, idx, size0);
    fread(mpBuf, 1, size0, mpBsFp);
    if (mBsType == idx) {
        mpCsdData = (uint8_t*)malloc(size0);
        memcpy(mpCsdData, mpBuf, size0);
        mCsdLen = size0;
        mpMediaBuffer = new MediaBuffer(mpCsdData, mCsdLen);
        ALOGD("spspps:%#x,%#x,%#x,%#x,%#x,%#x", mpCsdData[0], mpCsdData[1], mpCsdData[2], mpCsdData[3], mpCsdData[4], mpCsdData[5]);
    }

    // audio csd extract
    fread(mpHeader, 1, 15, mpBsFp);
    ret = sscanf(mpHeader, "$$AV$$csd%d,%d:", &idx, &size0);
    ALOGD("ret:%d, idx:%d, acsd_len:%d", ret, idx, size0);
    fread(mpBuf, 1, size0, mpBsFp);
    if (mBsType == idx) {
        mpCsdData = (uint8_t*)malloc(size0);
        memcpy(mpCsdData, mpBuf, size0);
        mCsdLen = size0;
        mpMediaBuffer = new MediaBuffer(mpCsdData, mCsdLen);
    }

    mbCsdSend = false;
    return OK;
}

status_t FakeMediaSource::stop()
{
    ALOGI("MPEG2TSWriter ask FakeMediaSource to stop!");
    fclose(mpBsFp);
    mpBsFp = NULL;

    free(mpBuf);
    free(mpHeader);
    free(mpCsdData);
    return OK;
}

sp<MetaData> FakeMediaSource::getFormat()
{
    sp<MetaData> meta = new MetaData();

    if (mBsType == 0) {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_VIDEO_AVC /*"video/avc"*/);
    } else {
        meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_AUDIO_AAC /*"audio/mp4a-latm"*/);
    }
    return meta;
}

status_t FakeMediaSource::read(MediaBuffer **buffer, const ReadOptions *options)
{
    if (mbCsdSend == false) {
        *buffer = mpMediaBuffer;
        mbCsdSend = true;
    } else {
        int size0, size1;
        int idx, flag, dura;
        long long pts;
        int ret;
        int cnt = 0;

        /*
         * read one frame bitstream from local file
         * format: $$AV$$%d,%d,%lld,%d,%d,%d:...bitstream...
         * format: $$AV$$idx,flag,pts,dura,size0,size1:...bitstream...
         * idx: 0-avc; 1-aac
         * flag: 1-avc sync frame
         * pts/dura: by unit of ms
         * size0/size1: size1 usually is zero
         */
        while ((ret = fread(mpHeader, 1, 42, mpBsFp)) == 42) {
            sscanf(mpHeader, "$$AV$$%d,%d,%lld,%d,%d,%d:", &idx, &flag, &pts, &dura, &size0, &size1);
            cnt++;
            ret = fread(mpBuf, 1, size0, mpBsFp);
            if (size1 != 0)
                ret = fread(mpBuf+size0, 1, size1, mpBsFp);
            // may skip lots of package until find my wanted track data.
            if (mBsType == idx) {
                ALOGD("ret=%d, offset=%#lx, search_cnt=%d, idx=%d, flag=%d, pts=%lld, dura=%d, size0=%#x, size1=%#x",
                    ret, ftell(mpBsFp), cnt, idx, flag, pts, dura, size0, size1);
                mpMediaBuffer = new MediaBuffer(mpBuf, size0+size1);
                mpMediaBuffer->meta_data()->setInt64(kKeyTime, pts*1000);
                mpMediaBuffer->meta_data()->setInt32(kKeyIsSyncFrame, flag);
                *buffer = mpMediaBuffer;
                // for the purpose of av sync
                // file's framerate/samplerate: 30fps + 8k
                // that means duration: video-33ms, audio-128ms
                if (mBsType == 0) {
                    usleep(33*1000);
                } else {
                    usleep(128*1000);
                }
                return OK;
            }
        }
        if (ret != 42) {
            ALOGI("rd_ret=%d, must be EOF found!", ret);
            return ERROR_END_OF_STREAM;
        }
    }

    return OK;
}

status_t FakeMediaSource::openBsFile(const char *path, uint8_t type)
{
    mpBsFp = fopen(path, "rb");
    if (mpBsFp == NULL) {
        ALOGE("fopen() failed: %s", strerror(errno));
        return NAME_NOT_FOUND;
    }
    ALOGI("success to open file(%s)! mpBsFp=%p", path, mpBsFp);

    // two threads open and read the same file.
    // so I distinguish av_track by using this flag.
    // in file, video use index=0, audio use index=1.
    mBsType = type;

    return OK;
}

int main()
{
    ALOGI("---------------MPEG2TSWriter begin------------------");

    sp<FakeMediaSource> avc_source = new FakeMediaSource();
    avc_source->openBsFile("V3_avbs", 0);

    sp<FakeMediaSource> aac_source = new FakeMediaSource();
    aac_source->openBsFile("V3_avbs", 1);

    const char *ts_path = "video.ts";
    sp<MPEG2TSWriter> muxer = new MPEG2TSWriter(ts_path);

    muxer->addSource(avc_source);
    muxer->addSource(aac_source);

    muxer->start();
    while (!muxer->reachedEOS()) {
        sleep(1);
    }
    muxer->stop();

    muxer.clear();
    avc_source.clear();
    aac_source.clear();

    ALOGI("---------------MPEG2TSWriter end------------------");
}
