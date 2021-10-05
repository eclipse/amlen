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
		'dojo/aspect',
        'dojo/dom-construct',
		'dojo/query',
        'dojo/number',
        'dojo/on',
        'dojo/store/Memory',
        'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
	    'dijit/form/Button',
        'ism/config/Resources',
        'ism/IsmUtils',
        'ism/widgets/IsmQueryEngine',
		'ism/controller/content/AdminEndpointConfigDialog',
        'dojo/i18n!ism/nls/adminEndpointConfig',
        'dojo/text!ism/controller/content/templates/AdminEndpointConfig.html'
        ], function(declare, lang, aspect, domConstruct, query, number, on, Memory, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
        		Button, Resources, Utils, IsmQueryEngine, AdminEndpointConfigDialog, nls, template) {	

    var AdminEndpointConfig = declare("ism.controller.content.AdminEndpointConfig",  [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		
		// 
		// description:
		//	
		nls: nls,
		templateString: template,
        adminEndpoint: {},
        queryEngine: IsmQueryEngine,

		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);	
		},

		postCreate: function(args) {			
			this.inherited(arguments);
			
			// initialize by getting the configuration
			this._initConfig();
		
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.appliance.nav.adminEndpoint.topic, lang.hitch(this, function(message){
				this._initConfig();
			}));
			
		},
		
		_initConfig: function() {
		    var _this = this;            
            var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.getDefaultGetOptions();
			var promise = endpoint.doRequest("/configuration/AdminEndpoint", options);
			promise.then(
		        function(data) {
                    var adminEndpoint = data.AdminEndpoint;
                    if (!adminEndpoint) {
                        Utils.showPageError(nls.getAdminEndpointError);
                        return;
                    }
		            if (!adminEndpoint.SecurityProfile) {
		                adminEndpoint.SecurityProfile = "";
		            }
		            if (!adminEndpoint.ConfigurationPolicies) {
		                adminEndpoint.ConfigurationPolicies = "";
		            }
		            _this.adminEndpoint = adminEndpoint;
		            _this._updatePage();
		        },
		        function(error) {
                    Utils.showPageErrorFromPromise(nls.getAdminEndpointError, error);
		        }
			);
		},
		
		
		editConfig: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();
		    
		    if (!this.editDialog) {
		        this._initDialogs();
		        this._initEvents();
		    }
		    var configPolicyList = dijit.byId("configPolicyList");
		    var configPolicyStore = configPolicyList ? configPolicyList.store : new Memory({data:[], idProperty: "id"});
		    this.editDialog.show(this.adminEndpoint, configPolicyStore);
		},	        
		

		_updatePage: function() {
		    var value, prop;
            for (prop in this.adminEndpoint) {
                value = this.adminEndpoint[prop];
                this._updateTd(prop, value);                
            }
            // decide which tagline to show...
            var tagline = nls.configurationTagline;
            if (!this._haveValues([this.adminEndpoint.SecurityProfile, this.adminEndpoint.ConfigurationPolicies])) {
                // unsecured tagline
                tagline = "<img src='css/images/msgWarning16.png' width='12' height='12' alt=''/>" + nls.configurationTagline_unsecured;
            } 
            this.configurationTagline.innerHTML = tagline;
		},
		
		_updateTd: function(prop, value) {
            var td = this[prop];
            if (td !== undefined && value !== undefined) {
                if (prop == "ConfigurationPolicies") {
                    if (value === "") {
                        value = nls.none;
                    } else {
                        value = value.replace(/,/g, ", ");
                    }
                }
                else  if (prop === "Interface") {
                    if (value.toLowerCase() === "all") {
                    	value = nls.all;
                    }
                }
                else if ( value === "" && prop == "SecurityProfile") {
                    value = nls.none;
                }
                // TODO: It would be nice to make the SecurityProfile name a link which opens a view dialog of the profile
                td.innerHTML = value;
            }		    
		},
		
		_haveValues: function(propArray) {
		    if (!propArray) return false;
		    for (var i = 0, len = propArray.length; i < len; i++) {
		        if (!this._hasValue(propArray[i])) {
		            return false;
		        }
		    }
		    return true;
		},
		
		_hasValue: function(prop) {
		    return prop !== undefined && prop !== "" && prop !== -1;
		},
		
		_initDialogs: function() {
			
			var editId = "edit"+this.id+"Dialog";
			var editDialogNode = editId + "Node";
			domConstruct.place("<div id='"+editDialogNode+"'></div>", this.id);
			
			this.editDialog = new AdminEndpointConfigDialog({dialogId: editId}, editDialogNode);
			this.editDialog.startup();
			
		},
		
		_initEvents: function() {		
		    var _this = this;
			dojo.subscribe(_this.editDialog.dialogId + "_saveButton", function(message) {
				if (_this.editDialog.form.validate()) {
				    _this.editDialog.save(function(values) {
				        _this.editDialog.hide();
				        _this._initConfig();
				    });
				}
			});
		}
	});
	
	return AdminEndpointConfig;
});
