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
	"dojo/_base/declare" // declare
],function(declare){
	return declare("idx.form._AutoCompleteA11yMixin",[],{
		_showResultList: function(){
			var temp = this.domNode;
			this.domNode = this.oneuiBaseNode;
			this.inherited(arguments);
			this.domNode = temp;
		},
		
		closeDropDown: function(){
			var temp = this.domNode;
			this.domNode = this.oneuiBaseNode;
			this.inherited(arguments);
			this.domNode = temp;
		},
		
		_announceOption: function(){
			this.inherited(arguments);
			this.focusNode.removeAttribute("aria-activedescendant");
		}
	});
});
