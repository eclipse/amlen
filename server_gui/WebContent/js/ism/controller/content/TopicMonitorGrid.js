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
		'dojo/aspect',
		'dojo/query',
		'dojo/on',
		'dojo/when',
		'dojo/number',
		'dojo/store/Memory',
		'dojo/data/ItemFileWriteStore',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/layout/ContentPane',
		'dijit/form/Button',
		'dojo/i18n!ism/nls/monitoring',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/widgets/Select',
		'ism/widgets/_TemplatedContent',
		'ism/controller/content/TopicMonitorDialog',
		'dojo/text!ism/controller/content/templates/TopicMonitorGrid.html',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/DeferredList',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, array, domClass, domConstruct, aspect, query, on, when, number, Memory, 
				ItemFileWriteStore, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, 
				ContentPane, Button, nls, GridFactory, Dialog, Select, _TemplatedContent, 
				TopicMonitorDialog, gridtemplate, Resources, Utils, MonitoringUtils, DeferredList, nls_strings) {

	return declare("ism.controller.content.TopicMonitorGrid", _TemplatedContent, {

		nls: nls,
		templateString: gridtemplate,
		
		addDialog: null,
		removeDialog: null,

		grid: null,
		store: null,

		addButton: null,
		deleteButton: null,

		restURI: "rest/config/topicMonitors",
		
		domainUid: 0,

		contentId: "topicMonitorGridContent",
		//_setDefault: true,     // tmp variable to override the Metric field with a default value upon initialization
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
			this.store = new Memory({data:[]});			
		},
		
		postCreate: function() {		
            
            this.domainUid = ism.user.getDomainUid();

		    console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.monitoring.nav.topicMonitor.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this.updateGrid();
				this._checkForDiscardOldestMessages();
			}));	

			this._checkForDiscardOldestMessages();
			this.createGrid();
			this._updateStore();
			
			// if we can't configure topic monitors, replace the tagline on page and don't set up
			// the dialogs or the toolbar/dialog events
			if (!ism.user.hasPermission(Resources.actions.configureTopicMonitor)) {
				dojo.byId("mtm_tagline").innerHTML = nls.monitoring.topicMonitor.taglineNoConfigure;
			} else {
				this._initDialogs();
				this._initEvents();				
			}

		},
		
		// if any endpoints in this domain are using a policy with discardOldestMessages, then show a message about it
		// if we cannot get them for any reason, don't worry about it
		_checkForDiscardOldestMessages: function () {
            var adminEndpoint = ism.user.getAdminEndpoint();
            var endpointQuery = adminEndpoint.doRequest("/configuration/Endpoint", adminEndpoint.getDefaultGetOptions());
            //var policyQuery = adminEndpoint.doRequest("/configuration/MessagingPolicy", adminEndpoint.getDefaultGetOptions());
            //adminEndpoint.getAllQueryResults([endpointQuery, policyQuery], ["Endpoint", "MessagingPolicy"], function(queryResults) {
            var topicpolicyQuery = adminEndpoint.doRequest("/configuration/TopicPolicy", adminEndpoint.getDefaultGetOptions());
            var subscriptionpolicyQuery = adminEndpoint.doRequest("/configuration/SubscriptionPolicy", adminEndpoint.getDefaultGetOptions());
            adminEndpoint.getAllQueryResults([endpointQuery, topicpolicyQuery, subscriptionpolicyQuery], ["Endpoint", "TopicPolicy", "SubscriptionPolicy"], function(queryResults) {
                var discardOldestMessagesInForce = false;
                var endpoints = queryResults.Endpoint;
                var topicpolicies = queryResults.TopicPolicy;
                var subscriptionpolicies = queryResults.SubscriptionPolicy;

                // add each policy with MaxMessagesBehavior == DiscardOldestMessages to an array
                var policiesWithDiscardOldestMessages = dojo.map(topicpolicies, function(policy) {
                    if (policy.MaxMessagesBehavior === "DiscardOldMessages") {
                        return policy.Name;
                    }
                });
                // if there are some, check to see if they are used in an enabled endpoint
                if (policiesWithDiscardOldestMessages.length > 0) {
                    for (var i = 0, len=endpoints.length; i < len && !discardOldestMessagesInForce; i++) {
                        var endpoint = endpoints[i];
                        var enabled = false;
                        if (endpoint.Enabled === true || endpoint.Enabled === false) {
                            enabled = endpoint.Enabled;
                        } else if ( endpoint.Enabled.toLowerCase() == "true") {
                            enabled = true;
                        }
                        if (enabled && endpoint.TopicPolicies) {
                            // check the policies
                            var policiesInUse = endpoint.TopicPolicies.split(",");
                            for (var j = 0, numPoliciesInUse=policiesInUse.length; j < numPoliciesInUse; j++)  {
                                if (policiesWithDiscardOldestMessages.indexOf(policiesInUse[j]) >= 0) {
                                    discardOldestMessagesInForce = true;
                                    break;
                                }
                            }
                        }
                    }
                }
                if (!discardOldestMessagesInForce) {
                	policiesWithDiscardOldestMessages = dojo.map(subscriptionpolicies, function(policy) {
                        if (policy.MaxMessagesBehavior === "DiscardOldMessages") {
                            return policy.Name;
                        }
                    });
                    // if there are some, check to see if they are used in an enabled endpoint
                    if (policiesWithDiscardOldestMessages.length > 0) {
                        for (i = 0, len=endpoints.length; i < len && !discardOldestMessagesInForce; i++) {
                            endpoint = endpoints[i];
                            enabled = false;
                            if (endpoint.Enabled === true || endpoint.Enabled === false) {
                                enabled = endpoint.Enabled;
                            } else if ( endpoint.Enabled.toLowerCase() == "true") {
                                enabled = true;
                            }
                            if (enabled && endpoint.SubscriptionPolicies) {
                                // check the policies
                                policiesInUse = endpoint.SubscriptionPolicies.split(",");
                                for (j = 0, numPoliciesInUse=policiesInUse.length; j < numPoliciesInUse; j++)  {
                                    if (policiesWithDiscardOldestMessages.indexOf(policiesInUse[j]) >= 0) {
                                        discardOldestMessagesInForce = true;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }
                // show or hide the message
                if (discardOldestMessagesInForce) {
                    this.discardOldestMessagesInfo.style.display = 'block';
                } else {
                    this.discardOldestMessagesInfo.style.display = 'none';
                }               
            });			
		},

		_updateStore: function() {
			dojo.style(dojo.byId('tmg_refreshingLabel'), "display", "block");
			var topicString = "*";  
			var view = "PublishedMsgsHighest"; // the monitoring service complains if we don't send this
			var results = "All";

			var deferred = MonitoringUtils.getTopicData(topicString, view, results);
			
			deferred.then(lang.hitch(this, function(data) {
				console.debug(data);
				// Add 'id' field and throughput
				var mapped = array.map(data.Topic, function(item, index){
					var record = {};
					record.id = 1+index;
					record.topicSubtree = item.TopicString;
					record.msgPub = number.parse(item.PublishedMsgs);
					record.rejMsg = number.parse(item.RejectedMsgs);
					record.rejPub = number.parse(item.FailedPublishes);
					record.numSubs = number.parse(item.Subscriptions);
					return record;
				});
				
				console.log("creating grid memory with:", mapped);
				this.store = new Memory({data: mapped});
				this.grid.setStore(this.store);
				this.triggerListeners();
				this.lastUpdateTime.innerHTML = "<b>" + Utils.getFormattedDateTime() + "</b>";
				Utils.flashElement(this.lastUpdateTime.id);
				dojo.style(dojo.byId('tmg_refreshingLabel'), "display", "none");
				this._refreshGrid();
			}), function(error) {
				console.debug("Error from getTopicData: "+error);
				dojo.style(dojo.byId('tmg_refreshingLabel'), "display", "none");
				
				MonitoringUtils.showMonitoringPageError(error);
				return;
			});
		},
		
		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar
			
			var itemTooltip = nls.monitoring.dialog.topicStringTooltip;
			
			// create Add dialog
			domConstruct.place("<div id='addTopicMonitorDialog'></div>", this.contentId);
			var addTitle = nls.monitoring.dialog.addTopicMonitorTitle;
			var addId = "addTopicMonitorDialog";
			this.addDialog = new TopicMonitorDialog({dialogId: addId,
													 title: addTitle,
													 instruction: nls.monitoring.dialog.addTopicMonitorInstruction,
													 add: true});
			this.addDialog.startup();

			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeTopicMonitorDialog'></div>", this.contentId);
			var dialogId = "removeTopicMonitorDialog";
			var title = nls.monitoring.dialog.removeTopicMonitorTitle;
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + nls.monitoring.removeDialog.content + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_button_ok",
						label: nls.monitoring.removeDialog.deleteButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.monitoring.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.save = lang.hitch(this, function(topicString, onFinish, onError) { 
				if (!onError) { onError = lang.hitch(this,function(error) {	
					this.removeDialog.showErrorFromPromise(nls.monitoring.deletingFailed, error);
					});
				}
                var adminEndpoint = ism.user.getAdminEndpoint();
                var promise = adminEndpoint.doRequest("/configuration/TopicMonitor/"+ encodeURIComponent(topicString),adminEndpoint.getDefaultDeleteOptions());
                promise.then(onFinish,onError);
			});
		},
		
		createGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = "topicMonitorGrid";
			if (dijit.byId(gridId)) {
				// destroy the grid first, because it exists
				domConstruct.destroy(gridId);
				dijit.registry.remove(gridId);
			}

			var structure = [
				{	id: "topicSubtree", name: nls.monitoring.grid.topic.topicSubtree, tooltip: nls.monitoring.grid.topic.topicSubtreeTooltip, width: "150px", 
					decorator: function(name) {
						var sanitized = Utils.nameDecorator(name);
						return sanitized;
					}
				},
				{	id: "msgPub", name: nls.monitoring.grid.topic.msgPub, tooltip: nls.monitoring.grid.topic.msgPubTooltip, width: "140px", decorator: Utils.integerDecorator },
				{	id: "rejMsg", name: nls.monitoring.grid.topic.rejMsg, tooltip: nls.monitoring.grid.topic.rejMsgTooltip, width: "140px", decorator: Utils.integerDecorator },
				{	id: "rejPub", name: nls.monitoring.grid.topic.rejPub, tooltip: nls.monitoring.grid.topic.rejPubTooltip, width: "120px", decorator: Utils.integerDecorator },
				{	id: "numSubs", name: nls.monitoring.grid.topic.numSubs, tooltip: nls.monitoring.grid.topic.numSubsTooltip, width: "120px", decorator: Utils.integerDecorator }
			];
			
			var title = ism.user.hasPermission(Resources.actions.configureTopicMonitor) ? undefined :  nls_strings.action.Actions;
			
			this.grid = GridFactory.createGrid(gridId, "topicMonitorGridContent", this.store, structure, 
					{ filter: true, paging: true, actionsMenuTitle: title });
			
			if (ism.user.hasPermission(Resources.actions.configureTopicMonitor)) {
				this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
				this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
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
			this._updateButtons();
		},
		
		updateGrid: function() {
			console.debug(this.declaredClass + ".updateGrid()");
			this._updateStore();
		},

		_updateButtons: function() {
			if (this.deleteButton) {
				console.log("selected: "+this.grid.select.row.getSelected());
				if (this.grid.select.row.getSelected().length > 0) {
					var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
					console.debug("index="+ind);
					console.log("row count: "+this.grid.rowCount());
					this.deleteButton.set('disabled',false);
				} else {
					this.deleteButton.set('disabled',true);
				}
			}
		},
		
		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				this._updateButtons();
			}));

				on(this.addButton, "click", lang.hitch(this, function() {
				    console.log("clicked add button");
					// create array of existing names so we don't clash
					var existingNames = this.store.query({}).map(
							function(item){ return item.topicSubtree; });
				    this.addDialog.show(null, existingNames);
				}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var topicMonitor = this._getSelectedData();
				//this.removeDialog.recordToRemove = this._getSelectedData()["id"];
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				this.removeDialog.show();
			}));
					
			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function() {
				// any message on this topic means that we clicked "save"
				if (dijit.byId(this.addDialog.dialogId+"_form").validate()) {
					var bar = query(".idxDialogActionBarLead",this.addDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.monitoring.savingProgress;
					this.addDialog.save(lang.hitch(this, function(data) {
						this.addDialog.dialog.hide();
						this.updateGrid();
					}));
				}
			}));

			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function() {
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.monitoring.deletingProgress;
				var topicMonitor = this._getSelectedData();
				console.log("deleting " + topicMonitor.topicSubtree);
				this.removeDialog.save(topicMonitor.topicSubtree, lang.hitch(this, function() {
					this.updateGrid();
					this.removeDialog.hide();					
					bar.innerHTML = "";
				}));
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
