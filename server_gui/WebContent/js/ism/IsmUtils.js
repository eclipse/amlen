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

define(["dojo/_base/array", 
	    "dojo/_base/lang", 
	    "dojo/date/locale",
		'dojo/query',
		'dojo/dom-class',
		'dojo/number',
		'dojo/has',
		'dojo/promise/all',
		'dojo/when',
		'ism/widgets/IsmQueryEngine',
	    "dojo/i18n!ism/nls/strings",
	    "ism/config/Resources",
	    'ism/Utils',
	    'idx/string'
	    ], function(array, lang, locale, query, domClass, number, has, all, when, IsmQueryEngine, 
	   	   nls, Resources, Utils, iString) {
	
	/**
	 * This object is mixed in with Utils to reduce the amount of code change generated when
	 * splitting the more generic utilities from the more ISM specific ones.
	 */

	var IsmUtils = {       
		lastServerName: null,
		configObjName: {
			label: nls.name.label,
			tooltip: nls.name.tooltip,

			/**
			 * Validates a config object name.  Returns an object
			 * containing the result and an error message if validation
			 * fails
			 * parameters:  
			 *   name: The name to validate
			 *   existingNames: If provided, verifies name is not in existingNames
			 * returns: 
			 *   {result: true, message: ""} or 
			 *   {result: false, message: "error message to display"}
			 */
			validate: function(name, existingNames, focused, noSpacesAllowed, unicodeAlphanumericOnly) {
				var result = {valid: true, message: ""};

				if (!name || name === ""){
					result.valid = false;
					result.message = nls.global.missingRequiredMessage;
					result.reason = "required";
				} else if (existingNames && dojo.indexOf(existingNames, name) >= 0) {
					result.valid = false;
					result.message = nls.name.duplicateName;
					result.reason = "duplicate";
				} else if ((noSpacesAllowed)&&((name.indexOf(" ")>-1))) {
					result.valid = false;
					result.message = nls.name.noSpaces;
					result.reason = "spaces";
				} else if (!(new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$")).test(name)) {
					result.valid = false;
					result.message = nls.name.invalidSpaces;
					result.reason = "spaces";
				} else if (unicodeAlphanumericOnly) {
					// We cannot completely restrict to unicode alphanumeric characters in javascript.
					// Do what we can reasonably, then leave it to the REST API which has access to
					// Java's regex...				
					
					// Must not start with a digit
					if (new RegExp("[0-9]").test(name[0])) {
						result.valid = false;
						result.message = nls.name.unicodeAlphanumericOnly.invalidFirstChar;
						result.reason = "firstChar";
					// Check for some obvious invalid character ranges	
					} else if (new RegExp("^(.*[\u0000-\u002F\u003A-\u0040\u005B-\u0060\u007B-\u00A7].*)$").test(name)) {
						result.valid = false;
						result.message = nls.name.unicodeAlphanumericOnly.invalidChar;
						result.reason = "invalidChar";							
					}
					// could still be invalid, but will need to let Java help us on the server...
				} else if (new RegExp("^[\u0000-\u0040].*$").test(name)) {
					result.valid = false;
					result.message = nls.name.invalidFirstChar;
					result.reason = "firstChar";
				} else if (new RegExp("^(.*[\u0000-\u001F,=\\u0022\\u005C].*)$").test(name)) {
					result.valid = false;
					result.message = nls.name.invalidChar;
					result.reason = "invalidChar";
				}
				
				return result;
			},
			
			/**
			 * If edit of the config object name should be disabled,
			 * returns true, otherwise, return false
			 */
			editEnabled: function() {
				// for the Beta, always return false
				return false;
			}
		},
		
        portValidator: function(value, required) {
            if (value === null || value === "") {
                return !required;
            }
            var intValue = number.parse(value, {places: 0});
            if (isNaN(intValue) || intValue < 1 || intValue > 65535) {
                this.invalidMessage = nls.portInvalid;
                return false;
            }
            return true;                
        },

		
		/**
		 * Decorator used to display boolean value
		 */
		booleanDecorator: function(value) {
			if (value === undefined || value === false || value.toString().toLowerCase() != "true") {
				return nls.global.falseValue;
			}
			return nls.global.trueValue;		
		},
		
		/**
		 * Decorator used to display a numeric value formatted in the user's locale
		 * formatted to 0 places
		 */
		integerDecorator: function(value) {
			if (value === undefined) {
				return "";
			}
			return "<div style='float: right'>"+number.format(value, {places: 0, locale: ism.user.localeInfo.localeString})+"</div>";
		},

		/**
		 * Decorator used to display a numeric value formatted in the user's locale
		 * formatted to 2 places
		 */
		decimalDecorator: function(value) {
			if (value === undefined) {
				return "";
			}
			return "<div style='float: right'>"+number.format(value, {places: 2, locale: ism.user.localeInfo.localeString})+"</div>";
		},
		
		escapeApostrophe: function(value) {
            if (value) {
                return value.replace(/'/g,"&apos;");
            }
            return value;            
		},
		
		escapePsuedoTranslation: function(value) {
		    // when using a psuedoTranslated string in a regular expression, escape the leading '[' if it causes matching to fail
		    if (value && iString.startsWith(value,"[")) {
		        return "\\\\" + value;
		    }
		    return value;
		},
		
		/**
		 * Checks the int statusRC from a call to rest/config/appliance/status/ismserver
		 * to determine if the server is in maintenance mode
		 */
		isMaintenanceMode: function (statusRC) {
			if (statusRC == 9 || this.isStartingInProgress(statusRC)) {
				return true;
			}
			return false;
		},
		
		/**
		 * Check the int statusRC rom a call to rest/config/appliance/status/ismserver
		 * to determine if the server is in cleanStoreInProgress mode
		 */
		 isStartingInProgress: function(statusRC) {
			if (statusRC === 11 || statusRC === 12) {
				return true;
			} 
			return false;
		 },

		/**
		 * Checks the int statusRC from a call to rest/config/appliance/status/ismserver
		 * to determine if the server is in warning mode, i.e. the server not in 'Running'
		 * mode and is not in ErrorMode.
		 */
		isWarnMode: function (statusRC) {			
			if (statusRC == 1 || IsmUtils.isErrorMode(statusRC)) {
				return false;
			}
			return true;
		},
		
		/**
		 * Checks the int statusRC from a call to rest/config/appliance/status/ismserver
		 * to determine if the server is in error mode, i.e. the server is either down,
		 * or in some state such that normal operations are not possible
		 */
		isErrorMode: function (statusRC) {
			if (statusRC >= 99 || statusRC < 0) {
				return true;
			}
			return false;
		},
		
		/**
		 * Check statusRC to see if it is equivalent to 99 or "99", and if so
		 * returns true.
		 */
		isStopped: function (statusRC) {
			if (statusRC === 99 || statusRC === "99") {
				return true;
			}
			return false;
		},

	    /**
         * Check statusRC to see if it is stopping, and if so
         * returns true.
         */
        isStopping: function (statusRC) {
            if (statusRC === 2 || statusRC === "2") {
                return true;
            }
            return false;
        },
        
	    /**
         * Check statusRC to see if it is running, and if so
         * returns true.
         */
        isRunning: function (statusRC) {
            if (statusRC === 1 || statusRC === 9
            		|| statusRC === "1" || statusRC === "9") {
                return true;
            }
            return false;
        },

		/**
		 * Checks the HA Mode to determine if a warning should be shown
		 */
		isHAWarnMode: function (haMode) {
			if (haMode == "UNSYNC") {
				return true;
			}
			return false;
		},
		
		/**
		 * Checks the HA NewRole to determine if an error should be shown
		 */
		isHAErrorMode: function (haRole) {
			if (haRole == "UNSYNC_ERROR") {
				return true;
			}
			return false;
		},
		

		/**
		 * For the specified item, determine if it should
		 * be disabled based on the server status (rc) and HA mode (hamode) in 
		 * data.  The item is expected to have availability and ha properties
		 * if it needs to be disabled based on either server availability or ha role.
		 */
		isItemDisabled: function(item, data) {
		    // Disable all pages if the license for the target server
		    // is not accepted.
			var pendingLicenseAccept = (data.Server && data.Server.ErrorCode && data.Server.ErrorCode === 387);
			if (pendingLicenseAccept) {
				return {disabled: true, message: data.Server.ErrorMessage, collapsed: false};
			}
			if (item.availability && data.RC !== undefined && array.indexOf(item.availability,data.RC) === -1) {
				return {disabled: true, message: nls.global.pageNotAvailableServerDetail}; 
			}
			if (item.cluster && data.Cluster !== undefined && array.indexOf(item.cluster, data.Cluster.Status) === -1) {
				return {disabled: true, message: nls.global.pageNotAvailableServerDetail}; 
			}
			// if Enabled == false or we're in maintenance mode, ignore the harole
			// modified to handle both old and new status response payloads
			var haEnabled = data.HighAvailability ? data.HighAvailability.Enabled === true : false; 
			if (IsmUtils.isMaintenanceMode(data.RC) || haEnabled === false) {
				return {disabled: false};
			}
			if 	(item.ha && data.harole && array.indexOf(item.ha,data.harole) === -1){
				return {disabled: true, message: nls.global.pageNotAvailableHAroleDetail};
			}
			return {disabled: false};
		},
		
		/**
		 * Returns query parameters for a GET request.
		 * Use this function to construct the query parameter string for any GET request made to the Admin Endpoint.
		 */
		getQueryParams: function(data) {
			var i = 0;
			// If this function is called, we will assume there is at least one query parameter so the "?" is needed.
			var qstring = "?";
			for (var prop in data) {
				if (i !== 0) {
					qstring += "&"+prop+"="+encodeURIComponent(data[prop]);
				} else {
					qstring += prop+"="+encodeURIComponent(data[prop]);
				}
				i++;
			}
			console.debug("getQueryParams returning "+qstring);
			return qstring;
		},

		/**
		 * Returns formatted date/time string in the configured locale (from dojoConfig)
		 *
		 * Example:   
		 */
		getFormattedDateTime: function(date) {
			if (!date) {
				date = new Date();
			}
			
			return locale.format(date, { selector: "date", datePattern:  ism.user.localeInfo.dateTimeFormatString} );
		},
		
		/**
		 * Returns formatted time string in the configured locale (from dojoConfig)
		 *
		 * Example:   
		 */
		getFormattedTime: function(date, includeSeconds) {
			if (!date) {
				date = new Date();
			}
			
			var options = includeSeconds ? { selector: "time", timePattern:  ism.user.localeInfo.timeFormatString } : { selector: "time", datePattern:  ism.user.localeInfo.timeFormatString };
			
			return locale.format(date, options );
		},
		
		getHtmlFormattedTimestamp: function(date) {
			if (!date) {
				date = new Date();
			}
			
			var formattedDate = locale.format(date, { selector: "date", datePattern:  ism.user.localeInfo.dateFormatString} );
			var formattedTime = this.getFormattedTime(date);
			// IE9 and 10 don't handle markup correctly so don't include it
			if (has("ie") <= 10) {
				return formattedDate + " " + formattedTime;
			}
			return "<span>" + formattedDate + "<p>" + formattedTime + "</p></span>";			
		},
		
		/**
		 * Initiates a status request, returning the promise that represents the request.
		 * @param adminEndpoint Optional adminEndpoint to get status for. Defaults to current adminEndpoint.
		 * @returns promise
		 */
		getStatus: function(/*Object*/ adminEndpoint) {
		    adminEndpoint = adminEndpoint ? adminEndpoint : ism.user.getAdminEndpoint();
            return adminEndpoint.doRequest("/service/status", adminEndpoint.getDefaultGetOptions());
		},

		/**
		 * Initiates a cluster monitor request, returning the promise that represents the request.
		 * @param adminEndpoint Optional adminEndpoint to get status for. Defaults to current adminEndpoint.
		 * @returns promise
		 */
		getClusterStatus: function(/*Object*/ adminEndpoint) {
		    adminEndpoint = adminEndpoint ? adminEndpoint : ism.user.getAdminEndpoint();
            return adminEndpoint.doRequest("/monitor/Cluster", adminEndpoint.getDefaultGetOptions());
		},
		
		/*
		 * Gets the enabled setting for MQConnectivity.
		 */
		getMQConnEnabled: function(/*Object*/ adminEndpoint) {
		    adminEndpoint = adminEndpoint ? adminEndpoint : ism.user.getAdminEndpoint();
            return adminEndpoint.doRequest("/configuration/MQConnectivityEnabled", adminEndpoint.getDefaultGetOptions());
		},
		
		/*
		 * Sets MQConnectivityEnabled.
		 */
		setMQConnectivityEnabled: function(enabled, /*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({MQConnectivityEnabled: enabled});
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},
		
		/*
		 * Starts MQConnectivity.  This function is only accessible when MQConnectivity is enabled.
		 */
		startMQConnectivity: function(/*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({Service: "MQConnectivity"});
            var promise = adminEndpoint.doRequest("/service/start", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},

		/*
		 * Retarts MQConnectivity.  This function is only accessible when MQConnectivity is enabled.
		 */
		restartMQConnectivity: function(/*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({Service: "MQConnectivity"});
            var promise = adminEndpoint.doRequest("/service/restart", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},
		
		/*
		 * Stops MQConnectivity.  This function is only accessible when MQConnectivity is enabled.
		 */
		stopMQConnectivity: function(/*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({Service: "MQConnectivity"});
            var promise = adminEndpoint.doRequest("/service/stop", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},
		/**
		 * Query the status of the SNMP Service
		 */
		getSNMPStatus: function(onComplete, onError) {
			
			IsmUtils.updateStatus(
					function(data) {
						if (onComplete) {
							onComplete(data);
						}
						dojo.publish(Resources.snmpStatus.statusChangedTopic, data);	
					},
					function(error) {
						if (onError) {
							onError(error);
						}
				    	dojo.publish(Resources.snmpStatus.statusChangedTopic, { Status:  "UNKNOWN" });	
					});
		},		/*
		 * Gets the enabled setting for SNMP.
		 */
		getSNMPEnabled: function(/*Object*/ adminEndpoint) {
		    adminEndpoint = adminEndpoint ? adminEndpoint : ism.user.getAdminEndpoint();
            return adminEndpoint.doRequest("/configuration/SNMPEnabled", adminEndpoint.getDefaultGetOptions());
		},
		
		/*
		 * Sets SNMPEnabled.
		 */
		setSNMPEnabled: function(enabled, /*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({SNMPEnabled: enabled});
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},
		
		/*
		 * Starts SNMP.  This function is only accessible when SNMP is enabled.
		 */
		startSNMP: function(/*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({Service: "SNMP"});
            var promise = adminEndpoint.doRequest("/service/start", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},

		/*
		 * Stops SNMP.  This function is only accessible when SNMP is enabled.
		 */
		stopSNMP: function(/*Function*/onComplete, /*Function*/ onError) {
		    var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify({Service: "SNMP"});
            var promise = adminEndpoint.doRequest("/service/stop", options);
            promise.then(function() {
            	IsmUtils.updateStatus();
            	if (onComplete)
            		onComplete();
            }, function(error) {
                if (onError) 
                	onError(error);
            });
		},
		
		/**
		 * Adds harole, cluster and RC to the data object.  harole is the data.HighAvailability.NewRole adjusted
		 * for synchronization.  cluster is data.Cluster.Name. RC is the data.Server.State.
		 * @param data The result of a status request to the AdminEndpoint. This is modified as described above. 
		 */
		supplementStatus: function(data) {
		    if (!data) {
		        return;
		    }
            data.harole = data && data.HighAvailability  && data.HighAvailability.Enabled === false ? "HADISABLED" : data.HighAvailability.Enabled === true && data.HighAvailability.NewRole ? data.HighAvailability.NewRole : "UNKNOWN";
            if (data.harole === "PRIMARY" && data.HighAvailability.PctSyncCompletion > 0) {
                data.harole = "PRIMARY_SYNC";
            }

            var status = data && data.Server ? data.Server : {};
            data.RC = status.State === undefined ? -1 : status.State;		    
            
            var dateTime = data.Server && data.Server.ServerTime ? data.Server.ServerTime : undefined;
            if (dateTime) {
            	data.dateTime = dateTime;
            }
		},
		
		/**
		 * Get the status, supplement it, and publish it on the adminEndpointStatusChangedTopic
		 * @param onComplete The optional handler to call when the request is complete
		 * @param onError The optional handler to call when the request errors out 
		 */
		updateStatus: function(/*function*/onComplete, /*function*/onError) {
            var promise = this.getStatus();
            var _this = this;
            var unavailable = {RC: -1, harole: "UNKNOWN"};
            promise.then(
                function(data) {
                    if (!data) {
                        data = unavailable;
                    } else {
                    	if (data.Server && data.Server.Name) {
                    		if (_this.lastServerName !== data.Server.Name) {
                    			_this.lastServerName = data.Server.Name;
                    			dojo.publish(Resources.contextChange.nodeNameTopic, data.Server.Name);
                    		}
                    	}
                        _this.supplementStatus(data);

                        dojo.publish(Resources.serverStatus.adminEndpointStatusChangedTopic, data);
                        if (onComplete) {
                        	onComplete(data);
                        }
                    }
            }, function(error) {
                dojo.publish(Resources.serverStatus.adminEndpointStatusChangedTopic, unavailable);
                if (onError) {
                    onError(error);
                }
            });		    
		},
		
		/**
		 * Get the cluster status and publish it on the clusterStatusChangedTopic
		 * @param onComplete The optional handler to call when the request is complete
		 * @param onError The optional handler to call when the request errors out 
		 */
		updateClusterStatus: function(/*function*/onComplete, /*function*/onError) {
            var promise = this.getClusterStatus();
            var _this = this;
            promise.then(
                function(data) {
                    if (!data) {
                        console.log("No cluster data available");
                    } else {
                        dojo.publish(Resources.serverStatus.clusterStatusChangedTopic, data);  
                    }
                    if (onComplete) {
                        onComplete(data);
                    }
            }, function(error) {
                console.log("Error on request for cluster data");
                if (onError) {
                    onError(error);
                }
            });		    
		},
		
		/**
		 * Query the status of the MQ Connectivity Service
		 */
		getMQConnectivityStatus: function(onComplete, onError) {
			
			IsmUtils.updateStatus(
					function(data) {
						if (onComplete) {
							onComplete(data);
						}
						dojo.publish(Resources.mqConnectivityStatus.statusChangedTopic, data);	
					},
					function(error) {
						if (onError) {
							onError(error);
						}
				    	dojo.publish(Resources.mqConnectivityStatus.statusChangedTopic, { Status:  "UNKNOWN" });	
					});
		},
		
		/*
		 * If the data object shows the harole is PRIMARY and the PctSyncCompletion is > -1,
		 * then change the harole to PRIMARY_SYNC
		 */
		adjustForSync: function(data) {
			if (data && data.HighAvailability && data.HighAvailability.NewRole === "PRIMARY" && data.HighAvailability.PctSyncCompletion > 0) {
				data.harole = "PRIMARY_SYNC";
			}
		},
		
		/*
		 * Determing if the harole represents a primary node that is synchronizing with the standby
		 */
		isPrimarySynchronizing: function(harole) {
			return harole === "PRIMARY_SYNC";
		},
		
		setProtocolCapabilities: function(protocol) {
			protocol.Topic = protocol.UseTopic;
			protocol.Shared = protocol.UseShared;
			protocol.Queue = protocol.UseQueue;
			protocol.Browse = protocol.UseBrowse;
		},

		/**
		 * Create a store from protocol data returned from the server
		 */
		getProtocolStore: function(data) {
			
			var items = [];
            // check to see if its from the new API
            var adminEndpoint = ism.user.getAdminEndpoint();
            var protocols = data && data.Protocol ? adminEndpoint.getObjectsAsArray(data, "Protocol") : undefined;
            var len = protocols ? protocols.length : 0;
            // check if All label needed
            for (var i=0; i < len; i++) {
                var value = (protocols[i].Name);
                // backward compatible - we need to keep value as upper case
                value = (value === "jms" || value === "mqtt") ? value.toUpperCase() : value;
                protocols[i].value = value;
                protocols[i].label = value;
                protocols[i].Name = value;
                protocols[i].id = value;
                IsmUtils.setProtocolCapabilities(protocols[i]);
                // for use with ReferencedObjectGrid
                protocols[i].associated = {};
                items[i] = protocols[i];                
            }
			// add IsmQueryEngine so we get case insensitive sort
			var store = new dojo.store.Memory({data:items, idProperty: "Name", queryEngine: IsmQueryEngine});
			return store;
		},
		
		/*
		 * Retrieve the messaging policies associated with an endpoint.
		 */
		getMessagingPolicies: function(ep) {
			var mpName = [];
			var mpId = [];
			
			var tpolicies = (ep.TopicPolicies === undefined || ep.TopicPolicies === null) ? "" : ep.TopicPolicies;
			var spolicies = (ep.SubscriptionPolicies === undefined || ep.SubscriptionPolicies === null) ? "" : ep.SubscriptionPolicies;
			var qpolicies = (ep.QueuePolicies === undefined || ep.QueuePolicies === null) ? "" : ep.QueuePolicies;
			
			var tcount = (tpolicies === "") ? 0 : tpolicies.split(",").length;
			var scount = (spolicies === "") ? 0 : spolicies.split(",").length;
			var qcount = (qpolicies === "") ? 0 : qpolicies.split(",").length;
			
			tpolicies = tpolicies.split(",");
			spolicies = spolicies.split(",");
			qpolicies = qpolicies.split(",");

			for (var i = 0; i < tcount; i++) {
				mpName[i] = tpolicies[i];
				mpId[i] = tpolicies[i]+"Topic";
			}
			for (i = 0; i < scount; i++) {
				mpName[tcount+i] = spolicies[i];
				mpId[tcount+i] = spolicies[i]+"Subscription";
			}
			for (i = 0; i < qcount; i++) {
				mpName[tcount+scount+i] = qpolicies[i];
				mpId[tcount+scount+i] = qpolicies[i]+"Queue";
			}
			
			var mpList = {
				count: tcount + scount + qcount,
			    Name: mpName,
			    Id: mpId
			};
			
			return mpList;
		},
		
		getSecurityProfilesUsingResource: function(resource, onComplete, onError) {			
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/SecurityProfile", adminEndpoint.getDefaultGetOptions());
            promise.then(
                function(data) {
                    var result = [];
                    var property = resource.property;
                    var value = resource.value;
                    var profiles = adminEndpoint.getObjectsAsArray(data, "SecurityProfile");
                    dojo.forEach(profiles, function(item, index) {
                        if (item[property] == value) {
                            result.push(item.Name);
                        }
                    });
                    if (onComplete) {
                        onComplete(result);
                    }
                }, onError);          
		},
		
		getServerName: function(onComplete, onError) {
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/ServerName", adminEndpoint.getDefaultGetOptions());
            promise.then(
                function(data) {
                	if (onComplete) {
                		onComplete(data);
                	}
                },
                function(error) {
                	if(onError) {
                		onError(error);
                	}
                });
		},
		
		setServerName: function(servername, onComplete, onError) {
            var adminEndpoint = ism.user.getAdminEndpoint();
            var values = {};
            values["ServerName"] = servername;
            var options = adminEndpoint.getDefaultPostOptions();
            options.data = JSON.stringify(values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(onComplete, onError);
		},
		
		getLicensedUsage: function(onComplete, onError) {
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/LicensedUsage", adminEndpoint.getDefaultGetOptions());
            var result = undefined;
            promise.then(
                function(data) {
                	if (onComplete) {
                		onComplete(data);
                	}
                },
                function(error) {
                	if(onError) {
                		onError(error);
                	}
                });
		},
		
		acceptLicense: function(message) {
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultPostOptions();
			options.data = JSON.stringify(message);
			var promise = adminEndpoint.doRequest("/configuration", options);
			return promise;
		},
		
		getLicenseUrl: function() {
            var adminEndpoint = ism.user.getAdminEndpoint();
            return adminEndpoint.licenseURL;
		},

		showPageErrorIfUnique: function(title, error, ioargs) {
            // If the same error is already displayed, don't display another
		    var duplicateFound = false;
		    var messageId = Utils.getResultCode(ioargs);
		    messageId = messageId ? messageId : "";
            query(".errorMessage","mainContent").forEach(function(node) {
                var singleMessage = dijit.byId(node.id);
                if (singleMessage.get("title") === title && singleMessage.get("messageId") === messageId) {
                    // we already have one shown
                    duplicateFound = true;
                }
            });
            if (duplicateFound) {
                console.log("showPageErrorIfUnique returning because a duplicate error is already displayed"); 
                return;
            }
            Utils.showPageError(title, error, ioargs);
		},
		
		publishPageErrorIfUnique: function(title, messageId, message) {
            // If the same error is already displayed, don't display another
            var duplicateFound = false;
            messageId = messageId ? messageId : "";
            message = message ? message : "";
            query(".errorMessage","mainContent").forEach(function(node) {
                var singleMessage = dijit.byId(node.id);
                if (singleMessage.get("title") === title && 
                        singleMessage.get("messageId") === messageId && 
                        singleMessage.get("description") === message) {
                    // we already have one shown
                    duplicateFound = true;
                }
            });
            if (duplicateFound) {
                console.log("publishPageErrorIfUnique returning because a duplicate error is already displayed"); 
                return;
            }
            dojo.publish(Resources.errorhandling.errorMessageTopic, { title: title, message: message, code: messageId});		    
		},
		
		/**
		 * Simple restart of messaging server.
		 */
		restartMessagingServer: function(waitForRestart, onComplete, onError) {
			var values = {};
			values.Service = "Server";
			IsmUtils.restartRemoteServer(waitForRestart, values, onComplete, onError);
		},
		
		/**
		 * Uses the new restart command to restart a server, including one that is remote.
		 * If waitForRestart is true, does not call onComplete until the server is starting
		 * or started.
		 */
		restartRemoteServer: function(waitForRestart, values, onComplete, onError) {  
            var domainUid = ism.user.getDomainUid();
            var _this = this;
            
		    // restart the server
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultPostOptions();
			options.data = JSON.stringify(values);
			var promise = adminEndpoint.doRequest("/service/restart", options);
			promise.then(function(data) {
				if(waitForRestart) {
				    _this.waitForRestart(5, onComplete);
				} else if (onComplete) {
				    onComplete();
				}
			},
			function(error) {
				if (onError) {
					onError(error);
				}
			});
		},
		
		stopRemoteServer: function(values, onComplete, onError) {
            var _this = this;
            
		    // stop the server
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.getDefaultPostOptions();
			options.data = JSON.stringify(values);
			var promise = adminEndpoint.doRequest("/service/stop", options);
			promise.then(function(data) {
			    onComplete();
			},
			function(error) {
				if (onError) {
					onError();
				}
			});
		},
 
		/**
		 * Waits for the server to report that is is starting or started, then
		 * calls onComplete.
		 */
		waitForRestart: function(timeout, onComplete, isRetry) {
		    var _this = this;
		    var initialTimeout = 5000;
		    if(!isRetry) {
			    // Wait 5s before the first attempt to update the status.  Otherwise, 
			    // we can get a false positive that the server "running again" when it has not 
			    // even begun the restart process.
		    	initialTimeout = timeout * 1000;
		    }

		    setTimeout(function() {
            _this.updateStatus(
                function(data) {
                    if (_this.isErrorMode(data.RC) || _this.isStopping(data.RC) || (!_this.isRunning(data.RC) && !_this.isMaintenanceMode(data.RC))) { 
                        // wait 2 seconds more and try again
                        setTimeout(function(){
                            _this.waitForRestart(2, onComplete, true);
                        }, 2000);                   
                    } else {
                        onComplete();
                    }
                },
                function(error) {
                    // wait 2 seconds more and try again
                    setTimeout(function(){
                        _this.waitForRestart(2, onComplete);
                    }, 2000);                   
                });
		    }, initialTimeout);
        }
	};

	lang.mixin(IsmUtils, Utils);
	
	return IsmUtils;
});

