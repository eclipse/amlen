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

#include <sys/timerfd.h>
#include <time.h>

#include "mqttbench.h"
#include "mqttbenchrate.h"

/* ******************************** GLOBAL VARIABLES ********************************** */
/* Externs */
extern uint8_t g_StopProcessing;

extern int g_MBErrorRC;

extern double g_MsgRate;

/* *************************************************************************************
 * selectForSampling
 *
 * Description:  Will calculate the rate at which messages will be selected for
 *               timestamping based on rate of calls to this function.
 *
 *               RF_reset must be called at least once before calling this function
 *
 *   @param[in]  filter              = x
 *
 *   @return 0   = false (Do not select the current message for timestamping.)
 *           1   = true  (Timestamp the current message)
 * *************************************************************************************/
int selectForSampling (RateFilter *filter)
{
	/* --------------------------------------------------------------------------------
	 * Given that samplingRate is in units of messages per second deltaTime will
	 * converge to 1 second once the number of messages selected for latency sampling
	 * is greater than or equal to the requested sampling rate
	 * -------------------------------------------------------------------------------- */
	if (filter->selectCount >= filter->samplingRate) /* selects should be approx. selects per second*/
	{
		double deltaTime = sysTime() - filter->lastTime; /* deltaTime is time since
		                                                  * last call to RF_select */
		filter->lastTime += deltaTime; /* lastTime is initialized in RF_reset to
		                                * the time when RF_reset was called. Set
		                                * lastTime for the subsequent call to RF_select */

		/* ----------------------------------------------------------------------------
		 * If deltaTime is less than 3/4 of a second then we are selecting messages for
		 * sampling too fast.
		 * ---------------------------------------------------------------------------- */
		if (deltaTime < .75e0)
			deltaTime = .75e0;
		else
			/* if deltaTime is greated than 1 and a 1/4 second then we are
			 * selecting messages for sampling too slow */
			if (deltaTime > 1.25e0)
				deltaTime = 1.25e0;
			else
				/* otherwise divide the projected time plus the current delta time by 2*/
				deltaTime = .5e0 * (deltaTime + 1.0e0);

		/* ----------------------------------------------------------------------------
		 * Update estimation of the percentage of messages that need to be marked for
		 * timestamping.
		 * ---------------------------------------------------------------------------- */
		filter->selectRatio *= deltaTime;

		/* ----------------------------------------------------------------------------
		 * Safegaurd against the case where more than 1 select per message was
		 * calculated.   selectRatio cannot be > 1
		 * ---------------------------------------------------------------------------- */
		if (filter->selectRatio > 1.0e0)
			filter->selectRatio = 1.0e0;

		/* ----------------------------------------------------------------------------
		 * Recalibrate messages selected count and message sent count for the next call
		 * to RF_select.  This helps to accelerate the convergence of the sampling rate.
		 * ---------------------------------------------------------------------------- */
		filter->selectCount -= filter->samplingRate;
 		filter->messageCount -= filter->samplingRate;
	}

	/* Converge upon the sampling rate */
	filter->messageCount += filter->selectRatio;

	/* --------------------------------------------------------------------------------
	 * If messages selected for timestamping is less than the message count then select
	 * the current message  otherwise skip the current message
	 * -------------------------------------------------------------------------------- */
	if (filter->selectCount < filter->messageCount) {
		filter->selectCount++;
		return 1; /* select the current message for timestamping */
	}

	return 0; /* skip or do not select the current message for timestamping */
} /* selectForSampling */

/* *************************************************************************************
 * initMessageSampler
 *
 * Description:  Will set the message select filter based on the requested message send
 *               rate and the requested sampling rate.
 *
 *   @param[in]  filter              = x
 *   @param[in]  reqSelectRate       = x
 *   @param[in]  messageRate         = Desired message rate to run at.
 * *************************************************************************************/
void initMessageSampler (RateFilter *filter, int reqSelectRate, int messageRate)
{
	filter->lastTime = sysTime(); /* initialize the base time */

	/* --------------------------------------------------------------------------------
	 * If request select rate is set then set the sampling rate in the filter otherwise
	 * the default is 1000 messages selected per second.
	 * -------------------------------------------------------------------------------- */
	if (reqSelectRate > 0)
		filter->samplingRate = (double) reqSelectRate;
	else
		filter->samplingRate = 1.0e3;

	/* --------------------------------------------------------------------------------
	 * If the message send rate is negative (should not happen, but...) or the message
	 * send rate is less than or equal to the requested select rate then set the
	 * selectRatio to 1 ;  Otherwise set the selectRatio to the requested select rate
	 * divided by the message send rate.
	 * -------------------------------------------------------------------------------- */
	if (messageRate <= 0 || messageRate <= reqSelectRate)
		filter->selectRatio = 1.0e0;
	else
		filter->selectRatio = reqSelectRate / (double) messageRate;

	/* Initialize message sent and selected counts to 0 */
	filter->messageCount = filter->selectCount = 0e0;
} /* initMessageSampler */

/* *************************************************************************************
 * MW_init
 *
 * Description:  Initialize the microwait timer.  This technique of "sleeping" does not
 *               use any "sleep" function rather it continuously estimates how many
 *               scheduled yields need to be invoked per message or per second in order
 *               to achieve the requested message send rate
 *
 *   @param[in]  timer               = instance of the MicroWait timer object
 *   @param[in]  targetRate          = the desired rate
 * *************************************************************************************/
void MW_init (MicroWait *timer, double targetRate)
{
	timer->rate = targetRate;

	if (targetRate >= 1) {
		int numYields;
		double deltaTime;
		double baseTime;

		baseTime = sysTime();

		/* ----------------------------------------------------------------------------
		 * Do the initial time measurement for 1000 yields. this will be used to
		 * calibrate yield times later
		 * ---------------------------------------------------------------------------- */
		for ( numYields = 0 ; numYields < 1000 ; numYields++ )
			sched_yield();

		deltaTime = (sysTime() - baseTime) * MILLISECOND_EXP;
		timer->yieldsPerMessage = 1.5e0 / (deltaTime * timer->rate);
		timer->yieldsPerSecond = timer->yieldsPerMessage * timer->rate;

		int traceLevel = 6;
		if(timer->yieldsPerMessage < 100){
			traceLevel = 5;
			MBTRACE(MBINFO, traceLevel, "Yields per message is less than 100, consider adding another submitter thread (<-st #> command line parameter)\n");
		}

		MBTRACE(MBINFO, traceLevel, "Avg time of sched_yield() call: %f\n", deltaTime);
		MBTRACE(MBINFO, traceLevel, "yields/msg: %f\n", timer->yieldsPerMessage);
		MBTRACE(MBINFO, traceLevel, "yields/sec: %f\n", timer->yieldsPerSecond);

		timer->yieldCount = 0e0;
		timer->messageCount = 0e0;
		timer->timer_fd = 0;
	} else {
		timer->timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
		if (timer->timer_fd < 0) {
			timer->valid = 0;
			g_MBErrorRC = RC_ERROR_RATE_CONTROL_TIMER;
			MBTRACE(MBERROR, 1, "Unable to create the rate control timer (errno: %d).\n", errno);
			return;
		}

		if (timer->rate > 0)
			timer->delay = 1 / timer->rate;
		else
			timer->delay = 0.001;
	}

	timer->lastTime = sysTime();     /* last time MW_wait was called */
} /* MW_init */

/* *************************************************************************************
 * MW_wait
 *
 * Description:  Called once per loop of the submit message loop of each publisher thread
 *               this function continuously estimates how many scheduled yields need to be
 *               invoked per message or per second in order to achieve the requested
 *               message send rate.
 *
 *   @param[in]  timer               = x
 * *************************************************************************************/
void MW_wait (MicroWait *timer)
{
	if (timer->timer_fd == 0) {
		/* ----------------------------------------------------------------------------
		 * Given that messageRate is in units of messages per second deltaTime will
		 * converge to 1 second once the actual message send rate approaches the
		 * requested message send rate.
		 * ---------------------------------------------------------------------------- */
		if (timer->yieldCount >= timer->yieldsPerSecond) {
			double deltaTime;
			deltaTime = sysTime() - timer->lastTime;
			timer->lastTime += deltaTime; /* lastTime is initialized in MW_init to the
		                               	   * time when MW_init was called. Set lastTime
		                               	   * for the subsequent call to MW_wait */

			/* ------------------------------------------------------------------------
			 * This deltaTime check protects against an initial spike in message rate
			 * especially when running with multiple publishing threads.
			 * ------------------------------------------------------------------------ */
			deltaTime++;
			deltaTime *= .5e0;

			/* ------------------------------------------------------------------------
			 * If deltaTime is less than 3/4 of a second then we are yielding too much
			 * and need reduce the number of yields per second.  As the rate sinks below
			 * the target rate this will cause the rate to converge back up from below
			 * the target rate.
			 * ------------------------------------------------------------------------ */
			if (deltaTime < .75e0)
				deltaTime = .75e0;
			else
			{
				/* --------------------------------------------------------------------
				 * If deltaTime is greater than 1 and 1/4 of a second then we are
				 * yielding too little and need increase the number of yields per second.
				 * This causes a convergence from the top to the target message rate.
				 * -------------------------------------------------------------------- */
				if (deltaTime > 1.25e0)
					deltaTime = 1.25e0;
			}

			timer->yieldsPerMessage /= deltaTime; /* recalculate yield per message */
			timer->yieldsPerSecond = timer->yieldsPerMessage * timer->rate;

			/* ------------------------------------------------------------------------
			 * Reset the yield count and message count for the next call to MW_wait.
			 * This helps to accelerate the convergence of the yield rate.
			 * ------------------------------------------------------------------------ */
			timer->yieldCount = 0e0;
			timer->messageCount = 0e0;
		}

		/* Converge upon the yield rate */
		timer->messageCount += timer->yieldsPerMessage;

		/* ----------------------------------------------------------------------------
		 * If the yield count is less than the projected message count at which to send
		 * the next message then increment the yield count and yield the CPU to
		 * another thread.
		 * ---------------------------------------------------------------------------- */
		while (timer->yieldCount < timer->messageCount) {
			timer->yieldCount++;
			sched_yield();
		}
	} else {
		uint64_t rateExp;
		struct itimerspec tspec;

		tspec.it_value.tv_sec = timer->delay;
		tspec.it_value.tv_nsec = 0;
		tspec.it_interval.tv_sec = 0;
		tspec.it_interval.tv_nsec = 0;

		if (timerfd_settime(timer->timer_fd, 0, &tspec, NULL) >= 0) {
			if (read(timer->timer_fd, &rateExp, sizeof(uint64_t)) >= 0) {
				/* Increment the message count */
				timer->messageCount++;
			} else {
				memset(&tspec, 0, sizeof(tspec));
				timerfd_settime(timer->timer_fd, 0, &tspec, NULL);
				close(timer->timer_fd);
				g_MBErrorRC = RC_ERROR_READ_TIMER;
				g_StopProcessing = 1;
				MBTRACE(MBERROR, 1, "Failure reading the timer (fd: %d).\n", timer->timer_fd);
			}
		} else {
			close(timer->timer_fd);
			timer->valid = 0;
			g_MBErrorRC = RC_ERROR_SET_TIMER;
			g_StopProcessing = 1;
			MBTRACE(MBERROR, 1, "Unable to set the timer for rate control (fd: %d).\n", timer->timer_fd);
		}
	}
} /* MW_wait */
