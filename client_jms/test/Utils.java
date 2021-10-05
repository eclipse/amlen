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

import java.io.File;
import java.io.FileOutputStream;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.Date;
import java.util.zip.CRC32;

import javax.jms.MessageConsumer;

/* 
 * Utils() class
 * 
 * This class provides the utilities for performing latency measurements. 
 * Included are Timer variables, which are in units of seconds with millisecond precision.  
 */
public class Utils {
	@SuppressWarnings("unused")
	private double total;        /* Total running time since the base time (base) in seconds */
	@SuppressWarnings("unused")
	private double delta;        /* The delta time in seconds since the last call to getTimer */
	private double base;         /* The start or initial base time in seconds that is set in setTimer */
	private double last;         /* The last time in seconds that getTimer was called */
	private double selectCount;  /* Counter that holds the number of messages selected for sampling */
	private double messageCount; /* Counter that holds the total number of messages sent */
	private double samplingRate; /* Rate at which the messages should be sampled (units are in msgs/sec) */
	private double lastTime;     /* The last time in seconds since selectForSampling was called */
	private double selectRatio;  /* Expressed as a ratio of messages selected for sampling to the total 
	                              * number of messages sent.  It cannot be greater than 1 (i.e. every
	                              * message for sampling) */
	
	static String thread_lat_hdr = "ThreadID,MsgCount,SampleCount,Min(us),Max(us),Avg(us),50th,StdDev,95th,99th,99.9th,99.99th,99.999th";
	static String aggr_lat_hdr = "Aggr,MsgCount,SampleCount,Min(us),Max(us),Avg(us),50th,StdDev,95th,99th,99.9th,99.99th,99.999th,MsgSize(bytes)";
	
	/* 
	 * setTimer()
	 *
	 * Initialize the milliseconds timer.  This function sets the base time. 
	 */
	public void setTimer() 
	{
		getTimer();      /* Get the current time in milliseconds */
		base = last;     /* Set the intial base time to the last time which is also the 
                          * current time since setTimer is called only once per monitor thread */
	}

	
	/* 
	 * getTimer()
	 * 
	 * Update the milliseconds timer.  This function sets the total running time, the delta 
	 * time, and the previous time(used to calculate the delta time) in milliseconds */
	public void getTimer() 
	{
		double curTOD;         /* current time of day (TOD) in milliseconds */
		long millis = System.currentTimeMillis(); /* invoke the standard API for getting the date and time */
		curTOD = (double) millis * 1.0e-3; /* current time in seconds.milliseconds */
		total = curTOD - base; /* total time in seconds since the call to setTimer */
		delta = curTOD - last; /* time in seconds since the last call to getTimer */
		last = curTOD;	       /* set last time for subsequent call to getTimer */
	}	
	
	/* 
	 * selectForSampling()
	 * 
	 * Calculates the rate at which messages will be selected for time stamping based on rate of 
	 * calls to this function.  
	 * 
	 * Returns a boolean: 
	 *   - true  = time stamp the current message  
	 *   - false = do not select the current message of time stamping.  
	 *   
	 * initMessageSampler must be called exactly once before calling this function.
	 */
	public boolean selectForSampling()
	{
		/* Given that samplingRate is in units of messages per second deltaTime will converge to 
		 * 1 second once the number of messages selected for latency sampling is greater than or 
		 * equal to the requested sampling rate */
		if (selectCount >= samplingRate) /* selects should be approx. samplingRate selects per second*/
		{
			/* 
			 * deltaTime is time since last call to selectForSampling().  lastTime is initialized
			 * in initMessageSampler(). Subsequent calls set the lastTime to the subsequent call 
			 * to selectForSampling(). 
			 */
			double deltaTime = (double) (System.nanoTime() * 1e-9) - lastTime;
			lastTime += deltaTime;
			
			/* If deltaTime is less than 3/4 of a second, then selecting messages for sampling 
			 * is too fast */
			if (deltaTime < .75e0)
			{
				deltaTime = .75e0; 
			} 
			else
			{
				/* If deltaTime is greater than 1 and a 1/4 second, then selecting messages for
				 * sampling is too slow */
				if (deltaTime > 1.25e0)
					deltaTime = 1.25e0; 
				else
					/* Otherwise divide the projected time plus the current delta time by 2*/
					deltaTime = .5e0 * (deltaTime + 1.0e0);    
	
				/* Update estimation of the percentage of messages that need to be marked for time stamping */
				selectRatio *= deltaTime; 

				/* Safeguard against the case where more than 1 select per message was calculated.
				 * selectRatio cannot be > 1 */
				if (selectRatio > 1.0e0)	
					selectRatio = 1.0e0;

				/* Re-calibrate messages selected count and message sent count for the next call to 
				 * RF_select.  This helps to accelerate the convergence of the sampling rate */
				selectCount -= samplingRate;   
				messageCount -= samplingRate;
			}
		}

		/* Converge upon the sampling rate */
		messageCount += selectRatio;
		
		/* If messages selected for time stamping is less than the message count then select the 
		 * current message otherwise skip the current message */
		if (selectCount < messageCount)
		{
			selectCount++; 
			return true; /* select the current message for time stamping */
		}
	
		return false; /* skip or do not select the current message for time stamping */
	}

	
	/* 
	 * initMessageSampler( )
	 * 
	 * Will set the message select filter based on the requested message send rate and the 
	 * requested sampling rate. 
	 */
	public void initMessageSampler(int selectRate, int messageRate)
	{
		lastTime = (double) (System.nanoTime() * 1e-9); /* initialize the base time */
	  
		/* If request select rate is set then set the sampling rate in the filter 
		 * otherwise the default is 1000 messages selected per second */
		if (selectRate > 0)
			samplingRate = (double) selectRate; 
		else
			samplingRate = 1.0e3;
	  
		/* If the message send rate is negative or the message send rate is less than or
		 * equal to the requested select rate then set the selectRatio to 1, otherwise set
		 * the selectRatio to the requested select rate divided by the message send rate */
		if (messageRate <= 0 || messageRate <= selectRate)
			selectRatio = 1.0e0; 
		else
			selectRatio = selectRate / (double) messageRate; 
	  
		/* Initialize message sent and selected counts to 0*/
		messageCount = selectCount = 0e0; 
	}
	
	
	/* 
	 * calculateDestLatency( )
	 * 
	 * Calculates the latency and aggregate.
	 */
	static void calculateLatency(ArrayList<JMSBenchTest> jbtList, int latencyMask, PrintStream out, boolean printMinMax, boolean aggregateOnly, JMSBenchTest.GraphiteSender gs, long time, double units)
	{
		int indx = 1;
		int totalDests = 0, numDests = 0;
		int [] aggrHistogram = new int[JMSBenchTest.MAXLATENCY];
		int aggrMsgCount = 0;
		int aggrMinLatency = Integer.MAX_VALUE, aggrMaxLatency = 0;
		int aggr50thLatency = 0, aggr95thLatency = 0, aggr99thLatency = 0;
		int aggr999thLatency = 0, aggr9999thLatency = 0, aggr99999thLatency = 0;
		long aggrSumLatency = 0, aggrSampleCount = 0;
		double aggrAvgLatency = 0.0, aggrStdDev = 0.0, sigma = 0.0;
		
		String dest = null;
		String minDestLatencyId = null;
		JMSBenchTest.Latency minDestLatency = null;
		double minAvgDestLatency = 0.0;
		String  maxDestLatencyId = null;
		JMSBenchTest.Latency maxDestLatency = null;
		double maxAvgDestLatency = 0.0;

		if(!aggregateOnly)
			out.println(thread_lat_hdr);

		/* Obtain the latency for each JMSBenchTest instance (i.e. thread) */
    	for (JMSBenchTest jbt : jbtList) {
    		/* All latency measurements are calculated per thread, but only CHKTIMERTT is calculated per destination */
    		int [] threadHistogram = new int[JMSBenchTest.MAXLATENCY];
    		int thrdMinLatency = Integer.MAX_VALUE, thrdMaxLatency = 0;
			int thrd50thLatency = 0, thrd95thLatency = 0, thrd99thLatency = 0;
			int thrd999thLatency = 0, thrd9999thLatency = 0, thrd99999thLatency = 0;
			int thrdMsgCount = 0;
			double thrdAvgLatency = 0.0, thrdStdDev = 0.0;
    		double destAvgLatency = 0.0;
    		long thrdSampleCount = 0, thrdSumLatency = 0, destSampleCount = 0, destSumLatency = 0;
    		
			sigma = 0.0;  /* Reset the sigma value for this thread. */ 
	
			if( (latencyMask & JMSBenchTest.CHKTIMERTT) == JMSBenchTest.CHKTIMERTT) {  // CHKTIMERTT is per destination 
				if (jbt.action == JMSBenchTest.SEND) // CHKTIMERTT is only relevant to consumers
					continue;
				numDests = jbt.consTuples.size();
				for (int destCt = 0 ; destCt < numDests ; destCt++, totalDests++ ){
					JMSBenchTest.DestTuple<MessageConsumer> destTuple = jbt.consTuples.get(destCt);
					JMSBenchTest.Latency destLatData = destTuple.latencyData;
					dest = destTuple.toString();

					/* Add the current destination histograms into the particular thread histogram. */
					addDestHistograms(destLatData, threadHistogram);

					for (int j = 0; j < JMSBenchTest.MAXLATENCY; j++) {
						/* If there is data for this point then increment the sampleCount and the
						 * sumLatency with the appropriate values based on the histogram. */
						if (destLatData.histogram[j] > 0)
						{
							destSampleCount += destLatData.histogram[j];
							destSumLatency += (j * destLatData.histogram[j]);
						}
					}

					/* Need to add the message count for this thread. */
					thrdMsgCount += destTuple.count;

					/* Update the aggregate message count as well. */
					aggrMsgCount += thrdMsgCount;

					/* Now determine the average latency for this destination. */
					destAvgLatency = (double)destSumLatency / (double)destSampleCount;

					/* Update the thread sample count and sum of latency */
					thrdSampleCount += destSampleCount;
					thrdSumLatency += destSumLatency;

					/* Check to see if this is this destination has the minimum or maximum average
					 * latency for the entire process if so then update the aggregate minimum or
					 * maximum latency variables respectively.  Also store the ID since it will be
					 * printed to the output file. */
					if ((minDestLatency == null) && (maxDestLatency == null))
					{
						minAvgDestLatency = destAvgLatency;
						minDestLatency = destLatData;
						minDestLatencyId = dest;        				
						maxAvgDestLatency = destAvgLatency;
						maxDestLatency = destLatData;
						maxDestLatencyId = dest;
					}
					else
					{
						if (destAvgLatency < minAvgDestLatency) {
							minAvgDestLatency = destAvgLatency;
							minDestLatency = destLatData;
							minDestLatencyId = dest;
						}

						if (destAvgLatency > maxAvgDestLatency) {
							maxAvgDestLatency = destAvgLatency;
							minDestLatency = destLatData;
							maxDestLatencyId = dest;
						}
					}

					/* See if the current destination has the minimum and/or maximum latency 
					 * and if so update the thread variable for the minimum and/or maximum. */
					if (destLatData.minLatency < thrdMinLatency)
						thrdMinLatency = destLatData.minLatency;

					if (destLatData.maxLatency > thrdMaxLatency)
						thrdMaxLatency = destLatData.maxLatency;

					/* See if the current destination has the minimum and/or maximum latency 
					 * and if so update the aggregate variable for the minimum and/or maximum. */
					if (destLatData.minLatency < aggrMinLatency)
						aggrMinLatency = destLatData.minLatency;

					if (destLatData.maxLatency > aggrMaxLatency)
						aggrMaxLatency = destLatData.maxLatency;

				}  /* for loop for destination count. */
			}  else { // All other latency types are per thread (all but CHKTIMERTT)
				JMSBenchTest.Latency latData = null;
				if((latencyMask & JMSBenchTest.CHKTIMECOMMIT) == JMSBenchTest.CHKTIMECOMMIT) {
					latData = jbt.commitHist;
					if(!aggregateOnly)
						out.println("Latency for the commit call");
				}
				if((latencyMask & JMSBenchTest.CHKTIMESEND) == JMSBenchTest.CHKTIMESEND) {
					latData = jbt.sendHist;
					if(!aggregateOnly)
						out.println("Latency for the send call");
				}
				if((latencyMask & JMSBenchTest.CHKTIMERECV) == JMSBenchTest.CHKTIMERECV) {
					latData = jbt.recvHist;
					if(!aggregateOnly)
						out.println("Latency for the recv call");
				}
				if((latencyMask & JMSBenchTest.CHKTIMECONN) == JMSBenchTest.CHKTIMECONN) {
					latData = jbt.connHist;
					if(!aggregateOnly)
						out.println("Latency for JMS connection creation");
				}
				if((latencyMask & JMSBenchTest.CHKTIMESESS) == JMSBenchTest.CHKTIMESESS) {
					latData = jbt.sessHist;
					if(!aggregateOnly)
						out.println("Latency for JMS session creation");
				}
				
				addHistogram(latData.histogram, threadHistogram);
				
				for (int j = 0; j < JMSBenchTest.MAXLATENCY; j++) {
					/* If there is data for this point then increment the sampleCount and the
					 * sumLatency with the appropriate values based on the histogram. */
					if (threadHistogram[j] > 0)
					{
						thrdSampleCount += threadHistogram[j];
						thrdSumLatency += (j * threadHistogram[j]);
					}
				}
				
				/* See if the current destination has the minimum and/or maximum latency 
				 * and if so update the thread variable for the minimum and/or maximum. */
				if (latData.minLatency < thrdMinLatency)
					thrdMinLatency = latData.minLatency;

				if (latData.maxLatency > thrdMaxLatency)
					thrdMaxLatency = latData.maxLatency;

				/* See if the current destination has the minimum and/or maximum latency 
				 * and if so update the aggregate variable for the minimum and/or maximum. */
				if (latData.minLatency < aggrMinLatency)
					aggrMinLatency = latData.minLatency;

				if (latData.maxLatency > aggrMaxLatency)
					aggrMaxLatency = latData.maxLatency;

			}
    		
    		/* Get the average latency for this thread. */
    		thrdAvgLatency = (double)thrdSumLatency / (double)thrdSampleCount;

    		/* Update the aggregate sample count and sum of latency */
    		aggrSampleCount += thrdSampleCount;
    		aggrSumLatency += thrdSumLatency;
    		
    		/* Now determine the thread's standard deviation. */
    		for (int k = 0 ; k < JMSBenchTest.MAXLATENCY ; k++)
    		{
    			if (threadHistogram[k] > 0) {
    				sigma += (double)(((double)(k - thrdAvgLatency) * (double)(k - thrdAvgLatency)) * 
    					threadHistogram[k]);
    			}
    		}
    		
    		thrdStdDev = Math.sqrt(sigma / (double)thrdSampleCount);
    		
    		/* Get the 50th, 95th, 99th, and 999th Percentile. */
    		thrd50thLatency = getLatencyPercentile(threadHistogram, 0.50);
    		thrd95thLatency = getLatencyPercentile(threadHistogram, 0.95);
    		thrd99thLatency = getLatencyPercentile(threadHistogram, 0.99);
    		thrd999thLatency = getLatencyPercentile(threadHistogram, 0.999);
    		thrd9999thLatency = getLatencyPercentile(threadHistogram, 0.9999);
    		thrd99999thLatency = getLatencyPercentile(threadHistogram, 0.99999);
    		
    		/* Write the thread statistics out to the data file. */
    		if(!aggregateOnly){
    			out.printf("cons%d,%d,%d,%d,%d,%.2f,%d,%.2f,%d,%d,%d,%d,%d \n",
    					indx,
    					thrdMsgCount,
    					thrdSampleCount,
    					thrdMinLatency,
    					thrdMaxLatency,
    					thrdAvgLatency,
    					thrd50thLatency,
    					thrdStdDev,
    					thrd95thLatency,
    					thrd99thLatency,
    					thrd999thLatency,
    					thrd9999thLatency,
    					thrd99999thLatency
    			);
    		}
    		
    		/* Update the aggregate histogram with this thread's histogram. */
    		addHistogram(threadHistogram, aggrHistogram);
    		
    		/* Increment the jbt index. */
    		indx++;
		} /* for (JMSBenchTest jbt : jbtList) */
    	
    	/* Print the aggregate header to the latency file. */
		out.println("");
		if((latencyMask & JMSBenchTest.CHKTIMECOMMIT) == JMSBenchTest.CHKTIMECOMMIT) {
				out.println("Latency for the commit call");
		}
		if((latencyMask & JMSBenchTest.CHKTIMESEND) == JMSBenchTest.CHKTIMESEND) {
				out.println("Latency for the send call");
		}
		if((latencyMask & JMSBenchTest.CHKTIMERECV) == JMSBenchTest.CHKTIMERECV) {
				out.println("Latency for the recv call");
		}
		if((latencyMask & JMSBenchTest.CHKTIMECONN) == JMSBenchTest.CHKTIMECONN) {
				out.println("Latency for JMS connection creation");
		}
		if((latencyMask & JMSBenchTest.CHKTIMESESS) == JMSBenchTest.CHKTIMESESS) {
				out.println("Latency for JMS session creation");
		}
		out.println(aggr_lat_hdr);
		
		/* Determine the average for the aggregate. */
		aggrAvgLatency = (double)aggrSumLatency / (double)aggrSampleCount;

		/* Now determine the aggregate's standard deviation. */
		sigma = 0.0;    /* Reset the sigma since previously used for threads. */
		for (int k = 0 ; k < JMSBenchTest.MAXLATENCY ; k++)
		{
			if (aggrHistogram[k] > 0)
				sigma += (double)(((double)(k - aggrAvgLatency) * (double)(k - aggrAvgLatency)) * 
					aggrHistogram[k]);
		}
		
		aggrStdDev = Math.sqrt(sigma / (double)aggrSampleCount);
		
		/* Get the 50th, 95th, 99th, and 999th Percentile. */
		aggr50thLatency = getLatencyPercentile(aggrHistogram, 0.50);
		aggr95thLatency = getLatencyPercentile(aggrHistogram, 0.95);
		aggr99thLatency = getLatencyPercentile(aggrHistogram, 0.99);
		aggr999thLatency = getLatencyPercentile(aggrHistogram, 0.999);
		aggr9999thLatency = getLatencyPercentile(aggrHistogram, 0.9999);
		aggr99999thLatency = getLatencyPercentile(aggrHistogram, 0.99999);

		out.println(new Date().toString());
		out.printf("all cons,%d,%d,%d,%d,%.2f,%d,%.2f,%d,%d,%d,%d,%d,%d-%d\n",
				aggrMsgCount,
				aggrSampleCount,
				aggrMinLatency,
				aggrMaxLatency,
				aggrAvgLatency,
				aggr50thLatency,
				aggrStdDev,
				aggr95thLatency,
				aggr99thLatency,
				aggr999thLatency,
				aggr9999thLatency,
				aggr99999thLatency,
				JMSBenchTest.minMsg,
				JMSBenchTest.maxMsg
		);
    
		/* Print the minimum and maximum destinations if more than 1 to the datafile. */
		if (printMinMax && totalDests > 1) {
			out.println("");
			out.println("   Min Destination:  " + minDestLatencyId);
			out.println("   Max Destination:  " + maxDestLatencyId);
		}

		/* If requesting the histograms for the Min and Max then print out here. */
		if (printMinMax)
		{
			System.out.println("Printing the destination (min/max) histograms out...");
			
			String histogramFile = null;
			JMSBenchTest.Latency latHistogram = null;
			String destId = null;
			
			try
			{
				for (int k = 0 ; k < 2 ; k++ ) {
					if (k == 0)
					{
						histogramFile = "minDestHgram.csv";
						latHistogram = minDestLatency;
						destId = minDestLatencyId;
					}
					else
					{
						histogramFile = "maxDestHgram.csv";
						latHistogram = maxDestLatency;
						destId = maxDestLatencyId;
					}
					
					if(latHistogram == null)
						continue;
					
					PrintStream histogramStream = new PrintStream(new FileOutputStream(histogramFile));

					/* Print the destination on line 1 and the column headers on line 2 */
					histogramStream.println("Destination: " + destId);
					histogramStream.println("Latency,Count");
					
					for (int j = 0; j < JMSBenchTest.MAXLATENCY; j++) {
						histogramStream.println(j+ "," + latHistogram.histogram[j]);
					}
				}
			}
			catch (Exception e)
			{
    			e.printStackTrace();					
			}
			
			System.out.println("");
		}		
		
		if (gs != null) {
			gs.createGraphiteMetric(time, "Latency.MSGCOUNT", String.valueOf(aggrMsgCount));
			gs.createGraphiteMetric(time, "Latency.SAMPLECOUNT", String.valueOf(aggrSampleCount));
			gs.createGraphiteMetric(time, "Latency.UNITS", String.valueOf(units));
			gs.createGraphiteMetric(time, "Latency.MIN", String.valueOf(aggrMinLatency));
			gs.createGraphiteMetric(time, "Latency.MAX", String.valueOf(aggrMaxLatency));
			gs.createGraphiteMetric(time, "Latency.AVG", String.valueOf(aggrAvgLatency));
			gs.createGraphiteMetric(time, "Latency.50P", String.valueOf(aggr50thLatency));
			gs.createGraphiteMetric(time, "Latency.95P", String.valueOf(aggr95thLatency));
			gs.createGraphiteMetric(time, "Latency.99P", String.valueOf(aggr99thLatency));
			gs.createGraphiteMetric(time, "Latency.999P", String.valueOf(aggr999thLatency));
		}
		
	}  /* calculateLatency( ) */
	
	/*
	 * addDestHistograms( ) 
	 *
	 * Add Destination histograms to a thread histogram. 
	 */
	static void addDestHistograms(JMSBenchTest.Latency destLatData, int [] thread_histogram)
	{
		for (int j = 0; j < JMSBenchTest.MAXLATENCY; j++) {
			/* If there is data for this point then add to the thread histogram. */
			if (destLatData.histogram[j] > 0)
			{
				/* Add destination element count to the thread element count. */
				thread_histogram[j] += destLatData.histogram[j];
				
				/* Set the minimum latency for this destination it not already set. */
				if (destLatData.minLatency == 0)
					destLatData.minLatency = j;
    		}
    	}
	}  /* addDestHistograms( ) */
	
	/*
	 * addHistogram( ) 
	 * 
	 * Add src histogram to the dst histogram.
	 */
	static void addHistogram(int [] src, int [] dst)
	{
   		for (int j = 0; j < JMSBenchTest.MAXLATENCY; j++) {
   			if (src[j] > 0)
   				dst[j] += src[j];
   		}
	}  /* addHistogram( ) */


	/* 
	 * getLatencyPercentile( )
	 * 
	 * Calculates the latency percentiles from a histogram of latencies.
	 */
	static int getLatencyPercentile(int[] histogram, double percentile)
	{
		int k;
		int sampleCount=0;      /* Number of samples in the histogram */
		int percentileCount;    /* The sample count that is at the target latency percentile */
		int latency=0;       /* The latency found at the percentile of all latency in the histogram */
	
		/* Get number of samples in the histogram */
		for (k=0 ; k < JMSBenchTest.MAXLATENCY ; k++)
		{
			if (histogram[k] > 0)
			{
				sampleCount += histogram[k];
			}
		}
		
		/* Calculate at what number of samples the target latency percentile will be found */
		percentileCount = (int)(percentile * (double)sampleCount);
		
		/* Moving from lowest index(latency) to highest index(latency) in the histogram find the
		 * index at which point the number of samples reaches the target latency percentile */
		for (k=0, sampleCount=0 ; k < JMSBenchTest.MAXLATENCY ; k++)
		{
			if ((sampleCount < percentileCount) && ((sampleCount + histogram[k]) >= percentileCount))
			{	
				latency = k;
				break;
			}
			
			sampleCount += histogram[k];
		}

		return latency;
	}

	
	/* 
	 * writeTimestamp( )
	 * 
	 * Write the timestamp (long) into the buffer provided starting at byte 1.
	 */
	public void writeTimestamp(byte buf[], long tStamp) throws Exception
	{
		int i = 8;
		
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i--] = (byte) tStamp;	tStamp >>>= 8;
		buf[i]   = (byte) tStamp;
	}
	
	
	/* 
	 * readTimestamp( )
	 * 
	 * Read the timestamp (long) from buffer starting at buf[1] and pass back.
	 */
	public long readTimestamp(byte buf[]) throws Exception
	{
		int i = 1;
		int hi, lo;
		
		hi = buf[i++];
		hi = (hi << 8) | (buf[i++] & 0xff);
		hi = (hi << 8) | (buf[i++] & 0xff);
		hi = (hi << 8) | (buf[i++] & 0xff);
		
		lo = buf[i++];
		lo = (lo << 8) | (buf[i++] & 0xff);
		lo = (lo << 8) | (buf[i++] & 0xff);
		lo = (lo << 8) | (buf[i] & 0xff);
		
		return ((long) hi << 32 | ((lo & 0x00000000ffffffffL)));
	}
	
    public static String uniqueString(String clientid) {
        CRC32 crc = new CRC32();
        crc.update(clientid.getBytes());
        long now = System.currentTimeMillis();
        crc.update((int)((now/77)&0xff));
        crc.update((int)((now/79731)&0xff));
        long crcval = crc.getValue();
        char [] uniqueCh = new char[5];
        for (int i=0; i<5; i++) {
            uniqueCh[i] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz".charAt((int)(crcval%62));
            crcval /= 62;
        }
        return new String(uniqueCh);
    }
    
    public static void removeFile (String fileName)
    {
		File file = new File(fileName);
		if (file.exists())
			file.delete();
    } /* removeFile( ) */
}
