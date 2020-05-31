#include <BDTCore/BdtCore.h>
#include <BuckyFramework/framework.h>
#include <mbedtls-2.16.1/include/mbedtls/md5.h>

#include "./conn.h"

#define FILE_SIEZ_BYTES sizeof(uint64_t)
#define MD5_LEN	16
#define FILE_NAME_LEN 64
#define CONN_NAME_LEN 64
#define BUFFER_LEN (1024 * 1024)

/*
为了测试发送不同大小得文件，并且保证发送正确，同时为了兼容在内存有限路由器上测试，
所以发送文件得测试随机生成内容，以1M为一个UNIT单位，在大于1M得时候必须是1M得整数倍
每个文件得第一个UNIT得前8个字节为文件大小,紧接着FILE_NAME_LEN为文件名称,最后MD5_LEN
为（1M-MD5_LEN）得内容得md5作为当前块的md5，后面每个UNIT的前MD5_LEN字节为前一块UNIT的MD5，
最后MD5_LEN为（1M-MD5_LEN）内容得md5作为当前块的md5，这个可以循环认证（类似区块链），不用
存储所有内容，一旦某一次比较失败就认为文件发送失败
*/

typedef struct FileEntry
{
	BfxListItem base;
	uint64_t size;
	char name[FILE_NAME_LEN];
	uint64_t totaltime;
}FileEntry;

typedef struct FileRecvHandle
{
	char name[FILE_NAME_LEN];
	uint64_t fileSize;
	uint64_t totalRecv;

	uint8_t buffer[BUFFER_LEN];
	size_t datalen;
	uint8_t prevUnitMd5[MD5_LEN];
	
	uint64_t beginTime; //第一次收到的文件内容的时间
	bool error;
}FileRecvHandle;

typedef struct FileSendHandle
{
	char name[FILE_NAME_LEN];
	uint64_t fileSize;
	uint64_t totalSend;
	uint64_t totalConfire;

	uint64_t beginTime;//发送文件的开始时间
}FileSendHandle;

uint8_t* RandomGenerateFileContext(FileSendHandle* handle, size_t* len)
{
	*len = handle->fileSize - handle->totalSend;
	if (*len > BUFFER_LEN)
	{
		*len = BUFFER_LEN;
	}
	if (*len == 0)
	{
		return NULL;
	}

	uint8_t* buffer = BFX_MALLOC_ARRAY(uint8_t, *len);
	if (handle->totalSend == 0)
	{
		*((uint64_t*)buffer) = handle->fileSize;
		memcpy(buffer + FILE_SIEZ_BYTES, handle->name, FILE_NAME_LEN);
	}

	//TODO 中间随机加点内容
	uint8_t md5[MD5_LEN];
	int ret = mbedtls_md5_ret(buffer, *len - MD5_LEN, md5);
	assert(!ret);
	memcpy(buffer + (*len - MD5_LEN), md5, MD5_LEN);

	handle->totalSend += *len;

	return buffer;
}


struct TestDemoConnection
{
	BfxListItem base;
	TestDemo* stack;

	char name[CONN_NAME_LEN];

	BdtConnection* conn;

	const BdtPeerInfo* remotePeerInfo;
	uint64_t beginTime; //开始连接时间

	bool bKnownRemotePeerinfo;

	FileRecvHandle* recvHandle;
	FileSendHandle* sendHandle;
};

TestDemoConnection* TestDemoConnectionCreate(TestDemo* stack, const char* name, const BdtPeerInfo* remotePeerInfo, bool peerKnowRemotePeerInfo)
{
	TestDemoConnection* conn = BFX_MALLOC_OBJ(TestDemoConnection);
	memset(conn, 0, sizeof(TestDemoConnection));
	conn->stack = stack;
	strcpy(conn->name, name);
	if (remotePeerInfo)
	{
		conn->remotePeerInfo = remotePeerInfo;
	}
	conn->bKnownRemotePeerinfo = peerKnowRemotePeerInfo;

	return conn;
}

TestDemoConnection* TestDemoConnectionCreateFromBdtConnection(TestDemo* stack, BdtConnection* bdtConn, const char* name)
{
	TestDemoConnection* conn = BFX_MALLOC_OBJ(TestDemoConnection);
	memset(conn, 0, sizeof(TestDemoConnection));
	conn->stack = stack;
	strcpy(conn->name, name);
	conn->conn = bdtConn;

	return conn;
}

bool TestDemoConnectionIsEqual(TestDemoConnection* conn, BdtConnection* bdtConn)
{
	return conn->conn == bdtConn;
}

void TestDemoConnectionDestory(TestDemoConnection* conn)
{
	if (conn->recvHandle)
	{
		BFX_FREE(conn->recvHandle);
		conn->recvHandle = NULL;
	}

	if (conn->sendHandle)
	{
		BFX_FREE(conn->sendHandle);
		conn->sendHandle = NULL;
	}

	BFX_FREE(conn);
}

const char* TestDemoConnectionGetName(TestDemoConnection* conn)
{
	return conn->name;
}

FileRecvHandle* FileRecvHandleOnRecv(FileRecvHandle* handle, uint8_t* data, size_t recvSize)
{
	if (handle->beginTime == 0)
	{
		handle->beginTime = BfxTimeGetNow(false);
	}
	if (handle->fileSize == 0)
	{
		if (!handle->datalen)
		{
			handle->fileSize = *((uint64_t*)data);
			memcpy(handle->name, data + FILE_SIEZ_BYTES, FILE_NAME_LEN);
		}
		else
		{
			//收到的内容不够filename和filesize（目前应该不会进来）
			assert(handle->datalen < FILE_SIEZ_BYTES + FILE_NAME_LEN);
			memcpy(handle->buffer + handle->datalen, data, FILE_SIEZ_BYTES + FILE_NAME_LEN - handle->datalen);
			handle->fileSize = *((uint64_t*)handle->buffer);
			memcpy(handle->name, handle->buffer + FILE_SIEZ_BYTES, FILE_NAME_LEN);
		}
	}

	handle->totalRecv += recvSize;
	return NULL;

	size_t nCopySize = BUFFER_LEN - handle->datalen;
	nCopySize = nCopySize > recvSize ? recvSize : nCopySize;
	nCopySize = nCopySize > (handle->fileSize - handle->totalRecv) ? (handle->fileSize - handle->totalRecv) : nCopySize;

	//BLOG_DEBUG("----nCopy=%llu,total=%llu,recvSize=%llu,fileSize=%llu,datalen=%llu", nCopySize, handle->totalRecv, recvSize, handle->fileSize, handle->datalen);

	//if (!handle->error)
	//{
	//	//如果发送错误，不真正copy
	//	memcpy(handle->buffer + handle->datalen, data, nCopySize);
	//	handle->datalen = 0;
	//}
	//else
	//{
		handle->totalRecv += nCopySize;
		handle->datalen += nCopySize;
	//}

	FileRecvHandle* newHandle = NULL;
	if (handle->totalRecv == handle->fileSize && nCopySize < recvSize)
	{
		newHandle = BFX_MALLOC_OBJ(FileRecvHandle);
		memset(newHandle, 0, sizeof(FileRecvHandle));
		assert(recvSize - nCopySize < BUFFER_LEN);
		memcpy(newHandle->buffer, data + nCopySize, recvSize - nCopySize);
		newHandle->datalen = recvSize - nCopySize;
		newHandle->beginTime = BfxTimeGetNow(false);
		if (newHandle->datalen > FILE_SIEZ_BYTES + FILE_NAME_LEN)
		{
			handle->fileSize = *((uint64_t*)handle->buffer);
			memcpy(handle->name, handle->buffer + FILE_SIEZ_BYTES, FILE_NAME_LEN);
		}
	}

	/*if (handle->error)
	{
		return newHandle;
	}*/

	//刚好一块数据，或者数据接收完成
	if (handle->datalen == BUFFER_LEN || handle->totalRecv == handle->fileSize)
	{
		//uint8_t* currUnitMd5 = handle->buffer + handle->datalen - MD5_LEN;
		////TODO计算当前块md5
		//uint8_t calcMd5[MD5_LEN];
		//int ret = mbedtls_md5_ret(handle->buffer, handle->datalen - MD5_LEN, calcMd5);
		//assert(!ret);
		//if (memcmp(calcMd5, currUnitMd5, MD5_LEN) != 0)
		//{
		//	handle->error = true;
		//}
		//if (handle->totalRecv > BUFFER_LEN)
		//{
		//	//至少第二个UNIT
		//	char* prevMd5 = (char*)(handle->buffer);
		//	if (memcmp(handle->prevUnitMd5, prevMd5, MD5_LEN) != 0)
		//	{
		//		handle->error = true;
		//	}
		//}
		//else
		//{
		//	strcpy(handle->prevUnitMd5, currUnitMd5);
		//}
		handle->datalen = 0;
	}

	return newHandle;
}

static void BFX_STDCALL ConnectionRecvCB(uint8_t* data, size_t recvSize, void* userData)
{
	uint64_t endTime = BfxTimeGetNow(false);

	TestDemoConnection* conn = (TestDemoConnection*)userData;
	if (!conn->recvHandle)
	{
		conn->recvHandle = BFX_MALLOC_OBJ(FileRecvHandle);
		memset(conn->recvHandle, 0, sizeof(FileRecvHandle));
	}

	FileRecvHandle* newFile = FileRecvHandleOnRecv(conn->recvHandle, data, recvSize);
	
	//重复利用内存，不应该释放data
	BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };
	BdtConnectionRecv(conn->conn, data, BUFFER_LEN, ConnectionRecvCB, &newUserData);

	//BLOG_INFO("=========recv conn->recvHandle->totalRecv=%llu,fileSize=%llu", conn->recvHandle->totalRecv, conn->recvHandle->fileSize);
	if (conn->recvHandle->totalRecv > 0 && conn->recvHandle->totalRecv == conn->recvHandle->fileSize)
	{
		TestDemoOnConnectionRecvFinish(
			conn->stack, 
			conn, 
			conn->recvHandle->error ? BFX_RESULT_UNMATCH : BFX_RESULT_SUCCESS,
			conn->recvHandle->name, 
			endTime > conn->recvHandle->beginTime ? (endTime - conn->recvHandle->beginTime) / 1000 : 0
		);

		BFX_FREE(conn->recvHandle);
		conn->recvHandle = NULL;

		if (newFile)
		{
			conn->recvHandle = newFile;
		}
	}
	else
	{
		assert(!newFile);
	}
}

uint32_t TestDemoConnectionBeginRecv(TestDemoConnection* conn)
{
	BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };

	uint8_t* buffer = BFX_MALLOC_ARRAY(uint8_t, BUFFER_LEN);
	uint32_t errcode = BdtConnectionRecv(conn->conn, buffer, BUFFER_LEN, ConnectionRecvCB, &newUserData);
	//printf("BdtConnectionRecv errcode=%lu, buffer addr=0x%llx", errcode, (uint64_t)buffer);
	BLOG_INFO("BdtConnectionRecv errcode=%lu, buffer addr=0x%llx", errcode, (uint64_t)buffer);

	buffer = BFX_MALLOC_ARRAY(uint8_t, BUFFER_LEN);
	errcode = BdtConnectionRecv(conn->conn, buffer, BUFFER_LEN, ConnectionRecvCB, &newUserData);
	BLOG_INFO("BdtConnectionRecv errcode=%lu, buffer addr=0x%llx", errcode, (uint64_t)buffer);

	return BFX_RESULT_SUCCESS;
}

static void ConnectionEvent(BdtConnection* connection, uint32_t result, void* userData)
{
	uint64_t endTime = BfxTimeGetNow(false);
	TestDemoConnection* conn = (TestDemoConnection*)userData;
	BLOG_INFO("connection event, name=%s, result=%u, time=%d", conn->name, result, endTime > conn->beginTime ? (endTime - conn->beginTime)/1000 : 0);

	TestDemoOnConnectionFinish(conn->stack, conn, result, endTime > conn->beginTime ? (endTime - conn->beginTime) / 1000 : 0);
}

static void BFX_STDCALL OnFirstAnswer(const uint8_t* dataArray, size_t dataCount, void* userData)
{

}

uint32_t TestDemoConnectionConnect(TestDemoConnection* conn)
{
	if (conn->conn)
	{
        BLOG_WARN("TestDemoConnectionConnect BFX_RESULT_ALREADY_OPERATION, name:%s", conn->name);
		return BFX_RESULT_ALREADY_OPERATION;
	}

	uint32_t errcode = BdtCreateConnection(TestDemoGetBdtStack(conn->stack), BdtPeerInfoGetPeerid(conn->remotePeerInfo), true, &conn->conn);
	if (errcode)
	{
        BLOG_WARN("TestDemoConnectionConnect create connection failed:%u, name:%s", errcode, conn->name);
        return errcode;
	}

	conn->beginTime = BfxTimeGetNow(false);
	BfxUserData userData = {
		.userData = conn,
		.lpfnAddRefUserData = NULL,
		.lpfnReleaseUserData = NULL
	};

	BdtBuildTunnelParams params;
	memset(&params, 0, sizeof(BdtBuildTunnelParams));
	params.remoteConstInfo = BdtPeerInfoGetConstInfo(conn->remotePeerInfo);
	if (!conn->bKnownRemotePeerinfo) 
	{
		const BdtPeerInfo* snPeerInfo = TestDemoGetSnPeerInfo(conn->stack);
		if (!snPeerInfo)
		{
            BLOG_WARN("TestDemoConnectionConnect TestDemoGetSnPeerInfo, name:%s", errcode, conn->name);
            return BFX_RESULT_FAILED;
		}

		params.snList = BdtPeerInfoGetPeerid(snPeerInfo);
		params.snListLength = 1;
		params.flags |= BDT_BUILD_TUNNEL_PARAMS_FLAG_SN;
	}

	uint8_t fq[16] = { 0 };
	errcode = BdtConnectionConnect(
		conn->conn,
		0,
		&params,
		fq,
		0,
		ConnectionEvent,
		&userData,
		OnFirstAnswer,
		&userData
	);

    BLOG_INFO("TestDemoConnectionConnect create connection failed:%u, name:%s", errcode, conn->name);
    return errcode;
}

void BFX_STDCALL ConnectionSendCB(
	uint32_t result,
	const uint8_t* buffer,
	size_t length,
	void* userData)
{
	TestDemoConnection* conn = (TestDemoConnection*)userData;
	assert(conn->sendHandle);
	conn->sendHandle->totalConfire += length;
	BLOG_INFO("totalsize=%llu,totalsend=%llu, confiresize=%llu", conn->sendHandle->fileSize, conn->sendHandle->totalSend, conn->sendHandle->totalConfire)
	if (conn->sendHandle->totalConfire == conn->sendHandle->fileSize)
	{
		uint64_t endTime = BfxTimeGetNow(false);
		TestDemoOnConnectionSendFinish(
			conn->stack,
			conn,
			BFX_RESULT_SUCCESS,
			conn->sendHandle->name, endTime > conn->sendHandle->beginTime ? (endTime - conn->sendHandle->beginTime) / 1000 : 0
		);
		BFX_FREE(conn->recvHandle);
		conn->recvHandle = NULL;
	}
	else if (conn->sendHandle->fileSize > conn->sendHandle->totalSend)
	{
		BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };

		size_t  len = 0;
		uint8_t* context = RandomGenerateFileContext(conn->sendHandle, &len);
		assert(context && len);
		BdtConnectionSend(conn->conn, context, len, ConnectionSendCB, &newUserData);
	}

	BFX_FREE((uint8_t*)buffer);
}

uint32_t TestDemoConnectionSendFile(TestDemoConnection* conn, uint64_t filesize, const char* name)
{
	if (conn->sendHandle)
	{
		return BFX_RESULT_ALREADY_OPERATION;
	}
	if (filesize < FILE_SIEZ_BYTES + FILE_NAME_LEN + MD5_LEN)
	{
		filesize = FILE_SIEZ_BYTES + FILE_NAME_LEN + MD5_LEN;
	}
	if (filesize > BUFFER_LEN)
	{
		filesize = (filesize / BUFFER_LEN) * BUFFER_LEN;
	}
	
	conn->sendHandle = BFX_MALLOC_OBJ(FileSendHandle);
	memset(conn->sendHandle, 0, sizeof(FileSendHandle));
	strcpy(conn->sendHandle->name, name);
	conn->sendHandle->beginTime = BfxTimeGetNow(false);
	conn->sendHandle->fileSize = filesize;

	BfxUserData newUserData = { .userData = conn,.lpfnAddRefUserData = NULL,.lpfnReleaseUserData = NULL };

	size_t  len = 0;
	uint8_t* context = RandomGenerateFileContext(conn->sendHandle, &len);
	BdtConnectionSend(conn->conn, context, len, ConnectionSendCB, &newUserData);

	context = RandomGenerateFileContext(conn->sendHandle, &len);
	if (!context || !len) 
	{
		BLOG_ERROR("random context failed");
		return BFX_RESULT_FAILED;
	}

	BLOG_DEBUG("--------------------BdtConnectionSend len=%llu", len);
	uint32_t errcode = BdtConnectionSend(conn->conn, context, len, ConnectionSendCB, &newUserData);
	if (errcode == BFX_RESULT_PENDING)
	{
		errcode = BFX_RESULT_SUCCESS;
	}
	return errcode;
}

uint32_t TestDemoConnectionClose(TestDemoConnection* conn)
{
	return BdtConnectionClose(conn->conn);
}

uint32_t TestDemoConnectionReset(TestDemoConnection* conn)
{
	return BdtConnectionReset(conn->conn);
}

double TestDemoConnectionGetSendSpeed(TestDemoConnection* conn)
{
	if (!conn->sendHandle || conn->sendHandle->totalConfire == conn->sendHandle->fileSize)
	{
		return 0;
	}
	double offset = ((double)((uint64_t)BfxTimeGetNow(false) - conn->sendHandle->beginTime) / (double)1000000);
	if (!offset)
	{
		return 0;
	}
	return ((double)conn->sendHandle->totalConfire / offset);
}
