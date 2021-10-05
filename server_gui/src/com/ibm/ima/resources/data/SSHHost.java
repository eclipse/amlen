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

package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

/**
 * REST representation of ssh host or hosts. Used to set
 * the ssh host or hosts addresses
 *
 */
public class SSHHost extends  AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	@SuppressWarnings("unused")
	private final static String CLAS = SSHHost.class.getCanonicalName();


	@JsonProperty("HostSettingString")
	private String HostSettingString;

	/**
	 * Get ssh hosts
	 * 
	 * @return the ssh host string
	 */
	@JsonProperty("HostSettingString")
	public String getHostSettingString() {
		return HostSettingString;
	}

	/**
	 * Set the ssh host string value. This can be a 
	 * comma delimited list of up to 100 hosts
	 * 
	 * @param HostSettingString the ssh host string
	 */
	@JsonProperty("HostSettingString")
	public void setHostSettingString(String HostSettingString) {
		this.HostSettingString = HostSettingString;
	}


	@Override
	public String toString() {
		return "SSHHost [HostSettingString=" + HostSettingString + "]";
	}

	@Override
	@JsonIgnore
	public ValidationResult validate() {

		ValidationResult vr = new ValidationResult();

		if (HostSettingString == null || trimUtil(HostSettingString).length() == 0) {
			vr.setResult(VALIDATION_RESULT.VALUE_EMPTY);
			vr.setParams(new Object[] {"SSH Host"});
			return vr;
		}

		String[] hosts = HostSettingString.split(",");
		if (hosts.length > 100) {
			vr.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
			vr.setParams(new Object[] {"SSH Host", "1", "100"});
		}


		return vr;
	}

}
