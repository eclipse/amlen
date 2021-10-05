/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
		'dojo/date/locale',
		'dojo/dom-attr',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/aspect',
		'dojo/on',
		'dojo/number',
		'dojo/query',
		'dojo/store/Memory',
		'dojo/data/ItemFileWriteStore',
		'dijit/_base/manager',
		'dijit/layout/ContentPane',
		'dijit/form/Button',
		'dojo/i18n!ism/nls/home',
		'dojo/i18n!ism/nls/clusterStatistics',
		'ism/widgets/Select',
		'ism/widgets/GridFactory',
		'ism/widgets/_TemplatedContent',
		'ism/widgets/ToggleContainer',
		'dojo/text!ism/controller/content/templates/ClusterStatsGrid.html',
		'ism/config/Resources',
		'ism/widgets/ClusterStatsAssociationControl',
		'ism/widgets/Dialog',
		'ism/widgets/ServerControl',
		'ism/IsmUtils',
		'ism/widgets/IsmQueryEngine',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/messaging'
		], function(declare, lang, locale, domAttr, domClass, domConstruct, aspect, on, number, query, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				nlsHome, nls, Select, GridFactory, _TemplatedContent, ToggleContainer, clusterstatsgrid, Resources, ClusterStatsAssociationControl, Dialog,
				ServerControl, Utils, IsmQueryEngine, nls_strings, nls_messaging) {

	return declare("ism.controller.content.ClusterStatsGrid", _TemplatedContent, {

		nlsHome: nlsHome,
		nls: nls,
		templateString: clusterstatsgrid,
		
		statusUpdateInterval: 5000, // milliseconds
		initialUpdateInterval: 5, // milliseconds
				
		grid: null,
		store: null,
		channelStore: null,
		
		currentSize: {Server: "100px", 
			          Status: "80px", 
			          Health: "50px", 
			          HAStatus: "100px", 
			          StatusTime: "120px", 
			          ReadMsgRate: "100px", 
			          MsgSendRate: "100px", 
			          Buffered: "100px", 
			          Discarded: "100px"}, 

		structureMsg: null,
		resizePending: false,
		
		initialized: false,
		epListRefreshing: false,
		queryEngine: IsmQueryEngine,
		addServerDialog: undefined,
		serverStore: undefined,
		addServerName: undefined,
		removeDialog: undefined,
		removeNotAllowedDialog: undefined,
		pauseUpdates: false,
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[], idProperty: "id", queryEngine: this.queryEngine});
			this.channelStore = new Memory({data:[], idProperty: "id", queryEngine: this.queryEngine});
			this._initStructures();
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			// Disable the update button while grid is being updated
			this.resumeupdButton.set('disabled', true);
			this.updateInterval.innerHTML = nls.cacheInterval;

			this._initGrid();
			this._initDialogs();
			this._initEvents();
			
			// listen for a column resize event
			aspect.after(this.grid.columnResizer, "onResize", lang.hitch(this, function(colId, newWidth, oldWidth) {
				// currently all data is in the variable newWidth...
				this.currentSize[newWidth[0]] = newWidth[1] + "px";
				this.resizePending = true;
			}));
			
	         // subscribe to server status changes
            dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
                // Cluster information in status:  ClusterState, ClusterName, ServerName, ConnectedServers, DisconnectedServers
                if (!data) {
                    return;
                }
                this.updateStatus(data);
            }));
            
	         // subscribe to cluster status changes
            dojo.subscribe(Resources.serverStatus.clusterStatusChangedTopic, lang.hitch(this, function(cldata) {
                this.updateGrid(cldata);
            }));

			this._initStatus();
		},

		_initStructures: function() {
			var _this = this;
			this.structureMsg = [];
			this.structureMsg.push({ id: "Server", name: nls.serverNameLabel, tooltip:nls.serverNameTooltip, field: "ServerName", width: this.currentSize.ServerName,
				widgetsInCell: true,
				//decorator: Utils.nameDecorator
				decorator: function() {
					return "<a class='actionLink' id='serverLink'" + 
					"href='javascript:;'  data-dojo-attach-point='link' ></a>";
				},

				setCellValue: function(gridData, storeData, cellWidget) {
					// keep track of the value from the store
					cellWidget.ServerName = gridData;
					
					// get a reference to the domNode of the cell
					var cellNode = cellWidget.cell.grid.body.getCellNode({
						rowId: cellWidget.cell.row.id,
						colId: cellWidget.cell.column.id
					});
					
					cellWidget.link.innerHTML = gridData;
					var server = ism.user.getServerByName(gridData);
					if (server && server.length > 0) {
						cellWidget.link.setAttribute('href', _this._getUri(server[0]));
					}
					
					var text = gridData;
			    	// set the aria-label to the correct value
			    	domAttr.set(cellNode, "aria-label", text);
			    	domAttr.set(cellNode, "title", text);
			    },
				getCellWidgetConnects: function(cellWidget, cell){
					return [
					      [cellWidget.link, 'onclick', function(e){
					        _this.goToServer(cell.data());
					      }]
					 ];
				}
			});
			this.structureMsg.push({ id: "Status", name: nls.connStateLabel, tooltip:nls.connStateTooltip, width: this.currentSize.Status,
			    widgetsInCell: true,
			    setCellValue: function(gridData, storeData, cellWidget){
			    	
			    	// keep track of the value from the store
			    	cellWidget.Status = gridData;
			    	
			    	// get a reference to the domNode of the cell
			    	var cellNode = cellWidget.cell.grid.body.getCellNode({
						rowId: cellWidget.cell.row.id, 
						colId: cellWidget.cell.column.id
					});
			    	
			    	var text = nls.statusDown;					    	
					if (gridData.toLowerCase() == "active") {
						text = nls.statusUp;
					} else if (gridData.toLowerCase() == "connecting") {
						text = nls.statusConnecting;
					} 

			    	// set the aria-label to the correct value
			    	domAttr.set(cellNode, "aria-label", text);
			    	domAttr.set(cellNode, "title", text);
			    },
			    onCellWidgetCreated: function(cellWidget, column){
			    	// when the cell is rendered create a domNode for the icon
			    	cellWidget.StatusIconNode = domConstruct.toDom("<div></div>");
			        domConstruct.place(cellWidget.StatusIconNode, cellWidget.domNode);
			    },
			    initializeCellWidget: function(cellWidget, cell) {		    	
			    	// set the appropriate icon for the widget
			    	var cellData = cellWidget.Status;
			    	
			    	var statusIcon = _this._getStatusIcon(cellData);
			    	domClass.replace(cellWidget.StatusIconNode, statusIcon);
			    },
				formatter: lang.hitch(this, function(data) {
					// Add the OpState to the AdminState
					return data.Status;
				})
			});

			this.structureMsg.push({ id: "Health", name: nls.healthLabel, tooltip:nls.healthTooltip, field: "HealthName", width: this.currentSize.Health,
			    widgetsInCell: true,
			    setCellValue: function(gridData, storeData, cellWidget){
			    	
			    	// keep track of the value from the store
			    	cellWidget.HealthName = gridData;
			    	
			    	// get a reference to the domNode of the cell
			    	var cellNode = cellWidget.cell.grid.body.getCellNode({
						rowId: cellWidget.cell.row.id, 
						colId: cellWidget.cell.column.id
					});

			    	// set the aria-label to the correct value
			    	domAttr.set(cellNode, "aria-label", gridData);
			    	domAttr.set(cellNode, "title", gridData);
			    },
			    onCellWidgetCreated: function(cellWidget, column){
			    	// when the cell is rendered create a domNode for the icon
			    	cellWidget.HealthIconNode = domConstruct.toDom("<div></div>");
			        domConstruct.place(cellWidget.HealthIconNode, cellWidget.domNode);
			    },
			    initializeCellWidget: function(cellWidget, cell) {		    	
			    	// set the appropriate icon for the widget
			    	var cellData = cellWidget.HealthName;

					if (cellData == nls.healthGood) {
						domClass.replace(cellWidget.HealthIconNode, "ismIconSuccess");
					} else if (cellData == nls.healthWarning) {
						domClass.replace(cellWidget.HealthIconNode, "ismIconWarning");
					} else if (cellData == nls.healthBad) {
						domClass.replace(cellWidget.HealthIconNode, "ismIconError");
					} else {
						domClass.replace(cellWidget.HealthIconNode, "ismIconUnknown");
					}
			    },
				formatter: lang.hitch(this, function(data) {
					// Add the OpState to the AdminState
					return data.HealthName;
				})
			});

			this.structureMsg.push({ id: "HAStatus", name: nls.haStatusLabel, tooltip:nls.haStatusTooltip, field: "HAStatName", width: this.currentSize.HAStatus});
			
			this.structureMsg.push({ id: "StatusTime", name: nls.updTimeLabel, tooltip:nls.updTimeTooltip, width: this.currentSize.StatusTime,
				decorator: lang.hitch(this, function(cellData) {
					var statusDate = dojo.date.add(new Date(cellData), "second", 0);
					// convert from ISO format to make date more readable
					return locale.format(statusDate, { datePattern: "yyyy-MM-dd", timePattern: "HH:mm:ss" });
				})
			});
			
			this.structureMsg.push({ id: "ReadMsgRate", name: nls.incomingMsgsLabel, tooltip:nls.incomingMsgsTooltip, field: "ReadMsgRate", width:this.currentSize.ReadMsgRate, 
				decorator: Utils.integerDecorator });
			this.structureMsg.push({ id: "MsgSendRate", name: nls.outgoingMsgsLabel, tooltip:nls.outgoingMsgsTooltip, field: "MsgSendRate", width: this.currentSize.MsgSendRate, 
				decorator: Utils.integerDecorator });			
			this.structureMsg.push({ id: "Buffered", name: nls.bufferedMsgsLabel, field: "Buffered", filterable: false, width: this.currentSize.Buffered, 
			    widgetsInCell: true,
			    decorator: function() {	
					return "<div data-dojo-type='ism.widgets.ClusterStatsAssociationControl' data-dojo-props='associationType: \"Buffered\"' class='gridxHasGridCellValue'></div>";
			    }
			});
			this.structureMsg.push({ id: "Discarded", name: nls.discardedMsgsLabel, tooltip: nls.discardedMsgsTimeTooltip, field: "Discarded", filterable: false, 
		        widgetsInCell: true,
		        decorator: function() {	
				    return "<div data-dojo-type='ism.widgets.ClusterStatsAssociationControl' data-dojo-props='associationType: \"Discarded\"' class='gridxHasGridCellValue'></div>";
		        }
		    });
		},
		
		_initDialogs: function() {
			var _this = this;
        	var addServerId = "addServer"+this.id+"Dialog";
        	domConstruct.toDom(addServerId);
        	
			// instantiate the class that provides the add server dialog
			var classname =  "ism.controller.content.AddServerDialog";
			var klass = lang.getObject(classname);
			console.debug("classname: ", classname);
			this.addServerDialog = new klass({dialogId: addServerId, dialogInstruction: nls.addServerDialogInstruction, saveButtonLabel: nls.saveServerButton,
		        addServer: function(server) {
		        	_this.serverStore.add(server);
					ism.user.setServerList(_this.serverStore.data);
					_this.addServerName = server.name;
				}}, addServerId);

            this.addServerDialog.startup();
            dojo.subscribe(this.addServerDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
            	this.addServerDialog.save(lang.hitch(this,this.addServerDialog.addServer));
            	if (this.addServerName)
            		this.goToServer(this.addServerName);
            }));	

			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeRemoteServerDialog'></div>", this.id);
			var dialogId = "removeMessageHubDialog";

			this.removeDialog = new Dialog({
				title: nls.removeServerDialogTitle,
				content: "<div>" + nls.removeConfirmationTitle + "</div>",
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
			this.removeDialog.save = lang.hitch(this, function(remoteServer, onFinish, onError) { 
				if (!onFinish) { onFinish = {};}
				if (!onError) { onError = lang.hitch(this,function(error) {	
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.showErrorFromPromise(nls.removeServerError, error);
					});
				}
				console.debug("Deleting: ", remoteServer.Name);
	            var adminEndpoint = ism.user.getAdminEndpoint();
	            var options = adminEndpoint.getDefaultPostOptions();
	            options.data = JSON.stringify({ServerName:remoteServer.Name, ServerUID:remoteServer.UID});
	            var promise = adminEndpoint.doRequest("/service/remove/inactiveClusterMember/", options);
				promise.then(onFinish, onError);				
			});
			
			// create Remove not allowed dialog (remove not allowed when there are endpoints that reference the hub)
			var removeNotAllowedId = this.id+"_removeNotAllowedDialog";
			domConstruct.place("<div id='"+removeNotAllowedId+"'></div>", this.id);
			this.removeNotAllowedDialog = new Dialog({
				id: removeNotAllowedId,
				title: nls.removeNotAllowedTitle,
				content: "<div style='display: table;'>" +
						"<img style='display: inline-block; float: left; margin: 0 10px 0 0;' src='css/images/warning24.png' alt='' />" +
						"<span style='display: table-cell; vertical-align:middle; word-wrap:break-word;'>" + 
						nls.removeNotAllowed + "</span><div>",
				closeButtonLabel: nls_strings.action.Close
			}, removeNotAllowedId);
			this.removeNotAllowedDialog.dialogId = removeNotAllowedId;

		},
		
		_updateStore: function(data) {
		    // If updates were paused before we reached this call
		    // do not update the chart.
			if (this.pauseUpdates) {
				return;
			}
			dojo.style(dojo.byId('cg_refreshingLabel'), "display", "block");
			var _this = this;
			var cluster = [];
			var readrate = 0;
			var sendrate = 0;
			var buffered = 0;
			var discarded = 0;
			var bufferedStore = null;
			cluster = data.Cluster;
			var mapped = dojo.map(cluster, function(item, index){
				item.id = 1+index;
				// Set up server object so that UID is accessible when
				// a row is selected in order to delete an inactive cluster
				// member.
				item.Server = {Name: item.ServerName, UID: item.ServerUID};
				item.HealthName = _this._getHealthName(item.Health);
				item.HAStatName = _this._getHAStatName(item.HAStatus);
				item.ReadMsgRate = number.parse(item.ReadMsgRate);
				readrate += item.ReadMsgRate;
				item.MsgSendRate = number.parse(item.Reliable.MsgSendRate) + number.parse(item.Unreliable.MsgSendRate);
				sendrate += item.MsgSendRate;
				item.Reliable.Name = nls.channelReliable;
				item.Unreliable.Name = nls.channelUnreliable;
				item.ChannelList = [item.Reliable, item.Unreliable];
				item.Buffered = {Name:item.ServerName, list:item.ChannelList};
				item.Discarded = {Name:item.ServerName, list:item.ChannelList};
				buffered +=  number.parse(item.Reliable.BufferedMsgs) + number.parse(item.Unreliable.BufferedMsgs);
				discarded += number.parse(item.Reliable.DiscardedMsgs) + number.parse(item.Unreliable.DiscardedMsgs);
				return item;
			});
				
			console.log("creating grid memory with:", mapped);
			_this.store = new Memory({data: mapped, idProperty: "id", queryEngine: this.queryEngine});
			if (mapped && mapped.length > 0) {
			    _this.channelStore = new Memory({data: mapped[0].ChannelList, idProperty: "id", queryEngine: this.queryEngine});
			}

			console.log("current dataset: ", _this.currentDataset);
			
			_this.IncomingMsgs.innerHTML = readrate;
			_this.OutgoingMsgs.innerHTML = sendrate;
			_this.BufferedMsgs.innerHTML = buffered;
			_this.DiscardedMsgs.innerHTML = discarded;
			
			// make sure to re-init if a column was resized
			if (_this.resizePending) {
				_this._initStructures();
				_this.resizePending = false;
			}
			// One last check to assure updates have not been paused.
			// We want to avoid overwriting the grid (and deselecting
			// a currently selected row).
			if (!this.pauseUpdates) {
				var newStructure = [];
				newStructure = this.structureMsg;
				_this.grid.setColumns(newStructure);
				_this.grid.setStore(_this.store);
				_this.triggerListeners();
				_this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(_this.lastUpdateTime.id);
			}
			dojo.style(dojo.byId('cg_refreshingLabel'), "display", "none");
			_this.initialized = true;
		},
		
		_getHealthName: function(hcode) {			
			var text = nls.healthUnknown;
			if (hcode.toLowerCase() == "green") {
				text = nls.healthGood;
			} else if (hcode.toLowerCase() == "yellow") {
				text = nls.healthWarning;
			} else if (hcode.toLowerCase() == "red") {
				text = nls.healthBad;
			}
			return text;
		},
		
		_getHAStatName: function(status) {
            switch (status.toLowerCase()) {
            case "none":
            	return nls.haDisabled;
            case "pair":
            	return nls.haSynchronized;
            case "single":
            	return nls.haUnsynchronized;
            case "error":
            	return nls.haError;
            case "unknown":
            default:
            	return nls.haUnknown;
            }
		},			

		_getStatusIcon: function(status) {		
			if (status.toLowerCase() == "active") {
				return "ismIconEnabled";
			} else if (status.toLowerCase() == "connecting") {
				return "ismIconLoading";
			} else {
				return "ismIconDisabled";
			}
		},
		
		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "clusterStatsGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			this.grid = GridFactory.createGrid(gridId, "clusterStatsGridContent", this.store, this.structureMsg, { filter: true, paging: true, shortPaging: true, actionsMenuTitle: nls_strings.action.Actions });
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.grid.autoHeight = true;
			this.grid.body.refresh();
		},
		
		_getSelectedData: function() {
			// summary:
			// 		get the row data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).data();
		},

		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				console.debug("index="+ind);
				console.log("row count: "+this.grid.rowCount());
				this.deleteButton.set('disabled',false);
			} else {
				this.deleteButton.set('disabled',true);
			}
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
			this.deleteButton.set('disabled',true);			
		},
		
		_initEvents: function() {			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				// Pause grid updates any time a row is selected
				this.pauseUpdates = true;
				// Replace the message about update interval with 
				// messaging showing that updates are currently paused.
				this.updateInterval.innerHTML =  "<b>" + nls.updatesPaused + "</b>";
				Utils.flashElement(this.updateInterval.id);
				// Enable the resume updates button
				this.resumeupdButton.set('disabled', false);
				this._doSelect();
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var remoteServer = this._getSelectedData();
				var serverStatus = remoteServer.Status;
				if (serverStatus.toLowerCase() !== "inactive") {
					this.removeNotAllowedDialog.show();
				} else {
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.show();
				}
			}));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				this.pauseUpdates = true;
				// any message on this topic means that we confirmed the operation
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				// Using messaging deleting progress to avoid need to add more nls messages
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls_messaging.messaging.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var rowData = this.grid.row(id).data();
				console.log("deleting " + rowData.Server.Name);
				this.removeDialog.save(rowData.Server, lang.hitch(this, function() {
					this.store.remove(id);
					if (this.store.data.length === 0 ||
							this.grid.select.row.getSelected().length === 0) {
						this.deleteButton.set('disabled',true);
					}					
					this.removeDialog.hide();					
					bar.innerHTML = "";
				}),
				lang.hitch(this,function(error) {	
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					var code = this.removeDialog.showErrorFromPromise(nls.removeServerError, error);
					if (code === "CWLNA0113") {
						this.store.remove(id);
					}
				}));
			}));
		},

		_initStatus: function() {
		    Utils.updateStatus();
		    Utils.updateClusterStatus();
		},
		
		_getUri: function(server) {
			var uri = window.location.pathname;
		    var queryParams = Utils.getUrlVars();
		    if (queryParams) {
		    	queryParams["server"] = encodeURIComponent(server.uid);
		    	queryParams["serverName"] = encodeURIComponent(server.name);
		    }
		    var sep = "?";
            for (var param in queryParams) {
                uri += sep + param + "=" + queryParams[param];
                sep = "&";
            }
            return uri;
		},
		
		// Callback for resume updates button
		resumeUpdates: function() {			
			this.pauseUpdates = false;
			// Deselect any selected rows
			this.grid.row(this.grid.select.row.clear());
			// Restore the info about frequency of updates
			this.updateInterval.innerHTML = nls.cacheInterval;
			Utils.flashElement(this.updateInterval.id);
			// Disable the resume updates button
			this.resumeupdButton.set('disabled', true);
		},
		
		goToServer: function(serverName) {
			console.debug("server link clicked");
			var server = ism.user.getServerByName(serverName);
			if (server && server.length > 0)
			    window.location = this._getUri(server[0]);
			else {
				this.serverStore = null;
				this.serverStore = new Memory({data:ism.user.getServerList(), idProperty: "uid"});
			    this.addServerDialog.show(null, this.serverStore, serverName);
			}
		},
		
		updateGrid: function(data) {
			console.debug(this.declaredClass + ".updateGrid()");
			
			// If updates are paused, then do not do the work
			// related to updating the grid
			if (!this.pauseUpdates) {
				if (data && data.Cluster)
					this._updateStore(data);
			}			

		    var _this = this;
            setTimeout(lang.hitch(this, function(){
                // the change in status will be published on the subscribed topic, so we just need to schedule
    			// the next update here, whether it was successful or errored out
            	if (data)
            		Utils.updateClusterStatus();
            	else
                	_this.updateGrid();
            }), _this.statusUpdateInterval);             			    
		},
		
		/* 
		 * Update displayed status information for the server we are currently managing.
		 */
		updateStatus: function(data) {
			var serverName = (data.Server && data.Server.Name) ? data.Server.Name : "";
			var clusterStatus = (data.Cluster && data.Cluster.Status) ? data.Cluster.Status : "";
			var haStatus = data.harole;
			var clusterIcon = this._getStatusIcon(clusterStatus);
			
			this.ServerName.innerHTML = serverName;
			domClass.replace(this.ConnState, clusterIcon);
			this.HAStatus.innerHTML = nlsHome.home.role[haStatus];
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
