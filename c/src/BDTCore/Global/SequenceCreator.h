#ifndef __BDT_SEQUENCE_CREATOR_H__
#define __BDT_SEQUENCE_CREATOR_H__

#define SEQUENCE_TIME_BITS 20
#define SEQUENCE_INCREASE_BITS (32 - SEQUENCE_TIME_BITS)

typedef union
{
	struct
	{
		volatile uint32_t increase : SEQUENCE_INCREASE_BITS;
		volatile uint32_t time : SEQUENCE_TIME_BITS;
	} bits;
	volatile uint32_t u32;
	volatile int32_t i32;
} Bdt_Sequence;

typedef struct Bdt_SequenceCreator
{
	Bdt_Sequence lastValue;
} Bdt_SequenceCreator;

void Bdt_SequenceInit(Bdt_SequenceCreator* creator);
void Bdt_SequenceUninit(Bdt_SequenceCreator* creator);
uint32_t Bdt_SequenceValue(Bdt_SequenceCreator* creator);
uint32_t Bdt_SequenceNext(Bdt_SequenceCreator* creator);
int Bdt_SequenceCompare(uint32_t seq1, uint32_t seq2);
bool Bdt_SequenceIsTimeout(uint32_t seq, uint32_t timeLimitS);

#endif