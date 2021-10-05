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
    'dojo/dom-class',
    'ism/widgets/_TemplatedContent',
    'dojo/text!ism/widgets/templates/NoticeIcon.html'
], function(declare, lang, domClass, _TemplatedContent, template) {
 
    var NoticeIcon = declare("ism.widgets.NoticeIcon", _TemplatedContent, {
        
    	templateString: template,
		iconClass: "",
		hoverHelpText: "",
       
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function() {		
			this.inherited(arguments);
		},
		
		_setIconClassAttr: function(iconClass) {
			this.iconClass = iconClass;
			domClass.replace(this.noticeIcon, iconClass);
		},
		
		_setHoverHelpTextAttr: function(hoverHelpText) {
			this.hoverHelpText = hoverHelpText;
			this.noticeIcon_Tooltip.domNode.innerHTML = hoverHelpText;
		}

    });
    
    return NoticeIcon;
 
});
