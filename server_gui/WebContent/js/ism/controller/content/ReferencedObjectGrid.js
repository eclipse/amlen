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
		'dojo/data/ObjectStore',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Form',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'idx/form/CheckBox',
		'idx/form/Textarea',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils',
		'ism/widgets/TextBox',
		'ism/widgets/Select',
		'dojo/text!ism/controller/content/templates/ReferencedObjectGrid.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!dijit/form/nls/validate',
		'ism/widgets/GridFactory'
		], function(declare, lang, array, connect, aspect, on, domClass, domConstruct, domStyle,
				Memory, ObjectStore, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Form, Button, Dialog, CheckBox, Textarea, HoverHelpTooltip, Utils, TextBox, 
				Select, template, nls, nls_strings, nls_validate, GridFactory) {

	var ReferencedObjectGrid = declare("ism.controller.content.ReferencedObjectGrid", [_Widget, _TemplatedMixin], {
		// summary:
		//		Grid to show associated items

		grid: null,
		store: null,
		value: null,
		idPrefix: "",
		contentId: null,
		filterEnabled: true,
	    pagingEnabled: true,
		messagingPolicies: false,
		appliedLabel: nls.messaging.referencedObjectGrid.applied,
		nameLabel: nls.messaging.referencedObjectGrid.name,
		additionalColumns: undefined,
		
		nls: nls,
		templateString: template,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			
			this.contentId = this.idPrefix + "referencedObjContent";
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
		
		populateGrid: function(dialog, store, reference, excludeReferenced) {
			console.debug(this.declaredClass + ".populateGrid()");
			
			// cleanup when the dialog closes
			on.once(dialog, "hide", lang.hitch(this,function() {
				this.store.query(function(item) {
					if (item.associated) {
						return item.associated.checked === true;
					}
					return false;
				}).forEach(function(item) {
					if (item.associated) {
						item.associated.checked = false;
						item.associated.sortWeight = 1;
					} 
				});	
				
				// clear the filter
				if (this.grid.quickFilter) {
					this.grid.quickFilter.clear();
				}
				// clear the cache
				this.grid.model.clearCache();
				// reset the paging
				if (this.grid.pagination) {
					this.grid.pagination.gotoPage(0);
				}
				
			}));
			
			domStyle.set(this.iconNode, "visibility", "hidden");

			if (store) {
				this.store = store;
				var mapped;
				if (reference) {
					if (excludeReferenced && reference !== "") {
						// create a store that excludes the referenced items
						var exclude = reference.split(",");
						mapped = store.query(function(item){ 
							var result = result = array.indexOf(exclude, unescape(item.id)) < 0;
							return result;
						});  
						this.store = new Memory({data:mapped, idProperty: store.idProperty, queryEngine: store.queryEngine});
						// the value starts empty
						this._setValueAttr([]);
					} else {
						this._setValueAttr(reference.split(","));
					}
				}
				if (!mapped) {
					mapped = store.query(function(item){ return true; });  
					this.store = new Memory({data:mapped, idProperty: store.idProperty, queryEngine: store.queryEngine});
				}
				this.grid.setStore(this.store);
				this.triggerListeners();
				this.grid.body.refresh();
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
			// get the selected items and look for topic and subscription
			var topicFound = false;
			var subscriptionFound = false;
			if (this.store) {
				var results = this.store.query(function(item) {
					if (item.associated) {
						return item.associated.checked === true;
					}
					return false;
				});
				for (var i = 0, len=results.length; i < len; i++) {
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
			}
			
			domStyle.set(this.iconNode, "visibility", isValid ? "hidden" : "visible");
			this._set("state",isValid ? "" : "Error");
			return isValid;
		},
		
		focus: function(){
			console.debug("ReferencedObjectGrid -- focus");
		},
		
		getValue: function() {
			// summary:
			//		return an array of selected object names, unsorted
			var results = this.store.query(function(item) {
				if (item.associated) {
					return item.associated.checked === true;
				}
				return false;
			});
			return results.map(function(item){ return item.id; });								
		},
		
		getValueAsString: function() {
			// summary:
			//		return a comma-separated list of selected object names
			console.debug(this.declaredClass + ".getValueAsString()");

			var value = this.getValue();			
			value.sort();
			return value.join(",");
		},
		
		_isEmpty: function() {
			return this.getValueAsString().length === 0;
		},

		_setValueAttr: function(valueArray) {
			// mark as "checked" the applied box in the grid rows matching the elements of valueArray 
			// param: input array
			console.debug(this.declaredClass + ".setValueAttr(" + valueArray + ")");
			
			if (this.grid && valueArray) {
				for (var i=0, len=valueArray.length; i < len; i++) {
					var results = this.store.query({id: valueArray[i]});
					if (results.length === 0) {
						continue;
					}
					var item = results[0];
					item.associated.checked = true;
					item.associated.sortWeight = 0;
				}				
			} else {
				console.debug("ReferencedObjectGrid -- _setValueAttr -- grid or valueArray was null");
			}
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
					{ id: "associated", name: this.appliedLabel, field: "associated", dataType: "string", width: "50px", widgetsInCell: true,
						setCellValue: function(data) {
							this.checkbox.associated = data;							
							var checked = data && data.checked ? true : false;
							this.checkbox.set("checked", checked, false);
							var disabled = data && data.disabled ? data.disabled : false;
							this.checkbox.set("disabled", disabled, false);
							
							if (this._onChangeHandler) {
								this.disconnect(this._onChangeHandler);
							}
							this._onChangeHandler = this.checkbox.connect(this.checkbox, 'onChange', function(value) {
								console.debug("checkbox changed " + value);
								if (this.associated) {
									if (value) {
										this.associated.checked = true;
										this.associated.sortWeight = 0;										
									} else {
										this.associated.checked = false;
										this.associated.sortWeight = 1;										
									}
									t.validate();
								}
							});
						},
						decorator: function() {	
							return '<div data-dojo-type="idx.form.CheckBox" data-dojo-attach-point="checkbox"></div>';
						}						
					},
					{	id: "Name", name: this.nameLabel, field: "Name", dataType: "string", width: nameColWidth,
						decorator: Utils.nameDecorator
					}
				];
			var columns = this.structure;
			if (this.additionalColumns) {			
				columns = this.structure.concat(this.additionalColumns);
			}
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, columns, 
							{paging: this.pagingEnabled, filter: this.filterEnabled, suppressSelection: true, heightTrigger: 0});

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

	return ReferencedObjectGrid;
});
