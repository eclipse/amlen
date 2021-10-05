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

define([
	"dojo/_base/declare",
	'dojo/query',
	'dojo/dom-class',	
	"idx/form/Select",
	'ism/widgets/_RequiredIndicatorMixin',
	"dojo/text!idx/form/templates/Select.html",
	"dojo/text!./templates/Select.html"
], function(declare, query, domClass, Select, _RequiredIndicatorMixin, idxTemplate, template){
	/**
	 * @name ism.widgets.Select
	 * @class ISM extension of One UI version Select
	 * @augments idx.form.Select
	 */
	return declare("ism.widgets.Select", [Select, _RequiredIndicatorMixin],
	{
		// summary:
		// 		An extension of the One UI Select that allows for the
		// 		definition of a idx.widget.HoverHelpTooltip between the
		// 		label and the textbox.  If tooltipContent is set,
		//		then a tooltip will be added just to the right of the label.
		tooltipContent: "",
		tooltipPosition: ["above","below"],
		instantValidate: true,
		templateString: idxTemplate,
		alignWithRequired: false,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			if (this.tooltipContent !== "") {
				this.templateString = template;
			}
		},
		
		postCreate: function() {
			if (this.tooltipContent !== "") {
				dojo.parser.instantiate([dojo.byId(this.tooltipNode)]);
			}
			this.inherited(arguments);
		},
		
		startup: function() {
			this.inherited(arguments);
			if (this.tooltipContent !== "") {				
				var tooltip = dijit.byId(this.id+"_Tooltip");
				if (tooltip) {
					tooltip.set("connectId", this.id + "_Hover");	
				}
			}
			if (this.alignWithRequired) {
				var requiredIconSpan = dojo.query(".idxRequiredIcon", "widget_"+this.id);
				if (requiredIconSpan.length > 0) {
					requiredIconSpan[0].innerHTML = "";
					domClass.replace(requiredIconSpan[0], "alignWithRequired", "idxRequiredIcon");
				}
			}			
		}
	});
});
