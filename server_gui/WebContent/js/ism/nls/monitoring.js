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
		// Monitoring
		// ------------------------------------------------------------------------
		monitoring : {
			title : "Monitoring",
			monitoringNotAvailable: "Monitoring not available because of the status of the ${IMA_SVR_COMPONENT_NAME}.",
			monitoringError:"An error occurred while retrieving monitoring data.",
			pool1LimitReached: "Pool 1 usage is too high. Publications and subscriptions might be rejected.",
			connectionStats: {
				title: "Connection Monitor",
				tagline: "Monitor aggregated connection data and query best-performing and worst-performing connections across several connection statistics.",
		 		charts: {
					sectionTitle: "Connection Charts",
					tagline: "Monitor the number of active and new connections to the server. To pause chart updates, click the button located beneath the charts.",
					activeConnectionsTitle: "Snapshot of active connections, taken approximately every five seconds.",
					newConnectionsTitle: "Snapshot of new and closed connections, taken approximately every five seconds."
				},
		 		grids: {
					sectionTitle: "Connection Data",
					tagline: "View the best- or worst- performing connections for a particular endpoint and statistic. " +
							"Up to 50 connections can be viewed. The monitoring data is cached on the server and this cache is refreshed every minute. " +
							"Therefore, the data can be up to one minute out of date."
				}
			},
			endpointStats: {
				title: "Endpoint Monitor",
				tagline: "Monitor historical performance of individual endpoints as a time-series chart or as aggregated data.",
		 		charts: {
					sectionTitle: "Endpoint Chart",
					tagline: "View time-series charts for a particular endpoint, statistic, and time duration. " +
						"For durations of 1 hour or less, the charts are based on snapshots taken every 5 seconds. " +
						"For longer durations, the charts are based on snapshots taken every minute."
				},
		 		grids: {
					sectionTitle: "Endpoint Data",
					tagline: "View aggregated data for individual endpoints. The monitoring data is cached on the server and this cache is refreshed every minute. " +
							 "Therefore, the data can be up to one minute out of date."
				}
			},
			queueMonitor: {
				title: "Queue Monitor",
				tagline: "Monitor individual queues using various queue statistics. Up to 100 queues can be viewed."
			},
			mqttClientMonitor: {
				title: "Disconnected MQTT Clients",
				tagline: "Search for and delete unconnected MQTT clients. Up to 100 MQTT clients can be viewed.",
				taglineNoDelete: "Search for unconnected MQTT clients. Up to 100 MQTT clients can be viewed."
			},
			subscriptionMonitor: {
				title: "Subscription Monitor",
				tagline: "Monitor subscriptions using various subscription statistics. Delete durable subscriptions. Up to 100 subscriptions can be viewed.",
				taglineNoDelete: "Monitor subscriptions using various subscription statistics. Up to 100 subscriptions can be viewed."
			},
			transactionMonitor: {
				title: "Transaction Monitor",
				tagline: "Monitor XA transactions coordinated by an external transaction manager."
			},
			destinationMappingRuleMonitor: {
				title: "MQ Connectivity Destination Mapping Rule Monitor",
				tagline: "Monitor destination mapping rule statistics. Statistics are for a destination mapping rule / queue manager connection pair. Up to 100 pairs can be viewed."
			},
			applianceMonitor: {
				title: "Server Monitor",
				tagline: "Gather information on the various ${IMA_PRODUCTNAME_FULL} statistics.",
				storeMonitorTitle: "Persistent Store",
				storeMonitorTagline: "Monitor statistics of resources used by the ${IMA_PRODUCTNAME_FULL} persistent store.",
				memoryMonitorTitle: "Memory",
				memoryMonitorTagline: "Monitor statistics of memory used by other ${IMA_PRODUCTNAME_FULL} processes."
			},
			topicMonitor: {
				title: "Topic Monitor",
				tagline: "Configure topic monitors that provide various aggregated statistics about topic strings.",
				taglineNoConfigure: "View topic monitors that provide various aggregated statistics about topic strings."
			},
			views: {
				conntimeWorst: "Newest connections",
				conntimeBest: "Oldest connections",
				tputMsgWorst: "Lowest throughput (messages)",
				tputMsgBest: "Highest throughput (messages)",
				tputBytesWorst: "Lowest throughput (KB)",
				tputBytesBest: "Highest throughput (KB)",
				activeCount: "Active connections",
				connectCount: "Connections established",
				badCount: "Cumulative failed connections",
				msgCount: "Throughput (msg)",
				bytesCount: "Throughput (bytes)",

				msgBufHighest: "Queues with Most Messages Buffered",
				msgProdHighest: "Queues with Most Messages Produced",
				msgConsHighest: "Queues with Most Messages Consumed",
				numProdHighest: "Queues with Most Producers",
				numConsHighest: "Queues with Most Consumers",
				msgBufLowest: "Queues with Least Messages Buffered",
				msgProdLowest: "Queues with Least Messages Produced",
				msgConsLowest: "Queues with Least Messages Consumed",
				numProdLowest: "Queues with Least Producers",
				numConsLowest: "Queues with Least Consumers",
				BufferedHWMPercentHighestQ: "Queues that have come closest to capacity",
				BufferedHWMPercentLowestQ: "Queues that have stayed furthest from capacity",
				ExpiredMsgsHighestQ: "Queues with Most Messages Expired",
				ExpiredMsgsLowestQ: "Queues with Least Messages Expired",

				
				msgPubHighest: "Topics with Most Messages Published",
				subsHighest: "Topics with Most Subscribers",
				msgRejHighest: "Topics with Most Messages Rejected",
				pubRejHighest: "Topics with Most Failed Publishes",
				msgPubLowest: "Topics with Least Messages Published",
				subsLowest: "Topics with Least Subscribers",
				msgRejLowest: "Topics with Least Messages Rejected",
				pubRejLowest: "Topics with Least Failed Publishes",

				publishedMsgsHighest: "Subscriptions with Most Messages Published",
				bufferedMsgsHighest: "Subscriptions with Most Messages Buffered",
				bufferedPercentHighest: "Subscriptions with Highest % of Messages Buffered",
				rejectedMsgsHighest: "Subscriptions with Most Messages Rejected",
				publishedMsgsLowest: "Subscriptions with Least Messages Published",
				bufferedMsgsLowest: "Subscriptions with Least Messages Buffered",
				bufferedPercentLowest: "Subscriptions with Lowest % of Messages Buffered",
				rejectedMsgsLowest: "Subscriptions with Least Messages Rejected",
				BufferedHWMPercentHighestS: "Subscriptions that have come closest to capacity",
				BufferedHWMPercentLowestS: "Subscriptions that have stayed furthest from capacity",
				ExpiredMsgsHighestS: "Subscriptions with Most Messages Expired",
				ExpiredMsgsLowestS: "Subscriptions with Least Messages Expired",
				DiscardedMsgsHighestS: "Subscriptions with Most Messages Discarded",
				DiscardedMsgsLowestS: "Subscriptions with Least Messages Discarded",
				
				mostCommitOps: "Rules with most commit operations",
				mostRollbackOps: "Rules with most rollback operations",
				mostPersistMsgs: "Rules with most persistent messages",
				mostNonPersistMsgs: "Rules with most nonpersistent messages",
				mostCommitMsgs: "Rules with most committed messages"
			},
			rules: {
				any: "Any",
				imaTopicToMQQueue: "${IMA_PRODUCTNAME_SHORT} topic to MQ queue",
				imaTopicToMQTopic: "${IMA_PRODUCTNAME_SHORT} topic to MQ topic",
				mqQueueToIMATopic: "MQ queue to ${IMA_PRODUCTNAME_SHORT} topic",
				mqTopicToIMATopic: "MQ topic to ${IMA_PRODUCTNAME_SHORT} topic",
				imaTopicSubtreeToMQQueue: "${IMA_PRODUCTNAME_SHORT} topic subtree to MQ queue",
				imaTopicSubtreeToMQTopic: "${IMA_PRODUCTNAME_SHORT} topic subtree to MQ topic",
				imaTopicSubtreeToMQTopicSubtree: "${IMA_PRODUCTNAME_SHORT} topic subtree to MQ topic subtree",
				mqTopicSubtreeToIMATopic: "MQ topic subtree to ${IMA_PRODUCTNAME_SHORT} topic",
				mqTopicSubtreeToIMATopicSubtree: "MQ topic subtree to ${IMA_PRODUCTNAME_SHORT} topic subtree",
				imaQueueToMQQueue: "${IMA_PRODUCTNAME_SHORT} queue to MQ queue",
				imaQueueToMQTopic: "${IMA_PRODUCTNAME_SHORT} queue to MQ topic",
				mqQueueToIMAQueue: "MQ queue to ${IMA_PRODUCTNAME_SHORT} queue",
				mqTopicToIMAQueue: "MQ topic to ${IMA_PRODUCTNAME_SHORT} queue",
				mqTopicSubtreeToIMAQueue: "MQ topic subtree to ${IMA_PRODUCTNAME_SHORT} queue"
			},
			chartLabels: {
				activeCount: "Active Connections",
				connectCount: "Connections established",
				badCount: "Cumulative Failed Connections",
				msgCount: "Throughput (msg/sec)",
				bytesCount: "Throughput (MB/sec)"
			},
			connectionVolume: "Active connections",
			connectionVolumeAxisTitle: "Active",
			connectionRate: "New connections",
			connectionRateAxisTitle: "Changed",
			newConnections: "Connections Opened",
			closedConnections: "Connections Closed",
			timeAxisTitle: "Time",
			pauseButton: "Pause chart updates",
			resumeButton: "Resume chart updates",
			noData: "No Data",
			grid: {				
				name: "Client ID",
				// TRNOTE Do not translate "(Name)"
				nameTooltip: "The connection name. (Name)",
				endpoint: "Endpoint",
				// TRNOTE Do not translate "(Endpoint)"
				endpointTooltip: "The name of the endpoint. (Endpoint)",
				clientIp: "Client IP",
				// TRNOTE Do not translate "(ClientAddr)"
				clientIpTooltip: "The client IP address. (ClientAddr)",
				clientId: "Client ID:",
				userId: "User ID",
				// TRNOTE Do not translate "(UserId)"
				userIdTooltip: "The primary user ID. (UserId)",
				protocol: "Protocol",
				// TRNOTE Do not translate "(Protocol)"
				protocolTooltip: "The name of the protocol. (Protocol)",
				bytesRecv: "Received (KB)",
				// TRNOTE Do not translate "(ReadBytes)"
				bytesRecvTooltip: "The number of bytes read since connection time. (ReadBytes)",
				bytesSent: "Sent (KB)",
				// TRNOTE Do not translate "(WriteBytes)"
				bytesSentTooltip: "The number of bytes written since connection time. (WriteBytes)",
				msgRecv: "Received (messages)",
				// TRNOTE Do not translate "(ReadMsg)"
				msgRecvTooltip: "The number of messages read since connection time. (ReadMsg)",
				msgSent: "Sent (messages)",
				// TRNOTE Do not translate "(WriteMsg)"
				msgSentTooltip: "The number of messages written since connection time. (WriteMsg)",
				bytesThroughput: "Throughput (KB/sec)",
				msgThroughput: "Throughput (msg/sec)",
				connectionTime: "Connection Time (seconds)",
				// TRNOTE Do not translate "(ConnectTime)"
				connectionTimeTooltip: "The number of seconds since the connection was created. (ConnectTime)",
				updateButton: "Update",
				refreshButton: "Refresh",
				deleteClientButton: "Delete Client",
				deleteSubscriptionButton: "Delete Durable Subscription",
				endpointLabel: "Source:",
				listenerLabel: "Endpoint:",
				queueNameLabel: "Queue Name:",
				ruleNameLabel: "Rule Name:",
				queueManagerConnLabel: "Queue Manager Connection:",
				topicNameLabel: "Monitored Topics:",
				topicStringLabel: "Topic String:",
				subscriptionLabel: "Subscription Name:",
				subscriptionTypeLabel: "Subscription Type:",
				refreshingLabel: "Updating...",
				datasetLabel: "Query:",
				ruleTypeLabel: "Rule Type:",
				durationLabel: "Duration:",
				resultsLabel: "Results:",
				allListeners: "All",
				allEndpoints: "(all endpoints) ",
				allSubscriptions: "All",
				metricLabel: "Query:",
				// TRNOTE min is short for minutes
				last10: "Last 10 min",
				// TRNOTE hr is short for hours
				last60: "Last 1 hr",
				// TRNOTE hr is short for hours
				last360: "Last 6 hr",
				// TRNOTE hr is short for hours
				last1440: "Last 24 hr",
				// TRNOTE min is short for minutes
				min5: "5 min",
				// TRNOTE min is short for minutes
				min10: "10 min",
				// TRNOTE min is short for minutes
				min30: "30 min",
				// TRNOTE hr is short for hours
				hr1: "1 hr",
				// TRNOTE hr is short for hours
				hr3: "3 hr",
				// TRNOTE hr is short for hours
				hr6: "6 hr",
				// TRNOTE hr is short for hours
				hr12: "12 hr",
				// TRNOTE hr is short for hours
				hr24: "24 hr",
				totalMessages: "Total Messages",
				bufferedMessages: "Buffered Messages",
				subscriptionProperties: "Subscription Properties",
				commitTransactionButton: "Commit",
				rollbackTransactionButton: "Rollback",
				forgetTransactionButton: "Forget",
				queue: {
					queueName: "Queue Name",
					// TRNOTE Do not translate "(QueueName)"
					queueNameTooltip: "The name of the queue. (QueueName)",
					msgBuf: "Current",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "The number of messages that are buffered on the queue and waiting to be consumed. (BufferedMsgs)",
					msgBufHWM: "Peak",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "The highest number of messages that are buffered on the queue since the server was started or the queue was created, whichever is most recent. (BufferedMsgsHWM)",
					msgBufHWMPercent: "Peak % of Maximum",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "The peak number of buffered messages, as a percentage of the maximum number of messages that can be buffered. (BufferedHWMPercent)",
					perMsgBuf: "Current % of Maximum",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "The number of buffered messages as a percentage of the maximum number of buffered messages. (BufferedPercent)",
					totMsgProd: "Produced",
					// TRNOTE Do not translate "(ProducedMsgs)"
					totMsgProdTooltip: "The number of messages that are sent to the queue since the server was started or the queue was created, whichever is most recent. (ProducedMsgs)",
					totMsgCons: "Consumed",
					// TRNOTE Do not translate "(ConsumedMsgs)"
					totMsgConsTooltip: "The number of messages consumed from the queue since the server was started or the queue was created, whichever is most recent. (ConsumedMsgs)",
					totMsgRej: "Rejected",
					// TRNOTE Do not translate "(RejectedMsgs)"
					totMsgRejTooltip: "The number of messages that are not sent to the queue because the maximum number of buffered messages is reached. (RejectedMsgs)",
					totMsgExp: "Expired",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					totMsgExpTooltip: "The number of messages that expired on the queue since the server was started or the queue was created, whichever is most recent. (ExpiredMsgs)",			
					numProd: "Producers",
					// TRNOTE Do not translate "(Producers)"
					numProdTooltip: "The number of active producers on the queue. (Producers)", 
					numCons: "Consumers",
					// TRNOTE Do not translate "(Consumers)"
					numConsTooltip: "The number of active consumers on the queue. (Consumers)",
					maxMsgs: "Maximum",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgsTooltip: "The maximum number of buffered messages that are allowed on the queue. This value is a guideline, rather than an absolute limit. If the system is running under stress, then the number of buffered messages on a queue might be slightly higher than the MaxMessages value. (MaxMessages)"
				},
				destmapping: {
					ruleName: "Rule Name",
					qmConnection: "Queue Manager Connection",
					commits: "Commits",
					rollbacks: "Rollbacks",
					committedMsgs: "Committed Messages",
					persistentMsgs: "Persistent Messages",
					nonPersistentMsgs: "Nonpersistent Messages",
					status: "Status",
					// TRNOTE Do not translate "(RuleName)"
					ruleNameTooltip: "The name of the destination mapping rule that is monitored. (RuleName)",
					// TRNOTE Do not translate "(QueueManagerConnection)"
					qmConnectionTooltip: "The name of the queue manager connection that the destination mapping rule is associated with. (QueueManagerConnection)",
					// TRNOTE Do not translate "(CommitCount)"
					commitsTooltip: "The number of commit operations that are completed for the destination mapping rule. One commit operation can commit many messages. (CommitCount)",
					// TRNOTE Do not translate "(RollbackCount)"
					rollbacksTooltip: "The number of rollback operations that are completed for the destination mapping rule. (RollbackCount)",
					// TRNOTE Do not translate "(CommittedMessageCount)"
					committedMsgsTooltip: "The number of messages committed for the destination mapping rule. (CommittedMessageCount)",
					// TRNOTE Do not translate "(PersistedCount)"
					persistentMsgsTooltip: "The number of persistent messages that are sent by using the destination mapping rule. (PersistentCount)",
					// TRNOTE Do not translate "(NonpersistentCount)"
					nonPersistentMsgsTooltip: "The number of nonpersistent messages that are sent by using the destination mapping rule. (NonpersistentCount)",
					// TRNOTE Do not translate "(Status)"
					statusTooltip: "The status of the rule: Enabled, Disabled, Reconnecting, or Restarting. (Status)"
				},
				mqtt: {
					clientId: "Client ID",
					lastConn: "Last Connection Date",
					// TRNOTE Do not translate "(ClientId)"
					clientIdTooltip: "The Client ID. (ClientId)",
					// TRNOTE Do not translate "(LastConnectedTime)"
					lastConnTooltip: "The time at which the client last connected to the ${IMA_SVR_COMPONENT_NAME}. (LastConnectedTime)",	
					errorMessage: "Reason",
					errorMessageTooltip: "The reason the client could not be deleted."
				},
				subscription: {
					subsName: "Name",
					// TRNOTE Do not translate "(SubName)"
					subsNameTooltip: "The name that is associated with the subscription. This value can be an empty string for a non-durable subscription. (SubName)",
					topicString: "Topic String",
					// TRNOTE Do not translate "(TopicString)"
					topicStringTooltip: "The topic on which the subscription is subscribed. (TopicString)",					
					clientId: "Client ID",
					// TRNOTE Do not translate "(ClientID)"
					clientIdTooltip: "The Client ID of the client that owns the subscription. (ClientID)",
					consumers: "Consumers",
					// TRNOTE Do not translate "(Consumers)"
					consumersTooltip: "The number of consumers that the subscription has. (Consumers)",
					msgPublished: "Published",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPublishedTooltip: "The number of messages that are published to this subscription since the server was started or the subscription was created, whichever is most recent. (PublishedMsgs)",
					msgBuf: "Current",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "The number of published messages that are waiting to be sent to the client. (BufferedMsgs)",
					perMsgBuf: "Current % of Maximum",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "The percentage of the maximum buffered messages that the current buffered messages represent. (BufferedPercent)",
					msgBufHWM: "Peak",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "The highest number of buffered messages since the server was started or the subscription was created, whichever is most recent. (BufferedMsgsHWM)",
					msgBufHWMPercent: "Peak % of Maximum",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "The peak number of buffered messages, as a percentage of the maximum number of messages that can be buffered. (BufferedHWMPercent)",
					maxMsgBuf: "Maximum",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgBufTooltip: "The maximum number of buffered messages that are allowed for this subscription. (MaxMessages)",
					rejMsg: "Rejected",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "The number of messages that are rejected because the maximum number of buffered messages was reached when the messages were published to the subscription. (RejectedMsgs)",
					expDiscMsg: "Expired / Discarded",
					// TRNOTE Do not translate "(ExpiredMsgs + DiscardedMsgs)"
					expDiscMsgTooltip: "The number of messages that are not delivered because they either expired or were discarded when the buffer became full. (ExpiredMsgs + DiscardedMsgs)",
					expMsg: "Expired",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					expMsgTooltip: "The number of messages that are not delivered because they expired. (ExpiredMsgs)",
					DiscMsg: "Discarded",
					// TRNOTE Do not translate "(DiscardedMsgs)"
					DiscMsgTooltip: "The number of messages that are not delivered because they were discarded when the buffer became full. (DiscardedMsgs)",
					subsType: "Type",
					// TRNOTE Do not translate "(IsDurable, IsShared)"
					subsTypeTooltip: "This column indicates whether the subscription is durable or non-durable and whether the subscription is shared. Durable subscriptions survive server restarts, unless the subscription is explicitly deleted. (IsDurable, IsShared)",				
					isDurable: "Durable",
					isShared: "Shared",
					notDurable: "Non-Durable",
					noName: "(none)",
					// TRNOTE {0} and {1} contain a formatted integer number greater than or equal to zero, e.g. 5,250
					expDiscMsgCellTitle: "Expired: {0}; Discarded: {1}",
					errorMessage: "Reason",
					errorMessageTooltip: "The reason the subscripton could not be deleted.",
					messagingPolicy: "Messaging Policy",
					// TRNOTE Do not translate "(MessagingPolicy)"
					messagingPolicyTooltip: "The name of the messaging policy in use by the subscription. (MessagingPolicy)"						
				},
				transaction: {
					// TRNOTE Do not translate "(XID)"
					xId: "Transaction ID (XID)",
					timestamp: "Timestamp",
					state: "State",
					stateCode2: "Prepared",
					stateCode5: "Heuristically Committed",
					stateCode6: "Heuristically Rolled Back"
				},
				topic: {
					topicSubtree: "Topic String",
					msgPub: "Messages Published",
					rejMsg: "Messages Rejected",
					rejPub: "Failed Publishes",
					numSubs: "Subscribers",
					// TRNOTE Do not translate "(TopicString)"
					topicSubtreeTooltip: "The topic that is being monitored. The topic string always contains a wildcard. (TopicString)",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPubTooltip: "The number of messages that are successfully published to a topic that matches the wildcarded topic string. (PublishedMsgs)",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "The number of messages that are rejected by one or more subscriptions where the quality of service level did not cause the publish request to fail. (RejectedMsgs)",
					// TRNOTE Do not translate "(FailedPublishes)"
					rejPubTooltip: "The number of publish requests that failed because the message is rejected by one or more subscriptions. (FailedPublishes)",
					// TRNOTE Do not translate "(Subscriptions)"
					numSubsTooltip: "The number of active subscriptions on the topics that are monitored. The figure shows all active subscriptions that match the wildcarded topic string. (Subscriptions)"
				},
				store: {
					name: "Statistic",
					value: "Value",
                    diskUsedPercent: "Disk Used (%)",
                    diskFreeBytes: "Disk Free (MB)",
					persistentMemoryUsedPercent: "Memory Used (%)",
					memoryTotalBytes: "Memory Total (MB)",
					Pool1TotalBytes: "Pool 1 Total (MB)",
					Pool1UsedBytes: "Pool 1 Used (MB)",
					Pool1UsedPercent: "Pool 1 Used (%)",
					Pool1RecordLimitBytes: "Pool 1 Records Limit (MB)", 
					Pool1RecordsUsedBytes: "Pool 1 Records Used (MB)",
                    Pool2TotalBytes: "Pool 2 Total (MB)",
                    Pool2UsedBytes: "Pool 2 Used (MB)",
                    Pool2UsedPercent: "Pool 2 Used (%)"
				},
				memory: {
					name: "Statistic",
					value: "Value",
					memoryTotalBytes: "Total Memory (MB)",
					memoryFreeBytes: "Free Memory (MB)",
					memoryFreePercent: "Free Memory (%)",
					serverVirtualMemoryBytes: "Virtual Memory (MB)",
					serverResidentSetBytes: "Resident Set (MB)",
					messagePayloads: "Message Payloads (MB)",
					publishSubscribe: "Publish Subscribe (MB)",
					destinations: "Destinations (MB)",
					currentActivity: "Current Activity (MB)",
					clientStates: "Client States (MB)"
				}
			},
			logs: {
				title: "Logs",
				pageSummary: "", 
				downloadLogsSubTitle: "Download Logs",
				downloadLogsTagline: "Download logs for diagnostic purposes.",
				lastUpdated: "Last Updated",
				name: "Log File",
				size: "Size (bytes)"
			},
			lastUpdated: "Last Updated: ",
			bufferedMsgs: "Buffered Messages:",
			retainedMsgs: "Retained Messages:",
			dataCollectionInterval: "Data Collection Interval: ",
			interval: {fiveSeconds: "5 seconds", oneMinute: "1 minute" },
			cacheInterval: "Data Collection Interval: 1 minute",
			notAvailable: "n/a",
			savingProgress: "Saving...",
			savingFailed: "Save failed.",
			noRecord: "Couldn't find record at server.",
			deletingProgress: "Deleting...",
			deletingFailed: "Delete failed.",
			deletePending: "Delete pending.",
			deleteSuccessful: "Delete successful.",
			testingProgress: "Testing...",
			dialog: {
				// TRNOTE Do not translate "$SYS"
				addTopicMonitorInstruction: "Specify the topic string to monitor. The string cannot start with $SYS and must end with a multi-level wildcard (for example,  /animals/dogs/#). " +
						                    "To aggregate statistics for all topics, specify a single multi-level wildcard (#). " +
						                    "If a topic has no child topics, the monitor will show data only for that topic.  " +
						                    "For example, /animals/dogs/labradors/# will monitor only the topic /animals/dogs/labradors if labradors has no child topics.",
				addTopicMonitorTitle: "Add Topic Monitor",
				removeTopicMonitorTitle: "Delete Topic Monitor",
				removeSubscriptionTitle: "Delete Durable Subscription",
				removeSubscriptionsTitle: "Delete Durable Subscriptions",
				removeSubscriptionContent: "Are you sure you want to delete this subscription?",
				removeSubscriptionsContent: "Are you sure you want to delete {0} subscriptions?",
				removeClientTitle: "Delete MQTT Client",
				removeClientsTitle: "Delete MQTT Clients",
				removeClientContent: "Are you sure you want to delete this client? Deleting the client will also delete subscriptions for that client.",
				removeClientsContent: "Are you sure you want to delete {0} clients? Deleting the clients will also delete subscriptions for those clients.",
				topicStringLabel: "Topic String:",
				descriptionLabel: "Description",
				topicStringTooltip: "The topic string to monitor.  Except for the required ending wildcard (#), the following characters are not allowed within the topic string: + #",
				saveButton: "Save",
				cancelButton: "Cancel"
			},
			removeDialog: {
				title: "Delete entry",
				content: "Are you sure you want to delete this record?",
				deleteButton: "Delete",
				cancelButton: "Cancel"
			},
			actionConfirmDialog: {
				titleCommit: "Commit transaction",
				titleRollback: "Rollback transaction",
				titleForget: "Forget transaction",
				contentCommit: "Are you sure you want to commit this transaction?",
				contentRollback: "Are you sure you want to rollback this transaction?",
				contentForget: "Are you sure you want to forget this transaction?",
				actionButtonCommit: "Commit",
				actionButtonRollback: "Rollback",
				actionButtonForget: "Forget",
				cancelButton: "Cancel",
				failedRollback: "There was an error when attempting to rollback the transaction.",
				failedCommit: "There was an error when attempting to commit the transaction.",
				failedForget: "There was an error when attempting to forget the transaction."
			},
			asyncInfoDialog: {
				title: "Delete Pending",
				content: "The subscription could not be deleted immediately. It will be deleted as soon as possible.",
				closeButton: "Close"
			},
			deleteResultsDialog: {
				title: "Subscription Delete Status",
				clientsTitle: "Client Delete Status",
				allSuccessful:  "All subscriptions were successfully deleted.",
				allClientsSuccessful: "All clients were successfully deleted.",
				pendingTitle: "Delete Pending",
				pending: "The following subscriptions could not be deleted immediately. They will be deleted as soon as possible:",
				clientsPending: "The following clients could not be deleted immediately. They will be deleted as soon as possible:",
				failedTitle: "Delete Failed",
				failed:  "The following subscriptions could not be deleted:",
				clientsFailed: "The following clients could not be deleted:",
				closeButton: "Close"					
			},
			invalidName: "A record with that name already exists.",
			// TRNOTE Do not translate "$SYS"
			reservedName: "The topic string cannot begin with $SYS",
			invalidChars: "The following characters are not allowed: + #",			
			invalidTopicMonitorFormat: "The topic string must be #, or end with /#",
			invalidRequired: "A value is required.",
			clientIdMissingMessage: "This value is required. You can use one or more asterisks (*) as wildcards.",
			ruleNameMissingMessage: "This value is required. You can use one or more asterisks (*) as wildcards.",
			queueNameMissingMessage: "This value is required. You can use one or more asterisks (*) as wildcards.",
			subscriptionNameMissingMessage: "This value is required. You can use one or more asterisks (*) as wildcards.",
			discardOldestMessagesInfo: "At least one enabled endpoint is using a messaging policy with a Max Messages Behavior of <em>Discard Old Messages</em>.  Messages that are discarded do not affect the <em>Messages Rejected</em> or <em>Failed Publishes</em> counts.",
			// Strings for viewing and modifying SNMP Configuration
			snmp: {
				title: "SNMP Settings",
				tagline: "Enable community-based Simple Network Management Protocol (SNMPv2C), download Management Information Base (MIB) files, and configure SNMPv2C communities. " +
						"${IMA_PRODUCTNAME_FULL} supports SNMPv2C.",
				status: {
					title: "SNMP Status",
					tagline: {
						disabled: "SNMP is not enabled.",
						enabled: "SNMP is enabled and configured.",
						warning: "An SNMP community must be defined to allow access to the SNMP data on your server."
					},
					enableLabel: "Enable SNMPv2C Agent",
					addressLabel: "SNMP Agent Address:",					
					editLinkLabel: "Edit",
					editAddressDialog: {
						title: "Edit SNMP Agent Address",
						instruction: "Specify the static IP address that ${IMA_PRODUCTNAME_FULL} can use for SNMP. " +
								"To allow any IP address that is configured on the ${IMA_SVR_COMPONENT_NAME}, or if your server is using only dynamic IP addresses, select <em>All</em>."
					},
					getSnmpEnabledError: "An error occurred while retrieving the SNMP enabled state.",
					setSnmpEnabledError: "An error occurred while setting the SNMP enabled state.",
					getSnmpAgentAddressError: "An error occurred while retrieving the SNMP agent address.",
					setSnmpAgentAddressError: "An error occurred while setting the SNMP agent address."
				},
				mibs: {
					title: "Enterprise MIBs",
					tagline: "The Enterprise MIB files describe what functions and data are available from the embedded SNMP agent so that your client can appropriately access them.",
					messageSightLinkLabel: "${IMA_PRODUCTNAME_SHORT} MIB",
					hwNotifyLinkLabel: "HW Notification MIB"
				},
				communities: {
					title: "SNMP Communities",
					tagline: "You can define access to the SNMP data on your server by creating one or more SNMPv2C communities. " +
							"An SNMP community is required when monitoring is enabled. " +
							"All communities will have read-only access.",
					grid: {
						nameColumn: "Name",
						hostRestrictionColumn: "Host Restriction"
					},
					dialog: {
						addTitle: "Add SNMP Community",
						editTitle: "Edit SNMP Community",
						instruction: "An SNMP community allows access to the SNMP data on your server.",
						deleteTitle: "Delete SNMP Community",
						deletePrompt: "Are you sure that you want to delete this SNMP community?",
						deleteNotAllowedTitle: "Remove not Allowed", 
						deleteNotAllowedMessage: "The SNMP community cannot be removed because it is in use by one or more trap subscribers. " +
								"Remove the SNMP community from all trap subscribers, then try again.",
						defineTitle: "Define an SNMP Community",
						defineInstruction: "An SNMP community must be defined to allow access to the SNMP data on your server.",
						nameLabel: "Name",
						nameInvalid: "The community string must not contain quotes.",
						hostRestrictionLabel: "Host Restriction",
						hostRestrictionTooltip: "When specified, restricts access to the specified hosts. " +
								"If you leave this field empty, then you allow communication with all IP addresses. " +
								"Specify a comma-separated list of host names or addresses. " +
								"To specify a subnet, use a Classless Inter-Domain Routing (CIDR) IP address or host name.",
						hostRestrictionInvalid: "The host restriction must be empty or specify a comma-separated list containing valid IP addresses, host names, or domains."
					},
					getCommunitiesError: "An error occurred while retrieving SNMP communities.",
					saveCommunityError: "An error occurred while saving the SNMP community.",
					deleteCommunityError: "An error occurred while deleting the SNMP community."					
				},
				trapSubscribers: {
					title: "Trap Subscribers",
					tagline: "Trap subscribers represent the SNMP clients that are used to communicate with the SNMP agent embedded on the server. " +
							"SNMP trap subscribers must be existing SNMP clients. " +
							"Configure your SNMP client to receive traps before you create the trap subscriber. ",
					grid: {
						hostColumn: "Host",
						portColumn: "Client Port Number",
						communityColumn: "Community"
					},
					dialog: {
						addTitle: "Add Trap Subscriber",
						editTitle: "Edit Trap Subscriber",
						instruction: "A trap subscriber represents an SNMP client that is used to communicate with the SNMP agent embedded on the server.",
						deleteTitle: "Delete Trap Subscriber",
						deletePrompt: "Are you sure that you want to delete this trap subscriber?",
						hostLabel: "Host",
						hostTooltip: "The IP address where the SNMP client listens for trap information.",
						hostInvalid: "A valid IP address is required.",
				        duplicateHost: "A trap subscriber for that host already exists.",
						portLabel: "Client Port Number",
						portTooltip: "The port on which the SNMP client is listening for trap information. " +
								"Valid ports are between 1 and 65535, inclusive.",
						portInvalid: "The port must be a number between 1 and 65535, inclusive.",
						communityLabel: "Community",
	                    noCommunitiesTitle: "No Communities",
	                    noCommunitiesDetails: "A trap subscriber cannot be defined without first defining a community."						
					},
					getTrapSubscribersError: "An error occurred while retrieving SNMP trap subscribers.",
					saveTrapSubscriberError: "An error occurred while saving the SNMP trap subscriber.",
					deleteTrapSubscriberError: "An error occurred while deleting the SNMP trap subscriber."										
				},
				okButton: "OK",
				saveButton: "Save",
				cancelButton: "Cancel",
				closeButton: "Close"
			}
		}
	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
