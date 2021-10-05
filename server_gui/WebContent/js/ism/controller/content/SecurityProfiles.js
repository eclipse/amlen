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
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dojo/when',
		'dojo/promise/all',
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
		'ism/widgets/CheckBoxList',
		'ism/widgets/TextBox',
		'ism/widgets/Select',
		'ism/IsmUtils',
		'ism/widgets/IsmQueryEngine',
		'dojo/text!ism/controller/content/templates/SecurityProfileDialog.html',
		'dojo/text!ism/controller/content/templates/ClientCertificatesDialog.html',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/securityProfiles',
		'ism/config/Resources',
		'dojo/keys'
		], function(declare, lang, aspect, query, on, domClass, domConstruct, domStyle,
				Memory, when, all, entities, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				MenuItem, Button, Form, GridFactory, Dialog, CheckBoxList, TextBox, Select, Utils,
				IsmQueryEngine, secProfileDialog, clientCertsDialog, nls, nls_strings, nls_sp, Resources, keys) {

	/*
	 * This file defines a configuration widget that combines a grid, toolbar, and set of dialogs for editing.  All
	 * Dialog widgets are defined inline (rather than in their own file) for simplicity, but could easily be separated
	 * and referenced from an external file.
	 */
    
    /**
     * Get all certificates and return the data arranged by profile. For example { "profile1" : ["cert1.pem", "cert2.pem"] }
     */
    var getTrustedCertificatesByProfile = function(certType, onComplete, onError) {
        var _this = this;
        var adminEndpoint = ism.user.getAdminEndpoint();
        var promise = adminEndpoint.doRequest("/configuration/"+certType, adminEndpoint.getDefaultGetOptions());
        promise.then(
            function(data) {
                var certificates = {};
                if (data) {
                    var trustedCertificates = data[certType];
                    if(trustedCertificates && trustedCertificates.length > 0) {
                    	var certTypeName = certType === "TrustedCertificate" ? certType : "CertificateName";
                        for (var i=0, count = trustedCertificates.length; i < count; i++) {
                            var tc = trustedCertificates[i]; 
                            if (tc && tc.SecurityProfileName && tc[certTypeName]) {  
                            	if (!certificates[tc.SecurityProfileName])
                                    certificates[tc.SecurityProfileName] = [];
                            	certificates[tc.SecurityProfileName].push(tc[certTypeName]);
                            }
                        }
                    }
                }
                if (onComplete) onComplete(certificates);
            },
            function(error) {
                if (onError) onError(error);
            }
        );                 
    };

    /**
     * Gets an array of trusted certificate file names for the specified securityProfile
     */
    var getTrustedCertificatesForProfile = function(securityProfileName, certType, onComplete, onError) {
        var _this = this;
        getTrustedCertificatesByProfile(certType, function(data) {
            var mapped = [];
            if (data && data[securityProfileName]) {
                var certificates = data[securityProfileName];
                mapped = dojo.map(certificates, function(item){
                    var certificate = {};
                    certificate.id = escape(item);
                    certificate.name = item;
                    return certificate;
                });
            }
            if (onComplete) onComplete(mapped);
            }, onError
        );
    };

	var SecProfileDialog = declare("ism.controller.content.SecurityProfiles.SecProfileDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
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
		nls_sp: nls_sp,
		templateString: secProfileDialog,
		add: true,

        // template strings
        nameLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.grid.name),
        nameTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.nameTooltip),
        mprotocolLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.grid.mprotocol),
        mprotocolTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.mprotocolTooltip),
        mprotocolTooltipDisabled: Utils.escapeApostrophe(nls_sp.mprotocolTooltipDisabled),
        ciphersLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.grid.ciphers),
        ciphersTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.ciphersTooltip),
        ciphersTooltipDisabled: Utils.escapeApostrophe(nls_sp.ciphersTooltipDisabled),
        certprofileLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.grid.certprofile),
        certprofileTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.certprofileTooltip),
        certprofileTooltipDisabled: Utils.escapeApostrophe(nls_sp.certprofileTooltipDisabled),
        ltpaprofileLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.grid.ltpaprofile),
        ltpaprofileTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.ltpaprofileTooltip),
        oAuthProfileLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.grid.oAuthProfile),
        oAuthProfileTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.oAuthProfileTooltip),
        tlsEnabledLabel: Utils.escapeApostrophe(nls_sp.tlsEnabled),
        tlsEnabledLabelToolTip: Utils.escapeApostrophe(nls_sp.tlsEnabledToolTip),
        certauthTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.certauthTooltip),
        certauthTooltipDisabled: Utils.escapeApostrophe(nls_sp.certauthTooltipDisabled),
        clientciphersTooltip: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.clientciphersTooltip),
        clientciphersTooltipDisabled: Utils.escapeApostrophe(nls_sp.clientciphersTooltipDisabled),

		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			if (this.add === true || Utils.configObjName.editEnabled()) {			
				// check that the userid matches the pattern and doesn't clash with an existing object
				this.field_Name.isValid = function(focused) {
					var value = this.get("value");
					var validate =  Utils.configObjName.validate(value, this.existingNames, focused, true, true);
					if (!validate.valid) {
						this.invalidMessage = validate.message;									
						return false;
					}
					return true;				
				};
			} else {
				this.field_Name.set('readOnly', true);	
			}
			
			this.field_CertificateProfile.isValid = function(focused) {
				if (this.get("required") === true) {
					var value = this.get("value");
					if (!value || value === "chooseOne") {
						this.invalidMessage = nls_sp.noCertProfileSelected;
						return false;
					}
				}
				return true;
			};
			
			var dialogId = this.dialogId;
			
			// watch the form for changes between valid and invalid states
			dijit.byId(dialogId+"_form").watch('state', function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(dialogId+"_saveButton");
				if (newvalue) {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			});
		},
		
		changePasswordAuth: function(checked) {
			console.log("changePasswordAuth " + checked);
			if (checked !== undefined) {
				this.field_LTPAProfile.set('disabled', !checked);
				this._updateLTPAProfileTooltip();
				this.field_OAuthProfile.set('disabled', !checked);
				this._updateOAuthProfileTooltip();				
			}
		},
		
		changeTlsEnabled: function(checked) {
			console.log("changeTlsEnabled " + checked);
			if (checked !== undefined) {				
				this.field_CertificateProfile.setRequiredOnOff(checked);
				this.field_CertificateProfile.set('disabled', !checked);
				this._updateCertificateProfileTooltip();
				this.field_MinimumProtocolMethod.set('disabled', !checked);
				this._updateMinimumProtocolMethodTooltip();
				this.field_Ciphers.set('disabled', !checked);
				this._updateCiphersTooltip();
				this.field_UseClientCipher.set('disabled', !checked);
				this._updateUseClientCipherTooltip();
				this.field_UseClientCertificate.set('disabled', !checked);
				this._updateUseClientCertificateTooltip();
				// Force focus to assure a warning is provided if no certificate profile is specified
				if (checked === true)
				    this.field_CertificateProfile.focus();
			}
		},
		
		changeLTPAProfile: function(value) {
			console.log("changeLTPAProfile " + value);
			if (value !== undefined && !this.field_LTPAProfile.get('disabled')) {
				this.field_UsePasswordAuthentication.set('disabled', value !== "chooseOne");
				this._updateUsePasswordAuthenticationTooltip();
				this.field_OAuthProfile.set('disabled', value !== "chooseOne");
				this._updateOAuthProfileTooltip();				
			}
		},
		
		changeOAuthProfile: function(value) {
			console.log("changeOAuthProfile " + value);
			if (value !== undefined  && !this.field_OAuthProfile.get('disabled')) {
				this.field_UsePasswordAuthentication.set('disabled', value !== "chooseOne");
				this._updateUsePasswordAuthenticationTooltip();
				this.field_LTPAProfile.set('disabled', value !== "chooseOne");
				this._updateLTPAProfileTooltip();				
			}
		},

		
		_updateLTPAProfileTooltip: function() {
			var tooltipText = this.field_LTPAProfile.get('disabled') ? nls.appliance.securityProfiles.dialog.ltpaprofileTooltipDisabled : nls.appliance.securityProfiles.dialog.ltpaprofileTooltip;
			var tooltip = dijit.byId(this.field_LTPAProfile.id+"_Tooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},
		
		_updateOAuthProfileTooltip: function() {
			var tooltipText = this.field_OAuthProfile.get('disabled') ? nls.appliance.securityProfiles.dialog.oAuthProfileTooltipDisabled : nls.appliance.securityProfiles.dialog.oAuthProfileTooltip;
			var tooltip = dijit.byId(this.field_OAuthProfile.id+"_Tooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}						
		},

		_updateUsePasswordAuthenticationTooltip: function() {
			var tooltipText = this.field_UsePasswordAuthentication.get('disabled') ? nls.appliance.securityProfiles.dialog.passwordAuthTooltipDisabled : nls.appliance.securityProfiles.dialog.passwordAuthTooltip;
			var tooltip = dijit.byId(this.dialogId+"_passwordAuthTooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},
		
		_updateCertificateProfileTooltip: function() {
			var tooltipText = this.field_CertificateProfile.get('disabled') ?  nls_sp.certprofileTooltipDisabled : nls.appliance.securityProfiles.dialog.certprofileTooltip;
			var tooltip = dijit.byId(this.field_CertificateProfile.id+"_Tooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},
		
		_updateMinimumProtocolMethodTooltip: function() {
			var tooltipText = this.field_MinimumProtocolMethod.get('disabled') ?  nls_sp.mprotocolTooltipDisabled : nls.appliance.securityProfiles.dialog.mprotocolTooltip;
			var tooltip = dijit.byId(this.field_MinimumProtocolMethod.id+"_Tooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},

		_updateCiphersTooltip: function() {
			var tooltipText = this.field_Ciphers.get('disabled') ?  nls_sp.ciphersTooltipDisabled : nls.appliance.securityProfiles.dialog.ciphersTooltip;
			var tooltip = dijit.byId(this.field_Ciphers.id+"_Tooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},
		
		_updateUseClientCertificateTooltip: function() {
			var tooltipText = this.field_UseClientCertificate.get('disabled') ?  nls_sp.certauthTooltipDisabled : nls.appliance.securityProfiles.dialog.certauthTooltip;
			var tooltip = dijit.byId(this.dialogId+"_certauthTooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},
		
		_updateUseClientCipherTooltip: function() {
			var tooltipText = this.field_UseClientCipher.get('disabled') ?  nls_sp.clientciphersTooltipDisabled : nls.appliance.securityProfiles.dialog.clientciphersTooltip;
			var tooltip = dijit.byId(this.dialogId+"_clientciphersTooltip");
			if (tooltip) {
				tooltip.domNode.innerHTML = Utils.escapeApostrophe(tooltipText);
			}			
		},
		
		// populate the dialog with the passed-in values
		fillValues: function(values) {
			this._startingValues = values;
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}
						
		},
		
		show: function(list,certNames,ltpaNames,oAuthNames,values) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_Name.existingNames = list;
			
			if (this.add) {
				dijit.byId(this.dialogId+"_form").reset();
			}
			dijit.byId(this.dialogId+"_saveButton").set('disabled',this.add);
			
			// when certificate and ltpa profile names are available, continue...
			var adminEndpoint = ism.user.getAdminEndpoint();
			var _this = this;
			adminEndpoint.getAllQueryResults([certNames, ltpaNames, oAuthNames], ["CertificateProfile", "LTPAProfile","OAuthProfile"],
			    function(results, rawResults) {
    			    _this._updateSelect(_this.field_CertificateProfile, rawResults.CertificateProfile, true);
    			    _this._updateSelect(_this.field_LTPAProfile, rawResults.LTPAProfile, true);
    			    _this._updateSelect(_this.field_OAuthProfile, rawResults.OAuthProfile, true);								
    			    if (values) {
    			        // this is an edit dialog
    			        _this.fillValues(values);
    			        // if Use Password Authentication is not checked, cannot select an LTPA profile or OAuth profile
    			        if (!_this._getFieldValue("UsePasswordAuthentication")) {
    			            _this.field_LTPAProfile.set('disabled', true);
    			            _this.field_OAuthProfile.set('disabled', true);
    			        } else {
    			            var ltpaProfileSelected = _this._getFieldValue("LTPAProfile") !== "";
    			            var oAuthProfileSelected = _this._getFieldValue("OAuthProfile") !== "";						
    			            _this.field_LTPAProfile.set('disabled', oAuthProfileSelected);
    			            _this.field_OAuthProfile.set('disabled', ltpaProfileSelected);
    			            // if an LTPA Profile or OAuth Profile is selected, don't allow Use Password Authentication to be unchecked
    			            if (ltpaProfileSelected || oAuthProfileSelected) {
    			                _this.field_UsePasswordAuthentication.set('disabled', true);
    			            }
    			        }
    			        _this._updateLTPAProfileTooltip();
    			        _this._updateOAuthProfileTooltip();
    			        _this._updateUsePasswordAuthenticationTooltip();
    			    } 		
    
    			    _this.dialog.show();
    
    			    if ((_this.add) && (_this._getFieldValue("TLSEnabled"))) {
    			        // show a warning if there are no certificate profiles
    			        if (rawResults.CertificateProfile.length === 0) {
    			            _this.dialog.showWarningMessage(nls.appliance.securityProfiles.dialog.noCertProfilesTitle,
    			                    nls.appliance.securityProfiles.dialog.noCertProfilesDetails);
    			        }
    			        // Force focus to assure a warning is provided if no certificate profile is specified
    			        _this.field_CertificateProfile.focus();
    			    }
    			    _this.field_Name.focus(); 
    			}
			);			
		},
		
		_updateSelect: function(select, newValues, addSelectOne) {
			if (!select || !newValues) {
				return;
			}
			select.removeOption(select.getOptions());			
    		var mapped = dojo.map(newValues, function(item, index){
    			return { label: dojox.html.entities.encode(item), value: item };
    		});    		
    		if (addSelectOne) {
    			select.addOption({label: Utils.escapeApostrophe(nls.appliance.securityProfiles.dialog.chooseOne), value: "chooseOne", selected: true});    			
    		}    		
			select.addOption(mapped);
			select.validate();
		},
		
		// check to see if anything changed, and save the record if so
		save: function(onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var values = {};
			var recordChanged = this.add;
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
				onFinish(values);
			} else {
				console.debug("nothing changed, so no need to save");
				// Note: The check for recordChanged gives false positives, so we'll usually end
				// up saving. Not going to optimize this right now, though...
				this.dialog.hide();
			}
		},

		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");
			switch (prop) {
				case "UseClientCertificate":
				case "UseClientCipher":
				case "UsePasswordAuthentication":
				case "TLSEnabled":
					if (value === true || value === false) {
						this["field_"+prop].set("checked", value);						
					} else if (value.toString().toLowerCase() == "true") {
						this["field_"+prop].set("checked", true);
					} else {
						this["field_"+prop].set("checked", false);
					}
					break;										
				case "LTPAProfile":
				case "OAuthProfile":
					if (value === "") {
						value = "chooseOne";
					}
					// let fall through
					/*jsl:fallthru*/
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
				case "UseClientCertificate":
				case "UseClientCipher":
				case "UsePasswordAuthentication":
				case "TLSEnabled":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "CertificateProfile":
				case "LTPAProfile":
				case "OAuthProfile":					
					value = this["field_"+prop].get("value");
					if (value === "chooseOne") {
						value = "";
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
		}
	});

	
	var CertificatesDialog = declare("ism.controller.content.CertificatesDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {

		nls: nls,
		nls_strings: nls_strings,
		templateString: clientCertsDialog,	

		// template strings
        instruction: Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.instruction),
        certLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.certificate),
        browseHint: Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.browseHint),
        browseLabel: Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.browse),

		_certificatesToUpload: [],
		uploadBatchSize: 10,

		// variable to keep count of uploaded files
		uploadedFileCount: null,
		uploadTimeout: 30000, // milliseconds
		timeoutID: null,
		restURI: null,
		store: null,
		retry: true,  // to prevent multiple retries
		grid: null,
		contentId: null,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
			// Append truststore or clientcerts to distinguish list
			// content for trusted CA certs vs trusted client certs
			this.contentId = this.id + this.certType;
			this.store = new Memory({data:[]});
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			// start with an empty <div> for the grid content
			this.certificatesGridDiv.innerHTML = "<div id='"+this.contentId+"'></div>";
			this._initGrid();
			this._initDialogs();			
			this._initEvents();								
		},
				
		_initStore: function() {
			// summary:
			// 		get the trusted certificates from the server
            var _this = this;
            getTrustedCertificatesForProfile(this.securityProfile, this.certType, 
                function(data) {
                        _this.store = new Memory({data: data});
                        _this.grid.setStore(_this.store);
                        _this.triggerListeners();                           
                }, function(error) {
                    _this.dialog.showErrorFromPromise(nls.appliance.securityProfiles.certsDialog.retrieveError, error);
                }
            );          
		},
		
		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "name", name: nls.appliance.securityProfiles.certsDialog.certificate, field: "name", dataType: "string", width: "500px",
						decorator: Utils.nameDecorator
					}];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true});
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.removeTitle);
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.removeContent) + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_saveButton",
						label: nls.appliance.securityProfiles.certsDialog.remove,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.appliance.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;			
		},
		
		_initEvents: function() {
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected: "+this.grid.select.row.getSelected());
				if (this.grid.select.row.getSelected().length > 0) {
					this.deleteButton.set('disabled',false);
				} else {
					this.deleteButton.set('disabled',true);
				}
			}));

			// reset the form on close
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				this._certificatesToUpload = [];
				dijit.byId(this.dialogId+"_form").reset();
				if (this.onHide) {
					this.onHide(this.securityProfile, this.parentWidget);
				}
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				this.removeDialog.show();
			}));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var name = unescape(id);
				this._deleteEntry(name, lang.hitch(this, function(data) {
					this.store.remove(id);
					bar.innerHTML = "";
					this.removeDialog.hide();				
					// disable buttons if empty or no selection
					if (this.grid.store.data.length === 0 || 
							this.grid.select.row.getSelected().length === 0) {
						this.deleteButton.set('disabled',true);
					}	
					this.triggerListeners();
				}));
			}));
		},

        clickCertificateBrowseButton: function() {
            this.certificateFile.click();
        },

        handleFileChangeEvent: function() {                      
            var files = this.certificateFile.files;
            var field = this.field_Certificate;
            if (files && files.length > 0) {
            	this.uploadButton.set('disabled', false);
                var filename = [];
                for (var i = 0; i < files.length; i++) {
                	filename[i] = files[i].name;
                }
                field.set("value", filename);  
                if (field.validate()) {
                    this._certificatesToUpload = files;
                } else {
                    this._certificatesToUpload = [];
                }                  
            } else {
            	this.uploadButton.set('disabled', true);
                field.set("value", "");
                this._certificatesToUpload = [];
            }           
        },                      

		_deleteEntry: function(name, onLoad) {
            var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/"+this.certType+"/"+encodeURIComponent(this.securityProfile)+"/"+encodeURIComponent(name), 
                    adminEndpoint.getDefaultDeleteOptions());
            promise.then(onLoad, function(error) {
                    var dialog = _this.removeDialog;
                    query(".idxDialogActionBarLead",dialog.domNode)[0].innerHTML = "";
                    dialog.showErrorFromPromise(nls.appliance.securityProfiles.certsDialog.deleteError, error);
                    _this._initStore();                     
                });
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh().then(lang.hitch(this, function() {
				this.grid.resize();
			}));
		},

		
		show: function(securityProfile, onHide, parentWidget) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.securityProfile = securityProfile.Name;
			this.onHide = onHide;
			this.parentWidget = parentWidget;
			this.uploadButton.set('disabled',true);
			this._initStore();
			this.dialog.show();				
			this.certificateFile.value = '';
			this.certificateFile.focus();
		},
		
		uploadComplete: function(file, certsremaining, failed) {
			console.debug("upload complete for " + file.name);
			var certificate = {};			
			certificate.name = file.name;
			certificate.id = escape(certificate.name);

            // Apply it!
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultPostOptions();
			var data = undefined;
			// For consistency with pervious releases, always force overwrite when a new certificate has the same name as an
			// already existing certificate.
			if (this.certType === "TrustedCertificate") 
				data = { TrustedCertificate: [{ SecurityProfileName: this.securityProfile, TrustedCertificate: certificate.name, Overwrite: true }] };
			else if (this.certType === "ClientCertificate") 
				data = { ClientCertificate: [{ SecurityProfileName: this.securityProfile, CertificateName: certificate.name, Overwrite: true }] };
			options.data = JSON.stringify(data);
			var _this = this;
			var _file = file;
			var _certsremaining = certsremaining;
			var _failed = failed;
			adminEndpoint.doRequest("/configuration", options).then(function(data) {
	            var result = _this.store.query({id: certificate.id});
	            var method = "add";
	            if (result.length > 0) {
	                method = "put";
	            }
	            when(_this.store[method](certificate), function(){
	                dijit.byId(_this.dialogId+"_form").reset();
	                for (var i = 0; i < _certsremaining.length; i++) {
	                	if (certificate.name === _certsremaining[i].name) {
	                		_certsremaining.splice(i,1);
	                		break;
	                	}
	                }
	                if ((_certsremaining.length === 0) && (_failed.length > 0)) {
	    				_this.dialog.showErrorList(nls.appliance.securityProfiles.certsDialog.uploadError, _failed);
	                }
	                _this.triggerListeners();
	            });    			    
			}, function(error) {
				console.debug("Failed to add "+_this.certType+", "+certificate.name);
				_failed.push({id: _failed.length, name: certificate.name, errorMessage: error.response.data.Code + ":" + error.response.data.Message});
                for (var i = 0; i < _certsremaining.length; i++) {
                	if (certificate.name === _certsremaining[i].name) {
                		_certsremaining.splice(i,1);
                		break;
                	}
                }
                if (_certsremaining.length === 0) {
    				_this.dialog.showErrorList(nls.appliance.securityProfiles.certsDialog.uploadError, _failed);
                }
			});			
		},
		
		handleError: function(error) {
            this.dialog.showErrorFromPromise(nls.appliance.securityProfiles.certsDialog.uploadError, error);
			this._initStore();
			this._certificatesToUpload = [];
			this.field_Certificate.set("value", "");
			this.uploadedFileCount = 0;
		},
		
		upload: function() {

			this.uploadedFileCount = 0;
			
			var onFinish = function(t, results) {
				// This function will be used to report failed/pending uploads
				console.debug("finished uploading");
				query(".idxDialogActionBarLead",t.dialog.domNode)[0].innerHTML = "";
            	t.uploadButton.set('disabled', true);
				t.field_Certificate.set("value", "");
				t.certificateFile.value = '';
                t._certificatesToUpload = [];
                if (results && results.failed && results.failed.length > 0) {
    				t.dialog.showErrorList(nls.appliance.securityProfiles.certsDialog.uploadError, results.failed);
               	
                }
			};

			if (this._certificatesToUpload.length > 0) {
				var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.uploadingProgress;
				this.uploadButton.set('disabled',true);
				var failed = [];
				var success = [];
				var pending = [];
				var filestoupload = [];
				var certstoapply = [];
				for (var i = 0; i < this._certificatesToUpload.length; i++) {
					pending.push({id:this._certificatesToUpload[i].name, file: this._certificatesToUpload[i]});
					filestoupload.push(this._certificatesToUpload[i]);
					certstoapply.push(this._certificatesToUpload[i]);
				}
				this._processUploadRequests(filestoupload, certstoapply, onFinish, success, failed, pending);
			} 
		},

		handleResolved: function(response, args/* name, files, allfiles, onFinish, success, failed, pending */) {
			var t = this;
			var name = args.name;
			var files = args.files;
			var filestoupload = args.filestoupload;
			var certstoapply = args.certstoapply;
			var onFinish = args.onFinish;
			var success = args.success;
			var failed = args.failed;
			var pending = args.pending;
			
			for (var j = 0; j < files.length; j++) {
				if (files[j].name == name) {
					console.debug("resolved: " + name + " " + JSON.stringify(response));
					
					// Add the name to the array of successful requests
					success.push({id: name, code: response.Code, message: response.Message});
					
					for (var k = 0; k < pending.length; k++) {
						if (pending[k].id === name) {
							pending.splice(k, 1);
							break;
						}
					}
					
					// Check to see if we have finished processing the current batch.  If so, cancel
					// the batch timeout.  Then, if not all IDs have been deleted, make a recursive 
					// call to _processUploadRequests in order to process the next batch.  Otherwise,
					// call onFinish.
					files.splice(j, 1);
					if (files.length === 0) {
						if (t.timeoutID) {
							window.clearTimeout(t.timeoutID);
						}
						console.debug("Succeeded: " + JSON.stringify(success));
						if (filestoupload.length) {
							t._processUploadRequests(filestoupload, certstoapply, onFinish, success, failed, pending);
						} else {
							onFinish(t, {success: success, failed: failed, pending: pending});
						}
					}
					break;
				}
			}
		},
		
		handleRejected: function(response, args /* name, files, allfiles, onFinish, success, failed, pending */) {
			var t = this;
			var name = args.name;
			var files = args.files;
			var filestoupload = args.filestoupload;
			var certstoapply = args.certstoapply;
			var onFinish = args.onFinish;
			var success = args.success;
			var failed = args.failed;
			var pending = args.pending;

			for (var j = 0; j < files.length; j++) {
				if (files[j].name === name) {
					console.debug("rejected: "+ name + " " + JSON.stringify(response));
					
					// Add the ID to the array of failed requests
					var item = {id: failed.length, name: name, errorMessage: response.response.data.Code + ":" + response.response.data.Message};
					failed.push(item);
					
					// Remove the name from the array of pending requests
					for (var k = 0; k < pending.lenght; k++) {
						if (pending[k].id === name) {
							pending.splice(k, 1);
							break;
						}
					}
					
					// Check to see if we have finished processing the current batch.  If so, cancel
					// the batch timeout.  Then, if not all IDs have been deleted, make a recursive 
					// call to _processUploadRequests in order to process the next batch.  Otherwise,
					// call onFinish.
					files.splice(j, 1);

					if (files.length === 0) {
						if (t.timeoutID) {
							window.clearTimeout(t.timeoutID);
						}
						console.debug("Failed: " + JSON.stringify(failed));
						if (filestoupload.length) {
							t._processUploadRequests(filestoupload, certstoapply, onFinish, success, failed, pending);
						} else {
							onFinish(t, {success: success, failed: failed, pending: pending});
						}
					}
					break;
				}
			}
		},
		
		_processUploadRequests: function(filestoupload, certstoapply, onFinish, in_success, in_failed, in_pending) {
			var t = this;
			var upldReqs = [];
			var success = in_success;
			var failed = in_failed;
			var pending = in_pending;
			var batchSize = filestoupload.length > this.uploadBatchSize ? this.uploadBatchSize : filestoupload.length;
			var files = [];
			
			// When we enter this method, we are ready to process a new batch of delete requests.
			// If there's a pending timeout from an earlier batch, cancel it now.
			if (this.timeoutID) {
				 window.clearTimeout(this.timeoutID);
			}
			
			for (var i = 0; i < batchSize; i++) {
				files[i] = filestoupload[i];
			}
			filestoupload.splice(0, batchSize);
			
			for (i = 0; i < batchSize; i++) {
				var file = files[i];
				var args = {name: files[i].name, files: files, filestoupload: filestoupload, certstoapply: certstoapply, onFinish: onFinish, success: success, failed: failed, pending: pending};
				var uploadReq = {id: files[i].name, promise: ism.user.getAdminEndpoint()
						.uploadFile(file, function(data, file) {
                             t.uploadedFileCount++;
                             t.uploadComplete(file, certstoapply, failed);            
                         }, function(error) {
                             t.handleError(error);                        
                         }).trace({t: t, args: args})};
				upldReqs.push(uploadReq);
			}
			// schedule timeout
			this.timeoutID = window.setTimeout(lang.hitch(this, function(){
				this.handleError();
			}), this.uploadTimeout);			
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


	var SecurityProfiles = declare("ism.controller.content.SecurityProfiles", [_Widget, _TemplatedMixin], {
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
		
		addButton: null,
		deleteButton: null,
		editButton: null,
		
		queryEngine: IsmQueryEngine,
		
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		domainUid: 0,
		     			
		constructor: function(args) {
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
			dojo.subscribe(Resources.pages.appliance.nav.securitySettings.topic, lang.hitch(this, function(message){
				this._initStore();
			}));	
			dojo.subscribe(Resources.pages.appliance.nav.securitySettings.topic+":onShow", lang.hitch(this, function(message){
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
			var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/SecurityProfile", adminEndpoint.getDefaultGetOptions());
            promise.then(
                function(data) {
                    _this._populateProfiles(data);
                },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                        _this._populateProfiles({"SecurityProfile" : {}});
                    } else {
                        _this.isRefreshing = false;
                        _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
                        Utils.showPageErrorFromPromise(nls.appliance.securityProfiles.retrieveError, error);
                    }
                }
            );          
        },
        
        _populateProfiles: function(data) {
            var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var profiles = adminEndpoint.getObjectsAsArray(data, "SecurityProfile");
            var mapped = [];
            
            var mapData = function(certificateData) {
                // add an id property with an encoded form of the Name
                mapped = dojo.map(profiles, function(item){
                    Utils.convertStringsToBooleans(item, [{property: "TLSEnabled", defaultValue: true},
                                                          {property: "UseClientCertificate", defaultValue: false},
                                                          {property: "UseClientCipher", defaultValue: false},
                                                          {property: "UsePasswordAuthentication", defaultValue: true}]);
                    item.id = escape(item.Name);
                    item.CiphersDisplayName = _this._getCiphersDisplayName(item);
                    if (item.HasClientCerts === undefined || item.HasClientCerts === false) {
                    	item.HasClientCerts = (certificateData[item.Name] && certificateData[item.Name].length > 0);
                    }
                    item.MissingTrustedCertificates = false;
                    if (certificateData && item.UseClientCertificate) {
                        if (!item.HasClientCerts && (!certificateData[item.Name] || certificateData[item.Name].length === 0)) {
                            item.MissingTrustedCertificates = true;
                        }
                    }
                    return item;
                });
                
                _this.store = new Memory({data: mapped, idProperty: "id", queryEngine: _this.queryEngine});
                _this.grid.setStore(_this.store);
                _this.triggerListeners();                
            };
            
            if (profiles.length > 0) {
                // In order to show the missing client certificates warning, we need to get all the certificates...
                getTrustedCertificatesByProfile("TrustedCertificate", function(data){
                    mapData(data);
                }, function(error){
                    Utils.showPageErrorFromPromise(nls.errorRetrievingTrustedCertificates, error);
                    mapData();
                });
                getTrustedCertificatesByProfile("ClientCertificate", function(data){
                    mapData(data);
                }, function(error){
                    Utils.showPageErrorFromPromise(nls.errorRetrievingTrustedCertificates, error);
                    mapData();
                });
            } else {
                mapData();
            }
        },
		
		_getCiphersDisplayName: function(item) {
			var displayName = "";
			if (item && item.Ciphers) {
				var cipher = nls.appliance.securityProfiles.dialog.ciphers[item.Ciphers.toLowerCase()];
				displayName = cipher !== undefined ? cipher : "";
			}
			return displayName;
		},		
		
		_refreshEntry: function(profileName, widget) {
			var _this = this;
		    var adminEndpoint = ism.user.getAdminEndpoint();            
		    var promise = adminEndpoint.doRequest("/configuration/SecurityProfile/"+encodeURIComponent(profileName), adminEndpoint.getDefaultGetOptions());
            promise.then(function(data) {
                var profile = adminEndpoint.getNamedObject(data, "SecurityProfile", profileName);
                Utils.convertStringsToBooleans(profile, [{property: "TLSEnabled", defaultValue: true},
                                                         {property: "UseClientCertificate", defaultValue: false},
                                                         {property: "UseClientCipher", defaultValue: false},
                                                         {property: "UsePasswordAuthentication", defaultValue: true}]);

                if (profile) {
                    profile.id = escape(profile.Name);
                    var result = widget.store.query({id: profile.id});
                    if (result.length > 0) {
                    	profile.CiphersDisplayName = result[0].CiphersDisplayName;
                        if (profile.UseClientCertificate) {
                            getTrustedCertificatesForProfile(profile.Name, "TrustedCertificate", function(data) {
                                var certsNotfound = !data || data.length === 0;
                                if (certsNotfound) {
                                	getTrustedCertificatesForProfile(profile.Name, "ClientCertificate", function(data2) {
                                		profile.MissingTrustedCertificates = !data2 || data2.length === 0;
                                        widget.store.put(profile,{overwrite: true});
                                	}, function(error) {
                                		profile.MissingTrustedCertificates = false;
                                        widget.store.put(profile,{overwrite: true});
                                    });
                                } else {
                                	profile.MissingTrustedCertificates = false;
                                    widget.store.put(profile,{overwrite: true});
                                }
                            }, function(error) {
                            	profile.MissingTrustedCertificates = false;
                                widget.store.put(profile,{overwrite: true});
                            });          
                        } else {
                        	profile.MissingTrustedCertificates = false;
                            widget.store.put(profile,{overwrite: true});
                        }
                    }
                } 
            });
		},
		
		_saveSettings: function(values, dialog, onFinish, onError) {
            var _this = this;
            
            if (!onFinish) { onFinish = function() {    
                    dialog.hide();
                };
            }
            if (!onError) { onError = function(error) { 
                    query(".idxDialogActionBarLead", dialog.domNode)[0].innerHTML = "";
                    dialog.showErrorFromPromise(nls.appliance.savingFailed, error);
                };
            }

            // remove these 3 properties since REST API doesn't expect them.
			delete values.id;
			delete values.CiphersDisplayName;
			delete values.MissingTrustedCertificates;
			delete values.HasClientCerts;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("SecurityProfile", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(function(data) {
                // If UseClientCertificate is true, check to see if there are any certificates.
                // Set values.MissingTrustedCertificates accordingly.
                if (values.UseClientCertificate) {
                    getTrustedCertificatesForProfile(values.Name, "TrustedCertificate", function(data) {
                        var certsNotfound = !data || data.length === 0;
                        if (certsNotfound) {
                        	getTrustedCertificatesForProfile(values.Name, "ClientCertificate", function(data2) {
                                values.MissingTrustedCertificates = !data2 || data2.length === 0;
                        		onFinish(values);
                        	}, function(error) {
                                values.MissingTrustedCertificates = false;
                                onFinish(values);                        
                            });
                        } else {
                        	values.MissingTrustedCertificates = false;
                            onFinish(values);
                        }
                    }, function(error) {
                        values.MissingTrustedCertificates = false;
                        onFinish(values);                        
                    });          
                } else {
                    values.MissingTrustedCertificates = false;
                    onFinish(values);
                }                 
            }, function(error) {
                onError(error);
            });
		},
		
		_deleteEntry: function(id, name, dialog, onComplete, onError) {
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			var promise = adminEndpoint.doRequest("/configuration/SecurityProfile/"+encodeURIComponent(name), adminEndpoint.getDefaultDeleteOptions());
			promise.then(
			        function(data) {
			            _this.grid.store.remove(id);    
			            if (onComplete) onComplete();                       
			        }, function(error) {
			            query(".idxDialogActionBarLead",dialog.domNode)[0].innerHTML = "";
			            dialog.showErrorFromPromise(nls.appliance.deletingFailed, error);
			            if (onError) onError();                     
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
			
			this.editDialog = new SecProfileDialog({dialogId: editId, dialogTitle: editTitle, add: false }, editId);
			this.editDialog.startup();

			// create Add dialog
			var addTitle = dojo.string.substitute(this.addDialogTitle,{nls:nls});
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new SecProfileDialog({dialogId: addId, dialogTitle: addTitle, add: true },
					addId);
			this.addDialog.startup();
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = dojo.string.substitute(this.removeDialogTitle,{nls:nls});
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + Utils.escapeApostrophe(nls.appliance.removeDialog.content) + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_saveButton",
						label: nls.appliance.removeDialog.deleteButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.appliance.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;

			// create upload dialog for trusted certs (CA certs)
			var trustedCertsId = "trusted_certs_"+this.id+"Dialog";
			var trustedCertsTitle = Utils.escapeApostrophe(nls.appliance.securityProfiles.certsDialog.title);
			domConstruct.place("<div id='"+trustedCertsId+"'></div>", this.contentId);
			this.trustedCertsDialog = new CertificatesDialog(
					{title: trustedCertsTitle,
					 dialogId: trustedCertsId, 
					 certType: "TrustedCertificate"},
					 trustedCertsId);
			this.trustedCertsDialog.startup();
			
			// create upload dialog for client certs
			var clientCertsId = "certs_"+this.id+"Dialog";
			var clientCertsTitle = Utils.escapeApostrophe(nls.clientCertsTitle);
			domConstruct.place("<div id='"+clientCertsId+"'></div>", this.contentId);
			this.clientCertsDialog = new CertificatesDialog(
					{title: clientCertsTitle,
					 dialogId: clientCertsId, 
					 certType: "ClientCertificate"},clientCertsId);
			this.clientCertsDialog.startup();
			
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.appliance.securityProfiles.grid.name, field: "Name", dataType: "string", width: "125px",
						decorator: Utils.nameDecorator
					},
					{	id: "TLSEnabled", name: nls_sp.tlsEnabled, field: "TLSEnabled", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.TLSEnabled);
                        }                         
					},
					{	id: "MinimumProtocolMethod", name: nls.appliance.securityProfiles.grid.mprotocol, field: "MinimumProtocolMethod", dataType: "string", width: "100px" },
					{	id: "Ciphers", name: nls.appliance.securityProfiles.grid.ciphers, field: "CiphersDisplayName", dataType: "string", width: "100px" },
					{	id: "UseClientCertificate", name: nls.appliance.securityProfiles.grid.certauth, field: "UseClientCertificate", dataType: "string", width: "100px",
						decorator: lang.hitch(this, function(cellData) {
							if (cellData === Utils.booleanDecorator(true) + ": " +nls.appliance.securityProfiles.trustedCertMissing) {
								var value="<span style='color: red; vertical-align: middle;'>" + Utils.booleanDecorator(true) + "</span>" +
								"<span style='padding-left: 10px;'>" +
								"<img style='vertical-align: middle; width=16px;height=16px' src='css/images/msgWarning16.png' alt='" + 
								nls_strings.level.Warning +"' title='" + nls.appliance.securityProfiles.trustedCertMissing + "'/></span>";
								return value;
							}							
							return cellData;							
						}),
						formatter: function(data) {
							var cellData = Utils.booleanDecorator(data.UseClientCertificate);
							if (data.MissingTrustedCertificates) {
								cellData += ": " +nls.appliance.securityProfiles.trustedCertMissing;
							}	
							return cellData;
						}						
					},
					{	id: "UseClientCipher", name: nls.appliance.securityProfiles.grid.clientciphers, field: "UseClientCipher", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.UseClientCipher);
                        }                         
					},
					{	id: "UsePasswordAuthentication", name: nls.appliance.securityProfiles.grid.passwordAuth, field: "UsePasswordAuthentication", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.UsePasswordAuthentication);
                        }                         
					},
					{	id: "CertificateProfile", name: nls.appliance.securityProfiles.grid.certprofile, field: "CertificateProfile", dataType: "string", width: "125px",
						decorator: Utils.nameDecorator
					},
					{	id: "LTPAProfile", name: nls.appliance.securityProfiles.grid.ltpaprofile, field: "LTPAProfile", dataType: "string", width: "125px",
						decorator: Utils.nameDecorator
					},
					{	id: "OAuthProfile", name: nls.appliance.securityProfiles.grid.oAuthProfile, field: "OAuthProfile", dataType: "string", width: "125px",
						decorator: Utils.nameDecorator
					}
				];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
			
			this.trustedCertsButton = new MenuItem({
				label: nls.appliance.securityProfiles.grid.clientCertsButton
			});
			this.clientCertsButton = new MenuItem({
				label: nls.clientCertsButton
			});
			GridFactory.addToolbarMenuItem(this.grid, this.trustedCertsButton);
			this.trustedCertsButton.set('disabled', true);
			GridFactory.addToolbarMenuItem(this.grid, this.clientCertsButton);
			this.clientCertsButton.set('disabled', true);

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
			this.deleteButton.set('disabled',true);
			this.editButton.set('disabled',true);
			this.trustedCertsButton.set('disabled', true);
			this.clientCertsButton.set('disabled', true);
		},

		_getProfileNames: function(objectType) {
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/"+objectType, adminEndpoint.getDefaultGetOptions());
            return promise.then(
                function(data) {
                    var objects = adminEndpoint.getObjectsAsArray(data, objectType);
                    var names = null;
                    if (objects) {
                        // extract just the name and sort alphabetically
                        names = dojo.map(objects, function(item, index){
                            return item.Name;
                        });
                        names.sort();
                    } 
                    return names;                   
                },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                       return  [];
                    } else {
                        return null;
                    }
                }
            );			
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    // create array of existing userids so we don't clash
				var existingNames = this.store.query({}).map(
						function(item){ return item.Name; });
			    this.addDialog.show(existingNames,this._getProfileNames("CertificateProfile"), 
			    		this._getProfileNames("LTPAProfile"), this._getProfileNames("OAuthProfile"));
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				console.log("selected: "+this.grid.select.row.getSelected());
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				dijit.byId(this.removeDialog.dialogId + "_saveButton").set('disabled', false);
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
			
			on(this.trustedCertsButton, "click", lang.hitch(this, function() {
			    console.log("clicked trustedCerts button");
			    var rowData = this._getSelectedData();
			    this.trustedCertsDialog.show(rowData, this._refreshEntry, this);
			}));
			
			on(this.clientCertsButton, "click", lang.hitch(this, function() {
			    console.log("clicked clientCerts button");
			    var rowData = this._getSelectedData();
			    this.clientCertsDialog.show(rowData, this._refreshEntry, this);
			}));
			
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
				    var _this = this;
					var profile = this._getSelectedData();
					var name = profile.Name;
					var id = this.grid.select.row.getSelected()[0];
					_this.editDialog.save(function(data) {
						console.debug("edit is done, returned:", data);
						var bar = query(".idxDialogActionBarLead",_this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
						_this._saveSettings(data,_this.editDialog.dialog, function(data) {	
							console.debug("Saving:");
							console.dir(data);
							data.id = escape(data.Name);
							data.CiphersDisplayName = _this._getCiphersDisplayName(data);
							if (data.Name != _this.editDialog._startingValues.Name) {
								console.debug("name changed, removing old name");
								_this.store.remove(id);
								_this.store.add(data);		
							} else {
								_this.store.put(data,{overwrite: true});					
							}
							_this.editDialog.dialog.hide();
							_this.triggerListeners();
						});
					});
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {
                    var _this = this;
					this.addDialog.save(function(data) {
						console.debug("add is done, returned:", data);
						var bar = query(".idxDialogActionBarLead",_this.addDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
						_this._saveSettings(data, _this.addDialog.dialog, function(data) {
							data.id = escape(data.Name);
							data.CiphersDisplayName = _this._getCiphersDisplayName(data);							
							data.MissingTrustedCertificates = data.UseClientCertificate;
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
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var name = this._getSelectedData().Name;
                var _this = this;
				this._deleteEntry(id, name, this.removeDialog, function() {
					_this.removeDialog.hide();				
					// disable buttons if empty or no selection
					if (_this.grid.store.data.length === 0 || 
							_this.grid.select.row.getSelected().length === 0) {
						_this.deleteButton.set('disabled',true);
						_this.editButton.set('disabled',true);
						_this.trustedCertsButton.set('disabled', true);
						_this.clientCertsButton.set('disabled', true);
					}	
					_this.triggerListeners();
				}, function() {
					dijit.byId(_this.removeDialog.dialogId + "_saveButton").set('disabled', true);
				});
			}));
		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				this.deleteButton.set('disabled',false);
				this.editButton.set('disabled',false);
				this.trustedCertsButton.set('disabled', false);
				this.clientCertsButton.set('disabled', false);
			} else {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);										
				this.trustedCertsButton.set('disabled', true);
				this.clientCertsButton.set('disabled', true);
			}
		},
		
		_doEdit: function() {
			var rowData = this.store.query({id: this.grid.select.row.getSelected()[0]})[0];
			// create array of existing userids except for the current one, so we don't clash
			var existingNames = this.store.query(function(item){ return item.Name != rowData.Name; }).map(
					function(item){ return item.Name; });
			console.debug("existing names:");
			console.dir(existingNames);
			this.editDialog.show(existingNames,this._getProfileNames("CertificateProfile"), 
					this._getProfileNames("LTPAProfile"), this._getProfileNames("OAuthProfile"), rowData);
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

	return SecurityProfiles;
});
