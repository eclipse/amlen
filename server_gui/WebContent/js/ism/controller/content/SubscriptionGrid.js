/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
		'dojo/on',
		'dojo/promise/all',
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
		'dojo/i18n!ism/nls/messaging',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/widgets/Select',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/SubscriptionGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'idx/string',
		'ism/controller/content/MessagingPolicyDialog',
		'dojo/text!ism/controller/content/templates/SubscriptionDeleteResults.html'
		], function(declare, lang, array, domClass, domConstruct, domStyle, aspect, query, on, all, locale, when, number, Memory, ItemFileWriteStore, manager, ContentPane, MenuItem, Button, 
				Form, DropDownButton, Menu, nls, nls_strings, nls_messaging, GridFactory, Dialog, Select, _TemplatedContent, gridtemplate, Resources, Utils, MonitoringUtils, iString,
				MessagingPolicyDialog, subscriptionDeleteResults) {

	var SubscriptionDeleteResults = declare("ism.controller.content.SubscriptionDeleteResults", _TemplatedContent, {
		
		MAX_AUTO_HEIGHT_ROWS: 5,
		VSCROLLER_HEIGHT: "210px",
		
		nls: nls,
		templateString: subscriptionDeleteResults,
		pendingStore: null,
		failureStore: null,
		pendingGrid: null,
		failureGrid: null,		
		pendingContentId: "sg_pendingContent",
		failureContentId: "sg_failureContent",
		dialogId: "sg_subscriptionDeleteResults",
		Utils: Utils,
		deleteResultsTitle: Utils.escapeApostrophe(nls.monitoring.deleteResultsDialog.title),
		
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
	 				{   id: "name", name: nls.monitoring.grid.subscription.subsName, tooltip: nls.monitoring.grid.subscription.subsNameTooltip, width: "150px",
	 					decorator: function(value) {
	 						return (value.length > 0) ? Utils.nameDecorator(value) : nls.monitoring.grid.subscription.noName;
	 					}
	 				},
	 				{	id: "clientId", name: nls.monitoring.grid.subscription.clientId, tooltip: nls.monitoring.grid.subscription.clientIdTooltip, width: "150px", decorator: Utils.nameDecorator }
 				];
			
			this.pendingGrid = GridFactory.createGrid("pendingDeleteGridContent", this.pendingContentId, this.pendingStore, pendingStructure, 
									{ filter: false, paging: false, suppressSelection: true, height: this.VSCROLLER_HEIGHT, vscroller: true});						
		},

		_initFailureGrid: function() {
			var failureStructure = [
	 				{	id: "name", name: nls.monitoring.grid.subscription.subsName, tooltip: nls.monitoring.grid.subscription.subsNameTooltip, width: "150px",
	 					decorator: function(value) {
	 						return (value.length > 0) ? Utils.nameDecorator(value) : nls.monitoring.grid.subscription.noName;
	 					}
	 				},
	 				{	id: "clientId", name: nls.monitoring.grid.subscription.clientId, tooltip: nls.monitoring.grid.subscription.clientIdTooltip, width: "150px", decorator: Utils.nameDecorator },
	 				{	id: "errorMessage", name: nls.monitoring.grid.subscription.errorMessage, tooltip: nls.monitoring.grid.subscription.errorMessageTooltip, width: "250px", decorator: Utils.textDecorator}
 				];
			
			this.failureGrid = GridFactory.createGrid("failureDeleteGridContent", this.failureContentId, this.failureStore, failureStructure, 
					{ filter: false, paging: false, suppressSelection: true, height: this.VSCROLLER_HEIGHT, vscroller: true});	
			this.failureGrid.startup();
		},

		show: function(failure, pending) {
			
			failure = failure ? failure : [];
			pending = pending ? pending : [];
						
			this.reset();
			
			var showFailure = failure.length > 0;
			var showPending = pending.length > 0;
			var showSuccess = !showFailure /*&& !showPending*/;
			
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
	
	var SubscriptionGrid = declare("ism.controller.content.SubscriptionGrid", _TemplatedContent, {

		nls: nls,
		templateString: gridtemplate,
		
		removeDialog: null,

		timeOffset: null,

		grid: null,
		store: null,

		deleteButton: null,

		restURI: "rest/config/subscriptions",
		
		domainUid: 0,

		contentId: "subscriptionGridContent",

		timeoutID: null,
		
		deleteBatchSize: 10,
		
		_expDiscColumnType: "expDisc", // by default we show Expired+Discarded Messages in the column
		
		_expDiscColumnInfo: {
			expDisc: {
				// Expired / Discarded
				name: nls.monitoring.grid.subscription.expDiscMsg, 
				tooltip: nls.monitoring.grid.subscription.expDiscMsgTooltip
			},
			exp: {
				// Expired (when a query for most/least expired messages is issued)
				name: nls.monitoring.grid.subscription.expMsg, 
				tooltip: nls.monitoring.grid.subscription.expMsgTooltip				
			},
			disc: {
				// Discarded (when a query for most/least discarded messages is issued)
				name: nls.monitoring.grid.subscription.DiscMsg, 
				tooltip: nls.monitoring.grid.subscription.DiscMsgTooltip				
			}
		},
	
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			           
            this.domainUid = ism.user.getDomainUid();
            
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;
			

			// if we can't delete durable subscriptions, replace the tagline on page
			if (!ism.user.hasPermission(Resources.actions.deleteDurableSubscription)) {
				dojo.byId("sm_tagline").innerHTML = nls.monitoring.subscriptionMonitor.taglineNoDelete;
			} 

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
			
			// get protocols for viewing/editing messaging policy
			var adminEndpoint = ism.user.getAdminEndpoint();
			this.protocolDeferred = adminEndpoint.doRequest("/configuration/Protocol", adminEndpoint.getDefaultGetOptions());
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.subscriptionMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));
			
			this._setupFields();
			this.createGrid();
			this._updateStore();
			this._initDialogs();
			this._initEvents();
		},
		
		_getExpDiscColumnType: function() {
			var ds = this.datasetCombo.value;
			var result = "expDisc";
			if (iString.startsWith(ds, "Expired")) {
				result = "exp";
			} else if (iString.startsWith(ds, "Discarded")) {
				result = "disc";
			} 
			return result;
		},

		_updateStore: function() {
			dojo.style(dojo.byId('sg_refreshingLabel'), "display", "inline");
			var type = this._getExpDiscColumnType();
			
			var args = {
					SubType: this.subscriptionTypeCombo.value,
					StatType: this.datasetCombo.value,
					ResultCount: 100
				};
			if (this.subscription.value !== "") {
				args.SubName = this.subscription.value;
			}
			if (this.topicString.value !== "") {
				args.TopicString = this.topicString.value;
			}
			if (this.clientId.value !== "") {
				args.ClientID = this.clientId.value;
			}
			if (this.messagingPolicy.value !== "") {
				args.MessagingPolicy = this.messagingPolicy.value;
			}
			
			var deferred = MonitoringUtils.getSubscriptionData(args);
			
			deferred.then(lang.hitch(this, function(data) {
				// Add 'id' field and throughput
				var mapped = array.map(data.Subscription, function(item, index){
					var record = {};
					record.id = 1+index;
					record.subsName = item.SubName;
					record.topicString = item.TopicString;
					record.clientId = item.ClientID;
					record.isDurable = item.IsDurable;
					record.isShared = item.IsShared;
					record.consumers = number.parse(item.Consumers);
					record.msgPublished = number.parse(item.PublishedMsgs);
					record.msgBuf = number.parse(item.BufferedMsgs);
					record.perMsgBuf = parseFloat(item.BufferedPercent);					
					record.msgBufHWM = number.parse(item.BufferedMsgsHWM);
					record.msgBufHWMPercent = parseFloat(item.BufferedHWMPercent);
					record.maxMsgBuf = number.parse(item.MaxMessages);
					record.rejMsg = number.parse(item.RejectedMsgs);
					record.expMsg = number.parse(item.ExpiredMsgs) | 0;
					record.discMsg = number.parse(item.DiscardedMsgs) | 0;
					if (type === "exp") {
						record.expDiscMsg = record.expMsg;
					} else if (type === "disc") {
						record.expDiscMsg = record.discMsg;						
					} else {
						record.expDiscMsg = record.expMsg + record.discMsg;
					}
					record.messagingPolicy = {
							name: item.MessagingPolicy,
							policyType: "Topic"
					};
					if (record.clientId === "__Shared" && record.isShared === true && record.isDurable === true) {
						record.messagingPolicy.policyType = "Subscription";
					}
					
					return record;
				});
				
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
				this._updateExpDiscType(type);
				this.triggerListeners();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('sg_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(error) {
				console.debug("Error from getSubscriptionData: "+error);
				dojo.style(dojo.byId('sg_refreshingLabel'), "display", "none");
				
				MonitoringUtils.showMonitoringPageError(error);
				return;
			});
		},
		
		_setupFields: function() {
			console.log(this.declaredClass + ".setupFields");
			
			// Populate datasets combo
			for (var key in MonitoringUtils.Subscription.View) {
				this.datasetCombo.store.newItem({ 
					label: MonitoringUtils.Subscription.View[key].label, 
					name: MonitoringUtils.Subscription.View[key].name 
				});
			}
			this.datasetCombo.store.save();

			// Populate subscription type combo
			for (var type in MonitoringUtils.Subscription.Type) {
				this.subscriptionTypeCombo.store.newItem({ 
					label: MonitoringUtils.Subscription.Type[type].label, 
					name: MonitoringUtils.Subscription.Type[type].name 
				});
			}
			this.subscriptionTypeCombo.store.save();
		},

		_initDialogs: function() {

			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeSubscriptionDialog'></div>", this.contentId);
			var dialogId = "removeSubscriptionDialog";
			var title = Utils.escapeApostrophe(nls.monitoring.dialog.removeSubscriptionTitle);
			var removeDialogButton = new Button({
				id: dialogId + "_button_ok",
				label: Utils.escapeApostrophe(nls.monitoring.removeDialog.deleteButton),
				onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
			});
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + Utils.escapeApostrophe(nls.monitoring.dialog.removeSubscriptionContent) + "</div>",
				buttons: [ removeDialogButton ],
				closeButtonLabel: nls.monitoring.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.removeDialogButton = removeDialogButton;
			this.removeDialog.save = lang.hitch(this, function(subscriptions, onFinish, onError) { 
				if (!onFinish) { onFinish = {};}
				if (!onError) { onError = lang.hitch(this,function(error) {	
					this.removeDialog.showXhrError(nls.monitoring.deletingFailed, error);
					this.removeDialog.removeDialogButton.set('disabled', true);
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				});}
				var grid = this.grid;
				var mappedData = array.map(subscriptions, function(row) {
					var subscription = grid.row(row).data();
					return { ClientID: subscription.clientId, SubscriptionName: subscription.subsName };
				});
				var failed = [];
				var pending = [];
				var success = [];
				for (var idx=0; idx < mappedData.length; idx++) {
					pending.push({id: pending.length, name: mappedData[idx].SubscriptionName, clientId: mappedData[idx].ClientID});
				}
				this._processDeleteRequests(mappedData, onFinish, success, failed, pending);
			});

		},
		
		// update the expDisc grid column for the current type of query
		_updateExpDiscType: function(type) {
			if (type === this._expDiscColumnType) {
				return;
			}
			this._expDiscColumnType = type;
			// set the name and tooltip of the column
			var col = this.grid.column("expDiscMsg");
			if (col) {
				col.setName(this._expDiscColumnInfo[type].name);
			}
			var td = this.grid.header.getHeaderNode("expDiscMsg");
			if (td) {
				td.setAttribute('title', this._expDiscColumnInfo[type].tooltip);
				// setting the column name doesn't update the caption, so do it ourselves
				var caption = query(".gridxColCaption", td)[0];
				if (caption) {
					caption.innerHTML = this._expDiscColumnInfo[type].name;
				}
			}
		},
		
		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "subscriptionGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}
			
			var _t = this;

			var structure = [
				{	id: "topicString", name: nls.monitoring.grid.subscription.topicString, tooltip: nls.monitoring.grid.subscription.topicStringTooltip, width: "100px", decorator: Utils.nameDecorator },
				// Subscription Properties Group
				{	id: "subsName", name: nls.monitoring.grid.subscription.subsName, tooltip: nls.monitoring.grid.subscription.subsNameTooltip, width: "100px",
					decorator: function(value) {
						return (value.length > 0) ? Utils.nameDecorator(value) : nls.monitoring.grid.subscription.noName;
					}
				},
				{	id: "clientId", name: nls.monitoring.grid.subscription.clientId, tooltip: nls.monitoring.grid.subscription.clientIdTooltip, width: "100px", decorator: Utils.nameDecorator },
				{	id: "isDurable", name: nls.monitoring.grid.subscription.subsType, tooltip: nls.monitoring.grid.subscription.subsTypeTooltip, width: "80px",
					formatter: function(cellData) {
						var subTypeString = (cellData.isDurable === true) ? nls.monitoring.grid.subscription.isDurable : nls.monitoring.grid.subscription.notDurable;
						if (cellData.isShared === true) {
							subTypeString = subTypeString + " " + nls.monitoring.grid.subscription.isShared;
						}
						return subTypeString;
					}
				},
				{	id: "consumers", name: nls.monitoring.grid.subscription.consumers, tooltip: nls.monitoring.grid.subscription.consumersTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "messagingPolicy", name: nls.monitoring.grid.subscription.messagingPolicy, tooltip: nls.monitoring.grid.subscription.messagingPolicyTooltip, width: "120px", 
					widgetsInCell: true,
					navigable: true,
					decorator: function(){
						//Generate cell widget template string
						return '<a href="javascript:;" data-dojo-attach-point="messagingPolicyLink"></a>';
					},
					setCellValue: function(data){
						//"this" is the cell widget
						if (data === undefined) {
							this.messagingPolicyLink.title = "";
							this.messagingPolicyLink.onclick = undefined;
							this.messagingPolicyLink.innerHTML = "";
						} else {						
							this.messagingPolicyLink.title = data.name;
							this.messagingPolicyLink.policyType = data.policyType;
							this.messagingPolicyLink.onclick = function(event) {
					            // work around for IE event issue - stop default event
					            if (event && event.preventDefault) event.preventDefault();

							    dojo.publish("ismOpenMessagingPolicy", this);
								return false;
							};
							this.messagingPolicyLink.innerHTML = Utils.nameDecorator(data.name);
						}
					}
				},
				// Total Messages Group
				{	id: "msgPublished", name: nls.monitoring.grid.subscription.msgPublished, tooltip: nls.monitoring.grid.subscription.msgPublishedTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "rejMsg", name: nls.monitoring.grid.subscription.rejMsg, tooltip: nls.monitoring.grid.subscription.rejMsgTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "expDiscMsg", name: nls.monitoring.grid.subscription.expDiscMsg, tooltip: nls.monitoring.grid.subscription.expDiscMsgTooltip, width: "80px", 
					decorator: function(value, id) {
						if (value === undefined) {
							return "";
						}
						var title = "";
						if (_t._expDiscColumnType === "expDisc") {							
							var rowData = _t.store.query({id: id})[0];
							if (rowData) {
								var args = [number.format(rowData.expMsg, {places: 0, locale: ism.user.localeInfo.localeString}),
								            number.format(rowData.discMsg, {places: 0, locale: ism.user.localeInfo.localeString})];
								title = lang.replace(nls.monitoring.grid.subscription.expDiscMsgCellTitle, args);
							}
						}
						return "<div title='"+title+"'style='float: right'>"+number.format(value, {places: 0, locale: ism.user.localeInfo.localeString})+"</div>";
					}
				},
				// Buffered Messages Group
				{	id: "maxMsgBuf", name: nls.monitoring.grid.subscription.maxMsgBuf, tooltip: nls.monitoring.grid.subscription.maxMsgBufTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "msgBuf", name: nls.monitoring.grid.subscription.msgBuf, tooltip: nls.monitoring.grid.subscription.msgBufTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "perMsgBuf", name: nls.monitoring.grid.subscription.perMsgBuf, tooltip: nls.monitoring.grid.subscription.perMsgBufTooltip, width: "80px", 					
					decorator: function(cellData) {
						return "<div style='float: right'>"+number.format(cellData, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
					}
				}, 
				{	id: "msgBufHWM", name: nls.monitoring.grid.subscription.msgBufHWM, tooltip: nls.monitoring.grid.subscription.msgBufHWMTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "msgBufHWMPercent", name: nls.monitoring.grid.subscription.msgBufHWMPercent, tooltip: nls.monitoring.grid.subscription.msgBufHWMPercentTooltip, width: "80px", 					
					decorator: function(cellData) {
						return "<div style='float: right'>"+number.format(cellData, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
					}
				}								
			];
			var headerGroups = [			                
						    1,    // first column not in a group    
						    {name: nls.monitoring.grid.subscriptionProperties, children: 5, id: "subscriptionProperties"},
						    {name: nls.monitoring.grid.totalMessages, children: 3, id: "totalMessages"},
						    {name: nls.monitoring.grid.bufferedMessages, children: 5, id: "bufferedMessages"}			    
						];
			
			var headerGroupMap = {
					topicString: 0,
					subsName: "subscriptionProperties", clientId: "subscriptionProperties", isDurable: "subscriptionProperties", consumers: "subscriptionProperties", messagingPolicy: "subscriptionProperties",
					msgPublished: "totalMessages", rejMsg: "totalMessages", expDiscMsg: "totalMessages",
					maxMsgBuf: "bufferedMessages", msgBuf: "bufferedMessages", perMsgBuf: "bufferedMessages", msgBufHWM: "bufferedMessages", msgBufHWMPercent: "bufferedMessages"
				};

			this.grid = GridFactory.createGrid(gridId, "subscriptionGridContent", this.store, structure, 
					{ filter: true, paging: true, headerGroups: headerGroups, actionsMenuTitle: nls_strings.action.Actions,
					  heightTrigger: 0, headerGroupMap: headerGroupMap, extendedSelection: true, multiple: true});

			if (ism.user.hasPermission(Resources.actions.deleteDurableSubscription)) {
				this.deleteButton = new MenuItem({
					label: nls.monitoring.grid.deleteSubscriptionButton
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
			if (this.sg_form.validate()) {
				this._updateStore();
			}
		},

		_updateButtons: function() {
			console.log("selected: "+this.grid.select.row.getSelected());
			if (!this.deleteButton) { return; }
			
			var selections = this.grid.select.row.getSelected(); 
			var len = selections.length;			
			if (len < 1) {
				// nothing selected disable
				this.deleteButton.set('disabled', true);
			} else {
				// if only durable are selected, enable, otherwise disable
				var nonDurable = false;
				for (var si = 0; si < len; si++) {
					var subscription = this.store.query({id: selections[si]})[0]; 
					if (subscription.isDurable !== true) {
						nonDurable = true;
						break;
					}
				}
				this.deleteButton.set('disabled', nonDurable);
			}
			
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			if (ism.user.hasPermission(Resources.actions.deleteDurableSubscription)) {
				aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
					console.debug("after onRowClick");
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
						var message = nls.monitoring.dialog.removeSubscriptionContent;
						var title = nls.monitoring.dialog.removeSubscriptionTitle;
						if (selected.length > 1) {
							message = Utils.escapeApostrophe(lang.replace(nls.monitoring.dialog.removeSubscriptionsContent, [selected.length]));
							title = Utils.escapeApostrophe(nls.monitoring.dialog.removeSubscriptionsTitle);
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
							this.deleteResultsDialog = new SubscriptionDeleteResults({
								id: deleteResultsDialogId,
								title: Utils.escapeApostrophe(nls.monitoring.deleteResultsDialog.title),
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
			
			dojo.subscribe("ismOpenMessagingPolicy", lang.hitch(this, function(messagingPolicy) {
				// open the messaging policy for edit or view, depending on the user's authority
				this._openMessagingPolicy(messagingPolicy.title, messagingPolicy.policyType);
			}));
		},
		
		_openMessagingPolicy: function(messagingPolicyName, policyType) {
			if (!messagingPolicyName) {
				console.debug("_openMessagingPolicy called with no name");
				return;
			}
			
			if (!policyType) {
				console.debug("_openMessagingPolicy called with no messaging policy type");
				return;
			}

            var adminEndpoint = ism.user.getAdminEndpoint();
            var messagingPolicyDeferred;
            if (policyType === "Topic") {
            	messagingPolicyDeferred = adminEndpoint.doRequest("/configuration/TopicPolicy/"+encodeURIComponent(messagingPolicyName), adminEndpoint.getDefaultGetOptions());	
            } else {
            	messagingPolicyDeferred = adminEndpoint.doRequest("/configuration/SubscriptionPolicy/"+encodeURIComponent(messagingPolicyName), adminEndpoint.getDefaultGetOptions());
            }
            
			
			if (this.viewOnly === undefined) {
				this.viewOnly = !ism.user.hasPermission(Resources.actions.editMessagingPolicy);
			}
			
			if (policyType === "Topic") {
				if (!this.topicPolicyDialog) {
					var topicPolicyId =  this.id+"_TopicPolicyDialog";
					domConstruct.place("<div id='"+topicPolicyId+"'></div>", this.contentId);
					this.topicPolicyDialog = new MessagingPolicyDialog({dialogId: topicPolicyId,
						      type: policyType,
						      destinationLabel: nls_messaging["policyTypeName_" + policyType.toLowerCase()],
						      destinationTooltip: nls_messaging["policyTypeTooltip_" + policyType.toLowerCase()],
						      destinationLabelWidth: "131px",
						      actionLabelWidth: "120px",
							  dialogTitle: Utils.escapeApostrophe(this.viewOnly ? nls_messaging.viewTopicMPTitle : nls_messaging.editTopicMPTitle),
							  dialogInstruction: Utils.escapeApostrophe(nls_messaging.topicMPInstruction),
							  protocolDeferrred: this.protocolDeferred, 
							  add: false, viewOnly: this.viewOnly}, topicPolicyId);
					this.topicPolicyDialog.startup();
					
					if (!this.viewOnly) {
	                    aspect.after(this.topicPolicyDialog.dialog, "onHide", lang.hitch(this, function() {
	                        if (this.refreshNeeded) {
	                            this.updateGrid();
	                        }
	                        this.refreshNeeded = false;
	                    }));
					    
						dojo.subscribe(this.topicPolicyDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
							// any message on this topic means that we confirmed the operation
							if (dijit.byId(this.topicPolicyDialog.dialogId+"_DialogForm").validate()) {
								this.topicPolicyDialog.save(lang.hitch(this, function(data) {
									console.debug("save is done, returned:", data);
									this.refreshNeeded = true;
									this.topicPolicyDialog.dialog.hide();
								}));
							}
						}));
					}
				}
			} else {
				if (!this.subscriptionPolicyDialog) {
					var subscriptionPolicyId =  this.id+"_SubscriptionPolicyDialog";
					domConstruct.place("<div id='"+subscriptionPolicyId+"'></div>", this.contentId);
					this.subscriptionPolicyDialog = new MessagingPolicyDialog({dialogId: subscriptionPolicyId, 
					          type: policyType,
					          destinationLabel: nls_messaging["policyTypeName_" + policyType.toLowerCase()],
					          destinationTooltip: nls_messaging["policyTypeTooltip_" + policyType.toLowerCase()],
					          destinationLabelWidth: "131px",
					          actionLabelWidth: "120px",
							  dialogTitle: Utils.escapeApostrophe(this.viewOnly ? nls_messaging.viewSubscriptionMPTitle : nls_messaging.editSubscriptionMPTitle),
							  dialogInstruction: Utils.escapeApostrophe(nls_messaging.subscriptionMPInstruction),
							  protocolDeferrred: this.protocolDeferred, 
							  add: false, viewOnly: this.viewOnly}, subscriptionPolicyId);
					this.subscriptionPolicyDialog.startup();
					
					if (!this.viewOnly) {
	                    aspect.after(this.subscriptionPolicyDialog.dialog, "onHide", lang.hitch(this, function() {
	                        if (this.refreshNeeded) {
	                            this.updateGrid();
	                        }
	                        this.refreshNeeded = false;
	                    }));
					    
						dojo.subscribe(this.subscriptionPolicyDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
							// any message on this topic means that we confirmed the operation
							if (dijit.byId(this.subscriptionPolicyDialog.dialogId+"_DialogForm").validate()) {
								this.subscriptionPolicyDialog.save(lang.hitch(this, function(data) {
									console.debug("save is done, returned:", data);
									this.refreshNeeded = true;
									this.subscriptionPolicyDialog.dialog.hide();
								}));
							}
						}));
					}
				}				
			}
			// TODO when we support editing a name, we'll have to pass in the list of existing names
			messagingPolicyDeferred.then(lang.hitch(this,function(data) {
				if (policyType === "Topic") {
					this.topicPolicyDialog.show(adminEndpoint.getNamedObject(data, "TopicPolicy", messagingPolicyName), []);
				} else {
					this.subscriptionPolicyDialog.show(adminEndpoint.getNamedObject(data, "SubscriptionPolicy", messagingPolicyName), []);
				}
			}), function(error) {
                Utils.showPageErrorFromPromise(nls_messaging.messaging.messagingPolicies.retrieveOneMessagingPolicyError, error);
			});
		},
		
		handleResolved: function(response, args /* subscription, subscriptions, mappedData, onFinish, success, failed, pending */) {
			var t = this;
			var subscription = args.subscription;
			var subscriptions = args.subscriptions;
			var mappedData = args.mappedData;
			var onFinish = args.onFinish;
			var success = args.success;
			var failed = args.failed;
			var pending = args.pending;
			
			for(var i = 0; i < subscriptions.length; i++) {
				if (subscriptions[i].name == subscription.name && subscriptions[i].clientId == subscription.clientId) {
					console.debug("resolved: " + subscription.name + " for client ID " + subscription.clientId + " " + JSON.stringify(response));
					
					// Update array of successful requests
					success.push({name: subscription.name, clientId: subscription.clientId, code: response.Code, message: response.Message});
					
					// Remove the subscription from the array of pending requests
					for(var j = 0; j < pending.length; j++) {
						if (pending[j].name === subscription.name && pending[j].clientId === subscription.clientId) {
							pending.splice(j, 1);
							break;
						}
					}

					// Check to see if we have finished processing the current batch.  If so, cancel
					// the batch timeout.  Next, if not all subscriptions have been deleted, make a 
					// recursive call to _processDeleteRequests in order to process the next batch.
					// Otherwise, call onFinish.
					subscriptions.splice(i, 1);
					if (subscriptions.length === 0) {
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
		
		handleRejected: function(response, args /*subscription, subscriptions, mappedData, onFinish, success, failed, pending */) {
			var t = this;
			var subscription = args.subscription;
			var subscriptions = args.subscriptions;
			var mappedData = args.mappedData;
			var onFinish = args.onFinish;
			var success = args.success;
			var failed = args.failed;
			var pending = args.pending;

			for(var i = 0; i < subscriptions.length; i++) {
				if (subscriptions[i].name === subscription.name && subscriptions[i].clientId === subscription.clientId) {
					console.debug("rejected: " + subscription.name + " for client ID " + subscription.clientId + " " + JSON.stringify(response));
					
					// Add the subscription to the array of failed requests
					var item = {id: failed.length, name: subscription.name, clientId: subscription.clientId,  errorMessage: response.response.data.Code + ": " + response.response.data.Message};
					failed.push(item);
					
					// Remvoe the subscription from the array of pending requests
					for(var j = 0; j < pending.length; j++) {
						if (pending[j].name === subscription.name && pending[j].clientId === subscription.clientId) {
							pending.splice(j, 1);
							break;
						}
					}

					// Check to see if we have finished processing the current batch.  If so, cancel
					// the batch timeout.  Next, if there are additional subscriptions to delete, make 
					// a recursive call to _processDeleteRequests in order to process the next batch.
					// Otherwise, call onFinish.
					subscriptions.splice(i, 1);
					if (subscriptions.length === 0) {
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
			var subscriptions = [];
			
			// When we enter this method, we are ready to process a new batch of delete requests.
			// If there's a pending timeout from an earlier batch, cancel it now.
			if (this.timeoutID) {
				 window.clearTimeout(this.timeoutID);
			}

			for (var idx=0; idx < batchSize; idx++) {
				subscriptions[idx] = { name: mappedData[idx].SubscriptionName, clientId: mappedData[idx].ClientID };
			}
			mappedData.splice(0, batchSize);
			
			for (idx=0; idx < batchSize; idx++) {
				var args = {subscription: subscriptions[idx], subscriptions: subscriptions, mappedData: mappedData, onFinish: onFinish, success: success, failed: failed, pending: pending};
				var deleteReq = { subscription: subscriptions[idx], promise: MonitoringUtils.deleteSubscription(subscriptions[idx].name, subscriptions[idx].clientId).trace({t: t, args: args}) };
				delReqs.push(deleteReq);
			}
				
			// Allow up to deleteBatchSize seconds for deletes in the current batch to complete.
			// If there are additional IDs delete, make a recursive 
			// call to _processDeleteRequests in order to process the next batch.
			// Otherwise, call onFinish.
			var deleteTimeout = this.deleteBatchSize * 1000;
			this.timeoutID = window.setTimeout(lang.hitch(this, function(){
				if(subscriptions.length !== 0) {
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
	return SubscriptionGrid;
});
