#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__

#include "./CryptoToolKit.h"

#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/pk_internal.h>
#include <mbedtls/md5.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>

#ifdef _USE_AES_MBEDTLS_
#include "./Aes/AesMbedtls.imp.inl"
#else
#include "./Aes/AesOpenssl.imp.inl"
#endif

void Bdt_HashAesKey(const uint8_t aesKey[BDT_AES_KEY_LENGTH], uint8_t keyHash[BDT_AES_KEY_LENGTH])
{
	uint8_t sha256Result[32];
	Bdt_Sha256Hash(aesKey, BDT_AES_KEY_LENGTH, sha256Result);
	memcpy(keyHash, sha256Result, 8);
	return;
}


bool Bdt_CheckAesKeyHash(const uint8_t aesKey[BDT_AES_KEY_LENGTH], const uint8_t keyHash[BDT_AES_KEY_LENGTH])
{
	uint8_t sha256Result[32];
	Bdt_Sha256Hash(aesKey, BDT_AES_KEY_LENGTH, sha256Result);
	if (memcmp(sha256Result, keyHash, 8) == 0)
	{
		return true;
	}
	return false;
}

int Bdt_Sha256Hash(const uint8_t * data, size_t len, uint8_t hash[32])
{
    return mbedtls_sha256_ret(data, len, hash, 0);;
}


int Bdt_AesGenerateKey(uint8_t aesKey[32])
{
	BfxRandomBytes(aesKey, BDT_AES_KEY_LENGTH);
	return BFX_RESULT_SUCCESS;
}

int Bdt_RsaEncrypt(const uint8_t * data, size_t len, const uint8_t * publicKey, size_t publicKeyLen, uint8_t * output, size_t * olen, size_t osize)
{
    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);
    int ret = mbedtls_pk_parse_public_key(&pk, publicKey, publicKeyLen);
    if (ret)
    {
        return ret;
    }

    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    char* personalization = "bdt2.0";

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const unsigned char*)personalization,
        strlen(personalization));

    ret = mbedtls_pk_encrypt(&pk, data, len, output, olen, osize, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_pk_free(&pk);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
}

int Bdt_RsaDecrypt(const uint8_t* data, const uint8_t* secret, size_t secretLen, uint8_t* output, size_t* outputLen, size_t osize, size_t* outKeyBytes)
{
    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    char* personalization = "bdt2.0";

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const unsigned char*)personalization,
        strlen(personalization));

    mbedtls_pk_context spk;
    mbedtls_pk_init(&spk);
    int ret = mbedtls_pk_parse_key(&spk, secret, secretLen, NULL, 0);
    if (ret)
    {
        return ret;
    }

    if (outKeyBytes)
    {
        *outKeyBytes = mbedtls_pk_rsa(spk)->len;
    }

    ret = mbedtls_pk_decrypt(&spk, data, mbedtls_pk_rsa(spk)->len, output, outputLen, osize, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_pk_free(&spk);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
}

int Bdt_RsaSignMd5(const uint8_t * data, size_t len, const uint8_t * secret, size_t secretLen, uint8_t * output, size_t* outputLen)
{
    mbedtls_entropy_context entropy;
    mbedtls_entropy_init(&entropy);

    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ctr_drbg_init(&ctr_drbg);

    char* personalization = "bdt2.0";

    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const unsigned char*)personalization,
        strlen(personalization));

    uint8_t md[16];
    int ret = mbedtls_md5_ret(data, len, md);
    if (ret) 
    {
        return ret;
    }

    mbedtls_pk_context spk;
    mbedtls_pk_init(&spk);
    ret = mbedtls_pk_parse_key(&spk, secret, secretLen, NULL, 0);
    if (ret)
    {
        return ret;
    }

    ret = mbedtls_pk_sign(&spk, MBEDTLS_MD_MD5, md, 16, output, outputLen, mbedtls_ctr_drbg_random, &ctr_drbg);
	mbedtls_pk_free(&spk);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);
    return ret;
}

int Bdt_RsaVerifyMd5(const uint8_t* sign, size_t signLen, const uint8_t* publicKey, size_t publicLen, const uint8_t* data, size_t dataLen)
{
    uint8_t md[16];
    int ret = mbedtls_md5_ret(data, dataLen, md);
    if (ret)
    {
        return ret;
    }

    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    ret = mbedtls_pk_parse_public_key(&pk, publicKey, publicLen);
    if (ret)
    {
        return ret;
    }

    ret = mbedtls_pk_verify(&pk, MBEDTLS_MD_MD5, md, sizeof(md), sign, signLen);
	mbedtls_pk_free(&pk);
    return ret;
}

int Bdt_RsaGenerateToBuffer(uint32_t bits, uint8_t* publicKey, size_t* inoutPublicLen, uint8_t* secret, size_t* inoutSecretLen)
{
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;

    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);
    mbedtls_pk_init(&pk);
    mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA));

    char* personalization = "bdt2.0";
    mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
        (const unsigned char*)personalization,
        strlen(personalization));

	int ret = 0;
	do {
        mbedtls_rsa_context* ctx = mbedtls_pk_rsa(pk);
        // mbedtls_rsa_init(ctx, MBEDTLS_RSA_PKCS_V15, 0);
		ret = mbedtls_rsa_gen_key(ctx, mbedtls_ctr_drbg_random, &ctr_drbg, bits, 65537);
        if (ret)
        {
            break;
        }
		int secretLen = mbedtls_pk_write_key_der(&pk, secret, *inoutSecretLen);
		if (secretLen < 0) {
			ret = secretLen;
			break;
		}

        if (*inoutSecretLen != secretLen)
        {
            // shift
            memmove(secret, secret + (*inoutSecretLen - secretLen), secretLen);
        }
        *inoutSecretLen = secretLen;

		int publicLen = mbedtls_pk_write_pubkey_der(&pk, publicKey, *inoutPublicLen);
		if (publicLen < 0) {
			ret = publicLen;
			break;
		}
        if (*inoutPublicLen != publicLen)
        {
            // shift
            memmove(publicKey, publicKey + (*inoutPublicLen - publicLen), publicLen);
        }
		*inoutPublicLen = publicLen;

	} while (false);

	mbedtls_pk_free(&pk);
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);

	return ret;
}
