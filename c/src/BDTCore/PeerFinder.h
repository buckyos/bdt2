#ifndef __BDT_PEER_FINDER_H__
#define __BDT_PEER_FINDER_H__
#include "./BdtCore.h"
#include "./BdtSystemFramework.h"
#include "./EventCenter.h"

// trigger peer finder event by push to event center
// peer finder update a remote peer info cache
// eventParam is BdtPeerid which peer info is updated
#define BDT_PEER_FINDER_EVENT_UPDATE_CACHE	1

typedef struct BdtPeerFinderStatic
{
	const BdtPeerid* peerid;
	const BdtPeerInfo* peer;
	char color;
	struct BdtPeerFinderStatic* left;
	struct BdtPeerFinderStatic* right;
} BdtPeerFinderStatic;


typedef struct BdtPeerFinderCached
{
	const BdtPeerid* peerid;
	const BdtPeerInfo* peer;
	uint64_t expire;
	char color;
	struct BdtPeerFinderCached* left;
	struct BdtPeerFinderCached* right;
} BdtPeerFinderCached;

//TODO:从PeerFinder的功能来看，叫Finder似乎不太对
typedef struct BdtPeerFinder
{
	BdtSystemFramework* framework;
	Bdt_EventCenter* eventCenter;

	BfxRwLock localPeerLock;
	// localPeerLock protected members begin
	const BdtPeerInfo* localPeer;
	// localPeerLock protected members finish

	BfxRwLock lock;
	// lock protected members begin
	BdtPeerFinderStatic* staticEntry;
	BdtPeerFinderCached* cachedEntry;
	// lock protected members end
} BdtPeerFinder;

// not thread safe
void BdtPeerFinderInit(BdtPeerFinder* finder, BdtSystemFramework* framework, Bdt_EventCenter* eventCenter, const BdtPeerInfo* localPeer);
// not thread safe
void BdtPeerFinderUninit(BdtPeerFinder* finder);

// thread safe
uint32_t BdtPeerFinderAddStatic(BdtPeerFinder* finder, const BdtPeerInfo* peer);
// thread safe
// add ref
const BdtPeerInfo* BdtPeerFinderGetCachedOrStatic(BdtPeerFinder* finder, const BdtPeerid* peerid);
// thread safe
// add ref
const BdtPeerInfo* BdtPeerFinderGetStatic(BdtPeerFinder* finder, const BdtPeerid* peerid);
// thread safe
// add ref
const BdtPeerInfo* BdtPeerFinderGetCached(BdtPeerFinder* finder, const BdtPeerid* peerid);
// thread safe
uint32_t BdtPeerFinderAddCached(BdtPeerFinder* finder, const BdtPeerInfo* peer, uint64_t expire);

const BdtPeerInfo* BdtPeerFinderGetLocalPeer(BdtPeerFinder* finder);
uint32_t BdtPeerFinderUpdateLocalPeer(BdtPeerFinder* finder, const BdtPeerInfo* localPeer);

#endif //__BDT_PEER_FINDER_H__