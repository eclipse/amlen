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
	'dijit/_Widget',
	'dijit/_TemplatedMixin',
	'dijit/_WidgetsInTemplateMixin',
	'dijit/layout/ContentPane',
    'dojo/text!ism/controller/content/templates/ConnectionPoliciesPane.html',
    'ism/controller/content/ConnectionPolicyList',
	'dojo/i18n!ism/nls/messaging'
], function(declare, lang, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, ContentPane, template, 
		ConnectionPolicyList, nls) {
 
    var ConnectionPoliciesPane = declare("ism.controller.content.ConnectionPoliciesPane",[_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        
    	templateString: template,
    	nls: nls,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function(args) {
			this.ismConnectionPolicyList.set("messageHubItem", this.messageHubItem);
			this.ismConnectionPolicyList.set("detailsPage", this.detailsPage);
		},
		
		setItem: function(item) {
			this.messageHubItem = item;
			this.ismConnectionPolicyList.set("messageHubItem", item);
		}
		
    });
 
    return ConnectionPoliciesPane;
});
