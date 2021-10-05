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
	"dojo/dom-construct", 
	"dojo/_base/lang", 
	"dojo/on",
	"idx/form/TextBox",
	'ism/widgets/_RequiredIndicatorMixin',
	"dojo/text!idx/form/templates/TextBox.html",
	"dojo/text!./templates/TextBox.html",
	'ism/Utils'
], function(declare, query, domClass, domConstruct, lang, on, TextBox, _RequiredIndicatorMixin, idxTemplate, template, Utils){
	/**
	 * @name ism.widgets.TextBox
	 * @class ISM extension of One UI version TextBox
	 * @augments idx.form.TextBox
	 */
	return declare("ism.widgets.TextBox", [TextBox, _RequiredIndicatorMixin],
	{
		// summary:
		// 		An extension of the One UI TextBox that allows for the
		// 		definition of a idx.widget.HoverHelpTooltip between the
		// 		label and the textbox.  If tooltipContent is set,
		//		then a tooltip will be added just to the right of the label.
		tooltipContent: "",
		tooltipPosition: ["above","below"],
		instantValidate: true,
		templateString: idxTemplate,
		inputWidth: null,   // string,
		alignWithRequired: false,
		ismRequiredIcon: false,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			if (this.tooltipContent !== "") {
				this.templateString = template;
			}
			if (this.inputWidth) {
				this.fieldWidth = this.inputWidth;
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
			var requiredIconSpan = undefined;
			if (this.alignWithRequired) {
				requiredIconSpan = dojo.query(".idxRequiredIcon", "widget_"+this.id);
				if (requiredIconSpan.length > 0) {
					requiredIconSpan[0].innerHTML = "";
					domClass.replace(requiredIconSpan[0], "alignWithRequired", "idxRequiredIcon");
				}
			}
			if (this.ismRequiredIcon) {
				requiredIconSpan = dojo.query(".idxRequiredIcon", "widget_"+this.id);
				if (requiredIconSpan.length > 0) {
					domClass.replace(requiredIconSpan[0], "ismRequiredIcon", "idxRequiredIcon");
				}
			}
			
			
		},
		
		/*
		 * We need to override this in order to show validation errors as you type.
		 */
		_isValidSubset: function() {
			return false;
		},
	
		/**
		 * When disabling a required field, hides the required indicator
		 * @param disabled
		 */
		_setDisabledAttr: function(disabled) {
			this.inherited(arguments);
			if (this.get('required') === true) {
				this._toggleRequiredIndicator(!disabled);
			}
		},
		
		/**
		 * When setting a required field read-only, hides the required indicator
		 * @param disabled
		 */
		_setReadOnlyAttr: function(readOnly) {
			this.inherited(arguments);
			if (this.get('required') === true) {
				this._toggleRequiredIndicator(!readOnly);
			}
		},
		
		// override due to IDX defect 11440
		_setPlaceHolderAttr: function(v){
			this._set("placeHolder", v);
			if(!this._phspan){
				this._attachPoints.push('_phspan');
				// dijitInputField class gives placeHolder same padding as the input field
				// parent node already has dijitInputField class but it doesn't affect this <span>
				// since it's position: absolute.
				this._phspan = domConstruct.create('span',{ onmousedown:function(e){ e.preventDefault(); }, 
					className:'dijitPlaceHolder dijitInputField', id: this.id + "_hint_inside"},this.textbox,'after');
				this.own(on(this._phspan, "touchend, MSPointerUp", lang.hitch(this, function(){
					// If the user clicks placeholder rather than the <input>, need programmatic focus.  Normally this
					// is done in _FormWidgetMixin._onFocus() but after [30663] it's done on a delay, which is ineffective.
					this.focus();
				})));
			}
			this._phspan.innerHTML="";
			this._phspan.appendChild(this._phspan.ownerDocument.createTextNode(v));
			this._updatePlaceHolder();
		},

		
		// override dijit.form.TextBox._updatePlaceHolder to clear placeholder when the 
		// box has focus
		_updatePlaceHolder: function(){
			if(this._phspan){
				this._phspan.style.display = (this.placeHolder&&!this.focused&&!this.textbox.value) ? "" : "none";
			}
		},
		
		// override idx.form._CompositeMixin._setHintAttr because we do not want invalid 
		// aria-describedby attributes to be created
		_setHintAttr: function(hint) { 
			if (!hint) {
				return;
			} else {
				this.inherited(arguments);
			}
		}

	});
});
