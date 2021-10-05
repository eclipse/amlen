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
define(["dojo/_base/declare",
        "dojo/_base/lang",
        'dojo/_base/array',
        "dojo/on",
        "dojo/dom-construct",
        "dojo/dom-class",
		'dojo/dom-style',
		'dojo/dom-geometry',        
        'dojo/keys',
        "dojo/query",
        'dojo/store/Memory',
        'dijit/registry',
        "dijit/_Widget",
        "dijit/_TemplatedMixin",
        'idx/widget/HoverCard',
		'gridx/Grid',
		'gridx/core/model/cache/Sync',
		'gridx/modules/select/Row',
		'gridx/modules/CellWidget',
		'gridx/modules/Focus',
		'idx/gridx/modules/Sort',
		'ism/IsmUtils',
        "dojo/i18n!ism/nls/messaging",
        "dojo/text!./templates/AssociationControl.html"],
        function(declare, lang, array, on, domConstruct, domClass, domStyle, domGeom, keys, query,
        		Memory, registry, _Widget, _TemplatedMixin,
        		HoverCard, Grid, Sync, SelectRow, CellWidget, Focus, Sort, Utils, nls, template){
	
	/**
	 * This class was originally created to show the hover card for the destination mapping rules
	 * associate queue manager connections.  By default, it will still do that.  To customize
	 * for another association, provide the following parameters:
	 * hoverCardId:  The id to use for the hover card
	 * title: The title for the card
	 * structure: The column structure for the grid in the hover card
	 * gridId: The id for the grid in the hover card
	 * associatedObjectStore: The store to query for the contents of the card. Assumes the name field is the key.
	 */
	var AssociationControl = declare("ism.widgets.AssociationControl",[_Widget,_TemplatedMixin],{	
	
		templateString: template,
		qmcList: null,
		destName: null,
		hover: null,
		hoverCardId: "QMconnectionsHoverCard",
		title: nls.messaging.destinationMapping.associatedQMs,
		structure: [
					{	id: "Name", name: nls.messaging.destinationMapping.grid.name, field: "Name", dataType: "string", width: "120px",
						decorator: function(cellData) {
							var value = Utils.nameDecorator(cellData);
							return "<span title='"+value+"'>"+value+"</span>";
						}
					},
					{	id: "ConnectionName", name: nls.messaging.connectionProperties.grid.connName, field: "ConnectionName", dataType: "string", width: "140px",
						decorator: function(list) {
							var value = list.split(",").join(",<br>\n"); 
							return "<span title='"+list+"'>"+value+"</span>";
						}
					},
					{	id: "status", name: nls.messaging.connectionProperties.grid.status, field: "status", width: "190px",
						decorator: function(cellData) {
							var iconClass = "";
							var altText = "";
							var msg = "";
							if (cellData.rc !== undefined) {
								if (cellData.rc === 0) { // enabled
									iconClass = "ismIconEnabled";
									altText = nls.messaging.destinationMapping.status.active;
								} else if ((cellData.rc === 2) || (cellData.rc === 3)) { // reconnecting or starting
									iconClass = "ismIconLoading";
									altText = nls.messaging.destinationMapping.status.starting;
								} else { // disabled
									iconClass = "ismIconDisabled";
									altText = nls.messaging.destinationMapping.status.inactive;
								}
							}
							if (cellData.msg) {
								msg = cellData.msg;
							}
							return "<div class='"+iconClass+" dijitInline' title='"+altText+"'></div><span title='"+ msg +"'>"+ msg +"</span>";
						}						
					}
				],
		hoverGrid: null,
		gridId: null,
		associatedObjectStore: null,
		
		showDelay: 500, //ms
		showTimer: null,
		
		hoverCardWidth: 506, //px
		
		ruleStatus: null, // status code for each associated rule
		ruleMessage: null, // status message for each associated rule
		enabled: null, // whether the associated rule is enabled

		postCreate: function() {
			console.debug(this.declaredClass + ".postCreate");
			
			if (!this.gridId) {
				this.gridId = this.id + "QMConnectionGrid";
			}
			
			this.watch('value', lang.hitch(this, function(property, oldvalue, newvalue) {
				console.debug("AssociationControl newvalue name="+newvalue.name);
				//console.dir(newvalue);
				this.qmcList = newvalue.qmc;
				var count = newvalue.qmc ? newvalue.qmc.split(",").length : 0;
				this.associationCount.innerHTML = count;
				this.destName = newvalue.name;
				this.ruleStatus = newvalue.status;					
				this.ruleMessage = newvalue.message;					
				this.enabled = newvalue.enabled;
				
				var isWarn = false;
				var isStarting = false;
				if (newvalue.status && this.enabled && (this.enabled == "enabled")) {
					// look for any status values of 1 (disabled) or 2 (reconnecting), which indicate a problem
					// or 3 (starting) indicating we should show the starting icon
					for (var key in this.ruleStatus){
						if (this.ruleStatus.hasOwnProperty(key)){
							if ((this.ruleStatus[key] == 1) || ((this.ruleStatus[key] == 2))) {
								isWarn = true;
							} else if (this.ruleStatus[key] == 3) { 
								isStarting = true;
							}
						}
					}
				}
				if (isWarn) {
					domClass.add(this.hoverIcon,"ismIconWarning");
					domClass.remove(this.hoverIcon,"hoverHelpIcon ismIconLoading");		
					newvalue.sortWeight = count + "W";
				} else if (isStarting) {
					domClass.add(this.hoverIcon,"ismIconLoading");
					domClass.remove(this.hoverIcon,"hoverHelpIcon ismIconWarning");
					newvalue.sortWeight = count + "L";
				} else {
					domClass.add(this.hoverIcon,"hoverHelpIcon");
					domClass.remove(this.hoverIcon,"ismIconWarning ismIconLoading");
					newvalue.sortWeight = count + "H";
				}
				
				// update hovercard if it is showing
				if (this.hover && this.hover.domNode) {
					if (!domClass.contains(this.hover.domNode,"dijitHidden")) {
						console.debug("updating visible hovercard");	    	
					    var data = this.getGridData();
					    if (data) {
					    	var hoverStore = new Memory({data: data, idProperty: "id"});
					    	this.hoverGrid.setStore(hoverStore);					    		
					    }				    
					}
				}
			}));
		},
		
		getAssociatedObjectStore: function() {
			if (!this.associatedObjectStore) {
				var connProps = registry.byId("connprops");
		    	if (connProps && connProps.store) {
		    		this.associatedObjectStore = connProps.store;
		    	}
			}
			return this.associatedObjectStore;
		},
		
		getGridData: function() {
			if (!this.qmcList) {
				return null;
			}
			var associatedObjectStore = this.getAssociatedObjectStore();
			if (!associatedObjectStore) {
				return null;
			}
			if (this.associationType === "messagingPolicies") {
				var newObjectStore = this._combineStores(associatedObjectStore);
				if (!newObjectStore) {
					return null;
				}
				associatedObjectStore = newObjectStore;
			}
	    	return array.map(this.qmcList.split(","), lang.hitch(this, function(conn) {
	    		var item = associatedObjectStore.query({ Name: conn })[0];
	    		if (item) {
		    		item.id = escape(item.Name);
	    		} else {
	    			item = associatedObjectStore.query({ id: conn })[0];
	    		}
	    		item.status = {};
	    		if (this.ruleStatus) {
	    			item.status.rc = this.ruleStatus[item.Name];
	    		}
	    		if (this.ruleMessage && this.ruleMessage[item.Name]) {
	    			item.status.msg = this.ruleMessage[item.Name];
	    		}
	    		return item;
	    	}));
		},
		
		_combineStores: function(stores) {
			var newStore = undefined;
			var len = stores.length;
			for (var i = 0; i < len; i++) {
				var items = stores[i].query();
				if (newStore === undefined && items.length > 0) {
					newStore = new Memory({data: [], idProperty: "id"});
				}
				for(var j = 0; j < items.length; j++) {
					// Clone the items for the hover card
					var item = JSON.stringify(items[j]);
					newStore.put(JSON.parse(item));
				}
			}
			return newStore;
		},
		
		_onMouseOver: function() {
			console.debug(this.declaredClass + "._onMouseOver");
			
			// don't do anything if the hover is already displayed
			if (this.hover && this.hover.domNode) {
				if (!domClass.contains(this.hover.domNode,"dijitHidden")) {
					//console.debug("hover is already visible");
					return;
				}
			}
			
            if (!this.showTimer) {
                this.showTimer = setTimeout(lang.hitch(this, function(){
                    this.open();
                }), this.showDelay);
            }
		},
		
		_onMouseLeave: function() {
			console.debug(this.declaredClass + "._onMouseLeave");
			
            if (this.showTimer) {
                window.clearTimeout(this.showTimer);
                delete this.showTimer;
            }
		},
		
		open: function() {
			console.debug(this.declaredClass + ".open");
			
		    // close and delete existing hover if there is one 
			var hoverCardId = this.hoverCardId;
			var oldHover = registry.byId(hoverCardId);
		    if (oldHover) {
		    	oldHover.close();
		    	oldHover.fadeOut.stop(true);
		    	oldHover.destroyRecursive();
		    	domConstruct.destroy(hoverCardId);
		    }
		    
		    // clear the timer
            if (this.showTimer) {
                window.clearTimeout(this.showTimer);
                delete this.showTimer;
            }
            
            // create and show the hover
            var data = this.getGridData();
            if (data) {
            	// create and populate hover card
            	var hoverPane = domConstruct.place("<div style='padding: 10px; overflow: auto;'></div>", this.id);								
            	domConstruct.place("<h3>" + Utils.nameDecorator(unescape(this.destName)) + "</h3>", hoverPane);
            	domConstruct.place("<p>" + this.title + "</p>", hoverPane);
            	this._createQMConnectionGrid(hoverPane, data); 
            	
            	this.hover = new HoverCard({ id: hoverCardId });
            	this.hover.set('target', this.hoverIcon);
            	this.hover.set('content',hoverPane);			
            	this.hover.open(this.hoverIcon);
            }
            
		},
		
		_onKey: function(evt) {
			console.debug(this.declaredClass + "._onKey "+evt);
			if ((evt.keyCode === keys.ENTER) || (evt.charCode === keys.SPACE)) {
				this._onMouseOver();
			}
		},
		
		_createQMConnectionGrid: function(parent, data) {
			domConstruct.place("<div id='"+this.gridId+"'></div>", parent);
			var hoverStore = new Memory({data: data, idProperty: "id"});
			this.hoverGrid = new Grid({
				id: this.gridId,
				cacheClass: Sync,
				store: hoverStore,
				autoHeight: true,
				autoWidth: true,
				structure: this.structure,
				modules: [
					{ moduleClass: SelectRow, triggerOnCell: true, multiple: false },
					CellWidget, Sort, Focus
				]
			}, this.gridId);
			domClass.add(this.hoverGrid.domNode,"compact");
			this.hoverGrid.startup();
			parent.style.width = (this.hoverCardWidth + 18) + "px";
			var node = this.hoverGrid.domNode;
			var computedStyle = domStyle.get(node);
		    var box = domGeom.getContentBox(node, computedStyle);
		    if (box.h > 300) {
		    	this.hoverGrid.set('autoHeight', false);
				this.hoverGrid.resize({h: 300});
		    }
		}
		
	});
		
	return AssociationControl;
	
});
