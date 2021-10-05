/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
define(['dojo/_base/declare',
		'dojo/dom-class'
		], function(declare, domClass) {
		
	return declare("ism.widgets._RequiredIndicatorMixin", [],
	{	
		/**
		 * Modifies the dom for the widget to either show the required indicator and
		 * set the required attribute to true, or to replace the indicator with space and 
		 * set the required attribute to false.
		 * @param on
		 */
		setRequiredOnOff: function(/*boolean*/ on) {
			this._toggleRequiredIndicator(on);
			this.set('required', on);
		},
	
		
		_toggleRequiredIndicator: function(on) {
			if (on) {
				var alignWithRequiredSpan = dojo.query(".alignWithRequired", "widget_"+this.id);
				if (alignWithRequiredSpan.length > 0) {
					alignWithRequiredSpan[0].innerHTML = "*&nbsp;";
					domClass.replace(alignWithRequiredSpan[0], "idxRequiredIcon", "alignWithRequired");
				}
			} else {
				var requiredIconSpan = dojo.query(".idxRequiredIcon", "widget_"+this.id);
				if (requiredIconSpan.length > 0) {
					requiredIconSpan[0].innerHTML = "";
					domClass.replace(requiredIconSpan[0], "alignWithRequired", "idxRequiredIcon");
				}
			}
		}
	});
});
