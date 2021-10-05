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
	"dojo/_base/lang"
], function(declare, lang){
	
	//	module:
	//		idx/form/InputListItemMixin
	//	summary:
	//		An internal mix in class for CheckBoxList and RadioButtonSet

	return declare("idx.form._InputListItemMixin",
		null,
		{
		// summary:
		//		The individual items for a InputList
		
		widgetsInTemplate: true,
		
		// option: dojox.form.__SelectOption
		//		The option that is associated with this item
		option: null,
		parent: null,
		
		// disabled: boolean
		//		Whether or not this widget is disabled
		disabled: false,
	
		// readOnly: boolean
		//		Whether or not this widget is readOnly
		readOnly: false,
	
		postCreate: function(){
			this.inherited(arguments);
			this.focusNode.onClick = lang.hitch(this, "_changeBox");
			this.labelNode.innerHTML = this.option.label;
			this._updateBox();
		},
		
		_updateBox: function(){
			// summary:
			//		Called to force the box to match the state of the select
			this.focusNode.set("checked", !!this.option.selected);
		},
		
		_setDisabledAttr: function(value){
			// summary:
			//		Disables (or enables) all the children as well
			this.disabled = value;
			this.focusNode.set("disabled", this.disabled);
		},
		
		_setReadOnlyAttr: function(value){
			// summary:
			//		Sets read only (or unsets) all the children as well
			this.readOnly = value;
			this.focusNode.set("readOnly", value);
		},
		
		focus: function(){
			this.focusNode.focus();
		}
	});
});