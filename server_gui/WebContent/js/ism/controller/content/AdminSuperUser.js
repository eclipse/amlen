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
        'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Form',
	    'dijit/form/Button',
        'ism/config/Resources',
        'ism/IsmUtils',
	    'ism/widgets/Dialog',
	    'ism/widgets/TextBox',		
        'dojo/i18n!ism/nls/adminEndpointConfig',
        'dojo/i18n!ism/nls/appliance',
        'dojo/text!ism/controller/content/templates/AdminSuperUser.html',
        'idx/string'
        ], function(declare, lang, aspect, domConstruct, query, number, on, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
        		Form, Button, Resources, Utils, Dialog, TextBox, nls, nls2, template, iString) {	

    var AdminSuperUser = declare("ism.controller.content.AdminSuperUser",  [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		
		// 
		// description:
		//	
		nls: nls,
		nls2: nls2,
		templateString: template,
		adminUserID: undefined,
		
		adminUserIdDialogTitle: Utils.escapeApostrophe(nls.editAdminUserIdDialogTitle),      
		adminUserIdDialogInstruction: Utils.escapeApostrophe(nls.editAdminUserIdDialogInstruction),
		userIDLabel: Utils.escapeApostrophe(nls.adminUserIDLabel),
		
	    adminUserPasswordDialogTitle: Utils.escapeApostrophe(nls.editAdminUserPasswordDialogTitle),      
	    adminUserPasswordDialogInstruction: Utils.escapeApostrophe(nls.editAdminUserPasswordDialogInstruction),
		passwordLabel: Utils.escapeApostrophe(nls.adminUserPasswordLabel),
		confirmPasswordLabel: Utils.escapeApostrophe(nls.confirmPasswordLabel),
		passwordInvalidMessage: Utils.escapeApostrophe(nls.passwordInvalidMessage),


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
			var promise = endpoint.doRequest("/configuration/AdminUserID", options);
			promise.then(
		        function(data) {
                    var adminUserID = data.AdminUserID;
                    if (!adminUserID) {
                        Utils.showPageError(nls.getAdminUserIDError);
                        return;
                    }
		            _this.adminUserID = adminUserID;
		            _this._updateTd("AdminUserID", adminUserID);
		        },
		        function(error) {
                    Utils.showPageErrorFromPromise(nls.getAdminEndpointError, error);
		        }
			);
		},
				
		_updateTd: function(prop, value) {
            var td = this[prop];
            if (td !== undefined && value !== undefined) {
                td.innerHTML = value;
            }		    
		},

		editAdminUserID: function(event) {
		    if (event && event.preventDefault) event.preventDefault();

		    if (!this.adminUserIdDialog) {
		        this._createAdminUserIdDialog();
		    }
		    this.adminUserIdDialog.adminUserIdField.set('value', this.adminUserID);
		    this.adminUserIdDialog.show();
		},          
	        
		_createAdminUserIdDialog: function() {
			// create the admin user ID edit dialog
		    var _this = this;
            var adminUserIdDialogId = this.id+"_adminUserIdDialog";
            domConstruct.place("<div id='"+adminUserIdDialogId+"'></div>", this.id);
            var adminUserIdForm = new Form({id: this.id+'_adminUserIdForm'});
            var adminUserIdField = new TextBox({
                    label: _this.userIDLabel,
                    value: _this.adminUserID,
                    id: 'adminUserIdField',
                    required: true,
                    attachPoint: 'adminUserIdField',
                    labelAlignment: 'horizontal'});            
                        
            adminUserIdForm.containerNode.appendChild(adminUserIdField.domNode);
            
            this.adminUserIdDialog = new Dialog({
                id: adminUserIdDialogId,
                title: nls.editAdminUserIdDialogTitle,
                instruction: nls.editAdminUserIdDialogInstruction,
                content: adminUserIdForm,
                onEnter: lang.hitch(this, function() {this._saveAdminUserId(adminUserIdField.get("value"), adminUserIdForm, _this);}),
                buttons: [
                    new Button({
                        id: adminUserIdDialogId + "_save",
                        label: nls.saveButton,
                        onClick: lang.hitch(this, function() {this._saveAdminUserId(adminUserIdField.get("value"), adminUserIdForm, _this);})
                    })
                ],
                closeButtonLabel: nls.cancelButton
            }, adminUserIdDialogId);
            this.adminUserIdDialog.dialogId = adminUserIdDialogId;
            this.adminUserIdDialog.adminUserIdField = adminUserIdField;
            this.adminUserIdDialog.startup();			
		},

		_saveAdminUserId: function(adminUserID, form, context) {
		    if (!form.validate()) { return; } 
		    var adminUserIdDialog = context.adminUserIdDialog;
		    var bar = query(".idxDialogActionBarLead",this.adminUserIdDialog.domNode)[0];
		    bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;  

		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({AdminUserID: adminUserID});
            var promise = adminEndpoint.doRequest("/configuration",options);
            promise.then(
                    function(data) {
                        bar.innerHTML = "";
                        adminUserIdDialog.hide();
                        context.adminUserID = adminUserID;
                        context._updateTd("AdminUserID", adminUserID)();
                    },
                    function(error) {
                    	// Reusing the savingFailed message from the appliance nls file becuase it was never added
                    	// to the adminEndpoingConfig nls file and it's too late to add it for V2.0.
                        bar.innerHTML = nls2.appliance.savingFailed;
                        adminUserIdDialog.showErrorFromPromise(nls.saveAdminUserIDError, error);                
                    }
            );
		},
		
		editAdminUserPassword: function(event) {
            if (event && event.preventDefault) event.preventDefault();

            if (!this.adminUserPasswordDialog) {
                this._createAdminUserPasswordDialog();
            }
            this.adminUserPasswordDialog.reset();
            this.adminUserPasswordDialog.show();
        },          
            
        _createAdminUserPasswordDialog: function() {
            // create the admin user Password edit dialog
            var _this = this;
            var adminUserPasswordDialogId = this.id+"_adminUserPasswordDialog";
            domConstruct.place("<div id='"+adminUserPasswordDialogId+"'></div>", this.id);
            var adminUserPasswordForm = new Form({id: this.id+'_adminUserPasswordForm'});
            var adminUserPasswordField = new TextBox({
                type: 'password',
                label: _this.passwordLabel,
                id: 'adminUserPasswordField',
                required: true,
                inputWidth: '150',
                labelWidth: '150',                
                attachPoint: 'adminUserPasswordField',
                labelAlignment: 'horizontal'});            

            adminUserPasswordForm.containerNode.appendChild(adminUserPasswordField.domNode);

            var adminUserConfirmPasswordField = new TextBox({
                type: 'password',
                label: _this.confirmPasswordLabel,
                id: 'adminUserConfirmPasswordField',
                required: true,
                inputWidth: '150',
                labelWidth: '150',
                invalidMessage: _this.passwordInvalidMessage,
                attachPoint: 'adminUserConfirmPasswordField',
                labelAlignment: 'horizontal'});            

            adminUserConfirmPasswordField.isValid = function(focused) {
                // Valid if it matches field_AdminUserPassword
                var password = adminUserPasswordField.get('value');
                var confirmPassword = this.get('value');
                return  password == confirmPassword;
            };
            adminUserPasswordForm.containerNode.appendChild(adminUserConfirmPasswordField.domNode);

            
            this.adminUserPasswordDialog = new Dialog({
                id: adminUserPasswordDialogId,
                title: nls.editAdminUserPasswordDialogTitle,
                instruction: nls.editAdminUserPasswordDialogInstruction,
                content: adminUserPasswordForm,
                onEnter: lang.hitch(this, function() {this._saveAdminUserPassword(adminUserPasswordField.get("value"), adminUserPasswordForm, _this);}),
                buttons: [
                    new Button({
                        id: adminUserPasswordDialogId + "_save",
                        label: nls.saveButton,
                        onClick: lang.hitch(this, function() {this._saveAdminUserPassword(adminUserPasswordField.get("value"), adminUserPasswordForm, _this);})
                    })
                ],
                closeButtonLabel: nls.cancelButton
            }, adminUserPasswordDialogId);
            this.adminUserPasswordDialog.dialogId = adminUserPasswordDialogId;
            this.adminUserPasswordDialog.reset = function() {
                adminUserPasswordForm.reset();
            };
            this.adminUserPasswordDialog.startup();           
        },

        _saveAdminUserPassword: function(adminUserPassword, form, context) {
            if (!form.validate()) { return; } 
            var adminUserPasswordDialog = context.adminUserPasswordDialog;
            var bar = query(".idxDialogActionBarLead",this.adminUserPasswordDialog.domNode)[0];
            bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;  

            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({AdminUserPassword: adminUserPassword});
            var promise = adminEndpoint.doRequest("/configuration",options);
            promise.then(
                    function(data) {
                        bar.innerHTML = "";
                        adminUserPasswordDialog.hide();
                    },
                    function(error) {
                    	// Reusing the savingFailed message from the appliance nls file becuase it was never added
                    	// to the adminEndpoingConfig nls file and it's too late to add it for V2.0.
                        bar.innerHTML = nls2.appliance.savingFailed;
                        adminUserPasswordDialog.showErrorFromPromise(nls.saveAdminUserPasswordError, error);                
                    }
            );
        }
	});
	
	return AdminSuperUser;
});
