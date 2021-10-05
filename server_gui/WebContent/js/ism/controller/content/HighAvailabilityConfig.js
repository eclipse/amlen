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
		'dojo/aspect',
		'dojo/_base/json',
		'dojo/dom-construct',
		'dojo/query',
		'dojo/number',
		'dojo/dom',
		'dojo/on',
		'dojo/dom-style',
		"dijit/registry",
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'ism/widgets/ExternalString',
		'ism/widgets/TextBox',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/widgets/NoticeIcon',
		'ism/widgets/Dialog',
		'ism/controller/content/HASettingsAdvanced',
		'idx/form/RadioButtonSet',
		'idx/string',
		'ism/widgets/_TemplatedContent',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/home',
		'dojo/i18n!ism/nls/strings',
		'dojo/text!ism/controller/content/templates/HighAvailabilityConfig.html',
		'dojo/text!ism/controller/content/templates/HighAvailabilityConfigDialog.html'
		], function(declare, lang, aspect, json, domConstruct, query, number, dom, on, domStyle, registry, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, ExternalString, TextBox, Resources, Utils, NoticeIcon, Dialog,
				HASettingsAdvanced, RadioButtonSet, iString, _TemplatedContent, nls, nlsHome, nlsStrings, haConfig, haConfigDialog) {
	
	var HighAvailabilityConfigDialog = declare("ism.controller.content.HighAvailabilityConfigDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to editing the HA configuration
		// 
		// description:
		//	
		nls: nls,
		templateString: haConfigDialog,
		advForm: null,
		currentRole: null,
		currentGroup: null,
		activeNodes: null,
		restURI: "rest/config/appliance/highavailability/", 
		resized: false,
		//allowSingleNIC: false,
		domainUid: 0,
		
		// template strings
		title: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.title),
		instruction: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.instruction),
		groupNameLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.groupNameLabel),
		groupNameToolTip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.groupNameToolTip),
		localReplicationInterfaceLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localReplicationInterface),
		localReplicationInterfaceTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localReplicationInterfaceTooltip),
		localDiscoveryInterfaceLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localDiscoveryInterface),
		localDiscoveryInterfaceTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.localDiscoveryInterfaceTooltip),
		remoteDiscoveryInterfaceLabel: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.remoteDiscoveryInterface),
		remoteDiscoveryInterfaceTooltip: Utils.escapeApostrophe(nls.appliance.highAvailability.dialog.remoteDiscoveryInterfaceTooltip),
		ipAddrInvalid: Utils.escapeApostrophe(nls.appliance.highAvailability.ipAddrValidation),
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");

			this.domainUid = ism.user.getDomainUid();
			// Add the domain Uid to the REST API.
			this.restURI += this.domainUid + "/";

			this.advForm = this.advToggle.hAFormAdvanced;
			var haDialogWidget = this;

			// add validators for NIC fields - all must be set or none
			this.field_LocalReplicationNIC.isValid = this.advForm.nicValidator;
			this.field_RemoteDiscoveryNIC.isValid = this.advForm.nicValidator;
			//this.field_LocalDiscoveryNIC.isValid = this.advForm.nicValidator;
			   	
//			// Keep validator for local discovery for now in case we get a rest api for AllowSingleNIC
//			this.field_LocalDiscoveryNIC.isValid = function(focused){
//                var discAddress = this.get("value");                
//                // check empty addresses and patterns
//                var valid = haDialogWidget.advForm.nicValidator(focused, this);
//                // now check to see if the local discovery NIC is the same as the local Replication NIC
//                if (valid && discAddress !== "" && !haDialogWidget.allowSingleNIC) {
//                    if (haDialogWidget.field_LocalReplicationNIC.get("value") === discAddress) {
//                        this.invalidMessage = nls.appliance.highAvailability.discoverySameAsReplicationError;
//                        return false;
//                    }
//                }
//                return valid;
//            };
            
			this.field_EnableHA.on("change", lang.hitch(this, function(isChecked){
				if(isChecked){
					
					this.field_Group.setRequiredOnOff(true);
					// if HA is going to be enabled group name, LocalReplicationNIC, LocalDiscoveryNIC,  and RemoteDiscoveryNIC must not be empty...
					this.field_Group.isValid = function(focused) {
						var value = this.get("value");
						value = iString.nullTrim(value);
						if (!value) {
							return false;
						}
						if (value.length > 0) {
							return true;
						}
					};
					this.field_LocalReplicationNIC.setRequiredOnOff(true);
					this.field_LocalReplicationNIC.isValid = function(focused) {
						var value = this.get("value");
						value = iString.nullTrim(value);
						if (!value) {
							return false;
						}
						if (value.length > 0) {
							return true;
						}
					};
					this.field_LocalDiscoveryNIC.setRequiredOnOff(true);
					this.field_LocalDiscoveryNIC.isValid = function(focused) {
						var value = this.get("value");
						value = iString.nullTrim(value);
						if (!value) {
							return false;
						}
						if (value.length > 0) {
							return true;
						}
					};
					this.field_RemoteDiscoveryNIC.setRequiredOnOff(true);										
					this.field_RemoteDiscoveryNIC.isValid = function(focused) {
						var value = this.get("value");
						value = iString.nullTrim(value);
						if (!value) {
							return false;
						}
						if (value.length > 0) {
							return true;
						}
					};
				} else {
	
					this.field_Group.setRequiredOnOff(false);
					this.field_LocalReplicationNIC.setRequiredOnOff(false);
					this.field_LocalDiscoveryNIC.setRequiredOnOff(false);
					this.field_RemoteDiscoveryNIC.setRequiredOnOff(false);
					// group, LocalReplicationNIC, LocalDiscoveryNIC and RemoteDiscoveryNIC can only be empty if HA is disabled or will be disabled 
					this.field_Group.isValid = function(focused) {
						return true;
					};
					this.field_LocalReplicationNIC.isValid = function(focused) {
						return true;
					};
					this.field_LocalDiscoveryNIC.isValid = function(focused) {
						return true;
					};
					this.field_RemoteDiscoveryNIC.isValid = function(focused) {
						return true;
					};
					
					this.field_Group.validate();
					this.field_LocalReplicationNIC.validate();
					this.field_LocalDiscoveryNIC.validate();
					this.field_RemoteDiscoveryNIC.validate();
				}

			}));
			
			
			// reset the form to default values after it is closed...
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				dijit.byId(this.dialogId+"_form").reset();
			}));
			
			this.advToggle.watch('open', lang.hitch(this, function(open, oldValue, newValue){
				if (newValue && !this.resized) {
					this.resized = true;
					// delay the resize because the #@$! thing isn't actually visible yet
					window.setTimeout(lang.hitch(this, function(){this.dialog.resize();}), 100);
				}
			}));
		},
		
		
	
		

		// populate the dialog with the passed-in values
		fillValues: function(haConfigObj, haStatusObj) {
			
			// AllowSingleNIC is not provided by the REST API
			//this.allowSingleNIC = haConfigObj.AllowSingleNIC ? true : false;
			//this.advForm.set('allowSingleNIC', this.allowSingleNIC);
			
			this.currentRole = haStatusObj.NewRole;
			this.activeNodes = haStatusObj.ActiveNodes;
			this.currentConfig = haConfigObj;			
			
			this.field_EnableHA.set("checked", haConfigObj.EnableHA);
			this.currentGroup = (!haConfigObj.Group) ? "" : haConfigObj.Group;
			this.field_Group.set("value", this.currentGroup);
			
			// if standby node or start up StandAlone
			if (haStatusObj.NewRole === "STANDBY"  && haConfigObj.EnableHA === true) {
				this.field_Group.set("disabled", true);
			} else {
				this.field_Group.set("disabled", false);
			}		
						
			this.advForm.field_StartupMode.set("value",haConfigObj.StartupMode);
			this.advForm.field_PreferredPrimary.set("checked",haConfigObj.PreferredPrimary);
			this.advForm.field_UseSecuredConnections.set("checked",haConfigObj.UseSecuredConnections);
			
			var localRepNic =(!haConfigObj.LocalReplicationNIC) ? "" : haConfigObj.LocalReplicationNIC;
			this.field_LocalReplicationNIC.set("value", localRepNic);
			
			var localDisNic = (!haConfigObj.LocalDiscoveryNIC) ? "" : haConfigObj.LocalDiscoveryNIC;
			this.field_LocalDiscoveryNIC.set("value", localDisNic);
						
			var remoteDiscNic = (!haConfigObj.RemoteDiscoveryNIC) ? "" : haConfigObj.RemoteDiscoveryNIC;
			this.field_RemoteDiscoveryNIC.set("value",remoteDiscNic );
			
			if (haConfigObj.DiscoveryTimeout !== undefined) {
				this.advForm.field_DiscoveryTimeout.set("value",haConfigObj.DiscoveryTimeout );
			}

			if (haConfigObj.HeartbeatTimeout !== undefined) {
				this.advForm.field_HeartbeatTimeout.set("value",haConfigObj.HeartbeatTimeout );
			}
			
			if (haConfigObj.ReplicationPort !== undefined) {
				this.advForm.field_ReplicationPort.set("value",haConfigObj.ReplicationPort );
			}
			
			if (haConfigObj.ExternalReplicationPort !== undefined) {
				this.advForm.field_ExternalReplicationPort.set("value",haConfigObj.ExternalReplicationPort );
			}
			
			if (haConfigObj.RemoteDiscoveryPort !== undefined) {
				this.advForm.field_RemoteDiscoveryPort.set("value",haConfigObj.RemoteDiscoveryPort );
			}
			
			var externalReplicationNic = (!haConfigObj.ExternalReplicationNIC) ? "" : haConfigObj.ExternalReplicationNIC;
			this.advForm.field_ExternalReplicationNIC.set("value",externalReplicationNic);

		},
		
		show: function() {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.advToggle._toggleAdvanced();
			this.resized = this.advToggle.get('open'); // if its open on show, we don't need to resize anything
			this.dialog.show();
	
		},
	
		confirmGroupName: function() {
		
			// if active nodes=1 and the group name has been changed
			
			if (this.activeNodes === "1" && this.field_Group.get("value") !== this.currentGroup) {
				return true;
			}
			
			return false;
			
		},
		
		// check to see if anything changed, and save the record if so
		save: function(onFinish, onError) {
			
			// values - object used to put HA configuration
			var values = {};
			
			// if user cleared the group name with HA disabled...
			if (!this.field_Group.get("value")) {
				this.field_Group.set("value", "");
			}
			
			// get all of field values from the dialog's base template 
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(this, actualProp);
				}
			}
			
			// get all of the field values from the advanced toggle template
			for (prop in this.advForm) {
				if (this.advForm.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(this.advForm, actualProp);
				}
			}

			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
			
			// if no onFinish just close with success
			if (!onFinish) { onFinish = lang.hitch(this,function() {	
				this.dialog.hide();
				});
			}
			
			if (!onError) { 
				onError = lang.hitch(this,function(error, ioargs) {	
					bar.innerHTML = "";
					this.dialog.showErrorFromPromise(nls.appliance.highAvailability.saveFailed, error);
				});
			}
			
			var harole = this.currentRole;
			
			var restartRequired = false;
			// if HA is not disabled, or it is disabled but the user wants to enable it, we might need to restart the server.
			// Note that if HA is currently in disabled state, harole will be undefined.			
			if ((harole && harole != "HADISABLED") || values.EnableHA === true) {
				// if the following fields changed values, then we need to inform the user that a restart is required
				var changeRequiresRestart = ["EnableHA","RemoteDiscoveryNIC","LocalReplicationNIC","LocalDiscoveryNIC","DiscoveryTimeout","HeartbeatTimeout","ReplicationPort","RemoteDiscoveryPort","ExternalReplicationPort","ExternalReplicationNIC","UseSecuredConnections"];
				for (var i=0, len=changeRequiresRestart.length; i < len; i++) {
					var field = changeRequiresRestart[i];
					if (values[field] != this.currentConfig[field]) {
						// is it an empty string vs. no defined value?
						if (values[field] === "" && !this.currentConfig[field]) {
							continue;
						}
						restartRequired = true;
						break;
					}
				}
			}
						
			// send the configuration to the server
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("HighAvailability", values);
			var promise = adminEndpoint.doRequest("/configuration",options);
			promise.then(
				function(data) {
					onFinish(values, restartRequired);
				},
				onError
			);
			
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(formObj, prop) {
			
			var value;
			switch (prop) {
				case "PreferredPrimary":
				case "UseSecuredConnections":
				case "EnableHA":
					value = formObj["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "ReplicationPort":
				case "ExternalReplicationPort":
				case "RemoteDiscoveryPort":
				case "DiscoveryTimeout":
				case "HeartbeatTimeout":
					value = number.parse(formObj["field_"+prop].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
					break;
				default:
					if (formObj["field_"+prop].get) {
						value = formObj["field_"+prop].get("value");
					} else {
						value = formObj["field_"+prop].value;
					}
					break;
			}
			//console.debug("returning field_"+prop+" as '"+value+"'");
			return value;
			
		},
		
		securityRiskExists: function() {
			if (this._getFieldValue(this, "EnableHA") === false) {
				return false;
			}
			return this.field_RemoteDiscoveryNIC.get('value') === "" || 
				this.field_LocalDiscoveryNIC.get('value') === "" || 
				this.field_LocalReplicationNIC.get('value') === ""; 
		}
		
	});

	var HighAvailabilityConfig = declare("ism.controller.content.HighAvailabilityConfig",  [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		
		// 
		// description:
		//	
		nls: nls,
		nlsHome: nlsHome,
		nlsStrings: nlsStrings,
		templateString: haConfig,
		editDialog: null,
		restartInfoDialog: null,
		groupConfirmDialog: null,
		securityRiskDialog: null,
		advForm: null,
		haConfigObj: null,
		haStatusObj: null,
		contentId: null,
		restURI: "rest/config/appliance/highavailability/", 
		//ethRestURI: "rest/config/appliance/ethernet-interfaces/",
		haStatusRestURI: "rest/config/appliance/highavailability/role",
		domainUid: 0,
	
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
	
		},

		postCreate: function(args) {
			
			console.debug(this.declaredClass + ".startup()");
			this.inherited(arguments);
			this.domainUid = ism.user.getDomainUid();
			// Add the domain Uid to the REST API.
			this.restURI += this.domainUid + "/";
			this.haStatusRestURI += "/" + this.domainUid;
			
			// initialize by getting the configuration and ha role
			this._initStatusAndConfig();
			this._initDialogs();
			this._initEvents();
			
			this.advForm = this.editDialog.advToggle.hAFormAdvanced;
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.appliance.nav.highAvailability.topic, lang.hitch(this, function(message){
				this._initStatusAndConfig();
			}));
			
			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				if (!this.haStatusObj || !data.HighAvailability || (this.haStatusObj.NewRole === data.HighAvailability.NewRole)) {
					this._updateHaStatus(data);
				} else {
					this._initStatusAndConfig();
				}
			}));

		},
		
		_initStatusAndConfig: function() {
			console.debug(this.declaredClass + "._initStatusAndConfig()");

			// first get the ha configuration then the ha role
			this._getHaConfig(lang.hitch(this, function() {
				this._getHaRole();
			}));
			
		},
		
		_initDialogs: function() {
			
			var editId = "edit"+this.id+"Dialog";
			var editDialogNode = editId + "Node";
			domConstruct.place("<div id='"+editDialogNode+"'></div>", this.id);
			
			this.editDialog = new HighAvailabilityConfigDialog({dialogId: editId}, editDialogNode);
			this.editDialog.startup();
			
			// create restart info dialog
			domConstruct.place("<div id='restartInfo"+this.id+"Dialog'></div>", this.id);
			var restartDialogId = "restartInfo"+this.id+"Dialog";
			var restartTitle = nls.appliance.highAvailability.restartInfoDialog.title;
			this.restartInfoDialog = new Dialog({
				id: restartDialogId,
				title: restartTitle,
				content: "<div>" + nls.appliance.highAvailability.restartInfoDialog.content + "</div>",
				closeButtonLabel: nls.appliance.highAvailability.restartInfoDialog.closeButton,
				buttons: [
					new Button({
						id: restartDialogId + "_button_restart",
						label: nls.appliance.highAvailability.restartInfoDialog.restartButton,
						onClick: lang.hitch(this, function() {
							this.restartInfoDialog.restartButton.set('disabled', true);
							this._restart();
						})
					})
				],
				style: 'width: 600px;'
			}, restartDialogId);
			this.restartInfoDialog.dialogId = restartDialogId;
			this.restartInfoDialog.restartButton = dijit.byId(restartDialogId + "_button_restart");
			
			
			// create group name confirmation dialog
			domConstruct.place("<div id='groupConfirm"+this.id+"Dialog'></div>", this.id);
			var groupConfirmDialogId = "groupConfirm"+this.id+"Dialog";
			var confirmTitle = nls.appliance.highAvailability.confirmGroupNameDialog.title;
			this.groupConfirmDialog = new Dialog({
				id: groupConfirmDialogId,
				title: confirmTitle,
				content: "<div>" + nls.appliance.highAvailability.confirmGroupNameDialog.content + "</div>",
				closeButtonLabel: nls.appliance.highAvailability.confirmGroupNameDialog.closeButton,
				buttons: [
					new Button({
						id: groupConfirmDialogId + "_button_confirm",
						label: nls.appliance.highAvailability.confirmGroupNameDialog.confirmButton,
						onClick: lang.hitch(this, function() {
							this.groupConfirmDialog.hide();
							dojo.publish(groupConfirmDialogId + "_button_confirm", "");
						})
					})
				],
				style: 'width: 600px;'
			}, groupConfirmDialogId);
			this.groupConfirmDialog.dialogId = groupConfirmDialogId;
			this.groupConfirmDialog.startup();
			
			// create security risk confirmation dialog
			domConstruct.place("<div id='securityRisk"+this.id+"Dialog'></div>", this.id);
			var securityRiskDialogId = "securityRisk"+this.id+"Dialog";
			confirmTitle = nls.appliance.highAvailability.dialog.securityRiskTitle;
			this.securityRiskDialog = new Dialog({
				id: securityRiskDialogId,
				title: confirmTitle,
				content: "<div>" + nls.appliance.highAvailability.dialog.securityRiskText + "</div>",
				closeButtonLabel: nls.appliance.highAvailability.confirmGroupNameDialog.closeButton,
				buttons: [
					new Button({
						id: securityRiskDialogId + "_button_confirm",
						label: nls.appliance.highAvailability.confirmGroupNameDialog.confirmButton,
						onClick: lang.hitch(this, function() {
							this.securityRiskDialog.hide();
							dojo.publish(securityRiskDialogId + "_button_confirm", "");
						})
					})
				],
				style: 'width: 600px;'
			}, securityRiskDialogId);
			this.securityRiskDialog.dialogId = securityRiskDialogId;
			this.securityRiskDialog.startup();
			
		},
		
		_restart: function() {				
			var dialog = this.restartInfoDialog;
			var bar = query(".idxDialogActionBarLead",dialog.domNode)[0];
			var _this = this;
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.restarting;
			Utils.restartMessagingServer(true, function() {					  
				bar.innerHTML = "";
				dialog.hide();											
			}, function(error, ioargs) {
				_this._handleError(dialog, nls.restartingFailed, error, ioargs);
			});				 
		},
		
		_handleError: function(dialog, message, error, ioargs) {
			// refresh status
			Utils.updateStatus();
			// clean up
			query(".idxDialogActionBarLead",dialog.domNode)[0].innerHTML = "";
			dialog.hide();			
			// show error message
			dialog.showErrorFromPromise(message, error);
		},
		
		_saveConfiguration: function(data, restartRequired) {
			// refresh the information for the ha page
			this._initStatusAndConfig();					
			this.editDialog.dialog.hide();
			if (restartRequired) {
				this.restartInfoDialog.restartButton.set('disabled', false);
				query(".idxDialogActionBarLead",this.restartInfoDialog.domNode)[0].innerHTML="";
				this.restartInfoDialog.show();
			}
		},
			
		_initEvents: function() {
			
			
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				if (registry.byId(this.editDialog.dialogId + "_form").validate() && this.advForm.validateAdvancedForm()) {	
					if (this.editDialog.securityRiskExists()) {
						this.securityRiskDialog.show();
					} else {
						this._checkGroup();
					}
				}
			}));

			 dojo.subscribe(this.securityRiskDialog.dialogId + "_button_confirm", lang.hitch(this, function(message) {
					// user confirmed that group name change was OK - attempt to save...
					 this._checkGroup();
				}));
		
			dojo.subscribe(this.groupConfirmDialog.dialogId + "_button_confirm", lang.hitch(this, function(message) {
				// user confirmed that group name change was OK - attempt to save...
				this.editDialog.save(lang.hitch(this, "_saveConfiguration"));
			}));
			
		},
			
		_checkGroup: function() {
			if (this.editDialog.confirmGroupName()) {
				/*
				 * We have a case where we want confirmation about the group name change.
				 * This should happen with ActiveNodes=1 and the group name is changed.
				 */
				this.groupConfirmDialog.show();
			} else {
				// just do the save - no group name change confirmation needed.
				this.editDialog.save(lang.hitch(this, "_saveConfiguration"));
			}			   
		},	

		
		_getHaRole: function() {			
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultGetOptions();
			var promise = adminEndpoint.doRequest("/service/status", options);
			var _this = this;
			promise.then(
				function(data) {
					console.debug(data.HighAvailability);
					if (!data.HighAvailability) {
						Utils.showPageError(nls.appliance.highAvailability.error);
						return;
					}
					_this.haRoleObj = data.HighAvailability;
					_this.haStatusObj = data.HighAvailability;
					_this._initHaStatus(data.HighAvailability);
				},
				function(error) {
					Utils.showPageErrorFromPromise(nls.appliance.highAvailability.error, error);
				}
			);
		},
		
		_getHaConfig: function(onFinish) {
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultGetOptions();
			var promise = adminEndpoint.doRequest("/configuration/HighAvailability", options);
			var _this = this;
			promise.then(
				function(data) {
					console.debug(data.HighAvailability);
					if (!data.HighAvailability) {
						Utils.showPageError(nls.appliance.highAvailability.error);
						return;
					}
					_this.haConfigObj = data.HighAvailability;
					_this._populateHaConfig(data.HighAvailability);
					onFinish();
				},
				function(error) {
					Utils.showPageErrorFromPromise(nls.appliance.highAvailability.error, error);
				}
			);
			
		},
		
		_initHaStatus: function(statusData) {
			// summary:
			//		Set the status of the HA configuration based on a specific call
			//	  to get the HA role. This method should not be called with the
			//	  status object that is returned from the server status ping.
			// 
			// description:
			//		initialize the HA status section of the HA configuration page
			//	

			// make a call to determine the status string and what fileds should be displayed
			this._updateHaStatus(statusData);

			// the following data is only available with the actual server call to harole
			// TODO: getting these from haConfig now but not sure how that will go when doing autoconfig?
			var groupName = (!this.haConfigObj.Group) ? "" : this.haConfigObj.Group;
			dom.byId(this.id + "_haGroup").innerHTML = Utils.textDecorator(groupName);
			dom.byId(this.id + "_replicationInterface").innerHTML = this.haConfigObj.LocalReplicationNIC;
			dom.byId(this.id + "_externalReplicationInterface").innerHTML = this.haConfigObj.ExternalReplicationNIC;
			dom.byId(this.id + "_discoveryInterface").innerHTML = this.haConfigObj.LocalDiscoveryNIC;

			var remoteWebUI = statusData.RemoteWebUI;
			if (!remoteWebUI || remoteWebUI === "NotAvailable")  {
				remoteWebUI = "";
			} else  {
				remoteWebUI = lang.replace(nls.appliance.highAvailability.status.remoteWebUI, [remoteWebUI]);
			}
			
			dom.byId(this.id + "_remoteDiscoveryInterface").innerHTML = this.haConfigObj.RemoteDiscoveryNIC + "&nbsp;&nbsp;" + remoteWebUI;
			dom.byId(this.id + "_pairedWith").innerHTML = remoteWebUI;
			
		},
		
		_updateHaStatus: function(statusData) {
			// summary:
			//		Set the status of the HA configuration. This method may
			//	  be called with data returned from the server status ping or
			//	  an actual call to harole.
			//	  
			// description:
			//		Set the current HA status and hide/show appropriate fields based
			//	  on the current status.
			//	
			
			if (!statusData) {
				return;
			}
			
			var hastatus = statusData;
			
			var haRole = hastatus.NewRole ? hastatus.NewRole : hastatus.harole;
			if (!haRole) {
				haRole = hastatus.Enabled === false ? "HADISABLED" : undefined;
			}
			var niceStatus = nlsHome.home.role[haRole];
			if (!niceStatus) {
				niceStatus = nlsHome.home.role.unknown;
			}
			else if (Utils.isPrimarySynchronizing(haRole)) {
				niceStatus = lang.replace(niceStatus, [hastatus.PctSyncCompletion]);
			}				
			var reason = "";
			if (hastatus.ReasonCode && nlsHome.home.haReason[hastatus.ReasonCode]) {
				reason = nlsHome.home.haReason[hastatus.ReasonCode];
				if (hastatus.ReasonCode === "1" && hastatus.ReasonString) {
					reason = lang.replace(reason, [hastatus.ReasonString]);
				}
				if (hastatus.PrimaryLastTime && hastatus.PrimaryLastTime !== 0) {
					var formattedDate = Utils.getFormattedDateTime(new Date(hastatus.PrimaryLastTime));
					var lastPrimaryMessage = lang.replace(nlsHome.home.haReason.lastPrimaryTime, [formattedDate]);
					reason += " " + lastPrimaryMessage;
				}
				reason = ":&nbsp;&nbsp;<span style='font-weight: normal'>" + reason + "</span>";						
			}
			
			// set a pretty status string and reason...
			dom.byId(this.id + "_haStatus").innerHTML = niceStatus + reason;
			
			
			if (haRole === "HADISABLED") {
				 domStyle.set(this.id + "_haStatus", "display", "block");
				 domStyle.set(this.id + "_haStatusLabel", "display", "block");
				 
				 domStyle.set(this.id + "_haGroup", "display", "none");
				 domStyle.set(this.id + "_haGroupLabel", "display", "none");
				 
				 domStyle.set(this.id + "_replicationInterface", "display", "none");
				 domStyle.set(this.id + "_replicationInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_externalReplicationInterface", "display", "none");
				 domStyle.set(this.id + "_externalReplicationInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_discoveryInterface", "display", "none");
				 domStyle.set(this.id + "_discoveryInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_remoteDiscoveryInterface", "display", "none");
				 domStyle.set(this.id + "_remoteDiscoveryInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_pairedWith", "display", "none");
				 domStyle.set(this.id + "_pairedWithLabel", "display", "none");
			} else if (haRole === "UNSYNC_ERROR") {
				 domStyle.set(this.id + "_haStatus", "display", "block");
				 domStyle.set(this.id + "_haStatusLabel", "display", "block");
				 
				 domStyle.set(this.id + "_haGroup", "display", "block");
				 domStyle.set(this.id + "_haGroupLabel", "display", "block");
				 
				 domStyle.set(this.id + "_pairedWith", "display", "block");
				 domStyle.set(this.id + "_pairedWithLabel", "display", "block");
				 
				 domStyle.set(this.id + "_replicationInterface", "display", "none");
				 domStyle.set(this.id + "_replicationInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_externalReplicationInterface", "display", "none");
				 domStyle.set(this.id + "_externalReplicationInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_discoveryInterface", "display", "none");
				 domStyle.set(this.id + "_discoveryInterfaceLabel", "display", "none");
				 
				 domStyle.set(this.id + "_remoteDiscoveryInterface", "display", "none");
				 domStyle.set(this.id + "_remoteDiscoveryInterfaceLabel", "display", "none");

			} else {
				 domStyle.set(this.id + "_haStatus", "display", "block");
				 domStyle.set(this.id + "_haStatusLabel", "display", "block");
				 
				 domStyle.set(this.id + "_haGroup", "display", "block");
				 domStyle.set(this.id + "_haGroupLabel", "display", "block");
				 
				 domStyle.set(this.id + "_replicationInterface", "display", "block");
				 domStyle.set(this.id + "_replicationInterfaceLabel", "display", "block");
				 
				 domStyle.set(this.id + "_externalReplicationInterface", "display", "block");
				 domStyle.set(this.id + "_externalReplicationInterfaceLabel", "display", "block");
				 
				 domStyle.set(this.id + "_discoveryInterface", "display", "block");
				 domStyle.set(this.id + "_discoveryInterfaceLabel", "display", "block");
				 
				 domStyle.set(this.id + "_remoteDiscoveryInterface", "display", "block");
				 domStyle.set(this.id + "_remoteDiscoveryInterfaceLabel", "display", "block");
				 
				 domStyle.set(this.id + "_pairedWith", "display", "none");
				 domStyle.set(this.id + "_pairedWithLabel", "display", "none");
			}
			
		},
		
		_populateHaConfig: function(data) {
			
			if (!data.EnableHA) {
				dom.byId(this.id + "_config_haEnabled").innerHTML = nls.appliance.highAvailability.no;
				this.enabledIcon.set("iconClass", "ismIconUnchecked");
				this.enabledIcon.set("hoverHelpText", nls.appliance.highAvailability.status.disabled);
			} else {
				dom.byId(this.id + "_config_haEnabled").innerHTML = nls.appliance.highAvailability.yes;
				this.enabledIcon.set("iconClass", "ismIconChecked");
				this.enabledIcon.set("hoverHelpText", nls.appliance.highAvailability.status.enabled);
			}
			
			var group = (!data.Group) ? nls.appliance.highAvailability.notSet : data.Group;
			dom.byId(this.id + "_config_haGroup").innerHTML = group;

			var startupMode = nls.appliance.highAvailability[data.StartupMode];
			if (data.PreferredPrimary === true && data.StartupMode === "AutoDetect") {
				startupMode = nls.appliance.highAvailability.config.preferedPrimary;
			}
			dom.byId(this.id + "_config_startupMode").innerHTML = startupMode;
	
			var nicDefault = nls.appliance.highAvailability.AutoDetect;
			if (!data.Group) {
				nicDefault = nls.appliance.highAvailability.notSet;
			} 
			var localRepNic = (!data.LocalReplicationNIC) ? nicDefault :data.LocalReplicationNIC;
			dom.byId(this.id + "_config_replicationInterface").innerHTML = localRepNic;
			
			var localDisNic = (!data.LocalDiscoveryNIC) ? nicDefault :data.LocalDiscoveryNIC;
			dom.byId(this.id + "_config_discoveryInterface").innerHTML = localDisNic;
			
			var remoteDiscNic = (!data.RemoteDiscoveryNIC) ? nicDefault :data.RemoteDiscoveryNIC;
			dom.byId(this.id + "_config_remoteDiscoveryInterface").innerHTML = remoteDiscNic;
	
			var discoveryTimeout = lang.replace(nls.appliance.highAvailability.config.secondsDefault, [data.DiscoveryTimeout]);
			if (data.DiscoveryTimeout !== 600) {
				discoveryTimeout = lang.replace(nls.appliance.highAvailability.config.seconds, [data.DiscoveryTimeout]);
			}
			dom.byId(this.id + "_config_discoveryTimeout").innerHTML = discoveryTimeout;
			
			var heartbeatTimeout = lang.replace(nls.appliance.highAvailability.config.secondsDefault, [data.HeartbeatTimeout]);
			if (data.HeartbeatTimeout !== 10) {
				heartbeatTimeout = lang.replace(nls.appliance.highAvailability.config.seconds, [data.HeartbeatTimeout]);
			}
			dom.byId(this.id + "_config_heartbeat").innerHTML = heartbeatTimeout;
			
			var replicationPort = data.ReplicationPort;
			dom.byId(this.id + "_config_replicationPort").innerHTML = replicationPort;
			var externalReplicationPort = data.ExternalReplicationPort;
			dom.byId(this.id + "_config_externalReplicationPort").innerHTML = externalReplicationPort;
			var remoteDiscoveryPort = data.RemoteDiscoveryPort;
			dom.byId(this.id + "_config_remoteDiscoveryPort").innerHTML = remoteDiscoveryPort;
			
			var externalNicDefault = nls.appliance.highAvailability.notSet;
			var externalReplicationNic = (!data.ExternalReplicationNIC) ? externalNicDefault :data.ExternalReplicationNIC;
			dom.byId(this.id + "_config_externalReplicationInterface").innerHTML = externalReplicationNic;
			
			if (!data.UseSecuredConnections) {
				dom.byId(this.id + "_config_useSecuredConnections").innerHTML = nls.appliance.highAvailability.no;
				this.secureConnEnabledIcon.set("iconClass", "ismIconUnchecked");
				this.secureConnEnabledIcon.set("hoverHelpText", nls.haUseSecuredConnectionsDisabled);
			} else {
				dom.byId(this.id + "_config_useSecuredConnections").innerHTML = nls.appliance.highAvailability.yes;
				this.secureConnEnabledIcon.set("iconClass", "ismIconUnchecked");
				this.secureConnEnabledIcon.set("hoverHelpText", nls.haUseSecuredConnectionsEnabled);
			}
		},
		
		editConfig: function(event) {
			// work around for IE event issue - stop default event
			if (event && event.preventDefault) event.preventDefault();
			
			this.editDialog.fillValues(this.haConfigObj, this.haStatusObj);
			this.editDialog.show();
		}
		

	});
	
	return HighAvailabilityConfig;
});
