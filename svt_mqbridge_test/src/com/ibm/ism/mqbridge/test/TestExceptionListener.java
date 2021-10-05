/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.mqbridge.test;

import javax.jms.ExceptionListener;
import javax.jms.JMSException;

public class TestExceptionListener implements ExceptionListener {

	int timesCalled =0;
	public void onException(JMSException jmse) {
		timesCalled++;
		System.out.println("listener exception thrown " + timesCalled);
		jmse.printStackTrace();
		if (jmse.getMessage().contains("Destination is full")) {
			try {
				// wait 5 seconds to allow destination to clear
				Thread.sleep(5);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		} else {
		System.exit(2);
		}
	}
}
