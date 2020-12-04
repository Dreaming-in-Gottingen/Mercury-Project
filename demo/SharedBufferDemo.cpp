#define LOG_TAG "SharedBufferDemo"

#include <stdio.h>

#include <RefBase.h>
#include <SharedBuffer.h>

using namespace Mercury;

int main(void)
{
    printf("-------------SharedBufferDemo begin--------------\n");

    SharedBuffer *psb = SharedBuffer::alloc(4096);
    strcpy((char*)psb->data(), "SharedBufferDemo alloc 4096 Bytes!");

    printf("size=%u, data=(%p, %s)\n", psb->size(), psb->data(), (char*)psb->data());

    psb->release();

    printf("-------------SharedBufferDemo end--------------\n");

    return 0;
}
