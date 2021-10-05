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
package com.ibm.ism.svt.common;

import java.util.Properties;
import com.ibm.automation.core.Platform;

/**
 * Provides common test data for ISM testcases
 * 
 * Copies from fvt_Gui com.ibm.ism.test.common *
 */
public class IsmSvtTestData { 
	private Properties props;
	
	public static final String KEY_BROWSER = "BROWSER";
	public static final String KEY_URL = "URL";
	
	public static final String KEY_WSS = "WSS";
	public static final String KEY_UID = "UID";
	public static final String KEY_PASSWORD = "PASSWORD";
	public static final String KEY_IP   = "IP";
	public static final String KEY_PORT = "PORT";
	public static final String KEY_CLIENTID = "CLIENTID";
	public static final String KEY_CLEANSESSION = "CLEANSESSION";
	public static final String KEY_MSGTOPIC = "MSGTOPIC";
	public static final String KEY_MSGCOUNT = "MSGCOUNT";
	public static final String KEY_MSGTEXT = "MSGTEXT";
	public static final String KEY_RETAIN = "RETAIN";
	public static final String KEY_PUBQOS = "PUBQOS";
	public static final String KEY_SUBQOS = "SUBQOS";


	public IsmSvtTestData(String[] args) {
		props = new Properties();
		for (String arg : args) {
			String strs[] = arg.split("=");
			props.setProperty(strs[0], strs[1]);
		}
		// Set default properties
		props.setProperty("firefox", Platform.gsMozillaFirefox);
		props.setProperty("chrome", Platform.gsGoogleChrome);
		props.setProperty("safari", Platform.gsSafari);
		props.setProperty("iexplore", Platform.gsInternetExplorer);
	}
	
	public String getProperty(String key) {
		return props.getProperty(key);
	}
	
	public String getURL() {
		return getProperty(KEY_URL);
	}
	
	public boolean isHTTPS() {
		return getURL().startsWith("https:");
	}

	public boolean getWss() {
		return Boolean.parseBoolean( getProperty(KEY_WSS) );
	}

	public String getUid() {
		return getProperty(KEY_UID);
	}

	public String getPassword() {
		return getProperty(KEY_PASSWORD);
	}
	
	public String getIp() {
		return getProperty(KEY_IP);
	}
	public String getPort() {
		return getProperty(KEY_PORT);
	}
	public String getClientId() {
		return getProperty(KEY_CLIENTID);
	}
	public boolean getCleanSession() {
		return Boolean.parseBoolean( getProperty(KEY_CLEANSESSION) );
	}
	public String getMsgTopic() {
		return getProperty(KEY_MSGTOPIC);
	}
	public String getMsgCount() {
		return getProperty(KEY_MSGCOUNT);
	}
	public String getMsgText() {
		return getProperty(KEY_MSGTEXT);
	}
	public boolean getRetain() {
		return Boolean.parseBoolean( getProperty(KEY_RETAIN) );
	}
	public String getPubQoS() {
		return getProperty(KEY_PUBQOS);
	}
	public String getSubQoS() {
		return getProperty(KEY_SUBQOS);
	}
	public String getBrowser() {
		String browser = getProperty(KEY_BROWSER);
		if (browser == null) {
			browser = "firefox";
		}
		return getProperty(browser);
	}
	
}
