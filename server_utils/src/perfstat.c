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
/*
 */
#include <ismutil.h>
#include <fcntl.h>
#include <sys/times.h>
#include <sys/time.h>
#include <perfstat.h>
#include <math.h>

#ifdef DMALLOC
#include "dmalloc.h"
#endif

static int getFileContent(const char * name, char * buf, int len);
static int getProcValue(const char * name, const char * buf);

static char * g_procbuf;
static int    g_procbuflen;
static double g_clocks;
static struct timeval old_tv;

static double   old_prockernel, old_procuser;
static double   old_total;
static uint64_t old_cpu_idle[MAX_PROCESSORS];
static double  old_cpu_total[MAX_PROCESSORS];
static uint64_t old_idle;

static pthread_mutex_t  statlock;

void ism_perf_initPerfstat(void) {
    pthread_mutex_init(&statlock, NULL);
    g_procbuflen = 32750;
    g_procbuf = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,133),g_procbuflen);
    g_clocks = (double)sysconf(_SC_CLK_TCK);
    gettimeofday(&old_tv, NULL);
}

void ism_perf_getPerfStats(mon_perfstats_t * pstat) {
    char procpid [64];
    char * cp;
    sprintf(procpid, "/proc/%d/status", getpid());
    struct tms cpu;
    int  maxcpu = 0;
    char * eos;
    struct timeval tv;
    int    cpunum;
    double procuser_d, prockernel_d, elapsed;

    pthread_mutex_lock(&statlock);
    memset(pstat, 0, sizeof(* pstat));
    memset(g_procbuf, 0, g_procbuflen);
    /*
     * Get the memory sizes
     */
    getFileContent(procpid, g_procbuf, g_procbuflen);
    pstat->process_memory = getProcValue("VmRSS", g_procbuf);
    getFileContent("/proc/meminfo", g_procbuf, g_procbuflen);
    pstat->system_realmemory = getProcValue("MemTotal", g_procbuf);
    pstat->system_usedmemory = pstat->system_realmemory -
        (getProcValue("MemFree", g_procbuf) + getProcValue("Cached", g_procbuf));

    /*
     * Parse /proc/stat to get system times
     */
    getFileContent("/proc/stat", g_procbuf, g_procbuflen);
    cp = g_procbuf;
    while (*cp) {
        /*
         * Process lines starting with "cpu"
         */
        if (cp[0]=='c' && cp[1]=='p' && cp[2]=='u') {
            uint64_t user = 0, nice = 0, kernel = 0, idle = 0;
            double total;
            cp+=3;
            if (*cp >='0' && *cp <= '9') {
                cpunum = strtoul(cp, &eos, 10);
                if (cpunum >= maxcpu)
                    maxcpu = cpunum+1;
            } else {
                eos = cp;
                cpunum = -1;
            }
            if (*eos == ' ')
                user = strtoull(eos, &eos, 10);          /* Parse user time*/
            if (*eos == ' ')
                nice = strtoull(eos, &eos, 10);          /* Parse nice time*/
            if (*eos == ' ')
                kernel = strtoull(eos, &eos, 10);      /* Parse kernel time*/
            if (*eos == ' ')
                idle = strtoull(eos, &eos, 10);          /* Parse idle time*/
            total = (double)(user+nice+kernel+idle);

            /*
             * For individual processors we just compute the used permille
             */
            if (cpunum >= 0) {
                if (cpunum < MAX_PROCESSORS) {
                    pstat->cpu_used[cpunum] = 1000 - (int)(((double)(idle-old_cpu_idle[cpunum]) * 1000) / (total - old_cpu_total[cpunum]));
                    old_cpu_idle[cpunum] = idle;
                    old_cpu_total[cpunum] = total;
                }
            }

            /*
             * For the composite, compute cpu permille and also user and kernel
             */
            else {
                pstat->system_cpu = 1000 - (int)(((double)(idle-old_idle) * 1000) / (total - old_total));
                old_idle = idle;
                old_total = total;
            }
        }

        /*
         * Go to the next line
         */
        while (*cp && *cp != '\n')
            cp++;
        if (*cp)
            cp++;
    }

    /*
     * Get process times and convert them to double for calculations
     */
    pstat->cpucount = maxcpu;
    times(&cpu);
    procuser_d   = (double)(cpu.tms_utime / g_clocks * 1000000000.0);
    prockernel_d = (double)(cpu.tms_stime / g_clocks * 1000000000.0);
    gettimeofday(&tv, NULL);
    elapsed = ((double)(tv.tv_sec - old_tv.tv_sec) * 1000000.0) + (tv.tv_usec - old_tv.tv_usec);
    pstat->process_user = (int)((procuser_d-old_procuser) / elapsed);
    pstat->process_kernel = (int)((prockernel_d-old_prockernel) / elapsed);
    pstat->process_cpu = (int)(((procuser_d + prockernel_d) - (old_procuser + old_prockernel)) / elapsed);
    old_procuser = procuser_d;
    old_prockernel = prockernel_d;
    old_tv = tv;

    pthread_mutex_unlock(&statlock);
}


/*
 * Read a whole file into a buffer.
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

/*
 * Get a value from a proc file
 */
static int getProcValue(const char * name, const char * buf) {
    const char * pos = strstr(buf, name);
    if (pos) {
        pos = strchr(pos, ':');
        if (pos) {
            return atoi(pos+1);
        }
    }
    return 0;
}

/*
 * This will print the set of statistics shown below into a buffer
 *
 * samplecount (number of samples in the histogram)
 * units (units of the histogram)
 * min, max, big (number of samples larger than the histogram)
 * avg, stddev, 99th percentile, and 99.9th percentile
 *
 * param (in) stat      - contains the histogram data
 * param (in) buf       - the buffer that the statistics will be printed to
 * param (in) buflen    - length of the buffer
 *
 * */
void ism_common_printHistogramStats(ism_latencystat_t *stat, char *buf, int buflen)
{
	uint64_t samplecount=0;   /* Number of samples in the histogram */
	uint64_t percentilesum=0; /* used to calculate the percentiles */
	uint64_t sumlatency=0;    /* The latency found at the percentile of all latency in the histogram */
	int percentilecount;      /* The sample count that is at the target latency percentile */
	int histIndex;            /* Array index used to walk the histogram array */
    int avg=0;
	int min=0x7FFFFFFF;
	double sigma=0e0;
	int stddev=0;
	int i99=0;
	int i999=0;
	Dl_info info;
	int dlrc;

	/* Get number of samples in the histogram */
	for ( histIndex = 0 ; histIndex < stat->histSize ; histIndex++ )
	{
		samplecount+=stat->histogram[histIndex];
		sumlatency+=stat->histogram[histIndex]*histIndex;
		if (stat->histogram[histIndex]!=0 && histIndex<min)
			min=histIndex;
	}

	if (samplecount) {
		avg=sumlatency/samplecount;

		for (histIndex=0;histIndex<stat->histSize;histIndex++){
			sigma += (histIndex-avg) * (histIndex-avg)*stat->histogram[histIndex];
		}
		stddev = sqrt(sigma/(double)samplecount);

		/* Calculate at what number of samples the target latency percentile will be found */
		percentilecount = (int)(.99 * (double) samplecount);

		/* -----------------------------------------------------------------------
		 * Moving from lowest index(latency) to highest index(latency) in the
		 * histogram find the index at which point the number of samples reaches
		 * the target latency percentile.
		 * ------------------------------------------------------------------------*/
		for ( histIndex = 0, percentilesum = 0 ; histIndex < stat->histSize ; histIndex++ )
		{
			if ((percentilesum < percentilecount) &&
					((percentilesum + stat->histogram[histIndex]) >= percentilecount))
			{
				i99 = histIndex;
				break;
			}

			percentilesum += stat->histogram[histIndex];
		}

		/* Calculate at what number of samples the target latency percentile will be found */
		percentilecount = (int)(.999 * (double) samplecount);

		/* -----------------------------------------------------------------------
		 * Moving from lowest index(latency) to highest index(latency) in the
		 * histogram find the index at which point the number of samples reaches
		 * the target latency percentile.
		 * ------------------------------------------------------------------------*/
		for ( histIndex = 0, percentilesum = 0 ; histIndex < stat->histSize ; histIndex++ )
		{
			if ((percentilesum < percentilecount) &&
					((percentilesum + stat->histogram[histIndex]) >= percentilecount))
			{
				i999 = histIndex;
				break;
			}

			percentilesum += stat->histogram[histIndex];
		}

		if (stat->maxaddr != NULL && 0 != (dlrc = dladdr(stat->maxaddr, &info))) {
			snprintf(buf,buflen,"count=%llu samples=%llu, units(seconds)=%g, min=%d, max=%d in function %s, big=%d, avg=%d, stddev=%d, 99th=%d, 99.9th=%d",
					(unsigned long long)stat->count,(unsigned long long) samplecount,stat->histUnits,min,stat->max, info.dli_sname,stat->big,avg,stddev,i99,i999);
		} else {
			snprintf(buf,buflen,"count=%llu samples=%llu, units(seconds)=%g, min=%d, max=%d, big=%d, avg=%d, stddev=%d, 99th=%d, 99.9th=%d",
					(unsigned long long)stat->count,(unsigned long long) samplecount,stat->histUnits,min,stat->max, stat->big,avg,stddev,i99,i999);
		}
	}
	else {
		if (stat->maxaddr != NULL && 0 != (dlrc = dladdr(stat->maxaddr, &info))) {
			snprintf(buf,buflen,"units(seconds)=%g, max=%d in function %s, big=%d",
					 stat->histUnits,stat->max, info.dli_sname, stat->big);
		} else {
			snprintf(buf,buflen,"units(seconds)=%g, max=%d, big=%d",
					 stat->histUnits,stat->max, stat->big);
		}
	}
}


