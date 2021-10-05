/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <ismutil.h>
#include <ismxmlparser.h>
#include <fcntl.h>
#include <sys/utsname.h>
#include <dirent.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <assert.h>
#include <openssl/opensslv.h>
#include <openssl/rand.h>
#include <ismjson.h>


#ifdef DMALLOC
#include "dmalloc.h"
#endif


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
int    g_MHz = 0;
static int    numpcores = 1; //number of physical (not logical) cores
static int    numlcpu = 1; //num logical cpus (a 1 physical core cpu capable of 2 hyperthreads would have 2 in this variable)
static int    platform_isVM = -1;
static char * platform_serial_number = NULL;
static char * server_uid = NULL;
static char * server_name = NULL;
static ism_platformType_t platform_type = PLATFORM_TYPE_UNKNOWN;
static ism_licenseType_t platform_license_type = PLATFORM_LICENSE_UNKNOWN;
static int    platform_data_inited = 0;
static int    messaging_started = 0;
extern pthread_mutex_t     g_utillock;

static int cgroupMemLimited = 0;
static uint64_t g_ismTotalMemMB = 0;
static int useTSC = 0;

static int inContainer = 0;

static int ISM_CGROUP_PATH_MAX=1024; //longest path to a file in a cgroup directory

static const char *cgroupIdentifier[] = {"0:", NULL, ":cpu", NULL, NULL, NULL, NULL, NULL,":memory"};

static const char *cgroupPathPrefix[] = {"/sys/fs/cgroup", NULL,
                                         "/sys/fs/cgroup/cpu",
                                          NULL, NULL, NULL, NULL, NULL,
                                         "/sys/fs/cgroup/memory"};


/**
 * Read a whole file into a buffer.
 *
 * remark: The name can safely be a pointer into buf (but will be overwritten)
 */
static int getFileContent(const char * name, char * buf, int len) {
    int    f;
    int    bread;
    int    totallen = 0;
    f = open(name, O_RDONLY);
    if (f < 0)
        return 0;
    for (;;) {
        bread = read(f, buf+totallen, len-1-totallen);
        if (bread < 1) {
            if (errno == EINTR)
                continue;
            break;
        }
        totallen += bread;
    }
    buf[totallen] = 0;
    close(f);
    return totallen > 1 ? 1 : 0;
}

static void parseRedHat(char * buf);
static void parseCpuInfo(char * buf, struct utsname * unam, char * processor_name);
static void parseLsb(char * buf);
static void parseSuSE(char * buf);
static void parseMcp(char * buf);

static int ism_config_autotune_setATProp(const char *prop, int value);

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
 * Get the kernel level info
 */
const char * ism_common_getKernelInfo(void) {
    return os_kernelver;
}

/*
 * Get the hostname info
 */
const char * ism_common_getHostnameInfo(void) {
    return os_hostname;
}

/*
 * Initialize the TSC
 */
extern void ism_common_initTSC(void);
static void initTSC(void) {
    if (useTSC) {
        char buf[1024];
        if (getFileContent("/sys/devices/system/clocksource/clocksource0/current_clocksource", buf, sizeof(buf))) {
            if (strstr(buf, "tsc")) {
                ism_common_initTSC();
            }
        }
    }
}

//****************************************************************************
/// @brief Read /proc/<pid>/cgroup and determine group name
///
/// @param[in]     grouptype   Group to read. Usually memory or the newer unified cgroup
/// @param[in]     pid         Process Id to read data for (if 0 uses current process)
/// @param[in]     buf         Buffer this function can use for storage
///                             (Will be overwritten)
/// @param[in]     buflen      Size of the buffer provided to this function
/// @param[out]    groupname   Name of the requested cgroup (null-terminated).
///                            This is a pointer into buf so copy/use it before altering
///                                                                         buf contents
///
/// @return ISMRC_OK on success otherwise returns an error. Common errors:
///            ISMRC_FileCorrupt if can't parse the file contents
///            ISMRC_NotFound if no such group is listed
///            ISMRC_ObjectNotFound if the file is missing entirely or we can't read it
//*******************************************************************************

static int  ism_common_getCGroupName( ism_cgrouptype_t grouptype
                                    , int pid
                                    , char *buf
                                    , int buflen
                                    , char **groupname)
{
    char filename[128];
    const char *cgroupIndexThisProcess = "/proc/self/cgroup";
    const char *cgroupIndexByPID = "/proc/%d/cgroup";
    int rc = ISMRC_OK;

    assert(cgroupIdentifier[grouptype] != NULL);

    if (pid > 0) {
        sprintf(filename, cgroupIndexByPID, pid);
    } else {
        strcpy(filename, cgroupIndexThisProcess);
    }
    if (getFileContent(filename, buf, buflen)) {
        char *pos = strstr(buf,cgroupIdentifier[grouptype]);
        if (pos) {
            pos += strlen(cgroupIdentifier[grouptype]); //skip past the identifier for this cgroup type
            pos = strchr(pos, ':');
            if (pos) {
                pos++; //skip past colon
                char *endstr = strchr(pos, '\n');

                if (endstr) {
                    *endstr='\0'; //If no newline, groupname continues until end of buf
                }

                *groupname = pos;
            } else {
                rc = ISMRC_FileCorrupt;
            }
        } else {
            rc = ISMRC_NotFound;
        }
    } else {
        rc = ISMRC_ObjectNotFound;
    }

    return rc;
}


int ism_common_getCGroupPath(ism_cgrouptype_t groupType,
                             char *buf,
                             int buflen,
                             char **outPath,
                             int *pToplevelCGroup)
{
    char *cgroupName = NULL;
    bool foundPath=false;

    assert(cgroupPathPrefix[groupType] != NULL);

    int rc = ism_common_getCGroupName(groupType, 0,
                                      buf, buflen,
                                      &cgroupName);

    if (ISMRC_OK == rc) {
        size_t namelen = strlen(cgroupName)+1;//We want to include null terminator in length

        //Move the group name to the start of the buffer and cut that bit of the buffer off
        memmove(buf, cgroupName, namelen);
        cgroupName = buf;
        buf += namelen;
        buflen -= namelen;

        //First assume we can see the cgroup hierarchy
        assert(cgroupName[0] == '/');
        snprintf(buf, buflen, "%s%s", cgroupPathPrefix[groupType], cgroupName);

        struct stat statBuf;
        if (stat(buf, &statBuf) == 0) {
            *outPath = buf;
            foundPath = true;
        }

        if (!foundPath) {
            //Assume that we can't see full cgroup hierarchy and we'll
            //use top level group - common in a container
            snprintf(buf, buflen, "%s", cgroupPathPrefix[groupType]);

            if (stat(buf, &statBuf) == 0) {
                foundPath = true;
                *outPath = buf;

                if (pToplevelCGroup != NULL) {
                    *pToplevelCGroup = true;
                }
            }
        }

        if (foundPath) {
            rc = ISMRC_OK;
        }
    }

    return rc;
}


/**
 * There is no "correct" way to check we are in a container, partly
 * as there is not strict definition of container.
 *
 * We use a heuristic of looking at the init process (process 1) If this
 * has a cgroup that looks "top level" (0 or 1 slashes), we decide
 * it's the real init process for the OS and therefore decide we aren't
 * in a container - otherwise we are.
 */

static void checkContainer(char *buf, int buflen) {
    char *cgroupName = NULL;

    inContainer = 0;

    FILE* fin = fopen("/.dockerenv","r");
    if(fin) {
        inContainer = 1;
        fclose(fin);
    }

    int rc = ism_common_getCGroupName(ISM_CGROUP_MEMORY, 1,
                                      buf, buflen,
                                      &cgroupName);

    if (rc != ISMRC_OK) {
        //Didn't find a memory cgroup, is the system using the v2 "unified" cgroups?
        rc = ism_common_getCGroupName(ISM_CGROUP_UNIFIED, 1,
                                      buf, buflen,
                                      &cgroupName);
    }

    if (ISMRC_OK == rc) {
        //How many /'s are in the group name - 1 or fewer and we decide it's a top-level OS wide init
        char *pos = cgroupName;
        assert (pos != NULL);
        uint32_t slashCount = 0;
        do {
            pos = strstr(pos, "/");

            if (pos) {
                slashCount++;
                pos++; //skip past the /
            }
        } while (pos);

        if (slashCount > 1) {
            inContainer = 1;
        }
    }
}

/**
 * Find available memory for MessageSight - we attempt to check if we are running in a cgroup e.g. in a container.  If we are,
 * then read cgroup info otherwise, if we are not in a docker container read /proc/meminfo.
 *
 * NOTE: For build, cunit, and other small test environments the available memory can be overridden by setting the
 * IMATotalMemSizeMB variable in either the static configuration file or as an environment variable.  The environment
 * variable overrides the static configuration file.
 *
 */
static void checkTotalMemory(char *buf, int buflen) {
    char *cgroupPath = NULL;
    char * pos;

    g_ismTotalMemMB = ism_common_getIntConfig("IMATotalMemSizeMB",0);
    char *envVar = getenv("IMATotalMemSizeMB");
    if(envVar){
        int memSizeMB = atoi(envVar);
        if(memSizeMB > 0)
            g_ismTotalMemMB = memSizeMB;
    }

    if(g_ismTotalMemMB == 0) {
        int toplevelCGroup = 0;
        int rc = ism_common_getCGroupPath(ISM_CGROUP_MEMORY,
                                          buf, buflen,
                                          &cgroupPath,
                                          &toplevelCGroup);

        if (rc == ISMRC_OK) {
            const char *memLimitFileName = "memory.limit_in_bytes";
            char memFilePath[ISM_CGROUP_PATH_MAX+1];

            snprintf(memFilePath, ISM_CGROUP_PATH_MAX, "%s/%s", cgroupPath, memLimitFileName);

            if(getFileContent(memFilePath, buf, buflen)){
                g_ismTotalMemMB = strtoull(buf,NULL,10);
                cgroupMemLimited = 1;
            }
        } else {
            //Didn't find a memory cgroup, is the system using the v2 "unified" cgroups?
            rc = ism_common_getCGroupPath(ISM_CGROUP_UNIFIED,
                                          buf, buflen,
                                          &cgroupPath,
                                          &toplevelCGroup);

            if (rc == ISMRC_OK) {
                const char *memLimitFileName = "memory.max";
                char memFilePath[ISM_CGROUP_PATH_MAX+1];
                snprintf(memFilePath, ISM_CGROUP_PATH_MAX, "%s/%s", cgroupPath, memLimitFileName);

                if(getFileContent(memFilePath, buf, buflen)){
                    g_ismTotalMemMB = strtoull(buf,NULL,10);
                    cgroupMemLimited = 1;
                }
            }
        }

        if(g_ismTotalMemMB == 0 || g_ismTotalMemMB >= 0x7FFFFFFFFFFFF000){    /* if running not in a container and/or memlimit is not set, then query host memory from /proc/meminfo */
            cgroupMemLimited = 0;
            if (getFileContent("/proc/meminfo", buf, buflen)) {
                pos = strstr(buf, "MemTotal");
                if (pos) {
                    pos = strchr(pos+8, ':');
                    if (pos) {
                        g_ismTotalMemMB = strtoull(pos+1,NULL,10) * 1024;
                    }
                }
            }
        }

        g_ismTotalMemMB /= (1024*1024);  /* total memory in MB */
    }
}

static void checkProcessors( char *buf
                           , int buflen
                           , struct utsname * unam)
{
    char processor_name [256];
    processor_name[0] = 0;
    processor_name[255] = 0;
    bool limitedByNProc  = false;
    bool limitedByCGroup = false;
    int numlcpu_prelimit = 0;


    if (getFileContent("/proc/cpuinfo", buf, buflen)) {
        parseCpuInfo(buf, unam, processor_name);
    }
    os_processorinfo = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,237),strlen(processor_name) + 128);
    numlcpu_prelimit = numlcpu;

    /*
     * Although we just got the processor speed from /proc/cpuinfo, it can be the slowed down
     * speed for a variable clock.  Get the maximum processor speed.
     */
    if (getFileContent("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", buf, buflen)) {
        int value = strtoul(buf, NULL, 10);
        if (value) {
            g_MHz = value/1000;
        }
    }


    //We can be limited to run on fewer CPUs, check that:
    FILE *fin = popen("nproc", "r");
    if(fin) {
        char * line;
        line = fgets(buf,buflen,fin);
        if(line) {
            int numlcpu_reported_by_nproc = atoi(line);

            if (  (numlcpu_reported_by_nproc < numlcpu)
                &&(numlcpu_reported_by_nproc > 0)) {
                numlcpu = numlcpu_reported_by_nproc;
                limitedByNProc = true;
            }
        }
        pclose(fin);
    }

    //We can be limited by cgroups to run on fewer CPUs, check that:
    char *cgroupPath = NULL;
    int64_t quotaLimit = 0;  //signed as -ve if no limit
    int64_t quotaPeriod = 0;
    int toplevelCGroup = 0;
    int rc = ism_common_getCGroupPath(ISM_CGROUP_CPU,
                                      buf, buflen,
                                      &cgroupPath,
                                      &toplevelCGroup);

    //If we find that cgroup we are using cgroup v1
    if (rc == ISMRC_OK) {
        const char *quotaLimitFileName = "cpu.cfs_quota_us";
        const char *quotaPeriodFileName = "cpu.cfs_period_us";

        char filePath[ISM_CGROUP_PATH_MAX+1];

        snprintf(filePath, ISM_CGROUP_PATH_MAX, "%s/%s", cgroupPath, quotaLimitFileName);

        if(getFileContent(filePath, buf, buflen)){
            quotaLimit = strtol(buf,NULL,10);
        }

        if (quotaLimit > 0) {
            snprintf(filePath, ISM_CGROUP_PATH_MAX, "%s/%s", cgroupPath, quotaPeriodFileName);

            if(getFileContent(filePath, buf, buflen)){
                quotaPeriod = strtol(buf,NULL,10);
            }
        }
    } else {
        rc = ism_common_getCGroupPath(ISM_CGROUP_UNIFIED,
                                      buf, buflen,
                                      &cgroupPath,
                                      &toplevelCGroup);

        //If we find that cgroup we are using cgroup v1
        if (rc == ISMRC_OK) {
            const char *cpuFileName = "cpu.max";

            char filePath[ISM_CGROUP_PATH_MAX+1];

            snprintf(filePath, ISM_CGROUP_PATH_MAX, "%s/%s", cgroupPath, cpuFileName);

            if(getFileContent(filePath, buf, buflen)){
                   quotaLimit = strtol(buf,NULL,10);
            }

            if (quotaLimit > 0) {
                  char *pos = strstr(buf, " ");

                  if (pos != NULL) {
                      quotaPeriod = strtol(pos,NULL,10);
                  }
            }
        }
    }

    if (quotaPeriod > 0 && quotaLimit > 0) {
        double quotaCPU = quotaLimit / (1.0 * quotaPeriod);

        int cpuLimitRounded = lrint(quotaCPU);

        if (cpuLimitRounded < 1) {
            //We have a limit but it's less than 1 CPU, use 1 cpu
            cpuLimitRounded = 1;
        }

        if (cpuLimitRounded < numlcpu) {
            numlcpu = cpuLimitRounded;
            limitedByCGroup = true;
        }
    }

    initTSC();

    if (!g_MHz) {
        sprintf(os_processorinfo, "%s (%d%s%s) %llu MB%s", processor_name,
                                               numlcpu,
                                               (limitedByNProc  ? " (Limited by NProc)":""),
                                               (limitedByCGroup ? " (Limited by CGroup)":""),
                                               (unsigned long long) g_ismTotalMemMB,
                                                    (cgroupMemLimited ? " (Limited by CGroup)":""));
    } else {
        if (numlcpu != numlcpu_prelimit) {
            sprintf(os_processorinfo, "%s (%d phys:%d log: %d%s%s) %d MHz %llu MB%s", processor_name,
                                               numpcores, numlcpu_prelimit, numlcpu,
                                               (limitedByNProc  ? " (Limited by NProc)":""),
                                               (limitedByCGroup ? " (Limited by CGroup)":""),
                                               g_MHz,
                                               (unsigned long long) g_ismTotalMemMB,
                                               (cgroupMemLimited ? " (Limited by CGroup)":""));
        } else {
            sprintf(os_processorinfo, "%s (%d phys %d log) %d MHz %llu MB%s", processor_name,
                                              numpcores, numlcpu,
                                              g_MHz,
                                              (unsigned long long) g_ismTotalMemMB,
                                              (cgroupMemLimited ? " (Limited by CGroup)":""));
        }
    }
    if (!useTSC)
        strcat(os_processorinfo, " notsc");
}



/**
 * Return the total memory available to MessageSight.
 * @return the total memory available to MessageSight in MBs
 */
XAPI uint64_t ism_common_getTotalMemory(void){
  return g_ismTotalMemMB;
}


/*
 * Initialize the execution environment
 * This is called just once to set the various values.
 */
void ism_common_initExecEnv(int exetype) {
    char hostname [512];
    char * domain = NULL;
    int   domainlen;
    char * buf;
    int  buflen = 128*1024;
    struct utsname unam;


    /*
     * Only run this once
     */
    if (os_name)
       return;
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,235),buflen);
    os_processorinfo = "";

    /*
     * Get the base data from uname
     */
    domainlen = 0;
    uname(&unam);
#if __USE_GNU
    if (*unam.domainname && strcmp(unam.domainname, "(none)")) {
        domain = unam.domainname;
#else
    if (*unam.__domainname && strcmp(unam.__domainname, "(none)")) {
        domain = unam.__domainname;
#endif
        domainlen = strlen(domain)+1;
    }
    os_kernelver = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),unam.release);
    os_hostname = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,236),strlen(unam.nodename)+domainlen+2);
    strcpy(os_hostname, unam.nodename);
    if (domain) {
        strcat(os_hostname, ".");
        strcat(os_hostname, domain);
    }

    /*
     * Set the architecture
     */
    if (!strcmp(unam.sysname, "Linux")) {
        if (memcmp(unam.machine, "ia", 2) && memcmp(unam.machine, "ppc", 3)) {
            if (strstr(unam.machine, "64")) {
                os_arch = "x64";
            } else if (strstr(unam.machine, "86")) {
                os_arch = "x86";
            }
        }
        if (getFileContent("/etc/redhat-release", buf, buflen)) {
            parseRedHat(buf);
        } else if (getFileContent("/etc/base-release", buf, buflen)) {
            parseMcp(buf);
        } else if (getFileContent("/etc/SuSE-release", buf, buflen)) {
            parseSuSE(buf);
        } else if (getFileContent("/etc/lsb-release", buf, buflen)) {
            parseLsb(buf);
        }

        checkContainer(buf, buflen);

        checkTotalMemory(buf, buflen);

        /* This is not done for bridge */
        if (exetype != 2) {
            // We have to set SslUseBuffersPool before ism_ssl_init
            // we support 4K concurrent connections per GB of allocated memory. double the max connection limit in case of large reconnect event
            pthread_mutex_lock(&g_utillock);
            ism_config_autotune_setATProp("TcpMaxConnections",(g_ismTotalMemMB/1024)*2*4000);
            ism_config_autotune_setATProp("SslUseBuffersPool", 0);  /* Disable TLS buffer pool by default, pool lock adds too much lock contention, getting much better performance using glibc malloc/free */
            pthread_mutex_unlock(&g_utillock);
        }

        /* Check number/frequency etc. of processors available to this process */
        checkProcessors(buf, buflen,
                        &unam);
    } else {
        os_processorinfo = "";
    }

    if (!os_name) {
        os_name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),unam.sysname);
        os_ver  = os_kernelver;
    }
    if (!os_arch) {
        os_arch = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),unam.machine);
    }
    if (!os_processorinfo) {
        os_processorinfo = os_arch;
    }


    if(inContainer) {
        sprintf(buf,"%s (in Container) ", os_name);
        os_name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),buf);
    }

    ism_common_free(ism_memory_utils_misc,buf);
    /*
     * Create the composite execution environment
     */
    if (*os_ver || *os_arch) {
        snprintf(hostname, sizeof(hostname), "%s %s %s", os_name, os_ver, os_arch);
        os_execenv = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),hostname);
    } else {
        os_execenv = os_name;
    }

}

/*
 * Parse the redhat-release file which exists for RHEL, Fedora, and CentOS
 */
static void parseRedHat(char * buf) {
    char * pos;
    char * epos;

    char osver[256];
    int update = 0;
    os_name = "RedHat Linux";
    osver[0] = 0;
    if (strstr(buf, " Enterprise ")) {
        strcat(osver, "RHEL ");
        if (strstr(buf, " AS ")) {
            strcat(osver, "AS ");
        }
        if (strstr(buf, " ES ")) {
            strcat(osver, "ES ");
        }
    } else  if (strstr(buf, "CentOS")) {
        os_name = "CentOS Linux";
    } else if (strstr(buf, "Fedora")) {
        os_name = "Fedora Linux";
    }
    pos = strstr(buf, "release");
    if (pos) {
        int  verlen = strlen(osver);
        pos += 8;
        while (*pos==' ')
            pos++;
        epos = pos;
        while (*epos && *epos != ' ' && *epos != '\n')
            epos++;
        memcpy(osver+verlen, pos, epos-pos);
        osver[verlen+(epos-pos)] = 0;
    }
    pos = strstr(buf, "Update");
    if (pos) {
        update = atoi(pos + 7);
    }
    if (update)
        sprintf(osver+strlen(osver), "u%d", update);
    os_ver = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),osver);
}



/*
 * Parse Mcp
 */
static void parseMcp(char * buf) {
    char osver[256];
    char * epos;
    char * pos;

    os_name = "MCP";
    pos = strstr(buf, "release");
    if (pos) {
        pos += 8;
        while (*pos==' ')
            pos++;
        epos = pos;
        while (*epos && *epos != ' ' && *epos != '\n')
            epos++;
        memcpy(osver, pos, epos-pos);
        osver[(epos-pos)] = 0;
    }
    os_ver = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),osver);
}



/*
 * Parse LSB
 */
static void parseLsb(char * buf) {
    char osver[256];
    char * epos;
    char * pos;

    pos = strstr(buf, "DISTRIB_ID");
    if (!pos) {
        os_name = "Linux";
    } else {
        pos += 11;
        epos = strchr(pos, '\n');
        if (epos) {
            *epos = 0;
            os_name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),pos);
            *epos = '\n';
        }
    }
    pos = strstr(buf, "DISTRIB_RELEASE");
    if (!pos) {
        osver[0] = 0;
    } else {
        pos += 16;
        epos = strchr(pos, '\n');
        if (epos) {
            *epos = 0;
            ism_common_strlcpy(osver, pos, sizeof(osver));
        }
    }
    os_ver = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),osver);
}


/*
 * Parse SuSE
 */
static void parseSuSE(char * buf) {
    char osver[256];
    char * pos;
    char * epos;

    osver[0] = 0;
    os_name = "SuSE Linux";
    if (strstr(buf, " Enterprise Server ")) {
        strcat(osver, "SLES ");
    }
    if (strstr(buf, " Enterprise Desktop ")) {
        strcat(osver, "SLED ");
    }
    pos = strstr(buf, "VERSION");
    if (pos) {
        pos += 10;
        epos = strchr(pos, '\n');
        if (epos && (epos-pos)<64) {
            *epos = 0;
            strcpy(osver+strlen(osver), pos);
        }
    }
    os_ver = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),osver);
}

xUNUSED static int getHardwareNumCPUs(void) {
	return sysconf(_SC_NPROCESSORS_ONLN);
}

/*
 * Parse /proc/cpuinfo
 */
static void parseCpuInfo(char * buf, struct utsname * unam, char * processor_name) {
    char * procpos;
    char * corepos;
    char * eos = NULL;
    int   procmap [8192];
    char * pos;
    char * epos;
    int    i;

    if(strstr(buf,"constant_tsc") && strstr(buf,"nonstop_tsc"))
        useTSC = 1;

    numlcpu = 0;
    numpcores = 0;
    memset(procmap, 0, sizeof(procmap));
    /*
     * Get the count of CPUs  (try to avoid counting hyper-threads for numpcores)
     */
    procpos = strstr(buf, "processor");
    while (procpos) {
        int which_proc = 64;
        int which_core = 64;
        pos = strstr(procpos, "physical id");
        if (pos) {
            corepos = strstr(pos, "core id");
            if (corepos) {
                pos = strchr(pos+11, ':');
                if (pos) {
                    which_proc = strtoul(pos+1, &eos, 10);
                    if (*eos > ' ') {
                        which_proc = 64;
                    } else {
                        pos = strchr(corepos+7, ':');
                        if (pos) {
                            which_core = strtoul(pos+1, &eos, 10);
                            if (*eos > ' ')
                                which_core = 64;
                        }
                    }
                }
            }
        }

        if (which_proc < 64 && which_core<64) {
            if(procmap[which_proc*64 + which_core]) {
            	// This CPU is a hyperthread of CPU numlcpu (store SMT sibling in upper 16 bits of procmap entry)
            	procmap[which_proc*64 + which_core] |= ((numlcpu+1) << 16);
            } else {
            	procmap[which_proc*64 + which_core] = numlcpu+1;
            }
        } else {
            numpcores++;
        }
        numlcpu++;
        procpos = strstr(procpos+10, "processor");
    }
    for (i=0; i<8192; i++) {
        if(procmap[i])
        	numpcores++;
    }

    /*
     * Get the processor name
     */
    *processor_name = 0;
    pos = strstr(buf, "model name");
    if (pos) {

        pos = strchr(pos+10, ':');
        if (pos) {
            int wasblank = 0;
            int ppos = 0;
            pos++;
            while (*pos == ' ')
                pos++;

            /* Remove the blanks Intel puts in the processor name */
            while (*pos && *pos != '\n' && ppos<255) {
                 if (*pos == ' ') {
                     if (!wasblank) {
                         wasblank = 1;
                         processor_name[ppos++] = ' ';
                     }
                 } else {
                     wasblank = 0;
                     processor_name[ppos++] = *pos;
                 }
                  pos++;
            }
            processor_name[ppos] = 0;
        }
    } else {
        pos = strstr(buf, "cpu\t");    /* Format on PPC */
        if (pos) {
            pos = strchr(pos+4, ':');
            if (pos) {
                pos++;
                while (*pos == ' ')
                    pos++;
                epos = strchr(pos, ' ');
                if (epos) {
                    *epos = 0;
                    strncpy(processor_name, pos, 255);
                    *epos = ' ';
                }
            }
        } else {
            strncpy(processor_name, unam->machine, 255);
        }
    }
    pos = strstr(buf, "cpu MHz");
    if (pos) {
        pos = strchr(pos+7, ':');
        if (pos) {
            g_MHz = atoi(pos+1);
        }
    } else {
        pos = strstr(buf, "clock");    /* Format on PPC */
        if (pos) {
            pos = strchr(pos+5, ':');
            if (pos) {
                g_MHz = atoi(pos+1);
            }
        }

    }

    TRACE(6, "cpuinfo: %s %d - %d physical cores, %d  logical cpus\n", processor_name, g_MHz, numpcores, numlcpu);
}

#define MAPPING_FILE "/etc/bedrock/interfaceMappings.xml"
#define IF_CONFIG_DIR "/etc/bedrock/config/default/ethernet-interface/"
#define IF_ETH_CONFIG_DIR  "/etc/bedrock/config/default/ethernet-interface/"
#define IF_AGG_CONFIG_DIR  "/etc/bedrock/config/default/aggregate-interface/"
#define IF_VLAN_CONFIG_DIR "/etc/bedrock/config/default/vlan-interface/"
#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

static const char * INTERFACE_DIRS[] = { IF_ETH_CONFIG_DIR, IF_AGG_CONFIG_DIR, IF_VLAN_CONFIG_DIR };
static const char * TEST_INTERFACE_DIRS[] = { "test/ethernet-interface/" };

static const char * default_map [8] =  {"eth0", "eth0", "eth1", "eth1", "eth2", "eth2", "eth3", "eth3"};
const char * * ism_ifmaps = default_map;
int            ism_ifmap_count = 4;

/*
 * Initialize the map
 */
int ism_common_ifmap_init(void) {
    static int inited = 0;
    char   source[16*1024];
    int    len;
    int    rc;
    int    count;
    xdom * ifmap;
    xnode_t * n;
    int    xrc = 0;

    if (!inited) {
        inited = 1;
        if (getFileContent(MAPPING_FILE, source, sizeof(source))) {
            len = strlen(source);
            ifmap = ism_xml_new("interfaceMappings");
       //     ism_xml_setLogx(ifmap, logxml);
            ism_xml_setOptions(ifmap, XOPT_NoNamespace);
            rc = ism_xml_parse(ifmap, source, len, 0);
            if (rc) {
                TRACE(6, "The interface map file is not valid. rc=%d\n", rc);
                xrc = 1;
            } else {
                count = 0;
                n = ism_xml_first(ifmap);
                while (n) {
                    if (!strcmp(n->name, "map")) {
                        count++;
                    }
                    n = ism_xml_next(ifmap, LEVEL_ALL);
                }
                ism_ifmaps = ism_common_calloc(ISM_MEM_PROBE(ism_memory_utils_misc,239),count*2, sizeof(const char *));
                n = ism_xml_first(ifmap);
                ism_ifmap_count = 0;
                while (n) {
                    if (!strcmp(n->name, "map")) {
                        char * virtname = ism_xml_getNodeValue(ifmap, n, "bedrock-name");
                        char * sysname  = ism_xml_getNodeValue(ifmap, n, "*");
                        if (virtname && sysname) {
                            ism_ifmaps[ism_ifmap_count*2]     = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),virtname);
                            ism_ifmaps[(ism_ifmap_count*2)+1] = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),sysname);
                            ism_ifmap_count++;
                            TRACE(8, "Interface map %d: virtual=%s system=%s\n", ism_ifmap_count, virtname, sysname);
                        }
                    }
                    n = ism_xml_next(ifmap, LEVEL_ALL);
                }
                ism_xml_free(ifmap);
            }
        } else {
            TRACE(6, "The interface map file was not found.\n");
            xrc = 1;
        }
    }
    return xrc;
}


/*
 * Find the specified name in the map.
 * A case independent compare is done.
 */
int findMap(const char * ifname, int start) {
    int  i;
    if (!ifname)
        return -1;
    start = !!start;
    for (i=0; i<ism_ifmap_count; i++) {
        if (!strcmpi(ifname, ism_ifmaps[i*2+start]))
            return i;
    }
    return -1;
}

/*
 * Map an interface between the virtual interface name and the system interface name.
 *
 * If the specified name is unknown, return the input map name.
 * @param ifname  The interface name to map
 * @return The map name
 */
const char * ism_common_ifmap(const char * ifname, int fromsystem) {
    int which = findMap(ifname, fromsystem);
    if (which < 0)
        return ifname;
    if (fromsystem)
        return ism_ifmaps[which * 2];
    else
        return ism_ifmaps[(which * 2)+1];
}


/*
 * Retrieve the IP Addresses of specified interface.
 */
XAPI char * ism_common_ifmapip(const char * ifmapname) {
	int count=0, finished=0;
	int i;
	char * result = NULL;
	char * ifname = NULL;
	char *index_str = NULL;
	int index = -1;
	int num_of_dirs;
	const char ** directories;
	char xbuf[8192];
	concat_alloc_t ipbuf = {xbuf, sizeof xbuf, 0};

	if (!ifmapname)
		return result;

	/* Get the ifname with index */
	int len = strlen(ifmapname);
	ifname = alloca(strlen(ifmapname) + 1);
	memcpy(ifname, ifmapname, len);
	ifname[len] = '\0';

	ifname = ism_common_getToken(ifname, " \t\r\n", "_\r\n", &index_str);

	if (!ifname)
		return result;

	if (index_str != NULL) {
		index = atoi(index_str);
	}

	memset(&xbuf, 0, sizeof xbuf);

	if (getenv("CUNIT") == NULL) {
		directories = INTERFACE_DIRS;
		num_of_dirs = sizeof(INTERFACE_DIRS) / sizeof(const char *);
	} else {
		directories = TEST_INTERFACE_DIRS;
		num_of_dirs = sizeof(TEST_INTERFACE_DIRS) / sizeof(const char *);
	}

	/*
	 * Go through all groups of network interfaces (ethernet, aggregate and vlan)
	 * and check if an interface with this name exists.
	 */
	for (i = 0; i < num_of_dirs; i++) {
		const char * ifconfigpath = directories[i];
		char   source[16*1024];
		len = strlen(ifconfigpath) + strlen(ifname) + 4;  /* +4 for .xml characters */
		char * mapifconfig = alloca(len+1);
		snprintf(mapifconfig,len+1,"%s%s.xml", ifconfigpath, ifname);

		if (getFileContent(mapifconfig, source, sizeof(source))) {
			len = strlen(source);
			xdom * ifmap;
			xUNUSED int    xrc = 0;

			ifmap = ism_xml_new("interfaceIP");
			ism_xml_setOptions(ifmap, XOPT_NoNamespace);
			xrc = ism_xml_parse(ifmap, source, len, 0);

			if (xrc == 0) {
				xnode_t * n = ism_xml_first(ifmap);
				while (n) {
					if (!strcmp(n->name, "I")) {
						char * ifip  = ism_xml_getNodeValue(ifmap, n, "*");
						if (ifip) {
							char * more;
							char * iptoken;
							char * temp = alloca(strlen(ifip)+1);
							strcpy(temp, ifip);
							more = temp;
							iptoken = (char *)ism_common_getToken(more, " ,\t\n\r", " ,\t\n\r", &more);
							while (iptoken) {
								TRACE(8, "Interface IP: ifname=%s ip=%s, file=%s\n", ifname, iptoken, mapifconfig);
								iptoken = ism_common_getToken(iptoken, " \t\r\n", "/\r\n", NULL);

								/*If IPv6 address, put them in brackets if not already*/
								char * xiptoken = iptoken;
								if (strstr(iptoken, ":") && *iptoken != '[') {
									int iptokenlen = strlen(iptoken);
									xiptoken = alloca(iptokenlen+2+1); //Plus 2 for brackets []
									sprintf(xiptoken, "[%s]", iptoken);
								}

								if (index>-1 && count==index) {
									ism_common_allocBufferCopyLen(&ipbuf, xiptoken,strlen(xiptoken));
									count++;
									finished=1;
									break;
								}
								if (index < 0) {
									if (count>0)
										ism_common_allocBufferCopyLen(&ipbuf, ",",1);

									ism_common_allocBufferCopyLen(&ipbuf, xiptoken,strlen(xiptoken));
								}
								count++;
								iptoken = (char *)ism_common_getToken(more, " ,\t\n\r", " ,\t\n\r", &more);
							}
						}
					}

					if (index > -1 && finished) break;
					n = ism_xml_next(ifmap, LEVEL_ALL);
				}
				ism_xml_free(ifmap);
			} else {
				TRACE(8, "Failed to parse the interface XML:  ifname=%s\n", ifname);
			}
		}

		if (index > -1 && finished) break;
	}

	if (count > 0) {
		result = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,240),ipbuf.used+1);
		memcpy(result, ipbuf.buf, ipbuf.used);
		result[ipbuf.used] = '\0';
	}

	ism_common_freeAllocBuffer(&ipbuf);
	return result;
}

/**
 * Retrieve the interface name with index for an IP.
 * The format of the return is interfacename_index.
 * For example: eth6_1
 *
 * Note: Caller need to free the return value.
 *
 * @param ip the IP address
 * @param admin_state the AdminState value (0 - disabled, 1 - enabled)
 * @return the interface name with the index associate with the ip.
 */
XAPI char * ism_common_ifmapname(const char * ip, int * admin_state) {
	char * result = NULL;
	char * xip=NULL;
	DIR *  dir;
	char filepath[1024];
	struct dirent * dents;
	struct stat st;
	int found=0;

	int i;
	int num_of_dirs;
	const char ** directories;

	if (!ip) {
		return result;
	}

	if (getenv("CUNIT") == NULL) {
		directories = INTERFACE_DIRS;
		num_of_dirs = sizeof(INTERFACE_DIRS) / sizeof(const char *);
	} else {
		directories = TEST_INTERFACE_DIRS;
		num_of_dirs = sizeof(TEST_INTERFACE_DIRS) / sizeof(const char *);
	}

	/*If IPv6, strip off brackets*/
	int iplen = strlen(ip);
	xip = alloca(iplen+1);
	memcpy(xip, ip, iplen);
	xip[iplen]='\0';

	if(xip[0] == '[' && xip[iplen-1] == ']'){
		xip = (char *) ism_common_getToken(xip, " [\t\n\r", " ]\t\n\r", NULL);
	}

	for (i = 0; i < num_of_dirs; i++) {
		const char * ifconfigpath = directories[i];

		/*Use default Config Plugin path */
		dir = opendir(ifconfigpath);
		if (!dir) {
			continue;
		}

		while ((dents = readdir(dir)) != NULL) {
			char * dname = dents->d_name;
			if (dname[0] != '.') {
				memset(filepath, '\0', sizeof(filepath));
				snprintf(filepath, sizeof(filepath), "%s%s", ifconfigpath, dname);

				lstat(filepath, &st);

				if (!S_ISDIR(st.st_mode)) {
					FILE * configFile = fopen(filepath, "rb");
					if (!configFile) {
						continue;
					}

					fseek(configFile, 0, SEEK_END);
					long fsize = ftell(configFile);
					fseek(configFile, 0, SEEK_SET);

					char *configBuffer = alloca(fsize + 1);

                    xUNUSED long bytesRead = 0;
                    bytesRead = fread(configBuffer, fsize, 1, configFile);
					fclose(configFile);
					configBuffer[fsize] = 0;

					int index = 0;
					int len = fsize;
					xdom * ifmap;
					xUNUSED int xrc = 0;

					ifmap = ism_xml_new("interfaceName");
					ism_xml_setOptions(ifmap, XOPT_NoNamespace);
					xrc = ism_xml_parse(ifmap, configBuffer, len, 0);

					if (ism_xml_parse(ifmap, configBuffer, len, 0) == 0) {
						xnode_t * n = ism_xml_first(ifmap);
						while (n) {
							if (!strcmp(n->name, "I")) {
								char *more = NULL, *iptoken = NULL;
								char * ifip = ism_xml_getNodeValue(ifmap, n, "*");

								if (ifip != NULL) {
									char *temp = alloca(strlen(ifip) + 1);
									strcpy(temp, ifip);
									more = temp;
									iptoken = (char *) ism_common_getToken(more, " ,\t\n\r", " ,\t\n\r", &more);
									while (iptoken) {
										/* Handle IP which have multiple values */
										iptoken = ism_common_getToken(iptoken, " \t\r\n", "/\r\n", NULL);
										if (iptoken ) {

											/*If IPv6, strip off brackets*/
											int iptokenlen = strlen(iptoken);
											char * xiptoken = alloca(iptokenlen+1);
											memcpy(xiptoken, iptoken, iptokenlen);
											xiptoken[iptokenlen]='\0';

											if(xiptoken[0] == '[' && xiptoken[iptokenlen-1] == ']'){
												xiptoken = (char *) ism_common_getToken(xiptoken, " [\t\n\r", " ]\t\n\r", NULL);
											}

											if(!strcmp(xip, xiptoken)){
												found = 1;
												break;
											}
										}
										index++;
										iptoken = (char *) ism_common_getToken(more, " ,\t\n\r", " ,\t\n\r", &more);
									}
								}
							}

							if (found)
								break;

							n = ism_xml_next(ifmap, LEVEL_ALL);
						}

						if (found) {
							/*Look for name*/
							n = ism_xml_first(ifmap);
							while (n) {
								if (!strcmp(n->name, "name")) {
									char * ifname = ism_xml_getNodeValue(ifmap, n, "*");
									if (ifname) {
										TRACE(8, "Interface IP: ifname=%s ip=%s idx=%d, dir=%s\n", ifname, ip, index, ifconfigpath);
										char tmpbuf[256];
										snprintf(tmpbuf, 256, "%s_%d", ifname, index);

										result = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),tmpbuf);
										break;
									}
								}
								n = ism_xml_next(ifmap, LEVEL_ALL);
							}

							if (admin_state) {
								/* Look for AdminState */
								n = ism_xml_first(ifmap);

								*admin_state = 1;

								while (n) {
									if (!strcmp(n->name, "AdminState")) {
										const char * state = ism_xml_getNodeValue(ifmap, n, "*");
										if (state) {
											TRACE(8, "AdminState: ifname=%s state=%s\n", result, state);
											*admin_state = (!strcasecmp(state, "Disabled"))?0:1;
										}
										break;
									}
									n = ism_xml_next(ifmap, LEVEL_ALL);
								}
							}
						}

						ism_xml_free(ifmap);

					} else {
						TRACE(8, "Failed to parse the interface XML:  ifname=%s\n", dname);
						continue;
					}

					if (found) {
						break;
					}
				}
			}
		}

		if (found && result) {
			break;
		}
	}

	return result;
}

/*
 * Return if a interface name is known
 */
int ism_common_ifmap_known(const char * ifname, int fromsystem) {
    return findMap(ifname, fromsystem) >= 0;
}


/*
 * Process the server UID and server name from static config if they are there
 */
int ism_common_initServerName(void) {
    const char * value;
    if (!server_name) {
        value = ism_common_getStringConfig("ServerName");
        if (value)
            server_name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value);
    }
    if (!server_uid) {
        value = ism_common_getStringConfig("ServerUID");
        if (value) {
            server_uid = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value);
        } else {
            ism_common_generateServerUID();
        }
        if (server_name == NULL)
            server_name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),server_uid);
    }
    return 0;
}

/*
 * Return the server name
 */
const char * ism_common_getServerName(void) {
    if (server_name)
        return server_name;
    if (server_uid)
        return server_uid;
    return "(none)";
}

/*
 * Return teh server UID
 */
const char * ism_common_getServerUID(void) {
    return server_uid;
}

/*
 * Set the server name
 */
void ism_common_setServerName(const char * value) {
    if (!value || !*value) {
        server_name = NULL;
    } else {
        if (!server_name || strcmp(value, server_name)) {
            if (server_name != NULL) ism_common_free(ism_memory_utils_misc, server_name);
            server_name = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value);
        }
    }
}

/*
 * Set the server UID
 */
void ism_common_setServerUID(const char * value) {
    if (!value)
        return;
    pthread_mutex_lock(&g_utillock);
    if (!server_uid || strcmp(value, server_uid)) {
        server_uid = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),value);
        TRACE(3, "Set server UID to: %s\n", value);
    }
    pthread_mutex_unlock(&g_utillock);
}

/*
 * Tell utils that messaging has started
 */
void ism_common_startMessaging(void) {
    messaging_started = 1;
}

/*
 * Get the out of build order address for ism_config_set_fromJSONStr()
 */
typedef int32_t (* ism_config_set_fromJSONStr_t)(const char *jsonStr, const char *object, int validate);
static ism_config_set_fromJSONStr_t config_set_dynamic = NULL;
XAPI void ism_common_setDynamicConfig(void * func) {
    config_set_dynamic = (ism_config_set_fromJSONStr_t) func;
}

XAPI int ism_common_setPlatformLicenseType(const char *licensedUsage) {
	int rc = ISMRC_OK;

    if (!strcmp(licensedUsage, "Developers"))
    	platform_license_type = PLATFORM_LICENSE_DEVELOPMENT;
    else if (!strcmp(licensedUsage, "Non-Production"))
    	platform_license_type = PLATFORM_LICENSE_TEST;
    else if (!strcmp(licensedUsage, "Production"))
    	platform_license_type = PLATFORM_LICENSE_PRODUCTION;
    else if (!strcmp(licensedUsage, "IdleStandby"))
    	platform_license_type = PLATFORM_LICENSE_STANDBY;
    else
    	rc = ISMRC_Error;

    return rc;
}

/*
 * Update a singleton configuration item of type string
 */
static void updateDynamicConfigItem(const char * name, const char * value) {
    char command[256];
    int    rc;

    int validate = 0;
    sprintf(command, "{ \"%s\":\"%s\" }", name, value);
    rc = config_set_dynamic(command, name, validate);
    if (rc) {
        TRACE(2, "Unable to set dynamic config: item=%s value=%s rc=%d\n", name, value, rc);
    }
}


static char base62 [62] = {
    '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
    'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
    'W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l',
    'm','n','o','p','q','r','s','t','u','v','w','x','y','z',
};

/*
 * Generate a server UID
 */
const char * ism_common_generateServerUID(void) {
    char * buf;

    pthread_mutex_lock(&g_utillock);

    /* Generate a new random server UID */
    buf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,241),10);
    uint64_t rval;
    uint8_t * randbuf = (uint8_t *)&rval;
    char * bp = buf;

#if OPENSSL_VERSION_NUMBER < 0x10100000L
    RAND_pseudo_bytes(randbuf, 8);
#else
    RAND_bytes(randbuf, 8);
#endif
    for (int i=0; i<8; i++) {
        *bp++ = base62[(int)(rval%62)];
        rval /= 62;
    }
    *bp++ = 0;
    server_uid = buf;
    TRACE(3, "Generate new ServerUID=%s\n", server_uid);
    updateDynamicConfigItem("ServerUID", server_uid);
    pthread_mutex_unlock(&g_utillock);
    return server_uid;
}

/*
 * String form of platform name
 */
const char * ism_common_platform_type_str(ism_platformType_t ptype) {
    switch (ptype) {
    case PLATFORM_TYPE_9005:       return "9005";       /* Real appliance - 9005 systems */
    case PLATFORM_TYPE_APPLIANCE:  return "Appliance";  /* Real appliance with nvDIMM    */
    case PLATFORM_TYPE_CCIVM:      return "CCI";        /* SoftLayer virtual server      */
    case PLATFORM_TYPE_CCIBM:      return "BMI";        /* SoftLayer Baremetal with      */
    case PLATFORM_TYPE_VMWARE:     return "VMware";     /* VMware                        */
    case PLATFORM_TYPE_XEN:        return "Xen";        /* XEN                           */
    case PLATFORM_TYPE_VIRTUALBOX: return "VirtualBox"; /* Virtual Box                   */
    case PLATFORM_TYPE_KVM:        return "KVM";        /* KVM                           */
    case PLATFORM_TYPE_AZURE:      return "Azure";      /* Microsoft AZURE               */
    case PLATFORM_TYPE_EC2:        return "EC2";        /* Amazone EC2                   */
    case PLATFORM_TYPE_DOCKER:     return "Docker";     /* Docker container              */
    default:
    case PLATFORM_TYPE_UNKNOWN:    return "Unknown";    /* Unknown                       */
    }
}

/*
 * Cache platform data variables
 * Expects platfm.dat file in config directory
 */
int ism_common_initPlatformDataFile(void) {
    int rc = ISMRC_OK;
    int fread = 0;

    char * platform_dat;
    char * cfgdir;
    char * bindir;
    char * script;
    char * pos;
    char * epos;
    char   buf [10*256];  /* Assumption - max 10 lines of 256 chars */

    /* Only read the file once */
    if (platform_data_inited)
        return 0;

    if ( inContainer == 1 ) {
        platform_data_inited = 1;
        return 0;
    }

    cfgdir = (char *)ism_common_getStringConfig("ConfigDir");
    if (!cfgdir) {
        cfgdir = IMA_SVR_INSTALL_PATH "/config";
    }

    platform_dat = alloca(strlen(cfgdir) + 16);
    sprintf(platform_dat, "%s/platform.dat", cfgdir);

    /*
     * Read the file contents
     */
    fread = getFileContent(platform_dat, buf, sizeof buf);
    if (fread == 0) {
        /* Create platform data file */
        TRACE(3, "Initialize the platform data file.\n");

        bindir = (char *)ism_common_getStringConfig("BinDir");
        if (!bindir)  {
            bindir = IMA_SVR_INSTALL_PATH "/bin";
        }

        int buflen = strlen(bindir)+24;
        char scriptbuf[buflen];
        script = &scriptbuf[0];
        sprintf(script, "%s/setPlatformData.sh", bindir);

        pid_t pid = vfork();
        if (pid < 0) {
            TRACE(1, "Could not vfork process to call setPlatformData.sh\n");
            return ISMRC_Error;
        }
        if (pid == 0){
            if (execl(script, "setPlatformData.sh", platform_dat, NULL) < 0) {
                //Can't trace after a vfork...
                _exit(1);
            }
        }

        int st;
        waitpid(pid, &st, 0);
        rc = WIFEXITED(st)? WEXITSTATUS(st):1;
        fread = getFileContent(platform_dat, buf, sizeof buf);
    }

    /*
     * If the file exists, read it
     */
    if (fread) {
        rc = 0;

        /* Check if platform is VM */
        pos = strstr(buf, "PLATFORM_ISVM");
        if (pos) {
            pos = strchr(pos+13, ':');
            if (pos) {
                pos += 1;
                if ( *pos == '1' ) {
                    platform_isVM = 1;
                } else {
                    platform_isVM = 0;
                }
            }
        }

        /* Get platform type */
        pos = strstr(buf, "PLATFORM_TYPE");
        if (pos) {
            pos = strchr(pos+13, ':');
            if (pos++) {
                int val = 0;
                while (*pos >= '0' && *pos <= '9') {
                    val = val*10 + (*pos-'0');
                    pos++;
                }
                platform_type = (ism_platformType_t)val;
            }
        }

        /* Get platform serial number */
        pos = strstr(buf, "PLATFORM_SNUM");
        if (pos) {
            pos = strchr(pos+13, ':');
            if (pos) {
                pos += 1;
                epos = pos;
                while (*epos && *epos != ' ' && *epos != '\n')
                    epos++;
                int snolen = (epos - pos ) + 1;
                platform_serial_number = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,242),snolen);
                memcpy(platform_serial_number, pos, epos-pos);
                platform_serial_number[(epos-pos)] = 0;
            }
        }
        TRACE(3, "PlatformInfo: Type=%s isVM=%d Serial=%s\n",
                ism_common_platform_type_str(platform_type), platform_isVM, platform_serial_number);
    }
    if (platform_isVM < 0) {
        platform_isVM = 1;
    }
    if (!platform_serial_number) {
        /* If platform.dat is not found, set some defaults */
        char * snum = NULL;
        snum = getenv("ISMSSN");
        char sbuf [16];
        if (snum == NULL) {
            uint32_t  sval;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
            RAND_pseudo_bytes((uint8_t *)&sval, 4);
#else
            RAND_bytes((uint8_t *)&sval, 4);
#endif
            sprintf(sbuf, "%u", sval%10000000);
            snum = sbuf;
        }
        platform_serial_number = ism_common_strdup(ISM_MEM_PROBE(ism_memory_utils_misc,1000),snum);

    }
    platform_data_inited = 1;
    return rc;
}


/*
 * Returns 1 if Platform is VM
 */
int ism_common_getPlatformIsVM(void) {
    if ( inContainer == 1 ) {
        platform_isVM = 1;
    }
    return platform_isVM;
}

/*
 * Get platform Type
 */
ism_platformType_t ism_common_getPlatformType(void) {
    if ( inContainer == 1 ) {
        return PLATFORM_TYPE_DOCKER;
    }
    return platform_type;
}

/*
 * Returns platform serial number
 */
const char * ism_common_getPlatformSerialNumber(void) {
    if ( inContainer == 1 ) {
        return server_uid;
    }
    return platform_serial_number;
}


/*
 * Updates accepted License type
 */
ism_platformType_t ism_common_updatePlatformLicenseType(void) {
    char   buf [10*256];   /* assumption - max 10 lines of 256 chars */
    char * fname;

    platform_license_type = PLATFORM_LICENSE_DEVELOPMENT;

    /* Read accepted.json file, if file doesn't exist, then get data from platform.dat file */
    char * cfgdir = (char *)ism_common_getStringConfig("ConfigDir");
    if (!cfgdir)
        cfgdir = IMA_SVR_INSTALL_PATH "/config";
    fname = alloca(strlen(cfgdir)+20);
    sprintf(fname, "%s/accepted.json", cfgdir);
    if (getFileContent(fname, buf, sizeof buf)) {
        if ( strstr(buf, "Developers"))
            platform_license_type = PLATFORM_LICENSE_DEVELOPMENT;
        else if ( strstr(buf, "Non-Production"))
            platform_license_type = PLATFORM_LICENSE_TEST;
        else if ( strstr(buf, "Production"))
            platform_license_type = PLATFORM_LICENSE_PRODUCTION;
    }
    return platform_license_type;
}

/**
 * Returns platform license type
 */
XAPI ism_licenseType_t ism_common_getPlatformLicenseType(void) {
    return platform_license_type;
}

/*
 * MessageSight autotuning - this logic was previously written in bash scripts (vmscale.sh) for appliances.  As
 * MessageSight has evolved from an appliance based form factor to software based packaging it no longer makes
 * to depend on external scripts.  The logic has been moved here.
 *
 * There are are three primary classes of system resources that affect MessageSight configuration: cpu, memory, and disk.
 * The autotuning function will attempt to adjust key MessageSight configuration parameters based on these resources
 * in order to optimize performance for most workloads.
 *
 * Autotuning is enabled by default, but any and all configuration parameters that are "autotuned" can be overridden by the
 * user by setting them in the server static configuration file.  This may be necessary to optimize for very specific workloads.
 *
 */

/* Autotuning constants */
#define ISM_AT_THREADS_CRITICAL "AdminIOP","AdminHA","IMAConfigHA","store","diskUtil" /* threads which cannot be starved (i.e. which must always be very responsive */
#define ISM_AT_THREADS_HOTRESERVE "tcplisten","shmPersist"  /* hot threads that we will reserve extra CPUs for */

/* Autotuning globals
 * g_assignedCPUMap   - the set of CPUs that this process was assigned to by the user, all child threads/processes will be restricted to this CPU mask
 *                      Only CRITICAL threads are permitted to run on these CPUs
 * g_hotRsrvCPUMap    - a subset of g_assignedCPUMap CPUs, on which HOTRESERVE threads are permitted to run on
 * g_hotCPUMap        - a subset of g_hotRsrvCPUMap CPUs on which all other threads/processes are permitted to run on in order to prevent CPU starvation
 *                      of the CRITICAL and HOTRESERVE threads
 * g_assignedCPUs     - the number of CPUs assigned to this process and all child threads/processes (i.e. CPU count in g_assignedCPUMap)
 * g_hotRsrvCPUs      - CPU count in g_hotRsrvCPUMap
 * g_hotCPUs          - CPU count in g_hotCPUs */
static char g_assignedCPUMap[CPU_SETSIZE] = {0};
static char g_hotRsrvCPUMap[CPU_SETSIZE] = {0};
static char g_hotCPUMap[CPU_SETSIZE] = {0};
static int g_assignedCPUs = 0;
static int g_hotRsrvCPUs = 0;
static int g_hotCPUs = 0;

/* Check if autotuning property is set. Set property ONLY if the user has not configured it. */
static int ism_config_autotune_setATProp(const char *prop, int value) {
	char val[128];
	const char *isPropSet;
	ism_field_t  f;
	f.type = VT_String;
	f.val.s = val;
	isPropSet = ism_common_getStringConfig(prop);
	if(!isPropSet) {
		sprintf(val,"%d",value);
		ism_common_setProperty(ism_common_getConfigProperties(),prop, &f);
		return value;
	} else {
		return ism_common_getIntConfig(prop,0);
	}
}

/*
 * Dynamically tune the server configuration based on available resources and empirical data from performance testing.
 * Any and all of the configuration parameters set here can be overridden by the user in the server static configuration file.
 */
XAPI void ism_config_autotune(void){
	int i,j,k;
	int crit,hotrsrv;
	int numIOP, numSec, numAP, numHATX;
	cpu_set_t *assignedCPUSet;
	int maxConn;
	char assignedCPUMapStr[CPU_SETSIZE]={0}, hotCPUMapStr[CPU_SETSIZE]={0}, hotRsrvCPUMapStr[CPU_SETSIZE]={0};

	int nrcpu = sysconf(_SC_NPROCESSORS_CONF);

	if ( nrcpu < 1 ) nrcpu = 1;

	assignedCPUSet = CPU_ALLOC(nrcpu);
	CPU_ZERO_S(CPU_ALLOC_SIZE(nrcpu),assignedCPUSet);
	sched_getaffinity(0,CPU_ALLOC_SIZE(nrcpu),assignedCPUSet);  /* get the CPUs assigned to this process by the user */

	g_assignedCPUs = CPU_COUNT_S(CPU_ALLOC_SIZE(nrcpu),assignedCPUSet);

	if ( g_assignedCPUs < 1 ) g_assignedCPUs = nrcpu;

	//If we've been limited by cgroups etc... reduce the number of CPUS we'll use
	if (g_assignedCPUs > numlcpu) {
	    g_assignedCPUs = numlcpu;
	}

	crit = g_assignedCPUs/4 > 4 ? 4 : g_assignedCPUs/4; /* calculate how many CPU to reserve for CRITICAL threads (max 4) */
	hotrsrv = g_assignedCPUs/9 > 2 ? 2 : g_assignedCPUs/9; /* calculate how many CPU to reserve for HOTRESERVE threads (max 2) */
	g_hotRsrvCPUs = g_assignedCPUs - crit;
	g_hotCPUs = g_assignedCPUs - (crit + hotrsrv);

	/* Populate CPU maps for assigned, HOTRESERVE, and HOT CPU maps, start from the end of the map and remove CPUs
	 * from HOTRESERVE and HOT CPU maps as needed */
	for(i=nrcpu,j=crit,k=crit + hotrsrv; i>0 ; i--){
		if(CPU_ISSET_S(i-1,CPU_ALLOC_SIZE(nrcpu),assignedCPUSet)) {
			g_assignedCPUMap[i-1]=1;
			(j>0) ? j-- : (g_hotRsrvCPUMap[i-1]=1);
			(k>0) ? k-- : (g_hotCPUMap[i-1]=1);
		}
	}

	ism_common_affMaskToStr(g_assignedCPUMap,nrcpu,assignedCPUMapStr);
	ism_common_affMaskToStr(g_hotCPUMap,nrcpu,hotCPUMapStr);
	ism_common_affMaskToStr(g_hotRsrvCPUMap,nrcpu,hotRsrvCPUMapStr);

	CPU_FREE(assignedCPUSet);

	/*
	 * MessageSight configuration that should be scaled based on CPU resources
	 */

	/* Scale the IOP, AP, Security, and HA TX threads based on number of HOT CPUs
	 nrassign=1 nrcrit=0 nrhotrsrv=1 nrhot=1 iop=1 ap=1 sec=2 hatx=1
     nrassign=2 nrcrit=0 nrhotrsrv=2 nrhot=2 iop=2 ap=1 sec=3 hatx=1
     nrassign=3 nrcrit=0 nrhotrsrv=3 nrhot=3 iop=2 ap=2 sec=3 hatx=1
     nrassign=4 nrcrit=1 nrhotrsrv=3 nrhot=3 iop=2 ap=2 sec=3 hatx=1
     nrassign=5 nrcrit=1 nrhotrsrv=4 nrhot=4 iop=2 ap=2 sec=4 hatx=2
     nrassign=6 nrcrit=1 nrhotrsrv=5 nrhot=5 iop=3 ap=2 sec=4 hatx=2
     nrassign=7 nrcrit=1 nrhotrsrv=6 nrhot=6 iop=4 ap=2 sec=5 hatx=2
     nrassign=8 nrcrit=2 nrhotrsrv=6 nrhot=6 iop=4 ap=2 sec=5 hatx=2
     nrassign=9 nrcrit=2 nrhotrsrv=7 nrhot=6 iop=4 ap=2 sec=5 hatx=2
     nrassign=10 nrcrit=2 nrhotrsrv=8 nrhot=7 iop=4 ap=2 sec=5 hatx=2
     nrassign=11 nrcrit=2 nrhotrsrv=9 nrhot=8 iop=5 ap=2 sec=6 hatx=3
     nrassign=12 nrcrit=3 nrhotrsrv=9 nrhot=8 iop=5 ap=2 sec=6 hatx=3
     nrassign=13 nrcrit=3 nrhotrsrv=10 nrhot=9 iop=6 ap=3 sec=6 hatx=3
     nrassign=14 nrcrit=3 nrhotrsrv=11 nrhot=10 iop=6 ap=3 sec=7 hatx=3
     nrassign=15 nrcrit=3 nrhotrsrv=12 nrhot=11 iop=7 ap=3 sec=7 hatx=3
     nrassign=16 nrcrit=4 nrhotrsrv=12 nrhot=11 iop=7 ap=3 sec=7 hatx=3
     nrassign=17 nrcrit=4 nrhotrsrv=13 nrhot=12 iop=8 ap=4 sec=8 hatx=4
     nrassign=18 nrcrit=4 nrhotrsrv=14 nrhot=12 iop=8 ap=4 sec=8 hatx=4
     nrassign=19 nrcrit=4 nrhotrsrv=15 nrhot=13 iop=8 ap=4 sec=8 hatx=4
     nrassign=20 nrcrit=4 nrhotrsrv=16 nrhot=14 iop=9 ap=4 sec=9 hatx=4
     nrassign=21 nrcrit=4 nrhotrsrv=17 nrhot=15 iop=10 ap=5 sec=9 hatx=4
     nrassign=22 nrcrit=4 nrhotrsrv=18 nrhot=16 iop=10 ap=5 sec=10 hatx=5
     nrassign=23 nrcrit=4 nrhotrsrv=19 nrhot=17 iop=11 ap=5 sec=10 hatx=5
     nrassign=24 nrcrit=4 nrhotrsrv=20 nrhot=18 iop=12 ap=6 sec=11 hatx=5
     nrassign=25 nrcrit=4 nrhotrsrv=21 nrhot=19 iop=12 ap=6 sec=11 hatx=5
     nrassign=26 nrcrit=4 nrhotrsrv=22 nrhot=20 iop=13 ap=6 sec=12 hatx=6
     nrassign=27 nrcrit=4 nrhotrsrv=23 nrhot=21 iop=14 ap=7 sec=12 hatx=6
     nrassign=28 nrcrit=4 nrhotrsrv=24 nrhot=22 iop=14 ap=7 sec=13 hatx=6
     nrassign=29 nrcrit=4 nrhotrsrv=25 nrhot=23 iop=15 ap=7 sec=13 hatx=6
     nrassign=30 nrcrit=4 nrhotrsrv=26 nrhot=24 iop=16 ap=8 sec=14 hatx=7
     nrassign=31 nrcrit=4 nrhotrsrv=27 nrhot=25 iop=16 ap=8 sec=14 hatx=7
     nrassign=32 nrcrit=4 nrhotrsrv=28 nrhot=26 iop=17 ap=8 sec=15 hatx=7
     nrassign=33 nrcrit=4 nrhotrsrv=29 nrhot=27 iop=18 ap=9 sec=15 hatx=7
     nrassign=34 nrcrit=4 nrhotrsrv=30 nrhot=28 iop=18 ap=9 sec=16 hatx=8
     nrassign=35 nrcrit=4 nrhotrsrv=31 nrhot=29 iop=19 ap=9 sec=16 hatx=8
     nrassign=36 nrcrit=4 nrhotrsrv=32 nrhot=30 iop=20 ap=10 sec=17 hatx=8
	 ...
	 */
	numIOP=(g_hotCPUs*2)/3 + ((g_hotCPUs*2)/3 < 2 ? 1 : 0);
	numAP=g_hotCPUs/3 + (g_hotCPUs/3 <= 1 ? 1 : 0);
	numSec=g_hotCPUs/2 + 2;
	numHATX=g_hotCPUs/4 + 1;

    pthread_mutex_lock(&g_utillock);
	ism_config_autotune_setATProp("TcpThreads",numIOP > 100 ? 100 : numIOP); // max of 100 IOP threads
	ism_config_autotune_setATProp("Store.PersistAsyncThreads",numAP > 25 ? 25 : numAP); // max of 25 asyncPersist threads
	ism_config_autotune_setATProp("SecurityThreadPoolSize",numSec > 16 ? 16 : numSec); // max of 16 Security threads
	ism_config_autotune_setATProp("Store.PersistHaTxThreads",numHATX > 4 ? 4 : numHATX); // max of 4 HA Tx threads

	/*
	 * MessageSight configuration that should be scaled based on Memory resources
	 */
	// we support 4K concurrent connections per GB of allocated memory. double the max connection limit in case of large reconnect event
	maxConn = ism_config_autotune_setATProp("TcpMaxConnections",(g_ismTotalMemMB/1024)*2*4000);

	/* Transport and OpenSSL buffer pools*/
	ism_config_autotune_setATProp("TcpMaxTransportPoolSizeMB",g_ismTotalMemMB/16);
	ism_config_autotune_setATProp("SslUseBuffersPool", 0);  /* Disable TLS buffer pool by default, pool lock adds too much lock contention, getting much better performance using glibc malloc/free */
    pthread_mutex_unlock(&g_utillock);

    /* Protocol msgid hashmaps sized based on TcpMaxConnections
     * g_ismTotalMemMB(GB)	MsgId Map Mem(GB)
     * 16					0.25
     * 32					0.5
     * 64					1
     * 128					2
     * 256					4
     * */
    ism_config_autotune_setATProp("NumMsgIdMaps", MAX(10, (maxConn/1000)));
    ism_config_autotune_setATProp("MsgIdMapCapacity", 128 * 1024);

    /*
     * MessageSight configuration that should be scaled based on Disk resources
     */
    TRACE(2, "MessageSight autotuned configuration: mem=%lluMB, cpu=%d(%s) hot=%d(%s) hotrsrv=%d(%s) iop=%d ap=%d sec=%d hatx=%d maxconn=%d\n",
            (unsigned long long) g_ismTotalMemMB,g_assignedCPUs,assignedCPUMapStr,g_hotCPUs,hotCPUMapStr,g_hotRsrvCPUs,hotRsrvCPUMapStr,
            ism_common_getIntConfig("TcpThreads",0),
            ism_common_getIntConfig("Store.PersistAsyncThreads",0),
            ism_common_getIntConfig("SecurityThreadPoolSize",0),
            ism_common_getIntConfig("Store.PersistHaTxThreads",0),
            ism_common_getIntConfig("TcpMaxConnections",0));

}

/*
 * Pass back the user provided (static configuration) thread affinity for thread named by thrName, or if not provided
 * pass back the computed thread affinity mask.
 *
 * thrName (input) - name of the thread to return a CPU affinity map for
 * affMap (input/output) - memory allocated by caller, return the CPU map filled in with CPUs
 *                         that thread named by affStr is to be assigned to
 * return - the index of the last CPU which is set in affMap
 */
int ism_config_autotune_getaffinity(const char *thrName, char affMap[CPU_SETSIZE]){
	int lastSetCPUInMap = 0;
	int i, nelements;
	const char *affString;
	char cfgName[64];
	char *criticalThreads[] = {ISM_AT_THREADS_CRITICAL};
	char *hotReserveThreads[] = {ISM_AT_THREADS_HOTRESERVE};

	/* Check if the affinity for this thread has been set by the user in the server static configuration.  If
	 * yes, then return the user configured affinity map */
	sprintf(cfgName, "Affinity.%s", thrName);
	affString = ism_common_getStringConfig(cfgName);
	if (affString) {
		return ism_common_parseThreadAffinity(affString, affMap);
	}

	/* Check if the thread is one of the CRITICAL threads */
	nelements = sizeof(criticalThreads)/sizeof(criticalThreads[0]);
	for (i=0;i<nelements;i++) {
		if(!strncmp(criticalThreads[i],thrName,strlen(criticalThreads[i]))) {
			int j;
			for(j=0;j<CPU_SETSIZE;j++) {
				affMap[j] = g_assignedCPUMap[j];
				if(g_assignedCPUMap[j])
					lastSetCPUInMap = j + 1;
			}
			return lastSetCPUInMap;
		}
	}

	/* Check if the thread is one of the HOTRESERVE threads */
	nelements = sizeof(hotReserveThreads)/sizeof(hotReserveThreads[0]);
	for (i=0;i<nelements;i++) {
		if(!strncmp(hotReserveThreads[i],thrName,strlen(hotReserveThreads[i]))) {
			int j;
			for(j=0;j<CPU_SETSIZE;j++) {
				affMap[j] = g_hotRsrvCPUMap[j];
				if(g_hotRsrvCPUMap[j])
					lastSetCPUInMap = j + 1;
			}
			return lastSetCPUInMap;
		}
	}

	/* For all other child threads/processes use the HOT CPU map */
	for(i=0;i<CPU_SETSIZE;i++) {
		affMap[i] = g_hotCPUMap[i];
		if(g_hotCPUMap[i])
			lastSetCPUInMap = i + 1;
	}
	return lastSetCPUInMap;

}
