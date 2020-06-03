#ifndef __BDT_TCP_INTERFACE_INL__
#define __BDT_TCP_INTERFACE_INL__
#ifndef __BDT_INTERFACE_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of interface module"
#endif //__BDT_INTERFACE_MODULE_IMPL__
#include "./TcpInterface.h"

#define TCP_INTERFACE_CACHE_SIZE (64*1024)
//sizeof(cryptoAESKey) + sizeof(64bit key hash) + sizeof(boxsize)
#define TCP_FIRST_PACKAGE_SIZE 6
#define TCP_FIRST_MIXHASH_SIZE  (BDT_MIX_HASH_LENGTH + 2)
#define TCP_FIRST_EXCHANGE_SIZE 138
#define TCP_FIRST_BLCOK_SIZE 138
#define TCP_FIRST_PACKAGE_MAX_SIZE 4096
#define TCP_FIRST_PACKAGE_MIN_SIZE 40
#define TCP_MAX_BOX_LEN 32

typedef Bdt_TcpListener TcpListener;
static TcpListener* TcpListenerCreate(BdtSystemFramework* framework, const BdtEndpoint* localEndpoint);
static uint32_t TcpListenerListen(TcpListener* listener);
static const BdtEndpoint* TcpListenerGetLocal(const TcpListener* tl);

struct Bdt_TcpInterface
{
	int32_t refCount;
	
	BfxRwLock stateLock;
	// stateLock protected members begin
	int32_t socketRefCount;
	Bdt_TcpInterfaceState state;
	Bdt_TcpInterfaceOwner* owner;
	// stateLock protected members end

	const BdtNetTcpConfig* config;
	BdtSystemFramework* framework;
	BDT_SYSTEM_TCP_SOCKET socket;
	BdtEndpoint localEndpoint;
	BdtEndpoint remoteEndpoint;

	//for unbox parser 
	BDT_SYSTEM_TIMER waitFirstPackageTimer;
	size_t cacheLength;
	//uint8_t* cache[TCP_INTERFACE_CACHE_SIZE];
	uint8_t* cache;
	uint8_t* backCache;
	uint8_t cacheData[TCP_INTERFACE_CACHE_SIZE];
	uint8_t backCacheData[TCP_INTERFACE_CACHE_SIZE];
	bool isCrypto;
	uint8_t aesKey[BDT_AES_KEY_LENGTH];
	size_t firstBlockSize;
	uint8_t unboxState;
	uint16_t boxSize;
	size_t boxCount;

	//for debug
	char* name;
};


static Bdt_TcpInterface* TcpInterfaceCreate(
	BdtSystemFramework* framework,
	const BdtEndpoint* localEndpoint,
	const BdtEndpoint* remoteEndpoint,
	const BdtNetTcpConfig* config,
	BDT_SYSTEM_TCP_SOCKET passiveSocket, 
	// just to name tcp interface
	int32_t localId
);

typedef enum TcpIntefaceUnboxState
{
	BDT_TCP_UNBOX_STATE_READING_LENGTH,
	BDT_TCP_UNBOX_STATE_READING_BODY,
} TcpIntefaceUnboxState;

#endif //__BDT_TCP_INTERFACE_INL__