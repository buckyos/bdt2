#ifndef __BDT_UDP_INTERFACE_H__
#define __BDT_UDP_INTERFACE_H__
#include "../BdtSystemFramework.h"
#include "../Protocol/Package.h"

typedef struct Bdt_UdpInterface Bdt_UdpInterface;

int32_t Bdt_UdpInterfaceAddRef(const Bdt_UdpInterface* ui);
int32_t Bdt_UdpInterfaceRelease(const Bdt_UdpInterface* ui);
BDT_SYSTEM_UDP_SOCKET Bdt_UdpInterfaceGetSocket(const Bdt_UdpInterface* ui);
const BdtEndpoint* Bdt_UdpInterfaceGetLocal(const Bdt_UdpInterface* ui);


uint32_t Bdt_BoxUdpPackage(
	const Bdt_Package** packages,
	size_t count, 
	const Bdt_Package* refPackage,
	uint8_t* buffer,
	size_t* inoutLength
);

uint32_t Bdt_BoxEncryptedUdpPackage(
	const Bdt_Package** packages,
	size_t count,
	const Bdt_Package* refPackage, 
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	uint8_t* buffer,
	size_t* inoutLength
);

// 当前能进行对称加解密的只有RSA key
// 这里需要确定的public和secret长度
uint32_t Bdt_BoxEncryptedUdpPackageStartWithExchange(
	const Bdt_Package** packages,
	size_t count,
	const Bdt_Package* refPackage,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	uint8_t remotePublicType,
	const uint8_t* remotePublic,
	uint16_t secretLength,
	const uint8_t* secret,
	uint8_t* buffer,
	size_t* inoutLength
);

uint32_t Bdt_UnboxUdpPackage(
	BdtStack* stack,
	const uint8_t* data,
	size_t bytes,
	Bdt_Package** packages,
	size_t count, 
	const Bdt_Package* refPackage, 
	uint8_t outKey[BDT_AES_KEY_LENGTH],
	BdtPeerid* outKeyPeerid,
	bool* outIsEncrypto,
	bool* outHasExchange);


#endif //__BDT_UDP_INTERFACE_H__
