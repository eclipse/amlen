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
			title : "監視",
			monitoringNotAvailable: "因為 ${IMA_PRODUCTNAME_SHORT} 伺服器的狀態而無法使用監視。",
			monitoringError:"擷取監視資料時發生錯誤。",
			pool1LimitReached: "儲存區 1 用量太高。可能會拒絕發佈及訂閱。",
			connectionStats: {
				title: "連線監視器",
				tagline: "透過數個連線統計資料監視聚焦的連線資料，並查詢執行最好及執行最差的連線。",
		 		charts: {
					sectionTitle: "連線圖表",
					tagline: "監視伺服器作用中的連線數及新連線數。若要暫停圖表更新項目，請按一下位於圖表下方的按鈕。",
					activeConnectionsTitle: "作用中連線的 Snapshot，大約每 5 秒鐘拍攝一次。",
					newConnectionsTitle: "新連線及已關閉連線的 Snapshot，大約每 5 秒鐘拍攝一次。"
				},
		 		grids: {
					sectionTitle: "連線資料",
					tagline: "針對特定的端點及統計資料，檢視執行最好或執行最差的連線。" +
							"最多可檢視 50 個連線。監視資料會在伺服器中快取，並且每分鐘重新整理此快取一次。" +
							"因此，資料最多一分鐘即會過期。"
				}
			},
			endpointStats: {
				title: "端點監視器",
				tagline: "以時間序列圖表或聚集資料形式，監視將個別端點的歷程效能。",
		 		charts: {
					sectionTitle: "端點圖表",
					tagline: "檢視特定端點、統計資料及期間的時間序列圖表。" +
						"對於 1 小時（含）以下的期間，圖表會以每 5 秒所拍攝的 Snapshot 為依據。" +
						"對於更長的期間，圖表會以每分鐘所拍攝的 Snapshot 為依據。"
				},
		 		grids: {
					sectionTitle: "端點資料",
					tagline: "檢視個別端點的聚集資料。監視資料會在伺服器中快取，並且每分鐘重新整理此快取一次。" +
							 "因此，資料最多一分鐘即會過期。"
				}
			},
			queueMonitor: {
				title: "佇列監視器",
				tagline: "使用各種佇列統計資料監視個別佇列。最多可檢視 100 個佇列。"
			},
			mqttClientMonitor: {
				title: "中斷連線的 MQTT 用戶端",
				tagline: "搜尋並刪除未連線的 MQTT 用戶端。最多可檢視 100 個 MQTT 用戶端。",
				taglineNoDelete: "搜尋未連線的 MQTT 用戶端。最多可檢視 100 個 MQTT 用戶端。"
			},
			subscriptionMonitor: {
				title: "訂閱監視器",
				tagline: "使用各種訂閱統計資料監視訂閱。刪除可延續訂閱。最多可檢視 100 個訂閱。",
				taglineNoDelete: "使用各種訂閱統計資料監視訂閱。最多可檢視 100 個訂閱。"
			},
			transactionMonitor: {
				title: "交易監視器",
				tagline: "監視器 XA 交易由外部交易管理程式進行協調。"
			},
			destinationMappingRuleMonitor: {
				title: "MQ Connectivity 目的地對映規則監視器",
				tagline: "監視目的地對映規則統計資料。這些統計資料用於目的地對映規則/佇列管理程式連線配對。最多可檢視 100 個配對。"
			},
			applianceMonitor: {
				title: "伺服器監視器",
				tagline: "收集各種 ${IMA_PRODUCTNAME_FULL} 統計資料的相關資訊。",
				storeMonitorTitle: "持續儲存庫",
				storeMonitorTagline: "監視 ${IMA_PRODUCTNAME_FULL} 持續儲存庫所使用資源的統計資料。",
				memoryMonitorTitle: "記憶體",
				memoryMonitorTagline: "監視其他 ${IMA_PRODUCTNAME_FULL} 程序所使用記憶體的統計資料。"
			},
			topicMonitor: {
				title: "主題監視器",
				tagline: "配置提供有關主題字串之各種聚集統計資料的主題監視器。",
				taglineNoConfigure: "檢視提供有關主題字串之各種聚集統計資料的主題監視器。"
			},
			views: {
				conntimeWorst: "最新連線數",
				conntimeBest: "最早連線數",
				tputMsgWorst: "最低傳輸量（訊息數）",
				tputMsgBest: "最高傳輸量（訊息數）",
				tputBytesWorst: "最低傳輸量 (KB)",
				tputBytesBest: "最高傳輸量 (KB)",
				activeCount: "作用中的連線數",
				connectCount: "已建立的連線數",
				badCount: "累加的失敗連線數",
				msgCount: "傳輸量（訊息數）",
				bytesCount: "傳輸量（位元組數）",

				msgBufHighest: "包含大部分已緩衝訊息的佇列",
				msgProdHighest: "包含大部分已產生訊息的佇列",
				msgConsHighest: "包含大部分已耗用訊息的佇列",
				numProdHighest: "包含大部分產生者的佇列",
				numConsHighest: "包含大部分消費者的佇列",
				msgBufLowest: "包含最少已緩衝訊息的佇列",
				msgProdLowest: "包含最少已產生訊息的佇列",
				msgConsLowest: "包含最少已耗用訊息的佇列",
				numProdLowest: "包含最少產生者的佇列",
				numConsLowest: "包含最少消費者的佇列",
				BufferedHWMPercentHighestQ: "距離容量上限最近的佇列",
				BufferedHWMPercentLowestQ: "距離容量上限最遠的佇列",
				ExpiredMsgsHighestQ: "包含大部分已過期訊息的佇列",
				ExpiredMsgsLowestQ: "包含最少已過期訊息的佇列",

				msgPubHighest: "包含大部分已發佈訊息的主題",
				subsHighest: "包含大部分訂閱者的主題",
				msgRejHighest: "包含大部分已拒絕訊息的主題",
				pubRejHighest: "包含大部分已失敗發佈的主題",
				msgPubLowest: "包含最少已發佈訊息的主題",
				subsLowest: "包含最少訂閱者的主題",
				msgRejLowest: "包含最少已拒絕訊息的主題",
				pubRejLowest: "包含最少已失敗發佈的主題",

				publishedMsgsHighest: "包含大部分已發佈訊息的訂閱",
				bufferedMsgsHighest: "包含大部分已緩衝訊息的訂閱",
				bufferedPercentHighest: "包含最高已緩衝訊息百分比的訂閱",
				rejectedMsgsHighest: "包含大部分已拒絕訊息的訂閱",
				publishedMsgsLowest: "包含最少已發佈訊息的訂閱",
				bufferedMsgsLowest: "包含最少已緩衝訊息的訂閱",
				bufferedPercentLowest: "包含最低已緩衝訊息百分比的訂閱",
				rejectedMsgsLowest: "包含最少已拒絕訊息的訂閱",
				BufferedHWMPercentHighestS: "距離容量上限最近的訂閱",
				BufferedHWMPercentLowestS: "距離容量上限最遠的訂閱",
				ExpiredMsgsHighestS: "包含大部分已過期訊息的訂閱",
				ExpiredMsgsLowestS: "包含最少已過期訊息的訂閱",
				DiscardedMsgsHighestS: "包含大部分已捨棄訊息的訂閱",
				DiscardedMsgsLowestS: "包含最少已捨棄訊息的訂閱",
				mostCommitOps: "包含大部分確定作業的規則",
				mostRollbackOps: "包含大部分回復作業的規則",
				mostPersistMsgs: "包含大部分持續訊息的規則",
				mostNonPersistMsgs: "包含大部分非持續訊息的規則",
				mostCommitMsgs: "包含大部分已確定訊息的規則"
			},
			rules: {
				any: "任何",
				imaTopicToMQQueue: "${IMA_PRODUCTNAME_SHORT} 主題至 MQ 佇列",
				imaTopicToMQTopic: "${IMA_PRODUCTNAME_SHORT} 主題至 MQ 主題",
				mqQueueToIMATopic: "MQ 佇列至 ${IMA_PRODUCTNAME_SHORT} 主題",
				mqTopicToIMATopic: "MQ 主題至 ${IMA_PRODUCTNAME_SHORT} 主題",
				imaTopicSubtreeToMQQueue: "${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構至 MQ 佇列",
				imaTopicSubtreeToMQTopic: "${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構至 MQ 主題",
				imaTopicSubtreeToMQTopicSubtree: "${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構至 MQ 主題子樹狀結構",
				mqTopicSubtreeToIMATopic: "MQ 主題子樹狀結構至 ${IMA_PRODUCTNAME_SHORT} 主題",
				mqTopicSubtreeToIMATopicSubtree: "MQ 主題子樹狀結構至 ${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構",
				imaQueueToMQQueue: "${IMA_PRODUCTNAME_SHORT} 佇列至 MQ 佇列",
				imaQueueToMQTopic: "${IMA_PRODUCTNAME_SHORT} 佇列至 MQ 主題",
				mqQueueToIMAQueue: "MQ 佇列至 ${IMA_PRODUCTNAME_SHORT} 佇列",
				mqTopicToIMAQueue: "MQ 主題至 ${IMA_PRODUCTNAME_SHORT} 佇列",
				mqTopicSubtreeToIMAQueue: "MQ 主題子樹狀結構至 ${IMA_PRODUCTNAME_SHORT} 佇列"
			},
			chartLabels: {
				activeCount: "作用中的連線數",
				connectCount: "已建立的連線數",
				badCount: "累加的失敗連線數",
				msgCount: "傳輸量（訊息數/秒）",
				bytesCount: "傳輸量（MB/秒）"
			},
			connectionVolume: "作用中的連線數",
			connectionVolumeAxisTitle: "作用中",
			connectionRate: "新連線數",
			connectionRateAxisTitle: "已變更",
			newConnections: "已開啟的連線數",
			closedConnections: "已關閉的連線數",
			timeAxisTitle: "時間",
			pauseButton: "暫停圖表更新",
			resumeButton: "回復圖表更新",
			noData: "無資料",
			grid: {				
				name: "用戶端 ID",
				// TRNOTE Do not translate "(Name)"
				nameTooltip: "連線名稱。(Name)",
				endpoint: "端點",
				// TRNOTE Do not translate "(Endpoint)"
				endpointTooltip: "端點的名稱。(Endpoint)",
				clientIp: "用戶端 IP",
				// TRNOTE Do not translate "(ClientAddr)"
				clientIpTooltip: "用戶端 IP 位址。(ClientAddr)",
				clientId: "用戶端 ID：",
				userId: "使用者 ID",
				// TRNOTE Do not translate "(UserId)"
				userIdTooltip: "主要使用者 ID。(UserId)",
				protocol: "通訊協定",
				// TRNOTE Do not translate "(Protocol)"
				protocolTooltip: "通訊協定的名稱。(Protocol)",
				bytesRecv: "已接收 (KB)",
				// TRNOTE Do not translate "(ReadBytes)"
				bytesRecvTooltip: "自連線時間後讀取的位元組數。(ReadBytes)",
				bytesSent: "已傳送 (KB)",
				// TRNOTE Do not translate "(WriteBytes)"
				bytesSentTooltip: "自連線時間後寫入的位元組數。(WriteBytes)",
				msgRecv: "已接收（訊息數）",
				// TRNOTE Do not translate "(ReadMsg)"
				msgRecvTooltip: "自連線時間後讀取的訊息數。(ReadMsg)",
				msgSent: "已傳送（訊息數）",
				// TRNOTE Do not translate "(WriteMsg)"
				msgSentTooltip: "自連線時間後寫入的訊息數。(WriteMsg)",
				bytesThroughput: "傳輸量（KB/秒）",
				msgThroughput: "傳輸量（訊息數/秒）",
				connectionTime: "連線時間（秒）",
				// TRNOTE Do not translate "(ConnectTime)"
				connectionTimeTooltip: "自連線建立後的秒數。(ConnectTime)",
				updateButton: "更新",
				refreshButton: "重新整理",
				deleteClientButton: "刪除用戶端",
				deleteSubscriptionButton: "刪除可延續訂閱",
				endpointLabel: "來源：",
				listenerLabel: "端點：",
				queueNameLabel: "佇列名稱：",
				ruleNameLabel: "規則名稱：",
				queueManagerConnLabel: "佇列管理程式連線：",
				topicNameLabel: "監視的主題：",
				topicStringLabel: "主題字串：",
				subscriptionLabel: "訂閱名稱：",
				subscriptionTypeLabel: "訂閱類型：",
				refreshingLabel: "更新中...",
				datasetLabel: "查詢：",
				ruleTypeLabel: "規則類型：",
				durationLabel: "持續時間：",
				resultsLabel: "結果：",
				allListeners: "All",
				allEndpoints: "（所有端點）",
				allSubscriptions: "All",
				metricLabel: "查詢：",
				// TRNOTE min is short for minutes
				last10: "前 10 分鐘",
				// TRNOTE hr is short for hours
				last60: "前 1 小時",
				// TRNOTE hr is short for hours
				last360: "前 6 小時",
				// TRNOTE hr is short for hours
				last1440: "前 24 小時",
				// TRNOTE min is short for minutes
				min5: "5 分鐘",
				// TRNOTE min is short for minutes
				min10: "10 分鐘",
				// TRNOTE min is short for minutes
				min30: "30 分鐘",
				// TRNOTE hr is short for hours
				hr1: "1 小時",
				// TRNOTE hr is short for hours
				hr3: "3 小時",
				// TRNOTE hr is short for hours
				hr6: "6 小時",
				// TRNOTE hr is short for hours
				hr12: "12 小時",
				// TRNOTE hr is short for hours
				hr24: "24 小時",
				totalMessages: "訊息數總計",
				bufferedMessages: "緩衝的訊息數",
				subscriptionProperties: "訂閱內容",
				commitTransactionButton: "確定",
				rollbackTransactionButton: "回復",
				forgetTransactionButton: "忽略",
				queue: {
					queueName: "佇列名稱",
					// TRNOTE Do not translate "(QueueName)"
					queueNameTooltip: "佇列的名稱。(QueueName)",
					msgBuf: "現行",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "已緩衝到佇列並在等待耗用的訊息數。(BufferedMsgs)",
					msgBufHWM: "尖峰",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "自伺服器啟動後或佇列建立後，緩衝到佇列中的最高訊息數（取兩者中時間較近者）。(BufferedMsgsHWM)",
					msgBufHWMPercent: "尖峰數佔上限的 %",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "已緩衝的訊息尖峰數佔可以緩衝之訊息數上限的百分比。(BufferedHWMPercent)",
					perMsgBuf: "現行數佔上限的 %",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "已緩衝的訊息數佔已緩衝之訊息數上限的百分比。(BufferedPercent)",
					totMsgProd: "已產生",
					// TRNOTE Do not translate "(ProducedMsgs)"
					totMsgProdTooltip: "自伺服器啟動後或佇列建立後，傳送到佇列中的訊息數（取兩者中時間較近者）。(ProducedMsgs)",
					totMsgCons: "已耗用",
					// TRNOTE Do not translate "(ConsumedMsgs)"
					totMsgConsTooltip: "自伺服器啟動後或佇列建立後，從佇列中耗用的訊息數（取兩者中時間較近者）。(ConsumedMsgs)",
					totMsgRej: "已拒絕",
					// TRNOTE Do not translate "(RejectedMsgs)"
					totMsgRejTooltip: "因為已達到緩衝訊息數上限而未傳送至佇列的訊息數。(RejectedMsgs)",
					totMsgExp: "已過期",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					totMsgExpTooltip: "自伺服器啟動後或佇列建立後，在佇列中過期的訊息數（取兩者中時間較近者）。(ExpiredMsgs)",			
					numProd: "產生者",
					// TRNOTE Do not translate "(Producers)"
					numProdTooltip: "佇列中作用中的產生者人數。(Producers)", 
					numCons: "消費者",
					// TRNOTE Do not translate "(Consumers)"
					numConsTooltip: "佇列中作用中的消費者人數。(Consumers)",
					maxMsgs: "上限",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgsTooltip: "容許緩衝到佇列的訊息數上限。此值僅為準則，而不是絕對限制。當系統負載處於高峰時，緩衝到佇列中的訊息數，可能會略高於 MaxMessages 值。(MaxMessages)"
				},
				destmapping: {
					ruleName: "規則名稱",
					qmConnection: "佇列管理程式連線",
					commits: "確定數",
					rollbacks: "回復數",
					committedMsgs: "已確定訊息數",
					persistentMsgs: "持續訊息數",
					nonPersistentMsgs: "非持續訊息數",
					status: "狀態",
					// TRNOTE Do not translate "(RuleName)"
					ruleNameTooltip: "監視之目的地對映規則的名稱。(RuleName)",
					// TRNOTE Do not translate "(QueueManagerConnection)"
					qmConnectionTooltip: "目的地對映規則所關聯之佇列管理程式連線的名稱。(QueueManagerConnection)",
					// TRNOTE Do not translate "(CommitCount)"
					commitsTooltip: "已完成之目的地對映規則的確定作業數。一項確定作業可以確定許多訊息。(CommitCount)",
					// TRNOTE Do not translate "(RollbackCount)"
					rollbacksTooltip: "已完成之目的地對映規則的回復作業數。(RollbackCount)",
					// TRNOTE Do not translate "(CommittedMessageCount)"
					committedMsgsTooltip: "確定目的地對映規的訊息數。(CommittedMessageCount)",
					// TRNOTE Do not translate "(PersistedCount)"
					persistentMsgsTooltip: "使用目的地對映規則所傳送的持續訊息數。(PersistentCount)",
					// TRNOTE Do not translate "(NonpersistentCount)"
					nonPersistentMsgsTooltip: "使用目的地對映規則所傳送的非持續訊息數。(NonpersistentCount)",
					// TRNOTE Do not translate "(Status)"
					statusTooltip: "規則的狀態：「已啟用」、「已停用」、「重新連接」或「重新啟動」。(Status)"
				},
				mqtt: {
					clientId: "用戶端 ID",
					lastConn: "前次連線日期",
					// TRNOTE Do not translate "(ClientId)"
					clientIdTooltip: "用戶端 ID。(ClientId)",
					// TRNOTE Do not translate "(LastConnectedTime)"
					lastConnTooltip: "用戶端前次連接至 ${IMA_PRODUCTNAME_SHORT} 伺服器的時間。(LastConnectedTime)",	
					errorMessage: "原因",
					errorMessageTooltip: "無法刪除用戶端的原因。"
				},
				subscription: {
					subsName: "名稱",
					// TRNOTE Do not translate "(SubName)"
					subsNameTooltip: "與訂閱相關聯的名稱。對於不可延續訂閱，此值可以是空字串。(SubName)",
					topicString: "主題字串",
					// TRNOTE Do not translate "(TopicString)"
					topicStringTooltip: "訂閱所訂閱的主題。(TopicString)",					
					clientId: "用戶端 ID",
					// TRNOTE Do not translate "(ClientID)"
					clientIdTooltip: "擁有訂閱之用戶端的用戶端 ID。(ClientID)",
					consumers: "消費者",
					// TRNOTE Do not translate "(Consumers)"
					consumersTooltip: "訂閱擁有的消費者人數。(Consumers)",
					msgPublished: "已發佈",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPublishedTooltip: "自伺服器啟動後或訂閱建立後，發佈至此訂閱的訊息數（取兩者中時間較近者）。 (PublishedMsgs)",
					msgBuf: "現行",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "等待傳送至用戶端的已發佈訊息數。(BufferedMsgs)",
					perMsgBuf: "現行數佔上限的 %",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "現行已緩衝的訊息數佔已緩衝訊息數上限的百分比。(BufferedPercent)",
					msgBufHWM: "尖峰",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "自伺服器啟動或訂閱建立後，已緩衝訊息的最高數目（取兩者中時間較近者）。(BufferedMsgsHWM)",
					msgBufHWMPercent: "尖峰數佔上限的 %",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "已緩衝的訊息尖峰數佔可以緩衝之訊息數上限的百分比。(BufferedHWMPercent)",
					maxMsgBuf: "上限",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgBufTooltip: "此訂閱所能緩衝的訊息數上限。(MaxMessages)",
					rejMsg: "已拒絕",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "因為當訊息發佈到訂閱時，已達到緩衝訊息數上限而遭拒絕的訊息數。(RejectedMsgs)",
					expDiscMsg: "已過期/已捨棄",
					// TRNOTE Do not translate "(ExpiredMsgs + DiscardedMsgs)"
					expDiscMsgTooltip: "因為緩衝區滿載而遭到期或捨棄處理並未分送的訊息數。(ExpiredMsgs + DiscardedMsgs)",
					expMsg: "已過期",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					expMsgTooltip: "因為過期而未分送的訊息數。(ExpiredMsgs)",
					DiscMsg: "已捨棄",
					// TRNOTE Do not translate "(DiscardedMsgs)"
					DiscMsgTooltip: "因為緩衝區滿載而遭捨棄處理並未分送的訊息數。(DiscardedMsgs)",
					subsType: "類型",
					// TRNOTE Do not translate "(IsDurable, IsShared)"
					subsTypeTooltip: "此直欄指出訂閱為可延續或不可延續，以及訂閱是否共用。除非明確刪除訂閱，否則可延續訂閱生存伺服器會重新啟動。(IsDurable, IsShared)",				
					isDurable: "可延續",
					isShared: "已共用",
					notDurable: "不可延續",
					noName: "（無）",
					// TRNOTE {0} and {1} contain a formatted integer number greater than or equal to zero, e.g. 5,250
					expDiscMsgCellTitle: "已到期：{0}；已捨棄：{1}",
					errorMessage: "原因",
					errorMessageTooltip: "無法刪除訂閱的原因。",
					messagingPolicy: "傳訊原則",
					// TRNOTE Do not translate "(MessagingPolicy)"
					messagingPolicyTooltip: "訂閱所使用的傳訊原則名稱。(MessagingPolicy)"						
				},
				transaction: {
					// TRNOTE Do not translate "(XID)"
					xId: "交易 ID (XID)",
					timestamp: "時間戳記",
					state: "狀態",
					stateCode2: "已準備",
					stateCode5: "已探索性地確定",
					stateCode6: "已探索性地回復"
				},
				topic: {
					topicSubtree: "主題字串",
					msgPub: "已發佈的訊息數",
					rejMsg: "已拒絕的訊息數",
					rejPub: "失敗的發佈數",
					numSubs: "訂閱者",
					// TRNOTE Do not translate "(TopicString)"
					topicSubtreeTooltip: "正在監視的主題。主題字串一律包含萬用字元。(TopicString)",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPubTooltip: "順利地發佈到符合萬用字元所表示之主題字串的主題的訊息數。(PublishedMsgs)",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "不是因為服務品質等級造成發佈要求失敗，但遭一或多個訂閱拒絕的訊息數。(RejectedMsgs)",
					// TRNOTE Do not translate "(FailedPublishes)"
					rejPubTooltip: "因為訊息遭一或多個訂閱拒絕而失敗的發佈要求數。(FailedPublishes)",
					// TRNOTE Do not translate "(Subscriptions)"
					numSubsTooltip: "受監視主題的作用中訂閱數。此數字會顯示所有符合萬用字元所表示之主題字串的作用中訂閱數。(Subscriptions)"
				},
				store: {
					name: "統計資料",
					value: "值",
                    diskUsedPercent: "已用磁碟 (%)",
                    diskFreeBytes: "磁碟可用量 (MB)",
					persistentMemoryUsedPercent: "已用記憶體 (%)",
					memoryTotalBytes: "記憶體總量 (MB)",
					Pool1TotalBytes: "儲存區 1 總量 (MB)",
					Pool1UsedBytes: "儲存區 1 用量 (MB)",
					Pool1UsedPercent: "儲存區 1 用量 (%)",
					Pool1RecordLimitBytes: "儲存區 1 記錄限制 (MB)", 
					Pool1RecordsUsedBytes: "儲存區 1 記錄用量 (MB)",
                    Pool2TotalBytes: "儲存區 2 總量 (MB)",
                    Pool2UsedBytes: "儲存區 2 用量 (MB)",
                    Pool2UsedPercent: "儲存區 2 用量 (%)"
				},
				memory: {
					name: "統計資料",
					value: "值",
					memoryTotalBytes: "記憶體總量 (MB)",
					memoryFreeBytes: "記憶體可用量 (MB)",
					memoryFreePercent: "記憶體可用量 (%)",
					serverVirtualMemoryBytes: "虛擬記憶體 (MB)",
					serverResidentSetBytes: "常駐集 (MB)",
					messagePayloads: "訊息內容 (MB)",
					publishSubscribe: "發佈訂閱 (MB)",
					destinations: "目的地 (MB)",
					currentActivity: "現行活動 (MB)",
					clientStates: "用戶端狀態 (MB)"
				}
			},
			logs: {
				title: "日誌",
				pageSummary: "", 
				downloadLogsSubTitle: "下載日誌",
				downloadLogsTagline: "下載日誌供診斷之用。",
				lastUpdated: "前次更新時間",
				name: "日誌檔",
				size: "大小（位元組）"
			},
			lastUpdated: "前次更新時間：",
			bufferedMsgs: "緩衝的訊息數：",
			retainedMsgs: "保留的訊息數：",
			dataCollectionInterval: "資料收集間隔：",
			interval: {fiveSeconds: "5 秒", oneMinute: "1 分鐘" },
			cacheInterval: "資料收集間隔：1 分鐘",
			notAvailable: "不適用",
			savingProgress: "儲存中…",
			savingFailed: "儲存失敗。",
			noRecord: "在伺服器上找不到記錄。",
			deletingProgress: "刪除中...",
			deletingFailed: "刪除失敗。",
			deletePending: "刪除擱置中。",
			deleteSuccessful: "刪除成功。",
			testingProgress: "測試中...",
			dialog: {
				// TRNOTE Do not translate "$SYS"
				addTopicMonitorInstruction: "指定要監視的主題字串。字串不能以 $SYS 開頭，且必須以多層萬用字元結尾（例如，/animals/dogs/#）。" +
						                    "若要聚集所有主題的統計資料，請指定單一的多層萬用字元 (#)。" +
						                    "如果主題沒有任何子主題，監視器將僅顯示該主題的資料。" +
						                    "例如，如果 labradors 沒有任何子主題，/animals/dogs/labradors/# 將僅監視主題 /animals/dogs/labradors。",
				addTopicMonitorTitle: "新增主題監視器",
				removeTopicMonitorTitle: "刪除主題監視器",
				removeSubscriptionTitle: "刪除可延續訂閱",
				removeSubscriptionsTitle: "刪除可延續訂閱",
				removeSubscriptionContent: "您確定要刪除此訂閱嗎？",
				removeSubscriptionsContent: "您確定要刪除 {0} 個訂閱嗎？",
				removeClientTitle: "刪除 MQTT 用戶端",
				removeClientsTitle: "刪除 MQTT 用戶端",
				removeClientContent: "您確定要刪除此用戶端嗎？刪除用戶端還將刪除該用戶端的訂閱。",
				removeClientsContent: "您確定要刪除 {0} 個用戶端嗎？刪除用戶端還將刪除這些用戶端的訂閱。",
				topicStringLabel: "主題字串：",
				descriptionLabel: "說明",
				topicStringTooltip: "要監視的主題字串。除了必要的結尾萬用字元 (#) 以外， 主題字串內不允許下列字元：+ #",
				saveButton: "儲存",
				cancelButton: "取消"
			},
			removeDialog: {
				title: "刪除項目",
				content: "您確定要刪除此記錄嗎？",
				deleteButton: "刪除",
				cancelButton: "取消"
			},
			actionConfirmDialog: {
				titleCommit: "確定交易",
				titleRollback: "回復交易",
				titleForget: "忽略交易",
				contentCommit: "您確定要確定此交易嗎？",
				contentRollback: "您確定要回復此交易嗎？",
				contentForget: "您確定要忽略此交易嗎？",
				actionButtonCommit: "確定",
				actionButtonRollback: "回復",
				actionButtonForget: "忽略",
				cancelButton: "取消",
				failedRollback: "在嘗試回復交易時發生錯誤。",
				failedCommit: "在嘗試確定交易時發生錯誤。",
				failedForget: "在嘗試忽略交易時發生錯誤。"
			},
			asyncInfoDialog: {
				title: "刪除擱置中",
				content: "無法立即刪除訂閱。將會盡早刪除。",
				closeButton: "關閉"
			},
			deleteResultsDialog: {
				title: "訂閱刪除狀態",
				clientsTitle: "用戶端刪除狀態",
				allSuccessful:  "所有訂閱已順利刪除。",
				allClientsSuccessful: "所有用戶端已順利刪除。",
				pendingTitle: "刪除擱置中",
				pending: "無法立即刪除下列訂閱。將會盡早刪除。",
				clientsPending: "無法立即刪除下列用戶端。將會盡早刪除。",
				failedTitle: "刪除失敗",
				failed:  "無法刪除下列訂閱。",
				clientsFailed: "無法刪除下列用戶端。",
				closeButton: "關閉"					
			},
			invalidName: "已存在具有該名稱的記錄。",
			// TRNOTE Do not translate "$SYS"
			reservedName: "主題字串不能以 $SYS 開頭",
			invalidChars: "不容許下列字元：+ #",			
			invalidTopicMonitorFormat: "主題字串必須是 #，或以 /# 結尾",
			invalidRequired: "需要輸入值。",
			clientIdMissingMessage: "必須指定此值。您可以使用一或多個星號 (*) 作為萬用字元。",
			ruleNameMissingMessage: "必須指定此值。您可以使用一或多個星號 (*) 作為萬用字元。",
			queueNameMissingMessage: "必須指定此值。您可以使用一或多個星號 (*) 作為萬用字元。",
			subscriptionNameMissingMessage: "必須指定此值。您可以使用一或多個星號 (*) 作為萬用字元。",
			discardOldestMessagesInfo: "至少有一個啟用的端點在使用傳訊原則，「訊息行為上限」為「<em>捨棄舊訊息</em>」。捨棄的訊息不會影響「<em>已拒絕的訊息數</em>」或「<em>失敗的發佈數</em>」計數。",
			// Strings for viewing and modifying SNMP Configuration
			snmp: {
				title: "SNMP 設定",
				tagline: "啟用採用社群概念的「簡易網路管理通訊協定」(SNMPv2C)、下載「管理資訊庫 (MIB)」檔案，以及配置 SNMPv2C 社群。" +
						"${IMA_PRODUCTNAME_FULL} 支援 SNMPv2C。",
				status: {
					title: "SNMP 狀態",
					tagline: {
						disabled: "尚未啟用 SNMP。",
						enabled: "已啟用並配置 SNMP。",
						warning: "必須定義 SNMP 社群，才能容許存取伺服器上的 SNMP 資料。"
					},
					enableLabel: "啟用 SNMPv2C 代理程式",
					addressLabel: "SNMP 代理程式位址：",					
					editLinkLabel: "編輯",
					editAddressDialog: {
						title: "編輯 SNMP 代理程式位址",
						instruction: "指定 ${IMA_PRODUCTNAME_FULL} 可用於 SNMP 的靜態 IP 位址。" +
								"若要容許 ${IMA_PRODUCTNAME_SHORT} 伺服器上所配置的任何 IP 位址，或者您的伺服器只使用動態 IP 位址，請選取「<em>全部</em>」。"
					},
					getSnmpEnabledError: "擷取 SNMPEnabled 狀態時發生錯誤。",
					setSnmpEnabledError: "設定 SNMPEnabled 狀態時發生錯誤。",
					getSnmpAgentAddressError: "擷取 SNMP 代理程式位址時發生錯誤。",
					setSnmpAgentAddressError: "設定 SNMP 代理程式位址時發生錯誤。"
				},
				mibs: {
					title: "企業 MIB",
					tagline: "企業 MIB 檔會說明透過內嵌 SNMP 代理程式可以使用的功能及資料，以便用戶端能夠適當地存取它們。",
					messageSightLinkLabel: "${IMA_PRODUCTNAME_SHORT} MIB",
					hwNotifyLinkLabel: "HW 通知 MIB"
				},
				communities: {
					title: "SNMP 社群",
					tagline: "您可以透過建立一個以上的 SNMPv2C 社群，來定義對伺服器上 SNMP 資料的存取權。" +
							"啟用監視時需要 SNMP 社群。" +
							"所有社群皆會具有唯讀存取權。",
					grid: {
						nameColumn: "名稱",
						hostRestrictionColumn: "主機限制"
					},
					dialog: {
						addTitle: "新增 SNMP 社群",
						editTitle: "編輯 SNMP 社群",
						instruction: "SNMP 社群容許存取您伺服器上的 SNMP 資料。",
						deleteTitle: "刪除 SNMP 社群",
						deletePrompt: "您確定要刪除此 SNMP 社群嗎？",
						deleteNotAllowedTitle: "不容許移除", 
						deleteNotAllowedMessage: "無法移除 SNMP 社群，因為有一或多個設陷訂閱者正在使用此社群。" +
								"從所有設陷訂閱者中移除 SNMP 社群，然後重試。",
						defineTitle: "定義 SNMP 社群",
						defineInstruction: "必須定義 SNMP 社群，才能容許存取伺服器上的 SNMP 資料。",
						nameLabel: "名稱",
						nameInvalid: "社群字串不得包含引號。",
						hostRestrictionLabel: "主機限制",
						hostRestrictionTooltip: "如有指定，將會限制只能存取指定的主機。" +
								"如果您將此欄位保留空白，則會容許與所有 IP 位址通訊。" +
								"請指定主機名稱或位址的逗點區隔清單。" +
								"若要指定子網路，請使用「無類別內部網域遞送」(CIDR) IP 位址或主機名稱。",
						hostRestrictionInvalid: "主機限制必須是空的，否則請指定包含有效 IP 位址、主機名稱或網域的逗點區隔清單。"
					},
					getCommunitiesError: "擷取 SNMP 社群時發生錯誤。",
					saveCommunityError: "儲存 SNMP 社群時發生錯誤。",
					deleteCommunityError: "刪除 SNMP 社群時發生錯誤。"					
				},
				trapSubscribers: {
					title: "設陷訂閱者",
					tagline: "設陷訂閱者代表用來與內嵌在伺服器上的 SNMP 代理程式通訊的 SNMP 用戶端。" +
							"SNMP 設陷訂閱者必須是現有 SNMP 用戶端。" +
							"建立設陷訂閱者之前，請將 SNMP 用戶端配置為接收設陷。",
					grid: {
						hostColumn: "主機",
						portColumn: "用戶端埠號",
						communityColumn: "社群"
					},
					dialog: {
						addTitle: "新增設陷訂閱者",
						editTitle: "編輯設陷訂閱者",
						instruction: "設陷訂閱者代表用來與內嵌在伺服器上的 SNMP 代理程式通訊的 SNMP 用戶端。",
						deleteTitle: "刪除設陷訂閱者",
						deletePrompt: "您確定要刪除此設陷訂閱者嗎？",
						hostLabel: "主機",
						hostTooltip: "SNMP 用戶端用以接聽設陷資訊的 IP 位址。",
						hostInvalid: "需要有效的 IP 位址。",
				        duplicateHost: "已經存在該主機的設陷訂閱者。",
						portLabel: "用戶端埠號",
						portTooltip: "SNMP 用戶端要用於接聽設陷資訊的埠。" +
								"有效埠介於 1（含）與 65535（含）之間。",
						portInvalid: "埠必須是介於 1（含）與 65535（含）之間的數字。",
						communityLabel: "社群",
	                    noCommunitiesTitle: "無社群",
	                    noCommunitiesDetails: "如果未先定義社群，則無法定義設陷訂閱者。"						
					},
					getTrapSubscribersError: "擷取 SNMP 設陷訂閱者時發生錯誤。",
					saveTrapSubscriberError: "儲存 SNMP 設陷訂閱者時發生錯誤。",
					deleteTrapSubscriberError: "刪除 SNMP 設陷訂閱者時發生錯誤。"										
				},
				okButton: "確定",
				saveButton: "儲存",
				cancelButton: "取消",
				closeButton: "關閉"
			}
		}
});
