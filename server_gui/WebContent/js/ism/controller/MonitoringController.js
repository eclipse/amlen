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
		'ism/widgets/_TemplatedContent',
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/strings',
		'ism/controller/content/ConnectionChart',
		'ism/controller/content/ConnectionGrid',
		'ism/controller/content/EndpointChart',
		'ism/controller/content/EndpointGrid',
		'ism/controller/content/QueueMonitorGrid',
		'ism/controller/content/TopicMonitorGrid',
		'ism/controller/content/MQTTClientGrid',
		'ism/controller/content/SubscriptionGrid',
		'ism/controller/content/TransactionGrid',
		'ism/controller/content/DestinationMappingRuleMonitorGrid',
		'ism/controller/content/StoreMonitorGrid',
		'ism/controller/content/MemoryMonitorGrid',		
		'ism/widgets/ExternalString'
		], function(declare, _TemplatedContent, Resources, _PageController, nls) {

	var MonitoringController = declare("ism.controller.MonitoringController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the Monitoring page.
		
		name: "monitoring",
		
		constructor: function() {
		},

		init: function() {
			console.debug(this.declaredClass + ".init()");
			this.inherited(arguments);
		},

		_initLayout: function() {
			console.debug(this.declaredClass + "._initLayout()");
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

	return MonitoringController;
});
