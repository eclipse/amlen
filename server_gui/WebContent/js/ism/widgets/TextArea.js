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
	"idx/form/Textarea",
	"dojo/text!idx/form/templates/Textarea.html"
], function(declare, query, domClass, TextArea, idxTemplate){
	/**
	 * @name ism.widgets.TextArea
	 * @class ISM extension of One UI version TextArea
	 * @augments idx.form.TextArea
	 */
	return declare("ism.widgets.TextArea", [TextArea],
	{
		// summary:
		// 		An extension of the One UI TextArea that allows setting the inputWidth
		//      consistent with ism.widgets.TextBox
		tooltipContent: "",
		tooltipPosition: ["above","below"],
		instantValidate: true,
		templateString: idxTemplate,
		inputWidth: null,   // string
		alignWithRequired: false,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
		},

		postCreate: function() {
			this.inherited(arguments);
			if (this.inputWidth !== null && this.inputWidth !== undefined) {
				this.stateNode.style.width = this.inputWidth;
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
