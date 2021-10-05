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
		'dojo/aspect',
		'dojo/number',
		'dojo/query',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dojo/date/locale',
		'dojo/when',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/MenuItem',
		'dijit/form/Button',
		'dijit/form/Form',
		'idx/form/CheckBox',
		'ism/widgets/AssociationControl',
		'ism/widgets/Dialog',
		'ism/widgets/GridFactory',
		'ism/widgets/TextBox',
		'ism/widgets/Select',
		'ism/widgets/ConfirmationDialog',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'ism/config/Resources',
		'dojo/text!ism/controller/content/templates/DestinationMappingDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'ism/widgets/IsmQueryEngine',
		'dojo/keys'
		], function(declare, lang, array, aspect, number, query, on, domClass, domConstruct, domStyle, 
				Memory, locale, when, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				MenuItem, Button, Form, CheckBox, AssociationControl,
				Dialog, GridFactory, TextBox, Select, ConfirmationDialog, Utils, MonitoringUtils, Resources,
				secProfileDialog, nls, nls_strings, IsmQueryEngine, keys) {

	/*
	 * This file defines a configuration widget that combines a grid with toolbar, and set of dialogs for editing.  All
	 * Dialog widgets are defined inline (rather than in their own file) for simplicity, but could easily be separated
	 * and referenced from an external file.
	 */

	var DestinationMappingDialog = declare("ism.controller.content.DestinationMapping.DestinationMappingDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add or Edit button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds functions to
		//		populate the dialog fields with a record (fillValues) and save the changes
		//		via XHR put (save).  For inputs that are more complex than a text box,
		//		_getFieldValue and _setFieldValue are defined to get/set a generic value for
		//		different input types (e.g. checkbox).
		nls: nls,
		nls_strings: nls_strings,
		templateString: secProfileDialog,

		// template strings
        dialogInstruction: Utils.escapeApostrophe(nls.messaging.destinationMapping.dialog.instruction),
        nameLabel: Utils.escapeApostrophe(nls.messaging.destinationMapping.grid.name),
	    nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
	    typeLabel: Utils.escapeApostrophe(nls.messaging.destinationMapping.grid.type),
	    ruleTypeMessage: Utils.escapeApostrophe(nls.messaging.destinationMapping.ruleTypeMessage),
        sourceLabel: Utils.escapeApostrophe(nls.messaging.destinationMapping.grid.source),
        sourceTooltip: Utils.escapeApostrophe(nls.messaging.destinationMapping.sourceTooltips.type1),
        destinationLabel: Utils.escapeApostrophe(nls.messaging.destinationMapping.grid.destination),
        destinationTooltip: Utils.escapeApostrophe(nls.messaging.destinationMapping.targetTooltips.type1),
        maxMessagesLabel: Utils.escapeApostrophe(nls.messaging.destinationMapping.grid.maxMessages),
        maxMessagesTooltip: Utils.escapeApostrophe(nls.messaging.destinationMapping.dialog.maxMessagesTooltip),
        maxMessagesInvalid: Utils.escapeApostrophe(nls.messaging.destinationMapping.dialog.maxMessagesInvalid),      
        retainedMessagesLabel: Utils.escapeApostrophe(nls.messaging.destinationMapping.dialog.retainedMessages.label),
        retainedMessagesTooltip: Utils.escapeApostrophe(nls.messaging.destinationMapping.dialog.retainedMessages.tooltip.basic),
	    	    
		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},
		
		// list of fields that cannot be edited while an existing rule is enabled
		_notEditableWhileEnabled: ["Name", "RuleType", "Source", "Destination", "QueueManagerConnection"],
		
		_confirmLeadingSpaceId: undefined,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
			this._confirmLeadingSpaceId = this.dialogId + "_confirmLeadingSpaceDialog";
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			if (!this.edit || Utils.configObjName.editEnabled()) {			
				// check that the name matches the pattern and doesn't clash with an existing object
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
				this.field_Name.set('required', false);
				this.field_Name.set('alignWithRequired', true);
				this.field_Name.set('readOnly', true);				
			}
			
			this.field_MaxMessages.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			this.field_MaxMessages.isValid = function(focused) {
				var value = this.get("value");
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				var max = 20000000;
				if (isNaN(intValue) || intValue < 1 || intValue > max) {
					return false;
				}
				return true;
			};
			
			// enable Rule Type select to be marked invalid when the type is not
			// consistent with the Retained Messages setting.
			this.field_RuleType.isRuleTypeConsistentWithRetainedMessages = true;
			this.field_RuleType.isValid = function(focused) {	
				console.debug("field_RuleType.isValid() returning " + this.isRuleTypeConsistentWithRetainedMessages);
				return this.isRuleTypeConsistentWithRetainedMessages;
			};
			
			// watch the form for changes between valid and invalid states
			dijit.byId(this.dialogId+"_form").watch('state', lang.hitch(this, function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(this.dialogId+"_saveButton");
				if (newvalue) {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			}));
			
			// reset the form on close
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				dijit.byId(this.dialogId+"_form").reset();
			}));
			
			dojo.subscribe("RetainedMessageAssociationCountChanged", lang.hitch(this, function(message) {
				message.ruleType = this._getFieldValue("RuleType");
				this._checkRetainedMessages(message);
			}));
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			//make sure fields are enabled
			var len = this._notEditableWhileEnabled ? this._notEditableWhileEnabled.length : 0;
			for (var i=0; i < len; i++) {
				var prop = this._notEditableWhileEnabled[i];
				this["field_"+prop].set('disabled', false);
			}

			this._startingValues = values;
			for (var field in values) {
				this._setFieldValue(field, values[field]);
			}
		},
		
		show: function(list, connProps, queueManagerConnection) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Name.existingNames = list;
			this.dialog.clearErrorMessage();
			this.dialog.show();
			
			// must be done after the dialog has shown
			this.field_QueueManagerConnection.populateGrid(this.dialog, connProps, queueManagerConnection, this.field_RetainedMessages);
			this._checkRetainedMessages({count: this.field_QueueManagerConnection.getCount(), ruleType: this.field_RuleType.get('value')});
			this._startingValues.QueueManagerConnection = queueManagerConnection;
			this.changeEnabled();
		    this.field_Name.focus();
		    
		    // show a warning if there are no queue manager connections
		    if (connProps && connProps.store.data.length === 0) {
		    	this.dialog.showWarningMessage(nls.messaging.destinationMapping.dialog.noQmgrsTitle,
		    			nls.messaging.destinationMapping.dialog.noQmrsDetail);
		    }
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, id, onFinish, skipCheck) {
			var cancel = skipCheck ? false : this._checkForLeadingSpaceInTopic(store, id, onFinish);
			if (cancel) {
				return;
			}
			if (!onFinish) { onFinish = function() {}; }
			var values = {};
			var recordChanged = !this.edit;
			var disablingRule = false;
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (this.edit && actualProp == "Enabled") {
						var realOriginalValue = this._startingValues[actualProp] == "enabled" ? true : false;
						if (values[actualProp] != realOriginalValue) {
							recordChanged = true;
							disablingRule = realOriginalValue;
						}
					}
					else if (values[actualProp] != this._startingValues[actualProp]) {
						recordChanged = true;
					}
				}
			}
			if (recordChanged) {
				// if state changed from enabled to disabled, show confirmation dialog
				if (disablingRule) {
					console.debug("editing rule includes disabling it...");
					var confirmation = manager.byId(this.confirmationId);
					if(confirmation){
						confirmation.confirm(function(){
							onFinish(values);
						}); 
					} else {
						onFinish(values);
					}					
				} else {
					onFinish(values);
				}
			} else {
				console.debug("nothing changed, so no need to save");
				this.dialog.hide();
			}
		},
		
		/**
		 * If the source or destination are topics, check if there are leading
		 * spaces. If so, ask the user if they are sure they want leading spaces.
		 * If they do, return false, otherwise return true.
		 * @returns {Boolean}
		 */
		_checkForLeadingSpaceInTopic: function(store, id, onFinish) {
			// if it's a changed topic, alert when there is a leading space
			var type = this.field_RuleType.get('value');
			var startingValues = this._startingValues ? this._startingValues : {};
			var source = this.field_Source.get('value');	
			var promptSource = false;
			// check source
			switch (type) {
				case "3":
				case "10":
				case "11":
				case "12":
					break;
				default: 
					if (source && source.indexOf(" ") === 0 && source !== startingValues.Source) {
						promptSource = true;
					}
					break;
			}
			// check destination
			var destination = this.field_Destination.get('value');
			var promptDestination = false;
			switch (type) {
				case "1":
				case "5":
				case "10":
				case "12":
				case "13":
				case "14":
					break;
				default: 
					if (destination && destination.indexOf(" ") === 0 && destination !== startingValues.Destination) {
						promptDestination = true;
					}
					break;
			}
			if (!promptSource && !promptDestination) {
				return false;
			}
			
			var text = nls.messaging.destinationMapping.leadingBlankConfirmationDialog.textSource;
			if (promptSource && promptDestination) {
				text = nls.messaging.destinationMapping.leadingBlankConfirmationDialog.textBoth;
			} else if (promptDestination) {
				text = nls.messaging.destinationMapping.leadingBlankConfirmationDialog.textDestination;
			}
			var confirmationDialogId = this._confirmLeadingSpaceId;
			var confirmLeadingSpace = manager.byId(confirmationDialogId);
			if (!confirmLeadingSpace) {
				domConstruct.place("<div id='"+confirmationDialogId+"'></div>", this.id);
				confirmLeadingSpace = new ConfirmationDialog({
					id: confirmationDialogId,
					showConfirmationId: confirmationDialogId,
					text: text,
					info: nls.messaging.destinationMapping.leadingBlankConfirmationDialog.info, // TODO
					buttonLabel: nls.messaging.destinationMapping.leadingBlankConfirmationDialog.buttonLabel, // TODO
					type: "confirmation",
					dupCheck: false          // TODO: Change to true when we implement a user preferences UI
				}, confirmationDialogId);
			} else {
				confirmLeadingSpace.set('text', text);
			}
			confirmLeadingSpace.confirm(function(){
				this.save(store, id, onFinish, true);
			}, this); 
 
			return true;			
		},

		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");
			switch (prop) {
				case "Enabled":
					if (value == "enabled") {
						this["field_"+prop].set("checked", true);
					} else {
						this["field_"+prop].set("checked", false);
					}
					break;
				case "QueueManagerConnection":
					if (value) {
						if (value.qmc) {					
							this["field_"+prop]._setValueAttr(value.qmc.split(","));
						} else {
							this["field_"+prop]._setValueAttr(value.split(","));							
						}
					} 
					break;
				case "MaxMessages":
				case "RuleType":
					this["field_"+prop]._setValueAttr(number.format(value, {places: 0, locale: ism.user.localeInfo.localeString}));					
					break;	
				default:
					if (this["field_"+prop] && this["field_"+prop].set) {
						this["field_"+prop].set("value", value);
					} else {
						this["field_"+prop] = { value: value };
					}
					break;
			}
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value;
			switch (prop) {
				case "Enabled":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "QueueManagerConnection":
					value = this["field_"+prop].getValueAsString();
					break;
				case "MaxMessages":
				case "RuleType":
					value = number.parse(this["field_"+prop].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});					
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
		
		_checkRetainedMessages: function(message) {
			var tooltips = nls.messaging.destinationMapping.dialog.retainedMessages.tooltip;
			var tooltipText = tooltips.basic;			
			console.dir(message);
			switch (message.ruleType) {
			case "1":
			case "5":
			case "10":
			case "12":
			case "13":
			case "14":
				this._setFieldValue("RetainedMessages", "None");						
				this.field_RetainedMessages.set('disabled',true);				
				if (message.count > 1) {
					tooltipText = tooltips.disabled4Both;
				} else {
					tooltipText = tooltips.disabled4Type;
				}
				break;
			default:
				if (message.count > 1) {
					this._setFieldValue("RetainedMessages", "None");						
					this.field_RetainedMessages.set('disabled',true);
					tooltipText = tooltips.disabled4Associations;
				} else {
					this.field_RetainedMessages.set('disabled',false);
				}
				break;
			}
			var retainedMessagesTip = dijit.byId(this.field_RetainedMessages.id+"_Tooltip");
			if (retainedMessagesTip) {
				retainedMessagesTip.domNode.innerHTML = tooltipText;
			}
		},
		
		/**
		 * If this is an edit dialog and the edit started off with an enabled rule, 
		 * checks to see if the enabled field is checked. If it is checked, disables 
		 * fields in _notEditableWhileEnabled, otherwise enabled them. If the value 
		 * has been edited, disabled fields will also have their values reset.  
		 */
		changeEnabled: function() {
			if (!this.edit) {
				// nothing to do on an add
				return;
			}
			if (this._startingValues.Enabled === "disabled") {
				// if we started the edit on a disabled rule, no restrictions should apply
				return;
			}
			
			var enabled = this._getFieldValue("Enabled");
			var len = this._notEditableWhileEnabled ? this._notEditableWhileEnabled.length : 0;
			for (var i=0; i < len; i++) {
				var prop = this._notEditableWhileEnabled[i];
				this["field_"+prop].set('disabled', enabled);
				if (enabled && this._getFieldValue(prop) !== this._startingValues[prop]) {
					this._setFieldValue(prop, this._startingValues[prop]);
				}
			}
			if (enabled) {
				// show an explanatory message
				this.dialog.showInfoMessage(nls.messaging.destinationMapping.editDialog.restrictedEditingTitle,
						nls.messaging.destinationMapping.editDialog.restrictedEditingContent);
			}
		},
		
		changeRuleType: function(rule) {
			console.debug("changeRuleType: "+rule);

			var sourceTip = dijit.byId(this.field_Source.id+"_Tooltip");
			if (sourceTip) {
				sourceTip.domNode.innerHTML = nls.messaging.destinationMapping.sourceTooltips["type"+rule];		
			}
			
			var targetTip = dijit.byId(this.field_Destination.id+"_Tooltip");
			if (targetTip) {
				targetTip.domNode.innerHTML = nls.messaging.destinationMapping.targetTooltips["type"+rule];		
			}
			
			// enable/disable MaxMessages according to rule type
			if (rule==1 || rule==2 || rule==5 || rule==6 || rule==7) {
				this.field_MaxMessages.set('disabled',false);
			} else {
				this.field_MaxMessages.set('disabled',true);
			}
			
			var validNow = this.field_RuleType.isRuleTypeConsistentWithRetainedMessages;
			if (this.field_RetainedMessages && this.field_RetainedMessages.get('value') !== 'None') {
				// RetainedMessages special case
				if (rule==1 || rule==5 || rule==10 || rule==12 || rule==13 || rule==14) {
					this.field_RuleType.isRuleTypeConsistentWithRetainedMessages = false;
				} else {
					this.field_RuleType.isRuleTypeConsistentWithRetainedMessages = true;
				}				
			} else {
				this.field_RuleType.isRuleTypeConsistentWithRetainedMessages = true;
				// enable/disable RetainedMessages according to rule type & number of associations
				this._checkRetainedMessages({count: this.field_QueueManagerConnection.getCount(), ruleType: rule});
			}
			if (validNow != this.field_RuleType.isRuleTypeConsistentWithRetainedMessages) {
				console.debug("changeRuleType is validating RuleType");
				// twiddle with _hasBeenBlurred to get consistent error behavior
				this.field_RuleType._hasBeenBlurred = true;
				this.field_RuleType.validate();
			}			
		},
		
		changeRetainedMessages: function(value) {
			console.debug("changeRetainedMessages value=" + value);
			if (value === 'None') {
				// might need to clear some error states...
				this.field_RuleType.isRuleTypeConsistentWithRetainedMessages = true;
				this.field_RuleType.validate();
				this.field_QueueManagerConnection.validate();
			}
		}
	});

	var DestinationMapping = declare("ism.controller.content.DestinationMapping", [_Widget, _TemplatedMixin], {
		// summary:
		//		Dialog tied to the Add button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds a function to
		//		save the result (adding a new record).  For input that is more complex than a text box,
		//		_getFieldValue is defined to get the value for different input types (e.g. checkbox).

		grid: null,
		store: null,
		queryEngine: IsmQueryEngine,
		editDialog: null,
		addDialog: null,
		removeDialog: null,
		removeNotAllowedDialog: null,
		contentId: null,
		connProps: null,
		hover: null,
		
		addDialogTitle: Utils.escapeApostrophe(nls.messaging.destinationMapping.addDialog.title),
        editDialogTitle: Utils.escapeApostrophe(nls.messaging.destinationMapping.editDialog.title),
        removeDialogTitle: Utils.escapeApostrophe(nls.messaging.destinationMapping.removeDialog.title),		
		
		addButton: null,
		deleteButton: null,
		editButton: null,
		enableButton: null,
		disableButton: null,
		refreshStatusButton: null,
		
		isRefreshing: false,
		
		confirmationId: "disableDestinationMappingRule",
		
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		domainUid: 0,
		     			
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		
			this.contentId = this.id + "Content";
			console.debug("contentId="+this.contentId);
			this.store = new Memory({data:[], idProperty: "Name", queryEngine: this.queryEngine});
		},

		postCreate: function() {
			this.inherited(arguments);
            
            this.domainUid = ism.user.getDomainUid();

			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;

			// start with an empty <div> for the Listeners content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.messaging.nav.mqConnectivity.topic, lang.hitch(this, function(message){
				this._initStore();
			}));	
			dojo.subscribe(Resources.pages.messaging.nav.mqConnectivity.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this._refreshGrid();
			}));	
			
			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();
			
			this.connProps = dijit.byId("connprops");
			aspect.after(this.connProps, "triggerListeners", lang.hitch(this,function() {
				console.debug("connProps triggered");	
				this._refreshGrid();
			}));
		},
		
		_initStore: function() {
			// summary:
			// 		get the initial records from the server
	    	this.isRefreshing = true;
			this.store = new Memory({data:[], idProperty: "id"});
    		this.grid.setStore(this.store);			
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;

			var page = this;
			
			var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.getDefaultGetOptions();
            var promise = endpoint.doRequest("/configuration/DestinationMappingRule", options);
            
            promise.then(
            		function(data){
    			    	console.debug("destination mapping rest GET returned");
    			    	page.isRefreshing = false;
    			    	page.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
    			    	var destmappingrules = endpoint.getObjectsAsArray(data, "DestinationMappingRule");
    			    	if (destmappingrules) {
    			    		console.debug("success");
    			    		page.numStatusResultsToRequest = 100; // start with 100 just as a buffer
    			    		// add an id property with an encoded form of the name
    			    		array.map(destmappingrules,lang.hitch(this,function(item) {
    			    			item.id = escape(item.Name);
    			    			item.Enabled = item.Enabled ? "enabled" : "disabled";
    			    			item.RetainedMessagesDisplayValue = nls.messaging.destinationMapping.grid[item.RetainedMessages.toLowerCase()];
    			    			page.numStatusResultsToRequest += item.QueueManagerConnection ? item.QueueManagerConnection.split(",").length : 0;
    			    			item.QueueManagerConnection = { qmc: item.QueueManagerConnection, name: item.Name };			    						    			
    			    		}));
    			    		page.store = new Memory({data: destmappingrules, idProperty: "id", queryEngine: page.queryEngine});
    			    		page.grid.setStore(page.store);
    			    		page.triggerListeners();
    						page._updateMonitoring(); // get initial status for all rules
    			    	} else {
    			    		console.debug("error");
    			    	}
    			    },
    			    function(error) {
    			    	console.debug("error: "+error);
    			    	this.isRefreshing = false;
    			    	page.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
    			    	Utils.showPageErrorFromPromise(nls.messaging.destinationMapping.retrieveError, error);
    			    }
            		);            
		},
		
		_updateMonitoring: function(name) {
			console.debug(this.declaredClass + "._updateMonitoring("+name+")");
			if (!name) {
				name = "*";
			}
			
			// Note the numStatusResultsToRequest is the most we expect, as we keep adding to it and 
			// never decrement it. We just want to be sure we get all the statuses back!
			var numRecords = 500 > this.numStatusResultsToRequest ? 500 : this.numStatusResultsToRequest;
			console.debug("_updateMonitoring requesting up to " + numRecords + " records, numStatusResultsToRequest is " + this.numStatusResultsToRequest);
			
			var deferred = MonitoringUtils.getDestinationMappingData({
				ruleType: "Any",
				association: "*",
				statType: "Status",
				name: name,
				numRecords: numRecords
			});
						
			deferred.then(lang.hitch(this, function(data) {
				console.debug("got monitoring data back:");
				console.dir(data);
				
				if (!data || !data.DestinationMappingRule) {
					// This should never happen but there's nothing to do if it does happen
					return;
				}
				
				array.forEach(data.DestinationMappingRule, lang.hitch(this,function(item){
					var ruleName = item.RuleName;
					//console.debug("rule="+ruleName);
					var dest = this.store.get(escape(ruleName));
					if (dest) {
						var t = dest.QueueManagerConnection;
						if (!t.status) {
							t.status = {};
						}
						t.status[item.QueueManagerConnection] = item.Status;
						if (!t.message) {
							t.message = {};
						}
						t.message[item.QueueManagerConnection] = item.ExpandedMessage;
						t.enabled = dest.Enabled;						
						dest.QueueManagerConnection = {
								qmc: t.qmc,
								name: t.name,
								status: t.status,
								message: t.message,
								enabled: dest.Enabled,
								sortWeight: t.qmc.split(",").length
						};
						this.store.put(dest,{overwrite: true});
						// If we asked for the status of a specific rule and got a 'pending' status back, we schedule
						// another update as we expect the status to change soon
						if (name != "*") {
							if ((item.Status == 2) || (item.Status == 3)) { // reconnecting or starting
								console.debug("pending status, scheduling update");
								// schedule update
								setTimeout(lang.hitch(this, function(){
									this._updateMonitoring(name);
								}), 2000);
							}
						}
					}
				}));
				this._refreshGrid();
			}), function(err) {
				console.debug("Error from getDestinationMappingData: "+err);

		    	Utils.showPageErrorFromPromise(nls.messaging.destinationMapping.retrieveError, err);
				return;
			});
		},
		
		_saveSettings: function(values, dialog, onFinish, onError) {
            console.debug("Saving...");
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
			var ret = false;
			
			delete values.id;  // remove id because REST API doesn't expect it.
			delete values.RetainedMessagesDisplayValue; // remove RetainedMessagesDisplayValue because REST API doesn't expect it.
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("DestinationMappingRule", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(function() {
	    		console.debug("success");
	    		page._updateMonitoring(values.Name); // get status for new rule
	    		onFinish();
            },
            function(error) {
		    	console.debug("error: "+error);
				dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
				onError();
		    });            
		},
		
		_changeEnabled: function(values) {
			console.debug(this.declaredClass + "._changeEnabled("+values.Enabled+")");
			var data = {"Enabled" : values.Enabled == "enabled", "Name" : values.Name};
			console.debug("data="+data);
			
			var newState = values.Enabled;
			values.id = escape(values.Name);
			values.Enabled = "pending";
			this.store.put(values,{overwrite: true});
			this._refreshGrid();
			
			var page = this;
			var ret = false;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            values.Enabled = data.Enabled;
            var options = adminEndpoint.createPostOptions("DestinationMappingRule", data);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(function() {
		    	console.debug("destination mapping rest PUT returned");
		    	values.Enabled = newState;
		    	page.store.put(values,{overwrite: true});
		    	page._refreshGrid();
		    	page._updateMonitoring(values.Name); // update status for changed rule
		    	
		    	// update actions if this entry is selected
		    	if (page._getSelectedData().Name == values.Name) {
		    		if (values.Enabled == "enabled") {
		    			page.enableButton.set('disabled', true);
		    			page.disableButton.set('disabled', false);
		    		} else {
		    			page.enableButton.set('disabled', false);
		    			page.disableButton.set('disabled', true); 			
		    		}
		    	}
            },
            function(error) {
		    	console.debug("error: "+error);
				var code = Utils.showPageErrorFromPromise(nls.messaging.savingFailed, error);
				if (code == "CWLNA5017") { // object not found
					console.debug("Error CWLNA5017 thrown for object not found");						
					page.store.remove(values.id); // remove from grid
				}
            });
		},
		
		_updateEntry: function(entry, values, dialog, onFinish, onError) {
            console.debug("Saving... "+entry);
			
			console.debug("data="+JSON.stringify(values));
			
			delete values.id;  // remove id since REST API doesn't expect it.
			delete values.RetainedMessagesDisplayValue; // remove RetainedMessagesDisplayValue because REST API doesn't expect it.
			
			var ret = false;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("DestinationMappingRule", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(function(){
			    console.debug("destination mapping rest PUT returned");
			    if (onFinish)
			    	onFinish();
            },
            function(error) {
		    	console.debug("error: "+error);
				dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
				if (onError)
					onError(error);
            });
		},
		
		_deleteEntry: function(id, name, dialog, onFinish, onError) {
			console.debug("delete: "+name);
			var _this = this;
			
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultDeleteOptions();
            var promise = adminEndpoint.doRequest("/configuration/DestinationMappingRule/" + encodeURIComponent(name), options);
            promise.then(function() {
				_this.store.remove(id);
				onFinish();
            }, function(error) {
				console.error(error);
				dialog.showErrorFromPromise(nls.messaging.deletingFailed, error);
				onError();
			});
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar
						
			// create Edit dialog
			console.debug("id="+this.id+" contentId="+this.contentId);
			console.debug("editDialogTitle="+this.editDialogTitle);
			var editTitle = dojo.string.substitute(this.editDialogTitle,{nls:nls});
			var editId = "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);
			
			this.editDialog = new DestinationMappingDialog({dialogId: editId, dialogTitle: editTitle, edit: true, confirmationId: this.confirmationId }, editId);
			this.editDialog.startup();

			// create Add dialog
			var addTitle = dojo.string.substitute(this.addDialogTitle,{nls:nls});
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new DestinationMappingDialog({dialogId: addId, dialogTitle: addTitle, edit: false },
					addId);
			this.addDialog.startup();
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = dojo.string.substitute(this.removeDialogTitle,{nls:nls});
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + nls.messaging.connectionProperties.removeDialog.content + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_button_ok",
						label: nls.messaging.dialog.deleteButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.messaging.dialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			
			// create Remove not allowed dialog (remove not allowed when the mapping is active)
			var removeNotAllowedId = this.id+"_removeNotAllowedDialog";
			domConstruct.place("<div id='"+removeNotAllowedId+"'></div>", this.id);
			this.removeNotAllowedDialog = new Dialog({
				id: removeNotAllowedId,
				title: nls.messaging.destinationMapping.removeNotAllowedDialog.title,
				content: "<div style='display: table;'>" +
						"<img style='display: inline-block; float: left; margin: 0 10px 0 0;' src='css/images/warning24.png' alt='' />" +
						"<span style='display: table-cell; vertical-align:middle; word-wrap:break-word;'>" + 
						nls.messaging.destinationMapping.removeNotAllowedDialog.content + "</span><div>",
				closeButtonLabel: nls.messaging.destinationMapping.removeNotAllowedDialog.closeButton
			}, removeNotAllowedId);
			this.removeNotAllowedDialog.dialogId = removeNotAllowedId;

			var confirmationDialogId = this.id+"_confirmationDialogId";
			domConstruct.place("<div id='"+confirmationDialogId+"'></div>", this.id);
			var	confirmation = new ConfirmationDialog({
				id: this.confirmationId,
				showConfirmationId: this.confirmationId,
				text: nls.messaging.destinationMapping.disableRulesConfirmationDialog.text,
				info: nls.messaging.destinationMapping.disableRulesConfirmationDialog.info,
				buttonLabel: nls.messaging.destinationMapping.disableRulesConfirmationDialog.buttonLabel,
				type: "confirmation",
				dupCheck: false          // TODO: Change to true when we implement a user preferences UI
			}, confirmationDialogId);

		},
		
		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
								{	id: "Name", name: nls.messaging.destinationMapping.grid.name, field: "Name", dataType: "string", width: "120px",
									decorator: Utils.nameDecorator
								},
								{	id: "RuleType", name: nls.messaging.destinationMapping.grid.type, field: "RuleType", dataType: "string", width: "180px",
									formatter: function(data) {
											return nls.messaging.destinationMapping.ruleTypes["type"+data.RuleType];
										}
									},
								{	id: "Source", name: nls.messaging.destinationMapping.grid.source, field: "Source", dataType: "string", width: "161px" },
								{	id: "Destination", name: nls.messaging.destinationMapping.grid.destination, field: "Destination", dataType: "string", width: "161px" },
								{	id: "MaxMessages", name: nls.messaging.destinationMapping.grid.maxMessages, field: "MaxMessages", width: "74px",
									formatter: function(data) {
										var formattedData = number.format(data.MaxMessages, {places: 0, locale: ism.user.localeInfo.localeString}); 
										return formattedData;
									}
								},
								{	id: "RetainedMessagesDisplayValue", name: nls.messaging.destinationMapping.grid.retainedMessages, field: "RetainedMessagesDisplayValue", width: "74px"},
								{	id: "Enabled", name: nls.messaging.destinationMapping.grid.enabled, field: "Enabled", dataType: "string", width: "50px",
									decorator: function(cellData) {
											var iconClass = "ismIconChecked";
											var altText = nls.messaging.destinationMapping.state.enabled;
											if (cellData == "disabled") {
												iconClass = "ismIconUnchecked";
												altText = nls.messaging.destinationMapping.state.disabled;
											} else if (cellData == "pending") {
												iconClass = "ismIconLoading";
												altText = nls.messaging.destinationMapping.state.pending;								
											}
											return "<div class='"+iconClass+"' title='"+altText+"'></div>";
										}
									},
								{	id: "QueueManagerConnection", name: nls.messaging.destinationMapping.grid.associations, field: "QueueManagerConnection", filterable: false, dataType: "string", width: "80px",
										widgetsInCell: true,
										decorator: function() {
											return "<div data-dojo-type='ism.widgets.AssociationControl' data-dojo-attach-point='destMap' data-dojo-props='' class='gridxHasGridCellValue'></div>";
										},
									    setCellValue: function(gridData, storeData, cellWidget){
									    	// set the value since the formatter is only returning a single value...
									    	cellWidget.destMap.set("value", storeData);
									    },
									    formatter: function(data) {							    	
									    	var filterText =  data.QueueManagerConnection.qmc ? data.QueueManagerConnection.qmc.split(",").length : 0;
									    	return filterText;
									    }
								}
     
			                 ];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
			
			this.enableButton = new MenuItem({
				label: nls.messaging.destinationMapping.action.Enable
			});
			GridFactory.addToolbarMenuItem(this.grid, this.enableButton);
			this.enableButton.set('disabled', true);
			
			this.disableButton = new MenuItem({
				label: nls.messaging.destinationMapping.action.Disable
			});
			GridFactory.addToolbarMenuItem(this.grid, this.disableButton);
			this.disableButton.set('disabled', true);
			
			this.refreshStatusButton = new MenuItem({
				label: nls.messaging.destinationMapping.action.Refresh
			});
			GridFactory.addToolbarMenuItem(this.grid, this.refreshStatusButton);
			
		},

		_getSelectedData: function() {
			// summary:
			// 		get the row data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).rawData();
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			console.debug("refresh");
			this.grid.body.refresh();
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar buttons, and dialog
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    // create array of existing userids so we don't clash
				var existingNames = this.store.query({}).map(
						function(item){ return item.Name; });
			    this.addDialog.show(existingNames, this.connProps);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var rowData = this._getSelectedData();
				if (rowData.Enabled == "disabled") {
					this.removeDialog.recordToRemove = rowData.Name;
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.show();		
				} else {					
					this.removeNotAllowedDialog.show();
				}
			}));
					
			on(this.editButton, "click", lang.hitch(this, function() {
				console.debug("clicked edit button");
				this._doEdit();
			}));
			
			aspect.after(this.grid, "onRowDblClick", lang.hitch(this, function(event) {
				// if the edit button is enabled, pretend it was clicked
				if (!this.editButton.get('disabled')) {
					this._doEdit();
				}
			}));

		    aspect.after(this.grid, "onRowKeyPress", lang.hitch(this, function(event) {
		    	// if the enter key was pressed on a row and the edit button is enabled, 
		    	// pretend it was clicked				
				if (event.keyCode == keys.ENTER && !this.editButton.get('disabled')) {
					this._doEdit();
				} else if (event.charCode == keys.SPACE) {
					this._doSelect();
				}
			}), true);
			
			on(this.enableButton, "click", lang.hitch(this, function() {
			    console.log("clicked enable button");
			    this.enableButton.set('disabled', true);
			    var rowData = this._getSelectedData();
			    rowData.Enabled = "enabled";
			    this._changeEnabled(rowData);
			}));
			on(this.disableButton, "click", lang.hitch(this, function() {
			    console.log("clicked disable button");
				var confirmation = manager.byId(this.confirmationId);
				if(confirmation){
					confirmation.confirm(function(){
					    this.disableButton.set('disabled', true);
					    var rowData = this._getSelectedData();
					    rowData.Enabled = "disabled";
					    this._changeEnabled(rowData);
					}, this); 
				} else {
				    this.disableButton.set('disabled', true);
				    var rowData = this._getSelectedData();
				    rowData.Enabled = "disabled";
				    this._changeEnabled(rowData);
				}								    
			}));
			on(this.refreshStatusButton, "click", lang.hitch(this, function() {
			    console.log("clicked refreshStatus button");
			    this._updateMonitoring();
			}));			
			
			
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
					var name = this._getSelectedData().Name;
					var id = this.grid.select.row.getSelected()[0];
					this.editDialog.save(this.store,id, lang.hitch(this, function(data) {
						console.debug("edit is done, returned:", data);
						var bar = query(".idxDialogActionBarLead",this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
						var _this = this;
						this._updateEntry(name,data,this.editDialog.dialog, function() {
							console.debug("Saving:");
							console.dir(data);
							data.id = escape(data.Name);
							data.Enabled = data.Enabled ? "enabled" : "disabled";
							data.RetainedMessagesDisplayValue = nls.messaging.destinationMapping.grid[data.RetainedMessages.toLowerCase()];
							_this.numStatusResultsToRequest += data.QueueManagerConnection ? data.QueueManagerConnection.split(",").length : 0;
							data.QueueManagerConnection = { qmc: data.QueueManagerConnection, name: data.Name };
							if (data.Name != _this.editDialog._startingValues.Name) {
								console.debug("name changed, removing old name");
								_this.store.remove(id);
								_this.store.add(data);		
							} else {
								_this.store.put(data);					
							}
							_this.enableButton.set('disabled', data.Enabled === "enabled");
							_this.disableButton.set('disabled', data.Enabled === "disabled");
							_this._updateMonitoring(data.Name); // update status for changed rule
							_this.editDialog.dialog.hide();
							_this._refreshGrid();
							_this.triggerListeners();
						}, function(error) {
							console.debug("edit failed");
							bar.innerHTML = "";
						});
					}));
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving addDialog");
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {		
					var bar = query(".idxDialogActionBarLead",this.addDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
					this.addDialog.save(this.store, null, lang.hitch(this, function(data) {
						console.debug("add is done, returned:", data);
						var _this = this;
						this._saveSettings(data, _this.addDialog.dialog, function() {
							data.id = escape(data.Name);
							data.Enabled = data.Enabled ? "enabled" : "disabled";
							data.RetainedMessagesDisplayValue = nls.messaging.destinationMapping.grid[data.RetainedMessages.toLowerCase()];
							_this.numStatusResultsToRequest += data.QueueManagerConnection ? data.QueueManagerConnection.split(",").length : 0;
							data.QueueManagerConnection = { qmc: data.QueueManagerConnection, name: data.Name };
							when(_this.store.add(data),lang.hitch(_this, function(){
								_this.addDialog.dialog.hide();
								_this.triggerListeners();								
							}));
						}, 
						function (error) {
							console.debug("save failed");
							bar.innerHTML = "";
						});
					}));
				}
			}));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				console.debug("remove is done");
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var name = this._getSelectedData().Name;
				var _this = this;
				this._deleteEntry(id, name, this.removeDialog, function() {
					_this.removeDialog.hide();
					// disable buttons if empty or no selection
					if (_this.store.data.length === 0 ||
							_this.grid.select.row.getSelected().length === 0) {
						_this.editButton.set('disabled', true);
						_this.deleteButton.set('disabled', true);
					}
					_this.triggerListeners();
				}, function(error) {
					console.debug("delete failed");
					bar.innerHTML = "";
				});
			}));
			
		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				console.debug("index="+ind);
				console.log("row count: "+this.grid.rowCount());
				this.editButton.set('disabled', false);
				this.deleteButton.set('disabled', false);
				if (this._getSelectedData().Enabled == "enabled") {
					this.enableButton.set('disabled', true);
					this.disableButton.set('disabled', false);
				} else {
					this.enableButton.set('disabled', false);
					this.disableButton.set('disabled', true);
				}
			}
		},
		
		_doEdit: function() {
			var rowData = lang.clone(this._getSelectedData());
			console.dir(rowData);
			// create array of existing userids except for the current one, so we don't clash
			var existingNames = this.store.query(function(item){ return item.Name != rowData.Name; }).map(
					function(item){ return item.Name; });
			var queueManagerConnection = rowData.QueueManagerConnection.qmc ? rowData.QueueManagerConnection.qmc : rowData.QueueManagerConnection;
			console.debug("queueManagerConnection="+queueManagerConnection);
			this.editDialog.fillValues(rowData);
			this.editDialog.show(existingNames, this.connProps, queueManagerConnection);
		},
		
		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated 
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");
			
			// update grid quick filter if there is one
			if (this.grid.quickFilter) {
				this.grid.quickFilter.apply();				
			}	
		}
	});

	return DestinationMapping;
});
