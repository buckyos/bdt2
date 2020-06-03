#ifndef _BDT_AUTO_TEST_ARM_DUMP_H_
#define _BDT_AUTO_TEST_ARM_DUMP_H_


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#ifndef __USE_GNU
# define __USE_GNU
#endif
#include <signal.h>
#include <sys/ucontext.h>
#include <dlfcn.h>

char g_szDumpSavePath[256] = { 0 };

int my_backtrack(void** buffer, int size, void* ucontext, FILE* f)
{
	char out[1024] = { 0 };
	sprintf(out, "size=%d\n", size);
	fwrite(out, 1, strlen(out), f);
	fflush(f);

	if (size <= 0)
	{
		return 0;
	}

	int cnt = 0;
	ucontext_t* sigContext = (ucontext_t*)ucontext;
	int pc = sigContext->uc_mcontext.arm_pc;

	sprintf(out, "pc=0x%08x\n", pc);
	fwrite(out, 1, strlen(out), f);
	fflush(f);

	buffer[cnt++] = (void*)pc;

	sprintf(out, "%d.0x%08x\n",cnt, buffer[cnt-1]);
	fwrite(out, 1, strlen(out), f);
	fflush(f);

	int* fp = (int*)(sigContext->uc_mcontext.arm_fp);

	int* next_fp = 0;
	int ret = 0;

	buffer[cnt++] = (void*)(*(fp - 2));
	next_fp = (int*)(*(fp - 1));

	while ((cnt <= size) && (next_fp != NULL))
	{
		buffer[cnt++] = (void*)(*(next_fp - 2));
		next_fp = (int*)(*(next_fp - 1));

		sprintf(out, "%d.0x%08x\n", cnt, buffer[cnt - 1]);
		fwrite(out, 1, strlen(out), f);
		fflush(f);
	}

	sprintf(out, "backtrace finish=========");
	fwrite(out, 1, strlen(out), f);
	fflush(f);

	ret = ((cnt <= size) ? cnt : size);

	return ret;
}

char** my_backtrace_symbols(void* const* buffer, int size)
{
# define WORD_WIDTH 8  
	Dl_info info[size];
	int status[size];
	int cnt;
	size_t total = 0;
	char** result;

	for (cnt = 0; cnt < size; ++cnt)
	{
		status[cnt] = dladdr(buffer[cnt], &info[cnt]);
		if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0')
		{
			total += (strlen(info[cnt].dli_fname ? info[cnt].dli_fname : "")
				+ (info[cnt].dli_sname ? (strlen(info[cnt].dli_sname) + 3 + WORD_WIDTH + 3) : 1)
				+ WORD_WIDTH + 5);
		}
		else
		{
			total += 5 + WORD_WIDTH;
		}
	}

	total = 4096;
	result = (char**)malloc(size * sizeof(char*) + total);
	if (result != NULL)
	{
		char* last = (char*)(result + size);

		for (cnt = 0; cnt < size; ++cnt)
		{
			result[cnt] = last;

			if (status[cnt] && info[cnt].dli_fname && info[cnt].dli_fname[0] != '\0')
			{
				char buf[20] = { 0 };

				if (buffer[cnt] >= (void*)info[cnt].dli_saddr)
					sprintf(buf, "+%#lx", \
					(unsigned long)(buffer[cnt] - info[cnt].dli_saddr));
				else
					sprintf(buf, "-%#lx", \
					(unsigned long)(info[cnt].dli_saddr - buffer[cnt]));

				last += 1 + sprintf(last, "%s%s%s%s%s(+%p)",
					info[cnt].dli_fname ? info[cnt].dli_fname: "",
					/*info[cnt].dli_sname ? "(" : "",
					info[cnt].dli_sname ? : "",
					info[cnt].dli_sname ? buf : "",
					info[cnt].dli_sname ? ") " : " ",*/
					"[",
					info[cnt].dli_sname ? : "",
					buf,
					"] ",
					buffer[cnt]);
			}
			else
			{
				last += 1 + sprintf(last, "[%p]", buffer[cnt]);
			}
		}
	}

	return result;
}

void signal_crash_handler(int sig, siginfo_t* info, void* ucontext)
{
	//打开文件
	time_t tSetTime;
	time(&tSetTime);
	struct tm* ptm = localtime(&tSetTime);
	char fname[256] = { 0 };
	sprintf(fname, "%s/arm_dump.%d-%d-%d_%d_%d_%d", g_szDumpSavePath,
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	FILE* f = fopen(fname, "a");
	if (f == NULL) {
		return;
	}
	fwrite("dd\n", 1, 3, f);
	fflush(f); 
	int fd = fileno(f);

	//锁定文件
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	fl.l_pid = getpid();
	fcntl(fd, F_SETLKW, &fl);

	//输出程序的绝对路径
	char buffer[4096];
	memset(buffer, 0, sizeof(buffer));
	int count = readlink("/proc/self/exe", buffer, sizeof(buffer));
	if (count > 0) {
		buffer[count] = '\n';
		buffer[count + 1] = 0;
		fwrite(buffer, 1, count + 1, f);
		fflush(f);
	}

	//输出信息的时间
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "Dump Time: %d-%d-%d %d:%d:%d\n",
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	fwrite(buffer, 1, strlen(buffer), f);
	fflush(f);

	sprintf(buffer, "exception code=%d, at 0x%08x\n", info->si_signo, info->si_addr);
	fwrite(buffer, 1, strlen(buffer), f);
	fflush(f);

	void* DumpArray[60];
	int    nSize = my_backtrack(DumpArray, 60,ucontext, f);
	sprintf(buffer, "backtrace rank = %d\n", nSize);
	fwrite(buffer, 1, strlen(buffer), f);
	fflush(f);
	if (nSize > 0) {
		char** symbols = my_backtrace_symbols(DumpArray, nSize);
		if (symbols != NULL) {
			for (int i = 0; i < nSize; i++) {
				fwrite(symbols[i], 1, strlen(symbols[i]), f);
				fwrite("\n", 1, 1, f);
				fflush(f);
			}
			free(symbols);
		}
	}
	sprintf(buffer, "finish===========");
	fwrite(buffer, 1, strlen(buffer), f);
	fflush(f);

	//文件解锁后关闭
	fl.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &fl);
	fclose(f);

	exit(0);
}


void InitMinDump(const char* savePath)
{
	strncpy(g_szDumpSavePath, savePath, strlen(savePath));

	// ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	struct sigaction stAction;
	memset(&stAction, 0, sizeof(struct sigaction));
	stAction.sa_sigaction = signal_crash_handler;
	sigemptyset(&stAction.sa_mask);
	stAction.sa_flags = SA_SIGINFO;

	sigaction(SIGBUS, &stAction, NULL);     // 总线错误
	sigaction(SIGSEGV, &stAction, NULL);    // SIGSEGV，非法内存访问
	sigaction(SIGFPE, &stAction, NULL);       // SIGFPE，数学相关的异常，如被0除，浮点溢出，等等
	sigaction(SIGABRT, &stAction, NULL);     // SIGABRT，由调用abort函数产生，进程非正常退出
	signal(SIGTERM, SIG_IGN);
}

#endif
