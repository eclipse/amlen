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
		'dojo/query',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Form',
		'dijit/form/Button',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils',
		'ism/widgets/Dialog',
		'ism/widgets/TextBox',
		'ism/widgets/TextArea',
		'dojo/text!ism/controller/content/templates/MessageHubDialog.html',		
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, query, omConstruct, domStyle,  _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,Form, Button, 
				    HoverHelpTooltip, Utils, Dialog, TextBox, TextArea, messageHubDialog, nls, nls_strings) {

	var MessageHubDialog = declare("ism.controller.content.MessageHubDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Form that allows you to define a new message hub
		// 
		// description:
		//		
		nls: nls,
		nls_strings: nls_strings,
		templateString: messageHubDialog,	
		_startingValues: {},
		domainUid: 0,
		nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments); 
		},

		postCreate: function() {
            
		    this.domainUid = ism.user.getDomainUid();
					
			var saveId = this.dialogId + "_saveButton";
			if (this.add === true || Utils.configObjName.editEnabled()) {			
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
			
			var t = this;
			// set up onChange for description
			t.field_Description.on("change", function(args) {
				t.dialog.resize();
			}, true);
			
			this._setupFields();

			// watch the form for changes between valid and invalid states
			dijit.byId(this.dialogId +"_form").watch('state', function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(saveId);
				if (newvalue) {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			});
		},
		
		show: function(messageHubItem, list) {
			this.messageHubItem = messageHubItem;			
			this._setupFields();			
			if (messageHubItem) {
				this.fillValues(messageHubItem);
			}			
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Name.existingNames = list;			
			dijit.byId(this.dialogId+"_saveButton").set('disabled',false);
			this.dialog.clearErrorMessage();
			this.dialog.show();
			this.field_Description.focus(); // needed to get the text area to resize
			if (this.add === true) {
				this.field_Name.focus();
			}
		},

		
		// save the record
		save: function(onFinish, onError) {
			if (!onFinish) { onFinish = lang.hitch(this,function() {	
				this.dialog.hide();
				});
			}
			if (!onError) { 	onError = lang.hitch(this,function(error) {	
				query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
				this.dialog.showErrorFromPromise(nls.messaging.messageHubs.saveMessageHubError, error);
				});
			}
			var values = {};
			var recordChanged = false;
			
			for (var i = 0; i < this._attachPoints.length; i++) {
				var prop = this._attachPoints[i];
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (values[actualProp] != this._startingValues[prop]) {
						recordChanged = true;
					}
				}
			}
			console.debug("Saving: ", values);
			
			var dialog = this.dialog;

			if (recordChanged) {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.createPostOptions("MessageHub", values);
				var promise = adminEndpoint.doRequest("/configuration", options);
				promise.then(function(data) {
					onFinish(values);
				},
				onError);
			} else {
				dialog.hide(); // nothing to save
			}
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}
		},

		
		// do any adjustment of form field values necessary before showing the form.  
		_setupFields: function() {
			console.debug("setupFields");

			// clear fields and any validation errors
			dijit.byId(this.dialogId +"_form").reset();
			//dijit.byId(this.dialogId +"_form").validate();
			
			// clear all fields first
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0, 6) == "field_") {
					var actualProp = prop.substring(6);
					this._setFieldValue(actualProp, "");
				}
			}

		},
		
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			switch (prop) {
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
			var value = "";
			switch (prop) {
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
	return MessageHubDialog;
});
