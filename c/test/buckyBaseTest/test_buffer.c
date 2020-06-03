#include "../src/BuckyBase/BuckyBase.h"


void testBuffer() {

    BFX_BUFFER_HANDLE buffer1 = BfxCreateBuffer(1024);

    size_t len;
    uint8_t* data = BfxBufferGetData(buffer1, &len);
    assert(len == 1024);
    for (size_t index = 0; index < 1024; ++index) {
        data[index] = (uint8_t)(index % 256);
    }

    {
        BFX_BUFFER_HANDLE buffer2 = BfxCreateBindBuffer(data, 256);
        size_t len;
        uint8_t* data = BfxBufferGetData(buffer2, &len);
        assert(len == 256);
        for (size_t index = 0; index < 256; ++index) {
            BLOG_CHECK(data[index] == (uint8_t)index);
        }

        uint8_t data2[256];
        size_t ret = BfxBufferRead(buffer2, 128, 128, data2, 0);
        assert(ret == 128);
        for (size_t index = 0; index < 128; ++index) {
            BLOG_CHECK(data2[index] == (uint8_t)index + 128);
        }
    }

    {
        BFX_BUFFER_HANDLE buffer2 = BfxClipBuffer(buffer1, 0, 256);
        size_t len;
        uint8_t* data = BfxBufferGetData(buffer2, &len);
        assert(len == 256);
        for (size_t index = 0; index < 256; ++index) {
            BLOG_CHECK(data[index] == (uint8_t)index);
        }

        const int32_t ret = BfxBufferRelease(buffer2);
        BLOG_CHECK(ret == 0);
    }


    const int32_t ret = BfxBufferRelease(buffer1);
    BLOG_CHECK(ret == 0);
}

void testStackBuffer() {

    BFX_STACK_BUFFER_HANDLE buffer1 = BFX_CREATE_STACK_BUFFER(1024);

    size_t len;
    uint8_t* data = BfxStackBufferGetData(buffer1, &len);
    assert(len == 1024);
    for (size_t index = 0; index < 1024; ++index) {
        data[index] = (uint8_t)(index % 256);
    }

    {
        BFX_STACK_BUFFER_HANDLE buffer2 = BFX_CREATE_STACK_BIND_BUFFER(data, 256);
        size_t len;
        uint8_t* data = BfxStackBufferGetData(buffer2, &len);
        assert(len == 256);
        for (size_t index = 0; index < 256; ++index) {
            BLOG_CHECK(data[index] == (uint8_t)index);
        }

        uint8_t data2[256];
        size_t ret = BfxStackBufferRead(buffer2, 128, 128, data2, 0);
        assert(ret == 128);
        for (size_t index = 0; index < 128; ++index) {
            BLOG_CHECK(data2[index] == (uint8_t)index + 128);
        }
    }

    {
        BFX_STACK_BUFFER_HANDLE buffer2 = BFX_CLIP_STACK_BUFFER(buffer1, 0, 256);
        size_t len;
        uint8_t* data = BfxStackBufferGetData(buffer2, &len);
        assert(len == 256);
        for (size_t index = 0; index < 256; ++index) {
            BLOG_CHECK(data[index] == (uint8_t)index);
        }

        BfxFinishStackBuffer(buffer2);
    }


    BfxFinishStackBuffer(buffer1);
}