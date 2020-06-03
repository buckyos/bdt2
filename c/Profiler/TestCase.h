#ifndef __BDT_TEST_CASE_H__
#define __BDT_TEST_CASE_H__

#define BDT_TEST_CASE_MAX_NAME_LENGTH	1024
#define BDT_TEST_CASE_MAX_CASE_COUNT	1024

typedef struct Bdt_TestCase Bdt_TestCase;
typedef int (*Bdt_TestCaseRoutine)(Bdt_TestCase* testCase);
struct Bdt_TestCase
{
	char name[BDT_TEST_CASE_MAX_NAME_LENGTH];
	Bdt_TestCaseRoutine routine;
};

typedef struct Bdt_TestCaseGroup
{
	Bdt_TestCase base;
	Bdt_TestCase* cases[BDT_TEST_CASE_MAX_CASE_COUNT];
} Bdt_TestCaseGroup;

int Bdt_AddTestCase(const char* name, Bdt_TestCaseRoutine routine);

#endif //__BDT_TEST_CASE_H__