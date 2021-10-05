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
		'ism/widgets/CheckBoxList',
		'ism/widgets/TextArea',
		'ism/IsmUtils',		
		'dojo/text!ism/controller/content/templates/ConfigurationPolicyDialog.html',
		'dojo/i18n!ism/nls/configurationPolicy',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, query, aspect, on, domStyle, domConstruct, number, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, Dialog, CheckBox, CheckBoxList, TextArea, Utils, configPolicyDialog, nls, nls_strings) {

	/*
	 * This file defines a configuration widget that displays the list of message hubs
	 */
	
	var ConfigurationPolicyDialog = declare("ism.controller.content.ConfigurationPolicyDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add/Edit buttons in the ConfigurationPolicyList widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds functions to
		//		populate the dialog fields with a record (fillValues) and save the changes
		//		via XHR put (save).  For inputs that are more complex than a text box,
		//		_getFieldValue and _setFieldValue are defined to get/set a generic value for
		//		different input types (e.g. checkbox).
		nls: nls,
		nls_strings: nls_strings,
		templateString: configPolicyDialog,

		// template strings
		nameLabel: Utils.escapeApostrophe(nls.nameLabel),
		nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.descriptionLabel),
        clientIPLabel: Utils.escapeApostrophe(nls.clientIPLabel),
        invalidClientIP: Utils.escapeApostrophe(nls.invalidClientIP),
        userIdLabel: Utils.escapeApostrophe(nls.userIdLabel),
        invalidWildcard: Utils.escapeApostrophe(nls.invalidWildcard),
        commonNameLabel: Utils.escapeApostrophe(nls.commonNameLabel),
        groupIdLabel: Utils.escapeApostrophe(nls.groupIdLabel),
        authorityLabel: Utils.escapeApostrophe(nls.authorityLabel),
        authorityTooltip: Utils.escapeApostrophe(nls.authorityTooltip),
        
        filters: ['ClientAddress', 'UserID', 'CommonNames', 'GroupID'],
		viewOnly: false,
		closeButtonLabel: nls.cancelButton,
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		    this.closeButtonLabel = this.viewOnly === true ? nls.closeButton : nls.cancelButton; 
		},

		postCreate: function() {
            
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
				this.field_Name.set('readOnly', true);	
			}
			
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
							if (!matches) {
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
			
			// set up onChange for description
			t.field_Description.on("change", function(args) {
				t.dialog.resize();
			}, true);
			
			aspect.after(this.dialog, "onHide", lang.hitch(this, function() {
				dijit.byId(this.dialogId +"_DialogForm").reset();
			}));
			
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			console.debug("ConfigurationPolicyDialog -- fillValues", values);
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}
		},

		show: function(configPolicy, list) {								
			this.field_Name.existingNames = list;
			this._setupFields(configPolicy, true);

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
		save: function(onFinish, onError) {
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;

			var values = {};
			var filterSelected = false;
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					// don't send disabled values
					values[actualProp] = this._getFieldValue(actualProp);
					if (array.indexOf(this.filters, actualProp) >= 0 && 
							values[actualProp] !== null && values[actualProp] !== undefined  && lang.trim(values[actualProp]) !== "") {
						filterSelected = true;
					}
				}
			}
			
			// verify at least one filter has been selected
			if (filterSelected === false) {
				query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
				this.dialog.showErrorMessage(nls.invalidFilter);	
				return;
			}
		
			if (!onFinish) { onFinish = lang.hitch(this,function() {	
				    this.dialog.hide();
				});
			}
			if (!onError) { onError = lang.hitch(this,function(error) {	
    				bar.innerHTML = "";
    				this.dialog.showErrorFromPromise(nls.saveConfigPolicyError, error);
				});
			}

            var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.createPostOptions("ConfigurationPolicy", values);
            var promise = endpoint.doRequest("/configuration", options);
            promise.then(
                function(data) {
                    onFinish(values);
                },
                onError
            );
		},
		
		// do any adjustment of form field values necessary before showing the form.  in this
		_setupFields: function(configPolicy, showWarning) {
			console.debug("ConfigurationPolicyDialog -- setupFields");

			// clear fields and any validation errors
			dijit.byId(this.dialogId +"_DialogForm").reset();
			dijit.byId(this.dialogId + "_saveButton").set('disabled',false);
			
			if (configPolicy) {	
				this.fillValues(configPolicy);
			} 
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
		    var value = "";	
		    
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
					var arrayValue = this["field_"+prop].get("value");
					array.forEach(arrayValue, function(entry, i) {
						if (i > 0) {
							value += ",";
						}
						value += entry;
					});
					break;
				default:
					value = this["field_"+prop].get("value");
					break;
			}
			return value;
		},
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("ConfigurationPolicyDialog -- _setFieldValue", prop, value);

			// if there is a check box for this field, determine whether or not 
			// it should be checked
			var checkbox = this["checkbox_"+prop]; 
			if (checkbox) {
				var valueProvided = value !== null && value !== undefined && value !== "";
				this["checkbox_"+prop].set("checked", valueProvided);					
			}

			switch (prop) {
				case "ActionList":
					if (value) {
						this["field_"+prop]._setValueAttr(value.split(","));
					}
					break;
				default:
					if (this["field_"+prop]) {
						this["field_"+prop].set("value", value);
					}
					break;
			}
		}
		
	});
	
	return ConfigurationPolicyDialog;
});
