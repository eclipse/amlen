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

// Reviewed by Dev and ID
// Any exceptions are flagged with // Needs review until review is complete

define({
	root : ({

		// ------------------------------------------------------------------------
		// Home
		// ------------------------------------------------------------------------
		home : {
			title : "Home",
			taskContainer : {
				title : "Common configuration and customization tasks",
				tagline: "Quick links to general administrative tasks.",
				// TRNOTE {0} is a number from 1 - 5.
				taskCount : "({0} tasks remaining)",
				links : {
					restore : "Restore Tasks",
					restoreTitle: "Restores closed tasks to the common configuration and customization tasks section.",
					hide : "Hide Section",
					hideTitle: "Hides the common configuration and customization tasks section. " +
							"To restore the section, select Restore Tasks on Home Page from the Help menu."						
				}
			},
			tasks : {
				messagingTester : {
					heading: "Verify your configuration with the ${IMA_PRODUCTNAME_SHORT} Messaging Tester sample application",
					description: "The Messaging Tester is a simple HTML5 sample application that uses MQTT over WebSockets to verify that the server is properly configured.",
					links: {
						messagingTester: "Messaging Tester Sample Application"
					}
				}, 
//				applianceSettings : {
//					heading : "Customize server settings",
//					description : "Configure Ethernet interfaces and domain name servers. " +
//							      "You can also customize the server locale, date and time settings, and set the node name.",
//					links : {
//						networkSettings : "Network Settings",
//						timeAndDate : "Locale, Date and Time Settings",
//						hostname: "System Control"							
//					}
//				},
				users : {
					heading : "Create users",
					description : "Give users access to the Web UI.",
					links : {
						users : "Web UI Users",
						userGroups : "LDAP Configuration"						
					}
				},
				
				connections: {
					heading: "Configure ${IMA_PRODUCTNAME_FULL} to accept connections",
					description: "Define a message hub to manage server connections.  Configure MQ Connectivity to connect ${IMA_PRODUCTNAME_FULL} to MQ queue managers. Configure LDAP to authenticate messaging users.",
					links: {
						listeners: "Message Hubs",
						mqconnectivity: "MQ Connectivity"
					}
				},
				security : {
					heading : "Secure your environment",
					description : "Control the interface and port that the Web UI listens on. Import keys and certificates to secure connections to the server.",
					links : {
						keysAndCerts : "Server Security",
						webuiSettings : "Web UI Settings"
					}
				}
			},
			dashboards: {
				tagline: "An overview of server connections and performance.",
				monitoringNotAvailable: "Monitoring not available because of the status of the ${IMA_SVR_COMPONENT_NAME}.",
				flex: {
					title: "Server Dashboard",
					// TRNOTE {0} is a user provided name for the server.
					titleWithNodeName: "Server Dashboard for {0}",
					quickStats: {
						title: "Quick Stats"
					},
					liveCharts: {
						title: "Active Connections and Throughput"
					},
					applianceResources: {
						title: "Server Memory Usage"
					},
					resourceDetails: {
						title: "Store Memory Usage"
					},
					diskResourceDetails: {
						title: "Disk Usage"
					},
					activeConnectionsQS: {
						title: "Active Connections Count",
						description: "Number of active connections.",
						legend: "Active connections"
					},
					throughputQS: {
						title: "Current Throughput",
						description: "Current throughput in messages per second.",
						legend: "Messages / second"
					},
					uptimeQS: {
						title: "${IMA_PRODUCTNAME_FULL} Uptime",
						description: "Status of ${IMA_PRODUCTNAME_FULL} and how long it has been running.",
						legend: "${IMA_PRODUCTNAME_SHORT} uptime"
					},
					applianceQS: {
						title: "Server Resources",
						description: "Percentages of the persistent store disk and server memory used.",
						bars: {
							pMem: {
								label: "Persistent Memory:"								
							},
							disk: { 
								label: "Disk:",
								warningThresholdText: "Resource usage is at or above the warning threshold.",
								alertThresholdText: "Resource usage is at or above the alert threshold."
							},
							mem: {
								label: "Memory:",
								warningThresholdText: "Memory usage is getting high. When memory usage reaches 85%, publications will be rejected and connections might be dropped.",
								alertThresholdText: "Memory usage is too high. Publications will be rejected and connections might be dropped."	
							}
						},
						warningThresholdAltText: "Warning icon",
						alertThresholdText: "Resource usage is at or above the alert threshold.",
						alertThresholdAltText: "Alert icon"
					},
					connections: {
						title: "Connections",
						description: "Snapshot of connections, taken approximately every five seconds.",
						legend: {
							x: "Time",
							y: "Connections"
						}
					},
					throughput: {
						title: "Throughput",
						description: "Snapshot of messages per second, taken approximately every five seconds.",
						legend: {
							x: "Time",						
							y: "Messages / second",
							title: "Messages / second",
							incoming: "Incoming",
							outgoing: "Outgoing",
							hover: {
								incoming: "Snapshot of messages read per second from clients, taken approximately every five seconds.",
								outgoing: "Snapshot of messages written per second to clients, taken approximately every five seconds."
							}
						}
					},
					memory: {
						title: "Memory",
						description: "Snapshot of memory usage, taken approximately every minute.",
						legend: {
							x: "Time",
							y: "Memory Used (%)"
						}
					},
					disk: {
						title: "Disk",
						description: "Snapshot of persistent store disk usage, taken approximately every minute.",
						legend: {
							x: "Time",
							y: "Disk Used (%)"
						}						
					},
					memoryDetail: {
						title: "Details of Memory Usage",
						description: "Snapshot of memory usage, taken approximately every minute, for key messaging resources.",
						legend: {
							x: "Time",
							y: "Memory Used (%)",
							title: "Messaging Memory Usage",
							system: "Server Host System",
							messagePayloads: "Message Payloads",
							publishSubscribe: "Publish Subscribe",
							destinations: "Destinations",
							currentActivity: "Current Activity",
							clientStates: "Client States",
							hover: {
								system: "Memory allocated for other system uses, such as the operating system, connections, security, logging, and administration.",
								messagePayloads: "Memory allocated for messages. " +
										"When a message is published to multiple subscribers, only one copy of the message is allocated in memory. " +
										"Memory for retained messages is also allocated in this category. " +
										"Workloads which buffer a large volume of messages in the server for disconnected or slow consumers, and workloads which use a large number of retained messages can consume a lot of message payload memory.",
								publishSubscribe: "Memory allocated for performing publish/subscribe messaging. " +
										"The server allocates memory in this category to keep track of retained messages and subscriptions. " +
										"The server maintains caches of publish/subscribe information for performance reasons. " +
										"Workloads which use a large number of subscriptions, and workloads which use a large number of retained messages can consume a lot of publish/subscribe memory.",
								destinations: "Memory allocated for messaging destinations. " +
										"This category of memory is used to organise messages into the queues and subscriptions that are used by clients. " +
										"Workloads which buffer a large number of messages in the server, and workloads which use a large number of subscriptions can consume a lot of destinations memory.",
								currentActivity: "Memory allocated for current activity. " +
										"This includes sessions, transactions and message acknowledgements. " +
										"Information to satisfy monitoring requests is also allocated in this category. " +
										"Complex workloads with a large number of connected clients, and workloads which make extensive use of features such as transactions or unacknowledged messages can consume a lot of current activity memory.",
								clientStates: "Memory allocated for connected and disconnected clients. " +
										"The server allocates memory in this category for each connected client. " +
										"In MQTT, clients which connected setting CleanSession to 0 must be remembered when they are disconnected. " +
										"In addition, memory is allocated from this category to keep track of incoming and outgoing message acknowledgements in MQTT. " +
										"Workloads which use a large number of connected and disconnected clients can consume a lot of client-state memory, particularly if high quality of service messages are being used."								
							}
						}
					},
					storeMemory: {
						title: "Details of Persistent Store Memory Usage",
						description: "Snapshot of persistent store memory usage, taken approximately every minute.",
						legend: {
							x: "Time",
							y: "Memory Used (%)",
							pool1Title: "Pool 1 Memory Usage",
							pool2Title: "Pool 2 Memory Usage",
							system: "Server Host System",
							IncomingMessageAcks: "Incoming Message Acknowledgements",
							MQConnectivity: "MQ Connectivity",
							Transactions: "Transactions",
							Topics: "Topics with Retained Messages",
							Subscriptions: "Subscriptions",
							Queues: "Queues",										
							ClientStates: "Client States",
							recordLimit: "Limit for Records",
							hover: {
								system: "Store memory reserved or in use by the system.",
								IncomingMessageAcks: "Store memory allocated for acknowledging incoming messages. " +
										"The server allocates memory in this category for MQTT clients that connected using a CleanSession=0 setting and are publishing messages the quality of service of which is 2. " +
										"This memory is used to ensure once-and-once-only delivery.",
								MQConnectivity: "Store memory allocated for connectivity with IBM WebSphere MQ queue managers.",
								Transactions: "Store memory allocated for transactions. " +
										"The server allocates memory in this category for each transaction so that it can complete recovery when the server restarts.",
								Topics: "Store memory allocated for topics. " +
										"The server allocates memory in this category for each topic with retained messages.",
								Subscriptions: "Store memory allocated for durable subscriptions. " +
										"In MQTT, these are subscriptions for clients that connected using a CleanSession=0 setting.",
								Queues: "Store memory allocated for queues. " +
										"Memory is allocated in this category for each queue that is created for point-to-point messaging.",										
								ClientStates: "Store memory allocated for clients which must be remembered when they are disconnected. " +
										"In MQTT, these are clients that connected using a CleanSession=0 setting or clients that connected and set a will message the quality of service of which is 1 or 2.",
								recordLimit: "When the record limit is reached, records cannot be created for retained messages, durable subscriptions, queues, clients, and connectivity with WebSphere MQ queue managers."
							}
						}						
					},					
					peakConnectionsQS: {
						title: "Peak Connections",
						description: "Peak number of active connections over the specified period.",
						legend: "Peak connections"
					},
					avgConnectionsQS: {
						title: "Average Connections",
						description: "Average number of active connections over the specified period.",
						legend: "Average connections"
					},
					peakThroughputQS: {
						title: "Peak Throughput",
						description: "Peak number of messages per second over the specified period.",
						legend: "Peak messages / second"
					},
					avgThroughputQS: {
						title: "Average Throughput",
						description: "Average number of messages per second over the specified period.",
						legend: "Average messages / second"
					}
				}
			},
			status: {
				rc0: "Initializing",
				rc1: "Running (production)",
				rc2: "Stopping",
				rc3: "Initialized",
				rc4: "Transport started",
				rc5: "Protocol started",
				rc6: "Store started",
				rc7: "Engine started",
				rc8: "Messaging started",
				rc9: "Running (maintenance)",
				rc10: "Standby",
				rc11: "Store Starting",
				rc12: "Maintenance (clean store in progress)",
				rc99: "Stopped",				
				unknown: "Unknown",
				// TRNOTE {0} is a mode, such as production or maintenance.
				modeChanged: "When the server is restarted, it will be in <em>{0}</em> mode.",
				cleanStoreSelected: "The <em>clean store</em> action has been requested. It will take effect when the server is restarted.",
				mode_0: "production",
				mode_1: "maintenance",
				mode_2: "clean store",
				adminError: "${IMA_PRODUCTNAME_FULL} detects an error.",
				adminErrorDetails: "The error code is: {0}. The error string is: {1}"
			},
			storeStatus: {
				mode_0: "persistent",
				mode_1: "memory only"
			},
			memoryStatus: {
				ok: "OK",
				unknown: "Unknown",
				error: "The memory check reports an error.",
				errorMessage: "The memory check reports an error. Contact Support."
			},
			role: {
				PRIMARY: "Primary node",
				PRIMARY_SYNC: "Primary node (Synchronizing, {0}% complete)",
				STANDBY: "Standby node",
				UNSYNC: "Node out of synchronization",
				TERMINATE: "Standby node terminated by the primary node",
				UNSYNC_ERROR: "Node can no longer be synchronized",
				HADISABLED: "Disabled",
				UNKNOWN: "Unknown",
				unknown: "Unknown"
			},
			haReason: {
				"1": "The store configurations of the two nodes does not allow them to work as an HA pair. " +
				     "The mismatched configuration item is: {0}",
				"2": "Discovery time expired. Failed to communicate with the other node in the HA pair.",
				"3": "Two primary nodes have been identified.",
				"4": "Two unsynchronized nodes have been identified.",
				"5": "Cannot determine which node should be the primary node because two nodes have non-empty stores.",
				"7": "The node failed to pair with the remote node since the two nodes have different group identifiers.",
				"9": "An HA server or internal error has occurred",
				// TRNOTE {0} is a time stamp (date and time).
				lastPrimaryTime: "This node was the primary node on {0}."
			},
			statusControl: {
				label: "Status",
				ismServer: "${IMA_SVR_COMPONENT_NAME_FULL}:",
				serverNotAvailable: "Contact your server system administrator to start the server",
				serverNotAvailableNonAdmin: "Contact your server system administrator to start the server",
				haRole: "High availability:",
				pending: "Pending..."
			}
		},
		recordsLabel: "Records",
		// Additions for cluster status to appear in status dropdown
		statusControl_cluster: "Cluster:",
		clusterStatus_Active: "Active",
		clusterStatus_Inactive: "Inactive",
		clusterStatus_Removed: "Removed",
		clusterStatus_Connecting: "Connecting",
		clusterStatus_None: "Not configured",
		clusterStatus_Unknown: "Unknown",
		
		// Additions to throughput chart on dashboard (also appears on cluster status page)
		
		//TRNOTE: Remote throughput refers to messages from remote servers that are members 
		// of the same cluster as the local server.
		clusterThroughputLabel: "Remote",
		// TRNOTE: Local throughput refers to messages that are received directly from
		// or that are sent directly to messaging clients that are connected to the
		// local server.
		clientThroughputLabel: "Local",
		clusterLegendIncoming: "Incoming",
		clusterLegendOutgoing: "Outgoing",
		clusterHoverIncoming: "Snapshot of messages read per second from remote cluster members, taken approximately every five seconds.",
		clusterHoverOutgoing: "Snapshot of messages written per second to to remote cluster members, taken approximately every five seconds."

	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
