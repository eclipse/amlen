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
	"idx/form/RadioButtonSet",
	"dojo/text!idx/form/templates/RadioButtonSet.html",
	"dojo/text!./templates/RadioButtonSet.html"
], function(declare, RadioButtonSet, idxTemplate, template){
	/**
	 * @name ism.widgets.RadioButtonSet
	 * @class ISM extension of One UI version RadioButtonSet
	 * @augments idx.form.RadioButtonSet
	 */
	return declare("ism.widgets.RadioButtonSet", [RadioButtonSet],
	{
		// summary:
		// 		An extension of the One UI RadioButtonSet that allows for the
		// 		definition of a idx.widget.HoverHelpTooltip between the
		// 		label and the control.  If tooltipContent is set,
		//		then a tooltip will be added just to the right of the label.
		tooltipContent: "",
		tooltipPosition: ["above","below"],
		instantValidate: true,
		templateString: idxTemplate,

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
		}

	});
});
