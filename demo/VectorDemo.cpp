//#define LOG_NDEBUG
#define LOG_TAG "VectorDemo"

#include <stdio.h>

#include <Log.h>
#include <Vector.h>

using namespace Mercury;

typedef struct TmpData {
    int x;
    char c;
    const char *ptr;
} TmpData;

class TmpClass {
public:
    TmpClass() {
        printf("ctor:%p\n", this);
    }
    ~TmpClass() {
        printf("dtor:%p\n", this);
    }
private:
    int x;
};

int main(void)
{
    printf("-----------VectorDemo begin------------\n");

    Vector<TmpData*> vectorTmpData;
    ALOGD("00. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());

    TmpData* pTD = new TmpData;
    vectorTmpData.push(pTD);
    ALOGD("11. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());
    pTD = vectorTmpData.editTop();
    pTD->x = 11;
    pTD->c = 'A';
    pTD->ptr = "1111";

    vectorTmpData.push(new TmpData);
    ALOGD("22. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());
    //pTD = vectorTmpData[2]; // will assert!
    pTD = vectorTmpData[vectorTmpData.size()-1];
    pTD->x = 22;
    pTD->c = 'B';
    pTD->ptr = "2222";

    vectorTmpData.push(new TmpData);
    ALOGD("33. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());
    pTD = vectorTmpData[vectorTmpData.size()-1];
    pTD->x = 33;
    pTD->c = 'C';
    pTD->ptr = "3333";

    //vectorTmpData.push(); // when node is pointer, can not use push()!
    //ALOGD("33. sizeof=%d, size=%zu, capacity=%zu, empty=%d", sizeof(vectorTmpData), vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());
    //ALOGD("%p, %p, %p, %p", vectorTmpData.editItemAt(0), vectorTmpData.editItemAt(1), vectorTmpData.editItemAt(2), vectorTmpData.editItemAt(3));

    Vector<TmpData*>::iterator it = vectorTmpData.begin();
    for (; it!= vectorTmpData.end(); it++) {
        ALOGD("index[&(*it):%p, (*it):%p] (%d,%c,%s)", &(*it), (*it), (*it)->x, (*it)->c, (*it)->ptr);
    }

    delete(vectorTmpData[0]);
    vectorTmpData.removeAt(0);
    ALOGD("44. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());
    for (it=vectorTmpData.begin(); it!= vectorTmpData.end(); it++) {
        ALOGD("index[&(*it):%p, (*it):%p] (%d,%c,%s)", &(*it), (*it), (*it)->x, (*it)->c, (*it)->ptr);
    }

    // remove all nodes -> coredump
    // for it increase throught the vector whenever remove one.
    //for (it=vectorTmpData.begin(); it!= vectorTmpData.end(); it++) {
    //    delete(*it);
    //    vectorTmpData.removeAt(0);
    //}
    // remove one node
    for (int i=0; i<vectorTmpData.size(); i++) {
        if (pTD == vectorTmpData.editItemAt(i)) {
            delete(pTD);
            vectorTmpData.removeAt(i);
        }
    }
    ALOGD("55. after remove one node. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());

    for (it=vectorTmpData.begin(); it!= vectorTmpData.end(); it++) {
        ALOGD("index[&(*it):%p, (*it):%p] (%d,%c,%s)", &(*it), (*it), (*it)->x, (*it)->c, (*it)->ptr);
    }

    ALOGD("delete all nodes...");
    // delete all nodes
    for (int i=0; i<vectorTmpData.size(); i++) {
        pTD = vectorTmpData.editItemAt(i);
        delete(pTD);
    }
    ALOGD("dump all nodes info(unreadable, because of delete)...");
    for (it=vectorTmpData.begin(); it!= vectorTmpData.end(); it++) {
        ALOGD("index[&(*it):%p, (*it):%p] (%d,%c,%s)", &(*it), (*it), (*it)->x, (*it)->c, (*it)->ptr);
    }
    ALOGD("remove all nodes...");
    // remove all nodes
    for (int i=0; i<vectorTmpData.size(); i++) {
        vectorTmpData.removeAt(i);
    }

    ALOGD("66. after delete and remove all nodes. size=%zu, capacity=%zu, empty=%d", vectorTmpData.size(), vectorTmpData.capacity(), vectorTmpData.empty());

    puts("----------------------------------------------------------------------------------");

    Vector<TmpClass*> vectorTmpClass;
    ALOGD("00. size=%zu, empty=%d", vectorTmpClass.size(), vectorTmpClass.empty());

    vectorTmpClass.push_back(new TmpClass);
    ALOGD("11. size=%zu, empty=%d", vectorTmpClass.size(), vectorTmpClass.empty());

    // careful!!! do not forget delete! otherwise will memory leak!
    // because List's contents is in heap space: new _Node(val) when insert().
    for (Vector<TmpClass*>::iterator it=vectorTmpClass.begin(); it!=vectorTmpClass.end(); it++) {
        delete(*it);
    }
    vectorTmpClass.clear();

    puts("----------------------------------------------------------------------------------");

    Vector<TmpClass> vectorTmpClass2;
    ALOGD("00. size=%zu, empty=%d", vectorTmpClass2.size(), vectorTmpClass2.empty());

    TmpClass tc0;
    vectorTmpClass2.push_back(tc0);
    ALOGD("11. size=%zu, empty=%d", vectorTmpClass2.size(), vectorTmpClass2.empty());

    Vector<TmpClass>::iterator it_tc = vectorTmpClass2.begin();
    for (; it_tc!= vectorTmpClass2.end(); it_tc++) {
        ALOGD("index[&(*it):%p, (*it):%p], &tc0:%p", &(*it), (*it), &tc0);
    }

    TmpClass tc1;
    vectorTmpClass2.push_back(tc1);
    ALOGD("22. size=%zu, empty=%d", vectorTmpClass2.size(), vectorTmpClass2.empty());

    // &vectorTmpClass2, &tc: stack addr, decrease(for most platform)
    // &(*it) heap addr: heap addr, increase
    ALOGD("stack/heap addr test...");
    ALOGD("stack addr test: &vector=%p, &tc0=%p, &tc1=%p", &vectorTmpClass2, &tc0, &tc1);
    Vector<TmpClass>::iterator it_tc2 = vectorTmpClass2.begin();
    ALOGD("heap addr test: &(*it)=%p", &(*it_tc2));
    it_tc2++;
    ALOGD("heap addr test: &(*(it+1))=%p", &(*it_tc2));
    it_tc2++;
    ALOGD("heap addr test: &(*(it+2))=%p", &(*it_tc2));

    ALOGD("clear vecotr...");
    vectorTmpClass2.clear();
    ALOGD("33. size=%zu, empty=%d", vectorTmpClass2.size(), vectorTmpClass2.empty());

    printf("-----------VectorDemo end------------\n");

    return 0;
}
