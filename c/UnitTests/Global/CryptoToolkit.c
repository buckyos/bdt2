//
//int testGenKeyAndEncryptDecrypt() {
//	uint8_t secretKeyDer[1024] = { 0 };
//	size_t secretLen = 1024;
//	uint8_t publicKeyDer[1024] = { 0 };
//	size_t publicLen = 1024;
//	int ret = Bdt_RsaGenerateToBuffer(1024, publicKeyDer, &publicLen, secretKeyDer, &secretLen);
//	uint8_t input[] = "test0000";
//	uint8_t input2[10] = { 0 };
//	size_t input2Len = 10;
//	uint8_t output[1024] = { 0 };
//	size_t olen = 1024;
//	size_t keyBits = 0;
//	ret = Bdt_RsaEncrypt(input, sizeof(input), publicKeyDer + 1024 - publicLen, publicLen, output, &olen, olen);
//	ret = Bdt_RsaDecrypt(output, secretKeyDer + 1024 - secretLen, secretLen, input2, &input2Len, input2Len, &keyBits);
//	printf("decrypt result :%s", input2);
//	return 0;
//}
