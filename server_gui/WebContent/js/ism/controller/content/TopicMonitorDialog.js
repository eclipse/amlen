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
		'dojo/text!ism/controller/content/templates/TopicMonitorDialog.html',		
		'dojo/i18n!ism/nls/monitoring',
		'dojo/i18n!ism/nls/strings',
		'idx/string'
		], function(declare, lang, query, omConstruct, domStyle,  _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,Form, Button, 
				    HoverHelpTooltip, Utils, Dialog, TextBox, TextArea, topicMonitorDialog, nls, nls_strings, iStrings) {

	var TopicMonitorDialog = declare("ism.controller.content.TopicMonitorDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Form that allows you to define a new message hub
		// 
		// description:
		//		
		nls: nls,
		nls_strings: nls_strings,
		templateString: topicMonitorDialog,	
		_startingValues: {},
		domainUid: 0,
		Utils: Utils,
		topicStringLabel: Utils.escapeApostrophe(nls.monitoring.dialog.topicStringLabel),
	    topicStringTooltip: Utils.escapeApostrophe(nls.monitoring.dialog.topicStringTooltip),


		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {
            
            this.domainUid = ism.user.getDomainUid();
					
			var saveId = this.dialogId + "_saveButton";
			// check that the topic string matches the pattern and doesn't clash with an existing object
			this.field_TopicString.isValid = function(focused) {
				var value = this.get("value");
				var len = this.existingNames ? this.existingNames.length : 0;
				for (var i=0; i < len; i++) {
					if (value == this.existingNames[i]) {
						this.invalidMessage = nls.monitoring.invalidName;
						return false;
					}
				}
				
				if (value.indexOf("+") != -1 || (value.indexOf("#") >= 0 && value.indexOf("#") != (value.length - 1))) {
					// if we have a + in the string, or a # at someplace that is not the last character...
					this.invalidMessage = nls.monitoring.invalidChars;
					return false;
				} else if (value.length === 0) {
					this.invalidMessage = nls.monitoring.invalidRequired;
					return false;
				} else if (value.length == 1 && value != "#") {
					// if it isn't "#"
					this.invalidMessage = nls.monitoring.invalidTopicMonitorFormat;
					return false;
				} else if (value.length > 3 && iStrings.startsWith(value, "$SYS")) {
						this.invalidMessage = nls.monitoring.reservedName;
						return false;
				} else if (value.length > 1 && value.indexOf("/#") != value.length - 2) {
					// if we don't end in /# ...
					this.invalidMessage = nls.monitoring.invalidTopicMonitorFormat;
					return false;
				} 
				return true;
			};
			
			this._setupFields();

			// watch the form for changes between valid and invalid states
			dijit.byId(this.dialogId +"_form").watch('state', lang.hitch(this, function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(saveId);
				if (newvalue && !this.field_TopicString.isValid()) {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			}));
			
			var button = dijit.byId(saveId);
			button.set('disabled', true);
		},
		
		show: function(topicMonitorItem, list) {
			this.topicMonitorItem = topicMonitorItem;			
			this._setupFields();			
			if (topicMonitorItem) {
				this.fillValues(topicMonitorItem);
			}			
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_TopicString.existingNames = list;			
			dijit.byId(this.dialogId+"_saveButton").set('disabled',false);
			this.dialog.clearErrorMessage();
			this.dialog.show();
		    this.field_TopicString.focus();
		},

		
		// save the record
		save: function(onFinish, onError) {
			if (!onFinish) { onFinish = lang.hitch(this,function() {	
				this.dialog.hide();
				});
			}
			if (!onError) { onError = lang.hitch(this,function(error, ioargs) {	
				query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
				this.dialog.showErrorFromPromise(nls.monitoring.savingFailed, error);
				});
			}
			var values = [];
			values[0] = this._getFieldValue("TopicString");

			var dialog = this.dialog;			
			if (values[0]) {
	            var adminEndpoint = ism.user.getAdminEndpoint();
	            var options = adminEndpoint.createPostOptions("TopicMonitor", values);
	            var promise = adminEndpoint.doRequest("/configuration",options);
	            promise.then(
	                    function(data) {
	                        onFinish(values);
	                    },
	                    onError
	            );	            
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
	return TopicMonitorDialog;
});
