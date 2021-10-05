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
		'dojo/json',
		'ism/Utils',
		"dojo/i18n!ism/nls/license"
		], function(declare, JSON, Utils, nls) {

	var License = {
		// summary:
		// 		A representation of an License configuration object
		nls: nls,
		
		constructor: function(args) {
			// summary:
			// 		Constructor.  Set fields from args.
			dojo.safeMixin(this, args);
		},

		getStatus: function() {
			console.debug(this.declaredClass + ".getStatus()");
			
			var STATUS_LICENSE_ACCEPTED = 5;
			var status = {accepted: false, virtual: false};
			dojo.xhrGet({
			    url: Utils.getBaseUri() + "rest/config/license/",
			    handleAs: "json",
			    preventCache: true,
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.dir(data);
			    	if (data.status == STATUS_LICENSE_ACCEPTED) {
			    		status.accepted = true;			    		
			    	} 
			    	if (data.virtual) {
			    		status.virtual = data.virtual;
			    	}
			    },
				error: function(error, ioargs) {
					console.error(error);
					Utils.checkXhrError(ioargs);
				}
			  });
			console.debug("returning from getStatus: "+status);
			return status;
		},
		
		getLicense: function(type,lang,onload) {
			console.debug(this.declaredClass + ".getLicense("+lang+")");
			
			var STATUS_LICENSE_VALID = 0;
			dojo.xhrGet({
			    url: Utils.getBaseUri() + "rest/config/license/"+type+"/"+lang,
			    handleAs: "json",
			    preventCache: true,
			    load: function(data){
			    	console.dir(data);
			    	if (data.status == STATUS_LICENSE_VALID) {
			    		onload.load(data.licenseText);
			    	} else {
			    		console.error("License request returned status: "+data.status);
			    		onload.error(nls.license.error);
			    	}
			    },
			    error: function(error, ioargs) {
			    	console.error(error);
		    		onload.error(nls.license.error);
			    	Utils.checkXhrError(ioargs);
			    }
			  });
		},

		getNonIbmLicense: function(onload) {
			console.debug(this.declaredClass + ".getNonIbmLicense()");
			
			var STATUS_LICENSE_VALID = 0;
			dojo.xhrGet({
			    url: Utils.getBaseUri() + "rest/config/license/nonibm/en",
			    handleAs: "json",
			    preventCache: true,
			    load: function(data){
			    	console.dir(data);
			    	if (data.status == STATUS_LICENSE_VALID) {
			    		onload.load(data.licenseText);
			    	} else {
			    		console.error("License request returned status: "+data.status);
			    		onload.error(nls.license.error);
			    	}
			    },
			    error: function(error, ioargs) {
			    	console.error(error);
		    		onload.error(nls.license.error);
			    	Utils.checkXhrError(ioargs);
			    }
			  });
		},
		
		getNotices: function(onload) {
			console.debug(this.declaredClass + ".getNotices()");
			
			var STATUS_LICENSE_VALID = 0;
			dojo.xhrGet({
			    url: Utils.getBaseUri() + "rest/config/license/notices/en",
			    handleAs: "json",
			    preventCache: true,
			    load: function(data){
			    	console.dir(data);
			    	if (data.status == STATUS_LICENSE_VALID) {
			    		onload.load(data.licenseText);
			    	} else {
			    		console.error("License request returned status: "+data.status);
			    		onload.error(nls.license.error);
			    	}
			    },
			    error: function(error, ioargs) {
			    	console.error(error);
		    		onload.error(nls.license.error);
			    	Utils.checkXhrError(ioargs);
			    }
			  });
		}
	};

	return License;
});

