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
	"dojo/_base/window",
	"dojo/_base/event",
	"dojo/_base/Deferred",
	"dojo/dom-class",
	"dojo/dom-style",
	"dojo/data/util/filter",
	"dojo/window",
	"dojo/keys",
	"dojo/has",
	"dojo/query", 
	"dijit/form/ComboBox",
	"dijit/_WidgetsInTemplateMixin",
	"idx/widget/HoverHelpTooltip",
	"./_CssStateMixin",
	"./_ComboBoxMenu",
	"./_CompositeMixin",
	"./_AutoCompleteA11yMixin",
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
	"idx/has!#idx_form_ComboBox-desktop?dojo/text!./templates/ComboBox.html" 	// desktop widget, load the template
		+ ":#idx_form_ComboBox-mobile?"											// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/ComboBox.html"						// global desktop platform, load template
		+ ":#mobile?"															// global mobile platform, don't load
		+ ":dojo/text!./templates/ComboBox.html", 								// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_ComboBox-mobile?./plugins/phone/ComboBoxPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_ComboBox-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/ComboBoxPlugin"							// global mobile platform, load plugin
		+ ":"																// no features, don't load plugin

	// ====================================================================================================================
], function(declare, lang, win, event, Deferred, domClass, domStyle, filter, winUtils, keys, has, query, ComboBox, 
			_WidgetsInTemplateMixin, HoverHelpTooltip, _CssStateMixin, _ComboBoxMenu,  _CompositeMixin, _AutoCompleteA11yMixin, 
			TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin){
	
	var baseClassName = "idx.form.ComboBox";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2
	
	/**
	 * @name idx.form.ComboBox
	 * @class idx.form.ComboBox is implemented according to IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y27&vsub=*&hsub=*&openpanes=0000010000">Combo Boxes Standard</a></b>.
	 * It is a composite widget which enhanced dijit.form.ComboBox with following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in label positioning</li>
	 * <li>Built-in hint</li>
	 * <li>Built-in hint positioning</li>
	 * <li>Built-in required attribute</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit.form.ComboBox
	 * @augments idx.form._CssStateMixin
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._ValidationMixin
	 */
	iForm.ComboBox = declare(baseClassName, [ComboBox, _CssStateMixin, _CompositeMixin],
	/**@lends idx.form.ComboBox.prototype*/
	{
		// summary:
		//		One UI version ComboBox
		
		instantValidate: false,
		
		baseClass: "idxComboBoxWrap",
		
		oneuiBaseClass: "dijitTextBox dijitComboBox",
		
		templateString: desktopTemplate,
		
		dropDownClass: _ComboBoxMenu,
		
		cssStateNodes: {
			"_buttonNode": "dijitDownArrowButton"
		},
		
		/**
		 * 
		 */
		postCreate: function(){
			this.dropDownClass = _ComboBoxMenu;
			this.inherited(arguments);
			//Defect 11821, remove the aria-labelledby attribute from the dom node
			this.domNode.removeAttribute("aria-labelledby");
			if(this.instantValidate){
				this.connect(this, "_onFocus", function(){
					if (this._hasBeenBlurred && (!this._refocusing)) {
						this.validate(true);
					}
				});
				this.connect(this, "_onInput", function(){
					this.validate(true);
				});
			}else{
				this.connect(this, "_onFocus", function(){
					if (this.message && this._hasBeenBlurred && (!this._refocusing)) {
						this.displayMessage(this.message);
					}
				})
			}
			this._resize();
		},
		/**
		 * Defect 11885
		 * overwrite the method in _CompositeMixin
		 * @param {Object} v
		 */
		_setPlaceHolderAttr: function(){
			this._placeholder = false;
			this.inherited(arguments);
		},

		
		/**
		 * Provides a method to return focus to the widget without triggering
		 * revalidation.  This is typically called when the validation tooltip
		 * is closed.
		 */
		refocus: function() {
			this._refocusing = true;
			this.focus();
			this._refocusing = false;
		},
		
		/**
		 * Override the method in _HasDropDown.js to move the attribute
		 * # Defect 9469
		 */
		closeDropDown: function(){
			this.inherited(arguments);
			this.domNode.removeAttribute("aria-expanded");			
		},
		
		_openResultList: function(/*Object*/ results, /*Object*/ query, /*Object*/ options){
			// summary:
			//		Overwrite dijit.form._AutoCompleterMixin._openResultList to focus the selected
			//		item when open the menu.
			
			// ie 11 oninput defect for combobox/filteringselect
			// open dropdown when startup
			this._fetchHandle = null;
			if(	this.disabled ||
				this.readOnly ||
				(query[this.searchAttr] !== this._lastQuery)	// TODO: better way to avoid getting unwanted notify
			){
				return;
			}
			var wasSelected = this.dropDown.getHighlightedOption();
			this.dropDown.clearResultList();
			if(!results.length && options.start == 0){ // if no results and not just the previous choices button
				this.closeDropDown();
				return;
			}
	
			this._nextSearch = this.dropDown.onPage = lang.hitch(this, function(direction){
				results.nextPage(direction !== -1);
				this.focus();
			});
			
			// Fill in the textbox with the first item from the drop down list,
			// and highlight the characters that were auto-completed. For
			// example, if user typed "CA" and the drop down list appeared, the
			// textbox would be changed to "California" and "ifornia" would be
			// highlighted.
			
			// This method does not return any value in 1.8.
			this.dropDown.createOptions(
				results,
				options,
				lang.hitch(this, "_getMenuLabelFromItem")
			);
			
			// Use following code to get child nodes.
			var nodes = this.dropDown.containerNode.childNodes;
	
			// show our list (only if we have content, else nothing)
			this._showResultList();
			this.domNode.removeAttribute("aria-expanded");
			// Focus the selected item
			if( !this._lastInput && this.focusNode.value ){

				for(var i = 0; i < nodes.length; i++){
					var item = this.dropDown.items[nodes[i].getAttribute("item")];
					if(item){
						var queryOption = {};
						queryOption[this.searchAttr] = item[this.searchAttr];
						var value = this.store.query(queryOption);
						if ( value && value.length > 0)
							value = value[0][this.searchAttr];
						
						if(value == this.displayedValue){
							this.dropDown._setSelectedAttr(nodes[i]);
							winUtils.scrollIntoView(this.dropDown.selected);
							break;
						}
					}
				}
			}
			
			// #4091:
			//		tell the screen reader that the paging callback finished by
			//		shouting the next choice
			if(options.direction){
				if(1 == options.direction){
					this.dropDown.highlightFirstOption();
				}else if(-1 == options.direction){
					this.dropDown.highlightLastOption();
				}
				if(wasSelected){
					this._announceOption(this.dropDown.getHighlightedOption());
				}
			}else if(this.autoComplete && !this._prev_key_backspace
				// when the user clicks the arrow button to show the full list,
				// startSearch looks for "*".
				// it does not make sense to autocomplete
				// if they are just previewing the options available.
				&& !/^[*]+$/.test(query[this.searchAttr].toString())){
					this._announceOption(nodes[1]); // 1st real item
			}
		},
		
		_onInputContainerEnter: function(){
			domClass.toggle(this.oneuiBaseNode, "dijitComboBoxInputContainerHover", true);
		},
		
		_onInputContainerLeave: function(){
			domClass.toggle(this.oneuiBaseNode, "dijitComboBoxInputContainerHover", false);
		},
		
		_setReadOnlyAttr: function(value){
			// summary:
			//		Sets read only (or unsets) all the children as well
			this.inherited(arguments);
			if(this.dropDown){
				this.dropDown.set("readOnly", value);
			}
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
		},
		_setBlurValue: function(){
			// if the user clicks away from the textbox OR tabs away, set the
			// value to the textbox value
			// #4617:
			//		if value is now more choices or previous choices, revert
			//		the value
			var newvalue = this.get('displayedValue');
			var pw = this.dropDown;
			if(pw && (
				newvalue == pw._messages["previousMessage"] ||
				newvalue == pw._messages["nextMessage"]
				)
			){
				this._setValueAttr(this._lastValueReported, true);
			}else if(typeof this.item == "undefined"){
				// Update 'value' (ex: KY) according to currently displayed text
				this.item = null;
				this.set('displayedValue', newvalue);
			}else{
				if(this.value != this._lastValueReported){
					this._handleOnChange(this.value, true);
				}
				this._setValueAttr(newvalue, true);
			}
		}
	});
	
	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/ComboBox", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		var localDropDownClass = _ComboBoxMenu;
		if (MobilePlugin && MobilePlugin.prototype && MobilePlugin.prototype.dropDownClass){
			localDropDownClass = MobilePlugin.prototype.dropDownClass;
		}
		iForm.ComboBox = declare("idx.form.ComboBox",[iForm.ComboBox, TemplatePlugableMixin, _WidgetsInTemplateMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			templatePath: require.toUrl("idx/form/templates/ComboBox.html"),  
		
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			/**
			 * 
			 */
			inProcessInput: false,
			/**
			 * 
			 */
			postCreate: function(){
				// set the dropDownClass into property of an instance
				this.dropDownClass = localDropDownClass;
				return this.doWithPlatformPlugin(arguments, "postCreate", "postCreate");
			},
			/**
			 * Stub Plugable method
			 */
			onCloseButtonClick: function(){
				return this.doWithPlatformPlugin(arguments, "onCloseButtonClick", "onCloseButtonClick");
			},
			/**
			 * Stub Plugable method
			 */
			onInput: function(){
				return this.doWithPlatformPlugin(arguments, "onInput", "onInput");
			},
			/**
			 * 
			 * @param {Object} item
			 * @param {Object} checked
			 */
			onCheckStateChanged: function(item, checked){
				return this.doWithPlatformPlugin(arguments, "onCheckStateChanged", "onCheckStateChanged",item, checked);
			},
			/**
			 * Stub Plugable method
			 */
			loadDropDown: function(){
				return this.doWithPlatformPlugin(arguments, "loadDropDown", "loadDropDown");
			},
			/**
			 * Stub Plugable method
			 */
			openDropDown: function(){
				return this.doWithPlatformPlugin(arguments, "openDropDown", "openDropDown");
			},
			/**
			 * Stub Plugable method
			 */
			_createDropDown : function(){
				return this.doWithPlatformPlugin(arguments, "_createDropDown", "_createDropDown");
			},
			/**
			 * Stub Plugable method
			 */
			closeDropDown:function(){
				return this.doWithPlatformPlugin(arguments, "closeDropDown", "closeDropDown");
			},
			/**
			 * Stub Plugable method
			 */
			displayMessage: function(message){
				return this.doWithPlatformPlugin(arguments, "displayMessage", "displayMessage",message);
			},
			/**
			 * Stub Plugable method
			 * @param {Object} helpText
			 */
			_setHelpAttr: function(helpText){
				return this.doWithPlatformPlugin(arguments, "_setHelpAttr", "_setHelpAttr",helpText);
			}
		});
	}
	
	return iForm.ComboBox;
});
