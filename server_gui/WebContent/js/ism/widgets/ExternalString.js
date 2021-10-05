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
		'dojo/_base/lang',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/login',
		'dojo/i18n!ism/nls/license',
		'dojo/i18n!ism/nls/home',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/monitoring',
		'dojo/i18n!ism/nls/appliance',
        'dojo/i18n!ism/nls/snmp',
		'dojo/i18n!ism/nls/clusterMembership',
		'dojo/i18n!ism/nls/clusterStatistics',
		'dojo/i18n!ism/nls/firstserver'
		], function(declare, lang, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, nlsGlobal, nlsLogin, nlsLicense, 
				nlsHome, nlsMsg, nlsMon, nlsAppl, nlsSnmp, nlsClusterMembership, nlsClusterStats, nlsFirstServer) {

	var ExternalString = declare("ism.widgets.ExternalString", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin ], {
		// summary:
		//		This widget allows an externalized string to be inserted into a static HTML file in a lightweight manner.
		// description:
		//		Specify a 'source' for the widget, which is the normal template definition of an NLS string.  All NLS
		//		string packages are included in this widget, referenced by "nlsLicense", "nlsMon", "nlsAppl", etc.
		//
		//		Usage:
		//			<span data-dojo-type="ism.widgets.ExternalString"
		//				  data-dojo-props="source: '${nlsAppl.appliance.networkSettings.dnsSubTitle}'" />
		nlsGlobal: nlsGlobal,
		nlsLogin: nlsLogin,
		nlsLicense: nlsLicense,
		nlsHome: nlsHome,
		nlsMsg: nlsMsg,
		nlsMon: nlsMon,
		nlsAppl: nlsAppl,
        nlsSnmp: nlsSnmp,
		nlsClusterMembership: nlsClusterMembership,
		nlsClusterStats: nlsClusterStats,
		nlsFirstServer: nlsFirstServer,
		
		templateString: null,
		source: null,
        clusterstatus: null,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			if (this.source) {
				this.templateString = "<span data-dojo-attach-point='stringNode'>" + this.source + "</span>";
			} else if (this.clusterstatus) {
			    if(ism.user.serverName && ism.user.serverName !== "" && ism.user.clusterName && ism.user.clusterName !== "") {
			    	var title = lang.replace(nlsClusterStats.detail_title,[ism.user.serverName, ism.user.clusterName]);
				    this.templateString = "<span data-dojo-attach-point='stringNode'>" + title + "</span>";
			    } else {
				    this.templateString = "<span data-dojo-attach-point='stringNode'>" + nlsClusterStats.title + "</span>";
			    }
			} else {
				console.error("source is required for ism.widgets.ExternalString");
			}
		}
	});
		
	return ExternalString;
});
