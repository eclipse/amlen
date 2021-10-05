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

define(["dojo/_base/lang", 
	    "dojo/_base/fx",
	    "dojo/cookie",
		'dojo/query',
		'dojo/dom-class'
	    ], function(lang, fx, cookie, query, domClass) {
	
	/**
	 * This Utilities class should not contain anything ${IMA_PRODUCTNAME_SHORT} specific.  Use IsmUtils instead.
	 */

	var _getBaseUri = function() {
		var baseUri = location.pathname.split("/").slice(0,1).join("/");
		if (baseUri.length === 0 || baseUri[baseUri.length-1] !== "/") {
			baseUri += "/"; 
		}
		return baseUri;
	};
	
	var Utils = {
		reloading: false,
		getBaseUri: function () {
			return _getBaseUri();
		},
		
		errorhandling: {
			errorMessageTopic: "global/message/error",
			warningMessageTopic: "global/message/warning",
			infoMessageTopic: "global/message/info"
		},

		
		/*
		 * Call checkXhrError() in every xhr error callback (or errback) to check for credential timeout. 
		 * 
		 * The default ltpa token timeout in Liberty is 120 minutes. This means that every 120 minutes, 
		 * regardless of activity, the user needs to re-login. If the user issues an xhr request after 
		 * the ltpa token has timed out, then the xhr request is redirected to the login with a status 
         * 302 temporary redirect,  but it won't actually load the login. It actually tries to parse the
         * login page, resulting in an error callback with a status of 200 and a response containing the
         * html markup of the login page.  If we just redirect to the login page, the page will display, 
         * but after login the user will be redirected to the XHR url instead of the page they were on! 
         * To solve this, we reload the current page. The current page reload will check the authorization, 
         * prompt for login, then show the page the user was on.  There is one more small problem - if
         * the xhr request that was redirected was a post, Liberty leaves a nasty surprise cookie, called
         * WASPostParam, with a path of the xhr url and a domain of the server.  If we leave this cookie
         * lingering, then subsequent GETs to the same REST resource will fail because they will be treated
         * as POSTs.  So, this method also clears that cookie.
		 */
		checkXhrError: function (/*xhr ioargs object*/ ioargs) {
			if (this.reloading) {
				return {reloading: true};
			}
			if (!ioargs) {
				console.debug("checkXhrError called with no ioargs...");
				return {};
			}
			if (!ioargs.xhr && ioargs.message) {
				console.debug("checkXhrError forwarding to checkError...");
				return Utils.checkError(ioargs);
			}
			if (ioargs.xhr.status == 200) {
				var responseText = ioargs.xhr.responseText;
				if (dojo.isString(responseText) && responseText.indexOf("<!DOCTYPE") >= 0 && responseText.indexOf("LoginForm") > 0) {
					console.debug("Reloading page " + window.location);
					var path = ioargs.url;
					var domain = window.location.hostname;
				    dojo.cookie("WASPostParam", null, {expires: -1, path: path, domain: domain}); // clear the WASPostParam cookie if it exists
				    if (!this.reloading) {
					    this.reloading = true;
						window.location.reload(true);
				    }
					return {reloading: true};
				}
			} else if (ioargs.xhr.status == 415 && ioargs.args && ioargs.args.preventCache === true) {
				// This looks like an xhrGet that Liberty tried to process as a Post because we weren't
				// able to properly delete the WASPostParam cookie after a session timeout or ltpa token expiration.
				// The cookie should be cleared, since this was a get that failed. Try publishing a page switch.
				// If that doesn't work, try reloading the page again.
				return {retry: true};
			} else {
				console.debug("checkXhrError:");
				console.dir(ioargs);				
			}
			return {reloading: this.reloading};
		},
		
		/**
		 * Promises only provide an error object, not ioargs. Observable JsonRest stores only deal
		 * in promises. Others can deal in deferreds, which provides better error access.
		 */
		checkError: function(/* error arg from an errback*/ error, /* if available, deferred */ deferred) {
			if (deferred && deferred.ioArgs) {
				return Utils.checkXhrError(deferred.ioArgs);
			}
			if (error) {
				console.debug("CheckError error.name is " + error.name);
				console.debug("CheckError error.message is " + error.message);
				console.debug("CheckError error.stack is " + error.stack);
			} else {
				return {reloading: this.reloading};
			}
			if (error.message == "syntax error") {
				var stack = error.stack;
				if (dojo.isString(stack) && stack.indexOf("<!DOCTYPE") >= 0 && stack.indexOf("LoginForm") > 0) {
					this.reloading = true;
					window.location.reload(true);
				} else {
					console.debug("checkError:");
					console.dir(error);					
				}
			} else if (error.message == "Unexpected token <") {
				this.reloading = true;
				window.location.reload(true);				
			} else {
				console.debug("checkError:");
				console.dir(error);
			}
			return {reloading: this.reloading};
		},
		
		/**
		 * Inspect the ioargs to see if there is a code in the response. If found,
		 * return it, otherwise returns null.
		 */
		getResultCode: function(ioargs) {
			var code = null;
			var response = undefined;
			if (ioargs && ioargs.xhr) {	
				if (ioargs.xhr.response) {
					response = JSON.parse(ioargs.xhr.response);
				} else if (ioargs.xhr.responseText) {
					response = JSON.parse(ioargs.xhr.responseText);
				}
				if (response) {
					code = response.code || null;
				}
			}
			return code;			
		},

		showPageError : function(title, error, ioargs) {
			var response = null;
			var message = null;
			var code = null;			
			if (ioargs && ioargs.xhr) {	
				if (ioargs.xhr.response) {
					response = JSON.parse(ioargs.xhr.response);
				} else if (ioargs.xhr.responseText) {
					response = JSON.parse(ioargs.xhr.responseText);
				}
				if (response) {
					message = response.message || null;
					if (message !== null) {
						message = message.replace(/\n/g, "<br />"); // replace newline with break
					}
					code = response.code || null;
				}
			}
			dojo.publish(Utils.errorhandling.errorMessageTopic, { title: title, message: message, code: code });
			return code; // return the extracted code
		},
		
		checkForCertificateError: function(error, admEndpoint, addServerError) {
		    var message = null;
            if (this.getStatusFromPromise(error) === 0) {
                var adminEndpoint;
                if (admEndpoint) {
                	adminEndpoint = admEndpoint;
                } else {
                	adminEndpoint = ism.user.getAdminEndpoint();
                }
                if (adminEndpoint) {
                    // good chance we have a certificate error
                    if (error.response) console.debug(JSON.stringify(error.response.xhr));
                    message = adminEndpoint.getCertificateInvalidMessage(addServerError);
                }
            }	
            return message;
		},
		
		showPageErrorFromPromise : function(title, error, admEndpoint, addServerError) {
		    // is it a certificate issue?
		    var code = undefined;
		    var sticky = false;
		    var message = this.checkForCertificateError(error, admEndpoint, addServerError);
		    if (message === null) {
	            message = this.getErrorMessageFromPromise(error);
	            code = this.getErrorCodeFromPromise(error);
		    } else {
	            sticky = true; // if its a certificate error, don't clear the message until resolved
		    }
            dojo.publish(Utils.errorhandling.errorMessageTopic, { title: title, message: message, code: code, sticky: sticky });
            return code; // return the extracted code		    
		},
		
		getErrorMessageFromPromise: function(error) {
		    if (!error) return null;		    
		    var message = null;
		    if (error.response && error.response.data && error.response.data.Message) {
		        message = error.response.data.Message;
		    } else if (error.message) {
		        message = error.message;
		    }
		    return message;
		},

		getErrorCodeFromPromise: function(error) {
		    if (!error) return null;
		    var code = null;
		    if (error.response && error.response.data && error.response.data.Code) {
		        code = error.response.data.Code;
		    } 
		    return code;
		},

		getStatusFromPromise: function(error) {
		    if (error && error.response && error.response.xhr) {
		        return error.response.xhr.status;
		    }
		    return undefined;
		},
		
		/**
		 * Check to see if the error response really meant that none of the requested objects were found
		 */
		noObjectsFound: function(error) {
		    // check to see if it is a 404 with an error code of 113, which really means that there aren't any.
		    return this.getStatusFromPromise(error) == 404 && this.getErrorCodeFromPromise(error) == "CWLNA0113";
		},
		
		/**
		 * Returns true if value is defined and true or value.toLowerCase() == "true". If value is 
		 * not defined and a defaultValue is provided, returns the defaultValue.
		 */
		isTrue: function(value, defaultValue) {
		    var isTrue = false;
		    if (value !== undefined && value !== null) {
		        if (value === true || value === false) {
		            isTrue = value;
		        } else if (value.toLocaleLowerCase() == "true") {
		            isTrue = true;
		        }
		    } else if (defaultValue !== undefined && defaultValue !== null) {
		        isTrue = defaultValue;
		    }
		    return isTrue;
		},
		
		/**
		 * For each property in propertiesToConvert, if the property is in data, sets it to a 
		 * boolean value by calling isTrue() on it.
		 */
		convertStringsToBooleans: function(/*Object*/data, /*Object*/propertiesToConvert) {
		    if (!data || !propertiesToConvert) {
		        return;
		    }
		    for (var i = 0, len = propertiesToConvert.length; i < len; i++) {
		        var prop = propertiesToConvert[i].property;            		        
		        data[prop] = this.isTrue(data[prop], propertiesToConvert[i].defaultValue);		       
		    }
		},
		
        /**
         * For each property in propertiesToConvert, if the property is in data, sets it to an 
         * integer value if it's currently a string.
         */
        convertStringsToIntegers: function(/*Object*/data, /*Object*/propertiesToConvert) {
            if (!data || !propertiesToConvert) {
                return;
            }
            for (var i = 0, len = propertiesToConvert.length; i < len; i++) {
                var prop = propertiesToConvert[i].property;
                var value = data[prop];
                if (typeof value === "string") {
                    data[prop] = parseInt(value, 10);
                }
            }
        },

		
		showPageWarning : function(title, message, code) {
			dojo.publish(Utils.errorhandling.warningMessageTopic, { title: title, message: message, code: code });
		},	
		
		showPageWarningXhr: function(title, ioargs) {
			var message = null;
			var code = null;			
			if (ioargs) {
				var response;
				if (ioargs.xhr.response) {
					response = JSON.parse(ioargs.xhr.response);
				} else if (ioargs.xhr.responseText) {
					response = JSON.parse(ioargs.xhr.responseText);
				}
				if (response) {
					message = response.message || null;
					code = response.code || null;
				}
			}
			dojo.publish(Utils.errorhandling.warningMessageTopic, { title: title, message: message, code: code });			
		},
		
		
		showPageInfo : function(title, message, code) {
			dojo.publish(Utils.errorhandling.infoMessageTopic, { title: title, message: message, code: code });
		},	
				
		/**
		 * Decorator used to display names that can contain '<', '>', or space characters in grids
		 */
		nameDecorator: function(name) {
			if (name) {
				return name.replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/ /g,"&nbsp;");
			}
			return name;
		},
		
		/**
		 * Text decorator used to display text that can contain '<' or '>'
		 */
		textDecorator: function(name) {
			if (name) {
				return name.replace(/</g,"&lt;").replace(/>/g,"&gt;");
			}
			return name;
		},
				
		/**
		 * "flashes" a DOM element, using the CSS opacity property
		 *
		 * id:          DOM id of the element to flash (required)
		 * args: {
				duration:    duration of animation, in millis (optional)
				beforeBegin: function to execute synchronously before the animation (optional)
				onEnd:       function to execute synchronously after the animation finishes (optional)
		   }
		 * 
		 * 
		 */
		flashElement: function(id, args) { 
			var duration = 500;
			if (args && args.duration) { duration = args.duration; } 

			var props = {
				node: id,
				duration: duration,
				properties: {
					opacity: { start: "0", end: "1" }
				}
			};

			if (args && args.beforeBegin) { props.beforeBegin = args.beforeBegin; }
			if (args && args.onEnd) { props.onEnd = args.onEnd; }

			fx.animateProperty(props).play();
		},
		
		/*
		 *  If the specified widget contains an element with class 'idxRequiredIcon',
		 *  clear the element innerHTML and replace the class with 'alignWithRequired'
		 */
		alignWidgetWithRequired: function(widget) {
			if (widget == undefined) {
				return;
			}
			var requiredIconSpan = dojo.query(".idxRequiredIcon", "widget_"+widget.id);
			if (requiredIconSpan.length > 0) {
				requiredIconSpan[0].innerHTML = "";
				domClass.replace(requiredIconSpan[0], "alignWithRequired", "idxRequiredIcon");
			}
		},
		
		getUrlVars: function() {
		    var vars = {};
		    var parts = window.location.href.replace(/[?&]+([^=&]+)=([^&]*)/gi, function(m, key, value) {
		        vars[key] = value;
		    });
		    return vars;        
		}
	};

	return Utils;
});

