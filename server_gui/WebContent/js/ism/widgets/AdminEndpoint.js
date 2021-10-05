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
        'dojo/request',
		'dojo/json',
		'dojo/cookie',
		'dojo/DeferredList',
		'dojo/Deferred',
		'ism/widgets/User',
		'ism/IsmUtils',
		'dojo/i18n!ism/nls/adminEndpointConfig'
		], function(declare, lang, request, json, cookie, DeferredList, Deferred, User, Utils, nls) {

    // Used to interact with an AdminEndpoint's REST API    
	var AdminEndpoint = declare("ism.widgets.AdminEndpoint", null, {	    
	    
		uid: undefined,
		server: {},
		prefix: undefined,
		version: "v1",
	    sendCredentials : false,
	    checkCertificateURL: undefined,
	    licenseURL: undefined,

		constructor: function(args) {
			// summary:
			// 		Constructor.  Set fields from args.  Args must include the AdminEndpoint uid.
			dojo.safeMixin(this, args);
			if (this.server.uid) {	
			    // A Test Connection may pass in a server that is not saved yet, so use it if it's there
			    this.uid = this.server.uid;
			} else if (this.uid) {
			    this.server = ism.user.getServer(this.uid);
			}
			var address = this.uid;
			// if the server is defined as localhost, we need to resolve that to the address the web ui is served from
			if (this.server.adminServerAddress == "127.0.0.1") {
			    var hostname = location.hostname;
			    var port = this.server.adminServerPort;
			    if (hostname.indexOf(":") > -1 && hostname.indexOf("[") !== 0) {
			        hostname = "[" + hostname + "]";
			    }
			    address = hostname + ":" + port;
			}
			this.prefix = "https://" + address +"/ima/"+this.version;
			this.sendCredentials = this.server.sendWebUICredentials ? true : false;
            this.checkCertificateURL = "https://" + address + "/index.html";
            this.licenseURL = "https://" + address + "/license";
		},
		
		/**
		 * Get an object that contains our default options for a GET request
		 */
		getDefaultGetOptions: function() {
		    return this._addSSOHeader({ method: "GET", preventCache: true, handleAs: "json", headers: { "X-Requested-With": null } });
		},

	    /**
         * Get an object that contains our default options for a DELETE request
         */
		getDefaultDeleteOptions: function() {
		    return this._addSSOHeader({ method: "DELETE", handleAs: "json", headers: { "X-Requested-With": null } });
		},

        /**
         * Get an object that contains our default options for a POST request
         */
		getDefaultPostOptions: function() {
            return this._addSSOHeader({ method: "POST", handleAs: "json", headers: { "Content-Type": "application/json", "X-Requested-With": null } });
        },
        
        /**
         * Create post options that use the specified values as the payload data
         * @param objectType string, type of object
         * @param values object, values for the object. Named objects must include 'Name' property.
         */
        createPostOptions: function(objectType, values) {
            var options = this.getDefaultPostOptions();
            if (objectType && values) {
                var data = {};
                var name = values.Name;
                if (name) {
                    data[objectType] = {};
                    data[objectType][name] = dojo.clone(values);
                    delete data[objectType][name].Name;
                } else {
                    data[objectType] = values;
                } 
                options.data = json.stringify(data);
            }            
            return options;
        },

        /**
         * Add the ssoHeader if sendCredentials is true and the SSO cookie is available.
         * Returns the updated options object.
         */
        _addSSOHeader: function(options) {            

            if (this.sendCredentials) {
                var ssoCookie = cookie("imaSSO");
                if (!options) {
                    options = { headers: {}};
                } else if (!options.headers) {
                    options.headers = {};
                }
                if (ssoCookie) {
                   options.headers["imaSSO"] = ssoCookie;
                }
            }
            
            /* We need withCredentials if passwords are in use at all. 
               Always allow credentials to be sent. Some browsers will not prompt the
               user for password otherwise. */
            
            options.withCredentials = true;
            return options;
        },
        
        /**
         * Submit a request to the admin endpoint using the new Dojo request API
         * 
         * resource: The resource uri, properly encoded. For example: /configuration/Endpoint/DemoEndpoint
         * options:  The Dojo request options as defined by http://dojotoolkit.org/reference-guide/1.9/dojo/request.html#dojo-request
         * 
         * Returns the request promise.
         */
		doRequest: function(resource, options) {
		    
		    resource = resource ? resource : "";
		    if(resource.indexOf("/service/status") < 0 && resource.indexOf("/monitor") < 0) {
		    	// Register user interaction for http session.
		    	console.debug("updating last access time");
		        dojo.publish("lastAccess", {requestName: resource});
		    }
		    	    
		    var uri = resource.indexOf("/") === 0 ? this.prefix + resource : this.prefix + "/" + resource;
		    var promise = request(uri, options);
		    
//		    // Output debug information.  This can be removed at some point...  
//		    promise.then(
//		         function(data) {
//		             console.debug("doRequest: uri = " + uri + ", options: " + json.stringify(options) + ", data: " 
//		                     + options.handleAs == "json" ? json.stringify(data) : data);
//		         },
//		         function(error) {
//                     console.debug("doRequest: uri = " + uri + ", options: " + json.stringify(options) + ", error: " 
//                             + json.stringify(error));
//		         }
//		    );
//            promise.response.then(
//                    function(response) {
//                        console.debug("doRequest response: " + json.stringify(response));
//                    },
//                    function(error) {
//                        console.debug("doRequest response error: " + json.stringify(error));
//                    }
//            );
            
            // Return the promise to the caller. The caller can use promise.then or promise.response.then to get what it needs.
		    return promise;
		},
		
		doLicenseRequest: function(lang, licensedUsage) {
			var uri = this.licenseURL + (licensedUsage === "Developers" ? "/developer/LA.html" : "/LA.html");
			var promise = request(uri, { method: "GET", preventCache: true, headers: { "X-Requested-With": null, "Accept-Language": lang} });
			
//			promise.then(
//					function(data) {
//						console.debug("license then: " + json.stringify(data));
//					},
//					function(error){
//						console.debug("license error: " + json.stringify(error));
//					});
//			promise.response.then(
//					function(data) {
//						return !data.text;
//					},
//					function(error){
//						console.debug("license error response: " + json.stringify(error));
//					});
			return promise;
		},
		
		/**
		 * Upload the specified file.
		 * @param file The file to upload
		 * @param onComplete The onComplete handler
		 * @param onError The onError handler
		 * @param contentType The content type of the file. Defaults to application/octet-stream
		 */
		uploadFile: function(/*File*/ file, /*Function*/ onComplete, /*Function*/ onError, /*String*/ contentType) {
            contentType = contentType ? contentType : "application/octet-stream";
		    var options = this._addSSOHeader({ method: "PUT", handleAs: "json", headers: { "Content-Type": contentType } });
		    /* Debug console output looks for the value of data so set it to the file name */
		    options.data = file.name;
		    var promise = new Deferred();
		    var resource = "/file/" + encodeURIComponent(file.name);
		    var _file = file;
            var uploadreq = new XMLHttpRequest();
            uploadreq.onload = function(evt) {
            	console.debug(file.name + " upload complete");
            	if (onComplete) {
                	onComplete(evt, _file);
            	}
            	promise.resolve(evt.target.response);
            };
            uploadreq.onerror = function(error) {
            	console.debug(file.name + " upload FAILED. Error: " + error);
            	if (onError) {
                	onError(error);
            	}
            	promise.reject(error, _file);
            };
            uploadreq.onloadend = function(evt) {
            	console.debug(file.name + " upload attempt has finished");
            	console.debug("Result: "+evt+" Details: "+json.stringify(evt));
            	/*onComplete(evt);*/
            };
            uploadreq.open('PUT', this.prefix + resource, true);
            if (options.headers) {
            	for(var hdr in options.headers) {
            		if(options.headers.hasOwnProperty(hdr)) {
            			console.debug("Setting header "+hdr+" to "+options.headers[hdr]);
            		    uploadreq.setRequestHeader(hdr, options.headers[hdr]);
            		}
            	}
            }
            uploadreq.withCredentials = options.withCredentials;
            try {
                uploadreq.send(file);
            } catch(err) {
            	console.debug("Error on call to send "+file.name+" - Error: "+err);
            	uploadreq.onerror(err);
            }
            return promise.promise;
		},
		
		/**
		 * Creates an array containing all of the objects of the requested objectType from
		 * data.  If data is null, objectType is null, or data does not contain any objects
		 * of objectType, returns an empty array.
		 * @param data
		 * @param objectType
		 * @returns {Array}
		 */
		getObjectsAsArray: function(data, objectType) {
		    var results = [];
		    // returns an array of named objects of objectType
		    if (!data || !objectType || !data[objectType]) {
		        return results;
		    }
		    var all = data[objectType];
		    var i = 0;
		    for (var name in all) {
		        results[i] = all[name];
		        results[i].Name = name;
		        i++;
		    }
		    return results;
		},
		
		/**
		 * Check data for the objectType with name objectName. If found,
		 * return it, otherwise return undefined.
		 * @param data
		 * @param objectType
		 * @param objectName
		 * @returns named object
		 */
		getNamedObject: function(data, objectType, objectName) {
		    var results = undefined;
            // returns an array of named objects of objectType
            if (!data || !objectType || !objectName || !data[objectType] || !data[objectType][objectName]) {
                return results;
            }
            results = data[objectType][objectName];
            results.Name = objectName;
            return results;		    
		},
		
		/**
		 * Get the results of the promises.  If any promise fails and it's not because there were none of the requested object types,
		 * then onError is called with the first failure.  Otherwise, onComplete is called with an object that contains each result.
		 * @param promises  An array containing the promises to get results for
		 * @param objectTypes An array containing the objectType each promise is querying for.
		 * @param onComplete The function to call when all queries complete successfully. This function is passed an object that uses 
		 * the objectType as the key and an array of objects from the query result as the value.  A second object provides the raw
		 * results of each query using the same naming convention.
		 * @param onError (optional) The function to call when any query fails. This function is passed the error object. 
		 */
		getAllQueryResults: function(promises, objectTypes, onComplete, onError) {

            // NOTE: promise/all doesn't work well because the admin REST API is treating no config objects as a 404 instead of returning an empty object.
            // promise/all will return only the first error, which makes it unsatisfactory. 
		    
		    if (!promises || !objectTypes || !onComplete) {
		        return;
		    }

		    var dl = new dojo.DeferredList(promises);                       
            dl.then(function(results) {
                var error = undefined;                
                var len = results.length;                
                for (var i = 0; i < len; i++) {
                    if (results[i][0] === false) {
                        if (Utils.noObjectsFound(results[i][1])) {
                            results[i][1] = {};
                            results[i][1][objectTypes[i]] = {};
                        } else { 
                            if (onError) {
                                onError(results[i][1]);
                            }
                            return;
                        }
                    }                    
                }

                var queryResults = {};
                var rawData = {};
                var adminEndpoint = ism.user.getAdminEndpoint();
                for (i = 0; i < len; i++) {
                    queryResults[objectTypes[i]] = adminEndpoint.getObjectsAsArray(results[i][1], objectTypes[i]);
                    rawData[objectTypes[i]] = results[i][1];
                }
                onComplete(queryResults, rawData);
            });		    
		},
		
		getCertificateInvalidMessage: function(addServerError) {
			var errString = lang.replace(nls.certificateInvalidMessage, [this.checkCertificateURL]);
			if (addServerError && (addServerError === true)) {
		        return errString + nls.certificatePreventError;
			} else {
				return errString + nls.certificateRefreshInstruction + nls.certificatePreventError;
			}
		}
	});

	return AdminEndpoint;
});
