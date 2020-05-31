#include "./TestCase.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

#include "./Global/Test.h"


static Bdt_TestCaseGroup* rootCase;

#define TEST_CASE_MAX_GROUP_DEPTH	10
#define TEST_CASE_NAME_SPLITER		'.'

static SplitTestCaseName(const char* name, int indices[TEST_CASE_MAX_GROUP_DEPTH])
{
	size_t len = strlen(name);
	size_t iy = 0;
	memset(indices, -1, TEST_CASE_MAX_GROUP_DEPTH);
	for (size_t ix = 0; ix < len; ++ix)
	{
		if (name[ix] == TEST_CASE_NAME_SPLITER)
		{
			indices[iy] = (int)ix;
			++iy;
			if (iy == TEST_CASE_MAX_GROUP_DEPTH - 1)
			{
				break;
			}
		}
	}
	indices[iy] = (int)len;
}

static int TestCaseGroupRoutine(Bdt_TestCase* testCase)
{
	int iy = 0;
	do
	{
		Bdt_TestCase* curCase = ((Bdt_TestCaseGroup*)testCase)->cases[iy];
		if (!curCase)
		{
			break;
		}
		curCase->routine(curCase);
		++iy;
	} while (true);
	return 0;
}

int Bdt_AddTestCase(const char* name, Bdt_TestCaseRoutine routine)
{
	int indices[TEST_CASE_MAX_GROUP_DEPTH];
	SplitTestCaseName(name, indices);
	size_t namelen = strlen(name);
	char rname[BDT_TEST_CASE_MAX_NAME_LENGTH];
	int s = 0;
	Bdt_TestCase* upperCase = (Bdt_TestCase*)rootCase;
	int ix = 0;
	bool leaf = false;
	do
	{
		int e = indices[ix];
		++ix;
		leaf = (e == strlen(name));
		memcpy(rname, name + s, e - s);
		rname[e - s] = 0;
		int iy = 0;
		Bdt_TestCase* curCase = NULL;
		do
		{
			curCase = ((Bdt_TestCaseGroup*)upperCase)->cases[iy];
			if (!curCase)
			{
				break;
			}
			if (!strcmp(rname, curCase->name))
			{
				break;
			}
			++iy;
		} while (true);
		if (leaf)
		{
			assert(!curCase);
			curCase = (Bdt_TestCase*)malloc(sizeof(Bdt_TestCase));
			memset(curCase, 0, sizeof(Bdt_TestCase));
			strcpy(curCase->name, rname);
			curCase->routine = routine;
			((Bdt_TestCaseGroup*)upperCase)->cases[iy] = curCase;
			break;
		}
		else
		{
			if (!curCase)
			{
				curCase = (Bdt_TestCase*)malloc(sizeof(Bdt_TestCaseGroup));
				memset(curCase, 0, sizeof(Bdt_TestCaseGroup));
				strcpy(curCase->name, rname);
				curCase->routine = TestCaseGroupRoutine;
				((Bdt_TestCaseGroup*)curCase)->cases[0] = NULL;
				((Bdt_TestCaseGroup*)upperCase)->cases[iy] = curCase;
			}
		}
		upperCase = curCase;
		s = e + 1;
	} while (true);
	return 0;
}

static Bdt_TestCase* GetTestCase(const char* name)
{
	if (!name)
	{
		return (Bdt_TestCase*)rootCase;
	}
	int indices[TEST_CASE_MAX_GROUP_DEPTH];
	SplitTestCaseName(name, indices);
	size_t namelen = strlen(name);
	char rname[BDT_TEST_CASE_MAX_NAME_LENGTH];
	int s = 0;
	Bdt_TestCase* upperCase = (Bdt_TestCase*)rootCase;
	int ix = 0;
	do
	{
		int e = indices[ix + 1];
		++ix;
		memcpy(rname, name + s, e - s);
		rname[e - s + 1] = 0;
		int iy = 0;
		Bdt_TestCase* curCase = NULL;
		do
		{
			curCase = ((Bdt_TestCaseGroup*)upperCase)->cases[iy];
			if (!curCase)
			{
				break;
			}
			if (!strcmp(rname, curCase->name))
			{
				break;
			}
			++iy;
		} while (true);
		if (!curCase)
		{
			return NULL;
		}
		if (e == strlen(name))
		{
			return curCase;
		}
		upperCase = curCase;
		s = e + 1;
	} while (true);

	return NULL;
}



int main(int argc, char* argv[])
{
	
	rootCase = (Bdt_TestCaseGroup*)malloc(sizeof(Bdt_TestCaseGroup));
	memset(rootCase, 0, sizeof(Bdt_TestCaseGroup));
	((Bdt_TestCase*)rootCase)->routine = TestCaseGroupRoutine;
	rootCase->cases[0] = NULL;

	// register cases
	BdtGlobal_Profiler();

	const char* testName = argc > 1 ? argv[1] : NULL;
	Bdt_TestCase* testCase = GetTestCase(testName);
	if (!testCase)
	{
		printf("test case: %s not found\r\n", testName);
		return -1;
	}
	return testCase->routine(testCase);
}