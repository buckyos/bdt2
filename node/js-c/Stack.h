#pragma once
#include <napi.h>
#include <BdtCore.h>
#include "./Global.h"

class OnAcceptConnection
: public AsyncCallbackOnce
{
	public:
        OnAcceptConnection(Napi::Env env, Napi::Function cb);
        BdtAcceptConnectionCallback ToCFunction();
    private:
        static void BFX_STDCALL CCallback(uint32_t result, 
			const uint8_t* buffer, 
			size_t recv, 
			BdtConnection* connection, 
			void* userData);
        static void JsCallback(Napi::Env env, Napi::Function callback, OnAcceptConnection* cb);
    private:
        const uint8_t* m_data;
		size_t m_recv;
		BdtConnection* m_connection;
};

class Stack
	: public Napi::ObjectWrap<Stack> 
{
	public:
		static Napi::Object Register(Napi::Env env, Napi::Object exports);
		Stack(const Napi::CallbackInfo &cb);
	private:
		Napi::Value ConnectRemote(const Napi::CallbackInfo &cb);
		Napi::Value ListenConnection(const Napi::CallbackInfo &cb);
		Napi::Value AcceptConnection(const Napi::CallbackInfo &cb);
        Napi::Value AddStaticPeerInfo(const Napi::CallbackInfo& cb);
		Napi::Value CreateNetModifier(const Napi::CallbackInfo& cb);
        static Napi::FunctionReference s_constructor;
	private:
		static void OnStackEvent(BdtStack* stack, uint32_t eventId, const void* eventParams, void* userData);
	private:
		BdtStack* m_stack;
};
