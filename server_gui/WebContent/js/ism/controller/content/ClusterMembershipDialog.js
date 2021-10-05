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
        'dojo/_base/xhr',
        'dojo/_base/json',
        'dojo/dom-construct',
        'dojo/number',
    	'dojo/query',
    	'dijit/registry',
        'dijit/_Widget',
        'dijit/_TemplatedMixin',
        'dijit/_WidgetsInTemplateMixin',	
        'dijit/form/Button',
        'idx/form/CheckBox',
        'ism/widgets/TextBox',
        'ism/config/Resources',
        'ism/widgets/ToggleContainer',
        'ism/IsmUtils',
        'dojo/i18n!ism/nls/clusterMembership',
        'dojo/i18n!ism/nls/messaging',
        'dojo/text!ism/controller/content/templates/ClusterMembershipDialog.html'     
        ], function(declare, lang, xhr, json, domConstruct, number, query,  registry, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, Button, CheckBox, TextBox, Resources, ToggleContainer, Utils, nls, nlsm, template) {

	var ClusterMembershipDialog = declare("ism.controller.content.ClusterMembershipDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		nlsm: nlsm,
		templateString: template,
        restURI: Utils.getBaseUri() + "rest/config/cluster/clusterMembership/", 
        domainUid: 0,
		triggerId: "clusterMembershipAdvancedSettings_trigger",
		startsOpen: false,
		isMember: false,
		parent: undefined,
		defaultControlPort: undefined,
		defaultMessagingPort: undefined,
		defaultDiscoveryPort: undefined,
		_startingValues: undefined,
		haStandby: false,
		
		_memberInstruction: Utils.escapeApostrophe(nls.editDialogInstruction_Member),
		_notAMemberInstruction: Utils.escapeApostrophe(nls.editDialogInstruction_NotAMember),
		

		// template strings
		dialogTitle: Utils.escapeApostrophe(nls.editDialogTitle),		
		dialogInstruction: Utils.escapeApostrophe(nls.editDialogInstruction_NotAMember),
		clusterNameLabel: Utils.escapeApostrophe(nls.clusterNameLabel),
		controlAddressLabel: Utils.escapeApostrophe(nls.controlAddressLabel),
		advancedTitle: Utils.escapeApostrophe(nls.advancedSettingsLabel),
		
        useMulticastDiscoveryLabel: Utils.escapeApostrophe(nls.useMulticastDiscoveryLabel),
        discoveryPortLabel: Utils.escapeApostrophe(nls.discoveryPortLabel),
        discoveryTimeLabel: Utils.escapeApostrophe(nls.discoveryTimeLabel),
        withMembersLabel: Utils.escapeApostrophe(nls.withMembersLabel),
        multicastTTLLabel: Utils.escapeApostrophe(nls.multicastTTLLabel),
        addressLabel: Utils.escapeApostrophe(nls.addressLabel),
        portLabel: Utils.escapeApostrophe(nls.portLabel),
        externalAddressLabel: Utils.escapeApostrophe(nls.externalAddressLabel),
        externalPortLabel: Utils.escapeApostrophe(nls.externalPortLabel),
        useTLSLabel: Utils.escapeApostrophe(nls.useTLSLabel),
        clusterNameTooltip: Utils.escapeApostrophe(nls.clusterNameTooltip),
        controlAddressTooltip: Utils.escapeApostrophe(nls.controlAddressTooltip),
        membersTooltip: Utils.escapeApostrophe(nls.membersTooltip),
        useMulticastTooltip: Utils.escapeApostrophe(nls.useMulticastTooltip),
        discoveryPortTooltip: Utils.escapeApostrophe(nls.discoveryPortTooltip),
        discoveryTimeTooltip: Utils.escapeApostrophe(nls.discoveryTimeTooltip),
        multicastTTLTooltip: Utils.escapeApostrophe(nls.multicastTTLTooltip),
        controlAddressTooltip: Utils.escapeApostrophe(nls.controlAddressTooltip),
        controlPortTooltip: Utils.escapeApostrophe(nls.controlPortTooltip),
        controlExternalAddressTooltip: Utils.escapeApostrophe(nls.controlExternalAddressTooltip),
        controlExternalPortTooltip: Utils.escapeApostrophe(nls.controlExternalPortTooltip),
        messagingAddressTooltip: Utils.escapeApostrophe(nls.messagingAddressTooltip),
        messagingPortTooltip: Utils.escapeApostrophe(nls.messagingPortTooltip),
        messagingExternalAddressTooltip: Utils.escapeApostrophe(nls.messagingExternalAddressTooltip),
        messagingExternalPortTooltip: Utils.escapeApostrophe(nls.messagingExternalPortTooltip),
        controlInterfaceSectionTooltip: Utils.escapeApostrophe(nls.controlInterfaceSectionTooltip),
        messagingInterfaceSectionTooltip: Utils.escapeApostrophe(nls.messagingInterfaceSectionTooltip),
        addressInvalid: Utils.escapeApostrophe(nls.addressInvalid),
        portInvalid: Utils.escapeApostrophe(nls.portInvalid),
        enableLabel: Utils.escapeApostrophe(nlsm.messaging.endpoints.form.enabled),
       
        
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
			this.startsOpen = ism.user.getSectionOpen(this.triggerId, false);
			this.defaultControlPort = this.parent.get('defaultControlPort');
			this.defaultDiscoveryPort = this.parent.get('defaultDiscoveryPort');
			this.defaultMessagingPort = this.parent.get('defaultMessagingPort');
		},

		startup: function(args) {
			this.inherited(arguments);
		},
		
		postCreate: function() {		
		    console.debug(this.declaredClass + ".postCreate()");			
		    this.domainUid = ism.user.getDomainUid();
		    // Add the domain Uid to the REST API.
		    this.restURI += this.domainUid;
            this.advancedSection.watch('open', lang.hitch(this, function(open, oldValue, newValue){
                if (newValue) {
                    // delay the resize because the #@$! thing isn't actually visible yet
                    window.setTimeout(lang.hitch(this, function(){this.dialog.resize();}), 100);
                }
            }));
            
            
            this.field_ControlPort.isValid = function(focused) {
                return Utils.portValidator(this.get("value"), true);
            };
            
            this.field_ControlExternalPort.isValid = function(focused) {
                return Utils.portValidator(this.get("value"), false);
            };

            this.field_MessagingPort.isValid = function(focused) {
                return Utils.portValidator(this.get("value"), true);
            };
            
            this.field_MessagingExternalPort.isValid = function(focused) {
                return Utils.portValidator(this.get("value"), false);
            };

            this.field_DiscoveryPort.isValid = function(focused) {
                return Utils.portValidator(this.get("value"), !this.get('disabled'));
            };
            
		},
		
		show: function(/*ClusterMembership*/ cm, status, haInfo) {
			this.haInfo = haInfo;
            if (haInfo.standby === true || haInfo.primary === true) {
                this.enabledBlock.style.display = 'inline-block';
            }
		    query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
		    this.clusterMembership = cm;
		    this.status = status;
            this.form.reset();
            this._fillValues(cm);                            
            this._updateMemberStatus(status);
            if (haInfo.standby) {
            	this.field_ClusterName.set('required', false);
            	this.field_DiscoveryServerList.set('required', false);
            	this.field_DiscoveryServerList.set('disabled', true);
            	this.field_EnableClusterMembership.set('disabled', true);
            }
            this.field_ClusterName.set('disabled', this.isMember || haInfo.standby);
            this.dialog.clearErrorMessage();
            this.dialog.show();
            this.field_ClusterName.focus();
            this.clusterMembershipStatus.scrollTop = 0;            
		},
		
		hide: function() {
		    this.dialog.hide();
            var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
		    bar.innerHTML = "";
		},
		
		getValues: function() {
            var values = {"EnableClusterMembership" : this.clusterMembership.EnableClusterMembership};
            // start with our fields, _startingValues doesn't always have non-required values in it!
            for (var prop in this) {
                if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_" && !this[prop].get('disabled')) {
                    var actualProp = prop.substring(6);
                    values[actualProp] = this._getFieldValue(actualProp);
                }
            }
            return values;
		},
		
		save: function(onFinish, onError) {
            var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
            bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;

            var dialog = this.dialog;
            if (!onFinish) { onFinish = function() { 
                dialog.hide();
                bar.innerHTML = "";
                }; 
            }
            if (!onError) { onError = function(error) { 
                    bar.innerHTML = "";
                    dialog.showErrorFromPromise(nls.saveClusterMembershipError, error);
                };
            }

            var values = this.getValues();
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("ClusterMembership", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(
                function(data) {
                    onFinish(values);
                },
                onError
            );            
		},
		
		changeUseMulticastDiscovery: function(checked) {
            this.field_DiscoveryPort.set('disabled', !checked);           
		    this.field_MulticastDiscoveryTTL.set('disabled', !checked);		    
		    if (!this.haInfo.standby) {
			    this.field_DiscoveryServerList.setRequiredOnOff(!checked);
			    if (checked) {
			        this.field_DiscoveryServerList.validate(); // if no longer required, make sure to clear validation
			    }
		    }
		},
		
		leave: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();
		    var _this = this;

		    _this.parent.confirmLeave(function() {
		        _this._updateMemberStatus(_this.status, false);                
		    });
		},

		
		_updateMemberStatus: function(status, enabledState) {
            this.isMember = status && status.member ? status.member : false;
            this.dialog.set('instruction', this.isMember ? lang.replace(this._memberInstruction, [this._getFieldValue("ClusterName")]) : this._notAMemberInstruction);
            if (status && status.membership) {
                this.clusterMembershipStatus.innerHTML = this.isMember ? this._getFieldValue("ClusterName") : status.membership;
            }
            if (this.isMember && !this.haInfo.standby) {
                this.leaveLink.style.display = 'inline';                   
            } else {
                this.leaveLink.style.display = 'none';                   
                if (enabledState !== undefined) {
                	this.field_EnableClusterMembership.checked = enabledState;
                }
            }
            this.field_ClusterName.set('disabled', this.isMember);            
		},
		
	    // populate the dialog with the passed-in values
        _fillValues: function(values) {
            this._startingValues = values;
            for (var prop in values) {
                this._setFieldValue(prop, values[prop]);
            }
        },
		
        // define any special rules for getting a field value (e.g. for checkboxes)
        _getFieldValue: function(prop) {  
            var value;
            var fieldName = "field_"+prop;
            var field = this[fieldName] ? this[fieldName] : this.advancedSection[fieldName];
            switch (prop) {
                case "ControlPort":
                case "ControlExternalPort":
                case "DiscoveryPort":
                case "MessagingPort":
                case "MessagingExternalPort":
                case "MulticastDiscoveryTTL":
                case "DiscoveryTime":
                    value = number.parse(field.get("value"));                    
                    break;  
                case "UseMulticastDiscovery":
                case "MessagingUseTLS":
                case "EnableClusterMembership":
                    value = field.checked;
                    if (value == "off") { value = false; }
                    if (value == "on") { value = true; }
                    break;
                default:
                    value = field.get("value");
                    break;
            }
             return value;
        },
        // set a field value according to special rules (prefixes, checkbox on/off, etc.)
        _setFieldValue: function(prop, value) {
            var field = this["field_"+prop];
            if (field === undefined || field === null) {
                return;
            }
            switch (prop) {
            case "UseMulticastDiscovery":
            case "MessagingUseTLS":
            case "EnableClusterMembership":
                if (value === true || value === false) {
                    field.set("checked", value);                      
                } else if (value.toString().toLowerCase() == "true") {
                    field.set("checked", true);
                } else {
                    field.set("checked", false);
                }
                break;
            default:
                value = field.set("value", value);
            break;
            }
        }
		
	});
	
	return ClusterMembershipDialog;
});
