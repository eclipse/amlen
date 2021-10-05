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
		'dojo/query',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dojo/store/Observable',
		'dojo/date/locale',
		'dojo/when',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'dijit/form/Form',
		'dijit/MenuItem',
		'idx/string',
		'ism/widgets/Dialog',
		'ism/widgets/CheckBoxList',
		'ism/widgets/GridFactory',
		'ism/widgets/TextBox',
		'ism/widgets/Select',
		'ism/IsmUtils',
		'ism/config/Resources',
		'dojo/text!ism/controller/content/templates/QMConnectionPropertiesDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/messages',
		'dojo/keys'
		], function(declare, lang, array, aspect, query, on, domClass, domConstruct, domStyle, 
				Memory, Observable, locale, when, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, Form, MenuItem, iString, Dialog, CheckBoxList, GridFactory,
				TextBox, Select, Utils, Resources, template, nls, nls_strings, nls_messages, keys) {

	/*
	 * This file defines a configuration widget that combines a grid with toolbar and a set of dialogs for editing.  All
	 * Dialog widgets are defined inline (rather than in their own file) for simplicity, but could easily be separated
	 * and referenced from an external file.
	 */

	var QMConnectionDialog = declare("ism.controller.content.QMConnectionProperties.QMConnectionDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Edit button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds functions to
		//		populate the dialog fields with a record (fillValues) and save the changes
		//		via XHR put (save).  For inputs that are more complex than a text box,
		//		_getFieldValue and _setFieldValue are defined to get/set a generic value for
		//		different input types (e.g. checkbox).
		nls: nls,
        nls_strings: nls_strings,
		templateString: template,
		labelWidth: 175,
        
        // template strings
        dialogInstruction: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.instruction),
		nameLabel: Utils.escapeApostrophe(nls.messaging.connectionProperties.grid.name),
		nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        qmLabel: Utils.escapeApostrophe(nls.messaging.connectionProperties.grid.qmgr),
        qmTooltip: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.qmTooltip),
        qmInvalid: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.qmInvalid),
        connName: Utils.escapeApostrophe(nls.messaging.connectionProperties.grid.connName),
        connTooltip: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.connTooltip),
        connInvalid: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.connInvalid),
        channelLabel: Utils.escapeApostrophe(nls.messaging.connectionProperties.grid.channel),
        channelTooltip: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.channelTooltip),
        channelInvalid: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.channelInvalid),
        channelUserLabel: Utils.escapeApostrophe(nls.channelUser),
        channelUserTooltip: Utils.escapeApostrophe(nls.channelUserTooltip),
        channelPasswordLabel: Utils.escapeApostrophe(nls.channelPassword),
        channelPasswordTooltip: Utils.escapeApostrophe(nls.channelPasswordTooltip),
        SSLCipherSpecLabel: Utils.escapeApostrophe(nls.messaging.connectionProperties.grid.SSLCipherSpec),
        SSLCipherSpecTooltip: Utils.escapeApostrophe(nls.messaging.connectionProperties.dialog.SSLCipherSpecTooltip),

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// dialog counter to keep track of which dialog is opened for callback purposes
		dialogCounter: 0,
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},
		
		// Whether to send the channel password in a create or update request
		sendPassword: false,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
            var locale = ism.user.localeInfo.localeString;
	        locale = locale && locale.length > 2 ? locale.substring(0, 2) : locale;
	        if (locale == "de") {
	             this.labelWidth = 230;
	        } else if (locale == "fr") {
	        	this.labelWidth = 205;
	        }
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
						
			if (this.add === true || Utils.configObjName.editEnabled()) {			
				// check that the Name matches the pattern and doesn't clash with an existing object
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
			
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				dijit.byId(this.dialogId+"_form").reset();
			}));
			
			this.field_ChannelUserPassword.validator = function() {
				var val = this.get("value");
				var ph = this.get("placeHolder");
				var valid = (((val !== "") && (val !== undefined)) 
			              || ((val === "") && (ph !== "")));
				if (valid) {
					this.set("state", "");
				} else {
					this.set("state", "Incomplete");
				}
				return valid;
			};
		},
        
        toggleChannelUserRequired: function () {
		    var uval = this["field_ChannelUserName"].get('value');
		    var required = (uval !== undefined && uval !== "");
		    if (required) {
		    	this["field_ChannelUserPassword"].set("disabled", false);
		    }
		    this["field_ChannelUserPassword"].setRequiredOnOff(required);						
		    this["field_ChannelUserName"].setRequiredOnOff(required);
		    if (!required) {
		    	if (this["field_ChannelUserPassword"].get("value") !== "" || this["field_ChannelUserPassword"].get("placeHolder") !== "") {
		    		this["field_ChannelUserPassword"].set("value","");
		    		this["field_ChannelUserPassword"].set("placeHolder","");
		    		this.sendPassword = true;
		    	}
		    	this["field_ChannelUserPassword"].set("disabled", true);
		    	// Assure field errors are cleared in case they appeared before toggling
		    	this["field_ChannelUserPassword"].set("state","");
		    	this["field_ChannelUserName"].set("state","");
		    }
        },
        
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}
		    var required = (values["ChannelUserName"] !== undefined && values["ChannelUserName"] !== "");
		    if (required) {
		    	this["field_ChannelUserPassword"].set("disabled",false);
		    }
		    this["field_ChannelUserPassword"].setRequiredOnOff(required);		
		    this["field_ChannelUserName"].setRequiredOnOff(required);
		    if (!required) {
		    	this["field_ChannelUserPassword"].set("disabled",true);
		    	// Assure field errors are cleared in case they appeared previosly
		    	this["field_ChannelUserPassword"].set("state","");
		    	this["field_ChannelUserName"].set("state","");
		    }
		},

		show: function(list) {
			this.dialogCounter++;
			this.sendPassword = false;
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Name.existingNames = list;
			if (this.add === true) {
				this.field_Name.set("value", "");
				this.field_QueueManagerName.set("value", "");
				this.field_ConnectionName.set("value", "");
				this.field_ChannelName.set("value", "");
				this.field_ChannelUserName.set("value", "");
				this.field_ChannelUserPassword.set("value", "");
				this.field_ChannelUserPassword.set("disabled", true);
				this.field_SSLCipherSpec.set("value", "");
			} 

			dijit.byId(this.dialogId+"_saveButton").set('disabled',false);
			dijit.byId(this.dialogId+"_testButton").set('disabled',false);
			this.dialog.clearErrorMessage();
			this.dialog.show();
		    this.field_Name.focus();
		    
		    // show a warning and disable Save button if editing when associated with an enabled mapping rule
		    if (this.add !== true) {
		    	var destmap = dijit.byId("destmap");
		    	if (destmap && destmap.store) {
		    		var connName = this.field_Name.get("value");
		    		var names = [];
		    		var count = destmap.store.query(function(dest){
		    				var qmc = dest.QueueManagerConnection.qmc + ",";
		    				if ((dest.Enabled == "enabled") && (qmc.indexOf(connName+",") != -1)) {
		    					names.push("\"" + Utils.nameDecorator(dest.Name) + "\"");
		    					return true;
		    				}
		    				return false;
		    			}).length;
		    		if (count > 0) {		    		    			
			    		var nameList = names.join(", ");
			    		var content = "";
			    		if (count == 1) {
			    			content = lang.replace(nls.messaging.connectionProperties.dialog.activeRuleDetailOne,[nameList]);    			
			    		} else {
			    			content = lang.replace(nls.messaging.connectionProperties.dialog.activeRuleDetailMany,[nameList]);
			    		}
			    		this.dialog.showInfoMessage(nls.messaging.connectionProperties.dialog.activeRuleTitle,content);
			    		dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
						this.field_QueueManagerName.set("readOnly", true);
						this.field_ConnectionName.set("readOnly", true);
						this.field_ChannelName.set("readOnly", true);
						this.field_ChannelUserName.set("readOnly", true);
						this.field_ChannelUserPassword.set("readOnly", true);
						this.field_SSLCipherSpec.set("readOnly", true);
		    		} else {
						this.field_QueueManagerName.set("readOnly", false);
						this.field_ConnectionName.set("readOnly", false);
						this.field_ChannelName.set("readOnly", false);
						this.field_ChannelUserName.set("readOnly", false);
						this.field_ChannelUserPassword.set("readOnly", false);
						this.field_SSLCipherSpec.set("readOnly", false);
		    		}
		    	}
		    }
		    
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, position, onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var values = {};
			var obj = store.get(position);
			if (!obj) {
				obj = {};
			}
			var recordChanged = false;
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
				    if (actualProp !== "ChannelUserPassword") {
						values[actualProp] = this._getFieldValue(actualProp);
						if (!this._startingValues || (values[actualProp] != this._startingValues[actualProp])) {
							obj[actualProp] = this._getFieldValue(actualProp);
							recordChanged = true;
						}
				    } else {
                    	var pwval = this._getFieldValue("ChannelUserPassword");
                    	if (this.sendPassword ||((pwval !== "") && (pwval !== undefined))) {
                    		this.sendPassword = true;
							recordChanged = true;
							values["ChannelUserPassword"] = pwval ? pwval : "";
							obj["ChannelUserPassword"] = pwval ? pwval : "";
                    	}
                    }
				}
			}
			if (this.add || recordChanged) {
				onFinish(values,obj);
			} else {
				console.debug("nothing changed, so no need to save");
				this.dialog.hide();
			}
		},

		getTestConnectionProperties: function() {
			var connectionProperties = {};
			var excludeFields = ["field_id"];
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_" && excludeFields.indexOf(prop) < 0) {
					var actualProp = prop.substring(6);
					connectionProperties[actualProp] = this._getFieldValue(actualProp);
				}
			}
		    // Fields should only include ChannelUserPassword if the value has changed!
			this.checkChangedValues();
			if (!this.sendPassword) {
				delete connectionProperties.ChannelUserPassword;
			}
			
			connectionProperties["Verify"] = true;
			return connectionProperties;
		},

		checkChangedValues: function () {
			var changed = false;
			var values = {};
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && (prop.substring(0,6) == "field_") && (prop !== "field_id")) {
					var actualProp = prop.substring(6);
					// Only track changed values.  Omit unchanged values from update request.
				    if (actualProp !== "ChannelUserPassword") {
						if (this._getFieldValue(actualProp) != this._startingValues[actualProp]) {
							values[actualProp] = this._getFieldValue(actualProp);
							console.debug("Changed: " + actualProp);
							changed = true;
						} else {
					        // Include unchanged values except for "ChannelUserPassword" because
					        // the correct value might not be known or available to the current user.
					        values[actualProp] = this._getFieldValue(actualProp);
						}
				    } else {
                        if (actualProp == "ChannelUserPassword") {
                        	var pwval = this._getFieldValue("ChannelUserPassword");
                        	if (this.sendPassword || ((pwval !== "") && (pwval !== undefined))) {
                            	this.sendPassword = true;
                            	changed = true;
                            	values["ChannelUserPassword"] = pwval;
                        	}
                        }
				    }
				}
			}
			if (changed) {
				return values;
			}
			return undefined;
		},
		
		resetSuccessOrFailMessage: function() {
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			if (bar) {
				if (iString.endsWith(bar.innerHTML, nls.messaging.testConnectionSuccess) || 
					iString.endsWith(bar.innerHTML, nls.messaging.testConnectionFailed)) {
				        bar.innerHTML = "";
				}
			}
			this.toggleChannelUserRequired();
		},

		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");

			//console.debug("setting field_"+prop+" to '"+value+"'");
			switch(prop) {
			case "ChannelUserPassword":
				if ((value !== undefined) && (value !== "")) {
					// Reuse bindPasswoerdHint value for LDAP to avoid the need to update PII files after final drop
				    this["field_ChannelUserPassword"].set("placeHolder",nls.bindPasswordHint);
				} else {					
				    this["field_ChannelUserPassword"].set("value", "");						
				    this["field_ChannelUserPassword"].set("placeHolder", "");						
				}
				break;
			default:
				if (this["field_"+prop] && this["field_"+prop].set) {
					this["field_"+prop].set("value", value);
					this["field_"+prop].validate();
				} else {
					this["field_"+prop] = { value: value };
				}
				break;
			}

		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value;

			if (this["field_"+prop].get) {
				value = this["field_"+prop].get("value");
			} else {
				value = this["field_"+prop].value;
			}

			//console.debug("returning field_"+prop+" as '"+value+"'");
			return value;
		}

	});


	var QMConnectionProperties = declare("ism.controller.content.QMConnectionProperties", [_Widget, _TemplatedMixin], {
		// summary:
		//		Dialog tied to the Add button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds a function to
		//		save the result (adding a new record).  For input that is more complex than a text box,
		//		_getFieldValue is defined to get the value for different input types (e.g. checkbox).

		grid: null,
		store: null,
		editDialog: null,
		addDialog: null,
		removeDialog: null,
		contentId: null,

		addDialogTitle: Utils.escapeApostrophe(nls.messaging.connectionProperties.addDialog.title),
        editDialogTitle: Utils.escapeApostrophe(nls.messaging.connectionProperties.editDialog.title),
        removeDialogTitle: Utils.escapeApostrophe(nls.messaging.connectionProperties.removeDialog.title),
		
		addButton: null,
		deleteButton: null,
		editButton: null,
		testButton: null,
		testingDialog: null,
		
		isRefreshing: false,
		
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		domainUid: 0,
		
		orphanedTransactions: false, // whether the error related to orphaned transactions has just been thrown
	
		constructor: function(args) {
			dojo.safeMixin(this, args);
		
			this.contentId = this.id + "Content";
			console.debug("contentId="+this.contentId);
			this.store = new Observable(new Memory({data:[], idProperty: "Name"}));
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
		},

		_initStore: function() {
			// summary:
			// 		get the initial records from the server
	    	this.isRefreshing = true;
			this.store = new Observable(new Memory({data:[], idProperty: "id"}));
    		this.grid.setStore(this.store);			
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
			
			var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.getDefaultGetOptions();
            var promise = endpoint.doRequest("/configuration/QueueManagerConnection", options);
            
			var page = this;			
            
            promise.then(
            		function(data){
    			    	console.debug("connection properties rest GET returned");
    			    	page.isRefreshing = false;
    			    	page.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
    			    	var qmconnections = endpoint.getObjectsAsArray(data, "QueueManagerConnection");
    			    	if (qmconnections && qmconnections.length > 0) {
    			    		console.debug("success");
    			    		// add an id property with an encoded form of the name
    			    		array.map(qmconnections,function(item) {
    			    			item.id = escape(item.Name);
    			    		});
    			    		page.store = new Observable(new Memory({data: qmconnections, idProperty: "id"}));
    			    		page.grid.setStore(page.store);
    			    		page.triggerListeners();
    			    	} else {
    			    		console.debug("error");
    			    	}
    			    },
    			    function(error) {
    			    	console.debug("error: "+error);
    			    	page.isRefreshing = false;
    			    	page.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
    					Utils.showPageErrorFromPromise(nls.messaging.connectionProperties.retrieveError, error);
    			    }
            		);
            
		},
		
		_saveSettings: function(values, dialog, onFinish, onError) {
            console.debug("Saving...");
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
			var ret = false;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("QueueManagerConnection", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(
            		function(data){
    		    		console.debug("success");
    		    		if (onFinish)
    		    		    onFinish(data);
    			    },
    			    function(error) {
    			    	console.debug("error: "+error);
    			    	dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
    			    	if (onError)
    			    	    onError(error);
    			    });
		},
		
		_updateEntry: function(entry, values, updatedialog, onFinish, onError) {
            console.debug("Saving... "+entry);
			var dialog = updatedialog.dialog;
			
            if (!updatedialog.sendPassword) {
            	delete values.ChannelUserPassword;
            }
            console.debug("orphanedTransactions = "+this.orphanedTransactions);
            if (this.orphanedTransactions) {
            	values.Force = true;
            }
			console.debug("data="+JSON.stringify(values));
			
			delete values.id;  // remove id since REST API doesn't expect it.
			var page = this;
			var ret = false;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("QueueManagerConnection", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(
            		function(data){
    			    	console.debug("connection properties rest PUT returned");
    			    	onFinish();
            		},
            		function(error) {
    			    	console.debug("error: "+error);
    					var code = dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
    					console.debug("error code="+code);
    					if (code == "CWLNA0307") { // orphaned transactions
    						console.debug("Error CWLNA0307 thrown for orphaned transactions");
    						page.orphanedTransactions = true;
    					}
    					onError();
            		}
            		);
		},
		
		_deleteEntry: function(id, name, dialog, onFinish, onError) {
			console.debug("delete: "+name);
			
			var queryParams = "";
			console.debug("orphanedTransactions = "+this.orphanedTransactions);
            if (this.orphanedTransactions) {
            	queryParams = "?Force=true";
            }
			console.debug("queryParams="+queryParams);
			
			var page = this;
			
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultDeleteOptions();
            var promise = adminEndpoint.doRequest("/configuration/QueueManagerConnection/" + encodeURIComponent(name), options);
			promise.then(function() {
				page.store.remove(id);
				if (onFinish)
					onFinish();
			}, function(error) {
				console.error(error);
				var code = dialog.showErrorFromPromise(nls.messaging.deletingFailed, error);
				console.debug("error code="+code);
				if (code == "CWLNA0307") { // orphaned transactions
					console.debug("Error CWLNA0307 thrown for orphaned transactions");
					page.orphanedTransactions = true;
				}
				if (onError)
					onError(error);
			});
		},
		
		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid
						
			// create Edit dialog
			console.debug("id="+this.id+" contentId="+this.contentId);
			console.debug("editDialogTitle="+this.editDialogTitle);
			var editTitle = dojo.string.substitute(this.editDialogTitle,{nls:nls});
			var editId = "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);
			
			this.editDialog = new QMConnectionDialog({dialogId: editId, dialogTitle: editTitle, sendPassword: false}, editId);
			this.editDialog.startup();

			// create Add dialog
			var addTitle = dojo.string.substitute(this.addDialogTitle,{nls:nls});
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new QMConnectionDialog({add: true, dialogId: addId, dialogTitle: addTitle, sendPassword: true },
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
			
			// create Remove not allowed dialog (remove not allowed when used by a mapping rule)
			var removeNotAllowedId = this.id+"_removeNotAllowedDialog";
			domConstruct.place("<div id='"+removeNotAllowedId+"'></div>", this.id);
			this.removeNotAllowedDialog = new Dialog({
				id: removeNotAllowedId,
				title: nls.messaging.connectionProperties.removeNotAllowedDialog.title,
				content: "<div style='display: table;'>" +
						"<img style='display: inline-block; float: left; margin: 0 10px 0 0;' src='css/images/warning24.png' alt='' />" +
						"<span style='display: table-cell; vertical-align:middle; word-wrap:break-word;'>" + 
						nls.messaging.connectionProperties.removeNotAllowedDialog.content + "</span><div>",
				closeButtonLabel: nls.messaging.connectionProperties.removeNotAllowedDialog.closeButton
			}, removeNotAllowedId);
			this.removeNotAllowedDialog.dialogId = removeNotAllowedId;
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
						{	id: "Name", name: nls.messaging.connectionProperties.grid.name, field: "Name", dataType: "string", width: "160px",
							decorator: Utils.nameDecorator
						},
						{	id: "QueueManagerName", name: nls.messaging.connectionProperties.grid.qmgr, field: "QueueManagerName", dataType: "string", width: "189px" },
						{	id: "ConnectionName", name: nls.messaging.connectionProperties.grid.connName, field: "ConnectionName", dataType: "string", width: "189px",
							decorator: function(list) {
								return list.split(",").join(",<br>\n");
							}
						},
						{	id: "ChannelName", name: nls.messaging.connectionProperties.grid.channel, field: "ChannelName", dataType: "string", width: "160px" },
						{	id: "ChannelUserName", name: nls.channelUser, field: "ChannelUserName", dataType: "string", width: "160px" },
						{	id: "SSLCipherSpec", name: nls.messaging.connectionProperties.grid.SSLCipherSpec, field: "SSLCipherSpec", dataType: "string", width: "160px" }
					];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
			
			this.testButton = new MenuItem({label: nls.messaging.dialog.testButton});
			GridFactory.addToolbarMenuItem(this.grid, this.testButton);
			this.testButton.set('disabled', true);

		},
		
		_getSelectedData: function() {
			// summary:
			// 		get the row data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).data();
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid and dialogs
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    // create array of existing Name so we don't clash
				var existingNames = this.store.query({}).map(
						function(item){ return item.Name; });
			    this.addDialog.show(existingNames);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var rowData = this._getSelectedData();
				
			    // show not allowed dialog if item is associated with a mapping rule
			    var destmap = dijit.byId("destmap");
			    if (destmap && destmap.store) {
			    	var connName = rowData.Name;
			    	var names = [];
			    	var count = destmap.store.query(function(dest){
			    			var qmc = dest.QueueManagerConnection.qmc + ",";
			    			if (qmc.indexOf(connName+",") != -1) {
			    				names.push("\"" + Utils.nameDecorator(dest.Name) + "\"");
			    				return true;
			    			}
			    			return false;
			    		}).length;
			    	if (count > 0) {		    		
			    		var nameList = names.join(", ");
			    		var content = "";
			    		if (count == 1) {
			    			content = lang.replace(nls.messaging.connectionProperties.removeNotAllowedDialog.contentOne,[nameList]);    			
			    		} else {
			    			content = lang.replace(nls.messaging.connectionProperties.removeNotAllowedDialog.contentMany,[nameList]);
			    		}
			    		this.removeNotAllowedDialog.set("content", "<div>" + content + "</div>");
			    		this.removeNotAllowedDialog.show();
			    		return;
			    	}
			    }
			    
				this.removeDialog.recordToRemove = rowData.id;
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				this.orphanedTransactions = false;
				this.removeDialog.show();
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
			
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
					var name = this._getSelectedData().Name;
					var id = this.grid.select.row.getSelected()[0];
					this.editDialog.save(this.store, id, lang.hitch(this, function(data, values) {
						console.debug("edit is done, returned:", data);
						var bar = query(".idxDialogActionBarLead",this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
						var _this = this;
						this._updateEntry(name,data,this.editDialog, function() {
							console.debug("Saving:");
							// Mask ChannelUserPassword value if set
							if (values.ChannelUserPassword && (values.ChannelUserPassword !== undefined) && (values.ChannelUserPassword !== "")) {
								values.ChannelUserPassword = "XXXXXX";
							}
							console.dir(values);
							values.id = escape(values.Name);
							if (values.Name != _this.editDialog._startingValues.Name) {
								console.debug("name changed, removing old name");
								_this.store.remove(id);
								_this.store.add(values);		
							} else {
								_this.store.put(values,{overwrite: true});					
							}
							_this.editDialog.dialog.hide();
							_this._refreshGrid();
							_this.triggerListeners();							
						},
						function(error) {
							console.debug("edit failed");
							bar.innerHTML = "";							
						});
					}));
				}
			}));
			
			dojo.subscribe(this.editDialog.dialogId + "_testButton", lang.hitch(this, function(message) {
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
					this._doTestConnection(this.editDialog, this.editDialog.dialogId + "_testButton");
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_testButton", lang.hitch(this, function(message) {
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {
					this._doTestConnection(this.addDialog, this.addDialog.dialogId + "_testButton");
				}
			}));
			
			on(this.testButton, "click", lang.hitch(this, function() {
			    console.log("clicked test connection button");
			    this._testSelectedRow();	    		
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
						this._saveSettings(data, this.addDialog.dialog, function() {
							data.id = escape(data.Name);
							// Mask ChannelUserPassword value if set
							if (data.ChannelUserPassword && (data.ChannelUserPassword !== undefined) && (data.ChannelUserPassword !== "")) {
								data.ChannelUserPassword = "XXXXXX";
							}
							when(_this.store.add(data),lang.hitch(this, function(){
								_this.addDialog.dialog.hide();
								_this.triggerListeners();								
							}));							
						},
						function(error) {
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
						_this.testButton.set('disabled', true);
					}
					_this.triggerListeners();
				},
				function(error) {
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
				this.deleteButton.set('disabled', false);
				this.testButton.set('disabled', false);
				this.editButton.set('disabled', false);
			}
		},
		
		_doEdit: function() {
			var rowData = this._getSelectedData();
			// create array of existing Name values except for the current one, so we don't clash
			var existingNames = this.store.query(function(item){ return item.Name != rowData.Name; }).map(
					function(item){ return item.Name; });
			// retrieve full object data
			var fullData = this.store.query(function(item){ return item.Name === rowData.Name; })[0];
			this.editDialog.fillValues(fullData);
			this.orphanedTransactions = false;
			this.editDialog.show(existingNames);
		},
		
		_doTestConnection: function(testConnectionDialog, testConnectionButtonId) {
			// any message on this topic means that we clicked "test connection" on the dialog
			var dialog = testConnectionDialog.dialog? testConnectionDialog.dialog : testConnectionDialog;
			dialog.clearErrorMessage();
			var button = dijit.byId(testConnectionButtonId);
			var connectionProperties =  {};
			if (testConnectionDialog.getTestConnectionProperties) {
				connectionProperties = testConnectionDialog.getTestConnectionProperties();
			} 
			if (button) {
				button.set('disabled', true);
			}
			var page = this;
			page.updateProgressIndicator("testing", dialog);
			var dialogCounter = testConnectionDialog.dialogCounter;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("QueueManagerConnection", connectionProperties);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(
            		function(data){
    		    		console.debug("success");
						if (button) {
							button.set('disabled', false);
						}
		    			page.updateProgressIndicator("success", dialog);
    			    },
    			    function(error) {
    			    	console.debug("error: "+error);
						page.updateProgressIndicator("failed", dialog);
    			    	dialog.showErrorFromPromise(nls.messaging.testConnectionFailed, error);
						if (button) {
							button.set('disabled', false);
						}
    			    }
    		);
		},
		
		_testSelectedRow: function() {
		    var rowData = this._getSelectedData();
		    var t = this;
    		if (!t.testingDialog) {
				var testingDialogId = "testingInfo"+this.id;
				domConstruct.place("<div id='"+testingDialogId+"'></div>", this.contentId);
				t.testingDialog = new Dialog({
					id: testingDialogId,
					title: nls.messaging.dialog.testButton,
					closeButtonLabel: nls.messaging.dialog.closeButton
				}, testingDialogId);
				t.testingDialog.dialogId = testingDialogId;
				t.testingDialog.dialogCounter = 0;
    		} 

    		t.testingDialog.clearErrorMessage();
    		t.testingDialog.set('content', "<div><img src=\"css/images/loading_wheel.gif\" alt=''/>&nbsp;" + 
	    			lang.replace(nls.messaging.dialog.testingContent, [rowData.Name]) + "</div>");
    		// prevent two tests at the same time
	    	this.testButton.set('disabled', true);
    		t.testingDialog.show();
			
			var connectionProperties = {};
            for (var field in rowData) {
				connectionProperties[field] = rowData[field];
			}
            connectionProperties["Verify"] = true;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("QueueManagerConnection", connectionProperties);
            var promise = adminEndpoint.doRequest("/configuration", options);
            
            promise.then(function(data){			    	
    			// test successful
    	    	t.testingDialog.set('content', "<div><img src=\"css/images/msgSuccess16.png\" alt=''/>&nbsp;" + 
    	    			lang.replace(nls.messaging.dialog.testingSuccessContent, [rowData.Name]) + "</div>");
	    		t.testButton.set('disabled', false);
		    },
		    function(error) {
				t.testingDialog.set('content', "<div></div>");
				t.testingDialog.showErrorFromPromise(lang.replace(nls.messaging.dialog.testingFailedContent, [rowData.Name]), error);
	    		t.testButton.set('disabled', false);
		    });	
		},
		
		updateProgressIndicator: function(state, dialog) {
			var bar = query(".idxDialogActionBarLead",dialog.domNode)[0];
			if (!bar) {
				return;
			}
			switch(state) {
			case "testing":
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.testing;				
				break;
			case "success":
				bar.innerHTML = "<img src='css/images/msgSuccess16.png' width='10' height='10' alt=''/> " + nls.messaging.testConnectionSuccess;
				break;
			case "failed":
				bar.innerHTML = "<img src='css/images/msgError16.png' width='10' height='10' alt=''/> " + nls.messaging.testConnectionFailed;
				break;
			default:
				bar.innerHTML = "";
				break;
			}
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

	return QMConnectionProperties;
});
