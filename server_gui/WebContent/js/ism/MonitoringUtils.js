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
	    'dojo/DeferredList',
		'dojo/query',
		'dojo/number',
	    "dojo/i18n!ism/nls/monitoring",
	    "dojo/i18n!ism/nls/home",
	    "ism/config/Resources",
	    "ism/IsmUtils",
	    "ism/Utils"
	    ], function(array, lang, locale, DeferredList, query, number,  
	   	nlsMon, nlsHome, Resources, Utils, _Utils) {

	var _getBaseUri = function() {
		var baseUri = location.pathname.split("/").slice(0,1).join("/");
		if (baseUri.length === 0 || baseUri[baseUri.length-1] !== "/") {
			baseUri += "/"; 
		}
		return baseUri;
	};
	
	var MonitoringUtils = {
		nlsMon: nlsMon,
		
		    domainUid: undefined,
		
		    getDomainUid: function() {
		        if (this.domainUid === undefined) {
		            this.domainUid = ism.user.getDomainUid();
		        }
		        return this.domainUid;
		    },
		
			Connection: {
				Type: {
					DATA: "All",
					VOLUME: "Volume"
				},
				Category: "connection",
				View: {
					CONNTIME_WORST: {
						name: "NewestConnection",
						label: nlsMon.monitoring.views.conntimeWorst
					},
					CONNTIME_BEST: {
						name: "OldestConnection",
						label: nlsMon.monitoring.views.conntimeBest
					},
					TPUT_MSG_WORST: {
						name: "LowestThroughputMsgs",
						label: nlsMon.monitoring.views.tputMsgWorst
					},
					TPUT_MSG_BEST: {
						name: "HighestThroughputMsgs",
						label: nlsMon.monitoring.views.tputMsgBest
					},
					TPUT_BYTES_WORST: {
						name: "LowestThroughputKB",
						label: nlsMon.monitoring.views.tputBytesWorst
					},
					TPUT_BYTES_BEST: {
						name: "HighestThroughputKB",
						label: nlsMon.monitoring.views.tputBytesBest
					}
				}
			},
			Endpoint: {
				Type: {
					CURRENT: "Current",
					HISTORY: "History"
				},
				Category: "endpoint",
				Metric: {
					ACTIVE_CONNECTIONS: {
						name: "ActiveConnections",
						label: nlsMon.monitoring.views.activeCount,
						chartLabel: nlsMon.monitoring.chartLabels.activeCount,
						integers: true
					},
					TOTAL_CONNECTIONS: {
						name: "Connections",
						label: nlsMon.monitoring.views.connectCount,
						chartLabel: nlsMon.monitoring.chartLabels.connectCount,
						integers: true
					},
					BAD_COUNT: {
						name: "BadConnections",
						label: nlsMon.monitoring.views.badCount,
						chartLabel: nlsMon.monitoring.chartLabels.badCount,
						integers: true
					},
					MSG_COUNT: {
						name: "Msgs",
						label: nlsMon.monitoring.views.msgCount,
						chartLabel: nlsMon.monitoring.chartLabels.msgCount,
						integers: false
					},
					BYTES_COUNT: {
						name: "Bytes",
						label: nlsMon.monitoring.views.bytesCount,
						chartLabel: nlsMon.monitoring.chartLabels.bytesCount,
						integers: false
					}
				}
			},
			Queue: {
				Category: "queue",
				View: {
					BUFFERED_MSGS_HIGHEST: {
						name: "BufferedMsgsHighest",
						label: nlsMon.monitoring.views.msgBufHighest
					},
					PRODUCED_MSGS_HIGHEST: {
						name: "ProducedMsgsHighest",
						label: nlsMon.monitoring.views.msgProdHighest
					},
					CONSUMED_MSGS_HIGHEST: {
						name: "ConsumedMsgsHighest",
						label: nlsMon.monitoring.views.msgConsHighest
					},
					EXPIRED_HIGHEST: {
						name: "ExpiredMsgsHighest",
						label: nlsMon.monitoring.views.ExpiredMsgsHighestQ
					},					
					NUM_PRODUCERS_HIGHEST: {
						name: "ProducersHighest",
						label: nlsMon.monitoring.views.numProdHighest
					},
					NUM_CONSUMERS_HIGHEST: {
						name: "ConsumersHighest",
						label: nlsMon.monitoring.views.numConsHighest
					},
					BUFFERED_HWM_PERCENT_HIGHEST: {
						name: "BufferedHWMPercentHighest",
						label: nlsMon.monitoring.views.BufferedHWMPercentHighestQ
					},
					BUFFERED_MSGS_LOWEST: {
						name: "BufferedMsgsLowest",
						label: nlsMon.monitoring.views.msgBufLowest
					},
					PRODUCED_MSGS_LOWEST: {
						name: "ProducedMsgsLowest",
						label: nlsMon.monitoring.views.msgProdLowest
					},
					CONSUMED_MSGS_LOWEST: {
						name: "ConsumedMsgsLowest",
						label: nlsMon.monitoring.views.msgConsLowest
					},
					EXPIRED_LOWEST: {
						name: "ExpiredMsgsLowest",
						label: nlsMon.monitoring.views.ExpiredMsgsLowestQ
					},
					NUM_PRODUCERS_LOWEST: {
						name: "ProducersLowest",
						label: nlsMon.monitoring.views.numProdLowest
					},
					NUM_CONSUMERS_LOWEST: {
						name: "ConsumersLowest",
						label: nlsMon.monitoring.views.numConsLowest
					},
					BUFFERED_HWM_PERCENT_LOWEST: {
						name: "BufferedHWMPercentLowest",
						label: nlsMon.monitoring.views.BufferedHWMPercentLowestQ
					}
				}
			},
			Rules: {
				Category: "mappingrules",
				View: {
					COMMIT_COUNT: {
						name: "CommitCount",
						label: nlsMon.monitoring.views.mostCommitOps
					},
					ROLLBACK_COUNT: {
						name: "RollbackCount",
						label: nlsMon.monitoring.views.mostRollbackOps
					},
					PERSISTENT_COUNT: {
						name: "PersistentCount",
						label: nlsMon.monitoring.views.mostPersistMsgs
					},
					NONPERSISTENT_COUNT: {
						name: "NonpersistentCount",
						label: nlsMon.monitoring.views.mostNonPersistMsgs
					},
					COMMITTED_MSG_COUNT: {
						name: "CommittedMessageCount",
						label: nlsMon.monitoring.views.mostCommitMsgs
					}					
				},
				RuleType: {
					ANY: {
						name: "Any",
						label: nlsMon.monitoring.rules.any
					},
					IMATOPIC_TO_MQQUEUE: {
						name: "IMATopicToMQQueue",
						label: nlsMon.monitoring.rules.imaTopicToMQQueue
					},
					IMATOPIC_TO_MQTOPIC: {
						name: "IMATopicToMQTopic",
						label: nlsMon.monitoring.rules.imaTopicToMQTopic
					},
					MQQUEUE_TO_IMATOPIC: {
						name: "MQQueueToIMATopic",
						label: nlsMon.monitoring.rules.mqQueueToIMATopic
					},
					MQTOPIC_TO_IMATOPIC: {
						name: "MQTopicToIMATopic",
						label: nlsMon.monitoring.rules.mqTopicToIMATopic
					},
					IMATOPICTREE_TO_MQQUEUE: {
						name: "IMATopicSubtreeToMQQueue",
						label: nlsMon.monitoring.rules.imaTopicSubtreeToMQQueue
					},
					IMATOPICTREE_TO_MQTOPIC: {
						name: "IMATopicSubtreeToMQTopic",
						label: nlsMon.monitoring.rules.imaTopicSubtreeToMQTopic
					},
					IMATOPICTREE_TO_MQTOPICTREE: {
						name: "IMATopicSubtreeToMQTopicSubtree",
						label: nlsMon.monitoring.rules.imaTopicSubtreeToMQTopicSubtree
					},
					MQTOPICTREE_TO_IMATOPIC: {
						name: "MQTopicSubtreeToIMATopic",
						label: nlsMon.monitoring.rules.mqTopicSubtreeToIMATopic
					},
					MQTOPICTREE_TO_IMATOPICTREE: {
						name: "MQTopicSubtreeToIMATopicSubtree",
						label: nlsMon.monitoring.rules.mqTopicSubtreeToIMATopicSubtree
					},
					IMAQUEUE_TO_MQQUEUE: {
						name: "IMAQueueToMQQueue",
						label: nlsMon.monitoring.rules.imaQueueToMQQueue
					},
					IMAQUEUE_TO_MQTOPIC: {
						name: "IMAQueueToMQTopic",
						label: nlsMon.monitoring.rules.imaQueueToMQTopic
					},
					MQQUEUE_TO_IMAQUEUE: {
						name: "MQQueueToIMAQueue",
						label: nlsMon.monitoring.rules.mqQueueToIMAQueue
					},
					MQTOPIC_TO_IMAQUEUE: {
						name: "MQTopicToIMAQueue",
						label: nlsMon.monitoring.rules.mqTopicToIMAQueue
					},
					MQTOPICTREE_TO_IMAQUEUE: {
						name: "MQTopicSubtreeToIMAQueue",
						label: nlsMon.monitoring.rules.mqTopicSubtreeToIMAQueue
					}
				}
			},	
			Topic: {
				Category: "topic",
				View: {
					PUBLISHED_MSGS_HIGHEST: {
						name: "PublishedMsgsHighest",
						label: nlsMon.monitoring.views.msgPubHighest
					},
					SUBSCRIPTIONS_HIGHEST: {
						name: "SubscriptionsHighest",
						label: nlsMon.monitoring.views.subsHighest
					},
					REJECTED_MSGS_HIGHEST: {
						name: "RejectedMsgsHighest",
						label: nlsMon.monitoring.views.msgRejHighest
					},
					FAILED_PUBLISHES_HIGHEST: {
						name: "FailedPublishesHighest",
						label: nlsMon.monitoring.views.pubRejHighest
					},
					PUBLISHED_MSGS_LOWEST: {
						name: "PublishedMsgsLowest",
						label: nlsMon.monitoring.views.msgPubLowest
					},
					SUBSCRIPTIONS_LOWEST: {
						name: "SubscriptionsLowest",
						label: nlsMon.monitoring.views.subsLowest
					},
					REJECTED_MSGS_LOWEST: {
						name: "RejectedMsgsLowest",
						label: nlsMon.monitoring.views.msgRejLowest
					},
					FAILED_PUBLISHES_LOWEST: {
						name: "FailedPublishesLowest",
						label: nlsMon.monitoring.views.pubRejLowest
					}
				}
			},
			Subscription: {
				Category: "subscription",
				View: {
					PUBLISHED_MSGS_HIGHEST: {
						name: "PublishedMsgsHighest",
						label: nlsMon.monitoring.views.publishedMsgsHighest
					},
					BUFFERED_MSGS_HIGHEST: {
						name: "BufferedMsgsHighest",
						label: nlsMon.monitoring.views.bufferedMsgsHighest
					},
					BUFFERED_PERCENT_HIGHEST: {
						name: "BufferedPercentHighest",
						label: nlsMon.monitoring.views.bufferedPercentHighest
					},
					REJECTED_MSGS_HIGHEST: {
						name: "RejectedMsgsHighest",
						label: nlsMon.monitoring.views.rejectedMsgsHighest
					},					
					EXPIRED_HIGHEST: {
						name: "ExpiredMsgsHighest",
						label: nlsMon.monitoring.views.ExpiredMsgsHighestS
					},
					DISCARDED_HIGHEST: {
						name: "DiscardedMsgsHighest",
						label: nlsMon.monitoring.views.DiscardedMsgsHighestS
					},										
					BUFFERED_HWM_PERCENT_HIGHEST: {
						name: "BufferedHWMPercentHighest",
						label: nlsMon.monitoring.views.BufferedHWMPercentHighestS
					},	
					PUBLISHED_MSGS_LOWEST: {
						name: "PublishedMsgsLowest",
						label: nlsMon.monitoring.views.publishedMsgsLowest
					},
					BUFFERED_MSGS_LOWEST: {
						name: "BufferedMsgsLowest",
						label: nlsMon.monitoring.views.bufferedMsgsLowest
					},
					BUFFERED_PERCENT_LOWEST: {
						name: "BufferedPercentLowest",
						label: nlsMon.monitoring.views.bufferedPercentLowest
					},
					REJECTED_MSGS_LOWEST: {
						name: "RejectedMsgsLowest",
						label: nlsMon.monitoring.views.rejectedMsgsLowest
					},
					
					EXPIRED_LOWEST: {
						name: "ExpiredMsgsLowest",
						label: nlsMon.monitoring.views.ExpiredMsgsLowestS
					},
					DISCARDED_LOWEST: {
						name: "DiscardedMsgsLowest",
						label: nlsMon.monitoring.views.DiscardedMsgsLowestS
					},					
					
					BUFFERED_HWM_PERCENT_LOWEST: {
						name: "BufferedHWMPercentLowest",
						label: nlsMon.monitoring.views.BufferedHWMPercentLowestS
					}
				},
				Type: {
					ALL: {
						name: "All",
						label: nlsMon.monitoring.grid.allSubscriptions
					},
					DURABLE: {
						name: "Durable",
						label: nlsMon.monitoring.grid.subscription.isDurable
					},
					NONDURABLE: {
						name: "Nondurable",
						label: nlsMon.monitoring.grid.subscription.notDurable
					}
				}
			},

			deleteMQTTClient: function(clientId) {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultDeleteOptions();
				return adminEndpoint.doRequest("/service/MQTTClient/" + encodeURIComponent(clientId), options);
			},

			deleteSubscription: function(subName, clientId) {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultDeleteOptions();
				return adminEndpoint.doRequest("/service/Subscription/" + encodeURIComponent(clientId) + "/" + encodeURIComponent(subName), options);
			},
			
			getQueueData: function(name, view, numRecords) {
				// name - * or name of a specific queue or name ending in * wildcard
				// view - value for StatType query parameter
				// numRecords - value for ResultCount query parameter. It is always 100
				var qprops = { "StatType": view, "ResultCount": numRecords };
				if (name !== "*") {
					qprops.Name = name;
				}
				var qparams = Utils.getQueryParams(qprops);
				
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/Queue" + qparams, options);
			},
			
			getTopicData: function(topicString, view, numRecords, onFinish, onError) {
				// topicString - * or a specific topic or topic ending in * wildcard
				// view - Value for StatType query parameter. Will always be PublishedMsgsHighest?
				// numRecords - Value for ResultCount query parameter. It is always All
				var qprops = { "TopicString": topicString, "StatType": view, "ResultCount": numRecords };
				var qparams = Utils.getQueryParams(qprops);
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/Topic" + qparams, options);
			},

			getDestinationMappingData: function(args) {
				var qparams = Utils.getQueryParams(args);
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/DestinationMappingRule" + qparams, options);
			},

			getSubscriptionData: function(args, onFinish, onError) {
				var qparams = Utils.getQueryParams(args);
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/Subscription" + qparams, options);
			},
			
			getTransactionData: function(args) {
				var qparams = Utils.getQueryParams(args);
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/Transaction" + qparams, options);
			},

			getMQTTClientData: function(clientId, numRecords) {
				var qprops = { "ClientID": clientId, "ResultCount": numRecords };
				var qparams = Utils.getQueryParams(qprops);
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/MQTTClient" + qparams, options);
			},

			getStoreData: function() {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/Store", options);
			},

			getMemoryData: function() {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				return adminEndpoint.doRequest("/monitor/Memory", options);
			},

			getConnData: function(query, endpoint, numRecords) {
				if (!query) {
					throw "Valid query must be passed";
				}
				if (numRecords === null || numRecords === undefined) {
					numRecords = 50;
				} else {
					if (numRecords > 50) {
						numRecords = 50;
					}
					if (numRecords < 0) {
						numRecords = 0;
					}
				}

		    	var qprops = { "StatType": query.name, "SubType": this.Connection.Type.DATA };
		    	if (endpoint) {
		    		qprops.Endpoint = endpoint;
		    	}
		    	var qparams = Utils.getQueryParams(qprops);
	            var adminEndpoint = ism.user.getAdminEndpoint();
	            return adminEndpoint.doRequest("/monitor/Connection" + qparams, adminEndpoint.getDefaultGetOptions());

			},
			
			serverMonitoringTimeoutId: null,
			serverMonitoringStarted: false,
			/**
			 * Starts or stops the server monitoring (gets connection and msg volume in one call
			 * and publishes the data on Resources.monitoring.serverDataTopic every n seconds).
			 * If called with start == true and it's already running, has no effect
			 */			
			startStopServerMonitoring: function(/*boolean*/ start, timeout) {
				if (start && !MonitoringUtils.serverMonitoringStarted) {
					MonitoringUtils.serverMonitoringStarted = true;
					if (!timeout) {
						timeout = 5000;
					}
					MonitoringUtils.publishServerData(timeout);
				} else if (!start && MonitoringUtils.serverMonitoringTimeoutId) {
					MonitoringUtils.serverMonitoringStarted = false;
					window.clearTimeout(MonitoringUtils.serverMonitoringTimeoutId);
				}
			},			
			
			publishServerData: function(timeout) {
	            var adminEndpoint = ism.user.getAdminEndpoint();
	            var promise = adminEndpoint.doRequest("/monitor/Server", adminEndpoint.getDefaultGetOptions());
	            
	            promise.then(function(data) {
	            	console.log("entering publishServerData then function");
	            	data = data.Server;
		    		var retObj = {
		    			connVolume: 0,
		    			connTotal: 0,
		    			msgVolume: 0,
		    			timestamp: Utils.getFormattedTime()
		    		};
		    		if (data && data && data.ActiveConnections !== undefined && data.TotalConnections !== undefined) {
		    			retObj.connVolume = data.ActiveConnections;
		    			retObj.connTotal = data.TotalConnections;
		    		} else {
		    			console.error("server monitoring request returns invalid values");
		    		}
		    		if (data && data.MsgRead !== undefined && data.MsgWrite !== undefined) {
		    			retObj.msgVolume = parseFloat(data.MsgRead) + parseFloat(data.MsgWrite);
		    		} else {
		    			console.error("server monitoring request returns invalid values");
		    		}
		    		if (data) {
		    			retObj.BufferedMessages = data.BufferedMessages;
		    			retObj.RetainedMessages = data.RetainedMessages;
		    		}
		    		if (MonitoringUtils.serverMonitoringStarted) {
		    			console.log("Publishing serverData: " + JSON.stringify(retObj));
			    		dojo.publish(Resources.monitoring.serverDataTopic, retObj);
						MonitoringUtils.serverMonitoringTimeoutId = setTimeout(function(){
							MonitoringUtils.publishServerData(timeout);
						}, timeout);
		    		}
	            }, function(error) {
	            	console.log("publishServerData on error: ");
			    	console.dir(error);
			    	if (MonitoringUtils.serverMonitoringStarted) {
			    		dojo.publish(Resources.monitoring.serverDataTopic, { error: error });
			    		MonitoringUtils.serverMonitoringTimeoutId = setTimeout(function(){
			    			MonitoringUtils.publishServerData(timeout);
			    		}, timeout);
			    	}
	            });
			},

			getMsgVolume: function(onComplete) {
				var adminEndpoint = ism.user.getAdminEndpoint();
	            var promise = adminEndpoint.doRequest("/monitor/Server", adminEndpoint.getDefaultGetOptions());
	            
	            promise.then(function(data) {
	            	console.log("entering getMsgVolume then function");
	            	data = data.Server;
	            	
	            	var retObj = {
	    					msgVolume: 0
	    			};
	    			if (data && data.MsgRead !== undefined && data.MsgWrite !== undefined ) {
	    				retObj.msgVolume = parseFloat(data.MsgRead) + parseFloat(data.MsgWrite);
	    			} else {
	    				console.error("server monitoring request returns invalid values");
	    			}
	    			if (onComplete) {
	    				onComplete(retObj);
	    			}
	            }, function(error) {
	    			console.debug("getMsgVolume error " + error);
	    			if (onComplete) {
	    				onComplete({ error: error });
	    			}		    			
	    		});
			},

			getEndpointNames: function(onComplete) {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				var qparams = Utils.getQueryParams({ "SubType": this.Endpoint.Type.CURRENT });
				var promise = adminEndpoint.doRequest("/monitor/Endpoint" + qparams, options);
				
				promise.then(function(data) {
					console.log(data);

					var namesArray = dojo.map(data.Endpoint, function(item, index){
						return item.Name;
					});
					console.log(namesArray);

					if (onComplete) {
						onComplete(namesArray);
					}
				},
				function(error) {
					console.debug("getEndpointNames error " + error);
					if (onComplete) {
						onComplete({ error: error });
					}
				});
			},
			
			getEndpointStatusDeferred: function() {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				var qparams = Utils.getQueryParams({ "SubType": this.Endpoint.Type.CURRENT });
				return adminEndpoint.doRequest("/monitor/Endpoint" + qparams, options);
			},
			
			getNamedEndpointStatusDeferred: function(endpoint) {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.getDefaultGetOptions();
				var qparams = Utils.getQueryParams({ "Name": endpoint, "SubType": this.Endpoint.Type.CURRENT });
				return adminEndpoint.doRequest("/monitor/Endpoint" + qparams, options);
			},

			getEndpointHistory: function(duration, options) {
				console.log("getEndpointHistory(" + duration + ", " + JSON.stringify(options) + ")");
				if (duration > 86400 || duration <= 0) {
					console.error("Invalid duration");
					return null;
				}

				var metric = this.Endpoint.Metric.BYTES_COUNT.name;
				var numPoints = 100;
				var makeDelta = true;

				var endpoint = undefined;
				var onComplete = undefined;
				if (options) {
					endpoint = options.endpoint ? options.endpoint : undefined;
					metric = options.metric ? options.metric : metric;
					numPoints = options.numPoints ? options.numPoints : numPoints;
					makeDelta = options.makeDelta === false ? false : true;
					onComplete = options.onComplete ? options.onComplete : undefined;
				}
				var interval = 5; // seconds
				if (duration > 3600) {
					interval = 60;
				}
				
				// do the actual request to the server
				switch (metric) {
					case this.Endpoint.Metric.ACTIVE_CONNECTIONS.name:
					case this.Endpoint.Metric.TOTAL_CONNECTIONS.name:
					case this.Endpoint.Metric.BAD_COUNT.name:
						
						var qprops = { "StatType": metric, "SubType": this.Endpoint.Type.HISTORY, "Duration": duration };
						if (endpoint !== undefined) {
							qprops.Name = endpoint;
						}
						var qparams = Utils.getQueryParams(qprops);
						
						var adminEndpoint = ism.user.getAdminEndpoint();
						var restOptions = adminEndpoint.getDefaultGetOptions();
						var promise = adminEndpoint.doRequest("/monitor/Endpoint" + qparams, restOptions);
						
						return promise.then(function(results) {
							var data = results.Endpoint;
							var localTime = new Date();
							var lastupd = Date.parse(data.LastUpdateTimestamp);
			    			data.LastUpdateTimestamp = lastupd;
			    			
			    			if (!data.Data && onComplete) {
			    				onComplete(data);
			    				return;
			    			}

			    			data.CurrentSystemTimestamp = localTime;
							if (data.LastUpdateTimestamp && data.CurrentSystemTimestamp) {
								// adjust to local time
								var timeOffset = dojo.date.difference(new Date(data.CurrentSystemTimestamp), localTime, "second");
								var lastUpdate = dojo.date.add(new Date(data.LastUpdateTimestamp), "second", timeOffset);
								data.LastUpdateTimestamp = lastUpdate.getTime();
							}

			    			var result = dojo.map(data.Data.split(","), function(item, index){
			    				return parseFloat(item);
			    			});

			    			// trim the array to fit the actual duration
			    			var actualSamples = duration / interval;
			    			result = result.slice(result.length - actualSamples);

			    			var finalResult = [];
			    			var delta = actualSamples / numPoints;

			    			if (delta <= 1) {
			    				finalResult = result;
			    			} else {
			    				var curDelta = delta;
			    				var curPoint = 0;

			    				var i = 0, j = 0;
			    				while (i < result.length) {
			    					if (curDelta >= 1) {
			    						curPoint += result[i];
			    						curDelta -= 1;
			    						i++;
			    					} else {
			    						curPoint += curDelta * result[i];
			    						curPoint /= delta;
			    						finalResult[j] = curPoint;
			    						j++;
			    						curPoint = (1.0 - curDelta) * result[i];
			    						curDelta = delta - (1.0 - curDelta);
			    						i++;
			    					}
			    				}
			    				finalResult[j] = finalResult[j - 1];
			    			}

			    			// flip array around, so that it goes OLD --> NEW
			    			if (onComplete) {
			    				delete data.Data;
			    				data[metric] = finalResult.reverse(); 
			    				onComplete(data);
			    			}
						},
						function(error) {
							console.debug("getEndpointHistory error " + error);
			    			if (onComplete) {
			    				onComplete({ error: error });
			    			}	
						});
						break;
						
					case this.Endpoint.Metric.MSG_COUNT.name:
					case this.Endpoint.Metric.BYTES_COUNT.name:
						// do separate requests for read+write, then add them together
						var qpropsThroughput = { "StatType": "Read" + metric, "SubType": this.Endpoint.Type.HISTORY, "Duration": duration };
						if (endpoint !== undefined) {
							qpropsThroughput.Name = endpoint;
						}
						var qparamsThroughput = Utils.getQueryParams(qpropsThroughput);
						
						var adminEndpointThroughput = ism.user.getAdminEndpoint();
						var restOptionsThroughput = adminEndpointThroughput.getDefaultGetOptions();
						var dRead = adminEndpointThroughput.doRequest("/monitor/Endpoint" + qparamsThroughput, restOptionsThroughput);
						
						qpropsThroughput.StatType = "Write" + metric;
						qparamsThroughput = Utils.getQueryParams(qpropsThroughput);
						var dWrite = adminEndpointThroughput.doRequest("/monitor/Endpoint" + qparamsThroughput, restOptionsThroughput);
						
						var dl = new dojo.DeferredList([dRead, dWrite]);
						return dl.then(lang.hitch(this, function(res) {
							//res = res.Endpoint;
							var localTime = new Date();
			    			
							// error checks							
							var error = undefined;
							if (res[0][0] === false) {
								error = res[0][1];
							} else if (res[1][0] === false) {
								error = res[1][1];
							} else if (res[0][1].Endpoint === undefined || res[0][1].Endpoint.Data === undefined) {							
								error = res[0][1];
							} else if (res[1][1].Endpoint === undefined || res[1][1].Endpoint.Data === undefined) {
								error = res[1][1]; 
							}							

							if (error) {
								if (onComplete) {
									onComplete({error: error});
								}
								return;
							}
							
							res[0][1] = res[0][1].Endpoint;
							res[1][1] = res[1][1].Endpoint;
							res[0][1].LastUpdateTimestamp = Date.parse(res[0][1].LastUpdateTimestamp);
			    			res[0][1].CurrentSystemTimestamp = localTime;
			    			res[1][1].LastUpdateTimestamp = Date.parse(res[1][1].LastUpdateTimestamp);
			    			res[1][1].CurrentSystemTimestamp = localTime;
							
							var readRes = res[0][1].Data.split(",");
							var writeRes = res[1][1].Data.split(",");
							var lastUpdateTimestamp = res[0][1].LastUpdateTimestamp;
							var currentSystemTimestamp = res[0][1].CurrentSystemTimestamp;
							if (lastUpdateTimestamp && currentSystemTimestamp) {
								// adjust to local time
								var timeOffset = dojo.date.difference(new Date(currentSystemTimestamp), localTime, "second");
								var lastUpdate = dojo.date.add(new Date(lastUpdateTimestamp), "second", timeOffset);
								lastUpdateTimestamp = lastUpdate.getTime();
							}
							var interval = res[0][1].Interval;
							var result = [];
							var len = Math.min(readRes.length, writeRes.length);
							for (var i = 0; i < len; i++) {
							    result[i] = parseFloat(readRes[i]) + parseFloat(writeRes[i]);
							}
							
			    			// trim the array to fit the actual duration
			    			var actualSamples = duration / interval;
							result = result.slice(result.length - actualSamples);

							// convert returned data to delta values
							len = result.length - 1;
							for (i = 0; i < len; i++) {
								result[i] = (result[i] - result[i+1]) / interval;
							}
							result[result.length - 1] = result[result.length - 2];

							var finalResult = [];
							var delta = actualSamples / numPoints;

							if (delta <= 1) {
								finalResult = result;
							} else {
								var curDelta = delta;
								var curPoint = 0;

								var index = 0;
								var frIndex = 0;
								while (index < result.length) {
									if (curDelta >= 1) {
										curPoint += result[index];
										curDelta -= 1;
										index++;
									} else {
										curPoint += curDelta * result[index];
										curPoint /= delta;

										if (curPoint < 0) {
											curPoint = 0;
										}
										finalResult[frIndex] = curPoint;
										frIndex++;
										curPoint = (1.0 - curDelta) * result[index];
										curDelta = delta - (1.0 - curDelta);
										index++;
									}
								}
								finalResult[frIndex] = finalResult[frIndex - 1];
							}
	
							// flip array around, so that it goes OLD --> NEW					    	
							if (metric == this.Endpoint.Metric.BYTES_COUNT.name) {
								// if bytes, convert to MB
								for (var j=0, jlen=finalResult.length; j < jlen; j++) {
									finalResult[j] /= 1048576;
								}
							}
			    			if (onComplete) {
			    				var data = {};
			    				data.LastUpdateTimestamp = lastUpdateTimestamp;
			    				data.Type = metric;
			    				data.Interval = interval;
			    				data[metric] = finalResult.reverse(); 
			    				onComplete(data);
			    			}
				    	})); 
						break;
					default:
						break;
				}
				return null;
			},

			getThroughputHistory: function(duration, options) {
				console.log("getThrougput(" + JSON.stringify(options) + ")");
				
				if (duration > 86400 || duration <= 0) {
					console.error("Invalid duration");
					return null;
				}

				var metric = this.Endpoint.Metric.MSG_COUNT.name;
				var onComplete = undefined;
				var interval = 5; // seconds

				if (options) {
					metric = options.metric ? options.metric : metric;
					onComplete = options.onComplete ? options.onComplete : onComplete;
					interval = options.interval ? options.interval : interval;
					duration = options.duration ? options.duration : duration;
				}
				
				var numPoints = Math.floor(duration / interval);
				
				// do separate requests for read+write, cluster read rate, cluster write rate, and then add them together
				var qpropsThroughput = { "StatType": "Read" + metric, "SubType": this.Endpoint.Type.HISTORY, "Duration": duration };
				var qparamsThroughput = Utils.getQueryParams(qpropsThroughput);
				
				var adminEndpointThroughput = ism.user.getAdminEndpoint();
				var restOptionsThroughput = adminEndpointThroughput.getDefaultGetOptions();
				var dRead = adminEndpointThroughput.doRequest("/monitor/Endpoint" + qparamsThroughput, restOptionsThroughput);
				
				qpropsThroughput.StatType = "Write" + metric;
				qparamsThroughput = Utils.getQueryParams(qpropsThroughput);
				var dWrite = adminEndpointThroughput.doRequest("/monitor/Endpoint" + qparamsThroughput, restOptionsThroughput);
				
				qpropsThroughput.StatType = "ReceiveRate";
				qparamsThroughput = Utils.getQueryParams(qpropsThroughput);
				var dClRead = adminEndpointThroughput.doRequest("/monitor/Forwarder" + qparamsThroughput, restOptionsThroughput);
				
				qpropsThroughput.StatType = "SendRate";
				qparamsThroughput = Utils.getQueryParams(qpropsThroughput);
				var dClWrite = adminEndpointThroughput.doRequest("/monitor/Forwarder" + qparamsThroughput, restOptionsThroughput);

				var dl = new dojo.DeferredList([dRead, dWrite, dClRead, dClWrite]);
				return dl.then(lang.hitch(this, function(res) {
					var result = {};
					result.timestamp = Utils.getFormattedTime();					
					
					// error checks
					var error = undefined;
					if (res[0][0] === false) {
						error = res[0][1];
					} else if (res[1][0] === false) {
						error = res[1][1];
					} else if (res[2][0] === false) {
						error = res[2][1];
					} else if (res[3][0] === false) {
						error = res[3][1];
					} else if (res[0][1].Endpoint === undefined || res[0][1].Endpoint.Data === undefined) {							
						error = res[0][1];
					} else if (res[1][1].Endpoint === undefined || res[1][1].Endpoint.Data === undefined) {
						error = res[1][1]; 
					} else if (res[2][1].Forwarder === undefined || res[2][1].Forwarder.Data === undefined) {
						error = res[2][1]; 
					} else if (res[3][1].Forwarder === undefined || res[3][1].Forwarder.Data === undefined) {
						error = res[3][1]; 
					}							
					if (error) {
						if (onComplete) {
							result.error = error;
							onComplete(result);
						}
						return;
					}
					
					res[0][1] = res[0][1].Endpoint;
					res[1][1] = res[1][1].Endpoint;
					res[2][1] = res[2][1].Forwarder;
					res[3][1] = res[3][1].Forwarder;

					var readRes = res[0][1].Data.split(",");
					var writeRes = res[1][1].Data.split(",");
					var clreadRes = res[2][1].Data.split(",");
					var clwriteRes = res[3][1].Data.split(",");
					
					var readMetric = metric+"Read";
					var writeMetric = metric+"Write";
					var clreadMetric = "Cl"+metric+"Read";
					var clwriteMetric = "Cl"+metric+"Write";
					
					var count = Math.min(readRes.length-1, writeRes.length-1, clreadRes.length-1, clwriteRes.length-1, numPoints-1);
					result[readMetric] = [];
					result[writeMetric] = [];
					result[clreadMetric] = [];
					result[clwriteMetric] = [];
					result[metric] = [];
									
					for (var i = 0; i < count; i++) {
						result[readMetric][i] = (parseFloat(readRes[i]) - parseFloat(readRes[i+1])) / interval;
						result[writeMetric][i] = (parseFloat(writeRes[i]) - parseFloat(writeRes[i+1])) / interval;	
						result[clreadMetric][i] = parseFloat(clreadRes[i+1]);
						result[clwriteMetric][i] = parseFloat(clwriteRes[i+1]);	
						result[metric][i] = result[readMetric][i] + result[writeMetric][i] + result[clreadMetric][i] + result[clwriteMetric][i];						
					}
					
                    // the currentThroughput
					result["current"+metric] = result[metric][0];
					
					// the last data point doesn't have something to compare against, so add a zero(s)
					for (var j = count; j < numPoints; j++) {
						result[readMetric][j] = 0;
						result[writeMetric][j] = 0;	
						result[clreadMetric][j] = 0;
						result[clwriteMetric][j] = 0;	
						result[metric][j] = 0;				
					}
									
					
					// reverse the results
					result[readMetric] = result[readMetric].reverse();
					result[writeMetric] = result[writeMetric].reverse();
					result[clreadMetric] = result[clreadMetric].reverse();
					result[clwriteMetric] = result[clwriteMetric].reverse();
					result[metric] = result[metric].reverse();
					
					if (onComplete) {
						onComplete(result);
					} else {
						console.debug("getThroughputHistory called with no onComplete defined.");
					}
		    	})); 
			},
			
			getMemoryHistory: function(duration, options) {
				
				if (duration > 86400 || duration <= 0) {
					duration = 86400;
				}

				var onComplete = undefined;
				var metric = "MemoryFreePercent";
				var invert = false;

				if (options) {					
					metric = options.metric ? options.metric : metric;
					onComplete = options.onComplete ? options.onComplete : undefined;
				}
				var requestedMetric = metric;
				if (metric == "MemoryUsedPercent") {
					metric = "MemoryFreePercent";
					invert = true;
				}
				
				var request = {
						SubType: "History",
						StatType: metric,
						Duration: duration
				};
				var qparams = Utils.getQueryParams(request);
				
				var adminEndpoint = ism.user.getAdminEndpoint();
				var restOptions = adminEndpoint.getDefaultGetOptions();
				var promise = adminEndpoint.doRequest("/monitor/Memory" + qparams, restOptions);
				
				return promise.then(function(data) {
					data = data.Memory;
					console.log("getMemoryHistory load().... data is ");
	    			console.dir(data);
	    			var results = {};
	    			for (var prop in data) {
	    				if (metric != prop) {
	    					results[prop] = data[prop];
	    					continue;
	    				}
	    			}
	    			var dataPoints = data[metric];
	    			if (dataPoints) {
		    			var result = dojo.map(dataPoints.split(","), function(item, index){
		    				var value = parseFloat(item);
		    				if (invert) {
		    					if (value === -1) {
		    						value = 0;
		    					} else {
			    					value = 100 - value;
			    				}
		    				} 
		    				return value;
		    			});
		    			results[requestedMetric] = result.reverse();
	    			} else {
	    				console.log("getMemoryHistory found no results");
	    			}
	    			// flip array around, so that it goes OLD --> NEW
	    			if (onComplete) {
	    				onComplete(results);
	    			}
				}, function(error) {
	    			if (onComplete) {
	    				console.log("getMemoryHistory returning error ");
	    				console.dir(error);
	    				onComplete({ error: error });
	    			}	
				});
			},

			getMemoryDetailHistory: function(duration, options) {
				
				if (duration > 86400 || duration <= 0) {
					duration = 86400;
				}

				var onComplete = undefined;
				var metric = "MemoryTotalBytes,MemoryFreeBytes,MessagePayloads,PublishSubscribe,Destinations,CurrentActivity,ClientStates";

				if (options) {					
					onComplete = options.onComplete ? options.onComplete : undefined;
				}
				
				var request = {
						SubType: "History",
						StatType: metric,
						Duration: duration
				};
				var qparams = Utils.getQueryParams(request);
				
				var adminEndpoint = ism.user.getAdminEndpoint();
				var restOptions = adminEndpoint.getDefaultGetOptions();
				var promise = adminEndpoint.doRequest("/monitor/Memory" + qparams, restOptions);
				
				return promise.then(function(data) {
					data = data.Memory;
					var results = {};
	    			var stats = {};
	    			for (var prop in data) {
	    				if (metric.indexOf(prop) < 0) {
	    					results[prop] = data[prop];
	    					continue;
	    				}
	    				stats[prop] = "";
	    				var mapped = dojo.map(data[prop].split(","), function(item, index){
	    					var value = parseInt(item, 10);
	    					if (value === -1) {
	    						value = 0;
	    					}
	    					return value;
		    			});
		    			// flip array around, so that it goes OLD --> NEW
	    				results[prop] = mapped.reverse();
	    			}
	    			// convert to percentages and add in the calculated system category
	    			var length = results.MemoryTotalBytes === undefined || results.MemoryFreeBytes === undefined ? 0 : results.MemoryTotalBytes.length;
	    			// calculate bytesUsedPercent and find highest number
	    			var maxBytesUsedPercent = 0;
	    			var bytesUsedPercent = 0;
	    			var totalBytes = 0;
	    			results.BytesUsedPercent = [];
	    			for (var i = 0; i < length; i++) {
	    				totalBytes = results.MemoryTotalBytes[i];
	    				var freeBytes = results.MemoryFreeBytes[i];
	    				bytesUsedPercent = 0;
	    				if (freeBytes < totalBytes) {
	    					bytesUsedPercent = freeBytes === 0 ? 100 : number.round(((totalBytes - freeBytes)/totalBytes) * 100, 2);
	    					maxBytesUsedPercent = Math.max(maxBytesUsedPercent, bytesUsedPercent);
	    				}	
	    				results.BytesUsedPercent[i] = bytesUsedPercent;
	    			}
	    			var places = 2;
	    			if (maxBytesUsedPercent < 1) {
	    				places = 4;
	    			} else if (maxBytesUsedPercent < 10) {
	    				places = 3;
	    			}
	    			delete stats.MemoryTotalBytes;
	    			delete stats.MemoryFreeBytes;
	    			results.System = [];
	    			for (i = 0; i < length; i++) {
	    				// do the calculations unless MemoryTotalBytes is 0
	    				if (results.MemoryTotalBytes[i] === 0) {
	    					results.System[i] = 0;
	    					continue;
	    				}
	    				totalBytes = results.MemoryTotalBytes[i];
	    				bytesUsedPercent = results.BytesUsedPercent[i];
	    				var allocatedPercent = 0;
	    				for (var stat in stats) {
	    					var val = results[stat][i];
	    					if (val === 0) {
	    						continue;
	    					}
	    					val = (val/totalBytes) * 100;
	    					val = number.round(val, places);
	    					results[stat][i] = val;
	    					allocatedPercent += val;		    					
	    				}
	    				var systemUsage = allocatedPercent > bytesUsedPercent ? 0 : bytesUsedPercent - allocatedPercent; 
	    				results.System[i] = systemUsage;		    				
	    			}
	    			
	    			if (onComplete) {
	    				onComplete(results);
	    			}
				}, function(error) {								    			
	    			if (onComplete) {
	    				console.log("getMemoryDetailHistory returning error ");
	    				console.dir(error);
	    				onComplete({ error: error });
	    			}	
				});
			},

			getStoreHistory: function(duration, options) {
				
				if (duration > 86400 || duration <= 0) {
					duration = 86400;
				}

				var onComplete = undefined;
				var metric = "DiskUsedPercent";

				if (options) {					
					metric = options.metric ? options.metric : metric;
					onComplete = options.onComplete ? options.onComplete : undefined;
				}
				
				var request = {
						SubType: "History",
						StatType: metric,
						Duration: duration
				};
				var qparams = Utils.getQueryParams(request);
				
				var adminEndpoint = ism.user.getAdminEndpoint();
				var restOptions = adminEndpoint.getDefaultGetOptions();
				var promise = adminEndpoint.doRequest("/monitor/Store" + qparams, restOptions);
				
				return promise.then(function(data) {
					data = data.Store;
					var results = {};
	    			for (var prop in data) {
	    				if (metric.indexOf(prop) < 0) {
	    					results[prop] = data[prop];
	    					continue;
	    				}
	    				var mapped = dojo.map(data[prop].split(","), function(item, index){
		    				var value = parseFloat(item);
		    				if (value === -1) {
		    					value = 0;
		    				} 
		    				return value;
		    			});
		    			// flip array around, so that it goes OLD --> NEW
	    				results[prop] = mapped.reverse();
	    			}
	    			if (onComplete) {
	    				onComplete(results);
	    			}
				}, function(error) {									    			
	    			if (onComplete) {
	    				onComplete({ error: error });
	    			}
				});
			},
			
			getStoreDetailHistory: function(duration, options) {
				
				if (duration > 86400 || duration <= 0) {
					duration = 86400;
				}

				var onComplete = undefined;
                var metric = "Pool1TotalBytes,Pool1UsedPercent,Pool1UsedBytes,Pool1RecordsLimitBytes,Pool1RecordsUsedBytes,ClientStatesBytes,QueuesBytes,TopicsBytes," +
                             "SubscriptionsBytes,TransactionsBytes,MQConnectivityBytes,Pool2TotalBytes,Pool2UsedBytes,Pool2UsedPercent,IncomingMessageAcksBytes";

				if (options) {					
					onComplete = options.onComplete ? options.onComplete : undefined;
				}
				
				var request = {
						SubType: "History",
						StatType: metric,
						Duration: duration
				};
				var qparams = Utils.getQueryParams(request);
				
				var adminEndpoint = ism.user.getAdminEndpoint();
				var restOptions = adminEndpoint.getDefaultGetOptions();
				var promise = adminEndpoint.doRequest("/monitor/Store" + qparams, restOptions);
				
				return promise.then(function(data) {
					data = data.Store;
					var results = {};
	    			var stats = {};
	    			for (var prop in data) {
	    				if (metric.indexOf(prop) < 0) {
	    					results[prop] = data[prop];
	    					continue;
	    				}
	    				var mapped = dojo.map(data[prop].split(","), function(item, index){
	    					var value = parseInt(item, 10);
	    					if (value === -1) {
	    						value = 0;
	    					}
	    					return value;
		    			});

		    			// flip array around, so that it goes OLD --> NEW
	    				results[prop] = mapped.reverse();
	    			}
	    			
	    			if (results.Pool1RecordsLimitBytes) {
                        // Has a pool1 limit been reached?
		    			var latest = results.Pool1RecordsLimitBytes.length-1;
                        stats.Pool1Limit = (results.Pool1RecordsLimitBytes[latest] / results.Pool1TotalBytes[latest]) * 100;
                        // warn when less than 4096 bytes remains
                        var warnLimit = results.Pool1RecordsLimitBytes[latest] - 4097;
                        if (results.Pool1RecordsUsedBytes[latest] < warnLimit) {
                            stats.Pool1LimitMessage = "";
                        } else {
                            stats.Pool1LimitMessage = nlsMon.monitoring.pool1LimitReached;
                        }
	    			} else {
	    			    stats.Pool1LimitMessage = "";
	    			}

	    			
	    			// convert to percentages and add in the calculated system category
                    var length = results.Pool1TotalBytes === undefined || results.Pool1TotalBytes === undefined ? 0 : results.Pool1TotalBytes.length;
                    
	    			// find highest MemoryUsedPercent
	    			var maxBytesUsedPercent = 0;
	    			for (var i = 0; i < length; i++) {
    					maxBytesUsedPercent = Math.max(maxBytesUsedPercent,  results.Pool1UsedPercent[i]);
	    			}
	    			var pool1Places = 2;
	    			if (maxBytesUsedPercent < 1) {
	    				pool1Places = 4;
	    			} else if (maxBytesUsedPercent < 10) {
	    				pool1Places = 3;
	    			}

                    maxBytesUsedPercent = 0;
                    for (i = 0; i < length; i++) {
                        maxBytesUsedPercent = Math.max(maxBytesUsedPercent,  results.Pool2UsedPercent[i]);
                    }
                    var pool2Places = 2;
                    if (maxBytesUsedPercent < 1) {
                        pool2Places = 4;
                    } else if (maxBytesUsedPercent < 10) {
                        pool2Places = 3;
                    }
	    			
                    var recordTypes = ["ClientStates","Queues","Topics","Subscriptions","Transactions","MQConnectivity"];
                    var statLen=recordTypes.length;
                    for (var statIndex=0; statIndex < statLen; statIndex++) {
                        var stat = recordTypes[statIndex]; 
                        stats[stat] = results[stat+"Bytes"];                              
                    }
                    stats.IncomingMessageAcks = results.IncomingMessageAcksBytes;
                    stats.Pool1System = [];
                    stats.Pool2System = [];                        
	    			for (i = 0; i < length; i++) {
	    			    // Pool1
	    				// do the calculations unless Pool1TotalBytes is 0
                        var totalBytes = results.Pool1TotalBytes[i];
	    			    if (totalBytes === 0) {
	                        stats.Pool1System[i] = 0;
	                        stats.Pool2System[i] = 0;                     
	    					continue;
	    				}
	    			    for (statIndex=0; statIndex < statLen; statIndex++) {
	    				    stat = recordTypes[statIndex];
	    					var val = stats[stat][i];
	    					if (val === 0) {
                                continue;
                            }
	    					val = (val/totalBytes) * 100;
	    					stats[stat][i] = number.round(val, pool1Places);		    					
	    				}
	    				var systemUsage =  results.Pool1UsedBytes[i] - results.Pool1RecordsUsedBytes[i];
                        val = (systemUsage/totalBytes) * 100;
	    				stats.Pool1System[i] = number.round(val, pool1Places);
	    				// Pool2
	    				if (stats.IncomingMessageAcks[i] !== 0) {
	    				    val = (stats.IncomingMessageAcks[i] / results.Pool2TotalBytes[i]) * 100;
	    				    stats.IncomingMessageAcks[i] = number.round(val, pool2Places);
	    				}
                        systemUsage =  results.Pool2UsedBytes[i] - results.IncomingMessageAcksBytes[i];
	    				val = (systemUsage/results.Pool2TotalBytes[i]) * 100;
	    				stats.Pool2System[i] = number.round(val, pool2Places);
	    			}
	    			
	    			if (onComplete) {
	    			    console.dir(stats);
	    				onComplete(stats);
	    			}
				}, function(error) {									    			
	    			if (onComplete) {
	    				console.log("getStoreDetailHistory returning error ");
	    				console.dir(error);
	    				onComplete({ error: error });
	    			}	
				});
			},

			monitoringPageErrorShown: false,
			
			clearMonitoringPageError: function() {
				query(".errorMessage","mainContent").forEach(function(node) {
	                var message = dijit.byId(node.id);
	                if (!message.sticky) {
	                    message.destroy();
	                } else {
	                    console.debug("found a sticky message: " + message.get("title"));
	                }      
				});

				MonitoringUtils.monitoringPageErrorShown = false;
			},
			
			showMonitoringPageError: function(error) {
				
				if (_Utils.reloading) {
					console.log("MonitoringUtils.showMonitoringPageError: returning because reloading is in progress");
					return;
				}
				
				if (MonitoringUtils.monitoringPageErrorShown) {
					return;
				}
				
				// request a status update, this might be due to a server status change...
				Utils.updateStatus();
				
				var title = nlsMon.monitoring.monitoringError;
				
				// If a monitoring error is already displayed, don't display another				
				var duplicateFound = false;
				query(".errorMessage","mainContent").forEach(function(node) {
					var singleMessage = dijit.byId(node.id);
					if (singleMessage.get("title") === title) {
						// we already have one shown
						duplicateFound = true;
					}
				});
	            if (duplicateFound) {
	                MonitoringUtils.monitoringPageErrorShown = true;
                    console.log("showMonitoringPageError returning because a monitoring error is already displayed");
	                return;
	            }
	            
	            // If monitoring warning message is displayed, don't display the error message, too
                query(".warningMessage","mainContent").forEach(function(node) {
                    var singleMessage = dijit.byId(node.id);
                    if (singleMessage.get("title") === nlsHome.home.dashboards.monitoringNotAvailable) {
                        // we already have one shown
                        duplicateFound = true;
                    }
                });	            

                if (duplicateFound) {
                    console.log("showMonitoringPageError returning because a monitoring warning is already displayed");
                    return;
                }

				var message = null;
				var code = null;
				
				if (error) {
					var response = error.response.data;
					if (response) {
						message = response.Message ? response.Message : null;
						code = response.Code ? response.Code : null;
					}
				}
				
				// if the code is CWLNA5035 or CWLNA5102, don't show the error page because monitoring pages will
				// show a warning for this situation
				if (code === "CWLNA5035" || code === "CWLNA5102" || code === "130") {
					console.log("showMonitoringPageError returning because the code inidicates the server is not available or not in production mode");
					return;
				}
				
				MonitoringUtils.monitoringPageErrorShown = true;			
				dojo.publish(Resources.errorhandling.errorMessageTopic, { title: title, message: message, code: code });				
			},
			
			mbDecorator: function(/*String*/ value, /*Number*/ places) {
			    var intValue = parseInt(value, 10);
			    places = places == undefined ? 2 : places;
                if (isNaN(intValue)) {
                    console.log("mbDecorator receive invalid value " + value);
                    return value;
                }			    
			    var num =  intValue / Math.pow(1024, 2);
			    return number.format(num, {places: places, locale: ism.user.localeInfo.localeString}) + " MB";			    
			}

	};

	return MonitoringUtils;
});

