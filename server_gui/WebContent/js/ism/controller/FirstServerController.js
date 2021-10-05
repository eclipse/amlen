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
define(['dojo/_base/declare',
        'dojo/_base/lang',
		'dojo/dom-style',
		'dijit/_base/manager',
		'ism/widgets/_TemplatedContent',
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/firstserver',
		'ism/controller/content/FirstServer',
		'ism/widgets/ExternalString'
		], function(declare, lang, domStyle, manager, _TemplatedContent, Resources, _PageController, nlsGlobal, nlsFirstServer) {

	var FirstServerController = declare("ism.controller.FirstServerController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the First Use page.
		
		name: "firstserver",
		widget: undefined,
		
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
	
	return FirstServerController;
});
