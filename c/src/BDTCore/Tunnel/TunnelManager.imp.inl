#ifndef __BDT_TUNNEL_MANAGER_IMP_INL__
#define __BDT_TUNNEL_MANAGER_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include <SGLib/SGLib.h>
#include "./TunnelManager.inl"


typedef struct TunnelContainerMap {
	const BdtPeerid* remote;
	TunnelContainer* container;
	char color;
	struct TunnelContainerMap* left;
	struct TunnelContainerMap* right;
} TunnelContainerMap, tunnel_container_map;

static inline int tunnel_container_map_compare(const tunnel_container_map* left, const tunnel_container_map* right)
{
	return memcmp(left->remote, right->remote, BDT_PEERID_LENGTH);
}

SGLIB_DEFINE_RBTREE_PROTOTYPES(tunnel_container_map, left, right, color, tunnel_container_map_compare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(tunnel_container_map, left, right, color, tunnel_container_map_compare)


typedef struct TunnelContainerVector
{
	TunnelContainer** containers;
	size_t size;
	size_t allocSize;
} TunnelContainerVector, tunnel_container;
BFX_VECTOR_DEFINE_FUNCTIONS(tunnel_container, TunnelContainer*, containers, size, allocSize)


struct Bdt_TunnelContainerManager
{
	BdtStack* stack;
	const BdtTunnelConfig* config;
	BfxRwLock mapLock;
	// mapLock proctect members begin
	struct TunnelContainerMap* containerEntry;
	// mapLock proctect members end
};



static void TunnelContainerManagerInit(TunnelContainerManager* tunnelManager, BdtStack* stack)
{
	memset(tunnelManager, 0, sizeof(TunnelContainerManager));
	tunnelManager->stack = stack;
	tunnelManager->config = &BdtStackGetConfig(stack)->tunnel;
	BfxRwLockInit(&tunnelManager->mapLock);
}

static void TunnelContainerManagerUninit(TunnelContainerManager* tunnelManager)
{
	struct sglib_tunnel_container_map_iterator it;
	TunnelContainerMap* node = sglib_tunnel_container_map_it_init(&it, tunnelManager->containerEntry);
	for (; node != NULL; node = sglib_tunnel_container_map_it_next(&it))
	{
		Bdt_TunnelContainerRelease(node->container);
		BFX_FREE(node);
	}
	tunnelManager->containerEntry = NULL;
	BfxRwLockDestroy(&tunnelManager->mapLock);
	memset(tunnelManager, 0, sizeof(TunnelContainerManager));
}

Bdt_TunnelContainerManager* Bdt_TunnelContainerManagerCreate(BdtStack* stack)
{
	TunnelContainerManager* tunnelManager = BFX_MALLOC_OBJ(TunnelContainerManager);
	TunnelContainerManagerInit(tunnelManager, stack);
	return tunnelManager;
}

void Bdt_TunnelContainerManagerDestroy(Bdt_TunnelContainerManager* tunnelManager)
{
	TunnelContainerManagerUninit(tunnelManager);
	BfxFree(tunnelManager);
}

Bdt_TunnelContainer* BdtTunnel_GetContainerByRemotePeerid(
	TunnelContainerManager* tunnelManager,
	const BdtPeerid* remote
)
{
	TunnelContainerMap toFind = {.remote = remote};
	TunnelContainerMap* entry = tunnelManager->containerEntry;
	TunnelContainer* container = NULL;

	BfxRwLockRLock(&tunnelManager->mapLock);
	TunnelContainerMap* found = sglib_tunnel_container_map_find_member(entry, &toFind);
	container = found ? found->container : NULL;
	if (container)
	{
		Bdt_TunnelContainerAddRef(container);
	}
	BfxRwLockRUnlock(&tunnelManager->mapLock);

	return container;
}

Bdt_TunnelContainer* BdtTunnel_CreateContainer(
	TunnelContainerManager* tunnelManager,
	const BdtPeerid* remote
)
{
	TunnelContainerMap toFind = { .remote = remote };
	TunnelContainerMap** pEntry = &tunnelManager->containerEntry;
	TunnelContainer* container = NULL;


	BfxRwLockWLock(&tunnelManager->mapLock);
	TunnelContainerMap* found = sglib_tunnel_container_map_find_member(*pEntry, &toFind);
	if (!found)
	{
		
		container = TunnelContainerCreate(
			BdtStackGetFramework(tunnelManager->stack),
			BdtStackGetTls(tunnelManager->stack),
			BdtStackGetLocalPeerid(tunnelManager->stack),
			BdtStackGetSecret(tunnelManager->stack), 
			BdtStackGetNetManager(tunnelManager->stack), 
			BdtStackGetEventCenter(tunnelManager->stack), 
			BdtStackGetPeerFinder(tunnelManager->stack),
			BdtStackGetKeyManager(tunnelManager->stack), 
			BdtStackGetSnClient(tunnelManager->stack), 
			BdtStackGetConnectionManager(tunnelManager->stack), 
            BdtStackGetHistoryPeerUpdater(tunnelManager->stack),
			remote, 
			tunnelManager->config);
		found = BFX_MALLOC_OBJ(TunnelContainerMap);
		found->container = container;
		found->remote = BdtTunnel_GetRemotePeerid(container);
		sglib_tunnel_container_map_add(pEntry, found);
	}
	else
	{
		container = found->container;
	}
	Bdt_TunnelContainerAddRef(container);
	BfxRwLockWUnlock(&tunnelManager->mapLock);
	
	return container;
}

static void TunnelContainerManagerRemoveTunnel(
	TunnelContainerManager* tunnelManager,
	const BdtPeerid* remote)
{
	TunnelContainerMap toFind = { .remote = remote };
	TunnelContainerMap** pEntry = &tunnelManager->containerEntry;
	TunnelContainerMap* del = NULL;
	TunnelContainer* container = NULL;
	BfxRwLockWLock(&tunnelManager->mapLock);
	int result = sglib_tunnel_container_map_delete_if_member(pEntry, &toFind, &del);
	if (result)
	{
		container = del->container;
		BFX_FREE(del);
	}
	BfxRwLockWUnlock(&tunnelManager->mapLock);
	if (container)
	{
		Bdt_TunnelContainerRelease(del->container);
	}
}

#endif //__BDT_TUNNEL_MANAGER_IMP_INL__
