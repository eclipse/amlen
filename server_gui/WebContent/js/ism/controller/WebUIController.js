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
		'ism/widgets/_TemplatedContent',
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/appliance',
		'ism/controller/content/UsersGroups',
		'ism/widgets/ExternalString',
		'ism/controller/content/WebUITimeouts',
		'ism/controller/content/WebUICacheAuth',
		'ism/controller/content/WebUIPort',
		'ism/controller/content/WebUICertificate',
		'ism/controller/content/WebUIService',
		'ism/controller/content/WebUIReadTimeout'
		], function(declare, lang, _TemplatedContent, Resources, _PageController, nls, nlsAppl) {

	var WebUIController = declare("ism.controller.ApplianceController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the Appliance page.
		
		name: "webui",
		
		constructor: function() {
		},

		init: function() {
			console.debug(this.declaredClass + ".init()");
			this.inherited(arguments);
		},

		_initLayout: function() {
			console.debug(this.declaredClass + "._initLayout()");
			var currentUri;
            currentUri = Resources.pages.appliance.nav.systemControl.uri;
			this.inherited(arguments);
		},

		_initWidgets: function() {
			console.debug(this.declaredClass + "._initWidgets()");
			this.inherited(arguments);
		},

		_initEvents: function() {
			console.debug(this.declaredClass + "._initEvents()");
			this.inherited(arguments);

			dojo.subscribe(Resources.contextChange.nodeNameTopic, lang.hitch(this, function(nodeName){
				var newNodeName = nodeName === null || nodeName === undefined || nodeName === nlsAppl.appliance.systemControl.appliance.hostnameNotSet ? "" : nodeName;
				if (newNodeName != this.nodeName) {
					console.debug("Updating node name to " + newNodeName);
					this.updateNodeName(newNodeName);
				}
			}));					

		}
	});

	return WebUIController;
});
