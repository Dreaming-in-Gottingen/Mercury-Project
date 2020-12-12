#define LOG_TAG "RefBaseDemo"

#include <stdio.h>

#include <Log.h>
#include <RefBase.h>

using namespace Mercury;

class MyClass : public RefBase {
public:
    MyClass(void) {
        puts("MyClass ctor!");
    }

    ~MyClass(void) {
        puts("MyClass dtor!");
    }
};

void testIncAndDec()
{
    sp<MyClass> mc0 = new MyClass();
    sp<MyClass> mc1 = mc0;
    printf("0000 mc0 ref_cnt:%d\n", mc1->getStrongCount());
    mc1->incStrong(NULL);
    printf("1111 mc0 ref_cnt:%d\n", mc1->getStrongCount());
    mc1->decStrong(NULL);
    printf("2222 mc0 ref_cnt:%d\n", mc1->getStrongCount());
}

class AClass;
class BClass : public RefBase {
public:
    BClass() {
        puts("BClass ctor!");
    }
    ~BClass() {
        puts("BClass dtor!");
    }

    void refStrongObj(sp<AClass> &ref) {
        mSpAc = ref;
    }

private:
    sp<AClass> mSpAc;
};

class AClass : public RefBase {
public:
    AClass() {
        puts("AClass ctor!");
    }
    ~AClass() {
        puts("AClass dtor!");
    }

    void refWeakObj(sp<BClass> &ref) {
        mWpBc = ref;
    }

    void promoteTest() {
        sp<BClass> sp_tmp = mWpBc.promote();
        if (sp_tmp != NULL) {
            ALOGD("promote success! mWpBc. strongCnt=%d, weakCnt=%d", sp_tmp->getStrongCount(), sp_tmp->getWeakRefs()->getWeakCount());
        } else {
            ALOGD("promote fail!");
        }
    }

private:
    wp<BClass> mWpBc;
};

void testPromote()
{
    wp<AClass> wp_ac = new AClass();
    sp<AClass> sp_ac = wp_ac.promote();
    if (sp_ac != NULL) {
        ALOGD("promote success!");
    } else {
        ALOGD("promote fail!");
    }
}

void testMutualRef()
{
    sp<AClass> sp_ac = new AClass();
    sp<BClass> sp_bc = new BClass();

    ALOGD("0000. sp_ac strongCnt=%d, weakCnt=%d", sp_ac->getStrongCount(), sp_ac->getWeakRefs()->getWeakCount());
    ALOGD("0000. sp_bc strongCnt=%d, weakCnt=%d", sp_bc->getStrongCount(), sp_bc->getWeakRefs()->getWeakCount());
    sp_ac->promoteTest();

    sp_bc->refStrongObj(sp_ac);
    ALOGD("1111. sp_ac strongCnt=%d, weakCnt=%d", sp_ac->getStrongCount(), sp_ac->getWeakRefs()->getWeakCount());
    ALOGD("1111. sp_bc strongCnt=%d, weakCnt=%d", sp_bc->getStrongCount(), sp_bc->getWeakRefs()->getWeakCount());
    sp_ac->promoteTest();

    sp_ac->refWeakObj(sp_bc);
    ALOGD("2222. sp_ac strongCnt=%d, weakCnt=%d", sp_ac->getStrongCount(), sp_ac->getWeakRefs()->getWeakCount());
    ALOGD("2222. sp_bc strongCnt=%d, weakCnt=%d", sp_bc->getStrongCount(), sp_bc->getWeakRefs()->getWeakCount());
    sp_ac->promoteTest();
}

int main(void)
{
    puts("----------------RefBaseDemo begin----------------");

    puts("---------------testIncAndDec...---------------");
    testIncAndDec();
    puts("---------------testPromote...---------------");
    testPromote();
    puts("---------------testMutualRef...---------------");
    testMutualRef();

    puts("----------------RefBaseDemo end----------------");

    return 0;
}
