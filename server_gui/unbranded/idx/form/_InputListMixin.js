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
	"dojo/_base/kernel",
	"dojo/_base/lang",
	"dojo/_base/array",
	"dojo/_base/sniff",
	"dojo/dom-attr",
	"dojo/dom-class",
	"dojo/dom-style",
	"dojo/query",
	"dojo/i18n",
	"idx/widget/HoverHelpTooltip",
	"dijit/form/_FormValueWidget",
	"dojo/i18n!./nls/_InputListMixin"
], function(declare, kernel, lang, array, has, domAttr, domClass, domStyle, query, i18n, HoverHelpTooltip, _FormValueWidget, _inputListMixinNls){
	
	//	module:
	//		idx/form/_InputListMixin
	//	summary:
	//		An internal mix in class for CheckBoxList and RadioButtonSet

	return declare("idx.form._InputListMixin", null, {
		// summary:
		//		An internal mix in class for CheckBoxList and RadioButtonSet

		instantValidate: true,
		
		groupAlignment: "vertical",
		
		lastFocusedChild: null,
		
		postCreate: function(){
			this._event = {
				"input" : "onChange",
				"blur" 	: "_onBlur",
				"focus" : "_onFocus"
			}
//			if(this.instantValidate){;
//				this.connect(this, "_onFocus", function(){
//					if(this.message == ""){return;}
//					this.displayMessage(this.message);
//				});
//			}
			
			this.inherited(arguments);
			this.connect(this, "focusChild", function(){this.lastFocusedChild = this.focusedChild;});
			domAttr.set(this.stateNode, {tabIndex: -1});
			
			// Indicate that the widget is just loaded.
			// Do not perform the _isEmpty() check.
			this.initLoaded = true;
		},
		
		setStore: function(store, selectedValue, fetchArgs){
			// summary:
			//		If there is any items selected in the store, the value
			//		of the widget will be set to the values of these items.
			this.inherited(arguments);
			var setSelectedItems = function(items){
				var value = array.map(items, function(item){ return item.value[0]; });
				if(value.length){
					this.set("value", value);
				}
			};
			this.store.fetch({query:{selected: true}, onComplete: setSelectedItems, scope: this});
		},
		
		postMixInProperties: function(){
			this._nlsResources = _inputListMixinNls;
			this.missingMessage || (this.missingMessage = this._nlsResources.invalidMessage);
			this.inherited(arguments);
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

			array.forEach(opts, function(i){
				i.selected = array.some(newValue, function(v){ return v.value === i.value; });
			});
			var	val = array.map(newValue, function(i){ return i.value; }),
				disp = array.map(newValue, function(i){ return i.label; });

			if(typeof val == "undefined"){ return; } // not fully initialized yet or a failed value lookup
			this._setDisplay(this.multiple ? disp : disp[0]);
			_FormValueWidget.prototype._setValueAttr.apply(this, arguments);
			this._updateSelection();
		},
		
		_getValueFromOpts: function(){
			// summary:
			//		Returns the value of the widget by reading the options for
			//		the selected flag
			var opts = this.getOptions() || [];
			if(!this.multiple && opts.length){
				// Mirror what a select does - choose the first one
				var opt = array.filter(opts, function(i){
					return i.selected && i.selected !== "false";
				})[0];
				if(opt && opt.value){
					return opt.value
				}
			}else if(this.multiple){
				// Set value to be the sum of all selected
				return array.map(array.filter(opts, function(i){
					return i.selected && i.selected !== "false";
				}), function(i){
					return i.value;
				}) || [];
			}
			return "";
		},
		
		onAfterAddOptionItem: function(item, option){
			// summary:
			//		a function that can be connected to in order to receive a
			//		notification that an item as been added to this dijit.
		},
	
		onChange: function(newValue){
			// summary:
			//		Validate if selection changes.
			this.validate(this.focused);
		},
		
		reset: function(){
			// summary: Overridden so that the state will be cleared.
			this.inherited(arguments);
			this.displayMessage("");
		},
		
		_onMouseDown: function(e){
			// summary:
			//		Cancels the mousedown event to prevent others from stealing
			//		focus
			this.mouseFocus = true;
			domAttr.set(this.domNode, "tabIndex", "-1");
			if(has("ie") < 9 || has("quirks")){
				e.cancelBubble = true;
			}else{
				e.stopPropagation();
			}
		},
		
		_onContainerFocus: function(evt){
			if(evt.target !== this.domNode){ return; }
			if(!this.mouseFocus){
				if(this.lastFocusedChild){
					this.focusChild(this.lastFocusedChild);
				}else{
					this.focusFirstChild();
				}
			}
			domAttr.set(this.domNode, "tabIndex", "-1");
		},
		
		_updateSelection: function(){
			// summary:
			//		Overwrite dijit.form._FormSelectWidget._updateSelection to remove aria-selected attribute
			this._set("value", this._getValueFromOpts());
			var val = this.value;
			if(!lang.isArray(val)){
				val = [val];
			}
			if(val && val[0]){
				array.forEach(this._getChildren(), function(child){
					var isSelected = array.some(val, function(v){
						return child.option && (v === child.option.value);
					});
					domClass.toggle(child.domNode, this.baseClass.replace(/\s+|$/g, "SelectedOption "), isSelected);
				}, this);
			}
			
			domAttr.set(this.valueNode, "value", this.value);
			array.forEach(this._getChildren(), function(item){ 
				item._updateBox(); 
			});
		},
		
		_getChildren: function(){
			return this.getChildren();
		},
		
		invertSelection: function(onChange){
			// summary: Invert the selection
			// onChange: Boolean
			//		If null, onChange is not fired.
			array.forEach(this.options, function(i){
				i.selected = !i.selected;
			});
			this._updateSelection();
			this._handleOnChange(this.value);
		},
		
		_setDisabledAttr: function(value){
			// summary:
			//		Disable (or enable) all the children as well
			this.inherited(arguments);
			// Fixed dojo.attr(this.focusNode, "disabled", false) bug
			value || domAttr.remove(this.valueNode, "disabled");
			
			array.forEach(this.getChildren(), function(node){
				if(node && node.set){
					node.set("disabled", value);
				}
			});
		},
		
		_setReadOnlyAttr: function(value){
			// summary:
			//		Sets read only (or unsets) all the children as well
			this.inherited(arguments);
			array.forEach(this.getChildren(), function(node){
				if(node && node.set){
					node.set("readOnly", value);
				}
			});
		},
		
		_setGroupAlignmentAttr: function(/*String*/ value){
			this._set("groupAlignment", value);
			array.forEach(this.getChildren(), function(item){
				domClass.toggle(item.domNode, "dijitInline", !(value == "vertical"));
			}, this);	
		},
		
		_setRequiredAttr: function(required){
			this._set("required", required);
			this.inherited(arguments);
		},
		
		_isEmpty: function(){
			if(this.initLoaded){ return false; }
			return this.value == "";
		},
		
		_relayout: function(node){
			domStyle.set(node, "zoom", "1");
			domStyle.set(node, "zoom", "");
		},
		
		startup: function(){
			this.inherited(arguments);
			this.initLoaded = false;
		},
		
		_setLabelAttr: function(/*String*/ label){
			// Overwrite idx.form._CompositeMixin to add aria-label attribute.
			this.compLabelNode.innerHTML = label;
			var islabelEmpty = /^\s*$/.test(label);
			islabelEmpty && domAttr.remove(this.compLabelNode, "for");
			domClass.toggle(this.labelWrap, "dijitHidden", islabelEmpty);
			this._set("label", label);
			domAttr.set(this.stateNode, "aria-label", label);
		},
		
		uninitialize: function(){
			HoverHelpTooltip.hide(this.domNode);
			// Make sure these children are destroyed
			array.forEach(this._getChildren(), function(child){
				child.destroyRecursive();
			});
			this.inherited(arguments);
		}
	
	});
});