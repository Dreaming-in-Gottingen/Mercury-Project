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

#ifndef A_TS_PARSER_H_

#define A_TS_PARSER_H_

#include <sys/types.h>

#include <RefBase.h>
#include <Vector.h>
#include <KeyedVector.h>

#include <ABase.h>

#include <AMessage.h>

#include <MediaSource.h>

namespace Mercury {

struct ABitReader;
struct ABuffer;
struct MediaSource;

struct ATSParser : public RefBase {
    enum DiscontinuityType {
        DISCONTINUITY_NONE              = 0,
        DISCONTINUITY_TIME              = 1,
        DISCONTINUITY_AUDIO_FORMAT      = 2,
        DISCONTINUITY_VIDEO_FORMAT      = 4,
        DISCONTINUITY_ABSOLUTE_TIME     = 8,
        DISCONTINUITY_TIME_OFFSET       = 16,
        DISCONTINUITY_SEEK              = DISCONTINUITY_TIME,

        // For legacy reasons this also implies a time discontinuity.
        DISCONTINUITY_FORMATCHANGE      =
            DISCONTINUITY_AUDIO_FORMAT
                | DISCONTINUITY_VIDEO_FORMAT
                | DISCONTINUITY_TIME,
    };

    enum Flags {
        // The 90kHz clock (PTS/DTS) is absolute, i.e. PTS=0 corresponds to
        // a media time of 0.
        // If this flag is _not_ specified, the first PTS encountered in a
        // program of this stream will be assumed to correspond to media time 0
        // instead.
        TS_TIMESTAMPS_ARE_ABSOLUTE = 1,
        // Video PES packets contain exactly one (aligned) access unit.
        ALIGNED_VIDEO_DATA         = 2,
    };

    ATSParser(uint32_t flags = 0);

    // Event is used to signal sync point event at feedTSPacket().
    struct SyncEvent {
        SyncEvent(off64_t offset);

        void init(off64_t offset, const sp<MediaSource> &source,
                int64_t timeUs);

        bool isInit() { return mInit; }
        off64_t getOffset() { return mOffset; }
        const sp<MediaSource> &getMediaSource() { return mMediaSource; }
        int64_t getTimeUs() { return mTimeUs; }

    private:
        bool mInit;
        /*
         * mInit == false: the current offset
         * mInit == true: the start offset of sync payload
         */
        off64_t mOffset;
        /* The media source object for this event. */
        sp<MediaSource> mMediaSource;
        /* The timestamp of the sync frame. */
        int64_t mTimeUs;
    };
    status_t feedTSPacket(const void *data, size_t size);
    status_t feedTSPacket(const void *data, size_t size, SyncEvent *event);

    // TS file has too much of the head.
    bool isNULLPacket();

    void signalDiscontinuity(
            DiscontinuityType type, const sp<AMessage> &extra);

    void signalEOS(status_t finalResult);

    enum SourceType {
        VIDEO = 0,
        AUDIO = 1,
        NUM_SOURCE_TYPES = 2
    };
    sp<MediaSource> getSource(SourceType type);
    bool hasSource(SourceType type) const;

    bool PTSTimeDeltaEstablished();

    enum {
        // From ISO/IEC 13818-1: 2000 (E), Table 2-29
        STREAMTYPE_RESERVED             = 0x00,
        STREAMTYPE_MPEG1_VIDEO          = 0x01,
        STREAMTYPE_MPEG2_VIDEO          = 0x02,
        STREAMTYPE_MPEG1_AUDIO          = 0x03,
        STREAMTYPE_MPEG2_AUDIO          = 0x04,
        STREAMTYPE_MPEG2_AUDIO_ADTS     = 0x0f,
        STREAMTYPE_MPEG4_VIDEO          = 0x10,
        STREAMTYPE_H264                 = 0x1b,
        STREAMTYPE_PCM_AUDIO            = 0x83,
    };

protected:
    virtual ~ATSParser();

private:
    struct Program;
    struct Stream;
    struct PSISection;

    uint32_t mFlags;
    Vector<sp<Program> > mPrograms;

    // Keyed by PID
    KeyedVector<unsigned, sp<PSISection> > mPSISections;

    int64_t mAbsoluteTimeAnchorUs;

    bool mTimeOffsetValid;
    int64_t mTimeOffsetUs;

    size_t mNumTSPacketsParsed;

    void parseProgramAssociationTable(ABitReader *br);
    void parseProgramMap(ABitReader *br);
    void parsePES(ABitReader *br);
    // Parse PES packet where br is pointing to. If the PES contains a sync
    // frame, set event with the time and the start offset of this PES.
    // Note that the method itself does not touch event.
    void parsePES(ABitReader *br, SyncEvent *event);

    status_t parsePID(
        ABitReader *br, unsigned PID,
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator);

    // Strip remaining packet headers and pass to appropriate program/stream
    // to parse the payload. If the payload turns out to be PES and contains
    // a sync frame, event shall be set with the time and start offset of the
    // PES.
    // Note that the method itself does not touch event.
    status_t parsePID(
        ABitReader *br, unsigned PID,
        unsigned continuity_counter,
        unsigned payload_unit_start_indicator,
        SyncEvent *event);

    void parseAdaptationField(ABitReader *br, unsigned PID);
    // see feedTSPacket().
    status_t parseTS(ABitReader *br);
    status_t parseTS(ABitReader *br, SyncEvent *event);

    void updatePCR(unsigned PID, uint64_t PCR, size_t byteOffsetFromStart);

    uint64_t mPCR[2];
    size_t mPCRBytes[2];
    int64_t mSystemTimeUs[2];
    size_t mNumPCRs;

    DISALLOW_EVIL_CONSTRUCTORS(ATSParser);
};

}  // namespace Mercury

#endif  // A_TS_PARSER_H_
