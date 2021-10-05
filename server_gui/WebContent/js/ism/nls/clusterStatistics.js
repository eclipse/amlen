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
        // TRNOTE: {0} is replaced with the server name of the server currently managed in the Web UI.
        //         {1} is replaced with the name of the cluster where the current server is a member.
		detail_title: "Cluster Status for {0} in Cluster {1}",
		title: "Cluster Status",
		tagline: "Monitor the status of cluster members. View statistics about the messaging channels on this server. Messaging channels provide " +
				"connectivity to and buffer messages for remote cluster members.  If buffered messages for inactive members are consuming too many " +
				"resources, you can remove the inactive members from the cluster.",
		chartTitle: "Throughput Chart",
		
		gridTitle: "Cluster Status Data",
		none: "None",
		lastUpdated: "Last Updated",
		notAvailable: "n/a",
		cacheInterval: "Data Collection Interval: 5 seconds",
		refreshingLabel: "Updating...",
		resumeUpdatesButton: "Resume updates",
		updatesPaused: "Data collection is paused",
		serverNameLabel: "Server Name",
		connStateLabel: "Connection State:",
		healthLabel: "Server Health",
		haStatusLabel: "HA Status:",
		updTimeLabel: "Last State Update",
		incomingMsgsLabel: "Incoming messages/second:",
		outgoingMsgsLabel: "Outgoing messages/second:",
		bufferedMsgsLabel: "Buffered Messages:",
		discardedMsgsLabel: "Discarded Messages:",
			
		serverNameTooltip: "Server Name",
		connStateTooltip: "Connection State",
		healthTooltip: "Server Health",
		haStatusTooltip: "HA Status",
		updTimeTooltip: "Last State Update",
		incomingMsgsTooltip: "Incoming messages/second",
		outgoingMsgsTooltip: "Outgoing messages/second",
		bufferedMsgsTooltip: "Buffered Messages",
		discardedMsgsTooltip: "Discarded Messages",
		
		statusUp: "Active",
		statusDown: "Inactive",
		statusConnecting: "Connecting",
		
		healthUnknown: "Unknown",
		healthGood: "Good",
		healthWarning: "Warning",
		healthBad: "Error",
		
		haDisabled: "Disabled",
		haSynchronized: "Synchronized",
		haUnsynchronized: "Unsynchronized",
		haError: "Error",
		haUnknown: "Unknown",
		
		BufferedAssocType: "Buffered Messages Details",
		DiscardedAssocType: "Discarded Messages Details",
		
		channelTitle: "Channel",
		channelTooltip: "Channel",
		channelUnreliable: "Unreliable",
		channelReliable: "Reliable",
		bufferedBytesLabel: "Buffered Bytes",
		bufferedBytesTooltip: "Buffered Bytes",
		bufferedHWMLabel: "Buffered Messages HWM",
		bufferedHWMTooltip: "Buffered Messages HWM",
			
		addServerDialogInstruction: "Add the cluster member to the list of managed servers and manage the cluster member server.",
		saveServerButton: "Save and manage",
		
		removeServerDialogTitle: "Remove Server from Cluster",
		removeConfirmationTitle: "Are you sure that you want to remove this server from the cluster?",
		removeServerError: "An error occurred while deleting the server from the cluster.",

		removeNotAllowedTitle: "Remove not Allowed",
		removeNotAllowed: "The server cannot be removed from the cluster because the server is currently active. " +
		                  "Use the remove action to remove only those servers that are not active.",

		cancelButton: "Cancel",
		yesButton: "Yes",
		closeButton: "Close"
	}),
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
