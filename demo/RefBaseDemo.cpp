#include <stdio.h>

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

int main(void)
{
    puts("----------------RefBaseDemo begin----------------");

    sp<MyClass> mc0 = new MyClass();
    sp<MyClass> mc1 = mc0;
    printf("0000 mc0 ref_cnt:%d\n", mc1->getStrongCount());
    mc1->incStrong(NULL);
    printf("1111 mc0 ref_cnt:%d\n", mc1->getStrongCount());
    mc1->decStrong(NULL);
    printf("2222 mc0 ref_cnt:%d\n", mc1->getStrongCount());

    puts("----------------RefBaseDemo end----------------");

    return 0;
}
