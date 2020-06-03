#ifndef __BDT_GLOBAL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of gloal module"
#endif //__BDT_GLOBAL_MODULE_IMPL__
#include "../BdtCore.h"

static const char* bdtEndpointStrIpvUnk = "v?";
static const char* bdtEndpointStrIpv4 = "v4";
static const char* bdtEndpointStrIpv6 = "v6";
static const char* bdtEndpointStrPtlUnk = "unk";
static const char* bdtEndpointStrPtlTcp = "tcp";
static const char* bdtEndpointStrPtlUdp = "udp";


#include <ctype.h>

//TODO ipv6 comfortable
BDT_API(uint32_t) BdtEndpointFromString(BdtEndpoint* endpoint, const char* str)
{
	memset(endpoint, 0, sizeof(BdtEndpoint));
	int offset = 0;
	if (!memcmp(str, bdtEndpointStrIpv4, 2))
	{
		endpoint->flag = endpoint->flag | BDT_ENDPOINT_IP_VERSION_4;
	}
	else if (!memcmp(str, bdtEndpointStrIpv6, 2))
	{
		endpoint->flag = endpoint->flag | BDT_ENDPOINT_IP_VERSION_6;
	}
	offset += 2;
	if (!memcmp(str + offset, bdtEndpointStrPtlTcp, 3))
	{
		endpoint->flag = endpoint->flag | BDT_ENDPOINT_PROTOCOL_TCP;
	}
	else if (!memcmp(str + offset, bdtEndpointStrPtlUdp, 3))
	{
		endpoint->flag = endpoint->flag | BDT_ENDPOINT_PROTOCOL_UDP;
	}
	offset += 3;

	if (endpoint->flag & BDT_ENDPOINT_IP_VERSION_4)
	{
		int u8offset = 0;
		char u8[4];
		char split[4] = { '.', '.', '.', ':' };
		for (int ix = 0; ix < 4; ++ix)
		{
			memset(u8, 0, sizeof(u8));
			u8offset = 0;

			while (true)
			{
				char c = *(str + offset + u8offset);
				if (c != split[ix])
				{
					u8[u8offset] = c;
					++u8offset;
					if (u8offset > 3)
					{
						return BFX_RESULT_INVALID_FORMAT;
					}
				}
				else
				{
					break;
				}
			}
			if (!u8offset)
			{
				return BFX_RESULT_INVALID_FORMAT;
			}
			endpoint->address[ix] = atoi(u8);
			offset += u8offset + 1;
			endpoint->port = atoi(str + offset);
		}
	}
	else if (endpoint->flag & BDT_ENDPOINT_IP_VERSION_6)
	{
		if (str[offset] != '[')
		{
			return BFX_RESULT_INVALID_FORMAT;
		}
		++offset;

		
		int zeroStart = -1;
		int parts = 0;
		uint16_t address[8] = { 0, };
		do
		{
			uint8_t u16[4];

			if (str[offset] == ':')
			{
				++offset;
				// must start with ::
				if (str[offset] != ':')
				{
					return BFX_RESULT_INVALID_FORMAT;
				}
			}
			// explore parts like :abcd: or :abc] or ::
			while (true)
			{
				char c = str[offset];
				if (c == ':')
				{
					if (zeroStart == -1)
					{
						zeroStart = parts;
						offset += 1;
						continue;
					}
					else
					{
						// shouldn't contain multi ::
						return BFX_RESULT_INVALID_FORMAT;
					}
				}
				else if (c == ']')
				{
					offset += 1;
					// got end ]
					break;
				}
				int u16offset = 0;
				for (; u16offset < 4; ++u16offset)
				{
					c = str[offset + u16offset];
					if (isalpha(c))
					{
						u16[u16offset] = 10 + (uint8_t)(c - (isupper(c) ? 'A' : 'a'));
					}
					else if (isalnum(c))
					{
						u16[u16offset] = c - '0';
					}
					else
					{
						break;
					}
				}
				offset += u16offset;
				c = str[offset];
				if (c == ':'
					|| c == ']')
				{
					if (u16offset == 0)
					{
						if (zeroStart != -1)
						{
							// shouldn't contain multi ::
							return BFX_RESULT_INVALID_FORMAT;
						}
						zeroStart = parts;
						continue;
					}
					
					for (int ix = 0; ix < u16offset; ++ix)
					{
						*(address + parts) += (u16[ix] << (4 * (u16offset -1 - ix)));
					}
					++offset;
					++parts;
					if (c == ']')
					{
						break;
					}
					if (parts == 8)
					{
						// more than 8 parts
						return BFX_RESULT_INVALID_FORMAT;
					}
				}
				else
				{
					return BFX_RESULT_INVALID_FORMAT;
				}
			}
		} while (false);

		if (zeroStart == -1)
		{
			if (parts < 8)
			{
				// less than 8 parts
				return BFX_RESULT_INVALID_FORMAT;
			}
			for (int ix = 0; ix < 8; ++ix)
			{
				// change sort 
				endpoint->addressV6[ix<<1] = *((uint8_t*)(address + ix) + 1);
				endpoint->addressV6[(ix<<1) + 1] = *((uint8_t*)(address + ix));
			}
		}
		else
		{
			if (parts == 8)
			{
				// more than 8 parts
				return BFX_RESULT_INVALID_FORMAT;
			}
			
			memset(endpoint->addressV6, 0, 16);
			int iy = 0;
			int ix = 0;
			for (; ix < zeroStart; ++ix, ++iy)
			{
				endpoint->addressV6[ix << 1] = *((uint8_t*)(address + iy) + 1);
				endpoint->addressV6[(ix << 1) + 1] = *((uint8_t*)(address + iy));
			}
			for (ix = zeroStart + 8 - parts; ix < 8; ++ix, ++iy)
			{
				endpoint->addressV6[ix << 1] = *((uint8_t*)(address + iy) + 1);
				endpoint->addressV6[(ix << 1) + 1] = *((uint8_t*)(address + iy));
			}
		}

		if (str[offset] != ':')
		{
			return BFX_RESULT_INVALID_FORMAT;
		}
		++offset;
		endpoint->port = atoi(str + offset);
	}

	return BFX_RESULT_SUCCESS;
}

BDT_API(void) BdtEndpointToString(const BdtEndpoint* endpoint, char name[BDT_ENDPOINT_STRING_MAX_LENGTH + 1])
{
	memset(name, 0, BDT_ENDPOINT_STRING_MAX_LENGTH + 1);
	size_t offset = 0;
	const char* ipv = bdtEndpointStrIpvUnk;
	if (endpoint->flag & BDT_ENDPOINT_IP_VERSION_4)
	{
		ipv = bdtEndpointStrIpv4;
	}
	else if (endpoint->flag & BDT_ENDPOINT_IP_VERSION_6)
	{
		ipv = bdtEndpointStrIpv6;
	}
	
	const char* ptl = bdtEndpointStrPtlUnk;
	if (endpoint->flag & BDT_ENDPOINT_PROTOCOL_TCP)
	{
		ptl = bdtEndpointStrPtlTcp;
	}
	else if (endpoint->flag & BDT_ENDPOINT_PROTOCOL_UDP)
	{
		ptl = bdtEndpointStrPtlUdp;
	}
	
	if (endpoint->flag & BDT_ENDPOINT_IP_VERSION_4)
	{
		sprintf(name, "%s%s%u.%u.%u.%u:%u",
			ipv, ptl,
			endpoint->address[0],
			endpoint->address[1],
			endpoint->address[2],
			endpoint->address[3],
			endpoint->port);
	}
	else if (endpoint->flag & BDT_ENDPOINT_IP_VERSION_6)
	{
		uint16_t address[8];
		for (int8_t ix = 0; ix < 8; ++ix)
		{
			address[ix] = (((uint16_t)endpoint->addressV6[ix<<1]) << 8) + endpoint->addressV6[(ix<<1)+1];
		}
		
		int8_t zeroStart = -1;
		int8_t zeroEnd = zeroStart;
		for (int8_t ix = 0; ix < 8; ++ix)
		{
			if (address[ix] == 0)
			{
				zeroEnd = zeroStart = ix;
				for (; ix < 8; ++ix)
				{
					if (address[ix] == 0)
					{
						++zeroEnd;
					}
					else
					{
						break;
					}
				}
				if (zeroEnd == zeroStart)
				{
					zeroStart = -1;
					continue;
				}
				else
				{
					break;
				}
			}
		}
	
		if (zeroStart == -1)
		{
			sprintf(name, "%s%s[%x:%x:%x:%x:%x:%x:%x:%x]:%u",
				ipv, ptl,
				address[0], 
				address[1],
				address[2],
				address[3],
				address[4],
				address[5],
				address[6],
				address[7],
				endpoint->port);
		}
		else
		{
			int ni = 0;
			ni += sprintf(name + ni, "%s%s[",
				ipv, ptl);
			int8_t ix = 0;
			if (zeroStart != 0)
			{
				for (; ix < zeroStart; ++ix)
				{
					ni += sprintf(name + ni, "%x:", address[ix]);
				}
			}
			else
			{
				ni += sprintf(name + ni, ":");
			}
			
            if (zeroEnd == 8)
            {
                ni += sprintf(name + ni, ":");
            }
            else
            {
                for (ix = zeroEnd; ix < 8; ++ix)
                {
                    ni += sprintf(name + ni, ":%x", address[ix]);
                }
            }
			ni += sprintf(name + ni, "]:%u", endpoint->port);
		}
	}
}

BDT_API(bool) BdtEndpointIsSameIpVersion(const BdtEndpoint* left, const BdtEndpoint* right)
{
	return (left->flag & BDT_ENDPOINT_IP_VERSION_4
		&& right->flag & BDT_ENDPOINT_IP_VERSION_4)
		|| (left->flag & BDT_ENDPOINT_IP_VERSION_6
			&& right->flag & BDT_ENDPOINT_IP_VERSION_6);
}

#define BDT_ENDPOINT_COMPARE_FLAG (BDT_ENDPOINT_IP_VERSION_4 | \
									BDT_ENDPOINT_IP_VERSION_6 | \
									BDT_ENDPOINT_IP_VERSION_UNK | \
									BDT_ENDPOINT_PROTOCOL_TCP | \
									BDT_ENDPOINT_PROTOCOL_UDP | \
									BDT_ENDPOINT_PROTOCOL_UNK)

BDT_API(int) BdtEndpointCompare(const BdtEndpoint* left, const BdtEndpoint* right, bool ignorePort)
{
	uint8_t leftFlag = left->flag & BDT_ENDPOINT_COMPARE_FLAG;
	uint8_t rightFlag = right->flag & BDT_ENDPOINT_COMPARE_FLAG;
	if (leftFlag < rightFlag)
	{
		return -1;

	}
	if (leftFlag > rightFlag)
	{
		return 1;
	}
	int result = 0;
	if (leftFlag & BDT_ENDPOINT_IP_VERSION_4)
	{
		result = memcmp(left->address, right->address, sizeof(left->address));
	}
	else
	{
		result = memcmp(left->addressV6, right->addressV6, sizeof(left->addressV6));
	}
	
	if (result != 0)
	{
		return result;
	}
	if (ignorePort)
	{
		return result;
	}
	if (left->port == right->port)
	{
		return 0;
	}
	return left->port > right->port ? 1 : -1;
}

BDT_API(void) BdtEndpointCopy(BdtEndpoint* dest, const BdtEndpoint* src) {
    memcpy(dest, src, sizeof(BdtEndpoint));
}

BDT_API(int) BdtEndpointToSockAddr(const BdtEndpoint* ep, struct sockaddr* addr) {
    int ret = 0;
    if (ep->flag & BDT_ENDPOINT_IP_VERSION_4) {
        ((struct sockaddr_in*)addr)->sin_family = AF_INET;
        memcpy((void*)(&(((struct sockaddr_in*)addr)->sin_addr)), (void*)(ep->address), sizeof(ep->address));
        ((struct sockaddr_in*)addr)->sin_port = htons(ep->port);
    } else if (ep->flag & BDT_ENDPOINT_IP_VERSION_6) {
		memset(addr, 0, sizeof(struct sockaddr_in6));
        ((struct sockaddr_in6*)addr)->sin6_family = AF_INET6;
        memcpy((void*)(&(((struct sockaddr_in6*)addr)->sin6_addr)), (void*)(ep->addressV6), sizeof(ep->addressV6));
        ((struct sockaddr_in6*)addr)->sin6_port = htons(ep->port);
    } else {
        BLOG_ERROR("unknown endpoint flag: %d", ep->flag);
        ret = BFX_RESULT_INVALID_PARAM;
    }

    return ret;
}

BDT_API(int) SockAddrToBdtEndpoint(const struct sockaddr* addr, BdtEndpoint* ep, uint8_t protocol) {
    int ret = 0;
	memset(ep, 0, sizeof(BdtEndpoint));
    if (addr->sa_family == AF_INET) {
        ep->flag |= BDT_ENDPOINT_IP_VERSION_4;
        memcpy((void*)(ep->address), (void*)(&(((struct sockaddr_in*)addr)->sin_addr)), sizeof(((struct sockaddr_in*)addr)->sin_addr));
        ep->port = ntohs(((struct sockaddr_in*)addr)->sin_port);
    } else if (addr->sa_family == AF_INET6) {
        ep->flag |= BDT_ENDPOINT_IP_VERSION_6;
        memcpy((void*)(ep->addressV6), (void*)(&(((struct sockaddr_in6*)addr)->sin6_addr)), sizeof(((struct sockaddr_in6*)addr)->sin6_addr));
        ep->port = ntohs(((struct sockaddr_in6*)addr)->sin6_port);
    } else {
        BLOG_ERROR("unknown sockaddr family: %d", addr->sa_family);
        ret = BFX_RESULT_INVALID_PARAM;
    }

	ep->flag |= (protocol & (BDT_ENDPOINT_PROTOCOL_UNK | BDT_ENDPOINT_PROTOCOL_TCP | BDT_ENDPOINT_PROTOCOL_UDP));

    return 0;
}

BDT_API(int) BdtEndpointGetFamily(const BdtEndpoint* ep) {
    int ret;
    if (ep->flag & BDT_ENDPOINT_IP_VERSION_4) {
        ret = AF_INET;
    } else if (ep->flag & BDT_ENDPOINT_IP_VERSION_6) {
        ret = AF_INET6;
    } else {
        BLOG_ERROR("unknown sockaddr family: %d", ep->flag);
        ret = AF_UNSPEC;
    }

    return ret;
}

BDT_API(size_t) BdtEndpointGetSockaddrSize(const BdtEndpoint* ep) {
    size_t ret;
    if (ep->flag & BDT_ENDPOINT_IP_VERSION_4) {
        ret = sizeof(struct sockaddr_in);
    } else if (ep->flag & BDT_ENDPOINT_IP_VERSION_6) {
        ret = sizeof(struct sockaddr_in6);
    } else {
        BLOG_NOT_REACHED();
        ret = 0;
    }

    return ret;
}

BFX_VECTOR_DEFINE_FUNCTIONS(BdtEndpointArray, BdtEndpoint, list, size, allocSize)

BDT_API(int) BdtEndpointArrayInit(BdtEndpointArray* pArray, size_t allocSize)
{
	bfx_vector_BdtEndpointArray_init(pArray);
	if (allocSize != 0)
	{
		bfx_vector_BdtEndpointArray_reserve(pArray, allocSize);
	}
	return BFX_RESULT_SUCCESS;;
}

BDT_API(int) BdtEndpointArrayPush(BdtEndpointArray* pArray, const BdtEndpoint* pEndpoint)
{
	bfx_vector_BdtEndpointArray_push_back(pArray, *pEndpoint);
	return BFX_RESULT_SUCCESS;
}

BDT_API(int) BdtEndpointArrayUninit(BdtEndpointArray* pArray)
{
	bfx_vector_BdtEndpointArray_cleanup(pArray);
	return BFX_RESULT_SUCCESS;
}

BDT_API(bool) BdtEndpointArrayIsEqual(const BdtEndpointArray* left, const BdtEndpointArray* right)
{
	if (left->size != right->size) 
	{
		return false; 
	} 
	for (size_t i = 0; i < left->size; ++i) 
	{
		if (BdtEndpointCompare(left->list + i, right->list + i, false))
		{
			return false; 
		}
	} 
	return true; 
}

BDT_API(void) BdtEndpointArrayClone(const BdtEndpointArray* src, BdtEndpointArray* dest)
{
	bfx_vector_BdtEndpointArray_clone(src, dest);
}


BFX_VECTOR_DEFINE_FUNCTIONS(BdtStreamArray, BdtStreamRange, list, size, allocSize)

BDT_API(int) BdtStreamArrayInit(BdtStreamArray* pArray, size_t allocSize)
{
	bfx_vector_BdtStreamArray_init(pArray);
	if (allocSize != 0)
	{
		bfx_vector_BdtStreamArray_resize(pArray, allocSize);
		pArray->size = 0;
	}
	return BFX_RESULT_SUCCESS;;
}
BDT_API(int) BdtStreamArrayPush(BdtStreamArray* pArray, BdtStreamRange* pStreamRange)
{
	bfx_vector_BdtStreamArray_push_back(pArray, *pStreamRange);
	return BFX_RESULT_SUCCESS;
}
BDT_API(int) BdtStreamArrayUninit(BdtStreamArray* pArray)
{
	bfx_vector_BdtStreamArray_cleanup(pArray);
	return BFX_RESULT_SUCCESS;
}