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
	"dojo/has",
	"dojo/dom-style",
	"dijit/form/ValidationTextBox",
	"idx/widget/HoverHelpTooltip",
	"./_CssStateMixin",
	"./_CompositeMixin",
	
	"idx/has!#mobile?idx/_TemplatePlugableMixin:#platform-plugable?idx/_TemplatePlugableMixin", 
	"idx/has!#mobile?idx/PlatformPluginRegistry:#platform-plugable?idx/PlatformPluginRegistry",
	
	"idx/has!#idx_form_TextBox-desktop?dojo/text!./templates/TextBox.html"  // desktop widget, load the template
		+ ":#idx_form_TextBox-mobile?"										// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/TextBox.html"						// global desktop platform, load template
		+ ":#mobile?"														// global mobile platform, don't load
		+ ":dojo/text!./templates/TextBox.html", 							// no dojo/has features, load the template
		
	"idx/has!#idx_form_TextBox-mobile?./plugins/mobile/TextBoxPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_TextBox-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/mobile/TextBoxPlugin"							// global mobile platform, load plugin
		+ ":"																// no features, don't load plugin
		
], function(declare, lang, has, domStyle, ValidationTextBox, HoverHelpTooltip, _CssStateMixin, _CompositeMixin, 
	_TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin){
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2
	/**
	 * @name idx.form.TextBox
	 * @class One UI version.
	 */
	var TextBox = declare([ValidationTextBox, _CompositeMixin, _CssStateMixin], {
		/**@lends idx.form.TextBox*/
		
		// summary:
		//		Base class for textbox widgets with the ability to validate content of various types and provide user feedback.
		// tags:
		//		protected
		// instantValidate: Boolean
		//		Fire validation when widget get input by set true, 
		//		fire validation when widget get blur by set false
		instantValidate: false,
		
		templateString: desktopTemplate,
		baseClass: "idxTextBoxWrap",
		oneuiBaseClass: "dijitTextBox dijitValidationTextBox",
		/**
		 * Deprecated, use "help" instead
		 */
		hoverHelpMessage: "",
		
		postCreate: function(){
			this.inherited(arguments);
			if (this.instantValidate) {
				this.connect(this, "_onFocus", function(){
					if (this._hasBeenBlurred && (!this._refocusing)) {
						this.validate(true);
					}
				});
				this.connect(this, "_onInput", function(){
					this.validate(true);
				});
			}
			else {
				this.connect(this, "_onFocus", function(){
					if (this.message && this._hasBeenBlurred && (!this._refocusing)) {
						this.displayMessage(this.message);
					}
				})
			}
			this._resize();
		},
		
		/**
		 * Provides a method to return focus to the widget without triggering
		 * revalidation.  This is typically called when the validation tooltip
		 * is closed.
		 */
		refocus: function(){
			this._refocusing = true;
			this.focus();
			this._refocusing = false;
		},
		
		/**
		 * deprecated,
		 * @param {string | number} width
		 * Unit of "pt","em","px" will be normalized to "px", and "px" by default for numeral value.
		 */
		_setHoverHelpMessageAttr: function(message){
			this.set("help", message);
		},
		/**
		* Overridable method to display validation errors/hints
		*/
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
		}
	});
	
	if(has("mobile") || has("platform-plugable")){
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/TextBox", {	
			desktop: "inherited",	// no plugin for desktop, use inherited methods  
			mobile: MobilePlugin	// use the mobile plugin if loaded
		});
		
		TextBox = declare([TextBox,_TemplatePlugableMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			
			
			templatePath: require.toUrl("idx/form/templates/TextBox.html"),  
		
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			 			
			displayMessage: function(message){
				return this.doWithPlatformPlugin(arguments, "displayMessage", "displayMessage", message);
			},
			_setHelpAttr: function(helpText){
				return this.doWithPlatformPlugin(arguments, "_setHelpAttr", "setHelpAttr", helpText);
			}
		});
	}
	return iForm.TextBox = declare("idx.form.TextBox", TextBox);
});