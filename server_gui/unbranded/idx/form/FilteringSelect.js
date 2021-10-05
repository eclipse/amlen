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
	"dojo/dom-class",
	"dojo/dom-style",
	"dojo/window",
	"dojo/has",
	"dijit/_WidgetsInTemplateMixin",
	"dijit/form/FilteringSelect",
	"idx/widget/HoverHelpTooltip",
	"./_CompositeMixin",
	"./_CssStateMixin",
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
	"idx/has!#idx_form_FilteringSelect-desktop?dojo/text!./templates/ComboBox.html" 	// desktop widget, load the template
		+ ":#idx_form_FilteringSelect-mobile?"											// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/ComboBox.html"								// global desktop platform, load template
		+ ":#mobile?"																	// global mobile platform, don't load
		+ ":dojo/text!./templates/ComboBox.html", 										// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_FilteringSelect-mobile?./plugins/phone/FilteringSelectPlugin"		// mobile widget, load the plugin
		+ ":#idx_form_FilteringSelect-desktop?"												// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/FilteringSelectPlugin"									// global mobile platform, load plugin
		+ ":"																				// no features, don't load plugin

], function(declare, lang, domClass, domStyle, winUtils, has, _WidgetsInTemplateMixin, FilteringSelect, 
	HoverHelpTooltip, _CompositeMixin, _CssStateMixin, _AutoCompleteA11yMixin, 
	TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin) {
	
	var baseClassName = "idx.form.FilteringSelect";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2

	/**
	 * @name idx.form.FilteringSelect
	 * @class idx.form.FilteringSelect is implemented according to IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y27&vsub=*&hsub=*&openpanes=0000010000">Combo Boxes Standard</a></b>.
	 * It is a composite widget which enhanced dijit.form.FilteringSelect with following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in label positioning</li>
	 * <li>Built-in hint</li>
	 * <li>Built-in hint positioning</li>
	 * <li>Built-in required attribute</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit.form.FilteringSelect
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._CssStateMixin
	 */
	iForm.FilteringSelect = declare(baseClassName, [FilteringSelect, _CompositeMixin, _CssStateMixin,_AutoCompleteA11yMixin],
	/**@lends idx.form.FilteringSelect.prototype*/
	{
		
		baseClass: "idxFilteringSelectWrap",
		
		oneuiBaseClass: "dijitTextBox dijitComboBox",
		
		templateString: desktopTemplate,
		
		selectOnClick: true,
		
		cssStateNodes: {
			"_buttonNode": "dijitDownArrowButton"
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
		
		postCreate: function() {
			this.dropDownClass = FilteringSelect.prototype.dropDownClass;
			this.inherited(arguments);
			//Defect 11821, remove the aria-labelledby attribute from the dom node
			this.domNode.removeAttribute("aria-labelledby");
			this.connect(this, "_onFocus", function(){
				if (this.message && this._hasBeenBlurred && (!this._refocusing)) {
					this.displayMessage(this.message);
				}
			});
			this._resize();
			//
			//A11y defect to remove the aria-labelledby when the label is empty
			//

			var islabelEmpty = /^\s*$/.test(this.label);
			if ( islabelEmpty ){
				this.oneuiBaseNode.removeAttribute("aria-labelledby");
				this.compLabelNode.innerHTML = "hidden";
			}
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

		//fix for defect 14571
		_onInput: function(/*Event*/ evt){
			//If texts are cleared by clicking a button or clear icon in IE10+, set charcode to 229
			if (!this._lastInputProducingEvent && !this.textbox.value) {
				var charOrCode = 229;
			    // create fake event to set charOrCode and to know if preventDefault() was called
			    var faux = { faux: true },
			        attr;
			    for (attr in evt) {
			        if (!/^(layer[XY]|returnValue|keyLocation)$/.test(attr)) { // prevent WebKit warnings
			            var v = evt[attr];
			            if (typeof v != "function" && typeof v != "undefined") {
			                faux[attr] = v;
			            }
			        }
			    }
			    lang.mixin(faux, {
			        charOrCode: charOrCode,
			        _wasConsumed: false,
			        preventDefault: function() {
			            faux._wasConsumed = true;
			            evt.preventDefault();
			        },
			        stopPropagation: function() {
			            evt.stopPropagation();
			        }
			    });

			    this._lastInputProducingEvent = faux;
			}

			this.inherited(arguments);
		},
		
			
		_isEmpty: function(){
			return (/^\s*$/.test(this.textbox.value || ""));
		},

		_onBlur: function(){
			this.inherited(arguments);
			this.validate(this.focused);
		},

		_openResultList: function(/*Object*/ results, /*Object*/ query, /*Object*/ options){
			//	Overwrite dijit.form.FilteringSelect._openResultList to focus the selected
			//	item when open the menu.
			this.inherited(arguments);

			// Use following code to get child nodes.
			var nodes = this.dropDown.containerNode.childNodes;
			
			// Focus the selected item
			if(!this._lastInput && this.focusNode.value && this.dropDown.items){
				for(var i = 0; i < nodes.length; i++){
					var item = this.dropDown.items[nodes[i].getAttribute("item")];
					if(item){
						var value = this.store._oldAPI ?	// remove getValue() for 2.0 (old dojo.data API)
								this.store.getValue(item, this.searchAttr) : item[this.searchAttr];
						value = value.toString();
						if(value == this.displayedValue){
							this.dropDown._setSelectedAttr(nodes[i]);
							winUtils.scrollIntoView(this.dropDown.selected);
							break;
						}
					}
				}
			}
			
			
			if(this.item === undefined){ // item == undefined for keyboard search
				// If the search returned no items that means that the user typed
				// in something invalid (and they can't make it valid by typing more characters),
				// so flag the FilteringSelect as being in an invalid state
				this.validate(true);
			}
		},
		
		_onInputContainerEnter: function(){
			domClass.toggle(this.oneuiBaseNode, "dijitComboBoxInputContainerHover", true);
		},
		
		_onInputContainerLeave: function(){
			domClass.toggle(this.oneuiBaseNode, "dijitComboBoxInputContainerHover", false);
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
		}
	});

	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/FilteringSelect", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		var localDropDownClass = iForm.FilteringSelect.prototype.dropDownClass;
		if (MobilePlugin && MobilePlugin.prototype && MobilePlugin.prototype.dropDownClass){
			localDropDownClass = MobilePlugin.prototype.dropDownClass;
		}
		iForm.FilteringSelect = declare("idx.form.FilteringSelect",[iForm.FilteringSelect, TemplatePlugableMixin, _WidgetsInTemplateMixin], {
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
			 * Dot Not Call inherited function in the plugin stub method
			 */
			postCreate: function(){
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
			 * 
			 */
			_onBlur: function(){
				return this.doWithPlatformPlugin(arguments, "_onBlur", "_onBlur");
			},
			/**
			 * Stub Plugable method
			 */
			openDropDown: function(){
				return this.doWithPlatformPlugin(arguments, "openDropDown", "openDropDown");
			},
			closeDropDown:function(){
				return this.doWithPlatformPlugin(arguments, "closeDropDown", "closeDropDown");
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
	
	return iForm.FilteringSelect;

});
