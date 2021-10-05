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
	"dojo/dom-construct",
	"dijit/_Widget",
	"dijit/_CssStateMixin",
	"dijit/_TemplatedMixin",
	"idx/form/_InputListItemMixin",
	"dijit/form/CheckBox"
], function(declare, domConstruct, _Widget, _CssStateMixin, _TemplatedMixin, _InputListItemMixin, CheckBox){
	//	module:
	//		idx/form/_CheckBoxListItem
	//	summary:
	//		An internal used list item for CheckBoxList.
	
	return declare("idx.form._CheckBoxListItem", [_Widget, _CssStateMixin, _TemplatedMixin, _InputListItemMixin], {
		//	summary:
		//		An internal used list item for CheckBoxList.
		
		templateString: "<div class='dijitReset ${baseClass}'><label for='${_inputId}' dojoAttachPoint='labelNode'></label></div>",
		
		baseClass: "idxCheckBoxListItem",
		
		postMixInProperties: function(){
			this.focusNode = new CheckBox({
				id: this._inputId
			});
			this.inherited(arguments);
		},
		
		postCreate: function(){
			domConstruct.place(this.focusNode.domNode, this.domNode, "first");
			this.inherited(arguments);
		},
		
		_changeBox: function(){
			// summary:
			//		Called to force the select to match the state of the check box
			//		(only on click of the checkbox)	 Radio-based calls _setValueAttr
			//		instead.
			if(this.get("disabled") || this.get("readOnly")){ return; }
			this.option.selected = !!this.focusNode.get("checked");
			this.parent.set("value",  this.parent.getValueFromOpts(this.option));
			this.parent.focusChild(this);
		},
		destroy: function(){
			this.focusNode.destroy();
			this.inherited(arguments);
		}
	});
});