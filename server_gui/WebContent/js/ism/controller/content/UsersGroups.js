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
		'dojo/aspect',
		'dojo/query',
		'dojo/_base/array',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dojo/when',
		'dojox/html/entities',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/MenuItem',
		'dijit/form/Button',
		'dijit/form/Form',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/widgets/CheckBoxSelect',
		'ism/widgets/TextBox',
		'ism/IsmUtils',
		'ism/config/Resources',
		'dojo/text!ism/controller/content/templates/UserDialog.html',
		'dojo/text!ism/controller/content/templates/ResetPasswordDialog.html',
		'dojo/text!ism/controller/content/templates/ChangePasswordDialog.html',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/strings',
		'ism/widgets/IsmQueryEngine',
		'dojo/keys'
		], function(declare, lang, aspect, query, array, on, domClass, domConstruct, domStyle, 
				Memory, when, entities, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				MenuItem, Button, Form, GridFactory, Dialog, CheckBoxSelect, TextBox, Utils, Resources,
				userDialog, resetPasswordDialog, changePasswordDialog, nls, nls_strings, IsmQueryEngine, keys) {

	/*
	 * This file defines a configuration widget that combines a grid, toolbar, and set of dialogs for editing.  All
	 * Dialog widgets are defined inline (rather than in their own file) for simplicity, but could easily be separated
	 * and referenced from an external file.
	 */

	var EditUserDialog = declare("ism.controller.content.UsersGroups.EditUserDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
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
		templateString: userDialog,

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: null,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			if (this.hidepasswords) {
				this.field_passwd1.domNode.style.display = 'none';
				this.field_passwd2.domNode.style.display = 'none';
				this.field_passwd1.set('required',false);
				this.field_passwd2.set('required',false);
			}
			
			// check that the userid matches the pattern and doesn't clash with an existing object
			this.field_id.validChars = this.validChars;
			this.field_id.tooltipContent = this.tooltipContent;
			this.field_id.invalidMessage = this.invalidMessage;
			this.field_id.isValid = function(focused) {
				var value = this.get("value");
				if (!value || value === ""){
					return false;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				var match = new RegExp("^(?:"+this.validChars+")$").test(value);
				var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
				return unique && match && spaces;
			};
			this.field_id.getErrorMessage = function() {
				var value = this.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				return unique ? this.invalidMessage : nls.appliance.invalidName;
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
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values, list) {
			this.field_id.existingNames = list;
			dijit.byId(this.dialogId+"_form").reset();
			
			this._startingValues = values;
			for (var prop in values) {
				if (prop != "groups") {
					this._setFieldValue(prop, values[prop]);
				}
			}
		},
		
		show: function(values, isGroupGrid, userGroupWidget) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_passwd1.set('required',false);
			this.field_passwd2.set('required',false);
			this.field_passwd1.set("value", "");
			this.field_passwd2.set("value", "");

			var isSpecialUser = false;
			
			if (this.specialuser !== undefined && (this._getFieldValue('id') == this.specialuser)) {
				isSpecialUser = true;
			}

			this.field_groups.setRequiredOnOff(this.fixedgroups && !isSpecialUser);
			
			// cannot edit the id or groups for the special user, if there is one
			this.field_id.set('readOnly', isSpecialUser);
			this.field_groups.set('readOnly', isSpecialUser);
			
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.users.dialog.refreshingGroupsLabel;
			this.field_groups.set("disabled", true);
			userGroupWidget._getGroupNames(userGroupWidget, lang.hitch(this, function(groupNames) {
				var newStore = new dojo.data.ItemFileWriteStore({
					data: {
						label: "label",
						identifier: "name",
						items: []
					}
				});
				var len = groupNames ? groupNames.length : 0;
				for (var i=0; i < len; i++) {
					if (this.isGroupGrid) {
						if (groupNames[i] != values.id) {
							continue;
						}
					}
					newStore.newItem({ name: groupNames[i], label: dojox.html.entities.encode(groupNames[i]) });
				}
				newStore.save();
				this.field_groups.setStore(newStore);

				var groupCopy = lang.clone(values.groups);
				this.field_groups.set("value", groupCopy);
				
				this.field_groups.set("disabled", false);
				var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
				bar.innerHTML = "";
			}));
			
			dijit.byId(this.dialogId+"_form").validate();
			this.dialog.show();
		    this.field_id.focus();
		},
				
		getId : function() {
			return this._startingValues.id;
		},
		
		getValues: function() {
			var values = {};
			var recordChanged = false;
			var updates = {};
			for (var prop in this._startingValues) {
				values[prop] = this._getFieldValue(prop);
				if (values[prop] != this._startingValues[prop]) {
					recordChanged = true;
					updates[prop] = values[prop];
				}
			}
			if (recordChanged) {
				return updates;
			}
			return undefined;
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, position, groups) {
			var values = {}; 
			var recordChanged = false;
			var updates = {};
			var obj = store.get(position);
			for (var prop in this._startingValues) {
				values[prop] = this._getFieldValue(prop); 
				if (prop == "groups" && groups && groups != values[prop]) {
					values[prop] = groups;
					recordChanged = true;
				}
				if (values[prop] != this._startingValues[prop]) {
					recordChanged = true;
					updates[prop] = values[prop];
					obj[prop] = values[prop];
				}
			}
			if (recordChanged) {
				console.debug("Saving:", values);
				store.put(obj,{overwrite: true});					
			} else {
				console.debug("nothing changed, so no need to save");
			}
		},
		
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");
			switch (prop) {
				case "enabled":
					if (value === true) {
						//console.debug("setting field_"+prop+" to 'on'");
						this["field_"+prop].set("checked", "true");
					} 
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
				case "enabled":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
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
		}
	});

	var AddUserDialog = declare("ism.controller.content.UsersGroups.AddUserDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds a function to
		//		save the result (adding a new record).  For input that is more complex than a text box,
		//		_getFieldValue is defined to get the value for different input types (e.g. checkbox).
		nls: nls,
		templateString: userDialog,
		saveButtonValidate: false,

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
	
		postCreate: function() {
			console.debug(this.declaredClass + ".postCreate()");
			
			if (this.hidepasswords) {
				this.field_passwd1.domNode.style.display = 'none';
				this.field_passwd2.domNode.style.display = 'none';
				// disable and validate to clear Incomplete state
				this.field_passwd1.set('disabled',true); 
				this.field_passwd1.validate();
				this.field_passwd2.set('disabled',true);
				this.field_passwd2.validate();
			}
			else {
				// set validator to check that the passwords match
				this.field_passwd1.runValidate = true;
				this.field_passwd2.runValidate = true;
				this.field_passwd1.hasInput = false;
				this.field_passwd2.hasInput = false;
				this.field_passwd1.forceFocusValidate = false;
				this.field_passwd2.forceFocusValidate = false;
			
				this.field_passwd1.validator = lang.hitch(this,function() {
					var value = this.field_passwd1.get("value");
					if (value && value !== "") {
						this.field_passwd1.hasInput = true;
					}
					var p2Value = this.field_passwd2.get("value");
					if ((p2Value && p2Value !== "") || this.saveButtonValidate || this.field_passwd1.forceFocusValidate){
						if (!value || value === ""){
							return false;
						}
						var passwordMatch = (p2Value == value);
						var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
						if (!spaces) {
							return false;
						}
						var valid = passwordMatch && spaces; 
						if (passwordMatch) {
							this.field_passwd1.runValidate = false;
							if (this.field_passwd2.runValidate) {
								dijit.byId(this.dialogId+"_confirmPassword").validate(true);
							}
							this.field_passwd2.runValidate = true;
							this.field_passwd1.runValidate = true;
						}
						if (this.field_passwd2.get("value") === "") {
							if (valid) {
								dijit.byId(this.dialogId+"_confirmPassword").validate(true);
							}
							var button = dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
							valid = true;
						}
						return valid;
					}
					return !this.field_passwd2.hasInput;
				});
				this.field_passwd1.getErrorMessage = lang.hitch(this, function() {
					var value = this.field_passwd1.get("value");
					if (!value || value === ""){
						return nls.appliance.invalidRequired;
					}
					var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
					if (!spaces) {
						return nls.appliance.users.dialog.passwordNoSpaces;
					}
					return nls.appliance.users.dialog.password2Invalid;
				});
			
				this.field_passwd2.validator = lang.hitch(this,function() {
					var value = this.field_passwd2.get("value");
					if (value && value !== "") {
						this.field_passwd2.hasInput = true;
					}
					var p1Value = this.field_passwd1.get("value");
					if ((p1Value && p1Value !== "") || this.saveButtonValidate || this.field_passwd2.forceFocusValidate){
						if (!value || value === ""){
							return false;
						}
						var passwordMatch = (p1Value == value);
						var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
						if (!spaces) {
							return false;
						}
						var valid = passwordMatch && spaces; 
						if (passwordMatch) {
							this.field_passwd2.runValidate = false;
							if (this.field_passwd1.runValidate) {
								dijit.byId(this.dialogId+"_password").validate(true);
							}
							this.field_passwd1.runValidate = true;
							this.field_passwd2.runValidate = true;
						}
						if (this.field_passwd1.get("value") === "") {
							if (valid) {
								dijit.byId(this.dialogId+"_password").validate(true);
							}
							var button = dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
							valid = true;
						}
						return valid;
					}
					return !this.field_passwd1.hasInput;
				});
				this.field_passwd2.getErrorMessage = lang.hitch(this, function() {
					var value = this.field_passwd2.get("value");
					if (!value || value === ""){
						return nls.appliance.invalidRequired;
					}
					var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
					if (!spaces) {
						return nls.appliance.users.dialog.passwordNoSpaces;
					}
					return nls.appliance.users.dialog.password2Invalid;
				});
			}

			// check that the userid matches the pattern and doesn't clash with an existing object
			this.field_id.validChars = this.validChars;
			this.field_id.tooltipContent = this.tooltipContent;
			this.field_id.invalidMessage = this.invalidMessage;
			this.field_id.isValid = function(focused) {
				var value = this.get("value");
				if (!value || value === ""){
					return false;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				var match = new RegExp("^(?:"+this.validChars+")$").test(value);
				var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
				return unique && match && spaces;				
			};
			this.field_id.getErrorMessage = function() {
				var value = this.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				return unique ? this.invalidMessage : nls.appliance.invalidName;
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
			
			on(this.field_passwd1, "focus", lang.hitch(this, function() {
				this.field_passwd1.forceFocusValidate = true;
			}));
			
			on(this.field_passwd2, "focus", lang.hitch(this, function() {
				this.field_passwd2.forceFocusValidate = true;
			}));
		},
		
		show: function(list, userGroupWidget) {
			// start with save enabled
			dijit.byId(this.dialogId+"_form").reset();
			dijit.byId(this.dialogId+"_saveButton").set('disabled',false); 
			
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_id.existingNames = list;
			if (!this.hidepasswords) {
				this.field_passwd1.set('required',true);
				this.field_passwd2.set('required',true);
				this.field_passwd1.set("value", "");
				this.field_passwd2.set("value", "");
				this.field_passwd1.hasInput = false;
				this.field_passwd2.hasInput = false;
				this.field_passwd1.forceFocusValidate = false;
				this.field_passwd2.forceFocusValidate = false;
			}
			this.field_id.set("value", "");
			this.field_description.set("value", "");
						
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.users.dialog.refreshingGroupsLabel;
			this.field_groups.set("disabled", true);
			userGroupWidget._getGroupNames(userGroupWidget, lang.hitch(this, function(groupNames) {
				var newStore = new dojo.data.ItemFileWriteStore({
					data: {
						label: "label",
						identifier: "name",
						items: []
					}
				});
				var len = groupNames ? groupNames.length : 0;				
				for (var i=0; i < len; i++) {
					newStore.newItem({ name: groupNames[i], label: dojox.html.entities.encode(groupNames[i]) });
				}
				newStore.save();
				this.field_groups.setStore(newStore);
				
				this.field_groups.set("disabled", false);
				var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
				bar.innerHTML = "";
			}));
			
			if (this.fixedgroups) {
				this.field_groups.set("required", true);
				this.field_groups.set("value", ["Users"]);		
			} else {
				Utils.alignWidgetWithRequired(this.field_groups);
			}

			dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
			this.dialog.show();
		    this.field_id.focus();
		},
		
		// save the record
		getValues: function() {
			var values = {};
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
				}
			}
			// send password only once
			values.password = values.passwd1;
			delete values.passwd1;
			delete values.passwd2;
			
			return values;
		},
		
		save: function(store, values, nextId, onadd) {
			console.debug("Saving at id "+ nextId +": ", values);
			when(store.add({gridId: nextId, id: values.id, description: values.description, groups: values.groups}, onadd()));
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value;
			switch (prop) {
				case "enabled":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
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
		}
	});

	var ResetPasswordDialog = declare("ism.controller.content.UsersGroups.ResetPasswordDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add button in the widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds a function to
		//		save the result (adding a new record).  For input that is more complex than a text box,
		//		_getFieldValue is defined to get the value for different input types (e.g. checkbox).
		nls: nls,
		templateString: resetPasswordDialog,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
	
		startup: function() {
			// set validator to check that the passwords match
			this.field_passwd1.runValidate = true;
			this.field_passwd2.runValidate = true;
			this.field_passwd1.forceFocusValidate = false;
			this.field_passwd2.forceFocusValidate = false;
			this.saveButtonValidate = false;

			this.field_passwd1.validator = lang.hitch(this,function() {
				var value = this.field_passwd1.get("value");
				if (value && value !== "") {
					this.field_passwd1.hasInput = true;
				}
				var p2Value = this.field_passwd2.get("value");
				if ((p2Value && p2Value !== "") || this.saveButtonValidate || this.field_passwd1.forceFocusValidate){
					if (!value || value === ""){
						return false;
					}
					var passwordMatch = (p2Value == value);
					var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
					if (!spaces) {
						return false;
					}
					var valid = passwordMatch && spaces; 
					if (passwordMatch) {
						this.field_passwd1.runValidate = false;
						if (this.field_passwd2.runValidate) {
							dijit.byId(this.dialogId+"_passwd2").validate(true);
						}
						this.field_passwd2.runValidate = true;
						this.field_passwd1.runValidate = true;
					}
					if (this.field_passwd2.get("value") === "") {
						if (valid) {
							dijit.byId(this.dialogId+"_passwd2").validate(true);
						}
						var button = dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
						valid = true;
					}
					return valid;
				}
				return !this.field_passwd2.hasInput;
			});
			this.field_passwd1.getErrorMessage = lang.hitch(this, function() {
				var value = this.field_passwd1.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
				if (!spaces) {
					return nls.appliance.users.dialog.passwordNoSpaces;
				}
				return nls.appliance.users.dialog.password2Invalid;
			});
			
			this.field_passwd2.validator = lang.hitch(this,function() {
				var value = this.field_passwd2.get("value");
				if (value && value !== "") {
					this.field_passwd2.hasInput = true;
				}
				var p1Value = this.field_passwd1.get("value");
				if ((p1Value && p1Value !== "") || this.saveButtonValidate || this.field_passwd2.forceFocusValidate){
					if (!value || value === ""){
						return false;
					}
					var passwordMatch = (p1Value == value);
					var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
					if (!spaces) {
						return false;
					}
					var valid = passwordMatch && spaces; 
					if (passwordMatch) {
						this.field_passwd2.runValidate = false;
						if (this.field_passwd1.runValidate) {
							dijit.byId(this.dialogId+"_passwd1").validate(true);
						}
						this.field_passwd1.runValidate = true;
						this.field_passwd2.runValidate = true;
					}
					if (this.field_passwd1.get("value") === "") {
						if (valid) {
							dijit.byId(this.dialogId+"_passwd1").validate(true);
						}
						var button = dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
						valid = true;
					}
					return valid;
				}
				return !this.field_passwd1.hasInput;
			});
			this.field_passwd2.getErrorMessage = lang.hitch(this, function() {
				var value = this.field_passwd2.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
				if (!spaces) {
					return nls.appliance.users.dialog.passwordNoSpaces;
				}
				return nls.appliance.users.dialog.password2Invalid;
			});
			
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
			
			on(this.field_passwd1, "focus", lang.hitch(this, function() {
				this.field_passwd1.forceFocusValidate = true;
			}));
			
			on(this.field_passwd2, "focus", lang.hitch(this, function() {
				this.field_passwd2.forceFocusValidate = true;
			}));
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			dijit.byId(this.dialogId+"_form").reset();
			this.field_id.set("value", values.id);
			this.field_passwd1.set("value", "");
			this.field_passwd2.set("value", "");
		},
		
		show: function() {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_passwd1.hasInput = false;
			this.field_passwd2.hasInput = false;
			this.field_passwd1.forceFocusValidate = false;
			this.field_passwd2.forceFocusValidate = false;
			dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
			this.dialog.show();
		    this.field_passwd1.focus();
		},
		
		// save the record
		save: function(onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var values = {};
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
				}
			}
			
			// send password only once
			values.password = values.passwd1;
			
			delete values.passwd1;
			delete values.passwd2;
			
			console.debug("Saving: ", values);
			onFinish(values);
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value;
			switch (prop) {
				case "enabled":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
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
		}
	});

	var UserGrid = declare("ism.controller.content.UsersGroups", [_Widget, _TemplatedMixin], {
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
		contentId: null,
		idName: null,
		idLength: 100,
		
		ldapEnabled: null,
		isMessagingWidget: false,
		isRefreshing: false,
		_refreshDeferred: undefined,
		
		addButton: null,
		deleteButton: null,
		editButton: null,
		resetButton: null,
		
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		     			
		constructor: function(args) {
			dojo.safeMixin(this, args);
		
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], queryEngine: this.queryEngine});
			
			if (this.id.indexOf("msg") === 0) {
				this.isMessagingWidget = true;
			}
			
			this.idName = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.itemName,{nls:nls})));
		},

		postCreate: function() {
			this.inherited(arguments);

			// start with an empty <div> for the Listeners content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";

			// when switching to this page, refresh the field values
			var topic = Resources.pages.webui.nav.users.topic;
			if (this.isMessagingWidget) {
				topic = Resources.pages.messaging.nav.userAdministration.topic;
			}
			dojo.subscribe(topic, lang.hitch(this, function(message){
				this._refreshDeferred = this._initStore();			
			}));
			dojo.subscribe(topic + ":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");
				// on page switch, wait for the initStore to finish then refresh grid
				if (this._refreshDeferred) {
					this._refreshDeferred.then(this._refreshGrid());					
				}
			}));							
			
			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();
		},

		_initStore: function() {
			// summary:
			// 		get the initial records from the server
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.users.grid.refreshingLabel;
			this.isRefreshing = true;
			
			var page = this;
			return dojo.xhrGet({
			    url: Utils.getBaseUri() + this.restURI,
			    handleAs: "json",
			    preventCache: true,
			    load: lang.hitch(this, function(data){
			    	console.debug("users rest GET returned", data);
			    	page.isRefreshing = false;
			    	page.grid.emptyNode.innerHTML = nls.appliance.users.grid.noEntryMessage;
			    	if (data && (data.status === 0)) {
			    		var mapped = dojo.map(data.data, function(item, index){
						    return {
						    	gridId: escape(item.id),
						    	id: item.id,
								description: item.description,
								groups: item.groups
						    };
						});
						console.dir(mapped);
			    		page.store = new Memory({data: mapped, idProperty: "gridId", queryEngine: this.queryEngine});
			    		page.grid.setStore(page.store);
			    		page.triggerListeners();
			    		
			    		if (this.ldapEnabled) {
			    			this.notifyLDAPEnablement(this.ldapEnabled);
			    		}
			    	} else {
			    		console.debug("error");
			    	}
			    }),
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);
			    }
			  });
		},
		
		_saveSettings: function(userid, values, dialog) {
            console.debug("Saving... "+userid);
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
			var ret = false;
			dojo.xhrPut({
			    url: Utils.getBaseUri() + this.restURI + "/" + encodeURIComponent(userid),
			    handleAs: "json",
			    headers: { 
					"Content-Type": "application/json"
				},
				postData: JSON.stringify(values),
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.debug("users rest PUT returned ", data);
			    	if (data && data.code && data.code == "CWLNA5056") {
			    		// update the value so the store is properly updated			    		
						values.groups = data.groups;
			    		Utils.showPageWarning(nls.appliance.users.returnCodes[data.code], data.message, data.code);
			    	}
			    	ret=true;
			    },
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);
					dialog.showXhrError(nls.appliance.savingFailed, error, ioargs);
			    	ret=false;
			    }
			  });
			return ret;
		},
		
		_createObject: function(values, dialog) {
            console.debug("Creating... ");
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
			var ret = false;
			dojo.xhrPost({
			    url: Utils.getBaseUri() + this.restURI,
			    handleAs: "json",
			    headers: { 
					"Content-Type": "application/json"
				},
				postData: JSON.stringify(values),
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.debug("users rest POST returned ", data);
			    	//console.debug("success");
			    	if (data && data.code && data.code == "CWLNA5056") {
			    		// update the value so the store is properly updated
						values.groups = data.groups;
			    		Utils.showPageWarning(nls.appliance.users.returnCodes[data.code], data.message, data.code);
			    	}
			    	ret=true;
			    },
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);
					dialog.showXhrError(nls.appliance.savingFailed, error, ioargs);
			    	ret=false;
			    }
			  });
			return ret;
		},

		_deleteObject: function(userid, dialog) {
            console.debug("Deleting... "+userid);
						
			var page = this;
			var ret = false;
			dojo.xhrDelete({
			    url: Utils.getBaseUri() + this.restURI+ "/" + encodeURIComponent(userid),
			    handleAs: "json",
			    headers: { 
					"Content-Type": "application/json"
				},
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.debug("users rest DEL returned "+data);
			    	//console.debug("success");
			    	ret=true;
			    },
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);
					dialog.showXhrError(nls.appliance.deletingFailed, error, ioargs);
			    	ret=false;
			    }
			  });
			return ret;
		},

		_resetPasswd: function(values, dialog) {
            console.debug("Password reset... ");
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
			var ret = false;
			dojo.xhrPost({
			    url: Utils.getBaseUri() + this.passwdURI,
			    handleAs: "json",
			    headers: { 
					"Content-Type": "application/json"
				},
				postData: JSON.stringify(values),
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.debug("passwd rest POST returned "+data);
			    	//console.debug("success");
			    	ret=true;
			    },
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);
					dialog.showXhrError(nls.appliance.savingFailed, error, ioargs);
			    	ret=false;
			    }
			  });
			return ret;
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar
			var tooltip = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.tooltipContent,{nls:nls})));
			var invalid = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.invalidMessage,{nls:nls})));
			var instruction = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.instruction,{nls:nls})));
			
			// create Edit dialog
			console.debug("id="+this.id+" contentId="+this.contentId);
			console.debug("editDialogTitle="+this.editDialogTitle);
			var editTitle = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.editDialogTitle,{nls:nls})));
			var editId = "edit"+this.id+"Dialog";
			var editIdNode = editId+"Node";
			domConstruct.place("<div id='"+editIdNode+"'></div>", this.contentId);
			
			this.editDialog = new EditUserDialog({dialogId: editId, dialogTitle: editTitle, validChars: this.validChars,
				idName: this.idName, hidepasswords: true, tooltipContent: tooltip, invalidMessage: invalid,
				fixedgroups: this.fixedgroups, specialuser: this.specialuser, instruction: instruction,
				idLength: this.idLength}, editIdNode);
			this.editDialog.startup();

			
			// create Add dialog
			var addTitle = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.addDialogTitle,{nls:nls})));
			var addId = "add"+this.id+"Dialog";
			var addDialogNode = addId+"Node";
			domConstruct.place("<div id='"+addDialogNode+"'></div>", this.contentId);
			this.addDialog = new AddUserDialog({dialogId: addId, dialogTitle: addTitle, validChars: this.validChars,
				idName: this.idName, hidepasswords: this.hidepasswords, tooltipContent: tooltip, invalidMessage: invalid, 
				fixedgroups: this.fixedgroups, instruction: instruction, idLength: this.idLength}, addDialogNode);
			this.addDialog.startup();

			// create Reset Password dialog
			if (!this.hidepasswords) {
				var resetId = "reset"+this.id+"Dialog";
				domConstruct.place("<div id='"+resetId+"'></div>", this.contentId);
				this.resetDialog = new ResetPasswordDialog({dialogId: resetId}, resetId);
				this.resetDialog.startup();
			}
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.removeDialogTitle,{nls:nls})));
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + nls.appliance.removeDialog.content + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_button_ok",
						label: nls.appliance.removeDialog.deleteButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.appliance.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			
			// create a place for the warning dialog
			domConstruct.place("<div id='"+this.id+"WarningDialog'></div>", this.contentId);

		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "id", name: this.idName, field: "id", dataType: "string", width: "160px",
						decorator: Utils.nameDecorator
					},
					{	id: "description", name: nls.appliance.users.grid.description, field: "description", dataType: "string", width: "200px",
						decorator: Utils.nameDecorator
					},
					{	id: "groups", name: nls.appliance.users.grid.groups, field: "groups", dataType: "array", width: "280px",
						decorator: function(cellData) {
							return array.map(cellData, Utils.nameDecorator).join(",<br>\n");
						}
					}
				];			
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, {paging: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
			
			if (!this.hidepasswords) {
				this.resetButton = new MenuItem({
					label: nls_strings.action.ResetPassword
				});
				GridFactory.addToolbarMenuItem(this.grid, this.resetButton);
				this.resetButton.set('disabled', true);
			}
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

		_getGroupNames: function(userGroupWidget, callback) {
			var groups = null;
			if (userGroupWidget.fixedgroups) {
				groups = [ "SystemAdministrators",
				         "MessagingAdministrators",
				         "Users"];
				callback(groups);
			} else {
				dojo.xhrGet({
				    url: Utils.getBaseUri() + userGroupWidget.groupsURI,
				    handleAs: "json",
				    preventCache: true,
				    load: function(data){
				    	console.debug("groups rest GET returned");
				    	if (data.data) {
				    		// extract just the group id and sort alphabetically
				    		groups = dojo.map(data.data, function(item, index){
								return item.id;
							});
				    		groups.sort();
				    		callback(groups);
				    	} else {
				    		console.debug("error");
				    	}
				    },
				    error: function(error, ioargs) {
				    	console.debug("error: "+error);
						Utils.checkXhrError(ioargs);
				    }
				  });
			}
			return groups;
		},
		
		_notifyGroupEdit: function(oldId, newId) {
			if ((oldId == newId) || (newId == undefined))  {
				return;
			}
			if (this.isGroupGrid) {
				dijit.byId(this.userGridID)._notifyGroupEdit(oldId, newId);
			}
			
			this.store.query(function(group){
				return group.groups.indexOf(oldId) > -1;
			}).forEach(lang.hitch(this, function(group){
				group.groups.splice(group.groups.indexOf(oldId), 1, newId);
				this.store.put(group,{overwrite: true});	
			}));
			
			this._refreshGrid();	
		},
		
		_notifyGroupDelete: function(oldId) {
			if (this.isGroupGrid) {
				dijit.byId(this.userGridID)._notifyGroupDelete(oldId);
			}
			
			this.store.query(function(group){
				return group.groups.indexOf(oldId) > -1;
			}).forEach(lang.hitch(this, function(group){
				group.groups.splice(group.groups.indexOf(oldId), 1);
				this.store.put(group,{overwrite: true});	
			}));
			
			this._refreshGrid();	
		},
		
		notifyLDAPEnablement: function(enabled) {
			console.debug("notifyLDAPEnablement", enabled);
			this.ldapEnabled = enabled;
			if (enabled) {
				this.grid.setStore(new Memory({data: []}));
				this._refreshGrid();
				this.grid.emptyNode.innerHTML = "<img src='css/images/msgInfo16.png' width='10' height='10' alt=''/> " + nls.appliance.users.grid.ldapEnabledMessage;
				
				this.addButton.set('disabled', true);
				this.editButton.set('disabled', true);
				this.deleteButton.set('disabled', true);
				if (!this.hidepasswords) {
					this.resetButton.set('disabled', true);
				}
			}
			else {
				this.grid.setStore(this.store);
				this._refreshGrid();
				if (this.isRefreshing) {
					this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.users.grid.refreshingLabel;
				}
				else {
					this.grid.emptyNode.innerHTML = nls.appliance.users.grid.noEntryMessage;
				}
				
				this.addButton.set('disabled', false);
				this.editButton.set('disabled', true);
				this.deleteButton.set('disabled', true);
				if (!this.hidepasswords) {
					this.resetButton.set('disabled', true);
				}
			}
		},
		
		_updateButtons: function() {
			console.log("selected: "+this.grid.select.row.getSelected());
			if (this.grid.select.row.getSelected().length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				console.debug("index="+ind);
				console.log("row count: "+this.grid.rowCount());
				var rowData = this._getSelectedData();
				var existingName = rowData["id"];
				this.editButton.set('disabled', false);
				if (this.specialuser &&
						((ism.user.displayName == existingName) || (existingName == this.specialuser))) {
					this.deleteButton.set('disabled', true);
				} else {
					this.deleteButton.set('disabled', false);
				}
				if (!this.hidepasswords) {
					this.resetButton.set('disabled', false);
				}
			} else {
				this.editButton.set('disabled',true);
				this.deleteButton.set('disabled',true);
				if (!this.hidepasswords) {
					this.resetButton.set('disabled', true);
				}
			}
		},
		
		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				this._updateButtons();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    // create array of existing ids so we don't clash
				var existingNames = this.store.query({}).map(
						function(item){ return item.id; });
			    this.addDialog.show(existingNames, this);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				console.log("selected: "+this.grid.select.row.getSelected());
				this.removeDialog.recordToRemove = this._getSelectedData()["id"];
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
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
					this._updateButtons();
				}
			}), true);
			
			if (!this.hidepasswords) {
				on(this.resetButton, "click", lang.hitch(this, function() {
					console.log("clicked reset password");
					var rowData = this._getSelectedData();
					this.resetDialog.fillValues(rowData);
					this.resetDialog.show();
				}));
			}
			
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
					var values = this.editDialog.getValues();
					console.debug("performing edit, dialog values:", values);
					if (values != undefined) { 
						var bar = query(".idxDialogActionBarLead",this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
						if (this._saveSettings(this.editDialog.getId(), values, this.editDialog.dialog)) {
							if (this.isGroupGrid) {
								var oldValues = this.store.get(this.grid.select.row.getSelected()[0]);
								this._notifyGroupEdit(oldValues.id, values.id);
							}
							this.editDialog.save(this.store, this.grid.select.row.getSelected()[0], values.groups);
							this.editDialog.dialog.hide();
							this._refreshGrid();
							this.triggerListeners();
						} else {
							console.debug("edit failed");
							bar.innerHTML = "";
						}
					}
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				this.addDialog.saveButtonValidate = true;
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {
					var bar = query(".idxDialogActionBarLead",this.addDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
					var values = this.addDialog.getValues();
					console.debug("performing add, dialog values:", values);
					if (this._createObject(values, this.addDialog.dialog)) {
						this.addDialog.save(this.store, values, escape(values.id), lang.hitch(this,function() {
							this.addDialog.dialog.hide();
							this.triggerListeners();									
						}));
					} else {
						console.debug("save failed");
						bar.innerHTML = "";
					}
				}
				this.addDialog.saveButtonValidate = false;
			}));

			if (!this.hidepasswords) {
				dojo.subscribe(this.resetDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
					this.resetDialog.saveButtonValidate = true;
					// any message on this topic means that we clicked "save"
					if (dijit.byId("reset"+this.id+"Dialog_form").validate()) {					
						this.resetDialog.save(lang.hitch(this, function(values) {
							console.debug("reset is done, returned:", values);
							var bar = query(".idxDialogActionBarLead",this.resetDialog.dialog.domNode)[0];
							bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
							if (this._resetPasswd(values, this.resetDialog.dialog)) {
								this.resetDialog.dialog.hide();						
							} else {
								console.debug("save failed");
								bar.innerHTML = "";
							}						
						}));
					}
					this.resetDialog.saveButtonValidate = false;
				}));
			}
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				var obj = this.grid.select.row.getSelected()[0];
				var objName = this._getSelectedData().id;
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.deletingProgress;
				if (this._deleteObject(objName, this.removeDialog)) {
					if (this.isGroupGrid) {
						var oldValues = this.store.get(this.grid.select.row.getSelected()[0]);
						this._notifyGroupDelete(oldValues.id);
					}
					this.grid.store.remove(obj);
					this._updateButtons();
					this.removeDialog.hide();	
					this.triggerListeners();
				} else {
					console.debug("delete failed");
					bar.innerHTML = "";
				}
			}));
		},
		
		_doEdit: function() {
			var rowData = this._getSelectedData();
			// create array of existing ids except for the current one, so we don't clash
			var existingNames = this.store.query(function(item){ return item.id != rowData.id; }).map(
				function(item){ return item.id; });
			
			this.editDialog.fillValues(rowData, existingNames);
			this.editDialog.show(rowData, this.isGroupGrid, this);
		},
		
		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated 
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");
			
			// update grid quick filter if there is one
			if (this.grid.quickFilter) {
				this.grid.quickFilter.apply();				
			}
			// re-sort
			if (this.grid.sort) {
				var sortData = this.grid.sort.getSortData();
				if (sortData && sortData !== []) {
					this.grid.model.clearCache();
					this.grid.sort.sort(sortData);
				}
			}	
		}
		
		
	});

	return UserGrid;
});
