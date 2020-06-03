#include <assert.h>
#include <time.h>

#include <BuckyBase/BuckyBase.h>
#include <BDTCore/BdtCore.h>
#include <BDTCore/Protocol/Package.h>
#include <BDTCore/Protocol/PackageEncoder.h>
#include <BDTCore/Protocol/PackageDecoder.h>
#include <BDTCore/Global/PeerInfo.h>
#include <BDTCore/Global/CryptoToolKit.h>

void print_bits(char x);
void print_bits2(int x);
void print_chars_bits(char* x, int size);

void testPeerid() {
	printf("test peerid \n");

	BdtPeerConstInfo constInfo;
	constInfo.areaCode.continent = 0x10;
	constInfo.areaCode.country = 0x02;
	constInfo.areaCode.carrier = 0x03;
	constInfo.areaCode.city = 0x3001;
	constInfo.areaCode.inner = 0x05;
	uint8_t _deviceId[6] = { 'd','e','v','i','c','e' };
	memcpy(constInfo.deviceId, _deviceId, 6);
	//pInfo->constInfo.deviceId = { 'd','e','v','i','c','e' };
	constInfo.publicKeyType = BDT_PEER_PUBLIC_KEY_TYPE_RSA1024;

	BdtPeerid* peerid = (BdtPeerid*)malloc(sizeof(BdtPeerid));
	BdtPeerSecret secret;
	
	uint32_t ret = BdtCreatePeerid(&(constInfo.areaCode), _deviceId, constInfo.publicKeyType, peerid, &constInfo, &secret);

	printf("BdtCreatePeerid ret %d  \n", ret);

	// test rsa key
	printf("test rsa key  \n");
	uint8_t input[] = "test0000";
	uint8_t input2[10] = { 0 };
	size_t input2Len = 10;
	uint8_t output[1024] = { 0 };
	size_t olen = 1024;
	size_t keyBits = 0;
	size_t publicLen = BdtGetPublicKeyLength(constInfo.publicKeyType);
	size_t secretLen = BdtGetSecretKeyMaxLength(constInfo.publicKeyType);
	// encrypt
	Bdt_RsaEncrypt(input, sizeof(input), (uint8_t*)constInfo.publicKey.rsa1024, publicLen, output, &olen, olen);
	// decrypt
	Bdt_RsaDecrypt(output, (uint8_t*)secret.secret.rsa1024, secretLen, input2, &input2Len, input2Len, &keyBits);
	printf("decrypt result :%s", input2);
	printf("================================ \n");


	printf("print  pid raw => \n");
	BdtPeerid pid;
	BdtGetPeeridFromConstInfo(&constInfo, &pid);
	print_chars_bits(pid.pid, 32);
	printf("end <==== \n");

	/*
	uint8_t hash[32];
	Bdt_Sha256Hash(constInfo.publicKey.rsa1024, 162, hash);
	print_chars_bits(hash, sizeof(hash));
	printf("=>\n");
	printf("\n");
	uint8_t result[32];
	encode(&(constInfo.areaCode), hash, result);
	print_chars_bits(result, 32);
	printf("\n");
	*/
}

void print_chars_bits(char* x, int size) {
	for (int j = 0; j < size; j++) {
		for (int i = 8 * sizeof(x[j]) - 1; i >= 0; i--) {
			(x[j] & (1 << i)) ? putchar('1') : putchar('0');
		}
	}
}

void print_bits(char x)
{
	for (int i = 8 * sizeof(x) - 1; i >= 0; i--) {
		(x & (1 << i)) ? putchar('1') : putchar('0');
	}
	printf("\n");
}

void print_bits2(int x)
{
	for (int i = 8 * sizeof(x) - 1; i >= 0; i--) {
		(x & (1 << i)) ? putchar('1') : putchar('0');
	}
	printf("\n");
}



