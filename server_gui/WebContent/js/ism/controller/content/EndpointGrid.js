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
		'dojo/text!ism/controller/content/templates/EndpointGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, domClass, domConstruct, aspect, on, number, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				nls, Select, GridFactory, _TemplatedContent, connectgrid, Resources, Utils, MonitoringUtils, nls_strings) {

	return declare("ism.controller.content.EndpointGrid", _TemplatedContent, {

		nls: nls,
		templateString: connectgrid,
		
		grid: null,
		store: null,
		
		currentMetric: MonitoringUtils.Endpoint.Metric.TOTAL_CONNECTIONS.name,
		
		currentSize: {endpoint: "150px", 
			 		  last10: "140px", 
			 		  last60: "140px", 
			 		  last360: "140px", 
			 		  last1440: "140px"}, 

		structureMsg: null,
		structureBytes: null,
		resizePending: false,
		msgUnitDecorator: null,
		bytesUnitDecorator: null,
		

		//_setDefault: true,     // tmp variable to override the Metric field with a default value upon initialization
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
			this._initDecorators();
			this._initStructures();
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.endpointStatistics.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));	

			this._setupFields();
			this.createGrid();
			this._updateStore();
			
			// listen for a column resize event
			aspect.after(this.grid.columnResizer, "onResize", lang.hitch(this, function(colId, newWidth, oldWidth) {
				// currently all data is in the variable newWidth...
				this.currentSize[newWidth[0]] = newWidth[1] + "px";
				this.resizePending = true;
			}));
			
		},
		
		_initDecorators: function() {
			
			this.msgUnitDecorator = lang.hitch(this, function(cellData) {
				return "<div style='float: right'>"+number.format(cellData, {locale: ism.user.localeInfo.localeString})+"</div>";
			});
			
			this.bytesUnitDecorator = lang.hitch(this, function(cellData) {
				var val = parseFloat(cellData);
				var units = "";
				if (val < 1024) {
					units = "B";
				} else if (val >= 1024 && val < 1024*1024) {   // KB
					units = "KB";
					val /= 1024;
				} else {
					units = "MB";	
					val /= 1024*1024;
				}
				return "<div style='float: right'>"+number.format(val, {places: 2, locale: ism.user.localeInfo.localeString}) + " " + units+"</div>";
			});
			
		},

		_initStructures: function() {

			this.structureMsg = []; 
			this.structureMsg = [
				{	id: "endpoint", name: nls.monitoring.grid.endpoint, width: this.currentSize.endpoint,
					decorator: Utils.nameDecorator
				},
				{	id: "last10", name: nls.monitoring.grid.last10, width:  this.currentSize.last10, decorator: this.msgUnitDecorator },
				{	id: "last60", name: nls.monitoring.grid.last60, width:  this.currentSize.last60, decorator: this.msgUnitDecorator },
				{	id: "last360", name: nls.monitoring.grid.last360, width:  this.currentSize.last360, decorator: this.msgUnitDecorator },
				{	id: "last1440", name: nls.monitoring.grid.last1440, width:  this.currentSize.last1440, decorator: this.msgUnitDecorator }
			];
			
			this.structureBytes = [];
			this.structureBytes = [
				{	id: "endpoint", name: nls.monitoring.grid.endpoint, width:  this.currentSize.endpoint,
					decorator: Utils.nameDecorator
				},
				{	id: "last10", name: nls.monitoring.grid.last10, width:  this.currentSize.last10, decorator: this.bytesUnitDecorator },
				{	id: "last60", name: nls.monitoring.grid.last60, width:  this.currentSize.last60, decorator: this.bytesUnitDecorator },
				{	id: "last360", name: nls.monitoring.grid.last360, width:  this.currentSize.last360, decorator: this.bytesUnitDecorator },
				{	id: "last1440", name: nls.monitoring.grid.last1440, width:  this.currentSize.last1440, decorator: this.bytesUnitDecorator }
			];
		},

		_getStructure: function() {
			
			if (this.resizePending) {
				this._initStructures();
				this.resizePending = false;
			}
			
			if (this.currentMetric == "Bytes") {
				return this.structureBytes;
			} else {
				return this.structureMsg;
			}
		},

		_updateStore: function() {
			dojo.style(dojo.byId('eg_refreshingLabel'), "display", "block");

			MonitoringUtils.getEndpointNames(lang.hitch(this, function(namesValue) {
				if (namesValue.error) {
					console.debug("Error from getEndpointNames: "+namesValue.error);
					dojo.style(dojo.byId('eg_refreshingLabel'), "display", "none");
					MonitoringUtils.showMonitoringPageError(namesValue.error);
					return;
				}

				namesValue.sort();
				var endpointData = [];
				var dlArgs = [];
				var time;
				dojo.forEach(namesValue, lang.hitch(this, function(name, index) {
					dlArgs[index] = MonitoringUtils.getEndpointHistory(24*60*60, { endpoint: name, metric: this.currentMetric, 
						numPoints: 1440 , onComplete: lang.hitch(this, function(data) {
						if (data.error) {
							console.debug("Error from getEndpointHistory: "+data.error);
							MonitoringUtils.showMonitoringPageError(data.error);
							return;
						}
						
						var type = this.currentMetric ? this.currentMetric : data.Type;
						var dataPoints = data[type];
						time = data.LastUpdateTimestamp ? new Date(data.LastUpdateTimestamp) : new Date();						

						// convert the data from data/sec to data/interval
						switch (this.currentMetric) {
							case MonitoringUtils.Endpoint.Metric.BYTES_COUNT.name:
							case MonitoringUtils.Endpoint.Metric.MSG_COUNT.name:
								for (var i = 0, len=dataPoints.length; i < len; i++) {
									dataPoints[i] *= 60;
								}
								break;
							default:
								break;
						}
						//console.log("pushing endpoint data for " + name, dataPoints);
						endpointData.push(this._calculateGridRow(name, dataPoints));
					})});
				}));
				var dl = new dojo.DeferredList(dlArgs);
				dl.then(lang.hitch(this, function(results) {
					var mapped = dojo.map(endpointData, function(item, index) {
						item.id = 1+index;
						return item;
					});
					console.log("creating grid memory with:", mapped);
					this.store = new Memory({data: mapped});
					this.grid.setColumns(this._getStructure());
					this.grid.setStore(this.store);
					this.triggerListeners();
					this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime(time) + "</b>";
					Utils.flashElement(this.lastUpdateTime.id);
					dojo.style(dojo.byId('eg_refreshingLabel'), "display", "none");
				}));
			}));
		},

		_calculateGridRow: function(endpoint, data) {
			var metric = this.currentMetric;

			var val10 = 0;
			var val60 = 0;
			var val360 = 0;
			var val1440 = 0;
			var dataLength = data ? data.length : 0;
			for (var i = 0; i < dataLength; i++) {
				switch (metric) {
					case MonitoringUtils.Endpoint.Metric.BAD_COUNT.name:
					case MonitoringUtils.Endpoint.Metric.TOTAL_CONNECTIONS.name:
						if (i == dataLength - 10) {
							val10 = data[dataLength - 1] - data[i];
						}
						if (i == dataLength - 60) {
							val60 = data[dataLength - 1] - data[i];
						}
						if (i == dataLength - 360) {
							val360 = data[dataLength - 1] - data[i];
						}
						if (i == dataLength - 1440) {
							val1440 = data[dataLength - 1] - data[i];
						}

						break;
					default:
						if (i >= dataLength - 10) {
							val10 += data[i];
						}
						if (i >= dataLength - 60) {
							val60 += data[i];
						}
						if (i >= dataLength - 360) {
							val360 += data[i];
						}
						if (i >= dataLength - 1440) {
							val1440 += data[i];
						}
						break;
				}
			}
			if (metric == MonitoringUtils.Endpoint.Metric.BYTES_COUNT.name) {
				val10 *= 1048576 ; 
				val60 *= 1048576 ; 
				val360 *= 1048576 ; 
				val1440 *= 1048576 ; 
			} 
			val10 = Math.round(val10);
			val60 = Math.round(val60);
			val360 = Math.round(val360);
			val1440 = Math.round(val1440);
			
			return {
				endpoint: endpoint,
				last10: val10,
				last60: val60,
				last360: val360,
				last1440: val1440
			};
		},
		
		_setupFields: function() {
			console.log(this.declaredClass + ".setupFields");
			
			// Populate datasets combo
			for (var key in MonitoringUtils.Endpoint.Metric) {
				if (key != "ACTIVE_CONNECTIONS") {
					this.metricCombo.store.newItem({ name: MonitoringUtils.Endpoint.Metric[key].label, key: key });
				}
			}
			this.metricCombo.store.save();
		},

		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "endpointGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			this.grid = GridFactory.createGrid(gridId, "endpointGridContent", this.store, this._getStructure(), { filter: true, paging: true, actionsMenuTitle: nls_strings.action.Actions });
			this.grid.autoHeight = true;
			this.grid.body.refresh();
		},
		
		changeMetric: function(sel) {
			console.debug(this.declaredClass + ".changeMetric("+sel+")");
	
			var selKey = null;
			for (var key in MonitoringUtils.Endpoint.Metric) {
				if (sel == MonitoringUtils.Endpoint.Metric[key].label) {
					selKey = MonitoringUtils.Endpoint.Metric[key].name;
				}
			}
			//console.debug("selected key="+selKey);
			if (selKey) {
				this.currentMetric = selKey;
			}
			/*
			if (this._setDefault) {
				this._setDefault = false;
				this.metricCombo.setValue(MonitoringUtils.Endpoint.Metric.TOTAL_CONNECTIONS.label);
				return;
			}
			*/
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
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
