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
		'gridx/Grid',
		'gridx/core/model/cache/Sync',
		'gridx/modules/CellWidget',
		'gridx/modules/Focus',
		'gridx/modules/ColumnResizer',
		'idx/gridx/modules/Sort',
		'ism/widgets/Dialog',
		'idx/form/CheckBox',
		'idx/form/Textarea',
		'idx/widget/HoverHelpTooltip',
		'ism/IsmUtils',
		'ism/widgets/TextBox',
		'ism/widgets/Select',
		'dojo/text!ism/controller/content/templates/QMConnectionGrid.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!dijit/form/nls/validate'
		], function(declare, lang, array, connect, aspect, on, domClass, domConstruct, domStyle,
				Memory, ObjectStore, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Form, Button, Grid, Sync, CellWidget, Focus, ColumnResizer, Sort, Dialog,
				CheckBox, Textarea, HoverHelpTooltip, Utils, TextBox, 
				Select, template, nls, nls_strings, nls_validate) {

	var QMConnectionGrid = declare("ism.controller.content.QMConnectionGrid", [_Widget, _TemplatedMixin], {
		// summary:
		//		Grid to show associated queue manager connections

		grid: null,
		store: null,
		restURI: null,
		value: null,
		idPrefix: "",
		contentId: null,
		listeners: null,
		retainedMessages: null,
		
		nls: nls,
		templateString: template,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			
			this.contentId = this.idPrefix + "qmConnectionContent";
			this.title = "";
		},

		postCreate: function() {
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
			
			
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			this.store = new Memory({data:[]});
			
			this.message = nls_validate.missingMessage;	
			this.connect(this.iconNode, "onmouseenter", function(){
				if(this.message && domStyle.get(this.iconNode, "visibility") == "visible"){
					HoverHelpTooltip.show(this.message, this.iconNode, this.tooltipPosition, !this.isLeftToRight());
				}
			});
			
		},
		
		populateGrid: function(dialog, connProps, queueManagerConnection, retainedMessages) {
			console.debug(this.declaredClass + ".populateGrid()");
			this.retainedMessages = retainedMessages;
				
			// cleanup when the dialog closes
			on.once(dialog, "hide", lang.hitch(this,function() {
				// uncheck checkboxes
				for (var prop in this.checkboxes) {
					console.debug("unchecking: "+this.checkboxes[prop]);
					this.checkboxes[prop].set("checked", false);
				}
				// remove old listeners
				if (this.listeners) {
					array.forEach(this.listeners, function(entry, i){
					    entry.remove();
					  });
					this.listeners = null;
				}
				// hide the tooltip if it's showing
				if (this.iconNode) {
					HoverHelpTooltip.hide(this.iconNode);
				}
			}));
			
			domStyle.set(this.iconNode, "visibility", "hidden");

			if (!this.grid) {
				this._initGrid(this.domNode);
			}
			if (connProps) {
				this.store = connProps.store;
				this.grid.setStore(this.store);
				// restrict height and use scrollbars with many entries (or no entries)
				if ((this.store.data.length === 0) || (this.store.data.length > 3)) {
					this.grid.set("autoHeight",false);
					this.grid.domNode.style.height = "158px";
				} else {
					this.grid.set("autoHeight",true);
					this.grid.domNode.style.height = "100%";
				}
				this.grid.resize();
			}			
			
			// listen for changes to the checkboxes
			this.listeners = [];
			for (var prop in this.checkboxes) {
				var listener = on(this.checkboxes[prop], "change", lang.hitch(this,function() {
					this.validate();
				}));
				this.listeners.push(listener);			
			}
			
			if (queueManagerConnection) {
				this._setValueAttr(queueManagerConnection.split(","));
			}
		},
		
		validate: function() {
			console.debug(this.declaredClass + ".validate()");

			// if RetainedMessages is not 'None', then exactly one association is needed,
			// otherwise at least one is needed.
			var selected = this.getCount();
			var isValid = true;
			if (this.retainedMessages && this.retainedMessages.get('value') !== 'None') {
				// RetainedMessages special case
				isValid = selected === 1;
				this.message = nls.messaging.destinationMapping.associatedMessages.errorRetained;
			} else {
				isValid = selected > 0;
				this.message = nls.messaging.destinationMapping.associatedMessages.errorMissing;
				// toggle the enabled state of RetainedMessages if necessary
				dojo.publish("RetainedMessageAssociationCountChanged", {count: selected});
			}
			domStyle.set(this.iconNode, "visibility", isValid ? "hidden" : "visible");
			this._set("state",isValid ? "" : "Incomplete");
			return isValid;
		},
		
		focus: function(){
			console.debug("QMConnectionGrid -- focus");
		},
		
		getCount: function() {
			var valueAsString = this.getValueAsString();
			if (valueAsString.length === 0) {
				return 0;
			}
			return valueAsString.split(",").length;
		}, 
		
		getValueAsString: function() {
			// summary:
			//		return a comma-separated list of select queue manager connection names
			console.debug(this.declaredClass + ".getValueAsString()");

			var value = "";
			for (var prop in this.checkboxes) {
				//console.debug("prop="+prop);
				var checkbox = this.checkboxes[prop];
				if (checkbox && checkbox.checked === true) {
					//console.debug("checked");		
					var res = this.store.query({ Name: prop });
					if (res) {						
						var qmc = res.map(function(item){ return item.Name; })[0];
						if (qmc) {							
							var list = value + ",";
							if (list.indexOf(qmc+",") === -1) {
								if (value !== "") {
									value += ",";
								}
								value += qmc;						
							}
						}
					}
				}
			}

			console.debug("getValueAsString returns ", value);
			return value;
		},

		_setValueAttr: function(valueArray) {
			// mark as "checked" the applied box in the grid rows matching the elements of valueArray 
			// param: input array
			console.debug(this.declaredClass + ".setValueAttr(" + valueArray + ")");
			var list = valueArray + ",";
			if (this.grid) {
				for (var prop in this.checkboxes) {
					var checkbox = this.checkboxes[prop];
					if (checkbox) {
						//console.debug("prop="+prop+" checkbox="+checkbox);
						var res = this.store.query({ Name: prop });
						var qmc = res.map(function(item){ return item.Name; })[0];
						//console.debug("qmc="+qmc);
						if (list.indexOf(qmc+",") === -1) {
							checkbox.set("checked", false);
						} else {
							checkbox.set("checked", true);
						}
					} else {
						console.debug("Oops, checkbox " + prop + " wasn't found");
					}
				}
			} else {
				console.debug("QMConnectionGrid -- _setValueAttr -- grid was null");
			}
		},
		
		_setDisabledAttr: function(disabled) {
			// sets attribute on each of the check boxes
			if (this.grid) {
				for (var prop in this.checkboxes) {
					var checkbox = this.checkboxes[prop];
					if (checkbox) {
						checkbox.set('disabled', disabled);
					}
				}
			}
		},

		_initGrid: function() {
			console.debug(this.declaredClass + ".initGrid()");
			var parent = this.domNode;
			var gridId = this.id + "QMConnectionGrid";
			domConstruct.place("<div id='"+gridId+"'></div>", this.contentId);

			this.checkboxes = {};
			var self = this; 
			this.grid = new Grid({
				id: gridId,
				cacheClass: Sync,
				store: this.store,
				autoHeight: true,
				autoWidth: true,
				structure: [
					{ id: "associated", name: nls.messaging.destinationMapping.associated, field: "associated", dataType: "string", width: "70px", widgetsInCell: true,
						setCellValue: function(data) {
							self.checkboxes[data] = this.checkbox;
						},
						decorator: function(data) {
							return '<div data-dojo-type="idx.form.CheckBox" data-dojo-attach-point="checkbox"></div>';
						},
						formatter: function(data) {
							return data.Name;
						}
					},
					{	id: "Name", name: nls.messaging.connectionProperties.grid.name, field: "Name", dataType: "string", width: "80px",
						decorator: Utils.nameDecorator
					},
					{	id: "ConnectionName", name: nls.messaging.connectionProperties.grid.connName, field: "ConnectionName", dataType: "string", width: "150px",
						decorator: function(list) {
							return list.split(",").join(",<br>\n");
						}
					}
				],
				modules: [
					CellWidget, Sort, Focus, ColumnResizer
				]
			}, gridId);
			domClass.add(this.grid.domNode,"compact");
			this.grid.startup();
		}

	});

	return QMConnectionGrid;
});
