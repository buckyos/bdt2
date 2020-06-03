#pragma once
#include <napi.h>
#include <BdtCore.h>

class BdtResult
{
	public:
		static Napi::Value ToObject(Napi::Env env, uint32_t result);
		static Napi::Object Register(Napi::Env env, Napi::Object exports);
};

class AsyncCallbackOnce
{
	public:
		void Release();
		void ToBfxUserData(BfxUserData* ud);
	protected:
		void AddRef();
		AsyncCallbackOnce(Napi::Env env, Napi::Function cb, const char* name);
		virtual ~AsyncCallbackOnce();
    protected:
        Napi::ThreadSafeFunction m_tsfn;
	private:
		static void BFX_STDCALL AddRefAsUserData(void* ud);
		static void BFX_STDCALL ReleaseAsUserData(void* ud);
		volatile int32_t m_ref;
};

class RingBuffer
{
public:
	RingBuffer(size_t cap);
	~RingBuffer();

	size_t Push(const uint8_t* buffer, size_t length, bool bPartPush);
	size_t Pop(uint8_t* buffer, size_t length);
	inline size_t Capacity() const;
	inline size_t DataSize() const;
	inline size_t MaxCapacity() const;

private:
	uint8_t* m_buffer;
	size_t m_cap;
	size_t m_readPos;
	size_t m_dataSize;
};

inline size_t RingBuffer::Capacity() const
{
	return m_cap - m_dataSize;
}

inline size_t RingBuffer::DataSize() const
{
	return m_dataSize;
}

inline size_t RingBuffer::MaxCapacity() const
{
	return m_cap;
}