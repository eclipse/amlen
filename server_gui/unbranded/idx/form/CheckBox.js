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
	"dojo/has",
	"dojo/_base/lang",
	"dijit/form/CheckBox",
	"./_CssStateMixin",
	"./_CompositeMixin",
	"./_ValidationMixin",
	"dojo/has!dojo-bidi?../bidi/form/CheckBox",
	
	// ====================================================================================================================
	// ------
	// Load _TemplatePlugableMixin and PlatformPluginRegistry if on "mobile" or if on desktop, but using the 
	// platform-plugable API.  Any prior call to PlaformPluginRegistry.setGlobalTargetPlatform() or 
	// PlatformPluginRegistry.setRegistryDefaultPlatform() sets "platform-plugable" property for dojo/has.
	// ------
	"idx/has!#mobile?idx/_TemplatePlugableMixin:#platform-plugable?idx/_TemplatePlugableMixin", 
	"idx/has!#mobile?idx/PlatformPluginRegistry:#platform-plugable?idx/PlatformPluginRegistry",
	
	// ------
	// We want to load the desktop template unless we are using the mobile implementation.
	// ------
	"idx/has!#idx_form_CheckBox-desktop?dojo/text!./templates/CheckBox.html" 	// desktop widget, load the template
		+ ":#idx_form_CheckBox-mobile?"											// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/CheckBox.html"						// global desktop platform, load template
		+ ":#mobile?"															// global mobile platform, don't load
		+ ":dojo/text!./templates/CheckBox.html", 								// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_CheckBox-mobile?./plugins/phone/CheckBoxPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_CheckBox-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/CheckBoxPlugin"							// global mobile platform, load plugin
		+ ":"																// no features, don't load plugin

	// ====================================================================================================================
], function(declare, has, lang, CheckBox, _CssStateMixin, _CompositeMixin, _ValidationMixin, bidiExtension, 
		TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin){
			
	
	var baseClassName = "idx.form.CheckBox";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	if (has("dojo-bidi")) {
		baseClassName = baseClassName + "_";
	}		
		
	
	var iForm = lang.getObject("idx.oneui.form", true);
	
	/**
	 * @name idx.form.CheckBox
	 * @class idx.form.CheckBox is implemented according to IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y6&vsub=*&hsub=*&openpanes=0100110000">Check Boxes Standard</a></b>.
	 * It is a composite widget which enhanced dijit.form.CheckBox with following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in required attribute</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit.form.CheckBox
	 * @augments idx.form._CssStateMixin
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._ValidationMixin
	 */
	iForm.CheckBox = declare(baseClassName, [CheckBox, _CssStateMixin, _CompositeMixin, _ValidationMixin],
	/**@lends idx.form.CheckBox.prototype*/
	{
		// summary:
		// 		One UI version CheckBox
		
		instantValidate: true,
		
		baseClass: "idxCheckBoxWrap",
		
		oneuiBaseClass: "dijitCheckBox",
		
		labelAlignment: "horizontal",
		
		templateString: desktopTemplate,
		
		postCreate: function(){
			this._event = {
				"input" : "onChange",
				"blur" 	: "_onBlur",
				"focus" : "_onFocus"
			}
			this.inherited(arguments);
		},
		
		_isEmpty: function(){
			return !this.get("checked");
		},
		_onBlur: function(evt){
			this.mouseFocus = false;
			this.inherited(arguments);
		},
		
		_setDisabledAttr: function(){
			this.inherited(arguments);
			this._refreshState();
		},
		
		_setLabelAlignmentAttr: null,
		_setFieldWidthAttr: null,
		_setLabelWidthAttr: null,
		resize: function(){return false;}
	});
	
	if (has("dojo-bidi")) {
		baseClassName = baseClassName.substring(0, baseClassName.length-1);
		var baseCheckBox = iForm.CheckBox;
		iForm.CheckBox = declare(baseClassName,[baseCheckBox,bidiExtension]);
	}
	
	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/CheckBox", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		var localOneuiBaseClass = iForm.CheckBox.prototype.oneuiBaseClass;
		if (MobilePlugin && MobilePlugin.prototype && MobilePlugin.prototype.oneuiBaseClass != null && MobilePlugin.prototype.oneuiBaseClass != undefined){
			localOneuiBaseClass = MobilePlugin.prototype.oneuiBaseClass;
		}

		iForm.CheckBox = declare("idx.form.CheckBox",[iForm.CheckBox, TemplatePlugableMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			templatePath: require.toUrl("idx/form/templates/CheckBox.html"),  
			/**
			 * 
			 */
			oneuiBaseClass: localOneuiBaseClass,
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			/**
			 * Stub Plugable method
			 */
			_setCheckedAttr: function(value){
				var node = this.focusNode;
				node.setAttribute("value",value);
				this.inherited(arguments);
			},		 			
			/**
			 * 
			 * @param {Object} message
			 */
			displayMessage: function(message){
				return this.doWithPlatformPlugin(arguments, "displayMessage", "displayMessage", message);
			},
			/**
			 * 
			 * @param {Object} helpText
			 */
			_setHelpAttr: function(helpText){
				return this.doWithPlatformPlugin(arguments, "_setHelpAttr", "setHelpAttr", helpText);
			}
		});
	}
	
	return iForm.CheckBox;
});