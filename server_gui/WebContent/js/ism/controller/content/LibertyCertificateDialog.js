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
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/on',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Form',
		'dijit/form/Button',
		'dojox/form/Uploader',
		'dojox/form/uploader/FileList',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils',
		'ism/widgets/Dialog',
		'ism/widgets/TextBox',
		'dojo/text!ism/controller/content/templates/LibertyCertificateDialog.html',		
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, aspect, query, omConstruct, domStyle, on, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,Form, Button, 
				Uploader, FileList, HoverHelpTooltip, Utils, Dialog, TextBox, libertyCertDialog, nls, nls_strings) {

	var LibertyCertificateDialog = declare("ism.controller.content.LibertyCertificateDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Form that allows you to define a new message hub
		// 
		// description:
		//		
		nls: nls,
		nls_strings: nls_strings,
		templateString: libertyCertDialog,
	    
	    // template strings
		nameLabel: Utils.escapeApostrophe(nls.appliance.certificateProfiles.grid.name),
        nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        certLabel: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.certificate),
        certTooltip: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.certificateTooltip),
        browseHint: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.browseHint),
        browseLabel: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.browse),
        certpasswordLabel: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.certpassword),
        privkeyLabel: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.privkey),
        privkeyTooltip: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.privkeyTooltip),
        keypasswordLabel: Utils.escapeApostrophe(nls.appliance.certificateProfiles.dialog.keypassword),

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: null,
		_certificateToUpload: null,
		_keyToUpload: null,
		// function to call when all files needing to be uploaded are complete
		uploadComplete: null,
		_savedValues: null,
		_obj: null,

		// variable to keep count of uploaded files
		uploadedFileCount: null,
		uploadTimeout: 30000, // milliseconds
		timeoutID: null,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			this.field_Certificate.isValid = function(focused) {
				var value = this.get("value");
				if (!value || value === ""){
					return false;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				return unique;				
			};
			this.field_Certificate.getErrorMessage = function() {
				var value = this.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				return nls.appliance.certificateProfiles.dialog.duplicateFile;
			};
			
			this.field_Key.isValid = function(focused) {
				var value = this.get("value");
				if (!value || value === ""){
					return false;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				return unique;				
			};
			this.field_Key.getErrorMessage = function() {
				var value = this.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				return nls.appliance.certificateProfiles.dialog.duplicateFile;
			};
			
			// listen for selections from the file uploader buttons
			on(dijit.byId(this.dialogId+"_certUploader"), "change", lang.hitch(this, function(files) {
				console.debug("file changed");
				console.dir(files);
				if (files && files.length && (files.length > 0) && files[0].name) {
					var filename = files[0].name;
					this.field_Certificate.set("value", filename);	
					if (this.field_Certificate.validate()) {
						this._certificateToUpload = filename;
					}					
				}
			}));
			on(dijit.byId(this.dialogId+"_keyUploader"), "change", lang.hitch(this, function(files) {
				console.debug("file changed");
				console.dir(files);
				if (files && files.length && (files.length > 0) && files[0].name) {
					var filename = files[0].name;
					this.field_Key.set("value", filename);
					if (this.field_Key.validate()) {
						this._keyToUpload = filename;
					}
				}
			}));
			
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
			
			// After certificate file has uploaded, upload key file, if needed, otherwise post form data
			on(dijit.byId(this.dialogId+"_certUploader"), "complete", lang.hitch(this, function(evt) {
				this.uploadedFileCount++;
				this._certificateToUpload = null;
				console.debug("First upload complete");
				console.dir(evt);
				if (evt && evt.code && evt.message) {
					this.handleError(evt, nls.appliance.certificateProfiles.uploadCertError);
					return;
				}				
				// is there a key to upload?
				if (this._keyToUpload) {
					dijit.byId(this.dialogId+"_keyUploader").upload();					
				} else {
					window.clearTimeout(this.timeoutID);
					this.uploadComplete(this._savedValues, this._obj);					
				}
			}));
			// After key file has uploaded, POST form data
			on(dijit.byId(this.dialogId+"_keyUploader"), "complete", lang.hitch(this, function(evt) {
				this.uploadedFileCount++;
				this._keyToUpload = null;
				console.debug("Second upload complete");
				console.dir(evt);
				if (evt && evt.code && evt.message) {
					this.handleError(evt, nls.appliance.certificateProfiles.uploadKeyError);
					return;
				}
				window.clearTimeout(this.timeoutID);
				this.uploadComplete(this._savedValues, this._obj);
			}));

			// error handling
			on(dijit.byId(this.dialogId+"_certUploader"), "error", lang.hitch(this, function(evtObject) {
				console.debug("Upload failed: " + evtObject);
				this.handleError(evtObject, nls.appliance.certificateProfiles.uploadCertError);
			}));
			on(dijit.byId(this.dialogId+"_keyUploader"), "error", lang.hitch(this, function(evtObject) {
				console.debug("Upload failed: " + evtObject);
				this.handleError(evtObject, nls.appliance.certificateProfiles.uploadKeyError);
			}));			
			
			
			// reset the form on close
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				this._certificateToUpload = null;
				this._keyToUpload = null;
				this._savedValues = null;
				this._obj = null;
				dijit.byId(this.dialogId+"_form").reset();
			}));
		},
		
		handleError: function(evt, errorMessage) {
			if (this.timeoutID) {
				window.clearTimeout(this.timeoutID);
			}
			if (!errorMessage) {
				errorMessage = nls.appliance.certificateProfiles.uploadTimeoutError; 
			}
			var message = evt && evt.message ? evt.message.replace(/\n/g, "<br />") : null;
			var code = evt && evt.code ? evt.code : null;	
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";					
			this.dialog.showErrorMessage(errorMessage, message, code);
		},

		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);				
			}
			this._certificateToUpload = null;
			this._keyToUpload = null;
		},
		
		show: function(certificateNames, certificateFiles, rowData) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Certificate.existingNames = certificateFiles;
			this.field_Key.existingNames = certificateFiles;
			if (this.add === true) {
				this.field_Certificate.set("value", "");
				this.field_Key.set("value", "");
				this.field_CertificatePassword.set("value", "");
				this.field_KeyPassword.set("value", "");
			} else if (rowData) {
				this.fillValues(rowData);
			}
			dijit.byId(this.dialogId+"_saveButton").set('disabled',false);
			this.dialog.show();
			this.uploadedFileCount = 0;
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, position, onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var obj = store.get(position);
			var recordChanged = false;
						
			if (this.timeoutID) {
				window.clearTimeout(this.timeoutID);
			}
			var values = {};
			if (!this._startingValues) {
				this._startingValues = {};
			}
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (!this._startingValues || (values[actualProp] != this._startingValues[actualProp])) {
						if (values[actualProp] === "" && this._startingValues[actualProp] === undefined) {
							// consider it unchanged
							continue;
						}
						recordChanged = true;
					}
				}
			}
			
			if (!this.add && this._startingValues["Name"] == values["Name"]) {
				// for an update with no name change, warn the user if they haven't selected any file to upload
				if (!this._certificateToUpload && !this._keyToUpload && this.uploadedFileCount === 0) {
					this.dialog.showWarningMessage(nls.appliance.certificateProfiles.dialog.noFilesToUpload);
					return;
				}
			}
			
			if (this._certificateToUpload || this._keyToUpload) {
				recordChanged = true;
			}
			
			if (recordChanged) {
				
				this._savedValues = values;
				this._obj = obj;
				this.uploadComplete = onFinish;
				
				// is there a certificate to upload?
				if (this._certificateToUpload) {
					console.debug("uploading cert");
					dijit.byId(this.dialogId+"_certUploader").upload();	
				} else if (this._keyToUpload) {
					console.debug("uplaoding key");
					dijit.byId(this.dialogId+"_keyUploader").upload();
				} else {
					onFinish(values,obj);
					return;
				}
				
				// schedule timeout
				this.timeoutID = window.setTimeout(lang.hitch(this, function(){
					this.handleError(null, nls.appliance.certificateProfiles.uploadTimeoutError);
				}), this.uploadTimeout);
				
			} else {
				console.debug("nothing changed, so no need to save");
				this.dialog.hide();
			}			
		},
		
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");

			//console.debug("setting field_"+prop+" to '"+value+"'");
			if (this["field_"+prop] && this["field_"+prop].set) {
				this["field_"+prop].set("value", value);
				this["field_"+prop].validate();
			} else {
				this["field_"+prop] = { value: value };
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
	return LibertyCertificateDialog;
});
