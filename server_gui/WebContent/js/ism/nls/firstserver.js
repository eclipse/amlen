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
		title : "Get started with ${IMA_PRODUCTNAME_FULL} Web UI",
		tagline: "<br /><p>${IMA_PRODUCTNAME_SHORT} Web UI can manage and monitor ${IMA_SVR_COMPONENT_NAME}s that are version 2.0 or later.</p><br />" +
		     "<p>To get started, you need to know the hostname or IP address that the Web UI can use to communicate with a ${IMA_PRODUCTNAME_SHORT} administrative server. " +
		     "If the administrative server cannot be accessed via the default port, 9089, then you also need to know what port the Web UI can use to access the server.</p><br />" +
		     "<p>To manage additional servers, use the Servers menu that appears in the menu bar after you click Save and Manage.</p><br />",
		fsTitle: "Configure a server connection",
		fsTagline: "Specify the first ${IMA_SVR_COMPONENT_NAME} you want to manage by using this Web UI.",
		savedTitle: "Server saved",
		serverSaved: "The server has been saved.",
		saveButton: "Save and Manage"
	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
