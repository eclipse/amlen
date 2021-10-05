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
import java.util.ArrayList;

/* The RateController class controls the rate at which messages are sent per send thread */
public class RateController implements Runnable
{
	public static int batchRate = 100000; /* the rate at which the rate control thread will control the rate of message 
     * transmission for each publishing thread units of loops/second.  this is also referred to 
     * as the batching rate or scheduling rate or the rate control thread. 100K loops/second was 
     * determined to be the optimal rate to schedule the rate control thread.  At 100K loops/second 
     * both very high message rates and low message rates should be attainable. */
	
	private double messageRate;
	private double yieldsPerMessage;
	private double yieldsPerSecond;
	
	private double lastTime;
	private double yieldCount;
	private double messageCount;
	ArrayList<JMSBenchTest> publishers;
	int cpu; /* CPU on which ratecontroller will run */
	
	public RateController(ArrayList<JMSBenchTest> senders) {
		publishers = senders;
	}
	
	/* Initialize the rateController.  This technique of "sleeping" does not use any "sleep" function 
	 * rather it continuously estimates how many scheduled yields need to be invoked per message or per second
	 * in order to achieve the requested message send rate */
	public void initRateController(double msgRate)
	{
		int numYields; /* a counter to hold the number of Thread.yield() calls*/
		double baseTime, deltaTime; /* time in units of seconds*/

		/* Do the initial time measurement for 1000 yields. this will be used to calibrate yield times later */
		baseTime = (double) (System.nanoTime() * 1e-9);
		for(numYields=0; numYields < 1000 ; numYields++) 
			Thread.yield(); 
		deltaTime = ((double) (System.nanoTime() * 1e-9) - baseTime) / (double) 1000; /* deltaTime is the number of seconds it 
		 * took to do a single yield */

		messageRate = msgRate;
		yieldsPerMessage = ((double) 1 / (double) (deltaTime * messageRate));
		//timer->yieldsPerSecond = timer->yieldsPerMessage * messageRate;
		yieldsPerSecond = 0; /* testing showed that setting the initial yields per second to 0 helped
		 * reduce the magnitude of the initial burst in the data rate while converging to the specified rate */
		lastTime = (double) (System.nanoTime() * 1e-9); /* establish the base time for rate controller */
		yieldCount = 0;
		messageCount = 0;
	}
	
	/* Called once per loop of the submit message loop of each publisher thread this function
	 * continuously estimates how many scheduled yields need to be invoked to achieve the requested message send rate
	 * This is per submit thread. */
	public void controlRate()
	{
		/* Given that messageRate is in units of messages per second deltaTime will converge to 1 second
		 * once the actual message send rate approaches the requested message send rate */
		if(yieldCount >= yieldsPerSecond)
		{
			double deltaTime; 
			deltaTime = (double) (System.nanoTime() * 1e-9)  - lastTime;
			lastTime += deltaTime; /* lastTime is initialized in initRateController. Set lastTime for the subsequent calls controlRate */
			
			/* this deltaTime check protects against an initial spike in message rate especially when running
			 * with multiple publishing threads.  For optimal rate control when running multiple publisher threads
			 *  use a single rate thread */
			/*if(deltaTime < .30e0)
			{
				deltaTime = .30e0;
			}
			else
			{*/
				/* if deltaTime is less than 3/4 of a second then we are yielding too much and need reduce the
				 * number of yields per second.  As the rate sinks below the target rate this will cause the rate
				 * to converge back up from below the target rate. */
				if ( deltaTime < .75e0 )
				{
					deltaTime = .75e0;
				} 
				else
				{
					/* if deltaTime is greater than 1 and 1/4 of a second then we are yielding too little
					 * and need increase the number of yields per second.  this causes a convergence from the top
					 * to the target message rate */
					if ( deltaTime > 1.25e0 )
					{
						deltaTime = 1.25e0;
					}
				}
			/*}*/
			
			yieldsPerMessage /= deltaTime; /* recalculate yield per message */   
			yieldsPerSecond = yieldsPerMessage * messageRate;
			
			/* Reset the yield count and message count for the next call to MW_wait.  This helps to 
			 * accelerate the convergence of the yield rate */
			yieldCount = 0;
			messageCount = 0; 
		}
		
		/* Converge upon the yield rate */
		messageCount += yieldsPerMessage; 
		
		/* If the yield count is less than the projected message count at which to send the next message
		 * then increment the yield count and yield the CPU to another thread */
		while(yieldCount < messageCount)
		{
			yieldCount++; 
			Thread.yield();
		}
	}
	
	public void run() {
		if(JMSBenchTest.rateControllerCPU >=0 )
			JMSBenchTest.setAffinity(JMSBenchTest.rateControllerCPU);
		
		/* The rate control thread is the optimal rate control mechanism when publishing on multiple threads.  
		 * The mechanism uses a token bucket mechanism in which each publishing thread has its own token 
		 * bucket that it empties up by one token per transmitted message.  When the token bucket for a 
		 * particular thread is full then that publishing thread yields the CPU.  The rate control thread 
		 * fills each publishing threads token bucket in batches.  Batch size is a function of the 
		 * batching rate (i.e. scheduling rate) of the rate control thread and the per thread 
		 * message transmission rate. 
		 * 
		 * note: when using the rate control thread mechanism this implies that you have one less CPU 
		 * core to play with.  When using the rate control thread you should have a maximum of (n - 1) 
		 * publishing threads (i.e. streams) where n is the number of CPU cores available on the 
		 * system */
		
		double batchFactor; /* the batch size by which to increment token bucket for each 
		 * publishing thread.  the token bucket controls how often to yield the CPU per publishing thread. */
		
		//batchFactor = (double)threadData.get(0).perStreamRate / (double) batchRate; /* batch size equals the per thread message rate divided by the batching rate */
		int perPublisherRate = JMSBenchTest.globalMsgRate / publishers.size();
		batchFactor = (double) perPublisherRate / (double) batchRate; /* batch size equals the per thread message rate divided by the batching rate */
		initRateController((double) batchRate);
		
		/* If there is a delaySend then need to sleep for that amount prior to initializing the RateController. */
   		if(JMSBenchTest.delaySend > 0){
   			try {
				Thread.sleep(JMSBenchTest.delaySend * 1000);  // delaySend (-ds) is in units of seconds
			} catch (InterruptedException e) {
				System.out.println("Took an interrupt exception on RateController during delay time before sending messages.");
				e.printStackTrace();
			}  
			
			/* Reset the lastTime since it is used for ControlRate method */
			lastTime = (double) (System.nanoTime() * 1e-9); 
   		}

		/* Signal to main thread that it is time to start the clock on the test duration or enter the command loop */
		JMSBenchTest.rateControllerReady = true;

		System.out.println("Starting rate control thread. per thread msg rate = " + 
				perPublisherRate + "; " + "batch factor = " + batchFactor + "; batchRate = " + 
				batchRate + "\n");
		
		/* Infinite rate control loop - running as long as the publisher threads keep publishing */
		while (!JMSBenchTest.stopRateController)
		{
			/* Iterate through the array of publishing threads emptying each threads token bucket by the batch size
			 * calculated above */
			for (JMSBenchTest jbt : publishers) {
				jbt.tokenIn += batchFactor;
			}

			/* rate control thread itself must also yield the processor to avoid over scheduling itself at the cost of the publishing
			 * threads */
			controlRate(); /* sched_yield implemented "sleep" function */
			
			if (JMSBenchTest.RateChanged_RCThread == 1){
				perPublisherRate = JMSBenchTest.globalMsgRate / publishers.size();
				batchFactor = (double) perPublisherRate / (double) batchRate; /* batch size equals the per thread message rate divided by the batching rate */
				initRateController((double) batchRate);
				JMSBenchTest.RateChanged_RCThread = 0;
			}
		}
		
		System.out.println("Exiting rate control thread.");
	}
}
