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
		'dojo/text!ism/controller/content/templates/MemoryMonitorGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'gridx/Grid',
		'gridx/core/model/cache/Sync'
		], function(declare, lang, array, domClass, domConstruct, on, when, number, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				nls, Select, GridFactory, _TemplatedContent, gridtemplate, Resources, Utils, MonitoringUtils, Grid, Sync) {

	return declare("ism.controller.content.MemoryMonitorGrid", _TemplatedContent, {

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
			dojo.style(dojo.byId('memorygrid_refreshingLabel'), "display", "block");
			var memoryData = MonitoringUtils.getMemoryData();
			memoryData.then(lang.hitch(this,function(data) {
				data = data.Memory;
				
				// Add 'id' field and throughput
				var mapped = [];
				var index = 0;

				if (data.MemoryTotalBytes) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.memoryTotalBytes,
						value: MonitoringUtils.mbDecorator(data.MemoryTotalBytes, 2)
					});
					index++;
				}
				if (data.MemoryFreeBytes) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.memoryFreeBytes,
						value: MonitoringUtils.mbDecorator(data.MemoryFreeBytes, 2)
					});
					index++;
				}
				if (data.MemoryFreePercent) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.memoryFreePercent,
						value: data.MemoryFreePercent + " %"
					});
					index++;
				}
				if (data.ServerVirtualMemoryBytes) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.serverVirtualMemoryBytes,
						value: MonitoringUtils.mbDecorator(data.ServerVirtualMemoryBytes, 2)
					});
					index++;
				}
				if (data.ServerResidentSetBytes) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.serverResidentSetBytes,
						value: MonitoringUtils.mbDecorator(data.ServerResidentSetBytes, 2)
					});
					index++;
				}
				if (data.MessagePayloads) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.messagePayloads,
						value: MonitoringUtils.mbDecorator(data.MessagePayloads, 2)
					});
					index++;
				}
				if (data.PublishSubscribe) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.publishSubscribe,
						value: MonitoringUtils.mbDecorator(data.PublishSubscribe, 2)
					});
					index++;
				}
				if (data.Destinations) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.destinations,
						value: MonitoringUtils.mbDecorator(data.Destinations, 2)
					});
					index++;
				}
				if (data.CurrentActivity) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.currentActivity,
						value: MonitoringUtils.mbDecorator(data.CurrentActivity, 2)
					});
					index++;
				}
				if (data.ClientStates) {
					mapped.push({
						id: index,
						name: nls.monitoring.grid.memory.clientStates,
						value: MonitoringUtils.mbDecorator(data.ClientStates, 2)
					});
					index++;
				}


				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('memorygrid_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(err) {					
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},
		
		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "memoryMonitorGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			var structure = [
				{	id: "name", name: nls.monitoring.grid.memory.name, width: "300px" },
				{	id: "value", name: nls.monitoring.grid.memory.value, width: "150px", 
	                decorator: function(cellData) {
	                    return "<div style='float: right'>"+cellData+"</div>";
	                }
				}
			];

			this.grid = GridFactory.createGrid(gridId, "memoryMonitorGridContent", this.store, structure, { basic: true });
		},
		
		_refreshGrid: function() {
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
