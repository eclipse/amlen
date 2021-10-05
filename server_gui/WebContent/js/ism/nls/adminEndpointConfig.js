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
	    configurationTitle: "Admin Endpoint",
	    editLink: "Edit",
	    configurationTagline_unsecured: "The admin endpoint is not secured. " +
	            "The admin endpoint allows you to administer the server by using the Web UI or REST APIs. " +
	    		"To secure the endpoint, configure a security profile and one or more configuration policies.",
	    configurationTagline: "The admin endpoint allows you to administer the server by using the Web UI or REST APIs. " +
	    		"Verify that the appropriate security profile and configuration policies are available.",
	    	    	    
	    editDialogTitle: "Edit Admin Endpoint",
	    editDialogInstruction: "The Web UI and REST API connect to the admin endpoint to perform administrative and monitoring tasks.",
	    		
	    // property labels and hover helps
	    interfaceLabel:  "IP Address:",
	    interfaceTooltip: "The IP Address that the admin endpoint listens on.",
	    portLabel: "Port:",
	    portTooltip: "The port that the admin endpoint listens on. The port number must be in the range 1 to 65535 inclusive.",
	    securityProfileLabel: "Security Profile:",
	    securityProfileTooltip: "To secure connections and define how authentication is performed, select a security profile. " +
	    		"The existing security profiles are included in the list. " +
	    		"To create a new security profile, go to the <em>Security Settings</em> page.",
	    configurationPoliciesLabel: "Configuration Policies:",
	    configurationPoliciesTooltip: "To secure the admin endpoint, add at least one but not more than 100 configuration policies. " +
	    		"A configuration policy authorizes connected clients to perform specific administrative and monitoring tasks. " +
	    		"Configuration policies are evaluated in the order shown. To change the order, use the up and down arrows on the toolbar.",
	    
	    // configuration policy chooser dialog
	    addConfigPolicyDialogTitle: "Add Configuration Policies to the Admin Endpoint",
	    addConfigPoliciesDialogInstruction: "A configuration policy authorizes connected clients to perform specific administrative and monitoring tasks.",
	    addLabel: "Add",
	    configurationPolicyLabel: "Configuration Policy",
	    descriptionLabel: "Description",	
	    
	    authorityLabel: "Authority",
	    
	    none: "None",
	    all: "All",
	    
	    // buttons
	    saveButton: "Save",
	    cancelButton: "Cancel",
	    add: "Add",
	    
        // messages
        savingProgress: "Saving...",
        getAdminEndpointError: "An error occurred while the admin endpoint configuration was being retrieved.",
        saveAdminEndpointError: "An error occurred while the admin endpoint configuration was being saved.",
        getSecurityProfilesError: "An error occurred while the security profiles were being retrieved.",
        tooManyPolicies: "Too many policies are selected. The admin endpoint can have a maximum of 100 configuration policies.",
        certificateInvalidMessage: "The Web UI cannot connect to the ${IMA_SVR_COMPONENT_NAME}. " +
            "The server might be stopped, or the certificate that secures the admin endpoint might be invalid. " +
            "If the certificate is invalid, you must add an exception. " +
            "<a href='{0}' target='_blank'>Click here</a> to navigate to a page that will prompt you to add an exception if one is required. ",
        certificateRefreshInstruction: "After adding the exception, you might need to refresh this page. ",
        certificatePreventError: "Refer to the ${IMA_PRODUCTNAME_SHORT} documentation in IBM Knowledge Center for information about preventing this problem.",
        		
        // AdminUserID and AdminUserPassword
        superUserTitle: "Admin Superuser",
        superUserTagline: "The admin superuser ID and password are stored locally to ensure that the ${IMA_SVR_COMPONENT_NAME} can be administered even if LDAP is misconfigured or unavailable. " +
        		"The admin superuser has authority to perform all actions. " +
        		"Ensure that the admin superuser ID and password are difficult to guess.",

        adminUserIDLabel: "User ID:",
        changeAdminUserIDLink: "Change User ID",
        editAdminUserIdDialogTitle: "Edit Admin Superuser ID",
        editAdminUserIdDialogInstruction: "Change the admin superuser ID to a value that is difficult to guess.",
        saveAdminUserIDError: "An error occurred while the admin superuser ID was being saved.",
        getAdminUserIDError: "An error occurred while the admin superuser ID was being retrieved.",

        changeAdminUserPasswordLink: "Change Password",
        editAdminUserPasswordDialogTitle: "Edit Admin Superuser Password",
        editAdminUserPasswordDialogInstruction: "Change the admin superuser password to a value that is difficult to guess.",
        adminUserPasswordLabel: "Password",
        confirmPasswordLabel: "Confirm Password",
        passwordInvalidMessage : "Passwords do not match.",
        saveAdminUserPasswordError: "An error occurred while the admin superuser password was being saved."            
	}),
    
    "zh": true,
    "zh-tw": true,
    "ja": true,
    "fr": true,
    "de": true

});
