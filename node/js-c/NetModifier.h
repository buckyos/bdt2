#pragma once
#include <napi.h>
#include <BdtCore.h>

class NetModifier
	: public Napi::ObjectWrap<NetModifier> 
{
	public:
        static Napi::Value Wrap(
            Napi::Env env,
            BdtStack* stack,  
            BdtNetModifier* modifier);
		static Napi::Object Register(Napi::Env env, Napi::Object exports);
		NetModifier(const Napi::CallbackInfo &cb);
	private:
        Napi::Value ChangeLocalAddress(const Napi::CallbackInfo& cb);
        Napi::Value Apply(const Napi::CallbackInfo& cb);
		static Napi::FunctionReference s_constructor;
    private:
        BdtStack* m_stack;
        BdtNetModifier* m_modifier;
};