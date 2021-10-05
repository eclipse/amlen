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
		// Messaging
		// ------------------------------------------------------------------------
		messaging : {
			title : "傳訊",
			users: {
				title: "使用者鑑別",
				tagline: "配置 LDAP 來鑑別傳訊伺服器使用者。"
			},
			endpoints: {
				title: "端點配置",
				listenersSubTitle: "端點",
				endpointsSubTitle: "定義端點",
		 		form: {
					enabled: "已啟用",
					name: "名稱",
					description: "說明",
					port: "埠",
					ipAddr: "IP 位址",
					all: "All",
					protocol: "通訊協定",
					security: "安全",
					none: "無",
					securityProfile: "安全設定檔",
					connectionPolicies: "連線原則",
					connectionPolicy: "連線原則",
					messagingPolicies: "傳訊原則",
					messagingPolicy: "傳訊原則",
					destinationType: "目的地類型",
					destination: "目的地",
					maxMessageSize: "訊息大小上限",
					selectProtocol: "選取通訊協定",
					add: "新增",
					tooltip: {
						description: "",
						port: "輸入可用的埠。有效埠介於 1（含）與 65535（含）之間。",
						connectionPolicies: "至少新增一項連線原則。連線原則會授權用戶端連接至端點。" +
								"連線原則以顯示的順序進行評估。若要變更順序，請使用工具列上的上移鍵與下移鍵。",
						messagingPolicies: "至少新增一個傳訊原則。" +
								"傳訊原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題或佇列。" +
								"傳訊原則會依顯示的順序評估。若要變更順序，請使用工具列上的上移鍵與下移鍵。",									
						maxMessageSize: "訊息大小上限 (KB)。有效值介於 1（含）與 262,144（含）之間。",
						protocol: "指定此端點的有效通訊協定。"
					},
					invalid: {						
						port: "埠號必須是介於 1（含）與 65535（含）之間的數字。",
						maxMessageSize: "訊息大小上限必須是介於 1（含）與 262,144（含）之間的數字。",
						ipAddr: "需要有效的 IP 位址。",
						security: "需要輸入值。"
					},
					duplicateName: "已存在具有該名稱的記錄。"
				},
				addDialog: {
					title: "新增端點",
					instruction: "端點是用戶端應用程式所能連接的埠。端點至少須有一項連線原則及一項傳訊原則。"
				},
				removeDialog: {
					title: "刪除端點",
					content: "您確定要刪除此記錄嗎？"
				},
				editDialog: {
					title: "編輯端點",
					instruction: "端點是用戶端應用程式所能連接的埠。端點至少須有一項連線原則及一項傳訊原則。"
				},
				addProtocolsDialog: {
					title: "將通訊協定新增至端點",
					instruction: "新增容許連接至此端點的通訊協定。必須至少選取一個通訊協定。",
					allProtocolsCheckbox: "所有通訊協定均對此端點有效。",
					protocolSelectErrorTitle: "未選取任何通訊協定。",
					protocolSelectErrorMsg: "必須至少選取一個通訊協定。指定新增所有通訊協定，或從通訊協定清單中選取特定的通訊協定。"
				},
				addConnectionPoliciesDialog: {
					title: "將連線原則新增至端點",
					instruction: "連線原則會授權用戶端連接至 ${IMA_PRODUCTNAME_FULL} 端點。" +
						"每個端點至少須有一項連線原則。"
				},
				addMessagingPoliciesDialog: {
					title: "將傳訊原則新增至端點",
					instruction: "傳訊原則容許您控制用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題、佇列或廣域共用訂閱。" +
							"每個端點至少須有一個傳訊原則。" +
							"如果端點有一個用於廣域共用訂閱的傳訊原則，它還必須有一個用於已訂閱主題的傳訊原則。"
				},
                retrieveEndpointError: "擷取端點時發生錯誤。",
                saveEndpointError: "儲存端點時發生錯誤。",
                deleteEndpointError: "刪除端點時發生錯誤。",
                retrieveSecurityProfilesError: "擷取安全原則時發生錯誤。"
			},
			connectionPolicies: {
				title: "連線原則",
				grid: {
					applied: "已套用",
					name: "名稱"
				},
		 		dialog: {
					name: "名稱",
					protocol: "通訊協定",
					description: "說明",
					clientIP: "用戶端 IP 位址",  
					clientID: "用戶端 ID",
					ID: "使用者 ID",
					Group: "群組 ID",
					selectProtocol: "選取通訊協定",
					commonName: "憑證通用名稱",
					protocol: "通訊協定",
					tooltip: {
						name: "",
						filter: "您必須至少指定一個過濾器欄位。" +
								"大部分過濾器都可以在最後一個字元使用單一星號 (*) 代表 0 個或多個字元。" +
								"「用戶端 IP 位址」可以包含一個星號 (*) 或逗點區隔的 IPv4 或 IPv6 位址或者範圍清單，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								"IPv6 位址必須用方括弧括住，例如：[2001:db8::]。"
					},
					invalid: {						
						filter: "您必須至少指定一個過濾器欄位。",
						wildcard: "值的結尾可以使用單一星號 (*) 代表 0 個或多個字元。",
						vars: "不得包含替代變數 ${UserID}、${GroupID}、${ClientID} 或 ${CommonName}。",
						clientIDvars: "不得包含替代變數 ${GroupID} 或 ${ClientID}。",
						clientIP: "「用戶端 IP 位址」必須包含一個星號 (*) 或逗點區隔的 IPv4 或 IPv6 位址或者範圍清單，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								"IPv6 位址必須用方括弧括住，例如：[2001:db8::]。"
					}										
				},
				addDialog: {
					title: "新增連線原則",
					instruction: "連線原則會授權用戶端連接至 ${IMA_PRODUCTNAME_FULL} 端點。每個端點至少須有一項連線原則。"
				},
				removeDialog: {
					title: "刪除連線原則",
					instruction: "連線原則會授權用戶端連接至 ${IMA_PRODUCTNAME_FULL} 端點。每個端點至少須有一項連線原則。",
					content: "您確定要刪除此記錄嗎？"
				},
                removeNotAllowedDialog: {
                	title: "不容許移除",
                	content: "連線原則無法移除，因為有一或多個端點正在使用它。" +
                			 "從所有端點移除連線原則，然後再試一次。",
                	closeButton: "關閉"
                },								
				editDialog: {
					title: "編輯連線原則",
					instruction: "連線原則會授權用戶端連接至 ${IMA_PRODUCTNAME_FULL} 端點。每個端點至少須有一項連線原則。"
				},
                retrieveConnectionPolicyError: "擷取連線原則時發生錯誤。",
                saveConnectionPolicyError: "儲存連線原則時發生錯誤。",
                deleteConnectionPolicyError: "刪除連線原則時發生錯誤。"
 			},
			messagingPolicies: {
				title: "傳訊原則",
				listSeparator : ",",
		 		dialog: {
					name: "名稱",
					description: "說明",
					destinationType: "目的地類型",
					destinationTypeOptions: {
						topic: "主題",
						subscription: "廣域共用訂閱",
						queue: "佇列"
					},
					topic: "主題",
					queue: "佇列",
					selectProtocol: "選取通訊協定",
					destination: "目的地",
					maxMessages: "訊息數上限",
					maxMessagesBehavior: "訊息上限行為",
					maxMessagesBehaviorOptions: {
						RejectNewMessages: "拒絕新訊息",
						DiscardOldMessages: "捨棄舊訊息"
					},
					action: "權限",
					actionOptions: {
						publish: "發佈",
						subscribe: "訂閱",
						send: "傳送",
						browse: "瀏覽",
						receive: "接收",
						control: "控制"
					},
					clientIP: "用戶端 IP 位址",  
					clientID: "用戶端 ID",
					ID: "使用者 ID",
					Group: "群組 ID",
					commonName: "憑證通用名稱",
					protocol: "通訊協定",
					disconnectedClientNotification: "通知中斷連線的用戶端",
					subscriberSettings: "訂閱者設定",
					publisherSettings: "發佈者設定",
					senderSettings: "傳送者設定",
					maxMessageTimeToLive: "訊息存活時間上限",
					unlimited: "無限制",
					unit: "秒",
					// TRNOTE {0} is formatted an integer number.
					maxMessageTimeToLiveValueString: "{0} 秒",
					tooltip: {
						name: "",
						destination: "套用此原則的訊息主題、佇列或廣域共用訂閱。請小心使用星號 (*)。" +
							"星號可用於代表 0 個或多個字元，包括斜線 (/)。因此可以用於代表主題樹狀結構中的多個層次。",
						maxMessages: "為訂閱所保持的訊息數上限。必須是介於 1 與 20,000,000 之間的數字",
						maxMessagesBehavior: "在用於訂閱的緩衝區已滿時套用的行為。" +
								"亦即，當緩衝區中用於訂閱的訊息數達到「訊息數上限」值時。<br />" +
								"<strong>拒絕新訊息：&nbsp;</strong>當緩衝區已滿時，拒絕新訊息。<br />" +
								"<strong>捨棄舊訊息：&nbsp;</strong>當緩衝區已滿且新訊息到達時，捨棄最早的未分送訊息。",
						discardOldestMessages: "此設定可導致部分訊息未分送給訂閱者，即使發佈者接收到遞送確認。" +
								"即使發佈者和訂閱者已要求可靠傳訊，仍會捨棄訊息。",
						action: "此原則允許的動作",
						filter: "您必須至少指定一個過濾器欄位。" +
								"大部分過濾器都可以在最後一個字元使用單一星號 (*) 代表 0 個或多個字元。" +
								"「用戶端 IP 位址」可以包含一個星號 (*) 或逗點區隔的 IPv4 或 IPv6 位址或者範圍清單，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								"IPv6 位址必須用方括弧括住，例如：[2001:db8::]。",
				        disconnectedClientNotification: "指定在訊息到達時是否為中斷連線的 MQTT 用戶端發佈通知訊息。" +
				        		"僅為 MQTT CleanSession=0 的用戶端啟用通知。",
				        protocol: "為傳訊原則啟用通訊協定過濾。如果已啟用，則必須指定一或多個通訊協定。如果未啟用，將對傳訊原則容許所有通訊協定。",
				        destinationType: "容許存取的目的地類型。" +
				        		"若要容許存取廣域共用訂閱，需要兩項傳訊原則："+
				        		"<ul><li>目的地類型為「<em>主題</em>」的傳訊原則，容許存取一或多個主題。</li>" +
				        		"<li>目的地類型為「<em>廣域共用訂閱</em>」的傳訊原則，容許存取有關這些主題的廣域共用可延續訂閱。</li></ul>",
						action: {
							topic: "<dl><dt>發佈：</dt><dd>容許用戶端將訊息發佈到傳訊原則中所指定的主題。</dd>" +
							       "<dt>訂閱：</dt><dd>容許用戶端訂閱傳訊原則中所指定的主題。</dd></dl>",
							queue: "<dl><dt>傳送：</dt><dd>容許用戶端將訊息傳送至傳訊原則中所指定的佇列。</dd>" +
								    "<dt>瀏覽：</dt><dd>容許用戶端瀏覽傳訊原則中所指定的佇列。</dd>" +
								    "<dt>接收：</dt><dd>容許用戶端從傳訊原則中所指定的佇列接收訊息。</dd></dl>",
							subscription:  "<dl><dt>接收：</dt><dd>容許用戶端從傳訊原則中所指定的廣域共用訂閱接收訊息。</dd>" +
									"<dt>控制：</dt><dd>容許用戶端建立及刪除傳訊原則中所指定的廣域共用訂閱。</dd></dl>"
							},
						maxMessageTimeToLive: "根據原則發佈的訊息可以存活的秒數上限。" +
								"如果發佈者指定較小的有效期限值，會使用發佈者值。" +
								"有效值為「<em>無限制</em>」或 1 - 2,147,483,647 秒。「<em>無限制</em>」值設定為無上限。",
						maxMessageTimeToLiveSender: "根據原則傳送的訊息可以存在的秒數上限。" +
								"如果傳送者指定較小的有效期限值，會使用傳送者值。" +
								"有效值為「<em>無限制</em>」或 1 - 2,147,483,647 秒。「<em>無限制</em>」值設定為無上限。"
					},
					invalid: {						
						maxMessages: "必須是介於 1 與 20,000,000 之間的數字。",                       
						filter: "您必須至少指定一個過濾器欄位。",
						wildcard: "值的結尾可以使用單一星號 (*) 代表 0 個或多個字元。",
						vars: "不得包含替代變數 ${UserID}、${GroupID}、${ClientID} 或 ${CommonName}。",
						clientIP: "「用戶端 IP 位址」必須包含一個星號 (*) 或逗點區隔的 IPv4 或 IPv6 位址或者範圍清單，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								  "IPv6 位址必須用方括弧括住，例如：[2001:db8::]。",
					    subDestination: "當目的地類型為「<em>廣域共用訂閱</em>」時，不容許用戶端 ID 變數替代。",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    protocol: "通訊協定 {0} 對目的地類型 {1} 無效。",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    destinationType: "為通訊協定過濾器指定的通訊協定 {0} 對目的地類型 {1} 無效。",
					    maxMessageTimeToLive: "必須是「<em>無限制</em>」，或是介於 1 與 2,147,483,647 之間的數字。"
					}					
				},
				addDialog: {
					title: "新增傳訊原則",
					instruction: "傳訊原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題、佇列或廣域共用訂閱。" +
							     "在廣域共用的訂閱中，從可延續主題訂閱中接收訊息的工作，會在多個訂閱者之間共用。每個端點至少須有一個傳訊原則。"
				},
				removeDialog: {
					title: "刪除傳訊原則",
					instruction: "傳訊原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題、佇列或廣域共用訂閱。" +
							"在廣域共用的訂閱中，從可延續主題訂閱中接收訊息的工作，會在多個訂閱者之間共用。每個端點至少須有一個傳訊原則。",
					content: "您確定要刪除此記錄嗎？"
				},
                removeNotAllowedDialog: {
                	title: "不容許移除",
                	// TRNOTE: {0} is the messaging policy type where the value can be topic, queue or subscription.
                	content: "無法移除 {0} 原則，因為有一個以上的端點正在使用它。" +
                			 "從所有端點移除 {0} 原則，然後再試一次。",
                	closeButton: "關閉"
                },				
				editDialog: {
					title: "編輯傳訊原則",
					instruction: "傳訊原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題、佇列或廣域共用訂閱。" +
							     "在廣域共用的訂閱中，從可延續主題訂閱中接收訊息的工作，會在多個訂閱者之間共用。每個端點至少須有一個傳訊原則。"
				},
				viewDialog: {
					title: "檢視傳訊原則"
				},		
				confirmSaveDialog: {
					title: "儲存傳訊原則",
					// TRNOTE: {0} is a numeric count, {1} is a table of properties and values.
					content: "大約 {0} 個訂閱者或產生者正在使用此原則。" +
							"此原則授權的用戶端將會使用下列新設定：{1}" +
							"您確定要變更此原則嗎？",
					// TRNOTE: {1} is a table of properties and values.							
					contentUnknown: "訂閱者或產生者可能正在使用此原則。" +
							"此原則授權的用戶端將會使用下列新設定：{1}" +
							"您確定要變更此原則嗎？",
					saveButton: "儲存",
					closeButton: "取消"
				},
                retrieveMessagingPolicyError: "在擷取傳訊原則時發生錯誤。",
                retrieveOneMessagingPolicyError: "在擷取傳訊原則時發生錯誤。",
                saveMessagingPolicyError: "在儲存傳訊原則時發生錯誤。",
                deleteMessagingPolicyError: "在刪除傳訊原則時發生錯誤。",
                pendingDeletionMessage:  "此原則正在等待刪除。不再使用此原則時即會將其刪除。",
                tooltip: {
                	discardOldestMessages: "「訊息行為上限」設定為「<em>捨棄舊訊息</em>」。" +
                			"此設定可導致部分訊息未分送給訂閱者，即使發佈者接收到遞送確認。" +
                			"即使發佈者和訂閱者已要求可靠傳訊，仍會捨棄訊息。"
                }
			},
			messageProtocols: {
				title: "傳訊通訊協定",
				subtitle: "傳訊通訊協定用於在用戶端與 ${IMA_PRODUCTNAME_FULL} 之間傳送訊息。",
				tagline: "可用的傳訊通訊協定及其功能。",
				messageProtocolsList: {
					name: "通訊協定名稱",
					topic: "主題",
					shared: "廣域共用訂閱",
					queue: "佇列",
					browse: "瀏覽"
				}
			},
			messageHubs: {
				title: "訊息中心",
				subtitle: "系統管理者及傳訊管理者可以定義、編輯或刪除訊息中心。" +
						  "訊息中心是將相關的端點、連線原則及傳訊原則組織在一起的組織配置物件。<br /><br />" +
						  "端點是用戶端應用程式所能連接的埠。端點至少須有一項連線原則及一項傳訊原則。" +
						  "連線原則會授權用戶端連接至端點，而傳訊原則會在用戶端連接至端點時，為用戶端授權特定的傳訊動作。",
				tagline: "定義、編輯或刪除訊息中心。",
				defineAMessageHub: "定義訊息中心",
				editAMessageHub: "編輯訊息中心",
				defineAnEndpoint: "定義端點",
				endpointTabTagline: "端點是用戶端應用程式所能連接的埠。" +
						"端點至少須有一項連線原則及一項傳訊原則。",
				messagingPolicyTabTagline: "傳訊原則容許您控制用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題、佇列或廣域共用訂閱。" +
						"每個端點至少須有一個傳訊原則。",
				connectionPolicyTabTagline: "連線原則會授權用戶端連接至 ${IMA_PRODUCTNAME_FULL} 端點。" +
						"每個端點至少須有一項連線原則。",						
                retrieveMessageHubsError: "在擷取訊息中心時發生錯誤。",
                saveMessageHubError: "在儲存訊息中心時發生錯誤。",
                deleteMessageHubError: "在刪除訊息中心時發生錯誤。",
                messageHubNotFoundError: "找不到訊息中心。其他使用者可能已刪除它。",
                removeNotAllowedDialog: {
                	title: "不容許移除",
                	content: "訊息中心無法移除，因為它包含一或多個端點。" +
                			 "編輯訊息中心以刪除端點，然後再試一次。",
                	closeButton: "關閉"
                },
                addDialog: {
                	title: "新增訊息中心",
                	instruction: "定義訊息中心，以管理伺服器連線。",
                	saveButton: "儲存",
                	cancelButton: "取消",
                	name: "名稱：",
                	description: "說明："
                },
                editDialog: {
                	title: "編輯訊息中心內容",
                	instruction: "編輯訊息中心的名稱及說明。"
                },                
				messageHubList: {
					name: "訊息中心",
					description: "說明",
					metricLabel: "端點",
					removeDialog: {
						title: "刪除訊息中心",
						content: "您確定要刪除此訊息中心嗎？"
					}
				},
				endpointList: {
					name: "端點",
					description: "說明",
					connectionPolicies: "連線原則",
					messagingPolicies: "傳訊原則",
					port: "埠",
					enabled: "已啟用",
					status: "狀態",
					up: "向上",
					down: "向下",
					removeDialog: {
						title: "刪除端點",
						content: "您確定要刪除此端點嗎？"
					}
				},
				messagingPolicyList: {
					name: "傳訊原則",
					description: "說明",					
					metricLabel: "端點",
					destinationLabel: "目的地",
					maxMessagesLabel: "訊息數上限",
					disconnectedClientNotificationLabel: "通知中斷連線的用戶端",
					actionsLabel: "權限",
					useCountLabel: "使用計數",
					unknown: "不明",
					removeDialog: {
						title: "刪除傳訊原則",
						content: "您確定要刪除此傳訊原則嗎？"
					},
					deletePendingDialog: {
						title: "刪除擱置中",
						content: "已收到刪除要求，但此時無法刪除此原則。" +
							"大約 {0} 個訂閱者或產生者正在使用此原則。" +
							"不再使用此原則時即會將其刪除。",
						contentUnknown: "已收到刪除要求，但此時無法刪除此原則。" +
						"訂閱者或產生者可能正在使用此原則。" +
						"不再使用此原則時即會將其刪除。",
						closeButton: "關閉"
					},
					deletePendingTooltip: "不再使用此原則時即會將其刪除。"
				},	
				connectionPolicyList: {
					name: "連線原則",
					description: "說明",					
					endpoints: "端點",
					removeDialog: {
						title: "刪除連線原則",
						content: "您確定要刪除此連線原則嗎？"
					}
				},				
				messageHubDetails: {
					backLinkText: "回到訊息中心",
					editLink: "編輯",
					endpointsTitle: "端點",
					endpointsTip: "為此訊息中心配置端點及連線原則",
					messagingPoliciesTitle: "傳訊原則",
					messagingPoliciesTip: "為此訊息中心配置傳訊原則",
					connectionPoliciesTitle: "連線原則",
					connectionPoliciesTip: "為此訊息中心配置連線原則"
				},
				hovercard: {
					name: "名稱",
					description: "說明",
					endpoints: "端點",
					connectionPolicies: "連線原則",
					messagingPolicies: "傳訊原則",
					warning: "警告"
				}
			},
			referencedObjectGrid: {
				applied: "已套用",
				name: "名稱"
			},
			savingProgress: "儲存中…",
			savingFailed: "儲存失敗。",
			deletingProgress: "刪除中...",
			deletingFailed: "刪除失敗。",
			refreshingGridMessage: "更新中...",
			noItemsGridMessage: "無項目可顯示",
			testing: "測試中...",
			testTimedOut: "測試連線需要的時間太長無法完成。",
			testConnectionFailed: "測試連線失敗",
			testConnectionSuccess: "測試連線成功",
			dialog: {
				saveButton: "儲存",
				deleteButton: "刪除",
				deleteContent: "您確定要刪除此記錄嗎？",
				cancelButton: "取消",
				closeButton: "關閉",
				testButton: "測試連線",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingContent: "正在測試與 {0} 的連線...",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingFailedContent: "與 {0} 的連線測試失敗。",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingSuccessContent: "與 {0} 的連線測試成功。"
			},
			updating: "更新中...",
			invalidName: "已存在具有該名稱的記錄。",
			filterHeadingConnection: "若要將使用此原則的連線限制為特定的用戶端，請指定下列一或多個過濾器。" +
					"例如，選取「<em>群組 ID</em>」將此原則限制為特定群組的成員。" +
					"此原則僅在所有指定過濾器皆為 True 時，才會容許存取：",
			filterHeadingMessaging: "若要將在此原則中定義的傳訊動作限制為特定的用戶端，請指定下列一或多個過濾器。" +
					"例如，選取「<em>群組 ID</em>」將此原則限制為特定群組的成員。" +
					"此原則僅在所有指定過濾器皆為 True 時，才會容許存取：",
			settingsHeadingMessaging: "指定允許用戶端存取的資源及傳訊動作：",
			mqConnectivity: {
				title: "MQ Connectivity",
				subtitle: "配置與一或多個 WebSphere MQ 佇列管理程式的連線。"				
			},
			connectionProperties: {
				title: "佇列管理程式連線內容",
				tagline: "定義、編輯或刪除伺服器如何連接至佇列管理程式的相關資訊。",
				retrieveError: "在擷取佇列管理程式連線內容時發生錯誤。",
				grid: {
					name: "名稱",
					qmgr: "佇列管理程式",
					connName: "連線名稱",
					channel: "通道名稱",
					description: "說明",
					SSLCipherSpec: "SSL CipherSpec",
					status: "狀態"
				},
				dialog: {
					instruction: "與 MQ 的連線功能需要下列佇列管理程式連線詳細資料。",
					nameInvalid: "名稱不能有前導空格或尾端空格",
					connTooltip: "以 IP 位址或主機名稱及埠號的形式，輸入逗點區隔的連線名稱清單，例如 224.0.138.177(1414)",
					connInvalid: "連線名稱包含無效字元。",
					qmInvalid: "佇列管理程式名稱必須僅包含字母、數字以及四個特殊字元 . _ % 及 /",
					qmTooltip: "輸入僅包含字母、數字以及四個特殊字元 . _ % 及 / 的佇列管理程式名稱",
				    channelTooltip: "輸入僅包含字母、數字以及四個特殊字元 . _ % 及 / 的通道名稱",
					channelInvalid: "通道名稱必須僅包含字母、數字以及四個特殊字元 . _ % 及 /",
					activeRuleTitle: "不容許編輯",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailOne: "無法編輯佇列管理程式連線，因為已啟用的目的地對映規則 {0} 正在使用它。",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailMany: "無法編輯佇列管理程式連線，因為下列已啟用的目的地對映規則正在使用它：{0}。",
					SSLCipherSpecTooltip: "輸入 SSL 連線的密碼規格，長度上限為 32 個字元"
				},
				addDialog: {
					title: "新增佇列管理程式連線"
				},
				removeDialog: {
					title: "刪除佇列管理程式連線",
					content: "您確定要刪除此記錄嗎？"
				},
				editDialog: {
					title: "編輯佇列管理程式連線"
				},
                removeNotAllowedDialog: {
                	title: "不容許移除",
					// TRNOTE {0} is a user provided name of a rule.
                	contentOne: "無法移除佇列管理程式連線，因為目的地對映規則 {0} 正在使用它。",
					// TRNOTE {0} is a list of user provided names of rules.
                	contentMany: "無法移除佇列管理程式連線，因為下列目的地對映規則使用正在使用它：{0}。",
                	closeButton: "關閉"
                }
			},
			destinationMapping: {
				title: "目的地對映規則",
				tagline: "系統管理者及傳訊管理者可以定義、編輯或刪除用於控管與佇列管理程式之間所轉遞訊息的規則。",
				note: "必須先停用規則，然後才能刪除。",
				retrieveError: "在擷取目的地對映規則時發生錯誤。",
				disableRulesConfirmationDialog: {
					text: "您確定要停用此規則嗎？",
					info: "停用規則會停止規則，這樣會導致緩衝的訊息及目前在傳送的任何訊息遺失。",
					buttonLabel: "停用規則"
				},
				leadingBlankConfirmationDialog: {
					textSource: "「<em>來源</em>」有前導空白。您確定要使用前導空白儲存此規則嗎？",
					textDestination: "「<em>目的地</em>」有前導空白。您確定要使用前導空白儲存此規則嗎？",
					textBoth: "「<em>來源</em>」及「<em>目的地</em>」有前導空白。您確定要使用前導空白儲存此規則嗎？",
					info: "允許主題有前導空白，但通常沒有。請檢查主題字串以確定其正確無誤。",
					buttonLabel: "儲存規則"
				},
				grid: {
					name: "名稱",
					type: "規則類型",
					source: "來源",
					destination: "目的地",
					none: "無",
					all: "All",
					enabled: "已啟用",
					associations: "關聯數",
					maxMessages: "訊息數上限",
					retainedMessages: "保留的訊息數"
				},
				state: {
					enabled: "已啟用",
					disabled: "已停用",
					pending: "擱置中"
				},
				ruleTypes: {
					type1: "${IMA_PRODUCTNAME_SHORT} 主題至 MQ 佇列",
					type2: "${IMA_PRODUCTNAME_SHORT} 主題至 MQ 主題",
					type3: "MQ 佇列至 ${IMA_PRODUCTNAME_SHORT} 主題",
					type4: "MQ 主題至 ${IMA_PRODUCTNAME_SHORT} 主題",
					type5: "${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構至 MQ 佇列",
					type6: "${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構至 MQ 主題",
					type7: "${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構至 MQ 主題子樹狀結構",
					type8: "MQ 主題子樹狀結構至 ${IMA_PRODUCTNAME_SHORT} 主題",
					type9: "MQ 主題子樹狀結構至 ${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構",
					type10: "${IMA_PRODUCTNAME_SHORT} 佇列至 MQ 佇列",
					type11: "${IMA_PRODUCTNAME_SHORT} 佇列至 MQ 主題",
					type12: "MQ 佇列至 ${IMA_PRODUCTNAME_SHORT} 佇列",
					type13: "MQ 主題至 ${IMA_PRODUCTNAME_SHORT} 佇列",
					type14: "MQ 主題子樹狀結構至 ${IMA_PRODUCTNAME_SHORT} 佇列"
				},
				sourceTooltips: {
					type1: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 主題",
					type2: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 主題",
					type3: "輸入要從中對映的 MQ 佇列。此值可以包含 a-z、A-Z 及 0-9 範圍內的字元，以及下列任何字元：% . /  _",
					type4: "輸入要從中對映的 MQ 主題",
					type5: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構，例如 MessageGatewayRoot/Level2",
					type6: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構，例如 MessageGatewayRoot/Level2",
					type7: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構，例如 MessageGatewayRoot/Level2",
					type8: "輸入要從中對映的 MQ 主題子樹狀結構，例如 MQRoot/Layer1",
					type9: "輸入要從中對映的 MQ 主題子樹狀結構，例如 MQRoot/Layer1",
					type10: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 佇列。" +
							"值不得有前導空格或尾端空格，且不得包含控制字元、逗點、雙引號、反斜線或等號。" +
							"第一個字元不得是數字、引號或下列任何特殊字元：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type11: "輸入要從中對映的 ${IMA_PRODUCTNAME_SHORT} 佇列。" +
							"值不得有前導空格或尾端空格，且不得包含控制字元、逗點、雙引號、反斜線或等號。" +
							"第一個字元不得是數字、引號或下列任何特殊字元：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type12: "輸入要從中對映的 MQ 佇列。此值可以包含 a-z、A-Z 及 0-9 範圍內的字元，以及下列任何字元：% . /  _",
					type13: "輸入要從中對映的 MQ 主題",
					type14: "輸入要從中對映的 MQ 主題子樹狀結構，例如 MQRoot/Layer1"
				},
				targetTooltips: {
					type1: "輸入要對映的 MQ 佇列。此值可以包含 a-z、A-Z 及 0-9 範圍內的字元，以及下列任何字元：% . /  _",
					type2: "輸入要對映的 MQ 主題",
					type3: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 主題",
					type4: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 主題",
					type5: "輸入要對映的 MQ 佇列。此值可以包含 a-z、A-Z 及 0-9 範圍內的字元，以及下列任何字元：% . /  _",
					type6: "輸入要對映的 MQ 主題",
					type7: "輸入要對映的 MQ 主題子樹狀結構，例如 MQRoot/Layer1",
					type8: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 主題",
					type9: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 主題子樹狀結構，例如 MessageGatewayRoot/Level2",
					type10: "輸入要對映的 MQ 佇列。此值可以包含 a-z、A-Z 及 0-9 範圍內的字元，以及下列任何字元：% . /  _",
					type11: "輸入要對映的 MQ 主題",
					type12: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 佇列。" +
							"值不得有前導空格或尾端空格，且不得包含控制字元、逗點、雙引號、反斜線或等號。" +
							"第一個字元不得是數字、引號或下列任何特殊字元：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type13: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 佇列。" +
							"值不得有前導空格或尾端空格，且不得包含控制字元、逗點、雙引號、反斜線或等號。" +
							"第一個字元不得是數字、引號或下列任何特殊字元：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type14: "輸入要對映的 ${IMA_PRODUCTNAME_SHORT} 佇列。" +
							"值不得有前導空格或尾端空格，且不得包含控制字元、逗點、雙引號、反斜線或等號。" +
							"第一個字元不得是數字、引號或下列任何特殊字元：! # $ % &amp; ( ) * + , - . / : ; < = > ? @"
				},
				associated: "相關聯",
				associatedQMs: "關聯的佇列管理程式連線",
				associatedMessages: {
					errorMissing: "必須選取佇列管理程式連線。",
					errorRetained: "保留的訊息設定容許單一佇列管理程式連線關聯。" +
							"將保留的訊息變更為「<em>無</em>」或移除部分關聯。"
				},
				ruleTypeMessage: "保留的訊息設定容許使用主題或主題子樹狀結構目的地的規則類型。" +
							"將保留的訊息變更為「<em>無</em>」或選取不同的規則類型。",
				status: {
					active: "作用中",
					starting: "啟動中",
					inactive: "非作用中"
				},
				dialog: {
					instruction: "目的地對映規則定義訊息的移動方向，以及來源與目標物件的本質。",
					nameInvalid: "名稱不能有前導空格或尾端空格",
					noQmgrsTitle: "無佇列管理程式連線",
					noQmrsDetail: "如果不先定義佇列管理程式連線，就無法定義目的地對映規則。",
					maxMessagesTooltip: "指定可以在目的地緩衝的訊息數上限。",
					maxMessagesInvalid: "訊息數上限必須是介於 1（含）與 20,000,000（含）之間的數字。",
					retainedMessages: {
						label: "保留的訊息數",
						none: "無",
						all: "All",
						tooltip: {
							basic: "指定哪些訊息作為保留的訊息轉遞至主題。",
							disabled4Type: "在目的地為佇列時，保留的訊息必須是「<em>無</em>」。",
							disabled4Associations: "在選取多個佇列管理程式連線時，保留的訊息必須是「<em>無</em>」。",
							disabled4Both: "在目的地為佇列或選取多個佇列管理程式連線時，保留的訊息必須是「<em>無</em>」。"
						}
					}
				},
				addDialog: {
					title: "新增目的地對映規則"
				},
				removeDialog: {
					title: "刪除目的地對映規則",
					content: "您確定要刪除此記錄嗎？"
				},
                removeNotAllowedDialog: {
                	title: "不容許移除",
                	content: "無法移除目的地對映規則，因為已啟用。" +
                			 "請使用「其他動作」功能表停用此規則，然後再試一次。",
                	closeButton: "關閉"
                },
				editDialog: {
					title: "編輯目的地對映規則",
					restrictedEditingTitle: "在啟用規則時會限制編輯。",
					restrictedEditingContent: "您可以在啟用規則時編輯受限制的內容。" +
							"若要編輯其他內容，請停用規則、編輯內容，並儲存變更。" +
							"停用規則會停止規則，這樣會導致緩衝的訊息及目前在傳送的任何訊息遺失。" +
							"規則保持停用，直至您再次啟用它。"
				},
				action: {
					Enable: "啟用規則",
					Disable: "停用規則",
					Refresh: "重新整理狀態"
				}
			},
			sslkeyrepository: {
				title: "SSL 金鑰儲存庫",
				tagline: "系統管理者及傳訊管理者可以上傳及下載 SSL 金鑰儲存庫及密碼檔。" +
						 "SSL 金鑰儲存庫及密碼檔用於保護伺服器與佇列管理程式之間的連線安全。",
                uploadButton: "上傳",
                downloadLink: "下載",
                deleteButton: "刪除",
                lastUpdatedLabel: "SSL 金鑰儲存庫檔案前次更新時間：",
                noFileUpdated: "絕不",
                uploadFailed: "上傳失敗。",
                deletingFailed: "刪除失敗。",                
                dialog: {
                	uploadTitle: "上傳 SSL 金鑰儲存庫檔案",
                	deleteTitle: "刪除 SSL 金鑰儲存庫檔案",
                	deleteContent: "若 MQConnectivity 服務正在執行中，將會重新啟動。您確定要刪除 SSL 金鑰儲存庫檔案嗎？",
                	keyFileLabel: "金鑰資料庫檔：",
                	passwordFileLabel: "密碼隱藏檔：",
                	browseButton: "瀏覽...",
					browseHint: "選取檔案...",
					savingProgress: "儲存中…",
					deletingProgress: "刪除中...",
                	saveButton: "儲存",
                	deleteButton: "確定",
                	cancelButton: "取消",
                	keyRepositoryError:  "SSL 金鑰儲存庫檔案必須是 .kdb 檔。",
                	passwordStashError:  "密碼隱藏檔必須是 .sth 檔。",
                	keyRepositoryMissingError: "需要 SSL 金鑰儲存庫檔案。",
                	passwordStashMissingError: "需要密碼隱藏檔。"
                }
			},
			externldap: {
				subTitle: "LDAP 配置",
				tagline: "如果已啟用 LDAP，它將用於伺服器使用者和群組。",
				itemName: "LDAP 連線",
				grid: {
					ldapname: "LDAP 名稱",
					url: "URL",
					userid: "使用者 ID",
					password: "密碼"
				},
				enableButton: {
					enable: "啟用 LDAP",
					disable: "停用 LDAP"
				},
				dialog: {
					ldapname: "LDAP 物件名稱",
					url: "URL",
					certificate: "憑證",
					checkcert: "檢查伺服器憑證",
					checkcertTStoreOpt: "使用傳訊伺服器信任儲存庫",
					checkcertDisableOpt: "停用憑證驗證",
					checkcertPublicTOpt: "使用公用信任儲存庫",
					timeout: "逾時",
					enableCache: "啟用快取",
					cachetimeout: "快取逾時",
					groupcachetimeout: "群組快取逾時值",
					ignorecase: "不區分大小寫",
					basedn: "BaseDN",
					binddn: "BindDN",
					bindpw: "連結密碼",
					userSuffix: "使用者字尾",
					groupSuffix: "群組字尾",
					useridmap: "使用者 ID 對映",
					groupidmap: "群組 ID 對映",
					groupmemberidmap: "群組成員 ID 對映",
					nestedGroupSearch: "包括巢狀群組",
					tooltip: {
						url: "指向 LDAP 伺服器的 URL。格式為：<br/> &lt;protocol&gt;://&lt;server&nbsp;ip&gt;:&lt;port&gt;。",
						checkcert: "指定如何檢查 LDAP 伺服器憑證。",
						basedn: "基本識別名稱。",
						binddn: "在連結至 LDAP 時使用的識別名稱。若為匿名連結，請保留空白。",
						bindpw: "在連結至 LDAP 時使用的密碼。若為匿名連結，請保留空白。",
						userSuffix: "要搜尋的使用者識別名稱字尾。" +
									"如果未指定，請使用使用者 ID 對映搜尋使用者識別名稱，然後使用該識別名稱連結。",
						groupSuffix: "群組識別名稱字尾。",
						useridmap: "使用者 ID 要對映的內容。",
						groupidmap: "群組 ID 要對映的內容。",
						groupmemberidmap: "群組成員 ID 要對映的內容。",
						timeout: "LDAP 呼叫逾時值（以秒為單位）。",
						enableCache: "指定是否應該快取認證。",
						cachetimeout: "快取逾時值（以秒為單位）。",
						groupcachetimeout: "群組快取逾時值（以秒為單位）。",
						nestedGroupSearch: "如果勾選，則在搜尋使用者的群組成員資格時會包括所有巢狀群組。",
						testButtonHelp: "測試與 LDAP 伺服器的連線。您必須指定 LDAP 伺服器 URL，並有選擇性地指定憑證、BindDN 及 BindPassword 值。"
					},
					secondUnit: "秒",
					browseHint: "選取檔案...",
					browse: "瀏覽...",
					clear: "清除",
					connSubHeading: "LDAP 連線設定",
					invalidTimeout: "逾時值必須是介於 1（含）與 60（含）之間的數字。",
					invalidCacheTimeout: "逾時值必須是介於 1（含）與 60（含）之間的數字。",
					invalidGroupCacheTimeout: "逾時值必須是介於 1（含）與 86,400（含）之間的數字。",
					certificateRequired: "指定 ldaps URL 並使用信任儲存庫時需要憑證",
					urlThemeError: "輸入使用 ldap 或 ldaps 通訊協定的有效 URL。"
				},
				addDialog: {
					title: "新增 LDAP 連線",
					instruction: "配置 LDAP 連線"
				},
				editDialog: {
					title: "編輯 LDAP 連線"
				},
				removeDialog: {
					title: "刪除 LDAP 連線"
				},
				warnOnlyOneLDAPDialog: {
					title: "LDAP 連線已定義",
					content: "只能指定一個 LDAP 連線",
					closeButton: "關閉"
				},
				restartInfoDialog: {
					title: "LDAP 設定已變更",
					content: "下次對用戶端或連線進行鑑別或授權時，將會使用新的 LDAP 設定。",
					closeButton: "關閉"
				},
				enableError: "在啟用/停用外部 LDAP 配置時發生錯誤。",				
				retrieveError: "在擷取 LDAP 配置時發生錯誤。",
				saveSuccess: "儲存成功。必須重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器，才能使變更生效。"
			},
			messagequeues: {
				title: "訊息佇列",
				subtitle: "訊息佇列用於點對點傳訊。"				
			},
			queues: {
				title: "佇列",
				tagline: "定義、編輯或刪除訊息佇列。",
				retrieveError: "在擷取訊息佇列清單時發生錯誤。",
				grid: {
					name: "名稱",
					allowSend: "容許傳送",
					maxMessages: "訊息數上限",
					concurrentConsumers: "並行消費者",
					description: "說明"
				},
				dialog: {	
					instruction: "定義與您的傳訊應用程式配合使用的佇列。",
					nameInvalid: "名稱不能有前導空格或尾端空格",
					maxMessagesTooltip: "指定可以在佇列中儲存的訊息數上限。",
					maxMessagesInvalid: "訊息數上限必須是介於 1（含）與 20,000,000（含）之間的數字。",
					allowSendTooltip: "指定應用程式是否可以將訊息傳送至此佇列。此內容不會影響應用程式從此佇列接收訊息的能力。",
					concurrentConsumersTooltip: "指定佇列是否容許多個並行消費者。如果清除勾選框，將在佇列中僅容許一個消費者。",
					performanceLabel: "效能內容"
				},
				addDialog: {
					title: "新增佇列"
				},
				removeDialog: {
					title: "刪除佇列",
					content: "您確定要刪除此記錄嗎？"
				},
				editDialog: {
					title: "編輯佇列"
				}	
			},
			messagingTester: {
				title: "Messaging Tester 範例應用程式",
				tagline: "Messaging Tester 是簡式 HTML5 範例應用程式，可使用 MQTT over WebSocket 來模擬數個用戶端與 ${IMA_PRODUCTNAME_SHORT} 伺服器互動。" +
						 "當這些用戶端連線到伺服器之後，即可發佈/訂閱三個主題。",
				enableSection: {
					title: "1. 啟用端點 DemoMqttEndpoint",
					tagline: "Messaging Tester 範例應用程式必須連接至未受保護的 ${IMA_PRODUCTNAME_SHORT} 端點。您可以使用 DemoMqttEndpoint。"					
				},
				downloadSection: {
					title: "2. 下載及執行 Messaging Tester 範例應用程式",
					tagline: "按一下鏈結以下載 MessagingTester.zip。解壓縮檔案，然後在支援 WebSocket 的瀏覽器中開啟 index.html。" +
							 "遵循網頁上的指示，來驗證 ${IMA_PRODUCTNAME_SHORT} 是否準備就緒以進行 MQTT 傳訊。",
					linkLabel: "下載 MessagingTester.zip"
				},
				disableSection: {
					title: "3. 停用端點 DemoMqttEndpoint",
					tagline: "當您完成驗證 ${IMA_PRODUCTNAME_SHORT} MQTT 傳訊時，請停用端點 DemoMqttEndpoint，以避免未經授權而存取 ${IMA_PRODUCTNAME_SHORT}。"					
				},
				endpoint: {
					// TRNOTE {0} is replaced with the name of the endpoint
					label: "{0} 狀態",
					state: {
						enabled: "已啟用",					
						disabled: "已停用",
						missing: "遺漏",
						down: "向下"
					},
					linkLabel: {
						enableLinkLabel: "啟用",
						disableLinkLabel: "停用"						
					},
					missingMessage: "如果其他未受保護的端點無法使用，請建立一個。",
					retrieveEndpointError: "擷取端點配置時發生錯誤。",					
					retrieveEndpointStatusError: "擷取狀態狀態時發生錯誤。",
					saveEndpointError: "設定端點狀態時發生錯誤。"
				}
			}
		},
		protocolsLabel: "通訊協定",

		// Messaging policy types
		topicPoliciesTitle: "主題原則",
		subscriptionPoliciesTitle: "訂閱原則",
		queuePoliciesTitle: "佇列原則",
		// Messaging policy dialog strings
		addTopicMPTitle: "新增主題原則",
	    editTopicMPTitle: "編輯主題原則",
	    viewTopicMPTitle: "檢視主題原則",
		removeTopicMPTitle: "刪除主題原則",
		removeTopicMPContent: "您確定要刪除此主題原則嗎？",
		topicMPInstruction: "主題原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題。" +
					     "每個端點必須至少有一個主題原則、訂閱原則或佇列原則。",
		addSubscriptionMPTitle: "新增訂閱原則",
		editSubscriptionMPTitle: "編輯訂閱原則",
		viewSubscriptionMPTitle: "檢視訂閱原則",
		removeSubscriptionMPTitle: "刪除訂閱原則",
		removeSubscriptionMPContent: "您確定要刪除這個訂閱原則嗎？",
		subscriptionMPInstruction: "訂閱原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些廣域共用訂閱。" +
					     "在廣域共用的訂閱中，從可延續主題訂閱中接收訊息的工作，會在多個訂閱者之間共用。每個端點必須至少有一個主題原則、訂閱原則或佇列原則。",
		addQueueMPTitle: "新增佇列原則",
		editQueueMPTitle: "編輯佇列原則",
		viewQueueMPTitle: "檢視佇列原則",
		removeQueueMPTitle: "刪除佇列原則",
		removeQueueMPContent: "您確定要刪除此佇列原則嗎？",
		queueMPInstruction: "佇列原則會授權連接的用戶端執行特定的傳訊動作，例如用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些佇列。" +
					     "每個端點必須至少有一個主題原則、訂閱原則或佇列原則。",
		// Messaging and Endpoint dialog strings
		policyTypeTitle: "原則類型",
		policyTypeName_topic: "主題",
		policyTypeName_subscription: "廣域共用訂閱",
		policyTypeName_queue: "佇列",
		// Messaging policy type names that are embedded in other strings
		policyTypeShortName_topic: "主題",
		policyTypeShortName_subscription: "訂閱",
		policyTypeShortName_queue: "佇列",
		policyTypeTooltip_topic: "此原則適用的主題。請小心使用星號 (*)。" +
		    "星號可用於代表 0 個或多個字元，包括斜線 (/)。因此可以用於代表主題樹狀結構中的多個層次。",
		policyTypeTooltip_subscription: "此原則適用的廣域共用可延續訂閱。請小心使用星號 (*)。" +
			"星號符合 0 個以上的字元。",
	    policyTypeTooltip_queue: "此原則適用的佇列。請小心使用星號 (*)。" +
			"星號符合 0 個以上的字元。",
	    topicPoliciesTagline: "主題原則容許您控制用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些主題。",
		subscriptionPoliciesTagline: "訂閱原則容許您控制用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些廣域共用可延續訂閱。",
	    queuePoliciesTagline: "佇列原則容許您控制用戶端可在 ${IMA_PRODUCTNAME_FULL} 上存取哪些佇列。",
	    // Additional MQConnectivity queue manager connection dialog properties
	    channelUser: "通道使用者",
	    channelPassword: "通道使用者密碼",
	    channelUserTooltip: "如果您的佇列管理程式配置成鑑別通道使用者，您必須設定此值。",
	    channelPasswordTooltip: "如果您指定通道使用者，則也必須設定此值。",
	    // Additional LDAP dialog properties
	    emptyLdapURL: "未設定 LDAP URL",
		externalLDAPOnlyTagline: "請針對用於 ${IMA_PRODUCTNAME_SHORT} 伺服器使用者和群組的 LDAP 儲存庫配置連線內容。", 
		// TRNNOTE: Do not translate bindPasswordHint.  This is a place holder string.
		bindPasswordHint: "********",
		clearBindPasswordLabel: "清除",
		resetLdapButton: "重設",
		resetLdapTitle: "重設 LDAP 配置",
		resetLdapContent: "所有 LDAP 配置設定都會重設為預設值。",
		resettingProgress: "正在重設...",
		// New SSL Key repository dialog strings
		sslkeyrepositoryDialogUploadContent: "若 MQConnectivity 服務正在執行中，在上傳儲存庫檔案之後，服務會重新啟動。",
		savingRestartingProgress: "正在儲存並重新啟動...",
		deletingRestartingProgress: "正在刪除並重新啟動..."
});
