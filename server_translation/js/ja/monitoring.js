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
			title : "モニター",
			monitoringNotAvailable: "${IMA_PRODUCTNAME_SHORT} サーバーの状況が原因でモニターが使用できません。",
			monitoringError:"モニター・データ取得中にエラーが発生しました。",
			pool1LimitReached: "プール 1 の使用率が高すぎます。 パブリケーションとサブスクリプションは拒否される可能性があります。",
			connectionStats: {
				title: "接続モニター",
				tagline: "集約接続データをモニターし、複数の接続統計の中でパフォーマンス最高およびパフォーマンス最低の接続について照会します。",
		 		charts: {
					sectionTitle: "接続グラフ",
					tagline: "サーバーとのアクティブおよび新規の接続の数をモニターします。 グラフの更新を一時停止するには、グラフの下にあるボタンをクリックします。",
					activeConnectionsTitle: "アクティブ接続のスナップショット (約 5 秒ごとに取得)",
					newConnectionsTitle: "新規および閉じられた接続のスナップショット (約 5 秒ごとに取得)"
				},
		 		grids: {
					sectionTitle: "接続データ",
					tagline: "特定のエンドポイントおよび統計についてパフォーマンスが最高/最低の接続を表示します。 " +
							"最大 50 までの接続が表示可能です。 モニター・データはサーバー上のキャッシュに入れられ、そのキャッシュは 1 分ごとに更新されます。 " +
							"したがって、データは最大 1 分前のものの可能性があります。"
				}
			},
			endpointStats: {
				title: "エンドポイント・モニター",
				tagline: "個々のエンドポイントの履歴パフォーマンスを、時系列グラフまたは集約データとしてモニターします。",
		 		charts: {
					sectionTitle: "エンドポイント・グラフ",
					tagline: "特定のエンドポイント、統計、および期間の時系列グラフを表示します。 " +
						"1 時間以下の期間の場合、グラフは 5 秒ごとに取られたスナップショットに基づくものです。 " +
						"それより長い期間の場合、グラフは 1 分ごとに取られたスナップショットに基づくものです。"
				},
		 		grids: {
					sectionTitle: "エンドポイント・データ",
					tagline: "個々のエンドポイントの集約データを表示します。 モニター・データはサーバー上のキャッシュに入れられ、そのキャッシュは 1 分ごとに更新されます。 " +
							 "したがって、データは最大 1 分前のものの可能性があります。"
				}
			},
			queueMonitor: {
				title: "キュー・モニター",
				tagline: "さまざまなキュー統計を使用して個々のキューをモニターします。 最大 100 個までのキューが表示可能です。"
			},
			mqttClientMonitor: {
				title: "切断された MQTT クライアント",
				tagline: "切断されている MQTT クライアントを検索し、削除します。 最大 100 個までの MQTT クライアントが表示可能です。",
				taglineNoDelete: "切断されている MQTT クライアントを検索します。 最大 100 個までの MQTT クライアントが表示可能です。"
			},
			subscriptionMonitor: {
				title: "サブスクリプション・モニター",
				tagline: "さまざまなサブスクリプション統計を使用してサブスクリプションをモニターします。 永続サブスクリプションを削除します。 最大 100 個までのサブスクリプションが表示可能です。",
				taglineNoDelete: "さまざまなサブスクリプション統計を使用してサブスクリプションをモニターします。 最大 100 個までのサブスクリプションが表示可能です。"
			},
			transactionMonitor: {
				title: "トランザクション・モニター",
				tagline: "外部トランザクション・マネージャーによって調整される XA トランザクションをモニターします。"
			},
			destinationMappingRuleMonitor: {
				title: "MQ Connectivity 宛先マッピング・ルール・モニター",
				tagline: "宛先マッピング・ルール統計をモニターします。 統計情報は、宛先マッピング・ルール / キュー・マネージャー接続のペアに関するものです。 最大 100 までのペアが表示可能です。"
			},
			applianceMonitor: {
				title: "サーバー・モニター",
				tagline: "${IMA_PRODUCTNAME_FULL} のさまざまな統計についての情報を収集します。",
				storeMonitorTitle: "永続的ストア",
				storeMonitorTagline: "${IMA_PRODUCTNAME_FULL} 永続的ストアで使用されるリソースの統計をモニターします。",
				memoryMonitorTitle: "メモリー",
				memoryMonitorTagline: "他の ${IMA_PRODUCTNAME_FULL} プロセスで使用されるメモリーの統計をモニターします。"
			},
			topicMonitor: {
				title: "トピック・モニター",
				tagline: "トピック・ストリングに関するさまざまな集約統計を提供するトピック・モニターを構成します。",
				taglineNoConfigure: "トピック・ストリングに関するさまざまな集約統計を提供するトピック・モニターを表示します。"
			},
			views: {
				conntimeWorst: "最新の接続",
				conntimeBest: "最古の接続",
				tputMsgWorst: "最低スループット (メッセージ)",
				tputMsgBest: "最高スループット (メッセージ)",
				tputBytesWorst: "最低スループット (KB)",
				tputBytesBest: "最高スループット (KB)",
				activeCount: "アクティブ接続",
				connectCount: "確立された接続",
				badCount: "累積接続障害数",
				msgCount: "スループット (msg)",
				bytesCount: "スループット (バイト)",

				msgBufHighest: "バッファーに入れられたメッセージ数が最大のキュー",
				msgProdHighest: "生成されたメッセージ数が最大のキュー",
				msgConsHighest: "消費メッセージ数が最大のキュー",
				numProdHighest: "プロデューサーの数が最大のキュー",
				numConsHighest: "コンシューマー数が最大のキュー",
				msgBufLowest: "バッファーに入れられたメッセージ数が最小のキュー",
				msgProdLowest: "生成されたメッセージ数が最小のキュー",
				msgConsLowest: "消費メッセージ数が最小のキュー",
				numProdLowest: "プロデューサーの数が最小のキュー",
				numConsLowest: "コンシューマー数が最小のキュー",
				BufferedHWMPercentHighestQ: "容量に最も近いキュー",
				BufferedHWMPercentLowestQ: "容量から最も離れているキュー",
				ExpiredMsgsHighestQ: "期限切れメッセージ数が最大のキュー",
				ExpiredMsgsLowestQ: "期限切れメッセージ数が最小のキュー",

				msgPubHighest: "公開メッセージ数が最大のトピック",
				subsHighest: "サブスクライバー数が最大のトピック",
				msgRejHighest: "拒否されたメッセージ数が最大のトピック",
				pubRejHighest: "公開失敗の数が最大のトピック",
				msgPubLowest: "公開されたメッセージ数が最小のトピック",
				subsLowest: "サブスクライバー数が最小のトピック",
				msgRejLowest: "拒否されたメッセージ数が最小のトピック",
				pubRejLowest: "公開失敗の数が最小のトピック",

				publishedMsgsHighest: "公開メッセージ数が最大のサブスクリプション",
				bufferedMsgsHighest: "バッファーに入れられたメッセージ数が最大のサブスクリプション",
				bufferedPercentHighest: "バッファーに入れられたメッセージの % が最高のサブスクリプション",
				rejectedMsgsHighest: "拒否されたメッセージ数が最大のサブスクリプション",
				publishedMsgsLowest: "公開メッセージ数が最小のサブスクリプション",
				bufferedMsgsLowest: "バッファーに入れられたメッセージ数が最小のサブスクリプション",
				bufferedPercentLowest: "バッファーに入れられたメッセージの % が最小のサブスクリプション",
				rejectedMsgsLowest: "拒否されたメッセージが最小のサブスクリプション",
				BufferedHWMPercentHighestS: "容量に最も近いサブスクリプション",
				BufferedHWMPercentLowestS: "容量から最も離れているサブスクリプション",
				ExpiredMsgsHighestS: "期限切れメッセージ数が最大のサブスクリプション",
				ExpiredMsgsLowestS: "期限切れメッセージ数が最小のサブスクリプション",
				DiscardedMsgsHighestS: "廃棄メッセージ数が最大のサブスクリプション",
				DiscardedMsgsLowestS: "廃棄メッセージ数が最小のサブスクリプション",
				mostCommitOps: "コミット操作数が最大のルール",
				mostRollbackOps: "ロールバック操作数が最大のルール",
				mostPersistMsgs: "永続メッセージ数が最大のルール",
				mostNonPersistMsgs: "非永続メッセージ数が最大のルール",
				mostCommitMsgs: "コミットされたメッセージ数が最大のルール"
			},
			rules: {
				any: "すべて",
				imaTopicToMQQueue: "${IMA_PRODUCTNAME_SHORT} トピックから MQ キューへ",
				imaTopicToMQTopic: "${IMA_PRODUCTNAME_SHORT} トピックから MQ トピックへ",
				mqQueueToIMATopic: "MQ キューから ${IMA_PRODUCTNAME_SHORT} トピックへ",
				mqTopicToIMATopic: "MQ トピックから ${IMA_PRODUCTNAME_SHORT} トピックへ",
				imaTopicSubtreeToMQQueue: "${IMA_PRODUCTNAME_SHORT} トピック・サブツリーから MQ キューへ",
				imaTopicSubtreeToMQTopic: "${IMA_PRODUCTNAME_SHORT} トピック・サブツリーから MQ トピックへ",
				imaTopicSubtreeToMQTopicSubtree: "${IMA_PRODUCTNAME_SHORT} トピック・サブツリーから MQ トピック・サブツリーへ",
				mqTopicSubtreeToIMATopic: "MQ トピック・サブツリーから ${IMA_PRODUCTNAME_SHORT} トピックへ",
				mqTopicSubtreeToIMATopicSubtree: "MQ トピック・サブツリーから ${IMA_PRODUCTNAME_SHORT} トピック・サブツリーへ",
				imaQueueToMQQueue: "${IMA_PRODUCTNAME_SHORT} キューから MQ キューへ",
				imaQueueToMQTopic: "${IMA_PRODUCTNAME_SHORT} キューから MQ トピックへ",
				mqQueueToIMAQueue: "MQ キューから ${IMA_PRODUCTNAME_SHORT} キューへ",
				mqTopicToIMAQueue: "MQ トピックから ${IMA_PRODUCTNAME_SHORT} キューへ",
				mqTopicSubtreeToIMAQueue: "MQ トピック・サブツリーから ${IMA_PRODUCTNAME_SHORT} キューへ"
			},
			chartLabels: {
				activeCount: "アクティブ接続",
				connectCount: "確立された接続",
				badCount: "累積接続障害数",
				msgCount: "スループット (msg/秒)",
				bytesCount: "スループット (MB/秒)"
			},
			connectionVolume: "アクティブ接続",
			connectionVolumeAxisTitle: "アクティブ",
			connectionRate: "新しい接続",
			connectionRateAxisTitle: "変更",
			newConnections: "開かれている接続",
			closedConnections: "閉じられている接続",
			timeAxisTitle: "時間",
			pauseButton: "グラフ更新を一時停止",
			resumeButton: "グラフ更新を再開",
			noData: "データなし",
			grid: {				
				name: "クライアント ID",
				// TRNOTE Do not translate "(Name)"
				nameTooltip: "接続名。 (Name)",
				endpoint: "エンドポイント",
				// TRNOTE Do not translate "(Endpoint)"
				endpointTooltip: "エンドポイントの名前。 (Endpoint)",
				clientIp: "クライアント IP",
				// TRNOTE Do not translate "(ClientAddr)"
				clientIpTooltip: "クライアント IP アドレス。 (ClientAddr)",
				clientId: "クライアント ID:",
				userId: "ユーザー ID",
				// TRNOTE Do not translate "(UserId)"
				userIdTooltip: "プライマリー・ユーザー ID。 (UserId)",
				protocol: "プロトコル",
				// TRNOTE Do not translate "(Protocol)"
				protocolTooltip: "プロトコルの名前。 (Protocol)",
				bytesRecv: "受信 (KB)",
				// TRNOTE Do not translate "(ReadBytes)"
				bytesRecvTooltip: "接続時以降読み取られたバイト数。 (ReadBytes)",
				bytesSent: "送信済み (KB)",
				// TRNOTE Do not translate "(WriteBytes)"
				bytesSentTooltip: "接続時以降に書き込まれたバイト数。 (WriteBytes)",
				msgRecv: "受信 (メッセージ)",
				// TRNOTE Do not translate "(ReadMsg)"
				msgRecvTooltip: "接続時以降読み取られたメッセージの数。 (ReadMsg)",
				msgSent: "送信 (メッセージ)",
				// TRNOTE Do not translate "(WriteMsg)"
				msgSentTooltip: "接続時以降書き込まれたメッセージの数。 (WriteMsg)",
				bytesThroughput: "スループット (KB/秒)",
				msgThroughput: "スループット (msg/秒)",
				connectionTime: "接続時間 (秒)",
				// TRNOTE Do not translate "(ConnectTime)"
				connectionTimeTooltip: "接続の作成以降の秒数。 (ConnectTime)",
				updateButton: "更新",
				refreshButton: "最新表示",
				deleteClientButton: "クライアントの削除",
				deleteSubscriptionButton: "永続サブスクリプションの削除",
				endpointLabel: "ソース:",
				listenerLabel: "エンドポイント:",
				queueNameLabel: "キュー名:",
				ruleNameLabel: "ルール名:",
				queueManagerConnLabel: "キュー・マネージャー接続:",
				topicNameLabel: "モニター対象のトピック:",
				topicStringLabel: "トピック・ストリング:",
				subscriptionLabel: "サブスクリプション名:",
				subscriptionTypeLabel: "サブスクリプション・タイプ:",
				refreshingLabel: "更新中...",
				datasetLabel: "照会:",
				ruleTypeLabel: "ルール・タイプ:",
				durationLabel: "期間:",
				resultsLabel: "結果:",
				allListeners: "すべて",
				allEndpoints: "(すべてのエンドポイント) ",
				allSubscriptions: "すべて",
				metricLabel: "照会:",
				// TRNOTE min is short for minutes
				last10: "過去 10 分",
				// TRNOTE hr is short for hours
				last60: "過去 1 時間",
				// TRNOTE hr is short for hours
				last360: "過去 6 時間",
				// TRNOTE hr is short for hours
				last1440: "過去 24 時間",
				// TRNOTE min is short for minutes
				min5: "5 分",
				// TRNOTE min is short for minutes
				min10: "10 分",
				// TRNOTE min is short for minutes
				min30: "30 分",
				// TRNOTE hr is short for hours
				hr1: "1 時間",
				// TRNOTE hr is short for hours
				hr3: "3 時間",
				// TRNOTE hr is short for hours
				hr6: "6 時間",
				// TRNOTE hr is short for hours
				hr12: "12 時間",
				// TRNOTE hr is short for hours
				hr24: "24 時間",
				totalMessages: "メッセージ総数",
				bufferedMessages: "バッファーに入れられているメッセージ",
				subscriptionProperties: "サブスクリプションのプロパティー",
				commitTransactionButton: "コミット",
				rollbackTransactionButton: "ロールバック",
				forgetTransactionButton: "無視",
				queue: {
					queueName: "キュー名",
					// TRNOTE Do not translate "(QueueName)"
					queueNameTooltip: "キューの名前。 (QueueName)",
					msgBuf: "現在",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "キュー上でバッファーに入れられていて消費待機中のメッセージの数。 (BufferedMsgs)",
					msgBufHWM: "ピーク",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "サーバー始動とキュー作成のいずれか後に生じた方の時点以降にキュー上でバッファーに入れられたメッセージの最大数。 (BufferedMsgsHWM)",
					msgBufHWMPercent: "最大のピーク %",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "バッファーに入れることが可能なメッセージの最大数に対する、バッファーに入れられているメッセージのピーク数のパーセント。 (BufferedHWMPercent)",
					perMsgBuf: "最大の現行 %",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "バッファーに入れられたメッセージの最大数に対する、バッファーに入れられたメッセージ数のパーセント。 (BufferedPercent)",
					totMsgProd: "生成",
					// TRNOTE Do not translate "(ProducedMsgs)"
					totMsgProdTooltip: "サーバー始動とキュー作成のいずれか後に生じた方の時点以降にキューに送信されたメッセージの数。 (ProducedMsgs)",
					totMsgCons: "消費",
					// TRNOTE Do not translate "(ConsumedMsgs)"
					totMsgConsTooltip: "サーバー始動とキュー作成のいずれか後に生じた方の時点以降にキューから消費されたメッセージの数。 (ConsumedMsgs)",
					totMsgRej: "拒否",
					// TRNOTE Do not translate "(RejectedMsgs)"
					totMsgRejTooltip: "バッファーに入れられたメッセージが最大数に達したため、キューに送信されないメッセージの数。 (RejectedMsgs)",
					totMsgExp: "期限切れ",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					totMsgExpTooltip: "サーバー始動とキュー作成のいずれか後に生じた方の時点以降にキュー上で期限切れになったメッセージの数。 (ExpiredMsgs)",			
					numProd: "プロデューサー",
					// TRNOTE Do not translate "(Producers)"
					numProdTooltip: "キュー上でアクティブなプロデューサーの数。 (Producers)", 
					numCons: "コンシューマー",
					// TRNOTE Do not translate "(Consumers)"
					numConsTooltip: "キュー上でアクティブなコンシューマーの数。 (Consumers)",
					maxMsgs: "最大",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgsTooltip: "キュー上でバッファーに入れることができるメッセージ数の最大値。 この値は、絶対的な制限ではなく指標です。 システムに負荷がかかる状態で稼働している場合、キュー上のバッファーに入れられたメッセージ数は、MaxMessages 値に比べて少し高い数値の可能性があります。 (MaxMessages)"
				},
				destmapping: {
					ruleName: "ルール名",
					qmConnection: "キュー・マネージャー接続",
					commits: "コミット",
					rollbacks: "ロールバック",
					committedMsgs: "コミット済みメッセージ",
					persistentMsgs: "永続メッセージ",
					nonPersistentMsgs: "非永続メッセージ",
					status: "状況",
					// TRNOTE Do not translate "(RuleName)"
					ruleNameTooltip: "モニター対象の宛先マッピング・ルールの名前。 (RuleName)",
					// TRNOTE Do not translate "(QueueManagerConnection)"
					qmConnectionTooltip: "宛先マッピング・ルールを関連付けるキュー・マネージャー接続の名前。 (QueueManagerConnection)",
					// TRNOTE Do not translate "(CommitCount)"
					commitsTooltip: "宛先マッピング・ルールについて完了したコミット操作の数。 1 回のコミット操作で多くのメッセージがコミットされる場合があります。 (CommitCount)",
					// TRNOTE Do not translate "(RollbackCount)"
					rollbacksTooltip: "宛先マッピング・ルールについて完了したロールバック操作の数。 (RollbackCount)",
					// TRNOTE Do not translate "(CommittedMessageCount)"
					committedMsgsTooltip: "宛先マッピング・ルールについてコミットされたメッセージの数。 (CommittedMessageCount)",
					// TRNOTE Do not translate "(PersistedCount)"
					persistentMsgsTooltip: "宛先マッピング・ルールを使用して送信される永続メッセージの数。 (PersistentCount)",
					// TRNOTE Do not translate "(NonpersistentCount)"
					nonPersistentMsgsTooltip: "宛先マッピング・ルールを使用して送信される非永続メッセージの数。 (NonpersistentCount)",
					// TRNOTE Do not translate "(Status)"
					statusTooltip: "ルールの状況: 有効、無効、再接続、再始動。 (Status)"
				},
				mqtt: {
					clientId: "クライアント ID",
					lastConn: "最終接続日",
					// TRNOTE Do not translate "(ClientId)"
					clientIdTooltip: "クライアント ID。 (ClientId)",
					// TRNOTE Do not translate "(LastConnectedTime)"
					lastConnTooltip: "クライアントが ${IMA_PRODUCTNAME_SHORT} サーバーに最後に接続した時刻。 (LastConnectedTime)",	
					errorMessage: "理由",
					errorMessageTooltip: "クライアントを削除できなかった理由。"
				},
				subscription: {
					subsName: "名前",
					// TRNOTE Do not translate "(SubName)"
					subsNameTooltip: "サブスクリプションに関連する名前。 非永続サブスクリプションの場合、この値として空ストリングが可能です。 (SubName)",
					topicString: "トピック・ストリング",
					// TRNOTE Do not translate "(TopicString)"
					topicStringTooltip: "サブスクリプションがサブスクライブされるトピック。 (TopicString)",					
					clientId: "クライアント ID",
					// TRNOTE Do not translate "(ClientID)"
					clientIdTooltip: "サブスクリプションを所有するクライアントのクライアント ID。 (ClientID)",
					consumers: "コンシューマー",
					// TRNOTE Do not translate "(Consumers)"
					consumersTooltip: "サブスクリプションのコンシューマーの数。 (Consumers)",
					msgPublished: "公開済み",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPublishedTooltip: "サーバー始動とサブスクリプション作成のいずれか後で生じた方の時点以降に、このサブスクリプションに公開されたメッセージの数。 (PublishedMsgs)",
					msgBuf: "現在",
					// TRNOTE Do not translate "(BufferedMsgs)"
					msgBufTooltip: "クライアントへの送信待ちになっている公開済みメッセージの数。 (BufferedMsgs)",
					perMsgBuf: "最大の現行 %",
					// TRNOTE Do not translate "(BufferedPercent)"
					perMsgBufTooltip: "バッファーに入れるメッセージの最大数に対する、現在バッファーに入れられているメッセージによって表されるパーセント。 (BufferedPercent)",
					msgBufHWM: "ピーク",
					// TRNOTE Do not translate "(BufferedMsgsHWM)"
					msgBufHWMTooltip: "サーバー始動またはサブスクリプション作成のいずれか後に生じた時点以降に、バッファーに入れられたメッセージの最大数。 (BufferedMsgsHWM)",
					msgBufHWMPercent: "最大のピーク %",
					// TRNOTE Do not translate "(BufferedHWMPercent)"
					msgBufHWMPercentTooltip: "バッファーに入れることが可能なメッセージの最大数に対する、バッファーに入れられているメッセージのピーク数のパーセント。 (BufferedHWMPercent)",
					maxMsgBuf: "最大",
					// TRNOTE Do not translate "(MaxMessages)"
					maxMsgBufTooltip: "このサブスクリプションでバッファーに入れることができるメッセージの最大数。 (MaxMessages)",
					rejMsg: "拒否",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "メッセージをサブスクリプションに公開する際、バッファーに入れられたメッセージが最大数に達したために拒否されたメッセージの数。 (RejectedMsgs)",
					expDiscMsg: "期限切れ / 廃棄",
					// TRNOTE Do not translate "(ExpiredMsgs + DiscardedMsgs)"
					expDiscMsgTooltip: "期限切れになったか、またはバッファーがいっぱいになったときに廃棄されたため、配信されないメッセージの数。 (ExpiredMsgs + DiscardedMsgs)",
					expMsg: "期限切れ",
					// TRNOTE Do not translate "(ExpiredMsgs)"
					expMsgTooltip: "期限切れになったため、配信されないメッセージの数。 (ExpiredMsgs)",
					DiscMsg: "廃棄済み",
					// TRNOTE Do not translate "(DiscardedMsgs)"
					DiscMsgTooltip: "バッファーがいっぱいになった時点で廃棄されたため、配信されないメッセージの数。 (DiscardedMsgs)",
					subsType: "タイプ",
					// TRNOTE Do not translate "(IsDurable, IsShared)"
					subsTypeTooltip: "この列は、サブスクリプションが永続か非永続か、またサブスクリプションが共用されているかどうかを示します。 永続サブスクリプションは、サブスクリプションが明示的に削除されるのでない限り、サーバー再始動後も存続します。 (IsDurable, IsShared)",				
					isDurable: "永続",
					isShared: "共用",
					notDurable: "非永続",
					noName: "(なし)",
					// TRNOTE {0} and {1} contain a formatted integer number greater than or equal to zero, e.g. 5,250
					expDiscMsgCellTitle: "期限切れ: {0}; 廃棄: {1}",
					errorMessage: "理由",
					errorMessageTooltip: "サブスクリプションを削除できなかった理由。",
					messagingPolicy: "メッセージング・ポリシー",
					// TRNOTE Do not translate "(MessagingPolicy)"
					messagingPolicyTooltip: "サブスクリプションによって使用中のメッセージング・ポリシーの名前。 (MessagingPolicy)"						
				},
				transaction: {
					// TRNOTE Do not translate "(XID)"
					xId: "トランザクション ID (XID)",
					timestamp: "タイム・スタンプ",
					state: "状態",
					stateCode2: "準備済み",
					stateCode5: "ヒューリスティック・コミット",
					stateCode6: "ヒューリスティック・ロールバック"
				},
				topic: {
					topicSubtree: "トピック・ストリング",
					msgPub: "公開されたメッセージ",
					rejMsg: "拒否されたメッセージ",
					rejPub: "公開失敗",
					numSubs: "サブスクライバー",
					// TRNOTE Do not translate "(TopicString)"
					topicSubtreeTooltip: "モニター対象のトピック。 トピック・ストリングには、常にワイルドカードが含まれます。 (TopicString)",
					// TRNOTE Do not translate "(PublishedMsgs)"
					msgPubTooltip: "ワイルドカード使用のトピック・ストリングに一致するトピックに対して公開が成功したメッセージの数。 (PublishedMsgs)",
					// TRNOTE Do not translate "(RejectedMsgs)"
					rejMsgTooltip: "サービス品質レベルのために公開要求が失敗することはなかったものの、1 つ以上のサブスクリプションによって拒否されたメッセージの数。 (RejectedMsgs)",
					// TRNOTE Do not translate "(FailedPublishes)"
					rejPubTooltip: "1 つ以上のサブスクリプションによってメッセージが拒否されたため、失敗した公開要求の数。 (FailedPublishes)",
					// TRNOTE Do not translate "(Subscriptions)"
					numSubsTooltip: "モニター対象のトピック上でアクティブなサブスクリプションの数。 図は、ワイルドカード使用のトピック・ストリングに一致するアクティブ・サブスクリプションのすべてを示します。 (Subscriptions)"
				},
				store: {
					name: "統計",
					value: "値",
                    diskUsedPercent: "使用ディスク (%)",
                    diskFreeBytes: "空きディスク (MB)",
					persistentMemoryUsedPercent: "使用メモリー (%)",
					memoryTotalBytes: "メモリー合計 (MB)",
					Pool1TotalBytes: "プール 1 の合計 (MB)",
					Pool1UsedBytes: "プール 1 の使用量 (MB)",
					Pool1UsedPercent: "プール 1 の使用率 (%)",
					Pool1RecordLimitBytes: "プール 1 のレコード限度 (MB)", 
					Pool1RecordsUsedBytes: "プール 1 のレコード使用量 (MB)",
                    Pool2TotalBytes: "プール 2 の合計 (MB)",
                    Pool2UsedBytes: "プール 2 の使用量 (MB)",
                    Pool2UsedPercent: "プール 2 の使用率 (%)"
				},
				memory: {
					name: "統計",
					value: "値",
					memoryTotalBytes: "合計メモリー (MB)",
					memoryFreeBytes: "空きメモリー (MB)",
					memoryFreePercent: "空きメモリー (%)",
					serverVirtualMemoryBytes: "仮想メモリー (MB)",
					serverResidentSetBytes: "常駐セット (MB)",
					messagePayloads: "メッセージ・ペイロード (MB)",
					publishSubscribe: "公開サブスクライブ (MB)",
					destinations: "宛先 (MB)",
					currentActivity: "現行アクティビティー (MB)",
					clientStates: "クライアント状態 (MB)"
				}
			},
			logs: {
				title: "ログ",
				pageSummary: "", 
				downloadLogsSubTitle: "ログのダウンロード",
				downloadLogsTagline: "診断のためにログをダウンロードします。",
				lastUpdated: "最終更新日時",
				name: "ログ・ファイル",
				size: "サイズ (バイト)"
			},
			lastUpdated: "最終更新日時: ",
			bufferedMsgs: "バッファーに入れられているメッセージ:",
			retainedMsgs: "保存メッセージ:",
			dataCollectionInterval: "データ収集間隔: ",
			interval: {fiveSeconds: "5 秒", oneMinute: "1 分" },
			cacheInterval: "データ収集間隔: 1 分",
			notAvailable: "n/a",
			savingProgress: "保管中...",
			savingFailed: "保管に失敗しました。",
			noRecord: "サーバーにレコードが見つかりませんでした。",
			deletingProgress: "削除中...",
			deletingFailed: "削除に失敗しました。",
			deletePending: "削除保留中です。",
			deleteSuccessful: "削除に成功しました。",
			testingProgress: "テスト中...",
			dialog: {
				// TRNOTE Do not translate "$SYS"
				addTopicMonitorInstruction: "モニター対象のトピック・ストリングを指定します。 このストリングの先頭を $SYS にすることはできず、末尾は複数レベル・ワイルドカードでなければなりません (例: /animals/dogs/#)。 " +
						                    "すべてのトピックの統計を集約するには、単一の複数レベル・ワイルドカード (#) を指定します。 " +
						                    "あるトピックに子トピックがない場合、そのトピックのみのデータがモニターで表示されます。  " +
						                    "例えば、/animals/dogs/labradors/# の場合、labradors に子トピックがない場合、トピック /animals/dogs/labradors のみモニターされます。",
				addTopicMonitorTitle: "トピック・モニターの追加",
				removeTopicMonitorTitle: "トピック・モニターの削除",
				removeSubscriptionTitle: "永続サブスクリプションの削除",
				removeSubscriptionsTitle: "永続サブスクリプションの削除",
				removeSubscriptionContent: "このサブスクリプションを削除しますか?",
				removeSubscriptionsContent: "{0} 個のサブスクリプションを削除しますか?",
				removeClientTitle: "MQTT クライアントの削除",
				removeClientsTitle: "MQTT クライアントの削除",
				removeClientContent: "このクライアントを削除しますか? クライアントを削除すると、そのクライアントのサブスクリプションも削除されます。",
				removeClientsContent: "{0} 個のクライアントを削除しますか? クライアントを削除すると、そのクライアントのサブスクリプションも削除されます。",
				topicStringLabel: "トピック・ストリング:",
				descriptionLabel: "説明",
				topicStringTooltip: "モニター対象のトピック・ストリング。  必須である末尾のワイルドカード (#) を除き、トピック・ストリングの中で以下の文字を使用することはできません: + #",
				saveButton: "保管",
				cancelButton: "キャンセル"
			},
			removeDialog: {
				title: "項目の削除",
				content: "このレコードを削除してもよろしいですか?",
				deleteButton: "削除",
				cancelButton: "キャンセル"
			},
			actionConfirmDialog: {
				titleCommit: "トランザクションのコミット",
				titleRollback: "トランザクションのロールバック",
				titleForget: "トランザクションの無視",
				contentCommit: "このトランザクションをコミットしますか?",
				contentRollback: "このトランザクションをロールバックしますか?",
				contentForget: "このトランザクションを無視しますか?",
				actionButtonCommit: "コミット",
				actionButtonRollback: "ロールバック",
				actionButtonForget: "無視",
				cancelButton: "キャンセル",
				failedRollback: "トランザクションをロールバックしようとして、エラーが発生しました。",
				failedCommit: "トランザクションをコミットしようとして、エラーが発生しました。",
				failedForget: "トランザクションを無視しようとして、エラーが発生しました。"
			},
			asyncInfoDialog: {
				title: "削除保留中",
				content: "サブスクリプションをすぐに削除することができませんでした。 可能な限り速やかに削除されます。",
				closeButton: "閉じる"
			},
			deleteResultsDialog: {
				title: "サブスクリプション削除状況",
				clientsTitle: "クライアント削除状況",
				allSuccessful:  "すべてのサブスクリプションが正常に削除されました。",
				allClientsSuccessful: "すべてのクライアントが正常に削除されました。",
				pendingTitle: "削除保留中",
				pending: "以下のサブスクリプションは、すぐに削除することができませんでした。 これらは可能な限り速やかに削除されます。",
				clientsPending: "以下のクライアントは、すぐに削除することができませんでした。 これらは可能な限り速やかに削除されます。",
				failedTitle: "削除失敗",
				failed:  "以下のサブスクリプションは削除できませんでした。",
				clientsFailed: "以下のクライアントは削除できませんでした。",
				closeButton: "閉じる"					
			},
			invalidName: "その名前のレコードはすでに存在します。",
			// TRNOTE Do not translate "$SYS"
			reservedName: "トピック・ストリングの先頭を $SYS にすることはできません",
			invalidChars: "次の文字は使用できません: + #",			
			invalidTopicMonitorFormat: "トピック・ストリングは # であるか、または /# で終了しなければなりません。",
			invalidRequired: "値が必要です。",
			clientIdMissingMessage: "この値は必須です。 ワイルドカードとして 1 つ以上のアスタリスク (*) を使用できます。",
			ruleNameMissingMessage: "この値は必須です。 ワイルドカードとして 1 つ以上のアスタリスク (*) を使用できます。",
			queueNameMissingMessage: "この値は必須です。 ワイルドカードとして 1 つ以上のアスタリスク (*) を使用できます。",
			subscriptionNameMissingMessage: "この値は必須です。 ワイルドカードとして 1 つ以上のアスタリスク (*) を使用できます。",
			discardOldestMessagesInfo: "最大メッセージ動作が<em>「古いメッセージ廃棄」</em>であるメッセージング・ポリシーを使用するエンドポイントが少なくとも 1 つ有効になっていなければなりません。  廃棄されたメッセージは、<em>「拒否されたメッセージ」</em>または<em>「公開失敗」</em>のカウントには影響しません。",
			// Strings for viewing and modifying SNMP Configuration
			snmp: {
				title: "SNMP 設定",
				tagline: "コミュニティー・ベースの Simple Network Management Protocol (SNMPv2C) を使用可能にし、管理情報ベース (MIB) ファイルをダウンロードし、SNMPv2C コミュニティーを構成します。 " +
						"${IMA_PRODUCTNAME_FULL} は SNMPv2C をサポートしています。",
				status: {
					title: "SNMP 状況",
					tagline: {
						disabled: "SNMP は使用不可です。",
						enabled: "SNMP は使用可能であり、構成済みです。",
						warning: "サーバー上の SNMP データへのアクセスを可能にするには、SNMP コミュニティーを定義する必要があります。"
					},
					enableLabel: "SNMPv2C エージェントの使用可能化",
					addressLabel: "SNMP エージェントのアドレス:",					
					editLinkLabel: "編集",
					editAddressDialog: {
						title: "SNMP エージェントのアドレスの編集",
						instruction: "${IMA_PRODUCTNAME_FULL} が SNMP で使用できる静的 IP アドレスを指定します。 " +
								"${IMA_PRODUCTNAME_SHORT} サーバーで構成されているあらゆる IP アドレスを許可する場合、またはサーバーで動的 IP アドレスのみを使用している場合、<em>「すべて」</em> を選択します。"
					},
					getSnmpEnabledError: "SNMP 使用可能状態の取得中にエラーが発生しました。",
					setSnmpEnabledError: "SNMP 使用可能状態の設定中にエラーが発生しました。",
					getSnmpAgentAddressError: "SNMP エージェントのアドレスを取得中にエラーが発生しました。",
					setSnmpAgentAddressError: "SNMP エージェントのアドレスを設定中にエラーが発生しました。"
				},
				mibs: {
					title: "エンタープライズ MIB",
					tagline: "エンタープライズ MIB ファイルは、組み込み SNMP エージェントで使用可能な機能とデータを記述して、クライアントが適切にこれらの機能とデータにアクセスできるようにします。",
					messageSightLinkLabel: "${IMA_PRODUCTNAME_SHORT} MIB",
					hwNotifyLinkLabel: "HW 通知 MIB"
				},
				communities: {
					title: "SNMP コミュニティー",
					tagline: "1 つ以上の SNMPv2C コミュニティーを作成することで、サーバー上の SNMP データへのアクセスを定義できます。 " +
							"モニターが有効になっている場合は SNMP コミュニティーが必要です。 " +
							"すべてのコミュニティーに、読み取り専用権限が付与されます。",
					grid: {
						nameColumn: "名前",
						hostRestrictionColumn: "ホスト制限"
					},
					dialog: {
						addTitle: "SNMP コミュニティーの追加",
						editTitle: "SNMP コミュニティーの編集",
						instruction: "SNMP コミュニティーは、サーバー上の SNMP データへのアクセスを可能にします。",
						deleteTitle: "SNMP コミュニティーの削除",
						deletePrompt: "この SNMP コミュニティーを削除しますか?",
						deleteNotAllowedTitle: "削除は許可されていません", 
						deleteNotAllowedMessage: "1 つ以上のトラップ・サブスクライバーによって使用されているため、SNMP コミュニティーを削除できません。 " +
								"すべてのトラップ・サブスクライバーから SNMP コミュニティーを削除してから、再試行してください。",
						defineTitle: "SNMP コミュニティーの定義",
						defineInstruction: "サーバー上の SNMP データへのアクセスを可能にするには、SNMP コミュニティーを定義する必要があります。",
						nameLabel: "名前",
						nameInvalid: "コミュニティー・ストリングに引用符を含めることはできません。",
						hostRestrictionLabel: "ホスト制限",
						hostRestrictionTooltip: "指定すると、指定ホストへのアクセスが制限されます。 " +
								"このフィールドを空にすると、すべての IP アドレスとの通信が許可されます。 " +
								"ホスト名またはアドレスのコンマ区切りリストを指定します。 " +
								"サブネットを指定するには、クラスレス・ドメイン間ルーティング (CIDR) IP アドレスまたはホスト名を使用します。",
						hostRestrictionInvalid: "ホスト制限は、空にするか、あるいは有効な IP アドレス、ホスト名、またはドメインを含むコンマ区切りリストを指定する必要があります。"
					},
					getCommunitiesError: "SNMP コミュニティーの取得中にエラーが発生しました。",
					saveCommunityError: "SNMP コミュニティーの保管中にエラーが発生しました。",
					deleteCommunityError: "SNMP コミュニティーの削除中にエラーが発生しました。"					
				},
				trapSubscribers: {
					title: "トラップ・サブスクライバー",
					tagline: "トラップ・サブスクライバーとは、サーバーに組み込まれた SNMP エージェントとの通信に使用される SNMP クライアントのことです。 " +
							"SNMP トラップ・サブスクライバーは、既存の SNMP クライアントである必要があります。 " +
							"トラップを受信するように SNMP クライアントを構成してから、トラップ・サブスクライバーを作成してください。 ",
					grid: {
						hostColumn: "ホスト",
						portColumn: "クライアント・ポート番号",
						communityColumn: "コミュニティー"
					},
					dialog: {
						addTitle: "トラップ・サブスクライバーの追加",
						editTitle: "トラップ・サブスクライバーの編集",
						instruction: "トラップ・サブスクライバーとは、サーバーに組み込まれた SNMP エージェントとの通信に使用される SNMP クライアントのことです。",
						deleteTitle: "トラップ・サブスクライバーの削除",
						deletePrompt: "このトラップ・サブスクライバーを削除しますか?",
						hostLabel: "ホスト",
						hostTooltip: "SNMP クライアントがトラップ情報を待機する対象の IP アドレス。",
						hostInvalid: "有効な IP アドレスが必要です。",
				        duplicateHost: "そのホストに対するトラップ・サブスクライバーは既に存在します。",
						portLabel: "クライアント・ポート番号",
						portTooltip: "SNMP クライアントがトラップ情報を待機しているポート。 " +
								"有効なポートは 1 から 65535 までです。",
						portInvalid: "ポートは、1 から 65535 までの数値でなければなりません。",
						communityLabel: "コミュニティー",
	                    noCommunitiesTitle: "コミュニティーなし",
	                    noCommunitiesDetails: "トラップ・サブスクライバーを定義するには、その前にコミュニティーを定義する必要があります。"						
					},
					getTrapSubscribersError: "SNMP トラップ・サブスクライバーの取得中にエラーが発生しました。",
					saveTrapSubscriberError: "SNMP トラップ・サブスクライバーの保管中にエラーが発生しました。",
					deleteTrapSubscriberError: "SNMP トラップ・サブスクライバーの削除中にエラーが発生しました。"										
				},
				okButton: "OK",
				saveButton: "保管",
				cancelButton: "キャンセル",
				closeButton: "閉じる"
			}
		}
});
