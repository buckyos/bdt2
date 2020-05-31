#include "./Global.h"


Napi::Value BdtResult::ToObject(Napi::Env env, uint32_t result)
{
	return Napi::Number::New(env, result);
}

Napi::Object BdtResult::Register(Napi::Env env, Napi::Object exports)
{
	Napi::Object obj = Napi::Object::New(env);
	//TODO: all result
	obj.Set("success", BdtResult::ToObject(env, BFX_RESULT_SUCCESS));
	obj.Set("failed", BdtResult::ToObject(env, BFX_RESULT_FAILED));

	obj.Set("invalidState", BdtResult::ToObject(env, BFX_RESULT_INVALID_STATE));

	obj.Set("pending", BdtResult::ToObject(env, BFX_RESULT_PENDING));

	obj.Set("timeout", BdtResult::ToObject(env, BFX_RESULT_TIMEOUT));

	exports.Set("BdtResult", obj);

	return exports;
}

void AsyncCallbackOnce::Release()
{
	int32_t ref = BfxAtomicDec32(&this->m_ref);
	if (ref <= 0)
	{
		delete this;
	}
}

void AsyncCallbackOnce::AddRef()
{
	BfxAtomicInc32(&this->m_ref);
}

void BFX_STDCALL AsyncCallbackOnce::AddRefAsUserData(void* ud)
{
	reinterpret_cast<AsyncCallbackOnce*>(ud)->AddRef();
}

void BFX_STDCALL AsyncCallbackOnce::ReleaseAsUserData(void* ud)
{
	reinterpret_cast<AsyncCallbackOnce*>(ud)->Release();
}

void AsyncCallbackOnce::ToBfxUserData(BfxUserData* ud)
{
	ud->userData = this;
	ud->lpfnAddRefUserData = AddRefAsUserData;
	ud->lpfnReleaseUserData = ReleaseAsUserData;
}

AsyncCallbackOnce::AsyncCallbackOnce(
	Napi::Env env, 
	Napi::Function cb, 
	const char* name)
	: m_ref(1)
{
	this->m_tsfn = Napi::ThreadSafeFunction::New(env, cb, name, 0, 1);
}

AsyncCallbackOnce::~AsyncCallbackOnce()
{
	this->m_tsfn.Release();
}

RingBuffer::RingBuffer(size_t cap)
	:m_cap(cap)
	,m_readPos(0)
	,m_dataSize(0)
{
	m_buffer = BFX_MALLOC_ARRAY(uint8_t, cap);
}

RingBuffer::~RingBuffer()
{
	BFX_FREE(m_buffer);
	m_buffer = NULL;
}

size_t RingBuffer::Push(const uint8_t* buffer, size_t length, bool bPartPush)
{
	if (!bPartPush && length > Capacity())
	{
		return 0;
	}

	size_t nSaveLen = length > Capacity() ? Capacity() : length;

	if (m_readPos + m_dataSize >= m_cap)
	{
		//剩余空间连续
		memcpy(m_buffer + (m_readPos + m_dataSize - m_cap), buffer, nSaveLen);
	}
	else
	{
		//剩余空间需要跨越首尾
		size_t nTailSize = m_cap - m_readPos - m_dataSize;
		if (nTailSize >= nSaveLen)
		{
			memcpy(m_buffer + (m_readPos + m_dataSize), buffer, nSaveLen);
		}
		else
		{
			memcpy(m_buffer + (m_readPos + m_dataSize), buffer, nTailSize);
			memcpy(m_buffer, buffer + nTailSize, nSaveLen - nTailSize);
		}
	}
	m_dataSize += nSaveLen;

	return nSaveLen;
}

size_t RingBuffer::Pop(uint8_t* buffer, size_t length)
{
	if (0 == m_dataSize)
	{
		return 0;
	}

	size_t nPopSize = (length > m_dataSize) ? m_dataSize : length;

	if (m_readPos + nPopSize <= m_cap)
	{
		//pop的数据连续
		memcpy(buffer, m_buffer + m_readPos, nPopSize);
	}
	else
	{
		size_t nTailSize = m_cap - m_readPos;
		if (nTailSize >= nPopSize)
		{
			memcpy(buffer, m_buffer + m_readPos, nPopSize);
		}
		else
		{
			memcpy(buffer, m_buffer + m_readPos, nTailSize);
			memcpy(buffer + nTailSize, m_buffer, nPopSize - nTailSize);
		}
	}
	m_dataSize -= nPopSize;
	m_readPos = (m_readPos + nPopSize) % m_cap;

	return nPopSize;
}

