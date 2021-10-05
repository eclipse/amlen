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
    root : {
    	serverMenuLabel: "Server:",
    	serverMenuHint: "Select a server to manage",
    	recentServersLabel: "Recent Servers:",
    	addRemoveLinkLabel: "Add or remove servers",
    	addRemoveDialogTitle: "Add or Remove Servers",
    	addRemoveDialogInstruction: "Add or remove ${IMA_SVR_COMPONENT_NAME}s that you can manage by using this Web UI.",
    	serverName: "Server Name",
    	serverNameTooltip: "The display name for this server.",
        adminAddress: "Admin Address:",
        adminAddressTooltip: "The IP address or hostname that the server's admin endpoint is listening on.",
        adminPort: "Admin Port:",
        adminPortTooltip: "The port that the server's admin endpoint is listening on. The port number must be in the range 1 to 65535 inclusive.",
        sendWebUICredentials: "Send Web UI Credentials:",
        sendWebUICredentialsTooltip: "Whether the Web UI should send the user ID and password that you logged in with to the admin endpoint. " +
        		"If the admin endpoint requires a valid user ID and password, the Web UI will send the user ID and password that you logged in with to the admin endpoint. " +
        		"Otherwise, you will be prompted for the user ID and password that are needed to access the admin endpoint.",
        //cluster: "Cluster",
        description: "Description:",
        portInvalidMessage: "The port number must be in the range 1 to 65535 inclusive.",
        duplicateServerTitle: "Duplicate server",
        duplicateServerMessage: "The server cannot be added because a server with the specified address and port is already in the list.",
        closeButton: "Close",
        saveButton: "Save",
        cancelButton: "Cancel",
        testButton: "Test connection",
        YesButton: "Yes",
        deletingProgress: "Deleting...",
        removeConfirmationTitle: "Are you sure that you want to remove this server from the list of managed servers?",
        addServerDialogTitle: "Add Server",
        editDialogTitle: "Edit Server",
        removeServerDialogTitle: "Remove Server",
        addServerDialogInstruction: "Add a ${IMA_SVR_COMPONENT_NAME} that you can manage by using this Web UI.  " /*+
        		"When you add a server that is in a cluster, other servers in the same cluster will appear in your list of servers."*/,
        testingProgress: "Testing...",
        testingFailed: "Test connection failed.",
        testingSuccess: "Test connection succeeded."        
    },
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
