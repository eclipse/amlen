/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
    'dojo/_base/array',
	'dojo/_base/declare',
	'dojo/dom-style',
	'dojo/query',
	'dojo/dom-class',
	'dojox/html/entities',
	'idx/form/CheckBoxSelect',
	'ism/widgets/_RequiredIndicatorMixin',
	'idx/string'
], function(array, declare, domStyle, query, domClass, entities, CheckBoxSelect, _RequiredIndicatorMixin, iString){
	/**
	 * @name ism.widgets.CheckBoxList
	 * @class ISM extension of One UI version CheckBoxList
	 * @augments idx.form.CheckBoxList
	 */
	return declare("ism.widgets.CheckBoxSelect", [CheckBoxSelect, _RequiredIndicatorMixin],
	{
		// summary:
		// 		An extension of the One UI CheckBoxSelect that 
		//      fixes the zero width issue
		spanWidth: 130,

		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			if (this.fieldWidth && this.fieldWidth > 50) {
				this.spanWidth = this.fieldWidth - 50;
			}
		},
		
		startup: function() {
			this.inherited(arguments);
			var width = domStyle.get(this.containerNode, "width");
			width = width > 0 ? width : this.spanWidth;
			domStyle.set(this.containerNode, "width", width + "px");
		},
		
		/*
		 * override to address ism defect 41642 (idx defect 11404)
		 */
		_setDisplay: function(newDisplay){
			// if newDisplay is null or undefined or an empty string, let super handle it
			if (!newDisplay || (typeof newDisplay == "string" && iString.nullTrim(newDisplay) === null)) {
				this.inherited(arguments);
				return;
			}

			// make newDisplay an array if it isn't already an array
			if(!(newDisplay instanceof Array)){
				newDisplay = [newDisplay];
			}
			
			// if newDisplay is empty or there are no options, let super handle it
			var opts = this.getOptions() || [];			
			if (newDisplay.length === 0 || opts.length === 0) {
				this.inherited(arguments);
				return;				
			}
						
			// check to see if we need to use the value instead of the label
			array.forEach(newDisplay, function(item, index){
				// find the option that matches the item
				var option = array.filter(opts, function(node){					
					return node.label == item && dojox.html.entities.encode(node.value) == item;	
				})[0];
				if (option) {
					newDisplay[index] = option.value;
				}
			});
			this.inherited(arguments);
		}

	});
});
