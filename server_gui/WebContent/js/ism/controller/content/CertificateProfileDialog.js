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
		'dojo/dom',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/on',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Form',
		'dijit/form/Button',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils',
		'ism/widgets/Dialog',
		'ism/widgets/TextBox',
		'dojo/text!ism/controller/content/templates/CertificateProfileDialog.html',		
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, aspect, query, dom, domConstruct, domStyle, on, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,Form, Button, 
				HoverHelpTooltip, Utils, Dialog, TextBox, certProfileDialog, nls, nls_strings) {

	var CertificateProfileDialog = declare("ism.controller.content.CertificateProfileDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Form that allows you to define a new certificate
		// 
		// description:
		//		
		nls: nls,
		nls_strings: nls_strings,
		templateString: certProfileDialog,
	    
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
        overwriteLabel: Utils.escapeApostrophe(nls.allowCertfileOverwriteLabel),
        overwriteTooltip: Utils.escapeApostrophe(nls.allowCertfileOverwriteTooltip),

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: null,
		_files: {},
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
			var _this = this;
			
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
			
			if (this.add === true || Utils.configObjName.editEnabled()) {			
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
				this._files = {};
				this._savedValues = null;
				this._obj = null;
				this.form.reset();
			}));
		},
		
		clickCertificateBrowseButton: function() {
		    this.certificateFile.click();
		},

		clickKeyBrowseButton: function() {
		    this.keyFile.click();
		},
		
		certificateChanged: function() {
		    this.handleFileChangeEvent(this.certificateFile.files, 'Certificate');
		},
		
		keyChanged: function() {
            this.handleFileChangeEvent(this.keyFile.files, 'Key');		    
		},
		
		/**
		 * Handle the onchange event that occurs when the user selects the file to upload
		 * @param files  The FileList containing the file to upload
		 * @param fileType The type of file we are uploading (Certificate or Key)
		 */
		handleFileChangeEvent: function(/*FileList*/files, /*String*/fileType) {
		    var field = this["field_"+fileType];
		    if (!field) {
		        console.debug("CertificateProfile2.handleFileChangeEvent called with unrecognized fileType " + fileType);
		        return;
		    }
            if (files && files.length > 0 && files[0].name) {
                var filename = files[0].name;
                field.set("value", filename);  
                if (field.validate()) {
                    this._files[fileType] = files[0];
                }                   
            } else {
                console.debug("CertificateProfile2.handleFileChangeEvent called without any files " + console.debug(JSON.stringify(files)));                
            }		    
		},						
		 		
		handleError: function(error, errorMessage) {
			if (this.timeoutID) {
				window.clearTimeout(this.timeoutID);
			}
			if (!errorMessage) {
				errorMessage = nls.appliance.certificateProfiles.uploadTimeoutError; 
			}
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";					
			this.dialog.showErrorFromPromise(errorMessage, error);
		},

		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);				
			}
			this._files = {};
		},
		
		show: function(certificateNames, certificateFiles, rowData) {
			this.field_Name.existingNames = certificateNames;
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Certificate.existingNames = certificateFiles;
			this.field_Key.existingNames = certificateFiles;
            this.certificateFile.value = "";
            this.keyFile.value = "";
			if (this.add === true) {
				this.field_Name.set("value", "");
				this.field_Certificate.set("value", "");
				this.field_Key.set("value", "");
				this.field_CertFilePassword.set("value", "");
				this.field_KeyFilePassword.set("value", "");
				dom.byId(this.dialogId+"_overwrite").innerHTML = "";
			} else if (rowData) {
				this.fillValues(rowData);
			}
			dijit.byId(this.dialogId+"_saveButton").set('disabled',false);
			this.dialog.show();
			this.field_Name.focus();		    
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
				if (!this._files.Certificate && !this._files.Key && this.uploadedFileCount === 0) {
					this.dialog.showWarningMessage(nls.appliance.certificateProfiles.dialog.noFilesToUpload);
					return;
				}
			}
			
			if (this._files.Certificate || this._files.Key) {
				recordChanged = true;
			}
			
			if (recordChanged) {
				
				this._savedValues = values;
				this._obj = obj;
				this.uploadComplete = onFinish;
				
				var _this = this;
				var adminEndpoint = ism.user.getAdminEndpoint();
				
				// is there a certificate to upload?
				if (_this._files.Certificate) {
					console.debug("uploading cert");
					adminEndpoint.uploadFile(_this._files.Certificate, function(data) {
		                _this.uploadedFileCount++;
		                _this._files.Certificate = undefined;
		                console.debug("First upload complete");
		                // is there a key to upload?
		                if (_this._files.Key) {
		                    console.debug("uploading key");
		                    _this._uploadKey();                 
		                } else {
		                    window.clearTimeout(_this.timeoutID);
		                    _this.uploadComplete(_this._savedValues, _this._obj);                  
		                }					   
					}, function(error) {
                        _this.handleError(error, nls.appliance.certificateProfiles.uploadCertError);					    
					});	
				} else if (_this._files.Key) {
					console.debug("uploading key");
					_this._uploadKey();
				} else {
					onFinish(values,obj);
					return;
				}
				
				if (this._files.Certificate || this._files.Key) {
					// schedule timeout if one or both files need to be uploaded
					this.timeoutID = window.setTimeout(lang.hitch(this, function(){
						this.handleError(null, nls.appliance.certificateProfiles.uploadTimeoutError);
					}), this.uploadTimeout);
				}
				
			} else {
				console.debug("nothing changed, so no need to save");
				this.dialog.hide();
			}			
		},
		
		_uploadKey: function() {
		    var _this = this;
		    var adminEndpoint = ism.user.getAdminEndpoint();
		    adminEndpoint.uploadFile(_this._files.Key, function(data) {
                _this.uploadedFileCount++;
                _this._files.Key = undefined;
                console.debug("Key upload complete");
                window.clearTimeout(_this.timeoutID);
                _this.uploadComplete(_this._savedValues, _this._obj);
            }, function(error) {
                _this.handleError(error, nls.appliance.certificateProfiles.uploadKeyError);                        
            }); 
		    
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

			switch(prop) {
			case "Overwrite":
				value = this["field_Overwrite"].get("checked");
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
	return CertificateProfileDialog;
});
