// Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.tools.msg;

import java.io.File;

/**
 * Constants contains all the constant variables for the compoent
 *
 *
 */
public class Constants {

	// public static final String BUILD_ROOT=System.getenv("BUILD_ROOT");

	public static final String ALL_COMPONENTS_STR = "UTILS;TRANSPORT;PROTOCOL;ENGINE;ADMIN;STORE;MONITORING;MQCONNECTIVITY;PROXY";

	public static String SRC_ROOT = null;

	public static final String ISM_MSG_KEY_PREFIX = "CWLNA";

	public static final int TRANSPORT_MSG_KEY_MIN = 1000;

	public static final int TRANSPORT_MSG_KEY_MAX = 1999;

	public static final String[] ISM_TRANSPORT_COMPONENTS_SRC = { "server_transport"
			+ File.separator + "src" };


	public static final String IGNORE_DUP = "LOG_TOOL_IGNORE_DUP";

	public static final int UTILS_MSG_KEY_MIN = 1;

	public static final int UTILS_MSG_KEY_MAX = 999;

	public static final String[] ISM_UTILS_COMPONENTS_SRC = {"server_utils"+ File.separator + "include"+File.separator+"ismrc.h",  "server_utils"
			+ File.separator + "src", "server_proxy"+ File.separator+"src"};

	public static final int PROTOCOL_MSG_KEY_MIN = 2000;

	public static final int PROTOCOL_MSG_KEY_MAX = 2999;

	public static final String[] ISM_PROTOCOL_COMPONENTS_SRC = { "server_protocol"
			+ File.separator + "src" };

	public static final int ENGINE_MSG_KEY_MIN = 3000;

	public static final int ENGINE_MSG_KEY_MAX = 3999;

	public static final String[] ISM_ENGINE_COMPONENTS_SRC = { "server_engine"
			+ File.separator + "src" };

	public static final int STORE_MSG_KEY_MIN = 4000;

	public static final int STORE_MSG_KEY_MAX = 4999;

	public static final String[] ISM_STORE_COMPONENTS_SRC = { "server_store"
			+ File.separator + "src" };

	public static final int ADMIN_MSG_KEY_MIN = 6000;

	public static final int ADMIN_MSG_KEY_MAX = 6499;

	public static final String[] ISM_ADMIN_COMPONENTS_SRC = { "server_admin"
			+ File.separator + "src" };

	public static final int MONITORING_MSG_KEY_MIN = 6500;

	public static final int MONITORING_MSG_KEY_MAX = 6999;

	public static final String[] ISM_MONITORING_COMPONENTS_SRC = {"server_monitoring"
			+ File.separator + "src"  };



	public static final int GUI_MSG_KEY_MIN = 5000;

	public static final int GUI_MSG_KEY_MAX = 5999;

	public static final String[] ISM_WEBUI_COMPONENTS_SRC = { "server_gui"
			+ File.separator + "src" };

	public static final int MQCONNECTIVITY_MSG_KEY_MIN = 7000;

	public static final int MQCONNECTIVITY_MSG_KEY_MAX = 7999;

	public static final String[] ISM_MQCONNECTIVITY_COMPONENTS_SRC = { "server_mqcbridge"
			+ File.separator + "src" };

	//PROXY is using Util from 900 to 999
	public static final int PROXY_MSG_KEY_MIN = 1900;

	public static final int PROXY_MSG_KEY_MAX = 1999;

	public static final String[] ISM_PROXY_COMPONENTS_SRC = { "server_proxy"
			+ File.separator + "src" };


}
