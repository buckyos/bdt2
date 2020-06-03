#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "../CryptoToolKit.h"
#include <mbedtls/aes.h>

int Bdt_AesEncryptDataWithPadding(const uint8_t aesKey[BDT_AES_KEY_LENGTH], uint8_t* input, size_t dataLen, size_t iLen, uint8_t* output, size_t oLen)
{
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_enc(&ctx, aesKey, BDT_AES_KEY_LENGTH * 8);

    size_t targetLen = (dataLen / 16 + 1) * 16; // padding后的数据长度
    if (oLen < targetLen || iLen < targetLen)
    {
        return -1;
    }
    // 得到应该padding的长度
    uint8_t padding = (uint8_t)(targetLen - dataLen);

    // 在空余长度处填充padding
    for (uint8_t i = 0; i < padding; i++)
    {
        input[dataLen + i] = padding;
    }

    uint8_t iv[16] = { 0 };
    int ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, targetLen, iv, input, output);
    mbedtls_aes_free(&ctx);
    if (ret != 0)
    {
        return ret;
    }

    return (int)targetLen;
}

int Bdt_AesDecryptDataWithPadding(const uint8_t aesKey[BDT_AES_KEY_LENGTH], const uint8_t* input, size_t iLen, uint8_t* output, size_t oLen)
{
    if (oLen < iLen)
    {
        return -1;
    }
    mbedtls_aes_context ctx;
    mbedtls_aes_init(&ctx);
    mbedtls_aes_setkey_dec(&ctx, aesKey, BDT_AES_KEY_LENGTH * 8);
    uint8_t iv[16] = { 0 };
    int ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, iLen, iv, input, output);
    mbedtls_aes_free(&ctx);
    if (ret != 0)
    {
        return ret;
    }
    uint8_t padding = output[iLen - 1];
    for (size_t i = 0; i < padding; i++)
    {
        if (output[iLen - 1 - i] != padding)
        {
            return -1;
        }
    }
    return (int)(iLen - padding);
}