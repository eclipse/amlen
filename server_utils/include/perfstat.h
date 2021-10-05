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

#ifndef __PERFSTAT_DEFINED
#define __PERFSTAT_DEFINED


/* These interfaces are defined in C */
#ifdef __cplusplus
extern "C" {
#endif

#define MAX_PROCESSORS 64    /* TODO: check this */

typedef struct mon_perfstats_t {
    int process_memory;    /* Process working set memory in K Bytes */
    int process_cpu;       /* Process CPU in permille */
    int process_user;      /* Process user CPU in permille */
    int process_kernel;    /* Process kernel CPU in permille */
    int system_realmemory; /* Real memory in the system in K Bytes */
    int system_usedmemory; /* Used memory in the system in K Bytes */
    int system_cpu;        /* System CPU in permille */
    int cpucount;          /* Count of filled in CPU entries */
    int cpu_used [MAX_PROCESSORS];     /* CPU used for individual CPUs in permille */
} mon_perfstats_t;

typedef struct ism_latencystat_t {
	uint32_t  *histogram;    /* contains the histogram data */
	double    histUnits;     /* units of the histogram in seconds (e.g. 1e-6 is microseconds) */
	int       histSize;		 /* the size of the histogram */
	int		  takesample;    /* used to determine when to take a measurement */
	uint64_t  count;         /* number of operations (equals samples + non-samples) */
	int       big;           /* number of measurements larger than the size of the histogram */
	int       max;           /* the max latency */
	int       count1Sec;     /* count of latency > 1 sec. */
	int       count5Sec;     /* count of latency > 5 sec. */
	void      *maxaddr;      /* job address for this max latency */
} ism_latencystat_t;

void ism_perf_initPerfstat(void);
void ism_perf_getPerfStats(mon_perfstats_t * pstat);

/* This will print the set of statistics shown below into a buffer */
extern void ism_common_printHistogramStats(ism_latencystat_t *stat, char *buf, int buflen);

#ifdef __cplusplus
}
#endif

#endif

