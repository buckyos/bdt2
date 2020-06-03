#ifndef _BDT_PACKAGE_H_
#define _BDT_PACKAGE_H_

#include "../BdtCore.h"
#include "../History/KeyManager.h"

#define BDT_PROTOCOL_UDP_MAGIC_LENGTH 2
#define BDT_PROTOCOL_UDP_MAGIC_NUM_0 0x00
#define BDT_PROTOCOL_UDP_MAGIC_NUM_1 0x80

#define BDT_PROTOCOL_MIN_CRYPTO_PACKAGE_LENGTH 16
#define BDT_PROTOCOL_MIN_PACKAGE_LENGTH 8
#define BDT_PROTOCOL_MAX_UDP_DATA_SIZE 1600 //Buffer大小，不是MTU
#define BDT_PROTOCOL_MAX_TCP_DATA_SIZE 1600
#define BDT_MIX_HASH_LENGTH 8 //64bits
#define BDT_MAX_PACKAGE_MERGE_LENGTH 32

//下面是各个package的cmdID
#define BDT_EXCHANGEKEY_PACKAGE 0
#define BDT_SYN_TUNNEL_PACKAGE  1
#define BDT_ACK_TUNNEL_PACKAGE  2
#define BDT_ACKACK_TUNNEL_PACKAGE  3
#define BDT_PING_TUNNEL_PACKAGE 4
#define BDT_PING_TUNNEL_RESP_PACKAGE 5
#define BDT_SN_CALL_PACKAGE 0x20
#define BDT_SN_CALL_RESP_PACKAGE 0x21
#define BDT_SN_CALLED_PACKAGE 0x22
#define BDT_SN_CALLED_RESP_PACKAGE 0x23
#define BDT_SN_PING_PACKAGE 0x24
#define BDT_SN_PING_RESP_PACKAGE 0x25
#define BDT_DATA_GRAM_PACKAGE 0x30
#define BDT_SESSION_DATA_PACKAGE 0x40
#define BDT_TCP_SYN_CONNECTION_PACKAGE 0x41
#define BDT_TCP_ACK_CONNECTION_PACKAGE 0x42
#define BDT_TCP_ACKACK_CONNECTION_PACKAGE 0x43

#define BDT_PACKAGE_FLAG_DISABLE_REF (1<<15) //打开的情况下不引用第一个包
#define BDT_PACKAGE_FLAG_DEFAULT 0x0 //默认FLAG目前为0就好


#define BDT_PACKAGE_RESULT_SUCCESS			0
#define BDT_PACKAGE_RESULT_REFUSED			1
#define BDT_PACKAGE_RESULT_DUPLICATED		2
#define BDT_PACKAGE_RESULT_INVALID_STATE	3


typedef uint8_t BdtProtocol_AesKeyBytes[32];
typedef uint8_t BdtProtocol_TinySignBytes[16];

typedef struct Bdt_Package 
{
	uint8_t cmdtype;
} Bdt_Package;

//除了Data包，逻辑包多用值语义，并不是很关注内存复制的性能损失
//所以不会再Package中加入引用计数的设计。

// 这里设置成RSA1024的签名长度128bytes，其余任何更长的签名都会被截断
#define BDT_SEQ_AND_KEY_SIGN_LENGTH 128

//TODO:这个用inline函数更好？
#define BDT_PACKAGE_GET_SYN_SEQ(pkg) ((pkg)->cmdtype == BDT_SYN_TUNNEL_PACKAGE ? ((Bdt_SynTunnelPackage*)(pkg))->seq : \
((pkg)->cmdtype == BDT_ACK_TUNNEL_PACKAGE ? ((Bdt_AckTunnelPackage*)(pkg))->seq : \
((pkg)->cmdtype == BDT_ACKACK_TUNNEL_PACKAGE ? ((Bdt_AckAckTunnelPackage*)(pkg))->seq : \
((pkg)->cmdtype == BDT_TCP_SYN_CONNECTION_PACKAGE ? ((Bdt_TcpSynConnectionPackage*)(pkg))->seq : \
((pkg)->cmdtype == BDT_TCP_ACK_CONNECTION_PACKAGE ? ((Bdt_TcpAckConnectionPackage*)(pkg))->seq : \
((pkg)->cmdtype == BDT_TCP_ACKACK_CONNECTION_PACKAGE ? ((Bdt_TcpAckAckConnectionPackage*)(pkg))->seq : BDT_UINT32_UNIQUE_INVALID_VALUE)))


//-----------------------------------------------------------------------------
#define BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQANDKEYSIGN (1<<1)
#define BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID (1<<2)
#define BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO (1<<3)
#define BDT_EXCHANGEKEY_PACKAGE_FLAG_SENDTIME (1<<4)
typedef struct Bdt_ExchangeKeyPackage {
	uint8_t cmdtype;
	uint16_t cmdflags;
	uint32_t seq;
	uint8_t seqAndKeySign[BDT_SEQ_AND_KEY_SIGN_LENGTH];
	BdtPeerid fromPeerid;
	uint64_t sendTime;
	const BdtPeerInfo* peerInfo;
} Bdt_ExchangeKeyPackage;
Bdt_ExchangeKeyPackage* Bdt_ExchangeKeyPackageCreate();
void Bdt_ExchangeKeyPackageInit(Bdt_ExchangeKeyPackage* package);
void Bdt_ExchangeKeyPackageFinalize(Bdt_ExchangeKeyPackage* package);
void Bdt_ExchangeKeyPackageFree(Bdt_ExchangeKeyPackage* package);
uint32_t Bdt_ExchangeKeyPackageFillWithNext(Bdt_ExchangeKeyPackage* package, const Bdt_Package* nextPackage);
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID (1<<0)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID (1<<1)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID (1<<2)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO (1<<3)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ (1<<4)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID (1<<5)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO (1<<6)
#define BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME (1<<7)
typedef struct Bdt_SynTunnelPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        BdtPeerid fromPeerid;
        BdtPeerid toPeerid;
        BdtPeerid proxyPeerid;
        uint32_t seq;
        uint32_t fromContainerId;
		uint64_t sendTime;
		const BdtPeerInfo* proxyPeerInfo;
        const BdtPeerInfo* peerInfo;
} Bdt_SynTunnelPackage;
Bdt_SynTunnelPackage* Bdt_SynTunnelPackageCreate();
void Bdt_SynTunnelPackageInit(Bdt_SynTunnelPackage* package);
void Bdt_SynTunnelPackageFinalize(Bdt_SynTunnelPackage* package);
void Bdt_SynTunnelPackageFree(Bdt_SynTunnelPackage* package);

#define BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH (1<<0)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ (1<<1)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID (1<<2)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID (1<<3)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT (1<<4)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO (1<<5)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME (1<<6)
#define BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU (1<<7)
typedef struct Bdt_AckTunnelPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint64_t aesKeyHash;
        uint32_t seq;
        uint32_t fromContainerId;
        uint32_t toContainerId;
        uint8_t result;
		uint64_t sendTime;
        uint16_t mtu;
		const BdtPeerInfo* peerInfo;
} Bdt_AckTunnelPackage;
Bdt_AckTunnelPackage* Bdt_AckTunnelPackageCreate();
void Bdt_AckTunnelPackageInit(Bdt_AckTunnelPackage* package);
void Bdt_AckTunnelPackageFinalize(Bdt_AckTunnelPackage* package);
void Bdt_AckTunnelPackageFree(Bdt_AckTunnelPackage* package);

#define BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID (1<<1)
typedef struct Bdt_AckAckTunnelPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
        uint32_t toContainerId;
} Bdt_AckAckTunnelPackage;
Bdt_AckAckTunnelPackage* Bdt_AckAckTunnelPackageCreate();
void Bdt_AckAckTunnelPackageInit(Bdt_AckAckTunnelPackage* package);
void Bdt_AckAckTunnelPackageFinalize(Bdt_AckAckTunnelPackage* package);
void Bdt_AckAckTunnelPackageFree(Bdt_AckAckTunnelPackage* package);

#define BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID (1<<0)
#define BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME (1<<1)
#define BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA (1<<2)
#define BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID (1<<3)
typedef struct Bdt_PingTunnelPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t packageId;
		uint32_t toContainerId;
		uint64_t sendTime;
        uint64_t recvData;
} Bdt_PingTunnelPackage;
Bdt_PingTunnelPackage* Bdt_PingTunnelPackageCreate();
void Bdt_PingTunnelPackageInit(Bdt_PingTunnelPackage* package);
void Bdt_PingTunnelPackageFinalize(Bdt_PingTunnelPackage* package);
void Bdt_PingTunnelPackageFree(Bdt_PingTunnelPackage* package);

#define BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID (1<<0)
#define BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME (1<<1)
#define BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA (1<<2)
#define BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID (1<<3)
typedef struct Bdt_PingTunnelRespPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t ackPackageId;
		uint32_t toContainerId;
		uint64_t sendTime;
        uint64_t recvData;
} Bdt_PingTunnelRespPackage;
Bdt_PingTunnelRespPackage* Bdt_PingTunnelRespPackageCreate();
void Bdt_PingTunnelRespPackageInit(Bdt_PingTunnelRespPackage* package);
void Bdt_PingTunnelRespPackageFinalize(Bdt_PingTunnelRespPackage* package);
void Bdt_PingTunnelRespPackageFree(Bdt_PingTunnelRespPackage* package);

#define BDT_SN_CALL_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_SN_CALL_PACKAGE_FLAG_TOPEERID (1<<1)
#define BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID (1<<2)
#define BDT_SN_CALL_PACKAGE_FLAG_SNPEERID (1<<3)
#define BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY (1<<4)
#define BDT_SN_CALL_PACKAGE_FLAG_PEERINFO (1<<5)
#define BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD (1<<6)
#define BDT_SN_CALL_PACKAGE_FLAG_ALWAYSCALL (1<<14) //带此标签标示SN会无视target发送Call
typedef struct Bdt_SnCallPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
        BdtPeerid toPeerid;
        BdtPeerid fromPeerid;
		BdtPeerid snPeerid;
		BdtEndpointArray reverseEndpointArray;
        const BdtPeerInfo* peerInfo;
        BFX_BUFFER_HANDLE payload;
} Bdt_SnCallPackage;
Bdt_SnCallPackage* Bdt_SnCallPackageCreate();
void Bdt_SnCallPackageInit(Bdt_SnCallPackage* package);
void Bdt_SnCallPackageFinalize(Bdt_SnCallPackage* package);
void Bdt_SnCallPackageFree(Bdt_SnCallPackage* package);

#define BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID (1<<1)
#define BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT (1<<2)
#define BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO (1<<3)

typedef struct Bdt_SnCallRespPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
		BdtPeerid snPeerid;
        uint8_t result;
        const BdtPeerInfo* toPeerInfo;
} Bdt_SnCallRespPackage;
Bdt_SnCallRespPackage* Bdt_SnCallRespPackageCreate();
void Bdt_SnCallRespPackageInit(Bdt_SnCallRespPackage* package);
void Bdt_SnCallRespPackageFinalize(Bdt_SnCallRespPackage* package);
void Bdt_SnCallRespPackageFree(Bdt_SnCallRespPackage* package);

#define BDT_SN_CALLED_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID (1<<1)
#define BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID (1<<2)
#define BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID (1<<3)
#define BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY (1<<4)
#define BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO (1<<5)
#define BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD (1<<6)
typedef struct Bdt_SnCalledPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
        BdtPeerid toPeerid;
        BdtPeerid fromPeerid;
		BdtPeerid snPeerid;
		BdtEndpointArray reverseEndpointArray;
        const BdtPeerInfo* peerInfo;
        BFX_BUFFER_HANDLE payload;
} Bdt_SnCalledPackage;
Bdt_SnCalledPackage* Bdt_SnCalledPackageCreate();
void Bdt_SnCalledPackageInit(Bdt_SnCalledPackage* package);
void Bdt_SnCalledPackageFinalize(Bdt_SnCalledPackage* package);
void Bdt_SnCalledPackageFree(Bdt_SnCalledPackage* package);

#define BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT (1<<1)
#define BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID (1<<2)

typedef struct Bdt_SnCalledRespPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
        uint8_t result;
		BdtPeerid snPeerid;
} Bdt_SnCalledRespPackage;
Bdt_SnCalledRespPackage* Bdt_SnCalledRespPackageCreate();
void Bdt_SnCalledRespPackageInit(Bdt_SnCalledRespPackage* package);
void Bdt_SnCalledRespPackageFinalize(Bdt_SnCalledRespPackage* package);
void Bdt_SnCalledRespPackageFree(Bdt_SnCalledRespPackage* package);

#define BDT_SN_PING_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_SN_PING_PACKAGE_FLAG_FROMPEERID (1<<1)
#define BDT_SN_PING_PACKAGE_FLAG_SNPEERID (1<<2)
#define BDT_SN_PING_PACKAGE_FLAG_PEERINFO (1<<3)

typedef struct Bdt_SnPingPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
        BdtPeerid fromPeerid;
		BdtPeerid snPeerid;
        const BdtPeerInfo* peerInfo;
} Bdt_SnPingPackage;
Bdt_SnPingPackage* Bdt_SnPingPackageCreate();
void Bdt_SnPingPackageInit(Bdt_SnPingPackage* package);
void Bdt_SnPingPackageFinalize(Bdt_SnPingPackage* package);
void Bdt_SnPingPackageFree(Bdt_SnPingPackage* package);

#define BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID (1<<1)
#define BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT (1<<2)
#define BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO (1<<3)
#define BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY (1<<4)
typedef struct Bdt_SnPingRespPackage {
        uint8_t cmdtype;
        uint16_t cmdflags;
        uint32_t seq;
		BdtPeerid snPeerid;
        uint8_t result;
		BdtEndpointArray endpointArray;
        const BdtPeerInfo* peerInfo;
} Bdt_SnPingRespPackage;
Bdt_SnPingRespPackage* Bdt_SnPingRespPackageCreate();
void Bdt_SnPingRespPackageInit(Bdt_SnPingRespPackage* package);
void Bdt_SnPingRespPackageFinalize(Bdt_SnPingRespPackage* package);
void Bdt_SnPingRespPackageFree(Bdt_SnPingRespPackage* package);
//-----------------------------------------------------------
#define BDT_DATA_GRAM_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE (1<<1)
#define BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT (1<<2)
#define BDT_DATA_GRAM_PACKAGE_FLAG_PIECE (1<<3)
#define BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ (1<<4)
#define BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME (1<<5)
#define BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID (1<<6)
#define BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO (1<<7)
#define BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN (1<<8)
#define BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE (1<<9)
#define BDT_DATA_GRAM_PACKAGE_FLAG_DATA (1<<10)
typedef struct Bdt_DataGramPackage {
	uint8_t cmdtype;
	uint16_t cmdflags;
	uint32_t seq;
	uint32_t destZone;
	uint8_t hopLimit;
	uint16_t piece;
	uint32_t ackSeq;
	uint64_t sendTime;
	BdtPeerid authorPeerid;
	BdtProtocol_TinySignBytes dataSign;
	uint8_t innerCmdType;
	const BdtPeerInfo* authorPeerInfo;
	BFX_BUFFER_HANDLE data;
} Bdt_DataGramPackage;
Bdt_DataGramPackage* Bdt_DataGramPackageCreate();
void Bdt_DataGramPackageInit(Bdt_DataGramPackage* package);
void Bdt_DataGramPackageFinalize(Bdt_DataGramPackage* package);
void Bdt_DataGramPackageFree(Bdt_DataGramPackage* package);
//-------------------------------------------------------------


#define BDT_SESSIONDATA_PACKAGE_FLAG_PACKAGEID (1<<0)
#define BDT_SESSIONDATA_PACKAGE_FLAG_ACK_PACKAGEID (1<<1)
#define BDT_SESSIONDATA_PACKAGE_FLAG_SYN (1<<2)
#define BDT_SESSIONDATA_PACKAGE_FLAG_ACK (1<<3) 
#define BDT_SESSIONDATA_PACKAGE_FLAG_SACK (1<<4) 
#define BDT_SESSIONDATA_PACKAGE_FLAG_SPEEDLIMIT (1<<5)
#define BDT_SESSIONDATA_PACKAGE_FLAG_SENDTIME (1<<6)
#define BDT_SESSIONDATA_PACKAGE_FLAG_PAYLOAD  (1<<7)
#define BDT_SESSIONDATA_PACKAGE_FLAG_FIN (1<<10)
#define BDT_SESSIONDATA_PACKAGE_FLAG_FINACK (1<<11)
#define BDT_SESSIONDATA_PACKAGE_FLAG_RESET (1<<12)

#define  PACKAGE_SESSIONDATA_SACK_RANGE_MAX_COUNT 10


typedef struct Bdt_SessionDataSynPart {
    uint32_t synSeq;
	uint8_t synControl;//用来控制后续的连接策略，比如强制要求反连
	uint8_t ccType;
	uint16_t toVPort;
	uint32_t fromSessionId;//自己本地的id,在握手的时候使用
} Bdt_SessionDataSynPart;

typedef struct Bdt_SessionDataPackageIdPart {
	uint32_t packageId ;//当ACK_PACKAGEID打开时，是ackPackageID
	uint64_t totalRecv;//48 bits长度
}Bdt_SessionDataPackageIdPart;

//为了提高SessionData包的encode/decode速度，将会使用一些特别的技巧。
typedef struct Bdt_SessionDataPackage {
	uint8_t cmdtype;
	uint16_t cmdflags;
	uint32_t sessionId;//面向对端的sessionID.必填(对端的id，在最开始SYN的时候可能为0,ACK(SYN_FLAG和ACK_FLAG）的时候可以为0也可以为fromSessionID，有ack后就是fromSessionId了）
	uint64_t streamPos;//当前payload数据的起始位置，默认从0开始。必填。48bits;
	//下面是正常的可选字段
	uint64_t ackStreamPos; //48bits;
	uint32_t toSessionId;//当SYN_FLAG和ACK_FLAG都打开时，该字段存在..ACK的时候发送给对方的
	BdtStreamRange* sackArray;//最多10个元素。顺序排列，互不连续，不是10个时以NULL结尾
	//BdtStreamArray sackStreamArray;
	uint32_t recvSpeedlimit;
	uint64_t sendTime;
	uint16_t payloadLength;
	uint8_t* payload;

	Bdt_SessionDataSynPart* synPart;//只有flags中包含了SESSION_SYN才会存在
	Bdt_SessionDataPackageIdPart* packageIDPart;//只有flag中包含了PACKAGEID或ACK_PACKAGEID才会存在

}Bdt_SessionDataPackage;
//TODO:BdtPackageSessionData相关Package的helper函数根据实际应用场景来设计
Bdt_SessionDataPackage* Bdt_SessionDataPackageCreate();
void Bdt_SessionDataPackageInit(Bdt_SessionDataPackage* package);
void Bdt_SessionDataPackageFinalize(Bdt_SessionDataPackage* package);
void Bdt_SessionDataPackageFree(Bdt_SessionDataPackage* package);

#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT (1<<1)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT (1<<2)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID (1<<3)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID (1<<4)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID (1<<5)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID (1<<6)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO (1<<7)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD (1<<8)
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY (1<<9)
// use this flag to request a reverse tcp stream directly
#define BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSE (1<<10)	
typedef struct Bdt_TcpSynConnectionPackage {
	uint8_t cmdtype;
	uint16_t cmdflags;
	uint32_t seq;
	uint8_t result;
	uint16_t toVPort;
	uint32_t fromSessionId;
	BdtPeerid fromPeerid;
	BdtPeerid toPeerid;
	BdtPeerid proxyPeerid;
	const BdtPeerInfo* peerInfo;
	BdtEndpointArray reverseEndpointArray;
	BFX_BUFFER_HANDLE payload;
} Bdt_TcpSynConnectionPackage;
Bdt_TcpSynConnectionPackage* Bdt_TcpSynConnectionPackageCreate();
void Bdt_TcpSynConnectionPackageInit(Bdt_TcpSynConnectionPackage* package);
void Bdt_TcpSynConnectionPackageFinalize(Bdt_TcpSynConnectionPackage* package);
void Bdt_TcpSynConnectionPackageFree(Bdt_TcpSynConnectionPackage* package);


#define BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID (1<<1)
#define BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT (1<<2)
#define BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO (1<<3)
#define BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD (1<<4)
typedef struct Bdt_TcpAckConnectionPackage {
	uint8_t cmdtype;
	uint16_t cmdflags;
	uint32_t seq;
	uint32_t toSessionId;
	uint8_t result;
	const BdtPeerInfo* peerInfo;
	BFX_BUFFER_HANDLE payload;
} Bdt_TcpAckConnectionPackage;
Bdt_TcpAckConnectionPackage* Bdt_TcpAckConnectionPackageCreate();
void Bdt_TcpAckConnectionPackageInit(Bdt_TcpAckConnectionPackage* package);
void Bdt_TcpAckConnectionPackageFinalize(Bdt_TcpAckConnectionPackage* package);
void Bdt_TcpAckConnectionPackageFree(Bdt_TcpAckConnectionPackage* package);

#define BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ (1<<0)
#define BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT (1<<1)
typedef struct Bdt_TcpAckAckConnectionPackage {
	uint8_t cmdtype;
	uint16_t cmdflags;
	uint32_t seq;
	uint8_t result;
} Bdt_TcpAckAckConnectionPackage;
Bdt_TcpAckAckConnectionPackage* Bdt_TcpAckAckConnectionPackageCreate();
void Bdt_TcpAckAckConnectionPackageInit(Bdt_TcpAckAckConnectionPackage* package);
void Bdt_TcpAckAckConnectionPackageFinalize(Bdt_TcpAckAckConnectionPackage* package);
void Bdt_TcpAckAckConnectionPackageFree(Bdt_TcpAckAckConnectionPackage* package);

void Bdt_PackageFinalize(Bdt_Package* pkg);
void Bdt_PackageFree(Bdt_Package* pkg);

typedef struct Bdt_PackageWithRef Bdt_PackageWithRef;
Bdt_PackageWithRef* Bdt_PackageCreateWithRef(uint8_t cmdtype);
int32_t Bdt_PackageAddRef(const Bdt_PackageWithRef* packageRef);
int32_t Bdt_PackageRelease(const Bdt_PackageWithRef* packageRef);
Bdt_Package* Bdt_PackageWithRefGet(const Bdt_PackageWithRef* packageRef);

bool Bdt_IsEqualPackage(const Bdt_Package* lhs, const Bdt_Package* rhs);
const char* Bdt_PackageGetNameByCmdType(uint8_t cmdtype);
size_t Bdt_GetPackageMinSize(uint8_t cmdtype);
bool Bdt_IsSnPackage(const Bdt_Package* package);
bool Bdt_IsTcpPackage(const Bdt_Package* package);

size_t BdtProtocol_ConnectionFirstQuestionMaxLength();
size_t BdtProtocol_ConnectionFirstResponseMaxLength();

int Bdt_BufferReadPeerid(BfxBufferStream* bufferStream, BdtPeerid* pResult);
int Bdt_BufferWritePeerid(BfxBufferStream* bufferStream, const BdtPeerid* pPeerID, size_t* pWriteBytes);

int Bdt_BufferReadEndpointArray(BfxBufferStream* bufferStream, BdtEndpointArray* pResult);
int Bdt_BufferWriteEndpointArray(BfxBufferStream* bufferStream, BdtEndpointArray* pResult, size_t* pWriteBytes);

//int _BufferReadStreamPosArray();
//int _BufferWriteStreamPosArray();

#endif //_BDT_PACKAGE_H_