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
		'dojo/_base/connect',
		'dojo/aspect',
		'dojo/on',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/_base/manager',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils',
		'dojo/text!ism/controller/content/templates/OrderedReferencedObjectGrid.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!dijit/form/nls/validate',
		'ism/widgets/GridFactory',
		'ism/widgets/IsmQueryEngine'
		], function(declare, lang, array, connect, aspect, on, domClass, domConstruct, domStyle,
				Memory, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, manager,	HoverHelpTooltip, Utils, 
				template, nls, nls_validate, GridFactory, IsmQueryEngine) {

	var OrderedReferencedObjectGrid = declare("ism.controller.content.OrderedReferencedObjectGrid", [_Widget, _TemplatedMixin], {
		// summary:
		//		Grid to show associated items

		grid: null,
		store: null,
		policyStore: undefined,
		addDialogId: undefined,
		value: null,
		idPrefix: "",
		contentId: null,
		messagingPolicies: false,
		nameLabel: nls.messaging.referencedObjectGrid.name,
		additionalColumns: undefined,
		
		addButton: null,
		deleteButton: null,
		upButton: null,
		downButton: null,
		
		nls: nls,
		templateString: template,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			
			this.contentId = this.idPrefix + "orderedRefObjContent";
			this.title = "";
		},

		postCreate: function() {
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);			
		},
		
		startup: function() {
			console.debug(this.declaredClass + ".startup()");
			this.inherited(arguments);
			
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			this.store = new Memory({data:[]});
			this._initGrid();
			this._initEvents();
			
			this.message = nls_validate.missingMessage;	
			this.connect(this.iconNode, "onmouseenter", function(){
				if(this.message && domStyle.get(this.iconNode, "visibility") == "visible"){
					HoverHelpTooltip.show(this.message, this.iconNode, this.tooltipPosition, !this.isLeftToRight());
				}
			});	
			
			if (this.tooltipContent) {
				this.tooltip = new HoverHelpTooltip({
					connectId: [this.idPrefix + "_Hover"],
					forceFocus: true, 
					showLearnMore:false,
					label: this.tooltipContent
				});
				this.tooltip.startup();
			} else {
				domStyle.set(this.tooltipNode, "display", "none");
			}
			
			if (!this.required) {
				domStyle.set(this.requiredIconNode, "display", "none");
			}
		}, 
		
		populateMultiStoreGrid: function(dialog, stores, reference) {
			console.debug(this.declaredClass + ".populateGrid()");
			var len = stores.length;
			var combinedStore = new Memory({data:[], idProperty: "id", queryEngine: IsmQueryEngine});
			for (var i = 0; i < len; i++) {
				var policies = stores[i].query();
				for(var idx = 0; idx < policies.length; idx++) {
					var pol = policies[idx];
					combinedStore.put(pol);
				}			
			}
			this.populateGrid(dialog, combinedStore, reference);
		},
		
		populateGrid: function(dialog, policyStore, reference) {
			console.debug(this.declaredClass + ".populateGrid()");
			
			// cleanup when the dialog closes
			on.once(dialog, "hide", lang.hitch(this,function() {
				// clear the cache
				this.grid.model.clearCache();
				// reset the paging
				this.grid.pagination.gotoPage(0);
				
			}));
			
			domStyle.set(this.iconNode, "visibility", "hidden");

			this.policyStore = policyStore;
			if (policyStore) {
				if (reference) {
					this.value = reference.split(",");
					// create a store that includes the referenced items
					var mapped = [];
					for (var i = 0, len=this.value.length; i < len; i++) {
						var item = policyStore.query({id: escape(this.value[i])})[0];
						item.order = i;
						mapped[i] = item;						
					}
					this.store = new Memory({data:mapped, idProperty: policyStore.idProperty, queryEngine: policyStore.queryEngine});
					this._setValueAttr(reference.split(","));
				} else {
					this.store = new Memory({data:[], idProperty: policyStore.idProperty, queryEngine: policyStore.queryEngine});
					this._setValueAttr([]);
				}
				this.grid.setStore(this.store);
				this._refreshGrid();
			} 			
		},
		
		validate: function() {
			console.debug(this.declaredClass + ".validate()");

			if (!this.required) {
				// no validation if not required
				return true;
			}
			// need at least one checkbox ticked
			// if messaging policies and one of them is a subscription policy,
			// ensure a topic policy is also selected
			var isValid = !this._isEmpty();
			if (!isValid || !this.messagingPolicies) {
				domStyle.set(this.iconNode, "visibility", isValid ? "hidden" : "visible");
				this._set("state",isValid ? "" : "Incomplete");
				this.message = nls_validate.missingMessage;
				return isValid;				
			}
			/* No Longer a restriction!!!! get the selected items and look for topic and subscription
			var topicFound = false;
			var subscriptionFound = false;
			if (this.store) {
				var results = this.store.query({});
				for (var i = 0; i < results.length; i++) {
					var item = results[i];
					if (item.DestinationType === "Topic") {
						topicFound = true;
						break;
					} else if (item.DestinationType === "Subscription") {
						subscriptionFound = true;
					}
				}
				isValid = topicFound || !subscriptionFound;
				if (!isValid) {
					this.message = this.invalidMessage;
				}
			} */
			
			domStyle.set(this.iconNode, "visibility", isValid ? "hidden" : "visible");
			this._set("state",isValid ? "" : "Error");
			return isValid;
		},
		
		getValue: function() {			
			if (this.value && this.value.length > 0) {
				var retval = [];
				for (var i = 0; i < this.value.length; i++) {
					retval[i] = unescape(this.value[i]);
				}
				return retval;
			}
			return this.value;
		},
		
		getValueAsString: function() {
			// summary:
			//		return a comma-separated list of selected object names
			console.debug(this.declaredClass + ".getValueAsString()");

			var value = this.getValue();			
			return value.join(",");
		},
		
		_updateValue: function() {
			if (this.store) {
				var results = this.store.query({},{sort: [{attribute: "order"}]});
				this.value = results.map(function(item){ return item.id; });  
			} 	
			this.validate();
		},
		
		_isEmpty: function() {
			return this.getValueAsString().length === 0;
		},

		_setValueAttr: function(valueArray) {
			this.value = valueArray;
		},
		
		_setAdditionalColumnsAttr: function(columnArray) {
			console.debug(this.declaredClass + "._setAdditionalColumnsAttr()");
			this.additionalColumns = columnArray ? columnArray : [];
			if (this.grid) {
				// grid already defined, reset the columns
				this.grid.setColumns(this.structure.concat(this.additionalColumns));
			}
		},

		_initGrid: function() {
			console.debug(this.declaredClass + ".initGrid()");
			var gridId = this.id + "ReferencedObjectGrid";

			var t = this;
			var nameColWidth = "350px";
			if (this.additionalColumns && this.additionalColumns.length > 0) {
				nameColWidth = "150px";
			}
			this.structure = [ 
					{	id: "Name", name: this.nameLabel, field: "Name", dataType: "string", width: nameColWidth,
						decorator: Utils.nameDecorator
					}
				];
			var columns = this.structure;
			if (this.additionalColumns) {			
				columns = this.structure.concat(this.additionalColumns);
			}
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, columns, 
			        {paging: true, suppressSort: true, moveRow: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.upButton = GridFactory.addToolbarButton(this.grid, GridFactory.UP_BUTTON, false);
			this.downButton = GridFactory.addToolbarButton(this.grid, GridFactory.DOWN_BUTTON, false);
			this.grid.move.row.moveSelected = false;
			this.grid.move.row.onMoved = lang.hitch(this, function() {
				this._updateButtons();
				this._updateValue();
			});
		},
		
		_updateButtons: function() {
			var selected = this.grid.select.row.getSelected();
			console.log("selected: "+ selected);
			if (selected && selected.length > 0) {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);
				this.deleteButton.set('disabled',false);
				if (ind > 0) {
					this.upButton.set('disabled',false);
				} else {
					this.upButton.set('disabled',true);
				}
				if (ind < this.grid.rowCount()-1) {
					this.downButton.set('disabled',false);
				} else {
					this.downButton.set('disabled',true);
				}
			} else {
				console.debug("nothing is selected");
				this.deleteButton.set('disabled',true);
				this.upButton.set('disabled',true);
				this.downButton.set('disabled',true);
			}
		},
		
		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
			this.triggerListeners();
		},
		
		_initEvents: function() {
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				this._updateButtons();
			}));
			
			on(this.upButton, "click", lang.hitch(this, function() {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);			
				this.grid.move.row.move([ind], ind-1, false);
			}));

			on(this.downButton, "click", lang.hitch(this, function() {
				var ind = this.grid.model.idToIndex(this.grid.select.row.getSelected()[0]);			
				this.grid.move.row.move([ind], ind+2, false);				
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
				if(this.addDialog){
					this.addDialog.show(this.getValueAsString(), this.policyStore, lang.hitch(this,function(items){
						this._addItems(items);
					})); 
				} else {
					console.error("addDialog not found!");
				}					
			    
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				// intentionally do not confirm delete, just remove it from our local model				
				this.store.remove(this.grid.select.row.getSelected()[0]);
				this._refreshGrid();
				this._updateValue();
				this._updateButtons();
			}));

		},	
		
		_getNextOrder: function() {
			if (this.store) {
				var results = this.store.query({}, {sort: [{attribute: "order", descending: true}]});
				if (results && results.length > 0) {
					return results[0].order + 1;
				}
			}
			return 0;
		},
		
		_addItems: function(itemArray) {
			console.debug("addItems called with " + itemArray);
			if (this.policyStore && this.store && itemArray && itemArray.length > 0) {
				var nextOrder = this._getNextOrder();
				// create a store that includes the referenced items
				for (var i = 0, len=itemArray.length; i < len; i++) {
					var item = this.policyStore.query({id: itemArray[i]})[0];
					item.order = nextOrder;
					nextOrder++;
					this.store.add(item);					
				}
				this._refreshGrid();
				this._updateValue();
				this._updateButtons();
			} 			
		},
		
		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated 
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");			
		}

	});

	return OrderedReferencedObjectGrid;
});
