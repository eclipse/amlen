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

#ifndef __MQTTBENCHRATE_H
#define __MQTTBENCHRATE_H

#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <pthread.h>


/* ------------------------------------------------------------------------------------
 * Used to hold counts of messages and marked messages per call to the RF_select.
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	double   lastTime;     /* the last time RF_select was called */
	double   samplingRate; /* number of messages per second to timestamp; the sampling rate */
	double   messageCount; /* number of messages sent */
	double   selectCount;  /* number of messages marked or selected for timestamping */
	double   selectRatio;  /* expressed as a ratio of messages selected for timestamping
	                        * to the total number of messages sent.  it cannot be greater
	                        * than 1 (select every message for timestamping) */
} RateFilter;

/* ------------------------------------------------------------------------------------
 * Used to hold message counts, yield counts, and yields to messages sent ratios.  This
 * is used to determine how many scheduled yields to invoke per second in order to
 * achieve the specified message rate
 * ------------------------------------------------------------------------------------ */
typedef struct
{
	double   lastTime;         /* the last time MW_wait was called */
	double   rate;             /* the requested message send rate */
	double   messageCount;     /* number of messages sent */
	double   yieldCount;       /* number of yields scheduled */
	double   yieldsPerMessage; /* number of yields per message */
	double   yieldsPerSecond;  /* number of yields per second */
	uint64_t delay;          /* */
	int      timer_fd;            /* TimerTask file descriptor */
	int      valid;               /* Flag to indicate that the timer is valid. */
} MicroWait;


/* ***************************************************************************************
 * Function Prototypes
 * ***************************************************************************************/
void MW_init (MicroWait *, double);
void MW_wait (MicroWait *);

void initMessageSampler (RateFilter *, int, int);
int selectForSampling (RateFilter *);


#endif /* __MQTTBENCHRATE_H */
