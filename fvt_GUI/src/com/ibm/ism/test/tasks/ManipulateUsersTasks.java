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
 * Performs navigation tasks on the tabs of the ISM Admin GUI console.
 *  
 *
 */
public class ManipulateUsersTasks {

	

	/**
	 * Manipulate objects on the Appliance Users page.
	 * 
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
		
		ApplianceTasks.clickUsersAction(textToVerify);
		IsmTest.delay(2000);
		ApplianceTasks.addNewUser();
		IsmTest.delay(2000);
		ApplianceTasks.clickAddUserDialog();
		IsmTest.delay(2000);
		ApplianceTasks.setUserID("tester1");
		IsmTest.delay(5000);
		ApplianceTasks.setDescription("User ID for tester1");
		IsmTest.delay(2000);
		ApplianceTasks.setUserPassword("nyc4meto");
		IsmTest.delay(2000);
		ApplianceTasks.setUserConfirmPassword("nyc4meto");
		IsmTest.delay(2000);
		ApplianceTasks.clickDropDownGroupMembership();
		IsmTest.delay(2000);
		ApplianceTasks.saveUserID();
		IsmTest.delay(2000);
		

		return success;
	}	

}
