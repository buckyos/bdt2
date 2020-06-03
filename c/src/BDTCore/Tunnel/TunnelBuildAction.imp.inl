#ifndef __BDT_TUNNEL_BUILD_ACTION_IMP_INL__
#define __BDT_TUNNEL_BUILD_ACTION_IMP_INL__
#ifndef __BDT_TUNNEL_MODULE_IMPL__
#error "should only include in inl, impl.inl, Module.c of tunnel module"
#endif //__BDT_TUNNEL_MODULE_IMPL__

#include "./TunnelBuildAction.inl"
#include "./TunnelBuilder.inl"
#include <SGLib/SGLib.h>

static const BdtEndpoint* BuildActionGetPackageConnectionEndpoint()
{
	static BdtEndpoint endpoint = { 
		.flag = BDT_ENDPOINT_IP_VERSION_4 | BDT_ENDPOINT_PROTOCOL_UDP, 
		.reserve = 0, 
		.port = 0, 
		.addressV6 = {0, } };
	return &endpoint;
}

typedef struct BuildingTunnelMap
{
	struct BuildingTunnelMap* left;
	struct BuildingTunnelMap* right;
	uint8_t color;
	BdtEndpoint local;
	BdtEndpoint remote;
	bool actived;
	BuildAction* action;
} BuildingTunnelMap, building_tunnel_map;


static int BuildingTunnelMapCompare(const BuildingTunnelMap* left, const BuildingTunnelMap* right)
{
	int result = BdtEndpointCompare(&left->local, &right->local, false);
	if (result)
	{
		return result;
	}
	return BdtEndpointCompare(&left->remote, &right->remote, false);
}

SGLIB_DEFINE_RBTREE_PROTOTYPES(building_tunnel_map, left, right, color, BuildingTunnelMapCompare)
SGLIB_DEFINE_RBTREE_FUNCTIONS(building_tunnel_map, left, right, color, BuildingTunnelMapCompare)

typedef struct BuildingTunnelVector
{
	size_t size;
	size_t allocSize;
	BuildAction** actions;
} BuildingTunnelVector, building_tunnels;
BFX_VECTOR_DECLARE_FUNCTIONS(building_tunnels, BuildAction*, actions, size, allocSize)
BFX_VECTOR_DEFINE_FUNCTIONS(building_tunnels, BuildAction*, actions, size, allocSize)


struct BuildActionRuntime
{
	int32_t refCount;
	TunnelBuilder* builder;

	BfxRwLock remoteInfoLock;
	// remoteInfoLock protected members begin
	const BdtPeerInfo* remoteInfo;
	// remoteInfoLock protected members end

	BfxRwLock buildingTunnelsLock;
	// buildingTunnelsLock protected members begin
	bool finished;
	BuildingTunnelMap* buildingTunnels;
	// buildingTunnelsLock protected members end

	BuildActionRuntimeOnBuildingTunnelActive onBuildingTunnelActive;
	BuildActionRuntimeOnBuildingTunnelFinish onBuildingTunnelFinish;
	BfxUserData userData;
};

static void BuildActionRuntimeUpdateRemoteInfo(
	BuildActionRuntime* runtime,
	const BdtPeerInfo* remoteInfo)
{
	const BdtPeerInfo* oldInfo = NULL;
	bool updated = false;
	BfxRwLockWLock(&runtime->remoteInfoLock);
	// tofix: compare remote info timestampe 
	do
	{
		if (runtime->remoteInfo
			&& BdtPeerInfoGetCreateTime(runtime->remoteInfo) < BdtPeerInfoGetCreateTime(remoteInfo))
		{
			break;
		}
		updated = true;
		oldInfo = runtime->remoteInfo;
		runtime->remoteInfo = remoteInfo;
	} while (false);
	BfxRwLockWUnlock(&runtime->remoteInfoLock);

	if (updated)
	{
		BLOG_DEBUG("%s %u update runtime remote peer info", Bdt_TunnelContainerGetName(runtime->builder->container), runtime->builder->seq);
		BdtAddRefPeerInfo(remoteInfo);
	}
	
	if (oldInfo)
	{
		BdtReleasePeerInfo(oldInfo);
	}
}

static const BdtPeerInfo* BuildActionRuntimeGetRemoteInfo(BuildActionRuntime* runtime)
{
	const BdtPeerInfo* remoteInfo = NULL;
	BfxRwLockRLock(&runtime->remoteInfoLock);
	remoteInfo = runtime->remoteInfo;
	if (remoteInfo)
	{
		BdtAddRefPeerInfo(remoteInfo);
	}
	BfxRwLockRUnlock(&runtime->remoteInfoLock);
	return remoteInfo;
}

static BuildAction* BuildActionRuntimeGetBuildingTunnelAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote)
{
	BuildingTunnelMap toFind;
	toFind.local = *local;
	toFind.remote = *remote;
	
	BuildAction* action = NULL;
	BuildingTunnelMap* found = NULL;

	BfxRwLockRLock(&runtime->buildingTunnelsLock);
	found = sglib_building_tunnel_map_find_member(runtime->buildingTunnels, &toFind);
	if (found)
	{
		action = found->action;
		BuildActionAddRef(action);
	}
	BfxRwLockRUnlock(&runtime->buildingTunnelsLock);

	return action;
}

static uint32_t BuildActionRuntimeAddBuildingTunnelAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action, 
	BuildActionCallbacks* callbacks, 
	bool actived)
{
	BuildingTunnelMap* add = BFX_MALLOC_OBJ(BuildingTunnelMap);
	add->action = action;
	add->local = *local;
	add->remote = *remote;
	add->actived = actived;
	BuildingTunnelMap* exists = NULL;

	bool finished = false;

	BfxRwLockWLock(&runtime->buildingTunnelsLock);
	do
	{
		if (runtime->finished)
		{
			finished = true;
			break;
		}
		sglib_building_tunnel_map_add_if_not_member(&runtime->buildingTunnels, add, &exists);
	} while (false);
	BfxRwLockWUnlock(&runtime->buildingTunnelsLock);

	if (finished)
	{
		BLOG_DEBUG("ignore %s for runtime finished", BuildActionGetName(action));
		BFX_FREE(add);
		return BFX_RESULT_INVALID_STATE;
	}
	if (exists)
	{
		BLOG_DEBUG("ignore %s for action exists", BuildActionGetName(action));
		BFX_FREE(add);
		return BFX_RESULT_ALREADY_EXISTS;
	}
	BuildActionAddRef(action);

	BuildActionExecute((BuildAction*)action, callbacks);
	
	if (actived)
	{
		BLOG_DEBUG("%s actived", BuildActionGetName(action));
		BuildActionAddRef(action);
		runtime->onBuildingTunnelActive(runtime, local, remote, action, runtime->userData.userData);
		BuildActionRelease(action);
	}

	// must check first resp cache after add to buildingTunnels!!!
	if (TunnelBuilderIsPassive(runtime->builder))
	{
		const Bdt_PackageWithRef* firstResp = TunnelContainerGetCachedResponse(
			runtime->builder->container, 
			TunnelBuilderGetSeq(runtime->builder));
		if (firstResp)
		{
			BLOG_DEBUG("confirm %s for response cache hits", BuildActionGetName(action));
			PassiveBuildActionConfirm((PassiveBuildAction*)action, firstResp);
			Bdt_PackageRelease(firstResp);
		}
	}
	return BFX_RESULT_SUCCESS;
}

static void BuildActionRuntimeActiveBuildingTunnel(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action
)
{
	BuildingTunnelMap toFind;
	toFind.local = *local;
	toFind.remote = *remote;
	bool fireActived = false;
	BuildingTunnelMap* found = NULL;
	BfxRwLockRLock(&runtime->buildingTunnelsLock);
	found = sglib_building_tunnel_map_find_member(runtime->buildingTunnels, &toFind);
	if (found && !found->actived && action == found->action)
	{
		found->actived = true;
		fireActived = true;
	}
	BfxRwLockRUnlock(&runtime->buildingTunnelsLock);

	if (fireActived)
	{
		BLOG_DEBUG("%s actived", BuildActionGetName(action));
		runtime->onBuildingTunnelActive(runtime, local, remote, action, runtime->userData.userData);
	}
}

static void BuildActionRuntimeFinishBuildingTunnel(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote,
	BuildAction* action,
	uint32_t result
)
{
	BLOG_DEBUG("%s finished with result %u", BuildActionGetName(action), result);
	runtime->onBuildingTunnelFinish(runtime, local, remote, action, result, runtime->userData.userData);
}

struct BuildingTunnelActionIterator
{
	BuildActionRuntime* runtime;
	struct sglib_building_tunnel_map_iterator iter;
};

BuildAction* BuildActionRuntimeEnumBuildingTunnelAction(
	BuildActionRuntime* runtime,
	BuildingTunnelActionIterator** outIter)
{
	BuildingTunnelActionIterator* iter = BFX_MALLOC_OBJ(BuildingTunnelActionIterator);
	iter->runtime = runtime;
	BuildActionRuntimeAddRef(runtime);
	*outIter = iter;
	BfxRwLockRLock(&runtime->buildingTunnelsLock);
	BuildingTunnelMap* node = sglib_building_tunnel_map_it_init(&iter->iter, runtime->buildingTunnels);
	return node ? node->action : NULL;
}

BuildAction* BuildActionRuntimeEnumBuildingTunnelActionNext(
	BuildingTunnelActionIterator* iter)
{
	BuildingTunnelMap* node = sglib_building_tunnel_map_it_next(&iter->iter);
	return node ? node->action : NULL;
}

void BuildActionRuntimeEnumBuildingTunnelActionFinish(
	BuildingTunnelActionIterator* iter)
{
	BfxRwLockRUnlock(&iter->runtime->buildingTunnelsLock);
	BuildActionRuntimeRelease(iter->runtime);
	BfxFree(iter);
}



static BuildActionRuntime* BuildActionRuntimeCreate(
	TunnelBuilder* builder,
	const BdtPeerInfo* remoteInfo, 
	BuildActionRuntimeOnBuildingTunnelActive onBuildingTunnelActive, 
	BuildActionRuntimeOnBuildingTunnelFinish onBuildingTunnelFinish, 
	const BfxUserData* userData)
{
	BuildActionRuntime* runtime = BFX_MALLOC_OBJ(BuildActionRuntime);
	memset(runtime, 0, sizeof(BuildActionRuntime));


	runtime->builder = builder;
	Bdt_TunnelBuilderAddRef(builder);

	runtime->onBuildingTunnelActive = onBuildingTunnelActive;
	runtime->onBuildingTunnelFinish = onBuildingTunnelFinish;
	runtime->userData = *userData;
	if (userData->lpfnAddRefUserData)
	{
		userData->lpfnAddRefUserData(userData->userData);
	}

	runtime->remoteInfo = remoteInfo;
	if (remoteInfo)
	{
		BdtAddRefPeerInfo(remoteInfo);
	}
	BfxRwLockInit(&runtime->remoteInfoLock);
	BfxRwLockInit(&runtime->buildingTunnelsLock);
	runtime->finished = false;

	runtime->refCount = 1;

	return runtime;
}


static void BuildActionRuntimeFinishBuilder(BuildActionRuntime* runtime)
{
	uint32_t result = TunnelBuilderFinishWithRuntime(
		runtime->builder,
		runtime);
	if (result == BFX_RESULT_SUCCESS)
	{
		TunnelContainerResetBuilder(
			runtime->builder->container,
			runtime->builder);
	}
}


static void BuildActionRuntimeOnBegin(BuildActionRuntime* runtime)
{
	// for builder may release runtime before runtime finished, add ref it with begin
	BuildActionRuntimeAddRef(runtime);
}

static void BuildActionRuntimeOnFinish(BuildActionRuntime* runtime)
{
	bool finished = false;
	BuildingTunnelVector actions;
	bfx_vector_building_tunnels_init(&actions);
	struct sglib_building_tunnel_map_iterator iter;

	BfxRwLockWLock(&runtime->buildingTunnelsLock);
	do
	{
		if (runtime->finished)
		{
			break;
		}
		finished = runtime->finished = true;
		BuildingTunnelMap* node = sglib_building_tunnel_map_it_init(&iter, runtime->buildingTunnels);
		while (node)
		{
			bfx_vector_building_tunnels_push_back(&actions, node->action);
			BuildingTunnelMap* prenode = node;
			node = sglib_building_tunnel_map_it_next(&iter);
			BfxFree(prenode);
		}
	} while (false);
	BfxRwLockWUnlock(&runtime->buildingTunnelsLock);
	
	if (finished)
	{
		for (size_t ix = 0; ix < actions.size; ++ix)
		{
			BuildActionCancel(actions.actions[ix]);
			BuildActionRelease(actions.actions[ix]);
		}
		bfx_vector_building_tunnels_cleanup(&actions);

		// match BuildActionRuntimeOnBegin
		BuildActionRuntimeRelease(runtime);
		BLOG_DEBUG("runtime finished");
	}
}

static int32_t BuildActionRuntimeAddRef(BuildActionRuntime* runtime)
{
	return BfxAtomicInc32(&runtime->refCount);
}

static int32_t BuildActionRuntimeRelease(BuildActionRuntime* runtime)
{
	int32_t refCount = BfxAtomicDec32(&runtime->refCount);
	if (refCount <= 0)
	{
		if (runtime->remoteInfo)
		{
			BdtReleasePeerInfo(runtime->remoteInfo);
			runtime->remoteInfo = NULL;
		}
		BfxRwLockDestroy(&runtime->remoteInfoLock);
		Bdt_TunnelBuilderRelease(runtime->builder);
		runtime->builder = NULL;

		if (runtime->userData.lpfnReleaseUserData)
		{
			runtime->userData.lpfnReleaseUserData(runtime->userData.userData);
		}
	}
	return refCount;
}


static void BuildActionCallbacksClone(BuildActionCallbacks* callbacks, BuildActionCallbacks* proto)
{
	callbacks->flags = 0;
	if (proto->flags & BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION)
	{
		callbacks->flags |= BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION;
		callbacks->resolve.function = proto->resolve.function;
	}
	else if (proto->flags & BUILD_ACTION_CALLBACK_RESOLVE_ACTION)
	{
		callbacks->flags |= BUILD_ACTION_CALLBACK_RESOLVE_ACTION;
		callbacks->resolve.action = proto->resolve.action;
		BuildActionAddRef(proto->resolve.action.action);
		callbacks->resolve.action.callbacks = BFX_MALLOC_OBJ(BuildActionCallbacks);
		BuildActionCallbacksClone(callbacks->resolve.action.callbacks, proto->resolve.action.callbacks);
	}
	if (proto->flags & BUILD_ACTION_CALLBACK_REJECT_FUNCTION)
	{
		callbacks->flags |= BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
		callbacks->reject.function = proto->reject.function;
	}
	else if (proto->flags & BUILD_ACTION_CALLBACK_REJECT_ACTION)
	{
		callbacks->flags |= BUILD_ACTION_CALLBACK_REJECT_ACTION;
		callbacks->reject.action = proto->reject.action;
		BuildActionAddRef(proto->reject.action.action);
		callbacks->reject.action.callbacks = BFX_MALLOC_OBJ(BuildActionCallbacks);
		BuildActionCallbacksClone(callbacks->reject.action.callbacks, proto->reject.action.callbacks);
	}
}

static void BuildActionCallbacksUninit(BuildActionCallbacks* callbacks)
{
	if (callbacks->flags & BUILD_ACTION_CALLBACK_RESOLVE_ACTION)
	{
		BuildActionRelease(callbacks->resolve.action.action);
		BuildActionCallbacksUninit(callbacks->resolve.action.callbacks);
		BfxFree(callbacks->resolve.action.callbacks);
	}
	if (callbacks->flags & BUILD_ACTION_CALLBACK_REJECT_ACTION)
	{
		BuildActionRelease(callbacks->reject.action.action);
		BuildActionCallbacksUninit(callbacks->reject.action.callbacks);
		BfxFree(callbacks->reject.action.callbacks);
	}
	callbacks->flags = 0;
}

typedef struct BuildActionWithCallbacks
{
	int32_t refCount;
	BuildAction* action;
	BuildActionCallbacks callbacks;
} BuildActionWithCallbacks;

static void BFX_STDCALL BuildActionWithCallbacksAddRef(void* userData)
{
	BuildActionWithCallbacks* ac = (BuildActionWithCallbacks*)userData;
	BfxAtomicInc32(&ac->refCount);
}

static void BFX_STDCALL BuildActionWithCallbacksRelease(void* userData)
{
	BuildActionWithCallbacks* ac = (BuildActionWithCallbacks*)userData;
	int32_t refCount = BfxAtomicDec32(&ac->refCount);
	if (refCount <= 0)
	{
		BuildActionCallbacksUninit(&ac->callbacks);
		BuildActionRelease(ac->action);
		BfxFree(ac);
	}
}

static void BuildActionWithCallbacksAsUserData(
	BuildAction* action, 
	BuildActionCallbacks* callbacks, 
	BfxUserData* outUserData)
{
	BuildActionWithCallbacks* ac = BFX_MALLOC_OBJ(BuildActionWithCallbacks);
	ac->action = action;
	BuildActionAddRef(action);
	BuildActionCallbacksClone(&ac->callbacks, callbacks);
	ac->refCount = 0;
	outUserData->userData = ac;
	outUserData->lpfnAddRefUserData = BuildActionWithCallbacksAddRef;
	outUserData->lpfnReleaseUserData = BuildActionWithCallbacksRelease;
}

struct BuildAction
{
	DEFINE_BUILD_ACTION()
};

static int32_t BuildActionAddRef(BuildAction* action)
{
	return BfxAtomicInc32(&action->refCount);
}

static int32_t BuildActionRelease(BuildAction* action)
{
	int32_t refCount = BfxAtomicDec32(&action->refCount);
	if (refCount <= 0)
	{
		action->uninit(action);
		BuildActionRuntimeRelease(action->runtime);
		action->runtime = NULL;
		BfxFree(action);
	}
	return refCount;
}

static BuildActionRuntime* BuildActionGetRuntime(BuildAction* action)
{
	return action->runtime;
}

static void BuildActionInit(BuildAction* action, BuildActionRuntime* runtime)
{
	memset(action, 0, sizeof(BuildAction));
	action->refCount = 1;
	action->state = BUILD_ACTION_STATE_NONE;
	action->runtime = runtime;
	BuildActionRuntimeAddRef(runtime);	
}

static const char* BuildActionGetName(BuildAction* action)
{
	return action->name;
}

static void BuildActionSetResolveFunction(
	BuildAction* action,
	BuildActionOnResolve resolve,
	BfxUserData* userData)
{
	action->resolve = resolve;
	if (userData)
	{
		action->resolveUserData = *userData;
		if (action->resolveUserData.lpfnAddRefUserData)
		{
			action->resolveUserData.lpfnAddRefUserData(action->resolveUserData.userData);
		}
	}
}


static void BuildActionSetRejectFunction(
	BuildAction* action,
	BuildActionOnReject reject,
	BfxUserData* userData)
{
	action->reject = reject;
	if (userData)
	{
		action->rejectUserData = *userData;
		if (action->rejectUserData.lpfnAddRefUserData)
		{
			action->rejectUserData.lpfnAddRefUserData(action->rejectUserData.userData);
		}
	}
}

static void BFX_STDCALL BuildActionAsUserDataAddRef(void* userData)
{
	BuildActionAddRef((BuildAction*)userData);
}

static void BFX_STDCALL BuildActionAsUserDataRelease(void* userData)
{
	BuildActionRelease((BuildAction*)userData);
}

static void BuildActionAsUserData(BuildAction* action, BfxUserData* outUserData)
{
	outUserData->userData = action;
	outUserData->lpfnAddRefUserData = BuildActionAsUserDataAddRef;
	outUserData->lpfnReleaseUserData = BuildActionAsUserDataRelease;
}

static void BuildActionTailCallbackResolve(BuildAction* action, void* userData)
{
	BuildAction* owner = (BuildAction*)userData;
	BuildActionResolve(owner);
}

static void BuildActionTailCallbackReject(BuildAction* action, void* userData, uint32_t error)
{
	BuildAction* owner = (BuildAction*)userData;
	BuildActionReject(owner, error);
}

static void BuildActionAsCallbacks(BuildAction* action, BuildActionCallbacks* callbacks)
{
	callbacks->flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks->resolve.function.function = BuildActionTailCallbackResolve;
	BuildActionAsUserData(action, &callbacks->resolve.function.userData);
	callbacks->reject.function.function = BuildActionTailCallbackReject;
	BuildActionAsUserData(action, &callbacks->reject.function.userData);
}

static uint32_t BuildActionBeginExecute(
	BuildAction* action,
	uint32_t preResult,
	BuildActionCallbacks* callback,
	BuildActionState oldState);

static void BuildActionWithCallbacksAsResolveFunction(BuildAction* action, void* userData)
{
	BuildActionWithCallbacks* ac = (BuildActionWithCallbacks*)userData;
	BuildActionBeginExecute(ac->action, BFX_RESULT_SUCCESS, &ac->callbacks, BUILD_ACTION_STATE_NONE);
}

static void BuildActionWithCallbacksAsRejectFunction(BuildAction* action, void* userData, uint32_t error)
{
	BuildActionWithCallbacks* ac = (BuildActionWithCallbacks*)userData;
	BuildActionBeginExecute(ac->action, error, &ac->callbacks, BUILD_ACTION_STATE_NONE);
}

static void BuildActionSetResolveAction(
	BuildAction* action,
	BuildAction* resolveAction, 
	BuildActionCallbacks* callbacks
)
{
	BfxUserData userData;
	BuildActionWithCallbacksAsUserData(resolveAction, callbacks, &userData);
	BuildActionSetResolveFunction(action, BuildActionWithCallbacksAsResolveFunction, &userData);
}

static void BuildActionSetRejectAction(
	BuildAction* action,
	BuildAction* rejectAction,
	BuildActionCallbacks* callbacks
)
{
	BfxUserData userData;
	BuildActionWithCallbacksAsUserData(rejectAction, callbacks, &userData);
	BuildActionSetRejectFunction(action, BuildActionWithCallbacksAsRejectFunction, &userData);
}


static void BuildActionFinishExecute(BuildAction* action)
{
	if (action->resolveUserData.lpfnReleaseUserData)
	{
		action->resolveUserData.lpfnReleaseUserData(action->resolveUserData.userData);
	}
	if (action->rejectUserData.lpfnReleaseUserData)
	{
		action->rejectUserData.lpfnReleaseUserData(action->rejectUserData.userData);
	}
	BuildActionRelease(action);
}

static uint32_t BuildActionBeginExecute(
	BuildAction* action, 
	uint32_t preResult, 
	BuildActionCallbacks* callback, 
	BuildActionState oldState)
{
	BuildActionAddRef(action);
	if (oldState != BfxAtomicCompareAndSwap32(
		(int32_t*)& action->state,
		oldState,
		BUILD_ACTION_STATE_EXECUTING))
	{
		BLOG_DEBUG("%s ignore excute for not in state %u", BuildActionGetName(action), oldState);
		BuildActionRelease(action);
		return BFX_RESULT_INVALID_STATE;
	}
	if (callback->flags & BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION)
	{
		BuildActionSetResolveFunction(action, callback->resolve.function.function, &callback->resolve.function.userData);
	}
	else if (callback->flags & BUILD_ACTION_CALLBACK_RESOLVE_ACTION)
	{
		BuildActionSetResolveAction(action, callback->resolve.action.action, callback->resolve.action.callbacks);
	}
	if (callback->flags & BUILD_ACTION_CALLBACK_REJECT_FUNCTION)
	{
		BuildActionSetRejectFunction(action, callback->reject.function.function, &callback->reject.function.userData);
	}
	else if (callback->flags & BUILD_ACTION_CALLBACK_REJECT_ACTION)
	{
		BuildActionSetRejectAction(action, callback->reject.action.action, callback->reject.action.callbacks);
	}
	BLOG_DEBUG("excute %s", BuildActionGetName(action));
	action->execute(action, preResult);

	oldState = BfxAtomicCompareAndSwap32(
		(int32_t*)&action->state,
		BUILD_ACTION_STATE_EXECUTING,
		BUILD_ACTION_STATE_EXECUTED);
	if (oldState != BUILD_ACTION_STATE_EXECUTING)
	{
		if (oldState == BUILD_ACTION_STATE_RESOLVED)
		{
			if (action->resolve)
			{
				action->resolve(action, action->resolveUserData.userData);
			}
		}
		else if (oldState == BUILD_ACTION_STATE_REJECTED)
		{
			if (action->reject)
			{
				action->reject(action, action->rejectUserData.userData, BFX_RESULT_FAILED/*tofix: how to pass error*/);
			}
		}
		else if (oldState == BUILD_ACTION_STATE_CANCELED)
		{
			if (action->onCanceled)
			{
				action->onCanceled(action);
			}
		}
		BuildActionFinishExecute(action);
	}
	
	return BFX_RESULT_SUCCESS;
}




static uint32_t BuildActionExecute(BuildAction* action, BuildActionCallbacks* callback)
{
	return BuildActionBeginExecute(action, BFX_RESULT_SUCCESS, callback, BUILD_ACTION_STATE_NONE);
}

static uint32_t BuildActionCancelImpl(BuildAction* action, BuildActionState oldState)
{
	bool finishExecute = false;
	if (BUILD_ACTION_STATE_EXECUTING == BfxAtomicCompareAndSwap32(
		(int32_t*)&action->state,
		BUILD_ACTION_STATE_EXECUTING,
		BUILD_ACTION_STATE_CANCELED))
	{
		BLOG_DEBUG("wait cancel %s for in executing", BuildActionGetName(action));
		return BFX_RESULT_SUCCESS;
	}

	if (oldState != BfxAtomicCompareAndSwap32(
		(int32_t*)&action->state,
		oldState,
		BUILD_ACTION_STATE_CANCELED))
	{
		if (BUILD_ACTION_STATE_EXECUTED == BfxAtomicCompareAndSwap32(
			(int32_t*)&action->state,
			BUILD_ACTION_STATE_EXECUTED,
			BUILD_ACTION_STATE_CANCELED))
		{
			finishExecute = true;
		}
		else
		{
			BLOG_DEBUG("ignore cancel %s for not in state %u or executed", BuildActionGetName(action), oldState);
			return BFX_RESULT_INVALID_STATE;
		}
	}

	if (action->onCanceled)
	{
		action->onCanceled(action);
	}
	if (finishExecute)
	{
		BuildActionFinishExecute(action);
	}
	BLOG_DEBUG("%s canceled", BuildActionGetName(action));
	return BFX_RESULT_SUCCESS;
}

static uint32_t BuildActionCancel(BuildAction* action)
{
	return BuildActionCancelImpl(action, BUILD_ACTION_STATE_NONE);
}


static void BuildActionResolve(BuildAction* action)
{
	if (BUILD_ACTION_STATE_EXECUTING == BfxAtomicCompareAndSwap32(
		(int32_t*)&action->state,
		BUILD_ACTION_STATE_EXECUTING,
		BUILD_ACTION_STATE_CANCELED))
	{
		BLOG_DEBUG("wait resolve %s for in executing", BuildActionGetName(action));
		return;
	}

	if (BUILD_ACTION_STATE_EXECUTED != BfxAtomicCompareAndSwap32(
		(int32_t*)& action->state,
		BUILD_ACTION_STATE_EXECUTED,
		BUILD_ACTION_STATE_RESOLVED))
	{
		BLOG_DEBUG("ignore resolve %s for not executed", BuildActionGetName(action));
		return;
	}

	if (action->resolve)
	{
		action->resolve(action, action->resolveUserData.userData);
	}
	BuildActionFinishExecute(action);
	BLOG_DEBUG("%s resolved", BuildActionGetName(action));
}

static void BuildActionReject(BuildAction* action, uint32_t error)
{
	if (BUILD_ACTION_STATE_EXECUTING == BfxAtomicCompareAndSwap32(
		(int32_t*)&action->state,
		BUILD_ACTION_STATE_EXECUTING,
		BUILD_ACTION_STATE_CANCELED))
	{
		BLOG_DEBUG("wait reject %s for in executing", BuildActionGetName(action));
		return;
	}

	if (BUILD_ACTION_STATE_EXECUTED != BfxAtomicCompareAndSwap32(
		(int32_t*)&action->state,
		BUILD_ACTION_STATE_EXECUTED,
		BUILD_ACTION_STATE_RESOLVED))
	{
		BLOG_DEBUG("ignore reject %s for not executed", BuildActionGetName(action));
		return;
	}

	if (action->reject)
	{
		action->reject(action, action->rejectUserData.userData, error);
	}
	BuildActionFinishExecute(action);
	BLOG_DEBUG("%s rejected", BuildActionGetName(action));
}


uint32_t PassiveBuildActionConfirm(PassiveBuildAction* action, const Bdt_PackageWithRef* firstResp)
{
	uint32_t result = action->confirm(action, firstResp);
	BLOG_DEBUG("%s confirmed", BuildActionGetName((BuildAction*)action));
	return result;
}

typedef struct BuildActionListItem
{
	BfxListItem base;
	BuildAction* action;
} BuildActionListItem;


struct BuildActionList
{
	int32_t refCount;
	BfxThreadLock actionsLock;
	// actionsLock protected members begin
	BfxList actions;
	// actionsLock protected members end
};

static BuildActionList* BuildActionListCreate()
{
	BuildActionList* list = BFX_MALLOC_OBJ(BuildActionList);
	memset(list, 0, sizeof(BuildActionList));
	list->refCount = 1;
	BfxListInit(&list->actions);
	BfxThreadLockInit(&list->actionsLock);
	return list;
}

static int32_t BuildActionListAddRef(BuildActionList* list)
{
	return BfxAtomicInc32(&list->refCount);
}

static int32_t BuildActionListRelease(BuildActionList* list)
{
	int32_t refCount = BfxAtomicDec32(&list->refCount);
	if (refCount <= 0)
	{
		BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&list->actions);
		while (item)
		{
			BuildActionCancel(item->action);
			BuildActionRelease(item->action);
			BfxListItem* preItem = (BfxListItem*)item;
			item = (BuildActionListItem*)BfxListNext(&list->actions, (BfxListItem*)item);
			BfxFree(preItem);
		}
		BfxThreadLockDestroy(&list->actionsLock);
	}
	return refCount;
}

static void BuildActionListAddAction(BuildActionList* list, BuildAction* action)
{
	BuildActionListItem* item = BFX_MALLOC_OBJ(BuildActionListItem);
	memset(item, 0, sizeof(BuildActionListItem));
	item->action = action;
	BuildActionAddRef(action);
	BfxThreadLockLock(&list->actionsLock);
	do
	{
		BfxListPushBack(&list->actions, (BfxListItem*)item);
	} while (false);
	BfxThreadLockUnlock(&list->actionsLock);
}


typedef struct ListBuildAction
{
	DEFINE_BUILD_ACTION_LIST()
} ListBuildAction;

static void ListBuildActionExecute(BuildAction* action, uint32_t preResult)
{
	ListBuildAction* actions = (ListBuildAction*)action;
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&actions->actions);
	if (!item)
	{
		BuildActionResolve(action);
		return;
	}
	BuildActionCallbacks callbacks;
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = actions->oneResolve;
	BuildActionAsUserData(action, &callbacks.resolve.function.userData);
	callbacks.reject.function.function = actions->oneReject;
	BuildActionAsUserData(action, &callbacks.reject.function.userData);
	while (item)
	{
		// tofix: may async call action
		uint32_t result = BuildActionBeginExecute(item->action, preResult, &callbacks, BUILD_ACTION_STATE_BOUND);
		assert(!result);
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)action)->actions, (BfxListItem*)item);
	}
}

static void ListBuildActionOnCanceled(BuildAction* action)
{
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&((ListBuildAction*)action)->actions);
	while (item)
	{
		BuildActionCancelImpl(item->action, BUILD_ACTION_STATE_BOUND);
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)action)->actions, (BfxListItem*)item);
	}
}

static void ListBuildActionUninit(BuildAction* action)
{
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&((ListBuildAction*)action)->actions);
	while (item)
	{
		BuildActionRelease(item->action);
		BfxListItem* preItem = (BfxListItem*)item;
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)action)->actions, (BfxListItem*)item);
		BfxFree(preItem);
	}
	BfxFree(action->name);
}

static void ListBuildActionAllOneResolve(BuildAction* action, void* userData)
{
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&((ListBuildAction*)userData)->actions);
	bool resolved = true;
	while (item)
	{
		if (item->action->state != BUILD_ACTION_STATE_RESOLVED)
		{
			resolved = false;
			break;
		}
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)userData)->actions, (BfxListItem*)item);
	}
	if (resolved)
	{
		BuildActionResolve((BuildAction*)userData);
	}
}

static void ListBuildActionAllOneReject(BuildAction* action, void* userData, uint32_t error)
{
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&((ListBuildAction*)userData)->actions);
	while (item)
	{
		BuildActionCancelImpl(item->action, BUILD_ACTION_STATE_BOUND);
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)userData)->actions, (BfxListItem*)item);
	}
	BuildActionReject((BuildAction*)userData, error);
}

static void ListBuildActionInit(ListBuildAction* actions, BuildActionRuntime* runtime, const char* name)
{
	BuildActionInit((BuildAction*)actions, runtime);
	BfxListInit(&actions->actions);
	((BuildAction*)actions)->execute = ListBuildActionExecute;
	((BuildAction*)actions)->uninit = ListBuildActionUninit;
	((BuildAction*)actions)->onCanceled = ListBuildActionOnCanceled;

	size_t nameLen = strlen(name);
	char* cpname = BFX_MALLOC_ARRAY(char, nameLen + 1);
	memcpy(cpname, name, nameLen + 1);
	actions->name = cpname;
}


static uint32_t ListBuildActionAdd(ListBuildAction* actions, BuildAction* action)
{
	if (BUILD_ACTION_STATE_NONE != actions->state)
	{
		BLOG_DEBUG("add %s to %s failed for invalid state", BuildActionGetName(action), BuildActionGetName((BuildAction*)actions));
		return BFX_RESULT_INVALID_STATE;
	}
	if (BUILD_ACTION_STATE_NONE != BfxAtomicCompareAndSwap32(
		(int32_t*)& action->state,
		BUILD_ACTION_STATE_NONE,
		BUILD_ACTION_STATE_BOUND))
	{
		BLOG_DEBUG("add %s to %s failed for invalid state", BuildActionGetName(action), BuildActionGetName((BuildAction*)actions));
		return BFX_RESULT_INVALID_STATE;
	}
	BLOG_DEBUG("add %s to %s", BuildActionGetName(action), BuildActionGetName((BuildAction*)actions));
	BuildActionListItem* item = BFX_MALLOC_OBJ(BuildActionListItem);
	memset(item, 0, sizeof(BuildActionListItem));
	item->action = action;
	BuildActionAddRef(action);
	BfxListPushBack(&actions->actions, (BfxListItem*)item);
	return BFX_RESULT_SUCCESS;
}

static ListBuildAction* ListBuildActionAllCreate(BuildActionRuntime* runtime, const char* name)
{
	ListBuildAction* actions = BFX_MALLOC_OBJ(ListBuildAction);
	ListBuildActionInit(actions, runtime, name);
	actions->oneResolve = ListBuildActionAllOneResolve;
	actions->oneReject = ListBuildActionAllOneReject;

	return actions;
}


static void ListBuildActionRaceOneResolve(BuildAction* action, void* userData)
{
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&((ListBuildAction*)userData)->actions);
	while (item)
	{
		BuildActionCancelImpl(item->action, BUILD_ACTION_STATE_BOUND);
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)userData)->actions, (BfxListItem*)item);
	}
	BuildActionResolve((BuildAction*)userData);
}

static void ListBuildActionRaceOneReject(BuildAction* action, void* userData, uint32_t error)
{
	BuildActionListItem* item = (BuildActionListItem*)BfxListFirst(&((ListBuildAction*)userData)->actions);
	bool rejected = true;
	while (item)
	{
		if (item->action->state != BUILD_ACTION_STATE_REJECTED)
		{
			rejected = false;
			break;
		}
		item = (BuildActionListItem*)BfxListNext(&((ListBuildAction*)userData)->actions, (BfxListItem*)item);
	}
	if (rejected)
	{
		BuildActionReject((BuildAction*)userData, error);
	}
}

static ListBuildAction* ListBuildActionRaceCreate(BuildActionRuntime* runtime, const char* name)
{
	ListBuildAction* actions = BFX_MALLOC_OBJ(ListBuildAction);
	ListBuildActionInit(actions, runtime, name);
	actions->oneResolve = ListBuildActionRaceOneResolve;
	actions->oneReject = ListBuildActionRaceOneReject;
	return actions;
}



typedef struct TimerTickActionListItem
{
	BfxListItem base;
	BuildTimeoutActionTickCallback callback; 
	BfxUserData userData;
} TimerTickActionListItem;

struct BuildTimeoutAction
{
	DEFINE_BUILD_ACTION()
	Bdt_TimerHelper timer;
	int timeout;
	int cycle;
	uint64_t start;
	BfxList tickActions;
};


static void BFX_STDCALL BuildTimeoutActionOnTimer(BDT_SYSTEM_TIMER timer, void* userData)
{
	BuildTimeoutAction* action = (BuildTimeoutAction*)userData;
	uint64_t now = BfxTimeGetNow(false);
	int passed = (int)((now - action->start) / 1000);
	if (passed > action->timeout)
	{
		BLOG_DEBUG("%s over time", action->name);
		Bdt_TimerHelperStop(&action->timer);
		BuildActionResolve((BuildAction*)userData);
	}
	else
	{
		if (action->state != BUILD_ACTION_STATE_EXECUTING
			&& action->state != BUILD_ACTION_STATE_EXECUTED)
		{
			//BLOG_DEBUG("%s ignore timer tick for not executing", action->name);
			return;
		}
		BfxUserData timerUserData;
		BuildActionAsUserData((BuildAction*)action, &timerUserData);
		Bdt_TimerHelperRestart(&action->timer, BuildTimeoutActionOnTimer, &timerUserData, action->cycle);
		TimerTickActionListItem* item = (TimerTickActionListItem*)BfxListFirst(&action->tickActions);
		while (item)
		{
			item->callback(item->userData.userData);
			item = (TimerTickActionListItem*)BfxListNext(&action->tickActions, (BfxListItem*)item);
		}
	}
}

static void BuildTimeoutActionExecute(BuildAction* baseAction, uint32_t preResult)
{
	BuildTimeoutAction* action = (BuildTimeoutAction*)baseAction;
	action->start = BfxTimeGetNow(false);
	BfxUserData userData;
	BuildActionAsUserData(baseAction, &userData);
	if (!action->cycle)
	{
		Bdt_TimerHelperStart(&action->timer, BuildTimeoutActionOnTimer, &userData, action->timeout);
	}
	else
	{
		Bdt_TimerHelperStart(&action->timer, BuildTimeoutActionOnTimer, &userData, action->cycle);
	}
}

// not thread safe!!!
static uint32_t BuildTimeoutActionAddTickAction(BuildTimeoutAction* timeoutAction, BuildTimeoutActionTickCallback callback, const BfxUserData* userData)
{
	if (BUILD_ACTION_STATE_NONE != timeoutAction->state
		&& BUILD_ACTION_STATE_BOUND != timeoutAction->state)
	{
		BLOG_DEBUG("add to %s failed for timeout action in state %u", timeoutAction->name, timeoutAction->state);
		return BFX_RESULT_INVALID_STATE;
	}
	TimerTickActionListItem* item = BFX_MALLOC_OBJ(TimerTickActionListItem);
	memset(item, 0, sizeof(TimerTickActionListItem));
	item->callback = callback;
	item->userData = *userData;
	if (userData->lpfnAddRefUserData)
	{
		userData->lpfnAddRefUserData(userData->userData);
	}
	BfxListPushBack(&timeoutAction->tickActions, (BfxListItem*)item);
	return BFX_RESULT_SUCCESS;
}

static void BuildTimeoutActionUninit(
	BuildAction* baseAction)
{
	BuildTimeoutAction* action = (BuildTimeoutAction*)baseAction;
	TimerTickActionListItem* item = (TimerTickActionListItem*)BfxListFirst(&action->tickActions);
	while (item)
	{
		if (item->userData.lpfnReleaseUserData)
		{
			item->userData.lpfnReleaseUserData(item->userData.userData);
		}
		BfxListItem* preItem = (BfxListItem*)item;
		item = (TimerTickActionListItem*)BfxListNext(&action->tickActions, (BfxListItem*)item);
		BfxFree(preItem);
	}

	Bdt_TimerHelperUninit(&action->timer);

	BfxFree(action->name);
}

static void BuildTimeoutActionOnCanceled(BuildAction* baseAction)
{
	BuildTimeoutAction* action = (BuildTimeoutAction*)baseAction;
	Bdt_TimerHelperStop(&action->timer);
}

static BuildTimeoutAction* CreateBuildTimeoutAction(
	BuildActionRuntime* runtime, 
	int timeout, 
	int cycle, 
	const char* name)
{
	BuildTimeoutAction* action = BFX_MALLOC_OBJ(BuildTimeoutAction);
	BuildActionInit((BuildAction*)action, runtime);
	BfxListInit(&action->tickActions);
	action->execute = BuildTimeoutActionExecute;
	action->uninit = BuildTimeoutActionUninit;
	action->onCanceled = BuildTimeoutActionOnCanceled;
	action->timeout = timeout;
	action->cycle = cycle;
	Bdt_TimerHelperInit(&action->timer, runtime->builder->framework);
	
	size_t nameLen = strlen(name);
	char* cpname = BFX_MALLOC_ARRAY(char, nameLen + 1);
	memcpy(cpname, name, nameLen + 1);
	action->name = cpname;

	return action;
}
//--------------------------------------------------------------------------

static void SynUdpTunnelActionUninit(
	BuildAction* baseAction)
{
	SynUdpTunnelAction* action = ((SynUdpTunnelAction*)baseAction);
	Bdt_UdpInterfaceRelease(action->udpInterface);
	action->udpInterface = NULL;
	if (action->tunnel)
	{
		Bdt_UdpTunnelRelease(action->tunnel);
		action->tunnel = NULL;
	}

	BfxFree(action->name);
}

static void SynUdpTunnelActionOnTick(void* userData)
{
	SynUdpTunnelAction* action = ((SynUdpTunnelAction*)userData);
	if (action->state != BUILD_ACTION_STATE_EXECUTING
		&& action->state != BUILD_ACTION_STATE_EXECUTED)
	{
		return; 
	}
	TunnelBuilder* builder = action->runtime->builder;
	size_t sendlen = 0;
	const Bdt_PackageWithRef** firstPackages = TunnelBuilderGetFirstPackages(builder, &sendlen);
	if (!action->tunnel)
	{
		return;
	}
	Bdt_UdpTunnel* tunnel = action->tunnel;
	if (SocketTunnelGetLastRecv((Bdt_SocketTunnel*)tunnel) > action->startingLastRecv)
	{
		BuildActionResolve((BuildAction*)action);
		return;
	}
	BLOG_DEBUG("%s send syn package", BuildActionGetName((BuildAction*)action));
	const Bdt_Package* packages[TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH];
	for (size_t ix = 0; ix < sendlen; ++ix)
	{
		packages[ix] = Bdt_PackageWithRefGet(firstPackages[ix]);
	}
	TunnelContainerSendFromTunnel(builder->container, (Bdt_SocketTunnel*)tunnel, packages, &sendlen);
}

static void SynUdpTunnelActionExecute(BuildAction* baseAction, uint32_t preResult)
{
	SynUdpTunnelAction* action = ((SynUdpTunnelAction*)baseAction);
	TunnelBuilder* builder = action->runtime->builder;
	Bdt_SocketTunnel* tunnel = NULL;
	
	if (!(tunnel = BdtTunnel_GetTunnelByEndpoint(builder->container, Bdt_UdpInterfaceGetLocal(action->udpInterface), &action->remote)))
	{
		uint32_t result = TunnelContainerCreateTunnel(
			builder->container,
			Bdt_UdpInterfaceGetLocal(action->udpInterface),
			&action->remote, 
			0, 
			&tunnel);
		if (!result)
		{
			UdpTunnelSetInterface((UdpTunnel*)tunnel, action->udpInterface);
		}
	}
	action->startingLastRecv = SocketTunnelGetLastRecv(tunnel);
	assert(tunnel);
	BfxAtomicExchangePointer(&action->tunnel, tunnel);
	//TODO:可读性  实在没有办法把函数名和里面的动作联系起来，叫这个是因为会在ontimer里resend吧
	
	SynUdpTunnelActionOnTick((BuildAction*)action);
}


static void SynUdpTunnelActionOnCanceled(BuildAction* baseAction)
{
	SynUdpTunnelAction* action = (SynUdpTunnelAction*)baseAction;
	if (action->tunnel)
	{
		TunnelContainerRemoveTunnel(action->runtime->builder->container, (SocketTunnel*)action->tunnel, action->startingLastRecv);
	}
}

static void SynUdpTunnelActionOnResolved(BuildAction* baseAction, void* userData)
{
	SynUdpTunnelAction* action = (SynUdpTunnelAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime, 
		Bdt_UdpInterfaceGetLocal(action->udpInterface),
		&action->remote,
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void SynUdpTunnelActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	SynUdpTunnelAction* action = (SynUdpTunnelAction*)baseAction;
	TunnelContainerRemoveTunnel(action->runtime->builder->container, (SocketTunnel*)action->tunnel, action->startingLastRecv);
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_UdpInterfaceGetLocal(action->udpInterface),
		&action->remote,
		baseAction,
		error
	);
}

static SynUdpTunnelAction* CreateSynUdpTunnelAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction, 
	const Bdt_UdpInterface* udpInterface,
	const BdtEndpoint* remote)
{
	SynUdpTunnelAction* action = BFX_MALLOC_OBJ(SynUdpTunnelAction);
	BuildActionInit((BuildAction*)action, runtime);
	action->udpInterface = udpInterface;
	Bdt_UdpInterfaceAddRef(udpInterface);
	action->remote = *remote;
	action->execute = SynUdpTunnelActionExecute;
	action->onCanceled = SynUdpTunnelActionOnCanceled;
	action->uninit = SynUdpTunnelActionUninit;

	const char* prename = "SynUdpTunnelAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_UdpInterfaceGetLocal(udpInterface), localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remote, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	BfxUserData udTick;
	BuildActionAsUserData((BuildAction*)action, &udTick);
	BuildTimeoutActionAddTickAction(timeoutAction, SynUdpTunnelActionOnTick, &udTick);

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = SynUdpTunnelActionOnResolved;
	callbacks.reject.function.function = SynUdpTunnelActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime, 
		Bdt_UdpInterfaceGetLocal(action->udpInterface),
		&action->remote, 
		(BuildAction*)action, 
		&callbacks, 
		false);

	return action;
}



//--------------------------------------------------------------------------

static void AckUdpTunnelActionUninit(
	BuildAction* baseAction)
{
    AckUdpTunnelAction* action = ((AckUdpTunnelAction*)baseAction);
	Bdt_UdpInterfaceRelease(action->udpInterface);
	action->udpInterface = NULL;
	if (action->tunnel)
	{
		Bdt_UdpTunnelRelease(action->tunnel);
		action->tunnel = NULL;
	}

	BfxFree(action->name);
}

static void AckUdpTunnelActionOnTick(void* userData)
{
	AckUdpTunnelAction* action = ((AckUdpTunnelAction*)userData);
	if (action->state != BUILD_ACTION_STATE_EXECUTING
		&& action->state != BUILD_ACTION_STATE_EXECUTED)
	{
		BLOG_DEBUG("%s ignore tick for not executing", action->name);
		return;
	}
	TunnelBuilder* builder = action->runtime->builder;
	size_t sendlen = 0;
	const Bdt_PackageWithRef** firstPackages = TunnelBuilderGetFirstPackages(builder, &sendlen);
	if (!action->tunnel)
	{
		BLOG_DEBUG("%s ignore tick for no tunnel exists", action->name);
		return;
	}
	Bdt_UdpTunnel* tunnel = action->tunnel;
	if (SocketTunnelGetLastRecv((Bdt_SocketTunnel*)tunnel) > action->startingLastRecv)
	{
		BuildActionResolve((BuildAction*)action);
		return;
	}
	const Bdt_PackageWithRef* respPackage = TunnelContainerGetCachedResponse(
		action->runtime->builder->container, 
		TunnelBuilderGetSeq(action->runtime->builder));
	if (!respPackage)
	{
		uint64_t now = BfxTimeGetNow(false);
		uint32_t executed = (uint32_t)((now - action->startTime) / 1000);
		if (executed < action->waitConfirm)
		{
			BLOG_DEBUG("%s ignore tick for waiting confirm", action->name);
			return;
		}
	}
	
	BLOG_DEBUG("%s send ack package", BuildActionGetName((BuildAction*)action));
	const Bdt_Package* packages[TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH];
	for (size_t ix = 0; ix < sendlen; ++ix)
	{
		packages[ix] = Bdt_PackageWithRefGet(firstPackages[ix]);
	}
	if (respPackage)
	{
		packages[sendlen] = Bdt_PackageWithRefGet(respPackage);
		++sendlen;
	} 
	TunnelContainerSendFromTunnel(builder->container, (Bdt_SocketTunnel*)tunnel, packages, &sendlen);
	if (respPackage)
	{
		Bdt_PackageRelease(respPackage);
	}
}

static void AckUdpTunnelActionExecute(BuildAction* baseAction, uint32_t preResult)
{
	AckUdpTunnelAction* action = ((AckUdpTunnelAction*)baseAction);
	action->startTime = BfxTimeGetNow(false);
	TunnelBuilder* builder = action->runtime->builder;
	Bdt_SocketTunnel* tunnel = NULL;
	if (!(tunnel = BdtTunnel_GetTunnelByEndpoint(builder->container, Bdt_UdpInterfaceGetLocal(action->udpInterface), &action->remote)))
	{
		uint32_t result = TunnelContainerCreateTunnel(
			builder->container,
			Bdt_UdpInterfaceGetLocal(action->udpInterface),
			&action->remote,
			0, 
			&tunnel);
		if (!result)
		{
			UdpTunnelSetInterface((UdpTunnel*)tunnel, action->udpInterface);
		}
	}
	assert(tunnel);
	action->startingLastRecv = SocketTunnelGetLastRecv(tunnel);
	BfxAtomicExchangePointer(&action->tunnel, tunnel);
	AckUdpTunnelActionOnTick((BuildAction*)action);
}

static void AckUdpTunnelActionOnCanceled(BuildAction* baseAction)
{
	AckUdpTunnelAction* action = (AckUdpTunnelAction*)baseAction;
	if (action->tunnel)
	{
		TunnelContainerRemoveTunnel(action->runtime->builder->container, (SocketTunnel*)action->tunnel, action->startingLastRecv);
	}
}

static uint32_t AckUdpTunnelActionConfirm(PassiveBuildAction* baseAction, const Bdt_PackageWithRef* firstResp)
{
	// do nothing
	return BFX_RESULT_SUCCESS;
}

static void AckUdpTunnelActionOnResolved(BuildAction* baseAction, void* userData)
{
	AckUdpTunnelAction* action = (AckUdpTunnelAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_UdpInterfaceGetLocal(action->udpInterface),
		&action->remote,
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void AckUdpTunnelActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	AckUdpTunnelAction* action = (AckUdpTunnelAction*)baseAction;
	TunnelContainerRemoveTunnel(action->runtime->builder->container, (SocketTunnel*)action->tunnel, action->startingLastRecv);
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_UdpInterfaceGetLocal(action->udpInterface),
		&action->remote,
		baseAction,
		error
	);
}

static AckUdpTunnelAction* CreateAckUdpTunnelAction(
	BuildActionRuntime* runtime,
	BuildTimeoutAction* timeoutAction,
	const Bdt_UdpInterface* udpInterface,
	const BdtEndpoint* remote, 
	uint32_t waitConfirm)
{
	AckUdpTunnelAction* action = BFX_MALLOC_OBJ(AckUdpTunnelAction);
	BuildActionInit((BuildAction*)action, runtime);
	action->udpInterface = udpInterface;
	Bdt_UdpInterfaceAddRef(udpInterface);
	action->remote = *remote;
	action->confirm = AckUdpTunnelActionConfirm;
	action->execute = AckUdpTunnelActionExecute;
	action->onCanceled = AckUdpTunnelActionOnCanceled;
	action->uninit = AckUdpTunnelActionUninit;
	action->waitConfirm = waitConfirm;

	const char* prename = "AckUdpTunnelAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_UdpInterfaceGetLocal(udpInterface), localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remote, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	BfxUserData udTick;
	BuildActionAsUserData((BuildAction*)action, &udTick);
	BuildTimeoutActionAddTickAction(timeoutAction, AckUdpTunnelActionOnTick, &udTick);

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = AckUdpTunnelActionOnResolved;
	callbacks.reject.function.function = AckUdpTunnelActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime,
		Bdt_UdpInterfaceGetLocal(action->udpInterface),
		&action->remote,
		(BuildAction*)action,
		&callbacks, 
		false);

	return action;
}


static void SynTcpTunnelActionUninit(BuildAction* baseAction)
{
	SynTcpTunnelAction* action = (SynTcpTunnelAction*)baseAction;
	if (action->tunnel)
	{
		Bdt_TcpTunnelRelease(action->tunnel);
		action->tunnel = NULL;
	}

	BfxFree(action->name);
}

static void SynTcpTunnelActionOnTick(void* userData)
{
	SynTcpTunnelAction* action = (SynTcpTunnelAction*)userData;
	if (action->tunnel && Bdt_TcpTunnelGetState(action->tunnel) == BDT_TCP_TUNNEL_STATE_ALIVE)
	{
		BuildActionResolve((BuildAction*)action);
	}
}

static void SynTcpTunnelActionExecute(BuildAction* baseAction, uint32_t preError)
{
	SynTcpTunnelAction* action = (SynTcpTunnelAction*)baseAction;
	TunnelBuilder* builder = action->runtime->builder;
	Bdt_SocketTunnel* tunnel = BdtTunnel_GetTunnelByEndpoint(builder->container, &action->local, &action->remote);
	if (!tunnel)
	{
		TunnelContainerCreateTunnel(builder->container, &action->local, &action->remote, 0, &tunnel);
	}
	assert(tunnel);
	BfxAtomicExchangePointer(&action->tunnel, tunnel);
	TcpTunnelState oldState;
	uint32_t result = TcpTunnelConnect((Bdt_TcpTunnel*)tunnel, builder->seq, NULL, 0, &oldState);
	if (result != BFX_RESULT_SUCCESS)
	{
		if (oldState == BDT_TCP_TUNNEL_STATE_ALIVE)
		{
			BuildActionResolve((BuildAction*)action);
		}
		else if (oldState == BDT_TCP_TUNNEL_STATE_DEAD)
		{
			BuildActionReject((BuildAction*)action, BFX_RESULT_FAILED);
		}
	}
}


static void SynTcpTunnelActionOnResolved(BuildAction* baseAction, void* userData)
{
	SynTcpTunnelAction* action = (SynTcpTunnelAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		&action->local, 
		&action->remote,
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void SynTcpTunnelActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	SynTcpTunnelAction* action = (SynTcpTunnelAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		&action->local,
		&action->remote,
		baseAction,
		error
	);
}

static SynTcpTunnelAction* CreateSynTcpTunnelAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction, 
	const BdtEndpoint* local,
	const BdtEndpoint* remote
)
{
	SynTcpTunnelAction* action = BFX_MALLOC_OBJ(SynTcpTunnelAction);
	BuildActionInit((BuildAction*)action, runtime);
	action->local = *local;
	action->remote = *remote;
	action->uninit = SynTcpTunnelActionUninit;
	action->execute = SynTcpTunnelActionExecute;

	const char* prename = "SynTcpTunnelAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(local, localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remote, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	BfxUserData udTick;
	BuildActionAsUserData((BuildAction*)action, &udTick);
	BuildTimeoutActionAddTickAction(timeoutAction, SynTcpTunnelActionOnTick, &udTick);

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = SynTcpTunnelActionOnResolved;
	callbacks.reject.function.function = SynTcpTunnelActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime,
		&action->local, 
		&action->remote,
		(BuildAction*)action, 
		&callbacks, 
		false);

	return action;
}

struct ConnectConnectionAction
{
	DEFINE_CONNECT_CONNECTION_ACTION()
};


static uint32_t ConnectConnectionActionInit(
	ConnectConnectionAction* action,
	BuildActionRuntime* runtime
)
{
	BuildActionInit((BuildAction*)action, runtime);
	return BFX_RESULT_SUCCESS;
}

// called when active builder decide to use this action as connection's provider
static uint32_t ConnectConnectionActionContinueConnect(
	ConnectConnectionAction* action
)
{
	uint32_t result = action->continueConnect(action);
	BLOG_DEBUG("%s continue", BuildActionGetName((BuildAction*)action));
	return result;
}



static Bdt_TcpInterfaceOwner* ConnectStreamActionAsInterfaceOwner(ConnectStreamAction* action)
{
	return (Bdt_TcpInterfaceOwner*) & (action->addRef);
}

static ConnectStreamAction* ConnectStreamActionFromInterfaceOwner(Bdt_TcpInterfaceOwner* owner)
{
	static ConnectStreamAction ins;
	return (ConnectStreamAction*)((intptr_t)owner - ((intptr_t)ConnectStreamActionAsInterfaceOwner(&ins) - (intptr_t)&ins));
}


static uint32_t ConnectStreamActionContinueConnect(ConnectConnectionAction* baseAction)
{
	if (baseAction->state != BUILD_ACTION_STATE_EXECUTING
		&& baseAction->state != BUILD_ACTION_STATE_EXECUTED)
	{
		return BFX_RESULT_INVALID_STATE;
	}
	ConnectStreamAction* action = (ConnectStreamAction*)baseAction;
	Bdt_TunnelBuilder* builder = action->runtime->builder;
	Bdt_TunnelEncryptOptions encrypt;
	BdtTunnel_GetEnryptOptions(builder->container, &encrypt);
	Bdt_TcpInterfaceSetAesKey(action->tcpInterface, encrypt.key);
	Bdt_Package* packages[BDT_MAX_PACKAGE_MERGE_LENGTH];
	uint8_t count = 0;

	BLOG_DEBUG("%s send tcp syn connection", BuildActionGetName((BuildAction*)action));

	Bdt_ConnectionGenTcpSynPackage(builder->connection, BDT_PACKAGE_RESULT_SUCCESS, &encrypt, packages, &count);

	uint32_t result = BdtTunnel_SendFirstTcpPackage(
		action->runtime->builder->tls, 
		action->tcpInterface, 
		packages, 
		count, 
		&encrypt);

	for (uint8_t ix = 0; ix < count; ++ix)
	{
		Bdt_PackageFree(packages[ix]);
	}

	return result;
}

static void ConnectStreamActionOnCanceled(BuildAction* baseAction)
{
	ConnectStreamAction* action = (ConnectStreamAction*)baseAction;
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceReset(action->tcpInterface);
	}
}

static void ConnectStreamActionOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error)
{
	ConnectStreamAction* action = ConnectStreamActionFromInterfaceOwner(owner);
	Bdt_TcpInterfaceReset(tcpInterface);
	
	BuildActionReject((BuildAction*)action, BFX_RESULT_FAILED);
}

static void ConnectStreamActionExecute(BuildAction* baseAction, uint32_t preError)
{
	ConnectStreamAction* action = (ConnectStreamAction*)baseAction;
	Bdt_TcpInterface* tcpInterface = NULL;
	Bdt_NetManagerCreateTcpInterface(
		action->runtime->builder->netManager,
		&action->local,
		&action->remote,
		&tcpInterface);
	assert(tcpInterface);
	action->tcpInterface = tcpInterface;
	Bdt_TcpInterfaceSetOwner(tcpInterface, ConnectStreamActionAsInterfaceOwner(action));
	uint32_t result = Bdt_TcpInterfaceConnect(tcpInterface);
	if (result != BFX_RESULT_SUCCESS)
	{
		ConnectStreamActionOnTcpError(ConnectStreamActionAsInterfaceOwner(action), tcpInterface, result);
	}
}

static uint32_t ConnectStreamActionOnTcpEstablish(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	ConnectStreamAction* action = ConnectStreamActionFromInterfaceOwner(owner);
	assert(action->tcpInterface == tcpInterface);
	Bdt_TcpInterfaceSetState(
		tcpInterface,
		BDT_TCP_INTERFACE_STATE_ESTABLISH,
		BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP);
	BuildActionRuntimeActiveBuildingTunnel(
		action->runtime,
		Bdt_TcpInterfaceGetLocal(action->tcpInterface),
		Bdt_TcpInterfaceGetRemote(action->tcpInterface),
		(BuildAction*)action);

	return BFX_RESULT_SUCCESS;
}




static uint32_t ConnectStreamActionOnFirstResp(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package* package)
{
	ConnectStreamAction* action = ConnectStreamActionFromInterfaceOwner(owner);
	if (package->cmdtype == BDT_TCP_ACK_CONNECTION_PACKAGE)
	{
		const Bdt_TcpAckConnectionPackage* ackPackage = (Bdt_TcpAckConnectionPackage*)package;

		BdtTunnel_OnTcpFirstResp(action->runtime->builder->container, NULL, tcpInterface, ackPackage->peerInfo);

		BLOG_DEBUG("%s got tcp ack connection package with result %u", BuildActionGetName((BuildAction*)action), ackPackage->result);
		if (ackPackage->result != BDT_PACKAGE_RESULT_SUCCESS)
		{
			Bdt_TcpInterfaceReset(tcpInterface);

			BuildActionReject((BuildAction*)action, BFX_RESULT_FAILED);
			return BFX_RESULT_SUCCESS;
		}
		size_t faLen = 0;
		const uint8_t* fa = NULL;
		if (ackPackage->payload)
		{
			fa = BfxBufferGetData(ackPackage->payload, &faLen);
		}
		Bdt_ConnectionFireFirstAnswer(action->runtime->builder->connection, ackPackage->toSessionId, fa, faLen);
		
		uint32_t result = Bdt_ConnectionTryEnterEstablish(action->runtime->builder->connection);
		if (result == BFX_RESULT_SUCCESS)
		{
			result = Bdt_ConnectionEstablishWithStream(action->runtime->builder->connection, tcpInterface);
		}
		else
		{
			Bdt_TcpInterfaceReset(tcpInterface);
		}
		
		if (result == BFX_RESULT_SUCCESS)
		{
			BuildActionResolve((BuildAction*)action);
		}
		else
		{
			BuildActionReject((BuildAction*)action, result);
		}


		return result;
	}
	else
	{
		return BFX_RESULT_INVALID_PARAM;
	}
}

static void ConnectStreamActionUninit(
	BuildAction* baseAction)
{
	ConnectStreamAction* action = ((ConnectStreamAction*)baseAction);
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceRelease(action->tcpInterface);
	}

	BfxFree(action->name);
}

static int32_t ConnectStreamActionAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	ConnectStreamAction* action = ConnectStreamActionFromInterfaceOwner(owner);
	return BuildActionAddRef((BuildAction*)action);
}

static int32_t ConnectStreamActionAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	ConnectStreamAction* action = ConnectStreamActionFromInterfaceOwner(owner);
	return BuildActionRelease((BuildAction*)action);
}

static void ConnectStreamActionInitAsTcpInterfaceOwner(ConnectStreamAction* action)
{
	action->addRef = ConnectStreamActionAsTcpInterfaceOwnerAddRef;
	action->release = ConnectStreamActionAsTcpInterfaceOwnerRelease;
	action->onEstablish = ConnectStreamActionOnTcpEstablish;
	action->onError = ConnectStreamActionOnTcpError;
	action->onFirstResp = ConnectStreamActionOnFirstResp;
}

static void ConnectStreamActionOnResolved(BuildAction* baseAction, void* userData)
{
	ConnectStreamAction* action = (ConnectStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		&action->local,
		&action->remote,
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void ConnectStreamActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	ConnectStreamAction* action = (ConnectStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		&action->local,
		&action->remote,
		baseAction,
		error
	);
}

static ConnectStreamAction* CreateConnectStreamAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote)
{
	ConnectStreamAction* action = BFX_MALLOC_OBJ(ConnectStreamAction);
	memset(action, 0, sizeof(ConnectStreamAction));
	ConnectConnectionActionInit((ConnectConnectionAction*)action, runtime);
	action->local = *local;
	action->remote = *remote;
	action->execute = ConnectStreamActionExecute;
	action->uninit = ConnectStreamActionUninit;
	action->onCanceled = ConnectStreamActionOnCanceled;
	action->continueConnect = ConnectStreamActionContinueConnect;
	ConnectStreamActionInitAsTcpInterfaceOwner(action);

	const char* prename = "ConnectStreamAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(local, localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remote, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = ConnectStreamActionOnResolved;
	callbacks.reject.function.function = ConnectStreamActionOnRejected;
	
	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime, 
		local, 
		remote, 
		(BuildAction*)action, 
		&callbacks, 
		false);

	return action;
}

static Bdt_TcpInterfaceOwner* AcceptReverseStreamActionAsInterfaceOwner(AcceptReverseStreamAction* action)
{
	return (Bdt_TcpInterfaceOwner*) & (action->addRef);
}

static AcceptReverseStreamAction* AcceptReverseStreamActionFromInterfaceOwner(Bdt_TcpInterfaceOwner* owner)
{
	static AcceptReverseStreamAction ins;
	return (AcceptReverseStreamAction*)((intptr_t)owner - ((intptr_t)AcceptReverseStreamActionAsInterfaceOwner(&ins) - (intptr_t)&ins));
}


static uint32_t AcceptReverseStreamActionContinueConnect(ConnectConnectionAction* baseAction)
{
	AcceptReverseStreamAction* action = (AcceptReverseStreamAction*)baseAction;
	Bdt_TunnelBuilder* builder = action->runtime->builder;
	Bdt_Package* package = NULL;

	BLOG_DEBUG("%s send tcp ack ack connection package", BuildActionGetName((BuildAction*)action));

	uint32_t result = Bdt_ConnectionTryEnterEstablish(builder->connection);
	if (result == BFX_RESULT_SUCCESS)
	{
		Bdt_ConnectionGenTcpAckAckPackage(builder->connection, BDT_PACKAGE_RESULT_SUCCESS, &package);
		result = BdtTunnel_SendFirstTcpResp(
			action->runtime->builder->tls,
			action->tcpInterface,
			&package,
			1);

		Bdt_PackageFree(package);

		result = Bdt_ConnectionEstablishWithStream(action->runtime->builder->connection, action->tcpInterface);
	}
	
	return result;
}

static void AcceptReverseStreamActionExecute(BuildAction* baseAction, uint32_t preError)
{
	AcceptReverseStreamAction* action = (AcceptReverseStreamAction*)baseAction; 
	// do nothing
}


static void AcceptReverseStreamActionOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error)
{
	AcceptReverseStreamAction* action = AcceptReverseStreamActionFromInterfaceOwner(owner);
	//todo: reject action if tcp error occured
}

static void AcceptReverseStreamActionOnCanceled(BuildAction* baseAction)
{
	AcceptReverseStreamAction* action = (AcceptReverseStreamAction*)baseAction;
	// tofix: send ackack with canceled result
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceReset(action->tcpInterface);
	}
}


static void AcceptReverseStreamActionUninit(
	BuildAction* baseAction)
{
	AcceptReverseStreamAction* action = ((AcceptReverseStreamAction*)baseAction);
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceRelease(action->tcpInterface);
	}
	BfxFree(action->name);
}

static int32_t AcceptReverseStreamActionAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	AcceptReverseStreamAction* action = AcceptReverseStreamActionFromInterfaceOwner(owner);
	return BuildActionAddRef((BuildAction*)action);
}

static int32_t AcceptReverseStreamActionAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	AcceptReverseStreamAction* action = AcceptReverseStreamActionFromInterfaceOwner(owner);
	return BuildActionRelease((BuildAction*)action);
}

static void AcceptReverseStreamActionInitAsTcpInterfaceOwner(AcceptReverseStreamAction* action)
{
	action->addRef = AcceptReverseStreamActionAsTcpInterfaceOwnerAddRef;
	action->release = AcceptReverseStreamActionAsTcpInterfaceOwnerRelease;
	action->onError = AcceptReverseStreamActionOnTcpError;
}

static void AcceptReverseStreamActionOnResolved(BuildAction* baseAction, void* userData)
{
	AcceptReverseStreamAction* action = (AcceptReverseStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_TcpInterfaceGetLocal(action->tcpInterface),
		Bdt_TcpInterfaceGetRemote(action->tcpInterface),
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void AcceptReverseStreamActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	AcceptReverseStreamAction* action = (AcceptReverseStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_TcpInterfaceGetLocal(action->tcpInterface),
		Bdt_TcpInterfaceGetRemote(action->tcpInterface),
		baseAction,
		error
	);
}

static AcceptReverseStreamAction* CreateAcceptReverseStreamAction(
	BuildActionRuntime* runtime,
	Bdt_TcpInterface* tcpInterface)
{
	AcceptReverseStreamAction* action = BFX_MALLOC_OBJ(AcceptReverseStreamAction);
	memset(action, 0, sizeof(AcceptReverseStreamAction));
	ConnectConnectionActionInit((ConnectConnectionAction*)action, runtime);
	action->tcpInterface = tcpInterface;
	Bdt_TcpInterfaceAddRef(tcpInterface);
	action->execute = AcceptReverseStreamActionExecute;
	action->onCanceled = AcceptReverseStreamActionOnCanceled;
	action->continueConnect = AcceptReverseStreamActionContinueConnect;
	action->uninit = AcceptReverseStreamActionUninit;

	const char* prename = "AcceptReverseStreamAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_TcpInterfaceGetLocal(tcpInterface), localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	AcceptReverseStreamActionInitAsTcpInterfaceOwner(action);

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = AcceptReverseStreamActionOnResolved;
	callbacks.reject.function.function = AcceptReverseStreamActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime, 
		Bdt_TcpInterfaceGetLocal(tcpInterface), 
		Bdt_TcpInterfaceGetRemote(tcpInterface), 
		(BuildAction*)action, 
		&callbacks, 
		true);

	return action;
}


static Bdt_TcpInterfaceOwner* AcceptStreamActionAsInterfaceOwner(AcceptStreamAction* action)
{
	return (Bdt_TcpInterfaceOwner*) & (action->addRef);
}

static AcceptStreamAction* AcceptStreamActionFromInterfaceOwner(Bdt_TcpInterfaceOwner* owner)
{
	static AcceptStreamAction ins;
	return (AcceptStreamAction*)((intptr_t)owner - ((intptr_t)AcceptStreamActionAsInterfaceOwner(&ins) - (intptr_t)&ins));
}

static int32_t AcceptStreamActionAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	AcceptStreamAction* action = AcceptStreamActionFromInterfaceOwner(owner);
	return BuildActionAddRef((BuildAction*)action);
}

static int32_t AcceptStreamActionAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	AcceptStreamAction* action = AcceptStreamActionFromInterfaceOwner(owner);
	return BuildActionRelease((BuildAction*)action);
}

static uint32_t AcceptStreamActionConfirm(PassiveBuildAction* baseAction, const Bdt_PackageWithRef* firstResp)
{
	AcceptStreamAction* action = (AcceptStreamAction*)baseAction;
	if (action->state != BUILD_ACTION_STATE_EXECUTING
		&& action->state != BUILD_ACTION_STATE_EXECUTED)
	{
		return BFX_RESULT_INVALID_STATE;
	}

	//tofix: check state to protected reenter

	uint32_t result = Bdt_ConnectionTryEnterEstablish(action->runtime->builder->connection);
	if (result == BFX_RESULT_SUCCESS)
	{
		ConnectStreamAction* action = (ConnectStreamAction*)baseAction;
		Bdt_TunnelBuilder* builder = action->runtime->builder;

		Bdt_Package* packages[BDT_MAX_PACKAGE_MERGE_LENGTH];
		uint8_t count = 0;

		BLOG_DEBUG("%s send tcp ack connection package", BuildActionGetName((BuildAction*)action));

		Bdt_ConnectionGenTcpAckPackage(builder->connection, BDT_PACKAGE_RESULT_SUCCESS, NULL, packages, &count);
		uint32_t result = BdtTunnel_SendFirstTcpResp(
			action->runtime->builder->tls,
			action->tcpInterface,
			packages,
			count);
		for (uint8_t ix = 0; ix < count; ++ix)
		{
			Bdt_PackageFree(packages[ix]);
		}

		result = Bdt_ConnectionEstablishWithStream(action->runtime->builder->connection, action->tcpInterface);
		
		if (result == BFX_RESULT_SUCCESS)
		{
			BuildActionResolve((BuildAction*)action);
		}
		else
		{
			BuildActionReject((BuildAction*)action, result);
		}
		return result;
	}
	else
	{
		// tofix: send ack with error result
		Bdt_TcpInterfaceReset(action->tcpInterface);
		BuildActionReject((BuildAction*)action, result);

		return result;
	}
}

static void AcceptStreamActionExecute(BuildAction* baseAction, uint32_t preError)
{
	//do nothing
}

static void AcceptStreamActionOnCanceled(BuildAction* baseAction)
{
	AcceptStreamAction* action = (AcceptStreamAction*)baseAction;
	Bdt_TcpInterfaceReset(action->tcpInterface);
}

static void AcceptStreamActionOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error)
{
	AcceptStreamAction* action = AcceptStreamActionFromInterfaceOwner(owner);
	// todo: reject action when error occured
}

static void AcceptStreamActionInitAsTcpInterfaceOwner(AcceptStreamAction* action)
{
	action->addRef = AcceptStreamActionAsTcpInterfaceOwnerAddRef;
	action->release = AcceptStreamActionAsTcpInterfaceOwnerRelease;
	action->onError = AcceptStreamActionOnTcpError;
}

static void AcceptStreamActionUninit(BuildAction* baseAction)
{
	AcceptStreamAction* action = (AcceptStreamAction*)baseAction;
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceRelease(action->tcpInterface);
	}
	BfxFree(action->name);
}

static void AcceptStreamActionOnResolved(BuildAction* baseAction, void* userData)
{
	AcceptStreamAction* action = (AcceptStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_TcpInterfaceGetLocal(action->tcpInterface),
		Bdt_TcpInterfaceGetRemote(action->tcpInterface),
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void AcceptStreamActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	AcceptStreamAction* action = (AcceptStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		Bdt_TcpInterfaceGetLocal(action->tcpInterface),
		Bdt_TcpInterfaceGetRemote(action->tcpInterface),
		baseAction,
		error
	);
}

static AcceptStreamAction* CreateAcceptStreamAction(
	BuildActionRuntime* runtime,
	Bdt_TcpInterface* tcpInterface
)
{
	AcceptStreamAction* action = BFX_MALLOC_OBJ(AcceptStreamAction);
	memset(action, 0, sizeof(AcceptStreamAction));
	BuildActionInit((BuildAction*)action, runtime);

	action->tcpInterface = tcpInterface;
	Bdt_TcpInterfaceAddRef(tcpInterface);
	
	action->execute = AcceptStreamActionExecute;
	action->onCanceled = AcceptStreamActionOnCanceled;
	action->uninit = AcceptStreamActionUninit;

	action->confirm = AcceptStreamActionConfirm;

	AcceptStreamActionInitAsTcpInterfaceOwner(action);
	Bdt_TcpInterfaceSetOwner(tcpInterface, AcceptStreamActionAsInterfaceOwner(action));

	const char* prename = "AcceptStreamAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_TcpInterfaceGetLocal(tcpInterface), localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(Bdt_TcpInterfaceGetRemote(tcpInterface), remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = AcceptStreamActionOnResolved;
	callbacks.reject.function.function = AcceptStreamActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime,
		Bdt_TcpInterfaceGetLocal(tcpInterface),
		Bdt_TcpInterfaceGetRemote(tcpInterface),
		(BuildAction*)action, 
		&callbacks, 
		true);

	return action;
}


static Bdt_TcpInterfaceOwner* ConnectReverseStreamActionAsInterfaceOwner(ConnectReverseStreamAction* action)
{
	return (Bdt_TcpInterfaceOwner*) & (action->addRef);
}

static ConnectReverseStreamAction* ConnectReverseStreamActionFromInterfaceOwner(Bdt_TcpInterfaceOwner* owner)
{
	static ConnectReverseStreamAction ins;
	return (ConnectReverseStreamAction*)((intptr_t)owner - ((intptr_t)ConnectReverseStreamActionAsInterfaceOwner(&ins) - (intptr_t)&ins));
}

static uint32_t ConnectReverseStreamActionSendAck(ConnectReverseStreamAction* action)
{
	Bdt_TunnelEncryptOptions encrypt;
	BdtTunnel_GetEnryptOptions(action->runtime->builder->container, &encrypt);
	Bdt_TcpInterfaceSetAesKey(action->tcpInterface, encrypt.key);
	Bdt_Package* packages[BDT_MAX_PACKAGE_MERGE_LENGTH];
	uint8_t count = 0;

	BLOG_DEBUG("%s send tcp ack connection package", BuildActionGetName((BuildAction*)action));

	Bdt_ConnectionGenTcpAckPackage(
		action->runtime->builder->connection, 
		BDT_PACKAGE_RESULT_SUCCESS, 
		&encrypt, 
		packages, 
		&count);

	uint32_t result = BdtTunnel_SendFirstTcpPackage(
		action->runtime->builder->tls, 
		action->tcpInterface, 
		packages, 
		count, 
		&encrypt);

	for (uint8_t ix = 0; ix < count; ++ix)
	{
		Bdt_PackageFree(packages[ix]);
	}

	return result;
}

static uint32_t ConnectReverseStreamActionOnTcpEstablish(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface)
{
	ConnectReverseStreamAction* action = ConnectReverseStreamActionFromInterfaceOwner(owner);
	assert(action->tcpInterface == tcpInterface);
	Bdt_TcpInterfaceSetState(
		tcpInterface,
		BDT_TCP_INTERFACE_STATE_ESTABLISH,
		BDT_TCP_INTERFACE_STATE_WAIT_FIRST_RESP);
	BuildActionRuntimeActiveBuildingTunnel(
		action->runtime,
		Bdt_TcpInterfaceGetLocal(action->tcpInterface),
		Bdt_TcpInterfaceGetRemote(action->tcpInterface),
		(BuildAction*)action);
	
	bool confirmed = false;
	BfxThreadLockLock(&action->tcpEstablishedLock);
	do
	{
		action->tcpEstablished = true;
		confirmed = action->confirmed;
	} while (false);
	BfxThreadLockUnlock(&action->tcpEstablishedLock);
	
	uint32_t result = BFX_RESULT_SUCCESS;
	if (confirmed)
	{
		BLOG_DEBUG("%s has confirmed when tcp establish, send ack", action->name);
		result = ConnectReverseStreamActionSendAck(action);
	}

	return BFX_RESULT_SUCCESS;
}


static uint32_t ConnectReverseStreamActionOnFirstResp(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, const Bdt_Package* package)
{
	ConnectReverseStreamAction* action = ConnectReverseStreamActionFromInterfaceOwner(owner);
	if (package->cmdtype == BDT_TCP_ACKACK_CONNECTION_PACKAGE)
	{
		BLOG_DEBUG("%s got tcp ack ack connection package", BuildActionGetName((BuildAction*)action));

		const BdtPeerInfo* remoteInfo = BuildActionRuntimeGetRemoteInfo(action->runtime);
		BdtTunnel_OnTcpFirstResp(
			action->runtime->builder->container,
			NULL,
			tcpInterface,
			remoteInfo
		);
		if (remoteInfo)
		{
			BdtReleasePeerInfo(remoteInfo);
		}

		uint32_t result = Bdt_ConnectionTryEnterEstablish(action->runtime->builder->connection);

		if (result == BFX_RESULT_SUCCESS)
		{
			result = Bdt_ConnectionEstablishWithStream(action->runtime->builder->connection, tcpInterface);
		}

		if (result == BFX_RESULT_SUCCESS)
		{
			BuildActionResolve((BuildAction*)action);
		}
		else
		{
			BuildActionReject((BuildAction*)action, result);
		}
		
		return BFX_RESULT_SUCCESS;
	}
	else
	{
		return BFX_RESULT_INVALID_PARAM;
	}
}

static uint32_t ConnectReverseStreamActionConfirm(PassiveBuildAction* baseAction, const Bdt_PackageWithRef* resp)
{
	ConnectReverseStreamAction* action = (ConnectReverseStreamAction*)baseAction;
	bool tcpEstablished = false;
	BfxThreadLockLock(&action->tcpEstablishedLock);
	do
	{
		tcpEstablished = action->tcpEstablished;
		action->confirmed = true;
	} while (false);
	BfxThreadLockUnlock(&action->tcpEstablishedLock);

	uint32_t result = BFX_RESULT_SUCCESS;
	if (tcpEstablished)
	{
		BLOG_DEBUG("%s tcp established when confirm, send ack", action->name);
		result = ConnectReverseStreamActionSendAck(action);
	}
	
	return BFX_RESULT_SUCCESS;
}

static void ConnectReverseStreamActionOnTcpError(Bdt_TcpInterfaceOwner* owner, Bdt_TcpInterface* tcpInterface, uint32_t error)
{
	ConnectReverseStreamAction* action = ConnectReverseStreamActionFromInterfaceOwner(owner);
	Bdt_TcpInterfaceReset(action->tcpInterface);
	BuildActionReject((BuildAction*)action, BFX_RESULT_FAILED);
}

static void ConnectReverseStreamActionExecute(BuildAction* baseAction, uint32_t preError)
{
	ConnectReverseStreamAction* action = (ConnectReverseStreamAction*)baseAction;
	Bdt_TcpInterface* tcpInterface = NULL;
	Bdt_NetManagerCreateTcpInterface(
		action->runtime->builder->netManager,
		&action->local,
		&action->remote,
		&tcpInterface);
	assert(tcpInterface);
	action->tcpInterface = tcpInterface;
	Bdt_TcpInterfaceSetOwner(tcpInterface, ConnectReverseStreamActionAsInterfaceOwner(action));
	uint32_t result = Bdt_TcpInterfaceConnect(tcpInterface);
	if (result != BFX_RESULT_SUCCESS)
	{
		ConnectReverseStreamActionOnTcpError(ConnectReverseStreamActionAsInterfaceOwner(action), tcpInterface, result);
	}
}

static void ConnectReverseStreamActionOnCanceled(BuildAction* baseAction)
{
	ConnectReverseStreamAction* action = (ConnectReverseStreamAction*)baseAction;
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceReset(action->tcpInterface);
	}
}

static void ConnectReverseStreamActionUninit(BuildAction* baseAction)
{
	ConnectReverseStreamAction* action = (ConnectReverseStreamAction*)baseAction;
	if (action->tcpInterface)
	{
		Bdt_TcpInterfaceRelease(action->tcpInterface);
	}
	BfxThreadLockDestroy(&action->tcpEstablishedLock);
	BfxFree(action->name);
}

static void ConnectReverseStreamActionOnResolved(BuildAction* baseAction, void* userData)
{
	ConnectReverseStreamAction* action = (ConnectReverseStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		&action->local,
		&action->remote,
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void ConnectReverseStreamActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	ConnectReverseStreamAction* action = (ConnectReverseStreamAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		&action->local,
		&action->remote,
		baseAction,
		error
	);
}

static int32_t ConnectReverseStreamActionAsTcpInterfaceOwnerAddRef(Bdt_TcpInterfaceOwner* owner)
{
	ConnectReverseStreamAction* action = ConnectReverseStreamActionFromInterfaceOwner(owner);
	return BuildActionAddRef((BuildAction*)action);
}

static int32_t ConnectReverseStreamActionAsTcpInterfaceOwnerRelease(Bdt_TcpInterfaceOwner* owner)
{
	ConnectReverseStreamAction* action = ConnectReverseStreamActionFromInterfaceOwner(owner);
	return BuildActionRelease((BuildAction*)action);
}

static void ConnectReverseStreamActionInitAsTcpInterfaceOwner(ConnectReverseStreamAction* action)
{
	action->addRef = ConnectReverseStreamActionAsTcpInterfaceOwnerAddRef;
	action->release = ConnectReverseStreamActionAsTcpInterfaceOwnerRelease;
	action->onEstablish = ConnectReverseStreamActionOnTcpEstablish;
	action->onError = ConnectReverseStreamActionOnTcpError;
	action->onFirstResp = ConnectReverseStreamActionOnFirstResp;
}


static ConnectReverseStreamAction* CreateConnectReverseStreamAction(
	BuildActionRuntime* runtime,
	const BdtEndpoint* local,
	const BdtEndpoint* remote
)
{
	ConnectReverseStreamAction* action = BFX_MALLOC_OBJ(ConnectReverseStreamAction);
	memset(action, 0, sizeof(ConnectReverseStreamAction));
	BuildActionInit((BuildAction*)action, runtime);
	BfxThreadLockInit(&action->tcpEstablishedLock);
	action->local = *local;
	action->remote = *remote;
	action->execute = ConnectReverseStreamActionExecute;
	action->uninit = ConnectReverseStreamActionUninit;
	action->onCanceled = ConnectReverseStreamActionOnCanceled;
	action->confirm = ConnectReverseStreamActionConfirm;

	ConnectReverseStreamActionInitAsTcpInterfaceOwner(action);


	const char* prename = "ConnectReverseStreamAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + BDT_ENDPOINT_STRING_MAX_LENGTH + strlen(epsplit) + BDT_ENDPOINT_STRING_MAX_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char localstr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(local, localstr);
	char remotestr[BDT_ENDPOINT_STRING_MAX_LENGTH + 1];
	BdtEndpointToString(remote, remotestr);
	sprintf(name, "%s%s%s%s", prename, localstr, epsplit, remotestr);
	action->name = name;

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = ConnectReverseStreamActionOnResolved;
	callbacks.reject.function.function = ConnectReverseStreamActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime, 
		local, 
		remote, 
		(BuildAction*)action, 
		&callbacks, 
		false);

	return action;
}


static uint32_t ConnectPackageConnectionActionBeginSyn(ConnectPackageConnectionAction* action)
{
	//begin send session data with syn from actived udp tunnel
	BLOG_DEBUG("%s begin send syn", BuildActionGetName((BuildAction*)action));
	action->beginSyn = true;
	return BFX_RESULT_SUCCESS;
}

static uint32_t ConnectPackageConnectionActionOnSessionData(
	ConnectPackageConnectionAction* action, 
	const Bdt_SessionDataPackage* sessionData)
{
	if (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN)
	{
		//On ack
		if (sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_ACK)
		{
			BLOG_DEBUG("%s got session data with ack flag", BuildActionGetName((BuildAction*)action));

			Bdt_ConnectionFireFirstAnswer(
				action->runtime->builder->connection, 
				sessionData->synPart->fromSessionId, 
				sessionData->payload, 
				sessionData->payloadLength);
			BuildActionRuntimeActiveBuildingTunnel(
				action->runtime,
				BuildActionGetPackageConnectionEndpoint(),
				BuildActionGetPackageConnectionEndpoint(),
				(BuildAction*)action
			);

			return BFX_RESULT_SUCCESS;
		}
	}
	return BFX_RESULT_INVALID_PARAM;
}

static uint32_t ConnectPackageConnectionActionContinueConnect(ConnectConnectionAction* action)
{
	// send ackack with session data should be implement in PackageConnection
	uint32_t result = Bdt_ConnectionTryEnterEstablish(action->runtime->builder->connection);
	if (result == BFX_RESULT_SUCCESS)
	{
		result = Bdt_ConnectionEstablishWithPackage(action->runtime->builder->connection, NULL);
	}
	
	if (result == BFX_RESULT_SUCCESS)
	{
		BuildActionResolve((BuildAction*)action);
	}
	else
	{
		BuildActionReject((BuildAction*)action, result);
	}
	return result;
}

static void ConnectPackageConnectionActionOnTick(void* userData)
{
	ConnectPackageConnectionAction* action = (ConnectPackageConnectionAction*)userData;
	if (action->state != BUILD_ACTION_STATE_EXECUTED)
	{
		return;
	}
	if (!action->beginSyn)
	{
		return;
	}
	const Bdt_PackageWithRef* pkg = action->synPackage;
	const Bdt_Package* toSend = Bdt_PackageWithRefGet(pkg);
	size_t count = 1;

	BLOG_DEBUG("%s send session data with syn flag", BuildActionGetName((BuildAction*)action));
	BdtTunnel_SendParams sendParams =
	{
		.flags = BDT_TUNNEL_SEND_FLAGS_UDP_ONLY, 
		.buildParams = NULL
	};
	uint32_t result = BdtTunnel_Send(
		action->runtime->builder->container, 
		&toSend, 
		&count, 
		&sendParams);
}

static void ConnectPackageConnectionActionExecute(BuildAction* baseAction, uint32_t preError)
{
	// do nothing
}

static void ConnectPackageConnectionActionOnCanceled(BuildAction* baseAction)
{

}

static void ConnectPackageConnectionActionUninit(BuildAction* baseAction)
{
	ConnectPackageConnectionAction* action = (ConnectPackageConnectionAction*)baseAction;
	Bdt_PackageRelease(action->synPackage);

	BfxFree(action->name);
}



static void ConnectPackageConnectionActionOnResolved(BuildAction* baseAction, void* userData)
{
	ConnectPackageConnectionAction* action = (ConnectPackageConnectionAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		BuildActionGetPackageConnectionEndpoint(),
		BuildActionGetPackageConnectionEndpoint(),
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void ConnectPackageConnectionActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	ConnectPackageConnectionAction* action = (ConnectPackageConnectionAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		BuildActionGetPackageConnectionEndpoint(),
		BuildActionGetPackageConnectionEndpoint(),
		baseAction,
		error
	);
}

static ConnectPackageConnectionAction* CreateConnectPackageConnectionAction(
	BuildActionRuntime* runtime, 
	BuildTimeoutAction* timeoutAction)
{
	ConnectPackageConnectionAction* action = BFX_MALLOC_OBJ(ConnectPackageConnectionAction);
	memset(action, 0, sizeof(ConnectPackageConnectionAction));
	ConnectConnectionActionInit((ConnectConnectionAction*)action, runtime);
	action->execute = ConnectPackageConnectionActionExecute;
	action->uninit = ConnectPackageConnectionActionUninit;
	action->onCanceled = ConnectPackageConnectionActionOnCanceled;
	action->continueConnect = ConnectPackageConnectionActionContinueConnect;

	action->synPackage = Bdt_ConnectionGenSynPackage(action->runtime->builder->connection);
	action->beginSyn = false;

	const char* prename = "ConnectPackageConnectionAction";
	size_t prenameLen = strlen(prename);
	size_t nameLen = prenameLen + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	sprintf(name, "%s", prename);
	action->name = name;

	BfxUserData udTick;
	BuildActionAsUserData((BuildAction*)action, &udTick);
	BuildTimeoutActionAddTickAction(
		timeoutAction,
		ConnectPackageConnectionActionOnTick,
		&udTick);

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = ConnectPackageConnectionActionOnResolved;
	callbacks.reject.function.function = ConnectPackageConnectionActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime, 
		BuildActionGetPackageConnectionEndpoint(), 
		BuildActionGetPackageConnectionEndpoint(), 
		(BuildAction*)action, 
		&callbacks, 
		false);

	return action;
}


static uint32_t AcceptPackageConnectionActionBeginAck(AcceptPackageConnectionAction* action)
{
	//begin send session data with syn from actived udp tunnel
	BLOG_DEBUG("%s begin send ack", BuildActionGetName((BuildAction*)action));
	action->beginAck = true;
	return BFX_RESULT_SUCCESS;
}

static uint32_t AcceptPackageConnectionActionOnSessionData(
	AcceptPackageConnectionAction* action,
	const Bdt_SessionDataPackage* sessionData)
{
	if (!(sessionData->cmdflags & BDT_SESSIONDATA_PACKAGE_FLAG_SYN))
	{
		BLOG_DEBUG("%s got session data", BuildActionGetName((BuildAction*)action));
		uint32_t result = Bdt_ConnectionTryEnterEstablish(action->runtime->builder->connection);
		if (result == BFX_RESULT_SUCCESS)
		{
			result = Bdt_ConnectionEstablishWithPackage(action->runtime->builder->connection, sessionData);
		}
			
		if (result == BFX_RESULT_SUCCESS)
		{
			BuildActionResolve((BuildAction*)action);
		}
		else
		{
			BuildActionReject((BuildAction*)action, result);
		}
		return result;
	}
	else
	{
		return BFX_RESULT_INVALID_PARAM;
	}
}

static uint32_t AcceptPackageConnectionActionConfirm(PassiveBuildAction* baseAction, const Bdt_PackageWithRef* resp)
{
	AcceptPackageConnectionAction* action = (AcceptPackageConnectionAction*)baseAction;
	BfxAtomicExchangePointer((void**)&action->ackPackage, (void*)resp);
	Bdt_PackageAddRef(resp);
	return BFX_RESULT_SUCCESS;
}

static void AcceptPackageConnectionActionOnTick(void* userData)
{
	AcceptPackageConnectionAction* action = (AcceptPackageConnectionAction*)userData;
	if (action->state != BUILD_ACTION_STATE_EXECUTED)
	{
		return;
	}
	if (!action->ackPackage)
	{
		return;
	}
	if (!action->beginAck)
	{
		return;
	}
	const Bdt_PackageWithRef* pkg = action->ackPackage;
	const Bdt_Package* toSend = Bdt_PackageWithRefGet(pkg);
	size_t count = 1;

	BLOG_DEBUG("%s send session data with ack flag", BuildActionGetName((BuildAction*)action));
	BdtTunnel_SendParams sendParams =
	{
		.flags = BDT_TUNNEL_SEND_FLAGS_UDP_ONLY,
		.buildParams = NULL
	};
	uint32_t result = BdtTunnel_Send(
		action->runtime->builder->container, 
		&toSend, 
		&count, 
		&sendParams);
}

static void AcceptPackageConnectionActionExecute(BuildAction* baseAction, uint32_t preError)
{
	// do nothing
}

static void AcceptPackageConnectionActionOnCanceled(BuildAction* baseAction)
{
	// do nothing
}

static void AcceptPackageConnectionActionUninit(BuildAction* baseAction)
{
	AcceptPackageConnectionAction* action = (AcceptPackageConnectionAction*)baseAction;
	if (action->ackPackage)
	{
		Bdt_PackageRelease(action->ackPackage);
	}

	BfxFree(action->name);
}

static void AcceptPackageConnectionActionOnResolved(BuildAction* baseAction, void* userData)
{
	AcceptPackageConnectionAction* action = (AcceptPackageConnectionAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		BuildActionGetPackageConnectionEndpoint(),
		BuildActionGetPackageConnectionEndpoint(),
		baseAction,
		BFX_RESULT_SUCCESS
	);
}

static void AcceptPackageConnectionActionOnRejected(BuildAction* baseAction, void* userData, uint32_t error)
{
	AcceptPackageConnectionAction* action = (AcceptPackageConnectionAction*)baseAction;
	BuildActionRuntimeFinishBuildingTunnel(
		action->runtime,
		BuildActionGetPackageConnectionEndpoint(),
		BuildActionGetPackageConnectionEndpoint(),
		baseAction,
		error
	);
}

static AcceptPackageConnectionAction* CreateAcceptPackageConnectionAction(
	BuildActionRuntime* runtime,
	BuildTimeoutAction* timeoutAction)
{
	AcceptPackageConnectionAction* action = BFX_MALLOC_OBJ(AcceptPackageConnectionAction);
	memset(action, 0, sizeof(AcceptPackageConnectionAction));
	BuildActionInit((BuildAction*)action, runtime);
	action->execute = AcceptPackageConnectionActionExecute;
	action->uninit = AcceptPackageConnectionActionUninit;
	action->onCanceled = AcceptPackageConnectionActionOnCanceled;

	action->confirm = AcceptPackageConnectionActionConfirm;

	action->beginAck = false;

	const char* prename = "AcceptPackageConnectionAction";
	size_t prenameLen = strlen(prename);
	size_t nameLen = prenameLen + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	sprintf(name, "%s", prename);
	action->name = name;

	BfxUserData udTick;
	BuildActionAsUserData((BuildAction*)action, &udTick);
	BuildTimeoutActionAddTickAction(
		timeoutAction,
		AcceptPackageConnectionActionOnTick,
		&udTick);

	BuildActionCallbacks callbacks;
	memset(&callbacks, 0, sizeof(BuildActionCallbacks));
	callbacks.flags = BUILD_ACTION_CALLBACK_RESOLVE_FUNCTION | BUILD_ACTION_CALLBACK_REJECT_FUNCTION;
	callbacks.resolve.function.function = AcceptPackageConnectionActionOnResolved;
	callbacks.reject.function.function = AcceptPackageConnectionActionOnRejected;

	uint32_t result = BuildActionRuntimeAddBuildingTunnelAction(
		runtime,
		BuildActionGetPackageConnectionEndpoint(),
		BuildActionGetPackageConnectionEndpoint(),
		(BuildAction*)action,
		&callbacks, 
		false);

	return action;
}


struct SnCallAction
{
	DEFINE_BUILD_ACTION()
	BdtPeerid snPeerid;
	BdtSnClient_CallSession* session;
	bool updateRemoteInfo;
};

static void SnCallActionSessionCallback(
	uint32_t errorCode,
	BdtSnClient_CallSession* session,
	const BdtPeerInfo* remotePeerInfo,
	void* userData)
{
	SnCallAction* action = (SnCallAction*)userData;
	if (errorCode)
	{
		BLOG_DEBUG("%s got error %u", action->name, errorCode);
		BuildActionReject((BuildAction*)action, errorCode);
		return;
	}
	if (action->updateRemoteInfo)
	{
		BuildActionRuntimeUpdateRemoteInfo(action->runtime, remotePeerInfo);
	}
	BuildActionResolve((BuildAction*)action);
}

static void SnCallActionExecute(BuildAction* baseAction, uint32_t preError)
{
	SnCallAction* action = (SnCallAction*)baseAction;
	TunnelBuilder* builder = action->runtime->builder;

	const BdtPeerInfo* snInfo = BdtPeerFinderGetCachedOrStatic(builder->peerFinder, &action->snPeerid);
	if (!snInfo)
	{
		BLOG_DEBUG("%s get cached sn peer info failed", BuildActionGetName((BuildAction*)action));
		BuildActionReject(baseAction, BFX_RESULT_NOT_FOUND);
		return;
	}

	size_t packageLength = 0;
	const Bdt_PackageWithRef** synPackages = TunnelBuilderGetFirstPackages(builder, &packageLength);

	BdtSnClient_CallSession* session = NULL;
	BfxUserData userData;
	BuildActionAsUserData(baseAction, &userData);

	Bdt_PackageWithRef* callPkg = NULL;
	uint32_t result = BdtSnClient_CreateCallSession(
		builder->snClient,
		((Bdt_SynTunnelPackage*)Bdt_PackageWithRefGet(synPackages[0]))->peerInfo,
		BdtTunnel_GetRemotePeerid(builder->container),
		NULL,
		0,
		snInfo,
		false,
		true,
		SnCallActionSessionCallback,
		&userData,
		&session,
		&callPkg);
	BdtReleasePeerInfo(snInfo);
	BfxAtomicExchangePointer(&action->session, session);

	Bdt_StackTlsData* tlsData = Bdt_StackTlsGetData(builder->tls);
	size_t encodeLength = sizeof(tlsData->udpEncodeBuffer);

	Bdt_TunnelEncryptOptions encrypt;
	result = BdtTunnel_GetEnryptOptions(
		builder->container,
		&encrypt
	);
	if (encrypt.exchange)
	{
		const Bdt_Package* firstPackage = Bdt_PackageWithRefGet(synPackages[0]);
		Bdt_ExchangeKeyPackage exchange;
		Bdt_ExchangeKeyPackageInit(&exchange);
		Bdt_ExchangeKeyPackageFillWithNext(&exchange, firstPackage);
		exchange.sendTime = 0;

		const Bdt_Package* packages[TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH];
		packages[0] = (Bdt_Package*)&exchange;
		for (size_t ix = 0; ix < packageLength; ++ix)
		{
			packages[ix + 1] = Bdt_PackageWithRefGet(synPackages[ix]);
		}
		result = Bdt_BoxEncryptedUdpPackageStartWithExchange(
			packages,
			packageLength + 1,
			Bdt_PackageWithRefGet(callPkg),
			encrypt.key,
			encrypt.remoteConst->publicKeyType,
			(const uint8_t*)&encrypt.remoteConst->publicKey,
			encrypt.localSecret->secretLength,
			(const uint8_t*)&encrypt.localSecret->secret,
			tlsData->udpEncodeBuffer,
			&encodeLength
		);
		Bdt_ExchangeKeyPackageFinalize(&exchange);
	}
	else
	{
		const Bdt_Package* packages[TUNNEL_BUILDER_FIRST_PACKAGES_MAX_LENGTH];
		for (size_t ix = 0; ix < packageLength; ++ix)
		{
			packages[ix] = Bdt_PackageWithRefGet(synPackages[ix]);
		}
		result = Bdt_BoxEncryptedUdpPackage(
			packages,
			packageLength,
			Bdt_PackageWithRefGet(callPkg),
			encrypt.key,
			tlsData->udpEncodeBuffer,
			&encodeLength
		);
	}
	BFX_BUFFER_HANDLE payload = NULL;
	if (result)
	{
		BLOG_DEBUG("%s %u box sn call payload failed for %u", Bdt_TunnelContainerGetName(builder->container), builder->seq, result);
		assert(false);
		payload = BfxCreateBuffer(0);
	}
	else
	{
		payload = BfxCreateBuffer(encodeLength);
		BfxBufferWrite(payload, 0, encodeLength, tlsData->udpEncodeBuffer, 0);
	}
	
	BLOG_DEBUG("%s send sn call", BuildActionGetName((BuildAction*)action));
	BdtSnClient_CallSessionStart(session, payload);
	BfxBufferRelease(payload);
}

static void SnCallActionUninit(BuildAction* baseAction)
{
	SnCallAction* action = (SnCallAction*)baseAction;
	if (action->session)
	{
		BdtSnClient_CallSessionRelease(action->session);
		action->session = NULL;
	}
	BfxFree(action->name);
}

static SnCallAction* CreateSnCallAction(
	BuildActionRuntime* runtime,
	const BdtPeerid* snPeerid,
	bool updateRemoteInfo)
{
	SnCallAction* action = BFX_MALLOC_OBJ(SnCallAction);
	memset(action, 0, sizeof(SnCallAction));
	BuildActionInit((BuildAction*)action, runtime);
	action->snPeerid = *snPeerid;
	action->updateRemoteInfo = updateRemoteInfo;
	action->execute = SnCallActionExecute;
	action->uninit = SnCallActionUninit;

	const char* prename = "SnCallAction";
	size_t prenameLen = strlen(prename);
	const char* epsplit = " to ";
	size_t nameLen = prenameLen + strlen(epsplit) + BDT_PEERID_STRING_LENGTH + 1;
	char* name = BFX_MALLOC_ARRAY(char, nameLen);
	memset(name, 0, nameLen);
	char snstr[BDT_PEERID_STRING_LENGTH + 1];
	BdtPeeridToString(snPeerid, snstr);
	sprintf(name, "%s%s%s", prename, epsplit, snstr);
	action->name = name;

	return action;
}


#endif //__BDT_TUNNEL_BUILD_ACTION_IMP_INL__
