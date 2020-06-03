#ifndef __BDT_PEERINFO_H__
#define __BDT_PEERINFO_H__
#include "../BdtCore.h"

#define BDT_PEER_INFO_FLAG_SIGNED				(1<<4) // 内部标记，调用方使用无效

uint32_t BufferWritePeerInfo(BfxBufferStream* bufferStream, const BdtPeerInfo* peerInfo, size_t* pWriteBytes);
int BufferReadPeerInfo(BfxBufferStream* bufferStream, const BdtPeerInfo** pResult);

const BdtEndpoint* BdtPeerInfoFindEndpoint(const BdtPeerInfo* peerInfo, const BdtEndpoint* endpoint);
void BdtPeerInfoSetSignature(BdtPeerInfoBuilder* peerInfoBuilder, const uint8_t* signature, size_t signSize);
void BdtPeerInfoSetCreateTime(BdtPeerInfoBuilder* peerInfoBuilder, uint64_t createTime);
BDT_API(const uint8_t*) BdtPeerInfoGetSignature(const BdtPeerInfo* peerInfo, size_t* outSize);
const char* BdtPeerInfoFormat(const BdtPeerInfo* peerInfo, char* outBuffer, size_t* inoutBufferSize);

#endif //__BDT_PEERINFO_H__