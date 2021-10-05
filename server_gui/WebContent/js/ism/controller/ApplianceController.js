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
        'dojo/_base/lang',
		'ism/widgets/_TemplatedContent',
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/snmp',
		'ism/widgets/ExternalString',
		'ism/controller/content/SystemWideSecurity',
		'ism/controller/content/ExternalLDAP',
		'ism/controller/content/CertificateProfiles',
		'ism/controller/content/LtpaProfiles',
		'ism/controller/content/OAuthProfiles',
		'ism/controller/content/SecurityProfiles',
		'ism/controller/content/MessagingServerControl',
		'ism/controller/content/MQConnectivityControl',
		'ism/controller/content/SNMPControl',
		'ism/controller/content/LogLevelControl',
		'ism/controller/content/HighAvailabilityConfig',
		'ism/controller/content/AdminEndpointConfig',
	    'ism/controller/content/ConfigurationPolicyList',
	    'ism/controller/content/AdminSuperUser'
		], function(declare, lang, _TemplatedContent, Resources, _PageController, nls, nlsAppl, nlsSnmp) {

	var ApplianceController = declare("ism.controller.ApplianceController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the Appliance page.
		
		name: "appliance",
		
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

	return ApplianceController;
});
