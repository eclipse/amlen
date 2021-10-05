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
	"dojo/query",
	"dijit/form/_FormSelectWidget",
	"dijit/_Container",
	"dijit/_base/wai", 
	"./_CssStateMixin",
	"./_CompositeMixin",
	"./_ValidationMixin",
	"./_InputListMixin",
	"./_CheckBoxListItem",
	"idx/widget/HoverHelpTooltip",
	"dojo/has!dojo-bidi?../bidi/form/CheckBoxList",
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
	"idx/has!#idx_form_CheckBoxList-desktop?dojo/text!./templates/CheckBoxList.html" 	// desktop widget, load the template
		+ ":#idx_form_CheckBoxList-mobile?"											// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/CheckBoxList.html"						// global desktop platform, load template
		+ ":#mobile?"															// global mobile platform, don't load
		+ ":dojo/text!./templates/CheckBoxList.html", 								// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_CheckBoxList-mobile?./plugins/phone/CheckBoxListPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_CheckBoxList-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/CheckBoxListPlugin"							// global mobile platform, load plugin
		+ ":"																// no features, don't load plugin

	// ====================================================================================================================
], function(declare, lang, array, has, domAttr, domClass, domStyle, query, _FormSelectWidget, _Container, wai, _CssStateMixin, _CompositeMixin, _ValidationMixin, _InputListMixin, _CheckBoxListItem, HoverHelpTooltip, bidiExtension, 
		TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin){
			
	var baseClassName = "idx.form.CheckBoxList";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	if (has("dojo-bidi")) {
		baseClassName = baseClassName + "_";
	}		

	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2
	
	/**
	 * @name idx.form.CheckBoxList
	 * @class idx.form.CheckBoxList is a composit widget which consists of a group of checkboxes.
	 * CheckBoxList can be created in the same way of creating a dijit.form.Select control.
	 * The only difference is that more than one options can be marked as selected. 
	 * NOTE: The "startup" method should be called after a CheckBoxList is created in Javascript.
	 * In order to get the value of the checkboxes, you don't need to invoke the get("value") method of
	 * each checkbox anymore, but simply call get("value") method of the CheckBoxList.
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
	 *	new idx.form.CheckBoxList({
	 *	options: [
	 *			{ label: 'foo', value: 'foo', selected: true },
	 *			{ label: 'bar', value: 'bar' },
	 *		]
	 *	});
	 *	
	 *Declarative:
	 *	&lt;select jsId="cbl" data-dojo-type="oneui.form.CheckBoxList" data-dojo-props='
	 *		name="cbl", label="CheckBoxList:", value="foo"'>
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
	 *	var cbl = new idx.form.CheckBoxList({
	 *		store: readStore
	 *	});
	 */
	iForm.CheckBoxList = declare(baseClassName, [_FormSelectWidget, _Container, _CssStateMixin, _CompositeMixin, _ValidationMixin, _InputListMixin],
	/**@lends idx.form.CheckBoxList.prototype*/
	{
		
		templateString: desktopTemplate,
		
		instantValidate: false,
		
		baseClass: "idxCheckBoxListWrap",
		
		oneuiBaseClass: "idxCheckBoxList",
		
		multiple: true,
		
		checkBoxListItems: [], // need to override this static value in postMixinProperties

		/**
		 * Fix defect 13980, clean up checkBoxListItems
		 */
		_loadChildren: function(){
			this.checkBoxListItems=[];
			this.inherited(arguments);
		},
		
		postMixInProperties: function() {
			this.checkBoxListItems = []; // set array to instance-local value
			this.inherited(arguments);
		},
		/**
		 * Fix defect 11116, recognize id property in option
		 */
		_fillContent: function(){
			this.inherited(arguments);
			if (this.srcNodeRef){
				query("> *", this.srcNodeRef).map(
					function(node, index){
						if ( node.getAttribute("id") )
							this.options[index].id = node.getAttribute("id");
					},this);
			}
		},
		/**
		 * 
		 * @param {Object} String label
		 */
		_setLabelAttr: function(/*String*/ label){
			this.inherited(arguments);
			domAttr.remove(this.compLabelNode, "for");
		},
		/**
		 * 
		 */
		postCreate: function(){
			this.inherited(arguments);
			this._resize();
		},
		/**
		 * 
		 * @param {Object} dojox.form.__SelectOption option
		 * Fix defect 11116
		 * Recognize the id property on option and set it to the id of the CheckBoxListItem
		 */
		_addOptionItem: function(/* dojox.form.__SelectOption */ option){
			var targetInputId =   this.id + "_CheckItem" + array.indexOf(this.options, option);
			var listItemParam = {
				_inputId: targetInputId,
				option: option,
				disabled: option.disabled || this.disabled || false,
				readOnly: option.readOnly || this.readOnly || false,
				parent: this
			};
			if ( option.id )
				listItemParam.id = option.id
			var item = new _CheckBoxListItem(listItemParam);
			domClass.toggle(item.domNode, "dijitInline", !(this.groupAlignment == "vertical"));
			this.addChild(item);
			this.checkBoxListItems.push(item);
			// IE8 standard document mode has a bug that we have to re-layout the dom
			// to make it occupy the space correctly.
			if(has("ie") > 7 && !has("quirks")) this._relayout(this.domNode);
			this.onAfterAddOptionItem(item, option);
		},
		
		_setNameAttr: function(value){
			this._set("name", value);
			domAttr.set(this.valueNode, "name", value);
		},
		
		_onBlur: function(evt){
			this.mouseFocus = false;
			this.inherited(arguments);
			if(!this.instantValidate){this.validate();}
		},
		
		/**
		 * Multiple set false in CheckBoxList
		 * Single Selection
		 * @param {option} Object
		 */
		getValueFromOpts: function(/*Object*/ option){
			if ( !this.multiple && option ){
				if (option.selected)
					return [option.value];
				else 
					return [];
			}
			else 
				return this._getValueFromOpts()
		},
		
		_setDisabledAttr: function(){
			this.inherited(arguments);
			var value = this.disabled;
			array.forEach(this.options, function(option){
				if( option ){
					option.disabled = value;
				}
			});
			
			this._refreshState();
		},

		destroy: function(){
			this.inherited(arguments);
			for ( var index = 0; index < this.checkBoxListItems.length; index++){
				var item = this.checkBoxListItems[index];
				item.destroy();
			}
			this.checkBoxListItems.length = 0;
		},
		_errorIconWidth: 45,

		// Defect 14370 move describedby property to proper refered node
		_setHelpAttr: function(helpText){
			this.inherited(arguments);
			wai.removeWaiState(this.focusNode, "describedby");
			this._addWaiState(this.stateNode, "describedby", this.id + "_helpInner");
		}
	});
	
	
	if (has("dojo-bidi")) {
		baseClassName = baseClassName.substring(0, baseClassName.length-1);
		var baseCheckBoxList = iForm.CheckBoxList;
		iForm.CheckBoxList = declare(baseClassName,[baseCheckBoxList,bidiExtension]);
	}
	
	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/CheckBoxList", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		
		iForm.CheckBoxList = declare("idx.form.CheckBoxList",[iForm.CheckBoxList, TemplatePlugableMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			templatePath: require.toUrl("idx/form/templates/CheckBoxList.html"),  
		
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
	
	return iForm.CheckBoxList;
});