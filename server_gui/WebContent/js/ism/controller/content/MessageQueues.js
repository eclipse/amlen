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
		'dojo/store/Observable',
		'dojo/date/locale',
		'dojo/when',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'dijit/form/Form',
		'ism/widgets/Dialog',
		'ism/widgets/CheckBoxList',
		'ism/widgets/GridFactory',
		'ism/widgets/TextBox',
		'ism/widgets/Select',
		'ism/IsmUtils',
		'ism/config/Resources',
		'dojo/text!ism/controller/content/templates/MessageQueuesDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'dojo/keys'
		], function(declare, lang, array, aspect, number, query, on, domClass, domConstruct, domStyle, 
				Memory, Observable, locale, when, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, Form, Dialog, CheckBoxList, GridFactory,
				TextBox, Select, Utils, Resources, msgQueueDialog, nls, nls_strings, keys) {

	/*
	 * This file defines a configuration widget that combines a grid with toolbar and a set of dialogs for editing.  All
	 * Dialog widgets are defined inline (rather than in their own file) for simplicity, but could easily be separated
	 * and referenced from an external file.
	 */

	var EditDialog = declare("ism.controller.content.MessageQueues.EditDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
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
		templateString: msgQueueDialog,

		// template strings
        dialogInstruction: Utils.escapeApostrophe(nls.messaging.queues.dialog.instruction),
        nameLabel: Utils.escapeApostrophe(nls.messaging.queues.grid.name),
        nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.messaging.queues.grid.description),
        maxMessagesLabel: Utils.escapeApostrophe(nls.messaging.queues.grid.maxMessages),
        maxMessagesTooltip: Utils.escapeApostrophe(nls.messaging.queues.dialog.maxMessagesTooltip),
        maxMessagesInvalid: Utils.escapeApostrophe(nls.messaging.queues.dialog.maxMessagesInvalid),      

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: [],

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			if (Utils.configObjName.editEnabled()) {			
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
				var max = 20000000;
				if (isNaN(intValue) || intValue < 1 || intValue > max) {
					return false;
				}
				return true;
			};
			
			var t = this;
			// set up onChange for description
			t.field_Description.on("change", function(args) {
				t.dialog.resize();
			}, true);

			
			// watch the form for changes between valid and invalid states
			dijit.byId(this.dialogId+"_form").watch('state', lang.hitch(this, function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(this.dialogId+"_saveButton");
				if (newvalue && newvalue == "Error") {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			}));
			
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				dijit.byId(this.dialogId+"_form").reset();
			}));
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			for (var prop in values) {
				this._startingValues[prop] = values[prop];
				this._setFieldValue(prop, values[prop]);
			}
		},
		
		show: function(list) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Name.existingNames = list;
			this.dialog.show();
			this.field_Description.focus(); // needed to get the text area to resize
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, position, onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var values = {};
			var obj = store.get(position);
			var recordChanged = false;
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (values[actualProp] != this._startingValues[actualProp]) {
						recordChanged = true;
						obj[actualProp] = values[actualProp];
					}
				}
			}

			if (recordChanged) {
				onFinish(values,obj);
			} else {
				console.debug("nothing changed, so no need to save");
				this.dialog.hide();
			}
		},

		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");
			switch (prop) {
				case "AllowSend":
				case "ConcurrentConsumers":
					if (value === true) {
						//console.debug("setting field_"+prop+" to 'on'");
						this["field_"+prop].set("checked", true);
					} else {
						this["field_"+prop].set("checked", false);
					}
					break;
				case "MaxMessages":
					this["field_MaxMessages"]._setValueAttr(number.format(value, {places: 0, locale: ism.user.localeInfo.localeString}));					
					break;	
				default:
					//console.debug("setting field_"+prop+" to '"+value+"'");
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
				case "AllowSend":
				case "ConcurrentConsumers":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "MaxMessages":
					value = number.parse(this["field_MaxMessages"].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});					
					break;	
				default:
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					break;
			}
			console.debug("returning field_"+prop+" as '"+value+"'");
			return value;
		}
	});

	var AddDialog = declare("ism.controller.content.MessageQueues.AddDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds a function to
		//		save the result (adding a new record).  For input that is more complex than a text box,
		//		_getFieldValue is defined to get the value for different input types (e.g. checkbox).
		nls: nls,
        nls_strings: nls_strings,
		templateString: msgQueueDialog,

        // template strings
		dialogInstruction: Utils.escapeApostrophe(nls.messaging.queues.dialog.instruction),
        nameLabel: Utils.escapeApostrophe(nls.messaging.queues.grid.name),
        nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.messaging.queues.grid.description),
        maxMessagesLabel: Utils.escapeApostrophe(nls.messaging.queues.grid.maxMessages),
        maxMessagesTooltip: Utils.escapeApostrophe(nls.messaging.queues.dialog.maxMessagesTooltip),
        maxMessagesInvalid: Utils.escapeApostrophe(nls.messaging.queues.dialog.maxMessagesInvalid),      

        action: "messageQueueAdd",

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
					
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
	
		postCreate: function() {

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
			
			var t = this;
			// set up onChange for description
			t.field_Description.on("change", function(args) {
				t.dialog.resize();
			}, true);

			
			// watch the form for changes between valid and invalid states
			dijit.byId(this.dialogId+"_form").watch('state', lang.hitch(this,function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(this.dialogId+"_saveButton");
				if (newvalue && newvalue == "Error") {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			}));
			
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				dijit.byId(this.dialogId+"_form").reset();
			}));
		},
		
		show: function(list) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Name.existingNames = list;
			this.field_Name.set("value", "");
			this.field_Description.set("value", "");
			this.field_AllowSend.set("value",true);
			this.field_MaxMessages.set("value", 5000); // set default value
			this.field_ConcurrentConsumers.set("value",true);
			dijit.byId(this.dialogId+"_saveButton").set('disabled',false);
			this.dialog.show();
			this.field_Description.focus(); // needed to get the text area to resize
		    this.field_Name.focus();
		},
		
		// save the record
		save: function(store, onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var values = {};
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
				}
			}
			console.debug("Saving: ", values);
			onFinish(values);
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value;
			switch (prop) {
				case "AllowSend":
				case "ConcurrentConsumers":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "MaxMessages":
					value = number.parse(this["field_MaxMessages"].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});					
					break;	
				default:
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					break;
			}
			console.debug("returning field_"+prop+" as '"+value+"'");
			return value;
		}
	});

	var MessageQueues = declare("ism.controller.content.MessageQueues", [_Widget, _TemplatedMixin], {
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
    
		addDialogTitle: Utils.escapeApostrophe(nls.messaging.queues.addDialog.title),
	    editDialogTitle: Utils.escapeApostrophe(nls.messaging.queues.editDialog.title),
	    removeDialogTitle: Utils.escapeApostrophe(nls.messaging.queues.removeDialog.title),
	    
		addButton: null,
		deleteButton: null,
		editButton: null,
		
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		messagesRemaining: false, // whether the error related to messages on queue has just been thrown
		
		domainUid: 0,
	
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
			dojo.subscribe(Resources.pages.messaging.nav.messagequeues.topic, lang.hitch(this, function(message){
				this._initStore();
			}));	
			dojo.subscribe(Resources.pages.messaging.nav.messagequeues.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this._refreshGrid();
				
				if (this.isRefreshing) {
					this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
				}
			}));	
						
			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();

		},

		_initStore: function() {
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
	    	this.isRefreshing = true;
	    	
			// summary:
			// 		get the initial records from the server
			var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/Queue", adminEndpoint.getDefaultGetOptions());
            promise.then(
                function(data) {
                    _this._populateQueues(data);
                },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                        _this._populateQueues({"Queue" : {}});
                    } else {
                        _this.isRefreshing = false;
                        _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
                        Utils.showPageErrorFromPromise(nls.messaging.queues.retrieveError, error);
                    }
                }
            );          
		},
		
		_populateQueues: function(data) {
		    var _this = this;
		    _this.isRefreshing = false;
		    _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
		    var adminEndpoint = ism.user.getAdminEndpoint();
		    var queues = adminEndpoint.getObjectsAsArray(data, "Queue");
		    // add an id property with an encoded form of the Name
		    array.map(queues,function(item) {
		        Utils.convertStringsToBooleans(item, [{property: "AllowSend", defaultValue: true},
		                                              {property: "ConcurrentConsumers", defaultValue: true}]);
		        item.id = escape(item.Name);
		    });
		    _this.store = new Observable(new Memory({data: queues, idProperty: "id"}));
		    _this.grid.setStore(_this.store);
		    _this.triggerListeners();
		},
		
		_saveSettings: function(values, dialog, onFinish) {
            console.debug("Saving...");
			var sendValues = {};
			for(var prop in values) {
				if(prop !== "id") {
					sendValues[prop] = values[prop];
				}
			}
			console.debug("data="+JSON.stringify(sendValues));
			
			var _this = this;
			
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("Queue", sendValues);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(
					function(data) {
					    console.debug("success");
						onFinish(values);
					},
					function(error) {
					    query(".idxDialogActionBarLead", dialog.domNode)[0].innerHTML = "";
		                dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
					}
			);
			return;
		},
		
		_deleteEntry: function(id, Name, dialog, onFinish) {
			console.debug("delete: "+Name);
			
			var queryParams = "";
			console.debug("messagesRemaining = "+this.messagesRemaining);
            if (this.messagesRemaining) {
            	queryParams = "?DiscardMessages=true";
            }
			console.debug("queryParams="+queryParams);

			var _this = this;	        
			var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.getDefaultDeleteOptions();
            var promise = endpoint.doRequest("/configuration/Queue/" + encodeURIComponent(Name) + queryParams, options);
            promise.then(
            		function(data) {
            			onFinish(id);
            		},
            		function(error) {
            		    console.error(error);
            		    query(".idxDialogActionBarLead", dialog.domNode)[0].innerHTML = "";
                        dialog.showErrorFromPromise(nls.messaging.deletingFailed, error);
				        var code = Utils.getErrorCodeFromPromise(error);
				        console.debug("error code="+code);
				        if (code == "CWLNA0209") { // messages remaining
					        console.debug("Error CWLNA0209 thrown for messages remaining");
					        _this.messagesRemaining = true;
            		    }
            		});
			return;
		},
		
		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid
						
			// create Edit dialog
			console.debug("id="+this.id+" contentId="+this.contentId);
			console.debug("editDialogTitle="+this.editDialogTitle);
			var editTitle = dojo.string.substitute(this.editDialogTitle,{nls:nls});
			var editId = "edit"+this.id+"Dialog";
			var editDialogNode = editId + "Node";
			domConstruct.place("<div id='"+editDialogNode+"'></div>", this.contentId);
			
			this.editDialog = new EditDialog({dialogId: editId, dialogTitle: editTitle }, editDialogNode);
			this.editDialog.startup();

			// create Add dialog
			var addTitle = dojo.string.substitute(this.addDialogTitle,{nls:nls});
			var addId = "add"+this.id+"Dialog";
			var addDialogNode = addId+"Node";
			domConstruct.place("<div id='"+addDialogNode+"'></div>", this.contentId);
			this.addDialog = new AddDialog({dialogId: addId, dialogTitle: addTitle }, addDialogNode);
			this.addDialog.startup();
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = dojo.string.substitute(this.removeDialogTitle,{nls:nls});
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + nls.messaging.queues.removeDialog.content + "</div>",
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
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
						{	id: "Name", name: nls.messaging.queues.grid.name, field: "Name", width: "160px",
							decorator: Utils.nameDecorator
						},
						{	id: "AllowSend", name: nls.messaging.queues.grid.allowSend, field: "AllowSend", width: "60px", 
	                        formatter: function(data) {
	                            return Utils.booleanDecorator(data.AllowSend);
	                        }                         
						},
						{	id: "MaxMessages", name: nls.messaging.queues.grid.maxMessages, field: "MaxMessages", width: "100px",
							formatter: function(data) {								
								if (data.MaxMessages) {
									return number.format(data.MaxMessages, {places: 0, locale: ism.user.localeInfo.localeString});
								}
							}
						},
						{	id: "ConcurrentConsumers", name: nls.messaging.queues.grid.concurrentConsumers, field: "ConcurrentConsumers", width: "60px", 
	                        formatter: function(data) {
	                            return Utils.booleanDecorator(data.ConcurrentConsumers);
	                        }                         
						},
						{	id: "Description", name: nls.messaging.queues.grid.description, field: "Description", width: "240px",
							decorator: Utils.textDecorator }
					];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
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
			// 		init events connecting the grid and dialogs			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    // create array of existing userids so we don't clash
				var existingNames = this.store.query({}).map(
						function(item){ return item.Name; });
			    this.addDialog.show(existingNames);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				console.log("selected: "+this.grid.select.row.getSelected());
				this.removeDialog.recordToRemove = this._getSelectedData()["id"];
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				this.messagesRemaining = false;
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
				    var _this = this;
					var name = _this._getSelectedData().Name;
					var id = _this.grid.select.row.getSelected()[0];
					_this.editDialog.save(_this.store, id, function(data) {
						console.debug("edit is done, returned:", data);
						var bar = query(".idxDialogActionBarLead",_this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
						console.debug("Saving... "+name);
						_this._saveSettings(data,_this.editDialog.dialog, function(data) { /* onFinish */
							data.id = escape(data.Name);
							if (data.Name != _this.editDialog._startingValues.Name) {
								console.debug("name changed, removing old name");
								_this.store.remove(id);
								_this.store.add(data);		
							} else {
								_this.store.put(data,{overwrite: true});					
							}
							_this.editDialog.dialog.hide();
							_this._refreshGrid();
							_this.triggerListeners();
						});
					});
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving addDialog");
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {
					var _this = this;
					var bar = query(".idxDialogActionBarLead",_this.addDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
					_this.addDialog.save(_this.store, function(data) { /* onFinish */
						console.debug("add is done, returned:", data);
						_this._saveSettings(data, _this.addDialog.dialog, function(data) {
							data.id = escape(data.Name);
							when(_this.store.add(data), function(){
								_this.addDialog.dialog.hide();
								_this.triggerListeners();								
							});
						});
					});
				}
			}));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				console.debug("remove is done");
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.deletingProgress;
				var _this = this;
				var id = _this.grid.select.row.getSelected()[0];
				var name = _this._getSelectedData().Name;
				_this._deleteEntry(id, name, _this.removeDialog, function(id) {
					_this.store.remove(id);
					_this.removeDialog.hide();
					// disable buttons if empty or no selection
					if (_this.store.data.length === 0 ||
							_this.grid.select.row.getSelected().length === 0) {
						_this.editButton.set('disabled', true);
						_this.deleteButton.set('disabled', true);
					}
					_this.triggerListeners();
				});
			}));
		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				console.debug("index="+ind);
				console.log("row count: "+this.grid.rowCount());
				this.deleteButton.set('disabled', false);
				this.editButton.set('disabled', false);
			}
		},
		
		_doEdit: function() {
			var rowData = this._getSelectedData();
			// create array of existing userids except for the current one, so we don't clash
			var existingNames = this.store.query(function(item){ return item.userid != rowData.userid; }).map(
					function(item){ return item.userid; });
			this.editDialog.fillValues(rowData);
			this.editDialog.show(existingNames);
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

	return MessageQueues;
});
