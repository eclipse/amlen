/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
    root : {
        all: "All",
        unavailableAddressesTitle: "Some agent addresses are not available.",
        // TRNOTE {0} is a list of IP addresses
        unavailableAddressesMessage: "SNMP agent addresses must be valid, active ethernet interface addresses. " +
        		"The following agent addresses are configured but are not available: {0}",
        agentAddressInvalidMessage: "Select either <em>All</em> or at least one IP address.",
        notRunningError: "SNMP is enabled but not running.",
        snmpRestartFailureMessageTitle: "SNMP service failed to restart.",
        statusLabel: "Status",
        running: "Running",
        stopped: "Stopped",
        unknown: "Unknown",
    		
        title: "SNMP Service",
		tagLine: "Settings and actions that affect the SNMP service.",
    	enableLabel: "Enable SNMP",
    	stopDialogTitle: "Stop the SNMP Service",
    	stopDialogContent: "Are you sure that you want to stop the SNMP service?  Stopping the service might cause loss of SNMP messages.",
    	stopDialogOkButton: "Stop",
    	stopDialogCancelButton: "Cancel",
    	stopping: "Stopping",
    	stopError: "An error occurred while stopping the SNMP service.",
    	starting: "Starting",
    	started: "Running",
    	savingProgress: "Saving...",
    	setSnmpEnabledError: "An error occurred while setting the SNMP enabled state.",
    	startError: "An error occurred while starting the SNMP service.",
    	startLink: "Start the service",
    	stopLink: "Stop the service",
		savingProgress: "Saving..."
    },
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
