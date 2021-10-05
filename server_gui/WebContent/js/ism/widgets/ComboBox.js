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
	"idx/form/ComboBox",
	"dojo/text!idx/form/templates/ComboBox.html",
	"dojo/text!./templates/ComboBox.html"
], function(declare, ComboBox, idxTemplate, template){
	/**
	 * @name ism.widgets.ComboBox
	 * @class ISM extension of One UI version ComboBox
	 * @augments idx.form.ComboBox
	 */
	return declare("ism.widgets.ComboBox", [ComboBox],
	{
		// summary:
		// 		An extension of the One UI ComboBox that allows for the
		// 		definition of a idx.widget.HoverHelpTooltip between the
		// 		label and the textbox.  If tooltipContent is set,
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
