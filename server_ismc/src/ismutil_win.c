/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include <ismutil.h>
#ifdef _WIN32

HANDLE g_eventList[2];

const DWORD MS_VC_EXCEPTION=0x406D1388;

#pragma pack(push,8)
typedef struct tagTHREADNAME_INFO {
    DWORD dwType;       /* Must be 0x1000.                         */
    LPCSTR szName;      /* Pointer to name (in user addr space).   */
    DWORD dwThreadID;   /* Thread ID (-1=caller thread).           */
    DWORD dwFlags;
} THREADNAME_INFO;
#pragma pack(pop)

/*
 * Set a thread name
 */
static void setThreadName(DWORD dwThreadID, char * threadName) {
    THREADNAME_INFO info;
    info.dwType = 0x1000;
    info.szName = threadName;
    info.dwThreadID = dwThreadID;
    info.dwFlags = 0;

    __try {
        RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
    } __except(EXCEPTION_EXECUTE_HANDLER) {}
}


/*
 * Windows locking functions
 */

/*
 * Mutex lock
 */
int   ism_common_lock(ism_lock_t * lock) {
    EnterCriticalSection((CRITICAL_SECTION *)lock);
    return 0;
}

/*
 * Mutex unlock
 */
int   ism_common_unlock(ism_lock_t * lock) {
    LeaveCriticalSection((CRITICAL_SECTION *)lock);
    return 0;
}

/*
 * Mutex init
 */
int   ism_common_lockinit(ism_lock_t * lock) {
    InitializeCriticalSection((CRITICAL_SECTION *)lock);
    return 0;
}


int   ism_common_trylock(ism_lock_t * lock) {
    return !TryEnterCriticalSection((CRITICAL_SECTION *)lock);
}

/*
 * Mutex close
 */
int    ism_common_lockclose(ism_lock_t * lock) {
    DeleteCriticalSection((CRITICAL_SECTION *)lock);
    return 0;
}
#define SPIN_COUNT 1000
/*
 * Spinlock wait
 */
void ism_common_spinwait(pthread_spinlock_t * x) {
    int  count =  SPIN_COUNT;
    while(_InterlockedExchange(x, SL_BUSY) == SL_BUSY) {
        if (count)
            count--;
        else{
            count = SPIN_COUNT;
            SwitchToThread();
        }
    }
    return;
}

/*
 * Parameters to start a thread
 */
typedef struct thread_wrap_param {
    ism_thread_parm_t   threadProc;
    void *              threadParam;
    void *              context;
    int                 value;
    char                threadName[16];
} thread_wrap_param;


/*
 * First method called in each thread
 */
static DWORD __stdcall threadStarter(void* param) {
    DWORD result;
    thread_wrap_param  * threadWrap = (thread_wrap_param*)param;
    setThreadName(-1, threadWrap->threadName);
    result = (DWORD)((uintptr_t)(threadWrap->threadProc(threadWrap->threadParam, threadWrap->context, threadWrap->value)));
    ism_common_free(ism_memory_ismc_misc,threadWrap);
    return result;
}


/*
 * Start a thread
 */
XAPI int ism_common_startThread(ism_threadh_t * t, ism_thread_parm_t addr, void * data, void * context, int value,
                 enum thread_use_e usage, int flags, const char * name, const char * description) {
    uintptr_t result;
    thread_wrap_param  * threadWrap = (thread_wrap_param *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_ismc_misc,9),1,sizeof(thread_wrap_param));
    threadWrap->threadParam = data;
    threadWrap->context = context;
    threadWrap->value = value;
    threadWrap->threadProc = addr;
    ism_common_strlcpy(threadWrap->threadName, name, 16);

    result = _beginthreadex(NULL, 0, threadStarter, threadWrap, CREATE_SUSPENDED, NULL);
    if (result == 0) {
        *t = 0;
        return errno;
    }
    if (ResumeThread(((HANDLE)result)) == -1){
        *t = 0;
        return errno;
    } else {
        *t = result;
        return 0;
    }
}



/*
 * End a thread
 */
XAPI void ism_common_endThread(void * retval) {
    _endthreadex((DWORD)(uintptr_t)retval);
}


/*
 * Join the thread
 */
XAPI  int  ism_common_joinThread(ism_threadh_t thread, void ** retvalptr) {
    int rc;
    rc = WaitForSingleObject((HANDLE)thread, INFINITE);
    if (rc == WAIT_OBJECT_0) {
        DWORD lpExitCode = 0;
        rc = GetExitCodeThread((HANDLE)thread,&lpExitCode);
        if (retvalptr){
            ULONG_PTR lp = lpExitCode;
            *retvalptr = (void *)lp;
        }
        //printf("GetExitCode handle:%d rc:%d retval:%d\n", thread, rc, lpExitCode);
        if (rc) {
            rc = 0;
        } else {
            rc = GetLastError();
        }
    } else {
        rc = GetLastError();
    }
    return rc;
}


/*
 * Cancel a thread
 */
XAPI int ism_common_cancelThread(ism_threadh_t thread) {
    if (TerminateThread((HANDLE)thread, -1)) {
      return 0;
    }
    return GetLastError();
}


/*
 * Detach a thread
 */
XAPI int ism_common_detachThread(ism_threadh_t thread) {
    return CloseHandle((HANDLE)thread);
}


/*
 * Sleep in microseconds
 */
void  ism_common_sleep(int micros) {
    SleepEx((micros+500)/1000, 1);
}


/*
 * Get a thread ID
 */
ism_threadh_t ism_common_threadSelf() {
    return GetCurrentThreadId();
}

/*
 *
 */
XAPI void ism_common_setShutdownEvent(HANDLE shutdown) {
    g_eventList[1] = shutdown;
}


/*
 * Create a thread local key
 */
XAPI int pthread_key_create(uintptr_t * key, void * val) {
    *key = TlsAlloc();
    if (*key == TLS_OUT_OF_INDEXES)
        return -1;
    return 0;
}

/*
 * Get a thread local value
 */
XAPI void * pthread_getspecific(uintptr_t key) {
    return TlsGetValue((DWORD)key);
}


/*
 * Set a thread local value
 */
XAPI int pthread_setspecific(uintptr_t key, void * data) {
    return TlsSetValue((DWORD)key, data);
}


/*
 * Close a thread local key
 */
XAPI int pthread_key_delete(uintptr_t key) {
    return TlsFree((DWORD)key);
}

/*
 * These variables are only initialized once
 */
static char * os_name;
static char * os_ver;
static char * os_arch;
static char * os_kernelver;
static char * os_execenv;
static char * os_hostname;
static char * os_processorinfo;
static int    MHz = 0;

/*
 * Get the environment strings
 */
void ism_common_initExecEnv(void) {
    char processor_name [256];
    char hostname [512];
    char osver[256];
    int  MHz = 0;
    int  totalmem = 0;
    char processor_id [256];
    uint32_t bufsize;
    int  pcount = 0;
    int  rc;
    HKEY hKey;
    char * pname;
    OSVERSIONINFOEX verinfo;
    MEMORYSTATUSEX memstatus;
    char temp[256];

    /*
     * Get the OS name for Windows from the version string
     */
    if (os_name)
       return;
    verinfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
    GetVersionEx((OSVERSIONINFO *)&verinfo);
    os_name = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,10),16);
    os_arch = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,11),16);
    os_kernelver = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,12),16);
    strcpy(os_name, "Windows ");
    *osver = 0;
    switch (verinfo.dwMajorVersion) {
    case 5:
        switch (verinfo.dwMinorVersion) {
        case 0:
            strcat(os_name, "2000");
            break;
        case 1:
            strcat(os_name, "XP");
            if (verinfo.wSuiteMask & VER_SUITE_PERSONAL) {
                strcpy(osver, "Home");
            } else {
                strcpy(osver, "Professional");
            }
            break;
        case 2:
            if (verinfo.wProductType > 1) {
                strcat(os_name, "2003");
            } else {
                strcat(os_name, "XP");
                strcpy(osver, "Professional 64");
            }
        default:
            break;
        }
        break;
    case 6:
        switch (verinfo.dwMinorVersion) {
        case 0:
            if (verinfo.wProductType > 1) {
                strcat(os_name, "2008");
            } else {
                strcat(os_name, "Vista");
            }
            break;
        case 1:
            strcat(os_name, "7");
            break;
        default:
            break;
        }
        break;
    }

    /*
     * Get info structure for architecture and processor count
     */
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Control\\Session Manager\\Environment",
                      0, KEY_READ|KEY_WOW64_64KEY, &hKey);
    if (!rc) {
        bufsize = sizeof(temp);
        if (!RegQueryValueExA(hKey, "PROCESSOR_ARCHITECTURE", NULL, NULL, (LPBYTE)temp, &bufsize)) {
            if (!strcmp(temp, "X86"))
                strcpy(os_arch, "x86");
            else if (!strcmp(temp, "AMD64"))
                strcpy(os_arch, "x64");
            else
                strcpy_s(os_arch, 16,temp);
        }
        if (!RegQueryValueExA(hKey, "NUMBER_OF_PROCESSORS", NULL, NULL, (LPBYTE)temp, &bufsize))
            pcount = atoi(temp);
        RegCloseKey(hKey);
    }

    /*
     * Start the version with the edition name from the registry
     */
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion",
                      0, KEY_READ|KEY_WOW64_64KEY, &hKey);
    if (!rc) {
        bufsize = sizeof(temp);
        if (!RegQueryValueExA(hKey, "EditionID", NULL, NULL, (LPBYTE)temp, &bufsize))
            strcpy(osver, temp);
        RegCloseKey(hKey);
    }
    sprintf(os_kernelver, "%d", verinfo.dwBuildNumber);
    if (verinfo.wServicePackMajor) {
        if (*osver) {
            strcat(osver, " ");
        }
        sprintf(osver+strlen(osver), "SR%d", verinfo.wServicePackMajor);
    }
    os_ver = ism_common_strdup(ISM_MEM_PROBE(ism_memory_ismc_misc,1000),osver);

    /*
     * Fill in the processor information string.
     * We only look at the first processor and assume that the rest are the same.
     */
    rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0",
                      0, KEY_READ|KEY_WOW64_64KEY, &hKey);
    if (!rc) {
        bufsize = 4;
        if (RegQueryValueExA(hKey, "~MHz", NULL, NULL, (LPBYTE)&MHz, &bufsize))
            MHz = 0;
        bufsize = sizeof(processor_id);
        if (RegQueryValueExA(hKey, "Identifier", NULL, NULL, (LPBYTE)processor_id, &bufsize))
            strcpy(processor_id, os_arch);
        bufsize = sizeof(processor_name);
        if (RegQueryValueExA(hKey, "ProcessorNameString", NULL, NULL, (LPBYTE)processor_name, &bufsize))
            strcpy(processor_name, processor_id);
        pname = processor_name;
        while (*pname==' ')
            pname++;
        RegCloseKey(hKey);
    } else {
        *processor_name = 0;
        *processor_id   = 0;
        pname = "unknown";
        MHz = 0;
    }
    memstatus.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memstatus);
    totalmem = (int)(memstatus.ullTotalPhys / (1024 * 1024));
    os_processorinfo = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_ismc_misc,13),strlen(pname) + 40);
    sprintf(os_processorinfo, "%s (%d) %d MHz %d MB", pname, pcount, MHz, totalmem);

    /*
     * Get the hostname
     */
        bufsize = sizeof(hostname);
    if (!GetComputerNameExA(ComputerNameDnsFullyQualified, hostname, &bufsize))
        strcpy(hostname, "*");
    os_hostname = ism_common_strdup(ISM_MEM_PROBE(ism_memory_ismc_misc,1000),hostname);

    /*
     * Create the composite execution environment
     */
    if (*os_ver || *os_arch) {
        snprintf(hostname, sizeof(hostname), "%s %s %s", os_name, os_ver, os_arch);
        os_execenv = ism_common_strdup(ISM_MEM_PROBE(ism_memory_ismc_misc,1000),hostname);
    } else {
        os_execenv = os_name;
    }
}


/*
 * Get the platform info
 */
const char * ism_common_getPlatformInfo(void) {
    return os_execenv;
}


/*
 * Get the processor info
 */
const char * ism_common_getProcessorInfo(void) {
    return os_processorinfo;
}


/*
 * Get the kernel info
 */
const char * ism_common_getKernelInfo(void) {
    return os_kernelver;
}


/*
 * Get the hostname
 */
const char * ism_common_getHostnameInfo(void) {
    return os_hostname;
}


/*
 * Put this here so that it all links
 */
void ism_common_showWorkers(void)  {
}

int pthread_barrier_wait(pthread_barrier_t * x) {
	_InterlockedDecrement(x);
	while (*x > 0) {
		SleepEx(10, FALSE);
	}

	return 0;
}

#endif
