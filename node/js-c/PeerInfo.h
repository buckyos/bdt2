#pragma once
#include <napi.h>
#include <BdtCore.h>

class Endpoint
{
    public:
        // static const BdtEndpoint* FromObject(const Napi::Value value, BdtEndpoint* ep);
        static const BdtEndpoint* FromString(const Napi::Value value, BdtEndpoint* ep);
};


class Peerid
{
    public:
        static const BdtPeerid* FromObject(const Napi::Value value, BdtPeerid* peerid = NULL);
        static Napi::Value ToObject(Napi::Env env, const BdtPeerid* peerid);
};

class PeerArea
{
    public:
        static const BdtPeerArea* FromObject(const Napi::Value value, BdtPeerArea* area);
        static Napi::Value ToObject(Napi::Env env, const BdtPeerArea* area);
};

class PeerConstInfo
: public Napi::ObjectWrap<PeerConstInfo>
{
    public:
        static Napi::Object Register(Napi::Env env, Napi::Object exports);
        PeerConstInfo(const Napi::CallbackInfo &info);

        static const BdtPeerid* ToPeerid(const Napi::Value& jsValue, BdtPeerid* peerid);

    private:
        static Napi::Value ToPeerid(const Napi::CallbackInfo &info);
        static Napi::FunctionReference s_constructor;
    public: 
        static const BdtPeerConstInfo* FromObject(const Napi::Value value, BdtPeerConstInfo* info);
        static Napi::Value ToObject(Napi::Env env, const BdtPeerConstInfo* constInfo);
};

// typedef struct BdtBuildTunnelParams
// {
// 	const BdtPeerConstInfo* remoteConstInfo;
// 	int32_t flags;
// 	const uint8_t* key;
// 	const BdtEndpoint* localEndpoint;
// 	const BdtEndpoint* remoteEndpoint;
// 	const BdtPeerid* snList;
// 	size_t snListLength;
// } BdtBuildTunnelParams;
class BuildTunnelParams
{
    public:
		static BdtBuildTunnelParams* FromObject(const Napi::Value value);
        static void Destroy(BdtBuildTunnelParams* params);
};

class PeerInfo 
{
	public:
		static const BdtPeerInfo* FromObject(const Napi::Value value);
        static Napi::Value ToObject(Napi::Env env, const BdtPeerInfo* peerinfo);
};