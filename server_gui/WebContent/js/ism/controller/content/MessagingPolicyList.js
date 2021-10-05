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
		'dojo/query',
		'dojo/aspect',
		'dojo/on',
		'dojo/dom-construct',
		'dojo/store/Memory',
		'dojo/number',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'ism/widgets/GridFactory',
		'ism/IsmUtils',		
		'ism/controller/content/MessagingPolicyDialog',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'ism/config/Resources',
		'ism/widgets/MessageHubAssociationControl',
		'ism/widgets/IsmQueryEngine',
		'dojo/keys',
		'ism/widgets/NoticeIcon',
		'idx/string',
		'ism/widgets/ToggleContainer'
		], function(declare, lang, array, query, aspect, on, domConstruct, Memory, number, _Widget, _TemplatedMixin,
				Button, Dialog, GridFactory, Utils, MessagingPolicyDialog, nls, nls_strings, 
				Resources, MessageHubAssociationControl, IsmQueryEngine, keys, NoticeIcon, iString, ToggleContainer) {

	/*
	 * This file defines a configuration widget that displays the list of message hubs
	 */

	var MessagingPolicyList = declare ("ism.controller.content.MessagingPolicyList", [_Widget, _TemplatedMixin], {
		nls: nls,
		contentId: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug("constructing MessagingPolicyList");
			this.contentId = this.id + "Content";
		},
		
		startup: function() {
			this.inherited(arguments);			

			// start with an empty <div> for the content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";			
		}
	});
	
	var MPContainer = declare("ism.controller.content.MessagingPolicyList.MPContainer",[ToggleContainer], {
		content: null,
		constructor: function(args) {
			dojo.safeMixin(this, args);
		},
		
		toggle: function() {
			this.inherited(arguments);
			if (this.get("open")) {
				this.list._refreshGrid();
			}
		}		
	});

	var MPList = declare ("ism.controller.content.MessagingPolicyList.MPList", [_Widget, _TemplatedMixin/*, ToggleContainer*/], {

		nls: nls,
		paneId: null,
		store: null,
		queryEngine: IsmQueryEngine,
		editDialog: null,
		removeDialog: null,
		messageHubItem: null,
		contentId: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		existingNames: [],
		detailsPage: null,

		addButton: null,
		deleteButton: null,
		editButton: null,
		
		domainUid: 0,
		
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug("constructing MessagingPolicyList ("+this.id+")");
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], idProperty: "id", queryEngine: this.queryEngine});	
		},
		
		startup: function() {
			this.inherited(arguments);
            
            this.domainUid = ism.user.getDomainUid();
			
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;			

			// start with an empty <div> for the content	
			this.mainNode.innerHTML = "";

			var nodeText = [
				"<span class='tagline'>" + this.tagline + "</span><br />",
				"<div id='"+this.contentId+"'></div>"
			].join("\n");
			domConstruct.place(nodeText, this.mainNode);

			this._initGrid();
			this._initStore(false);
			this._initDialogs();
			this._initEvents();
		},

		_setMessageHubItemAttr: function(value) {
			var currentName = this.messageHubItem ? this.messageHubItem.Name : null;
			this.messageHubItem = value;
			if (currentName && currentName != value.Name) {
				// we have a new item, we need to refresh the grid's store
				this._initStore(true);
			}
		},
		
		_setDetailsPageAttr: function(value) {
			this.detailsPage = value;
			this.detailsPage[this.policyType+"Policies"] = this.store;
		},
				
		_initStore: function(reInit) {
			console.debug("initStore for MessagingPolicies");	
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
			
			var _this = this;
			if (!_this.detailsPage.stores.messagingPolicies) {
				_this.detailsPage.stores.messagingPolicies = [];
			}
			
			// we want to show any messaging policies who have any endpoint that is in the message hub 
			// or any messaging policies with no endpoints
			
			// kick off the queries
			var endpointResults = this.detailsPage.promises.endpoints;
			var messagingPolicyResults = this.detailsPage.promises[this.policyType + "Policies"];

            var adminEndpoint = ism.user.getAdminEndpoint();
            adminEndpoint.getAllQueryResults([endpointResults, messagingPolicyResults], ["Endpoint", _this.policyType+"Policy"], function(queryResults) {
                _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
                var allEndpoints = queryResults.Endpoint; // this is all of them, just need the ones for this message hub
                var hubPolicyNames = [];    
                var hubPolicyRefs = [];
                var all = "";
                for (var j=0, numEndpoints=allEndpoints.length; j < numEndpoints; j++) {
                	var policyList = allEndpoints[j][_this.policyType + "Policies"];                  
                    if (policyList && policyList !== "") {
                        all += policyList + ",";
                        var policies = policyList.split(',');
                        if (allEndpoints[j]['MessageHub'] == _this.messageHubItem.Name) {
                            for (var l=0, numPolicies=policies.length; l < numPolicies; l++) {
                                var index = dojo.indexOf(hubPolicyNames, policies[l]);
                                var endpointName = allEndpoints[j].Name;
                                if (index < 0) {
                                    hubPolicyNames.push(policies[l]);
                                    hubPolicyRefs.push([endpointName]);
                                } else {                                
                                    hubPolicyRefs[index].push(endpointName);
                                }
                            }
                        }
                    }
                }

                var allReferredNames = all.split(',');  // every named messaging policy, may contain dupes
                
                // hubPolicyNames now contains a list of all securityPolicy names for the hub
                var allMessagingPolicies = queryResults[_this.policyType + "Policy"]; // this is all of them, just need to ones for this message hub
                var messagingPolicies = [];
                var mpIndex = 0;
                _this.existingNames = [];
                
                for (var i=0, numMessagingPolicies=allMessagingPolicies.length; i < numMessagingPolicies; i++) { 
                    Utils.convertStringsToBooleans(allMessagingPolicies[i],[{property: "DisconnectedClientNotification", defaultValue: false}]);
                    var hpRefsIndex = dojo.indexOf(hubPolicyNames, allMessagingPolicies[i].Name); 
                    allMessagingPolicies[i].associated = {checked: false, Name: allMessagingPolicies[i].Name, sortWeight: 1};                   
                    allMessagingPolicies[i].MaxMessages = allMessagingPolicies[i].MaxMessages === "" ? undefined : parseFloat(allMessagingPolicies[i].MaxMessages);
                    allMessagingPolicies[i].ActionListLocalized = _this._getTranslatedAuthString(allMessagingPolicies[i].ActionList);
                    var sortWeight = allMessagingPolicies[i].MaxMessages === undefined ? -1 : allMessagingPolicies[i].MaxMessages;
                    allMessagingPolicies[i].MaxMessagesColumn = {warnBehavior: allMessagingPolicies[i].MaxMessagesBehavior === "DiscardOldMessages", sortWeight: sortWeight};
                    sortWeight = -1;
                    if ( hpRefsIndex >= 0) {
                        var endpointArray = hubPolicyRefs[hpRefsIndex].sort();
                        allMessagingPolicies[i].endpoint = {list: endpointArray.join(","), Name: allMessagingPolicies[i].Name, 
                                sortWeight: endpointArray.length};  // add the endpoints to the object
                        allMessagingPolicies[i].endpointCount = endpointArray.length;
                        allMessagingPolicies[i].id = escape(allMessagingPolicies[i].Name+_this.policyType);
                        messagingPolicies[mpIndex] = allMessagingPolicies[i];
                        messagingPolicies[mpIndex].policyType = _this.policyType;
                        mpIndex++;
                    } else if (dojo.indexOf(allReferredNames, allMessagingPolicies[i].Name) < 0) {
                        // found an orphan
                        allMessagingPolicies[i].endpoint = {list: "", Name: allMessagingPolicies[i].Name, sortWeight: 0};
                        allMessagingPolicies[i].endpointCount = 0;
                        allMessagingPolicies[i].id = escape(allMessagingPolicies[i].Name+_this.policyType);
                        messagingPolicies[mpIndex] = allMessagingPolicies[i];
                        messagingPolicies[mpIndex].policyType = _this.policyType;
                        mpIndex++;
                    }
                    _this.existingNames[i] = allMessagingPolicies[i].Name;                                       
                }

                messagingPolicies.sort(function(a,b){
                    if (a.Name < b.Name) {
                        return -1;
                    }
                    return 1;
                });         
                
                if (_this.store.data.length === 0 && messagingPolicies.length === 0) {
                    _this.detailsPage.stores[_this.policyType+"Policies"] = _this.store;
                    // Add the store to the messagingPolicies store (containing all messaging policies)
                    _this.detailsPage.stores.messagingPolicies.push(_this.store);
                    return;
                }
                _this.store = new Memory({data: messagingPolicies, idProperty: "id", queryEngine: _this.queryEngine});
                _this.grid.setStore(_this.store);
                _this.triggerListeners();
                _this.detailsPage.stores[_this.policyType+"Policies"] = _this.store;
                // Add the policies to the messagingPolicies (containing all messaging policies)
                _this.detailsPage.stores.messagingPolicies.push(_this.store);
                if (reInit) {
                    _this.grid.resize();
                }                
            }, function(error) {
                _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
                Utils.showPageErrorFromPromise(nls.messaging.messagingPolicies.retrieveMessagingPolicyError, error);                
            });
		},
		
		_getTranslatedAuthString: function(authString) {
			
			// get the correct translated values for authority...
			
			// if empty or null return empty string
			authString = iString.nullTrim(authString);
			if (!authString) {
				return "";
			}
			
			// get translated value for each auth in list
			var authArray = authString.split(",");
			var translatedAuthString = "";
			for (var i=0, len=authArray.length; i < len; i++) {
				  var authValue = authArray[i].toLowerCase();
				  authValue = iString.nullTrim(authValue);
				  translatedAuthString = translatedAuthString + nls.messaging.messagingPolicies.dialog.actionOptions[authValue] + " ";
				
			}

			// clean up auth list and replace with localized separator
			translatedAuthString = iString.nullTrim(translatedAuthString);
			translatedAuthString = translatedAuthString.replace(/\s/g, nls.messaging.messagingPolicies.listSeparator);
			return translatedAuthString;
			
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the toolbar
			
			var protocolDeferrred = this.detailsPage.deferreds.protocols;

			// create Edit dialog
			var editId =  "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);
			var edit_dialogTitle = null;
			var add_dialogTitle = null;
			var dialogInstruction = null;
			var remove_title = null;
			var remove_content = null;
			var dialogURI = null;
			switch (this.policyType) {
			case "Topic":
				edit_dialogTitle = Utils.escapeApostrophe(nls.editTopicMPTitle);
				add_dialogTitle = Utils.escapeApostrophe(nls.addTopicMPTitle);
				remove_title = Utils.escapeApostrophe(nls.removeTopicMPTitle);
				remove_content = Utils.escapeApostrophe(nls.removeTopicMPContent);
				dialogInstruction = Utils.escapeApostrophe(nls.topicMPInstruction);
				break;
			case "Subscription":
				edit_dialogTitle = Utils.escapeApostrophe(nls.editSubscriptionMPTitle);
				add_dialogTitle = Utils.escapeApostrophe(nls.addSubscriptionMPTitle);
				remove_title = Utils.escapeApostrophe(nls.removeSubscriptionMPTitle);
				remove_content = Utils.escapeApostrophe(nls.removeSubscriptionMPContent);
				dialogInstruction = Utils.escapeApostrophe(nls.subscriptionMPInstruction);
				break;
			case "Queue":
				edit_dialogTitle = Utils.escapeApostrophe(nls.editQueueMPTitle);
				add_dialogTitle = Utils.escapeApostrophe(nls.addQueueMPTitle);
				remove_title = Utils.escapeApostrophe(nls.removeQueueMPTitle);
				remove_content = Utils.escapeApostrophe(nls.removeQueueMPContent);
				dialogInstruction = Utils.escapeApostrophe(nls.queueMPInstruction);
				break;
			default:
				break;
			}
			this.editDialog = new MessagingPolicyDialog({dialogId: editId,
				                                  type: this.policyType,
				                      			  destinationLabel: nls["policyTypeName_" + this.policyType.toLowerCase()],
				                      			  destinationTooltip: nls["policyTypeTooltip_" + this.policyType.toLowerCase()],
												  dialogTitle: edit_dialogTitle,
												  dialogInstruction: dialogInstruction,
				                      			  destinationLabelWidth: "131px",
				                      			  actionLabelWidth: "120px",
												  protocolDeferrred:protocolDeferrred, add: false}, editId);
			this.editDialog.set("messageHubItem", this.messageHubItem);
			this.editDialog.startup();
			
			// create Add dialog
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new MessagingPolicyDialog({dialogId: addId,
				                                    type: this.policyType,
					                      			destinationLabel: nls["policyTypeName_" + this.policyType.toLowerCase()],
					                      			destinationTooltip: nls["policyTypeTooltip_" + this.policyType.toLowerCase()],
									                dialogTitle: add_dialogTitle,
									                dialogInstruction: dialogInstruction,
									                protocolDeferrred:protocolDeferrred, add: true}, addId);
			this.addDialog.set("messageHubItem", this.messageHubItem);
			this.addDialog.startup();			
			
			// create Remove dialog (confirmation)
			var dialogId = "removeMessagingPolicyDialog"+this.paneId;
			domConstruct.place("<div id=" + dialogId + "></div>", this.paneId);			

			this.removeDialog = new Dialog({
				title: remove_title,
				type: this.policyType,
				content: "<div>" + remove_content + "</div>",
				style: 'width: 600px;',
				buttons: [
				          new Button({
				        	  id: dialogId + "_saveButton",
				        	  label: nls_strings.action.Ok,
				        	  onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
				          })
				          ],
				          closeButtonLabel: nls_strings.action.Cancel
			}, "removeMessagingPolicyDialog");
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.save = lang.hitch(this, function(messagingPolicy, onFinish, onError) {
				
				var messagingPolicyName = messagingPolicy.Name;
				
				if (!onFinish) { onFinish = function() {this.removeDialog.hide();}; }
				if (!onError) { onError = lang.hitch(this,function(error) {	
					var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
					bar.innerHTML = "";
					this.removeDialog.showErrorFromPromise(nls.messaging.messagingPolicies.deleteMessagingPolicyError, error);
					});
				}
				
				console.debug("Deleting: ", messagingPolicyName);
				var endpoint = ism.user.getAdminEndpoint();
	            var options = endpoint.getDefaultDeleteOptions();
	            var promise = endpoint.doRequest("/configuration/"+this.policyType+"Policy/" + encodeURIComponent(messagingPolicyName), options);
				promise.then(onFinish, onError);
			});
			
			// create Remove not allowed dialog (remove not allowed when there are endpoints that reference the policy)
			var removeNotAllowedId = this.id+"_removeNotAllowedDialog";
			domConstruct.place("<div id='"+removeNotAllowedId+"'></div>", this.id);
			this.removeNotAllowedDialog = new Dialog({
				id: removeNotAllowedId,
				title: Utils.escapeApostrophe(nls.messaging.messagingPolicies.removeNotAllowedDialog.title),
				content: "<div style='display: table;'>" +
						"<img style='display: inline-block; float: left; margin: 0 10px 0 0;' src='css/images/warning24.png' alt='' />" +
						"<span style='display: table-cell; vertical-align:middle; word-wrap:break-word;'>" + 
						Utils.escapeApostrophe(lang.replace(nls.messaging.messagingPolicies.removeNotAllowedDialog.content,[nls["policyTypeShortName_" + this.policyType.toLowerCase()]])) + "</span><div>",
				closeButtonLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.removeNotAllowedDialog.closeButton)
			}, removeNotAllowedId);
			this.removeNotAllowedDialog.dialogId = removeNotAllowedId;			
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.messaging.messageHubs.messagingPolicyList.name, field: "Name", dataType: "string", width: "160px",
						decorator: Utils.nameDecorator
					},
					{	id: "endpoint", name: nls.messaging.messageHubs.messagingPolicyList.metricLabel, field: "endpoint", filterable: false, dataType: "number", width: "80px",
						widgetsInCell: true,
						decorator: function() {	
								return "<div data-dojo-type='ism.widgets.MessageHubAssociationControl' data-dojo-props='associationType: \"endpoints\"' class='gridxHasGridCellValue'></div>";
						}
					},
					{   id: "MaxMessages", name: nls.messaging.messageHubs.messagingPolicyList.maxMessagesLabel, field: "MaxMessagesColumn", dataType: "string", width: "90px",
					    widgetsInCell: true,
					    decorator: function() {
					    	var template = 
					    		"<div class='gridxHasGridCellValue'>"+
			    		            "<div style='display: none; float: right; padding-left: 5px;' data-dojo-type='ism.widgets.NoticeIcon' data-dojo-attach-point='maxMessagesBehaviorNoticeIcon'" +
			    		            	"data-dojo-props='hoverHelpText: \"" + Utils.escapeApostrophe(nls.messaging.messagingPolicies.tooltip.discardOldestMessages) + 
			    		            	"\", iconClass: \"noticeIcon\"'></div>"+			    		            
			    		            "<span style='float: right;' data-dojo-attach-point='maxMessagesValue'></span>" +
				    		     "</div>";
				    		return template;
					    },
					    setCellValue: function(gridData, storeData, cellWidget){				    	
					    	// set the max messages value
                            var value = gridData === undefined ? "" : gridData;
					    	cellWidget.maxMessagesValue.innerHTML = value;

                            // hide or show the icon
					    	if (storeData && storeData.warnBehavior === true && value !== "") {
					    		cellWidget.maxMessagesBehaviorNoticeIcon.domNode.style.display = 'inline';
					    		cellWidget.maxMessagesValue.style.paddingRight = '';					    			
					    	} else {
					    		cellWidget.maxMessagesBehaviorNoticeIcon.domNode.style.display = 'none';
					    		cellWidget.maxMessagesValue.style.paddingRight = '24px';					    			
					    	}
					    },
						formatter: function(data) {
							if (data.MaxMessages) {
								return number.format(data.MaxMessages, {places: 0, locale: ism.user.localeInfo.localeString});
							}
						}
					},
					{   id: "DisconnectedClientNotification", name: nls.messaging.messageHubs.messagingPolicyList.disconnectedClientNotificationLabel, field: "DisconnectedClientNotification", dataType: "string", width: "90px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.DisconnectedClientNotification);
                        }                         
					},					
					{	id: "ActionListLocalized", name: nls.messaging.messageHubs.messagingPolicyList.actionsLabel, field: "ActionListLocalized", dataType: "string", width: "100px" },					
					{	id: "Destination", name: this.destTitle, field: this.policyType, dataType: "string", width: "150px",
                        formatter: function(data) {
                            return data[data.policyType];
                        },
						decorator: Utils.nameDecorator
					},					
					{	id: "Description", name: nls.messaging.messageHubs.messagingPolicyList.description, field: "Description", dataType: "string", width: "150px",
						decorator: Utils.textDecorator
					}					
				];
			var presentationColumns = [];
			if (this.removeColumns && this.removeColumns.length > 0) {
				var colidx = 0;
				for (var i = 0; i < structure.length; i++) {
					// If the current ID is not found in the removeColumns
					// list, store it in the presenationColumns list
					if (this.removeColumns.indexOf(structure[i].id) < 0) {
						presentationColumns[colidx] = structure[i];
						colidx++;
					}
				}
			}
			else {
				presentationColumns = structure;
			}
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, presentationColumns, {paging: true, filter: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);		
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh().then(lang.hitch(this, function() {
				this.grid.resize();
			}));

		},
		

		_initEvents: function() {
			// summary:
			// 		init events

			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    this.addDialog.show(null, this.existingNames);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var policy = this.store.query({id: this.grid.select.row.getSelected()[0]})[0];
				var numEndpoints = policy.endpointCount;
				if (numEndpoints !== 0) {
					this.removeNotAllowedDialog.show();
				} else {
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.show();
				}
			}));
					
			on(this.editButton, "click", lang.hitch(this, function() {
				console.debug("clicked edit button");
				this._doEdit();
			}));
			
			aspect.after(this.grid, "onRowDblClick", lang.hitch(this, function(event) {
				// if the edit button is enabled, pretend it was clicked
				if (!this.editButton.get('disabled')) {
					this._doEdit();
				}
			}));
			
		    aspect.after(this.grid, "onRowKeyPress", lang.hitch(this, function(event) {
		    	// if the enter key was pressed on a row and the edit button is enabled, 
		    	// pretend it was clicked				
				if (event.keyCode == keys.ENTER && !this.editButton.get('disabled')) {
					this._doEdit();
				} else if (event.charCode == keys.SPACE) {
					this._doSelect();
				}
			}), true);
				
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				if (dijit.byId(this.editDialog.dialogId+"_DialogForm").validate()) {
					var id = this.grid.select.row.getSelected()[0];
					var policy = this.store.query({id: id})[0];
					var policyName = policy.Name;
					this.editDialog.save(lang.hitch(this, function(data, count) {
						console.debug("save is done, returned:", data);
						var _this = this;
						// need to get the number of endpoints
						data.endpoint = policy.endpoint;
						data.endpointCount = policy.endpointCount;
						data.id = escape(data.Name+_this.policyType);
						data.policyType = _this.policyType;
                        data.MaxMessages = data.MaxMessages === "" ? undefined : parseFloat(data.MaxMessages);
                        data.ActionListLocalized = this._getTranslatedAuthString(data.ActionList);
                        var sortWeight = data.MaxMessages === undefined ? -1 : data.MaxMessages;
                        data.MaxMessagesColumn = {warnBehavior: data.MaxMessagesBehavior === "DiscardOldMessages", sortWeight: sortWeight};
						data.associated = {checked: false, Name: data.Name, sortWeight: 1};
						if (policyName != data.Name) {
							this.store.remove(id);
							this.store.add(data);
							var index = dojo.indexOf(this.existingNames, policyName);
							if (index >=0) {
								this.existingNames[index] = data.Name;
							}						
						} else {
							this.store.put(data, {overwrite: true});
						}
						this.triggerListeners();
						
						this.editDialog.dialog.hide();
					}));
				}
			}));
			
			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
					console.debug("saving addDialog");
					var _this = this;
					if (dijit.byId(this.addDialog.dialogId+"_DialogForm").validate()) {		
						this.addDialog.save(lang.hitch(this, function(data) {
							console.debug("add is done, returned:", data);
							data.endpoint = {list: "", Name: data.Name, sortWeight: 0, zeroOK: false};
							data.endpointCount = 0;
							data.id = escape(data.Name+_this.policyType);
							data.policyType = _this.policyType;
							data.MaxMessages = data.MaxMessages === "" ? undefined : parseFloat(data.MaxMessages);
							data.ActionListLocalized = this._getTranslatedAuthString(data.ActionList);
							var sortWeight = data.MaxMessages === undefined ? -1 : data.MaxMessages;
                            data.MaxMessagesColumn = {warnBehavior: data.MaxMessagesBehavior === "DiscardOldMessages", sortWeight: sortWeight};                           
							data.associated = {checked: false, Name: data.Name, sortWeight: 1};
							this.store.add(data);
							this.triggerListeners();
							this.existingNames.push(data.Name);												
							this.addDialog.dialog.hide();
						}));
					}
				
			}));

			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var messagingPolicy = this.store.query({id: id})[0];				
				var messagingPolicyName = messagingPolicy.Name;
				console.log("deleting " + messagingPolicyName);
				this.removeDialog.save(messagingPolicy, lang.hitch(this, function(data) {
					this.removeDialog.hide();
					bar.innerHTML = "";						
					this.store.remove(id);
					var index = dojo.indexOf(this.existingNames, messagingPolicyName);
					if (index >= 0) {
						this.existingNames.splice(index, 1);
					}					
					if (this.store.data.length === 0 ||
							this.grid.select.row.getSelected().length === 0) {
						this.deleteButton.set('disabled',true);
						this.editButton.set('disabled',true);							
					}	
				}), lang.hitch(this, function(error) {
					var code = Utils.getErrorCodeFromPromise(error);
					bar.innerHTML = "";
					this.removeDialog.showErrorFromPromise(nls.messaging.messagingPolicies.deleteMessagingPolicyError, error);
				}));

			}));
			
			dojo.subscribe("EndpointRemoved", lang.hitch(this, function(message) {
				 var endpointRemoved = message.Name;
				 var currentEntries = this.store.query();
				 for (var i=0, numEntries=currentEntries.length; i < numEntries; i++) {
					 var item = currentEntries[i];
					 var endpoints = item.endpoint.list.split(",");
					 var index = array.indexOf(endpoints, endpointRemoved); 
					 if ( index >= 0) {
						 endpoints.splice(index, 1);
						 item.endpointCount = endpoints.length;
						 item.endpoint = {list: endpoints.join(","), Name: item.Name, sortWeight: endpoints.length, zeroOK: false};
						 this.store.put(item);
					 }
				 }
				 this.triggerListeners();
				 
			}));

			dojo.subscribe("EndpointChanged", lang.hitch(this, function(message) {
				 var oldPolicies = message.oldValue[this.policyType +"Policies"];
				 var newPolicies = message.newValue[this.policyType +"Policies"];
				 if (oldPolicies == newPolicies) {
					 return;
				 }
				 if (oldPolicies) {
					 this._removeEndpointFromPolicies(message.oldValue);
				 }
				 if (newPolicies) {
					 this._addEndpointToPolicies(message.newValue);
				 }
				 this.triggerListeners();
			}));
			
			dojo.subscribe("EndpointAdded", lang.hitch(this, function(message) {				
				if (message[this.policyType + "Policies"]) {
					this._addEndpointToPolicies(message);
					 this.triggerListeners();
				}
			}));
			
			dijit.byId("ismMHTabContainer").watch("selectedChildWidget", lang.hitch(this, function(Name, origVal, newVal) {
				if (newVal.id == Resources.pages.messaging.messageHubTabs.messagingPolicies.id) {
					this.grid.resize();
				}
			}));

		},
		
		_doSelect: function() {
			var selection = this.grid.select.row.getSelected(); 
			if (selection.length > 0) {
				var id = selection[0];
				var messagingPolicy = this.store.query({id: id})[0];				
				this.deleteButton.set('disabled',false);
				this.editButton.set('disabled',false);
			} else {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);
			}
		},
		
		_doEdit: function() {
			var id = this.grid.select.row.getSelected()[0];
			var messagingPolicy = this.store.query({id: id})[0];
			var messagingPolicyName = messagingPolicy.Name;
			
			// retrieve a fresh copy of the data so we have a new use count and Status
            var adminEndpoint = ism.user.getAdminEndpoint();            
            var promise = adminEndpoint.doRequest("/configuration/"+this.policyType+"Policy/"+encodeURIComponent(messagingPolicyName), adminEndpoint.getDefaultGetOptions()); 
			// create array of existing names except for the current one, so we don't clash
			var names = array.map(this.existingNames, function(item) { 
				if (item != messagingPolicyName) return item; 
			});
			promise.then(lang.hitch(this,function(data) {
			    var policy = adminEndpoint.getNamedObject(data, this.policyType + "Policy", messagingPolicyName);
			    Utils.convertStringsToBooleans(policy, [{property: "DisconnectedClientNotification", defaultValue: false}]);
				this.editDialog.show(policy, names);
			}), function(error) {
                Utils.showPageErrorFromPromise(nls.messaging.messagingPolicies.retrieveOneMessagingPolicyError, error);			    
			});
		},
		
		_removeEndpointFromPolicies: function(endpoint) {
			var endpointName = endpoint.Name;
			var policies = endpoint[this.policyType + "Policies"].split(',');
			for (var i=0, len=policies.length; i < len; i++) {
				var results = this.store.query({Name: policies[i]});
				if (!results || results.length === 0 || !results[0].endpoint) {
					continue;
				}
				var item = results[0];
				var endpoints = item.endpoint.list.split(",");
				var index = array.indexOf(endpoints, endpointName); 
				if ( index >= 0) {
					endpoints.splice(index, 1);
					item.endpointCount = endpoints.length;
					item.endpoint = {list: endpoints.join(","), Name: item.Name, sortWeight: endpoints.length, zeroOK: false};
					this.store.put(item);
				}
			}			
		},

		_addEndpointToPolicies: function(endpoint) {
			var endpointName = endpoint.Name;
			var policies = endpoint[this.policyType + "Policies"].split(',');
			for (var i=0, len=policies.length; i < len; i++) {
				var results = this.store.query({Name: policies[i]});
				if (!results) {
					continue;
				}
				var item = results[0];
				var endpoints = [];
				if (item.endpoint && item.endpoint.list !== "") {
					endpoints = item.endpoint.list.split(",");
				}
				endpoints.push(endpointName);
				endpoints.sort();
				item.endpoint = {list: endpoints.join(","), Name: item.Name, sortWeight: endpoints.length, zeroOK: false};
				item.endpointCount = endpoints.length;
				this.store.put(item);
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
			// re-sort
			if (this.grid.sort) {
				var sortData = this.grid.sort.getSortData();
				if (sortData && sortData !== []) {
					this.grid.model.clearCache();
					this.grid.sort.sort(sortData);
				}
			}
		}
	});

	return MessagingPolicyList;
});
