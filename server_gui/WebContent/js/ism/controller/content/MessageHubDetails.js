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
define(['dojo/dom',
        'dojo/aspect',
        'dojo/_base/declare',
		'dojo/_base/lang',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',		
		'dojo/query',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dijit/layout/TabContainer',		
		'dojo/_base/array',
		'dojox/html/entities',
		'dojo/text!ism/controller/content/templates/MessageHubDetails.html',
		'dojo/i18n!ism/nls/messaging',
		'ism/config/Resources',
		'ism/IsmUtils',
		'ism/controller/content/MessageHubDialog',
		'ism/controller/content/MessagingPoliciesPane',
		'ism/controller/content/EndpointsPane',
		'ism/controller/content/ConnectionPoliciesPane'
		], function(dom, aspect, declare, lang, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, query, on, 
				    domClass, domConstruct, domStyle, TabContainer, array, entities, template, nls, 
				    Resources, Utils, MessageHubDialog) {
	
	/**
	 * The MessageHubDetails displays the tab container showing details for a specific message hub.
	 * The Name of the message hub should be on the url as the "Name" parameter.
	 */
	var MessageHubDetails = declare("ism.controller.content.MessageHubDetails", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		
		templateString: template,
		nls: nls,
		store: null,
		
		Name: "",
		displayName: "",
	 	Description: "",
	 	tabContainer: null,
	 	tabs: null,
	 	deferreds: {}, // handy place to hold deferred queries to the endpoints and policies
	 	promises: {}, // handy place to hold promises from queries to the endpoints and policies
	 	stores: {},  // handy place to hold pointers to the endpoints and policies
	 	messageHubItem: null,
	 	domainUid: 0,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
						
			this.setName();

			if (!this.Name) {
				//this.showMain();
				this.Name = '';
			}
			if (!this.Description) {
				this.Description = '';
			}
			
		},
		
		postCreate: function() {
			this.backLink = dojo.query("#backToMHMain");
			this.tabContainer = dijit.byId("ismMHTabContainer");
			
			on(this.backLink, "click", lang.hitch(this, function(evt) {	
	            // work around for IE event issue - stop default event
	            if (evt && evt.preventDefault) evt.preventDefault();			    
				console.debug("details back link clicked with currentHub " + this.Name);
				dojo.publish(Resources.pages.messaging.nav.messageHubs.topic, {page: "messageHubs", currentHub: this.messageHubItem});
			}));
			this.domainUid = ism.user.getDomainUid();
			
			this._initData();	
			this._initDialogs();
			this._initEvents();
		},
			
		_setNameAndDescription: function(messageHub) {
			this.Name = messageHub.Name;
			this.MHDetailsName.innerHTML = entities.encode(this.Name);				
			this.Description = messageHub.Description;
			if (!this.Description) {
				this.Description = "";
			}
			this.MHDetailsDescription.innerHTML = entities.encode(this.Description);
			
		},
		
		setName: function() {
			var vars = Utils.getUrlVars();
			this.Name = decodeURIComponent(vars["Name"]);
			this.displayName = this.Name;
		},
		
		editMessageHub: function(event) {
			// work around for IE event issue - stop default event
			if (event.preventDefault) event.preventDefault();
			
			var existingNames = dojo.map(this.existingNames, function(item){ if (item.Name != this.Name) return item.Name; });
			this.editDialog.show(this.messageHubItem, existingNames);
		},
		
		_initData: function() {						
			var _this = this;
			
			//  show a loading indicator
			var updatingDiv = dom.byId("mhDetailsUpdating");
			domStyle.set(updatingDiv, "display", "block");
			
			var adminEndpoint = ism.user.getAdminEndpoint();
			this.deferreds.protocols = adminEndpoint.doRequest("/configuration/Protocol", adminEndpoint.getDefaultGetOptions());
			this.promises.endpoints = adminEndpoint.doRequest("/configuration/Endpoint", adminEndpoint.getDefaultGetOptions());
            this.promises.connectionPolicies = adminEndpoint.doRequest("/configuration/ConnectionPolicy", adminEndpoint.getDefaultGetOptions());
            this.promises.TopicPolicies = adminEndpoint.doRequest("/configuration/TopicPolicy", adminEndpoint.getDefaultGetOptions());
            this.promises.SubscriptionPolicies = adminEndpoint.doRequest("/configuration/SubscriptionPolicy", adminEndpoint.getDefaultGetOptions());
            this.promises.QueuePolicies = adminEndpoint.doRequest("/configuration/QueuePolicy", adminEndpoint.getDefaultGetOptions());
            
            // get the message hub
            var promise = adminEndpoint.doRequest("/configuration/MessageHub", adminEndpoint.getDefaultGetOptions());
            promise.then(
                    function(data) {
                        var hubs = adminEndpoint.getObjectsAsArray(data, "MessageHub");
                        _this.existingNames = hubs.map(function(item){ return item.Name; });
                        var foundMessageHub = false;
                        var mapped = hubs.map(function(item){
                            item.id = escape(item.Name);
                            if (item.Name == _this.Name) {
                                foundMessageHub = true;
                                _this.messageHubItem = item;
                                _this.Description = item.Description;
                                if (!_this.Description) {
                                    _this.Description = "";
                                }
                                _this.MHDetailsDescription.innerHTML = entities.encode(_this.Description);
                                _this.updateTabContainer(item);
                                // hide loading indicator
                                domStyle.set(updatingDiv, "display", "none");
                            }
                            return item;
                        });
                        if (!foundMessageHub) {
                            // hide loading indicator
                            domStyle.set(updatingDiv, "display", "none");
                            Utils.showPageError(nls.messaging.messageHubs.messageHubNotFoundError);
                        }
                    },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                        domStyle.set(updatingDiv, "display", "none");
                        Utils.showPageError(nls.messaging.messageHubs.messageHubNotFoundError);
                    } else {
                         Utils.showPageErrorFromPromise(nls.messaging.messageHubs.retrieveMessageHubsError, error);
                    }
                }
            );            
		},
		
		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialog triggered for editing the message hub

			var editId = "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", "ismMessageHubDetails");
			this.editDialog = new MessageHubDialog({dialogId: editId, 
												   title: nls.messaging.messageHubs.editDialog.title, 
												   instruction: nls.messaging.messageHubs.editDialog.instruction,
												   add: false});
			this.editDialog.startup();
		},
		
		_initEvents: function() {
					
			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving editDialog");
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {		
					var bar = query(".idxDialogActionBarLead",this.editDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;
					this.editDialog.save(lang.hitch(this, function(data) {
						if (this.messageHubItem.Name != data.Name) {
							var index = array.indexOf(this.existingNames, this.messageHubItem.Name);
							if (index >= 0) {
								this.existingNames[index] = data.Name;
							}
						}
						this.messageHubItem = data;
						this._setNameAndDescription(data);
						this.editDialog.dialog.hide();
						dojo.publish('messageHubItemChanged', data);
					}));
				}
			}));			
		},
		
		
		updateTabContainer: function(messageHubItem) {
			if (!this.tabs) {
				this.tabs = [];
				if (!this.tabContainer) {
					this.tabContainer = dijit.byId("ismMHTabContainer");
				}
				for (var tab in Resources.pages.messaging.messageHubTabs) {
					var id = Resources.pages.messaging.messageHubTabs[tab].id;
					var title =  Resources.pages.messaging.messageHubTabs[tab].title;
					var tooltip =  Resources.pages.messaging.messageHubTabs[tab].tooltip;
					
					// instantiate the class that provides the pane content
					var classname =  Resources.pages.messaging.messageHubTabs[tab].classname;
					var klass = lang.getObject(classname);
					console.debug("classname: ", classname);
					var tabInstance = new klass({ title: title, id: id, tooltip: tooltip, messageHubItem: messageHubItem, detailsPage: this});
					
					// add it the array of tabs
					this.tabs.push(tabInstance);
					
					// add the instance to the tabcontainer
					this.tabContainer.addChild(tabInstance);
				}
				
				this.tabContainer.startup();
				this.tabContainer.resize();
				aspect.after(this.tabContainer, "selectChild", function(){
					dojo.publish("lastAccess", {requestName: tabInstance.id+".tab"});
				});

			} else {
				dojo.forEach(this.tabs, function(tab, i) {
					tab.setItem(messageHubItem);
				});	
				this.tabContainer.selectChild(this.tabs[0]);
			}				
		}
		
	});

	return MessageHubDetails;
});


