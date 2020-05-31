#include <BuckyBase/BuckyBase.h>
#include <windows.h>
#include <psapi.h>
//#include <winternl.h>

//typedef DWORD SYSTEM_INFORMATION_CLASS;
//typedef NTSTATUS(__stdcall* NTQUERYSYSTEMINFORMATION)(
//    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
//    IN OUT PVOID SystemInformation,
//    IN ULONG SystemInformationLength,
//    OUT PULONG ReturnLength  OPTIONAL
//    );
//NTQUERYSYSTEMINFORMATION NtQuerySystemInformation;
//
//HMODULE hNtDll = NULL;
//
//static uint32_t PrepareFunctions()
//{
//    uint32_t error = ERROR_SUCCESS;
//
//    hNtDll = LoadLibrary(L"NtDll.dll");
//    if (hNtDll == NULL)
//    {
//        error = GetLastError();
//        BLOG_WARN("LoadLibrary(NtDll.dll) failed.");
//        return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, error);
//    }
//
//    NtQuerySystemInformation = (NTQUERYSYSTEMINFORMATION)GetProcAddress(hNtDll, "NtQuerySystemInformation");
//    if (NtQuerySystemInformation == NULL)
//    {
//        error = GetLastError();
//        BLOG_WARN("GetProcAddress for NtQuerySystemInformation failed.");
//        return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, error);
//    }
//    return BFX_RESULT_SUCCESS;
//}

uint32_t EnablePrivileges()
{
    //    TOKEN_PRIVILEGES info;
    //    HANDLE hCurrentProcess = GetCurrentProcess();
    //    HANDLE hToken = NULL;
    //    BOOL result;
    //    uint32_t error = ERROR_SUCCESS;
    //
    //    // Open the token.
    //
    //    result = OpenProcessToken(hCurrentProcess,
    //        TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES,
    //        &hToken);
    //
    //    if (result != TRUE)
    //    {
    //        error = GetLastError();
    //        BLOG_WARN("Cannot open process token.");
    //        goto PRIVILEGES_END;
    //    }
    //
    //    // Enable or disable?
    //
    //    info.Count = 1;
    //    info.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
    //
    //    // Get the LUID.
    //
    //    result = LookupPrivilegeValue(NULL,
    //        SE_LOCK_MEMORY_NAME,
    //        &(info.Privilege[0].Luid));
    //
    //    if (!result)
    //    {
    //        error = GetLastError();
    //        BLOG_WARN("Cannot get privilege for %s.\n", SE_LOCK_MEMORY_NAME);
    //        goto PRIVILEGES_END;
    //    }
    //
    //    // Adjust the privilege.
    //
    //    result = AdjustTokenPrivileges(token, FALSE,
    //        (PTOKEN_PRIVILEGES)&info,
    //        0, NULL, NULL);
    //
    //
    //    // Check the result.
    //
    //    if (result != TRUE)
    //    {
    //        error = GetLastError();
    //        BLOG_WARN("Cannot adjust token privileges.");
    //        goto PRIVILEGES_END;
    //    }
    //    else
    //    {
    //        error = GetLastError();
    //        if (error != ERROR_SUCCESS)
    //        {
    //            BLOG_WARN("Cannot enable the SE_LOCK_MEMORY_NAME privilege;please check the local policy.");
    //            goto PRIVILEGES_END;
    //        }
    //    }
    //
    //PRIVILEGES_END:
    //    if (hToken != NULL)
    //    {
    //        CloseHandle(hToken);
    //    }
    //
    //    if (hCurrentProcess != NULL)
    //    {
    //        CloseHandle(hCurrentProcess);
    //	}
    //
    //    if (error != ERROR_SUCCESS)
    //    {
    //        return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, error);
    //    }
    return BFX_RESULT_SUCCESS;
}

uint32_t SysResQueryEnable()
{
    // uint32_t errorCode1 = PrepareFunctions();
    uint32_t errorCode2 = EnablePrivileges();
    //if (errorCode1 != BFX_RESULT_SUCCESS)
    //{
    //    return errorCode1;
    //}
    if (errorCode2 != BFX_RESULT_SUCCESS)
    {
        return errorCode2;
    }
    return BFX_RESULT_SUCCESS;
}

uint32_t SysResUsageQuery(int64_t* memBytes, int64_t* vmemBytes, uint32_t* handleCount)
{
    PROCESS_MEMORY_COUNTERS pmc;
    HANDLE hCurrentProcess = GetCurrentProcess();
    uint32_t error = ERROR_SUCCESS;

    if (hCurrentProcess == NULL)
    {
        return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, GetLastError());
    }

    if (GetProcessMemoryInfo(hCurrentProcess, &pmc, sizeof(pmc)))
	{
        if (memBytes) {
            *memBytes = pmc.WorkingSetSize;
        }
		if (vmemBytes)
		{
			*vmemBytes = pmc.PagefileUsage;
		}
	}
    else
    {
        error = GetLastError();
        BLOG_WARN("GetProcessMemoryInfo failed.");
	}

    if (!GetProcessHandleCount(hCurrentProcess, handleCount))
    {
        error = GetLastError();
        BLOG_WARN("GetProcessHandleCount failed.");
    }

    CloseHandle(hCurrentProcess);

    if (error == ERROR_SUCCESS)
    {
        return BFX_RESULT_SUCCESS;
    }
	return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, error);
}
// 下面代码从网上copy的，逻辑大概正确，但数据结构可能有问题
//uint32_t SysResGetHandlesUsage(int32_t* count)
//{
//#define NT_PROCESSTHREAD_INFO 0x05
//#define MAX_INFO_BUF_LEN 0x500000
//#define STATUS_SUCCESS               ((NTSTATUS)0x00000000L)
//#define STATUS_INFO_LENGTH_MISMATCH  ((NTSTATUS)0xC0000004L)
//
//    typedef LONG NTSTATUS;
//
//    typedef struct _LSA_UNICODE_STRING
//    {
//        USHORT  Length;
//        USHORT  MaximumLength;
//        PWSTR   Buffer;
//    }LSA_UNICODE_STRING, * PLSA_UNICODE_STRING;
//    typedef LSA_UNICODE_STRING UNICODE_STRING, * PUNICODE_STRING;
//
//    typedef struct _CLIENT_ID
//    {
//        HANDLE UniqueProcess;
//        HANDLE UniqueThread;
//    }CLIENT_ID;
//    typedef CLIENT_ID* PCLIENT_ID;
//
//    typedef LONG KPRIORITY;
//
//    typedef struct _VM_COUNTERS
//    {
//        ULONG PeakVirtualSize;
//        ULONG VirtualSize;
//        ULONG PageFaultCount;
//        ULONG PeakWorkingSetSize;
//        ULONG WorkingSetSize;
//        ULONG QuotaPeakPagedPoolUsage;
//        ULONG QuotaPagedPoolUsage;
//        ULONG QuotaPeakNonPagedPoolUsage;
//        ULONG QuotaNonPagedPoolUsage;
//        ULONG PagefileUsage;
//        ULONG PeakPagefileUsage;
//    }VM_COUNTERS, * PVM_COUNTERS;
//
//    typedef struct _IO_COUNTERS
//    {
//        LARGE_INTEGER ReadOperationCount;
//        LARGE_INTEGER WriteOperationCount;
//        LARGE_INTEGER OtherOperationCount;
//        LARGE_INTEGER ReadTransferCount;
//        LARGE_INTEGER WriteTransferCount;
//        LARGE_INTEGER OtherTransferCount;
//    }IO_COUNTERS, * PIO_COUNTERS;
//
//    typedef enum _THREAD_STATE
//    {
//        StateInitialized,
//        StateReady,
//        StateRunning,
//        StateStandby,
//        StateTerminated,
//        StateWait,
//        StateTransition,
//        StateUnknown
//    }THREAD_STATE;
//
//    typedef enum _KWAIT_REASON
//    {
//        Executive,
//        FreePage,
//        PageIn,
//        PoolAllocation,
//        DelayExecution,
//        Suspended,
//        UserRequest,
//        WrExecutive,
//        WrFreePage,
//        WrPageIn,
//        WrPoolAllocation,
//        WrDelayExecution,
//        WrSuspended,
//        WrUserRequest,
//        WrEventPair,
//        WrQueue,
//        WrLpcReceive,
//        WrLpcReply,
//        WrVertualMemory,
//        WrPageOut,
//        WrRendezvous,
//        Spare2,
//        Spare3,
//        Spare4,
//        Spare5,
//        Spare6,
//        WrKernel
//    }KWAIT_REASON;
//
//    typedef struct _SYSTEM_THREADS
//    {
//        LARGE_INTEGER KernelTime;
//        LARGE_INTEGER UserTime;
//        LARGE_INTEGER CreateTime;
//        ULONG         WaitTime;
//        PVOID         StartAddress;
//        CLIENT_ID     ClientId;
//        KPRIORITY     Priority;
//        KPRIORITY     BasePriority;
//        ULONG         ContextSwitchCount;
//        THREAD_STATE  State;
//        KWAIT_REASON  WaitReason;
//    }SYSTEM_THREADS, * PSYSTEM_THREADS;
//
//    typedef struct _SYSTEM_PROCESSES
//    {
//        ULONG          NextEntryDelta;          //构成结构序列的偏移量；
//        ULONG          ThreadCount;             //线程数目；
//        ULONG          Reserved1[6];
//        LARGE_INTEGER  CreateTime;              //创建时间；
//        LARGE_INTEGER  UserTime;                //用户模式(Ring 3)的CPU时间；
//        LARGE_INTEGER  KernelTime;              //内核模式(Ring 0)的CPU时间；
//        UNICODE_STRING ProcessName;             //进程名称；
//        KPRIORITY      BasePriority;            //进程优先权；
//        ULONG          ProcessId;               //进程标识符；
//        ULONG          InheritedFromProcessId;  //父进程的标识符；
//        ULONG          HandleCount;             //句柄数目；
//        ULONG          Reserved2[2];
//        VM_COUNTERS    VmCounters;              //虚拟存储器的结构，见下；
//        IO_COUNTERS    IoCounters;              //IO计数结构，见下；
//        SYSTEM_THREADS Threads[1];              //进程相关线程的结构数组，见下；
//    } SYSTEM_PROCESSES, * PSYSTEM_PROCESSES;
//
//    PSYSTEM_PROCESSES pSystemProc = NULL;
//    DWORD dwNumberBytes = MAX_INFO_BUF_LEN;
//    LPVOID lpSystemInfo = NULL;
//    DWORD dwTotalProcess = 0;
//    DWORD dwReturnLength;
//    NTSTATUS Status;
//    DWORD error = ERROR_SUCCESS;
//
//    lpSystemInfo = (LPVOID)malloc(dwNumberBytes);
//    Status = NtQuerySystemInformation(NT_PROCESSTHREAD_INFO,
//        lpSystemInfo,
//        dwNumberBytes,
//        &dwReturnLength);
//    if (Status == STATUS_INFO_LENGTH_MISMATCH)
//    {
//        error = GetLastError();
//        BLOG_WARN("STATUS_INFO_LENGTH_MISMATCH.");
//        free(lpSystemInfo);
//        return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, error);
//    }
//    else if (Status != STATUS_SUCCESS)
//    {
//        error = GetLastError();
//        BLOG_WARN("NtQuerySystemInformation");
//        free(lpSystemInfo);
//        return BFX_MAKE_RESULT(BFX_RESULTTYPE_SYSTEM, error);
//    }
//
//    pSystemProc = (PSYSTEM_PROCESSES)lpSystemInfo;
//    *count = (int32_t)pSystemProc->HandleCount;
//    return BFX_RESULT_SUCCESS;
//}