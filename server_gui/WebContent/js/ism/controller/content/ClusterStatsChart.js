/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
		'dojo/i18n!ism/nls/clusterStatistics',
		'ism/widgets/_TemplatedContent',
		'dojo/text!ism/templates/cluster/statistics.html'
		], function(declare, lang, nls, _TemplatedContent, stats) {
	return declare("ism.controller.content.ClusterStatsChart", _TemplatedContent, {
		nls: nls,
		title: "",
		templateString: stats,
		
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		}
	});
});
