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
define([

// General
	"ism/tests/widgets/test_login",
	"ism/tests/widgets/test_License",
	"ism/tests/widgets/test_Preferences",

// Messaging
	// Message Hubs
	"ism/tests/widgets/test_Protocols",
	"ism/tests/widgets/test_MessageHub", 
	"ism/tests/widgets/test_ConnectionPolicy",
	"ism/tests/widgets/test_MessagingPolicy",
	"ism/tests/widgets/test_Endpoint",
	
	// Queues
	"ism/tests/widgets/test_Queue",

    // MQ Connectivity
	"ism/tests/widgets/test_MQConnectivity",
	
	// Messaging Users
	"ism/tests/widgets/test_MessagingUser",
	"ism/tests/widgets/test_LDAP",	

// Monitoring
	"ism/tests/widgets/test_Monitoring_Connections",
	"ism/tests/widgets/test_Monitoring_Endpoints",
	"ism/tests/widgets/test_Monitoring_Queues",
	"ism/tests/widgets/test_Monitoring_Topics",
	"ism/tests/widgets/test_Monitoring_Transactions",
	"ism/tests/widgets/test_Monitoring_MQTTClients",
	"ism/tests/widgets/test_Monitoring_Subscriptions",
	"ism/tests/widgets/test_Monitoring_MQConnectivity",
	"ism/tests/widgets/test_Monitoring_Appliance",	
	"ism/tests/widgets/test_DownloadLogs",
	"ism/tests/widgets/test_SNMP",

	// Appliance
	"ism/tests/widgets/test_WebUiUser",

	// Network settings
	"ism/tests/widgets/test_OpState",		
	"ism/tests/widgets/test_Ethernet",
	"ism/tests/widgets/test_DnsServers",
	"ism/tests/widgets/test_NetworkInterfaces",

	// Security Settings
	"ism/tests/widgets/test_FIPS",
	"ism/tests/widgets/test_CertificateProfiles",
	"ism/tests/widgets/test_SecurityProfiles",
	"ism/tests/widgets/test_OAuthProfiles",
	
	// SystemControl
	"ism/tests/widgets/test_Status",
    "ism/tests/widgets/test_SystemControl",
	"ism/tests/widgets/test_LogLevel",
	
	// Store Mode
	//"ism/tests/widgets/test_EnableDiskPersistence",

    // HA
    "ism/tests/widgets/test_HA",  
   
    // WebUI Settings
	"ism/tests/widgets/test_LibertyProperties",

	// Locale, Date, and Time     
	"ism/tests/widgets/test_Locale",
	"ism/tests/widgets/test_TimeZones",
	"ism/tests/widgets/test_NtpServers",
	"ism/tests/widgets/test_DateTime"  // This one is last because it restarts WebUI	
	], function() {});

