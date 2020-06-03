#include "./Stack.h"
#include <BuckyFramework/framework.h>
#include "./Global.h"
#include "./PeerInfo.h"
#include "./Connection.h"
#include "./NetModifier.h"

Napi::FunctionReference Stack::s_constructor;

Stack::Stack(const Napi::CallbackInfo &cb)
		: Napi::ObjectWrap<Stack>(cb)
		, m_stack(NULL)
{
	BuckyFrameworkOptions options;
	options.size = sizeof(BuckyFrameworkOptions);
	options.tcpThreadCount = 1;
    options.udpThreadCount = 1;
    options.timerThreadCount = 1;
    options.dispatchThreadCount = 1;

	BdtSystemFrameworkEvents events;
	events.bdtPushTcpSocketEvent = BdtPushTcpSocketEvent;
	events.bdtPushTcpSocketData = BdtPushTcpSocketData;
	events.bdtPushUdpSocketData = BdtPushUdpSocketData;

	BfxUserData userData;
	userData.userData = NULL;
	userData.lpfnAddRefUserData = NULL;
	userData.lpfnReleaseUserData = NULL;

	Napi::Object cbParams = cb[0].As<Napi::Object>();

	const BdtPeerInfo* localPeer = PeerInfo::FromObject(cbParams.Get("local"));
	BdtSystemFramework* framework = createBuckyFramework(&events, &options);

	const BdtPeerInfo** initial = NULL;
	size_t initialLen = 0;
	Napi::Value objInitial = cbParams.Get("initialPeers");
	if (objInitial.IsArray() && objInitial.As<Napi::Array>().Length())
	{
		initial = new const BdtPeerInfo*[objInitial.As<Napi::Array>().Length()];
		for (uint32_t ix = 0; ix < objInitial.As<Napi::Array>().Length(); ++ix)
		{
			const BdtPeerInfo* peer = PeerInfo::FromObject(objInitial.As<Napi::Object>()[ix]);
			if (peer)
			{
				*(initial + initialLen) = peer;
				++initialLen;
			}
		}
	}

	BdtCreateStack(framework, localPeer, initial, initialLen, NULL, Stack::OnStackEvent, &userData, &this->m_stack);
	BdtReleasePeerInfo(localPeer);
	for (size_t ix = 0; ix < initialLen; ++ix)
	{
		BdtReleasePeerInfo(*(initial + ix));
	}
	if (initial)
	{
		delete initial;
	}
}

void Stack::OnStackEvent(
	BdtStack* stack, 
	uint32_t eventId, 
	const void* eventParams, 
	void* userData)
{
	
}


// BDT_API(uint32_t) BdtCreateConnection(
// 	BdtStack* stack,
// 	const BdtPeerid* remotePeerid, 
// 	bool encrypted, 
// 	BdtConnection** outConnection);
Napi::Value Stack::ConnectRemote(const Napi::CallbackInfo &cb)
{
	Napi::Object cbParams = cb[0].As<Napi::Object>();
    const BdtPeerid* remotePeerid = Peerid::FromObject(cbParams.Get("remote"));
    BdtPeerid remotePeeridTmp;
    if (remotePeerid == NULL)
    {
        remotePeerid = &remotePeeridTmp;
        remotePeerid = PeerConstInfo::ToPeerid(cb[0].As<Napi::Object>().Get("constInfo"), &remotePeeridTmp);
    }

	BdtConnection* connection = NULL;
	uint32_t result = BdtCreateConnection(
		this->m_stack, 
        remotePeerid,
		true, 
		&connection);
	if (connection)
	{
		Napi::Value wrap = Connection::Wrap(cb.Env(), connection);
		BdtReleaseConnection(connection);

        Connection* wrapedConnection = Connection::Unwrap(wrap.ToObject());
        return wrapedConnection->Connect(cb);
	}
	else
	{
		return Napi::Value();
	}
}


// BDT_API(uint32_t) BdtListenConnection(
// 	BdtStack* stack, 
// 	uint16_t port, 
// 	const BdtAcceptConnectionConfig* config);
Napi::Value Stack::ListenConnection(const Napi::CallbackInfo &cb)
{
	Napi::Object cbParams = cb[0].As<Napi::Object>();
    uint16_t vport = (uint16_t)(static_cast<uint32_t>(cbParams.Get("vport").ToNumber()));
	uint32_t result = BdtListenConnection(this->m_stack, vport, NULL);
	return BdtResult::ToObject(cb.Env(), result);
}


OnAcceptConnection::OnAcceptConnection(Napi::Env env, Napi::Function cb)
: AsyncCallbackOnce(env, cb, "OnAcceptConnection")
, m_data(NULL)
, m_recv(0)
, m_connection(NULL)
{

}

BdtAcceptConnectionCallback OnAcceptConnection::ToCFunction()
{
	return CCallback;
}

void BFX_STDCALL OnAcceptConnection::CCallback(
	uint32_t result, 
	const uint8_t* buffer, 
	size_t recv, 
	BdtConnection* connection, 
	void* userData)
{
	OnAcceptConnection* cb = reinterpret_cast<OnAcceptConnection*>(userData);
    cb->AddRef();
    cb->m_data = buffer;
    cb->m_recv = recv;
	cb->m_connection = connection;
	BdtAddRefConnection(connection);
    cb->m_tsfn.BlockingCall(cb, JsCallback);
}

void OnAcceptConnection::JsCallback(
	Napi::Env env, 
	Napi::Function callback, 
	OnAcceptConnection* cb)
{
	Napi::Value fq;
	if (cb->m_recv)
	{
		fq = Napi::Buffer<uint8_t>::New(env, cb->m_recv);
		::memcpy(fq.As<Napi::Buffer<uint8_t> >().Data(), cb->m_data, cb->m_recv);
	}
	else
	{
		fq = Napi::Buffer<uint8_t>::New(env, 0);
	}
	callback.Call({Connection::Wrap(env, cb->m_connection), fq});
	BdtReleaseConnection(cb->m_connection);
	cb->Release();
}
// typedef void (BFX_STDCALL* BdtAcceptConnectionCallback)(
// 	uint32_t result, 
// 	const uint8_t* buffer, 
// 	size_t recv, 
// 	BdtConnection* connection, 
// 	void* userData);
// BDT_API(uint32_t) BdtAcceptConnection(
// 	BdtStack* stack,
// 	uint16_t port, 
// 	BdtAcceptConnectionCallback callback,
// 	const BfxUserData* userData
// );
Napi::Value Stack::AcceptConnection(const Napi::CallbackInfo &cb)
{
	Napi::Object cbParams = cb[0].As<Napi::Object>();
    uint16_t vport = (uint16_t)(static_cast<uint32_t>(cbParams.Get("vport").ToNumber()));

	OnAcceptConnection* callback = new OnAcceptConnection(cb.Env(), cbParams.Get("onAccept").As<Napi::Function>());
    BfxUserData userData;
    callback->ToBfxUserData(&userData);

	uint32_t result = BdtAcceptConnection(
		this->m_stack, 
		vport, 
		callback->ToCFunction(), 
		&userData
	);

    callback->Release();

	return BdtResult::ToObject(cb.Env(), result);
}

Napi::Value Stack::AddStaticPeerInfo(const Napi::CallbackInfo &cb)
{
	Napi::Object peerInfoJs = cb[0].As<Napi::Object>();

    const BdtPeerInfo* peerInfo = PeerInfo::FromObject(peerInfoJs);
    if (peerInfo == NULL)
    {
        return cb.Env().Undefined();
    }

    BdtAddStaticPeerInfo(this->m_stack, peerInfo);
    BdtReleasePeerInfo(peerInfo);
    return cb.Env().Undefined();
}

Napi::Value Stack::CreateNetModifier(const Napi::CallbackInfo& cb)
{
	BdtNetModifier* modifier = BdtCreateNetModifier(this->m_stack);
	return NetModifier::Wrap(cb.Env(), this->m_stack, modifier);
}


Napi::Object Stack::Register(
	Napi::Env env, 
	Napi::Object exports)
{
	Napi::Function func = DefineClass(
		env, 
		"Stack", 
		{
			InstanceMethod("connectRemote", &Stack::ConnectRemote),
			InstanceMethod("listenConnection", &Stack::ListenConnection),
			InstanceMethod("acceptConnection", &Stack::AcceptConnection),
			InstanceMethod("addStaticPeerInfo", &Stack::AddStaticPeerInfo),
			InstanceMethod("createNetModifier", &Stack::CreateNetModifier),
		});
	
	s_constructor = Napi::Persistent(func);
	s_constructor.SuppressDestruct();
	
	exports.Set("Stack", func);
	return exports;
}