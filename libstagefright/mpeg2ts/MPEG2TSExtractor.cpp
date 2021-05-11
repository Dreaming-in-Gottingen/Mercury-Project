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
#define LOG_TAG "MPEG2TSExtractor"
#include <Log.h>

#include <String8.h>

#include <ABuffer.h>
#include <ADebug.h>
#include <ALooper.h>
#include <ABitReader.h>

#include <DataSource.h>
#include <MediaDefs.h>
#include <MediaErrors.h>
#include <MediaSource.h>
#include <MetaData.h>

//#include <media/IStreamSource.h>

//#include "include/NuCachedSource2.h"

#include "MPEG2TSExtractor.h"

#include "AnotherPacketSource.h"
#include "ATSParser.h"

namespace Mercury {

static const size_t kTSPacketSize = 188;

struct MPEG2TSSource : public MediaSource {
    MPEG2TSSource(
            const sp<MPEG2TSExtractor> &extractor,
            const sp<AnotherPacketSource> &impl,
            bool doesSeek);

    virtual status_t start(MetaData *params = NULL);
    virtual status_t stop();
    virtual sp<MetaData> getFormat();

    virtual status_t read(
            MediaBuffer **buffer, const ReadOptions *options = NULL);

private:
    sp<MPEG2TSExtractor> mExtractor;
    sp<AnotherPacketSource> mImpl;

    // If there are both audio and video streams, only the video stream
    // will signal seek on the extractor; otherwise the single stream will seek.
    bool mDoesSeek;

    DISALLOW_EVIL_CONSTRUCTORS(MPEG2TSSource);
};

MPEG2TSSource::MPEG2TSSource(
        const sp<MPEG2TSExtractor> &extractor,
        const sp<AnotherPacketSource> &impl,
        bool doesSeek)
    : mExtractor(extractor),
      mImpl(impl),
      mDoesSeek(doesSeek) {
}

status_t MPEG2TSSource::start(MetaData *params) {
    return mImpl->start(params);
}

status_t MPEG2TSSource::stop() {
    return mImpl->stop();
}

sp<MetaData> MPEG2TSSource::getFormat() {
    return mImpl->getFormat();
}

status_t MPEG2TSSource::read(
        MediaBuffer **out, const ReadOptions *options) {
    //ALOGD("MPEG2TSSource::read");
    *out = NULL;

    int64_t seekTimeUs;
    ReadOptions::SeekMode seekMode;
    if (mDoesSeek){
	    //ALOGD("MPEG2TSSource::read mDoesSeek is true");
	}
    if (mDoesSeek && options && options->getSeekTo(&seekTimeUs, &seekMode)) {
        // seek is needed
        status_t err = mExtractor->seek(seekTimeUs, seekMode);
        if (err != OK) {
            return err;
        }
    }

    if (mExtractor->feedUntilBufferAvailable(mImpl) != OK) {
        return ERROR_END_OF_STREAM;
    }

    return mImpl->read(out, options);
}

////////////////////////////////////////////////////////////////////////////////

MPEG2TSExtractor::MPEG2TSExtractor(const sp<DataSource> &source)
    : mDataSource(source),
      mParser(new ATSParser),
      mOffset(0) {
    init();
}

size_t MPEG2TSExtractor::countTracks() {
    return mSourceImpls.size();
}

sp<MediaSource> MPEG2TSExtractor::getTrack(size_t index) {
    if (index >= mSourceImpls.size()) {
        return NULL;
    }

    bool seekable = true;
    if (mSourceImpls.size() >= 1) {
 //       CHECK_EQ(mSourceImpls.size(), 2u);

        sp<MetaData> meta = mSourceImpls.editItemAt(index)->getFormat();
        const char *mime;
        CHECK(meta->findCString(kKeyMIMEType, &mime));

        if (!strncasecmp("audio/", mime, 6) ||!strncasecmp("video/", mime, 6)) {
            seekable = false;
        }
    }

    return new MPEG2TSSource(this, mSourceImpls.editItemAt(index), (mSeekSyncPoints == &mSyncPoints.editItemAt(index)));
}

sp<MetaData> MPEG2TSExtractor::getTrackMetaData(
        size_t index, uint32_t /* flags */) {
    return index < mSourceImpls.size()
        ? mSourceImpls.editItemAt(index)->getFormat() : NULL;
}

sp<MetaData> MPEG2TSExtractor::getMetaData() {
    sp<MetaData> meta = new MetaData;
    meta->setCString(kKeyMIMEType, MEDIA_MIMETYPE_CONTAINER_MPEG2TS);

    return meta;
}

void MPEG2TSExtractor::init() {
    bool haveAudio = false;
    bool haveVideo = false;

    int64_t  filesize;
    status_t ret = mDataSource->getSize(&filesize);
    if (ret == OK) {
        if (filesize%kTSPacketSize != 0) {
            ALOGW("mod(%lld,%d) !=0 -> maybe the ts file is recording or broken ts file!!!", filesize, kTSPacketSize);
        }
    } else {
        ALOGW("mDataSource->getSize fail!");
    }

    int numPacketsParsed = 0;
    // TS file has too much of the head.
    bool isNULLPacket = false;
    int64_t startTime = ALooper::GetNowUs();

    while (feedMore() == OK) {
        ATSParser::SourceType type;
        if (haveAudio && haveVideo) {
            break;
        }
        if (!haveVideo) {
            sp<AnotherPacketSource> impl =
                (AnotherPacketSource *)mParser->getSource(
                        ATSParser::VIDEO).get();

            if (impl != NULL) {
                haveVideo = true;
                mSourceImpls.push(impl);
                mSyncPoints.push();
                mSeekSyncPoints = &mSyncPoints.editTop();
            }
        }

        if (!haveAudio) {
            sp<AnotherPacketSource> impl =
                (AnotherPacketSource *)mParser->getSource(
                        ATSParser::AUDIO).get();

            if (impl != NULL) {
                haveAudio = true;
                mSourceImpls.push(impl);
                mSyncPoints.push();
                if (!haveVideo) {
                    mSeekSyncPoints = &mSyncPoints.editTop();
                }
            }
        }

        // Wait only for 2 seconds to detect audio/video streams. -> 1s
        if (ALooper::GetNowUs() - startTime > 1000000ll) {
            break;
        }
    }

    off64_t size;
    if (mDataSource->getSize(&size) == OK && (haveAudio || haveVideo)) {
        sp<AnotherPacketSource> impl = haveVideo
                ? (AnotherPacketSource *)mParser->getSource(
                        ATSParser::VIDEO).get()
                : (AnotherPacketSource *)mParser->getSource(
                        ATSParser::AUDIO).get();
        size_t prevSyncSize = 1;
        int64_t durationUs = -1;
        List<int64_t> durations;
        // Estimate duration --- stabilize until you get < 500ms deviation.
        while (feedMore() == OK
                && ALooper::GetNowUs() - startTime <= 1000000ll) {
            //ALOGD("init feedMore() == OK  && ALooper::GetNowUs() - startTime <= 2000000ll");
            if (mSeekSyncPoints->size() > prevSyncSize) {
                prevSyncSize = mSeekSyncPoints->size();
                int64_t diffUs = mSeekSyncPoints->keyAt(prevSyncSize - 1) - mSeekSyncPoints->keyAt(0);
                off64_t diffOffset = mSeekSyncPoints->valueAt(prevSyncSize - 1)
                        - mSeekSyncPoints->valueAt(0);
                durationUs = size * diffUs / diffOffset;
                durations.push_back(durationUs);
                if (durations.size() > 5) {
                    durations.erase(durations.begin());
                    int64_t min = *durations.begin();
                    int64_t max = *durations.begin();
                    for (List<int64_t>::iterator i = durations.begin();
                            i != durations.end();
                            ++i) {
                        if (min > *i) {
                            min = *i;
                        }
                        if (max < *i) {
                            max = *i;
                        }
                    }
                    if (max - min < 500 * 1000) {
                        ALOGI("max - min < 500 * 1000");
                        break;
                    }
                }
            }
        }
        status_t err;
        int64_t bufferedDurationUs;
        bufferedDurationUs = impl->getBufferedDurationUs(&err);
        ALOGD("init impl->getBufferedDurationUs(&err), bufferedDurationUs is %llu", bufferedDurationUs);
        if (err == ERROR_END_OF_STREAM) {
			ALOGD("init err == ERROR_END_OF_STREAM");
            durationUs = bufferedDurationUs;
        }
        if (durationUs > 0) {
			ALOGD("init durationUs is %llu", durationUs);
            const sp<MetaData> meta = impl->getFormat();
            meta->setInt64(kKeyDuration, durationUs);
            impl->setFormat(meta);
        }
    }
    ALOGD("haveAudio=%d, haveVideo=%d, cache 1s' SyncPoint elaspedTime=%lld",
            haveAudio, haveVideo, ALooper::GetNowUs() - startTime);
}

status_t MPEG2TSExtractor::feedMore() {
    Mutex::Autolock autoLock(mLock);

    uint8_t packet[kTSPacketSize];
    //int64_t start_read = systemTime() /1000ll;
    ssize_t n = mDataSource->readAt(mOffset, packet, kTSPacketSize);
    //int64_t end_read = systemTime() /1000ll;

    if (n < (ssize_t)kTSPacketSize) {
        if (n >= 0) {
            mParser->signalEOS(ERROR_END_OF_STREAM);
        }
        return (n < 0) ? (status_t)n : ERROR_END_OF_STREAM;
    }

    ATSParser::SyncEvent event(mOffset);
    mOffset += n;
    status_t err = mParser->feedTSPacket(packet, kTSPacketSize, &event);
    if (event.isInit()) {
        //ALOGD("feedMore event.isInit() is true");
        for (size_t i = 0; i < mSourceImpls.size(); ++i) {
            if (mSourceImpls[i].get() == event.getMediaSource().get()) {
                KeyedVector<int64_t, off64_t> *syncPoints = &mSyncPoints.editItemAt(i);
                syncPoints->add(event.getTimeUs(), event.getOffset());
                // We're keeping the size of the sync points at most 5mb per a track.
                size_t size = syncPoints->size();
                if (size >= 327680) {
                    int64_t firstTimeUs = syncPoints->keyAt(0);
                    int64_t lastTimeUs = syncPoints->keyAt(size - 1);
                    if (event.getTimeUs() - firstTimeUs > lastTimeUs - event.getTimeUs()) {
                        syncPoints->removeItemsAt(0, 4096);
                    } else {
                        syncPoints->removeItemsAt(size - 4096, 4096);
                    }
                }
                break;
            }
        }
    }
    return err;
}

uint32_t MPEG2TSExtractor::flags() const {
    return CAN_PAUSE | CAN_SEEK_BACKWARD | CAN_SEEK_FORWARD | CAN_SEEK;
}

status_t MPEG2TSExtractor::seek(int64_t seekTimeUs,
        const MediaSource::ReadOptions::SeekMode &seekMode) {
    ALOGD("seek seekTimeUs = %llu", seekTimeUs);

    if (mSeekSyncPoints == NULL || mSeekSyncPoints->isEmpty()) {
        ALOGD("No sync point to seek to.");
        // ... and therefore we have nothing useful to do here.
        return OK;
    }
    // Determine whether we're seeking beyond the known area.
    bool shouldSeekBeyond =
            (seekTimeUs > mSeekSyncPoints->keyAt(mSeekSyncPoints->size() - 1));

    // Determine the sync point to seek.
    size_t index = 0;
    for (index = 0; index < mSeekSyncPoints->size(); ++index) {
        int64_t timeUs = mSeekSyncPoints->keyAt(index);
        if (timeUs > seekTimeUs) {
            break;
        }
    }
    ALOGD("seek seekMode = %d", seekMode);
    switch (seekMode) {
        case MediaSource::ReadOptions::SEEK_NEXT_SYNC:
            if (index == mSeekSyncPoints->size()) {
                ALOGD("Next sync not found; starting from the latest sync.");
                --index;
            }
            break;
        case MediaSource::ReadOptions::SEEK_CLOSEST_SYNC:
        case MediaSource::ReadOptions::SEEK_CLOSEST:
            ALOGD("seekMode not supported: %d; falling back to PREVIOUS_SYNC",
                    seekMode);
            // fall-through
        case MediaSource::ReadOptions::SEEK_PREVIOUS_SYNC:
            if (index == 0) {
                ALOGW("Previous sync not found; starting from the earliest "
                        "sync.");
            } else {
                --index;
            }
            break;
    }
    if (!shouldSeekBeyond || mOffset <= mSeekSyncPoints->valueAt(index)) {
        ALOGD("seek !shouldSeekBeyond");
        int64_t actualSeekTimeUs = mSeekSyncPoints->keyAt(index);
        ALOGD("seek actualSeekTimeUs = %llu", actualSeekTimeUs);
        mOffset = mSeekSyncPoints->valueAt(index);
        status_t err = queueDiscontinuityForSeek(actualSeekTimeUs);
        if (err != OK) {
            ALOGD("seek queueDiscontinuityForSeek error");
            return err;
        }
    }

    if (shouldSeekBeyond) {
        ALOGD("seek shouldSeekBeyond true");
        status_t err = seekBeyond(seekTimeUs);
        if (err != OK) {
            ALOGE("seek seekBeyond err");
            return err;
        }
    }

    // Fast-forward to sync frame.
    for (size_t i = 0; i < mSourceImpls.size(); ++i) {
        const sp<AnotherPacketSource> &impl = mSourceImpls[i];
        status_t err;
        feedUntilBufferAvailable(impl);
        while (impl->hasBufferAvailable(&err)) {
            sp<AMessage> meta = impl->getMetaAfterLastDequeued(0);
            sp<ABuffer> buffer;
            if (meta == NULL) {
                ALOGE("seek meta == NULL");
                return UNKNOWN_ERROR;
            }
            int32_t sync;
            if (meta->findInt32("isSync", &sync) && sync) {
                ALOGD("seek isSync is true");
                break;
            }
            err = impl->dequeueAccessUnit(&buffer);
            if (err != OK) {
                ALOGD("seek dequeueAccessUnit err");
                return err;
            }
            feedUntilBufferAvailable(impl);
        }
    }

    return OK;
}

status_t MPEG2TSExtractor::queueDiscontinuityForSeek(int64_t actualSeekTimeUs) {
    ALOGD("queueDiscontinuityForSeek");
    // Signal discontinuity
    sp<AMessage> extra(new AMessage);
    // FIX me, for pass compile.
    //extra->setInt64(IStreamListener::kKeyMediaTimeUs, actualSeekTimeUs);
    mParser->signalDiscontinuity(ATSParser::DISCONTINUITY_TIME, extra);

    // After discontinuity, impl should only have discontinuities
    // with the last being what we queued. Dequeue them all here.
    for (size_t i = 0; i < mSourceImpls.size(); ++i) {
        const sp<AnotherPacketSource> &impl = mSourceImpls.itemAt(i);
        sp<ABuffer> buffer;
        status_t err;
        while (impl->hasBufferAvailable(&err)) {
            if (err != OK) {
                ALOGE("queueDiscontinuityForSeek hasBufferAvailable err");
                return err;
            }
            err = impl->dequeueAccessUnit(&buffer);
            // If the source contains anything but discontinuity, that's
            // a programming mistake.
            CHECK(err == INFO_DISCONTINUITY);
        }
    }

    // Feed until we have a buffer for each source.
    for (size_t i = 0; i < mSourceImpls.size(); ++i) {
        const sp<AnotherPacketSource> &impl = mSourceImpls.itemAt(i);
        sp<ABuffer> buffer;
        status_t err = feedUntilBufferAvailable(impl);
        if (err != OK) {
            ALOGE("queueDiscontinuityForSeek feedUntilBufferAvailable err");
            return err;
        }
    }

    return OK;
}

status_t MPEG2TSExtractor::seekBeyond(int64_t seekTimeUs) {
    // If we're seeking beyond where we know --- read until we reach there.
    size_t syncPointsSize = mSeekSyncPoints->size();

    while (seekTimeUs > mSeekSyncPoints->keyAt(mSeekSyncPoints->size() - 1)) {
        status_t err;
        if (syncPointsSize < mSeekSyncPoints->size()) {
            syncPointsSize = mSeekSyncPoints->size();
            int64_t syncTimeUs = mSeekSyncPoints->keyAt(syncPointsSize - 1);
            // Dequeue buffers before sync point in order to avoid too much
            // cache building up.
            sp<ABuffer> buffer;
            for (size_t i = 0; i < mSourceImpls.size(); ++i) {
                const sp<AnotherPacketSource> &impl = mSourceImpls[i];
                int64_t timeUs;
                while ((err = impl->nextBufferTime(&timeUs)) == OK) {
                    if (timeUs < syncTimeUs) {
                        impl->dequeueAccessUnit(&buffer);
                    } else {
                        break;
                    }
                }
                if (err != OK && err != -EWOULDBLOCK) {
                    return err;
                }
            }
        }
        if (feedMore() != OK) {
            return ERROR_END_OF_STREAM;
        }
    }

    return OK;
}

status_t MPEG2TSExtractor::feedUntilBufferAvailable(
        const sp<AnotherPacketSource> &impl) {
    //ALOGI("feedUntilBufferAvailable");
    status_t finalResult;
    while (!impl->hasBufferAvailable(&finalResult)) {
        if (finalResult != OK) {
            return finalResult;
        }

        status_t err = feedMore();
        if (err != OK) {
            ALOGE("feedUntilBufferAvailable feedMore err");
            impl->signalEOS(err);
        }
    }
    return OK;
}

////////////////////////////////////////////////////////////////////////////////

bool SniffMPEG2TS(
        const sp<DataSource> &source, String8 *mimeType, float *confidence,
        sp<AMessage> *) {
    for (int i = 0; i < 5; ++i) {
        char header;
        if (source->readAt(kTSPacketSize * i, &header, 1) != 1
                || header != 0x47) {
            return false;
        }
    }

    *confidence = 0.1f;
    mimeType->setTo(MEDIA_MIMETYPE_CONTAINER_MPEG2TS);

    return true;
}

uint64_t MPEG2TSExtractor::estimateDuration() {
    ALOGI("estimateDuration");
    Mutex::Autolock autoLock(mLock);
    int64_t filesize;
    int64_t end_time;
    int64_t  offset, duration;
    int retry = 1;
    status_t re = mDataSource->getSize(&filesize);
    ALOGI("estimateDuration filesize=%lld", filesize);
    if (re != OK) {
        ALOGE("estimateDuration Failed to get file size");
        return ERROR_MALFORMED;
    }
    uint8_t packet[kTSPacketSize];
    unsigned payload_unit_start_indicator = 0;
    unsigned PID = 0;
    unsigned adaptation_field_control = 0;
    while (1) {
        if (filesize%kTSPacketSize != 0) {
            ALOGW("need to fix incomplete ts file, real_size=%lld", filesize);
            filesize = (filesize/kTSPacketSize+1)*kTSPacketSize;
            ALOGW("after fix incomplete ts file, pretend_size=%lld", filesize);
            retry++;
        }
        offset = filesize - kTSPacketSize*retry;
        ssize_t n = mDataSource->readAt(offset, packet, kTSPacketSize);
        if (n < (ssize_t)kTSPacketSize) {
            ALOGW("EOF found! rd_size(%d) < kTSPacketSize(%d)", n, kTSPacketSize);
            return (n < 0) ? (status_t)n : ERROR_END_OF_STREAM;
        }
        ABitReader* br= new ABitReader((const uint8_t *)packet, kTSPacketSize);
        unsigned sync_byte = br->getBits(8);
        if (sync_byte != 0x47u) {
            retry++;
            delete br;
            ALOGW("file offset:%#llX still have wrong sync_byte(%#x)", offset, sync_byte);
            continue;
        }
        CHECK_EQ(sync_byte, 0x47u);
        br->skipBits(1);
        unsigned payload_unit_start_indicator = br->getBits(1);
        br->skipBits(1);
        PID = br->getBits(13);
        br->skipBits(2);
        adaptation_field_control = br->getBits(2);
        //if ((payload_unit_start_indicator == 1) && (adaptation_field_control == 1) && //LIJUNCHEN MODIFY
		if ((payload_unit_start_indicator == 1) && (PID != 0x00u) && (PID != 0x01u) && (PID != 0x02u) ) {
            ALOGI("estimateDuration payload_unit_start_indicator == 1");
            break;
        }
        retry++;
        delete br;
    } ;
    ABitReader* br= new ABitReader((const uint8_t *)packet, kTSPacketSize);
    br->skipBits(8 + 3 + 13);
    br->skipBits(2);
    adaptation_field_control = br->getBits(2);
    ALOGV("adaptation_field_control = %u", adaptation_field_control);
    br->skipBits(4);
    if (adaptation_field_control == 2 || adaptation_field_control == 3) {
       unsigned adaptation_field_length = br->getBits(8);
       if (adaptation_field_length > 0) {
           br->skipBits(adaptation_field_length * 8);
       }
       ALOGI("adaptation_field_length = %u", adaptation_field_length);
    }
    if (adaptation_field_control == 1 || adaptation_field_control == 3) {
        unsigned packet_startcode_prefix = br->getBits(24);

        ALOGI("packet_startcode_prefix = 0x%08x", packet_startcode_prefix);
        CHECK_EQ(packet_startcode_prefix, 0x000001u);

        unsigned stream_id = br->getBits(8);
        ALOGV("stream_id = 0x%02x", stream_id);
        br->skipBits(16);
        //以下可以参考标准，标准上解释的很详细，应该不难理解
        if (stream_id != 0xbc  // program_stream_map
                && stream_id != 0xbe  // padding_stream
                && stream_id != 0xbf  // private_stream_2
                && stream_id != 0xf0  // ECM
                && stream_id != 0xf1  // EMM
                && stream_id != 0xff  // program_stream_directory
                && stream_id != 0xf2  // DSMCC
                && stream_id != 0xf8) { // H.222.1 type E
            CHECK_EQ(br->getBits(2), 2u);
            br->skipBits(6);
            unsigned PTS_DTS_flags = br->getBits(2);
            ALOGV("PTS_DTS_flags = %u", PTS_DTS_flags);
            br->skipBits(6);

            unsigned PES_header_data_length = br->getBits(8);

            unsigned optional_bytes_remaining = PES_header_data_length;

            uint64_t PTS = 0, DTS = 0;

            if (PTS_DTS_flags == 2 || PTS_DTS_flags == 3){
                //CHECK_GE(optional_bytes_remaining, 5u);

                CHECK_EQ(br->getBits(4), PTS_DTS_flags);

                PTS = ((uint64_t)br->getBits(3)) << 30;
                CHECK_EQ(br->getBits(1), 1u);
                PTS |= ((uint64_t)br->getBits(15)) << 15;
                CHECK_EQ(br->getBits(1), 1u);
                PTS |= br->getBits(15);
                CHECK_EQ(br->getBits(1), 1u);

                ALOGI("PTS = %llu", PTS);
                ALOGI("PTS = %.2f secs", PTS / 90000.0f);

                PTS = (PTS * 1000 * 1000ll) / 90000;
                delete br;
                return PTS;
            }
        }
    }
    delete br;
    ALOGI("estimateDuration end");
    return 0;
}

}  // namespace Mercury
