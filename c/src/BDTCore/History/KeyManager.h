#ifndef __BDT_HISTORY_KEYMANAGER_H__
#define __BDT_HISTORY_KEYMANAGER_H__

#include "../BdtCore.h"
#include "../Global/CryptoToolKit.h"

#define BDT_MIX_HASH_4_KEY_LIMIT	4 // 每个KEY最多计算的HASH数

typedef struct BdtStorage BdtStorage;
typedef struct BdtHistory_KeyInfo BdtHistory_KeyInfo;

typedef struct BdtHistory_KeyManager BdtHistory_KeyManager;


BdtHistory_KeyManager* BdtHistory_KeyManagerCreate(const BdtAesKeyConfig* config);

void BdtHistory_KeyManagerDestroy(BdtHistory_KeyManager* keyManager);

// should always sync finished
// always touch stub in manager to extend key's expire time
// if key already exists in key manager, returns BFX_RESULT_ALREADY_EXISTS
uint32_t BdtHistory_KeyManagerAddKey(
	BdtHistory_KeyManager* keyManager,
	const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	const BdtPeerid* peerid,
	// confirm the key by remote
	bool confirmedByRemote
);

// should always sync finished
uint32_t BdtHistory_KeyManagerFindByMixHash(
	BdtHistory_KeyManager* keyManager,
	const uint8_t mixHash[BDT_MIX_HASH_LENGTH],
	uint8_t outKey[BDT_AES_KEY_LENGTH],
	BdtPeerid* outPeerid,
	// if touch stub in manager to extend key's expire time
	bool touch,
	// confirm the key by remote
	bool confirmedByRemote
);

// should always sync finished
uint32_t BdtHistory_KeyManagerGetKeyByRemote(
	BdtHistory_KeyManager* keyManager,
	const BdtPeerid* peerid,
	uint8_t outKey[BDT_AES_KEY_LENGTH],
	// in: if create a new key for remote when no key exists 
	// out: if set to true when input, output value of this param shows if the key returned is newly created
	bool* inoutNew,
	// out: set 'true' when the key is confirmed by remote
	bool* outIsConfirmByRemote,
	// if touch stub in manager to extend key's expire time
	bool touch
);

bool BdtHistory_KeyManagerCalcKeyHash(const uint8_t aesKey[BDT_AES_KEY_LENGTH],
	uint8_t hash[BDT_MIX_HASH_LENGTH]);

void BdtHistory_KeyManagerOnLoadDone(
	BdtHistory_KeyManager* keyManager,
	BdtStorage* storage,
	uint32_t errorCode,
	const BdtHistory_KeyInfo* aesKeys[],
	size_t keyCount
	);

#endif //__BDT_HISTORY_KEYMANAGER_H__
