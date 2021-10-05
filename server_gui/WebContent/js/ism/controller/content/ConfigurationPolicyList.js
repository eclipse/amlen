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
		'dojo/_base/array',
		'dojo/query',
		'dojo/aspect',
		'dojo/on',
		'dojo/dom-construct',
		'dojo/store/Memory',
		'dojo/number',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'ism/widgets/GridFactory',
		'ism/IsmUtils',		
		'ism/Utils',		
		'ism/controller/content/ConfigurationPolicyDialog',
	    'dojo/text!ism/controller/content/templates/ConfigurationPolicyList.html',
		'dojo/i18n!ism/nls/configurationPolicy',
		'dojo/i18n!ism/nls/strings',
		// Temporary addition for access to translated copies of "Remove not allowed".
		'dojo/i18n!ism/nls/messaging',
		'ism/config/Resources',
		'ism/widgets/IsmQueryEngine',
		'dojo/keys',
		'idx/string'
		], function(declare, lang, array, query, aspect, on, domConstruct, Memory, number, _Widget, _TemplatedMixin,
				Button, Dialog, GridFactory, Utils, baseUtil, ConfigurationPolicyDialog, template, nls, nls_strings, nlsm,
				Resources, IsmQueryEngine, keys, iString) {

	/*
	 * This file defines a configuration widget that displays the list of message hubs
	 */

	var ConfigurationPolicyList = declare ("ism.controller.content.ConfigurationPolicyList", [_Widget, _TemplatedMixin], {

		nls: nls,
		nlsm: nlsm,
		store: null,
		queryEngine: IsmQueryEngine,
		addDialog: null,
		editDialog: null,
		removeDialog: null,
		templateString: template,
		existingNames: [],
		grid: undefined,
		store: undefined,

		addButton: null,
		deleteButton: null,
		editButton: null,
		
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			this.store = new Memory({data:[], idProperty: "id", queryEngine: this.queryEngine});	
		},
		
		postCreate: function() {
			this.inherited(arguments);
            
			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();
			
            // when switching to this page, refresh the field values
            dojo.subscribe(Resources.pages.appliance.nav.adminEndpoint.topic, lang.hitch(this, function(message){
                this._initStore();
            }));    
            dojo.subscribe(Resources.pages.appliance.nav.adminEndpoint.topic+":onShow", lang.hitch(this, function(message){
                this._refreshGrid();
            }));
            
		},

		_initStore: function() {
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.updatingMessage;
			
			var _this = this;
			
			// get the configuration policies
            var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.getDefaultGetOptions();
            var promise = endpoint.doRequest("/configuration/ConfigurationPolicy", options);
            promise.then(
                    function(data) {
                        _this._populatePolicies(data);
                    },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                        _this._populatePolicies({"ConfigurationPolicy" : {}});
                    } else {
                        _this.grid.emptyNode.innerHTML = nls.noItemsGridMessage;
                        Utils.showPageErrorFromPromise(nls.getConfigPolicyError, error);
                    }
                }
            );
		},
		
		_populatePolicies:  function(data) {
		    var _this = this;
            _this.grid.emptyNode.innerHTML = nls.noItemsGridMessage;
            var endpoint = ism.user.getAdminEndpoint();
            var policies = endpoint.getObjectsAsArray(data, "ConfigurationPolicy");
            policies.sort(function(a,b){
                if (a.Name < b.Name) {
                    return -1;
                }
                return 1;
            }); 
            var mapped = dojo.map(policies, function(item){
                item.id = escape(item.Name);
                _this.existingNames.push(item.Name);
                item.ActionListLocalized = _this._getTranslatedAuthString(item.ActionList);
                item.associated = {checked: false, Name: item.Name, sortWeight: 1};
                return item;
            });
            _this.store = new Memory({data: policies, idProperty: "id", queryEngine: _this.queryEngine});
            _this.grid.setStore(_this.store);
            _this.triggerListeners();
        },

		
		_getTranslatedAuthString: function(authString) {
			
			// get the correct translated values for authority...
			
			// if empty or null return empty string
			authString = iString.nullTrim(authString);
			if (!authString) {
				return "";
			}
			
			// get translated value for each auth in list
			var authArray = authString.split(",");
			var translatedAuthString = "";
			for (var i=0, len=authArray.length; i < len; i++) {
				  var authValue = authArray[i].toLowerCase();
				  authValue = iString.nullTrim(authValue);
				  translatedAuthString = translatedAuthString + nls[authValue] + " ";				
			}

			// clean up auth list and replace with localized separator
			translatedAuthString = iString.nullTrim(translatedAuthString);
			translatedAuthString = translatedAuthString.replace(/\s/g, nls.listSeparator + " ");
			return translatedAuthString;			
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the toolbar
			
			// create Edit dialog
			var editId =  "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.id);
			this.editDialog = new ConfigurationPolicyDialog({dialogId: editId, 
			                        dialogTitle: Utils.escapeApostrophe(nls.editDialogTitle),
									dialogInstruction: Utils.escapeApostrophe(nls.dialogInstruction), 
									add: false}, editId);
			this.editDialog.startup();
			
			// create Add dialog
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.id);
			this.addDialog = new ConfigurationPolicyDialog({dialogId: addId,
									 dialogTitle: Utils.escapeApostrophe(nls.addDialogTitle), 
									 dialogInstruction: Utils.escapeApostrophe(nls.dialogInstruction),
									 add: true}, addId);
			this.addDialog.startup();			
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeConfigurationPolicyDialog'></div>", this.id);
			var dialogId = "removeConfigurationPolicyDialog";
			this.removeDialog = new Dialog({
				title: nls.removeDialogTitle,
				content: "<div>" + nls.removeDialogContent + "</div>",
				style: 'width: 600px;',
				buttons: [
				          new Button({
				        	  id: dialogId + "_saveButton",
				        	  label: nls_strings.action.Ok,
				        	  onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
				          })
				          ],
				          closeButtonLabel: nls_strings.action.Cancel
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.save = lang.hitch(this, function(policy, onFinish, onError) {				
				var policyName = policy.Name;				
				if (!onFinish) { onFinish = function() {this.removeDialog.hide();}; }
				if (!onError) { onError = lang.hitch(this,function(error) {	
					var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
					bar.innerHTML = "";
					this.removeDialog.showErrorFromPromise(nls.deleteConfigurationPolicyError, error);
					});
				}
	            var endpoint = ism.user.getAdminEndpoint();
	            var options = endpoint.getDefaultDeleteOptions();
	            var promise = endpoint.doRequest("/configuration/ConfigurationPolicy/" + encodeURIComponent(policyName), options);
	            promise.then( onFinish, onError);
			});			
			
			// create Remove not allowed dialog
			domConstruct.place("<div id='removeNotAllowedConfigurationPolicyDialog'></div>", this.id);
			var rmNotAllowedId = "removeNotAllowedConfigurationPolicyDialog";
			this.removeNotAllowedDialog = new Dialog({
				title: nlsm.messaging.connectionPolicies.removeNotAllowedDialog.title,
				content: "<div></div>",
				style: 'width: 600px;',
				closeButtonLabel: nls_strings.action.Close,
				showError: function(error) {
					this.showErrorFromPromise(nls.deleteConfigurationPolicyError, error);
				}
			}, rmNotAllowedId);
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.nameColumnLabel, field: "Name", dataType: "string", width: "120px",
						decorator: Utils.nameDecorator
					},
					{	id: "UserID", name: nls.userIdColumnLabel, field: "UserID", dataType: "string", width: "120px",
					    decorator: Utils.textDecorator
					},
					{   id: "GroupID", name: nls.groupIdColumnLabel, field: "GroupID", dataType: "string", width: "120px",
					    decorator: Utils.textDecorator
				    },
					{   id: "ClientAddress", name: nls.clientIPColumnLabel, field: "ClientAddress", dataType: "string", width: "120px",
                        decorator: Utils.textDecorator
					},
					{   id: "CommonNames", name: nls.commonNameColumnLabel, field: "CommonNames", dataType: "string", width: "120px",
                        decorator: Utils.textDecorator
					},					
					{	id: "ActionListLocalized", name: nls.authorityColumnLabel, field: "ActionListLocalized", dataType: "string", width: "150px" },
	                {   id: "Description", name: nls.descriptionColumnLabel, field: "Description", dataType: "string", width: "150px",
					    decorator: Utils.textDecorator
					}
				];

			this.grid = GridFactory.createGrid(gridId, 'configPolicyGridContent', this.store, structure, {paging: true, filter: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);		
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh().then(lang.hitch(this, function() {
				this.grid.resize();
			}));
		},
		

		_initEvents: function() {
			// summary:
			// 		init events

			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    this.addDialog.show(null, this.existingNames);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
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
				
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				if (dijit.byId(this.editDialog.dialogId+"_DialogForm").validate()) {
					var id = this.grid.select.row.getSelected()[0];
					var policy = this.store.query({id: id})[0];
					var policyName = policy.Name;
					this.editDialog.save(lang.hitch(this, function(data) {
						data.id = escape(data.Name);
                        data.ActionListLocalized = this._getTranslatedAuthString(data.ActionList);
                        data.associated = {checked: false, Name: data.Name, sortWeight: 1};
                        if (policyName != data.Name) {
							this.store.remove(id);
							this.store.add(data);
							var index = dojo.indexOf(this.existingNames, policyName);
							if (index >=0) {
								this.existingNames[index] = data.Name;
							}						
						} else {
							this.store.put(data, {overwrite: true});
						}
						this.triggerListeners();						
						this.editDialog.dialog.hide();
					}));
				}
			}));
			
			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
					console.debug("saving addDialog");
					if (dijit.byId(this.addDialog.dialogId+"_DialogForm").validate()) {		
						this.addDialog.save(lang.hitch(this, function(data) {
							data.id = escape(data.Name);
							data.ActionListLocalized = this._getTranslatedAuthString(data.ActionList);
	                        data.associated = {checked: false, Name: data.Name, sortWeight: 1};
							this.store.add(data);
							this.triggerListeners();
							this.existingNames.push(data.Name);												
							this.addDialog.dialog.hide();
						}));
					}
				
			}));

			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var policy = this.store.query({id: id})[0];				
				var policyName = policy.Name;
				this.removeDialog.save(policy, lang.hitch(this, function(data) {
					this.removeDialog.hide();
					bar.innerHTML = "";						
					this.store.remove(id);
					var index = dojo.indexOf(this.existingNames, policyName);
					if (index >= 0) {
						this.existingNames.splice(index, 1);
					}					
					if (this.store.data.length === 0 ||
							this.grid.select.row.getSelected().length === 0) {
						this.deleteButton.set('disabled',true);
						this.editButton.set('disabled',true);							
					}	
				}), lang.hitch(this, function(error) {
						bar.innerHTML = "";
						var errCode = baseUtil.getErrorCodeFromPromise(error);
						if (errCode === "CWLNA0376") {
							// TODO: The lines related to CWLNA0376 are a temporary code work around for a defect opened
							// just before GM.  Post-V2, we should implement "remove not allowed" the way it is for other
							// configuration object in use cases.
							this.removeDialog.hide();
							this.removeNotAllowedDialog.show();
							this.removeNotAllowedDialog.showError(error);
						} else {
							this.removeDialog.showErrorFromPromise(nls.deleteConfigurationPolicyError, error);
						}
					}
				));

			}));
			
		},
		
		_doSelect: function() {
			var selection = this.grid.select.row.getSelected(); 
			if (selection.length > 0) {
				var id = selection[0];
				var policy = this.store.query({id: id})[0];				
				this.deleteButton.set('disabled', false);
				this.editButton.set('disabled',false);
			} else {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);
			}
		},
		
		_doEdit: function() {
		    var policy = this.grid.store.query({id: this.grid.select.row.getSelected()[0]})[0];
			var policyName = policy.Name;			
			// create array of existing names except for the current one, so we don't clash
			var names = array.map(this.existingNames, function(item) { 
				if (item != policyName) return item; 
			});
			this.editDialog.show(policy, names);
		},
		
		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated 
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");
			
			// update grid quick filter if there is one
			if (this.grid.quickFilter) {
				this.grid.quickFilter.apply();
			}
			// re-sort
			if (this.grid.sort) {
				var sortData = this.grid.sort.getSortData();
				if (sortData && sortData !== []) {
					this.grid.model.clearCache();
					this.grid.sort.sort(sortData);
				}
			}
		}
	});

	return ConfigurationPolicyList;
});
