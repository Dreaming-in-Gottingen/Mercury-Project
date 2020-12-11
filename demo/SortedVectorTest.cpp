#define LOG_TAG "SortedVectorTest"

//#include <utils/Log.h>
//#include <utils/String8.h>
//#include <utils/SortedVector.h>
#include <Log.h>
#include <String8.h>
#include <SortedVector.h>

using namespace Mercury;

int main(void)
{
    ALOGI("------------SortedVectorTest begin---------------");

    SortedVector<String8> svec_ruler;

    String8 mzd = String8("zedong.mao");
    String8 zel = String8("enlai.zhou");
    String8 lsq = String8("shaoqi.liu");
    String8 zd = String8("de.zhu");

    svec_ruler.add(mzd);
    svec_ruler.add(zel);
    svec_ruler.add(lsq);
    svec_ruler.add(zd);

    //C-style access
    ALOGD("-------test access--------");
    String8 const *pRuler = svec_ruler.array();
    ssize_t idx = svec_ruler.indexOf(mzd);
    ALOGD("mzd's index: %zd", idx);

    //accessors
    ALOGD("-------test accessors--------");
    ALOGD("rulers:");
    for (idx=0; idx<svec_ruler.size(); idx++) {
        ALOGD("\tidx:%zd, name: %s", idx, svec_ruler[idx].string());
    }
    ALOGD("top of ruluer is %s", svec_ruler.top().string());

    //modify
    ALOGD("-------test modify--------");
    String8 lb = String8("biao.lin");
    svec_ruler.add(lb);

    //another SortedVecotor
    SortedVector<String8> svec_successors;
    String8 hgf = String8("guofeng.hua");
    String8 lxn = String8("xiannian.li");
    String8 zzy = String8("ziyang.zhao");
    String8 dxp = String8("xiaoping.deng");
    svec_successors.add(hgf);
    svec_successors.add(lxn);
    svec_successors.add(zzy);
    svec_successors.add(dxp);
    ALOGD("successors:");
    for (idx=0; idx<svec_successors.size(); idx++) {
        ALOGD("\tidx:%zd, name: %s", idx, svec_successors[idx].string());
    }
    ALOGD("top of successors is %s", svec_successors.top().string());

    //merge
    ALOGD("-------test merge and dump all--------");
    svec_ruler.merge(svec_successors);
    for (idx=0; idx<svec_ruler.size(); idx++) {
        ALOGD("\tidx:%zd, name: %s", idx, svec_ruler[idx].string());
    }

    ALOGD("-------SortedVector status--------");
    ALOGD("ruler status => size:%zd, capacity:%zd", svec_ruler.size(), svec_ruler.capacity());
    ALOGD("successors status => size:%zu, capacity:%zd", svec_successors.size(), svec_successors.capacity());

    ALOGI("------------SortedVectorTest end---------------");

    return 0;
}
