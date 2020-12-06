//#define LOG_NDEBUG
#define LOG_TAG "ListDemo"

#include <stdio.h>

#include <List.h>
#include <Log.h>

using namespace Mercury;

typedef struct TmpData {
    int x;
    char c;
    const char *ptr;
} TmpData;

class TmpClass {
public:
    TmpClass() {
        puts("ctor");
    }
    ~TmpClass() {
        puts("dtor");
    }
private:
    int x;
};

int main(void)
{
    printf("-----------ListDemo begin------------\n");

    List<TmpData*> listTmpData;
    ALOGD("00. size=%zu, empty=%d", listTmpData.size(), listTmpData.empty());

    listTmpData.push_back(new TmpData);
    List<TmpData*>::iterator it = listTmpData.begin();
    (*it)->x = 11;
    (*it)->c = 'A';
    (*it)->ptr = "1111";
    ALOGD("11. size=%zu, empty=%d", listTmpData.size(), listTmpData.empty());

    TmpData *pTmp = new TmpData;
    pTmp->x = 22;
    pTmp->c = 'B';
    pTmp->ptr = "2222";
    listTmpData.push_back(pTmp);
    ALOGD("22. size=%zu, empty=%d", listTmpData.size(), listTmpData.empty());

    pTmp = new TmpData;
    pTmp->x = 33;
    pTmp->c = 'C';
    pTmp->ptr = "3333";
    listTmpData.insert(listTmpData.end(), pTmp);
    ALOGD("33. size=%zu, empty=%d", listTmpData.size(), listTmpData.empty());

    ALOGD("dump all the nodes...");
    for (it=listTmpData.begin(); it!=listTmpData.end(); it++) {
        ALOGD("index[&(*it):%p, (*it):%p] (%d,%c,%s)", &(*it), (*it), (*it)->x, (*it)->c, (*it)->ptr);
        // careful!!! do not forget delete! otherwise will memory leak!
        delete (*it);
    }

    listTmpData.clear();
    ALOGD("after clear. size=%zu, empty=%d", listTmpData.size(), listTmpData.empty());

    puts("----------------------------------------------------------------------------------");

    List<TmpClass*> listTmpClass;
    ALOGD("00. size=%zu, empty=%d", listTmpClass.size(), listTmpClass.empty());

    listTmpClass.push_back(new TmpClass);
    ALOGD("11. size=%zu, empty=%d", listTmpClass.size(), listTmpClass.empty());

    // careful!!! do not forget delete! otherwise will memory leak!
    // because List's contents is in heap space: new _Node(val) when insert().
    for (List<TmpClass*>::iterator it=listTmpClass.begin(); it!=listTmpClass.end(); it++) {
        delete (*it);
    }

    listTmpClass.clear();
    ALOGD("22. size=%zu, empty=%d", listTmpClass.size(), listTmpClass.empty());

    puts("----------------------------------------------------------------------------------");

    List<TmpClass> listTmpClass2;
    ALOGD("00. size=%zu, empty=%d", listTmpClass2.size(), listTmpClass2.empty());

    TmpClass tc0;
    TmpClass tc1;
    listTmpClass2.push_back(tc0);
    ALOGD("11. size=%zu, empty=%d", listTmpClass2.size(), listTmpClass2.empty());

    listTmpClass2.push_back(tc1);
    ALOGD("22. size=%zu, empty=%d", listTmpClass2.size(), listTmpClass2.empty());

    // &listTmpClass2, &tc: stack addr, decrease(for most platform)
    // &(*it) heap addr: heap addr, increase
    ALOGD("stack addr test: &list=%p, &tc0=%p, &tc1=%p", &listTmpClass2, &tc0, &tc1);
    List<TmpClass>::iterator it_tc = listTmpClass2.begin();
    ALOGD("heap addr test: &(*it)=%p", &(*it_tc));
    it_tc++;
    ALOGD("heap addr test: &(*(it+1))=%p", &(*it_tc));
    it_tc++;
    ALOGD("heap addr test: &(*(it+2))=%p", &(*it_tc));
    // for stack val(tc, listTmpClass2), memory collect will happen after program exit.

    printf("-----------ListDemo end------------\n");

    return 0;
}
