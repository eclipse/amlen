/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
	    // Page title and tagline

	    title: "Cluster Membership",
		tagline: "Control whether the server belongs to a cluster. " +
				"A cluster is a collection of servers connected together on a local high-speed LAN with the goal of scaling either the maximum number of concurrent connections or the maximum throughput beyond the capability of a single device. " +
				"Servers in a cluster share a common topic tree.",

		// Status section
				
		statusTitle: "Status",
		statusNotConfigured: "The server is not configured to participate in a cluster. " +
				"To enable the server to participate in a cluster, edit the configuration and then add the server to the cluster.",
		// TRNOTE {0} is replaced by the user specified cluster name.
		statusNotAMember: "This server is not a member of a cluster. " +
				"To add the server to the cluster, click <em>Restart server and join cluster {0}</em>.",
        // TRNOTE {0} is replaced by the user specified cluster name.
		statusMember: "This server is a member of cluster {0}. To remove this server from the cluster, click <em>Leave cluster</em>.",		
		clusterMembershipLabel: "Cluster Membership:",
		notConfigured: "Not configured",
		notSet: "Not set",
		notAMember: "Not a member",
        // TRNOTE {0} is replaced by the user specified cluster name.
        member: "Member of cluster {0}",

        // Cluster states  
        clusterStateLabel: "Cluster State:", 
        initializing: "Initializing",       
        discover: "Discover",               
        active:  "Active",                   
        removed: "Removed",                 
        error: "Error",                     
        inactive: "Inactive",               
        unavailable: "Unavailable",         
        unknown: "Unknown",                 
        
        // TRNOTE {0} is replaced by the user specified cluster name.
        joinLink: "Restart server and join cluster <em>{0}</em>",
        leaveLink: "Leave cluster",
        
        
        // Configuration section

        configurationTitle: "Configuration",
		editLink: "Edit",
	    resetLink: "Reset",
		configurationTagline: "The cluster configuration is shown. " +
				"Changes to the configuration do not take effect until the ${IMA_SVR_COMPONENT_NAME} is restarted.",
		clusterNameLabel: "Cluster Name:",
		controlAddressLabel: "Control Address",
		useMulticastDiscoveryLabel: "Use Multicast Discovery:",
		withMembersLabel: "Discovery Server List:",
		multicastTTLLabel: "Multicast Discovery TTL:",
		controlInterfaceSectionTitle: "Control Interface",
        messagingInterfaceSectionTitle: "Messaging Interface",
		addressLabel: "Address:",
		portLabel: "Port:",
		externalAddressLabel: "External Address:",
		externalPortLabel: "External Port:",
		useTLSLabel: "Use TLS:",
        yes: "Yes",
        no: "No",
        discoveryPortLabel: "Discovery Port:",
        discoveryTimeLabel: "Discovery Time:",
        seconds: "{0} seconds", 

		editDialogTitle: "Edit Cluster Configuration",
        editDialogInstruction_NotAMember: "Set a cluster name to determine which cluster this server joins. " +
                "After saving a valid configuration, you can join the cluster.",
        editDialogInstruction_Member: "Change cluster configuration. " +
        		"To change the name of the cluster this server belongs to, you must first remove the server from the {0} cluster.",
        saveButton: "Save",
        cancelButton: "Cancel",
        advancedSettingsLabel: "Advanced Settings",
        advancedSettingsTagline: "You can configure the following settings. " +
                "Refer to the ${IMA_PRODUCTNAME_FULL} documentation for detailed instructions and recommendations before you change these settings.",
		
		// Tooltips

        clusterNameTooltip: "The name of the cluster to join.",
        membersTooltip: "Comma-delimited list of server URIs for members belonging to the cluster that this server wants to join. " +
        		"If <em>Use Multicast Discovery</em> is not checked, specify at least one server URI. " +
        		"The server URI for a member is displayed on that member's Cluster Membership page.",
        useMulticastTooltip: "Specify if multicast should be used to discover servers with the same cluster name.",
        multicastTTLTooltip: "If you are using multicast discovery, this specifies the number of network hops that multicast traffic is allowed to pass through.", 
        controlAddressTooltip: "The IP address of the control interface.",
        controlPortTooltip: "The port number to use for the control interface.  The default port is 9104. ",
        controlExternalAddressTooltip: "The external IP address or host name of the control interface if it is different from the control address. " +
        		"The external address is used by other members to connect to this server's control interface.",
		controlExternalPortTooltip: "The external port to use for the control interface if it is different from the control port. "  +
                "The external port is used by other members to connect to this server's control interface.",
        messagingAddressTooltip: "The IP address of the messaging interface.",
        messagingPortTooltip: "The port number to use for the messaging interface.  The default port is 9105. ",
        messagingExternalAddressTooltip: "The external IP address or host name of the messaging interface if it is different from the messaging address. " +
                "The external address is used by other members to connect to this server's messaging interface.",
        messagingExternalPortTooltip: "The external port to use for the messaging interface if it is different from the messaging port. "  +
                "The external port is used by other members to connect to this server's messaging interface.",
        discoveryPortTooltip: "The port number to use for multicast discovery. The default port is 9106. " +
        		"The same port must be used on all members of the cluster.",
        discoveryTimeTooltip: "The time, in seconds, spent during server start up to discover other servers in the cluster and get updated information from them. " +
                "The valid range is 1 - 2147483647. The default is 10. " + 
                "This server becomes an active member of the cluster after the updated information is obtained or the discovery timeout occurs, whichever occurs first.",
                
        controlInterfaceSectionTooltip: "The control interface is used for configuration, monitoring, and control of the cluster.",
        messagingInterfaceSectionTooltip: "The messaging interface enables messages that have been published from a client to a member of the cluster to be forwarded to other members of the cluster.",
        
        // confirmation dialogs
        restartTitle: "Restart ${IMA_PRODUCTNAME_SHORT}",
        restartContent: "Are you sure that you want to restart ${IMA_PRODUCTNAME_SHORT}?",
        restartButton: "Restart",

        resetTitle: "Reset Cluster Membership",
        resetContent: "Are you sure that you want to reset the cluster membership configuration?",
        resetButton: "Reset",

        leaveTitle: "Leave Cluster",
        leaveContent: "Are you sure that you want to leave the cluster?",
        leaveButton: "Leave",      
        
        restartRequiredTitle: "Restart Required",
        restartRequiredContent: "Your changes were saved, but will not take effect until the ${IMA_SVR_COMPONENT_NAME} is restarted.<br /><br />" +
                 "To restart the ${IMA_SVR_COMPONENT_NAME} now, click <b>Restart Now</b>. Otherwise, click <b>Restart Later</b>.",
        restartLaterButton: "Restart Later",
        restartNowButton: "Restart Now",

        
        // messages
        savingProgress: "Saving...",
        errorGettingClusterMembership: "An error occurred while retrieving the cluster membership information.",
        saveClusterMembershipError: "An error occurred while saving the cluster membership configuration.",
        restarting: "Restarting the ${IMA_SVR_COMPONENT_NAME}...",
        restartingFailed: "Failed to restart the ${IMA_SVR_COMPONENT_NAME}.",
        resetting: "Resetting the configuration...",
        resettingFailed: "Failed to reset the cluster membership configuration.",
        addressInvalid: "A valid IP address is required.", 
        portInvalid: "The port number must be a number between 1 and 65535 inclusive."

	}),
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
