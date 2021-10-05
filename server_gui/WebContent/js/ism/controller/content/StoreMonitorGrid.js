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
		'dojo/on',
		'dojo/when',
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
		'dojo/text!ism/controller/content/templates/StoreMonitorGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils'
		], function(declare, lang, array, domClass, domConstruct, on, when, number, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				nls, Select, GridFactory, _TemplatedContent, gridtemplate, Resources, Utils, MonitoringUtils) {

	return declare("ism.controller.content.StoreMonitorGrid", _TemplatedContent, {

		nls: nls,
		templateString: gridtemplate,
		
		grid: null,
		store: null,

		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);

			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.applianceMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));
			
			this._updateStore();
			this.createGrid();
		},

		_updateStore: function() {
			console.debug(this.declaredClass + "._updateStore()");
			dojo.style(dojo.byId('storegrid_refreshingLabel'), "display", "block");
			var storeData = MonitoringUtils.getStoreData();
			storeData.then(lang.hitch(this,function(data) {
				data = data.Store;
				
				// Add 'id' field and throughput				
				var mapped = [];
				var index = 0;

				if (data.DiskUsedPercent !== undefined) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.store.diskUsedPercent,
						value: data.DiskUsedPercent + " %"
					});
					index++;
				}
				if (data.DiskFreeBytes !== undefined) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.store.diskFreeBytes,
						value: MonitoringUtils.mbDecorator(data.DiskFreeBytes, 2)
					});
					index++;
				}
                if (data.MemoryUsedPercent !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.persistentMemoryUsedPercent,
                        value: data.MemoryUsedPercent + " %"
                    });
                    index++;
                }
                if (data.MemoryTotalBytes !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.memoryTotalBytes,
                        value: MonitoringUtils.mbDecorator(data.MemoryTotalBytes, 2)
                    });
                    index++;
                }
                if (data.Pool1TotalBytes !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.Pool1TotalBytes,
                        value: MonitoringUtils.mbDecorator(data.Pool1TotalBytes, 2)
                    });
                    index++;
                }

                if (data.Pool1UsedBytes !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.Pool1UsedBytes,
                        value: MonitoringUtils.mbDecorator(data.Pool1UsedBytes, 2)
                    });
                    index++;
                }
				
                if (data.Pool1UsedPercent !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.Pool1UsedPercent,
                        value: data.Pool1UsedPercent + " %"
                    });
                    index++;
                }

                if (data.Pool1RecordsLimitBytes !== undefined) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.store.Pool1RecordLimitBytes,
						value: MonitoringUtils.mbDecorator(data.Pool1RecordsLimitBytes, 2)
					});
					index++;
				}
				
				if (data.Pool1RecordsUsedBytes !== undefined) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.store.Pool1RecordsUsedBytes,
						value: MonitoringUtils.mbDecorator(data.Pool1RecordsUsedBytes, 2)
					});
					index++;
				}
				
                if (data.Pool2TotalBytes !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.Pool2TotalBytes,
                        value: MonitoringUtils.mbDecorator(data.Pool2TotalBytes, 2)
                    });
                    index++;
                }

                if (data.Pool2UsedBytes !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.Pool2UsedBytes,
                        value: MonitoringUtils.mbDecorator(data.Pool2UsedBytes, 2)
                    });
                    index++;
                }
                
                if (data.Pool2UsedPercent !== undefined) {
                    mapped.push({
                        id: index,
                        name: nls.monitoring.grid.store.Pool2UsedPercent,
                        value: data.Pool2UsedPercent + " %"
                    });
                    index++;
                }
								
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('storegrid_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(err) {			
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},
		
		createGrid: function() {
			console.debug(this.declaredClass + ".createGrid()");
			// summary:
			// 		create and instantiate the grid
			var gridId = "storeMonitorGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			var structure = [
                {   id: "name", name: nls.monitoring.grid.memory.name, width: "300px" },
                {   id: "value", name: nls.monitoring.grid.memory.value, width: "150px", 
                    decorator: function(cellData) {
                        return "<div style='float: right'>"+cellData+"</div>";
                    }
                }
			];

			this.grid = GridFactory.createGrid(gridId, "storeMonitorGridContent", this.store, structure, { basic: true });
		},
		
		_refreshGrid: function() {
			console.debug(this.declaredClass + "._refreshGrid()");
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
			this._updateStore();
		}
	});

});
