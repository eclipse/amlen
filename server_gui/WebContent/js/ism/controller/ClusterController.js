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
        'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/clusterStatistics',
		'ism/controller/content/ClusterMembership',
		'ism/controller/content/FlexDashboard',
		'ism/controller/content/ClusterStatsChart',
		'ism/controller/content/ClusterStatsGrid',
		'ism/widgets/ExternalString'
		], function(declare, lang, Resources, _PageController, nlsClusterStats) {

	var ClusterController = declare("ism.controller.ClusterController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the Cluster page.
		
		name: "cluster",
		
		constructor: function() {
		},

		init: function() {
			console.debug(this.declaredClass + ".init()");
			this.inherited(arguments);
		},

		_initLayout: function() {
			console.debug(this.declaredClass + "._initLayout()");
			// for debugging charts
			this.inherited(arguments);
		},

		_initWidgets: function() {
			console.debug(this.declaredClass + "._initWidgets()");
			this.inherited(arguments);
		},

		_initEvents: function() {
			console.debug(this.declaredClass + "._initEvents()");
			this.inherited(arguments);
		}
	});

	return ClusterController;
});
