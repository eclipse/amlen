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

package com.ibm.ima.jms.impl;

import java.util.LinkedList;
import java.util.concurrent.locks.Condition;
import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;



/*
 * Implement the delivery thread for a session.
 * 
 * The delivery thread is used when there are message listeners in a session.
 */
final class ImaDeliveryThread implements Runnable {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

	
	private static final class DeliveryTask {
		ImaReceiveAction action = null;
		ImaMessage		 message = null;
	}
	private final Lock mutex = new ReentrantLock();
//	private final Condition queueFull  = mutex.newCondition(); 
	private final Condition queueEmpty = mutex.newCondition(); 
	private static final int TASK_ARRAY_SIZE = 256*1024;
	private final DeliveryTask[][]	taskArrays = new DeliveryTask[2][TASK_ARRAY_SIZE];
	private DeliveryTask[]	    currentTasks = taskArrays[0]; 
	private LinkedList<DeliveryTask> extraSpace = null;
	int      currSize    = 0;
	boolean	 stopped = false;
	boolean  draining = false;
	Thread   thread = null;
	
	/*
	 */
	ImaDeliveryThread() {
	}
	
	/* 
	 * TODO: add comments
	 * @see java.lang.Runnable#run()
	 */
	public void run() {
		int curr = 0;
		synchronized (this) {
			thread = Thread.currentThread();
        	if (ImaTrace.isTraceable(6)) {
        		ImaTrace.trace("Start JMS delivery thread: " + thread.getName());
        	}						
		}
		while (true) {
			int getSize;
            DeliveryTask[] tasks = null; 
			LinkedList<DeliveryTask> extra = null;
			mutex.lock();
			try {
				while (currSize == 0) {
					if (stopped) {
					    mutex.unlock();
						return;
					}
					queueEmpty.awaitUninterruptibly();
				}
				tasks = taskArrays[curr];
				extra = extraSpace;
				extraSpace = null;
				getSize = currSize;
				currSize = 0;
				curr = (curr + 1) & 0x1;
				currentTasks = taskArrays[curr];
			} finally {
				mutex.unlock();
			}
			for (int i = 0; i < getSize; i++) {
				DeliveryTask dt = tasks[i];
				dt.action.deliver(dt.message);
				dt.action = null;
				dt.message = null;
			}
			if (extra == null)
				continue;
			while (!extra.isEmpty()) {
				DeliveryTask dt = extra.removeFirst();
				dt.action.deliver(dt.message);
			}
		}
	}
	
	/*
	 * TODO: add comments
	 */
	void addTask(ImaReceiveAction action, ImaMessage message) {		
		mutex.lock();
		try {
            if (!stopped) {
                final DeliveryTask dt;
                if (currSize < TASK_ARRAY_SIZE) {
                    if (currentTasks[currSize] == null) {
                        currentTasks[currSize] = new DeliveryTask();
                    }
                    dt = currentTasks[currSize++];
                } else {
                    dt = new DeliveryTask();
                    if (extraSpace == null)
                        extraSpace = new LinkedList<DeliveryTask>();
                    extraSpace.addLast(dt);
                }
                dt.action = action;
                dt.message = message;
                if (currSize == 1)
                    queueEmpty.signalAll();
            }
		} finally {
			mutex.unlock();
		}
		
	}
	
	
	void drain() {
		mutex.lock();
		try {
		    currSize = 0;
		    extraSpace = null;
			queueEmpty.signalAll();
//			queueFull.signalAll();
		} finally {
		    mutex.unlock();
		}    
	}

	
	/*
	 * TODO: add comments
	 */
	Thread stop() {
		mutex.lock();
		try {
			stopped = true;
			queueEmpty.signalAll();
//			queueFull.signalAll();
		} finally {
			mutex.unlock();
		}
		return thread;
	}
}
