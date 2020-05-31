#include "./NetModifier.h"
#include "./Global.h"
#include "./PeerInfo.h"

Napi::FunctionReference NetModifier::s_constructor;

Napi::Value NetModifier::Wrap(
    Napi::Env env,
    BdtStack* stack, 
    BdtNetModifier* modifier)
{
    return s_constructor.New({ 
        Napi::External<BdtStack>::New(env, stack), 
        Napi::External<BdtNetModifier>::New(env, modifier), 
    });
}

Napi::Object NetModifier::Register(Napi::Env env, Napi::Object exports)
{
    Napi::Function func = DefineClass(
        env,
        "NetModifier",
        {
            InstanceMethod("changeLocalAddress", &NetModifier::ChangeLocalAddress),
            InstanceMethod("apply", &NetModifier::Apply),
		});
	
	s_constructor = Napi::Persistent(func);
	s_constructor.SuppressDestruct();
	
	exports.Set("NetModifier", func);
	return exports;
}

NetModifier::NetModifier(const Napi::CallbackInfo &cb)
: Napi::ObjectWrap<NetModifier>(cb)
, m_stack(NULL)
, m_modifier(NULL)
{
    this->m_stack = cb[0].As<Napi::External<BdtStack> >().Data();
    this->m_modifier = cb[1].As<Napi::External<BdtNetModifier> >().Data();
}

Napi::Value NetModifier::ChangeLocalAddress(const Napi::CallbackInfo& cb)
{
    Napi::Object cbParams = cb[0].As<Napi::Object>();
    Napi::Value objSrc = cbParams.Get("src");
    BdtEndpoint insSrc;
    const BdtEndpoint* src = Endpoint::FromString(objSrc, &insSrc);
    Napi::Value objDst = cbParams.Get("dst");
    BdtEndpoint insDst;
    const BdtEndpoint* dst = Endpoint::FromString(objDst, &insDst);
    uint32_t result = BdtChangeLocalAddress(this->m_modifier, src, dst);
    return BdtResult::ToObject(cb.Env(), result);
}

Napi::Value NetModifier::Apply(const Napi::CallbackInfo& cb)
{
    uint32_t result = BdtApplyBdtNetModifier(this->m_stack, this->m_modifier);
    return BdtResult::ToObject(cb.Env(), result);
}