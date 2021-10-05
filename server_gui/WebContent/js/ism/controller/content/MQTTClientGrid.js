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
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',				
		'dojo/aspect',
		'dojo/query',
		'dojo/promise/all',
		'dojo/on',
		'dojo/date/locale',
		'dojo/when',
		'dojo/number',
		'dojo/store/Memory',
		'dojo/data/ItemFileWriteStore',
		'dijit/_base/manager',
		'dijit/layout/ContentPane',
		'dijit/MenuItem',
		'dijit/form/Button',
		'dijit/form/Form',
		'dijit/form/DropDownButton',
		'dijit/Menu',
		'dojo/i18n!ism/nls/monitoring',
		'dojo/i18n!ism/nls/strings',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/widgets/Select',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/MQTTClientGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/text!ism/controller/content/templates/ClientDeleteResults.html'
		], function(declare, lang, array, domClass, domConstruct, domStyle, aspect, query, all, on, locale, when, number, Memory, 
				ItemFileWriteStore, manager, ContentPane, MenuItem, Button, Form, DropDownButton, Menu,	nls, nls_strings, 
				GridFactory, Dialog, Select, _TemplatedContent, gridtemplate, Resources, Utils,	MonitoringUtils, clientDeleteResults) {

	var ClientDeleteResults = declare("ism.controller.content.ClientDeleteResults", _TemplatedContent, {
		
		MAX_AUTO_HEIGHT_ROWS: 5,
		VSCROLLER_HEIGHT: "210px",
		
		nls: nls,
		templateString: clientDeleteResults,
		pendingStore: null,
		failureStore: null,
		pendingGrid: null,
		failureGrid: null,		
		pendingContentId: "mcg_pendingContent",
		failureContentId: "mcg_failureContent",
		dialogId: "mcg_clientDeleteResults",
		
		Utils: Utils,
	    deleteResultsTitle: Utils.escapeApostrophe(nls.monitoring.deleteResultsDialog.clientsTitle),

		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);			
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
		},
		
		_initPendingGrid: function() {
			var pendingStructure = [
   	 				{	id: "clientId", name: nls.monitoring.grid.mqtt.clientId, tooltip: nls.monitoring.grid.mqtt.clientIdTooltip, width: "150px", decorator: Utils.nameDecorator }
 				];
			
			this.pendingGrid = GridFactory.createGrid("clientsPendingDeleteGridContent", this.pendingContentId, this.pendingStore, pendingStructure, 
									{ filter: false, paging: false, suppressSelection: true, height: this.VSCROLLER_HEIGHT, vscroller: true});						
		},

		_initFailureGrid: function() {
			var failureStructure = [
	 				{	id: "clientId", name: nls.monitoring.grid.mqtt.clientId, tooltip: nls.monitoring.grid.mqtt.clientIdTooltip, width: "150px", decorator: Utils.nameDecorator },
	 				{	id: "errorMessage", name: nls.monitoring.grid.mqtt.errorMessage, tooltip: nls.monitoring.grid.mqtt.errorMessageTooltip, width: "400px", decorator: Utils.textDecorator}
 				];
			
			this.failureGrid = GridFactory.createGrid("clientsFailureDeleteGridContent", this.failureContentId, this.failureStore, failureStructure, 
					{ filter: false, paging: false, suppressSelection: true, height: this.VSCROLLER_HEIGHT, vscroller: true});	
			this.failureGrid.startup();
		},

		show: function(failure, pending) {
			
			failure = failure ? failure : [];
			pending = pending ? pending : [];
						
			this.reset();
			
			var showFailure = failure.length > 0;
			var showPending = pending.length > 0;
			var showSuccess = !showFailure && !showPending;
			
			if (showSuccess) {
				domStyle.set(this.successDiv, 'display', 'block');								
			} 
			
			if (showFailure) {				
				this.failureStore = new Memory({data:failure});
				if (!this.failureGrid) {
					this._initFailureGrid();
				} else {
					this.failureGrid.setStore(this.failureStore);
				}
				// if there are fewer than MAX_AUTO_HEIGHT_ROWS, use autoHeight
				if (failure.length < this.MAX_AUTO_HEIGHT_ROWS) {
					this.failureGrid.set("autoHeight", true);
				// otherwise, use a vscroller
				} else {
					this.failureGrid.set("autoHeight", false);						
					this.failureGrid.domNode.style.height = this.VSCROLLER_HEIGHT;	
				}
				domStyle.set(this.failureDiv, 'display', 'block');
			} 
			
			if (showPending) {
				this.pendingStore = new Memory({data:pending});
				if (!this.pendingGrid) {
					this._initPendingGrid();
				} else {
					this.pendingGrid.setStore(this.pendingStore);					
				}
				// if there are fewer than MAX_AUTO_HEIGHT_ROWS, use autoHeight				
				if (pending.length < this.MAX_AUTO_HEIGHT_ROWS) {
					this.pendingGrid.set("autoHeight", true);
				// otherwise, use a vscroller					
				} else {
					this.pendingGrid.set("autoHeight", false);						
					this.pendingGrid.domNode.style.height = this.VSCROLLER_HEIGHT;	
				}
				domStyle.set(this.pendingDiv, 'display', 'block');					
			}
			this.dialog.show();		
			this.dialog.resize();
		},
		
		reset: function() {
			domStyle.set(this.failureDiv, 'display', 'none');
			domStyle.set(this.pendingDiv, 'display', 'none');				
			domStyle.set(this.successDiv, 'display', 'none');	
			if (this.pendingGrid) {
				this.pendingGrid.setStore(new Memory({data:[]}));
			}
			if (this.failureGrid) {
				this.failureGrid.setStore(new Memory({data:[]}));
			}			
		}
			
	});

	var MQTTClientGrid = declare("ism.controller.content.MQTTClientGrid", _TemplatedContent, {

		nls: nls,
		templateString: gridtemplate,
		
		removeDialog: null,

		timeOffset: null,

		grid: null,
		store: null,

		deleteButton: null,

		restURI: "rest/config/mqttClients",

		contentId: "mqttClientGridContent",
		
		domainUid: 0,		
		
		timeoutID: null,
		
		deleteBatchSize: 10,
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
            
            this.domainUid = ism.user.getDomainUid();
			
			// Add the domain Uid to the REST API. For now default to domainUid 0.
			this.restURI += "/" + this.domainUid;
			

			// if we can't delete disconnect clients, replace the tagline on page
			if (!ism.user.hasPermission(Resources.actions.deleteMqttClient)) {
				dojo.byId("mcm_tagline").innerHTML = nls.monitoring.mqttClientMonitor.taglineNoDelete;
			} 

			Utils.updateStatus(function(data) {
				if (data.dateTime) {
					var appTime = new Date(data.dateTime);
					this.timeOffset = dojo.date.difference(new Date(), appTime, "second");
				}				
			}, function (error) {
				console.debug("Error: ", error);				
			});
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.mqttClientMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));

			this.createGrid();
			this._updateStore();
			this._initDialogs();
			this._initEvents();
		},

		_updateStore: function() {
			dojo.style(dojo.byId('mcg_refreshingLabel'), "display", "block");
			var name = this.clientId.value;
			var results = 100;

			var deferred = MonitoringUtils.getMQTTClientData(name, results);
			
			deferred.then(lang.hitch(this, function(data) {
				data = data.MQTTClient;
				// Add 'id' field and throughput
				var mapped = array.map(data, function(item, index){
					var record = {};
					record.id = 1+index;
					if (item.ClientID) {
						record.clientId = item.ClientID;
					}
					if (item.LastConnectedTime) {
						record.lastConn = item.LastConnectedTime;
					}
					return record;
				});
				
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
				this.triggerListeners();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('mcg_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(err) {
				console.debug("Error from getMQTTClientData: "+err);
				dojo.style(dojo.byId('mcg_refreshingLabel'), "display", "none");

				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},

		_initDialogs: function() {

			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeMQTTClientDialog'></div>", this.contentId);
			var dialogId = "removeMQTTClientDialog";
			var title = nls.monitoring.dialog.removeClientTitle;
			var removeDialogButton = new Button({
				id: dialogId + "_button_ok",
				label: nls.monitoring.removeDialog.deleteButton,
				onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
			});
			
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + nls.monitoring.dialog.removeClientContent + "</div>",
				buttons: [ removeDialogButton ],
				closeButtonLabel: nls.monitoring.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.removeDialogButton = removeDialogButton;			
			this.removeDialog.save = lang.hitch(this, function(clients, onFinish, onError) { 
				if (!onFinish) { onFinish = {};}
				if (!onError) { onError = lang.hitch(this,function(error) {	
					this.removeDialog.showErrorFromPromise(nls.monitoring.deletingFailed, error);
					this.removeDialog.removeDialogButton.set('disabled', true);
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					});
				}
				var grid = this.grid;
				var mappedData = array.map(clients, function(row) {
					var client = grid.row(row).data();
					return { ClientID: client.clientId };
				});
				var failed = [];
				var pending = [];
				var success = [];
				for (var idx=0; idx < mappedData.length; idx++) {
					pending.push({id: pending.length, clientId: mappedData[idx].ClientID});
				}
				this._processDeleteRequests(mappedData, onFinish, success, failed, pending);
			});

		},
		
		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "mqttClientGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}
			
			var structure = [
				{	id: "clientId", name: nls.monitoring.grid.mqtt.clientId, tooltip: nls.monitoring.grid.mqtt.clientIdTooltip, width: "300px", decorator: Utils.nameDecorator },
				{	id: "lastConn", name: nls.monitoring.grid.mqtt.lastConn, tooltip: nls.monitoring.grid.mqtt.lastConnTooltip, dataType: "string", width: "300px",
					decorator: lang.hitch(this, function(cellData) {
						var lastUpd = dojo.date.add(new Date(cellData), "second", this.timeOffset);
						return locale.format(lastUpd, { datePattern: "yyyy-MM-dd", timePattern: "HH:mm (z)" });
					})
				}
			];
			this.grid = GridFactory.createGrid(gridId, "mqttClientGridContent", this.store, structure, 
					{ filter: true, paging: true, actionsMenuTitle: nls_strings.action.Actions, 
					  extendedSelection: true, multiple: true });

			if (ism.user.hasPermission(Resources.actions.deleteMqttClient)) {
				this.deleteButton = new MenuItem({
					label: nls.monitoring.grid.deleteClientButton
				});
				GridFactory.addToolbarMenuItem(this.grid, this.deleteButton, nls_strings.action.Actions);				
				this.deleteButton.set('disabled', true);
			}
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
			if (this.deleteButton) {
				this.deleteButton.set('disabled', true);
			}
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
			if (this.mcg_form.validate()) {
				this._updateStore();
			}
		},

		_updateButtons: function() {
			console.log("selected: "+this.grid.select.row.getSelected());
			if (!this.deleteButton) { return; }
			if (this.grid.select.row.getSelected().length > 0) {
				this.deleteButton.set('disabled', false);
			} else {
				this.deleteButton.set('disabled', true);
			}
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			if (ism.user.hasPermission(Resources.actions.deleteMqttClient)) {
				aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
					this._updateButtons();
				}));

				aspect.after(this.grid, "onRowHeaderHeaderClick", lang.hitch(this, function() {
					console.debug("after onRowHeaderHeaderClick");
					this._updateButtons();
				}));
				
				on(this.deleteButton, "click", lang.hitch(this, function() {
					console.debug("clicked delete button");
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.removeDialogButton.set('disabled', false);
					var selected = this.grid.select.row.getSelected();
					if (selected && selected.length > 0) {
						var message = nls.monitoring.dialog.removeClientContent;
						var title = nls.monitoring.dialog.removeClientTitle;
						if (selected.length > 1) {
							message = lang.replace(nls.monitoring.dialog.removeClientsContent, [selected.length]);
							title = nls.monitoring.dialog.removeClientsTitle;
						}
						this.removeDialog.set('title', title);
						this.removeDialog.set('content', "<div>" + message + "</div>");
						this.removeDialog.show();
					}
				}));
			}

			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function() {
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.monitoring.deletingProgress;
				this.removeDialog.save(this.grid.select.row.getSelected(), lang.hitch(this, function(data) {
					this.removeDialog.hide();					
					bar.innerHTML = "";
					if (data) {						
						var failed = data.failed ? data.failed : [];
						var pending = data.pending ? data.pending : [];
						
			    		if (!this.deleteResultsDialog) {
							var deleteResultsDialogId = "deleteResultsInfo"+this.id;
							domConstruct.place("<div id='"+deleteResultsDialogId+"'></div>", this.contentId);
							this.deleteResultsDialog = new ClientDeleteResults({
								id: deleteResultsDialogId,
								title: nls.monitoring.deleteResultsDialog.clientsTitle,
								closeButtonLabel: nls.monitoring.deleteResultsDialog.closeButton
							}, deleteResultsDialogId);
							this.deleteResultsDialog.dialogId = deleteResultsDialogId;
                            aspect.after(this.deleteResultsDialog.dialog, "onHide", lang.hitch(this, function() {
                                this.updateGrid();
                           }));							
			    		}
			    		
			    		this.deleteResultsDialog.show(failed, pending);
					}					
				}));
			}));
		},
		
		handleResolved: function(response, args/* clId, clients, mappedData, onFinish, success, failed, pending */) {
			var t = this;
			var clId = args.clId;
			var clients = args.clients;
			var mappedData = args.mappedData;
			var onFinish = args.onFinish;
			var success = args.success;
			var failed = args.failed;
			var pending = args.pending;
			
			for(var i = 0; i < clients.length; i++) {
				if (clients[i] == clId) {
					console.debug("resolved: " + clId + " " + JSON.stringify(response));
					
					// Add the ID to the array of successful requests
					success.push({clientId: clId, code: response.Code, message: response.Message});
					
					// Remove the ID from the array of pending requests
					for(var j = 0; j < pending.length; j++) {
						if (pending[j].clientId === clId) {
							pending.splice(j, 1);
							break;
						}
					}

					// Check to see if we have finished processing the current batch.  If so, cancel
					// the batch timeout.  Next, if not all IDs have been deleted, make a recursive 
					// call to _processDeleteRequests in order to process the next batch.
					// Otherwise, call onFinish.
					clients.splice(i, 1);
					if (clients.length === 0) {
						if (t.timeoutID) {
							 window.clearTimeout(t.timeoutID);
						}
						console.debug("Succeeded: " + JSON.stringify(success));
						if (mappedData.length) {
							t._processDeleteRequests(mappedData, onFinish, success, failed, pending);
						} else {
							onFinish({success: success, failed: failed, pending: pending});
						}
					}
					break;
				}
			}
		},
		
		handleRejected: function(response, args/* clId, clients, mappedData, onFinish, success, failed, pending */) {
			var t = this;
			var clId = args.clId;
			var clients = args.clients;
			var mappedData = args.mappedData;
			var onFinish = args.onFinish;
			var success = args.success;
			var failed = args.failed;
			var pending = args.pending;

			for(var i = 0; i < clients.length; i++) {
				if (clients[i] == clId) {
					console.debug("rejected: " + clId + " " + JSON.stringify(response));
					
					// Add the ID to the array of failed requests
					var item = {id: failed.length, clientId: clId,  errorMessage: response.response.data.Code + ": " + response.response.data.Message};
					failed.push(item);
					
					// Remove the ID from the array of pending requests
					for(var j = 0; j < pending.length; j++) {
						if (pending[j].clientId === clId) {
							pending.splice(j, 1);
							break;
						}
					}

					// Check to see if we have finished processing the current batch.  If so, cancel
					// the batch timeout.  Next, if there are additional IDs delete, make a recursive 
					// call to _processDeleteRequests in order to process the next batch.
					// Otherwise, call onFinish.
					clients.splice(i, 1);
					if (clients.length === 0) {
						if (t.timeoutID) {
							 window.clearTimeout(t.timeoutID);
						}
						console.debug("Failed: " + JSON.stringify(failed));
						if (mappedData.length) {
							t._processDeleteRequests(mappedData, onFinish, success, failed, pending);
						} else {
							onFinish({success: success, failed: failed, pending: pending});
						}
					}
					break;
				}
			}
		},

		_processDeleteRequests: function(mappedData, onFinish, in_success, in_failed, in_pending) {
			var t = this;
			var delReqs = [];
			var success = in_success;
			var failed = in_failed;
			var pending = in_pending;
			var batchSize = mappedData.length > this.deleteBatchSize ? this.deleteBatchSize : mappedData.length;
			var clients = [];
			
			// When we enter this method, we are ready to process a new batch of delete requests.
			// If there's a pending timeout from an earlier batch, cancel it now.
			if (this.timeoutID) {
				 window.clearTimeout(this.timeoutID);
			}

			for (var idx=0; idx < batchSize; idx++) {
				clients[idx] = mappedData[idx].ClientID;
			}
			mappedData.splice(0, batchSize);
			
			for (idx=0; idx < batchSize; idx++) {
				var args = {clId: clients[idx], clients: clients, mappedData: mappedData, onFinish: onFinish, success: success, failed: failed, pending: pending};
				var deleteReq = { clientId: clients[idx], promise: MonitoringUtils.deleteMQTTClient(clients[idx]).trace({t: t, args: args}) };
				delReqs.push(deleteReq);
			}
			
			// Allow up to deleteBatchSize seconds for deletes in the current batch to complete.
			// If there are additional IDs delete, make a recursive 
			// call to _processDeleteRequests in order to process the next batch.
			// Otherwise, call onFinish.
			var deleteTimeout = this.deleteBatchSize * 1000;
			this.timeoutID = window.setTimeout(lang.hitch(this, function(){
				if(clients.length !== 0) {
					if (mappedData.length) {
						t._processDeleteRequests(mappedData, onFinish, success, failed, pending);
					} else {
						onFinish({success: success, failed: failed, pending: pending});
					}
				} 
			}), deleteTimeout);
		},
		
		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated 
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");
            
			if (this.grid.paginationBar) {
                this.grid.paginationBar.refresh();
            }
            			
			// update grid quick filter if there is one
			if (this.grid.quickFilter) {
				this.grid.quickFilter.apply();
			}
		}
	});
	return MQTTClientGrid;
});
