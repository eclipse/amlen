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
	    
	    title:  "Configuration Policies",
	    tagline: "A configuration policy controls which administrative tasks a user is permitted to perform. " +
	    		"Configuration policies must be added to the admin endpoint in order to take effect.",
	    
	    // Add / Edit dialog
	    addDialogTitle: "Add Configuration Policy",
	    editDialogTitle: "Edit Configuration Policy",
	    dialogInstruction: "A configuration policy controls which administrative tasks a user is permitted to perform.",
	    
		nameLabel:  "Name:",
		nameColumnLabel:  "Name",
		descriptionLabel: "Description:",
		descriptionColumnLabel: "Description",
		authorityLabel: "Authority:",
		authorityColumnLabel: "Authority",
		authorityTooltip: "<dl><dt>Configure:</dt><dd>Grants authority to modify the server configuration.</dd>" +
		                      "<dt>View:</dt><dd>Grants authority to view the server configuration and status.</dd>" +
                              "<dt>Monitor:</dt><dd>Grants authority to view monitoring data.</dd>" +
                              "<dt>Manage:</dt><dd>Grants authority to issue service requests, such as restarting the server.</dd>" +
                          "</dl>",
		configure: "Configure",
		view: "View",
		monitor: "Monitor",
		manage: "Manage",
		listSeparator: ",",

	    dialogFilterInstruction: "To restrict the administrative or monitoring actions that are defined in this policy to specific users or groups of users, specify one or more of the following filters. " +
            "For example, select Group ID to restrict this policy to members of a particular group. " +
            "The policy allows access only when all of the specified filters are true:",
        dialogFilterTooltip: "You must specify at least one of the filter fields. " +
            "You can use a single asterisk (*) as the last character in most filters to match 0 or more characters. " +
            "The client IP address can contain an asterisk(*) or be a comma-delimited list of IPv4 or IPv6 addresses or ranges, for example: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
            "IPv6 addresses must be enclosed in square brackets, for example: [2001:db8::].",

	    clientIPLabel: "Client IP Address:",
	    clientIPColumnLabel: "Client IP Address",
	    userIdLabel: "User ID:",
	    userIdColumnLabel: "User ID",
	    groupIdLabel: "Group ID:",
	    groupIdColumnLabel: "Group ID",
	    commonNameLabel: "Certificate Common Name:",
	    commonNameColumnLabel: "Certificate Common Name",
	    
	    invalidWildcard: "You can use a single asterisk (*) as the last character to match 0 or more characters.",
	    invalidClientIP: "The client IP address must contain an asterisk(*) or be a comma-delimited list of IPv4 or IPv6 addresses or ranges, for example: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
            "IPv6 addresses must be enclosed in square brackets, for example: [2001:db8::].",
        invalidFilter: "You must specify at least one of the filter fields.",
	    
	    // buttons
	    saveButton: "Save",
	    cancelButton: "Cancel",
	    closeButton: "Close",
	    
        // messages
        savingProgress: "Saving...",
        updatingMessage: "Updating...",
        deletingProgress: "Deleting...",
        noItemsGridMessage: "No items to display",
        getConfigPolicyError: "An error occurred while the configuration policies were being retrieved.",
        saveConfigPolicyError: "An error occurred while the configuration policy was being saved.",
        deleteConfigurationPolicyError: "An error occurred while the configuration policy was being deleted.",
        
        // remove dialog
        removeDialogTitle: "Delete Configuration Policy",
        removeDialogContent: "Are you sure that you want to delete this configuration policy?"
	}),
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
