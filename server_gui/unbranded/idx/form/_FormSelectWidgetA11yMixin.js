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
	"dojo/_base/array", // array.filter array.forEach array.map array.some
	"dojo/_base/declare" // declare
],function(array, declare){
	return declare("idx.form._FormSelectWidgetA11yMixin",[],{
		_updateSelection: function(){
			this.inherited(arguments);
			array.forEach(this._getChildren(), function(child){
				child.domNode.removeAttribute("aria-selected");
			}, this);
		}
	});
});
