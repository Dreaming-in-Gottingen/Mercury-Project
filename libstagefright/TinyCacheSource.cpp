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

#include <sys/types.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <ADebug.h>

#include <TinyCacheSource.h>

namespace Mercury {

TinyCacheSource::TinyCacheSource(const sp<DataSource>& source)
    : mSource(source), mCachedOffset(0), mCachedSize(0) {
}

status_t TinyCacheSource::initCheck() const {
    return mSource->initCheck();
}

ssize_t TinyCacheSource::readAt(off64_t offset, void* data, size_t size) {
    // Check if the cache satisfies the read.
    if ( (mCachedOffset <= offset) && (offset < (off64_t) (mCachedOffset + mCachedSize)) )
    {
        if (offset + size <= mCachedOffset + mCachedSize) {
            memcpy(data, &mCache[offset - mCachedOffset], size);
            return size;
        } else {
            // If the cache hits only partially, flush the cache and read the
            // remainder.

            // This value is guaranteed to be greater than 0 because of the
            // enclosing if statement.
            const ssize_t remaining = mCachedOffset + mCachedSize - offset;
            memcpy(data, &mCache[offset - mCachedOffset], remaining);
            const ssize_t readMore = readAt(offset + remaining,
                    (uint8_t*)data + remaining, size - remaining);
            if (readMore < 0) {
                return readMore;
            }
            return remaining + readMore;
        }
    }

    if (size >= kCacheSize) {
        return mSource->readAt(offset, data, size);
    }

    // Fill the cache and copy to the caller.
    const ssize_t numRead = mSource->readAt(offset, mCache, kCacheSize);
    if (numRead <= 0) {
        // Flush cache on error
        mCachedSize = 0;
        mCachedOffset = 0;
        return numRead;
    }
    if ((size_t)numRead > kCacheSize) {
        // Flush cache on error
        mCachedSize = 0;
        mCachedOffset = 0;
        return ERROR_OUT_OF_RANGE;
    }
    mCachedSize = numRead;
    mCachedOffset = offset;
    //CHECK(mCachedSize <= kCacheSize && mCachedOffset >= 0);
    const size_t numToReturn = (size < numRead)? size:numRead;
    memcpy(data, mCache, numToReturn);

    return numToReturn;
}

status_t TinyCacheSource::getSize(off64_t *size) {
    return mSource->getSize(size);
}

uint32_t TinyCacheSource::flags() {
    return mSource->flags();
}

}  // namespace Mercury
