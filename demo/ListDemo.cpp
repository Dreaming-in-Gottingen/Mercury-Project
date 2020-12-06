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
        ALOGD("[idx:%p] (%d,%c,%s)", (*it), (*it)->x, (*it)->c, (*it)->ptr);
    }

    listTmpData.clear();
    ALOGD("after clear. size=%zu, empty=%d", listTmpData.size(), listTmpData.empty());

    printf("-----------ListDemo end------------\n");

    return 0;
}
