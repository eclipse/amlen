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

		// ------------------------------------------------------------------------
		// Monitoring
		// ------------------------------------------------------------------------
		monitoring : {
			title : "监视",
			monitoringNotAvailable: "监视由于 ${IMA_PRODUCTNAME_SHORT} 服务器的状态而不可用。",
			monitoringError:"检索监视数据时出错。",
			pool1LimitReached: "池 1 使用率过高。必须拒绝发布和预订。",
			connectionStats: {
				title: "连接监视器",
				tagline: "监视汇总连接数据，并在多个连接统计信息中查询最佳连接和最差连接。",
		 		charts: {
					sectionTitle: "连接图表",
					tagline: "监视到服务器的活动连接和新连接的数目。要暂停图表更新，请单击位于图表下方的按钮。",
					activeConnectionsTitle: "活动连接的快照，大约每 5 秒钟创建一次。",
					newConnectionsTitle: "新连接和已关闭连接的快照，大约每 5 秒钟创建一次。"
				},
		 		grids: {
					sectionTitle: "连接数据",
					tagline: "查看特定端点和统计信息的最佳或最差连接。" +
							"最多可查看 50 条连接。监视数据高速缓存在服务器上，该高速缓存每分钟刷新一次。" +
							"因此，数据最多一分钟后过期。"
				}
			},
			endpointStats: {
				title: "端点监视器",
				tagline: "以时间序列图表或汇总数据的形式监视个别端点的历史性能。",
		 		charts: {
					sectionTitle: "端点图表",
					tagline: "查看特定端点、统计信息和持续时间的时间序列图表。" +
						"如果持续时间不超过 1 小时，那么图表将基于每 5 秒钟创建一次快照。" +
						"对于更长的持续时间，图表将基于每分钟创建一次快照。"
				},
		 		grids: {
					sectionTitle: "端点数据",
					tagline: "查看个别端点的汇总数据。监视数据高速缓存在服务器上，该高速缓存每分钟刷新一次。" +
							 "因此，数据最多一分钟后过期。"
				}
			},
			queueMonitor: {
				title: "队列监视器",
				tagline: "使用各种队列统计信息监视个别队列。最多可查看 100 个队列。"
			},
			mqttClientMonitor: {
				title: "断开连接的 MQTT 客户机",
				tagline: "搜索和删除未连接的 MQTT 客户机。最多可查看 100 个 MQTT 客户机。",
				taglineNoDelete: "搜索未连接的 MQTT 客户机。最多可查看 100 个 MQTT 客户机。"
			},
			subscriptionMonitor: {
				title: "预订监视器",
				tagline: "使用各种预订统计信息监视预订。删除持久预订。最多可查看 100 个预订。",
				taglineNoDelete: "使用各种预订统计信息监视预订。最多可查看 100 个预订。"
			},
			transactionMonitor: {
				title: "事务监视器",
				tagline: "监视由外部事务管理器协调的 XA 事务。"
			},
			destinationMappingRuleMonitor: {
				title: "MQ Connectivity 目标映射规则监视器",
				tagline: "监视目标映射规则统计信息。统计信息是针对目标映射规则/队列管理器连接对。最多可查看 100 个对。"
			},
			applianceMonitor: {
				title: "服务器监视器",
				tagline: "收集各种 ${IMA_PRODUCTNAME_FULL} 统计信息的相关信息。",
				storeMonitorTitle: "持久存储",
				storeMonitorTagline: "监视 ${IMA_PRODUCTNAME_FULL} 持久存储所用资源的统计信息。",
				memoryMonitorTitle: "内存",
				memoryMonitorTagline: "监视其他 ${IMA_PRODUCTNAME_FULL} 进程所用内存的统计信息。"
			},
			topicMonitor: {
				title: "主题监视器",
				tagline: "配置提供有关主题字符串的各种汇总统计信息的主题监视器。",
				taglineNoConfigure: "查看提供有关主题字符串的各种汇总统计信息的主题监视器。"
			},
			views: {
				conntimeWorst: "最新连接数",
				conntimeBest: "最旧连接数",
				tputMsgWorst: "最低吞吐量（消息数）",
				tputMsgBest: "最高吞吐量（消息数）",
				tputBytesWorst: "最低吞吐量 (KB)",
				tputBytesBest: "最高吞吐量 (KB)",
				activeCount: "活动连接数",
				connectCount: "已建立的连接数",
				badCount: "累计失败的连接数",
				msgCount: "吞吐量（消息数）",
				bytesCount: "吞吐量（字节数）",

				msgBufHighest: "缓冲消息数最多的队列",
				msgProdHighest: "生产消息数最多的队列",
				msgConsHighest: "使用消息数最多的队列",
				numProdHighest: "生产者数最多的队列",
				numConsHighest: "使用者数最多的队列",
				msgBufLowest: "缓冲消息数最少的队列",
				msgProdLowest: "生产消息数最少的队列",
				msgConsLowest: "使用消息数最少的队列",
				numProdLowest: "生产者数最少的队列",
				numConsLowest: "使用者数最少的队列",
				BufferedHWMPercentHighestQ: "与容量相差最近的队列",
				BufferedHWMPercentLowestQ: "与容量相差最远的队列",
				ExpiredMsgsHighestQ: "到期消息数最多的队列",
				ExpiredMsgsLowestQ: "到期消息数最少的队列",

				msgPubHighest: "发布消息数最多的主题",
				subsHighest: "订户数最多的主题",
				msgRejHighest: "拒绝消息数最多的主题",
				pubRejHighest: "发布失败数最多的主题",
				msgPubLowest: "发布消息数最少的主题",
				subsLowest: "订户数最少的主题",
				msgRejLowest: "拒绝消息数最少的主题",
				pubRejLowest: "发布失败数最少的主题",

				publishedMsgsHighest: "发布消息数最多的预订",
				bufferedMsgsHighest: "缓冲消息数最多的预订",
				bufferedPercentHighest: "缓冲消息数百分比最高的预订",
				rejectedMsgsHighest: "拒绝消息数最多的预订",
				publishedMsgsLowest: "发布消息数最少的预订",
				bufferedMsgsLowest: "缓冲消息数最少的预订",
				bufferedPercentLowest: "缓冲消息数百分比最低的预订",
				rejectedMsgsLowest: "拒绝消息数最少的预订",
				BufferedHWMPercentHighestS: "与容量相差最近的预订",
				BufferedHWMPercentLowestS: "与容量相差最远的预订",
				ExpiredMsgsHighestS: "到期消息数最多的预订",
				ExpiredMsgsLowestS: "到期消息数最少的预订",
				DiscardedMsgsHighestS: "丢弃消息数最多的预订",
				DiscardedMsgsLowestS: "丢弃消息数最少的预订",
				mostCommitOps: "落实操作数最多的规则",
				mostRollbackOps: "回滚操作数最多的规则",
				mostPersistMsgs: "持久消息数最多的规则",
				mostNonPersistMsgs: "非持久消息数最多的规则",
				mostCommitMsgs: "落实消息数最多的规则"
			},
			rules: {
				any: "任何",
				imaTopicToMQQueue: "${IMA_PRODUCTNAME_SHORT} 主题到 MQ 队列",
				imaTopicToMQTopic: "${IMA_PRODUCTNAME_SHORT} 主题到 MQ 主题",
				mqQueueToIMATopic: "MQ 队列到 ${IMA_PRODUCTNAME_SHORT} 主题",
				mqTopicToIMATopic: "MQ 主题到 ${IMA_PRODUCTNAME_SHORT} 主题",
				imaTopicSubtreeToMQQueue: "${IMA_PRODUCTNAME_SHORT} 主题子树到 MQ 队列",
				imaTopicSubtreeToMQTopic: "${IMA_PRODUCTNAME_SHORT} 主题子树到 MQ 主题",
				imaTopicSubtreeToMQTopicSubtree: "${IMA_PRODUCTNAME_SHORT} 主题子树到 MQ 主题子树",
				mqTopicSubtreeToIMATopic: "MQ 主题子树到 ${IMA_PRODUCTNAME_SHORT} 主题",
				mqTopicSubtreeToIMATopicSubtree: "MQ 主题子树到 ${IMA_PRODUCTNAME_SHORT} 主题子树",
				imaQueueToMQQueue: "${IMA_PRODUCTNAME_SHORT} 队列到 MQ 队列",
				imaQueueToMQTopic: "${IMA_PRODUCTNAME_SHORT} 队列到 MQ 主题",
				mqQueueToIMAQueue: "MQ 队列到 ${IMA_PRODUCTNAME_SHORT} 队列",
				mqTopicToIMAQueue: "MQ 主题到 ${IMA_PRODUCTNAME_SHORT} 队列",
				mqTopicSubtreeToIMAQueue: "MQ 主题子树到 ${IMA_PRODUCTNAME_SHORT} 队列"
			},
			chartLabels: {
				activeCount: "活动连接数",
				connectCount: "已建立的连接数",
				badCount: "累计失败的连接数",
				msgCount: "吞吐量（消息数/秒）",
				bytesCount: "吞吐量（MB/秒）"
			},
			connectionVolume: "活动连接数",
			connectionVolumeAxisTitle: "活动",
			connectionRate: "新连接数",
			connectionRateAxisTitle: "已更改",
			newConnections: "已打开的连接数",
			closedConnections: "已关闭的连接数",
			timeAxisTitle: "时间",
			pauseButton: "暂停图表更新",
			resumeButton: "恢复图表更新",
			noData: "无数据",
			grid: {				
				name: "客户机标识",
				// TRNOTE Do not translate "(Name)"
				nameTooltip: "连接名称。(Name)",
				endpoint: "端点",
				// TRNOTE Do not translate "(Endpoint)"
				endpointTooltip: "端点名称。(Endpoint)",
				clientIp: "客户机 IP",
				// TRNOTE Do not translate "(ClientAddr)"
				clientIpTooltip: "客户机 IP 地址。(ClientAddr)",
				clientId: "客户机标识：",
				userId: "用户标识",
				// TRNOTE Do not translate "(UserId)"
				userIdTooltip: "主用户标识。(UserId)",
				protocol: "协议",
				// TRNOTE Do not translate "(Protocol)"
				protocolTooltip: "协议名称。(Protocol)",
				bytesRecv: "已接收 (KB)",
				// TRNOTE Do not translate "(ReadBytes)"
				bytesRecvTooltip: "自连接建立起读取的字节数。(ReadBytes)",
				bytesSent: "已发送 (KB)",
				// TRNOTE Do not translate "(WriteBytes)"
				bytesSentTooltip: "自连接建立起写入的字节数。(WriteBytes)",
				msgRecv: "已接收（消息数）",
				// TRNOTE Do not translate "(ReadMsg)"
				msgRecvTooltip: "自连接建立起读取的消息数。(ReadMsg)",
				msgSent: "已发送（消息数）",
				// TRNOTE Do not translate "(WriteMsg)"
				msgSentTooltip: "自连接建起写入的消息数。(WriteMsg)",
				bytesThroughput: "吞吐量（KB/秒）",
				msgThroughput: "吞吐量（消息数/秒）",
				connectionTime: "连接时间（秒）",
				// TRNOTE Do not translate "(ConnectTime)"
				connectionTimeTooltip: "自创建连接起的秒数。(ConnectTime)",
				updateButton: "更新",
				refreshButton: "刷新",
				deleteClientButton: "删除客户机",
				deleteSubscriptionButton: "删除持久预订",
				endpointLabel: "源：",
				listenerLabel: "端点：",
				queueNameLabel: "队列名称：",
				ruleNameLabel: "规则名称：",
				queueManagerConnLabel: "队列管理器连接：",
				topicNameLabel: "监视的主题数：",
				topicStringLabel: "主题字符串：",
				subscriptionLabel: "预订名称：",
				subscriptionTypeLabel: "预订类型：",
				refreshingLabel: "正在更新...",
				datasetLabel: "查询：",
				ruleTypeLabel: "规则类型：",
				durationLabel: "持续时间：",
				resultsLabel: "结果：",
				allListeners: "全部",
				allEndpoints: "（所有端点）",
				allSubscriptions: "全部",
				metricLabel: "查询：",
				// TRNOTE min is short for minutes
				last10: "过去 10 分钟",
				// TRNOTE hr is short for hours
				last60: "过去 1 小时",
				// TRNOTE hr is short for hours
				last360: "过去 6 小时",
				// TRNOTE hr is short for hours
				last1440: "过去 24 小时",
				// TRNOTE min is short for minutes
				min5: "5 分钟",
				// TRNOTE min is short for minutes
				min10: "10 分钟",
				// TRNOTE min is short for minutes
				min30: "30 分钟",
				// TRNOTE hr is short for hours
				hr1: "1 小时",
				// TRNOTE hr is short for hours
				hr3: "3 小时",
				// TRNOTE hr is short for hours
				hr6: "6 小时",
				// TRNOTE hr is short for hours
				hr12: "12 小时",
				// TRNOTE hr is short for hours
				hr24: "24 小时",
				totalMessages: "消息总数",
				bufferedMessages: "已缓冲的消息数",
				subscriptionProperties: "预订属性",
				commitTransactionButton: "落实",
				rollbackTransactionButton: "回滚",
				forgetTransactionButton: "忘记",
				queue: {
					queueName: "队列名称",
					// TRNOTE Do not translate "(QueueName)"
					queueNameTooltip: "队列的名称。(QueueName)",
					msgBuf: "当前",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "队列上已缓冲并等待使用的消息数。(BufferedMsgs)",
					msgBufHWM: "峰值",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "自启动服务器或创建队列（以最近的操作为准）起队列上缓冲的最大消息数。(BufferedMsgsHWM)",
					msgBufHWMPercent: "最大数的峰值百分比",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "所缓冲的消息的峰值数，占可以缓冲的最大消息数的百分比。(BufferedHWMPercent)",
					perMsgBuf: "最大数的当前百分比",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "缓冲消息数占最大缓冲消息数的百分比。(BufferedPercent)",
					totMsgProd: "已生产",
					// TRNOTE Do not translate "(ProducedMsgs)"
					totMsgProdTooltip: "自启动服务器或创建队列（以最近的操作为准）起发送至队列的消息数。(ProducedMsgs)",
					totMsgCons: "已使用",
					// TRNOTE Do not translate "(ConsumedMsgs)"
					totMsgConsTooltip: "自启动服务器或创建队列（以最近的操作为准）起队列使用的消息数。(ConsumedMsgs)",
					totMsgRej: "已拒绝",
					// TRNOTE Do not translate "(RejectedMsgs)"
					totMsgRejTooltip: "由于已达到最大缓冲消息数而未发送至队列的消息数。(RejectedMsgs)",
					totMsgExp: "已到期",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					totMsgExpTooltip: "自启动服务器或创建队列（以最近的操作为准）起在队列上到期的消息数。(ExpiredMsgs)",			
					numProd: "生产者数",
					// TRNOTE Do not translate "(Producers)"
					numProdTooltip: "队列上的活动生产者数。(Producers)", 
					numCons: "使用者数",
					// TRNOTE Do not translate "(Consumers)"
					numConsTooltip: "队列上的活动使用者数。(Consumers)",
					maxMsgs: "最大值",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgsTooltip: "队列上允许的最大缓冲消息数。该值仅作为指导原则，而不是绝对限制。如果系统在运行时承受了压力，那么队列中的缓冲消息数可能会比 MaxMessages 值略高。(MaxMessages)"
				},
				destmapping: {
					ruleName: "规则名称",
					qmConnection: "队列管理器连接",
					commits: "落实数",
					rollbacks: "回滚数",
					committedMsgs: "已落实的消息数",
					persistentMsgs: "持久消息数",
					nonPersistentMsgs: "非持久消息数",
					status: "状态",
					// TRNOTE Do not translate "(RuleName)"
					ruleNameTooltip: "监视的目标映射规则的名称。(RuleName)",
					// TRNOTE Do not translate "(QueueManagerConnection)"
					qmConnectionTooltip: "目标映射规则关联的队列管理器连接的名称。(QueueManagerConnection)",
					// TRNOTE Do not translate "(CommitCount)"
					commitsTooltip: "针对目标映射规则完成的落实操作数。一项落实操作可落实多条消息。(CommitCount)",
					// TRNOTE Do not translate "(RollbackCount)"
					rollbacksTooltip: "针对目标映射规则完成的回滚操作数。(RollbackCount)",
					// TRNOTE Do not translate "(CommittedMessageCount)"
					committedMsgsTooltip: "针对目标映射规则落实的消息数。(CommittedMessageCount)",
					// TRNOTE Do not translate "(PersistedCount)"
					persistentMsgsTooltip: "使用目标映射规则发送的持久消息数。(PersistentCount)",
					// TRNOTE Do not translate "(NonpersistentCount)"
					nonPersistentMsgsTooltip: "使用目标映射规则发送的非持久消息数。(NonpersistentCount)",
					// TRNOTE Do not translate "(Status)"
					statusTooltip: "规则的状态：已启用、已禁用、正在重新连接或正在重新启动。(Status)"
				},
				mqtt: {
					clientId: "客户机标识",
					lastConn: "上次连接日期",
					// TRNOTE Do not translate "(ClientId)"
					clientIdTooltip: "客户机标识。(ClientId)",
					// TRNOTE Do not translate "(LastConnectedTime)"
					lastConnTooltip: "客户机上次连接到 ${IMA_PRODUCTNAME_SHORT} 服务器的时间。(LastConnectedTime)",	
					errorMessage: "原因",
					errorMessageTooltip: "无法删除客户机的原因。"
				},
				subscription: {
					subsName: "姓名",
					// TRNOTE Do not translate "(SubName)"
					subsNameTooltip: "与预订关联的名称。对于非持久预订，该值可以是空字符串。(SubName)",
					topicString: "主题字符串",
					// TRNOTE Do not translate "(TopicString)"
					topicStringTooltip: "预订此预订的主题。(TopicString)",					
					clientId: "客户机标识",
					// TRNOTE Do not translate "(ClientID)"
					clientIdTooltip: "拥有预订的客户机的客户机标识。(ClientID)",
					consumers: "使用者数",
					// TRNOTE Do not translate "(Consumers)"
					consumersTooltip: "预订所具有的使用者数。(Consumers)",
					msgPublished: "已发布",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPublishedTooltip: "自启动服务器或创建预订（以最近的操作为准）起发布至此预订的消息数。(PublishedMsgs)",
					msgBuf: "当前",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "等待发送至客户机的已发布消息数。(BufferedMsgs)",
					perMsgBuf: "最大数的当前百分比",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "当前缓冲消息数所表示的最大缓冲消息数的百分比。(BufferedPercent)",
					msgBufHWM: "峰值",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "自启动服务器或创建预订（以最近的操作为准）起已缓冲的最大消息数。(BufferedMsgsHWM)",
					msgBufHWMPercent: "最大数的峰值百分比",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "所缓冲的消息的峰值数，占可以缓冲的最大消息数的百分比。(BufferedHWMPercent)",
					maxMsgBuf: "最大值",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgBufTooltip: "针对此预订允许的最大缓冲消息数。(MaxMessages)",
					rejMsg: "已拒绝",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "由于在将消息发布至预订时达到最大缓冲消息数而被拒绝的消息数。(RejectedMsgs)",
					expDiscMsg: "已到期/已丢弃",
					// TRNOTE Do not translate "(ExpiredMsgs + DiscardedMsgs)"
					expDiscMsgTooltip: "由于到期或因缓冲区已满而被丢弃造成的未传递的消息数。(ExpiredMsgs + DiscardedMsgs)",
					expMsg: "已到期",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					expMsgTooltip: "由于到期而未传递的消息数。(ExpiredMsgs)",
					DiscMsg: "已丢弃",
					// TRNOTE Do not translate "(DiscardedMsgs)"
					DiscMsgTooltip: "由于缓冲区变满时被废弃而未传递的消息数。(DiscardedMsgs)",
					subsType: "类型",
					// TRNOTE Do not translate "(IsDurable, IsShared)"
					subsTypeTooltip: "该列指示预订是持久还是非持久的，以及预订是否可以共享。除非明确删除预订，否则在服务器重新启动时可以进行持久预订。(IsDurable, IsShared)",				
					isDurable: "持久",
					isShared: "共享",
					notDurable: "非持久",
					noName: "（无）",
					// TRNOTE {0} and {1} contain a formatted integer number greater than or equal to zero, e.g. 5,250
					expDiscMsgCellTitle: "已到期：{0}；已丢弃：{1}",
					errorMessage: "原因",
					errorMessageTooltip: "无法删除预订的原因。",
					messagingPolicy: "消息传递策略",
					// TRNOTE Do not translate "(MessagingPolicy)"
					messagingPolicyTooltip: "预订正在使用的消息传递策略的名称。(MessagingPolicy)"						
				},
				transaction: {
					// TRNOTE Do not translate "(XID)"
					xId: "事务标识 (XID)",
					timestamp: "时间戳记",
					state: "状态",
					stateCode2: "准备就绪",
					stateCode5: "已试探性落实",
					stateCode6: "已试探性回滚"
				},
				topic: {
					topicSubtree: "主题字符串",
					msgPub: "已发布的消息数",
					rejMsg: "已拒绝的消息数",
					rejPub: "失败的发布数",
					numSubs: "订户数",
					// TRNOTE Do not translate "(TopicString)"
					topicSubtreeTooltip: "正在监视的主题。主题字符串始终包含通配符。(TopicString)",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPubTooltip: "成功发布至与通配主题字符串匹配的主题的消息数。(PublishedMsgs)",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "其服务质量级别未导致发布请求失败的一个或多个预订所拒绝的消息数。(RejectedMsgs)",
					// TRNOTE Do not translate "(FailedPublishes)"
					rejPubTooltip: "由于一个或多个预订拒绝消息而失败的发布请求数。(FailedPublishes)",
					// TRNOTE Do not translate "(Subscriptions)"
					numSubsTooltip: "被监视主题上的活动预订数。该数字显示与通配主题字符串匹配的所有活动的预订。(Subscriptions)"
				},
				store: {
					name: "统计信息",
					value: "值",
                    diskUsedPercent: "已用磁盘空间 (%)",
                    diskFreeBytes: "磁盘可用容量 (MB)",
					persistentMemoryUsedPercent: "已用内存 (%)",
					memoryTotalBytes: "内存总容量 (MB)",
					Pool1TotalBytes: "池 1 总容量 (MB)",
					Pool1UsedBytes: "池 1 已用容量 (MB)",
					Pool1UsedPercent: "池 1 已用百分比 (%)",
					Pool1RecordLimitBytes: "池 1 记录限制 (MB)", 
					Pool1RecordsUsedBytes: "池 1 记录已用容量 (MB)",
                    Pool2TotalBytes: "池 2 总容量 (MB)",
                    Pool2UsedBytes: "池 2 已用容量 (MB)",
                    Pool2UsedPercent: "池 2 已用百分比 (%)"
				},
				memory: {
					name: "统计信息",
					value: "值",
					memoryTotalBytes: "总内存量 (MB)",
					memoryFreeBytes: "可用内存量 (MB)",
					memoryFreePercent: "可用内存量 (%)",
					serverVirtualMemoryBytes: "虚拟内存量 (MB)",
					serverResidentSetBytes: "驻留集 (MB)",
					messagePayloads: "消息有效内容 (MB)",
					publishSubscribe: "发布预订 (MB)",
					destinations: "目标 (MB)",
					currentActivity: "当前活动 (MB)",
					clientStates: "客户机状态 (MB)"
				}
			},
			logs: {
				title: "日志",
				pageSummary: "", 
				downloadLogsSubTitle: "下载日志",
				downloadLogsTagline: "下载日志以用于诊断。",
				lastUpdated: "上次更新时间",
				name: "日志文件",
				size: "大小（字节数）"
			},
			lastUpdated: "上次更新时间：",
			bufferedMsgs: "已缓冲的消息数：",
			retainedMsgs: "已保留的消息数：",
			dataCollectionInterval: "数据收集间隔：",
			interval: {fiveSeconds: "5 秒", oneMinute: "1 分钟" },
			cacheInterval: "数据收集间隔：1 分钟",
			notAvailable: "不适用",
			savingProgress: "正在保存...",
			savingFailed: "保存失败。",
			noRecord: "无法在服务器上找到记录。",
			deletingProgress: "正在删除...",
			deletingFailed: "删除失败。",
			deletePending: "删除暂挂。",
			deleteSuccessful: "删除成功。",
			testingProgress: "正在测试...",
			dialog: {
				// TRNOTE Do not translate "$SYS"
				addTopicMonitorInstruction: "指定要监视的主题字符串。字符串不能以 $SYS 开头，并且必须以多级通配符结尾（例如，/animals/dogs/#）。" +
						                    "要汇总所有主题的统计信息，请指定单个多级通配符 (#)。" +
						                    "如果主题没有子主题，那么监视器将只显示该主题的数据。" +
						                    "例如，如果 labradors 无子主题，那么 /animals/dogs/labradors/# 只能监视主题 /animals/dogs/labradors。",
				addTopicMonitorTitle: "添加主题监视器",
				removeTopicMonitorTitle: "删除主题监视器",
				removeSubscriptionTitle: "删除持久预订",
				removeSubscriptionsTitle: "删除持久预订",
				removeSubscriptionContent: "确定要删除此预订吗？",
				removeSubscriptionsContent: "确定要删除 {0} 个预订吗？",
				removeClientTitle: "删除 MQTT 客户机",
				removeClientsTitle: "删除 MQTT 客户机",
				removeClientContent: "确定要删除该客户机吗？删除该客户机还将删除该客户机的预订。",
				removeClientsContent: "确定要删除 {0} 个客户机吗？删除这些客户机还将删除这些客户机的预订。",
				topicStringLabel: "主题字符串：",
				descriptionLabel: "描述",
				topicStringTooltip: "要监视的主题字符串。除了必需的结束通配符 (#)，在主题字符串内不允许以下字符：+ #",
				saveButton: "保存",
				cancelButton: "取消"
			},
			removeDialog: {
				title: "删除条目",
				content: "确定要删除该记录吗？",
				deleteButton: "删除",
				cancelButton: "取消"
			},
			actionConfirmDialog: {
				titleCommit: "落实事务",
				titleRollback: "回滚事务",
				titleForget: "忘记事务",
				contentCommit: "确定要落实此事务吗？",
				contentRollback: "确定要回滚此事务吗？",
				contentForget: "确定要忘记此事务吗？",
				actionButtonCommit: "落实",
				actionButtonRollback: "回滚",
				actionButtonForget: "忘记",
				cancelButton: "取消",
				failedRollback: "尝试回滚事务时出错。",
				failedCommit: "尝试落实事务时出错。",
				failedForget: "尝试忘记事务时出错。"
			},
			asyncInfoDialog: {
				title: "删除暂挂",
				content: "无法立即删除预订。会尽快将其删除。",
				closeButton: "关闭"
			},
			deleteResultsDialog: {
				title: "预订删除状态",
				clientsTitle: "客户机删除状态",
				allSuccessful:  "已成功删除所有预订。",
				allClientsSuccessful: "已成功删除所有客户机。",
				pendingTitle: "删除暂挂",
				pending: "无法立即删除以下预订。会尽快将其删除：",
				clientsPending: "无法立即删除以下客户机。会尽快将其删除：",
				failedTitle: "删除失败",
				failed:  "无法删除以下预订：",
				clientsFailed: "无法删除以下客户机：",
				closeButton: "关闭"					
			},
			invalidName: "已存在同名记录。",
			// TRNOTE Do not translate "$SYS"
			reservedName: "主题字符串不能以 $SYS 开头",
			invalidChars: "不允许以下字符：+ #",			
			invalidTopicMonitorFormat: "主题字符串必须为 #，或以 /# 结尾",
			invalidRequired: "值是必需的。",
			clientIdMissingMessage: "该值是必需的。可以将一个或多个星号 (*) 作为通配符。",
			ruleNameMissingMessage: "该值是必需的。可以将一个或多个星号 (*) 作为通配符。",
			queueNameMissingMessage: "该值是必需的。可以将一个或多个星号 (*) 作为通配符。",
			subscriptionNameMissingMessage: "该值是必需的。可以将一个或多个星号 (*) 作为通配符。",
			discardOldestMessagesInfo: "至少有一个已启用的端点正在使用“最大消息数行为”为<em>丢弃旧消息</em>的消息传递策略。丢弃的消息不会影响<em>已拒绝的消息数</em>或<em>失败的发布数</em>计数。",
			// Strings for viewing and modifying SNMP Configuration
			snmp: {
				title: "SNMP 设置",
				tagline: "启用基于社区的简单网络管理协议 (SNMPv2C)，下载管理信息库 (MIB) 文件，并配置 SNMPv2C 社区。" +
						"${IMA_PRODUCTNAME_FULL} 支持 SNMPv2C。",
				status: {
					title: "SNMP 状态",
					tagline: {
						disabled: "未启用 SNMP。",
						enabled: "已启用并配置 SNMP。",
						warning: "必须定义 SNMP 社区才允许访问您的服务器上的 SNMP 数据。"
					},
					enableLabel: "启用 SNMPv2C 代理",
					addressLabel: "SNMP 代理地址：",					
					editLinkLabel: "编辑",
					editAddressDialog: {
						title: "编辑 SNMP 代理地址",
						instruction: "指定 ${IMA_PRODUCTNAME_FULL} 可对 SNMP 使用的静态 IP 地址。" +
								"要允许 ${IMA_PRODUCTNAME_SHORT} 服务器上配置的任何 IP 地址，或者如果服务器仅使用动态 IP 地址，请选择<em>全部</em>。"
					},
					getSnmpEnabledError: "检索 SNMP 已启用状态时发生错误。",
					setSnmpEnabledError: "设置 SNMP 已启用状态时发生错误。",
					getSnmpAgentAddressError: "检索 SNMP 代理地址时发生错误。",
					setSnmpAgentAddressError: "设置 SNMP 代理地址时发生错误。"
				},
				mibs: {
					title: "企业 MIB",
					tagline: "企业 MIB 文件描述了嵌入式 SNMP 代理提供的功能和数据，以便您的客户机可以相应地访问这些功能和数据。",
					messageSightLinkLabel: "${IMA_PRODUCTNAME_SHORT} MIB",
					hwNotifyLinkLabel: "HW 通知 MIB"
				},
				communities: {
					title: "SNMP 社区",
					tagline: "您可以通过创建一个或多个 SNMPv2C 社区来定义对服务器上 SNMP 数据的访问。" +
							"启用监视时需要 SNMP 社区。" +
							"所有社区都将具有只读访问权。",
					grid: {
						nameColumn: "姓名",
						hostRestrictionColumn: "主机限制"
					},
					dialog: {
						addTitle: "添加 SNMP 社区",
						editTitle: "编辑 SNMP 社区",
						instruction: "SNMP 社区允许访问您的服务器上的 SNMP 数据。",
						deleteTitle: "删除 SNMP 社区",
						deletePrompt: "确定要删除此 SNMP 社区吗？",
						deleteNotAllowedTitle: "不允许移除", 
						deleteNotAllowedMessage: "由于一个或多个陷阱订户正在使用此 SNMP 社区，因此无法移除此 SNMP 社区。" +
								"请从所有陷阱订户移除此 SNMP 社区，然后重试。",
						defineTitle: "定义 SNMP 社区",
						defineInstruction: "必须定义 SNMP 社区才允许访问您的服务器上的 SNMP 数据。",
						nameLabel: "姓名",
						nameInvalid: "社区字符串不能包含引号。",
						hostRestrictionLabel: "主机限制",
						hostRestrictionTooltip: "指定此选项时，将限制对指定主机的访问。" +
								"如果将此字段留空，那么允许与所有 IP 地址进行通信。" +
								"指定主机名或地址时使用以逗号分隔的列表。" +
								"要指定子网，请使用无类域间路由 (CIDR) IP 地址或主机名。",
						hostRestrictionInvalid: "主机限制必须为空，或者指定包含有效 IP 地址、主机名或域的逗号分隔列表。"
					},
					getCommunitiesError: "检索 SNMP 社区时发生错误。",
					saveCommunityError: "保存 SNMP 社区时发生错误。",
					deleteCommunityError: "删除 SNMP 社区时发生错误。"					
				},
				trapSubscribers: {
					title: "陷阱订户",
					tagline: "陷阱订户表示用于与服务器上嵌入的 SNMP 代理通信的 SNMP 客户机。" +
							"SNMP 陷阱订户必须是现有的 SNMP 客户机。" +
							"在创建陷阱订户之前，请将 SNMP 客户机配置为接收陷阱。",
					grid: {
						hostColumn: "主机",
						portColumn: "客户机端口号",
						communityColumn: "社区"
					},
					dialog: {
						addTitle: "添加陷阱订户",
						editTitle: "编辑陷阱订户",
						instruction: "陷阱订户表示用于与服务器上嵌入的 SNMP 代理通信的 SNMP 客户机。",
						deleteTitle: "删除陷阱订户",
						deletePrompt: "确定要删除此陷阱订户吗？",
						hostLabel: "主机",
						hostTooltip: "SNMP 客户机用来侦听陷阱信息的 IP 地址。",
						hostInvalid: "需要有效 IP 地址。",
				        duplicateHost: "该主机已存在陷阱订户。",
						portLabel: "客户机端口号",
						portTooltip: "SNMP 客户机用来侦听陷阱信息的端口。" +
								"有效的端口在 1 和 65535（包括 1 和 65535）之间。",
						portInvalid: "端口号必须是 1 到 65535（包括 1 和 65535）之间的数字。",
						communityLabel: "社区",
	                    noCommunitiesTitle: "无社区",
	                    noCommunitiesDetails: "必须先定义社区，然后才能定义陷阱订户。"						
					},
					getTrapSubscribersError: "检索 SNMP 陷阱订户时发生错误。",
					saveTrapSubscriberError: "保存 SNMP 陷阱订户时发生错误。",
					deleteTrapSubscriberError: "删除 SNMP 陷阱订户时发生错误。"										
				},
				okButton: "确定",
				saveButton: "保存",
				cancelButton: "取消",
				closeButton: "关闭"
			}
		}
});
