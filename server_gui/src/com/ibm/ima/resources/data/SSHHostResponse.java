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

package com.ibm.ima.resources.data;

import java.util.Arrays;
import java.util.List;

import com.ibm.ima.resources.ApplianceResource;

public class SSHHostResponse {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private int sshHostQuerySettingsRC = ApplianceResource.APPLIANCE_REQUEST_FAILED;
    private int sshHostQueryStatusRC = ApplianceResource.APPLIANCE_REQUEST_FAILED;
    private String sshHostSetting = "";
    private String sshHostStatus = "";

    public SSHHostResponse() {
    	
    }
    
	public int getSshHostQuerySettingsRC() {
		return sshHostQuerySettingsRC;
	}

	public void setSshHostQuerySettingsRC(int sshHostQuerySettingsRC) {
		this.sshHostQuerySettingsRC = sshHostQuerySettingsRC;
	}

	public int getSshHostQueryStatusRC() {
		return sshHostQueryStatusRC;
	}

	public void setSshHostQueryStatusRC(int sshHostQueryStatusRC) {
		this.sshHostQueryStatusRC = sshHostQueryStatusRC;
	}

	public List<String> getSshHostSetting() {
		String[] hostStringArray = sshHostSetting.split(",");
		return Arrays.asList(hostStringArray);
	}

	public void setSshHostSetting(String sshHostSetting) {
		if (sshHostSetting != null && sshHostSetting.length() >0) {
			sshHostSetting = sshHostSetting.replace("\n", "").replace("\r", "");
		}
		this.sshHostSetting = sshHostSetting;
	}

	public List<String> getSshHostStatus() {
		String[] hostStringArray = sshHostStatus.split(",");
		return Arrays.asList(hostStringArray);
	}

	public void setSshHostStatus(String sshHostStatus) {
		if (sshHostStatus != null && sshHostStatus.length() >0) {
			String[] statusArray = sshHostStatus.split("=");
			if (statusArray.length == 2) {
				sshHostStatus = statusArray[1];
				sshHostStatus = sshHostStatus.trim();
			}
			sshHostStatus = sshHostStatus.replace("\n", "").replace("\r", "");
		}
		this.sshHostStatus = sshHostStatus;
	}

}
