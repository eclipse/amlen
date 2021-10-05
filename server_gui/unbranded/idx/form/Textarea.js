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
	"dojo/_base/array",
	"dojo/has",
	"dojo/_base/event",
	"dojo/dom-style",
	"dojo/dom-attr",
	"dojo/dom-geometry",
	"dijit/form/Textarea",
	"dijit/form/SimpleTextarea",
	"dijit/form/TextBox",
	"idx/widget/HoverHelpTooltip",
	"../util",
	"./_CssStateMixin",
	"./_ValidationMixin",
	"./_CompositeMixin",
	"idx/has!#mobile?idx/_TemplatePlugableMixin:#platform-plugable?idx/_TemplatePlugableMixin", 
	"idx/has!#mobile?idx/PlatformPluginRegistry:#platform-plugable?idx/PlatformPluginRegistry",
	
	"idx/has!#idx_form_Textarea-desktop?dojo/text!./templates/Textarea.html"  // desktop widget, load the template
		+ ":#idx_form_Textarea-mobile?"										// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/Textarea.html"						// global desktop platform, load template
		+ ":#mobile?"														// global mobile platform, don't load
		+ ":dojo/text!./templates/Textarea.html", 							// no dojo/has features, load the template
		
	"idx/has!#idx_form_Textarea-mobile?./plugins/mobile/TextareaPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_Textarea-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/mobile/TextareaPlugin"							// global mobile platform, load plugin
		+ ":"
], function(declare, lang, array, has,  event, domStyle, domAttr, domGeometry, 
	Textarea, SimpleTextarea, TextBox, HoverHelpTooltip, iUtil, _CssStateMixin, 
	_ValidationMixin, _CompositeMixin, _TemplatePlugableMixin, PlatformPluginRegistry, 
	desktopTemplate, MobilePlugin){
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2
	
    /**
	 * @name idx.form.Textarea
	 * @class idx.form.Textarea is a composite widget which enhanced dijit.form.Textarea with following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in label positioning</li>
	 * <li>Built-in hint</li>
	 * <li>Built-in hint positioning</li>
	 * <li>Built-in required attribute</li>
	 * <li>Built-in validation support</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit.form.Textarea
	 * @augments idx.form._CssStateMixin
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._ValidationMixin
	 */

	var Textarea = declare([Textarea, _CompositeMixin,  _CssStateMixin, _ValidationMixin], 
	/**@lends idx.form.Textarea.prototype*/
	{
		/**
		 * 
		 */
		instantValidate: false,
		/**
		 * 
		 */
		templateString: desktopTemplate,
		/**
		 * 
		 */
		baseClass: "idxTextareaWrap",
		/**
		 * 
		 */
		oneuiBaseClass: "dijitTextBox dijitTextArea dijitExpandingTextArea",
		/**
		 * 
		 */
		rows: 1,
		/**
		 * 
		 */
		_minScrollHeight: 0,
		/**
		 *  Set height fixed
		 *  Default value is false
		 */
		heightFixed:false,
		/**
		 * 
		 */
		postCreate: function(){
			// summary:
			//     add style height setting, change to integer of rows number
			var textarea = this.textbox, 
				height = parseInt(this.srcNodeRef ? this.srcNodeRef.style.height : "0"), 
				empty = false, 
				tempValue = textarea.value, 
				tempRows = 1;
			textarea.value = ' ';
			while ( textarea.scrollHeight < height){
				textarea.value += ' \n';
				tempRows++;
			}
			textarea.value = tempValue;
			
			if ( tempRows > this.rows ){
				this._setRowsAttr( tempRows );
			}
			
			this._event = {
				"input" : "_onInput",
				"blur" 	: "_onBlur",
				"focus" : "_onFocus"
			};
			this.inherited(arguments);
			array.forEach(array.filter(this._connects, function(conn){ 
				return conn && conn[0] && conn[0][1] == "onfocus"; 
			}), this.disconnect, this);
			this._resize();

			// for task 13788
			// if heightFixed true, when input, set scroll bar automatically
			// by yifan ruan
			if(this.heightFixed)
				this.textbox.style.overflowY = "auto";
		},
		/**
		 * overwrite this funtion for  Defect 11013 Fixed
		 */
		_onBlur: function(){
			this.validate(this.focused);
			//
			// Trigger the onChanged event in the following method
			//
			this.inherited(arguments);
		},
		/**
		 * 
		 */
		_onInput: function(){
			SimpleTextarea.prototype._onInput.apply(this, arguments);
			this._resizeVertical();
		},
		/**
		 * 
		 */
		_resizeLater: function(){
			this.defer("_resizeVertical");
		},
		/**
		 * 
		 */
		_isEmpty: function(){
			return (this.trim ? /^\s*$/ : /^$/).test(this.get("value")); // Boolean
		},
		/**
		 * 
		 */
		_setValueAttr: function(){
			TextBox.prototype._setValueAttr.apply(this, arguments);
			this._updatePlaceHolder();
			//this.validate(this.focused);
		},
		/**
		 * 
		 * @param {Object} mousedownNode
		 */
		_isValidFocusNode: function(mousedownNode){
			return (this.hintPosition == "inside" && mousedownNode == this._phspan || 
				mousedownNode == this.oneuiBaseNode.parentNode) || this.inherited(arguments);
		},
		/**
		 * 
		 * @param {Object} Int rows
		 */
		_setRowsAttr: function(/*Int*/ rows){
			domAttr.set(this.oneuiBaseNode, "rows", rows);
			this.rows = parseInt(rows);
		},
		/**
		 * 
		 * @param {Object} Int cols
		 */
		_setColsAttr: function(/*Int*/ cols){
			domAttr.set(this.oneuiBaseNode, "cols", cols);
			this.cols = parseInt(cols);
		},
		/**
		 * 
		 */
		_estimateHeight: function(){
			// summary:
			//		Approximate the height when the textarea is invisible with the number of lines in the text.
			//		Fails when someone calls setValue with a long wrapping line, but the layout fixes itself when the user clicks inside so . . .
			//		In IE, the resize event is supposed to fire when the textarea becomes visible again and that will correct the size automatically.
			//
			var textarea = this.textbox;
			textarea.rows = (textarea.value.match(/\n/g) || []).length + 1;
			textarea.rows = (textarea.rows < this.rows) ? this.rows : textarea.rows;
		},
		_resize: function(){
			this._errorIconWidth = 27;
			this.inherited(arguments);
			this._resizeVertical();
			this._resizeHorizontal();
		},
		_resizeHorizontal:function(){
			if ( this.cols > 0 ){
				domStyle.set(this.oneuiBaseNode, "width", "");
			}
		},
		_resizeVertical: function(){

			// for task 13788
			// if heightFixed false, resize height
			// otherwise no resize
			// by yifan ruan
			if(!this.heightFixed){

			// summary:
			//		Resizes the textarea vertically (should be called after a style/value change)
			var textarea = this.textbox, self = this;
			function textareaScrollHeight(){
				var empty = false, sh = 0;
				if ( textarea.value === ''){
					for (var index = 0; index < self.rows - 1; index++){
						textarea.value += ' \n';
					}
					empty = true;
					
					self._minScrollHeight = sh = textarea.scrollHeight;
				}
				else{
					sh = ( textarea.scrollHeight < self._minScrollHeight) ? self._minScrollHeight : textarea.scrollHeight; 
				}

				if(empty){ textarea.value = ''; }
				return sh;
			}

			if(textarea.style.overflowY == "hidden"){ textarea.scrollTop = 0; }
			if(this.busyResizing){ return; }
			this.busyResizing = true;
			

			if (textareaScrollHeight() || textarea.offsetHeight) {
				var newH = textareaScrollHeight() + Math.max(textarea.offsetHeight - textarea.clientHeight, 0);
				//newH = (newH < this._height) ? this._height : newH;
				var newHpx = newH + "px";
				if (newHpx != textarea.style.height) {
					textarea.style.height = newHpx;
				}
				if (has("textarea-needs-help-shrinking") ) {
					var origScrollHeight = textareaScrollHeight(), newScrollHeight = origScrollHeight, origMinHeight = textarea.style.minHeight, decrement = 4, // not too fast, not too slow
						thisScrollHeight, origScrollTop = textarea.scrollTop;
					textarea.style.minHeight = newHpx; // maintain current height
					textarea.style.height = "auto"; // allow scrollHeight to change
					while ( newH > 0 ) {
						textarea.style.minHeight = Math.max(newH - decrement, 4) + "px";
						thisScrollHeight = textareaScrollHeight();
						var change = newScrollHeight - thisScrollHeight;
						newH -= change;
						if (change < decrement) {
							break; // scrollHeight didn't shrink
						}
						newScrollHeight = thisScrollHeight;
						decrement <<= 1;
					}
					textarea.style.height = newH + "px";
					textarea.style.minHeight = origMinHeight;
					textarea.scrollTop = origScrollTop;
				}
				textarea.style.overflowY = textareaScrollHeight() > textarea.clientHeight ? "auto" : "hidden";
				if (textarea.style.overflowY == "hidden") {
					textarea.scrollTop = 0;
				}
				this._onExpanded();
			}
			else {
				// hidden content of unknown size
				this._estimateHeight(); 
			}
	
			this.busyResizing = false;
		}

		},
		displayMessage: function(/*String*/ message, /*Boolean*/ force){
			if(message){
				if(!this.messageTooltip){
					this.messageTooltip = new HoverHelpTooltip({
						connectId: [this.iconNode],
						label: message,
						position: this.tooltipPosition,
						forceFocus: false
					});
				}else{
					this.messageTooltip.set("label", message);
				}
				if(this.focused || force){
					var node = domStyle.get(this.iconNode, "visibility") == "hidden" ? this.oneuiBaseNode : this.iconNode;
					this.messageTooltip.open(node);
				}
			}else{
				this.messageTooltip && this.messageTooltip.close();
			}
		},
		_onExpanded: function(){}
	});
	
	if(has("mobile") || has("platform-plugable")){
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/Textarea", {	
			desktop: "inherited",	// no plugin for desktop, use inherited methods  
			mobile: MobilePlugin	// use the mobile plugin if loaded
		});
		
		Textarea = declare([Textarea,_TemplatePlugableMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			
			
			templatePath: require.toUrl("idx/form/templates/Textarea.html"),  
		
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			 			
			displayMessage: function(message){
				return this.doWithPlatformPlugin(arguments, "displayMessage", "displayMessage", message);
			},
			_setHelpAttr: function(helpText){
				return this.doWithPlatformPlugin(arguments, "_setHelpAttr", "setHelpAttr", helpText);
			},
			_onExpanded: function(newHeight){
				return this.doWithPlatformPlugin(arguments, "_onExpanded", "onExpanded", newHeight);
			}
		});
	}
	return iForm.Textarea = declare("idx.form.Textarea", Textarea);
});
