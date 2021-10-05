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
		'dojo/query',
		'dojo/_base/array',
		'dojo/aspect',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',		
		'dojo/store/Memory',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/registry',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'ism/widgets/GridFactory',
		'ism/IsmUtils',	
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'ism/config/Resources',
		'ism/controller/content/MessageHubDialog',
		'dojo/keys'
		], function(declare, lang, query, array, aspect, on, domClass, domConstruct, domStyle, 
				Memory, _Widget, _TemplatedMixin, 
				registry, Button, Dialog, GridFactory, Utils, nls, nls_strings, 
				Resources, MessageHubDialog, keys) {

	/*
	 * This file defines a configuration widget that displays the list of message hubs
	 */

	var MessageHubList = declare ("ism.controller.content.MessageHubList", [_Widget, _TemplatedMixin], {

		nls: nls,
		grid: null,
		store: null,
		addDialog: null,
		editDialog: null,
		removeDialog: null,
		removeNotAllowedDialog: null,
		contentId: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		currentHub: null,
		isRefreshing: false,

		addButton: null,
		deleteButton: null,
		editButton: null,
		
		domainUid: 0,
		
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug("constructing MessageHubList");
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], idProperty: "Name"});
		},
		
		postCreate: function() {
			this.inherited(arguments);
            
            this.domainUid = ism.user.getDomainUid();
			
			// start with an empty <div> for the content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.messaging.nav.messageHubs.topic, lang.hitch(this, function(message){
				this._initStore();
			}));	
			dojo.subscribe(Resources.pages.messaging.nav.messageHubs.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this._refreshGrid();
				
				if (this.isRefreshing) {
					this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
				}
			}));	
			
			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();
		},
		
		_initStore: function() {
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
			this.isRefreshing = true;
			var _this = this;
			
            // kick off the queries
            var adminEndpoint = ism.user.getAdminEndpoint();
            var messageHubResults = adminEndpoint.doRequest("/configuration/MessageHub", adminEndpoint.getDefaultGetOptions());
            var endpointResults = adminEndpoint.doRequest("/configuration/Endpoint", adminEndpoint.getDefaultGetOptions());
            
            adminEndpoint.getAllQueryResults([messageHubResults, endpointResults], ["MessageHub", "Endpoint"], function(queryResults) {
                _this.isRefreshing = false;
                _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;

                var messageHubs = queryResults.MessageHub;                
                var endpoints = queryResults.Endpoint;
                                                
                messageHubs.sort(function(a,b){
                    if (a.Name < b.Name) {
                        return -1;
                    }
                    return 1;
                });         
            
                var items = [];
                dojo.forEach(messageHubs, function(messageHub, i) {
                    var endpointCount = 0;
                    for (var j=0, len=endpoints.length; j < len; j++) {
                        if (endpoints[j]['MessageHub'] == messageHub.Name) {
                            endpointCount++;
                        }
                    }
                    items.push({id: escape(messageHub.Name), Name: messageHub.Name, Description: messageHub.Description, endpointCount: endpointCount});                    
                });
                
                _this.store = new Memory({data: items, idProperty: "id"});              
                _this.grid.setStore(_this.store);
                _this.triggerListeners();
                
            }, function(error) {
                _this.isRefreshing = false;
                _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
                Utils.showPageErrorFromPromise(nls.messaging.messageHubs.retrieveMessageHubsError, error);                
            });
		},
		
		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the toolbar

			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeMessageHubDialog'></div>", this.contentId);
			var dialogId = "removeMessageHubDialog";

			this.removeDialog = new Dialog({
				title: nls.messaging.messageHubs.messageHubList.removeDialog.title,
				content: "<div>" + nls.messaging.messageHubs.messageHubList.removeDialog.content + "</div>",
				buttons: [
				          new Button({
				        	  id: dialogId + "_button_ok",
				        	  label: nls_strings.action.Ok,
				        	  onClick: function() { 
				        		  dojo.publish(dialogId + "_saveButton", ""); }
				          })
				          ],
				          closeButtonLabel: nls_strings.action.Cancel
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.save = lang.hitch(this, function(messageHubName, onFinish, onError) { 
				if (!onFinish) { onFinish = {};}
				if (!onError) { onError = lang.hitch(this,function(error) {	
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.showErrorFromPromise(nls.messaging.messageHubs.deleteMessageHubError, error);
					});
				}
				console.debug("Deleting: ", messageHubName);
				var endpoint = ism.user.getAdminEndpoint();
	            var options = endpoint.getDefaultDeleteOptions();
	            var promise = endpoint.doRequest("/configuration/MessageHub/" + encodeURIComponent(messageHubName), options);
				promise.then(onFinish, onError);				
			});
			
			// create Remove not allowed dialog (remove not allowed when there are endpoints that reference the hub)
			var removeNotAllowedId = this.id+"_removeNotAllowedDialog";
			domConstruct.place("<div id='"+removeNotAllowedId+"'></div>", this.id);
			this.removeNotAllowedDialog = new Dialog({
				id: removeNotAllowedId,
				title: nls.messaging.messageHubs.removeNotAllowedDialog.title,
				content: "<div style='display: table;'>" +
						"<img style='display: inline-block; float: left; margin: 0 10px 0 0;' src='css/images/warning24.png' alt='' />" +
						"<span style='display: table-cell; vertical-align:middle; word-wrap:break-word;'>" + 
						nls.messaging.messageHubs.removeNotAllowedDialog.content + "</span><div>",
				closeButtonLabel: nls.messaging.messageHubs.removeNotAllowedDialog.closeButton
			}, removeNotAllowedId);
			this.removeNotAllowedDialog.dialogId = removeNotAllowedId;

			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new MessageHubDialog({dialogId: addId, 
												   title: nls.messaging.messageHubs.addDialog.title, 
												   instruction: nls.messaging.messageHubs.addDialog.instruction,
												   add: true});
			this.addDialog.startup();
		},


		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.messaging.messageHubs.messageHubList.name, field: "Name", dataType: "string", width: "200px",
						decorator: Utils.nameDecorator
					},
					{	id: "endpointCount", name: nls.messaging.messageHubs.messageHubList.metricLabel, field: "endpointCount", dataType: "number", width: "100px",
						decorator: function(cellData) {					
							if (cellData === 0) {
								var value="<span style='color: red; vertical-align: middle;'>" + cellData + "</span>" +
								"<span style='padding-left: 10px;'>" +
								"<img style='vertical-align: middle' src='css/images/msgWarning16.png' alt='"+ nls_strings.level.Warning +"' /></span>";
								return value;
							} else {
								return cellData;
							}
						}
					},
					{	id: "Description", name: nls.messaging.messageHubs.messageHubList.description, field: "Description", dataType: "string", width: "400px",
						decorator: Utils.textDecorator
					}					
				];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, {paging: true, filter: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
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
			    this.addDialog.show(existingNames, existingNames);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var messageHub = this._getSelectedData();
				var numEndpoints = messageHub.endpointCount;
				if (numEndpoints !== 0) {
					this.removeNotAllowedDialog.show();
				} else {
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.show();
				}
			}));
					
			on(this.editButton, "click", lang.hitch(this, function() {
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
			
			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving addDialog");
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {		
					var bar = query(".idxDialogActionBarLead",this.addDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
					this.addDialog.save(lang.hitch(this, function(data) {
						data.endpointCount = 0;
						data.id = escape(data.Name);
						this.store.add(data);
						this.triggerListeners();
						this.addDialog.dialog.hide();
					}));
				}
			}));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var messageHubName = unescape(id);
				console.log("deleting " + messageHubName);
				this.removeDialog.save(messageHubName, lang.hitch(this, function() {
					this.store.remove(id);
					if (this.store.data.length === 0 ||
							this.grid.select.row.getSelected().length === 0) {
						this.deleteButton.set('disabled',true);
						this.editButton.set('disabled',true);
					}					
					this.removeDialog.hide();					
					bar.innerHTML = "";
				}));
			}));
			
			dojo.subscribe('messageHubItemChanged', lang.hitch(this, function(message){
				var messageHub = message;
				// update the name and description
				if (!messageHub.id) {
					messageHub.id = escape(messageHub.Name);
				}
				var item = this.grid.store.query({id: messageHub.id})[0];
				if (item) {
					item.Name = messageHub.Name;
					item.Description = messageHub.Description;
					this.grid.store.put(item, {id: item.id, overwrite: true});
					this.triggerListeners();
				}				
			}));			

			
			dojo.subscribe("EndpointRemoved", lang.hitch(this, function(message) {
				var messageHubName = message.messageHub;
				var item = this.grid.store.query({Name: messageHubName})[0];
				if (item) {
					var count = item.endpointCount;
					if (count > 0) {
						count--;
					}
					item.endpointCount = count;
					this.grid.store.put(item, {id: item.id, overwrite: true});
					this.triggerListeners();
				}
			}));

			dojo.subscribe("EndpointAdded", lang.hitch(this, function(message) {
				var messageHubName = message.messageHub;
				var item = this.grid.store.query({Name: messageHubName})[0];
				if (item) {
					var count = item.endpointCount + 1;						
					item.endpointCount = count;
					this.grid.store.put(item, {id: item.id, overwrite: true});
					this.triggerListeners();
				}
			}));			
		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				console.debug("index="+ind);
				console.log("row count: "+this.grid.rowCount());
				this.deleteButton.set('disabled',false);
				this.editButton.set('disabled',false);
			} else {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);
			}
		},
		
		_doEdit: function() {
			// any message on this topic means that we clicked "edit"
			// get the id and name of the selected hub
			var messageHubId = this.grid.select.row.getSelected()[0];
			var messageHub = this._getSelectedData();
			var messageHubName = messageHub.Name;
			
			// TODO: There is an issue with gridx in a tabbed pane - it is not displaying the
			//       grid for the selected tab if you have modified the contents of the grid in any way.
			//       As soon as you select a different tab and then reselect the tab, the grid renders.
			//       I've tried setStore followed by all manner of resize, refresh, relayout... I've tried 
			//       just adding and removing data from the store.  Gridx bug?
			//       If the grids need to be updated, for now, just reload the page.
			if (!this.currentHub) {
				var details = dijit.byId("ismMessageHubDetails");
				if (details) {
					// this can happen if the details page is bookmarked or refreshed
					this.currentHub = details.get("messageHubItem");
				}
			}
			
			// 23465 and the above TODO regarding not being able to update the grids in the tabbed panes,
			// which I confirmed is still an issue, requires that we reload the details page each time...
			/*if (this.currentHub != null && this.currentHub.Name != messageHub.Name) {*/
				var detailsUri = Utils.getBaseUri() + Resources.pages.messaging.uri + "?nav=messageHubDetails&Name=" + encodeURIComponent(messageHubName);
	            if (ism.user.server) {
	                detailsUri += "&server=" +  encodeURIComponent(ism.user.server) + "&serverName=" + encodeURIComponent(ism.user.serverName);
	            }
				console.debug("redirecting to " + detailsUri);
				window.location = detailsUri;				
			/*} else {
				this.currentHub = messageHub;
				// publish our desire to switch panes
				var data = {page: "messageHubDetails", urlParams: "Name=" + encodeURIComponent(messageHubName), messageHub: messageHub};				
				dojo.publish(Resources.pages.messaging.nav.messageHubDetails.topic, data);
			} */
			
		},

		refreshGrid:  function() {
			console.debug("RefreshGrid");
			this._refreshGrid();
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
	
	return MessageHubList;
});
