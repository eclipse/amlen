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

// Reviewed by Dev and ID
// Any exceptions are flagged with // Needs review until review is complete

define({
	root : ({
		// ------------------------------------------------------------------------
		// Global Definitions
		// ------------------------------------------------------------------------
		global : {
			productName : "${IMA_PRODUCTNAME_FULL}", // Do_Not_Translate
			productNameTM: "${IMA_PRODUCTNAME_FULL_HTML}",	// Do_Not_Translate 		
			webUIMainTitle: "${IMA_PRODUCTNAME_FULL} WebUI",
			node: "Node",
			// TRNOTE {0} is replaced with a user ID
			greeting: "{0}",    
			license: "License Agreement",
			menuContent: "Menu Selection Content",
			home : "Home",
			messaging : "Messaging",
			monitoring : "Monitoring",
			appliance : "Server",
			login: "Log in",
			logout: "Logout",
			changePassword: "Change Password",
			yes: "Yes",
			no: "No",
			all: "All",
			trueValue: "True",
			falseValue: "False",
			days: "days",
			hours: "hours",
			minutes: "minutes",
			// dashboard uptime, which shows a very short representation of elapsed time
			// TRNOTE e.g. '2d 1h' for 2 days and 1 hour
			daysHoursShort: "{0}d {1}h",  
			// TRNOTE e.g. '2h 30m' for 2 hours and 30 minutes
			hoursMinutesShort: "{0}h {1}m",
			// TRNOTE e.g. '1m 30s' for 1 minute and 30 seconds
			minutesSecondsShort: "{0}m {1}s",
			// TRNOTE  e.g. '30s' for 30 seconds
			secondsShort: "{0}s", 
			notAvailable: "NA",
			missingRequiredMessage: "A value is required",
			pageNotAvailable: "This page is not available because of the status of the ${IMA_SVR_COMPONENT_NAME}",
			pageNotAvailableServerDetail: "In order to operate this page, the ${IMA_SVR_COMPONENT_NAME} must be running in production mode.",
			pageNotAvailableHAroleDetail: "In order to operate this page, the ${IMA_SVR_COMPONENT_NAME} must be the primary server and not synchronizing, or HA must be disabled. "			
		},
		
		name: {
			label: "Name",
			tooltip: "The name must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. " +
					 "The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % &amp; ( ) * + , - . / : ; &lt; = &gt; ? @",
			invalidSpaces: "The name cannot have leading or trailing spaces.",
			noSpaces: "The name cannot have any spaces.",
			invalidFirstChar: "The first character cannot be a number, control character, or any of the following special characters: ! &quot; # $ % &amp; ' ( ) * + , - . / : ; &lt; = &gt; ? @",
			// disallow ",=\
			invalidChar: "The name cannot contain control characters or any of the following special characters: &quot; , = \\ ",
			duplicateName: "A record with that name already exists.",
			unicodeAlphanumericOnly: {
				invalidChar: "The name must consist of only alphanumeric characters.",
				invalidFirstChar: "The first character must not be a number."
			}
		},
		
		// ------------------------------------------------------------------------
		// Navigation (menu) items
		// ------------------------------------------------------------------------
		globalMenuBar: {
			messaging : {
				messageProtocols: "Messaging Protocols",
				userAdministration: "User Authentication",
				messageHubs: "Message Hubs",
				messageHubDetails: "Message Hub Details",
				mqConnectivity: "MQ Connectivity",
				messagequeues: "Message Queues",
				messagingTester: "Sample Application"
			},
			monitoring: {
				connectionStatistics: "Connections",
				endpointStatistics: "Endpoints",
				queueMonitor: "Queues",
				topicMonitor: "Topics",
				mqttClientMonitor: "Disconnected MQTT Clients",
				subscriptionMonitor: "Subscriptions",
				transactionMonitor: "Transactions",
				destinationMappingRuleMonitor: "MQ Connectivity",
				applianceMonitor: "Server",
				downloadLogs: "Download Logs",
				snmpSettings: "SNMP Settings"
			},
			appliance: {
				users: "Web UI Users",
				networkSettings: "Network Settings",
			    locale: "Locale, Date and Time",
			    securitySettings: "Security Settings",
			    systemControl: "Server Control",
			    highAvailability: "High Availability",
			    webuiSecuritySettings: "Web UI Settings"
			}
		},

		// ------------------------------------------------------------------------
		// Help
		// ------------------------------------------------------------------------
		helpMenu : {
			help : "Help",
			homeTasks : "Restore Tasks on Home Page",
			about : {
				linkTitle : "About",
				dialogTitle: "About ${IMA_PRODUCTNAME_FULL}",
				// TRNOTE: {0} is the license type which is Developers, Non-Production or Production
				viewLicense: "Show {0} License Agreement",
				iconTitle: "${IMA_PRODUCTNAME_FULL_HTML} Version ${ISM_VERSION_ID}"
			}
		},

		// ------------------------------------------------------------------------
		// Change Password dialog
		// ------------------------------------------------------------------------
		changePassword: {
			dialog: {
				title: "Change Password",
				currpasswd: "Current Password:",
				newpasswd: "New Password:",
				password2: "Confirm Password:",
				password2Invalid: "The passwords do not match",
				savingProgress: "Saving...",
				savingFailed: "Save failed."
			}
		},

		// ------------------------------------------------------------------------
		// Message Level Descriptions
		// ------------------------------------------------------------------------
		level : {
			Information : "Information",
			Warning : "Warning",
			Error : "Error",
			Confirmation : "Confirmation",
			Success : "Success"
		},

		action : {
			Ok : "OK",
			Close : "Close",
			Cancel : "Cancel",
			Save: "Save",
			Create: "Create",
			Add: "Add",
			Edit: "Edit",
			Delete: "Delete",
			MoveUp: "Move Up",
			MoveDown: "Move Down",
			ResetPassword: "Reset Password",
			Actions: "Actions",
			OtherActions: "Other Actions",
			View: "View",
			ResetColWidth: "Reset Column Widths",
			ChooseColumns: "Choose Visible Columns",
			ResetColumns: "Reset Visible Columns"
		},
		// new pages and tabs need to go here
        cluster: "Cluster",
        clusterMembership: "Join/Leave",
        adminEndpoint: "Admin Endpoint",
        firstserver: "Connect to a Server",
        portInvalid: "The port number must be a number in the range 1 to 65535.",
        connectionFailure: "Cannot connect to the ${IMA_SVR_COMPONENT_NAME_FULL}.",
        
        clusterStatus: "Status",
        
        webui: "Web UI",
        
        licenseType_Devlopers: "Developers",
        licenseType_NonProd: "Non-Production",
        licenseType_Prod: "Production",
        licenseType_Beta: "Beta"
	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
