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
    'dojo/text!ism/controller/content/templates/MessagingPoliciesPane.html',
    'ism/controller/content/MessagingPolicyList',
	'dojo/i18n!ism/nls/messaging'
], function(declare, lang, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, ContentPane, template, 
		MessagingPolicyList, nls) {
 
    var MessagingPoliciesPane = declare("ism.controller.content.MessagingPoliciesPane",[_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        
    	templateString: template,
    	nls: nls,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function(args) {		
			this.topicPoliciesContainer.set("content",this.topicMPList);
			this.topicPoliciesContainer.set("list",this.topicMPList);
			this.topicMPList.set("messageHubItem", this.messageHubItem);
			this.topicMPList.set("detailsPage", this.detailsPage);
			
			this.subscriptionPoliciesContainer.set("content",this.subscriptionMPList);
			this.subscriptionPoliciesContainer.set("list",this.subscriptionMPList);
			this.subscriptionMPList.set("messageHubItem", this.messageHubItem);
			this.subscriptionMPList.set("detailsPage", this.detailsPage);
			
			this.queuePoliciesContainer.set("content",this.queueMPList);
			this.queuePoliciesContainer.set("list",this.queueMPList);
			this.queueMPList.set("messageHubItem", this.messageHubItem);
			this.queueMPList.set("detailsPage", this.detailsPage);
		},
		
		setItem: function(item) {
			this.messageHubItem = item;
			
			this.topicMPList.set("messageHubItem", item);
			this.subscriptionMPList.set("messageHubItem", item);
			this.queueMPList.set("messageHubItem", item);
		}
		
    });
 
    return MessagingPoliciesPane;
});
