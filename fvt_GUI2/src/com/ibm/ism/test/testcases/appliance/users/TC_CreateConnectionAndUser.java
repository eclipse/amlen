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
package com.ibm.ism.test.testcases.appliance.users;

import com.ibm.automation.core.Platform;
//import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.loggers.control.CoreLogger;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ism.test.common.CLIHelper;
import com.ibm.ism.test.tasks.LoginTasks;
import com.ibm.ism.test.tasks.firstserver.FirstServerConnectionTasks;
import com.ibm.ism.test.tasks.appliance.users.WebUIUserTasks;
import com.ibm.ism.test.testcases.appliance.users.ImsCreateUserTestData;
import com.ibm.ism.test.tasks.monitoring.connections.MonitoringConnectionsTasks;

public class TC_CreateConnectionAndUser {
	private static final String HEADER_GETSTARTED = "Get started with IBM MessageSight Web UI";
	private static final String HEADER_LICENSEAGREEMENT = "License Agreement";
	private static final String HEADER_UNTRUSTED = "This Connection is Untrusted";
	public static void main(String[] args) {
		TC_CreateConnectionAndUser test = new TC_CreateConnectionAndUser();
		System.exit(test.runTest(new ImsCreateUserTestData(args)) ? 0 : 1);
	}
	
	public boolean runTest(ImsCreateUserTestData testData) {
		boolean result = false;
		ImaConfig imaConfig = null;
		
		
		if (testData.isCLICreateUserTest()) {
			try {
				CLIHelper.printWebUIHost(testData);
				CLIHelper.printWebUIPort(testData);
				imaConfig = new ImaConfig(testData.getA1Host(), testData.getUser(), testData.getPassword());
				imaConfig.connectToServer();
				imaConfig.createUser(testData.getNewUserID(), testData.getNewUserPwd(), ImaConfig.USER_GROUP_TYPE.WebUI, testData.getNewUserGroups(), testData.getNewUserDescription());
				imaConfig.disconnectServer();
			} catch (Exception e) {
				e.printStackTrace();
				return false;
			}
		}

		Platform.setEngineToSeleniumWebDriver();
		WebBrowser.start(testData.getBrowser());
		WebBrowser.loadUrl(testData.getURL());


		try {
			if (testData.isCLICreateUserTest()) {
				LoginTasks.login(testData);
				MonitoringConnectionsTasks.loadURL(testData);
				LoginTasks.logout();
			} else {
				LoginTasks.login(testData);
				CoreLogger.info("Back from Login via CoreLogger");
				System.out.println("Back from Login via print ");
				Platform.sleep(1);
				if  (WebBrowser.waitForText(HEADER_UNTRUSTED, 10)) {
					CoreLogger.info("Start Here for Untrusted");
					FirstServerConnectionTasks.getAddCertificate(testData);
					System.out.println("Back from get Certificate ");
					CoreLogger.info("Back from First Certificate");
					FirstServerConnectionTasks.getConfirmCertificate(testData);
					System.out.println("Back from get Confirm Certificate ");
					CoreLogger.info("Back from confirming First Certificate");
				}
				
				
				if  (WebBrowser.waitForText(HEADER_LICENSEAGREEMENT, 10)) {
					CoreLogger.info("Start Here for License Agreement");
					FirstServerConnectionTasks.loadLicenseAgreement(testData);
					System.out.println("Back from load License Agreement ");
					CoreLogger.info("Back from License Agreement Dialog");
					FirstServerConnectionTasks.loadLicenseAccepted(testData);
					System.out.println("Back from Close Accept License Agreement Dialog ");
					CoreLogger.info("Back from Close License Agreement Dialog");
				
					if  (WebBrowser.waitForText(HEADER_GETSTARTED, 10)) {
						CoreLogger.info("Start Here for Get Started");
					   System.out.println("This is the very first time to login and get Server Connection Page ");
					   CoreLogger.info("Back from WebUIUserTasks1");
					   FirstServerConnectionTasks.getServerConnection(testData);
					   System.out.println("Back from getServerConnection ");
					   CoreLogger.info("Back from getServerConnection");
					   FirstServerConnectionTasks.getVerifyServerConnection(testData);
					   System.out.println("Back from getVerifyServerConnection ");
					   CoreLogger.info("Back from getVerifyServerConnection");
					   System.out.println("Back from add New User ");
					   CoreLogger.info("Back from WebUIUserTasks3");
					   WebUIUserTasks.loadServerURL(testData);
					   CoreLogger.info("Back from load Server URL");
					  
//						 if  (WebBrowser.waitForText(HEADER_CANNOT_CONNECT, 10)) {
//						   CoreLogger.info("Start Here for Cannot Connect");
//						   FirstServerConnectionTasks.getClickHere(testData);
//						   System.out.println("Back from Click Here ");
//						   FirstServerConnectionTasks.loadAdminURL(testData);
//						   System.out.println("Back from getAdminURL ");
//						   CoreLogger.info("Back from Click Here");
//					     }
					}   
				}
				
				
				
				
				CoreLogger.info("Start Here to load Web UI URL to add new user");
				WebUIUserTasks.loadURL(testData);
				CoreLogger.info("Start Here to add new user");
				WebUIUserTasks.addNewUser(testData);
				CoreLogger.info("Start Here to logout");
				LoginTasks.logout(testData);
				Platform.sleep(5);
		
				if (testData.getNewUserGroupMembership().contains(ImsCreateUserTestData.GROUP_SYSTEM_ADMINISTRATORS)) {
					try {
						CoreLogger.info("Start Here to get new group membership");
						imaConfig = new ImaConfig(testData.getA1Host(), testData.getNewUserID(), testData.getNewUserPwd());
						LoginTasks.login(testData.getNewUserID(), testData.getNewUserPwd());
						CoreLogger.info("Start Here to login Web UI as new user for second time");
						if  (WebBrowser.waitForText(HEADER_GETSTARTED, 10)) {
							CoreLogger.info("Start Here for Get Started");
						   System.out.println("This is the very first time to login and get Server Connection Page for the new user ");
						}   
						LoginTasks.logout(testData);
						CoreLogger.info("Back from Logout as new user for second time");
						
					} catch (Exception e) {
						CoreLogger.info("Start Here to debug error1");
						e.printStackTrace();
						if (imaConfig != null && imaConfig.isConnected()) {
							CoreLogger.info("Disconnect Server");
							imaConfig.disconnectServer();
						}
						CoreLogger.info("Skipped Disconnect Server");
					}
				}
			}

			result = true;
		} catch (Exception e){
			CoreLogger.info("Start Here to debug Fail Test");
			result = false;
		}
		WebBrowser.shutdown();
		CoreLogger.info("Start Here to stop logging and shutdown");
		
		return result;
	}
	
}

