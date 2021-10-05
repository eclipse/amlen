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
	"dojo/dom-class",
	"dijit/CheckedMenuItem"
],function(declare, domClass, CheckedMenuItem){
	
	//	module:
	//		idx/form/_CheckBoxSelectMenuItem
	//	summary:
	//		A checkbox-like menu item for toggling on and off
	
	return declare("idx.form._CheckBoxSelectMenuItem", CheckedMenuItem, {
		// summary:
		//		A checkbox-like menu item for toggling on and off
		
		option: null,
		
		// reference of dojox.form._CheckedMultiSelectMenu
		parent: null,
		
		_updateBox: function(){
			// summary:
			//		Called to force the box to match the state of the select
			this.option.selected = !!this.option.selected; 
			this.set("checked", this.option.selected);
		},

		_onClick: function(/*Event*/ e){
			// summary:
			//		Clicking this item just toggles its state
			// tags:
			//		private
			if(e.keyCode != 13 && !this.disabled && !this.readOnly){
				this.option.selected = !this.option.selected;
				this.inherited(arguments);
			}
		},
		
		_setCheckedAttr: function(/*Boolean*/ checked){
			// summary:
			//		Hook so attr('checked', bool) works.
			//		Sets the class and state for the check box.
			domClass.toggle(this.domNode, "dijitCheckedMenuItemChecked", checked);
			this.domNode.setAttribute("aria-checked", checked);
			this._set("checked", checked);
		}
	});
});

