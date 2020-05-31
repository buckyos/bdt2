#include <SGLib/SGLib.h>
#include "./PeerFinder.h"

typedef BdtPeerFinderStatic bdt_peer_finder_static;
static inline int bdt_peer_finder_static_compare(const bdt_peer_finder_static* left, const bdt_peer_finder_static* right)
{
	return memcmp(left->peerid, right->peerid, BDT_PEERID_LENGTH);
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(bdt_peer_finder_static, left, right, color, bdt_peer_finder_static_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(bdt_peer_finder_static, left, right, color, bdt_peer_finder_static_compare)


typedef struct BdtPeerFinderCached bdt_peer_finder_cached;
static inline int bdt_peer_finder_cached_compare(const bdt_peer_finder_cached* left, const bdt_peer_finder_cached* right)
{
	return memcmp(left->peerid, right->peerid, BDT_PEERID_LENGTH);
}
SGLIB_DEFINE_RBTREE_PROTOTYPES(bdt_peer_finder_cached, left, right, color, bdt_peer_finder_cached_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(bdt_peer_finder_cached, left, right, color, bdt_peer_finder_cached_compare)

void BdtPeerFinderInit(
	BdtPeerFinder* finder, 
	BdtSystemFramework* framework, 
	Bdt_EventCenter* eventCenter, 
	const BdtPeerInfo* localPeer)
{
	memset(finder, 0, sizeof(BdtPeerFinder));
	finder->framework = framework;
	finder->eventCenter = eventCenter;
	BdtAddRefPeerInfo(localPeer);
	finder->localPeer = localPeer;
	BfxRwLockInit(&finder->lock);
	BfxRwLockInit(&finder->localPeerLock);
}

void BdtPeerFinderUninit(BdtPeerFinder* finder)
{
	struct sglib_bdt_peer_finder_static_iterator itStatic;
	BdtPeerFinderStatic* staticNode = sglib_bdt_peer_finder_static_it_init(&itStatic, finder->staticEntry);
	for (; staticNode != NULL; staticNode = sglib_bdt_peer_finder_static_it_next(&itStatic))
	{
		BdtReleasePeerInfo(staticNode->peer);
		BFX_FREE(staticNode);
	}
	finder->staticEntry = NULL;

	struct sglib_bdt_peer_finder_cached_iterator itCached;
	BdtPeerFinderCached* cachedNode = sglib_bdt_peer_finder_cached_it_init(&itCached, finder->cachedEntry);
	for (; cachedNode != NULL; cachedNode = sglib_bdt_peer_finder_cached_it_next(&itCached))
	{
		BdtReleasePeerInfo(cachedNode->peer);
		BFX_FREE(cachedNode);
	}
	finder->cachedEntry = NULL;
	if (finder->localPeer)
	{
		BdtReleasePeerInfo(finder->localPeer);
	}
	BfxRwLockDestroy(&finder->lock);
	BfxRwLockDestroy(&finder->localPeerLock);
	finder->framework = NULL;
}

uint32_t BdtPeerFinderAddStatic(BdtPeerFinder* finder, const BdtPeerInfo* peer)
{
	BdtPeerFinderStatic* newStatic = BFX_MALLOC_OBJ(BdtPeerFinderStatic);
	newStatic->peerid = BdtPeerInfoGetPeerid(peer);
	BdtPeerFinderStatic* existStatic = NULL;

	BfxRwLockWLock(&finder->lock);
	int result = sglib_bdt_peer_finder_static_add_if_not_member(&(finder->staticEntry), newStatic, &existStatic);
	if (!result)
	{
		BfxRwLockWUnlock(&finder->lock);
		BFX_FREE(newStatic);
		return BFX_RESULT_ALREADY_EXISTS;
	}
	BdtAddRefPeerInfo(peer);
	newStatic->peer = peer;
	newStatic->peerid = BdtPeerInfoGetPeerid(newStatic->peer);
	BfxRwLockWUnlock(&finder->lock);
	return BFX_RESULT_SUCCESS;
}

const BdtPeerInfo* BdtPeerFinderGetCachedOrStatic(BdtPeerFinder* finder, const BdtPeerid* peerid)
{
	const BdtPeerInfo* remoteInfo = BdtPeerFinderGetCached(finder, peerid);
	if (!remoteInfo)
	{
		remoteInfo = BdtPeerFinderGetStatic(finder, peerid);
	}
	return remoteInfo;
}

const BdtPeerInfo* BdtPeerFinderGetStatic(BdtPeerFinder* finder, const BdtPeerid* peerid)
{
	BdtPeerFinderStatic toFind;
	toFind.peerid = peerid;
	const BdtPeerInfo* peerInfo = NULL;
	BfxRwLockRLock(&finder->lock);
	BdtPeerFinderStatic* found = sglib_bdt_peer_finder_static_find_member(finder->staticEntry, &toFind);
	if (found)
	{
		peerInfo = found->peer;
		BdtAddRefPeerInfo(peerInfo);
	}
	BfxRwLockRUnlock(&finder->lock);
	return peerInfo;
}

const BdtPeerInfo* BdtPeerFinderGetCached(BdtPeerFinder* finder, const BdtPeerid* peerid)
{
	BdtPeerFinderCached toFind = { .peerid = peerid };
	const BdtPeerInfo* peerInfo = NULL;
	BfxRwLockRLock(&finder->lock);
	BdtPeerFinderCached* found = sglib_bdt_peer_finder_cached_find_member(finder->cachedEntry, &toFind);
	if (found)
	{
		peerInfo = found->peer;
		BdtAddRefPeerInfo(peerInfo);
	}
	BfxRwLockRUnlock(&finder->lock);
	return peerInfo;
}

// thread safe
//TODO:这里是否应该用 LRU_Cache,现在看起来PeerInfoCache的大小可以是无限的
uint32_t BdtPeerFinderAddCached(BdtPeerFinder* finder, const BdtPeerInfo* peer, uint64_t expire)
{
	BdtPeerFinderCached* newCached = BFX_MALLOC_OBJ(BdtPeerFinderCached);
	BdtAddRefPeerInfo(peer);
	newCached->peerid = BdtPeerInfoGetPeerid(peer);
	newCached->peer = peer;
	const BdtPeerInfo* existPeerInfo = NULL;
	bool updated = false;
	BfxRwLockWLock(&finder->lock);
	do
	{
		BdtPeerFinderCached* existCached = NULL;
		int result = sglib_bdt_peer_finder_cached_add_if_not_member(&(finder->cachedEntry), newCached, &existCached);
		if (existCached)
		{
			if (BdtPeerInfoGetCreateTime(existCached->peer) < BdtPeerInfoGetCreateTime(peer))
			{
				break;
			}
			existPeerInfo = existCached->peer;
			existCached->peer = peer;
			existCached->peerid = BdtPeerInfoGetPeerid(peer);
			existCached->expire = expire;
		}
		updated = true;
	} while (false);
	BfxRwLockWUnlock(&finder->lock);

	if (existPeerInfo)
	{
		BdtReleasePeerInfo(existPeerInfo);
		BFX_FREE(newCached);
	}
	
	{
		char strPeerid[BDT_PEERID_STRING_LENGTH + 1] = { 0, };
		BdtPeeridToString(BdtPeerInfoGetPeerid(peer), strPeerid);
		BLOG_DEBUG("update cached peer info %s", strPeerid);
	}

	/*if (updated)
	{
		Bdt_PushPeerFinderEvent(finder->eventCenter, finder, BDT_PEER_FINDER_EVENT_UPDATE_CACHE, NULL);
	}*/
	
	return BFX_RESULT_SUCCESS;
}

const BdtPeerInfo* BdtPeerFinderGetLocalPeer(BdtPeerFinder* finder)
{
	const BdtPeerInfo* localPeer = NULL;
	BfxRwLockRLock(&finder->localPeerLock);
	localPeer = finder->localPeer;
	BdtAddRefPeerInfo(localPeer);
	BfxRwLockRUnlock(&finder->localPeerLock);
	return localPeer;
}

uint32_t BdtPeerFinderUpdateLocalPeer(BdtPeerFinder* finder, const BdtPeerInfo* localPeer)
{
	BdtAddRefPeerInfo(localPeer);
	const BdtPeerInfo* old = NULL;
	BfxRwLockWLock(&finder->localPeerLock);
	old = finder->localPeer;
	finder->localPeer = localPeer;
	BfxRwLockWUnlock(&finder->localPeerLock);
	if (!old)
	{
		BdtReleasePeerInfo(old);
		// todo: notify with event center
	}
	return BFX_RESULT_SUCCESS;
}


