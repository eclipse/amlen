/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
		'dojo/query',
		'dojo/_base/array',
		'dojo/aspect',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/registry',
		'ism/widgets/IsmQueryEngine',
		'ism/widgets/GridFactory',
		'ism/IsmUtils',	
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'ism/config/Resources',
		'dojo/keys'
		], function(declare, lang, query, array, aspect, on, domClass, domConstruct, domStyle, Memory, _Widget, 
				    _TemplatedMixin, _WidgetsInTemplateMixin, registry, IsmQueryEngine, GridFactory, Utils, nls, nls_strings, Resources, keys) {

	/*
	 * This file defines a configuration widget that displays the list of available protocols
	 */

	var MessageProtocolsList = declare ("ism.controller.content.MessageProtocolsList", [_Widget, _TemplatedMixin], {

		nls: nls,
		grid: null,
		store: null,
		contentId: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		isRefreshing: false,
		
		domainUid: 0,
		
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug("constructing MessageProtocolsList");
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], idProperty: "Name"});
		},
		
		postCreate: function() {
			this.inherited(arguments);
            
            this.domainUid = ism.user.getDomainUid();
			
			// start with an empty <div> for the content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.messaging.nav.messageProtocols.topic, lang.hitch(this, function(message){
				this._initStore();
			}));	
			dojo.subscribe(Resources.pages.messaging.nav.messageProtocols.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this._refreshGrid();
				
				if (this.isRefreshing) {
					this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
				}
			}));	
			
			this._initGrid();
			this._initStore();
		},
		
		_initStore: function() {
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
			this.isRefreshing = true;
			var page = this;
			
			// kick off the query for the protocols...
            var adminEndpoint = ism.user.getAdminEndpoint();
            var messageProtocolsResults = adminEndpoint.doRequest("/configuration/Protocol", adminEndpoint.getDefaultGetOptions());
            
			messageProtocolsResults.then(lang.hitch(this,function(data) {
				this.isRefreshing = false;
		    	page.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
				page.store = Utils.getProtocolStore(data);	
				var items = page.store.query({}, {sort: [{attribute: "label"}]});
				page.store = new dojo.store.Memory({data:items, idProperty: "Name", queryEngine: IsmQueryEngine});
				page.grid.setStore(page.store);
				//page.grid.sort('Name', true);
			}));
			
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.messaging.messageProtocols.messageProtocolsList.name, field: "label", dataType: "string", width: "200px",decorator: Utils.nameDecorator},
					{	id: "Topic", name: nls.messaging.messageProtocols.messageProtocolsList.topic, field: "Topic", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.Topic);
                        }                         					    
					},
					{	id: "Shared", name: nls.messaging.messageProtocols.messageProtocolsList.shared, field: "Shared", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.Shared);
                        }                         
					},
					{	id: "Queue", name: nls.messaging.messageProtocols.messageProtocolsList.queue, field: "Queue", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.Queue);
                        }                         
					},
					{	id: "Browse", name: nls.messaging.messageProtocols.messageProtocolsList.browse, field: "Browse", dataType: "string", width: "100px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.Browse);
                        }                         
					}					
				];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, {paging: false, filter: false, suppressSelection: true, heightTrigger: 1});

		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
			
		},
		
		refreshGrid:  function() {
			console.debug("RefreshGrid");
			this._refreshGrid();
		}
		
	});
	
	return MessageProtocolsList;
});
