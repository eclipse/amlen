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
package com.ibm.ism.test.testcases.messaging.queues;


import com.ibm.automation.core.Platform;
import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.test.common.CLIHelper;
import com.ibm.ism.test.common.IsmGuiTestException;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.messaging.queues.QueuesTasks;

public class TC_CreateMessageQueue {
	public static void main(String[] args) {
		TC_CreateMessageQueue test = new TC_CreateMessageQueue();
		System.exit(test.runTest(new ImaMessageQueuesTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImaMessageQueuesTestData testData) {
		boolean result = false;
		String testDesc = "Create Message Queue";

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());		

		VisualReporter.startLogging(testData.getTestcaseAuthor(), testData.getTestcaseDescription(), testData.getTestArea());
		VisualReporter.logTestCaseStart(testData.getTestcaseDescription());

		try {
			LoginTasks.login(testData);
			QueuesTasks.loadURL(testData);
			QueuesTasks.addNewMessageQueue(testData);
			if (testData.isGB18030G2() == false) {
				CLIHelper.printQueue(testData, testData.getMessageQueueName());
			}
			LoginTasks.logout(testData);
			result = true;
		} catch (IsmGuiTestException ismException) {
			String queueName = testData.getMessageQueueName();
			if (testData.getMaxConfigItemNameLength() < queueName.codePointCount(0, queueName.length())) {
				if (ismException.getMessage().equals(testData.getErrorCWLNA5013())) {
					result = true;
				}
			}
			if (result == false) {
				VisualReporter.failTest(testDesc + " failed.", ismException);
			}
		} catch (Exception e){
			VisualReporter.failTest(testDesc + " failed.", e);
		}
		WebBrowser.shutdown();
		VisualReporter.stopLogging();
		if (result == true) {
		}
		return result;
	}
}

