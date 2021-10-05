/*
 * Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
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
	"dojo/_base/lang",
	"dojo/aspect",
	"dojo/dom-attr",
	"dojo/dom",
	"dojo/dom-construct",
	"dojo/dom-geometry",
	"dojo/i18n", 
	"dojo/query", 
	"dojo/dom-class", 
	"dojo/dom-style",
	"dojo/on",
	"dijit/_base/wai", 
	"idx/widget/HoverHelpTooltip",
	"../util",
	"../string"
	//"./_FocusManager" // Not able to extend in dojo1.10
], function(declare, lang, aspect, domAttr, dom, domConstruct, domGeometry, i18n, query, domClass, domStyle, on, wai, HoverHelpTooltip, iUtil, iString) {
	/**
	 * @public
	 * @name idx.form._CompositeMixin
	 * @class Mix-in class to provide customized label, hint, unit, and field layout, implemented according to 
	 * IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y29&vsub=*&hsub=*&openpanes=0000010000">Text Areas & Fields</a></b>.
	 * It takes the assumption that a composite widget will follow the dom structure below
	 * <br>
	 &lt;div class="idxComposite"&gt;
		&lt;div class="idxLabel"&gt;
			&lt;span&gt;*&lt;/span&gt;
				&lt;label dojotAttachPoint="compLabelNode"&gt;Label Text&lt;/label&gt;
		&lt;/div&gt;
		&lt;div&gt;other dom structure...&lt;/div&gt;&lt;div dojoAttachPoint="compUnitNode"&gt;unit text&lt;/div&gt;
		&lt;div dojoAttachPoint="compHintNode" class="idxHint dijitHidden"&gt;hint text&lt;/div&gt;
	 &lt;/div>
	 * <br>
	 * @aguments idx.form._FocusManager
	 */

	
	return declare("idx.form._CompositeMixin", null, 
	/**@lends idx.form._CompositeMixin#*/
	{
		/**
		 * Layout of the label and the field, "horizontal" or "vertical", implemented according to 
		 * IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y16&vsub=*&hsub=*&openpanes=0000011100">Field & Label Alignment</a></b>
		 * @type String
		 * @default "horizontal"
		 */
		labelAlignment: "horizontal",
		
		/**
		 * Label text
		 * @type String
		 */
		label: "",
		
		/**
		 * Width from the left of label to the left of corresponding field, this parameter works in the composite widget layout of "horizontal".
		 * @type String | Number
		 */
		labelWidth: "",
		
		/**
		 * Width of the field with a hidden validation icon
		 * @type String | Number
		 */
		fieldWidth: "",
		
		/**
		 * For input widgets only. The position of the hint text: "inside" / "outside", inner the field input or not.
		 * @type String
		 * @default "inside"
		 */
		hintPosition: "inside",
		
		/**
		 * For input widgets only. The hint text.
		 * @type String
		 */
		hint: "",
		
		/**
		 * Indicates that it's a required field or not. A required field will have a red asterisk.
		 * implemented according to 
	 	 * IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y17&vsub=*&hsub=*&openpanes=0000011100">Required Fields</a></b>.
		 * @type boolean
		 * @default false
		 */
		required: false,
		
		/**
		 * The text of unit for the numerical value input widget.
		 * @type String
		 */
		unit: "",
		
		/**
		 * Focus manager for all composite widget
		 * @type idx.form.FocusManager
		 * @private
		 */
		/** _focusManager: _focusManager **/
		
		
		/**
		 * Help message popup from help icon follows label.
		 * @type String
		 */
		help:"",
		
		_errorIconWidth: 27,
		
		postMixInProperties: function(){
			this.tooltipPosition = ["after-centered", "above"];
			this.inherited(arguments);
		},
		
		/**
		 * Handles resizing form widgets.
		 */
		resize: function() {
			
			if(this._holdResize()){
				return;
			}
			domStyle.set(this.domNode, {visibility: "hidden"});
			// if percentage style width then clear the label and field widths so the parent container
			// has the opportunity to resize
			if (iUtil.isPercentage(this._styleWidth)) {
				domStyle.set(this.labelWrap, {width: ""});
				domStyle.set(this.oneuiBaseNode, {width: ""});
			}
			
			// schedule a resize
			if (this._resizeTimeout) {
				clearTimeout(this._resizeTimeout);
				delete this._resizeTimeout;
			}
			this._resizeTimeout = setTimeout(lang.hitch(this, this._resize), 250);
		},
		/**
		 * Check if resize action should be hold, by widget visibility and applied width
		 */
		_holdResize: function(widgetInvisible){
			if(!this.domNode){return true;}
			if(!(this.labelWidth || this.fieldWidth || this._styleWidth)){
				domStyle.set(this.labelWrap, {width: ""});
				domStyle.set(this.oneuiBaseNode, {width: ""});				
				return true;
			}
			var swFixed = (this._styleWidth && !iUtil.isPercentage(this._styleWidth)),
				fwFixed = (this.fieldWidth && !iUtil.isPercentage(this.fieldWidth)),
				lwFixed = (this.labelWidth && !iUtil.isPercentage(this.labelWidth)),
				widthFixed = swFixed || (fwFixed&&lwFixed),				
				widgetInvisible = domGeometry.getContentBox(this.domNode).w <= 0;
				
			return (widthFixed && this._resized) || // If widget width is fixed, and widget has been resized of fit, then skip.
				(!widthFixed) && widgetInvisible; // If widget is invisible with percentage width, then skip.
		},
		/**
		 * Handles deferring the resize until the widget is started.  This function returns true
		 * if the resize should be deferred.
		 */
		_deferResize: function() {
			// check if the widget has no fixed width -- there is no point in resizing until the widget's 
			// DOM node is properly placed in the DOM since percentage width cannot be computed  before that
			if (!this._started) {
				// if we have not yet created a resize callback on "startup", then use aspect to do that
				if (!this._resizeHandle) {
					this._resizeHandle = aspect.after(this, "startup", lang.hitch(this, this._resize));
				}
				return true; // defer until startup
			}
			else {
				// if we are started and previously created a resizeHandle, then remove it
				if (this._resizeHandle) {
					this._resizeHandle.remove();
					delete this._resizeHandle;
				}
				
				return false;
			}	
		},
		
		_resize: function(){
			if (this._resizeTimeout) { 
				clearTimeout(this._resizeTimeout); 
				delete this._resizeTimeout; 
			}
			var deferring = this._deferResize();
			if (deferring) {
				return;
			}
			if(this._holdResize()){
				return;
			}
			
			var labelWidth = this.labelWidth,
				fieldWidth = this.fieldWidth,
				styleWidth = this._styleWidth;

			var	realWidth = domGeometry.getContentBox(this.domNode).w,
				realWidth = ((realWidth <= 0) && styleWidth)? iUtil.normalizedLength(styleWidth,this.domNode) : realWidth;

			if (!styleWidth) {
				if (iUtil.isPercentage(labelWidth)) domStyle.set(this.labelWrap, "width", "");
				if (iUtil.isPercentage(fieldWidth)) domStyle.set(this.oneuiBaseNode, "width", "");				
			}
			
			if(this.label && this.labelAlignment == "horizontal"){
				if(labelWidth){
					if(iUtil.isPercentage(labelWidth)){
						labelWidth = Math.floor(realWidth * parseInt(labelWidth)/100) - 10;
					}
					domStyle.set(this.labelWrap, "width", iUtil.normalizedLength(labelWidth,this.domNode) + "px");
				} else {
					domStyle.set(this.labelWrap, "width", "");
				}
				if(fieldWidth){
					var isFieldWidthPercentage = iUtil.isPercentage(fieldWidth);
					if(isFieldWidthPercentage){
						fieldWidth = Math.floor(realWidth * parseInt(fieldWidth)/100);
					}
					domStyle.set(this.oneuiBaseNode, "width", 
						iUtil.normalizedLength(fieldWidth,this.domNode) - (isFieldWidthPercentage ? (this._errorIconWidth + 2) : 0) + "px");
				} else {
					domStyle.set(this.oneuiBaseNode, "width", "");
				}
				if (styleWidth && !fieldWidth) {
					var compLabelWidth = domGeometry.getMarginBox(this.labelWrap).w;
					var compFieldWidth = realWidth - this._errorIconWidth - compLabelWidth - 2;
					if (compFieldWidth > 0) {
						domStyle.set(this.oneuiBaseNode, "width", compFieldWidth + "px");
					}
				}
			}else{
				if(styleWidth){
					domStyle.set(this.oneuiBaseNode, "width", realWidth - this._errorIconWidth - 2 + "px");
				}else if(fieldWidth && !iUtil.isPercentage(fieldWidth)){
					domStyle.set(this.oneuiBaseNode, "width", iUtil.normalizedLength(fieldWidth,this.domNode) + "px");
				} else {
					domStyle.set(this.oneuiBaseNode, "width", "");
				}
			}
			this._resizeHint();
			domStyle.set(this.domNode, {visibility: ""});
			this._resized = true;
		},
		_resizeHint: function(){
			if(this.hint && (this.hintPosition == "outside") && this._created){
				var inputWidth = domStyle.get(this.fieldWrap || this.oneuiBaseNode, "width")/*valication icon placeholder*/;
				domStyle.set(this.compHintNode, "width", inputWidth? inputWidth + "px" : "");
			}
		},
		_setStyleAttr: function(style){
			// If there's no label share the widget width with field,
			// field would occupy whole widget width if "width" is specified in given style.
			this.inherited(arguments);
			this._styleWidth = iUtil.getValidCSSWidth(style);
			this._created && this._resize();
		},
				/**
		 * Set the width of label, the width is from the start of label to the start of the field.
		 * @public
		 * @param {string | number} width 
		 * Unit of "pt","em","px" will be normalized to "px", and "px" by default for numeral value.
		 */
		_setLabelWidthAttr: function(/*String | Integer*/width){
			domStyle.set(this.labelWrap, "width", typeof width === "number" ? width + "px" : width);
			this._set("labelWidth", width);
			this._created && this._resize();
		},
		
		/**
		 * Set the width of field with a hidden validation icon.
		 * @public
		 * @param {string | number} width 
		 * Unit of "pt","em","px" will be normalized to "px", and "px" by default for numeral value.
		 */
		_setFieldWidthAttr: function(/*String | Integer*/width){
			if(!iUtil.isPercentage(width)){
				domStyle.set(this.oneuiBaseNode, "width", typeof width === "number" ? width + "px" : width);
			}
			this._set("fieldWidth", width);
			this._created && this._resize();
		},
		
		/**
		 * Set the alignment for the label and field,  update the style of the label node to make 
		 * it be at the right place.
		 * @public
		 * @param {string} alignment
		 * The alignment of the label and field. Can be "vertical" or "horizontal".
		 * If "vertical" is used, the label is put above the TextBox.
		 * If "horizontal" is used, the label is put on the left of the TextBox (on
		 * the right of the TextBox if RTL language is used).
		 */
		_setLabelAlignmentAttr: function(/*String*/ alignment){
			var h = alignment == "horizontal";
			domClass.toggle(this.labelWrap, "dijitInline", h);
			query(".idxCompContainer", this.domNode).toggleClass("dijitInline", h);
			this._set("labelAlignment", alignment);
			this._resize();
		},
		
		_setupHelpListener: function() {
			if (this._helpListener) return;
			var blurHandler = function(e) {
				if (this.widget.helpTooltip) {
					this.widget.helpTooltip.close(true);
				}
				if (this.handle) {
					this.handle.remove();
					delete this.handle;
					this.widget._blurHandler = null;
				}
			};
			var keyHandler = function(e) {
				var charOrCode = e.keyCode;
				if(e.type == "keydown"){
					if(e.ctrlKey && e.shiftKey && (charOrCode == 191) && this.helpTooltip && this._help) {
						this.helpTooltip.open(this.helpNode);
				
						if (e.target && !this._blurHandler) {
							var scope = {
									handle: null,
									widget: this
							};
							this._blurHandler = scope.handle = on(e.target, "blur", lang.hitch(scope, blurHandler));
						}
					}
				}
			};
			var node = (this.oneuiBaseNode) ? this.oneuiBaseNode : this.focusNode;
			if (node) {
				this._keyListener = this.own(on(node, "keydown", lang.hitch(this, keyHandler)));
			}
		},

		/**
		 * check before add value to aria state
		 */
		_addWaiState: function(/*Element*/ elem, /*String*/ state, /*String*/ value){
			var preState = wai.getWaiState(elem, state);
			if(preState){
				if(preState.indexOf(value) ==-1) {
					wai.setWaiState(elem, state, preState + " " + value);
				} 
			} else {
				wai.setWaiState(elem, state, value);				
			}
		},
		
		_setHelpAttr: function(/*String*/ helpText){
			this.help = helpText; // set the help to what the caller provided
			this._help = helpText = iString.nullTrim(helpText); // set the internal value to the trimmed version
			if (helpText) {
				if (!this.helpNode) {
					this.helpNode = domConstruct.toDom("<div class='dijitInline idxTextBoxHoverHelp'><span class='idxHelpIconAltText'>?</span><span id='" + this.id + "_helpInner' class='dijitHidden'>" + helpText + "</span></div>");
					this.helpInner = this.helpNode.childNodes[1];
					domConstruct.place(this.helpNode, this.compLabelNode, "after");
					this.helpTooltip = new HoverHelpTooltip({
						connectId: [this.helpNode],
						label: helpText,
						position: ['above', 'below'],
						forceFocus: false,
						textDir: this.textDir
					});
					this._setupHelpListener();
					this._addWaiState(this.focusNode, "describedby", this.id + "_helpInner");
				}
				else {
					this._setupHelpListener();
					if(this.helpInner){
						this.helpInner.innerHTML = helpText;
					}					
					this.helpTooltip.set("label", helpText);
					domClass.remove(this.helpNode, "dijitHidden");
				}
			}
			else {
				if (this.helpNode) {
					this.helpTooltip.set("label", "");
					domClass.add(this.helpNode, "dijitHidden");
					if(this.helpInner){
						this.helpInner.innerHTML = "";
					}
				}
			}
		},
		
		/**
		 * Set the label text. Update the content of the label node.
		 * @public
		 * @param {string} label
		 * The text will be displayed as the content of the label. If text is null or
		 * empty string, nothing would be displayed.
		 */
		_setLabelAttr: function(/*String*/ label){
			this.compLabelNode.innerHTML = label;
			var islabelEmpty = /^\s*$/.test(label);
			domAttr[islabelEmpty?"remove":"set"](this.compLabelNode, "for", this.id);
			domClass.toggle(this.labelWrap, "dijitHidden", islabelEmpty);
			this._set("label", label);
			this.compLabelNode.setAttribute("id", this.id + "_label");
		},
		
		/**
		 * Set this field as a required field or not. If this field is required,
		 * a red asterisk will be shown at the start of label.
		 * @public
		 * @param {boolean} required
		 */
		_setRequiredAttr: function(/*Boolean*/ required){
			wai.setWaiState(this.focusNode, "required", required + "");
			this._set("required", required);
			this._refreshState();
		},
		
		_refreshState: function(){
			if(!this._created){return;}
			if(this.disabled){
				this._set("state", "");
			}else if(this.required && this._isEmpty(this.value) && (this.state == "")){
				this._set("state", "Incomplete");
			}
		},
		_setValueAttr: function(){
			// summary:
			//		Hook so set('value', ...) works.
			this.inherited(arguments);
			this.validate(this.focused);
		},
		
		/**
		 * Set position of the hint text. If position is "outside", update the content
		 * of the hint node. If position is "inside" and the value of the TextBox is
		 * null, set the value of the TextBox to the hintText
		 * @protected
		 * @param {string} position
		 * The position of the label. Can be "outside" or "inside".
		 * If "outside" is used, the hint text is put below the TextBox.
		 * If "inside" is used and the TextBox has a value, display the value in the TextBox. Once
		 * the value of the TextBox is null, display the hint text inside the TextBox in a specified
		 * color (e.g: gray).
		 */
		_setHintPositionAttr: function(/*String*/ position){
			if(!this.compHintNode){ return; }
			this._set("hintPosition", position);
			domClass.toggle(this.compHintNode, "dijitVisible", position != "inside");
			this.set("hint", this.hint);
			this._resizeHint();
		},
		
		/**
		 * Set the hint text
		 * @public
		 * @param {string} hint
		 * The text will be displayed inside or below the TextBox per the "position" attribute.
		 */
		_setHintAttr: function(/*String*/ hint){
			this.set("placeHolder", this.hintPosition == "inside" ? hint : "");
			if(!this.compHintNode){ return; }
			this.compHintNode.innerHTML = this.hintPosition == "inside" ? "" : hint;
			
			if(this.hintPosition == "outside"){
				domAttr.set(this.compHintNode, "id", this.id + "_hint_outside");
				this._addWaiState(this.focusNode, "describedby", this.id + "_hint_outside");
			} else {
				var compHintId = this.id + "_hint_outside";
				var preState = wai.getWaiState(this.focusNode, "describedby");
				if(preState.indexOf(compHintId) != -1){
					var newState = preState.replace(compHintId, "");
					if(newState){
						wai.setWaiState(this.focusNode, "describedby", preState.replace(compHintId, ""));	
					} else {
						wai.removeWaiState(this.focusNode, "describedby");
					}
					
				}
			}
			this._set("hint", hint);
			this._resizeHint();
		},
		
		_setPlaceHolderAttr: function(v){
			this._set("placeHolder", v);
			if(v === null || v === undefined){
				v = "";
			}
			if(this.focusNode.placeholder !== undefined && this._placeholder !== false){
				domAttr.set(this.focusNode, "placeholder", v);
				this._placeholder = v;
			}else{
				if(!this._phspan){
					this._attachPoints.push('_phspan');
					this._phspan = domConstruct.create('span',{
						className:'dijitPlaceHolder dijitInputField',
						id: this.id + "_hint_inside"
					},this.focusNode,'after');
				}
				//Defect 14383
				//When clicking placeholder element, the input should get focused
				this.own(
					on(this._phspan, "mousedown", function(evt){ evt.preventDefault(); }),
					on(this._phspan, "touchend, pointerup, MSPointerUp", lang.hitch(this, function(){
						this.focus();
					}))
				);
				dom.setSelectable(this._phspan, false);
				this._phspan.innerHTML = "";
				this._phspan.appendChild(document.createTextNode(v));
				this._phspan.style.display=(this.placeHolder&&!this.focused&&!this.textbox.value)?"":"none";
			}
		},
		
		/**
		 * Set the text of unit
		 * @public
		 * @param {string} unit
		 * The unit will be displayed on the right of the input box(on the left of the input
		 * box if RTL language is used).
		 */
		_setUnitAttr: function(/*String*/ unit){
			if(!this.compUnitNode){ return; }
			this.compUnitNode.innerHTML = unit;
			domAttr.set(this.compUnitNode, "id", this.id + "_unitNode");
			this._addWaiState(this.focusNode, "describedby", this.id + "_unitNode");
			domClass.toggle(this.compUnitNode, "dijitHidden", /^\s*$/.test(unit));
			this._set("unit", unit);
		},
		_isValidFocusNode: function(mousedownNode){
			return dom.isDescendant(mousedownNode, this.oneuiBaseNode) ||
				!dom.isDescendant(mousedownNode, this.domNode);
		},
		
		/**
		 * Reset the value and state of the composite widget.
		 * @public
		 */
		reset: function(){
			this.set("state", this.required ? "Incomplete" : "");
			this.message = "";
			this.inherited(arguments);
		},
		/**
		 * Validate value, directly get focus and show error if turn out to be invalid.
		 * @public
		 */
		validateAndFocus: function(){
			if(this.validate && !this.disabled){
				var hasBeenBlurred = this._hasBeenBlurred;
				this._hasBeenBlurred = true;
				var valid = this.validate();
				if(!valid){
					this.focus();
				}
				this._hasBeenBlurred = hasBeenBlurred
				return valid;
			}
			return true;
		}
	});
});
