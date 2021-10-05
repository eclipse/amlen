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
		'dojo/_base/array',
		'dojo/query',
		'dojo/aspect',
		'dojo/on',
		'dojo/dom-style',
		'dojo/dom-construct',
		'dojo/number',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'idx/form/CheckBox',
		'idx/form/CheckBoxSelect',		
		'ism/widgets/CheckBoxList',
		'ism/widgets/RadioButtonSet',
		'ism/IsmUtils',		
		'dojo/text!ism/controller/content/templates/MessagingPolicyDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, query, aspect, on, domStyle, domConstruct, number, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, Dialog, CheckBox, CheckBoxSelect, CheckBoxList, RadioButtonSet, Utils, messagingPolicyDialog, nls, nls_strings) {

	/*
	 * This file defines a configuration widget that displays the list of message hubs
	 */
	
	var MessagingPolicyDialog = declare("ism.controller.content.MessagingPolicyDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add/Edit buttons in the MessagingPolicyList widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds functions to
		//		populate the dialog fields with a record (fillValues) and save the changes
		//		via POST.  For inputs that are more complex than a text box,
		//		_getFieldValue and _setFieldValue are defined to get/set a generic value for
		//		different input types (e.g. checkbox).
		nls: nls,
		nls_strings: nls_strings,
		templateString: messagingPolicyDialog,

		// template strings
		nameLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.name),
		nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.description),
        clientIPLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.clientIP),
        invalidClientIPMessage: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.invalid.clientIP),
        IDLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.ID),
        invalidWildcard: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.invalid.wildcard),
        commonNameLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.commonName),
        clientIDLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.clientID),
        groupLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.Group),
        protocolLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.protocol),
        protocolPlaceholder: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.selectProtocol),
        destinationTypeLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.destinationType),
        destinationTypeTooltip: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.tooltip.destinationType),
        actionLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.action),
        actionTooltip: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.tooltip.action.topic),
        maxMessagesLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.maxMessages),
        maxMessagesTooltip: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.tooltip.maxMessages),
        maxMessagesInvalid: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.invalid.maxMessages),
        maxMessagesBehaviorLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.maxMessagesBehavior),
        maxMessagesBehaviorTooltip: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.tooltip.maxMessagesBehavior),
        maxMessagesTTLLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.maxMessageTimeToLive),
        maxMessagesTTLTooltip: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.tooltip.maxMessageTimeToLive),
        maxMessagesTTLInvalid: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.invalid.maxMessageTimeToLive),
        unlimitedHintText: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.unlimited),
        unitLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.unit),    
        
        filters: ['ClientAddress', 'UserID', 'CommonNames', 'ClientID', 'GroupID', 'Protocol'],
		vars: ['${UserID}', '${GroupID}', '${ClientID}', '${CommonName}'],
		dynamicProperties: ['MaxMessages', 'MaxMessagesBehavior','DisconnectedClientNotification','MaxMessageTimeToLive'],
		dynamicPropertyLabels: {
				MaxMessages: nls.messaging.messagingPolicies.dialog.maxMessages,
			    MaxMessagesBehavior: nls.messaging.messagingPolicies.dialog.maxMessagesBehavior,
			    DisconnectedClientNotification: nls.messaging.messagingPolicies.dialog.disconnectedClientNotification,
			    MaxMessageTimeToLive: nls.messaging.messagingPolicies.dialog.maxMessageTimeToLive},
		settingsCache : null,
		protocolStore: null,
		protocolValidatorMsg: null,
		viewOnly: false,
		closeButtonLabel: nls.messaging.dialog.cancelButton,
		
		actionListOptions: {
			"Topic": [{label: nls.messaging.messagingPolicies.dialog.actionOptions.publish, value: "Publish" },
	                  {label: nls.messaging.messagingPolicies.dialog.actionOptions.subscribe, value: "Subscribe"}],
      		"Queue": [{label: nls.messaging.messagingPolicies.dialog.actionOptions.send, value: "Send" },
	    	           {label: nls.messaging.messagingPolicies.dialog.actionOptions.browse, value: "Browse"},
	    	           {label: nls.messaging.messagingPolicies.dialog.actionOptions.receive, value: "Receive" }],
	    	"Subscription": [{label: nls.messaging.messagingPolicies.dialog.actionOptions.receive, value: "Receive" },
	    		             {label: nls.messaging.messagingPolicies.dialog.actionOptions.control, value: "Control"}]	             
		},
		actionListTooltips: {
			"Topic" : nls.messaging.messagingPolicies.dialog.tooltip.action.topic,
			"Queue" : nls.messaging.messagingPolicies.dialog.tooltip.action.queue,
			"Subscription" : nls.messaging.messagingPolicies.dialog.tooltip.action.subscription
		},

		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},
		
		domainUid: 0,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		    this.closeButtonLabel = this.viewOnly === true ? nls.messaging.dialog.closeButton : nls.messaging.dialog.cancelButton; 
		    
		    // Set the destination and authority labels so they are based on the size of the (translated) label
		    // string.  Also attempt to align the input fields if the size required for the label does not prevent
		    // this.  This addresses several defects related to very long pseudo-translation screens that can cause 
		    // the hover help icon to go out of view or can cause the input fields to appear below their labels. 
		    // While there's still a chance that a field could appear below it's label, it's less likely now.
		    var dlabelWidth, alabelWidth;
		    if (this.destinationLabel.length < 20) {
		    	dlabelWidth = (this.destinationLabel.length * 7) + 35;
		    } else if (this.destinationLabel.length < 30) {
		    	dlabelWidth = (this.destinationLabel.length * 6) + 35;
		    } else {
	    		dlabelWidth = 215;
		    }
		    if (this.actionLabel.length < 20) {
		    	alabelWidth = (this.actionLabel.length * 7) + 35;
		    } else {
		    	alabelWidth = 175;
		    }
		    if (dlabelWidth - 11 > alabelWidth) {
		    	alabelWidth = dlabelWidth - 11;
		    } else if (alabelWidth + 11 > dlabelWidth) {
		    	dlabelWidth = alabelWidth + 11;
		    }
		    this.destinationLabelWidth = "" + dlabelWidth + "px";
		    this.actionLabelWidth = "" + alabelWidth + "px";
		},

		postCreate: function() {
            
            this.domainUid = ism.user.getDomainUid();
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;			
			
			switch (this.type) {
			case "Subscription":
				this.excludeProperties = ['DisconnectedClientNotification','MaxMessageTimeToLive'];
				break;
			case "Queue":
				this.excludeProperties = ['MaxMessages', 'MaxMessagesBehavior','DisconnectedClientNotification'];
				break;
			case "Topic":
			default:
				break;
			}
			
			var t = this;
			
			if (this.viewOnly === true) {
				// all fields should be read only
				for (var prop in this) {
					if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
						this[prop].set('readOnly', true);
					}
				}
				var len = this.filters ? this.filters.length : 0;
				for (var fi=0; fi < len; fi++) {				
					var filterCheckbox = this["checkbox_"+this.filters[fi]];
					if (filterCheckbox) {
						filterCheckbox.set('readOnly', true);
					}
				}
				// the save button should be hidden
				domStyle.set(dijit.byId(this.dialogId+"_saveButton").domNode, 'display', 'none');
				// The dialog onEnter should hide the dialog
				this.dialog.onEnter = function() {
					this.hide();
				};						
			} else if (this.add === true || Utils.configObjName.editEnabled()) {			
				// check that the userid matches the pattern and doesn't clash with an existing object
				this.field_Name.isValid = function(focused) {
					var value = this.get("value");
					var validate =  Utils.configObjName.validate(value, this.existingNames, focused);
					if (!validate.valid) {
						this.invalidMessage = validate.message;									
						return false;
					}
					return true;				
				};
			} else {
				this.field_Name.set('readOnly', true);	
			}
			
			this.field_MaxMessages.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			
			this.field_MaxMessages.isValid = function(focused) {
				var value = this.get("value");
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				if (isNaN(intValue) || intValue < 1 || intValue > 20000000) {
					return false;
				}
				return true;
			};
			
			// get store for protocols and initialize select widget
			this.protocolDeferrred.then(lang.hitch(this,function(data) {
				this.protocolStore = Utils.getProtocolStore(data);
				this.field_Protocol.addOption(this.protocolStore.query({}, {sort: [{attribute: "label"}]}));
			}));					
			
			// set up the onChange event handlers for filter check boxes
			// and validators for filters (if a check box is checked then
			// a value should be provided for a filter
			var ilen = this.filters ? this.filters.length : 0;			
			for (var i=0; i < ilen; i++) {				
				var filter = this.filters[i];
				var field = this["field_"+filter];
				field.defaultInvalidMessage = field.invalidMessage;
				var checkbox = this["checkbox_"+filter];
				if (checkbox && field) {
					checkbox.on("change", function(checked) {
						var dap = this.dojoAttachPoint;
						var prop = dap.substring(dap.indexOf("_")+1, dap.length); 
						t["field_"+prop].set("disabled", !checked);							
					}, true);	
					domStyle.set(checkbox.iconNode, "display", "none");
					// create a validator for the field
					field.isValid = function(focused) {
						// a disabled field is always valid
						if (this.get("disabled")) {
							return true;
						}
						var dap = this.dojoAttachPoint;
						var prop = dap.substring(dap.indexOf("_")+1, dap.length); 						
						var value = t._getFieldValue(prop);
						if (value !== null && value !== undefined && lang.trim(value) !== "") {
							// does it have a regex?
							var matches = true;
							var pattern = this.get('pattern');
							if (pattern != undefined && pattern !== "") {
								matches = new RegExp("^(?:"+pattern+")$").test(value);
							}
							if (matches) {
								// make sure it doesn't contain any of the variables
								var len = t.vars ? t.vars.length : 0;
								for (var vi=0; vi < len; vi++) {
									if (value.indexOf(t.vars[vi]) > -1) {
										matches = false;
										this.invalidMessage = nls.messaging.messagingPolicies.dialog.invalid.vars;
									}
								}
							} else {
								this.invalidMessage = this.defaultInvalidMessage;
							}
							return matches;
						}
						return false;							
					};
					field.getErrorMessage = function(focused) {
						var dap = this.dojoAttachPoint;
						var prop = dap.substring(dap.indexOf("_")+1, dap.length); 						
						var value = t._getFieldValue(prop);
						if (value !== null && value !== undefined && lang.trim(value) !== "") {
							return this.invalidMessage;
						} 
						return nls_strings.global.missingRequiredMessage;
					};
					// check invalidMessage
					if (!field.hasOwnProperty("invalidMessage")) {
						field.invalidMessage = nls_strings.global.missingRequiredMessage;
					}
				}
			}
			
			// validation for protocol	
			this.field_Protocol.isValid = lang.hitch(this,this._validateProtocol);
			this.protocolValidatorMsg = this.field_Protocol.invalidMessage;
			
			// set up change for protocol select
			t.field_Protocol.on("change", function(args) {
				t.field_Protocol.validate();
			}, true);
			
			// set up blur for protocol select
			t.field_Protocol.on("blur", function(args) {
				t.field_Protocol.validate();
			}, true);

			// set up onChange for description
			t.field_Description.on("change", function(args) {
				t.dialog.resize();
			}, true);
			
			aspect.after(this.dialog, "onHide", lang.hitch(this, function() {
				dijit.byId(this.dialogId +"_DialogForm").reset();
			}));
			
			this.field_Destination.isValid = function(focused) { 
				var type = t.type;
				var destValue = t.field_Destination.get('value');
				
				// if empty return false since it is required
				if (destValue.length <= 0) {
					return false;
				}
				// When destination type is subscription there can be no client id substitution 
				if (type === "Subscription"){
					t.field_Destination.invalidMessage = nls.messaging.messagingPolicies.dialog.invalid.subDestination;
					var valid = new RegExp("^(?!.*\\$\\{ClientID\\}).*$").test(destValue);
					return valid;
				} else {
					return true;
				}
			};
			
			this.field_MaxMessageTimeToLive.isValid = function(focused) {
				// valid values are empty, unlimited, or 1 - 2147483647
				var value = this.get("value");
				if (value === null || value === "") {
					return true;
				}						
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				if (isNaN(intValue)) {
				    var nlsUnlimited = nls.messaging.messagingPolicies.dialog.unlimited;
					if (value.toLowerCase() !== "unlimited" && value !== nlsUnlimited &&
					        value.toLowerCase() !== nlsUnlimited.toLowerCase()) {
						return false;
					}
				} else if (intValue < 1 || intValue > 2147483647) {
					return false;
				}
				return true;
			};


		},
		
		// validation for protocol checkboxselect
		_validateProtocol: function(isFocused) {
			
			// always validate true when protocol filter not selected...
			if (this.checkbox_Protocol.checked === false) {
				return true;
			}
			
			var destType = this.type;
			// match up with REST API
			destType = (destType === "Subscription") ? "Shared" : destType;
				
			var invalidProtocols = this._getInvalidProtocolsSelected(destType);
			var valid = invalidProtocols.length === 0 ? true : false;
		    
		    /*
		     * if there was no protocol selected validation should
		     * fail and use the original invalidMessage.
		     */
			var selectedProtocols = this.field_Protocol.get("value");
		    if (selectedProtocols.length === 0) {
		    	//this.field_Protocol.isValid = this.protocolValidator;
		    	valid = false;
		    	this.field_Protocol.invalidMessage = this.protocolValidatorMsg;
		    	
		    } else if (valid === false) {
		    	this.field_Protocol.invalidMessage = lang.replace(nls.messaging.messagingPolicies.dialog.invalid.protocol, [invalidProtocols.join(), destType]);
		    }
		    
		    return valid;
			
		},
		
		// validation for destination type select
		_validateDestination: function(isFocused) {
			
			var destType = this.type;
			// match up with REST API
			destType = (destType === "Subscription") ? "Shared" : destType;
			
			var invalidProtocols = this._getInvalidProtocolsSelected(destType);
			var valid = invalidProtocols.length === 0 ? true : false;
		    
		    return valid;
			
		},
		
		_getInvalidProtocolsSelected: function(destType) {		

			/*
			 * Get all selected protocols and check if all of them
			 * support the destination type selected.
			 */
			var valid = true;
			var invalidProtocols = [];
			var selectedProtocols = this.field_Protocol.get("value");
		    for (var i = 0, len=selectedProtocols.length; i < len; i++) {
		    	var queryResult =  this.protocolStore.query({value:selectedProtocols[i]});
		    	if (queryResult[0][destType] === false) {
		    		valid = false;
		    		invalidProtocols.push(queryResult[0].label);
		    	} 
		    }
		    
		    return invalidProtocols;
			
		},
		
		
		_setMessageHubItemAttr: function(value) {
			this.messageHubItem = value;
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			console.debug("MessagingPolicyDialog -- fillValues", values);
			this._startingValues = values;
			//console.debug(values);
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}
		},

		show: function(messagingPolicy, list) {			

			// Initialize a cache so we can keep track of values that are cleared
			// when changing destination type.
			var topicType = {clientIdChecked: false, Publish: false, Subscribe: false, clientNotify: false, protocolOptions: []};
			var queueType = {clientIdChecked: false, Send: false, Browse: false, Receive: false, protocolOptions: []};
			var subType = {Receive: false, Control: false, protocolOptions: []};
			
			// reset the action list
			this.field_ActionList.removeOption(this.field_ActionList.getOptions());
			this.field_ActionList.addOption(this.actionListOptions[this.type]);
			
			this.settingsCache = {activeType : this.type, maxMsgs: 5000, maxMsgsBehavior: "RejectNewMessages", clientIdChecked: false,
					clientId: "", Topic : topicType, Queue : queueType, Subscription: subType};
					
			// set the correct tooltip for the list of options
			var actionTooltip = dijit.byId(this.field_ActionList.id+"_Tooltip");
			if (actionTooltip) {
				actionTooltip.domNode.innerHTML = this.actionListTooltips[this.type];		
			}

			var protocol = undefined;
			var clientIDCheckbox = this["checkbox_ClientID"];
			var ttlTooltipText = nls.messaging.messagingPolicies.dialog.tooltip.maxMessageTimeToLive;
			
			switch (this.type) {
			case "Topic":
				this.field_MaxMessages.set("disabled", false);
				this.field_MaxMessages.set("value", this.settingsCache.maxMsgs);
				this.field_MaxMessagesBehavior.set("disabled", false);
				this.field_MaxMessagesBehavior.set("value", this.settingsCache.maxMsgsBehavior);				
				this.field_DisconnectedClientNotification.set("disabled", false);
				this.field_DisconnectedClientNotification.set("checked", this.settingsCache.Topic.clientNotify);
				clientIDCheckbox.set("disabled", false);
				clientIDCheckbox.set("checked", this.settingsCache.clientIdChecked);
				this.field_ClientID.set('value', this.settingsCache.clientId);
				//this.field_Protocol.set("value", this.settingsCache.Topic.protocolOptions, false);
				this.publisherSettingsHeading.innerHTML = nls.messaging.messagingPolicies.dialog.publisherSettings;
				this.field_MaxMessageTimeToLive.set("disabled", false);
				this.field_MaxMessageTimeToLive.set("value",this.settingsCache.maxMessageTimeToLive);
				break;
			case "Subscription":
				clientIDCheckbox.set('checked', false);
				clientIDCheckbox.set("disabled", false);
				this.field_ClientID.set('value', "");
				//this.field_Protocol.set("value", this.settingsCache.Subscription.protocolOptions, false);
				this.publisherSettingsHeading.innerHTML = nls.messaging.messagingPolicies.dialog.publisherSettings;
				this.field_MaxMessageTimeToLive.set("disabled", true);
				this.field_MaxMessageTimeToLive.set("value","");
				domStyle.set(this.dialogId+"_DCN_element",'display','none');
				domStyle.set(this.dialogId+"_publisherSettingsHeading", 'display', 'none');
				break;
			case "Queue":
				clientIDCheckbox.set("disabled", false);
				clientIDCheckbox.set("checked", this.settingsCache.clientIdChecked);
				this.field_ClientID.set('value', this.settingsCache.clientId);
				//this.field_Protocol.set("value",this.settingsCache.Queue.protocolOptions, false);
				this.publisherSettingsHeading.innerHTML = nls.messaging.messagingPolicies.dialog.senderSettings;
				ttlTooltipText = nls.messaging.messagingPolicies.dialog.tooltip.maxMessageTimeToLiveSender;
				this.field_MaxMessageTimeToLive.set("disabled", false);
				this.field_MaxMessageTimeToLive.set("value",this.settingsCache.maxMessageTimeToLive);
				domStyle.set(this.dialogId+"_subscriberSettingsHeading", 'display', 'none');
				break;
			default: 
				break;
			}
			
			var ttlTooltip = dijit.byId(this.field_MaxMessageTimeToLive.id+"_Tooltip");
			if (ttlTooltip) {
				ttlTooltip.domNode.innerHTML = ttlTooltipText;		
				}
	
				this.changeMaxMessagesBehavior();
					
			this.field_Name.existingNames = list;
			this._setupFields(messagingPolicy, true);
			// Make sure we know what the initial destination type is
			// and set it in the cache
			this.settingsCache.activeType = this.type;

			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";			
			dijit.byId(this.dialogId+"_saveButton").set('disabled',this.viewOnly === true);
			this.dialog.clearErrorMessage();
			this.dialog.show();
			this.field_Description.focus(); // needed to get the text area to resize
			if (this.add === true || this.viewOnly === true) {
				this.field_Name.focus();
			}
		},

		
		// check to see if anything changed, and save the record if so
		save: function(onFinish, onError, skipCheck) {
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;

			var values = {};
			var recordChanged = false;
			var filterSelected = false;
			var propertiesAffected = [];
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					// don't send excluded values
					if (!this.excludeProperties || !(array.indexOf(this.excludeProperties, actualProp) >=0 )) {
						if (actualProp === "Destination") {
							actualProp = this.type;
							values[actualProp] = this._getFieldValue("Destination");
						} else {
						    values[actualProp] = this._getFieldValue(actualProp);
						}
					    if (values[actualProp] != this._startingValues[actualProp]) {
						    recordChanged = true;
						    if (array.indexOf(this.dynamicProperties, actualProp) >= 0 ) {
							    propertiesAffected.push(actualProp);
						    }
					    }
						if (array.indexOf(this.filters, actualProp) >= 0 && 
								values[actualProp] !== null && values[actualProp] !== undefined  && lang.trim(values[actualProp]) !== "") {
							filterSelected = true;
						}
					}
				}
			}
			
			// verify at least one filter has been selected
			if (filterSelected === false) {
				query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
				this.dialog.showErrorMessage(nls.messaging.messagingPolicies.dialog.invalid.filter);	
				return;
			}
			
			if (this.add) {
				this._doSave(values, onFinish, onError);
			} else if (recordChanged) {
				if (propertiesAffected.length === 0) {
					this._doSave(values, onFinish, onError);					
				} else {
					this._confirmSave(values, propertiesAffected, onFinish, onError);
				}
			} else {
				this.dialog.hide(); // nothing to save
			}
		},
		
		_confirmSave: function(values, propertiesAffected, onFinish, onError) {
			// retrieve a fresh copy of the data so we have a new use count			
			var messagingPolicyName = this._startingValues.Name;
			var adminEndpoint = ism.user.getAdminEndpoint();			
			var promise = adminEndpoint.doRequest("/configuration/" + this.type + "Policy/"+encodeURIComponent(messagingPolicyName), adminEndpoint.getDefaultGetOptions());	
			
			var messagingPolicyDialog = this;
			if (!this.confirmSaveDialog) {
				// create confirmSave dialog
				var confirmSaveId = this.id+"_confirmSaveDialog";
				domConstruct.place("<div id='"+confirmSaveId+"'></div>", this.id);
				var confirmSaveButton = new Button({
		        	  id: confirmSaveId + "_saveButton",
		        	  label: nls.messaging.messagingPolicies.confirmSaveDialog.saveButton,
		        	  onClick: function() { 
		        		  var csDialog = messagingPolicyDialog.confirmSaveDialog;
		        		  csDialog.hide();
		        		  messagingPolicyDialog._doSave(csDialog.values, csDialog.onFinish, csDialog.onError, csDialog.count);
		        	  }
		         });
				this.confirmSaveDialog = new Dialog({
					id: confirmSaveId,
					title: nls.messaging.messagingPolicies.confirmSaveDialog.title,
					content: "<div>" +	nls.messaging.messagingPolicies.confirmSaveDialog.content + "<div>",
					closeButtonLabel: nls.messaging.messagingPolicies.confirmSaveDialog.closeButton,
					buttons: [confirmSaveButton]
				}, confirmSaveId);
				this.confirmSaveDialog.dialogId = confirmSaveId;
			}
			
			var propertiesString = "<table role='presentation' style='margin: 10px 0;'>";

			for (var index=0, len=propertiesAffected.length; index < len; index++) {
				var prop = propertiesAffected[index];
				var value = messagingPolicyDialog._getFieldValue(prop);
				propertiesString += "<tr><td style='padding: 0 5px 0 0;'>" + this.dynamicPropertyLabels[prop] + ":</td><td>";
				switch(prop) {
					case "MaxMessages" :
						value = number.format(value, {places: 0, locale: ism.user.localeInfo.localeString});
						break;
					case "MaxMessagesBehavior":
						value = nls.messaging.messagingPolicies.dialog.maxMessagesBehaviorOptions[value];
						break;
					case "DisconnectedClientNotification":
						value = Utils.booleanDecorator(value);
						break;
					case "MaxMessageTimeToLive":
						value = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
						if (isNaN(value)) {
							value = nls.messaging.messagingPolicies.dialog.unlimited;
						} else {
							value = lang.replace(nls.messaging.messagingPolicies.dialog.maxMessageTimeToLiveValueString, [value]);
						}
						break;
					default: break;
				}
				propertiesString += value + "</td></tr>";				
			}
			propertiesString += "</table>";

			
			promise.then(function(data) {
				var policy = adminEndpoint.getNamedObject(data, this.type + "Policy", messagingPolicyName);
					// nobody using it, just proceed
					messagingPolicyDialog._doSave(values, onFinish, onError/*, useCount*/);
			});

		},
		
		// check to see if anything changed, and save the record if so
		_doSave: function(values, onFinish, onError) {		
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
			
			if (!onFinish) { onFinish = lang.hitch(this,function() {	
				this.dialog.hide();
				});
			}
			if (!onError) { onError = lang.hitch(this,function(error) {	
				bar.innerHTML = "";
				this.dialog.showErrorFromPromise(nls.messaging.messagingPolicies.saveMessagingPolicyError, error);
				});
			}

			var policyName = values.Name;
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions(this.type + "Policy", values);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(function(data) {
				onFinish(values);
			},
			onError);
		},
		
		_updateSettingsCache: function() {
			
			// based on the previous destination type update the values
			// in the cache
			var type = this.settingsCache.activeType;
			
			if (type == "Topic") {
				// Topic specific cache settings
				this.settingsCache.Topic.clientNotify =  this.field_DisconnectedClientNotification.get("checked");
				var publishAction = this.field_ActionList.getOptions("Publish");
				var subscribeAction = this.field_ActionList.getOptions("Subscribe");
				this.settingsCache.Topic.Publish = publishAction.selected;
				this.settingsCache.Topic.Subscribe = subscribeAction.selected;
				this.settingsCache.clientIdChecked =  this["checkbox_ClientID"].get("checked");
				this.settingsCache.clientId = 	this.field_ClientID.get("value");
				this.settingsCache.maxMsgs = this.field_MaxMessages.get("value");
				this.settingsCache.maxMsgsBehavior = this.field_MaxMessagesBehavior.get("value");
				this.settingsCache.Topic.protocolOptions = this.field_Protocol.get("value");
				this.settingsCache.maxMessageTimeToLive = this.field_MaxMessageTimeToLive.get("value");
			} else if (type == "Subscription") {
				// Shared Subscription settings			
				var receiveAction = this.field_ActionList.getOptions("Receive");
				var contorlAction = this.field_ActionList.getOptions("Control");
				this.settingsCache.Subscription.Receive = receiveAction.selected;
				this.settingsCache.Subscription.Control = contorlAction.selected;
				this.settingsCache.maxMsgs = this.field_MaxMessages.get("value");
				this.settingsCache.maxMsgsBehavior = this.field_MaxMessagesBehavior.get("value");
				this.settingsCache.Subscription.protocolOptions = this.field_Protocol.get("value");
			} else if (type == "Queue") {
				// Queue settings
				var sendAction = this.field_ActionList.getOptions("Send");
				var browseAction = this.field_ActionList.getOptions("Browse");
				var receiveActionQ = this.field_ActionList.getOptions("Receive");
				this.settingsCache.Queue.Send = sendAction.selected;
				this.settingsCache.Queue.Browse = browseAction.selected;
				this.settingsCache.Queue.Receive = receiveActionQ.selected;
				this.settingsCache.clientIdChecked =  this["checkbox_ClientID"].get("checked");
				this.settingsCache.clientId = 	this.field_ClientID.get("value");
				this.settingsCache.Queue.protocolOptions = this.field_Protocol.get("value");
				this.settingsCache.maxMessageTimeToLive = this.field_MaxMessageTimeToLive.get("value");
			}
				
			this.settingsCache.activeType = this.type;
		},
		
		/**
		 * Hides or shows the Max Messages Behavior notice icon.
		 * The icon is shown only if Max Messages Behavior is enabled and not
		 * the default (RejectNewMessages)
		 */
		changeMaxMessagesBehavior: function() {
			var showNotice = this.field_MaxMessagesBehavior.get('disabled') === false &&
					this._getFieldValue("MaxMessagesBehavior") !== "RejectNewMessages";
			if (showNotice) {
				this.maxMessagesBehaviorNoticeIcon.style.display = 'inline-block';
			} else {
				this.maxMessagesBehaviorNoticeIcon.style.display = 'none';				
			}
		},		

		// do any adjustment of form field values necessary before showing the form.  in this
		_setupFields: function(messagingPolicy, showWarning) {
			console.debug("MessagingPolicyDialog -- setupFields");

			// clear fields and any validation errors
			dijit.byId(this.dialogId +"_DialogForm").reset();
			dijit.byId(this.dialogId + "_saveButton").set('disabled',false);
			
			if (messagingPolicy) {	
				this.fillValues(messagingPolicy);
			} 
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
					
			var value = prop === "MaxMessageTimeToLive" ? "unlimited" : "";
			
			// if there is a check box for this field, is it enabled?
			var checkbox = this["checkbox_"+prop]; 
			if (checkbox) {
				if (!checkbox.get('checked')) {
					// there is a check box and it's not checked, so return an empty value
					return value;  
				}
			}
			
			// is the field disabled?
			if (this["field_"+prop].get('disabled')) {
				// the field is disabled, so return an empty value
				return value;
			}
			
			switch (prop) {
				case "ActionList":
				case "Protocol":
					var arrayValue;
					if (this["field_"+prop].get) {
						arrayValue = this["field_"+prop].get("value");
					} else {
						arrayValue = this["field_"+prop].value;
					}
					array.forEach(arrayValue, function(entry, i) {
						if (i > 0) {
							value += ",";
						}
						value += entry;
					});
					break;
				case "MaxMessages":
					value = number.parse(this["field_MaxMessages"].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});					
					break;	
				case "DisconnectedClientNotification":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "MaxMessageTimeToLive":
					// if it's set to an empty string, the english keyword unlimited, or a translated version of that,
					// just return the keyword value
					value = this["field_"+prop].get("value");
					if (value === null || value === "") {
						value = "unlimited";
					} else {						
    					var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
    					if (isNaN(intValue)) {
    	                    var nlsUnlimited = nls.messaging.messagingPolicies.dialog.unlimited;
    	                    if (value.toLowerCase() === "unlimited" || value === nlsUnlimited ||
    	                            value.toLowerCase() === nlsUnlimited.toLowerCase()) {
    							value = "unlimited";
    						}
    					} else {
    						value = ""+intValue;
    					}
					}
					break;
				default:
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					break;
			}
			//console.debug("returning field_"+prop+" as '"+value+"'");
			return value;
		},
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("MessagingPolicyDialog -- _setFieldValue", prop, value);

			// if there is a check box for this field, determine whether or not 
			// it should be checked
			var checkbox = this["checkbox_"+prop]; 
			if (checkbox) {
				var valueProvided = value !== null && value !== undefined && value !== "";
				this["checkbox_"+prop].set("checked", valueProvided);					
			}
			
			if (prop === "Topic" 
				|| prop === "Subscription" 
				|| prop === "Queue") {
				prop = "Destination";
			}

			switch (prop) {
				case "Protocol":
				case "ActionList":
					if (value) {
						this["field_"+prop]._setValueAttr(value.split(","));
					}
					break;
				case "MaxMessages":
					this["field_MaxMessages"]._setValueAttr(number.format(value, {places: 0, locale: ism.user.localeInfo.localeString}));					
					break;	
				case "DisconnectedClientNotification":	
					if (value === true || value === false) {
						this["field_"+prop].set("checked", value);						
					} else if (value.toString().toLowerCase() == "true") {
						this["field_"+prop].set("checked", true);
					} else {
						this["field_"+prop].set("checked", false);
					}
					break;	
				case "MaxMessageTimeToLive":
					// if it's set to unlimited, just show the hint text, not the english keyword
					if (value && value.toLowerCase() === "unlimited") {
						value = "";
					} else {
						value = number.format(value, {places: 0, locale: ism.user.localeInfo.localeString});
					}
					this.field_MaxMessageTimeToLive.set("value", value);
					break;
				default:
					if (this["field_"+prop] && this["field_"+prop].set) {
						this["field_"+prop].set("value", value);
					} else if (this["field_"+prop]){
						this["field_"+prop] = { value: value };
					}
					break;
			}
		}
		
	});
	
	return MessagingPolicyDialog;
});
