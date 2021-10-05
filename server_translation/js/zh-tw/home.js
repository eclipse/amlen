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
			title : "首頁",
			taskContainer : {
				title : "共用配置及自訂作業",
				tagline: "一般管理作業的快速鏈結。",
				// TRNOTE {0} is a number from 1 - 5.
				taskCount : "（剩餘 {0} 個作業）",
				links : {
					restore : "還原作業",
					restoreTitle: "將結束的作業還原至一般配置及自訂作業區段。",
					hide : "隱藏區段",
					hideTitle: "隱藏一般配置及自訂作業區段。" +
							"若要還原區段，請從「說明」功能表中選取「在首頁上還原作業」。"						
				}
			},
			tasks : {
				messagingTester : {
					heading: "使用 ${IMA_PRODUCTNAME_SHORT} Messaging Tester 範例應用程式驗證您的配置",
					description: "Messaging Tester 是簡易的 HTML5 範例應用程式，可使用 MQTT over WebSocket 來驗證伺服器是否正確配置。",
					links: {
						messagingTester: "Messaging Tester 範例應用程式"
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
					heading : "建立使用者",
					description : "將 Web UI 的存取權提供給使用者。",
					links : {
						users : "Web UI 使用者",
						userGroups : "LDAP 配置"						
					}
				},
				connections: {
					heading: "配置 ${IMA_PRODUCTNAME_FULL} 以接受連線",
					description: "定義訊息中心，以管理伺服器連線。配置 MQ Connectivity 以將 ${IMA_PRODUCTNAME_FULL} 連接至 MQ 佇列管理程式。配置 LDAP 來鑑別傳訊使用者。",
					links: {
						listeners: "訊息中心",
						mqconnectivity: "MQ Connectivity"
					}
				},
				security : {
					heading : "保護環境安全",
					description : "控制 Web UI 接聽的介面與埠。匯入金鑰及憑證，以保護伺服器連線的安全。",
					links : {
						keysAndCerts : "伺服器安全",
						webuiSettings : "Web UI 設定"
					}
				}
			},
			dashboards: {
				tagline: "伺服器連線和效能的概觀。",
				monitoringNotAvailable: "因為 ${IMA_PRODUCTNAME_SHORT} 伺服器的狀態而無法使用監視。",
				flex: {
					title: "伺服器儀表板",
					// TRNOTE {0} is a user provided name for the server.
					titleWithNodeName: "{0} 的伺服器儀表板",
					quickStats: {
						title: "快速統計資料"
					},
					liveCharts: {
						title: "作用中的連線數及傳輸量"
					},
					applianceResources: {
						title: "伺服器記憶體用量"
					},
					resourceDetails: {
						title: "儲存庫記憶體用量"
					},
					diskResourceDetails: {
						title: "磁碟用量"
					},
					activeConnectionsQS: {
						title: "作用中的連線計數",
						description: "作用中的連線數。",
						legend: "作用中的連線數"
					},
					throughputQS: {
						title: "現行傳輸量",
						description: "訊息現行的每秒傳輸量。",
						legend: "訊息數/秒"
					},
					uptimeQS: {
						title: "${IMA_PRODUCTNAME_FULL} 執行時間",
						description: "${IMA_PRODUCTNAME_FULL} 的狀態及其執行時間。",
						legend: "${IMA_PRODUCTNAME_SHORT} 執行時間"
					},
					applianceQS: {
						title: "伺服器資源",
						description: "持續儲存庫磁碟及伺服器記憶體的已使用百分比。",
						bars: {
							pMem: {
								label: "持續性記憶體："								
							},
							disk: { 
								label: "磁碟：",
								warningThresholdText: "資源使用等於或大於警告臨界值。",
								alertThresholdText: "資源使用等於或大於警示臨界值。"
							},
							mem: {
								label: "記憶體：",
								warningThresholdText: "記憶體用量在變高。當記憶體用量達到 85% 時，將會拒絕出版品，並且連線可能會中斷。",
								alertThresholdText: "記憶體用量太高。將會拒絕出版品，並且連線可能會中斷。"	
							}
						},
						warningThresholdAltText: "警告圖示",
						alertThresholdText: "資源使用等於或大於警示臨界值。",
						alertThresholdAltText: "警示圖示"
					},
					connections: {
						title: "連線",
						description: "連線的 Snapshot，大約每 5 秒鐘拍攝一次。",
						legend: {
							x: "時間",
							y: "連線"
						}
					},
					throughput: {
						title: "傳輸量",
						description: "每秒訊息的 Snapshot，大約每 5 秒鐘拍攝一次。",
						legend: {
							x: "時間",						
							y: "訊息數/秒",
							title: "訊息數/秒",
							incoming: "送入",
							outgoing: "送出",
							hover: {
								incoming: "每秒從用戶端所讀取訊息的 Snapshot，大約每 5 秒鐘建立一次。",
								outgoing: "每秒寫入用戶端訊息的 Snapshot，大約每 5 秒鐘建立一次。"
							}
						}
					},
					memory: {
						title: "記憶體",
						description: "記憶體用量的 Snapshot，大約每分鐘拍攝一次。",
						legend: {
							x: "時間",
							y: "已用記憶體 (%)"
						}
					},
					disk: {
						title: "磁碟",
						description: "持續儲存庫磁碟用量的 Snapshot，大約每分鐘拍攝一次。",
						legend: {
							x: "時間",
							y: "已用磁碟 (%)"
						}						
					},
					memoryDetail: {
						title: "記憶體用量的詳細資料",
						description: "金鑰傳訊資源之記憶體用量的 Snapshot，大約每分鐘拍攝一次。",
						legend: {
							x: "時間",
							y: "已用記憶體 (%)",
							title: "傳訊記憶體用量",
							system: "伺服器主機系統",
							messagePayloads: "訊息內容",
							publishSubscribe: "發佈訂閱",
							destinations: "目的地",
							currentActivity: "現行活動",
							clientStates: "用戶端狀態",
							hover: {
								system: "為其他系統用途配置的記憶體，例如作業系統、連線、安全、記載及管理。",
								messagePayloads: "為訊息配置的記憶體。" +
										"將訊息發佈給多個訂閱者時，在記憶體中僅配置一個訊息副本。" +
										"已保留訊息的記憶體也會在此種類中配置。" +
										"在伺服器中為中斷連線或慢速消費者緩衝大量訊息的工作量，及使用大量已保留訊息的工作量，會耗用許多訊息內容記憶體。",
								publishSubscribe: "為執行發佈/訂閱傳訊所配置的記憶體。" +
										"伺服器會在此種類中配置記憶體，以追蹤保留的訊息及訂閱。" +
										"伺服器會因效能原因保留發佈/訂閱資訊的快取。" +
										"使用大量訂閱的工作量，及使用大量已保留訊息的工作量，會耗用許多發佈/訂閱記憶體。",
								destinations: "為傳訊目的地配置的記憶體。" +
										"此記憶體種類用於將訊息組織到用戶端使用的佇列及訂閱中。" +
										"在伺服器中緩衝大量訊息的工作量，及使用大量訂閱的工作量，會耗用許多目的地記憶體。",
								currentActivity: "為現行活動配置的記憶體。" +
										"這包括階段作業、交易及訊息確認。" +
										"在此各類中也會配置滿足監視要求的資訊。" +
										"使用大量已連接用戶端的複式工作量，以及廣泛使用交易或未確認訊息等特性的工作量，會耗用許多現行活動記憶體。",
								clientStates: "為已連接及中斷連線的用戶端配置的記憶體。" +
										"伺服器會在此種類中為每個已連接用戶端配置記憶體。" +
										"在 MQTT 中，當已連接的用戶端中斷連線時，必須記住將 CleanSession 設定為 0。" +
										"此外，會從此種類配置記憶體，以追蹤 MQTT 中的送入及送出訊息確認。" +
										"使用大量已連接及中斷連線用戶端的工作量，會耗用許多用戶端狀態記憶體，尤其是如果使用高服務品質的訊息。"								
							}
						}
					},
					storeMemory: {
						title: "持續儲存庫記憶體用量的詳細資料",
						description: "持續儲存庫記憶體用量的 Snapshot，大約每分鐘拍攝一次。",
						legend: {
							x: "時間",
							y: "已用記憶體 (%)",
							pool1Title: "儲存區 1 記憶體用量",
							pool2Title: "儲存區 2 記憶體用量",
							system: "伺服器主機系統",
							IncomingMessageAcks: "送入的訊息確認",
							MQConnectivity: "MQ Connectivity",
							Transactions: "交易",
							Topics: "具有保留訊息的主題",
							Subscriptions: "訂閱",
							Queues: "佇列",										
							ClientStates: "用戶端狀態",
							recordLimit: "記錄限制",
							hover: {
								system: "系統已保留或正在使用的儲存庫記憶體。",
								IncomingMessageAcks: "配置用於確認送入訊息的儲存庫記憶體。" +
										"對於使用 CleanSession=0 設定進行連接，且正在發佈服務品質為 2 的訊息之 MQTT 用戶端，伺服器會配置此種類的記憶體。" +
										"此記憶體可用於確保僅遞送一次。",
								MQConnectivity: "為與 IBM WebSphere MQ 佇列管理程式的連線功能所配置的儲存庫記憶體。",
								Transactions: "為交易配置的儲存庫記憶體。" +
										"伺服器會為每筆交易配置此種類的記憶體，使其在伺服器重新啟動時能夠完成回復。",
								Topics: "為主題配置的儲存庫記憶體。" +
										"伺服器會為具有保留訊息的每個主題配置此種類的記憶體。",
								Subscriptions: "為可延續訂閱配置的儲存庫記憶體。" +
										"在 MQTT 中，這些是使用 CleanSession=0 設定進行連接的用戶端之訂閱。",
								Queues: "為佇列配置的儲存庫記憶體。" +
										"對於為進行點對點傳訊而建立的每個佇列，將會配置此種類的記憶體。",										
								ClientStates: "為在中斷連線時必須記住的用戶端所配置的儲存庫記憶體。" +
										"在 MQTT 中，這些是使用 CleanSession=0 設定進行連接的用戶端，或是已連接且已設定服務品質為 1 或 2 的 will 訊息之用戶端。",
								recordLimit: "達到記錄限制時，將無法為保留的訊息、可延續訂閱、佇列、用戶端，以及與 WebSphere MQ 佇列管理程式的連線功能建立記錄。"
							}
						}						
					},					
					peakConnectionsQS: {
						title: "尖峰連線數",
						description: "在指定期間作用中的尖峰連線數。",
						legend: "尖峰連線數"
					},
					avgConnectionsQS: {
						title: "平均連線數",
						description: "在指定期間作用中連線的平均數。",
						legend: "平均連線數"
					},
					peakThroughputQS: {
						title: "尖峰傳輸量",
						description: "在指定期間的每秒尖峰訊息數。",
						legend: "尖峰訊息數/秒"
					},
					avgThroughputQS: {
						title: "平均傳輸量",
						description: "在指定期間的每秒平均訊息數。",
						legend: "平均訊息數/秒"
					}
				}
			},
			status: {
				rc0: "正在起始設定",
				rc1: "執行中（正式作業）",
				rc2: "停止中",
				rc3: "已起始設定",
				rc4: "傳輸已啟動",
				rc5: "通訊協定已啟動",
				rc6: "儲存庫已啟動",
				rc7: "引擎已啟動",
				rc8: "傳訊已啟動",
				rc9: "執行中（維護）",
				rc10: "待命",
				rc11: "儲存庫起始中",
				rc12: "維護（正在清除儲存庫）",
				rc99: "已停止",				
				unknown: "不明",
				// TRNOTE {0} is a mode, such as production or maintenance.
				modeChanged: "當伺服器重新啟動時，會處於 <em>{0}</em> 模式。",
				cleanStoreSelected: "已要求「<em>清除儲存庫</em>」動作。它將在伺服器重新啟動時生效。",
				mode_0: "Production",
				mode_1: "Maintenance",
				mode_2: "Clean Store",
				adminError: "${IMA_PRODUCTNAME_FULL} 偵測到錯誤。",
				adminErrorDetails: "錯誤碼為：{0}。錯誤字串為：{1}"
			},
			storeStatus: {
				mode_0: "持續",
				mode_1: "僅限記憶體"
			},
			memoryStatus: {
				ok: "確定",
				unknown: "不明",
				error: "記憶體檢查報告錯誤。",
				errorMessage: "記憶體檢查報告錯誤。請聯絡「支援中心」。"
			},
			role: {
				PRIMARY: "主要節點",
				PRIMARY_SYNC: "主要節點（正在同步化，{0}% 完成）",
				STANDBY: "待命節點",
				UNSYNC: "不同步的節點",
				TERMINATE: "主要節點終止的待命節點",
				UNSYNC_ERROR: "再也無法同步化節點",
				HADISABLED: "已停用",
				UNKNOWN: "不明",
				unknown: "不明"
			},
			haReason: {
				"1": "兩個節點的儲存庫配置不容許它們以 HA 配對運作。" +
				     "不符的配置項目為：{0}",
				"2": "探索時間已過。無法與 HA 配對中的其他節點通訊。",
				"3": "已識別兩個主要節點。",
				"4": "已識別兩個未同步化的節點。",
				"5": "無法判定哪個節點應該是主要節點，因為兩個節點均有非空白儲存庫。",
				"7": "節點無法與遠端節點配對，因為兩個節點有不同的群組 ID。",
				"9": "發生 HA 伺服器或內部錯誤",
				// TRNOTE {0} is a time stamp (date and time).
				lastPrimaryTime: "此節點在 {0} 時是主要節點。"
			},
			statusControl: {
				label: "狀態",
				ismServer: "${IMA_PRODUCTNAME_FULL} 伺服器：",
				serverNotAvailable: "請聯絡您的伺服器系統管理者，以啟動伺服器",
				serverNotAvailableNonAdmin: "請聯絡您的伺服器系統管理者，以啟動伺服器",
				haRole: "高可用性：",
				pending: "擱置中..."
			}
		},
		recordsLabel: "記錄",
		// Additions for cluster status to appear in status dropdown
		statusControl_cluster: "叢集：",
		clusterStatus_Active: "作用中",
		clusterStatus_Inactive: "非作用中",
		clusterStatus_Removed: "已移除",
		clusterStatus_Connecting: "連線中",
		clusterStatus_None: "未配置",
		clusterStatus_Unknown: "不明",
		// Additions to throughput chart on dashboard (also appears on cluster status page)
		//TRNOTE: Remote throughput refers to messages from remote servers that are members 
		// of the same cluster as the local server.
		clusterThroughputLabel: "遠端",
		// TRNOTE: Local throughput refers to messages that are received directly from
		// or that are sent directly to messaging clients that are connected to the
		// local server.
		clientThroughputLabel: "本端",
		clusterLegendIncoming: "送入",
		clusterLegendOutgoing: "送出",
		clusterHoverIncoming: "每秒從遠端叢集成員讀取訊息的 Snapshot，大約每 5 秒鐘拍攝一次。",
		clusterHoverOutgoing: "每秒寫入遠端叢集成員訊息的 Snapshot，大約每 5 秒鐘拍攝一次。"

});
