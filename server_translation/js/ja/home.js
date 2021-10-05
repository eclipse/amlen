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
		// Home
		// ------------------------------------------------------------------------
		home : {
			title : "ホーム",
			taskContainer : {
				title : "一般的な構成およびカスタマイズ・タスク",
				tagline: "一般的な管理用タスクへのクイック・リンクです。",
				// TRNOTE {0} is a number from 1 - 5.
				taskCount : "(残り {0} 個のタスク)",
				links : {
					restore : "タスクの復元",
					restoreTitle: "「一般的な構成およびカスタマイズ・タスク」セクションで閉じられたタスクを復元します。",
					hide : "セクションの非表示",
					hideTitle: "「一般的な構成およびカスタマイズ・タスク」セクションを非表示にします。 " +
							"このセクションを復元するには、「ヘルプ」メニューの「ホーム・ページ」から「タスクの復元」を選択します。"						
				}
			},
			tasks : {
				messagingTester : {
					heading: "${IMA_PRODUCTNAME_SHORT} の Messaging Tester サンプル・アプリケーションを使用して構成を検証",
					description: "Messaging Tester は、サーバーが適切に構成されているかどうかを MQTT over WebSockets を使用して検証するシンプルな HTML5 サンプル・アプリケーションです。",
					links: {
						messagingTester: "Messaging Tester サンプル・アプリケーション"
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
					heading : "ユーザーの作成",
					description : "Web UI へのアクセス権限をユーザーに付与します。",
					links : {
						users : "Web UI ユーザー",
						userGroups : "LDAP 構成"						
					}
				},
				connections: {
					heading: "接続を受け入れるように ${IMA_PRODUCTNAME_FULL} を構成",
					description: "サーバー接続を管理するためのメッセージング・ハブを定義します。  ${IMA_PRODUCTNAME_FULL} から MQ キュー・マネージャーに接続するように MQ Connectivity を構成します。 メッセージング・ユーザーを認証するように LDAP を構成します。",
					links: {
						listeners: "メッセージング・ハブ",
						mqconnectivity: "MQ Connectivity"
					}
				},
				security : {
					heading : "環境の保護",
					description : "Web UI が待機するインターフェースおよびポートを制御します。 鍵と証明書をインポートして、サーバーへの接続を保護します。",
					links : {
						keysAndCerts : "サーバー・セキュリティー",
						webuiSettings : "Web UI 設定"
					}
				}
			},
			dashboards: {
				tagline: "サーバー接続およびパフォーマンスの概要。",
				monitoringNotAvailable: "${IMA_PRODUCTNAME_SHORT} サーバーの状況が原因でモニターが使用できません。",
				flex: {
					title: "サーバー・ダッシュボード",
					// TRNOTE {0} is a user provided name for the server.
					titleWithNodeName: "{0} のサーバー・ダッシュボード",
					quickStats: {
						title: "クイック統計"
					},
					liveCharts: {
						title: "アクティブな接続とスループット"
					},
					applianceResources: {
						title: "サーバーのメモリー使用率"
					},
					resourceDetails: {
						title: "ストア・メモリー使用率"
					},
					diskResourceDetails: {
						title: "ディスク使用率"
					},
					activeConnectionsQS: {
						title: "アクティブ接続数",
						description: "アクティブな接続の数。",
						legend: "アクティブ接続"
					},
					throughputQS: {
						title: "現在のスループット",
						description: "現在のスループット (メッセージ数/秒)。",
						legend: "メッセージ/秒"
					},
					uptimeQS: {
						title: "${IMA_PRODUCTNAME_FULL} の実行時間",
						description: "${IMA_PRODUCTNAME_FULL} の状況と、これまでに稼働してきた時間。",
						legend: "${IMA_PRODUCTNAME_SHORT} 実行時間"
					},
					applianceQS: {
						title: "サーバーのリソース",
						description: "使用されているパーシスタント・ストア・ディスクおよびサーバー・メモリーのパーセンテージ。",
						bars: {
							pMem: {
								label: "パーシスタント・メモリー:"								
							},
							disk: { 
								label: "ディスク:",
								warningThresholdText: "リソース使用率が警告しきい値以上です。",
								alertThresholdText: "リソース使用率がアラートしきい値以上です。"
							},
							mem: {
								label: "メモリー:",
								warningThresholdText: "メモリー使用率が上昇しています。 メモリー使用率が 85% に達するとパブリケーションが拒否されるほか、接続が除去される可能性もあります。",
								alertThresholdText: "メモリー使用率が高すぎます。 パブリケーションが拒否されるほか、接続が除去される可能性もあります。"	
							}
						},
						warningThresholdAltText: "警告アイコン",
						alertThresholdText: "リソース使用率がアラートしきい値以上です。",
						alertThresholdAltText: "アラート・アイコン"
					},
					connections: {
						title: "接続",
						description: "接続のスナップショットです (約 5 秒ごとに取得)。",
						legend: {
							x: "時間",
							y: "接続"
						}
					},
					throughput: {
						title: "スループット",
						description: "「メッセージ/秒」のスナップショットです (約 5 秒ごとに取得)。",
						legend: {
							x: "時間",						
							y: "メッセージ/秒",
							title: "メッセージ/秒",
							incoming: "着信",
							outgoing: "発信",
							hover: {
								incoming: "クライアントからの読み取りメッセージ/秒のスナップショットです (約 5 秒ごとに取得)。",
								outgoing: "クライアントへの書き込みメッセージ/秒のスナップショットです (約 5 秒ごとに取得)。"
							}
						}
					},
					memory: {
						title: "メモリー",
						description: "メモリー使用率のスナップショットです (約 1 分ごとに取得)。",
						legend: {
							x: "時間",
							y: "使用メモリー (%)"
						}
					},
					disk: {
						title: "ディスク",
						description: "パーシスタント・ストア・ディスク使用率のスナップショットです (約 1 分ごとに取得)。",
						legend: {
							x: "時間",
							y: "使用ディスク (%)"
						}						
					},
					memoryDetail: {
						title: "メモリー使用率の詳細",
						description: "主要なメッセージング・リソースのメモリー使用率のスナップショットです (約 1 分ごとに取得)。",
						legend: {
							x: "時間",
							y: "使用メモリー (%)",
							title: "メッセージング・メモリー使用率",
							system: "サーバー・ホスト・システム",
							messagePayloads: "メッセージ・ペイロード",
							publishSubscribe: "パブリッシュ/サブスクライブ",
							destinations: "宛先",
							currentActivity: "現在のアクティビティー",
							clientStates: "クライアントの状態",
							hover: {
								system: "オペレーティング・システム、接続、セキュリティー、ロギング、管理など、その他のシステム用途に割り振られたメモリー。",
								messagePayloads: "メッセージに割り振られたメモリー " +
										"1 つのメッセージが複数のサブスクライバーにパブリッシュされた場合、メッセージの 1 つのコピーのみがメモリーに割り振られます。 " +
										"保存メッセージのためのメモリーも、このカテゴリーに割り振られます。 " +
										"コンシューマーが切断された、または低速であることが原因で大量のメッセージをサーバー内でバッファーに入れるワークロードの場合や、多数の保存メッセージを使用するワークロードの場合に、大量のメッセージ・ペイロード・メモリーが消費されることがあります。",
								publishSubscribe: "パブリッシュ/サブスクライブ・メッセージングを実行するために割り振られたメモリー。 " +
										"サーバーは、保存メッセージおよびサブスクリプションを追跡するためにこのカテゴリーにメモリーを割り当てます。 " +
										"サーバーは、パフォーマンス上の理由で、パブリッシュ/サブスクライブ情報のキャッシュを維持します。 " +
										"多数のサブスクリプションを使用するワークロードや、多数の保存メッセージを使用するワークロードの場合に、大量のパブリッシュ/サブスクライブ・メモリーが消費されることがあります。",
								destinations: "メッセージングの宛先のために割り振られるメモリー。 " +
										"このカテゴリーのメモリーは、メッセージを、クライアントが使用するキューまたはサブスクリプションに編成するために使用されます。 " +
										"サーバーで多数のメッセージをバッファーに入れるワークロードや、多数のサブスクリプションを使用するワークロードの場合に、大量の宛先メモリーを消費することがあります。",
								currentActivity: "現在のアクティビティーに割り振られるメモリー。 " +
										"これには、セッション、トランザクション、およびメッセージ確認応答が含まれます。 " +
										"モニター要求を満たすための情報も、このカテゴリーに割り振られます。 " +
										"多数のクライアントが接続される複雑なワークロードや、トランザクションまたは無応答メッセージなどの機能を大規模に使用するワークロードの場合に、現在のアクティビティーのメモリーが大量に消費されることがあります。",
								clientStates: "接続されたクライアントと切断されたクライアントのために割り振られるメモリー。 " +
										"サーバーは、接続された各クライアントのためにこのカテゴリーのメモリーを割り振ります。 " +
										"MQTT では、CleanSession を 0 に設定して接続したクライアントが切断される時、それらのクライアントについて記憶しておく必要があります。 " +
										"さらに、MQTT で着信および発信メッセージの確認応答を追跡するために、このカテゴリーからメモリーが割り振られます。 " +
										"接続されたクライアントおよび切断されたクライアントを多数使用するワークロードの場合、特にサービスの品質が高いメッセージを使用している場合に、大量のクライアント状態メモリーが消費されることがあります。"								
							}
						}
					},
					storeMemory: {
						title: "パーシスタント・ストア・メモリー使用率の詳細",
						description: "パーシスタント・ストア・メモリー使用率のスナップショットです (約 1 分ごとに取得)。",
						legend: {
							x: "時間",
							y: "使用メモリー (%)",
							pool1Title: "プール 1 メモリー使用率",
							pool2Title: "プール 2 メモリー使用率",
							system: "サーバー・ホスト・システム",
							IncomingMessageAcks: "着信メッセージの確認応答",
							MQConnectivity: "MQ Connectivity",
							Transactions: "トランザクション",
							Topics: "保存メッセージがあるトピック",
							Subscriptions: "サブスクリプション",
							Queues: "キュー",										
							ClientStates: "クライアントの状態",
							recordLimit: "レコードの限度",
							hover: {
								system: "システムで予約済みまたは使用中のストア・メモリー。",
								IncomingMessageAcks: "着信メッセージの確認応答用に割り振られたストア・メモリー。 " +
										"CleanSession=0 の設定を使用して接続し、サービスの品質 2 のメッセージをパブリッシュする MQTT クライアントに対して、サーバーはこのカテゴリーのメモリーを割り振ります。 " +
										"このメモリーは、1 回限りの送達を確実に実行するために使用されます。",
								MQConnectivity: "IBM WebSphere MQ キュー・マネージャーとの接続用に割り振られたストア・メモリー。",
								Transactions: "トランザクションのために割り振られたストア・メモリー。 " +
										"サーバーは、このカテゴリーのメモリーを各トランザクションに割り振り、サーバーの再始動時にリカバリーを完了できるようにします。",
								Topics: "トピックのために割り振られたストア・メモリー。 " +
										"サーバーは、保存メッセージがある各トピックに対してこのカテゴリーのメモリーを割り振ります。",
								Subscriptions: "永続サブスクリプションのために割り振られたストア・メモリー。 " +
										"MQTT では、これらは、CleanSession=0 の設定を使用して接続したクライアントに対するサブスクリプションです。",
								Queues: "キューのために割り振られたストア・メモリー。 " +
										"Point-to-Point メッセージング用に作成されるキューごとに、このカテゴリーにメモリーが割り振られます。",										
								ClientStates: "切断時に記憶しておく必要があるクライアントのために割り振られたストア・メモリー。 " +
										"MQTT では、これらは、CleanSession=0 の設定を使用して接続したクライアントです。あるいは、接続して、サービスの品質が 1 または 2 の Will メッセージを設定したクライアントです。",
								recordLimit: "レコードの限度に達したら、保存メッセージ、永続サブスクリプション、キュー、クライアント、WebSphere MQ キュー・マネージャーとの接続のためにレコードを作成できません。"
							}
						}						
					},					
					peakConnectionsQS: {
						title: "ピーク接続数",
						description: "指定された期間におけるアクティブ接続のピーク時の数。",
						legend: "ピーク接続数"
					},
					avgConnectionsQS: {
						title: "平均接続数",
						description: "指定された期間におけるアクティブ接続の平均数。",
						legend: "平均接続数"
					},
					peakThroughputQS: {
						title: "ピーク・スループット",
						description: "指定された期間における秒あたりメッセージ数のピーク値。",
						legend: "ピーク・メッセージ/秒"
					},
					avgThroughputQS: {
						title: "平均スループット",
						description: "指定された期間における秒あたりメッセージ数の平均値。",
						legend: "平均メッセージ/秒"
					}
				}
			},
			status: {
				rc0: "初期化中",
				rc1: "実行中 (実動)",
				rc2: "停止中",
				rc3: "初期化完了",
				rc4: "トランスポート開始",
				rc5: "プロトコル開始",
				rc6: "ストア開始",
				rc7: "エンジン開始",
				rc8: "メッセージング開始",
				rc9: "実行中 (保守)",
				rc10: "スタンバイ",
				rc11: "ストア開始中",
				rc12: "保守 (ストア消去の進行中)",
				rc99: "停止",				
				unknown: "不明",
				// TRNOTE {0} is a mode, such as production or maintenance.
				modeChanged: "サーバーが再始動したとき、サーバーは <em>{0}</em> モードになります。",
				cleanStoreSelected: "<em>ストア消去</em>アクションが要求されました。 これは、サーバーが再始動したときに有効になります。",
				mode_0: "実動",
				mode_1: "保守",
				mode_2: "ストア消去",
				adminError: "${IMA_PRODUCTNAME_FULL} がエラーを検出しました。",
				adminErrorDetails: "エラー・コードは {0} です。 エラー・ストリング: {1}"
			},
			storeStatus: {
				mode_0: "永続",
				mode_1: "メモリーのみ"
			},
			memoryStatus: {
				ok: "OK",
				unknown: "不明",
				error: "メモリー・チェックでエラーが報告されました。",
				errorMessage: "メモリー・チェックでエラーが報告されました。 サポートにお問い合わせください。"
			},
			role: {
				PRIMARY: "1 次ノード",
				PRIMARY_SYNC: "1 次ノード (同期中、{0}% 完了)",
				STANDBY: "スタンバイ・ノード",
				UNSYNC: "ノードが同期されていません",
				TERMINATE: "スタンバイ・ノードが 1 次ノードによって終了されました",
				UNSYNC_ERROR: "ノードを同期化できなくなりました",
				HADISABLED: "使用不可",
				UNKNOWN: "不明",
				unknown: "不明"
			},
			haReason: {
				"1": "2 つのノードのストア構成が適切でないため、HA ペアとして動作できません。 " +
				     "一致しない構成項目: {0}",
				"2": "ディスカバリー時間が満了しました。 HA ペアのもう一方のノードと通信できませんでした。",
				"3": "2 つの 1 次ノードが識別されました。",
				"4": "2 つの同期されていないノードが識別されました。",
				"5": "2 つのノードのストアがどちらも空でないため、どちらのノードを 1 次ノードにしたらよいか判別できませんでした。",
				"7": "2 つのノードのグループ ID が異なるため、ノードはリモート・ノードとのペア化に失敗しました。",
				"9": "HA サーバー・エラーまたは内部エラーが発生しました",
				// TRNOTE {0} is a time stamp (date and time).
				lastPrimaryTime: "このノードは、{0} に 1 次ノードでした。"
			},
			statusControl: {
				label: "状況",
				ismServer: "${IMA_PRODUCTNAME_FULL} サーバー",
				serverNotAvailable: "サーバー・システム管理者に連絡し、サーバーを始動するように依頼してください",
				serverNotAvailableNonAdmin: "サーバー・システム管理者に連絡し、サーバーを始動するように依頼してください",
				haRole: "高可用性:",
				pending: "保留中..."
			}
		},
		recordsLabel: "レコード",
		// Additions for cluster status to appear in status dropdown
		statusControl_cluster: "クラスター:",
		clusterStatus_Active: "アクティブ",
		clusterStatus_Inactive: "非アクティブ",
		clusterStatus_Removed: "削除済み",
		clusterStatus_Connecting: "接続中",
		clusterStatus_None: "未構成",
		clusterStatus_Unknown: "不明",
		// Additions to throughput chart on dashboard (also appears on cluster status page)
		//TRNOTE: Remote throughput refers to messages from remote servers that are members 
		// of the same cluster as the local server.
		clusterThroughputLabel: "リモート",
		// TRNOTE: Local throughput refers to messages that are received directly from
		// or that are sent directly to messaging clients that are connected to the
		// local server.
		clientThroughputLabel: "ローカル",
		clusterLegendIncoming: "着信",
		clusterLegendOutgoing: "発信",
		clusterHoverIncoming: "リモート・クラスター・メンバーからの読み取りメッセージ/秒のスナップショットです (約 5 秒ごとに取得)。",
		clusterHoverOutgoing: "リモート・クラスター・メンバーへの書き込みメッセージ/秒のスナップショットです (約 5 秒ごとに取得)。"

});
