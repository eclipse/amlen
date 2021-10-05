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
var profile = (function() {
	var testResourceRe = /\/tests\//,
	
	copyOnly = function(filename, mid) {
		var list = {};
		return (mid in list) ||  /(png|jpg|jpeg|gif|tiff)$/.test(filename);
	};

	return {
		resourceTags : {
			test : function(filename, mid) {
				return testResourceRe.test(mid);
			},

			copyOnly : function(filename, mid) {
				return copyOnly(filename, mid);
			},

			amd : function(filename, mid) {
				return !testResourceRe.test(mid) && !copyOnly(filename, mid) &&
					/\.js$/.test(filename);
			}
		},

		trees : [ [ ".", ".", /(\/\.)|(~$)/ ] ]
	};
})();
