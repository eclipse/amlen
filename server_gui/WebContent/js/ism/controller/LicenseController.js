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
define(['dojo/_base/declare',
        'dijit/_base/manager',
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/license',
		'ism/widgets/User',
		'ism/Utils',
		'ism/controller/content/LicensePresent',
		'ism/widgets/Dialog'
		], function(declare, manager, Resources, _PageController, nls, User, Utils, 
				LicensePresent, Dialog) {

	var LicenseController = declare("ism.controller.LicenceController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the License view page (allows Web UI 
		//      users to view the available product licenses).
		
		name: "license",
		
		constructor: function() {
		},

		init: function() {
			console.debug(this.declaredClass + ".init()");
			this.inherited(arguments);
		},

		_initLayout: function() {
			console.debug(this.declaredClass + "._initLayout()");
			this.inherited(arguments);
		},

		_initWidgets: function() {
			console.debug(this.declaredClass + "._initWidgets()");
			this.inherited(arguments);
		},

		_initEvents: function() {
			console.debug(this.declaredClass + "._initEvents()");
			this.inherited(arguments);
		}
	});

	return LicenseController;
});
