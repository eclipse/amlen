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
package com.ibm.ism.test.common;

import java.io.FileInputStream;
import java.io.InputStreamReader;
import java.util.Properties;

//import com.ibm.automation.core.Platform;
import com.ibm.automation.core.web.WebBrowser;
//import com.ibm.automation.core.web.WebBrowser;
/**
 * Provides common test data for ISM testcases
 * 
 *
 */
public class IsmTestData { 
	private Properties props;
	public static final String KEY_A1_HOST = "A1_HOST";
	public static final String KEY_A2_HOST = "A2_HOST";
	public static final String KEY_SSH_HOST = "SSH_HOST";
	public static final String KEY_SSH_USER = "SSH_USER";
	public static final String KEY_SSH_PASSWORD = "SSH_PASSWORD";
	public static final String KEY_BROWSER = "BROWSER";
	public static final String KEY_CHROME_DIRVER = "CHROME_DRIVER";
	public static final String KEY_IE_DIRVER = "IE_DRIVER";
	public static final String KEY_URL = "URL";
	public static final String KEY_ADMIN_URL = "ADMIN_URL";
	public static final String KEY_USER = "USER";
	public static final String KEY_SERVERCONNECTION = "SERVERCONNECTION";
	public static final String KEY_PASSWORD = "PASSWORD";
	public static final String KEY_DEBUG = "DEBUG";
	public static final String KEY_TC_DESC = "TC_DESC";
	public static final String KEY_TEST_AREA = "TEST_AREA";
	public static final String KEY_TC_AUTHOR = "TC_AUTHOR";
	public static final String KEY_PROPERTIES_FILE= "PROPERTIES_FILE";
	public static final String KEY_GVT_255_CHARS_TEST = "GVT_255_CHARS_TEST";
	public static final String KEY_GVT_255_CHARS = "GVT_255_CHARS";
	public static final String KEY_GVT_GB18030_CHARS_TEST = "GVT_GB18030_CHARS_TEST";
	public static final String KEY_GVT_TEST_DATA_CATEGORY = "GVT_TEST_DATA_CATEGORY";
	public static final String GVT_TEST_DATA_CATEGORY_G1 = "G1"; //GB 18030 G1
	public static final String GVT_TEST_DATA_CATEGORY_G2 = "G2"; //GB 18030 G2
	public static final String KEY_RUNTIME_ENV = "RUNTIME_ENV";
	public static final String KEY_NEW_SERVER_NAME = "NEW_SERVER_NAME";
	public static final String KEY_NEW_ADMIN_ADDRESS = "NEW_ADMIN_ADDRESS";
	public static final String KEY_NEW_ADMIN_PORT = "NEW_ADMIN_PORT";
	public static final String KEY_NEW_SERVER_DESCRIPTION = "NEW_SERVER_DESCRIPTION";
	public static final String KEY_NEW_SEND_WEBUI_CREDENTIALS = "NEW_SEND_WEBUI_CREDENTIALS";
	
	
	public static final int MAX_CONFIG_ITEM_NAME_LENGTH = 256;
	
	
	public static final String ERROR_CWLNA5013 = "CWLNA5013"; // The value specified for the property "Name" is too long. The maximum length is 256. or 32, 1024.etc..
	public static final String ERROR_CWLNA5051 = "CWLNA5051"; // A configuration object with that name already exists. Specify a different name and try again.
	
	
	public IsmTestData(String[] args) {
		props = new Properties();


		for (String arg : args) {
			String strs[] = arg.split("=");
			if (args.length == 1 && strs[0].equals(KEY_PROPERTIES_FILE)) {
				try {
					//FileInputStream in = new FileInputStream(strs[1]);
					InputStreamReader in = new InputStreamReader(new FileInputStream(strs[1]), "UTF-8");
					props.load(in);
				} catch (Exception e) {
					System.err.println("Exception loading " + strs[1]);
					e.printStackTrace();
					props.setProperty(strs[0], strs[1]);
				}
			} else {
				
				props.setProperty(strs[0], strs[1]);
			}
		}
		// Set default properties
		props.setProperty("firefox", WebBrowser.gsMozillaFirefox);
		props.setProperty("chrome", WebBrowser.gsGoogleChrome);
		props.setProperty("safari", WebBrowser.gsSafari);
		//props.setProperty("iexplore", WebBrowser.gsInternetExplorer);
		
		if (props.getProperty(KEY_DEBUG) == null) {
			props.setProperty(KEY_DEBUG, "false");
		}
		if (props.getProperty(KEY_GVT_255_CHARS_TEST) == null) {
			props.setProperty(KEY_GVT_255_CHARS_TEST, "false");
		}
		if (props.getProperty(KEY_GVT_GB18030_CHARS_TEST) == null) {
			props.setProperty(KEY_GVT_GB18030_CHARS_TEST, "false");
		}
	}
	
	public void setProperty(String key, String value) {
		props.setProperty(key, value);
	}
	
	public String getProperty(String key) {
		return props.getProperty(key);
	}
	
	public int getMaxConfigItemNameLength() {
		return MAX_CONFIG_ITEM_NAME_LENGTH;
	}
	
	public String getErrorCWLNA5013() {
		return ERROR_CWLNA5013;
	}
	
	public String getErrorCWLNA5051() {
		return ERROR_CWLNA5051;
	}
	
	public String getA1Host() {
		return getProperty(KEY_A1_HOST);
	}

	public String getA2Host() {
		return getProperty(KEY_A2_HOST);
	}
	
	public String getSSHHost() {
		return getProperty(KEY_SSH_HOST);
	}
	
	public String getSSHUser() {
		return getProperty(KEY_SSH_USER);
	}
	
	public String getSSHPassword() {
		return getProperty(KEY_SSH_PASSWORD);
	}
	
	public String getURL() {
		return getProperty(KEY_URL);
	}
	
	public boolean isHTTPS() {
		return getURL().startsWith("https:");
	}
	
	public String getAdminURL() {
		return getProperty(KEY_ADMIN_URL);
	}
	
	public boolean isAdminHTTPS() {
		return getURL().startsWith("https:");
	}

	public String getUser() {
		return getProperty(KEY_USER);
	}
	
	public String getServerConnection() {
		return getProperty(KEY_SERVERCONNECTION);
	}

	public String getPassword() {
		return getProperty(KEY_PASSWORD);
	}
	
	public String getRuntime() {
		return getProperty(KEY_RUNTIME_ENV);
	}
	public String getNewServerName() {
		return getProperty(KEY_NEW_SERVER_NAME);
	}
	
	public String getNewAdminAddress() {
		return getProperty(KEY_NEW_ADMIN_ADDRESS);
	}
	
	public String getNewAdminPort() {
		return getProperty(KEY_NEW_ADMIN_PORT);
	}
	
	public String getNewServerDescription() {
		return getProperty(KEY_NEW_SERVER_DESCRIPTION);
	}
	
	//public boolean isNewSendWebUICredentials() {
	//	return getNewSendWebUICredentials().equalsIgnoreCase("true");
	//}
	
	public String getNewSendWebUICredentials() {
		return getProperty(KEY_NEW_SEND_WEBUI_CREDENTIALS);
	}
	
	public boolean isNewSendWebUICredentials() {
		boolean result = false;
		String s = getNewSendWebUICredentials();
		if (s != null && s.equalsIgnoreCase("true")) {
				result = true;
		}
		return result;
	}
	
	public String getBrowser() {
		String browser = getProperty(KEY_BROWSER);
		if (browser == null) {
			browser = "firefox";
		}
		if (browser.equals("chrome")) {
			Properties props = System.getProperties();
			props.setProperty("webdriver.chrome.driver", getChromeDriver());
		}
		if (browser.equals("iexplore")) {
			Properties props = System.getProperties();
			props.setProperty("webdriver.ie.driver", getIEDriver());
		}
		return getProperty(browser);
	}
	
	public String getChromeDriver() {
		return getProperty(KEY_CHROME_DIRVER);
	}
	
	public String getIEDriver() {
		return getProperty(KEY_IE_DIRVER);
	}
	
	public String getDebug() {
		return getProperty(KEY_DEBUG);
	}
	
	public boolean isDebug() {
		return getDebug().equalsIgnoreCase("true");
	}
	
	public String getTestcaseAuthor() {
		return getProperty(KEY_TC_AUTHOR);
	}

	public String getTestcaseDescription() {
		return getProperty(KEY_TC_DESC);
	}

	public String getTestArea() {
		return getProperty(KEY_TEST_AREA);
	}
	
	public String getGVTTestDataCategory() {
		return getProperty(KEY_GVT_TEST_DATA_CATEGORY);
	}
	
	public boolean isGB18030G2() {
		boolean result = false;
		if (getGVTTestDataCategory() != null) {
			if (getGVTTestDataCategory().equalsIgnoreCase(GVT_TEST_DATA_CATEGORY_G2)) {
				result = true;
			}
		}
		return result;
	}
	
	public String getGVT255CharactersTest() {
		return getProperty(KEY_GVT_255_CHARS_TEST);
	}
	
	public boolean isGVT255CharactersTest() {
		return getGVT255CharactersTest().equalsIgnoreCase("true");
	}
	
	public String getGVTGB18030CharactersTest() {
		return getProperty(KEY_GVT_GB18030_CHARS_TEST);
	}
	
	public boolean isGVTGB18030CharactersTest() {
		return getGVTGB18030CharactersTest().equalsIgnoreCase("true");
	}
	
}
