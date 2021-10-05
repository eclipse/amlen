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
		'dojo/number',
		'dojo/query',
		'dojo/_base/array',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dojox/html/entities',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'dijit/form/Form',
		'dijit/MenuItem',
		'idx/string',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/widgets/TextBox',
		'ism/IsmUtils',
		'ism/config/Resources',
		'dojo/text!ism/controller/content/templates/LDAPDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/messages',
		'dojo/keys',
		'idx/widget/HoverHelpTooltip'
		], function(declare, lang, aspect, number, query, array, on, domClass, domConstruct, domStyle, 
				Memory, entities, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, Form, MenuItem, iString, GridFactory, Dialog, TextBox, Utils, Resources,
				userDialog, nls, nls_strings, nls_appl, nls_messages, keys, HoverHelpTooltip) {

	/*
	 * This file defines a configuration widget that combines a grid, toolbar, and set of dialogs for editing.  All
	 * Dialog widgets are defined inline (rather than in their own file) for simplicity, but could easily be separated
	 * and referenced from an external file.
	 */

	var LDAPDialog = declare("ism.controller.content.ExternalLDAP.EditLDAPDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
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
		templateString: userDialog,
		uploadedFileCount: null,
		uploadTimeout: 30000,
		timeoutID: null,
		missingCertificate: false,
		sendCertificate: false,
		sendPassword: false,

		// the unique field that identifies a record on server XHR requests
		keyField: "URL",
		
		// dialog counter to keep track of which dialog is opened for callback purposes
		dialogCounter: 0,
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},
		
        // template strings
        urlLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.url),
        urlTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.url),
        urlThemeError: Utils.escapeApostrophe(nls.messaging.externldap.dialog.urlThemeError),
        certificateLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.certificate),
        checkcertLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.checkcert),
        checkcertTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.checkcert),
        browseHint: Utils.escapeApostrophe(nls.messaging.externldap.dialog.browseHint),
        browseLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.browse),
        clearLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.clear),
        timeoutLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.timeout),
        timeoutTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.timeout),
        unitLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.secondUnit),
        cachetimeoutLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.cachetimeout),
        cachetimeoutTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.cachetimeout),
        groupcachetimeoutLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.groupcachetimeout),
        groupcachetimeoutTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.groupcachetimeout),
        basednLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.basedn),
        basednTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.basedn),
        binddnLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.binddn),
        binddnTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.binddn),
        bindpwLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.bindpw),
        bindpwHint: nls.emptyBindPwdHint,
        bindpwTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.bindpw),
        clearBindpwLabel: nls.clearBindPasswordLabel,
        useridmapLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.useridmap),
        useridmapTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.useridmap),
        userSuffixLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.userSuffix),
        userSuffixTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.userSuffix),
        groupmemberidmapLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.groupmemberidmap),
        groupmemberidmapTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.groupmemberidmap),
        groupidmapLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.groupidmap),
        groupidmapTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.groupidmap),
        groupSuffixLabel: Utils.escapeApostrophe(nls.messaging.externldap.dialog.groupSuffix),
        groupSuffixTooltip: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.groupSuffix),

		domainUid: 0,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {
			console.debug(this.declaredClass + ".postCreate()");
            
            this.domainUid = ism.user.getDomainUid();

			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;			
			
			this.field_Timeout.isValid = lang.hitch(this, function() {
				var value = this.field_Timeout.get("value");
				if (!isFinite(value)){
					return false;
				}

				if (value >= 1 && value <= 60 ) {
					return true;
				}
				return false;				
			});
			this.field_Timeout.getErrorMessage = lang.hitch(this, function() {
				return nls.messaging.externldap.dialog.invalidTimeout;
			});
			
			this.field_CacheTimeout.isValid = lang.hitch(this, function() {
				var value = this.field_CacheTimeout.get("value");
				if (!isFinite(value)){
					return false;
				}

				if (value >= 1 && value <= 60 ) {
					return true;
				}
				return false;				
			});
			this.field_CacheTimeout.getErrorMessage = lang.hitch(this, function() {
				return nls.messaging.externldap.dialog.invalidCacheTimeout;
			});
			
			this.field_GroupCacheTimeout.isValid = lang.hitch(this, function() {
				var value = this.field_GroupCacheTimeout.get("value");
				var intValue = number.parse(value, {places: 0});
				var max = 86400;
				if (isNaN(intValue) || intValue < 1 || intValue > max) {
					return false;
				}
				return true;
			});
			this.field_GroupCacheTimeout.getErrorMessage = lang.hitch(this, function() {
				return nls.messaging.externldap.dialog.invalidGroupCacheTimeout;
			});
			
			this.field_Certificate.isValid = lang.hitch(this, function() {
				var value = this.field_URL.get("value");
				var certValue = this.field_Certificate.get("value");
				var certCheckValue = this.field_CheckServerCert.get("value");
				
				if (value && value !== "") {
					if (   (value.toLowerCase().indexOf("ldaps") === 0) 
						&& (!certValue || certValue === "")
						&& (certCheckValue == "TrustStore")) {
						this.missingCertificate = true;
						return false;
					}
				}
				return true;
			});
			this.field_Certificate.getErrorMessage = lang.hitch(this, function() {
				return nls.messaging.externldap.dialog.certificateRequired;
			});
			
			// listen for selections from the url text box
			on(dijit.byId(this.dialogId+"_url"), 'change', lang.hitch(this, function() {
				if (this.missingCertificate) {
					console.debug("revalidating certificate");
					this.missingCertificate = false;
					this.field_Certificate.validate();
				}
			}));
			// listen for selections from the checkServerCert selwxr
			on(dijit.byId(this.dialogId+"_certCheck"), 'change', lang.hitch(this, function() {
				if (this.missingCertificate) {
					console.debug("revalidating certificate");
					this.missingCertificate = false;
					this.field_Certificate.validate();
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
			
		    var hoverhelptp = HoverHelpTooltip({ 
		    	id: this.dialogId + "_testButton_Tooltip",
		        connectId:[this.dialogId + "_testButton"],
		        forceFocus: true,  
		        showLearnMore:false,  
		        label: Utils.escapeApostrophe(nls.messaging.externldap.dialog.tooltip.testButtonHelp),
		        position: ["above", "below"] 
		     }); 
		  
		},
		
		// listen for selection of the file upload button
		clickFileUploaderButton: function() {
            this.fileUploader.click();
        },
        
        // handle the file upload event
        handleFileChangeEvent: function() {                      
            var files = this.fileUploader.files;
            var field = this.field_Certificate;
            if (files && files.length > 0 && files[0].name) {
                var filename = files[0].name;
                field.set("value", filename);  
                if (field.validate()) {
                    this._certificateToUpload = files[0];
                    this.sendCertificate = true;
        			dijit.byId(this.dialogId+"_clearCertButton").set('style',"display:block");
                } else {
                    this._certificateToUpload = undefined;
                }                  
            }
        },
        
        clickClearBindPassword: function() {
        	console.debug("ClearBindPassword clicked");
        	this.field_BindPassword.set("value", "");
        	this.sendPassword = true;
        	this.field_BindPassword.set("placeHolder", "");
			dijit.byId(this.dialogId+"_clearBindPwButton").set('style',"display:none");
        },
		
		handleError: function(error, errorMessage) {
			if (this.timeoutID) {
				window.clearTimeout(this.timeoutID);
			}

			if (!errorMessage)
				errorMessage = nls_appl.appliance.certificateProfiles.uploadCertError;
			
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";					
			this.dialog.showErrorFromPromise(errorMessage, error);
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			dijit.byId(this.dialogId+"_form").reset();
			
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}
		},
		
		resetFiles: function() {
			this.fileUploader.value = "";
			this.field_Certificate.set('value', "");
			this._certificateToUpload = undefined;
			this.sendCertificate = true;
			dijit.byId(this.dialogId+"_clearCertButton").set('style',"display:none");
		},
		
		show: function(values) {
			this.missingCertificate = false;
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			dijit.byId(this.dialogId+"_form").reset();
			if (values) {
				this.fillValues(values);
				if (values["Certificate"] !== undefined && values["Certificate"] !== "") {
					dijit.byId(this.dialogId+"_clearCertButton").set('style',"display:block");
				} else {
					dijit.byId(this.dialogId+"_clearCertButton").set('style',"display:none");
				}
				if (values["BindPassword"] !== undefined && values["BindPassword"] !== "") {
					dijit.byId(this.dialogId+"_clearBindPwButton").set('style',"display:block");
				} else {
					this.field_BindPassword.set("placeHolder", "");
					dijit.byId(this.dialogId+"_clearBindPwButton").set('style',"display:none");					
				}
			}
			
			// reset file upload variables
			this._certificateToUpload = null;
			this.uploadedFileCount = 0;
			this.sendCertificate = false;
			this.sendPassword = false;
			this.dialogCounter++;
			this.dialog.show();
		    this.field_URL.focus();
		    // handle a special case for the URL tooltip due to html characters...
		    var urlTooltip = dijit.byId(this.dialogId+ "_url_Tooltip");
			if (urlTooltip) {
				 urlTooltip.domNode.innerHTML = nls.messaging.externldap.dialog.tooltip.url;	
			}
		},
		
		getId : function() {
			return this._startingValues.URL;
		},
		
		getValues: function() {
			var values = {};
			var recordChanged = false;
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (values[actualProp] != this._startingValues[actualProp]) {
						recordChanged = true;
					}
				}
			}
			if (recordChanged) {
				return values;
			}
			return undefined;
		},
		
		checkChangedValues: function () {
			var changed = false;
			var values = {};
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && (prop.substring(0,6) == "field_") && (prop !== "field_gridId")) {
					var actualProp = prop.substring(6);
					// Only track changed values.  Omit unchanged values from update request.
				    if ((actualProp !== "BindPassword") && (actualProp !== "Certificate")) {
						if (this._getFieldValue(actualProp) != this._startingValues[actualProp]) {
							values[actualProp] = this._getFieldValue(actualProp);
							console.debug("Changed: " + actualProp);
							changed = true;
						} else {
					        // Include unchanged values except for "BindPassword" and "Certificate"
					        // because the correct values for these might not be known or available to
					        // the current user.
					        values[actualProp] = this._getFieldValue(actualProp);
						}
				    } else {
                        if (actualProp == "BindPassword") {
                        	var pwval = this._getFieldValue("BindPassword");
                        	if (this.sendPassword || ((pwval !== "") && (pwval !== undefined))) {
                            	this.sendPassword = true;
                            	changed = true;
                            	values["BindPassword"] = pwval;
                        	}
                        }
                        if ((actualProp == "Certificate") && this.sendCertificate) {
                        	changed = true;
                        	values["Certificate"] = this._getFieldValue("Certificate");
                        	// Always set overwrite to true for LDAP certificate uploads
                        	values["Overwrite"] = true;
                        }
				    }
				}
			}
			if (changed) {
				return values;
			}
			return undefined;
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, values, index) {
			
			var recordChanged = false;
			var obj = store.data[0];
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && (prop.substring(0,6) == "field_") && (prop !== "field_gridId")) {
					var actualProp = prop.substring(6);
					if ((actualProp !== "BindPassword" && (actualProp !== "Certificate"))) {
						if (values[actualProp] != this._startingValues[actualProp]) {
							recordChanged = true;
							obj[actualProp] = values[actualProp];
						}
					} else {
						if ((actualProp === "BindPassword") && this.sendPassword) {
							recordChanged = true;
							obj[actualProp] = values[actualProp];
						}
						if ((actualProp === "Certificate") && this.sendCertificate) {
							recordChanged = true;
							obj[actualProp] = values[actualProp];
						}
					}
				}
			}
			if (recordChanged) {
				console.debug("Saving:", values);
				store.put(obj,{overwrite: true});					
			} else {
				console.debug("nothing changed, so no need to save");
			}
		},
		
		getTestConnectionProperties: function() {
			var connectionProperties = {};
			var excludeFields = ["field_gridId"];
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_" && excludeFields.indexOf(prop) < 0) {
					var actualProp = prop.substring(6);
					connectionProperties[actualProp] = this._getFieldValue(actualProp);
				}
			}
		    // Fields should only include Certificate and/or BindPassword if the value has changed!
			this.checkChangedValues();
			if (!this.sendCertificate) {
				delete connectionProperties.Certificate;
			}
			if (!this.sendPassword) {
				delete connectionProperties.BindPassword;
			}
			
			connectionProperties["Verify"] = true;
			return connectionProperties;
		},
		
		resetSuccessOrFailMessage: function() {
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			if (!bar) {
				return;
			}
			if (iString.endsWith(bar.innerHTML, nls.messaging.testConnectionSuccess) || 
					iString.endsWith(bar.innerHTML, nls.messaging.testConnectionFailed)) {
				bar.innerHTML = "";
			}
		},
		
		tryUpload: function(onFinish) {
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			
			_this.uploadedFileCount = 0;
			if (_this.timeoutID) {
				window.clearTimeout(_this.timeoutID);
			}
			
			_this.uploadComplete = onFinish;
			
			if (_this._certificateToUpload) {
				console.debug("uploading LDAP cert");
				adminEndpoint.uploadFile(_this._certificateToUpload, function(data) {
	                _this.uploadedFileCount++;
	                _this._certificateToUpload = undefined;
	                console.debug("LDAP cert upload complete");
	                if(_this.timeoutID) {
	                    window.clearTimeout(_this.timeoutID);
	                }
	                _this.uploadComplete();                  					   
				}, function(error) {
				    _this.handleError(error, nls_appl.appliance.certificateProfiles.uploadCertError);
				});
				
				// schedule timeout
				_this.timeoutID = window.setTimeout(lang.hitch(_this, function(){
					_this.handleError(null, nls_appl.appliance.certificateProfiles.uploadCertError);
				}), _this.uploadTimeout);
			} else {
				onFinish();
			}
			
		},
		
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");
			switch (prop) {
			    case "URL":
					this["field_URL"].set("value", value);
					if (value === "" || value === undefined) {
						this["field_URL"].set("placeHolder", "");
					}
				break;
				case "IgnoreCase":
				case "EnableCache":
				case "NestedGroupSearch":
					this["field_"+prop].set("checked", value);
					break;
				case "GroupCacheTimeout":
					this["field_GroupCacheTimeout"].set("value", number.format(value, {places: 0}));
					break;
				case "BindPassword":
					if ((value !== undefined) && (value !== "")) {
					    this["field_BindPassword"].set("placeHolder",nls.bindPasswordHint);
					} else {					
					    this["field_BindPassword"].set("value", value);						
					    this["field_BindPassword"].set("placeHolder", "");						
						dijit.byId(this.dialogId+"_clearBindPwButton").set('style',"display:none");
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
				case "IgnoreCase":
				case "EnableCache":
				case "NestedGroupSearch":					
					value = this["field_"+prop].checked;
					break;
				case "CacheTimeout":
				case "GroupCacheTimeout":
				case "Timeout":
					var field_value = this["field_"+prop].get("value");
					value = number.parse(field_value, {places: 0});
					break;
				case "BindPassword":
					value = this["field_BindPassword"].get("value");
					if (value && (value !== "")) {
						dijit.byId(this.dialogId+"_clearBindPwButton").set('style',"display:none");
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
	});
	
				
	var LDAPGrid = declare("ism.controller.content.ExternalLDAP", [_Widget, _TemplatedMixin], {
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
		enableButton: null,
		LDAPenabled: false,
		contentId: null,
		idName: null,
		uploadAction: null,
		testButton: null,
		testingDialog: null,
		resetButton: null,
		resetDialog: null,
		defaultTimeout: 30,
		defaultCacheTimeout: 10,
		defaultGroupCacheTimeout: 300,
		defaultVerify: false,
		defaultEnabled: false,
		defaultIgnoreCase: true,
		defaultEnableCache: true,
		defaultNestedGroupSearch: false,
		
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		domainUid: 0,
		     			
		constructor: function(args) {
			dojo.safeMixin(this, args);
		
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[]});
			this.idName = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.itemName,{nls:nls})));
		},

		postCreate: function() {
			this.inherited(arguments);
			this.domainUid = ism.user.getDomainUid();
            
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;
			this.certificatesURI += "/" + this.domainUid; 			
			this.uploadAction = Utils.getBaseUri() + this.certificatesURI;

			// start with an empty <div> for the Listeners content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			
			dojo.subscribe(Resources.pages.appliance.nav.userAdministration.topic, lang.hitch(this, function(message){
				this._initStore();			
			}));
			dojo.subscribe(Resources.pages.appliance.nav.userAdministration.topic + ":onShow", lang.hitch(this, function(message){			
				console.debug(this.declaredClass + " refreshing grid");					
				this._refreshGrid();
			}));

			this._initButton();
			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();
		},

		_initStore: function() {
			// summary:
			// 		get the initial records from the server
            var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/LDAP", adminEndpoint.getDefaultGetOptions());
            promise.then(
                    function(data) {
                        _this._populateLDAP(data);
                    },
                    function(error) {
                        if (Utils.noObjectsFound(error)) {
                            _this._populateLDAP(null);
                        } else {
                            Utils.showPageErrorFromPromise(nls.messaging.externldap.retrieveError, error);
                        }
                    }
            );          
		},
		
		_populateLDAP: function(data) {
            var _this = this;
            var ldap = (data && data.LDAP)? data.LDAP : null;
		    
            if (!ldap) {
            	// This should never happen but handle it if it does
            	ldap = {};
            	ldap["URL"] = "";
            	ldap["Timeout"] = _this.defaultTimeout;
            	ldap["CacheTimeout"] = _this.defaultCacheTimeout;
            	ldap["GroupCacheTimeout"] = _this.defaultGroupCacheTimeout;
            	ldap["Enabled"] = _this.defaultEnabled;
            	ldap["Verify"] = _this.defaultVerify;
            	ldap["IgnoreCase"] = _this.defaultIgnoreCase;
            	ldap["EnableCache"] = _this.defaultEnableCache;
            	ldap["NestedGroupSearch"] = _this.defaultNestedGroupSearch;
            }
            if ((ldap.URL !== "")  && Object.keys(ldap).length > 0) {
                _this.LDAPenabled = ldap.Enabled;
                ldap.gridId = 1;
                _this.store = new Memory({data: [ldap], idProperty: "gridId"});
                _this.grid.setStore(_this.store);                
                _this.enableButton.set("disabled", false);
                _this.enableButton.set('label', nls.messaging.externldap.enableButton.disable);               
                _this._enableLDAP(this.LDAPenabled);
                _this.testButton.set("disabled", false);
                _this.resetButton.set("disabled", false);
            } else {
                ldap.gridId = 1;
                _this.store = new Memory({data: [ldap], idProperty: "gridId"});
                _this.grid.setStore(_this.store);
                _this.enableButton.set("disabled", true);
                _this.enableButton.set('label', nls.messaging.externldap.enableButton.enable);
                _this.testButton.set("disabled", true);
                _this.resetButton.set("disabled", true);
            }
		},
		
		_editObject: function(values, dialog, onFinish, onError) {
            console.debug("Saving... ");
			
			console.debug("data=", JSON.stringify(values));
			
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("LDAP", values);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(
					    function(data) {
				    	    if(_this.LDAPenabled) {
							    _this.restartInfoDialog.show();
						    }
				    		onFinish(values);	
					    },
					    function (error) {
					    	console.debug("error: "+error);
							dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
					    }
					);
			return;
		},
		
		_enableTopObject: function(enable) {

			var ldapid = this.store.data[0].URL;
            console.debug("Saving... "+ldapid);
            var props = this.store.query(function(item){ return item.URL == ldapid; })[0];
            props.Enabled = enable;
            
			var excludeFields = ["gridId", "Certificate", "BindPassword"];
			var values = {};
			for (var field in props) {
				if (excludeFields.indexOf(field) < 0) {
					values[field] = props[field];					
				}
			}
			console.debug("data=", JSON.stringify(values));
			
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("LDAP", values);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(
					function(data) {
			    		_this._enableLDAP(enable);
			    		_this.restartInfoDialog.show();
					},
					function(error) {
				    	console.debug("error: "+error);
						Utils.showPageErrorFromPromise(nls.messaging.externldap.enableError, error);
					});
		},
		
		_enableLDAP: function(enable) {
			console.debug("Enable LDAP: ", enable);
			this.LDAPenabled = enable;
			if (enable) {
				this.enableButton.set('label', nls.messaging.externldap.enableButton.disable);
			}
			else {
				this.enableButton.set('label', nls.messaging.externldap.enableButton.enable);
			}
			var userGrid = dijit.byId(this.userGridID);
			if (userGrid) {
			    userGrid.notifyLDAPEnablement(enable);
			    dijit.byId(this.groupGridID).notifyLDAPEnablement(enable);
			}
		},
		
		_createObject: function(values, dialog, onFinish, onError) {
            console.debug("Creating... ");
			
			console.debug("data="+JSON.stringify(values));
			
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("LDAP", values);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(
					function(data) {
				    	console.debug("LDAP rest POST returned ", data);
						if (values.Certificate && values.Certificate !== "") {
							dijit.byId(this.dialogId+"_clearCertButton").set('style',"display:block");
					    }
						onFinish();
					},
					function(error) {
				    	console.debug("error: "+error);
						dialog.showErrorFromPromise(nls.messaging.savingFailed, error);
						onError(error);
					});
			return;
		},

		// As of the ima15a release, the LDAP object can no longer be deleted.  The 
		// reset action sets all LDAP properties to default values.
		_resetObject: function(dialog, onFinish, onError) {
            console.debug("Resetting... ");
						
            var t = this;
    		var values = {};
			var fields = ["URL", "Certificate", "EnableCache", "BaseDN", "BindDN", "BindPassword", 
			                "UserIdMap", "UserSuffix","GroupMemberIdMap", "GroupIdMap", "GroupSuffix", "CheckServerCert"];
			for (var i=0, len=fields.length; i < len; i++) {
				var field = fields[i];
    			values[field] = "";
    		}
    		values["Timeout"] = t.defaultTimeout;
    		values["CacheTimeout"] = t.defaultCacheTimeout;
    		values["GroupCacheTimeout"] = t.defaultGroupCacheTimeout;
    		values["Enabled"] = t.defaultEnabled;
    		values["Verify"] = t.defaultVerify;
    		values["IgnoreCase"] = t.defaultIgnoreCase;
    		values["EnableCache"] = t.defaultEnableCache;
        	values["NestedGroupSearch"] = t.defaultNestedGroupSearch;

			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("LDAP", values);
			var promise = adminEndpoint.doRequest("/configuration", options);

			promise.then(onFinish, onError);
			return;
		},
		
		_initButton: function() {
			var buttonId = "enableLDAPButton";
			domConstruct.place("<div id='"+buttonId+"'></div>", this.contentId);
			this.enableButton = new Button({
	            label: nls.messaging.externldap.enableButton.enable
	            }, buttonId);
			this.enableButton.set("disabled", true);
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar
			var instruction = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.instruction,{nls:nls})));
			
			// create Edit dialog
			console.debug("id="+this.id+" contentId="+this.contentId);
			console.debug("editDialogTitle="+this.editDialogTitle);
			var editTitle = Utils.escapeApostrophe(Utils.escapeApostrophe(dojo.string.substitute(this.editDialogTitle,{nls:nls})));
			var editId = "edit"+this.id+"Dialog";
			var editDialoagNode = editId+"Node";
			domConstruct.place("<div id='"+editDialoagNode+"'></div>", this.contentId);

			this.editDialog = new LDAPDialog({dialogId: editId, dialogTitle: editTitle, validChars: this.validChars,
				idName: this.idName, instruction: instruction, uploadAction: this.uploadAction}, editDialoagNode);
			this.editDialog.startup();
			
			// create restart info dialog
			domConstruct.place("<div id='restartInfo"+this.id+"Dialog'></div>", this.contentId);
			var restartDialogId = "restartInfo"+this.id+"Dialog";
			var restartTitle = Utils.escapeApostrophe(nls.messaging.externldap.restartInfoDialog.title);
			this.restartInfoDialog = new Dialog({
				id: restartDialogId,
				title: restartTitle,
				content: "<div>" + Utils.escapeApostrophe(nls.messaging.externldap.restartInfoDialog.content) + "</div>",
				closeButtonLabel: nls.messaging.externldap.restartInfoDialog.closeButton
			}, restartDialogId);
			this.restartInfoDialog.dialogId = restartDialogId;

			
			// create a place for the warning dialog
			domConstruct.place("<div id='"+this.id+"WarningDialog'></div>", this.contentId);
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
				{	id: "URL", name: nls.messaging.externldap.grid.url, field: "URL", dataType: "string", width: "640px",
						decorator:this._urlDecorator
				}
			];	
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, {heightTrigger: 0});
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
			
			this.testButton = new MenuItem({label: nls.messaging.dialog.testButton});
			GridFactory.addToolbarMenuItem(this.grid, this.testButton);
			this.testButton.set('disabled', true);
			
			this.resetButton = new MenuItem({label: nls.resetLdapButton});
			GridFactory.addToolbarMenuItem(this.grid, this.resetButton);
			this.resetButton.set('disabled', true);
		},
		
		_urlDecorator: function(name) {
			if (name === "") {
				return nls.emptyLdapURL;
			} else {
				return Utils.nameDecorator(name);
			}
		},

		_getSelectedData: function() {
			// summary:
			// 		get the row data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).data();
		},

		/* Server always changes uploaded cert file to ldap.pem.  So we should store this now "dummy" value
		 * to match in the store.  And on GET, the server always sends XXXXXX as the BindPassword value if the
         * BindPassword value is not empty (""). The Web UI should always store this "dummy" value in order to 
		 * protect the user.  This is a safer approach because the Web UI has no way to know whether the LDAP
		 *  BindPassword value has been changed via REST or via a different Web UI instance or different Web UI user.
		 */
		_maskContent: function(values) {
			if (values.Certificate && (values.Certificate !== undefined) && values.Certificate !== "") {
				values.Certificate = "ldap.pem";
			}
			if (values.BindPassword && (values.BindPassword !== undefined) && values.BindPassword !== "") {
				values.BindPassword = "XXXXXX";
			}
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
		},
		
		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
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
		    
			on(this.testButton, "click", lang.hitch(this, function() {
			    this._testSelectedRow();	    		
			}));
			
			on(this.resetButton, "click", lang.hitch(this, function() {
			    this._resetLdapDefaults();	    		
			}));			
			
			dojo.subscribe(this.editDialog.dialogId + "_testButton", lang.hitch(this, function(message) {
				
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
					this.editDialog.tryUpload(lang.hitch(this, function() {
						this._doTestConnection(this.editDialog, this.editDialog.dialogId + "_testButton");
					}));
				}
			}));
			
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
				    var values = this.editDialog.getValues();
					var vchanges = this.editDialog.checkChangedValues();
					var _this = this;
					console.debug("performing edit, dialog values:", values);
					if (vchanges) { 
						var bar = query(".idxDialogActionBarLead",_this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;

						values.Enabled = _this.LDAPenabled;
						
						_this.editDialog.tryUpload(lang.hitch(this, function() {
							_this._editObject(vchanges, _this.editDialog.dialog, function(data) {
								_this._maskContent(values);
								_this.editDialog.save(_this.store, values);
				                _this.enableButton.set("disabled", false);
				                _this.testButton.set("disabled", false);
				                _this.resetButton.set("disabled", false);
								_this.editDialog.dialog.hide();
								_this._refreshGrid();			
							}, 
							function() {
								console.debug("edit failed");
								bar.innerHTML = "";
							});
						}));
					}
					else {
						console.debug("nothing to change, closing dialog");
						_this.editDialog.dialog.hide();
					}
				}
			}));
			
			on(this.enableButton, "click", lang.hitch(this, function() {
			    console.log("clicked enable button, cuurent status: ", this.LDAPenabled);
			    console.debug("nls test", nls.messaging.externldap.enableButton.enable, nls.messaging.externldap.enableButton.disable);
			    if(this.grid.rowCount() > 0) {
			    	this._enableTopObject(!this.LDAPenabled);
			    }
			}));
		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				console.debug("index="+ind);
				console.log("row count: "+this.grid.rowCount());
				this.editButton.set("disabled", false);
				var rowData = this._getSelectedData();
				if (rowData.URL !== "" && rowData.URL !== undefined) {
					// Only enable test connection if the URL is set
				    this.testButton.set('disabled', false);
				    this.resetButton.set("disabled", false);
				}
			}
		},
		
		_doEdit: function() {
			var rowData = this._getSelectedData();
			console.debug("Row Data: ", rowData);
			// retrieve full object data
			var fullData = this.store.query(function(item){ return item.URL == rowData.URL; })[0];
			console.debug("Full Data: ", fullData);
			this.editDialog.show(fullData);
		},
		
		_doTestConnection: function(testConnectionDialog, testConnectionButtonId) {
			// any message on this topic means that we clicked "test connection" on the dialog
			var dialog = testConnectionDialog.dialog? testConnectionDialog.dialog : testConnectionDialog;
			var button = dijit.byId(testConnectionButtonId);
			var connectionProperties =  {valid: false};
			if (testConnectionDialog.getTestConnectionProperties) {
				connectionProperties = testConnectionDialog.getTestConnectionProperties();
			} 
			if (button) {
				button.set('disabled', true);
			}
			var _this = this;
			dialog.clearErrorMessage();
			_this.updateProgressIndicator("testing", dialog);
			var dialogCounter = testConnectionDialog.dialogCounter;
			delete connectionProperties.valid;

			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("LDAP", connectionProperties);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(
					function(data) {
				    	if (dialogCounter === testConnectionDialog.dialogCounter) {
							if (button) {
								button.set('disabled', false);
							}
				    		// test successful
				    		_this.updateProgressIndicator("success", dialog);
				    	}
					},
					function(error) {
				    	if (dialogCounter === testConnectionDialog.dialogCounter) {
							if (button) {
								button.set('disabled', false);
							}
							_this.updateProgressIndicator("clear", dialog);
							dialog.showErrorFromPromise(nls.messaging.testConnectionFailed, error);
				    	}
					});
		},
		
		_testSelectedRow: function() {
		    var rowData = this._getSelectedData();
			var fullData = this.store.query(function(item){ return item.URL == rowData.URL; })[0];
			
		    var t = this;
    		if (!t.testingDialog) {
				var testingDialogId = "testingInfo"+t.id;
				domConstruct.place("<div id='"+testingDialogId+"'></div>", t.contentId);
				t.testingDialog = new Dialog({
					id: testingDialogId,
					title: nls.messaging.dialog.testButton,
					closeButtonLabel: nls.messaging.dialog.closeButton
				}, testingDialogId);
				t.testingDialog.dialogId = testingDialogId;
				t.testingDialog.dialogCounter = 0;
    		} 

    		t.testingDialog.clearErrorMessage();
    		t.testingDialog.setContent("<div><img src=\"css/images/loading_wheel.gif\" alt=''/>&nbsp;" + 
	    			lang.replace(nls.messaging.dialog.testingContent, [rowData.URL]) + "</div>");
    		// prevent two tests at the same time
	    	this.testButton.set('disabled', true);
    		t.testingDialog.show();
    		
			var excludeFields = ["gridId", "Certificate", "BindPassword"];
			var values = {};
			for (var field in fullData) {
				if (excludeFields.indexOf(field) < 0) {
					values[field] = fullData[field];					
				}
			}
			values["Verify"] = true;
    		
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("LDAP", values);
			var promise = adminEndpoint.doRequest("/configuration", options);
			
			promise.then(
					function(data) {			    	
		    			// test successful
		    	    	t.testingDialog.setContent("<div><img src=\"css/images/msgSuccess16.png\" alt=''/>&nbsp;" + 
		    	    			lang.replace(nls.messaging.dialog.testingSuccessContent, [rowData.URL]) + "</div>");
			    		t.testButton.set('disabled', false);
					},
					function(error) {
						t.testingDialog.setContent("<div></div>");
						t.testingDialog.showErrorFromPromise(lang.replace(nls.messaging.dialog.testingFailedContent, [rowData.Name]), error);
			    		t.testButton.set('disabled', false);
					});
		},
		
		_resetLdapDefaults: function() {
		    var t = this;
    		if (!t.resetDialog) {
				var resetDialogId = "resetLdap"+t.id;
				domConstruct.place("<div id='"+resetDialogId+"'></div>", t.contentId);

				t.resetDialog = new Dialog({
					id: resetDialogId,
					title: nls.resetLdapTitle,
					content: nls.resetLdapContent,
					buttons: [
								new Button({
									id: resetDialogId + "_button_ok",
									label: nls.resetLdapButton,
									onClick: function() { 
										dojo.publish(resetDialogId + "_saveButton", ""); 
									}
								})
							],
					closeButtonLabel: nls.messaging.dialog.closeButton
				}, resetDialogId);
				t.resetDialog.dialogId = resetDialogId;
				t.resetDialog.dialogCounter = 0;
				
				dojo.subscribe(t.resetDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
					var _this = t;
					var bar = query(".idxDialogActionBarLead",_this.resetDialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/>"  + nls.resettingProgress;
					_this._resetObject(_this.resetDialog, function(data) {
				    	_this._enableLDAP(false);
						_this.enableButton.set("disabled", true);
						_this.editButton.set("disabled", true);
						_this.resetButton.set("disabled", true);
						_this.testButton.set('disabled', true);
						_this._initStore();
						_this._refreshGrid();
						_this.resetDialog.hide();
						bar.innerHTML = "";
					}, 
	                function(error) {
						console.debug("reset failed");
						bar.innerHTML = "";
					});
				}));
    		} 

    		t.resetDialog.show();
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
		}
		
		
	});

	return LDAPGrid;
});
