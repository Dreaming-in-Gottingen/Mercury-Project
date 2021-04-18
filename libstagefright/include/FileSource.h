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

#ifndef FILE_SOURCE_H_

#define FILE_SOURCE_H_

#include <stdio.h>

#include <Mutex.h>

#include <MediaErrors.h>
#include <DataSource.h>
//#include <utils/threads.h>
//#include <drm/DrmManagerClient.h>

namespace Mercury {

class FileSource : public DataSource {
public:
    FileSource(const char *filename);
    FileSource(int fd, int64_t offset, int64_t length);

    virtual status_t initCheck() const;

    virtual ssize_t readAt(off64_t offset, void *data, size_t size);

    virtual status_t getSize(off64_t *size);

    //virtual sp<DecryptHandle> DrmInitialization(const char *mime);
    //virtual void getDrmInfo(sp<DecryptHandle> &handle, DrmManagerClient **client);

    virtual void getFd(int *fd,int64_t *offset) ;

protected:
    virtual ~FileSource();

private:
    int mFd;
    int64_t mOffset;
    int64_t mLength;
    Mutex mLock;

    /*for DRM*/
    //sp<DecryptHandle> mDecryptHandle;
    //DrmManagerClient *mDrmManagerClient;
    //int64_t mDrmBufOffset;
    //int64_t mDrmBufSize;
    //unsigned char *mDrmBuf;
    //ssize_t readAtDRM(off64_t offset, void *data, size_t size);

    FileSource(const FileSource &);
    FileSource &operator=(const FileSource &);
};

}  // namespace Mercury

#endif  // FILE_SOURCE_H_

