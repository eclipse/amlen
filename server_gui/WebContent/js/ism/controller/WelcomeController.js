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
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/strings',
		'ism/controller/content/Tasks',
		'ism/controller/content/FlexDashboard'
		], function(declare, Resources, _PageController, nls) {

	var WelcomeController = declare("ism.controller.WelcomeController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the Welcome page.
		
		name: "home",
		
		constructor: function() {
		},

		init: function() {
			console.debug(this.declaredClass + ".init()");
			this.inherited(arguments);
		},

		_initLayout: function() {
			console.debug(this.declaredClass + "._initLayout()");
			// for debugging charts
			if (this.d3 !== undefined && Resources.flexDashboard.availableSections[this.d3] !== undefined) {
				Resources.flexDashboard.defaultSections = [this.d3];
			} 			
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

	return WelcomeController;
});
