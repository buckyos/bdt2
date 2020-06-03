#ifndef _BUCKY_BDTCORE_H_
#define _BUCKY_BDTCORE_H_

#include <BuckyBase/BuckyBase.h>

#if defined(BFX_COMPILER_MSVC)
#if defined(BFX_STATIC)
#define BDT_API(x) BFX_EXTERN_C x
#elif defined(BDT_EXPORTS)
#define BDT_API(x) BFX_EXTERN_C __declspec(dllexport) x /*__stdcall*/ 
#else // BDT_EXPORTS
#define BDT_API(x) BFX_EXTERN_C __declspec(dllimport) x /*__stdcall*/ 
#endif // BDT_EXPORTS
#elif defined(BFX_COMPILER_GCC)
#if defined(BFX_STATIC)
#define BDT_API(x) BFX_EXTERN_C x
#elif defined(BDT_EXPORTS)
#define BDT_API(x) BFX_EXTERN_C __attribute__((__visibility__("default")/*, __stdcall__*/)) x
#else // BDT_EXPORTS
#define BDT_API(x) BFX_EXTERN_C __attribute__((__visibility__("default")/*, __stdcall__*/)) x 
#endif // BDT_EXPORTS
#endif // complier

//TODO: just an ipv4 address now
#define BDT_ENDPOINT_PROTOCOL_UNK		(1<<0)
#define BDT_ENDPOINT_PROTOCOL_TCP		(1<<1)
#define BDT_ENDPOINT_PROTOCOL_UDP		(1<<2)
#define BDT_ENDPOINT_IP_VERSION_UNK		(1<<0)
#define BDT_ENDPOINT_IP_VERSION_4		(1<<3)
#define BDT_ENDPOINT_IP_VERSION_6		(1<<4)
#define BDT_ENDPOINT_FLAG_STATIC_WAN	(1<<6)
#define BDT_ENDPOINT_FLAG_SIGNED		(1<<7)

#define BDT_ENDPOINT_STRING_MAX_LENGTH	53
typedef struct BdtEndpoint
{
	uint8_t flag;
	uint8_t reserve;
	uint16_t port;
	union {
		uint8_t address[4];
		uint8_t addressV6[16];
	};

} BdtEndpoint;
BDT_API(uint32_t) BdtEndpointFromString(BdtEndpoint* endpoint, const char* str);
BDT_API(void) BdtEndpointToString(const BdtEndpoint* endpoint, char name[BDT_ENDPOINT_STRING_MAX_LENGTH + 1]);
BDT_API(int) BdtEndpointCompare(const BdtEndpoint* left, const BdtEndpoint* right, bool ignorePort);
BDT_API(bool) BdtEndpointIsSameIpVersion(const BdtEndpoint* left, const BdtEndpoint* right);
BDT_API(void) BdtEndpointCopy(BdtEndpoint* dest, const BdtEndpoint* src);

BDT_API(int) BdtEndpointToSockAddr(const BdtEndpoint* ep, struct sockaddr* addr);
BDT_API(int) SockAddrToBdtEndpoint(const struct sockaddr* addr, BdtEndpoint* ep, uint8_t protocol);
BDT_API(int) BdtEndpointGetFamily(const BdtEndpoint* ep);
BDT_API(size_t) BdtEndpointGetSockaddrSize(const BdtEndpoint* ep);

typedef struct BdtEndpointArray
{
	BdtEndpoint* list;
	size_t size;
	size_t allocSize;
} BdtEndpointArray;

BDT_API(int) BdtEndpointArrayInit(BdtEndpointArray* pArray,size_t allocSize);
BDT_API(int) BdtEndpointArrayPush(BdtEndpointArray* pArray, const BdtEndpoint* pEndpoint);
BDT_API(int) BdtEndpointArrayUninit(BdtEndpointArray* pArray);
BDT_API(bool) BdtEndpointArrayIsEqual(const BdtEndpointArray* left, const BdtEndpointArray* right);
BDT_API(void) BdtEndpointArrayClone(const BdtEndpointArray* src, BdtEndpointArray* dest);

typedef struct BdtStreamRange
{
	uint64_t pos;
	uint32_t length;
} BdtStreamRange;

typedef struct BdtStreamArray
{
	BdtStreamRange* list;
	size_t size;
	size_t allocSize;
} BdtStreamArray;

BDT_API(int) BdtStreamArrayInit(BdtStreamArray* pArray, size_t allocSize);
BDT_API(int) BdtStreamArrayPush(BdtStreamArray* pArray, BdtStreamRange* pStreamRange);
BDT_API(int) BdtStreamArrayUninit(BdtStreamArray* pArray);

typedef struct BdtPeerArea
{
	uint8_t continent;
	uint8_t country;
	uint8_t carrier;
	uint16_t city;
	uint8_t inner;
} BdtPeerArea;

#define BDT_PEERID_LENGTH 32
#define BDT_PEERID_STRING_LENGTH (2 * BDT_PEERID_LENGTH)
typedef struct BdtPeerid
{
	uint8_t pid[BDT_PEERID_LENGTH];
} BdtPeerid;


#define BDT_PEER_PUBLIC_KEY_TYPE_RSA1024		0
#define BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024		162
#define BDT_PEER_SECRET_KEY_MAX_LENGTH_RSA1024	610
#define BDT_PEER_PUBLIC_KEY_TYPE_RSA2048		1
#define BDT_PEER_PUBLIC_KEY_LENTGH_RSA2048		294
#define BDT_PEER_SECRET_KEY_MAX_LENGTH_RSA2048	1194
#define BDT_PEER_PUBLIC_KEY_TYPE_RSA3072		2
#define BDT_PEER_PUBLIC_KEY_LENTGH_RSA3072		422
#define BDT_PEER_SECRET_KEY_MAX_LENGTH_RSA3072	1770
#define BDT_PEER_LENGTH_SIGNATRUE				128

#define BDT_MAX_DEVICE_ID_LENGTH 128
typedef struct BdtPeerConstInfo 
{
	BdtPeerArea areaCode;
	uint8_t deviceId[BDT_MAX_DEVICE_ID_LENGTH];
	uint8_t publicKeyType;
	union {
		uint8_t rsa1024[BDT_PEER_PUBLIC_KEY_LENTGH_RSA1024];
		uint8_t rsa2048[BDT_PEER_PUBLIC_KEY_LENTGH_RSA2048];
		uint8_t rsa3072[BDT_PEER_PUBLIC_KEY_LENTGH_RSA3072];
	} publicKey;
	//uint32_t difficulty;
	//uint8_t nonce[8];
} BdtPeerConstInfo;

typedef struct BdtPeerSecret
{
	union {
		uint8_t rsa1024[BDT_PEER_SECRET_KEY_MAX_LENGTH_RSA1024];
		uint8_t rsa2048[BDT_PEER_SECRET_KEY_MAX_LENGTH_RSA2048];
		uint8_t rsa3072[BDT_PEER_SECRET_KEY_MAX_LENGTH_RSA3072];
	} secret;
	uint16_t secretLength;
} BdtPeerSecret;

BDT_API(int32_t) BdtPeeridCompare(const BdtPeerid* left, const BdtPeerid* right);
BDT_API(bool) BdtPeeridIsGroup(const BdtPeerid* peerid);

BDT_API(void) BdtPeeridToString(const BdtPeerid* peerid, char out[BDT_PEERID_STRING_LENGTH]);
BDT_API(uint32_t) BdtGetPublicKeyLength(uint8_t keyType);
BDT_API(uint32_t) BdtGetSecretKeyMaxLength(uint8_t keyType);
BDT_API(uint32_t) BdtGetKeyBitLength(uint8_t keyType);

BDT_API(uint32_t) BdtCreatePeerid(
	const BdtPeerArea* areaCode,
	const char* deviceId,
	uint8_t publicKeyType,
	BdtPeerid* outPeerid,
	BdtPeerConstInfo* outConstInfo,
	BdtPeerSecret* outSecret
);
BDT_API(uint32_t) BdtGetPeeridFromConstInfo(const BdtPeerConstInfo* constInfo, BdtPeerid* outPeerid);
BDT_API(uint32_t) BdtPeeridVerifyConstInfo(const BdtPeerid* peerid, const BdtPeerConstInfo* constInfo);
//TODO:添加API，允许用户手工增加known peer
//void BdtUdpateKnownPeer(peerid,peerinfo)
//void BdtGetAllKnownPeer();

// read only!!!
typedef struct BdtPeerInfo BdtPeerInfo;
BDT_API(int32_t) BdtAddRefPeerInfo(const BdtPeerInfo* peerInfo);
BDT_API(int32_t) BdtReleasePeerInfo(const BdtPeerInfo* peerInfo);
BDT_API(const BdtEndpoint*) BdtPeerInfoListEndpoint(const BdtPeerInfo* peerInfo, size_t* outCount);
BDT_API(const BdtPeerConstInfo*) BdtPeerInfoGetConstInfo(const BdtPeerInfo* peerInfo);
BDT_API(const BdtPeerid*) BdtPeerInfoGetPeerid(const BdtPeerInfo* peerInfo);
BDT_API(const BdtPeerid*) BdtPeerInfoListSn(const BdtPeerInfo* peerInfo, size_t* outCount);
BDT_API(const BdtPeerInfo**) BdtPeerInfoListSnInfo(const BdtPeerInfo* peerInfo, size_t* outCount);
BDT_API(const BdtPeerSecret*) BdtPeerInfoGetSecret(const BdtPeerInfo* peerInfo);
BDT_API(bool) BdtPeerInfoIsSigned(const BdtPeerInfo* peerInfo);
BDT_API(bool) BdtPeerInfoIsEqual(const BdtPeerInfo* lhs, const BdtPeerInfo* rhs);
BDT_API(uint64_t) BdtPeerInfoGetCreateTime(const BdtPeerInfo* peerInfo);
//TODO 为测试，可能随时删除
BDT_API(BdtPeerInfo*) BdtPeerInfoCreateForTest();
BDT_API(uint32_t) BdtPeerInfoEncodeForTest(const BdtPeerInfo* peerInfo, BfxBufferStream* stream);
BDT_API(uint32_t) BdtPeerInfoDecodeForTest(BfxBufferStream* stream, BdtPeerInfo* peerInfo);

#define BDT_PEER_INFO_BUILD_FLAG_NULL           (1<<0)
#define BDT_PEER_INFO_BUILD_FLAG_HAS_SECRET		(1<<1)
#define BDT_PEER_INFO_BUILD_FLAG_HAS_SN_LIST	(1<<2)
#define BDT_PEER_INFO_BUILD_FLAG_HAS_SN_INFO	(1<<3)


typedef struct BdtPeerInfoBuilder BdtPeerInfoBuilder;
BDT_API(BdtPeerInfoBuilder*) BdtPeerInfoBuilderBegin(uint32_t flags);
BDT_API(void) BdtPeerInfoClone(BdtPeerInfoBuilder* builder, const BdtPeerInfo* proto);
BDT_API(void) BdtPeerInfoSetConstInfo(BdtPeerInfoBuilder* builder, const BdtPeerConstInfo* constInfo);
BDT_API(void) BdtPeerInfoAddEndpoint(BdtPeerInfoBuilder* builder, const BdtEndpoint* endpoint);
BDT_API(void) BdtPeerInfoClearEndpoint(BdtPeerInfoBuilder* builder);
BDT_API(void) BdtPeerInfoAddSn(BdtPeerInfoBuilder* builder, const BdtPeerid* peerid);
BDT_API(void) BdtPeerInfoAddSnInfo(BdtPeerInfoBuilder* builder, const BdtPeerInfo* peerInfo);
BDT_API(void) BdtPeerInfoSetSecret(BdtPeerInfoBuilder* builder, const BdtPeerSecret* secret);
BDT_API(const BdtPeerInfo*) BdtPeerInfoBuilderFinish(BdtPeerInfoBuilder* builder, const BdtPeerSecret* signSecret);

typedef struct BdtNetTcpConfig
{
	uint32_t firstPackageWaitTime;
} BdtNetTcpConfig;
typedef struct BdtNetConfig
{
	BdtNetTcpConfig tcp;
} BdtNetConfig;

typedef struct UdpTunnelConfig
{
	int pingInterval;
	int pingOvertime;
} UdpTunnelConfig;

typedef struct BdtTcpTunnelConfig
{
	int connectTimeout;
	int sendBufferSize;
} BdtTcpTunnelConfig;

typedef struct BdtTunnelBuilderConfig
{
	uint16_t holePunchCycle;
	uint16_t holePunchTimeout;
	uint16_t findPeerTimeout;
} BdtTunnelBuilderConfig;

typedef struct BdtTunnelHistoryConfig
{
	uint16_t respCacheSize;
	uint16_t tcpLogCacheSize;
	uint32_t logKeepTime; // ms
} BdtTunnelHistoryConfig;

typedef struct BdtTunnelConfig
{
	BdtTunnelHistoryConfig history;
	BdtTunnelBuilderConfig builder;
	UdpTunnelConfig udpTunnel;
	BdtTcpTunnelConfig tcpTunnel;
} BdtTunnelConfig;

typedef struct BdtPackageConnectionConfig
{
	uint32_t resendInterval;
	uint32_t recvTimeout;
	uint32_t msl;
	uint32_t mtu;
	uint32_t mss;
	size_t maxRecvBuffer;
	size_t maxSendBuffer;
	bool halfOpen;
}BdtPackageConnectionConfig;

typedef struct BdtStreamConnectionConfig
{
	uint16_t minRecordSize;
	uint16_t maxRecordSize;
	uint8_t maxNagleDelay;
	// resend interval of revers connect request
	//uint16_t resendInterval;
	uint32_t recvTimeout;
} BdtStreamConnectionConfig;

typedef struct BdtConnectionConfig
{
	BdtPackageConnectionConfig package;
	BdtStreamConnectionConfig stream;
	uint32_t connectTimeout;
} BdtConnectionConfig;

typedef struct BdtAcceptConnectionConfig
{
	uint16_t backlog;
	BdtConnectionConfig connection;
} BdtAcceptConnectionConfig;

typedef struct BdtSuperNodeClientConfig
{
	// ping
	uint32_t onlineSnLimit; // 上线SN数量
	uint32_t snRefreshInterval; // 更新SN列表间隔
	uint32_t pingIntervalInit; // SN响应前ping间隔
	uint32_t pingInterval; // SN响应后ping间隔
	uint32_t offlineTimeout; // SN在线测试超时时间
	// call
	uint32_t callSnLimit; // 穿透SN的数量；向对端节点发起连接时，要向对端归属SN发起call操作
	uint32_t callInterval; // 对SN发起call的时间间隔
	uint32_t callTimeout; // call的超时时间
} BdtSuperNodeClientConfig;

typedef struct BdtPeerHistoryConfig
{
	uint32_t activeTime; // 保持活跃时间，S
	uint32_t peerCapacity;
} BdtPeerHistoryConfig;

typedef struct BdtAesKeyConfig
{
	uint32_t activeTime; // 保持活跃时间，S
} BdtAesKeyConfig;

typedef struct BdtStackConfig
{
	BdtNetConfig net;
	BdtTunnelConfig tunnel;
	BdtAcceptConnectionConfig acceptConnection;
	BdtConnectionConfig connection;
	BdtSuperNodeClientConfig snClient;
	BdtPeerHistoryConfig peerHistoryConfig;
	BdtAesKeyConfig aesKeyConfig;
} BdtStackConfig;

typedef struct BdtStack BdtStack;

// add peer info to static cache
BDT_API(void) BdtAddStaticPeerInfo(BdtStack* stack, const BdtPeerInfo* peerInfo);


typedef struct BdtNetModifier BdtNetModifier;
BDT_API(BdtNetModifier*) BdtCreateNetModifier(
	BdtStack* stack
);
BDT_API(uint32_t) BdtChangeLocalAddress(
	BdtNetModifier* modifier,
	const BdtEndpoint* src, 
	const BdtEndpoint* dst);
BDT_API(uint32_t) BdtAddLocalEndpoint(
	BdtNetModifier* modifier,
	const BdtEndpoint* toAdd
);
BDT_API(uint32_t) BdtRemoveLocalEndpoint(
	BdtNetModifier* modifier,
	const BdtEndpoint* toRemove
);
BDT_API(uint32_t) BdtApplyBdtNetModifier(
	BdtStack* stack, 
	BdtNetModifier* modifier
);

// when stack finish create
// eventParam is NULL
#define BDT_STACK_EVENT_CREATE			1

// when error accured on stack 
// eventParam is uint32_t* error code
#define BDT_STACK_EVENT_ERROR			20
typedef void (BFX_STDCALL* StackEventCallback)(BdtStack* stack, uint32_t eventID, const void* eventParam, void* userData);

BDT_API(uint32_t) BdtCloseStack(BdtStack* stack);

// when connection enter establish state
#define BDT_CONNECTION_EVENT_ESTABLISH		1
// when connection enter close state
#define BDT_CONNECTION_EVENT_CLOSE			4
// when connection got an error
// eventParam is uint32_t error code
#define BDT_CONNECTION_EVENT_ERROR			10

typedef enum BdtConnectionState
{
	BDT_CONNECTION_STATE_NONE = 0,
	BDT_CONNECTION_STATE_CONNECTING,
	BDT_CONNECTION_STATE_ESTABLISH,
	BDT_CONNECTION_STATE_CLOSING,
	BDT_CONNECTION_STATE_CLOSED
} BdtConnectionState;


// if tcp flag set, tunnel builder try to build a tcp tunnel
#define BDT_BUILD_TUNNEL_PARAMS_FLAG_TCP			(1<<1)

// if udp flag set, tunnel builder try to build a udp tunnel
#define BDT_BUILD_TUNNEL_PARAMS_FLAG_UDP			(1<<2)

// if key flag set, tunnel builder will use key in params as crypto key
// otherwise tunnel builder will use an existing key or create a new key
#define BDT_BUILD_TUNNEL_PARAMS_FLAG_KEY			(1<<4)

// if sn flag set, tunnel builder will call sn list in params 
// otherwise tunnel builder will find which super nodes that remote peer has registered on them 
#define BDT_BUILD_TUNNEL_PARAMS_FLAG_SN				(1<<5)

// if endpoint flag set, tunnel builder will try to build a tunnel from localEndpoint to remoteEndpoint in params
#define BDT_BUILD_TUNNEL_PARAMS_FLAG_ENDPOINT		(1<<6)


// all flags in params means perfer but not must, tunnel builder not able to promise build a tunnel satisfy all flags in params
// if neither tcp nor udp flag set, tunnel builder will use default strategy (tcp first) to build tunnel
typedef struct BdtBuildTunnelParams
{
	const BdtPeerConstInfo* remoteConstInfo;
	int32_t flags;
	const uint8_t* key;
	const BdtEndpoint* localEndpoint;
	const BdtEndpoint* remoteEndpoint;
	const BdtPeerid* snList;
	size_t snListLength;
} BdtBuildTunnelParams;

typedef struct BdtConnection BdtConnection;

typedef void (BFX_STDCALL* BdtConnectionConnectCallback)(
	BdtConnection* connection, 
	uint32_t result, 
	void* userData);

typedef void (BFX_STDCALL* BdtConnectionFirstAnswerCallback)(const uint8_t* data, size_t len, void* userData);

BDT_API(uint32_t) BdtCreateConnection(
	BdtStack* stack,
	const BdtPeerid* remotePeerid, 
	bool encrypted, 
	BdtConnection** outConnection);

BDT_API(uint32_t) BdtConnectionConnect(
	BdtConnection* connection,
	uint16_t vport,
	const BdtBuildTunnelParams* params,
	const uint8_t* firstQuestion,
	size_t len,
	BdtConnectionConnectCallback connectCallback,
	const BfxUserData* connectUserData,
	BdtConnectionFirstAnswerCallback faCallback,
	const BfxUserData* faUserData);

BDT_API(uint32_t) BdtListenConnection(
	BdtStack* stack, 
	uint16_t port, 
	const BdtAcceptConnectionConfig* config);

typedef void (BFX_STDCALL* BdtAcceptConnectionCallback)(
	uint32_t result, 
	const uint8_t* buffer, 
	size_t recv, 
	BdtConnection* connection, 
	void* userData);
BDT_API(uint32_t) BdtAcceptConnection(
	BdtStack* stack,
	uint16_t port, 
	BdtAcceptConnectionCallback callback,
	const BfxUserData* userData
);

BDT_API(uint32_t) BdtConfirmConnection(
	BdtConnection* connection,
	uint8_t* answer,
	size_t len, 
	BdtConnectionConnectCallback connectCallback, 
	const BfxUserData* userData
);

#define BDT_CONNECTION_PROVIDER_TYPE_WAITING			0
#define BDT_CONNECTION_PROVIDER_TYPE_PACKAGE			1
#define BDT_CONNECTION_PROVIDER_TYPE_STREAM				2

BDT_API(uint32_t) BdtConnectionProviderType(BdtConnection* connection);
BDT_API(const char*) BdtConnectionGetProviderName(BdtConnection* connection);

BDT_API(int32_t) BdtAddRefConnection(BdtConnection* connection);
BDT_API(int32_t) BdtReleaseConnection(BdtConnection* connection);



BDT_API(uint32_t) BdtConnectionClose(
	BdtConnection* connection
);


BDT_API(uint32_t) BdtConnectionReset(BdtConnection* connection);
typedef void (BFX_STDCALL* BdtConnectionSendCallback)(
	uint32_t result, 
	const uint8_t* buffer, 
	size_t length, 
	void* userData);
BDT_API(uint32_t) BdtConnectionSend(
	BdtConnection* connection, 
	const uint8_t* buffer, 
	size_t length, 
	BdtConnectionSendCallback callback, 
	const BfxUserData* userData);


typedef void (BFX_STDCALL* BdtConnectionRecvCallback)(
	uint8_t* buffer,
	size_t recv,
	void* userData);
BDT_API(uint32_t) BdtConnectionRecv(
	BdtConnection* connection,
	uint8_t* buffer,
	size_t length,
	BdtConnectionRecvCallback callback,
	const BfxUserData* userData);

BDT_API(const char*) BdtConnectionGetName(BdtConnection* connection);

typedef struct BdtStatisticsSnapshot {
    size_t size;

    struct {
        uint32_t total;
        uint32_t failActive;
        uint32_t failPassive;
        uint32_t succActive;
        uint32_t succPassive;
        uint32_t tcp;
        uint32_t udp;
    } connectionCount;

    struct {
        uint32_t total;
        uint32_t udp;
        uint32_t tcp;
    } speed;
} BdtStatisticsSnapshot;

BDT_API(uint32_t) BdtGetStatisticsSnapshot(BdtStack* stack, BdtStatisticsSnapshot* snapshot);

//Tunnel API
// void BDTTunnelBind(vport_id,onRecvFrom)
// void BDTTunnelSend(port,data)

#endif //_BUCKY_BDTCORE_H_