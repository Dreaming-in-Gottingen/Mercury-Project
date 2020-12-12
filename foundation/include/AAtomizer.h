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

#ifndef _FOUNDATION_A_ATOMIZER_H_

#define _FOUNDATION_A_ATOMIZER_H_

#include <stdint.h>

//#include <media/stagefright/foundation/ABase.h>
//#include <media/stagefright/foundation/AString.h>
#include <ABase.h>
#include <AString.h>

//#include <utils/List.h>
//#include <utils/Vector.h>
//#include <utils/threads.h>
#include <List.h>
#include <Vector.h>
#include <Mutex.h>
//#include <threads.h>

namespace Mercury {

struct AAtomizer {
    static const char *Atomize(const char *name);

private:
    static AAtomizer gAtomizer;

    Mutex mLock;
    Vector<List<AString> > mAtoms;

    AAtomizer();

    const char *atomize(const char *name);

    static uint32_t Hash(const char *s);

    DISALLOW_EVIL_CONSTRUCTORS(AAtomizer);
};

}  // namespace Mercury

#endif  // _FOUNDATION_A_ATOMIZER_H_
