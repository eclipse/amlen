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
var profile = (function() {

	return {

		releaseDir : "../dojobuild",

		basePath : "./WebContent",

		action : "release",

		// Enabling this causes errors with IDX 1.3
		//cssOptimize : "comments",

		mini : true,

		// Enabling this causes errors with the dojo/i18n module
		//optimize : "shrinksafe",

		layerOptimize : "shrinksafe",
		
		selectorEngine: "acme",

		stripConsole : "all",

		dojoConfig : "./js/dojoConfig.js",

		layers : {
			"ism/layers/common___buildLabel__" : {
				include : [ 
				            "ism/controller/ApplianceController",
				            "ism/controller/FirstServerController",
				            "ism/controller/LicenseController",
				            "ism/controller/MessagingController",
				            "ism/controller/MonitoringController",
				            "ism/controller/WebUIController",
				            "ism/controller/WelcomeController",
				            "ism/controller/ServerLicenseController",
				            ],
				exclude : [ "d3/d3.min"]
			}
		}
	};
})();
