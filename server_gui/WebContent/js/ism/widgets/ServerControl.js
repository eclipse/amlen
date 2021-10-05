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
        'dojo/number',
        'dojo/query',
        'dojo/dom-construct',
        'dojo/dom',
		'dojo/dom-style',
        'dojo/on',
        'dojo/data/ItemFileWriteStore',
        'dojo/store/Memory',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/PopupMenuBarItem',
		'dijit/form/Button',
		'dijit/Tooltip',
		'idx/widget/MenuDialog',
		'idx/widget/Menu',
		'idx/form/FilteringSelect',
		'idx/form/CheckBox',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/widgets/AdminEndpoint',
        'dojo/i18n!ism/nls/serverControl',
        'dojo/text!ism/widgets/templates/ServerControl.html',
        'dojo/text!ism/widgets/templates/ServerListDialog.html',
        'dojo/text!ism/widgets/templates/AddServerDialog.html',
        'dojo/keys'],
        function(declare, lang, aspect, number, query, domConstruct, dom, domStyle, on, ItemFileWriteStore, Memory, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, PopupMenuBarItem, Button, Tooltip, MenuDialog, Menu, FilteringSelect, Checkbox, Resources, Utils, 
        		GridFactory, Dialog, AdminEndpoint, nls, template, listTemplate, addTemplate, keys){
	
	var ListServerDialog = declare("ism.controller.content.ServerListDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: listTemplate,
		dialogId: undefined,
		store: undefined,
		addServerDialog: undefined,
		editServerDialog: undefined,
		removeDialog: undefined,
		contentId: undefined,
		serverControl: undefined,
//		switching: false,
		
		dialogTitle: Utils.escapeApostrophe(nls.addRemoveDialogTitle),
		editDialogTitle: Utils.escapeApostrophe(nls.editDialogTitle),
        dialogInstruction: Utils.escapeApostrophe(nls.addRemoveDialogInstruction),
        closeButtonLabel: Utils.escapeApostrophe(nls.closeButton),
        cancelButtonLabel: Utils.escapeApostrophe(nls.cancelButton),
        deletingProgress: Utils.escapeApostrophe(nls.deletingProgress),
        serverNameLabel: Utils.escapeApostrophe(nls.serverName),
        adminAddressLabel: Utils.escapeApostrophe(nls.adminAddress),
        adminPortLabel: Utils.escapeApostrophe(nls.adminPort),
        descriptionLabel: Utils.escapeApostrophe(nls.description),
        
        removeServerDialogTitle: Utils.escapeApostrophe(nls.removeServerDialogTitle),
        removeConfirmationTitle: Utils.escapeApostrophe(nls.removeConfirmationTitle),
        removeConfirmButton: Utils.escapeApostrophe(nls.YesButton),
                
        //clusterLabel: Utils.escapeApostrophe(nls.cluster),
				
        constructor: function(args) {
            dojo.safeMixin(this, args);
            this.inherited(arguments);
        },

        postCreate: function() {        
            this.inherited(arguments);
            this.contentId = this.id + "Content";
            
            this.serversGridDiv.innerHTML = "<div id='"+this.contentId+"'></div>";
            
            this._initGrid();
            this._initDialogs();
            this._initEvents();
        },
        
        _initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "name", name: this.serverNameLabel, field: "name", dataType: "string", width: "120px",
						decorator: Utils.nameDecorator
					},
					/*{	id: "cluster", name: this.clusterLabel, field: "cluster", dataType: "string", width: "80px",
						decorator: Utils.nameDecorator
					},*/
					{	id: "adminServerAddress", name: this.adminAddressLabel, field: "adminServerAddress", dataType: "string", width: "80px",
						decorator: Utils.nameDecorator
					},
					{	id: "adminServerPort", name: this.adminPortLabel, field: "adminServerPort", dataType: "string", width: "80px",
						decorator: Utils.nameDecorator
					},
                    {   id: "sendWebUICredentials", name: nls.sendWebUICredentials, field: "sendWebUICredentials", dataType: "string", width: "80px",
                        decorator: Utils.booleanDecorator
                    },					
					{	id: "description", name: this.descriptionLabel, field: "description", dataType: "string", width: "120px",
						decorator: Utils.nameDecorator
					}];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true, extendedSelection: true, multiple: true});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
		},
		
        show: function(serverStore) {
            query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
            
            if (!this.store) {
            	this.store = serverStore;
            	this.grid.setStore(this.store);
            }

            this.dialog.show();
            this._refreshGrid();
        },

        _initDialogs: function() {
        	var addServerId = "addServer"+this.id+"Dialog";
            domConstruct.place("<div id='"+addServerId+"'></div>", this.contentId);
            this.addServerDialog = new AddServerDialog({dialogId: addServerId}, addServerId);
            this.addServerDialog.startup();
            dojo.subscribe(this.addServerDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
            	this.addServerDialog.save(lang.hitch(this, this._addServer));
            }));
            
            // create Edit Server dialog
		    var editServerId = "editServer"+this.id+"Dialog";
            domConstruct.place("<div id='"+editServerId+"'></div>", this.contentId);
            this.editServerDialog = new AddServerDialog({dialogId: editServerId, dialogTitle: this.editDialogTitle}, editServerId);
            this.editServerDialog.startup();
            dojo.subscribe(this.editServerDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
            	this.editServerDialog.save(lang.hitch(this, this._replaceServer));
            }));
        	
	        // create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = this.removeServerDialogTitle;
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + this.removeConfirmationTitle + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_saveButton",
						label: this.removeConfirmButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: this.cancelButtonLabel
			}, dialogId);
			this.removeDialog.dialogId = dialogId;		
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, this._removeServer));
		},
		
		_initEvents: function() {
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			aspect.after(this.grid, "onRowHeaderHeaderClick", lang.hitch(this, function() {
				console.debug("after onRowHeaderHeaderClick");
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    this.addServerDialog.show(null, this.store);
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
		    
		    aspect.after(this.dialog, "hide", lang.hitch(this, function(event) {
		    	this.serverControl._updateServers();
		    }));
		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length === 0) {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);										
			} else if (this.grid.select.row.getSelected().length === 1) {
				// Do not enable delete for the server that is currently being managed.
				if (this.grid.select.row.getSelected()[0] !== ism.user.server) {
					this.deleteButton.set('disabled',false);
				}
				this.editButton.set('disabled',false);	
			} else {
				this.deleteButton.set('disabled',false);
				this.editButton.set('disabled',true);
			}
		},
		
		_doEdit: function() {
			var id = this.grid.select.row.getSelected()[0];
			var name = unescape(id);
			
			var rowData = this.store.query({uid: name});

		    this.editServerDialog.show(rowData[0], this.store);
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh().then(lang.hitch(this, function() {
				var selected = this.grid.select.row.getSelected();
				for (var id in selected) {
					this.grid.select.row.clear(id);
				}
				this.grid.resize();
				this.dialog.resize();
			}));
			
			if (this.deleteButton) {
				this.deleteButton.set('disabled', true);
				this.editButton.set('disabled', true);
			}
		},
		
		_removeServer: function(message) {
			var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + this.deletingProgress;
			
			var selected = this.grid.select.row.getSelected();
			for (var id in selected) {
				var name = unescape(selected[id]);
				// If all rows were selected, disregard the selection of the currently managed server.
				if (name !== ism.user.server) {
					var matches = this.store.query({uid: name});
					if (matches && matches.length > 0) {
						this.store.remove(name);
					}
				}
			}
			
			var servers = this.store.query({});
			ism.user.setServerList(servers);
			this._refreshGrid();
			bar.innerHTML = "";
			this.removeDialog.hide();
			
			if (this.grid.store.data.length === 0 || 
					this.grid.select.row.getSelected().length === 0) {
				this.deleteButton.set('disabled',true);
			}
        },
		
		_addServer: function(server) {
			this.store.add(server);
			ism.user.setServerList(this.store.data);
			this._refreshGrid();
		},
		
		_replaceServer: function(newserver) {
			this.store.remove(newserver.uid);
			this.store.add(newserver);
			ism.user.setServerList(this.store.data);
			this.setServerName(newserver.name);
			this._updateServers();
			this._updateRecentServers();
		}
	});
	
    var AddServerDialog = declare("ism.controller.content.AddServerDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        // summary:
        //      Add a new server to manage from this Web UI
        
        nls: nls,
        add: undefined,
        templateString: addTemplate,
        dialogId: undefined,
        store: undefined,
        _startingValues: {},
        // dialog counter to keep track of which dialog is opened for callback purposes
        dialogCounter: 0,
        
        dialogTitle: Utils.escapeApostrophe(nls.addServerDialogTitle),
        dialogInstruction: Utils.escapeApostrophe(nls.addServerDialogInstruction),
        cancelButtonLabel: Utils.escapeApostrophe(nls.cancelButton),
        saveButtonLabel: Utils.escapeApostrophe(nls.saveButton),
        testButtonLabel: Utils.escapeApostrophe(nls.testButton),
        serverNameLabel: Utils.escapeApostrophe(nls.serverName),
        serverNameTooltip: Utils.escapeApostrophe(nls.serverNameTooltip),
        adminAddressLabel: Utils.escapeApostrophe(nls.adminAddress),
        adminAddressTooltip: Utils.escapeApostrophe(nls.adminAddressTooltip),
        adminPortLabel: Utils.escapeApostrophe(nls.adminPort),
        adminPortTooltip: Utils.escapeApostrophe(nls.adminPortTooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.description),
        sendWebUICredentialsTooltip: Utils.escapeApostrophe(nls.sendWebUICredentialsTooltip),
                
        constructor: function(args) {
            dojo.safeMixin(this, args);
            this.inherited(arguments);
        },

        postCreate: function() {        
            this.inherited(arguments);   
            
            this.field_serverAdminPort.isValid = function(focused) {
                var value = this.get("value");
                var intValue = number.parse(value, {places: 0});
                if (isNaN(intValue) || intValue < 1 || intValue > 65535) {
                    this.invalidMessage = nls.portInvalidMessage;
                    return false;
                }
                return true;
            };
            
            dojo.subscribe(this.dialogId + "_testButton", lang.hitch(this, function(message) {
                this.testConnection();
            }));

        },
        
        show: function(server, serverStore, addServerName) {
            // clear fields and any validation errors
            this.form.reset();
            query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
            
            if (server) {
            	// fill values
            	this.add = false;
            	this._startingValues = server;
            	this.setServer(server);
            	this.field_serverAdminAddress.set("disabled", true);
            	this.field_serverAdminAddress.set("required", false);

            	this.field_serverAdminPort.set("disabled", true);
            	this.field_serverAdminPort.set("required", false);

            } else {
            	this.add = true;
            	domStyle.set(this.field_serverName.domNode, "display", "none");
            }
                       
            this.store = serverStore;
            if (!this.store) {
                this.store = new Memory({data:ism.user.getServerList(), idProperty: "uid"});               
            }

            this.dialog.clearErrorMessage();
            this.dialogCounter++;
            this.dialog.show(); 
            if (this.add) {
                this.field_serverAdminAddress.focus();
            } else {
                this.field_description.focus();

            }
        },

        save: function(onSuccess) {
            if (this.form.validate()) {
                // check for duplicates against new uid
                var server = this.getServer();
                var oldName = null;

                // if matches not empty, new uid exists in store.
                var matches = this.store.query({uid: server.uid});
                
                if (this.add) {
                	// Adding a new entry
                	if (matches && matches.length > 0) {
                        // we already have this server in the store, don't allow it to be added
                        this.dialog.showErrorMessage(nls.duplicateServerTitle, nls.duplicateServerMessage);
                        return;
                    }
                } //else {
                	// Editing an existing entry
//	                if (server.uid != this._startingValues.uid) {
//	                	// The uid has changed.
//	                	if (matches && matches.length > 0) {
//	                		// The new uid already exists in the store.
//	                		this.dialog.showErrorMessage(nls.duplicateServerTitle, nls.duplicateServerMessage);
//	                		return;
//	                	}
//	                	oldName = this._startingValues.uid;
//	                }
//                }

                this.dialog.hide();
                if (onSuccess) {
                	if (!server.name || server.name === "") {
                		server.name = server.uid;
                	}
                    onSuccess(server);
                }                                    
            }
        },
               
        testConnection: function() {
            if (this.form.validate()) {
                var _this = this;
                var server = _this.getServer();
                var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
                bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.testingProgress;
                var testButton = dijit.byId(_this.dialogId+"_testButton");
                testButton.set('disabled', true);
                var dialogCounter = _this.dialogCounter;
                
                // check to see if we can get the status of the server
                var adminEndpoint = new AdminEndpoint({server: server});
                Utils.getStatus(adminEndpoint).then(
                    function(data) {
                        var status = data && data.Server ? data.Server : {};
                        var state = status.State === undefined ? -1 : status.State;
                        var name = status.Name !== undefined ? status.Name : _this.field_serverAdminAddress.get('value') + ":" + _this.field_serverAdminPort.get('value');
                		_this.field_serverName.set('value', name);
                        if (dialogCounter === _this.dialogCounter) {
                            if (Utils.isErrorMode(state)) {
                                bar.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>" + nls.testingFailed;
                            } else {
                                bar.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>" + nls.testingSuccess;
                                _this.dialog.clearErrorMessage();
                            }
                            testButton.set('disabled',false);
                        }
                    },
                    function(error) {
                        if (dialogCounter === _this.dialogCounter) {
                            var name = _this.field_serverAdminAddress.get('value') + ":" + _this.field_serverAdminPort.get('value');
                    		_this.field_serverName.set('value', name);
                            bar.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>" + nls.testingFailed;
                            _this.dialog.showErrorFromPromise(nls.testingFailed, error, adminEndpoint, /*addServerError*/true);
                            testButton.set('disabled',false);
                        }
                    }
                );                
            }
        },
        
        setServer: function(server) {
        	this.field_serverName.set('value', server.name);
        	this.field_serverAdminAddress.set('value', server.adminServerAddress);
        	this.field_serverAdminPort.set('value', server.adminServerPort);
        	this.field_description.set('value', server.description);
        	this.field_sendWebUICredentials.set('checked', server.sendWebUICredentials);
        },
        
        getServer: function() {
            var name = this.field_serverName.get('value');
            var adminAddress = this.field_serverAdminAddress.get('value');
            var adminPort = this.field_serverAdminPort.get('value');
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
        }
                
    });
    
    
	/**
	 * Displays servers in the menubar.
	 */
	var ServerControl = declare("ism.widgets.ServerControl", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin ], {	
	
		serverMenuBarItem: null,
		menuDialog: null,
		templateString: template,
        store: undefined,       
        servers: undefined,
        listServerDialog: null,
        addServerDialog: undefined,
		
        serverLabel: Utils.escapeApostrophe(nls.serverMenuLabel),
		hintText: Utils.escapeApostrophe(nls.serverMenuHint),
		recentServersLabel: Utils.escapeApostrophe(nls.recentServersLabel),
		viewAllLabel: Utils.escapeApostrophe(nls.addRemoveLinkLabel),
		
		serverName: "",
		serverUID: "",
				
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
			var name = ism.user.serverName ? ism.user.serverName : ism.user.server;
			this.serverUID = ism.user.server;
			this.serverName = (name.indexOf(this.serverUID) < 0) ? name + "(" + this.serverUID + ")" : name;
		},
		
		postCreate: function() {
			this.inherited(arguments);
			
			this._updateServers();
			this._updateRecentServers();

			on(this.viewAllLink, "click", lang.hitch(this, function() {
				this.listServers();
			}));
			
			// update the server uri's on open
			aspect.after(this.menuDialog, "onOpen", lang.hitch(this, function() {
				this._updateServers();
				this._updateRecentServers();
				dojo.publish("lastAccess",{requestName: "ServerControl.onOpen"});
			}));
		},
		
        startMonitoring: function () {
			// subscribe to nodeNameTopic to get server name updates
			dojo.subscribe(Resources.contextChange.nodeNameTopic, lang.hitch(this, function(newName) {
				if (this.serverName !== newName) {
					this.setServerName(newName);
				}
			}));

			// subscribe to serverUIDTopic to get server UID updates
			dojo.subscribe(Resources.contextChange.serverUIDTopic, lang.hitch(this, function(obj) {
				this._updateServer(obj.olduid, obj.newsrv);
			}));
        },
        
        showServerName: function() {
        	var menuNameNode = dojo.byId('ismServerMenu_text');
        	menuNameNode.innerHTML = Utils.escapeApostrophe(nls.serverMenuLabel) + " " + this.serverName;
        	var _this = this;
        	on(menuNameNode, "mouseover", function(){
            	Tooltip.show(_this.serverName, menuNameNode);       		
        	});
        	on(menuNameNode, "mouseout", function() {
        		Tooltip.hide(menuNameNode);
        	});
        },
        
        setServerName: function(newName) {
			ism.user.updateServerName(ism.user.server, this.serverName, newName);
			if (newName.indexOf(this.serverUID) < 0) {
				this.serverName = newName + "(" + this.serverUID + ")";
			} else {
	        	this.serverName = newName;
			}
        	this.showServerName();
        },
        
		listServers: function() {
			if (!this.listServerDialog) {
				var listServersId = "listServers"+this.id+"Dialog";
				domConstruct.place("<div id='"+listServersId+"'></div>", "ismServerMenu_rightContents");
				this.listServerDialog = new ListServerDialog({dialogId: listServersId, serverControl: this}, listServersId);
				this.listServerDialog.startup();
			}
			this.listServerDialog.show(this.store);
			this.menuDialog.close();  // cause menus to close
		},
		
		switchServer: function(evt) {
			console.debug(evt);
			if (this.server.get('value')) {
				this.menuDialog.onExecute();  // cause menus to close				
				Utils.reloading = true;
//				this._setSwitching(true);
				window.location = this._buildUri(this.server.get('value'), this.server.get('displayedValue'));
			}
		}, 
		
		_updateServer: function(olduid, newserver) {
			this.store.remove(olduid);
			this.store.add(newserver);
			ism.user.setServerList(this.store.data);
			this.setServerName(newserver.name);
			this._updateServers();
			this._updateRecentServers();
		},
		
		_buildUri: function(server, serverName) {
		    // Redirect to the home page to avoid needless errors
		    // if switch is done while on a page for a server speicific object
		    // like a hub.
		    var uri = Resources.pages.home.uri;
		    var queryParams = [];
		    this._adjustQueryParam("server", server, queryParams);
            this._adjustQueryParam("serverName", serverName, queryParams);
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
				
		_updateServers: function() {
			if (!this.store) {
		    	this.store = new Memory({data:ism.user.getServerList(), idProperty: "uid"});
		    }
			this.servers = this.store.query({});
			
		    this.servers.sort(function(a,b){
                if (a.name < b.name) {
                    return -1;
                }
                return 1;
            }); 
		    
			// add items to typeAhead widget store
		    var t = this;
            this.server.store.fetch({
                onComplete: function(items, request) {
                    // first clear any items
                    var len = items ? items.length : 0;
                    for (var i=0; i < len; i++) {
                        t.server.store.deleteItem(items[i]);
                    } 
                    t.server.store.save({onComplete: function() {
                        // now add them
                        dojo.forEach(t.servers, function(item) {
                            t.server.store.newItem({uid: item.uid, name: item.displayName});    
                        });
                        t.server.store.save();                        
                    }});
            }});
		},
		
//        _setSwitching: function(switching) {
//            if (switching === this.switching) {
//                console.debug("_setSwitching called but already set to " + switching);
//                return;
//            }
//
//            if (switching) {
//                this.switching = true;               
//                dojo.byId(this.id+"_status").innerHTML = nls.switching;
//                
//                // set a timer so it doesn't stay in connecting mode for more than a minute
//                this.switchTimeoutID = window.setTimeout(lang.hitch(this, function(){
//                    this._setSwitching(false);
//                    this._fetchStatus(this.id);
//                }), 60000);
//                
//            } else {
//                this.switching = false;
//                                
//                // clear any timer
//                if (this.switchTimeoutID) {
//                    window.clearTimeout(this.switchTimeoutID);
//                    delete(this.switchTimeoutID);
//                }                               
//            }
//        },
//        
//		_fetchStatus: function(id) {
//			var t = this;
//			Utils.updateStatus(
//				function(data) {
//					var status = t._updateServerStatus(data, t, id);
//					if (!status.unknownOrStopped && !Utils.isMaintenanceMode(status.serverState) && status.serverState != 1 && status.serverState != 10) {
//						// server is in a state of flux, schedule another in 1 second
//						setTimeout(function(){
//							dijit.byId(id)._fetchStatus(id);
//						}, 1000);
//					}
//				},
//				function(error) {	
//			    	console.debug("error: "+error);
//					// are we restarting?
//					if (t.switching) {
//                        setTimeout(function(){
//                            dijit.byId(id)._fetchStatus(id);
//                        }, 1000);
//					    return;
//					}
//					Utils.showPageErrorFromPromise(nls.appliance.systemControl.messagingServer.retrieveStatusError, error);
//					dojo.byId(id+"_status").innerHTML = nls.appliance.systemControl.messagingServer.unknown;
//				});
//		},
			
		_updateRecentServers: function() {		    
		    var recentServers = ism.user.getRecentServerUids();		    		    
			var links = "";
			for (var i = 0; i < recentServers.length; i++) {
			    var server = this.store.query({uid: recentServers[i]});
			    if (server && server.length > 0) {
			        server = server[0];
			    }
			    if (server && server.uid && server.name) {
			        var uri = this._buildUri(server.uid, server.name);
     				links += "<a id='ServerControl_recentServers_link'" + i + 
					    " href='"+uri+"'>" + server.displayName + "</a><br />";
    			    }
			}

			this.recentServers.innerHTML = links;
		}
	});
	
	return ServerControl;
	
});
