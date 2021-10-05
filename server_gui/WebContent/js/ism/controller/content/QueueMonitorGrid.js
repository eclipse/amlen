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
		'dijit/form/Form',
		'dojo/i18n!ism/nls/monitoring',
		'ism/widgets/Select',
		'ism/widgets/GridFactory',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/QueueMonitorGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, domClass, domConstruct, on, when, number, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				Form, nls, Select, GridFactory, _TemplatedContent, gridtemplate, Resources, Utils, MonitoringUtils, nls_strings) {

	return declare("ism.controller.content.QueueMonitorGrid", _TemplatedContent, {

		nls: nls,
		templateString: gridtemplate,
		
		grid: null,
		store: null,

		//_setDefault: true,     // tmp variable to override the Metric field with a default value upon initialization
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});
		},
		
		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.queueMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));	

			this._setupFields();
			this.createGrid();
			this._updateStore();
		},

		_updateStore: function() {
			dojo.style(dojo.byId('qmg_refreshingLabel'), "display", "block");
			var name = this.queueName.value;
			var view = this.datasetCombo.value;
			var results = 100;

			var deferred = MonitoringUtils.getQueueData(name, view, results);
			
			deferred.then(lang.hitch(this, function(data) {
				console.log(data);
				var mapped = array.map(data.Queue, function(item, index){
					var record = {};
					record.id = 1+index;
					record.queueName = item.Name;
					record.msgBuf = number.parse(item.BufferedMsgs);
					record.msgBufHWM = number.parse(item.BufferedMsgsHWM);
					record.msgBufHWMPercent = parseFloat(item.BufferedHWMPercent);
					record.perMsgBuf = parseFloat(item.BufferedPercent);					
					record.totMsgProd = number.parse(item.ProducedMsgs);
					record.totMsgCons = number.parse(item.ConsumedMsgs);
					record.numProd = number.parse(item.Producers);
					record.numCons = number.parse(item.Consumers);
					record.totMsgRej = number.parse(item.RejectedMsgs);
					record.totMsgExp = number.parse(item.ExpiredMsgs) | 0;
					record.maxMsgs = number.parse(item.MaxMessages);
					return record;
				});
				
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
			    this.triggerListeners();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('qmg_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(error) {
				console.debug("Error from getQueueData: "+error);
				dojo.style(dojo.byId('qmg_refreshingLabel'), "display", "none");

				MonitoringUtils.showMonitoringPageError(error);
				return;
			});
		},
		
		_setupFields: function() {
			console.log(this.declaredClass + ".setupFields");
			
			// Populate datasets combo
			for (var key in MonitoringUtils.Queue.View) {
				this.datasetCombo.store.newItem({ 
					label: MonitoringUtils.Queue.View[key].label, 
					name: MonitoringUtils.Queue.View[key].name 
				});
			}
			this.datasetCombo.store.save();
		},
		
		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "queueMonitorGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			var structure = [
				{	id: "queueName", name: nls.monitoring.grid.queue.queueName, tooltip: nls.monitoring.grid.queue.queueNameTooltip, width: "150px", decorator: Utils.nameDecorator },
				{	id: "numProd", name: nls.monitoring.grid.queue.numProd, tooltip: nls.monitoring.grid.queue.numProdTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "numCons", name: nls.monitoring.grid.queue.numCons, tooltip: nls.monitoring.grid.queue.numConsTooltip, width: "80px", decorator: Utils.integerDecorator },
				// Total Messages Group
				{	id: "totMsgProd", name: nls.monitoring.grid.queue.totMsgProd, tooltip: nls.monitoring.grid.queue.totMsgProdTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "totMsgCons", name: nls.monitoring.grid.queue.totMsgCons, tooltip: nls.monitoring.grid.queue.totMsgConsTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "totMsgRej", name: nls.monitoring.grid.queue.totMsgRej, tooltip: nls.monitoring.grid.queue.totMsgRejTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "totMsgExp", name: nls.monitoring.grid.queue.totMsgExp, tooltip: nls.monitoring.grid.queue.totMsgExpTooltip, width: "80px", decorator: Utils.integerDecorator },
				// Buffered Messages Group
				{	id: "maxMsgs", name: nls.monitoring.grid.queue.maxMsgs, tooltip: nls.monitoring.grid.queue.maxMsgsTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "msgBuf", name: nls.monitoring.grid.queue.msgBuf, tooltip: nls.monitoring.grid.queue.msgBufTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "perMsgBuf", name: nls.monitoring.grid.queue.perMsgBuf, tooltip: nls.monitoring.grid.queue.perMsgBufTooltip, width: "80px", 					
					decorator: function(cellData) {
						return "<div style='float: right'>"+number.format(cellData, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
					}
				}, 
				{	id: "msgBufHWM", name: nls.monitoring.grid.queue.msgBufHWM, tooltip: nls.monitoring.grid.queue.msgBufHWMTooltip, width: "80px", decorator: Utils.integerDecorator },
				{	id: "msgBufHWMPercent", name: nls.monitoring.grid.queue.msgBufHWMPercent, tooltip: nls.monitoring.grid.queue.msgBufHWMPercentTooltip, width: "80px", 					
					decorator: function(cellData) {
						return "<div style='float: right'>"+number.format(cellData, {places: 1, locale: ism.user.localeInfo.localeString})+"</div>";
					}
				} 	
			];
			var headerGroups = [			                
			    3,    // first 3 columns are not in a group    
			    {name: nls.monitoring.grid.totalMessages, children: 4, id: "totalMessages"},
			    {name: nls.monitoring.grid.bufferedMessages, children: 5, id: "bufferedMessages"}			    
			];
			var headerGroupMap = {
					queueName: 0, numProd: 0, numCons: 0,
					totMsgProd: "totalMessages", totMsgCons: "totalMessages", totMsgRej: "totalMessages", totMsgExp: "totalMessages",
					maxMsgs: "bufferedMessages", msgBuf: "bufferedMessages", perMsgBuf: "bufferedMessages", msgBufHWM: "bufferedMessages", msgBufHWMPercent: "bufferedMessages"
				};

			this.grid = GridFactory.createGrid(gridId, "queueMonitorGridContent", this.store, structure, 
					{ filter: true, paging: true, headerGroups: headerGroups, actionsMenuTitle: nls_strings.action.Actions, 
					  heightTrigger: 0, headerGroupMap: headerGroupMap });
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
			if (this.qmg_form.validate()) {
				this._updateStore();
			}
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
