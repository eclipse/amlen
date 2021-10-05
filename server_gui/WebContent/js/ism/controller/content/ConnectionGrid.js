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
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/aspect',
		'dojo/on',
		'dojo/number',
		'dojo/store/Memory',
		'dojo/data/ItemFileWriteStore',
		'dijit/_base/manager',
		'dijit/layout/ContentPane',
		'dijit/form/Button',
		'dojo/i18n!ism/nls/monitoring',
		'ism/widgets/Select',
		'ism/widgets/GridFactory',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/ConnectionGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, domClass, domConstruct, aspect, on, number, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				nls, Select, GridFactory, _TemplatedContent, connectgrid, Resources, Utils, MonitoringUtils, nls_strings) {

	return declare("ism.controller.content.ConnectionGrid", _TemplatedContent, {

		nls: nls,
		templateString: connectgrid,
		
		grid: null,
		store: null,
		
		currentListeners: "all",
		currentDataset: "CONNTIME_WORST",
		currentSize: {Name: "100px", 
			          Endpoint: "100px", 
			          ClientAddr: "100px", 
			          UserId: "80px", 
			          Protocol: "80px", 
			          Throughput: "80px", 
			          Read: "80px", 
			          Write: "80px", 
			          Duration: "100px"}, 

		structureMsg: null,
		structureBytes: null,
		resizePending: false,

		ColumnType: {
			MSG: "msg",
			BYTES: "bytes"
		},
		
		initialized: false,
		epListRefreshing: false,
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
			this._initStructures();
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".startup()");
			this.inherited(arguments);

			this._initGrid(this.ColumnType.MSG);
			this._setupFields(false, this.updateGrid());
			
			// listen for a column resize event
			aspect.after(this.grid.columnResizer, "onResize", lang.hitch(this, function(colId, newWidth, oldWidth) {
				// currently all data is in the variable newWidth...
				this.currentSize[newWidth[0]] = newWidth[1] + "px";
				this.resizePending = true;
			}));

		},

		_initStructures: function() {
			this.structureMsg = [];
			this.structureMsg.push({ id: "Name", name: nls.monitoring.grid.name, tooltip:nls.monitoring.grid.nameTooltip, width: this.currentSize.Name,
				decorator: Utils.nameDecorator
			});
			this.structureMsg.push({ id: "Endpoint", name: nls.monitoring.grid.endpoint, tooltip:nls.monitoring.grid.endpointTooltip, width: this.currentSize.Endpoint,
				decorator: Utils.nameDecorator
			});
			this.structureMsg.push({ id: "ClientAddr", name: nls.monitoring.grid.clientIp, tooltip:nls.monitoring.grid.clientIpTooltip, width: this.currentSize.ClientAddr });
			this.structureMsg.push({ id: "UserId", name: nls.monitoring.grid.userId, tooltip:nls.monitoring.grid.userIdTooltip, width: this.currentSize.UserId, decorator: Utils.nameDecorator });
			this.structureMsg.push({ id: "Protocol", name: nls.monitoring.grid.protocol, tooltip:nls.monitoring.grid.protocolTooltip, width: this.currentSize.Protocol });
			this.structureMsg.push({ id: "Throughput", name: nls.monitoring.grid.msgThroughput, field: "ThroughputMsg", width: this.currentSize.Throughput, 
				decorator: Utils.integerDecorator });
			this.structureMsg.push({ id: "Read", name: nls.monitoring.grid.msgRecv, tooltip:nls.monitoring.grid.msgRecvTooltip, field: "ReadMsg", width:this.currentSize.Read, 
				decorator: Utils.integerDecorator });
			this.structureMsg.push({ id: "Write", name: nls.monitoring.grid.msgSent, tooltip:nls.monitoring.grid.msgSentTooltip, field: "WriteMsg", width: this.currentSize.Write, 
				decorator: Utils.integerDecorator });			
			this.structureMsg.push({ id: "Duration", name: nls.monitoring.grid.connectionTime, tooltip: nls.monitoring.grid.connectionTimeTooltip, width: this.currentSize.Duration, 
				decorator: function(cellData) {
					var value = parseFloat(cellData) / 1000000000;
					return "<div style='float: right'>"+number.format(value, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
				}
			});

			this.structureBytes = [];
			this.structureBytes.push({ id: "Name", name: nls.monitoring.grid.name, tooltip:nls.monitoring.grid.nameTooltip, width: this.currentSize.Name,
				decorator: Utils.nameDecorator
			});
			this.structureBytes.push({ id: "Endpoint", name: nls.monitoring.grid.endpoint, tooltip:nls.monitoring.grid.endpointTooltip, width: this.currentSize.Endpoint,
				decorator: Utils.nameDecorator
			});
			this.structureBytes.push({ id: "ClientAddr", name: nls.monitoring.grid.clientIp, tooltip:nls.monitoring.grid.clientIpTooltip, width: this.currentSize.ClientAddr });
			this.structureBytes.push({ id: "UserId", name: nls.monitoring.grid.userId, tooltip:nls.monitoring.grid.userIdTooltip, width: this.currentSize.UserId, decorator: Utils.nameDecorator });
			this.structureBytes.push({ id: "Protocol", name: nls.monitoring.grid.protocol, tooltip:nls.monitoring.grid.protocolTooltip, width: this.currentSize.Protocol });
			this.structureBytes.push({ id: "Throughput", name: nls.monitoring.grid.bytesThroughput, field: "ThroughputBytes", width: this.currentSize.Throughput, 
				decorator: Utils.decimalDecorator});
			this.structureBytes.push({ id: "Read", name: nls.monitoring.grid.bytesRecv, tooltip:nls.monitoring.grid.bytesRecvTooltip, field: "ReadBytes", width: this.currentSize.Read,
				decorator: function(cellData) {
					var value = parseFloat(cellData) / 1024;
					return "<div style='float: right'>"+number.format(value, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
				}
			});
			this.structureBytes.push({ id: "Write", name: nls.monitoring.grid.bytesSent, tooltip:nls.monitoring.grid.bytesSentTooltip, field: "WriteBytes", width: this.currentSize.Write,
				decorator: function(cellData) {
					var value = parseFloat(cellData) / 1024;
					return "<div style='float: right'>"+number.format(value, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
				}
			});
			this.structureBytes.push({ id: "Duration", name: nls.monitoring.grid.connectionTime, tooltip:nls.monitoring.grid.connectionTimeTooltip, width: this.currentSize.Duration, 
				decorator: function(cellData) {
					var value = parseFloat(cellData) / 1000000000;
					return "<div style='float: right'>"+number.format(value, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
				}
			});
		},

		_getStructure: function(columnType) {
			if (!columnType || columnType == this.ColumnType.BYTES) {
				return this.structureBytes;
			} else {
				return this.structureMsg;
			}
		},

		_updateStore: function() {
			dojo.style(dojo.byId('cg_refreshingLabel'), "display", "block");
			var _this = this;
			var promise = MonitoringUtils.getConnData(MonitoringUtils.Connection.View[this.currentDataset],this.currentListeners);
			
			promise.then(function(results) {
				var data = results.Connection;
				// Add 'id' field and throughput
				var mapped = dojo.map(data, function(item, index){
					item.id = 1+index;
					item.ThroughputBytes = ((parseFloat(item.ReadBytes) + parseFloat(item.WriteBytes)) / 1024 / (parseFloat(item.Duration) / 1000000000));
					item.ThroughputMsg = ((parseFloat(item.ReadMsg) + parseFloat(item.WriteMsg)) / (parseFloat(item.Duration) / 1000000000));
					item.ConnectionTime = number.parse(item.ConnectionTime);	
					item.ReadBytes = number.parse(item.ReadBytes);	
					item.WriteBytes = number.parse(item.WriteBytes);
					item.ReadMsg = number.parse(item.ReadMsg);	
					item.WriteMsg = number.parse(item.WriteMsg);
					return item;
				});
				
				console.log("creating grid memory with:", mapped);
				_this.store = new Memory({data: mapped});

				console.log("current dataset: ", _this.currentDataset);
				
				// make sure to re-init if a column was resized
				if (_this.resizePending) {
					_this._initStructures();
					_this.resizePending = false;
				}
				var newStructure = [];
				if (_this.currentDataset == "TPUT_BYTES_WORST" || _this.currentDataset == "TPUT_BYTES_BEST") {
					newStructure = _this._getStructure(_this.ColumnType.BYTES);
				}
				else {
					newStructure = _this._getStructure(_this.ColumnType.MSG);
				}
				_this.grid.setColumns(newStructure);
				_this.grid.setStore(_this.store);
				_this.triggerListeners();
				_this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(_this.lastUpdateTime.id);
				dojo.style(dojo.byId('cg_refreshingLabel'), "display", "none");
				_this.initialized = true;
			}, function(err) {
				console.debug("Error from getConnData: "+err);
				dojo.style(dojo.byId('cg_refreshingLabel'), "display", "none");					
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},
		
		_setupFields: function(refresh, onComplete, onError) {
			console.log(this.declaredClass + ".setupFields");
			
			// Populate listeners combo
			this.listenerCombo.store.newItem({ name: nls.monitoring.grid.allEndpoints, id: nls.monitoring.grid.allEndpoints });		
			this.listenerCombo.store.save();

			MonitoringUtils.getEndpointNames(lang.hitch(this, function(namesValue) {
				if (namesValue.error) {
					console.debug("Error from getEndpointNames: "+namesValue.error);
					MonitoringUtils.showMonitoringPageError(namesValue.error);
					if (onError) {
						onError();
					}
					return;
				}
				
				// sort alphabetically
				namesValue.sort();
				
				// add items to combo widget
				dojo.forEach(namesValue, function(item, index) {
					this.listenerCombo.store.newItem({ name: Utils.textDecorator(item), id: item });	
				},this);
				this.listenerCombo.store.save();
				
				if (!refresh) {
					// Populate datasets combo
					for (var key in MonitoringUtils.Connection.View) {
						this.datasetCombo.store.newItem({ name: MonitoringUtils.Connection.View[key].label, key: key });
					}
					this.datasetCombo.store.save();
				}
				if (onComplete) {
					onComplete();
				}
			})); 
		},

		
		_initGrid: function(columnType) {
			// summary:
			// 		create and instantiate the grid
			var gridId = "connectionGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			this.grid = GridFactory.createGrid(gridId, "connectionGridContent", this.store, this._getStructure(columnType), { filter: true, paging: true, shortPaging: true, actionsMenuTitle: nls_strings.action.Actions });
			this.grid.autoHeight = true;
			this.grid.body.refresh();
		},
		
		changeListener: function(listener) {
			console.debug(this.declaredClass + ".changeListener("+listener+")");
			if (listener == nls.monitoring.grid.allEndpoints) {
				listener = null; // special case for showing all of the listeners
			}
			this.currentListeners = listener;
			//this._updateStore();			
		},
		
		changeDataset: function(sel) {
			console.debug(this.declaredClass + ".changeDataset("+sel+")");
	
			var selKey = null;
			for (var key in MonitoringUtils.Connection.View) {
				if (sel == MonitoringUtils.Connection.View[key].label) {
					selKey = key;
				}
			}
			//console.debug("selected key="+selKey);
			if (selKey) {
				this.currentDataset = selKey;
				//this._updateStore();
			}
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");

			if (this.initialized && !this.epListRefreshing) {
				// clear out the listenerCombo an repopulate it
				this.epListRefreshing = true;
				var t = this;
				var selectedEndpoint = t.currentListeners;
				this.listenerCombo.store.fetch({
					onComplete: function(items, request) {
						console.debug("deleting items");
						var len = items ? items.length : 0;
						for (var i=0; i < len; i++) {
							t.listenerCombo.store.deleteItem(items[i]);
						} 
						t.listenerCombo.store.save({onComplete: function() {
							t._setupFields(true, 
									lang.hitch(this, function() {
										t.listenerCombo.set('value', selectedEndpoint);	
										t.epListRefreshing = false;
									}), 
									lang.hitch(this, function() {
										// if by chance there was an error setting up fields
										t.epListRefreshing = false;
								}));
						}});
				}});
			}
			this._updateStore();
			
			
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
