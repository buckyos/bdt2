#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <node_api.h>

#include "BuckyBase/Global/compiler_specific.h"
#include "BDTCore/Global/PeerInfo.h"
#include "BDTCore/Protocol/Package.h"
#include "BDTCore/Protocol/PackageEncoder.h"
#include "BDTCore/Protocol/PackageDecoder.h"

#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

/*
#define AF_INET 2
#define AF_INET6 23
extern int uv_inet_pton(int af, const char* src, void* dst);
extern int uv_inet_ntop(int af, const void* src, char* dst, size_t size);
*/
static napi_value encode(napi_env env, napi_callback_info args);
static napi_value decode(napi_env env, napi_callback_info args);
static napi_value sign_local_peer_info(napi_env env, napi_callback_info args);

void protocol_export(napi_env env, napi_value exports)
{
	napi_status status = napi_ok;
	napi_value fn;

	status = napi_create_function(env, NULL, 0, encode, NULL, &fn);
	assert(status == napi_ok);
	status = napi_set_named_property(env, exports, "encode", fn);
	assert(status == napi_ok);
	status = napi_create_function(env, NULL, 0, decode, NULL, &fn);
	assert(status == napi_ok);
	status = napi_set_named_property(env, exports, "decode", fn);
	assert(status == napi_ok);
	status = napi_create_function(env, NULL, 0, sign_local_peer_info, NULL, &fn);
	assert(status == napi_ok);
	status = napi_set_named_property(env, exports, "signLocalPeerInfo", fn);
	assert(status == napi_ok);
}

// int Bdt_EncodePackage(const Bdt_Package** packages, size_t packageCount, const Bdt_Package* refPkg, uint8_t* buffer, size_t bufferLength, size_t* pWriteBytes);
static Bdt_Package** package_array_js2c(napi_env env, napi_value jsPkgArray, uint32_t* pkgCount);
static void free_package_array(Bdt_Package** pkgArray, uint32_t pkgCount);

static napi_value encode(napi_env env, napi_callback_info args) {
	napi_status status;

	size_t argc = 1;
	napi_value jsPkgArray = NULL;
	napi_value thisArg = NULL;
	void* data = NULL;
	status = napi_get_cb_info(env, args, &argc, &jsPkgArray, &thisArg, &data);
	if (status != napi_ok || argc == 0)
	{
		return NULL;
	}

	uint32_t pkgCount = 0;
	Bdt_Package** cPkgArray = package_array_js2c(env, jsPkgArray, &pkgCount);
	if (cPkgArray == NULL || pkgCount == 0)
	{
		return NULL;
	}

	void* buffer = NULL;
	napi_value jsBuffer = NULL;
	status = napi_create_buffer(env, 2048, &buffer, &jsBuffer);
	if (status != napi_ok)
	{
		return NULL;
	}
	size_t validSize = 0;
	Bdt_EncodePackage((const Bdt_Package * *)cPkgArray, pkgCount, NULL, (uint8_t*)buffer, 2048, &validSize);
	if (validSize == 0)
	{
		free_package_array(cPkgArray, pkgCount);
		return NULL;
	}

	napi_value jsResult = NULL;
	napi_value jsLength = NULL;
	status = napi_create_object(env, &jsResult);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsResult, "buffer", jsBuffer);
	assert(status == napi_ok);
	status = napi_create_uint32(env, (uint32_t)validSize, &jsLength);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsResult, "length", jsLength);
	assert(status == napi_ok);

	free_package_array(cPkgArray, pkgCount);

	return jsResult;
}

// int Bdt_DecodePackage(const uint8_t* buffer, size_t length, const Bdt_Package* refPkg, Bdt_Package** outResult, bool isStartWithExchangeKey);
static napi_value sn_ping_c2js(napi_env env, const Bdt_SnPingPackage* pkg);
static napi_value sn_call_c2js(napi_env env, const Bdt_SnCallPackage* pkg);
static napi_value sn_called_resp_c2js(napi_env env, const Bdt_SnCalledRespPackage* pkg);
static napi_value exchange_key_c2js(napi_env env, const Bdt_ExchangeKeyPackage* pkg);

static napi_value decode(napi_env env, napi_callback_info args) {
	napi_status status;

	size_t argc = 2;
	napi_value inArgs[2] = { NULL, NULL };
	napi_value thisArg = NULL;
	void* data = NULL;
	status = napi_get_cb_info(env, args, &argc, inArgs, &thisArg, &data);
	if (status != napi_ok || argc == 0)
	{
		return NULL;
	}

	void* buffer = NULL;
	size_t length = 0;
	status = napi_get_buffer_info(env, inArgs[0], &buffer, &length);
	if (status != napi_ok || buffer == NULL || length == 0)
	{
		return NULL;
	}

	bool isStartWithExchangeKey = FALSE;
	if (argc >= 2)
	{
		status = napi_get_value_bool(env, inArgs[1], &isStartWithExchangeKey);
		assert(status == napi_ok);
	}

	Bdt_Package* pkgs[BDT_AES_KEY_LENGTH];
	memset(pkgs, 0, sizeof(pkgs));
	int r = Bdt_DecodePackage((uint8_t*)buffer, length, NULL, pkgs, isStartWithExchangeKey);
    if (r != 0) {
        return NULL;
    }

	napi_value jsPkgsArray = NULL;
	status = napi_create_array(env, &jsPkgsArray);
	assert(status == napi_ok);

	int invalidCount = 0;
	uint32_t i = 0;
	for (i = 0; i < BFX_ARRAYSIZE(pkgs); i++)
	{
		if (pkgs[i] == NULL)
		{
			break;
		}

		napi_value jsPkg = NULL;
		switch (pkgs[i]->cmdtype)
		{
		case BDT_SN_PING_PACKAGE:
			{
				jsPkg = sn_ping_c2js(env, (Bdt_SnPingPackage*)pkgs[i]);
				Bdt_SnPingPackageFree((Bdt_SnPingPackage*)pkgs[i]);
			}
			break;
		case BDT_SN_CALL_PACKAGE:
			{
				jsPkg = sn_call_c2js(env, (Bdt_SnCallPackage*)pkgs[i]);
				Bdt_SnCallPackageFree((Bdt_SnCallPackage*)pkgs[i]);
			}
			break;
		case BDT_SN_CALLED_RESP_PACKAGE:
			{
				jsPkg = sn_called_resp_c2js(env, (Bdt_SnCalledRespPackage*)pkgs[i]);
				Bdt_SnCalledRespPackageFree((Bdt_SnCalledRespPackage*)pkgs[i]);
			}
			break;
		case BDT_EXCHANGEKEY_PACKAGE:
		{
			jsPkg = exchange_key_c2js(env, (Bdt_ExchangeKeyPackage*)pkgs[i]);
			Bdt_ExchangeKeyPackageFree((Bdt_ExchangeKeyPackage*)pkgs[i]);
		}
		break;
		default:
			invalidCount++;
			break;
		}

		if (jsPkg != NULL)
		{
			status = napi_set_element(env, jsPkgsArray, i - invalidCount, jsPkg);
			assert(status == napi_ok);
		}
	}

	return jsPkgsArray;
}

static napi_value peerid_c2js(napi_env env, const BdtPeerid* peerid)
{
	napi_status status = napi_ok;
	napi_value jsPeerid = NULL;

	void* resultBuffer = NULL;
	status = napi_create_buffer_copy(env, sizeof(peerid->pid), peerid->pid, &resultBuffer, &jsPeerid);
	assert(status == napi_ok);

	return jsPeerid;
}

static uint32_t peerid_js2c(napi_env env, napi_value jsPeerid, BdtPeerid* peerid)
{
	bool isBuffer = false;
	napi_status status = napi_is_buffer(env, jsPeerid, &isBuffer);
	if (!isBuffer)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	void* resultBuffer = NULL;
	size_t bufferSize = 0;
	status = napi_get_buffer_info(env, jsPeerid, &resultBuffer, &bufferSize);
	assert(status == napi_ok);
	if (status != napi_ok)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	memcpy(peerid->pid, resultBuffer, min(bufferSize, sizeof(peerid->pid)));

	return BFX_RESULT_SUCCESS;
}

static napi_value peer_areacode_c2js(napi_env env, const BdtPeerArea* area)
{
	napi_status status = napi_ok;
	napi_value jsArea = NULL;
	status = napi_create_object(env, &jsArea);
	assert(status == napi_ok);

	napi_value jsField = NULL;

	status = napi_create_uint32(env, area->continent, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsArea, "continent", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, area->country, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsArea, "country", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, area->carrier, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsArea, "carrier", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, area->city, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsArea, "city", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, area->inner, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsArea, "inner", jsField);
	assert(status == napi_ok);

	return jsArea;
}

static uint32_t peer_areacode_js2c(napi_env env, napi_value jsArea, BdtPeerArea* area)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsArea, &jsType);
	if (jsType != napi_object)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	napi_status status = napi_ok;
	napi_value jsField = NULL;
	uint32_t u32 = 0;

	status = napi_get_named_property(env, jsArea, "continent", &jsField);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsField, &u32);
	assert(status == napi_ok);
	area->continent = (uint8_t)u32;

	status = napi_get_named_property(env, jsArea, "country", &jsField);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsField, &u32);
	assert(status == napi_ok);
	area->country = (uint8_t)u32;

	status = napi_get_named_property(env, jsArea, "carrier", &jsField);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsField, &u32);
	assert(status == napi_ok);
	area->carrier = (uint8_t)u32;

	status = napi_get_named_property(env, jsArea, "city", &jsField);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsField, &u32);
	assert(status == napi_ok);
	area->city = (uint16_t)u32;

	status = napi_get_named_property(env, jsArea, "inner", &jsField);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsField, &u32);
	assert(status == napi_ok);
	area->inner = (uint8_t)u32;

	return BFX_RESULT_SUCCESS;
}

static napi_value peer_constinfo_c2js(napi_env env, const BdtPeerConstInfo* constInfo, napi_value jsAreaCode)
{
	napi_status status = napi_ok;
	napi_value jsConstInfo = NULL;
	status = napi_create_object(env, &jsConstInfo);
	assert(status == napi_ok);

	napi_value jsField = NULL;

	if (jsAreaCode == NULL)
	{
		jsAreaCode = peer_areacode_c2js(env, &constInfo->areaCode);
	}
	status = napi_set_named_property(env, jsConstInfo, "areaCode", jsAreaCode);
	assert(status == napi_ok);

	void* resultBuffer = NULL;
	status = napi_create_buffer_copy(env, sizeof(constInfo->deviceId), constInfo->deviceId, &resultBuffer, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsConstInfo, "deviceId", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, constInfo->publicKeyType, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsConstInfo, "publicKeyType", jsField);
	assert(status == napi_ok);

	switch (constInfo->publicKeyType)
	{
	case BDT_PEER_PUBLIC_KEY_TYPE_RSA1024:
		status = napi_create_buffer_copy(env, sizeof(constInfo->publicKey.rsa1024), constInfo->publicKey.rsa1024, &resultBuffer, &jsField);
		assert(status == napi_ok);
		break;
	case BDT_PEER_PUBLIC_KEY_TYPE_RSA2048:
		status = napi_create_buffer_copy(env, sizeof(constInfo->publicKey.rsa2048), constInfo->publicKey.rsa2048, &resultBuffer, &jsField);
		assert(status == napi_ok);
		break;
	case BDT_PEER_PUBLIC_KEY_TYPE_RSA3072:
		status = napi_create_buffer_copy(env, sizeof(constInfo->publicKey.rsa3072), constInfo->publicKey.rsa3072, &resultBuffer, &jsField);
		assert(status == napi_ok);
		break;
	/*
	case BDT_PEER_PUBLIC_KEY_TYPE_SECP256K1:
		status = napi_create_buffer_copy(env, sizeof(constInfo->publicKey.secp256k1), constInfo->publicKey.secp256k1, &resultBuffer, &jsField);
		assert(status == napi_ok);
		break;
		*/
	default:
		status = napi_create_buffer_copy(env, sizeof(constInfo->publicKey), &constInfo->publicKey, &resultBuffer, &jsField);
		assert(status == napi_ok);
	}
	status = napi_set_named_property(env, jsConstInfo, "publicKey", jsField);
	assert(status == napi_ok);

	return jsConstInfo;
}

static uint32_t peer_constinfo_js2c(napi_env env, napi_value jsConstInfo, BdtPeerConstInfo* constInfo)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsConstInfo, &jsType);
	if (jsType != napi_object)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	napi_status status = napi_ok;

	napi_value jsField = NULL;
	uint32_t u32 = 0;

	status = napi_get_named_property(env, jsConstInfo, "areaCode", &jsField);
	assert(status == napi_ok);
	if (jsField != NULL)
	{
		if (peer_areacode_js2c(env, jsField, &constInfo->areaCode) != BFX_RESULT_SUCCESS)
		{
			return BFX_RESULT_INVALIDPARAM;
		}
	}

	void* resultBuffer = NULL;
	size_t bufferSize = 0;
	status = napi_get_named_property(env, jsConstInfo, "deviceId", &jsField);
	assert(status == napi_ok);
	if (jsField != NULL)
	{
		status = napi_get_buffer_info(env, jsField, &resultBuffer, &bufferSize);
		assert(status == napi_ok);
		memcpy(constInfo->deviceId, resultBuffer, min(bufferSize, sizeof(constInfo->deviceId)));
	}

	status = napi_get_named_property(env, jsConstInfo, "publicKeyType", &jsField);
	assert(status == napi_ok);
	if (jsField != NULL)
	{
		status = napi_get_value_uint32(env, jsField, &u32);
		assert(status == napi_ok);
		constInfo->publicKeyType = (uint8_t)u32;
	}

	status = napi_get_named_property(env, jsConstInfo, "publicKey", &jsField);
	assert(status == napi_ok);
	if (jsField != NULL)
	{
		status = napi_get_buffer_info(env, jsField, &resultBuffer, &bufferSize);
		assert(status == napi_ok);
		memcpy(&constInfo->publicKey, resultBuffer, min(bufferSize, sizeof(constInfo->publicKey)));
	}

	return BFX_RESULT_SUCCESS;
}

static napi_value uint64_c2js(napi_env env, uint64_t u64)
{
	napi_status status = napi_ok;
	napi_value jsU64 = NULL;
	status = napi_create_object(env, &jsU64);
	assert(status == napi_ok);

	napi_value jsU32 = NULL;

	status = napi_create_uint32(env, (uint32_t)(u64 >> 32), &jsU32);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsU64, "h", jsU32);
	assert(status == napi_ok);

	status = napi_create_uint32(env, (uint32_t)(u64 & 0xFFFFFFFF), &jsU32);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsU64, "l", jsU32);
	assert(status == napi_ok);

	return jsU64;
}

static uint64_t uint64_js2c(napi_env env, napi_value jsU64)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsU64, &jsType);
	if (jsType != napi_object)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	napi_status status = napi_ok;
	napi_value jsU32 = NULL;
	uint64_t u64 = 0;
	uint32_t u32 = 0;

	status = napi_get_named_property(env, jsU64, "h", &jsU32);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsU32, &u32);
	assert(status == napi_ok);
	u64 = ((uint64_t)u32) << 32;

	status = napi_get_named_property(env, jsU64, "l", &jsU32);
	assert(status == napi_ok);
	status = napi_get_value_uint32(env, jsU32, &u32);
	assert(status == napi_ok);
	u64 |= u32;

	return u64;
}

static bool endpoint_is_static_wan(const BdtEndpoint* ep)
{
	if ((ep->flag & BDT_ENDPOINT_FLAG_STATIC_WAN) == 0)
	{
		return false;
	}
	if ((ep->flag & BDT_ENDPOINT_IP_VERSION_6) != 0)
	{
		return true;
	}

	switch (ep->address[0]) {
		case 10: return false;
		case 172: return ep->address[1] > 31;
		case 192: return ep->address[1] != 168;
	}
	return true;
}

static napi_value endpoint_c2js(napi_env env, const BdtEndpoint* ep)
{
	napi_status status = napi_ok;
	napi_value jsEpObj = NULL;
	napi_value jsFlag = NULL;
	napi_value jsEp = NULL;
	napi_value jsIsStaticWan = NULL;

	status = napi_create_array_with_length(env, 2, &jsEpObj);

	char epStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = "";
	BdtEndpointToString(ep, epStr);
	status = napi_create_string_utf8(env, epStr, NAPI_AUTO_LENGTH, &jsEp);
	assert(status == napi_ok);
	status = napi_set_element(env, jsEpObj, 0, jsEp);
	assert(status == napi_ok);

	status = napi_create_uint32(env, ep->flag, &jsFlag);
	assert(status == napi_ok);
	status = napi_set_element(env, jsEpObj, 1, jsFlag);
	assert(status == napi_ok);

	status = napi_get_boolean(env, endpoint_is_static_wan(ep), &jsIsStaticWan);
	assert(status == napi_ok);
	status = napi_set_element(env, jsEpObj, 2, jsIsStaticWan);
	assert(status == napi_ok);

	return jsEpObj;
}

static uint32_t endpoint_js2c(napi_env env, napi_value jsEp, BdtEndpoint* cEp)
{
	napi_status status = napi_ok;
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsEp, &jsType);
	if (jsType != napi_string && jsType != napi_object)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	napi_value jsEpStr = jsEp;
	uint32_t flag = 0;
	if (jsType == napi_object)
	{
		uint32_t fieldCount = 0;
		status = napi_get_array_length(env, jsEp, &fieldCount);
		assert(status == napi_ok);
		if (fieldCount <= 0)
		{
			return BFX_RESULT_INVALIDPARAM;
		}

		status = napi_get_element(env, jsEp, 0, &jsEpStr);
		assert(status == napi_ok);
		if (fieldCount > 1)
		{
			napi_value jsFlag = NULL;
			status = napi_get_element(env, jsEp, 1, &jsFlag);
			napi_get_value_uint32(env, jsFlag, &flag);
		}
	}

	char epStr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1] = "";
	size_t length = 0;
	status = napi_get_value_string_utf8(env, jsEpStr, epStr, sizeof(epStr), &length);
	assert(status == napi_ok);
	epStr[length] = '\0';

	uint32_t errorCode = BdtEndpointFromString(cEp, epStr);
	if (flag != 0)
	{
		cEp->flag |= (flag & (BDT_ENDPOINT_FLAG_SIGNED | BDT_ENDPOINT_FLAG_STATIC_WAN));
	}
	return errorCode;
}

static napi_value peer_endpoint_list_c2js(napi_env env, const BdtEndpoint* eplist, size_t count)
{
	napi_status status = napi_ok;
	napi_value jsEplist = NULL;
	status = napi_create_array_with_length(env, count, &jsEplist);

	napi_value jsEp = NULL;
	assert(status == napi_ok);
	size_t i = 0;
	for (i = 0; i < count; i++)
	{
		jsEp = endpoint_c2js(env, &eplist[i]);
		status = napi_set_element(env, jsEplist, i, jsEp);
		assert(status == napi_ok);
	}

	return jsEplist;
}

static uint32_t peer_endpoint_list_js2c(napi_env env, napi_value jsEplist, BdtEndpointArray* cEplist)
{
	bool jsIsArray = false;
	napi_is_array(env, jsEplist, &jsIsArray);
	if (!jsIsArray)
	{
		return BFX_RESULT_INVALIDPARAM;
	}

	napi_status status = napi_ok;
	uint32_t epCount = 0;
	status = napi_get_array_length(env, jsEplist, &epCount);
	assert(status == napi_ok);

	if (epCount == 0)
	{
		return BFX_RESULT_SUCCESS;
	}

	napi_value jsEp = NULL;
	int pushCount = 0;
	uint32_t i = 0;

	BdtEndpointArrayUninit(cEplist);
	BdtEndpointArrayInit(cEplist, epCount);
	for (i = 0; i < epCount; i++)
	{
		status = napi_get_element(env, jsEplist, i, &jsEp);
		if (status == napi_ok && jsEp != NULL)
		{
			BdtEndpoint* cEp = cEplist->list + pushCount;
			if (endpoint_js2c(env, jsEp, cEp) == BFX_RESULT_SUCCESS)
			{
				pushCount++;
			}
		}
	}
	cEplist->size = pushCount;
	return BFX_RESULT_SUCCESS;
}

static napi_value peerinfo_c2js_no_sn(napi_env env,
	const BdtPeerInfo* peerInfo,
	napi_value jsPeerid,
	napi_value jsConstInfo,
	napi_value jsEplist)
{
	napi_status status = napi_ok;
	napi_value jsPeerInfo = NULL;
	status = napi_create_object(env, &jsPeerInfo);
	assert(status == napi_ok);

	if (jsPeerid == NULL)
	{
		jsPeerid = peerid_c2js(env, BdtPeerInfoGetPeerid(peerInfo));
	}
	status = napi_set_named_property(env, jsPeerInfo, "peerid", jsPeerid);
	assert(status == napi_ok);

	if (jsConstInfo == NULL)
	{
		jsConstInfo = peer_constinfo_c2js(env, BdtPeerInfoGetConstInfo(peerInfo), NULL);
	}
	status = napi_set_named_property(env, jsPeerInfo, "constInfo", jsConstInfo);
	assert(status == napi_ok);

	if (jsEplist == NULL)
	{
		size_t epCount = 0;
		const BdtEndpoint* eplist = BdtPeerInfoListEndpoint(peerInfo, &epCount);
		jsEplist = peer_endpoint_list_c2js(env, eplist, epCount);
	}
	status = napi_set_named_property(env, jsPeerInfo, "endpointArray", jsEplist);
	assert(status == napi_ok);

	size_t signSize = 0;
	const uint8_t* signature = BdtPeerInfoGetSignature(peerInfo, &signSize);
	if (signature != NULL)
	{
		void* resultBuffer = NULL;
		napi_value jsSignature = NULL;
		status = napi_create_buffer_copy(env, signSize, signature, &resultBuffer, &jsSignature);
		assert(status == napi_ok);
		status = napi_set_named_property(env, jsPeerInfo, "signature", jsSignature);
		assert(status == napi_ok);
	}

	napi_value jsCreateTime = uint64_c2js(env, BdtPeerInfoGetCreateTime(peerInfo));
	status = napi_set_named_property(env, jsPeerInfo, "createTime", jsCreateTime);
	assert(status == napi_ok);

	return jsPeerInfo;
}

static napi_value peerinfo_c2js(napi_env env,
	const BdtPeerInfo* peerInfo,
	napi_value jsPeerid,
	napi_value jsConstInfo,
	napi_value jsEplist)
{
	napi_status status = napi_ok;
	napi_value jsPeerInfo = peerinfo_c2js_no_sn(env, peerInfo, jsPeerid, jsConstInfo, jsEplist);
	if (jsPeerInfo == NULL)
	{
		return NULL;
	}

	size_t i = 0;
	size_t invalidCount = 0;

	size_t snCount = 0;
	napi_value jsSnArray = NULL;
	const BdtPeerid* snArray = BdtPeerInfoListSn(peerInfo, &snCount);
	status = napi_create_array_with_length(env, snCount, &jsSnArray);
	assert(status == napi_ok);
	for (i = 0; i < snCount; i++)
	{
		napi_value jsSnPeerid = peerid_c2js(env, snArray + i);
		if (jsSnPeerid == NULL)
		{
			invalidCount++;
		}
		else
		{
			status = napi_set_element(env, jsSnArray, i - invalidCount, jsSnPeerid);
			assert(status == napi_ok);
		}
	}
	status = napi_set_named_property(env, jsPeerInfo, "snList", jsSnArray);
	assert(status == napi_ok);

	size_t snInfoCount = 0;
	napi_value jsSnInfoArray = NULL;
	const BdtPeerInfo** snInfoArray = BdtPeerInfoListSnInfo(peerInfo, &snInfoCount);
	status = napi_create_array_with_length(env, snInfoCount, &jsSnInfoArray);
	assert(status == napi_ok);
	for (i = 0, invalidCount = 0; i < snInfoCount; i++)
	{
		napi_value jsSnPeerInfo = peerinfo_c2js_no_sn(env, snInfoArray[i], NULL, NULL, NULL);
		if (jsSnPeerInfo == NULL)
		{
			invalidCount++;
		}
		else
		{
			status = napi_set_element(env, jsSnInfoArray, i - invalidCount, jsSnPeerInfo);
			assert(status == napi_ok);
		}
	}
	status = napi_set_named_property(env, jsPeerInfo, "snInfoList", jsSnInfoArray);
	assert(status == napi_ok);

	return jsPeerInfo;
}

static uint32_t peerinfo_js2c_no_sn(napi_env env,
	napi_value jsPeerInfo,
	BdtPeerid* cPeerid,
	BdtPeerConstInfo* cConstInfo,
	BdtEndpointArray* cEplist,
	BdtPeerInfoBuilder* infoBuilder)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsPeerInfo, &jsType);
	if (jsType != napi_object)
	{
		return BFX_RESULT_INVALIDPARAM;
	}
	
	napi_status status = napi_ok;
	napi_value jsField = NULL;

	BdtPeerid peeridFromJs;
	if (cPeerid == NULL)
	{
		status = napi_get_named_property(env, jsPeerInfo, "peerid", &jsField);
		assert(status == napi_ok);
		if (peerid_js2c(env, jsField, &peeridFromJs) == BFX_RESULT_SUCCESS)
		{
			cPeerid = &peeridFromJs;
		}
	}

	BdtPeerConstInfo constInfoFromJs;
	if (cConstInfo == NULL)
	{
		status = napi_get_named_property(env, jsPeerInfo, "constInfo", &jsField);
		assert(status == napi_ok);
		if (peer_constinfo_js2c(env, jsField, &constInfoFromJs) == BFX_RESULT_SUCCESS)
		{
			cConstInfo = &constInfoFromJs;
		}
	}

	if (cConstInfo != NULL)
	{
		BdtPeerInfoSetConstInfo(infoBuilder, cConstInfo);
	}

	BdtEndpointArray eplistFromJs;
	BdtEndpointArrayInit(&eplistFromJs, 0);
	if (cEplist == NULL)
	{
		status = napi_get_named_property(env, jsPeerInfo, "endpointArray", &jsField);
		assert(status == napi_ok);
		if (peer_endpoint_list_js2c(env, jsField, &eplistFromJs) == BFX_RESULT_SUCCESS)
		{
			cEplist = &eplistFromJs;
		}
	}

	if (cEplist != NULL)
	{
		size_t i = 0;
		for (i = 0; i < cEplist->size; i++)
		{
			BdtPeerInfoAddEndpoint(infoBuilder, &cEplist->list[i]);
		}
	}

	BdtEndpointArrayUninit(&eplistFromJs);

	napi_value jsSignature = NULL;
	status = napi_get_named_property(env, jsPeerInfo, "signature", &jsSignature);
	assert(status == napi_ok);
	bool isBuffer = false;
	status = napi_is_buffer(env, jsSignature, &isBuffer);
	assert(status == napi_ok);
	if (isBuffer)
	{
		void* signature = NULL;
		size_t signatureSize = 0;
		status = napi_get_buffer_info(env, jsSignature, &signature, &signatureSize);
		assert(status == napi_ok);
		BdtPeerInfoSetSignature(infoBuilder, signature, signatureSize);
	}

	napi_value jsCreateTime = NULL;
	status = napi_get_named_property(env, jsPeerInfo, "createTime", &jsCreateTime);
	assert(status == napi_ok);
	uint64_t createTime = uint64_js2c(env, jsCreateTime);
	BdtPeerInfoSetCreateTime(infoBuilder, createTime);

	return BFX_RESULT_SUCCESS;
}

static napi_value check_field_is_array(napi_env env, napi_value jsObj, char* fieldName)
{
	napi_value jsField = NULL;
	bool isArray = false;
	napi_status status = napi_get_named_property(env, jsObj, fieldName, &jsField);
	if (status == napi_ok)
	{
		if (napi_is_array(env, jsField, &isArray) == napi_ok &&
			isArray)
		{
			return jsField;
		}
	}
	return NULL;
}

static BdtPeerInfoBuilder* peerinfo_js2c(napi_env env,
	napi_value jsPeerInfo,
	BdtPeerid* cPeerid,
	BdtPeerConstInfo* cConstInfo,
	BdtEndpointArray* cEplist)
{
	napi_status status = napi_ok;
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsPeerInfo, &jsType);
	if (jsType != napi_object)
	{
		return NULL;
	}

	uint32_t flags = 0;
	napi_value jsSnListField = check_field_is_array(env, jsPeerInfo, "snList");
	if (jsSnListField != NULL)
	{
		uint32_t snPeeridCount = 0;
		status = napi_get_array_length(env, jsSnListField, &snPeeridCount);
		assert(status == napi_ok);
		if (snPeeridCount > 0)
		{
			flags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST;
		}
	}
	napi_value jsSnInfoListField = check_field_is_array(env, jsPeerInfo, "snInfoList");
	if (jsSnInfoListField != NULL)
	{
		uint32_t snInfoCount = 0;
		status = napi_get_array_length(env, jsSnInfoListField, &snInfoCount);
		assert(status == napi_ok);
		if (snInfoCount > 0)
		{
			flags |= BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO;
		}
	}

	BdtPeerInfoBuilder* infoBuilder = BdtPeerInfoBuilderBegin(flags);
	const BdtPeerInfo* peerInfo = NULL;
	if (peerinfo_js2c_no_sn(env, jsPeerInfo, cPeerid, cConstInfo, cEplist, infoBuilder) != BFX_RESULT_SUCCESS)
	{
		peerInfo = BdtPeerInfoBuilderFinish(infoBuilder, false);
		BdtReleasePeerInfo(peerInfo);
		return NULL;
	}

	if (jsSnListField != NULL)
	{
		uint32_t snPeeridCount = 0;
		napi_value jsSnPeerid = NULL;
		BdtPeerid snPeerid;
		status = napi_get_array_length(env, jsSnListField, &snPeeridCount);
		assert(status == napi_ok);
		for (uint32_t i = 0; i < snPeeridCount; i++)
		{
			status = napi_get_element(env, jsSnListField, i, &jsSnPeerid);
			if (peerid_js2c(env, jsSnPeerid, &snPeerid) == BFX_RESULT_SUCCESS)
			{
				BdtPeerInfoAddSn(infoBuilder, &snPeerid);
			}
		}
	}

	if (jsSnInfoListField != NULL)
	{
		uint32_t snInfoCount = 0;
		napi_value jsSnInfo = NULL;
		BdtPeerInfoBuilder* snPeerInfoBuilder = NULL;
		status = napi_get_array_length(env, jsSnInfoListField, &snInfoCount);
		assert(status == napi_ok);
		for (uint32_t i = 0; i < snInfoCount; i++)
		{
			snPeerInfoBuilder = BdtPeerInfoBuilderBegin(0);
			status = napi_get_element(env, jsSnInfoListField, i, &jsSnInfo);
			uint32_t snInfoParseResult = peerinfo_js2c_no_sn(env, jsSnInfo, NULL, NULL, NULL, snPeerInfoBuilder);
			const BdtPeerInfo* snPeerInfo = BdtPeerInfoBuilderFinish(snPeerInfoBuilder, false);
			if (snInfoParseResult == BFX_RESULT_SUCCESS)
			{
				BdtPeerInfoAddSnInfo(infoBuilder, snPeerInfo);
			}
			BdtReleasePeerInfo(snPeerInfo);
		}
	}

	return infoBuilder;
}

static napi_value sn_ping_c2js(napi_env env, const Bdt_SnPingPackage* pkg)
{
	napi_status status = napi_ok;
	napi_value jsPkg = NULL;
	status = napi_create_object(env, &jsPkg);
	assert(status == napi_ok);

	napi_value jsField = NULL;

	status = napi_create_uint32(env, pkg->cmdtype, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdType", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->cmdflags, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdFlags", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->seq, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "sequence", jsField);
	assert(status == napi_ok);

	jsField = peerid_c2js(env, BdtPeerInfoGetPeerid(pkg->peerInfo));
	status = napi_set_named_property(env, jsPkg, "fromPeerid", jsField);
	assert(status == napi_ok);

	if (pkg->cmdflags & BDT_SN_PING_PACKAGE_FLAG_SNPEERID)
	{
		jsField = peerid_c2js(env, &pkg->snPeerid);
		status = napi_set_named_property(env, jsPkg, "snPeerid", jsField);
		assert(status == napi_ok);
	}

	jsField = peerinfo_c2js(env, pkg->peerInfo, jsField, NULL, NULL);
	status = napi_set_named_property(env, jsPkg, "peerInfo", jsField);
	assert(status == napi_ok);

	return jsPkg;
}

static napi_value sn_call_c2js(napi_env env, const Bdt_SnCallPackage* pkg)
{
	napi_status status = napi_ok;
	napi_value jsPkg = NULL;
	status = napi_create_object(env, &jsPkg);
	assert(status == napi_ok);

	napi_value jsField = NULL;

	status = napi_create_uint32(env, pkg->cmdtype, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdType", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->cmdflags, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdFlags", jsField);
	assert(status == napi_ok);

	bool alwaysCall = (pkg->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_ALWAYSCALL) != 0;
	status = napi_get_boolean(env, alwaysCall, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "alwaysCall", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->seq, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "sequence", jsField);
	assert(status == napi_ok);

	jsField = peerid_c2js(env, &pkg->toPeerid);
	status = napi_set_named_property(env, jsPkg, "toPeerid", jsField);
	assert(status == napi_ok);

	jsField = peerid_c2js(env, BdtPeerInfoGetPeerid(pkg->peerInfo));
	status = napi_set_named_property(env, jsPkg, "fromPeerid", jsField);
	assert(status == napi_ok);

	if (pkg->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_SNPEERID)
	{
		jsField = peerid_c2js(env, &pkg->snPeerid);
		status = napi_set_named_property(env, jsPkg, "snPeerid", jsField);
		assert(status == napi_ok);
	}

	jsField = peerinfo_c2js(env, pkg->peerInfo, jsField, NULL, NULL);
	status = napi_set_named_property(env, jsPkg, "peerInfo", jsField);
	assert(status == napi_ok);

	if ((pkg->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD) && pkg->payload != NULL)
	{
		size_t payloadSize = 0;
		uint8_t* payload = BfxBufferGetData(pkg->payload, &payloadSize);
		void* buffer = NULL;
		status = napi_create_buffer_copy(env, payloadSize, payload, &buffer, &jsField);
		assert(status == napi_ok);
		status = napi_set_named_property(env, jsPkg, "payload", jsField);
		assert(status == napi_ok);
	}

	if ((pkg->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY) && pkg->reverseEndpointArray.size > 0)
	{
		jsField = peer_endpoint_list_c2js(env, pkg->reverseEndpointArray.list, pkg->reverseEndpointArray.size);
		if (jsField != NULL)
		{
			status = napi_set_named_property(env, jsPkg, "reverseEndpointArray", jsField);
			assert(status == napi_ok);
		}
	}

	return jsPkg;
}

static napi_value sn_called_resp_c2js(napi_env env, const Bdt_SnCalledRespPackage* pkg)
{
	napi_status status = napi_ok;
	napi_value jsPkg = NULL;
	status = napi_create_object(env, &jsPkg);
	assert(status == napi_ok);

	napi_value jsField = NULL;

	status = napi_create_uint32(env, pkg->cmdtype, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdType", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->cmdflags, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdFlags", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->seq, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "sequence", jsField);
	assert(status == napi_ok);

	if (pkg->cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID)
	{
		jsField = peerid_c2js(env, &pkg->snPeerid);
		status = napi_set_named_property(env, jsPkg, "snPeerid", jsField);
		assert(status == napi_ok);
	}

	status = napi_create_uint32(env, pkg->result, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "result", jsField);
	assert(status == napi_ok);

	return jsPkg;
}

static Bdt_SnCallRespPackage* sn_call_resp_js2c(napi_env env, napi_value jsPkg);
static Bdt_SnCalledPackage* sn_called_js2c(napi_env env, napi_value jsPkg);
static Bdt_SnPingRespPackage* sn_ping_resp_js2c(napi_env env, napi_value jsPkg);
static Bdt_ExchangeKeyPackage* exchange_key_js2c(napi_env env, napi_value jsPkg);
static Bdt_Package** package_array_js2c(napi_env env, napi_value jsPkgArray, uint32_t* pkgCount)
{
	napi_status status = napi_get_array_length(env, jsPkgArray, pkgCount);
	if (status != napi_ok || *pkgCount == 0)
	{
		return NULL;
	}

	Bdt_Package** cPkgArray = (Bdt_Package * *)malloc(sizeof(Bdt_Package*) * (*pkgCount));
	memset(cPkgArray, 0, sizeof(Bdt_Package*) * (*pkgCount));

	uint32_t invalidCount = 0;
	uint32_t i = 0;
	for (i = 0; i < (*pkgCount); i++)
	{
		napi_value jsPkg = NULL;
		status = napi_get_element(env, jsPkgArray, i, &jsPkg);
		assert(status == napi_ok);

		if (jsPkg == NULL)
		{
			invalidCount++;
			continue;
		}
		napi_value jsCmdType = NULL;

		status = napi_get_named_property(env, jsPkg, "cmdType", &jsCmdType);
		assert(status == napi_ok);

		uint32_t cmdType = 0;
		status = napi_get_value_uint32(env, jsCmdType, &cmdType);
		assert(status == napi_ok);

		switch (cmdType)
		{
		case BDT_SN_PING_RESP_PACKAGE:
		{
			cPkgArray[i - invalidCount] = (Bdt_Package*)sn_ping_resp_js2c(env, jsPkg);
		}
		break;
		case BDT_SN_CALL_RESP_PACKAGE:
		{
			cPkgArray[i - invalidCount] = (Bdt_Package*)sn_call_resp_js2c(env, jsPkg);
		}
		break;
		case BDT_SN_CALLED_PACKAGE:
		{
			cPkgArray[i - invalidCount] = (Bdt_Package*)sn_called_js2c(env, jsPkg);
		}
		break;
		case BDT_EXCHANGEKEY_PACKAGE:
		{
			cPkgArray[i - invalidCount] = (Bdt_Package*)exchange_key_js2c(env, jsPkg);
		}
		break;
		default:
			break;
		}

		if (cPkgArray[i - invalidCount] == NULL)
		{
			invalidCount++;
		}
	}

	(*pkgCount) -= invalidCount;
	return cPkgArray;
}

static void free_package_array(Bdt_Package** pkgArray, uint32_t pkgCount)
{
	uint32_t i = 0;
	for (i = 0; i < pkgCount; i++)
	{
		switch (pkgArray[i]->cmdtype)
		{
		case BDT_SN_PING_RESP_PACKAGE:
		{
			Bdt_SnPingRespPackage* pingResp = (Bdt_SnPingRespPackage*)pkgArray[i];
			BdtEndpointArrayUninit(&pingResp->endpointArray);
			Bdt_SnPingRespPackageFree(pingResp);
		}
		break;
		case BDT_SN_CALL_RESP_PACKAGE:
		{
			Bdt_SnCallRespPackageFree((Bdt_SnCallRespPackage*)pkgArray[i]);
		}
		break;
		case BDT_SN_CALLED_PACKAGE:
		{
			Bdt_SnCalledPackageFree((Bdt_SnCalledPackage*)pkgArray[i]);
		}
		break;
		case BDT_EXCHANGEKEY_PACKAGE:
		{
			Bdt_ExchangeKeyPackageFree((Bdt_ExchangeKeyPackage*)pkgArray[i]);
		}
		break;
		default:
			break;
		}
	}
	free(pkgArray);
}

static Bdt_SnCallRespPackage* sn_call_resp_js2c(napi_env env, napi_value jsPkg)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsPkg, &jsType);
	if (jsType != napi_object)
	{
		return NULL;
	}

	Bdt_SnCallRespPackage* cPkg = Bdt_SnCallRespPackageCreate();
	cPkg->cmdflags = 0;

	napi_status status = napi_ok;
	napi_value jsField = NULL;

	status = napi_get_named_property(env, jsPkg, "sequence", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		status = napi_get_value_uint32(env, jsField, &cPkg->seq);
		assert(status == napi_ok);
		cPkg->cmdflags |= BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ;
	}

	status = napi_get_named_property(env, jsPkg, "snPeerid", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		if (peerid_js2c(env, jsField, &cPkg->snPeerid) == BFX_RESULT_SUCCESS)
		{
			cPkg->cmdflags |= BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID;
		}
	}

	status = napi_get_named_property(env, jsPkg, "result", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		uint32_t resultField = 0;
		status = napi_get_value_uint32(env, jsField, &resultField);
		assert(status == napi_ok);
		cPkg->result = (uint8_t)resultField;
		cPkg->cmdflags |= BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT;
	}

	status = napi_get_named_property(env, jsPkg, "toPeerInfo", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		BdtPeerInfoBuilder* peerInfoBuilder = peerinfo_js2c(env, jsField, NULL, NULL, NULL);
		if (peerInfoBuilder != NULL)
		{
			const BdtPeerInfo* peerinfo = BdtPeerInfoBuilderFinish(peerInfoBuilder, false);
			if (peerinfo != NULL)
			{
				cPkg->toPeerInfo = peerinfo;
				cPkg->cmdflags |= BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO;
			}
		}
	}

	return cPkg;
}

static Bdt_SnCalledPackage* sn_called_js2c(napi_env env, napi_value jsPkg)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsPkg, &jsType);
	if (jsType != napi_object)
	{
		return NULL;
	}

	Bdt_SnCalledPackage* cPkg = Bdt_SnCalledPackageCreate();
	cPkg->cmdflags = 0;

	napi_status status = napi_ok;
	napi_value jsField = NULL;
	void* resultBuffer = NULL;
	size_t bufferSize = 0;

	status = napi_get_named_property(env, jsPkg, "sequence", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		status = napi_get_value_uint32(env, jsField, &cPkg->seq);
		assert(status == napi_ok);
		cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_SEQ;
	}

	status = napi_get_named_property(env, jsPkg, "peerInfo", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		BdtPeerInfoBuilder* peerInfoBuilder = peerinfo_js2c(env, jsField, NULL, NULL, NULL);
		if (peerInfoBuilder != NULL)
		{
			cPkg->peerInfo = BdtPeerInfoBuilderFinish(peerInfoBuilder, false);
			if (cPkg->peerInfo != NULL)
			{
				cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO;
			}
		}
	}
	else
	{
		status = napi_get_named_property(env, jsPkg, "fromPeerid", &jsField);
		if (status == napi_ok && jsField != NULL)
		{
			if (peerid_js2c(env, jsField, &cPkg->fromPeerid) == BFX_RESULT_SUCCESS)
			{
				cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID;
			}
		}
	}

	status = napi_get_named_property(env, jsPkg, "snPeerid", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		if (peerid_js2c(env, jsField, &cPkg->snPeerid) == BFX_RESULT_SUCCESS)
		{
			cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID;
		}
	}

	status = napi_get_named_property(env, jsPkg, "toPeerid", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		if (peerid_js2c(env, jsField, &cPkg->toPeerid) == BFX_RESULT_SUCCESS)
		{
			cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID;
		}
	}

	status = napi_get_named_property(env, jsPkg, "payload", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		napi_typeof(env, jsField, &jsType);
		if (jsType != napi_undefined && jsType != napi_null)
		{
			status = napi_get_buffer_info(env, jsField, &resultBuffer, &bufferSize);
			assert(status == napi_ok);
			cPkg->payload = BfxCreateBuffer(bufferSize);
			uint8_t * payloadBuffer = BfxBufferGetData(cPkg->payload, &bufferSize);
			memcpy(payloadBuffer, resultBuffer, bufferSize);
			cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD;
		}
	}

	status = napi_get_named_property(env, jsPkg, "reverseEndpointArray", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		napi_typeof(env, jsField, &jsType);
		if (jsType != napi_undefined && jsType != napi_null)
		{
			peer_endpoint_list_js2c(env, jsField, &cPkg->reverseEndpointArray);
			if (cPkg->reverseEndpointArray.size > 0)
			{
				cPkg->cmdflags |= BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY;
			}
		}
	}

	return cPkg;
}

static Bdt_SnPingRespPackage* sn_ping_resp_js2c(napi_env env, napi_value jsPkg)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsPkg, &jsType);
	if (jsType != napi_object)
	{
		return NULL;
	}

	Bdt_SnPingRespPackage* cPkg = Bdt_SnPingRespPackageCreate();
	BdtEndpointArrayInit(&cPkg->endpointArray, 0);
	cPkg->cmdflags = 0;

	napi_status status = napi_ok;
	napi_value jsField = NULL;

	status = napi_get_named_property(env, jsPkg, "sequence", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		status = napi_get_value_uint32(env, jsField, &cPkg->seq);
		assert(status == napi_ok);
		cPkg->cmdflags |= BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ;
	}

	status = napi_get_named_property(env, jsPkg, "snPeerid", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		if (peerid_js2c(env, jsField, &cPkg->snPeerid) == BFX_RESULT_SUCCESS)
		{
			cPkg->cmdflags |= BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID;
		}
	}

	status = napi_get_named_property(env, jsPkg, "result", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		uint32_t u32 = 0;
		status = napi_get_value_uint32(env, jsField, &u32);
		assert(status == napi_ok);
		cPkg->result = (uint8_t)u32;
		cPkg->cmdflags |= BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT;
	}

	status = napi_get_named_property(env, jsPkg, "peerInfo", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		BdtPeerInfoBuilder* peerInfoBuilder = peerinfo_js2c(env, jsField, NULL, NULL, NULL);
		if (peerInfoBuilder != NULL)
		{
			cPkg->peerInfo = BdtPeerInfoBuilderFinish(peerInfoBuilder, false);
			if (cPkg->peerInfo != NULL)
			{
				cPkg->cmdflags |= BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO;
			}
		}
	}

	status = napi_get_named_property(env, jsPkg, "endpointArray", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		if (peer_endpoint_list_js2c(env, jsField, &cPkg->endpointArray) == BFX_RESULT_SUCCESS)
		{
			cPkg->cmdflags |= BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY;
		}
	}

	return cPkg;
}

static napi_value exchange_key_c2js(napi_env env, const Bdt_ExchangeKeyPackage* pkg)
{
	napi_status status = napi_ok;
	napi_value jsPkg = NULL;
	status = napi_create_object(env, &jsPkg);
	assert(status == napi_ok);

	napi_value jsField = NULL;
	void* buffer = NULL;

	status = napi_create_uint32(env, pkg->cmdtype, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdType", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, (uint32_t)pkg->cmdflags, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "cmdFlags", jsField);
	assert(status == napi_ok);

	status = napi_create_uint32(env, pkg->seq, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "sequence", jsField);
	assert(status == napi_ok);

	status = napi_create_buffer_copy(env, sizeof(pkg->seqAndKeySign), pkg->seqAndKeySign, &buffer, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "seqAndKeySign", jsField);
	assert(status == napi_ok);

	jsField = peerid_c2js(env, BdtPeerInfoGetPeerid(pkg->peerInfo));
	status = napi_set_named_property(env, jsPkg, "fromPeerid", jsField);
	assert(status == napi_ok);

	jsField = peerinfo_c2js(env, pkg->peerInfo, jsField, NULL, NULL);
	status = napi_set_named_property(env, jsPkg, "peerInfo", jsField);
	assert(status == napi_ok);

	status = napi_create_double(env, (double)pkg->sendTime, &jsField);
	assert(status == napi_ok);
	status = napi_set_named_property(env, jsPkg, "sendTime", jsField);
	assert(status == napi_ok);

	return jsPkg;
}

static Bdt_ExchangeKeyPackage* exchange_key_js2c(napi_env env, napi_value jsPkg)
{
	napi_valuetype jsType = napi_undefined;
	napi_typeof(env, jsPkg, &jsType);
	if (jsType != napi_object)
	{
		return NULL;
	}

	Bdt_ExchangeKeyPackage* cPkg = Bdt_ExchangeKeyPackageCreate();
	cPkg->cmdflags = 0;

	napi_status status = napi_ok;
	napi_value jsField = NULL;
	void* resultBuffer = NULL;
	size_t bufferSize = 0;

	status = napi_get_named_property(env, jsPkg, "sequence", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		status = napi_get_value_uint32(env, jsField, &cPkg->seq);
		assert(status == napi_ok);
		cPkg->cmdflags |= BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQ;
	}

	status = napi_get_named_property(env, jsPkg, "seqAndKeySign", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		status = napi_get_buffer_info(env, jsField, &resultBuffer, &bufferSize);
		assert(status == napi_ok);
		memcpy(cPkg->seqAndKeySign, resultBuffer, min(sizeof(cPkg->seqAndKeySign), bufferSize));
		cPkg->cmdflags |= BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQANDKEYSIGN;
	}

	status = napi_get_named_property(env, jsPkg, "peerInfo", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		BdtPeerInfoBuilder* peerInfoBuilder = peerinfo_js2c(env, jsField, NULL, NULL, NULL);
		if (peerInfoBuilder != NULL)
		{
			cPkg->peerInfo = BdtPeerInfoBuilderFinish(peerInfoBuilder, false);
			if (cPkg->peerInfo != NULL)
			{
				cPkg->cmdflags |= BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO;
			}
		}
	}
	else
	{
		status = napi_get_named_property(env, jsPkg, "fromPeerid", &jsField);
		if (status == napi_ok && jsField != NULL)
		{
			if (peerid_js2c(env, jsField, &cPkg->fromPeerid) == BFX_RESULT_SUCCESS)
			{
				cPkg->cmdflags |= BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID;
			}
		}
	}

	status = napi_get_named_property(env, jsPkg, "sendTime", &jsField);
	if (status == napi_ok && jsField != NULL)
	{
		double sendTime = 0;
		status = napi_get_value_double(env, jsField, &sendTime);
		cPkg->sendTime = (uint64_t)sendTime;
		assert(status == napi_ok);
		cPkg->cmdflags |= BDT_EXCHANGEKEY_PACKAGE_FLAG_SENDTIME;
	}

	return cPkg;
}

static napi_value sign_local_peer_info(napi_env env, napi_callback_info args)
{
	napi_status status;

	size_t argc = 2;
	napi_value inArgs[2] = { NULL, NULL };
	napi_value thisArg = NULL;
	void* data = NULL;
	status = napi_get_cb_info(env, args, &argc, inArgs, &thisArg, &data);
	if (status != napi_ok || argc < 2)
	{
		return NULL;
	}

	napi_value jsPeerInfo = inArgs[0];

	BdtPeerSecret secret;

	void* secretBuffer = NULL;
	size_t length = 0;
	status = napi_get_buffer_info(env, inArgs[1], &secretBuffer, &length);
	if (status != napi_ok || secretBuffer == NULL || length == 0 || length > sizeof(secret.secret))
	{
		return NULL;
	}

	BdtPeerInfoBuilder* peerInfoBuilder = peerinfo_js2c(env, jsPeerInfo, NULL, NULL, NULL);
	if (peerInfoBuilder == NULL)
	{
		return NULL;
	}

	secret.secretLength = length;
	memcpy(&secret.secret, secretBuffer, length);
	BdtPeerInfoSetSecret(peerInfoBuilder, &secret);
	const BdtPeerInfo* peerInfo = BdtPeerInfoBuilderFinish(peerInfoBuilder, &secret);

	napi_value jsSignedPeerInfo = peerinfo_c2js(env, peerInfo, NULL, NULL, NULL);
	BdtReleasePeerInfo(peerInfo);
	return jsSignedPeerInfo;
}