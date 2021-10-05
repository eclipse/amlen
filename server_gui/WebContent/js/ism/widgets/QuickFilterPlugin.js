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
	'dojo/_base/lang',
	'dojo/query',
	'gridx/support/QuickFilter'
], function(declare, lang, query, QuickFilter){

	/**
	 * @name ism.widgets.QuickFilterPlugin
	 * @class ISM extension of gridx version to add an id to the div containing the filter input element
	 * @augments grid.support.QuickFilter
	 */
	return declare("ism.widgets.QuickFilterPlugin", [QuickFilter],
	{		
		containerId: undefined,
		
		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this,args);
		},

		postCreate: function() {
			this.inherited(arguments);

			var stateNode = this.textBox ? this.textBox.stateNode : undefined;
			if (stateNode && this.containerId) {
				var inputContainer = query(".dijitInputContainer", stateNode);
				if (inputContainer.length == 1) {
					inputContainer[0].id = this.containerId;
				}
			}
		}
	});
});
