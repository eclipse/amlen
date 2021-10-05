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
	'dojo/_base/declare',
	'dojo/query',
	'dijit/registry',
	'gridx/core/_Module',
	'ism/widgets/QuickFilterPlugin',
	'gridx/modules/Bar',
	'ism/widgets/TextBox',
	'idx/widget/Menu'
], function(declare, query, registry, _Module, QuickFilter){

// Create our own QuickFilter to work around IDX bug 11440
	
	return declare(_Module, {
		name: 'quickFilter',

		required: ['bar', 'filter'],

		preload: function(){
			this.grid.bar.defs.push({
				bar: 'top',
				row: 0,
				col: 3,
				pluginClass: QuickFilter,
				className: 'gridxBarQuickFilter',
				textBoxClass: 'ism/widgets/TextBox',
				menuClass: 'idx/widget/Menu',
				containerId: this.containerId
			});
		},
		
		// for backward compatibility
		apply: function(){
			query('.gridxQuickFilter', this.grid.headerNode).forEach(function(node){
				registry.byNode(node)._filter();
			});
		},

		// for backward compatibility
		clear: function(){
			query('.gridxQuickFilter', this.grid.headerNode).forEach(function(node){
				registry.byNode(node)._clear();
			});
		}
	});
});
