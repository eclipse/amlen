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
/**
 * 
 */
package com.ibm.ism.test.tasks;

import com.ibm.ism.test.appobjects.firststeps.FirstStepsDialogTask;
import com.ibm.ism.test.common.IsmTest;

/**
 Manipulate objects on the Appliance Locales, Timezone, Date and Time page.
	 *  
 *
 */
public class ManipulateLTDTTasks {

	

	/**
	 * Navigates the Appliance page to change the server's locale,
	 * timezone, date, and time.
	 * @param textToVerify Optional text to verify on the Appliance page.
	 * @return true if successful
	 */
	public static boolean navigateAppliance(String textToVerify) {
		boolean success = ApplianceTasks.clickApplianceTab(textToVerify);
		if (!success) {
			return success;
		}
		if (FirstStepsDialogTask.verifyFirstStepsDialog() == true) {
			FirstStepsDialogTask.leaveFirstSteps();
	        IsmTest.delay ();
		} 
		IsmTest.delay ();
		
		ApplianceTasks.clickLocaleAction("English");
		IsmTest.delay(2000);
		ApplianceTasks.setServerLocale_EN_US();
		IsmTest.delay(2000);
		ApplianceTasks.setServerTimeZoneCentral();
		IsmTest.delay(5000);
		ApplianceTasks.setServerDate("Oct 21, 2012");
		IsmTest.delay(2000);
		ApplianceTasks.setServerTime("11:55:22 AM");
		IsmTest.delay(2000);
		ApplianceTasks.saveServerTimeDate();
		IsmTest.delay(2000);
		ApplianceTasks.setServerLocale_Albanian();
		IsmTest.delay(2000);
		ApplianceTasks.setServerTimeZoneAlaska();
		IsmTest.delay(6000);
		ApplianceTasks.clickUsersAction(textToVerify);
		IsmTest.delay(2000);
		
		return success;
	}	

}
