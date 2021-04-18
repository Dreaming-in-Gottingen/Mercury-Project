/*
 * Copyright (C) 2009 The Android Open Source Project
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

#ifndef TINY_CACHE_SOURCE_H_

#define TINY_CACHE_SOURCE_H_

#include <stdio.h>

#include <MediaErrors.h>
#include <DataSource.h>
//#include <utils/threads.h>

namespace Mercury {

class TinyCacheSource : public DataSource {
public:
        TinyCacheSource(const sp<DataSource>& source);

        virtual status_t initCheck() const;
        virtual ssize_t readAt(off64_t offset, void* data, size_t size);
        virtual status_t getSize(off64_t* size);
        virtual uint32_t flags();

private:
        // 2kb comes from experimenting with the time-to-first-frame from a MediaPlayer
        // with an in-memory MediaDataSource source on a Nexus 5. Beyond 2kb there was
        // no improvement.
        // when kCacheSize exceed 4KB, no improvement found when test.
        enum {
            kCacheSize = 4096,
        };

        sp<DataSource> mSource;
        uint8_t mCache[kCacheSize];
        off64_t mCachedOffset;
        size_t mCachedSize;

        DISALLOW_EVIL_CONSTRUCTORS(TinyCacheSource);
};

}  // namespace Mercury

#endif  // TINY_CACHE_SOURCE_H_
