#ifndef __BDT_PACKAGE_CONNECTIOIN_LEDBAT_CONGESTION_CONTROL_INL__
#define __BDT_PACKAGE_CONNECTIOIN_LEDBAT_CONGESTION_CONTROL_INL__

#include "./BaseCc.inl"

typedef struct LedbatCc LedbatCc;

static LedbatCc* LedbatCcNew(uint16_t mtu, uint32_t mss);

static void LedbatCcOnAck(struct BaseCc* cc, const Bdt_SessionDataPackage* package, uint32_t nAcks, uint64_t flight);
static void LedbatCcOnChangeState(struct BaseCc* cc, CcState oldState, CcState newState);
static void LedbatCcOnSend(struct BaseCc* cc, Bdt_SessionDataPackage* package);
static void LedbatCcUninit(struct BaseCc* cc);
static uint32_t LedbatCcOnData(BaseCc* cc, const Bdt_SessionDataPackage* package, RecvPackageType type);
static uint32_t LedbatCcTrySendPayload(struct BaseCc* cc, uint32_t flight, uint16_t* outPayloadLength);
static uint32_t LedbatCcTrySendAck(struct BaseCc* cc);

static bool CompareStreamPosWith(uint64_t streamPos, BdtStreamRange* sackArray);
static void UpdateBaseDelay(LedbatCc* self, int64_t delay);
static void UpdateCurrDelay(LedbatCc* self, int64_t delay);
static void UpdateSample(LedbatCc* self, const Bdt_SessionDataPackage* package);

#endif