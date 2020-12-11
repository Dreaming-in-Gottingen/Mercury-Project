#define LOG_TAG "KeyedVectorTest"

//#include <utils/Log.h>
//#include <utils/RefBase.h>
//#include <utils/String8.h>
//#include <utils/KeyedVector.h>
#include <Log.h>
#include <RefBase.h>
#include <String8.h>
#include <KeyedVector.h>

using namespace Mercury;

class Person: public RefBase {
public:
    Person() {
        ALOGD("Person ctor");
    }

    Person(String8 name) {
        this->name = name;
        ALOGD("Person ctor with \"%s\"", name.string());
    }

    ~Person() {
        ALOGD("Person dtor of \"%s\"", name.string());
    }

    void setName(String8 name) {
        this->name = name;
    }

    String8 getName() {
        return name;
    }

    void printName() {
        ALOGD("printName: %s", name.string());
    }

private:
    String8 name;
};


int main(void)
{
    puts("------------KeyedVectorTest begin---------------");

    KeyedVector<String8, sp<Person> > kvect;
    ALOGD("at beginning! size:%zd, isEmpyt:%d, capacity:%zd", kvect.size(), kvect.isEmpty(), kvect.capacity());

    sp<Person> xjp = new Person(String8("jinping.xi"));
    sp<Person> lkq = new Person(String8("keqiang.li"));
    sp<Person> lzs = new Person(String8("zhanshu.li"));
    sp<Person> wy = new Person(String8("yang.wang"));
    sp<Person> whn = new Person(String8("huning.wang"));
    sp<Person> zlj = new Person(String8("leji.zhao"));
    sp<Person> hz = new Person(String8("zheng.han"));

    int x0 = kvect.add(String8("xjp"), xjp);
    int x1 = kvect.add(String8("lkq"), lkq);
    int x2 = kvect.add(String8("lzs"), lzs);
    int x3 = kvect.add(String8("wy"), wy);
    int x4 = kvect.add(String8("whn"), whn);
    int x5 = kvect.add(String8("zlj"), zlj);
    int x6 = kvect.add(String8("hz"), hz);
    ALOGD("when insert, sequence: %d, %d, %d, %d, %d, %d, %d", x0, x1, x2, x3, x4, x5, x6);

    ALOGD("print rulers of China in 2019...");
    for (int i=0; i<kvect.size(); i++) {
        ALOGD("idx:[%d], key:[%3s], value(name):[%12s], strong_cnt:[%d]", i, kvect.keyAt(i).string(),
            kvect[i]->getName().string(), kvect.valueAt(i)->getStrongCount());
    }
    ALOGD("after add these rulers! size:%zd, isEmpyt:%d, capacity:%zd", kvect.size(), kvect.isEmpty(), kvect.capacity());

    ALOGD("---------------------------------------------------");

    ALOGD("test valueFor/indexOfKey begin");
    sp<Person> sp_vf = kvect.valueFor(String8("xjp"));
    if (sp_vf != NULL) {
        ALOGD("find key(\"xjp\") with value(\"name\"):%s, index:%zd", sp_vf->getName().string(), kvect.indexOfKey(String8("xjp")));
    } else {
        ALOGD("cann`t find ruler: key(\"xjp\")");
    }
    ALOGD("test valueFor/indexOfKey end");

    ALOGD("---------------------------------------------------");

    ALOGD("test editValueFor/editValueAt begin");
    sp<Person> bxl = new Person();
    bxl->setName(String8("xilai.bo"));
    kvect.add(String8("bxl"), bxl);
    ssize_t idx = kvect.indexOfKey(String8("bxl"));
    if (idx>=0) {
        ALOGD("find key(\"%s\") with value(\"%s\"), fate will be changed!", kvect.keyAt(idx).string(), kvect.valueAt(idx)->getName().string());
        sp<Person> tmp = kvect.editValueAt(idx);
        tmp->setName(String8("oops! xilai.bo was sent to QinCheng by someone in 2012!"));
    } else {
        ALOGW("where is xilai.bo?");
    }
    ALOGD("test editValueFor/editValueAt end");

    ALOGD("---------------------------------------------------");

    ALOGD("test replaceValueFor/replaceValueAt/removeItem/removeItemsAt begin");
    sp<Person> wqs = new Person();
    wqs->setName(String8("qishan.wang"));
    kvect.replaceValueAt(idx, wqs);            // in the key of idx, we replace it's value
    ALOGD("after replace bxl, idx:[%zd], key:[%3s], value(name):[%12s]", idx, kvect.keyAt(idx).string(), kvect.valueAt(idx)->getName().string());
    kvect.removeItemsAt(idx);
    ALOGD("test replaceValueFor/replaceValueAt/removeItem/removeItemsAt end");

    ALOGD("------------KeyedVectorTest end---------------");

    ALOGD("------------------------------------------------------------------------------------");

    ALOGD("------------DefaultKeyedVectorTest begin---------------");
    DefaultKeyedVector<String8, sp<Person> > dft_kv(new Person(String8("zedong.mao")));
    dft_kv.valueFor(String8("xxx"))->printName(); // return default "zedong.mao", because can not find key "xxx"
    dft_kv.add(String8("xjp"), xjp);
    dft_kv.valueFor(String8("xjp"))->printName(); // return real value, because the key "xjp" exist
    dft_kv.valueFor(String8("xyz"))->printName();
    puts("------------DefaultKeyedVectorTest end---------------");

    return 0;
}
