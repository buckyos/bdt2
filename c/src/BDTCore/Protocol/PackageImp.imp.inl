

#include "./Package.h"
#include "./PackageDecoder.h"
#include "./PackageEncoder.h"

#include <assert.h>
	

static const char* s_fieldName_seq = "seq";
static const char* s_fieldName_seqAndKeySign = "seqAndKeySign";
static const char* s_fieldName_fromPeerid = "fromPeerid";
static const char* s_fieldName_peerInfo = "peerInfo";
static const char* s_fieldName_sendTime = "sendTime";
static const char* s_fieldName_toPeerid = "toPeerid";
static const char* s_fieldName_proxyPeerid = "proxyPeerid";
static const char* s_fieldName_proxyPeerInfo = "proxyPeerInfo";
static const char* s_fieldName_fromContainerId = "fromContainerId";
static const char* s_fieldName_aesKeyHash = "aesKeyHash";
static const char* s_fieldName_toContainerId = "toContainerId";
static const char* s_fieldName_result = "result";
static const char* s_fieldName_mtu = "mtu";
static const char* s_fieldName_packageId = "packageId";
static const char* s_fieldName_recvData = "recvData";
static const char* s_fieldName_ackPackageId = "ackPackageId";
static const char* s_fieldName_snPeerid = "snPeerid";
static const char* s_fieldName_reverseEndpointArray = "reverseEndpointArray";
static const char* s_fieldName_payload = "payload";
static const char* s_fieldName_toPeerInfo = "toPeerInfo";
static const char* s_fieldName_endpointArray = "endpointArray";
static const char* s_fieldName_destZone = "destZone";
static const char* s_fieldName_hopLimit = "hopLimit";
static const char* s_fieldName_piece = "piece";
static const char* s_fieldName_ackSeq = "ackSeq";
static const char* s_fieldName_authorPeerid = "authorPeerid";
static const char* s_fieldName_authorPeerInfo = "authorPeerInfo";
static const char* s_fieldName_dataSign = "dataSign";
static const char* s_fieldName_innerCmdType = "innerCmdType";
static const char* s_fieldName_data = "data";
static const char* s_fieldName_toVPort = "toVPort";
static const char* s_fieldName_fromSessionId = "fromSessionId";
static const char* s_fieldName_toSessionId = "toSessionId";

//THIS FUNCTION GENERATE BY packages.js	
static inline const char* GetPackageFieldName(uint8_t cmdtype, uint16_t fieldFlags)
{
	switch (cmdtype)
	{	
	case BDT_EXCHANGEKEY_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_EXCHANGEKEY_PACKAGE_FLAG_SEQANDKEYSIGN:
			return s_fieldName_seqAndKeySign;
		case BDT_EXCHANGEKEY_PACKAGE_FLAG_FROMPEERID:
			return s_fieldName_fromPeerid;
		case BDT_EXCHANGEKEY_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_EXCHANGEKEY_PACKAGE_FLAG_SENDTIME:
			return s_fieldName_sendTime;
		}
		break;	
	case BDT_SYN_TUNNEL_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID:
			return s_fieldName_fromPeerid;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID:
			return s_fieldName_toPeerid;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID:
			return s_fieldName_proxyPeerid;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO:
			return s_fieldName_proxyPeerInfo;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID:
			return s_fieldName_fromContainerId;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME:
			return s_fieldName_sendTime;
		}
		break;	
	case BDT_ACK_TUNNEL_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH:
			return s_fieldName_aesKeyHash;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID:
			return s_fieldName_fromContainerId;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID:
			return s_fieldName_toContainerId;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME:
			return s_fieldName_sendTime;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU:
			return s_fieldName_mtu;
		}
		break;	
	case BDT_ACKACK_TUNNEL_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID:
			return s_fieldName_toContainerId;
		}
		break;	
	case BDT_PING_TUNNEL_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID:
			return s_fieldName_toContainerId;
		case BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID:
			return s_fieldName_packageId;
		case BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME:
			return s_fieldName_sendTime;
		case BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA:
			return s_fieldName_recvData;
		}
		break;	
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID:
			return s_fieldName_toContainerId;
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID:
			return s_fieldName_ackPackageId;
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME:
			return s_fieldName_sendTime;
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA:
			return s_fieldName_recvData;
		}
		break;	
	case BDT_SN_CALL_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SN_CALL_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SN_CALL_PACKAGE_FLAG_SNPEERID:
			return s_fieldName_snPeerid;
		case BDT_SN_CALL_PACKAGE_FLAG_TOPEERID:
			return s_fieldName_toPeerid;
		case BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID:
			return s_fieldName_fromPeerid;
		case BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY:
			return s_fieldName_reverseEndpointArray;
		case BDT_SN_CALL_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD:
			return s_fieldName_payload;
		}
		break;	
	case BDT_SN_CALL_RESP_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID:
			return s_fieldName_snPeerid;
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO:
			return s_fieldName_toPeerInfo;
		}
		break;	
	case BDT_SN_CALLED_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SN_CALLED_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID:
			return s_fieldName_snPeerid;
		case BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID:
			return s_fieldName_toPeerid;
		case BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID:
			return s_fieldName_fromPeerid;
		case BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY:
			return s_fieldName_reverseEndpointArray;
		case BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD:
			return s_fieldName_payload;
		}
		break;	
	case BDT_SN_CALLED_RESP_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID:
			return s_fieldName_snPeerid;
		case BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		}
		break;	
	case BDT_SN_PING_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SN_PING_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SN_PING_PACKAGE_FLAG_SNPEERID:
			return s_fieldName_snPeerid;
		case BDT_SN_PING_PACKAGE_FLAG_FROMPEERID:
			return s_fieldName_fromPeerid;
		case BDT_SN_PING_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		}
		break;	
	case BDT_SN_PING_RESP_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID:
			return s_fieldName_snPeerid;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY:
			return s_fieldName_endpointArray;
		}
		break;	
	case BDT_DATA_GRAM_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_DATA_GRAM_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE:
			return s_fieldName_destZone;
		case BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT:
			return s_fieldName_hopLimit;
		case BDT_DATA_GRAM_PACKAGE_FLAG_PIECE:
			return s_fieldName_piece;
		case BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ:
			return s_fieldName_ackSeq;
		case BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME:
			return s_fieldName_sendTime;
		case BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID:
			return s_fieldName_authorPeerid;
		case BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO:
			return s_fieldName_authorPeerInfo;
		case BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN:
			return s_fieldName_dataSign;
		case BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE:
			return s_fieldName_innerCmdType;
		case BDT_DATA_GRAM_PACKAGE_FLAG_DATA:
			return s_fieldName_data;
		}
		break;	
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT:
			return s_fieldName_toVPort;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID:
			return s_fieldName_fromSessionId;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID:
			return s_fieldName_fromPeerid;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID:
			return s_fieldName_toPeerid;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID:
			return s_fieldName_proxyPeerid;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY:
			return s_fieldName_reverseEndpointArray;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD:
			return s_fieldName_payload;
		}
		break;	
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID:
			return s_fieldName_toSessionId;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO:
			return s_fieldName_peerInfo;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD:
			return s_fieldName_payload;
		}
		break;	
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		switch (fieldFlags)
		{
		case BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ:
			return s_fieldName_seq;
		case BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT:
			return s_fieldName_result;
		}
		break;
	}
	assert(0);
	return NULL;
}

//THIS FUNCTION GENERATE BY packages.js	
static bool IsEqualPackageFieldDefaultValue(const Bdt_Package* package, uint16_t fieldFlags)
{	
	bool r = false;
	static uint8_t defaultPid[BDT_PEERID_LENGTH] = {0};
	static uint8_t defaultBytes16[16] = {0};
	static uint8_t defaultBytes32[32] = {0};
	
	const Bdt_SynTunnelPackage* pSynTunnel = NULL;
	const Bdt_AckTunnelPackage* pAckTunnel = NULL;
	const Bdt_AckAckTunnelPackage* pAckAckTunnel = NULL;
	const Bdt_PingTunnelPackage* pPingTunnel = NULL;
	const Bdt_PingTunnelRespPackage* pPingTunnelResp = NULL;
	const Bdt_SnCallPackage* pSnCall = NULL;
	const Bdt_SnCallRespPackage* pSnCallResp = NULL;
	const Bdt_SnCalledPackage* pSnCalled = NULL;
	const Bdt_SnCalledRespPackage* pSnCalledResp = NULL;
	const Bdt_SnPingPackage* pSnPing = NULL;
	const Bdt_SnPingRespPackage* pSnPingResp = NULL;
	const Bdt_DataGramPackage* pDataGram = NULL;
	const Bdt_TcpSynConnectionPackage* pTcpSynConnection = NULL;
	const Bdt_TcpAckConnectionPackage* pTcpAckConnection = NULL;
	const Bdt_TcpAckAckConnectionPackage* pTcpAckAckConnection = NULL;
	switch (package->cmdtype)
	{
		
	case BDT_SYN_TUNNEL_PACKAGE:
		pSynTunnel = (Bdt_SynTunnelPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID:
			r = (memcmp(pSynTunnel->fromPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID:
			r = (memcmp(pSynTunnel->toPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID:
			r = (memcmp(pSynTunnel->proxyPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO:
			r = (pSynTunnel->proxyPeerInfo == NULL);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ:
			r = (pSynTunnel->seq == 0);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID:
			r = (pSynTunnel->fromContainerId == 0);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO:
			r = (pSynTunnel->peerInfo == NULL);
			break;
		case BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME:
			r = (pSynTunnel->sendTime == 0);
			break;
		}
		break;	
	case BDT_ACK_TUNNEL_PACKAGE:
		pAckTunnel = (Bdt_AckTunnelPackage*)package;
		switch (fieldFlags)
		{
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH:
			r = (pAckTunnel->aesKeyHash == 0);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ:
			r = (pAckTunnel->seq == 0);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID:
			r = (pAckTunnel->fromContainerId == 0);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID:
			r = (pAckTunnel->toContainerId == 0);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT:
			r = (pAckTunnel->result == 0);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO:
			r = (pAckTunnel->peerInfo == NULL);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME:
			r = (pAckTunnel->sendTime == 0);
			break;
		case BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU:
			r = (pAckTunnel->mtu == 0);
			break;
		}
		break;	
	case BDT_ACKACK_TUNNEL_PACKAGE:
		pAckAckTunnel = (Bdt_AckAckTunnelPackage*)package;
		switch (fieldFlags)
		{
		case BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ:
			r = (pAckAckTunnel->seq == 0);
			break;
		case BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID:
			r = (pAckAckTunnel->toContainerId == 0);
			break;
		}
		break;	
	case BDT_PING_TUNNEL_PACKAGE:
		pPingTunnel = (Bdt_PingTunnelPackage*)package;
		switch (fieldFlags)
		{
		case BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID:
			r = (pPingTunnel->toContainerId == 0);
			break;
		case BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID:
			r = (pPingTunnel->packageId == 0);
			break;
		case BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME:
			r = (pPingTunnel->sendTime == 0);
			break;
		case BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA:
			r = (pPingTunnel->recvData == 0);
			break;
		}
		break;	
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		pPingTunnelResp = (Bdt_PingTunnelRespPackage*)package;
		switch (fieldFlags)
		{
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID:
			r = (pPingTunnelResp->toContainerId == 0);
			break;
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID:
			r = (pPingTunnelResp->ackPackageId == 0);
			break;
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME:
			r = (pPingTunnelResp->sendTime == 0);
			break;
		case BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA:
			r = (pPingTunnelResp->recvData == 0);
			break;
		}
		break;	
	case BDT_SN_CALL_PACKAGE:
		pSnCall = (Bdt_SnCallPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SN_CALL_PACKAGE_FLAG_SEQ:
			r = (pSnCall->seq == 0);
			break;
		case BDT_SN_CALL_PACKAGE_FLAG_SNPEERID:
			r = (memcmp(pSnCall->snPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALL_PACKAGE_FLAG_TOPEERID:
			r = (memcmp(pSnCall->toPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID:
			r = (memcmp(pSnCall->fromPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY:
			r = (pSnCall->reverseEndpointArray.size == 0);
			break;
		case BDT_SN_CALL_PACKAGE_FLAG_PEERINFO:
			r = (pSnCall->peerInfo == NULL);
			break;
		case BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD:
			r = (pSnCall->payload == NULL);
			break;
		}
		break;	
	case BDT_SN_CALL_RESP_PACKAGE:
		pSnCallResp = (Bdt_SnCallRespPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ:
			r = (pSnCallResp->seq == 0);
			break;
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID:
			r = (memcmp(pSnCallResp->snPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT:
			r = (pSnCallResp->result == 0);
			break;
		case BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO:
			r = (pSnCallResp->toPeerInfo == NULL);
			break;
		}
		break;	
	case BDT_SN_CALLED_PACKAGE:
		pSnCalled = (Bdt_SnCalledPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SN_CALLED_PACKAGE_FLAG_SEQ:
			r = (pSnCalled->seq == 0);
			break;
		case BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID:
			r = (memcmp(pSnCalled->snPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID:
			r = (memcmp(pSnCalled->toPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID:
			r = (memcmp(pSnCalled->fromPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY:
			r = (pSnCalled->reverseEndpointArray.size == 0);
			break;
		case BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO:
			r = (pSnCalled->peerInfo == NULL);
			break;
		case BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD:
			r = (pSnCalled->payload == NULL);
			break;
		}
		break;	
	case BDT_SN_CALLED_RESP_PACKAGE:
		pSnCalledResp = (Bdt_SnCalledRespPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ:
			r = (pSnCalledResp->seq == 0);
			break;
		case BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID:
			r = (memcmp(pSnCalledResp->snPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT:
			r = (pSnCalledResp->result == 0);
			break;
		}
		break;	
	case BDT_SN_PING_PACKAGE:
		pSnPing = (Bdt_SnPingPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SN_PING_PACKAGE_FLAG_SEQ:
			r = (pSnPing->seq == 0);
			break;
		case BDT_SN_PING_PACKAGE_FLAG_SNPEERID:
			r = (memcmp(pSnPing->snPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_PING_PACKAGE_FLAG_FROMPEERID:
			r = (memcmp(pSnPing->fromPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_PING_PACKAGE_FLAG_PEERINFO:
			r = (pSnPing->peerInfo == NULL);
			break;
		}
		break;	
	case BDT_SN_PING_RESP_PACKAGE:
		pSnPingResp = (Bdt_SnPingRespPackage*)package;
		switch (fieldFlags)
		{
		case BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ:
			r = (pSnPingResp->seq == 0);
			break;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID:
			r = (memcmp(pSnPingResp->snPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT:
			r = (pSnPingResp->result == 0);
			break;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO:
			r = (pSnPingResp->peerInfo == NULL);
			break;
		case BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY:
			r = (pSnPingResp->endpointArray.size == 0);
			break;
		}
		break;	
	case BDT_DATA_GRAM_PACKAGE:
		pDataGram = (Bdt_DataGramPackage*)package;
		switch (fieldFlags)
		{
		case BDT_DATA_GRAM_PACKAGE_FLAG_SEQ:
			r = (pDataGram->seq == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_DESTZONE:
			r = (pDataGram->destZone == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_HOPLIMIT:
			r = (pDataGram->hopLimit == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_PIECE:
			r = (pDataGram->piece == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_ACKSEQ:
			r = (pDataGram->ackSeq == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_SENDTIME:
			r = (pDataGram->sendTime == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERID:
			r = (memcmp(pDataGram->authorPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_AUTHORPEERINFO:
			r = (pDataGram->authorPeerInfo == NULL);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_DATASIGN:
			r = (memcmp(pDataGram->dataSign,defaultBytes16, 16) ==0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_INNERCMDTYPE:
			r = (pDataGram->innerCmdType == 0);
			break;
		case BDT_DATA_GRAM_PACKAGE_FLAG_DATA:
			r = (pDataGram->data == NULL);
			break;
		}
		break;	
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		pTcpSynConnection = (Bdt_TcpSynConnectionPackage*)package;
		switch (fieldFlags)
		{
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ:
			r = (pTcpSynConnection->seq == 0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT:
			r = (pTcpSynConnection->result == 0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT:
			r = (pTcpSynConnection->toVPort == 0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID:
			r = (pTcpSynConnection->fromSessionId == 0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID:
			r = (memcmp(pTcpSynConnection->fromPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID:
			r = (memcmp(pTcpSynConnection->toPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID:
			r = (memcmp(pTcpSynConnection->proxyPeerid.pid,defaultPid, BDT_PEERID_LENGTH) ==0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO:
			r = (pTcpSynConnection->peerInfo == NULL);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY:
			r = (pTcpSynConnection->reverseEndpointArray.size == 0);
			break;
		case BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD:
			r = (pTcpSynConnection->payload == NULL);
			break;
		}
		break;	
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		pTcpAckConnection = (Bdt_TcpAckConnectionPackage*)package;
		switch (fieldFlags)
		{
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ:
			r = (pTcpAckConnection->seq == 0);
			break;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID:
			r = (pTcpAckConnection->toSessionId == 0);
			break;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT:
			r = (pTcpAckConnection->result == 0);
			break;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO:
			r = (pTcpAckConnection->peerInfo == NULL);
			break;
		case BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD:
			r = (pTcpAckConnection->payload == NULL);
			break;
		}
		break;	
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		pTcpAckAckConnection = (Bdt_TcpAckAckConnectionPackage*)package;
		switch (fieldFlags)
		{
		case BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ:
			r = (pTcpAckAckConnection->seq == 0);
			break;
		case BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT:
			r = (pTcpAckAckConnection->result == 0);
			break;
		}
		break;
	}
	return r;
}
	

//THIS FUNCTION GENERATE BY packages.js	
static int GetPackageFieldValueByName(const Bdt_Package* pPackage,const char* fieldName,void* pFieldValue)
{	
	const Bdt_ExchangeKeyPackage* pExchangeKey = NULL;
const Bdt_SynTunnelPackage* pSynTunnel = NULL;
const Bdt_AckTunnelPackage* pAckTunnel = NULL;
const Bdt_AckAckTunnelPackage* pAckAckTunnel = NULL;
const Bdt_PingTunnelPackage* pPingTunnel = NULL;
const Bdt_PingTunnelRespPackage* pPingTunnelResp = NULL;
const Bdt_SnCallPackage* pSnCall = NULL;
const Bdt_SnCallRespPackage* pSnCallResp = NULL;
const Bdt_SnCalledPackage* pSnCalled = NULL;
const Bdt_SnCalledRespPackage* pSnCalledResp = NULL;
const Bdt_SnPingPackage* pSnPing = NULL;
const Bdt_SnPingRespPackage* pSnPingResp = NULL;
const Bdt_DataGramPackage* pDataGram = NULL;
const Bdt_TcpSynConnectionPackage* pTcpSynConnection = NULL;
const Bdt_TcpAckConnectionPackage* pTcpAckConnection = NULL;
const Bdt_TcpAckAckConnectionPackage* pTcpAckAckConnection = NULL;

	switch (pPackage->cmdtype)
	{	
	
	case BDT_EXCHANGEKEY_PACKAGE:
		pExchangeKey = (Bdt_ExchangeKeyPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pExchangeKey->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_seqAndKeySign)
		{
			memcpy(pFieldValue,pExchangeKey->seqAndKeySign,BDT_SEQ_AND_KEY_SIGN_LENGTH);
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pExchangeKey->fromPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pExchangeKey->peerInfo;
				  BdtAddRefPeerInfo(pExchangeKey->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_sendTime)
		{
			(*(uint64_t*)pFieldValue) = pExchangeKey->sendTime;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SYN_TUNNEL_PACKAGE:
		pSynTunnel = (Bdt_SynTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_fromPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSynTunnel->fromPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSynTunnel->toPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_proxyPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSynTunnel->proxyPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_proxyPeerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSynTunnel->proxyPeerInfo;
				  BdtAddRefPeerInfo(pSynTunnel->proxyPeerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSynTunnel->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromContainerId)
		{
			(*(uint32_t*)pFieldValue) = pSynTunnel->fromContainerId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSynTunnel->peerInfo;
				  BdtAddRefPeerInfo(pSynTunnel->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_sendTime)
		{
			(*(uint64_t*)pFieldValue) = pSynTunnel->sendTime;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_ACK_TUNNEL_PACKAGE:
		pAckTunnel = (Bdt_AckTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_aesKeyHash)
		{
			(*(uint64_t*)pFieldValue) = pAckTunnel->aesKeyHash;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pAckTunnel->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromContainerId)
		{
			(*(uint32_t*)pFieldValue) = pAckTunnel->fromContainerId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toContainerId)
		{
			(*(uint32_t*)pFieldValue) = pAckTunnel->toContainerId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pAckTunnel->result;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pAckTunnel->peerInfo;
				  BdtAddRefPeerInfo(pAckTunnel->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_sendTime)
		{
			(*(uint64_t*)pFieldValue) = pAckTunnel->sendTime;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_mtu)
		{
			(*(uint16_t*)pFieldValue) = pAckTunnel->mtu;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		pAckAckTunnel = (Bdt_AckAckTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pAckAckTunnel->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toContainerId)
		{
			(*(uint32_t*)pFieldValue) = pAckAckTunnel->toContainerId;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_PING_TUNNEL_PACKAGE:
		pPingTunnel = (Bdt_PingTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_toContainerId)
		{
			(*(uint32_t*)pFieldValue) = pPingTunnel->toContainerId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_packageId)
		{
			(*(uint32_t*)pFieldValue) = pPingTunnel->packageId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_sendTime)
		{
			(*(uint64_t*)pFieldValue) = pPingTunnel->sendTime;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_recvData)
		{
			(*(uint64_t*)pFieldValue) = pPingTunnel->recvData;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		pPingTunnelResp = (Bdt_PingTunnelRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_toContainerId)
		{
			(*(uint32_t*)pFieldValue) = pPingTunnelResp->toContainerId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_ackPackageId)
		{
			(*(uint32_t*)pFieldValue) = pPingTunnelResp->ackPackageId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_sendTime)
		{
			(*(uint64_t*)pFieldValue) = pPingTunnelResp->sendTime;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_recvData)
		{
			(*(uint64_t*)pFieldValue) = pPingTunnelResp->recvData;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SN_CALL_PACKAGE:
		pSnCall = (Bdt_SnCallPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSnCall->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCall->snPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCall->toPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCall->fromPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_reverseEndpointArray)
		{
			BdtEndpointArrayClone(&(pSnCall->reverseEndpointArray),pFieldValue);
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSnCall->peerInfo;
				  BdtAddRefPeerInfo(pSnCall->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_payload)
		{
			assert(0);
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		pSnCallResp = (Bdt_SnCallRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSnCallResp->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCallResp->snPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pSnCallResp->result;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toPeerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSnCallResp->toPeerInfo;
				  BdtAddRefPeerInfo(pSnCallResp->toPeerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SN_CALLED_PACKAGE:
		pSnCalled = (Bdt_SnCalledPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSnCalled->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCalled->snPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCalled->toPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCalled->fromPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_reverseEndpointArray)
		{
			BdtEndpointArrayClone(&(pSnCalled->reverseEndpointArray),pFieldValue);
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSnCalled->peerInfo;
				  BdtAddRefPeerInfo(pSnCalled->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_payload)
		{
			assert(0);
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SN_CALLED_RESP_PACKAGE:
		pSnCalledResp = (Bdt_SnCalledRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSnCalledResp->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnCalledResp->snPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pSnCalledResp->result;
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SN_PING_PACKAGE:
		pSnPing = (Bdt_SnPingPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSnPing->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnPing->snPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnPing->fromPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSnPing->peerInfo;
				  BdtAddRefPeerInfo(pSnPing->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_SN_PING_RESP_PACKAGE:
		pSnPingResp = (Bdt_SnPingRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pSnPingResp->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pSnPingResp->snPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pSnPingResp->result;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pSnPingResp->peerInfo;
				  BdtAddRefPeerInfo(pSnPingResp->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_endpointArray)
		{
			BdtEndpointArrayClone(&(pSnPingResp->endpointArray),pFieldValue);
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_DATA_GRAM_PACKAGE:
		pDataGram = (Bdt_DataGramPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pDataGram->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_destZone)
		{
			(*(uint32_t*)pFieldValue) = pDataGram->destZone;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_hopLimit)
		{
			(*(uint8_t*)pFieldValue) = pDataGram->hopLimit;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_piece)
		{
			(*(uint16_t*)pFieldValue) = pDataGram->piece;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_ackSeq)
		{
			(*(uint32_t*)pFieldValue) = pDataGram->ackSeq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_sendTime)
		{
			(*(uint64_t*)pFieldValue) = pDataGram->sendTime;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_authorPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pDataGram->authorPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_authorPeerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pDataGram->authorPeerInfo;
				  BdtAddRefPeerInfo(pDataGram->authorPeerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_dataSign)
		{
			memcpy(pFieldValue,pDataGram->dataSign,16);
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_innerCmdType)
		{
			(*(uint8_t*)pFieldValue) = pDataGram->innerCmdType;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_data)
		{
			assert(0);
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		pTcpSynConnection = (Bdt_TcpSynConnectionPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pTcpSynConnection->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pTcpSynConnection->result;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toVPort)
		{
			(*(uint16_t*)pFieldValue) = pTcpSynConnection->toVPort;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromSessionId)
		{
			(*(uint32_t*)pFieldValue) = pTcpSynConnection->fromSessionId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pTcpSynConnection->fromPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pTcpSynConnection->toPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_proxyPeerid)
		{
			(*(BdtPeerid*)pFieldValue) = pTcpSynConnection->proxyPeerid;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pTcpSynConnection->peerInfo;
				  BdtAddRefPeerInfo(pTcpSynConnection->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_reverseEndpointArray)
		{
			BdtEndpointArrayClone(&(pTcpSynConnection->reverseEndpointArray),pFieldValue);
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_payload)
		{
			assert(0);
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		pTcpAckConnection = (Bdt_TcpAckConnectionPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pTcpAckConnection->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_toSessionId)
		{
			(*(uint32_t*)pFieldValue) = pTcpAckConnection->toSessionId;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pTcpAckConnection->result;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			(*(const BdtPeerInfo**)pFieldValue) = pTcpAckConnection->peerInfo;
				  BdtAddRefPeerInfo(pTcpAckConnection->peerInfo);	
				
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_payload)
		{
			assert(0);
			return BFX_RESULT_SUCCESS;
		}
		break;
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		pTcpAckAckConnection = (Bdt_TcpAckAckConnectionPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			(*(uint32_t*)pFieldValue) = pTcpAckAckConnection->seq;
			return BFX_RESULT_SUCCESS;
		}
		if (fieldName == s_fieldName_result)
		{
			(*(uint8_t*)pFieldValue) = pTcpAckAckConnection->result;
			return BFX_RESULT_SUCCESS;
		}
		break;
	}
	return BFX_RESULT_NOT_FOUND;
}

//THIS FUNCTION GENERATE BY packages.js	
static const void* GetPackageFieldPointByName(const Bdt_Package* pPackage, const char* fieldName)
{	
	const Bdt_ExchangeKeyPackage* pExchangeKey = NULL;
const Bdt_SynTunnelPackage* pSynTunnel = NULL;
const Bdt_AckTunnelPackage* pAckTunnel = NULL;
const Bdt_AckAckTunnelPackage* pAckAckTunnel = NULL;
const Bdt_PingTunnelPackage* pPingTunnel = NULL;
const Bdt_PingTunnelRespPackage* pPingTunnelResp = NULL;
const Bdt_SnCallPackage* pSnCall = NULL;
const Bdt_SnCallRespPackage* pSnCallResp = NULL;
const Bdt_SnCalledPackage* pSnCalled = NULL;
const Bdt_SnCalledRespPackage* pSnCalledResp = NULL;
const Bdt_SnPingPackage* pSnPing = NULL;
const Bdt_SnPingRespPackage* pSnPingResp = NULL;
const Bdt_DataGramPackage* pDataGram = NULL;
const Bdt_TcpSynConnectionPackage* pTcpSynConnection = NULL;
const Bdt_TcpAckConnectionPackage* pTcpAckConnection = NULL;
const Bdt_TcpAckAckConnectionPackage* pTcpAckAckConnection = NULL;

	switch (pPackage->cmdtype)
	{	
	
	case BDT_EXCHANGEKEY_PACKAGE:
		pExchangeKey = (Bdt_ExchangeKeyPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pExchangeKey->seq);
		}
		if (fieldName == s_fieldName_seqAndKeySign)
		{
			return &(pExchangeKey->seqAndKeySign);
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			return &(pExchangeKey->fromPeerid);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pExchangeKey->peerInfo);
		}
		if (fieldName == s_fieldName_sendTime)
		{
			return &(pExchangeKey->sendTime);
		}
		break;
	case BDT_SYN_TUNNEL_PACKAGE:
		pSynTunnel = (Bdt_SynTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_fromPeerid)
		{
			return &(pSynTunnel->fromPeerid);
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			return &(pSynTunnel->toPeerid);
		}
		if (fieldName == s_fieldName_proxyPeerid)
		{
			return &(pSynTunnel->proxyPeerid);
		}
		if (fieldName == s_fieldName_proxyPeerInfo)
		{
			return &(pSynTunnel->proxyPeerInfo);
		}
		if (fieldName == s_fieldName_seq)
		{
			return &(pSynTunnel->seq);
		}
		if (fieldName == s_fieldName_fromContainerId)
		{
			return &(pSynTunnel->fromContainerId);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pSynTunnel->peerInfo);
		}
		if (fieldName == s_fieldName_sendTime)
		{
			return &(pSynTunnel->sendTime);
		}
		break;
	case BDT_ACK_TUNNEL_PACKAGE:
		pAckTunnel = (Bdt_AckTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_aesKeyHash)
		{
			return &(pAckTunnel->aesKeyHash);
		}
		if (fieldName == s_fieldName_seq)
		{
			return &(pAckTunnel->seq);
		}
		if (fieldName == s_fieldName_fromContainerId)
		{
			return &(pAckTunnel->fromContainerId);
		}
		if (fieldName == s_fieldName_toContainerId)
		{
			return &(pAckTunnel->toContainerId);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pAckTunnel->result);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pAckTunnel->peerInfo);
		}
		if (fieldName == s_fieldName_sendTime)
		{
			return &(pAckTunnel->sendTime);
		}
		if (fieldName == s_fieldName_mtu)
		{
			return &(pAckTunnel->mtu);
		}
		break;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		pAckAckTunnel = (Bdt_AckAckTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pAckAckTunnel->seq);
		}
		if (fieldName == s_fieldName_toContainerId)
		{
			return &(pAckAckTunnel->toContainerId);
		}
		break;
	case BDT_PING_TUNNEL_PACKAGE:
		pPingTunnel = (Bdt_PingTunnelPackage*)pPackage;
		
		if (fieldName == s_fieldName_toContainerId)
		{
			return &(pPingTunnel->toContainerId);
		}
		if (fieldName == s_fieldName_packageId)
		{
			return &(pPingTunnel->packageId);
		}
		if (fieldName == s_fieldName_sendTime)
		{
			return &(pPingTunnel->sendTime);
		}
		if (fieldName == s_fieldName_recvData)
		{
			return &(pPingTunnel->recvData);
		}
		break;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		pPingTunnelResp = (Bdt_PingTunnelRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_toContainerId)
		{
			return &(pPingTunnelResp->toContainerId);
		}
		if (fieldName == s_fieldName_ackPackageId)
		{
			return &(pPingTunnelResp->ackPackageId);
		}
		if (fieldName == s_fieldName_sendTime)
		{
			return &(pPingTunnelResp->sendTime);
		}
		if (fieldName == s_fieldName_recvData)
		{
			return &(pPingTunnelResp->recvData);
		}
		break;
	case BDT_SN_CALL_PACKAGE:
		pSnCall = (Bdt_SnCallPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pSnCall->seq);
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			return &(pSnCall->snPeerid);
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			return &(pSnCall->toPeerid);
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			return &(pSnCall->fromPeerid);
		}
		if (fieldName == s_fieldName_reverseEndpointArray)
		{
			return &(pSnCall->reverseEndpointArray);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pSnCall->peerInfo);
		}
		if (fieldName == s_fieldName_payload)
		{
			return &(pSnCall->payload);
		}
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		pSnCallResp = (Bdt_SnCallRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pSnCallResp->seq);
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			return &(pSnCallResp->snPeerid);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pSnCallResp->result);
		}
		if (fieldName == s_fieldName_toPeerInfo)
		{
			return &(pSnCallResp->toPeerInfo);
		}
		break;
	case BDT_SN_CALLED_PACKAGE:
		pSnCalled = (Bdt_SnCalledPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pSnCalled->seq);
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			return &(pSnCalled->snPeerid);
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			return &(pSnCalled->toPeerid);
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			return &(pSnCalled->fromPeerid);
		}
		if (fieldName == s_fieldName_reverseEndpointArray)
		{
			return &(pSnCalled->reverseEndpointArray);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pSnCalled->peerInfo);
		}
		if (fieldName == s_fieldName_payload)
		{
			return &(pSnCalled->payload);
		}
		break;
	case BDT_SN_CALLED_RESP_PACKAGE:
		pSnCalledResp = (Bdt_SnCalledRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pSnCalledResp->seq);
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			return &(pSnCalledResp->snPeerid);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pSnCalledResp->result);
		}
		break;
	case BDT_SN_PING_PACKAGE:
		pSnPing = (Bdt_SnPingPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pSnPing->seq);
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			return &(pSnPing->snPeerid);
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			return &(pSnPing->fromPeerid);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pSnPing->peerInfo);
		}
		break;
	case BDT_SN_PING_RESP_PACKAGE:
		pSnPingResp = (Bdt_SnPingRespPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pSnPingResp->seq);
		}
		if (fieldName == s_fieldName_snPeerid)
		{
			return &(pSnPingResp->snPeerid);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pSnPingResp->result);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pSnPingResp->peerInfo);
		}
		if (fieldName == s_fieldName_endpointArray)
		{
			return &(pSnPingResp->endpointArray);
		}
		break;
	case BDT_DATA_GRAM_PACKAGE:
		pDataGram = (Bdt_DataGramPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pDataGram->seq);
		}
		if (fieldName == s_fieldName_destZone)
		{
			return &(pDataGram->destZone);
		}
		if (fieldName == s_fieldName_hopLimit)
		{
			return &(pDataGram->hopLimit);
		}
		if (fieldName == s_fieldName_piece)
		{
			return &(pDataGram->piece);
		}
		if (fieldName == s_fieldName_ackSeq)
		{
			return &(pDataGram->ackSeq);
		}
		if (fieldName == s_fieldName_sendTime)
		{
			return &(pDataGram->sendTime);
		}
		if (fieldName == s_fieldName_authorPeerid)
		{
			return &(pDataGram->authorPeerid);
		}
		if (fieldName == s_fieldName_authorPeerInfo)
		{
			return &(pDataGram->authorPeerInfo);
		}
		if (fieldName == s_fieldName_dataSign)
		{
			return &(pDataGram->dataSign);
		}
		if (fieldName == s_fieldName_innerCmdType)
		{
			return &(pDataGram->innerCmdType);
		}
		if (fieldName == s_fieldName_data)
		{
			return &(pDataGram->data);
		}
		break;
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		pTcpSynConnection = (Bdt_TcpSynConnectionPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pTcpSynConnection->seq);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pTcpSynConnection->result);
		}
		if (fieldName == s_fieldName_toVPort)
		{
			return &(pTcpSynConnection->toVPort);
		}
		if (fieldName == s_fieldName_fromSessionId)
		{
			return &(pTcpSynConnection->fromSessionId);
		}
		if (fieldName == s_fieldName_fromPeerid)
		{
			return &(pTcpSynConnection->fromPeerid);
		}
		if (fieldName == s_fieldName_toPeerid)
		{
			return &(pTcpSynConnection->toPeerid);
		}
		if (fieldName == s_fieldName_proxyPeerid)
		{
			return &(pTcpSynConnection->proxyPeerid);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pTcpSynConnection->peerInfo);
		}
		if (fieldName == s_fieldName_reverseEndpointArray)
		{
			return &(pTcpSynConnection->reverseEndpointArray);
		}
		if (fieldName == s_fieldName_payload)
		{
			return &(pTcpSynConnection->payload);
		}
		break;
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		pTcpAckConnection = (Bdt_TcpAckConnectionPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pTcpAckConnection->seq);
		}
		if (fieldName == s_fieldName_toSessionId)
		{
			return &(pTcpAckConnection->toSessionId);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pTcpAckConnection->result);
		}
		if (fieldName == s_fieldName_peerInfo)
		{
			return &(pTcpAckConnection->peerInfo);
		}
		if (fieldName == s_fieldName_payload)
		{
			return &(pTcpAckConnection->payload);
		}
		break;
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		pTcpAckAckConnection = (Bdt_TcpAckAckConnectionPackage*)pPackage;
		
		if (fieldName == s_fieldName_seq)
		{
			return &(pTcpAckAckConnection->seq);
		}
		if (fieldName == s_fieldName_result)
		{
			return &(pTcpAckAckConnection->result);
		}
		break;
	}
	return NULL;
}

//THIS FUNCTION GENERATE BY packages.js
static int EncodeSynTunnelPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SynTunnelPackage* pPackage = (Bdt_SynTunnelPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->fromPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->fromPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->toPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->toPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->proxyPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->proxyPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->proxyPeerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->proxyPeerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->fromContainerId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->fromContainerId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->sendTime == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME);
		}
	}

	if (pPackage->cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->sendTime),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeAckTunnelPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_AckTunnelPackage* pPackage = (Bdt_AckTunnelPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->aesKeyHash == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->aesKeyHash),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->fromContainerId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->fromContainerId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->toContainerId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->toContainerId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->sendTime == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->sendTime),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU);
		if (fieldName)
		{	
			uint16_t* rParam = (uint16_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->mtu == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU);
		}
	}

	if (pPackage->cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU)
	{
		//write to buffer
		r = BfxBufferWriteUInt16(pStream, (pPackage->mtu),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeAckAckTunnelPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_AckAckTunnelPackage* pPackage = (Bdt_AckAckTunnelPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->toContainerId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		}
	}

	if (pPackage->cmdflags & BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->toContainerId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodePingTunnelPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_PingTunnelPackage* pPackage = (Bdt_PingTunnelPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->toContainerId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->toContainerId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->packageId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->packageId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->sendTime == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->sendTime),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->recvData == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->recvData),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodePingTunnelRespPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_PingTunnelRespPackage* pPackage = (Bdt_PingTunnelRespPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->toContainerId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->toContainerId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->ackPackageId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->ackPackageId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->sendTime == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->sendTime),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA);
		if (fieldName)
		{	
			uint64_t* rParam = (uint64_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->recvData == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA);
		}
	}

	if (pPackage->cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA)
	{
		//write to buffer
		r = BfxBufferWriteUInt64(pStream, (pPackage->recvData),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeSnCallPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SnCallPackage* pPackage = (Bdt_SnCallPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_SNPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->snPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_SNPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_SNPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_SNPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_SNPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_SNPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_SNPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->snPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_TOPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->toPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_TOPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_TOPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_TOPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_TOPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_TOPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_TOPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->toPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->fromPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->fromPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		if (fieldName)
		{	
			BdtEndpointArray* rParam = (BdtEndpointArray*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtEndpointArrayIsEqual(rParam,&(pPackage->reverseEndpointArray));;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY)
	{
		//write to buffer
		r = Bdt_BufferWriteEndpointArray(pStream,&(pPackage->reverseEndpointArray),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD);
		if (fieldName)
		{	
			BFX_BUFFER_HANDLE* rParam = (BFX_BUFFER_HANDLE*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = false;;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD)
	{
		//write to buffer
		r = BfxBufferWriteByteArrayBeginWithU16Length(pStream,(pPackage->payload),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeSnCallRespPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SnCallRespPackage* pPackage = (Bdt_SnCallRespPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->snPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->snPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->toPeerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->toPeerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeSnCalledPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SnCalledPackage* pPackage = (Bdt_SnCalledPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->snPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->snPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->toPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->toPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->fromPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->fromPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		if (fieldName)
		{	
			BdtEndpointArray* rParam = (BdtEndpointArray*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtEndpointArrayIsEqual(rParam,&(pPackage->reverseEndpointArray));;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY)
	{
		//write to buffer
		r = Bdt_BufferWriteEndpointArray(pStream,&(pPackage->reverseEndpointArray),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD);
		if (fieldName)
		{	
			BFX_BUFFER_HANDLE* rParam = (BFX_BUFFER_HANDLE*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = false;;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD)
	{
		//write to buffer
		r = BfxBufferWriteByteArrayBeginWithU16Length(pStream,(pPackage->payload),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeSnCalledRespPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SnCalledRespPackage* pPackage = (Bdt_SnCalledRespPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->snPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->snPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeSnPingPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SnPingPackage* pPackage = (Bdt_SnPingPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_PACKAGE_FLAG_SNPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->snPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_SNPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_SNPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_PACKAGE_FLAG_SNPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_SNPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_SNPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_PACKAGE_FLAG_SNPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->snPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_PACKAGE_FLAG_FROMPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->fromPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_FROMPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_FROMPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_PACKAGE_FLAG_FROMPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_FROMPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_FROMPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_PACKAGE_FLAG_FROMPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->fromPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeSnPingRespPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_SnPingRespPackage* pPackage = (Bdt_SnPingRespPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->snPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->snPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY);
		if (fieldName)
		{	
			BdtEndpointArray* rParam = (BdtEndpointArray*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtEndpointArrayIsEqual(rParam,&(pPackage->endpointArray));;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY);
		}
	}

	if (pPackage->cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY)
	{
		//write to buffer
		r = Bdt_BufferWriteEndpointArray(pStream,&(pPackage->endpointArray),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeTcpSynConnectionPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_TcpSynConnectionPackage* pPackage = (Bdt_TcpSynConnectionPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT);
		if (fieldName)
		{	
			uint16_t* rParam = (uint16_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->toVPort == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT)
	{
		//write to buffer
		r = BfxBufferWriteUInt16(pStream, (pPackage->toVPort),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->fromSessionId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->fromSessionId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->fromPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->fromPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->toPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->toPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID);
		if (fieldName)
		{	
			BdtPeerid* rParam = (BdtPeerid*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (memcmp(rParam->pid, pPackage->proxyPeerid.pid, BDT_PEERID_LENGTH) ==0);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID)
	{
		//write to buffer
		r = Bdt_BufferWritePeerid(pStream, &(pPackage->proxyPeerid),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		if (fieldName)
		{	
			BdtEndpointArray* rParam = (BdtEndpointArray*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtEndpointArrayIsEqual(rParam,&(pPackage->reverseEndpointArray));;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY)
	{
		//write to buffer
		r = Bdt_BufferWriteEndpointArray(pStream,&(pPackage->reverseEndpointArray),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD);
		if (fieldName)
		{	
			BFX_BUFFER_HANDLE* rParam = (BFX_BUFFER_HANDLE*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = false;;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD)
	{
		//write to buffer
		r = BfxBufferWriteByteArrayBeginWithU16Length(pStream,(pPackage->payload),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeTcpAckConnectionPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_TcpAckConnectionPackage* pPackage = (Bdt_TcpAckConnectionPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->toSessionId == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->toSessionId),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO);
		if (fieldName)
		{	
			BdtPeerInfo** rParam = (BdtPeerInfo**)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = BdtPeerInfoIsEqual(pPackage->peerInfo,*rParam);;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO)
	{
		//write to buffer
		r = BufferWritePeerInfo(pStream, (pPackage->peerInfo),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD);
		if (fieldName)
		{	
			BFX_BUFFER_HANDLE* rParam = (BFX_BUFFER_HANDLE*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = false;;
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD)
	{
		//write to buffer
		r = BfxBufferWriteByteArrayBeginWithU16Length(pStream,(pPackage->payload),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		
//THIS FUNCTION GENERATE BY packages.js
static int EncodeTcpAckAckConnectionPackage(Bdt_Package* package, const Bdt_Package* refPackage, BfxBufferStream* pStream, size_t* pWriteBytes)
{
	Bdt_TcpAckAckConnectionPackage* pPackage = (Bdt_TcpAckAckConnectionPackage*)package;
	size_t writeBytes = 0;
	bool isEqual = false;
	bool haveRef = false;
	size_t totalWriteBytes = 0;
	int r = 0;
	*pWriteBytes = 0;

	if (package == refPackage)
	{
		refPackage = NULL;
	}
	if (pPackage->cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF)
	{
		refPackage = NULL;
	}

	size_t headerPos = BfxBufferStreamGetPos(pStream);
	
	//write cmdtype,totallen,cmdflags
	BfxBufferStreamSetPos(pStream,headerPos + 1 + 2 + 2);
	totalWriteBytes += 5;		
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ);
		if (fieldName)
		{	
			uint32_t* rParam = (uint32_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->seq == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ)
	{
		//write to buffer
		r = BfxBufferWriteUInt32(pStream, (pPackage->seq),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	haveRef = false;
	isEqual = false;
	if (refPackage)
	{
		const char* fieldName = GetPackageFieldName(package->cmdtype,BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT);
		if (fieldName)
		{	
			uint8_t* rParam = (uint8_t*)GetPackageFieldPointByName(refPackage, fieldName);
			if(rParam)
			{
				haveRef = true;
				isEqual = (pPackage->result == *rParam);
				if (isEqual)
				{
					pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT);
				}
				else
				{
					pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT);
				}
			}
		}
	}
	
	if (!haveRef) 
	{
		if(IsEqualPackageFieldDefaultValue(package, BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT))
		{
			pPackage->cmdflags = pPackage->cmdflags & (~BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT);
		}
		else
		{
			pPackage->cmdflags = pPackage->cmdflags | (BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT);
		}
	}

	if (pPackage->cmdflags & BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT)
	{
		//write to buffer
		r = BfxBufferWriteUInt8(pStream, (pPackage->result),&writeBytes);

		if (r != BFX_RESULT_SUCCESS)
		{
			return r;
		}
		totalWriteBytes += writeBytes;
		writeBytes = 0;
	}
	//-------------------------------------------------------------
	
	size_t tailPos = BfxBufferStreamGetPos(pStream);
	BfxBufferStreamSetPos(pStream, headerPos);
	BfxBufferWriteUInt16(pStream, (uint16_t)totalWriteBytes,&writeBytes);
	BfxBufferWriteUInt8(pStream, pPackage->cmdtype,&writeBytes);
	BfxBufferWriteUInt16(pStream, pPackage->cmdflags,&writeBytes);
	BfxBufferStreamSetPos(pStream, tailPos);
	*pWriteBytes = totalWriteBytes;
	
	return BFX_RESULT_SUCCESS;
}	

		

//THIS FUNCTION GENERATE BY packages.js	
int Bdt_EncodeSinglePackage(Bdt_Package* package, const Bdt_Package* refPackage,BfxBufferStream* pStream, size_t* pWriteBytes)
{

	switch (package->cmdtype)
	{
	case BDT_EXCHANGEKEY_PACKAGE:
		return EncodeExchangeKeyPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SYN_TUNNEL_PACKAGE:
		return EncodeSynTunnelPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_ACK_TUNNEL_PACKAGE:
		return EncodeAckTunnelPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_ACKACK_TUNNEL_PACKAGE:
		return EncodeAckAckTunnelPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_PING_TUNNEL_PACKAGE:
		return EncodePingTunnelPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		return EncodePingTunnelRespPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SN_CALL_PACKAGE:
		return EncodeSnCallPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SN_CALL_RESP_PACKAGE:
		return EncodeSnCallRespPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SN_CALLED_PACKAGE:
		return EncodeSnCalledPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SN_CALLED_RESP_PACKAGE:
		return EncodeSnCalledRespPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SN_PING_PACKAGE:
		return EncodeSnPingPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SN_PING_RESP_PACKAGE:
		return EncodeSnPingRespPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_DATA_GRAM_PACKAGE:
		return EncodeDataGramPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		return EncodeTcpSynConnectionPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		return EncodeTcpAckConnectionPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		return EncodeTcpAckAckConnectionPackage(package,refPackage, pStream, pWriteBytes);
		break;
	case BDT_SESSION_DATA_PACKAGE:
		return EncodeSessionDataPackage(package,refPackage, pStream, pWriteBytes);
		break;
	}
	BLOG_WARN("Encode SinglePackage:unknown package,cmdtype=%d", package->cmdtype);
	return BFX_RESULT_UNKNOWN_TYPE;
}	
	

//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSynTunnelPackage(
	Bdt_SynTunnelPackage* pResult, 
	uint16_t totallen, 
	BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SYN_TUNNEL_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_fromPeerid = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID;
	if (haveField_fromPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->fromPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read fromPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMPEERID, pRefPackage,(void*)&(pResult->fromPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field fromPeerid read from refPackage.");
			}
		}
	}
	bool haveField_toPeerid = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID;
	if (haveField_toPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->toPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read toPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_TOPEERID, pRefPackage,(void*)&(pResult->toPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field toPeerid read from refPackage.");
			}
		}
	}
	bool haveField_proxyPeerid = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID;
	if (haveField_proxyPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->proxyPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read proxyPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERID, pRefPackage,(void*)&(pResult->proxyPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field proxyPeerid read from refPackage.");
			}
		}
	}
	bool haveField_proxyPeerInfo = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO;
	if (haveField_proxyPeerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->proxyPeerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read proxyPeerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_PROXYPEERINFO, pRefPackage,(void*)&(pResult->proxyPeerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field proxyPeerInfo read from refPackage.");
			}
		}
	}
	bool haveField_seq = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_fromContainerId = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID;
	if (haveField_fromContainerId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->fromContainerId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read fromContainerId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID, pRefPackage,(void*)&(pResult->fromContainerId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field fromContainerId read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_sendTime = cmdflags & BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME;
	if (haveField_sendTime)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->sendTime));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,cann't read sendTime.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SYN_TUNNEL_PACKAGE_FLAG_SENDTIME, pRefPackage,(void*)&(pResult->sendTime));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SYN_TUNNEL_PACKAGE:field sendTime read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SYN_TUNNEL_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeAckTunnelPackage(Bdt_AckTunnelPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_ACK_TUNNEL_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_aesKeyHash = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH;
	if (haveField_aesKeyHash)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->aesKeyHash));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read aesKeyHash.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_AESKEYHASH, pRefPackage,(void*)&(pResult->aesKeyHash));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field aesKeyHash read from refPackage.");
			}
		}
	}
	bool haveField_seq = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_fromContainerId = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID;
	if (haveField_fromContainerId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->fromContainerId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read fromContainerId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_FROMCONTAINERID, pRefPackage,(void*)&(pResult->fromContainerId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field fromContainerId read from refPackage.");
			}
		}
	}
	bool haveField_toContainerId = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID;
	if (haveField_toContainerId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->toContainerId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read toContainerId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID, pRefPackage,(void*)&(pResult->toContainerId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field toContainerId read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field result read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_sendTime = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME;
	if (haveField_sendTime)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->sendTime));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read sendTime.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_SENDTIME, pRefPackage,(void*)&(pResult->sendTime));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field sendTime read from refPackage.");
			}
		}
	}
	bool haveField_mtu = cmdflags & BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU;
	if (haveField_mtu)
	{
		r = BfxBufferReadUInt16(bufferStream, &(pResult->mtu));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,cann't read mtu.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACK_TUNNEL_PACKAGE_FLAG_MTU, pRefPackage,(void*)&(pResult->mtu));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACK_TUNNEL_PACKAGE:field mtu read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_ACK_TUNNEL_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeAckAckTunnelPackage(Bdt_AckAckTunnelPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_ACKACK_TUNNEL_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_ACKACK_TUNNEL_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_ACKACK_TUNNEL_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACKACK_TUNNEL_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACKACK_TUNNEL_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACKACK_TUNNEL_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_toContainerId = cmdflags & BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID;
	if (haveField_toContainerId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->toContainerId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_ACKACK_TUNNEL_PACKAGE error,cann't read toContainerId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_ACKACK_TUNNEL_PACKAGE_FLAG_TOCONTAINERID, pRefPackage,(void*)&(pResult->toContainerId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_ACKACK_TUNNEL_PACKAGE:field toContainerId read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_ACKACK_TUNNEL_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodePingTunnelPackage(Bdt_PingTunnelPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_PING_TUNNEL_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_toContainerId = cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID;
	if (haveField_toContainerId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->toContainerId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE error,cann't read toContainerId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_PACKAGE_FLAG_TOCONTAINERID, pRefPackage,(void*)&(pResult->toContainerId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_PACKAGE:field toContainerId read from refPackage.");
			}
		}
	}
	bool haveField_packageId = cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID;
	if (haveField_packageId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->packageId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE error,cann't read packageId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_PACKAGE_FLAG_PACKAGEID, pRefPackage,(void*)&(pResult->packageId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_PACKAGE:field packageId read from refPackage.");
			}
		}
	}
	bool haveField_sendTime = cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME;
	if (haveField_sendTime)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->sendTime));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE error,cann't read sendTime.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_PACKAGE_FLAG_SENDTIME, pRefPackage,(void*)&(pResult->sendTime));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_PACKAGE:field sendTime read from refPackage.");
			}
		}
	}
	bool haveField_recvData = cmdflags & BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA;
	if (haveField_recvData)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->recvData));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE error,cann't read recvData.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_PACKAGE_FLAG_RECVDATA, pRefPackage,(void*)&(pResult->recvData));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_PACKAGE:field recvData read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_PING_TUNNEL_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodePingTunnelRespPackage(Bdt_PingTunnelRespPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_PING_TUNNEL_RESP_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_toContainerId = cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID;
	if (haveField_toContainerId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->toContainerId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE error,cann't read toContainerId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_TOCONTAINERID, pRefPackage,(void*)&(pResult->toContainerId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_RESP_PACKAGE:field toContainerId read from refPackage.");
			}
		}
	}
	bool haveField_ackPackageId = cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID;
	if (haveField_ackPackageId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->ackPackageId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE error,cann't read ackPackageId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_ACKPACKAGEID, pRefPackage,(void*)&(pResult->ackPackageId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_RESP_PACKAGE:field ackPackageId read from refPackage.");
			}
		}
	}
	bool haveField_sendTime = cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME;
	if (haveField_sendTime)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->sendTime));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE error,cann't read sendTime.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_SENDTIME, pRefPackage,(void*)&(pResult->sendTime));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_RESP_PACKAGE:field sendTime read from refPackage.");
			}
		}
	}
	bool haveField_recvData = cmdflags & BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA;
	if (haveField_recvData)
	{
		r = BfxBufferReadUInt64(bufferStream, &(pResult->recvData));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE error,cann't read recvData.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_PING_TUNNEL_RESP_PACKAGE_FLAG_RECVDATA, pRefPackage,(void*)&(pResult->recvData));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_PING_TUNNEL_RESP_PACKAGE:field recvData read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_PING_TUNNEL_RESP_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSnCallPackage(Bdt_SnCallPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SN_CALL_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SN_CALL_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_snPeerid = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_SNPEERID;
	if (haveField_snPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->snPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read snPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_SNPEERID, pRefPackage,(void*)&(pResult->snPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field snPeerid read from refPackage.");
			}
		}
	}
	bool haveField_toPeerid = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_TOPEERID;
	if (haveField_toPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->toPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read toPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_TOPEERID, pRefPackage,(void*)&(pResult->toPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field toPeerid read from refPackage.");
			}
		}
	}
	bool haveField_fromPeerid = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID;
	if (haveField_fromPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->fromPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read fromPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_FROMPEERID, pRefPackage,(void*)&(pResult->fromPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field fromPeerid read from refPackage.");
			}
		}
	}
	bool haveField_reverseEndpointArray = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY;
	if (haveField_reverseEndpointArray)
	{
		r = Bdt_BufferReadEndpointArray(bufferStream, &(pResult->reverseEndpointArray));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read reverseEndpointArray.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_REVERSEENDPOINTARRAY, pRefPackage,(void*)&(pResult->reverseEndpointArray));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field reverseEndpointArray read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_payload = cmdflags & BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD;
	if (haveField_payload)
	{
		r = BfxBufferReadByteArrayBeginWithU16Length(bufferStream,&(pResult->payload));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,cann't read payload.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_PACKAGE_FLAG_PAYLOAD, pRefPackage,(void*)&(pResult->payload));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_PACKAGE:field payload read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SN_CALL_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSnCallRespPackage(Bdt_SnCallRespPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SN_CALL_RESP_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_RESP_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_RESP_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_snPeerid = cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID;
	if (haveField_snPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->snPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE error,cann't read snPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_RESP_PACKAGE_FLAG_SNPEERID, pRefPackage,(void*)&(pResult->snPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_RESP_PACKAGE:field snPeerid read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_RESP_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_RESP_PACKAGE:field result read from refPackage.");
			}
		}
	}
	bool haveField_toPeerInfo = cmdflags & BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO;
	if (haveField_toPeerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->toPeerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE error,cann't read toPeerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALL_RESP_PACKAGE_FLAG_TOPEERINFO, pRefPackage,(void*)&(pResult->toPeerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALL_RESP_PACKAGE:field toPeerInfo read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SN_CALL_RESP_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSnCalledPackage(Bdt_SnCalledPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SN_CALLED_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SN_CALLED_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_snPeerid = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID;
	if (haveField_snPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->snPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read snPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_SNPEERID, pRefPackage,(void*)&(pResult->snPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field snPeerid read from refPackage.");
			}
		}
	}
	bool haveField_toPeerid = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID;
	if (haveField_toPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->toPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read toPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_TOPEERID, pRefPackage,(void*)&(pResult->toPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field toPeerid read from refPackage.");
			}
		}
	}
	bool haveField_fromPeerid = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID;
	if (haveField_fromPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->fromPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read fromPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_FROMPEERID, pRefPackage,(void*)&(pResult->fromPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field fromPeerid read from refPackage.");
			}
		}
	}
	bool haveField_reverseEndpointArray = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY;
	if (haveField_reverseEndpointArray)
	{
		r = Bdt_BufferReadEndpointArray(bufferStream, &(pResult->reverseEndpointArray));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read reverseEndpointArray.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_REVERSEENDPOINTARRAY, pRefPackage,(void*)&(pResult->reverseEndpointArray));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field reverseEndpointArray read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_payload = cmdflags & BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD;
	if (haveField_payload)
	{
		r = BfxBufferReadByteArrayBeginWithU16Length(bufferStream,&(pResult->payload));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,cann't read payload.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_PACKAGE_FLAG_PAYLOAD, pRefPackage,(void*)&(pResult->payload));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_PACKAGE:field payload read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SN_CALLED_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSnCalledRespPackage(Bdt_SnCalledRespPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SN_CALLED_RESP_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SN_CALLED_RESP_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SN_CALLED_RESP_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_RESP_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_RESP_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_RESP_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_snPeerid = cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID;
	if (haveField_snPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->snPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_RESP_PACKAGE error,cann't read snPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_RESP_PACKAGE_FLAG_SNPEERID, pRefPackage,(void*)&(pResult->snPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_RESP_PACKAGE:field snPeerid read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_CALLED_RESP_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_CALLED_RESP_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_CALLED_RESP_PACKAGE:field result read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SN_CALLED_RESP_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSnPingPackage(Bdt_SnPingPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SN_PING_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SN_PING_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SN_PING_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_SN_PING_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_snPeerid = cmdflags & BDT_SN_PING_PACKAGE_FLAG_SNPEERID;
	if (haveField_snPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->snPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_PACKAGE error,cann't read snPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_PACKAGE_FLAG_SNPEERID, pRefPackage,(void*)&(pResult->snPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_PACKAGE:field snPeerid read from refPackage.");
			}
		}
	}
	bool haveField_fromPeerid = cmdflags & BDT_SN_PING_PACKAGE_FLAG_FROMPEERID;
	if (haveField_fromPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->fromPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_PACKAGE error,cann't read fromPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_PACKAGE_FLAG_FROMPEERID, pRefPackage,(void*)&(pResult->fromPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_PACKAGE:field fromPeerid read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_SN_PING_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SN_PING_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeSnPingRespPackage(Bdt_SnPingRespPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_SN_PING_RESP_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_RESP_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_RESP_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_snPeerid = cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID;
	if (haveField_snPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->snPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,cann't read snPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_RESP_PACKAGE_FLAG_SNPEERID, pRefPackage,(void*)&(pResult->snPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_RESP_PACKAGE:field snPeerid read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_RESP_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_RESP_PACKAGE:field result read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_RESP_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_RESP_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_endpointArray = cmdflags & BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY;
	if (haveField_endpointArray)
	{
		r = Bdt_BufferReadEndpointArray(bufferStream, &(pResult->endpointArray));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,cann't read endpointArray.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_SN_PING_RESP_PACKAGE_FLAG_ENDPOINTARRAY, pRefPackage,(void*)&(pResult->endpointArray));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_SN_PING_RESP_PACKAGE:field endpointArray read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_SN_PING_RESP_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeTcpSynConnectionPackage(Bdt_TcpSynConnectionPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_TCP_SYN_CONNECTION_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field result read from refPackage.");
			}
		}
	}
	bool haveField_toVPort = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT;
	if (haveField_toVPort)
	{
		r = BfxBufferReadUInt16(bufferStream, &(pResult->toVPort));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read toVPort.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOVPORT, pRefPackage,(void*)&(pResult->toVPort));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field toVPort read from refPackage.");
			}
		}
	}
	bool haveField_fromSessionId = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID;
	if (haveField_fromSessionId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->fromSessionId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read fromSessionId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMSESSIONID, pRefPackage,(void*)&(pResult->fromSessionId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field fromSessionId read from refPackage.");
			}
		}
	}
	bool haveField_fromPeerid = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID;
	if (haveField_fromPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->fromPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read fromPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_FROMPEERID, pRefPackage,(void*)&(pResult->fromPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field fromPeerid read from refPackage.");
			}
		}
	}
	bool haveField_toPeerid = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID;
	if (haveField_toPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->toPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read toPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_TOPEERID, pRefPackage,(void*)&(pResult->toPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field toPeerid read from refPackage.");
			}
		}
	}
	bool haveField_proxyPeerid = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID;
	if (haveField_proxyPeerid)
	{
		r = Bdt_BufferReadPeerid(bufferStream, &(pResult->proxyPeerid));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read proxyPeerid.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PROXYPEERID, pRefPackage,(void*)&(pResult->proxyPeerid));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field proxyPeerid read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_reverseEndpointArray = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY;
	if (haveField_reverseEndpointArray)
	{
		r = Bdt_BufferReadEndpointArray(bufferStream, &(pResult->reverseEndpointArray));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read reverseEndpointArray.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_REVERSEENDPOINTARRAY, pRefPackage,(void*)&(pResult->reverseEndpointArray));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field reverseEndpointArray read from refPackage.");
			}
		}
	}
	bool haveField_payload = cmdflags & BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD;
	if (haveField_payload)
	{
		r = BfxBufferReadByteArrayBeginWithU16Length(bufferStream,&(pResult->payload));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,cann't read payload.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_SYN_CONNECTION_PACKAGE_FLAG_PAYLOAD, pRefPackage,(void*)&(pResult->payload));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_SYN_CONNECTION_PACKAGE:field payload read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_TCP_SYN_CONNECTION_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeTcpAckConnectionPackage(Bdt_TcpAckConnectionPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_TCP_ACK_CONNECTION_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACK_CONNECTION_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_toSessionId = cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID;
	if (haveField_toSessionId)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->toSessionId));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,cann't read toSessionId.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_TOSESSIONID, pRefPackage,(void*)&(pResult->toSessionId));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACK_CONNECTION_PACKAGE:field toSessionId read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACK_CONNECTION_PACKAGE:field result read from refPackage.");
			}
		}
	}
	bool haveField_peerInfo = cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO;
	if (haveField_peerInfo)
	{
		r = BufferReadPeerInfo(bufferStream, &(pResult->peerInfo));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,cann't read peerInfo.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PEERINFO, pRefPackage,(void*)&(pResult->peerInfo));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACK_CONNECTION_PACKAGE:field peerInfo read from refPackage.");
			}
		}
	}
	bool haveField_payload = cmdflags & BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD;
	if (haveField_payload)
	{
		r = BfxBufferReadByteArrayBeginWithU16Length(bufferStream,&(pResult->payload));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,cann't read payload.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACK_CONNECTION_PACKAGE_FLAG_PAYLOAD, pRefPackage,(void*)&(pResult->payload));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACK_CONNECTION_PACKAGE:field payload read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_TCP_ACK_CONNECTION_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}
//THIS FUNCTION GENERATE BY packages.js	
static int DecodeTcpAckAckConnectionPackage(Bdt_TcpAckAckConnectionPackage* pResult, uint16_t totallen, BfxBufferStream* bufferStream, Bdt_Package* pRefPackage)
{
	int r = 0;
	size_t minsize = Bdt_GetPackageMinSize(BDT_TCP_ACKACK_CONNECTION_PACKAGE);
	uint16_t cmdflags;
	int readlen = 0;

	if (totallen < minsize)
	{
		BLOG_WARN("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE package error,totallen(%hu) too small.", totallen);
		goto END;
	}

	r = BfxBufferReadUInt16(bufferStream, &(cmdflags));
	if (r <= 0)
	{
		BLOG_WARN("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE error,cann't read cmdflags.");
		goto END;
	}
	readlen += r;
	pResult->cmdflags = cmdflags;
	bool isDisableRef = cmdflags & BDT_PACKAGE_FLAG_DISABLE_REF;
	if (isDisableRef)
	{
		pRefPackage = NULL;
	}
	
	bool haveField_seq = cmdflags & BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ;
	if (haveField_seq)
	{
		r = BfxBufferReadUInt32(bufferStream, &(pResult->seq));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE error,cann't read seq.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_SEQ, pRefPackage,(void*)&(pResult->seq));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE:field seq read from refPackage.");
			}
		}
	}
	bool haveField_result = cmdflags & BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT;
	if (haveField_result)
	{
		r = BfxBufferReadUInt8(bufferStream, &(pResult->result));
		if (r <= 0)
		{
			BLOG_WARN("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE error,cann't read result.");
			r = -1;   
			goto END;
		}
		readlen += r;
	} 
	else
	{
		if (pRefPackage)
		{
			r = ReadPackageFieldValueFromReference(pResult->cmdtype, BDT_TCP_ACKACK_CONNECTION_PACKAGE_FLAG_RESULT, pRefPackage,(void*)&(pResult->result));
			if (r == BFX_RESULT_SUCCESS)
			{
				BLOG_INFO("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE:field result read from refPackage.");
			}
		}
	}
	r = BFX_RESULT_SUCCESS;
END:
	if (totallen != readlen)
	{
		BLOG_WARN("decode BDT_TCP_ACKACK_CONNECTION_PACKAGE error,totallen != readlen.");
		r = -1;
	}
	return r;
}

//THIS FUNCTION GENERATE BY packages.js
static Bdt_Package* DecodeSinglePackage(
	uint8_t cmdtype, 
	uint16_t totallen, 
	BfxBufferStream* bufferStream, 
	Bdt_Package* pRefPackage)
{

	int r = 0;
	Bdt_SynTunnelPackage* pSynTunnelPackage = NULL;
	Bdt_AckTunnelPackage* pAckTunnelPackage = NULL;
	Bdt_AckAckTunnelPackage* pAckAckTunnelPackage = NULL;
	Bdt_PingTunnelPackage* pPingTunnelPackage = NULL;
	Bdt_PingTunnelRespPackage* pPingTunnelRespPackage = NULL;
	Bdt_SnCallPackage* pSnCallPackage = NULL;
	Bdt_SnCallRespPackage* pSnCallRespPackage = NULL;
	Bdt_SnCalledPackage* pSnCalledPackage = NULL;
	Bdt_SnCalledRespPackage* pSnCalledRespPackage = NULL;
	Bdt_SnPingPackage* pSnPingPackage = NULL;
	Bdt_SnPingRespPackage* pSnPingRespPackage = NULL;
	Bdt_DataGramPackage* pDataGramPackage = NULL;
	Bdt_TcpSynConnectionPackage* pTcpSynConnectionPackage = NULL;
	Bdt_TcpAckConnectionPackage* pTcpAckConnectionPackage = NULL;
	Bdt_TcpAckAckConnectionPackage* pTcpAckAckConnectionPackage = NULL;
	Bdt_SessionDataPackage* pSessionDataPackage = NULL;

	switch (cmdtype)
	{
	case BDT_SYN_TUNNEL_PACKAGE:
		pSynTunnelPackage = Bdt_SynTunnelPackageCreate();
		r = DecodeSynTunnelPackage(pSynTunnelPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSynTunnelPackage;
		}
		else
		{
			Bdt_SynTunnelPackageFree(pSynTunnelPackage);
			return NULL;
		}
		break;	
	
	case BDT_ACK_TUNNEL_PACKAGE:
		pAckTunnelPackage = Bdt_AckTunnelPackageCreate();
		r = DecodeAckTunnelPackage(pAckTunnelPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pAckTunnelPackage;
		}
		else
		{
			Bdt_AckTunnelPackageFree(pAckTunnelPackage);
			return NULL;
		}
		break;	
	
	case BDT_ACKACK_TUNNEL_PACKAGE:
		pAckAckTunnelPackage = Bdt_AckAckTunnelPackageCreate();
		r = DecodeAckAckTunnelPackage(pAckAckTunnelPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pAckAckTunnelPackage;
		}
		else
		{
			Bdt_AckAckTunnelPackageFree(pAckAckTunnelPackage);
			return NULL;
		}
		break;	
	
	case BDT_PING_TUNNEL_PACKAGE:
		pPingTunnelPackage = Bdt_PingTunnelPackageCreate();
		r = DecodePingTunnelPackage(pPingTunnelPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pPingTunnelPackage;
		}
		else
		{
			Bdt_PingTunnelPackageFree(pPingTunnelPackage);
			return NULL;
		}
		break;	
	
	case BDT_PING_TUNNEL_RESP_PACKAGE:
		pPingTunnelRespPackage = Bdt_PingTunnelRespPackageCreate();
		r = DecodePingTunnelRespPackage(pPingTunnelRespPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pPingTunnelRespPackage;
		}
		else
		{
			Bdt_PingTunnelRespPackageFree(pPingTunnelRespPackage);
			return NULL;
		}
		break;	
	
	case BDT_SN_CALL_PACKAGE:
		pSnCallPackage = Bdt_SnCallPackageCreate();
		r = DecodeSnCallPackage(pSnCallPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSnCallPackage;
		}
		else
		{
			Bdt_SnCallPackageFree(pSnCallPackage);
			return NULL;
		}
		break;	
	
	case BDT_SN_CALL_RESP_PACKAGE:
		pSnCallRespPackage = Bdt_SnCallRespPackageCreate();
		r = DecodeSnCallRespPackage(pSnCallRespPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSnCallRespPackage;
		}
		else
		{
			Bdt_SnCallRespPackageFree(pSnCallRespPackage);
			return NULL;
		}
		break;	
	
	case BDT_SN_CALLED_PACKAGE:
		pSnCalledPackage = Bdt_SnCalledPackageCreate();
		r = DecodeSnCalledPackage(pSnCalledPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSnCalledPackage;
		}
		else
		{
			Bdt_SnCalledPackageFree(pSnCalledPackage);
			return NULL;
		}
		break;	
	
	case BDT_SN_CALLED_RESP_PACKAGE:
		pSnCalledRespPackage = Bdt_SnCalledRespPackageCreate();
		r = DecodeSnCalledRespPackage(pSnCalledRespPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSnCalledRespPackage;
		}
		else
		{
			Bdt_SnCalledRespPackageFree(pSnCalledRespPackage);
			return NULL;
		}
		break;	
	
	case BDT_SN_PING_PACKAGE:
		pSnPingPackage = Bdt_SnPingPackageCreate();
		r = DecodeSnPingPackage(pSnPingPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSnPingPackage;
		}
		else
		{
			Bdt_SnPingPackageFree(pSnPingPackage);
			return NULL;
		}
		break;	
	
	case BDT_SN_PING_RESP_PACKAGE:
		pSnPingRespPackage = Bdt_SnPingRespPackageCreate();
		r = DecodeSnPingRespPackage(pSnPingRespPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSnPingRespPackage;
		}
		else
		{
			Bdt_SnPingRespPackageFree(pSnPingRespPackage);
			return NULL;
		}
		break;	
	
	case BDT_DATA_GRAM_PACKAGE:
		pDataGramPackage = Bdt_DataGramPackageCreate();
		r = DecodeDataGramPackage(pDataGramPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pDataGramPackage;
		}
		else
		{
			Bdt_DataGramPackageFree(pDataGramPackage);
			return NULL;
		}
		break;	
	
	case BDT_TCP_SYN_CONNECTION_PACKAGE:
		pTcpSynConnectionPackage = Bdt_TcpSynConnectionPackageCreate();
		r = DecodeTcpSynConnectionPackage(pTcpSynConnectionPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pTcpSynConnectionPackage;
		}
		else
		{
			Bdt_TcpSynConnectionPackageFree(pTcpSynConnectionPackage);
			return NULL;
		}
		break;	
	
	case BDT_TCP_ACK_CONNECTION_PACKAGE:
		pTcpAckConnectionPackage = Bdt_TcpAckConnectionPackageCreate();
		r = DecodeTcpAckConnectionPackage(pTcpAckConnectionPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pTcpAckConnectionPackage;
		}
		else
		{
			Bdt_TcpAckConnectionPackageFree(pTcpAckConnectionPackage);
			return NULL;
		}
		break;	
	
	case BDT_TCP_ACKACK_CONNECTION_PACKAGE:
		pTcpAckAckConnectionPackage = Bdt_TcpAckAckConnectionPackageCreate();
		r = DecodeTcpAckAckConnectionPackage(pTcpAckAckConnectionPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pTcpAckAckConnectionPackage;
		}
		else
		{
			Bdt_TcpAckAckConnectionPackageFree(pTcpAckAckConnectionPackage);
			return NULL;
		}
		break;	
	
	case BDT_SESSION_DATA_PACKAGE:
		pSessionDataPackage = Bdt_SessionDataPackageCreate();
		r = DecodeSessionDataPackage(pSessionDataPackage, totallen, bufferStream, pRefPackage);
		if (r == BFX_RESULT_SUCCESS)
		{
			return (Bdt_Package*)pSessionDataPackage;
		}
		else
		{
			Bdt_SessionDataPackageFree(pSessionDataPackage);
			return NULL;
		}
		break;	
	
	}
	return NULL;
}
	
