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
        'dojo/dom-style',
        'dojo/number',
		'dojo/query',
        'dojo/store/Memory',
        'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/widgets/Dialog',
		'ism/widgets/AdminEndpoint',
        'dojo/i18n!ism/nls/firstserver',
        'dojo/i18n!ism/nls/serverControl',
        'dojo/text!ism/controller/content/templates/FirstServer.html',
        'dojo/keys'],
        function(declare, lang, domStyle, number, query, Memory, manager, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, Button, Resources, Utils, 
        		Dialog, AdminEndpoint, nlsFirstServer, nls, addTemplate, keys){
    
    return declare("ism.controller.content.FirstServer", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        // summary:
        //      Add a new server to manage from this Web UI
        
        nls: nls,
        nlsFirstServer: nlsFirstServer,
        templateString: addTemplate,
        store: undefined,
        // dialog counter to keep track of which dialog is opened for callback purposes
        dialogCounter: 0,
        firstServerButtons: "",
        serverName: null,    /* Server name from server.
        					    If no test connection is done for first server, the server name value in the
        					    Web UI will appear as admin IP:port until a connection can be established and
        					    server name can be queried from the server. */
        serverNameUid: null, /* We cannot safely assume that the last test connection action goes with the current
        					    admin IP/port settings. We'll use this to assure that we only use a server name from
        					    a test connection action if it corresponds with the uid (admin IP/port) we are 
        					    saving. */
        lastTestFailed: false,

        
        closeButtonLabel: Utils.escapeApostrophe(nls.closeButton),
        saveButtonLabel: Utils.escapeApostrophe(nlsFirstServer.saveButton),
        testButtonLabel: Utils.escapeApostrophe(nls.testButton),
        serverNameLabel: Utils.escapeApostrophe(nls.serverName),
        adminAddressLabel: Utils.escapeApostrophe(nls.adminAddress),
        adminAddressTooltip: Utils.escapeApostrophe(nls.adminAddressTooltip),
        adminPortLabel: Utils.escapeApostrophe(nls.adminPort),
        adminPortTooltip: Utils.escapeApostrophe(nls.adminPortTooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.description),
        sendWebUICredentialsLabel: Utils.escapeApostrophe(nls.sendWebUICredentials),
        sendWebUICredentialsTooltip: Utils.escapeApostrophe(nls.sendWebUICredentialsTooltip),
                
        constructor: function(args) {
            dojo.safeMixin(this, args);
            this.inherited(arguments);
            this.store = new Memory({data:ism.user.getServerList(), idProperty: "uid"});

        },

        postCreate: function() {        
            this.inherited(arguments);
            
            // For now, always hide serverName field in this dialog.
            // TODO: Clean up code that manipulates value of serverName field
            //       if we decide to omit it permanently.
            domStyle.set(this.field_serverName.domNode, "display", "none");
            
            this.field_serverAdminPort.isValid = function(focused) {
                var value = this.get("value");
                if (value === null || value === "") {
                    value = "9089";
                }
                var intValue = number.parse(value, {places: 0});
                if (isNaN(intValue) || intValue < 1 || intValue > 65535) {
                    this.invalidMessage = nls.portInvalidMessage;
                    return false;
                }
                return true;
            };
        },

        save: function() {
			var server = this.getServer();
			if (this.serverName && this.serverNameUid === server.uid) {
				server.name = this.serverName;
			}
			if (!server.name || server.name === "") {
				server.name = server.uid;
			}
            var matches = this.store.query({uid: server.uid});
            if (matches.length === 0) {
            	var serverlist = this.store.query();
            	// If uid does not match, remove the default value before adding the first server.
            	this.store.remove(serverlist[0].uid);
            	this.store.add(server);
            } else {
            	// If uid matches default, force an update with the values set here.
            	this.store.put(server, {overwrite: true});
            }
			ism.user.setServerList(this.store.data, true);
            this.promptForClose();
        },
        
        testConnection: function() {
            var _this = this;
            var server = _this.getServer();
        	if (this.lastTestFailed) {
    			this.firstServerButtons.innerHTML = "";
    			this.clearErrorMessage();
    			this.lastTestFailed = false;
        	}

            _this.serverNameUid = server.uid;
            _this.firstServerButtons.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.testingProgress;
            var testButton = dijit.byId(_this.id+"_testButton");
            testButton.set('disabled', true);
            var dialogCounter = _this.dialogCounter;
            
            // check to see if we can get the status of the server
            var adminEndpoint = new AdminEndpoint({server: server});
            Utils.getStatus(adminEndpoint).then(
                function(data) {
                    var status = data && data.Server ? data.Server : {};
                    var state = status.State === undefined ? -1 : status.State;
                    _this.serverName = status.Name === undefined ? _this.serverNameUid : status.Name;
                    _this.field_serverName.set(_this.serverName);
//                        _this.updateServerName(_this.serverName);
                    if (dialogCounter === _this.dialogCounter) {
                    	_this.firstServerButtons.innerHTML = "";
                        if (!Utils.isErrorMode(state)) {
                            _this.firstServerButtons.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>" + nls.testingSuccess;
                        }
                        testButton.set('disabled',false);
                    }
                },
                function(error) {
                	_this.lastTestFailed = true;
                    if (dialogCounter === _this.dialogCounter) {
                    	_this.serverName = null;
                    	_this.serverNameUid = null;
                    	_this.firstServerButtons.innerHTML = "";
                    	_this.firstServerButtons.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>" + nls.testingFailed;
                        Utils.showPageErrorFromPromise(nls.testingFailed, error, adminEndpoint, /*addServerError */true);
                        testButton.set('disabled',false);
                    }
                }
            );
        },
                
        getServer: function() {
            var name = this.field_serverName.get('value');
            var adminAddress = this.field_serverAdminAddress.get('value') ? this.field_serverAdminAddress.get('value') : "127.0.0.1";
            var adminPort = this.field_serverAdminPort.get('value') ? this.field_serverAdminPort.get('value') : "9089";
            var description = this.field_description.get('value');
            var sendWebUICredentials = this.field_sendWebUICredentials.get('checked');
            
            var uid = adminAddress.indexOf(":") < 0 ? adminAddress : "[" + adminAddress +"]";
            uid += ":" + adminPort;
            var displayName;
            if (name && name !== "") {
                if (name.indexOf(uid) < 0) {
                	displayName = name + "(" + uid + ")";
                } else {
                	displayName = name;
                }
            } else {
            	name = uid;
            	displayName = uid;
            }
            return {uid: uid, name: name, adminServerAddress: adminAddress, adminServerPort: adminPort, description: description,
                    sendWebUICredentials: sendWebUICredentials, displayName: displayName};
        },
        
		promptForClose: function() {
			console.debug(this.declaredClass + ".promptForClose()");
			
			var closeFirstServerDialogId = "closeFirstServerDialog";
			var dialog = manager.byId(closeFirstServerDialogId);
			if (!dialog) {
				dialog = new Dialog({
					id: closeFirstServerDialogId,
					title: nlsFirstServer.savedTitle,
					content: "<div>" + nlsFirstServer.serverSaved + "</div>",
					closeButtonLabel: this.closeButtonLabel,
					onCancel: function() {
						window.location = Resources.pages.home.uri;
					}
				},closeFirstServerDialogId);
			}
			dialog.show();
		},
		
		clearErrorMessage: function() {
			query(".errorMessage","mainContent").forEach(function(node) {
                var message = dijit.byId(node.id);
                message.destroy();
			});
		},
		
		clearTestStatus: function() {
			this.firstServerButtons.innerHTML = "";
		}
    });
});
