//统一加密算法的参数，避免直接使用ssl相关函数

#ifndef _BDT_CRYPTO_TOOLKIT_H_
#define _BDT_CRYPTO_TOOLKIT_H_

#include "../BdtCore.h"

#define BDT_AES_KEY_LENGTH			32 //256bits aes
#define BDT_MIX_HASH_LENGTH			8 //64bits
#define BDT_MAX_SIGN_LENGTH			1024

// 带padding的加密，返回padding后的长度，失败返回-1或MBEDTLS错误码，input和output长度必须大于等于padding后长度,允许原地加密
int Bdt_AesEncryptDataWithPadding(
	const uint8_t aesKey[BDT_AES_KEY_LENGTH], 
	uint8_t* input, 
	size_t dataLen, 
	size_t iLen, 
	uint8_t* output, 
	size_t oLen);

// 解密用AESEncryptDataWithPadding加密后的数据，成功返回数据长度，错误返回-1或MBEDTLS错误码,允许原地加密
int Bdt_AesDecryptDataWithPadding(
	const uint8_t aesKey[BDT_AES_KEY_LENGTH], 
	const uint8_t* input, 
	size_t iLen, 
	uint8_t* output, 
	size_t oLen);

void Bdt_HashAesKey(
	const uint8_t aesKey[BDT_AES_KEY_LENGTH], 
	uint8_t keyHash[BDT_MIX_HASH_LENGTH]);
bool Bdt_CheckAesKeyHash(
	const uint8_t aesKey[BDT_AES_KEY_LENGTH], 
	const uint8_t keyHash[BDT_MIX_HASH_LENGTH]);

int Bdt_AesGenerateKey(uint8_t aesKey[32]);


int Bdt_Sha256Hash(const uint8_t* data, size_t len, uint8_t hash[32]);

int Bdt_RsaEncrypt(const uint8_t* data, size_t len, const uint8_t* publicKey, size_t publicKeyLen, uint8_t* output, size_t* olen, size_t osize);
// 解密前的数据长度一定等于密钥的bytes长度，这里不需要传入dataLen, 可以顺带返回bytes长度方便后续使用
int Bdt_RsaDecrypt(const uint8_t* data, const uint8_t* secret, size_t secretLen, uint8_t* output, size_t* outputLen, size_t osize, size_t* outKeyBytes);
// 用Data的MD5值进行签名
int Bdt_RsaSignMd5(const uint8_t* data, size_t len, const uint8_t* secret, size_t secretLen, uint8_t* output, size_t* outputLen);
int Bdt_RsaVerifyMd5(const uint8_t* sign, size_t signLen, const uint8_t* publicKey, size_t publicLen, const uint8_t* data, size_t dataLen);

int Bdt_RsaGenerateToBuffer(uint32_t bits, uint8_t* publicKey, size_t* inoutPublicLen, uint8_t* secret, size_t* inoutSecretLen);

#endif //_BDT_CRYPTO_TOOLKIT_H_