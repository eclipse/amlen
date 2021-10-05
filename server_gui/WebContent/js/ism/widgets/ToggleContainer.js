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
define(['dojo/_base/declare',
        'dojo/dom-construct',
        'dojo/dom-attr',
        'dojo/dom-style',
		'idx/layout/TitlePane'
		], function(declare, domConstruct, domAttr, domStyle, TitlePane) {

	// extends TitlePane to capture and persist 'open' state
	// Note: This was refactored to use TitlePane to address accessibility issue with our custom widget
	var ToggleContainer = declare("ism.widgets.ToggleContainer", [TitlePane], {		

		// unique suffix for trigger id
		triggerId: "id",

		// if false, the content container is initially toggled closed
		startsOpen: true,

		// if true, no toggle icon is disabled and the container is permanently displayed 
		noToggle: false,
		
		width: undefined,

		constructor: function(args) {
			dojo.safeMixin(this, args);
		},

		postCreate: function() {
			this.inherited(arguments);
			this.set('open', this.startsOpen);
			this.set('collapsible', !this.noToggle);
			// create the legacy attach points (for backward compatibility)
			var titleId = this.triggerId+"_titleId";
			this.toggleNode = domConstruct.place("<div class='toggleTarget'></div>", this.containerNode);
			var customTitleOuter = domConstruct.place("<span class='toggleTitleContainer'></span>", this.titleNode);
			var customTitleInner = domConstruct.place("<h2 id='"+titleId+"' class='toggleHeading'></h2>", customTitleOuter);
			this.toggleTitle = domConstruct.place("<span data-dojo-attach-point='toggleTitle' class='toggleTitle'>"+this.title+"</span>", customTitleInner);
			this.toggleExtra = domConstruct.place("<span data-dojo-attach-point='toggleExtra' class='toggleExtra'></span>", customTitleInner);
			domAttr.set(this.containerNode, "aria-labelledby", titleId);
			if (this.width) {
				domStyle.set(this.domNode, {width: this.width}); 
				domStyle.set(this.domNode, {overflow: 'visible'});
			}
			// indent toggleTarget slightly if there is a twistie
			if (this.collapsible) {
				dojo.addClass(this.toggleNode, "toggleTargetIndent");
			}
		},
		
		resize: function() {
			this.inherited(arguments);
			domStyle.set(this.focusNode, {height: "auto"});
		}, 
		
		toggle: function() {
			this.inherited(arguments);
			// save state to user preferences
			ism.user.setSectionOpen(this.triggerId, this.get('open'));
		},
		
		_setTitleAttr: function(title) {
			if (this.toggleTitle) {				
				this.toggleTitle.innerHTML = title;
			}
			this.title = title;
		}
	});

	return ToggleContainer;
});
