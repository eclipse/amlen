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
        'dojo/dom-construct',
        'dojo/number',
    	'dojo/query',
        'dojo/store/Memory',
        'dojo/data/ItemFileWriteStore',
        'dojox/html/entities',    	
        'dijit/_Widget',
        'dijit/_TemplatedMixin',
        'dijit/_WidgetsInTemplateMixin',	
        'dijit/form/Button',
        'idx/form/CheckBox',
        'ism/widgets/TextBox',
        'ism/config/Resources',
        'ism/IsmUtils',
        'ism/widgets/IsmQueryEngine',
        'ism/controller/content/ReferencedObjectGrid',
        'ism/controller/content/OrderedReferencedObjectGrid',
        'dojo/i18n!ism/nls/adminEndpointConfig',
        'dojo/text!ism/controller/content/templates/AdminEndpointConfigDialog.html',
        'dojo/text!ism/controller/content/templates/AddConfigPoliciesDialog.html'
        ], function(declare, lang, domConstruct, number, query, Memory, ItemFileWriteStore, entities, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, Button, CheckBox, TextBox, Resources, Utils, IsmQueryEngine, ReferencedObjectGrid, 
        		OrderedReferencedObjectGrid, nls, template, addConfigPoliciesDialog) {

    var AddConfigPoliciesDialog = declare("ism.controller.content.AddConfigPoliciesDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        // summary:
        //      Dialog tied the the Add button in the EndpointDialog widget
        
        nls: nls,
        templateString: addConfigPoliciesDialog,
        dialogId: undefined,
        additionalColumns: undefined,
        selectedPolicies: undefined,
        onSave: undefined,

        constructor: function(args) {
            dojo.safeMixin(this, args);
            this.inherited(arguments);
        },

        postCreate: function() {        
            if (this.additionalColumns) {
                this.policies.set("additionalColumns", this.additionalColumns);
            }
            dojo.subscribe(this.dialogId + "_saveButton", lang.hitch(this, function(message) {
                this._updatePolicies();
                this.dialog.hide();
            }));
            


        },
        
        show: function(policies, policyStore, onSave) {         
            // clear fields and any validation errors
            dijit.byId(this.dialogId +"_DialogForm").reset();
            query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
            this.onSave = onSave ? onSave : undefined;
            this.dialog.show(); 
                        
            // must be done after the dialog has been shown
            this.policies.populateGrid(this.dialog,policyStore, policies, true);
            this.dialog.resize();
        },
        
        /**
         * returns an array of the names of the selected policies
         */
        _getSelectedPoliciesAttr: function() {
            return this.policies.getValue();
        },
        
        _updatePolicies: function() {
            if (this.onSave) {
                this.onSave(this.policies.getValue());
            }
        }

    });

    
    var AdminEndpointConfigDialog = declare("ism.controller.content.AdminEndpointConfigDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
		_startingValues: undefined,
		
		addConfigPoliciesDialog: undefined,
		additionalConfigPolicyColumns: 
		    [{  id: "ActionListLocalized", name: nls.authorityLabel, field: "ActionListLocalized", dataType: "string", width: "250px"}],

		
		// template strings
		dialogTitle: Utils.escapeApostrophe(nls.editDialogTitle),		
		dialogInstruction: Utils.escapeApostrophe(nls.editDialogInstruction),

        interfaceLabel: Utils.escapeApostrophe(nls.interfaceLabel),
        interfaceTooltip: Utils.escapeApostrophe(nls.interfaceTooltip),
        portLabel: Utils.escapeApostrophe(nls.portLabel),
        portTooltip: Utils.escapeApostrophe(nls.portTooltip),
        securityProfileLabel: Utils.escapeApostrophe(nls.securityProfileLabel),
        securityProfileTooltip: Utils.escapeApostrophe(nls.securityProfileTooltip),
        configurationPoliciesLabel: Utils.escapeApostrophe(nls.configurationPoliciesLabel),
        configurationPoliciesTooltip: Utils.escapeApostrophe(nls.configurationPoliciesTooltip),
        configurationPolicyLabel: Utils.escapeApostrophe(nls.configurationPolicyLabel),
        all: Utils.escapePsuedoTranslation(nls.all),
                        
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		startup: function(args) {
			this.inherited(arguments);
		},
		
		postCreate: function() {		
		    console.debug(this.declaredClass + ".postCreate()");			

            this.field_Port.isValid = function(focused) {
                return Utils.portValidator(this.get("value"), true);
            };
            
            // create Add messaging policy dialog
            var addConfigPoliciesId = "addConfigPolicies_"+this.id;
            var node = addConfigPoliciesId + "Node";
            domConstruct.place("<div id='"+node+"'></div>", this.id);
            this.addConfigPoliciesDialog = new AddConfigPoliciesDialog({dialogId: addConfigPoliciesId,
                dialogTitle: Utils.escapeApostrophe(nls.addConfigPolicyDialogTitle), 
                dialogInstruction: Utils.escapeApostrophe(nls.addConfigPoliciesDialogInstruction),
                policyNameLabel: Utils.escapeApostrophe(nls.configurationPolicyLabel)
            }, node);
            this.addConfigPoliciesDialog.startup();          
            	
            this.field_ConfigurationPolicies.set("addDialog", this.addConfigPoliciesDialog);
            this.field_ConfigurationPolicies.set("additionalColumns", this.additionalConfigPolicyColumns);

		},
		
		show: function(/*AdminEndpoint*/ adminEndpoint, configPolicyStore) {
		    query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
		    
		    this._setupFields(adminEndpoint);
            this._fillValues(adminEndpoint); 
            
            var configPolicies = adminEndpoint ? adminEndpoint.ConfigurationPolicies : "";
            
            this.dialog.clearErrorMessage();
            this.dialog.show();            
            this.field_Interface.focus();
            this.field_ConfigurationPolicies.populateGrid(this.dialog, configPolicyStore, configPolicies);
            this.dialog.resize();
		},
		
		hide: function() {
		    this.dialog.hide();
            var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
		    bar.innerHTML = "";
		},
		
		getValues: function() {
            var values = {};
            // start with our fields, _startingValues doesn't always have non-required values in it!
            for (var prop in this) {
                if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_" && !this[prop].get('disabled')) {
                    var actualProp = prop.substring(6);
                    values[actualProp] = this._getFieldValue(actualProp);
                }
            }
            return values;
		},
		
		save: function(onFinish, onError) {
            var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
            bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;

            var dialog = this.dialog;
            if (!onFinish) { onFinish = function() { 
                dialog.hide();
                bar.innerHTML = "";
                }; 
            }
            if (!onError) { onError = function(error) { 
                    bar.innerHTML = "";
                    dialog.showErrorFromPromise(nls.saveAdminEndpointError, error);
                };
            }

            var values = this.getValues();
            var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("AdminEndpoint", values);
            var promise = adminEndpoint.doRequest("/configuration",options);
            promise.then(
                    function(data) {
                    	var olduid = String(ism.user.server);
                    	var newserver = ism.user.getNewServer(olduid, values["Interface"], String(values["Port"]));
                    	if (newserver) {
                    		dojo.publish(Resources.contextChange.serverUIDTopic, {olduid: olduid, newsrv: newserver});
                    		_this.dialog.onHide = function () {
                        		window.location = _this._buildUri(newserver.uid,newserver.name);
                    		};
                    	}
                        onFinish(values);
                    },
                    onError
            );
		},

		_buildUri: function(uid, name) {
		    var uri = Resources.pages.appliance.uri;
		    var queryParams = [];
		    this._adjustQueryParam("nav", "adminEndpoint", queryParams);
		    this._adjustQueryParam("server", uid, queryParams);
            this._adjustQueryParam("serverName", name, queryParams);
            var sep = "?";
            for (var param in queryParams) {
                uri += sep + param + "=" + queryParams[param];
                sep = "&";
            }
            return uri;
		},

		_adjustQueryParam: function(name, value, queryParams) {
		    if (!name || !queryParams) {
		        return;
		    }
		    if (!value) {
		        if (queryParams[name]) {
		            delete queryParams[name];
		        }
		    } else {
		        queryParams[name] = encodeURIComponent(value);                
		    }
		},
		
        // do any adjustment of form field values necessary before showing the form.  in this
        // case, we populate the SecurityProfiles via REST API call
        _setupFields: function(values) {
            // clear fields and any validation errors
            this.form.reset();
            
            // Security Profiles
            var dialog = this.dialog;
            var _this = this;
            var addProfiles = lang.hitch(this, function() {
                var endpoint = ism.user.getAdminEndpoint();
                var options = endpoint.getDefaultGetOptions();
                var promise = endpoint.doRequest("/configuration/SecurityProfile", options);
                promise.then(
                        function(data) {
                            _this._populateSecurityProfiles(data, values);
                        },
                        function(error) {                            
                            if (Utils.noObjectsFound(error)) {
                                _this._populateSecurityProfiles({"SecurityProfile" : {}}, values);
                            } else {
                                dialog.showErrorFromPromise(nls.getSecurityProfilesError, error);
                            }
                        }
                ); 
            });

            this.field_SecurityProfile.store.fetch({
                onComplete: lang.hitch(this, function(items, request) {
                    console.debug("deleting items");
                    var len = items ? items.length : 0;
                    for (var i=0; i < len; i++) {
                        this.field_SecurityProfile.store.deleteItem(items[i]);
                    }
                    this.field_SecurityProfile.store.save({ onComplete: addProfiles() });
                })
            });
                        
        },
        
        _populateSecurityProfiles: function(data, values) {
            this.field_SecurityProfile.store.newItem({ name: "None", label: nls.none });
            var bChanged = false;
            var endpoint = ism.user.getAdminEndpoint();
            var profiles = endpoint.getObjectsAsArray(data, "SecurityProfile");
            var len = profiles ? profiles.length : 0;
            for (var i=0; i < len; i++) {
                var profile = profiles[i];
                if (profile.Name) {
                    this.field_SecurityProfile.store.newItem({ name: profile.Name, label: dojox.html.entities.encode(profile.Name) });
                    bChanged = true;
                }
            }
            if (bChanged) {
                this.field_SecurityProfile.store.save();
            }
            if (values && values.SecurityProfile) {
                this._setFieldValue("SecurityProfile", values.SecurityProfile);
            }

        },

		
	    // populate the dialog with the passed-in values
        _fillValues: function(values) {
            this._startingValues = values;
            for (var prop in values) {
                if (prop === "ConfigurationPolicies") {
                    continue; // handled in populateGrid
                }
                this._setFieldValue(prop, values[prop]);
            }
        },
		
        // define any special rules for getting a field value (e.g. for checkboxes)
        _getFieldValue: function(prop) {  
            var value;
            var fieldName = "field_"+prop;
            var field = this[fieldName] ? this[fieldName] : this.advancedSection[fieldName];
            switch (prop) {
            case "SecurityProfile":
                value = this["field_"+prop].get("value");
                // we want to map a "None" selection to an empty string
                if (value == "None") {
                    value = "";
                }
                break;
            case "Interface":
            	if (this["field_"+prop].get) {
                    value = this["field_"+prop].get("value");
            	} else {
            		value = this["field_"+prop].value;
            	}
                if (value === nls.all) {
                    value = "All";
                }
                else if (value.match(/[A-Fa-f]/) || value.match(/[:]/)) {
                    // wrap with [ ]
                    value = "[" + value + "]";
                }
                break;
            case "ConfigurationPolicies":
                value = this["field_"+prop].getValueAsString();
                break;                
            case "Port":
            	value = number.parse(field.get("value"), {places: 0});
            	break;
            default:
                value = field.get("value");
                break;
            }
            return value;
        },
        // set a field value according to special rules (prefixes, checkbox on/off, etc.)
        _setFieldValue: function(prop, value) {
            var field = this["field_"+prop];
            if (field == undefined) {
                return;
            }
            switch (prop) {
            case "ConfigurationPolicies":
                if (value) {
                    this["field_"+prop].set("value", value.split(","));
                }
                break;
            case "Interface":
                var isAll = false;
                if (value && value.toLowerCase() === "all") {
                    isAll = true;
                    value = nls.all;
                }
                if (!isAll && (value.match(/[A-Fa-f]/) || value.match(/[:]/))) {
                    // remove the [, ] characters that wrap this address
                    value = value.substr(1, value.length - 2);
                }
                this["field_Interface"].set("value", value);
                break;
            default:
                value = field.set("value", value);
            break;
            }
        }
		
	});
	
	return AdminEndpointConfigDialog;
});
