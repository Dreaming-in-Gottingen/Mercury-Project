/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

//#define LOG_NDEBUG 0
#define LOG_TAG "AnotherPacketSource"
#include <Log.h>

#include <Vector.h>

#include <ABuffer.h>
#include <ADebug.h>
#include <AMessage.h>
#include <AString.h>
#include <hexdump.h>

#include <MediaBuffer.h>
#include <MediaDefs.h>
#include <MetaData.h>
#include <Utils.h>

#include "AnotherPacketSource.h"

namespace Mercury {

const int64_t kNearEOSMarkUs = 2000000ll; // 2 secs

AnotherPacketSource::AnotherPacketSource(const sp<MetaData> &meta)
    : mIsAudio(false),
      mIsVideo(false),
      mEnabled(true),
      mFormat(NULL),
      mLastQueuedTimeUs(0),
      mEOSResult(OK),
      mLatestEnqueuedMeta(NULL),
      mLatestDequeuedMeta(NULL),
      mQueuedDiscontinuityCount(0) {
    setFormat(meta);

    mDiscontinuitySegments.push_back(DiscontinuitySegment());
}

void AnotherPacketSource::setFormat(const sp<MetaData> &meta) {
    if (mFormat != NULL) {
        // Only allowed to be set once. Requires explicit clear to reset.
        return;
    }

    mIsAudio = false;
    mIsVideo = false;

    if (meta == NULL) {
        return;
    }

    mFormat = meta;
    const char *mime;
    CHECK(meta->findCString(kKeyMIMEType, &mime));

    if (!strncasecmp("audio/", mime, 6)) {
        mIsAudio = true;
    } else  if (!strncasecmp("video/", mime, 6)) {
        mIsVideo = true;
    } else {
        CHECK(!strncasecmp("text/", mime, 5) || !strncasecmp("application/", mime, 12));
    }
}

AnotherPacketSource::~AnotherPacketSource() {
}

status_t AnotherPacketSource::start(MetaData *params) {
    return OK;
}

status_t AnotherPacketSource::stop() {
    return OK;
}

sp<MetaData> AnotherPacketSource::getFormat() {
    Mutex::Autolock autoLock(mLock);
    if (mFormat != NULL) {
        return mFormat;
    }

    List<sp<ABuffer> >::iterator it = mBuffers.begin();
    while (it != mBuffers.end()) {
        sp<ABuffer> buffer = *it;
        int32_t discontinuity;
        if (!buffer->meta()->findInt32("discontinuity", &discontinuity)) {
            sp<RefBase> object;
            if (buffer->meta()->findObject("format", &object)) {
                setFormat(static_cast<MetaData*>(object.get()));
                return mFormat;
            }
        }

        ++it;
    }
    return NULL;
}

status_t AnotherPacketSource::dequeueAccessUnit(sp<ABuffer> *buffer) {
    //ALOGD("dequeueAccessUnit");
    buffer->clear();

    Mutex::Autolock autoLock(mLock);
    while (mEOSResult == OK && mBuffers.empty()) {
        mCondition.wait(mLock);
    }

    if (!mBuffers.empty()) {
        *buffer = *mBuffers.begin();
        mBuffers.erase(mBuffers.begin());

        int32_t discontinuity;
        if ((*buffer)->meta()->findInt32("discontinuity", &discontinuity)) {
            if (wasFormatChange(discontinuity)) {
                mFormat.clear();
            }

            --mQueuedDiscontinuityCount;
            mDiscontinuitySegments.erase(mDiscontinuitySegments.begin());
            // CHECK(!mDiscontinuitySegments.empty());
            return INFO_DISCONTINUITY;
        }

        // CHECK(!mDiscontinuitySegments.empty());
        DiscontinuitySegment &seg = *mDiscontinuitySegments.begin();

        int64_t timeUs;
        mLatestDequeuedMeta = (*buffer)->meta()->dup();
        CHECK(mLatestDequeuedMeta->findInt64("timeUs", &timeUs));
        if (timeUs > seg.mMaxDequeTimeUs) {
            seg.mMaxDequeTimeUs = timeUs;
        }

        sp<RefBase> object;
        if ((*buffer)->meta()->findObject("format", &object)) {
            setFormat(static_cast<MetaData*>(object.get()));
        }

        return OK;
    }

    return mEOSResult;
}

status_t AnotherPacketSource::read(
        MediaBuffer **out, const ReadOptions *) {
    *out = NULL;

    Mutex::Autolock autoLock(mLock);
    while (mEOSResult == OK && mBuffers.empty()) {
        mCondition.wait(mLock);
    }

    if (!mBuffers.empty()) {
        const sp<ABuffer> buffer = *mBuffers.begin();
        mBuffers.erase(mBuffers.begin());

        int32_t discontinuity;
        if (buffer->meta()->findInt32("discontinuity", &discontinuity)) {
            if (wasFormatChange(discontinuity)) {
                mFormat.clear();
            }

            mDiscontinuitySegments.erase(mDiscontinuitySegments.begin());
            // CHECK(!mDiscontinuitySegments.empty());
            return INFO_DISCONTINUITY;
        }

        mLatestDequeuedMeta = buffer->meta()->dup();

        sp<RefBase> object;
        if (buffer->meta()->findObject("format", &object)) {
            setFormat(static_cast<MetaData*>(object.get()));
        }

        int64_t timeUs;
        CHECK(buffer->meta()->findInt64("timeUs", &timeUs));
        // CHECK(!mDiscontinuitySegments.empty());
        DiscontinuitySegment &seg = *mDiscontinuitySegments.begin();
        if (timeUs > seg.mMaxDequeTimeUs) {
            seg.mMaxDequeTimeUs = timeUs;
        }

        MediaBuffer *mediaBuffer = new MediaBuffer(buffer);

        mediaBuffer->meta_data()->setInt64(kKeyTime, timeUs);

        int32_t isSync;
        if (buffer->meta()->findInt32("isSync", &isSync)) {
            mediaBuffer->meta_data()->setInt32(kKeyIsSyncFrame, isSync);
        }

        *out = mediaBuffer;
        return OK;
    }

    return mEOSResult;
}

bool AnotherPacketSource::wasFormatChange(
        int32_t discontinuityType) const {
    if (mIsAudio) {
        return (discontinuityType & ATSParser::DISCONTINUITY_AUDIO_FORMAT) != 0;
    }

    if (mIsVideo) {
        return (discontinuityType & ATSParser::DISCONTINUITY_VIDEO_FORMAT) != 0;
    }

    return false;
}

void AnotherPacketSource::queueAccessUnit(const sp<ABuffer> &buffer) {
    int32_t damaged;
    if (buffer->meta()->findInt32("damaged", &damaged) && damaged) {
        // LOG(VERBOSE) << "discarding damaged AU";
        return;
    }

    Mutex::Autolock autoLock(mLock);
    mBuffers.push_back(buffer);
    mCondition.signal();

    int32_t discontinuity;
    if (buffer->meta()->findInt32("discontinuity", &discontinuity)){
        ALOGV("queueing a discontinuity with queueAccessUnit");
        ++mQueuedDiscontinuityCount;

        mLastQueuedTimeUs = 0ll;
        mEOSResult = OK;
        mLatestEnqueuedMeta = NULL;

        mDiscontinuitySegments.push_back(DiscontinuitySegment());
        return;
    }

    int64_t lastQueuedTimeUs;
    CHECK(buffer->meta()->findInt64("timeUs", &lastQueuedTimeUs));
    mLastQueuedTimeUs = lastQueuedTimeUs;
    ALOGV("queueAccessUnit timeUs=%" PRIi64 " us (%.2f secs)",
            mLastQueuedTimeUs, mLastQueuedTimeUs / 1E6);

    // CHECK(!mDiscontinuitySegments.empty());
    DiscontinuitySegment &tailSeg = *(--mDiscontinuitySegments.end());
    if (lastQueuedTimeUs > tailSeg.mMaxEnqueTimeUs) {
        tailSeg.mMaxEnqueTimeUs = lastQueuedTimeUs;
    }
    if (tailSeg.mMaxDequeTimeUs == -1) {
        tailSeg.mMaxDequeTimeUs = lastQueuedTimeUs;
    }

    if (mLatestEnqueuedMeta == NULL) {
        mLatestEnqueuedMeta = buffer->meta()->dup();
    } else {
        int64_t latestTimeUs = 0;
        int64_t frameDeltaUs = 0;
        CHECK(mLatestEnqueuedMeta->findInt64("timeUs", &latestTimeUs));
        if (lastQueuedTimeUs > latestTimeUs) {
            mLatestEnqueuedMeta = buffer->meta()->dup();
            frameDeltaUs = lastQueuedTimeUs - latestTimeUs;
            mLatestEnqueuedMeta->setInt64("durationUs", frameDeltaUs);
        } else if (!mLatestEnqueuedMeta->findInt64("durationUs", &frameDeltaUs)) {
            // For B frames
            frameDeltaUs = latestTimeUs - lastQueuedTimeUs;
            mLatestEnqueuedMeta->setInt64("durationUs", frameDeltaUs);
        }
    }
}

void AnotherPacketSource::clear() {
    Mutex::Autolock autoLock(mLock);

    mBuffers.clear();
    mEOSResult = OK;
    mQueuedDiscontinuityCount = 0;


    mDiscontinuitySegments.clear();
    mDiscontinuitySegments.push_back(DiscontinuitySegment());

    mFormat = NULL;
    mLatestEnqueuedMeta = NULL;
}

void AnotherPacketSource::queueDiscontinuity(
        ATSParser::DiscontinuityType type,
        const sp<AMessage> &extra,
        bool discard) {
    Mutex::Autolock autoLock(mLock);

    if (discard) {
        // Leave only discontinuities in the queue.
        List<sp<ABuffer> >::iterator it = mBuffers.begin();
        while (it != mBuffers.end()) {
            sp<ABuffer> oldBuffer = *it;

            int32_t oldDiscontinuityType;
            if (!oldBuffer->meta()->findInt32(
                        "discontinuity", &oldDiscontinuityType)) {
                it = mBuffers.erase(it);
                continue;
            }

            ++it;
        }

        for (List<DiscontinuitySegment>::iterator it2 = mDiscontinuitySegments.begin();
                it2 != mDiscontinuitySegments.end();
                ++it2) {
            DiscontinuitySegment &seg = *it2;
            seg.clear();
        }

    }

    mEOSResult = OK;
    mLastQueuedTimeUs = 0;
    mLatestEnqueuedMeta = NULL;

    if (type == ATSParser::DISCONTINUITY_NONE) {
        return;
    }

    ++mQueuedDiscontinuityCount;
    mDiscontinuitySegments.push_back(DiscontinuitySegment());

    sp<ABuffer> buffer = new ABuffer(0);
    buffer->meta()->setInt32("discontinuity", static_cast<int32_t>(type));
    buffer->meta()->setMessage("extra", extra);

    mBuffers.push_back(buffer);
    mCondition.signal();
}

void AnotherPacketSource::queueDiscontinuity(
        ATSParser::DiscontinuityType type,
        const sp<AMessage> &extra) {
    Mutex::Autolock autoLock(mLock);

    // Leave only discontinuities in the queue.
    List<sp<ABuffer> >::iterator it = mBuffers.begin();
    while (it != mBuffers.end()) {
        sp<ABuffer> oldBuffer = *it;

        int32_t oldDiscontinuityType;
        if (!oldBuffer->meta()->findInt32(
                    "discontinuity", &oldDiscontinuityType)) {
            it = mBuffers.erase(it);
            continue;
        }

        ++it;
    }

    mEOSResult = OK;
    mLastQueuedTimeUs = 0;
    mLatestEnqueuedMeta = NULL;

    sp<ABuffer> buffer = new ABuffer(0);
    buffer->meta()->setInt32("discontinuity", static_cast<int32_t>(type));
    buffer->meta()->setMessage("extra", extra);

    mBuffers.push_back(buffer);
    mCondition.signal();
}

void AnotherPacketSource::signalEOS(status_t result) {
    CHECK(result != OK);

    Mutex::Autolock autoLock(mLock);
    mEOSResult = result;
    mCondition.signal();
}

bool AnotherPacketSource::hasBufferAvailable(status_t *finalResult) {
    Mutex::Autolock autoLock(mLock);
    *finalResult = OK;
    if (!mEnabled) {
        return false;
    }
    if (!mBuffers.empty()) {
        ALOGI("hasBufferAvailable empty");
        return true;
    }

    *finalResult = mEOSResult;
    //ALOGI("hasBufferAvailable mEOSResult = %d", mEOSResult);
    return false;
}

int64_t AnotherPacketSource::getBufferedDurationUs(status_t *finalResult) {
    ALOGD("getBufferedDurationUs");
    Mutex::Autolock autoLock(mLock);
    *finalResult = mEOSResult;

    int64_t durationUs = 0;
    for (List<DiscontinuitySegment>::iterator it = mDiscontinuitySegments.begin();
            it != mDiscontinuitySegments.end();
            ++it) {
        const DiscontinuitySegment &seg = *it;
        // dequeued access units should be a subset of enqueued access units
        // CHECK(seg.maxEnqueTimeUs >= seg.mMaxDequeTimeUs);
        durationUs += (seg.mMaxEnqueTimeUs - seg.mMaxDequeTimeUs);
    }

    return durationUs;
    //Mutex::Autolock autoLock(mLock);
    //return getBufferedDurationUs_l(finalResult);
}

int64_t AnotherPacketSource::getBufferedDurationUs_l(status_t *finalResult) {
    *finalResult = mEOSResult;

    if (mBuffers.empty()) {
        return 0;
    }

    int64_t time1 = -1;
    int64_t time2 = -1;
    int64_t durationUs = 0;

    List<sp<ABuffer> >::iterator it = mBuffers.begin();
    while (it != mBuffers.end()) {
        const sp<ABuffer> &buffer = *it;

        int64_t timeUs;
        if (buffer->meta()->findInt64("timeUs", &timeUs)) {
            if (time1 < 0 || timeUs < time1) {
                time1 = timeUs;
            }

            if (time2 < 0 || timeUs > time2) {
                time2 = timeUs;
            }
        } else {
            // This is a discontinuity, reset everything.
            durationUs += time2 - time1;
            time1 = time2 = -1;
        }

        ++it;
    }

    return durationUs + (time2 - time1);
}

// A cheaper but less precise version of getBufferedDurationUs that we would like to use in
// LiveSession::dequeueAccessUnit to trigger downwards adaptation.
int64_t AnotherPacketSource::getEstimatedDurationUs() {
    Mutex::Autolock autoLock(mLock);
    if (mBuffers.empty()) {
        return 0;
    }

    if (mQueuedDiscontinuityCount > 0) {
        status_t finalResult;
        return getBufferedDurationUs_l(&finalResult);
    }

    List<sp<ABuffer> >::iterator it = mBuffers.begin();
    sp<ABuffer> buffer = *it;

    int64_t startTimeUs;
    buffer->meta()->findInt64("timeUs", &startTimeUs);
    if (startTimeUs < 0) {
        return 0;
    }

    it = mBuffers.end();
    --it;
    buffer = *it;

    int64_t endTimeUs;
    buffer->meta()->findInt64("timeUs", &endTimeUs);
    if (endTimeUs < 0) {
        return 0;
    }

    int64_t diffUs;
    if (endTimeUs > startTimeUs) {
        diffUs = endTimeUs - startTimeUs;
    } else {
        diffUs = startTimeUs - endTimeUs;
    }
    return diffUs;
}

status_t AnotherPacketSource::nextBufferTime(int64_t *timeUs) {
    *timeUs = 0;

    Mutex::Autolock autoLock(mLock);

    if (mBuffers.empty()) {
        return mEOSResult != OK ? mEOSResult : -EWOULDBLOCK;
    }

    sp<ABuffer> buffer = *mBuffers.begin();
    CHECK(buffer->meta()->findInt64("timeUs", timeUs));

    return OK;
}

bool AnotherPacketSource::isFinished(int64_t duration) const {
    if (duration > 0) {
        int64_t diff = duration - mLastQueuedTimeUs;
        if (diff < kNearEOSMarkUs && diff > -kNearEOSMarkUs) {
            ALOGV("Detecting EOS due to near end");
            return true;
        }
    }
    return (mEOSResult != OK);
}

sp<AMessage> AnotherPacketSource::getLatestEnqueuedMeta() {
    Mutex::Autolock autoLock(mLock);
    return mLatestEnqueuedMeta;
}

sp<AMessage> AnotherPacketSource::getLatestDequeuedMeta() {
    Mutex::Autolock autoLock(mLock);
    return mLatestDequeuedMeta;
}

void AnotherPacketSource::enable(bool enable) {
    Mutex::Autolock autoLock(mLock);
    mEnabled = enable;
}

/*
 * returns the sample meta that's delayUs after queue head
 * (NULL if such sample is unavailable)
 */
sp<AMessage> AnotherPacketSource::getMetaAfterLastDequeued(int64_t delayUs) {
    ALOGV("getMetaAfterLastDequeued");
    Mutex::Autolock autoLock(mLock);
    int64_t firstUs = -1;
    int64_t lastUs = -1;
    int64_t durationUs = 0;

    List<sp<ABuffer> >::iterator it;
    for (it = mBuffers.begin(); it != mBuffers.end(); ++it) {
        const sp<ABuffer> &buffer = *it;
        int32_t discontinuity;
        if (buffer->meta()->findInt32("discontinuity", &discontinuity)) {
            durationUs += lastUs - firstUs;
            firstUs = -1;
            lastUs = -1;
            continue;
        }
        int64_t timeUs;
        if (buffer->meta()->findInt64("timeUs", &timeUs)) {
            if (firstUs < 0) {
                firstUs = timeUs;
            }
            if (lastUs < 0 || timeUs > lastUs) {
                lastUs = timeUs;
            }
            if (durationUs + (lastUs - firstUs) >= delayUs) {
                return buffer->meta();
            }
        }
    }
    return NULL;
}

sp<AMessage> AnotherPacketSource::getLatestMeta() {
    Mutex::Autolock autoLock(mLock);
    return mLatestEnqueuedMeta;
}

}  // namespace Mercury
