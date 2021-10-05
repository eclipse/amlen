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
define(['ism/tests/Utils','dojo/_base/lang','ism/tests/widgets/LoginFixture'], function(Utils, lang, LoginFixture) {

	doh.register("ism.tests.widgets.test_OpState", [
		//
		// Tests defined with the DOH test fixture
		//
		new LoginFixture ({name: "test_getOpState_ethernet",
		    timeout: 10000,
       	    type: "ethernet-interface",
            name: "eth0",
            expectedState: 0,
		    runTest: function() {
		          var d = new doh.Deferred();
		          d.addCallback(lang.hitch(this,function(data) {
		                doh.assertTrue(data.state == this.expectedState); 
		          }));
		          dojo.xhrGet({
					    url:Utils.getBaseUri() + "rest/config/appliance/opState/"+this.type+"/"+this.name,
					    handleAs: "json",
						headers: {"Content-Type": "application/json",  "X-Session-Verify": dojo.cookie("xsrfToken")},
		                load: function(data) {
		                      d.callback(data);
		                }
				 }); 
		         return d;
		    }
		})
	]);
});
