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
		'ism/IsmUtils',
		'ism/controller/content/MessageHubDetails',
		'ism/controller/content/MessageHubList',
		'ism/controller/content/MessageProtocolsList',
		'ism/widgets/ExternalString',
		'ism/widgets/AssociationControl',
		'ism/controller/content/QMConnectionProperties',
		'ism/controller/content/QMConnectionGrid',
		'ism/controller/content/DestinationMapping',
		'ism/controller/content/SSLKeyRepository',
		'ism/controller/content/MessageQueues',
		'ism/controller/content/ToggleEndpoint'
		], function(declare, _TemplatedContent, Resources, _PageController, nls, Utils) {

	var MessagingController = declare("ism.controller.MessagingController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the Messaging page.
		
		name: "messaging",
		
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
		},
				
		/*
		 * iterates through the specified queryResults and returns an array of all items in the
		 * store whose refProperty property matches the refValue value
		 */
		getReferencingItems: function(queryResults, refProperty, refValue) {
			var results = [];
			var len = queryResults ? queryResults.length : 0;
			for (var i=0; i < len; i++) {
				if (queryResults[i][refProperty] == refValue) {
					results.push(queryResults[i]);
				}
			}
			return results;
		} 

	});

	return MessagingController;
});
