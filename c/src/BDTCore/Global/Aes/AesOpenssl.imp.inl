#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "../CryptoToolKit.h"
#include <openssl/aes.h>
#include <openssl/evp.h>

int Bdt_AesEncryptDataWithPadding(const uint8_t aesKey[BDT_AES_KEY_LENGTH], uint8_t* input, size_t dataLen, size_t iLen, uint8_t* output, size_t oLen)
{
    EVP_CIPHER_CTX* ctx;
    ctx = EVP_CIPHER_CTX_new();

    int targetLen = (int)(dataLen / 16 + 1) * 16; // padding后的数据长度
    if (oLen < targetLen || iLen < targetLen)
    {
        return -1;
    }

    uint8_t iv[16] = { 0 };

    BOOL isOk = EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), NULL, aesKey, iv, AES_ENCRYPT) &&
        EVP_CipherUpdate(ctx, output, &targetLen, input, (int)dataLen);

    if (!isOk)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    oLen = targetLen;
    isOk = EVP_CipherFinal(ctx, output + targetLen, &targetLen);
    if (!isOk)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    oLen += targetLen;
    EVP_CIPHER_CTX_free(ctx);

    return (int)oLen;
}

int Bdt_AesDecryptDataWithPadding(const uint8_t aesKey[BDT_AES_KEY_LENGTH], const uint8_t* input, size_t iLen, uint8_t* output, size_t oLen)
{
    if (oLen < iLen)
    {
        return -1;
    }

    int decLen = 0;
    EVP_CIPHER_CTX* ctx;
    ctx = EVP_CIPHER_CTX_new();

    uint8_t iv[16] = { 0 };
    BOOL isOk = EVP_CipherInit_ex(ctx, EVP_aes_256_cbc(), NULL, aesKey, iv, AES_DECRYPT) &&
        EVP_CipherUpdate(ctx, output, &decLen, input, (int)iLen);

    if (!isOk)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    oLen = decLen;
    isOk = EVP_CipherFinal(ctx, output + decLen, &decLen);
    if (!isOk)
    {
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }

    oLen += decLen;
    EVP_CIPHER_CTX_free(ctx);

    return (int)oLen;
}