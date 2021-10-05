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
	"dojo/_base/sniff",
	"dojo/dom-attr",
	"dojo/dom-class",
	"dojo/dom-style",
	"dojo/keys",
	"dijit/form/_FormSelectWidget",
	"dijit/_Container",
	"dijit/_base/wai",
	"./_CssStateMixin",
	"./_CompositeMixin",
	"./_ValidationMixin",
	"./_InputListMixin",
	"./_RadioButtonSetItem",
	"idx/widget/HoverHelpTooltip",
	"dojo/has!dojo-bidi?../bidi/form/RadioButtonSet",
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
	"idx/has!#idx_form_RadioButtonSet-desktop?dojo/text!./templates/RadioButtonSet.html" 	// desktop widget, load the template
		+ ":#idx_form_RadioButtonSet-mobile?"											// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/RadioButtonSet.html"						// global desktop platform, load template
		+ ":#mobile?"															// global mobile platform, don't load
		+ ":dojo/text!./templates/RadioButtonSet.html", 								// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_RadioButtonSet-mobile?./plugins/phone/RadioButtonSetPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_RadioButtonSet-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/RadioButtonSetPlugin"							// global mobile platform, load plugin
		+ ":"																// no features, don't load plugin

	// ====================================================================================================================
], function(declare, lang, array, has, domAttr, domClass, domStyle, keys, _FormSelectWidget, _Container, wai, _CssStateMixin, _CompositeMixin, _ValidationMixin, _InputListMixin, _RadioButtonSetItem, HoverHelpTooltip, bidiExtension, 
		TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin){
	var baseClassName = "idx.form.RadioButtonSet";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	if (has("dojo-bidi")) {
		baseClassName = baseClassName + "_";
	}		
	
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2
	
	/**
	 * @name idx.form.RadioButtonSet
	 * @class idx.form.RadioButtonSet is a composit widget which consists of a group of radiobuttons.
	 * RadioButtonSet can be created in the same way of creating a dijit.form.Select control.
	 * NOTE: The "startup" method should be called after a RadioButtonSet is created in Javascript.
	 * In order to get the value of the widget, you don't need to invoke the get("value") method of
	 * each radiobutton anymore, but simply call get("value") method of the RadioButtonSet.
	 * As a composite widget, it also provides following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in label positioning</li>
	 * <li>Built-in required attribute</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit.form._FormSelectWidget
	 * @augments dijit._Container
	 * @augments idx.form._CssStateMixin
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._ValidationMixin
	 * @augments idx.form._InputListMixin
	 * @example Programmatic:
	 *	new idx.form.RadioButtonSet({
	 *	options: [
	 *			{ label: 'foo', value: 'foo', selected: true },
	 *			{ label: 'bar', value: 'bar' }
	 *		]
	 *	});
	 *	
	 *Declarative:
	 *	&lt;select jsId="rbs" data-dojo-type="oneui.form.RadioButtonSet" data-dojo-props='
	 *		name="rbs", label="label1", value="foo"'>
	 *		&lt;option value="foo">foo</option>
	 *		&lt;option value="bar">bar</option>
	 *	&lt;/select>
	 *	
	 *Store Based:
	 *	var data = {
	 *		identifier: "value",
	 *		label: "label",
	 *		items: [
	 *			{value: "AL", label: "Alabama"},
	 *			{value: "AK", label: "Alaska"}
	 *		]
	 *	};
	 *	var readStore = new dojo.data.ItemFileReadStore({data: data});
	 *	var rbs = new idx.form.RadioButtonSet({
	 *		store: readStore
	 *	});
	 */
	iForm.RadioButtonSet = declare(baseClassName, [_FormSelectWidget, _Container, _CssStateMixin, _CompositeMixin, _ValidationMixin, _InputListMixin],
	/**@lends idx.form.RadioButtonSet.prototype*/
	{
		templateString: desktopTemplate,
		
		baseClass: "idxRadioButtonSetWrap",
		
		oneuiBaseClass: "idxRadioButtonSet",
		
		multiple: false,
		
		radioButtonSetItems: [], // need to override this class-static value in postMixInProperties
		
		instantValidate: false,
		
		postMixInProperties: function() {
			this.radioButtonSetItems = [];
			this.inherited(arguments);
		},
		buildRendering: function(){
			// Radio button set must have a name, otherwise all unnamed radio buttons will
			// be considered as one group.
			this.name = this.name || this.id;
			this.inherited(arguments);
		},
		postCreate: function(){
			this.inherited(arguments);
			this._resize();
		},
		_setNameAttr: function(value){
			this._set("name", value);
			array.forEach(this.getChildren(), function(item){ 
				item.set("name", value);
			});
		},
		
		_setValueAttr: function(/*anything*/ newValue, /*Boolean?*/ priorityChange){
			// summary:
			//		set the value of the widget.
			//		If a string is passed, then we set our value from looking it up.
			if(this._loadingStore){
				// Our store is loading - so save our value, and we'll set it when
				// we're done
				this._pendingValue = newValue;
				return;
			}
			var opts = this.getOptions() || [];
			if(!lang.isArray(newValue)){
				newValue = [newValue];
			}
			array.forEach(newValue, function(i, idx){
				if(!lang.isObject(i)){
					i = i + "";
				}
				if(typeof i === "string"){
					newValue[idx] = array.filter(opts, function(node){
						return node.value === i;
					})[0] || {value: "", label: ""};
				}
			}, this);
	
			// Make sure some sane default is set
			newValue = array.filter(newValue, function(i){ return i && i.value; });
			if(!this.multiple && (!newValue[0] || !newValue[0].value) && opts.length){
				newValue[0] = "";
			}
			
			// Set lastFocusedChild according to the new value
			this.lastFocusedChild = null;
			array.forEach(opts, function(i, index){
				i.selected = array.some(newValue, function(v){ return v.value === i.value; });
				if(i.selected && this.getChildren()[index]){
					this.lastFocusedChild = this.getChildren()[index]
					this.focusChild(this.lastFocusedChild);
				}
			}, this);
			var val = array.map(newValue, function(i){ return i.value; }),
				disp = array.map(newValue, function(i){ return i.label; });
	
			this._set("value", this.multiple ? val : val[0]);
			this._setDisplay(this.multiple ? disp : disp[0]);
			this._updateSelection();
			this._handleOnChange(this.value, priorityChange);
		},
		
		_addOptionItem: function(/* dojox.form.__SelectOption */ option){
			var item = new _RadioButtonSetItem({
				_inputId: this.id + "_RadioItem" + array.indexOf(this.options, option),
				option: option,
				name: this.name,
				disabled: option.disabled || this.disabled || false,
				readOnly: option.readOnly || this.readOnly || false,
				parent: this
			});
			
			domClass.toggle(item.domNode, "dijitInline", !(this.groupAlignment == "vertical"));
			this.addChild(item);
			this.radioButtonSetItems.push(item);
			if(option.selected){
				this.lastFocusedChild = item;
			}
			// IE8 standard document mode has a bug that we have to re-layout the dom
			// to make it occupy the space correctly.
			if(has("ie") > 7 && !has("quirks")) this._relayout(this.domNode);
			this.onAfterAddOptionItem(item, option);
		},
		//_isValid: function(isFocused){
		//	return (!this.required || this.value === 0 || !(/^\s*$/.test(this.value || ""))); 
		//},
		_isEmpty: function(){
			return /^\s*$/.test(this.value || "");
		},
		validate: function(/*Boolean*/ isFocused){
			// summary:
			//		Called by oninit, onblur, and onkeypress.
			// description:
			//		Show missing or invalid messages if appropriate, and highlight textbox field.
			//		Used when a select is initially set to no value and the user is required to
			//		set the value.
			var isValid = this._isValid(isFocused);
			this._set("state", isValid ? "" : (this._hasBeenBlurred || (isFocused && this.instantValidate)) ? "Error" : "Incomplete");
			this.focusNode.setAttribute("aria-invalid", isValid ? "false" : "true");
			var message = isValid ? "" : this.getErrorMessage();
			if(this.message !== message){
				this._set("message", message);
				this.displayMessage(this.message);
			}
			return isValid;
		},
		
		_setDisabledAttr: function(){
			this.inherited(arguments);
			this._refreshState();
		},
		_onBlur: function(){
			this.inherited(arguments);
			if(!this.instantValidate){this.validate();}
		},
		
		destroy: function(){
			this.inherited(arguments);
			for ( var index = 0; index < this.radioButtonSetItems.length; index++){
				var item = this.radioButtonSetItems[index];
				item.destroy();
			}
			this.radioButtonSetItems.length = 0;
		},
		
		// Defect 14370 move describedby property to proper refered node
		_setHelpAttr: function(helpText){
			this.inherited(arguments);
			wai.removeWaiState(this.focusNode, "describedby");
			this._addWaiState(this.stateNode, "describedby", this.id + "_helpInner");
		},

		_errorIconWidth: 45
	});
	
	if (has("dojo-bidi")) {
		baseClassName = baseClassName.substring(0, baseClassName.length-1);
		var baseRadioButtonSet = iForm.RadioButtonSet;
		iForm.RadioButtonSet = declare(baseClassName,[baseRadioButtonSet,bidiExtension]);
	}
	
	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/RadioButtonSet", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		
		iForm.RadioButtonSet = declare("idx.form.RadioButtonSet",[iForm.RadioButtonSet, TemplatePlugableMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			templatePath: require.toUrl("idx/form/templates/RadioButtonSet.html"),  
		
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			/**
			 * Stub Plugable method
			 */
			startup: function(){
				this.inherited(arguments);
				return this.doWithPlatformPlugin(arguments, "startup", "startup");
			},
			/**
			 * Stub Plugable method
			 */
			_addOptionItem: function(/* dojox.form.__SelectOption */ option){
				return this.doWithPlatformPlugin(arguments, "_addOptionItem", "_addOptionItem",option);
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
	return iForm.RadioButtonSet;
});
