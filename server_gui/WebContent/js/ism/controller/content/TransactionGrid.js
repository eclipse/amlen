/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
		'dojo/aspect',
		'dojo/query',
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
		'dojo/text!ism/controller/content/templates/TransactionGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils'
		], function(declare, lang, array, domClass, domConstruct, aspect, query, on, locale, when, number, Memory, ItemFileWriteStore, manager, ContentPane, MenuItem, Button, 
				Form, DropDownButton, Menu, nls, nls_strings, GridFactory, Dialog, Select, _TemplatedContent, gridtemplate, Resources, Utils, MonitoringUtils) {

	return declare("ism.controller.content.TransactionGrid", _TemplatedContent, {

		nls: nls,
		templateString: gridtemplate,
		actionConfirmDialog: null,
		timeOffset: null,

		grid: null,
		store: null,

		commitButton: null,
		rollbackButton: null,
		forgetButton: null,

		restURI: "rest/config/transactions",
		domainUid: 0,

		contentId: "transactionGridContent",
		
		STATE_PREPARED: 2,
		STATE_HEURISTICALLY_COMMITTED: 5,
		STATE_HEURISTICALLY_ROLLED_BACK: 6,
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
		},
		
		postCreate: function() {		
            
            this.domainUid = ism.user.getDomainUid();
			
		    console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;

			Utils.updateStatus(function(data) {
				console.debug("TIME");
				console.dir(data);
				if (data.dateTime) {
					var appTime = new Date(data.dateTime);
					this.timeOffset = dojo.date.difference(new Date(), appTime, "second");
				}				
			}, function(error) {
				console.debug("Error: ", error);
			});

			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.transactionMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));
			
	
			this.createGrid();
			this._updateStore();
			this._initDialogs();
			this._initEvents();
		},

		_updateStore: function() {
			dojo.style(dojo.byId('trans_grid_refreshingLabel'), "display", "inline");
			var deferred = MonitoringUtils.getTransactionData({});
			var _this = this;
			
			deferred.then(lang.hitch(this, function(data) {
				data = data.Transaction;
				// Add 'id' field and throughput
				var mapped = array.map(data, function(item, index){
					var record = {};
					record.id = 1+index;
					record.xId = item.XID;
					record.timestamp = number.parse(item.stateChangedTime);
					record.state = number.parse(item.state);
					record.stateString = _this._getStateString(item.state);
					return record;
				});
				
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
				this.triggerListeners();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('trans_grid_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(err) {
				console.debug("Error from getTransactionData: "+err);
				dojo.style(dojo.byId('trans_grid_refreshingLabel'), "display", "none");
				
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},
	

		_initDialogs: function() {

			// summary:
			//		create action dialog (confirmation)
			//      this dialog will be used for all three available actions
			domConstruct.place("<div id='transActionConfirmDialog'></div>", this.contentId);
			var dialogId = "transActionConfirmDialog";
			var title = "";
			var actionConfirmDialogButton = new Button({
				id: dialogId + "_button_ok",
				label: "",
				onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
			});
			this.actionConfirmDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "",
				buttons: [ actionConfirmDialogButton ],
				closeButtonLabel: nls.monitoring.actionConfirmDialog.cancelButton
			}, dialogId);
			this.actionConfirmDialog.dialogId = dialogId;
			this.actionConfirmDialog.actionConfirmDialogButton = actionConfirmDialogButton;
			this.actionConfirmDialog.save = lang.hitch(this, function(xId, onFinish, onError) { 
				if (!onFinish) { onFinish = {};}
				if (!onError) { onError = lang.hitch(this,function(error) {	
					this.actionConfirmDialog.showErrorFromPromise(this.actionConfirmDialog.failedMsg, error);
					this.actionConfirmDialog.actionConfirmDialogButton.set('disabled', true);
					query(".idxDialogActionBarLead",this.actionConfirmDialog.domNode)[0].innerHTML = "";
				});}
				console.debug("Processing transaction: ", this.actionConfirmDialog.action, xId);
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.createPostOptions("XID", xId);
				var promise = adminEndpoint.doRequest("/service/" + encodeURIComponent(this.actionConfirmDialog.action) + "/" + "transaction",options);
				promise.then(
					onFinish,
					onError
				);
			});

		},
		
		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "transactionGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}


			var structure = [
				{	id: "xId", name: nls.monitoring.grid.transaction.xId, width: "350px", decorator: Utils.nameDecorator},
				{	id: "timestamp", name: nls.monitoring.grid.transaction.timestamp, width: "140px",  		
							formatter: function(cellData) {
								var value = cellData.timestamp;
								if (!value) {
									value = 0;
								}
								var date = new Date();
								if (value > 0 ) {
									date = new Date(value / 1000000);
								}
								return Utils.getFormattedDateTime(date);
							} 	
				},
				{	id: "stateString", name: nls.monitoring.grid.transaction.state, width: "140px", decorator: Utils.nameDecorator	}
			];
			
			this.grid = GridFactory.createGrid(gridId, "transactionGridContent", this.store, structure, { filter: true, paging: true, actionsMenuTitle: nls_strings.action.Actions });
			
			// if the user has permissions add actions drop down
			if (ism.user.hasPermission(Resources.actions.processTransactions)) {
				
				this.commitButton = new MenuItem({
					label: nls.monitoring.grid.commitTransactionButton
				});
				
				this.rollbackButton = new MenuItem({
					label: nls.monitoring.grid.rollbackTransactionButton
				});
				
				this.forgetButton = new MenuItem({
					label: nls.monitoring.grid.forgetTransactionButton
				});
				GridFactory.addToolbarMenuItem(this.grid, this.commitButton, nls_strings.action.Actions);
				this.commitButton.set('disabled', true);
				GridFactory.addToolbarMenuItem(this.grid, this.rollbackButton, nls_strings.action.Actions);
				this.rollbackButton.set('disabled', true);
				GridFactory.addToolbarMenuItem(this.grid, this.forgetButton, nls_strings.action.Actions);
				this.forgetButton.set('disabled', true);
			
			}
		},
		
		_getStateString: function(code) {
	
				var stateCode = "stateCode" + code;
				var stateString = nls.monitoring.grid.transaction[stateCode];
				if (!stateString) {
					stateString = "";
				} 
				return stateString;
				
		},
		
		_getSelectedData: function() {
			// summary:
			// 		get the row raw data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).rawData();
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
			
			if (this.commitButton) {
				this.commitButton.set('disabled', true);
			}
			if (this.rollbackButton) {
				this.rollbackButton.set('disabled', true);
			}
			if (this.forgetButton) {
				this.forgetButton.set('disabled', true);
			}
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
			this._updateStore();
		},

		_updateButtons: function() {
			// summary:
			//		update the action buttons based on the row selection
			console.log("selected: " + this.grid.select.row.getSelected());
			
			// if buttons not defined return
			if (!this.commitButton || !this.rollbackButton || !this.forgetButton) { return; }
			
			if (this.grid.select.row.getSelected().length <= 0) {
				// nothing selected disable
				this.commitButton.set('disabled', true);
				this.rollbackButton.set('disabled', true);
				this.forgetButton.set('disabled', true);
			} else {
				var transactionState = this._getSelectedData()["state"];
				if (transactionState == this.STATE_PREPARED) {
					this.commitButton.set('disabled', false);
					this.rollbackButton.set('disabled', false);
					this.forgetButton.set('disabled', true);
				} else if (transactionState = this.STATE_HEURISTICALLY_COMMITTED || 
						   transactionState == this.STATE_HEURISTICALLY_ROLLED_BACK) {
					this.forgetButton.set('disabled', false);
					this.commitButton.set('disabled', true);
					this.rollbackButton.set('disabled', true);
				}
			}
			
		},
		
		_updateDialogForAction: function(action) {
			// summary:
			//		update the action confirm dialog based on the action specified
			var title = nls.monitoring.actionConfirmDialog["title" + action];
			var content =  "<div>" + nls.monitoring.actionConfirmDialog["content" + action] + "</div>";
			this.actionConfirmDialog.set('content', content);
			this.actionConfirmDialog.set('title', title);	
			this.actionConfirmDialog.actionConfirmDialogButton.set("label", nls.monitoring.actionConfirmDialog["actionButton" + action]);
			
			this.actionConfirmDialog.set('failedMsg', nls.monitoring.actionConfirmDialog["failed" + action]);
			this.actionConfirmDialog.set("action", action.toLowerCase());
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			if (ism.user.hasPermission(Resources.actions.processTransactions)) {
				aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
					this._updateButtons();
				}));

				// Commit requested
				on(this.commitButton, "click", lang.hitch(this, function() {
					console.debug("clicked commit button");
					query(".idxDialogActionBarLead",this.actionConfirmDialog.domNode)[0].innerHTML = "";
					this.actionConfirmDialog.actionConfirmDialogButton.set('disabled', false);
					this._updateDialogForAction("Commit");
					this.actionConfirmDialog.show();
				}));
				
				// Rollback requested
				on(this.rollbackButton, "click", lang.hitch(this, function() {
					console.debug("clicked rollback button");
					query(".idxDialogActionBarLead",this.actionConfirmDialog.domNode)[0].innerHTML = "";
					this.actionConfirmDialog.actionConfirmDialogButton.set('disabled', false);
					this._updateDialogForAction("Rollback");
					this.actionConfirmDialog.show();
				}));
				
				// Forget requested
				on(this.forgetButton, "click", lang.hitch(this, function() {
					console.debug("clicked forget button");
					query(".idxDialogActionBarLead",this.actionConfirmDialog.domNode)[0].innerHTML = "";
					this.actionConfirmDialog.actionConfirmDialogButton.set('disabled', false);
					this._updateDialogForAction("Forget");
					this.actionConfirmDialog.show();
				}));
			}

			// confirmation was acknowledged - perform action
			dojo.subscribe(this.actionConfirmDialog.dialogId + "_saveButton", lang.hitch(this, function() {
				var bar = query(".idxDialogActionBarLead",this.actionConfirmDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.monitoring.deletingProgress;
				var transaction = this._getSelectedData();
				console.log("processing transcation " + transaction.xId);
				this.actionConfirmDialog.save(transaction.xId, lang.hitch(this, function(data) {
						this.updateGrid();
						this.actionConfirmDialog.hide();					
						bar.innerHTML = "";						
					}
				));
			})); 
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

});
