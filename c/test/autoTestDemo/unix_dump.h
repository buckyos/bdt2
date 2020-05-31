#ifndef _BDT_AUTO_TEST_UNIX_DUMP_H_
#define _BDT_AUTO_TEST_UNIX_DUMP_H_
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/file.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <execinfo.h>
#include <signal.h>

char g_szDumpSavePath[256] = { 0 };

void server_backtrace(int sig)
{
	//���ļ�
	time_t tSetTime;
	time(&tSetTime);
	struct tm* ptm = localtime(&tSetTime);
	char fname[256] = { 0 };
	sprintf(fname, "%s/core.%d-%d-%d_%d_%d_%d", g_szDumpSavePath,
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	FILE* f = fopen(fname, "a");
	if (f == NULL) {
		return;
	}
	int fd = fileno(f);

	//�����ļ�
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	fl.l_pid = getpid();
	fcntl(fd, F_SETLKW, &fl);

	//�������ľ���·��
	char buffer[4096];
	memset(buffer, 0, sizeof(buffer));
	int count = readlink("/proc/self/exe", buffer, sizeof(buffer));
	if (count > 0) {
		buffer[count] = '\n';
		buffer[count + 1] = 0;
		fwrite(buffer, 1, count + 1, f);
	}

	//�����Ϣ��ʱ��
	memset(buffer, 0, sizeof(buffer));
	sprintf(buffer, "Dump Time: %d-%d-%d %d:%d:%d\n",
		ptm->tm_year + 1900, ptm->tm_mon + 1, ptm->tm_mday,
		ptm->tm_hour, ptm->tm_min, ptm->tm_sec);
	fwrite(buffer, 1, strlen(buffer), f);

	//�̺߳��ź�
	sprintf(buffer, "Curr thread: %u, Catch signal:%d\n",
		(int)pthread_self(), sig);
	fwrite(buffer, 1, strlen(buffer), f);

	//��ջ
	void* DumpArray[256];
	int    nSize = backtrace(DumpArray, 256);
	sprintf(buffer, "backtrace rank = %d\n", nSize);
	fwrite(buffer, 1, strlen(buffer), f);
	if (nSize > 0) {
		char** symbols = backtrace_symbols(DumpArray, nSize);
		if (symbols != NULL) {
			for (int i = 0; i < nSize; i++) {
				fwrite(symbols[i], 1, strlen(symbols[i]), f);
				fwrite("\n", 1, 1, f);
			}
			free(symbols);
		}
	}

	//�ļ�������ر�
	fl.l_type = F_UNLCK;
	fcntl(fd, F_SETLK, &fl);
	fclose(f);
}

void signal_crash_handler(int sig)
{
	server_backtrace(sig);
	exit(0);
}

void InitMinDump(const char* savePath)
{
	strncpy(g_szDumpSavePath, savePath, strlen(savePath));
	// ignore SIGPIPE
	signal(SIGPIPE, SIG_IGN);

	signal(SIGBUS, signal_crash_handler);     // ���ߴ���
	signal(SIGSEGV, signal_crash_handler);    // SIGSEGV���Ƿ��ڴ����
	signal(SIGFPE, signal_crash_handler);       // SIGFPE����ѧ��ص��쳣���类0��������������ȵ�
	signal(SIGABRT, signal_crash_handler);     // SIGABRT���ɵ���abort�������������̷������˳�
	signal(SIGTERM, SIG_IGN);
}

#endif