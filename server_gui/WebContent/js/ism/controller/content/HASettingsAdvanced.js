/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
		'dojo/_base/json',
		'dojo/dom-construct',
		'dojo/store/Memory',
		'dojo/number',
		'dojo/query',
		'dijit/registry',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',	
		'dijit/form/Button',
		'idx/form/CheckBox',
		'ism/widgets/RadioButtonSet',
		'ism/widgets/TextBox',
		'ism/config/Resources',
		'ism/widgets/ToggleContainer',
		'ism/IsmUtils',
		'dojo/i18n!ism/nls/appliance',
		'dojo/text!ism/controller/content/templates/HAFormAdvanced.html'	 
		], function(declare, lang, json, domConstruct, Memory, number, query,  registry, _Widget, _TemplatedMixin, 
				_WidgetsInTemplateMixin, Button, CheckBox, RadioButtonSet, TextBox, Resources, ToggleContainer, Utils, nls, template) {

	var HAFormAdvanced = declare("ism.controller.content.HAFormAdvanced", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
		//allowSingleNIC: false,

		// template strings
		startupModeLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.startupMode),
		startModeTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.startModeTooltip),
		nodePreferredLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.nodePreferred),
		localReplicationInterfaceLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localReplicationInterface),
		localReplicationInterfaceTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localReplicationInterfaceTooltip),
		localDiscoveryInterfaceLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localDiscoveryInterface),
		localDiscoveryInterfaceTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localDiscoveryInterfaceTooltip),
		remoteDiscoveryInterfaceLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.remoteDiscoveryInterface),
		remoteDiscoveryInterfaceTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.remoteDiscoveryInterfaceTooltip),
		ipAddrInvalid: Utils.escapeApostrophe(nls.appliance.highAvailability.ipAddrValidation),
		discoveryTimeoutLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.discoveryTimeout),
		discoveryTimeoutTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.discoveryTimeoutTooltip),
		heartbeatTimeoutLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.heartbeatTimeout),
		heartbeatTimeoutTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.heartbeatTimeoutTooltip),
		unitLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.seconds),
		
		replicationPortLabel: Utils.escapeApostrophe(nls.haReplicationPortLabel),
		replicationPortTooltip: Utils.escapeApostrophe(nls.haReplicationPortTooltip),
		replicationPortInvalid: Utils.escapeApostrophe(nls.haReplicationPortInvalid),
		externalReplicationInterfaceLabel: Utils.escapeApostrophe(nls.haExternalReplicationInterfaceLabel),
		externalReplicationInterfaceTooltip: Utils.escapeApostrophe(nls.haExternalReplicationInterfaceTooltip),
		externalReplicationPortLabel: Utils.escapeApostrophe(nls.haExternalReplicationPortLabel),
		externalReplicationPortTooltip: Utils.escapeApostrophe(nls.haExternalReplicationPortTooltip),
		externalReplicationPortInvalid: Utils.escapeApostrophe(nls.haExternalReplicationPortInvalid),
		remoteDiscoveryPortLabel: Utils.escapeApostrophe(nls.haRemoteDiscoveryPortLabel),
		remoteDiscoveryPortTooltip: Utils.escapeApostrophe(nls.haRemoteDiscoveryPortTooltip),
		remoteDiscoveryPortInvalid: Utils.escapeApostrophe(nls.haRemoteDiscoveryPortInvalid),
		useSecuredConnectionsLabel: Utils.escapeApostrophe(nls.haUseSecuredConnectionsLabel),
		useSecuredConnectionsTooltip: Utils.escapeApostrophe(nls.haUseSecuredConnectionsTooltip),
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		startup: function(args) {
			this.inherited(arguments);
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			var _this = this;
			
			this.field_ExternalReplicationNIC.isValid = this.nicValidator;
			
			this.field_DiscoveryTimeout.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			
			this.field_DiscoveryTimeout.isValid = function(focused) {
				var value = this.get("value");
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				if (isNaN(intValue) || intValue < 10 || intValue > 2147483647) {
					return false;
				}
				return true;
			};
			
			this.field_HeartbeatTimeout.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			
			this.field_HeartbeatTimeout.isValid = function(focused) {
				var value = this.get("value");
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				if (isNaN(intValue) || intValue < 1 || intValue > 2147483647) {
					return false;
				}
				return true;
			};
			
			this.field_ReplicationPort.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			this.field_ReplicationPort.isValid = this.portValidator;
			
			this.field_ExternalReplicationPort.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			this.field_ExternalReplicationPort.isValid = this.portValidator;
			
			this.field_RemoteDiscoveryPort.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			this.field_RemoteDiscoveryPort.isValid = this.portValidator;
		},
		
		validateAdvancedForm: function() {
			
			// keep track of first non-valid element
			var focusElement = null;
			// set to false if any elements are not valid
			var formValid = true;
			
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					if (!this[prop].validate()) {
						formValid = false;
						this[prop].focus();
						if (!focusElement) {
							focusElement = this[prop];
						}
					}
				}
			}
			
			
			// set focus to first invalid element
			if (!formValid) {
				focusElement.focus();
			}
			
			return (formValid);
			
		},
		
		portValidator: function(focused, context) {
			
			if (!context) {
				context = this;
			}
			
			var value = context.get("value");
			var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
			if (intValue === 0) {
				return true;
			}
			if (isNaN(intValue) || intValue < 1024 || intValue > 65635) {
				return false;
			}
			return true;
			
		},
		
		nicValidator: function(focused, context) {
			
			if (!context) {
				context = this;
			}
			
			// remote interface may be disabled - always validate true...
			if (context.get("disabled")) {
				return true;
			}

			// the NIC field being validated...
			var value = context.get("value");
			
			if (value === "") {
				// get the id of the form...
				var formId = context.get("formId");

				if (context.get("id") === (formId + "_externalReplicationInterface")) {
					// external replication nic can be empty while the other 3 are not
					return true;
				}
				
				// get values for all four NICs on dialog
				var localRepNic = registry.byId(formId + "_localReplicationInterface").get("value");
				var localDisNic = registry.byId(formId + "_localDiscoveryInterface").get("value");				
				var remoteDisNic = registry.byId(formId + "_remoteDiscoveryInterface").get("value");
				// external nic shouldn't be set if the other 3 aren't set
				var externalRepNic = registry.byId(formId + "_form_externalReplicationInterface").get("value");

				// if the others are empty current is valid
				if (localRepNic === "" && localDisNic === "" && remoteDisNic === "" && externalRepNic === "") {
					return true;
				} else {
					// at least one other NIC field has a value 
					context.set("invalidMessage", nls.appliance.highAvailability.dialog.nicsInvalid);
					return false;
				}
			
			} else {
				context.set("invalidMessage", nls.appliance.highAvailability.ipAddrValidation);
				// current field has a value validate against pattern
				var pattern = context.get('pattern');
				return new RegExp("^(?:"+pattern+")$").test(value);							
			}
			
			return false;
			
		},
		
		changeStartupMode: function() {
			// nothing to do here for now...
		}
		
		
	});

	var HASettingsAdvanced = declare("ism.controller.content.HASettingsAdvanced", [ToggleContainer], {

		startsOpen: true,
		contentId: "hASettingsAdvanced",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.appliance.highAvailability.dialog.advancedSettings;
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId, false);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
							"<span class='tagline'>" + nls.appliance.highAvailability.dialog.advSettingsTagline + "</span><br />",
							"<div id='"+this.contentId+"'></div>"
							].join("\n");
			domConstruct.place(nodeText, this.toggleNode);
			// formId should be the dialogId for the HA settings dialog
			this.hAFormAdvanced = new HAFormAdvanced({formId: this.formId}, this.contentId);
			this.hAFormAdvanced.startup();			
		},
		
		_toggleAdvanced: function() {
			
			if (this.hAFormAdvanced.field_StartupMode.get("value") === "StandAlone") {
				this.set('open', true);
				return;
			}
			
			if (this.hAFormAdvanced.field_PreferredPrimary.get("checked")) {
				this.set('open', true);
				return;
			}
			
			if (this.hAFormAdvanced.field_UseSecuredConnections.get("checked")) {
				this.set('open', true);
				return;
			}
			
			if (this.hAFormAdvanced.field_ReplicationPort.get("value") !== "0") {
				this.set('open', true);
				return;
			}
			
			if (this.hAFormAdvanced.field_ExternalReplicationPort.get("value") !== "0") {
				this.set('open', true);
				return;
			}
			
			if (this.hAFormAdvanced.field_RemoteDiscoveryPort.get("value") !== "0") {
				this.set('open', true);
				return;
			}
			
		}
		
	});

	return HASettingsAdvanced;
});
