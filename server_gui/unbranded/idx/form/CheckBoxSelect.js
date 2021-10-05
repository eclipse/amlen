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
	"dojo/_base/kernel",
	"dojo/_base/lang",
	"dojo/_base/array",
	"dojo/_base/event",
	"dojo/_base/window",
	"dojo/when",
	"dojo/query",
	"dojo/keys",
	"dojo/touch",
	"dojo/on",
	"dojo/dom-construct",
	"dojo/dom-attr",
	"dojo/dom-class",
	"dojo/dom-style",
	"dojo/dom-geometry",
	"dojo/html",
	"dojo/i18n",
	"dijit/popup",
	"dijit/form/_FormSelectWidget",
	"dijit/form/_FormValueWidget",
	"dijit/_HasDropDown",
	"dijit/MenuSeparator",
	"dijit/_WidgetsInTemplateMixin",
	"../util",
	"./_CssStateMixin",
	"./_CheckBoxSelectMenu",
	"./_CheckBoxSelectMenuItem",
	"./_CompositeMixin",
	"./_ValidationMixin",	
	"./_FormSelectWidgetA11yMixin",
	"dojo/has!dojo-bidi?../bidi/form/CheckBoxSelect",

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
	"idx/has!#idx_form_CheckBoxSelect-desktop?dojo/text!./templates/CheckBoxSelect.html"  // desktop widget, load the template
		+ ":#idx_form_CheckBoxSelect-mobile?"									// mobile widget, don't load desktop template
		+ ":#desktop?dojo/text!./templates/CheckBoxSelect.html"					// global desktop platform, load template
		+ ":#mobile?"															// global mobile platform, don't load
		+ ":dojo/text!./templates/CheckBoxSelect.html", 						// no dojo/has features, load the template
			
	// ------
	// Load the mobile plugin according to build-time/runtime dojo/has features
	// ------
	"idx/has!#idx_form_CheckBoxSelect-mobile?./plugins/phone/CheckBoxSelectPlugin"	// mobile widget, load the plugin
		+ ":#idx_form_CheckBoxSelect-desktop?"										// desktop widget, don't load plugin
		+ ":#mobile?./plugins/phone/CheckBoxSelectPlugin"							// global mobile platform, load plugin
		+ ":",																		// no features, don't load plugin

	// ====================================================================================================================
	"dojo/i18n!./nls/CheckBoxSelect",
	"dojox/html/entities",
	"dojox/html/ellipsis"
], function(declare, has, kernel, lang, array, event, win, when, query, keys, touch, on, domConstruct, domAttr, domClass, domStyle, domGeom, html, 
			i18n, popup, _FormSelectWidget, _FormValueWidget, _HasDropDown, MenuSeparator,_WidgetsInTemplateMixin, iUtil, _CssStateMixin, _CheckBoxSelectMenu,
			_CheckBoxSelectMenuItem, _CompositeMixin, _ValidationMixin,_FormSelectWidgetA11yMixin, 
			bidiExtension, 
			TemplatePlugableMixin, PlatformPluginRegistry, desktopTemplate, MobilePlugin,
			checkBoxSelectNls, entities){
			
	var baseClassName = "idx.form.CheckBoxSelect";
	if (has("mobile") || has("platform-plugable")) {
		baseClassName = baseClassName + "Base";
	}
	if (has("dojo-bidi")) {
		baseClassName = baseClassName + "_";
	}
	
	var iForm = lang.getObject("idx.oneui.form", true); // for backward compatibility with IDX 1.2

	/**
	 * @name idx.form.CheckBoxSelect
	 * @class idx.form.CheckBoxSelect is a composite widget which looks like a drop down version multi select control.
	 * CheckBoxSelect can be created in the same way of creating a dijit.form.Select control.
	 * The only difference is that more than one options can be marked as selected. 
	 * As a composite widget, it also provides following features:
	 * <ul>
	 * <li>Built-in label</li>
	 * <li>Built-in label positioning</li>
	 * <li>Built-in required attribute</li>
	 * <li>One UI theme support</li>
	 * </ul>
	 * @augments dijit._HasDropDown
	 * @augments idx.form._CssStateMixin
	 * @augments idx.form._CompositeMixin
	 * @augments idx.form._ValidationMixin
	 * @example Programmatic:
	 *	new idx.form.CheckBoxSelect({
	 *	options: [
	 *			{ label: 'foo', value: 'foo', selected: true },
	 *			{ label: 'bar', value: 'bar' },
	 *		]
	 *	});
	 *	
	 *Declarative:
	 *	&lt;select jsId="cbs" data-dojo-type="oneui.form.CheckBoxSelect" data-dojo-props='
	 *		name="cbs", label="CheckBoxSelect:", value="foo"'>
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
	 *	var cbs = new idx.form.CheckBoxSelect({
	 *		store: readStore
	 *	});
	 */
	iForm.CheckBoxSelect = declare(baseClassName, [_FormSelectWidget, _HasDropDown, _CssStateMixin, _CompositeMixin, _ValidationMixin,_FormSelectWidgetA11yMixin],
	/**@lends idx.form.CheckBoxSelect.prototype*/
	{
		// summary:
		//		A multi select control with check boxes.
		
		baseClass: "idxCheckBoxSelectWrap",
		
		oneuiBaseClass: "idxCheckBoxSelect dijitSelect",
		
		multiple: true,
		
		maxHeight: -1,//To show the dropdown according the window height
		
		instantValidate: true,
		
		/**
		 * Separator for the select button label.
		 * @type String
		 * @default ", "
		 */
		labelSeparator: ", ",
		
		cssStateNodes: {
			"titleNode": "dijitDownArrowButton"
		},
		
		templateString: desktopTemplate,
		
		// attributeMap: Object
		//		Add in our style to be applied to the focus node
		attributeMap: lang.mixin(lang.clone(_FormSelectWidget.prototype.attributeMap),{style:"tableNode"}),
	
		// required: Boolean
		//		Can be true or false, default is false.
		required: false,
	
		// state: String
		//		Shows current state (ie, validation result) of input (Normal, Warning, or Error)
		state: "",
	
		// message: String
		//		Currently displayed error/prompt message
		message: "",
	
		//	tooltipPosition: String[]
		//		See description of dijit.Tooltip.defaultPosition for details on this parameter.
		tooltipPosition: [], // need to override this class-static value in postMixInProperties
	
		// emptyLabel: string
		//		What to display in an "empty" dropdown
		emptyLabel: "&nbsp;",
	
		// _isLoaded: Boolean
		//		Whether or not we have been loaded
		_isLoaded: false,
	
		// _childrenLoaded: Boolean
		//		Whether or not our children have been loaded
		_childrenLoaded: false,
		
		postMixInProperties: function(){
			this.tooltipPosition = []; // set this to instance-local value
			this._nlsResources = checkBoxSelectNls;
			this.missingMessage || (this.missingMessage = this._nlsResources.missingMessage);
			this.inherited(arguments);
		},
		
		_createCheckBoxSelectMenu: function(){
			return new _CheckBoxSelectMenu({
				id: this.id + "_menu"});
		},
		
		_fillContent: function(){
			// summary:
			//		Overwrite dijit.form._FormSelectWidget._fillContent to fix a typo
			var opts = this.options;
			if(!opts){
				opts = this.options = this.srcNodeRef ? query("> *",
					this.srcNodeRef).map(function(node){
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
					}, this) : [];
			}
			if(!this.value){
				this._set("value", this._getValueFromOpts());
			}else if(this.multiple && typeof this.value == "string"){
				this._set("value", this.value.split(","));
			}
			
			// Create the dropDown widget
			when(this._createCheckBoxSelectMenu(), lang.hitch(this, function(dropDown) {
				if (dropDown)
					this.dropDown = dropDown;
			}));
		},
		
		_getValueFromOpts: function(){
			// summary:
			//		Returns the value of the widget by reading the options for
			//		the selected flag
			var opts = this.getOptions() || [];
			// Set value to be the sum of all selected
			return array.map(array.filter(opts, function(i){
				return i.selected && i.selected !== "false";
			}), function(i){
				return i.value;
			}) || [];
		},
		
		postCreate: function(){
			// summary:
			//		stop mousemove from selecting text on IE to be consistent with other browsers
			
			this._event = {
				"input" : "onChange",
				"blur" 	: "_onBlur",
				"focus" : "_onFocus"
			};
			
			this.inherited(arguments);

			//when initialized, options inside checkbox select should be selected 
			//if related items in the store set selected
			this._setValueFromStore();

			this.connect(this.domNode, "onmousemove", event.stop);
			this._resize();
		},

		startup: function(){
			// summary:
			// 		functions about sizing when widget completly created
			//		sizing calculations should not called in postCreate 
			
			this.inherited(arguments);
			this._resize();	
		},
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
					contentBoxWidth = domGeom.getContentBox(this.oneuiBaseNode).w;
				if ( styleSettingWidth.indexOf("px") || dojo.isNumber(styleSettingWidth) ){
					styleSettingWidth = parseInt(styleSettingWidth);
					contentBoxWidth = ( styleSettingWidth < contentBoxWidth || contentBoxWidth <= 0 )? styleSettingWidth : contentBoxWidth;
				}
					
				domStyle.set(this.containerNode, "width", contentBoxWidth - 40 + "px");
			}
			
		},
		
		setStore: function(store, selectedValue, fetchArgs){
			// summary:
			//		If there is any items selected in the store, the value
			//		of the widget will be set to the values of these items.
			this.inherited(arguments);
			/**
		 	* Fix defect 13101, deals with items with old or new store api
		 	*/
			/*
			var setSelectedItems = function(items){
				var value = array.map(items, function(item){ return item.value[0]; });
				if(value.length){
					this.set("value", value);
				}
			};*/
			//this.store.fetch({query:{selected: true}, onComplete: setSelectedItems, scope: this});
			this._setValueFromStore();
		},

		_setValueFromStore: function(){
			if(this.store){
				when(this.store.query({selected: true}), lang.hitch(this, function(items) {
					var value;
					if(this.store._oldAPI)
						value = array.map(items, function(item){ return item.value[0]; });
					else
						value = array.map(items, function(item){ return item.value; });
					if(value.length){
						this.set("value", value);
					}
				}));
			}
		},
		
		_getMenuItemForOption: function(/*dijit.form.__SelectOption*/ option){
			// summary:
			//		For the given option, return the menu item that should be
			//		used to display it.  This can be overridden as needed
			if(!option.value && !option.label){
				// We are a separator (no label set for it)
				return new MenuSeparator();
			}else{
				// Just a regular menu option
				var item = new _CheckBoxSelectMenuItem({
					parent: this,
					option: option,
					label: option.label || this.emptyLabel,
					onClick: this._onMenuItemClick,
					checked: option.selected || false,
					readOnly: this.readOnly || false,
					disabled: this.disabled || false
				});
				//For Edge we need to disable text selection from shift+click
				item.domNode.onselectstart = function() {
    				return false;
  				}
				return item;
			}
		},

		_onMenuItemClick: function(e){
			this.parent._doSelect(this, e.shiftKey);
			this.parent._updateValue();
		},
		
		_doSelect: function(node, range) {
			// summary:
			//		Add or remove the given node from selection, responding
			//		to a user action such as a click or keypress.
			// range: Boolean
			//		Indicates whether this is meant to be a ranged action (e.g. shift-click)

			if (this.anchor && range) {
				var distance = this.anchor.getIndexInParent() - node.getIndexInParent(),
					begin, end, anchor = this.anchor;

				if (distance < 0) { //current is after anchor
					begin = anchor;
					end = node;
				} else { //current is before anchor
					begin = node;
					end = anchor;
				}
				var isChecked = node.get('checked');
				//check/uncheck everything betweeen begin and end inclusively
				while (begin != end) {
					if (begin.option) { //exclude MenuSeparator
						begin.set('checked', isChecked);
						begin.option.selected = isChecked;
					}
					begin = begin.getNextSibling();
				}
				end.set('checked', isChecked);
				end.option.selected = isChecked;
			}
			this.anchor = node;
		},

		_addOptionItem: function(/*dijit.form.__SelectOption*/ option){
			// summary:
			//		For the given option, add an option to our dropdown.
			//		If the option doesn't have a value, then a separator is added
			//		in that place.
			if(this.dropDown){
				this.dropDown.addChild(this._getMenuItemForOption(option));
			}
		},
		
		_getChildren: function(){
			if(!this.dropDown){
				return [];
			}
			return this.dropDown.getChildren();
		},
		
		_loadChildren: function(/*Boolean*/ loadMenuItems){
			// summary:
			//		Resets the menu and the length attribute of the button - and
			//		ensures that the label is appropriately set.
			//	loadMenuItems: Boolean
			//		actually loads the child menu items - we only do this when we are
			//		populating for showing the dropdown.
			
			if(loadMenuItems === true){
				// this.inherited destroys this.dropDown's child widgets (MenuItems).
				// Avoid this.dropDown (Menu widget) having a pointer to a destroyed widget (which will cause
				// issues later in _setSelected). (see #10296)
				if(this.dropDown){
					delete this.dropDown.focusedChild;
				}
				if(this.options.length){
					this.inherited(arguments);
				}else{
					// Drop down menu is blank but add one blank entry just so something appears on the screen
					// to let users know that they are no choices (mimicing native select behavior)
					array.forEach(this._getChildren(), function(child){ child.destroyRecursive(); });
				}
			}else{
				this._updateSelection();
			}
			
			this._isLoaded = false;
			this._childrenLoaded = true;
			
			if(!this._loadingStore){
				// Don't call this if we are loading - since we will handle it later
				this._setValueAttr(this.value);
			}
		},
		
		_updateValue: function(){
			this.set("value", this._getValueFromOpts());
		},
		
		/**
		 * Event triggered when the widget value changes.
		 */
		onChange: function(newValue){
			// summary:
			//		Hook function
		},
		
		/**
		 * Reset the value of the widget.
		 */
		reset: function(){
			// summary:
			//		Overridden so that the state will be cleared.
			this.inherited(arguments);
			this._set("state", "");
			this._set("message", "");
		},
		
		_setDisplay: function(/*String*/ newDisplay){
			// summary:
			//		sets the display for the given value (or values)
			var length = 0;
			var label;
			if(lang.isArray(newDisplay)){
				length = newDisplay.length;
			}else{
				length = newDisplay ? 1 : 0;
			}
			label = length ? newDisplay.join(this.labelSeparator) : (this.placeHolder || "");
			
			// NOTE(bcaceres): I rewrote this section to avoid using "innerHTML" so that we
			// would not need to escape elements of the label to avoid issues with attributes
			// in the span tag or in the child text
			//
			// support special character encode for defect 11423
			//
			var spanAttrs = {"role": "option"}, labelStr = entities.decode(label);
			if ( !labelStr )
				spanAttrs["title"] = "empty";// Defect 11904
			else
				spanAttrs["title"] = labelStr;

			
			var span = domConstruct.create("span", spanAttrs, this.containerNode, "only");
			domClass.add(span, "dijitReset");
			domClass.add(span, "dijitInline");
			if (this.fieldWidth) domClass.add(span, "dojoxEllipsis");
			domClass.add(span, this.baseClass + "Label");
			//
			// support special character encode for defect 11423
			// The Old code is as follow
			// var labelTextNode = win.doc.createTextNode(label);
			//
			html.set(span, label);
			span.title = has("ff") ? span.textContent : span.innerText; //title: show actual texts instead of html tags
			
			// Defect 11904
			if(span.title === ""){
				span.title = 'empty';
			}
		},
		
		_setValueAttr: function(/*anything*/ newValue, /*Boolean?*/ priorityChange){
			// summary:
			//		set the value of the widget.
			//		If a string is passed, then we set our value from looking it up.
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
				newValue[0] = opts[0];
			}
			array.forEach(opts, function(i){
				i.selected = array.some(newValue, function(v){ return v.value === i.value; });
			});
			var	val = array.map(newValue, function(i){ return i.value; }),
				disp = array.map(newValue, function(i){ return i.label; });
			
			this._setDisplay(this.multiple ? disp : disp[0]);
			if(typeof val == "undefined"){ return; } // not fully initialized yet or a failed value lookup
			_FormValueWidget.prototype._setValueAttr.apply(this, arguments);
			this._updateSelection();
		},
		
		_setPlaceHolderAttr: null,
		
		isLoaded: function(){
			return this._isLoaded;
		},
		
		loadDropDown: function(/*Function*/ loadCallback){
			// summary:
			//		populates the menu
			this._loadChildren(true);
			this._isLoaded = true;
			loadCallback();
		},
		
		/**
		 * Close the drop down menu.
		 */
		closeDropDown: function(){
			// overriding _HasDropDown.closeDropDown()
			this.inherited(arguments);
			
			if(this.dropDown && this.dropDown.menuTableNode){
				// Erase possible width: 100% setting from _SelectMenu.resize().
				// Leaving it would interfere with the next openDropDown() call, which
				// queries the natural size of the drop down.
				this.dropDown.menuTableNode.style.width = "";
			}
		},
		
		/**
		 * Invert the selection. Using the parameter onChange to indicate whether
		 * onChange event should be fired.
		 * @param {Boolean} onChange
		 */
		invertSelection: function(onChange){
			// summary: Invert the selection
			// onChange: Boolean
			//		If null, onChange is not fired.
			array.forEach(this.options, function(i){
				i.selected = !i.selected;
			});
			this._updateSelection();
			this._updateValue();
		},
		
		_updateSelection: function(){
			this.inherited(arguments);
			this._handleOnChange(this.value);
			array.forEach(this._getChildren(), function(item){
				// dijit.MenuSeparator does not have _updateBox
				if ( item._updateBox )
					item._updateBox();

			});
		},
		
		_setDisabledAttr: function(value){
			// summary:
			//		Disable (or enable) all the children as well
			this.inherited(arguments);
			array.forEach(this._getChildren(), function(node){
				if(node && node.set){
					node.set("disabled", value);
				}
			});
		},
		
		_setReadOnlyAttr: function(value){
			// summary:
			//		Sets read only (or unsets) all the children as well
			this.inherited(arguments);
			array.forEach(this._getChildren(), function(node){
				if(node && node.set){
					node.set("readOnly", value);
				}
			});
		},
		
		_isEmpty: function(){
			// summary:
			// 		Checks for whitespace. Should be overridden.
			return !array.some(this.getOptions(), function(opt){
				return opt.selected && opt.value != null && opt.value.toString().length != 0;
			});
		},
		
		_setLabelAttr: function(/*String*/ label){
			this.inherited(arguments);
			this.focusNode.setAttribute("aria-labelledby", this.id + "_label");
		},
		/**
		 * Overwrite dijit._HasDropDown._onDropDownMouseDown
		 * Open the drop down menu when readOnly is true.
		 */
		_onDropDownMouseDown: function(/*Event*/ e){
			// summary:
			//		Please see the _HasDropDown when the idt upgrade
			//		DO NOT copy the "_onDropDownMouseDown" in _HasDropDown.js, 
			//		use the temparory variable to record the readonly value
			var tempReadOnly = this.readOnly;
			
			if ( tempReadOnly ) {
				this.set("ReadOnly", false);
			}
			this.inherited(arguments);
			
			if ( tempReadOnly ) {
				this.set("ReadOnly", tempReadOnly);
			}
		},	
		
		/**
		 * Toggle the drop down menu.
		 */
		toggleDropDown: function(){
			// summary:
			//		Overwrite dijit._HasDropDown.toggleDropDown
			//		Open the drop down menu when readOnly is true.
	
			if(this.disabled){ return; }
			if(!this._opened){
				// If we aren't loaded, load it first so there isn't a flicker
				if(!this.isLoaded()){
					this.loadDropDown(lang.hitch(this, "openDropDown"));
					return;
				}else{
					this.openDropDown();
				}
			}else{
				this.closeDropDown();
			}
		},
		openDropDown: function(){
			var v = this.inherited(arguments);
			var ltr = domGeom.isBodyLtr(this.ownerDocument);
			if(!ltr && (this.focusNode === popup._firstAroundNode)){
				// popup repositionAll break checkboxselect menu in rtl mode, so skip it for now.
				popup._firstAroundNode = null;
			}
		},
		/**
		 * Overwrite dijit._HasDropDown._onKey
		 * Open the drop down menu when readOnly is true.
		 */
		_onKey: function(/*Event*/ e){
			// summary:
			//		Overwrite dijit._HasDropDown._onKey
			//		Open the drop down menu when readOnly is true.
			//		DO NOT copy the "_onKey" in _HasDropDown.js, 
			//		use the temparory variable to record the readonly value
			var tempReadOnly = this.readOnly;
			
			if ( tempReadOnly ) {
				this.set("ReadOnly", false);
			}
			this.inherited(arguments);
			
			if ( tempReadOnly ) {
				this.set("ReadOnly", tempReadOnly);
			}
		},
		
		_setDisabledAttr: function(){
			this.inherited(arguments);
			this._refreshState();
		},
		
		uninitialize: function(preserveDom){
			if(this.dropDown && !this.dropDown._destroyed){
				this.dropDown.destroyRecursive(preserveDom);
				delete this.dropDown;
			}
			this.inherited(arguments);
		}
	});

	if (has("dojo-bidi")) {
		baseClassName = baseClassName.substring(0, baseClassName.length-1);
		var baseCheckBox = iForm.CheckBoxSelect;
		iForm.CheckBoxSelect = declare(baseClassName,[baseCheckBox,bidiExtension]);
	}
	
	if ( has("mobile") || has("platform-plugable")) {
	
		var pluginRegistry = PlatformPluginRegistry.register("idx/form/CheckBoxSelect", 
				{	
					desktop: "inherited",	// no plugin for desktop, use inherited methods  
				 	mobile: MobilePlugin	// use the mobile plugin if loaded
				});
		
		iForm.CheckBoxSelect = declare("idx.form.CheckBoxSelect",[iForm.CheckBoxSelect, TemplatePlugableMixin, _WidgetsInTemplateMixin], {
					
			oneuiBaseClass: "dijitSelect",
			/**
		     * Set the template path for the desktop template in case the template was not 
		     * loaded initially, but is later needed due to an instance being constructed 
		     * with "desktop" platform.
	     	 */
			templatePath: require.toUrl("idx/form/templates/CheckBoxSelect.html"),  
		
			// set the plugin registry
			pluginRegistry: pluginRegistry,
			 			
			// override _createCheckBoxSelectMenu to use the plugin (or inherited function for desktop)
			_createCheckBoxSelectMenu: function(){
				return this.doWithPlatformPlugin(arguments, "_createCheckBoxSelectMenu", "createCheckBoxSelectMenu");
			},
			/**
			 * 
			 */
			onCheckStateChanged: function(item, checked){
				return this.doWithPlatformPlugin(arguments, "onCheckStateChanged", "onCheckStateChanged",item, checked);
			},
			/**
			 * Not Need on Mobile Platform
			 */
			_onDropDownMouseDown: function(){
				return this.doWithPlatformPlugin(arguments, "_onDropDownMouseDown", "_onDropDownMouseDown");
			},
			/**
			 * 
			 */
			closeDropDown: function(){
				return this.doWithPlatformPlugin(arguments, "closeDropDown", "closeDropDown");
			},
			/**
			 * 
			 */
			onCloseButtonClick: function(){
				return this.doWithPlatformPlugin(arguments, "onCloseButtonClick", "onCloseButtonClick");
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
	
	return iForm.CheckBoxSelect;
});