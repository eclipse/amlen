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
        'dojo/i18n!ism/nls/messaging',
		'ism/widgets/Select',
		'ism/widgets/GridFactory',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/controller/content/templates/DestinationMappingRuleMonitorGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'idx/string',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, domClass, domConstruct, on, when, number, Memory, ItemFileWriteStore, manager, ContentPane, Button,
				Form, nls,msgNls, Select, GridFactory, _TemplatedContent, gridtemplate, Resources, Utils, MonitoringUtils, iStrings, nls_strings) {

	return declare("ism.controller.content.DestinationMappingRuleMonitorGrid", _TemplatedContent, {

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
			dojo.subscribe(Resources.pages.monitoring.nav.destinationMappingRuleMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
			}));	

			this._setupFields();
			this.createGrid();
			this._updateStore();
		},

		_updateStore: function() {
			dojo.style(dojo.byId('dmrg_refreshingLabel'), "display", "block");
			var name = this.ruleName.value;
			var ruleType = this.ruleCombo.value;
			var association = this.queueManagerName.value;
			var statType = this.datasetCombo.value;
			var numRecords = 100;
		
			var deferred = MonitoringUtils.getDestinationMappingData({
				RuleType: ruleType,
				Association: association,
				StatType: statType,
				Name: name,
				ResultCount: numRecords
			});
			
			deferred.then(lang.hitch(this, function(data) {
				if (data.error) {
					console.debug("Error from getDestinationMappingData: "+data.error);
					dojo.style(dojo.byId('dmrg_refreshingLabel'), "display", "none");
					MonitoringUtils.showMonitoringPageError(data.error);
					return;
				}
				
				// TODO: This is just a guess
				data = data.DestinationMappingRule;

				// Add 'id' field and throughput
				var mapped = array.map(data, function(item, index){
					var record = {};
					record.id = 1+index;
					record.ruleName = item.RuleName;
					record.qmConnection = item.QueueManagerConnection;
					record.commits = number.parse(item.CommitCount);
					record.rollbacks = number.parse(item.RollbackCount);
					record.committedMsgs = number.parse(item.CommittedMessageCount);
					record.persistentMsgs = number.parse(item.PersistentCount);
					record.nonPersistentMsgs = number.parse(item.NonpersistentCount);
					record.status =  number.parse(item.Status);
					record.expandedMsg = item.ExpandedMessage;
					return record;
				});
				
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
			    this.triggerListeners();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('dmrg_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(err) {
				console.debug("Error from getDestinationMappingData: "+err);
				dojo.style(dojo.byId('dmrg_refreshingLabel'), "display", "none");
				
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},
		
		_setupFields: function() {
			console.log(this.declaredClass + ".setupFields");
			
			for (var key in MonitoringUtils.Rules.RuleType) {
				this.ruleCombo.store.newItem({ 
					label: MonitoringUtils.Rules.RuleType[key].label, 
					name: MonitoringUtils.Rules.RuleType[key].name 
				});
			}
			this.ruleCombo.store.save();
			
			// Populate datasets combo
			for (var view in MonitoringUtils.Rules.View) {
				this.datasetCombo.store.newItem({ 
					label: MonitoringUtils.Rules.View[view].label, 
					name: MonitoringUtils.Rules.View[view].name 
				});
			}
			this.datasetCombo.store.save();
		},
		
		createGrid: function() {
			
			var _this = this;
			// summary:
			// 		create and instantiate the grid
			var gridId = "destinationRuleMappingMonitorGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			var structure = [
			    {	id: "ruleName", name: nls.monitoring.grid.destmapping.ruleName, tooltip: nls.monitoring.grid.destmapping.ruleNameTooltip, width: "150px", decorator: Utils.nameDecorator },
				{	id: "qmConnection", name: nls.monitoring.grid.destmapping.qmConnection, tooltip: nls.monitoring.grid.destmapping.qmConnectionTooltip, width: "150px", decorator: Utils.nameDecorator },
				{	id: "commits", name: nls.monitoring.grid.destmapping.commits, tooltip: nls.monitoring.grid.destmapping.commitsTooltip, width: "120px", decorator: Utils.integerDecorator },
				{	id: "rollbacks", name: nls.monitoring.grid.destmapping.rollbacks, tooltip: nls.monitoring.grid.destmapping.rollbacksTooltip, width: "120px", decorator: Utils.integerDecorator}, 
				{	id: "committedMsgs", name: nls.monitoring.grid.destmapping.committedMsgs, tooltip: nls.monitoring.grid.destmapping.committedMsgsTooltip, width: "120px", decorator: Utils.integerDecorator },
				{	id: "persistentMsgs", name: nls.monitoring.grid.destmapping.persistentMsgs, tooltip: nls.monitoring.grid.destmapping.persistentMsgsTooltip, width: "120px", decorator: Utils.integerDecorator },
				{	id: "nonPersistentMsgs", name: nls.monitoring.grid.destmapping.nonPersistentMsgs, tooltip: nls.monitoring.grid.destmapping.nonPersistentMsgsTooltip, width: "120px", decorator: Utils.integerDecorator },
				{	id: "status", name: nls.monitoring.grid.destmapping.status, tooltip: nls.monitoring.grid.destmapping.statusTooltip, width: "70px", 	
						decorator: function(cellData) {	
							var iconClass = null;
							if (cellData) {	
								if (iStrings.startsWith(cellData, msgNls.messaging.destinationMapping.status.active)) { // enabled
									iconClass = "ismIconEnabled";
								} else if (iStrings.startsWith(cellData, msgNls.messaging.destinationMapping.status.starting)) { // reconnecting or starting
									iconClass = "ismIconLoading";
								} else { // disabled
									iconClass = "ismIconDisabled";
								}
								return "<div class='"+iconClass+"' title='"+cellData+"'></div>";
							}
							return "";
			         	},
			         	formatter: function(cellData) {
							if (cellData !== null && cellData !== undefined) {	
								var altText = "";
								var status = cellData.status;
								if (status === 0) { // enabled
									altText = msgNls.messaging.destinationMapping.status.active;
								} else if ((status == 2) || (status == 3)) { // reconnecting or starting
									altText = msgNls.messaging.destinationMapping.status.starting;
								} else { // disabled
									altText = msgNls.messaging.destinationMapping.status.inactive;
								}
								
								if (cellData.expandedMsg) {
									altText = altText + " [" + cellData.expandedMsg + "]";
								}
								return altText;
							}
						return "";
			         }
				}
			];

			this.grid = GridFactory.createGrid(gridId, "destinationRuleMappingMonitorGridContent", this.store, structure, 
					{ filter: true, paging: true, heightTrigger: 0, actionsMenuTitle: nls_strings.action.Actions });
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
			if (this.dmrg_form.validate()) {
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
