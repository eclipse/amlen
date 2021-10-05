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
package com.ibm.ism.test.testcases.ldap;

import java.io.FileInputStream;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Properties;

import com.ibm.ism.test.common.IsmTestData;

public class ImsLDAPTestData extends IsmTestData {
	
	public static final String KEY_LDAP_PROPERTIES_FILE = "LDAP_PROPERTIES_FILE";
	public static final String KEY_CLI_CREATE_LDAP = "CLI_CREATE_LDAP";
	public static final String KEY_CLI_DELETE_LDAP = "CLI_DELETE_LDAP";
	public static final String KEY_CLI_UPDATE_LDAP = "CLI_UPDATE_LDAP";
	public static final String KEY_CLI_SHOW_LDAP = "CLI_SHOW_LDAP";
	public static final String KEY_CLI_TEST_LDAP = "CLI_TEST_LDAP";
	
	public ImsLDAPTestData(String[] args) {
		super(args);
	}
	
	public boolean isCLICreateLDAP() {
		boolean test = false;
		if (getProperty(KEY_CLI_CREATE_LDAP) != null) {
			test = getProperty(KEY_CLI_CREATE_LDAP).equalsIgnoreCase("true");
		}
		return test;
	}

	public boolean isCLIDeleteLDAP() {
		boolean test = false;
		if (getProperty(KEY_CLI_DELETE_LDAP) != null) {
			test = getProperty(KEY_CLI_DELETE_LDAP).equalsIgnoreCase("true");
		}
		return test;
	}

	public boolean isCLIUpdateLDAP() {
		boolean test = false;
		if (getProperty(KEY_CLI_UPDATE_LDAP) != null) {
			test = getProperty(KEY_CLI_UPDATE_LDAP).equalsIgnoreCase("true");
		}
		return test;
	}

	public boolean isCLIShowLDAP() {
		boolean test = false;
		if (getProperty(KEY_CLI_SHOW_LDAP) != null) {
			test = getProperty(KEY_CLI_SHOW_LDAP).equalsIgnoreCase("true");
		}
		return test;
	}

	public boolean isCLITestLDAP() {
		boolean test = false;
		if (getProperty(KEY_CLI_TEST_LDAP) != null) {
			test = getProperty(KEY_CLI_TEST_LDAP).equalsIgnoreCase("true");
		}
		return test;
	}
	
	public String getLDAPPropertiesFileName() {
		return getProperty(KEY_LDAP_PROPERTIES_FILE);
	}
	
	public Properties getLDAPProperties() throws Exception {
		if (getLDAPPropertiesFileName() == null) {
			return null;
		}
		Properties props = new Properties();
		props.load(new FileInputStream(getLDAPPropertiesFileName()));
		return props;
	}
	
	public Map<String, String> getLDAPCommandOptions() throws Exception {
		Map<String, String> map = new HashMap<String, String>();
		Properties props = getLDAPProperties();
		if (props == null) {
			return null;
		}
		for(Entry<Object, Object> e : props.entrySet()) {
		    map.put((String)e.getKey(), (String)e.getValue());
		}
		return map;
	}
	
	public String getLDAPURL() throws Exception {
		return getLDAPProperties().getProperty("URL");
	}
	
	public String getLDAPBaseDN() throws Exception {
		return getLDAPProperties().getProperty("BaseDN");
	}
	
	public String getLDAPBindDN() throws Exception {
		return getLDAPProperties().getProperty("BindDN");
	}
	
	public String getLDAPBindPassword() throws Exception {
		return getLDAPProperties().getProperty("BindPassword");
	}
	
	public String getLDAPUserSuffix() throws Exception {
		return getLDAPProperties().getProperty("UserSuffix");
	}
	
	public String getLDAPGroupSuffix() throws Exception {
		return getLDAPProperties().getProperty("GroupSuffix");
	}
	
	public String getLDAPUserIdMap() throws Exception {
		return getLDAPProperties().getProperty("UserIdMap");
	}
	
	public String getLDAPGroupIdMap() throws Exception {
		return getLDAPProperties().getProperty("GroupIdMap");
	}
	
	public String getLDAPGroupMemberIdMap() throws Exception {
		return getLDAPProperties().getProperty("GroupMemberIdMap");
	}
	
	/**
	 * 
	 * @return "True" or "False" 
	 * @throws Exception
	 */
	public String getLDAPEnableCache() throws Exception {
		return getLDAPProperties().getProperty("EnableCache");
	}
	
	public String getLDAPCacheTimeout() throws Exception {
		return getLDAPProperties().getProperty("CacheTimeout");
	}
	
	public String getLDAPGroupCacheTimeout() throws Exception {
		return getLDAPProperties().getProperty("GroupCacheTimeout");
	}
	
	public String getLDAPTimeout() throws Exception {
		return getLDAPProperties().getProperty("Timeout");
	}
	
	public String getLDAPMaxConnections() throws Exception {
		return getLDAPProperties().getProperty("MaxConnections");
	}
	
	/**
	 * 
	 * @return "True" or "False"
	 * @throws Exception
	 */
	public String getLDAPIgnoreCase() throws Exception {
		return getLDAPProperties().getProperty("IgnoreCase");
	}
	
	/**
	 * 
	 * @return "True" or "False"
	 * @throws Exception
	 */
	public String getLDAPNestedGroupSearch() throws Exception {
		return getLDAPProperties().getProperty("NestedGroupSearch");
	}
	
	/**
	 * 
	 * @return "True" or "False"
	 * @throws Exception
	 */
	public String getLDAPEnabled() throws Exception {
		return getLDAPProperties().getProperty("Enabled");
	}
	
	
}
