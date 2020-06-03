#include "./demo.h"
#if defined(BFX_OS_WIN)
#include "./dump.h"
#elif defined(BFX_OS_POSIX) && !defined(EMBEDDED_UBUNTU_12)
#include "./unix_dump.h"
#else
#include "./arm_dump.h"
#endif // XPF_OS_WIN

#if defined(BFX_OS_POSIX) && defined(EMBEDDED_UBUNTU_12)
FILE* gFile = NULL;
void f1()
{
	int* fp,*lr = 0;
	__asm__(
		"mov %0, fp\n"
		: "=r"(fp)
	);
	__asm__(
		"mov %0, lr\n"
		: "=r"(lr)
	);
	fprintf(gFile, "f1, fp=0x%x, lr=0x%x, precvstack=0x%x, lrfromstack=0x%x\n", fp, lr, *(fp - 1), *(fp-2));
	fflush(gFile);

	int* p = NULL;
	*p = 5;
}

void f2()
{
	int* fp, * lr = 0;
	__asm__(
		"mov %0, fp\n"
		: "=r"(fp)
	);
	__asm__(
		"mov %0, lr\n"
		: "=r"(lr)
	);
	fprintf(gFile, "f2, fp=0x%x, lr=0x%x, precvstack=0x%x, lrfromstack=0x%x\n", fp, lr, *(fp - 1), *(fp - 2));
	fflush(gFile);

	f1();
}

void f3()
{
	int* fp, * lr = 0;
	__asm__(
		"mov %0, fp\n"
		: "=r"(fp)
	);
	__asm__(
		"mov %0, lr\n"
		: "=r"(lr)
	);
	fprintf(gFile, "f3, fp=0x%x, lr=0x%x, precvstack=0x%x, lrfromstack=0x%x\n", fp, lr, *(fp - 1), *(fp - 2));
	fflush(gFile);

	f2();
}
#endif
int main(int argc, char* argv[])
{
	InitMinDump(argv[3]);
	/*gFile = fopen("./1.txt", "w+");
	f3();*/
	/*int* p = NULL;
	*p = 5;*/
	/*if (argc < 3)
	{
		printf("need port");

		return 0;
	}*/
#if defined(BFX_OS_WIN)
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif

	uint16_t port = atoi(argv[1]);
	TestDemo* demo = TestDemoCreate(port, argv[2]);
	//TestDemo* demo = TestDemoCreate(53759, "peer2");
	TestDemoRun(demo);
}