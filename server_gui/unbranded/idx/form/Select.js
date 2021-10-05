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
	"dojo/_base/array",
	"dojo/_base/event",
	"dojo/_base/window",
	"dojo/_base/lang",
	"dojo/_base/kernel",
	"dojo/dom-geometry",
	"dojo/dom-class",
	"dojo/dom-style",
	"dojo/dom-attr",
	"dojo/has",	
	"dojo/keys",
	"dojo/query",
	"dojo/when",
	"dijit/registry",
	"idx/widget/HoverHelpTooltip",
	"dijit/form/Select",
	"dijit/form/_FormSelectWidget",
	"dijit/form/_FormValueWidget",
	"dijit/_HasDropDown",
	"dijit/_WidgetsInTemplateMixin",
	"../util",
	"./_CompositeMixin",
	"./_CssStateMixin",
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
	"idx/has!#idx_form_Select-desktop?dojo/text!./templates/Select.html"  // desktop widget, load the template
		+ ":#idx_form_Select-mobile?"									// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/Select.html"					// global desktop platform, load template
		+ ":#mobile?"													// global mobile platform, don't load
		+ ":dojo/text!./templates/Select.html", 						// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_Select-mobile?./plugins/phone/SelectPlugin"			// mobile widget, load the plugin
		+ ":#idx_form_Select-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/SelectPlugin"							// global mobile platform, load plugin
		+ ":"																// no features, don't load plugin

	// ====================================================================================================================
], function(declare, array, event, win, lang, kernel, domGeometry, domClass, domStyle, domAttr, has, keys, query, when, registry,
			HoverHelpTooltip, Select, _FormSelectWidget, _FormValueWidget, _HasDropDown, _WidgetsInTemplateMixin, iUtil, _CompositeMixin, 
			_CssStateMixin,	TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin){
	
	
	var baseClassName = "idx.form.Select";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2

	/**
	 * @name idx.form.Select
	 * @class idx.form.Select is implemented according to IBM One UI(tm) <b><a href="http://dleadp.torolab.ibm.com/uxd/uxd_oneui.jsp?site=ibmoneui&top=x1&left=y28&vsub=*&hsub=*&openpanes=0000010000">Drop-Down Lists Standard</a></b>.
	 * It is a composite widget which enhanced dijit.form.Select with following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in label positioning</li>
	 * <li>Built-in required attribute</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit.form.Select
	 * @augments idx.form._CssStateMixin
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._ValidationMixin
	 */
	iForm.Select = declare(baseClassName, [Select, _CompositeMixin, _CssStateMixin],
	/**@lends idx.form.Select.prototype*/
	{
		// summary:
		//		One UI version Select control
		
		templateString: desktopTemplate,
		
		baseClass: "idxSelectWrap",
		
		oneuiBaseClass: "dijitSelect",
		
		// summary:
		//		Default maxHeight to limit to space available in viewport
		maxHeight: -1,
		
		/**
		 * Place holder of the drop down button.
		 * @type String
		 */
		placeHolder: "",
		
		cssStateNodes: {
			"titleNode": "dijitDownArrowButton"
		},
		/**
		 * Provide a method to move the attribute form domNode to oneuiBaseNode
		 * @param {Object} String : arrtributeName
		 */
		
		_exchangeAttribute: function(/*String*/ arrtributeName){
			if ( this.domNode.hasAttribute(arrtributeName) ){
				this.oneuiBaseNode.setAttribute( arrtributeName, this.domNode.getAttribute(arrtributeName) );
				this.domNode.removeAttribute(arrtributeName);
			}
		},
		/**
		 * Override the method in _HasDropDown.js to move the attribute
		 * # Defect 9173
		 */
		closeDropDown: function(){
			this.inherited(arguments);
			this._exchangeAttribute("aria-expanded");			
		},
		/**
		 * Override the method in _HasDropDown.js to move the attribute
		 * # Defect 9173
		 */
		openDropDown: function(){
			this.inherited(arguments);
			this._exchangeAttribute("aria-expanded");
			//Defect 14094 move the "aria-owns" and "popupactive" attributes
			this._exchangeAttribute("aria-owns");
			this._exchangeAttribute("popupactive");
		},
		
		postCreate: function(){
			var tempDomNode = this.domNode;
			this.domNode = this.oneuiBaseNode;
			this.inherited(arguments);
			// comment out for Defect 10859
			//domAttr.set(this.domNode, "aria-activedescendant", domAttr.get(this.domNode,"id") );
			this.domNode = tempDomNode;
			
			//this.domNode.setAttribute("role", "region");
			//this.domNode.setAttribute("aria-label",this.id+"_arialable");

			this._resize();
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
		_isEmpty: function(){
			return this.value === 0 || (/^\s*$/.test(this.value || ""));
		},
		getErrorMessage: function(){
			return (this.required && this._isEmpty()) ? this._missingMsg : this.invalidMessage;
		},
		validate: function(/*Boolean*/ isFocused){
			// summary:
			//		Called by oninit, onblur, and onkeypress, and whenever required/disabled state changes
			// description:
			//		Show missing or invalid messages if appropriate, and highlight textbox field.
			//		Used when a select is initially set to no value and the user is required to
			//		set the value.
			var isValid = this.disabled || this.isValid(isFocused);
			this._set("state", isValid ? "" : (this._hasBeenBlurred ? "Error" : "Incomplete"));
			this.focusNode.setAttribute("aria-invalid", isValid ? "false" : "true");
			var message = isValid ? "" : this.getErrorMessage();
			this._set("message", message);
			if (this._hasBeenBlurred) {
                this.displayMessage(message);
            }
			return isValid;
		},
		
		displayMessage: function(/*String*/ message){
			// summary:
			//		Overridable method to display validation errors/hints.
			//		By default uses a tooltip.
			// tags:
			//		extension
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
				if(this.focused){
					var node = domStyle.get(this.iconNode, "visibility") == "hidden" ? this.oneuiBaseNode : this.iconNode;
					this.messageTooltip.open(node);
				}
			}else{
				this.messageTooltip && this.messageTooltip.close();
			}
		},
		_setValueAttr: function(/*anything*/ newValue, /*Boolean?*/ priorityChange){
			// summary:
			//		Overwrite dijit.form._FormSelectWidget._setValueAttr to support "placeHolder"
			if(!this._onChangeActive){ priorityChange = null; }
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
				
				if(!lang.isObject(i) && !(typeof i === "number") ){
					i = i + "";
				}
				if(typeof i === "string" || typeof i === "number" ){
					newValue[idx] = array.filter(opts, function(node){
						return node.value === i;
					})[0] || {value: "", label: ""};
				}
			}, this);

			// Make sure some sane default is set
			newValue = array.filter(newValue, function(i){ return i && i.value; });
			if(!this.placeHolder && (!newValue[0] || !newValue[0].value) && opts.length){
				newValue[0] = opts[0];
			}
			array.forEach(opts, function(i){
				i.selected = array.some(newValue, function(v){ return v.value === i.value; });
			});
			var	val = array.map(newValue, function(i){ return i.value; }),
				disp = array.map(newValue, function(i){ return i.label; });

			//if(typeof val == "undefined" || typeof val[0] == "undefined"){ return; } // not fully initialized yet or a failed value lookup
			
			// Use placeHolder
			var strDisp = this.multiple ? disp : disp[0];
			if(!strDisp){
				strDisp = this.placeHolder;
			}
			this._setDisplay(strDisp);
			if (this.fieldWidth) {
				var selectContent = query("span.idxSelectWrapLabel", this.containerNode);
				if ( selectContent.length > 0)
					selectContent = selectContent[0];
				domClass.add(selectContent, "dojoxEllipsis");
			}
			var _previousValue = this.get("value");
			_FormValueWidget.prototype._setValueAttr.apply(this, [ this.multiple ? val : val[0], priorityChange ]);
			this._updateSelection();
			
			var _currentValue =  this.get("value");
			domAttr.set(this.valueNode, "value", this.get("value"));
			
			if(_previousValue != _currentValue){
				this.validate(this.focused);
			}
		},
		
		/**
		 * private function for supporting multichannel feature
		 */
		_createDropDown: function(){
			var self = this;
			return new Select._Menu({ 
				id: this.id + "_menu", 
				parentWidget: this,
				_onUpArrow: function( evt ){
					
					if ( evt.altKey && this.focusedChild && !this._getNext(this.focusedChild, -1) ){
						self.closeDropDown(true);
					}
					else{
						this.focusPrev();	
					}
					
				},
				_onDownArrow: function( evt ){
					if ( evt.altKey && this.focusedChild && !this._getNext(this.focusedChild, 1) ){
						self.closeDropDown(true);
					}
					else{
						this.focusNext();	
					}
				}
			});
		},
		/**
		 * 
		 */
		_fillContent: function(){
			// summary:
			//		Overwrite dijit.form._FormSelectWidget._fillContent to support empty value.
				
			if(!this.options){
				this.options =
					this.srcNodeRef
					? query("> *", this.srcNodeRef).map(
						function(node){
							if(node.getAttribute("type") === "separator"){
								return { value: "", label: "", selected: false, disabled: false };
							}
							return {
								value: (node.getAttribute("data-" + kernel._scopeName + "-value") || node.getAttribute("value")),
								label: String(node.innerHTML),
								// FIXME: disabled and selected are not valid on complex markup children (which is why we're
								// looking for data-dojo-value above.  perhaps we should data-dojo-props="" this whole thing?)
								// decide before 1.6
								selected: node.getAttribute("selected") || false,
								disabled: node.getAttribute("disabled") || false
							};
						},
						this)
					: [];
			}
			if(!this.value){
				this._set("value", this._getValueFromOpts());
			}else if(this.multiple && typeof this.value == "string"){
				this._set("value", this.value.split(","));
			}
			// Create the dropDown widget
			when(this._createDropDown(), lang.hitch(this, function(dropDown) {
				if (dropDown) {
					this.dropDown = dropDown;
					domClass.add(this.dropDown.domNode, this.baseClass + "Menu");
				}
			}));
			
		},
		
		_getValueFromOpts: function(){
			// summary:
			//		Overwrite dijit.form._FormSelectWidget._getValueFromOpts to support empty value.
			var opts = this.getOptions() || [];
			if(opts.length){
				// Mirror what a select does - choose the first one
				var opt = array.filter(opts, function(i){
					return i.selected;
				})[0];
				if(opt && opt.value){
					return opt.value;
				}else if(!this.placeHolder){
					opts[0].selected = true;
					return opts[0].value;
				}
			}
			return "";
		},
			
		_onFocus: function(){
			if (this._hasBeenBlurred && (!this._refocusing)){
				this.validate(true);
			}
			_FormSelectWidget.prototype._onFocus.apply(this, arguments);
		},
		
		_onBlur: function(){
			this.displayMessage("");
			_HasDropDown.prototype._onBlur.apply(this, arguments);
			this.validate(this.focused);
		},
		
		_onDropDownMouseUp: function(/*Event?*/ e){
			// summary:
			//		Overwrite dijit._HasDropDown._onDropDownMouseUp
			//		Focus the selected items once open the drop down menu.
				
			if(e && this._docHandler){
				this.disconnect(this._docHandler);
			}
			var dropDown = this.dropDown, overMenu = false;
	
			if(e && this._opened){
				// This code deals with the corner-case when the drop down covers the original widget,
				// because it's so large.  In that case mouse-up shouldn't select a value from the menu.
				// Find out if our target is somewhere in our dropdown widget,
				// but not over our _buttonNode (the clickable node)
				var c = domGeometry.position(this._buttonNode, true);
				if(!(e.pageX >= c.x && e.pageX <= c.x + c.w) ||
					!(e.pageY >= c.y && e.pageY <= c.y + c.h)){
					var t = e.target;
					while(t && !overMenu){
						if(domClass.contains(t, "dijitPopup")){
							overMenu = true;
						}else{
							t = t.parentNode;
						}
					}
					if(overMenu){
						t = e.target;
						if(dropDown.onItemClick){
							var menuItem;
							while(t && !(menuItem = registry.byNode(t))){
								t = t.parentNode;
							}
							if(menuItem && menuItem.onClick && menuItem.getParent){
								menuItem.getParent().onItemClick(menuItem, e);
							}
						}
						return;
					}
				}
			}
			if (this._opened) {
				if (dropDown.focus && dropDown.autoFocus !== false) {
					// Focus the dropdown widget - do it on a delay so that we
					// don't steal our own focus.
					this.focusSelectedItem();
				}
			}
			else {
				this.defer("focus");
			}
			
			if(has("touch")){
				this._justGotMouseUp = true;
				this.defer(function(){
					this._justGotMouseUp = false;
				
				});
			}
		},
		
		_onKeyUp: function(){
			// summary:
			//		Overwrite dijit._HasDropDown._onKeyUp
			//		Focus the selected items once open the drop down menu.
			
			if(this._toggleOnKeyUp){
				delete this._toggleOnKeyUp;
				this.toggleDropDown();
				var d = this.dropDown;	// drop down may not exist until toggleDropDown() call
				if(d && d.focus){
					setTimeout(lang.hitch(this, "focusSelectedItem"), 1);
				}
			}
		},
		
		_setReadOnlyAttr: function(value){
			this.inherited(arguments);
			if(this.dropDown){
				this.dropDown.set("readOnly", value);
			}
		},
		
//		_setFieldWidthAttr: function(/*String*/width){
//			domClass.toggle(this.oneuiBaseNode, this.oneuiBaseClass + "FixedWidth", !!width);
//			if(!width){ return; }
//			var widthInPx = iUtil.normalizedLength(width);
//			if(dojo.isFF){
//				var borderWidthInPx = iUtil.normalizedLength(domStyle.get(this.oneuiBaseNode,"border-left-width")) +
//				iUtil.normalizedLength(domStyle.get(this.oneuiBaseNode,"border-right-width"));
//				widthInPx += borderWidthInPx;
//			}else if(dojo.isIE){
//				widthInPx += 2;
//			}
//			domStyle.set(this.oneuiBaseNode, "width", widthInPx + "px");
//		},
		resize: function(){
			if(this._holdResize()){
				return;
			}
			if (iUtil.isPercentage(this._styleWidth)) {
				domStyle.set(this.containerNode, "width","");
			}
			this.inherited(arguments);
		},		
		_resize: function(){
			if (this._deferResize()) return;			
			domStyle.set(this.containerNode, "width","");
			this.inherited(arguments);
			
			if(this.oneuiBaseNode.style.width){
				var styleSettingWidth = this.oneuiBaseNode.style.width,
					contentBoxWidth = domGeometry.getContentBox(this.oneuiBaseNode).w;
				if ( styleSettingWidth.indexOf("px") || dojo.isNumber(styleSettingWidth) ){
					styleSettingWidth = parseInt(styleSettingWidth);
					contentBoxWidth = ( styleSettingWidth < contentBoxWidth || contentBoxWidth <= 0 )? styleSettingWidth : contentBoxWidth;
				}
					
				domStyle.set(this.containerNode, "width", contentBoxWidth - 40 + "px");
			}
		},
		focusSelectedItem: function(){
			// summary:
			//		Focus the item according to the value of the widget.
			
			var val = this.value,
				isFocused = false;
			if(!lang.isArray(val)){
				val = [val];
			}
			if(val && val[0]){
				isFocused = array.some(this._getChildren(), function(child){
					// child.option == undefined when this is an instance of MenuSeparator
					var isSelected = ( child.option && (val[0] === child.option.value) );
					if ( isSelected ){
						this.dropDown.focusChild(child);
					}
					return isSelected;
				}, this);

			}else{
				this.dropDown.focusFirstChild();
			}
			if(!isFocused){
				this.dropDown.focusFirstChild();
			}

		},
		
		_setPlaceHolderAttr: function(/*String*/ value){
			this.placeHolder = value;
			this.set("value", "");		
		}

	});
	
	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/Select", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		
		iForm.Select = declare("idx.form.Select",[iForm.Select, TemplatePlugableMixin, _WidgetsInTemplateMixin], {
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			templatePath: require.toUrl("idx/form/templates/Select.html"),  
		
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			
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
			_onDropDownMouseDown: function(){
				return this.doWithPlatformPlugin(arguments, "_onDropDownMouseDown", "_onDropDownMouseDown");
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
	
	
	return iForm.Select;
});
