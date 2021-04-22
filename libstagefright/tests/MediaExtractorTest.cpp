//#define LOG_NDEBUG
#define LOG_LEVEL 3
#define LOG_TAG "MediaExtractorTest"
#include <Log.h>

#include <RefBase.h>
#include <Errors.h>

#include <ABuffer.h>
#include <AMessage.h>

#include <Utils.h>
#include <MetaData.h>
#include <MediaBuffer.h>
#include <MediaDefs.h>
#include <MediaSource.h>
#include <DataSource.h>
#include <FileSource.h>
#include <TinyCacheSource.h>
#include <MediaExtractor.h>


using namespace Mercury;
int main()
{
    ALOGI("---------------MediaExtractorTest begin---------------");

    const char *input_path = "input.mp4";
    const char *output_path = "out_naked_bs.h264";
    ALOGD("Usage: extract in_file(%s)'s video track to out_file(%s).", input_path, output_path);

    DataSource::RegisterDefaultSniffers();
    sp<FileSource> file_src = new FileSource(input_path);

    // add TinyCacheSource to accelerate file access.
    sp<TinyCacheSource> cache_src = new TinyCacheSource(file_src);
    sp<MediaExtractor> extractor = MediaExtractor::Create(cache_src);
    if (extractor == NULL) {
        ALOGW("1.(container not support) or 2.(file not exist) happen for in_file(%s)! exit!!!", input_path);
        exit(1);
    }

    ALOGD("-------------File MetaData-------------");
    size_t track_cnt = extractor->countTracks();
    ALOGD("track_cnt=%d", track_cnt);
    sp<MetaData> meta = extractor->getMetaData();
    meta->dumpToLog();

    ALOGD("-------------Track MetaData-------------");
    sp<AMessage> fmt_msg = new AMessage();
    int video_track_idx = 0;
    for (int i=0; i<track_cnt; i++) {
        ALOGD("------track_idx=%d------", i);
        meta = extractor->getTrackMetaData(i);
        meta->dumpToLog();
        convertMetaDataToMessage(meta, &fmt_msg);
        ALOGD("dump format: %s", fmt_msg->debugString().c_str());
        const char *mime;
        meta->findCString(kKeyMIMEType, &mime);
        if (!strncmp(mime, "video/avc", strlen(mime))) {
            video_track_idx = i;
        }
    }

    ALOGD("-------------extract video bitstream-------------");
    FILE *output_fp = fopen(output_path, "wb");
    meta = extractor->getTrackMetaData(video_track_idx);
    convertMetaDataToMessage(meta, &fmt_msg);
    size_t i = 0;
    for (;;) {
        sp<ABuffer> csd;
        if (!fmt_msg->findBuffer(StringPrintf("csd-%u", i).c_str(), &csd)) {
            break;
        }
        fwrite(csd->data(), 1, csd->size(), output_fp);
        ALOGD("csd-%d: len=%d", i++, csd->size());
    }

    sp<MediaSource> source = extractor->getTrack(0);
    source->start();
    MediaBuffer *buffer;
    i = 0;
    while (true) {
        status_t err = source->read(&buffer, NULL);
        if (err != OK) {
            ALOGW("read failed! err=%d, maybe EOS!", err);
            break;
        }
        int32_t syncFlag = 0;
        int64_t ptsUs;
        bool findFlag;
        findFlag = buffer->meta_data()->findInt32(kKeyIsSyncFrame, &syncFlag);
        findFlag = buffer->meta_data()->findInt64(kKeyTime, &ptsUs);
        ALOGD("[sample_idx=%d] bs_len=%d, sync_flag=%d, pts_ms=%lld ms", i++, buffer->range_length(), syncFlag, ptsUs/1000);
        fwrite(buffer->data()+buffer->range_offset(), 1, buffer->range_length(), output_fp);
        buffer->release();
    }
    fclose(output_fp);

    ALOGI("---------------MediaExtractorTest end---------------");
}
