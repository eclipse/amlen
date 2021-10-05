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
			title : "消息传递",
			users: {
				title: "用户认证",
				tagline: "配置 LDAP 以对消息传递服务器用户进行认证。"
			},
			endpoints: {
				title: "端点配置",
				listenersSubTitle: "端点",
				endpointsSubTitle: "定义端点",
		 		form: {
					enabled: "已启用",
					name: "姓名",
					description: "描述",
					port: "端口",
					ipAddr: "IP 地址",
					all: "全部",
					protocol: "协议",
					security: "安全",
					none: "无",
					securityProfile: "安全概要文件",
					connectionPolicies: "连接策略",
					connectionPolicy: "连接策略",
					messagingPolicies: "消息传递策略",
					messagingPolicy: "消息传递策略",
					destinationType: "目标类型",
					destination: "目标",
					maxMessageSize: "最大消息大小",
					selectProtocol: "选择协议",
					add: "添加",
					tooltip: {
						description: "",
						port: "请输入可用端口。有效的端口在 1 和 65535（包括 1 和 65535）之间。",
						connectionPolicies: "请至少添加一个连接策略。连接策略授权客户机连接到端点。" +
								"连接策略按照显示的顺序进行评估。要更改顺序，请使用工具栏上的向上和向下箭头。",
						messagingPolicies: "请至少添加一个消息传递策略。" +
								"消息传递策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题或队列。" +
								"消息传递策略按照显示的顺序进行评估。要更改顺序，请使用工具栏上的向上和向下箭头。",									
						maxMessageSize: "最大消息大小，以 KB 为单位。有效值在 1 和 262,144 之间（包括二者）。",
						protocol: "为该端口指定有效协议。"
					},
					invalid: {						
						port: "端口号必须在 1 和 65535 之间（包括二者）。",
						maxMessageSize: "最大消息大小必须在 1 和 262,144 之间（包括二者）。",
						ipAddr: "需要有效 IP 地址。",
						security: "值是必需的。"
					},
					duplicateName: "已存在同名记录。"
				},
				addDialog: {
					title: "添加端点",
					instruction: "端点是客户机应用程序可以连接到的端口。端点必须至少具有一个连接策略和一个消息传递策略。"
				},
				removeDialog: {
					title: "删除端点",
					content: "确定要删除此记录吗？"
				},
				editDialog: {
					title: "编辑端点",
					instruction: "端点是客户机应用程序可以连接到的端口。端点必须至少具有一个连接策略和一个消息传递策略。"
				},
				addProtocolsDialog: {
					title: "向端点添加协议",
					instruction: "添加允许连接到该端点的协议。必须至少选择一个协议。",
					allProtocolsCheckbox: "所有协议对该端点均有效。",
					protocolSelectErrorTitle: "未选择任何协议。",
					protocolSelectErrorMsg: "必须至少选择一个协议。指定添加所有协议，或从协议列表中选择特定协议。"
				},
				addConnectionPoliciesDialog: {
					title: "向端点添加连接策略",
					instruction: "连接策略授权客户机连接到 ${IMA_PRODUCTNAME_FULL} 端点。" +
						"每个端点必须至少具有一个连接策略。"
				},
				addMessagingPoliciesDialog: {
					title: "向端点添加消息传递策略",
					instruction: "消息传递策略允许您控制客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题、队列或全局共享的预订。" +
							"每个端点必须至少具有一个消息传递策略。" +
							"如果端点具有针对全球共享的预订的消息传递策略，那么它还必须具有针对预订主题的消息传递策略。"
				},
                retrieveEndpointError: "检索端点时发生错误。",
                saveEndpointError: "保存端点时发生错误。",
                deleteEndpointError: "删除端点时发生错误。",
                retrieveSecurityProfilesError: "检索安全策略时发生错误。"
			},
			connectionPolicies: {
				title: "连接策略",
				grid: {
					applied: "已应用",
					name: "姓名"
				},
		 		dialog: {
					name: "姓名",
					protocol: "协议",
					description: "描述",
					clientIP: "客户机 IP 地址",  
					clientID: "客户机标识",
					ID: "用户标识",
					Group: "组标识",
					selectProtocol: "选择协议",
					commonName: "证书公共名称",
					protocol: "协议",
					tooltip: {
						name: "",
						filter: "必须至少指定一个过滤条件字段。" +
								"在大多数过滤条件中单个星号 (*) 可用作最后一个字符以匹配 0 个或更多个字符。" +
								"客户机 IP 地址可以包含星号 (*) 或 IPv4 或 IPv6 地址或范围的逗号分隔列表，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								"IPv6 地址必须括在方括号中，例如：[2001:db8::]。"
					},
					invalid: {						
						filter: "必须至少指定一个过滤条件字段。",
						wildcard: "单个星号 (*) 可用在值的末尾以匹配 0 个或更多个字符。",
						vars: "不能包含替换变量 ${UserID}、${GroupID}、${ClientID} 或 ${CommonName}。",
						clientIDvars: "不能包含替换变量 ${GroupID} 或 ${ClientID}。",
						clientIP: "客户机 IP 地址必须包含星号 (*) 或 IPv4 或 IPv6 地址或范围的逗号分隔列表，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								"IPv6 地址必须括在方括号中，例如：[2001:db8::]。"
					}										
				},
				addDialog: {
					title: "添加连接策略",
					instruction: "连接策略授权客户机连接到 ${IMA_PRODUCTNAME_FULL} 端点。每个端点必须至少具有一个连接策略。"
				},
				removeDialog: {
					title: "删除连接策略",
					instruction: "连接策略授权客户机连接到 ${IMA_PRODUCTNAME_FULL} 端点。每个端点必须至少具有一个连接策略。",
					content: "确定要删除此记录吗？"
				},
                removeNotAllowedDialog: {
                	title: "不允许移除",
                	content: "连接策略无法移除，因为它正由一个或多个端点使用。" +
                			 "从所有端点移除连接策略，然后重试。",
                	closeButton: "关闭"
                },								
				editDialog: {
					title: "编辑连接策略",
					instruction: "连接策略授权客户机连接到 ${IMA_PRODUCTNAME_FULL} 端点。每个端点必须至少具有一个连接策略。"
				},
                retrieveConnectionPolicyError: "检索连接策略时发生错误。",
                saveConnectionPolicyError: "保存连接策略时发生错误。",
                deleteConnectionPolicyError: "删除连接策略时发生错误。"
 			},
			messagingPolicies: {
				title: "消息传递策略",
				listSeparator : ",",
		 		dialog: {
					name: "姓名",
					description: "描述",
					destinationType: "目标类型",
					destinationTypeOptions: {
						topic: "主题",
						subscription: "全球共享的预订",
						queue: "队列"
					},
					topic: "主题",
					queue: "队列",
					selectProtocol: "选择协议",
					destination: "目标",
					maxMessages: "最大消息数",
					maxMessagesBehavior: "最大消息数行为",
					maxMessagesBehaviorOptions: {
						RejectNewMessages: "拒绝新消息",
						DiscardOldMessages: "废弃旧消息"
					},
					action: "权限",
					actionOptions: {
						publish: "发布",
						subscribe: "预订",
						send: "发送",
						browse: "浏览",
						receive: "接收",
						control: "控制"
					},
					clientIP: "客户机 IP 地址",  
					clientID: "客户机标识",
					ID: "用户标识",
					Group: "组标识",
					commonName: "证书公共名称",
					protocol: "协议",
					disconnectedClientNotification: "断开连接的客户机通知",
					subscriberSettings: "订户设置",
					publisherSettings: "发布者设置",
					senderSettings: "发件人设置",
					maxMessageTimeToLive: "最大消息生存时间",
					unlimited: "无限制",
					unit: "秒",
					// TRNOTE {0} is formatted an integer number.
					maxMessageTimeToLiveValueString: "{0} 秒",
					tooltip: {
						name: "",
						destination: "该策略应用到的消息主题、队列或全球共享的预订。使用星号 (*) 时请务必小心。" +
							"星号匹配 0 个或更多个字符，包括斜杠 (/)。因此，可匹配主题树中的多个级别。",
						maxMessages: "为预订保留的最大消息数。该值必须是 1 和 20,000,000 之间的数字",
						maxMessagesBehavior: "预订的缓冲区已满时应用的行为。" +
								"即，当预订的缓冲区中的消息数达到最大消息数值时。<br />" +
								"<strong>拒绝新消息：</strong>当缓冲区已满时，会拒绝新消息。<br />" +
								"<strong>废弃旧消息：</strong>当缓冲区已满且新消息到达时，将废弃最旧的未传递消息。",
						discardOldestMessages: "即使发布者接收传递确认，此设置也会导致某些消息未传递到订户。" +
								"即使发布者和订户都请求了可靠的消息传递，消息也会废弃。",
						action: "该策略许可的操作",
						filter: "必须至少指定一个过滤条件字段。" +
								"在大多数过滤条件中单个星号 (*) 可用作最后一个字符以匹配 0 个或更多个字符。" +
								"客户机 IP 地址可以包含星号 (*) 或 IPv4 或 IPv6 地址或范围的逗号分隔列表，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								"IPv6 地址必须括在方括号中，例如：[2001:db8::]。",
				        disconnectedClientNotification: "指定当消息到达时是否为断开连接的 MQTT 客户机发布通知消息。" +
				        		"仅针对 MQTT CleanSession=0 客户机启用通知。",
				        protocol: "针对消息传递策略启用协议过滤。如果已启用，那么必须指定一个或多个协议。如果未启用，那么全部协议都可适用于消息传递策略。",
				        destinationType: "允许访问的目标类型。" +
				        		"要允许访问全球共享的预订，需要两个消息传递策略："+
				        		"<ul><li>目标类型为<em>主题</em>的消息传递策略，允许访问一个或多个主题。</li>" +
				        		"<li>目标类型为<em>全球共享的预订</em>的消息传递策略，允许访问这些主题上的全球共享的持久预订。</li></ul>",
						action: {
							topic: "<dl><dt>发布：</dt><dd>允许客户机将消息发布至消息传递策略中指定的主题。</dd>" +
							       "<dt>预订：</dt><dd>允许客户机预订消息传递策略中指定的主题。</dd></dl>",
							queue: "<dl><dt>发送：</dt><dd>允许客户机将消息发送到消息传递策略中指定的队列。</dd>" +
								    "<dt>浏览：</dt><dd>允许客户机浏览消息传递策略中指定的队列。</dd>" +
								    "<dt>接收：</dt><dd>允许客户机从消息传递策略中指定的队列接收消息。</dd></dl>",
							subscription:  "<dl><dt>接收：</dt><dd>允许客户机从消息传递策略中指定的全球共享的预订接收消息。</dd>" +
									"<dt>控制：</dt><dd>允许客户机创建并删除消息传递策略中指定的全球共享的预订。</dd></dl>"
							},
						maxMessageTimeToLive: "根据策略发布的消息可生存的最大秒数。" +
								"如果发布者指定了较小的到期值，那么使用发布者值。" +
								"有效值为<em>无限制</em>或 1 - 2,147,483,647 秒。值<em>无限制</em>不设置最大值。",
						maxMessageTimeToLiveSender: "根据策略发送的消息可生存的最大秒数。" +
								"如果发件人指定了较小的到期值，那么使用发件人值。" +
								"有效值为<em>无限制</em>或 1 - 2,147,483,647 秒。值<em>无限制</em>不设置最大值。"
					},
					invalid: {						
						maxMessages: "该值必须是 1 和 20,000,000 之间的数字。",                       
						filter: "必须至少指定一个过滤条件字段。",
						wildcard: "单个星号 (*) 可用在值的末尾以匹配 0 个或更多个字符。",
						vars: "不能包含替换变量 ${UserID}、${GroupID}、${ClientID} 或 ${CommonName}。",
						clientIP: "客户机 IP 地址必须包含星号 (*) 或 IPv4 或 IPv6 地址或范围的逗号分隔列表，例如：9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100。" +
								  "IPv6 地址必须括在方括号中，例如：[2001:db8::]。",
					    subDestination: "当目标类型为<em>全球共享的预订</em>时，不允许客户机标识变量替换。",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    protocol: "协议 {0} 对目标类型 {1} 无效。",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    destinationType: "为协议过滤条件指定的协议 {0} 对目标类型 {1} 无效。",
					    maxMessageTimeToLive: "该值必须为<em>无限制</em>或 1 和 2,147,483,647 之间的数字。"
					}					
				},
				addDialog: {
					title: "添加消息传递策略",
					instruction: "消息传递策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题、队列或全局共享的预订。" +
							     "在全球共享的预订中，可在多个订户间共享从持久主题预订接收消息的工作。每个端点必须至少具有一个消息传递策略。"
				},
				removeDialog: {
					title: "删除消息传递策略",
					instruction: "消息传递策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题、队列或全局共享的预订。" +
							"在全球共享的预订中，可在多个订户间共享从持久主题预订接收消息的工作。每个端点必须至少具有一个消息传递策略。",
					content: "确定要删除此记录吗？"
				},
                removeNotAllowedDialog: {
                	title: "不允许移除",
                	// TRNOTE: {0} is the messaging policy type where the value can be topic, queue or subscription.
                	content: "“{0}”策略无法移除，因为它正由一个或多个端点使用。" +
                			 "从所有端点移除“{0}”策略，然后重试。",
                	closeButton: "关闭"
                },				
				editDialog: {
					title: "编辑消息传递策略",
					instruction: "消息传递策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题、队列或全局共享的预订。" +
							     "在全球共享的预订中，可在多个订户间共享从持久主题预订接收消息的工作。每个端点必须至少具有一个消息传递策略。"
				},
				viewDialog: {
					title: "查看消息传递策略"
				},		
				confirmSaveDialog: {
					title: "保存消息传递策略",
					// TRNOTE: {0} is a numeric count, {1} is a table of properties and values.
					content: "大约有 {0} 个订户或生产者正在使用该策略。" +
							"该策略授权的客户机将使用以下新设置：{1}" +
							"确定要更改该策略吗？",
					// TRNOTE: {1} is a table of properties and values.							
					contentUnknown: "订户或生产者可能正在使用该策略。" +
							"该策略授权的客户机将使用以下新设置：{1}" +
							"确定要更改该策略吗？",
					saveButton: "保存",
					closeButton: "取消"
				},
                retrieveMessagingPolicyError: "检索消息传递策略时发生错误。",
                retrieveOneMessagingPolicyError: "检索消息传递策略时发生错误。",
                saveMessagingPolicyError: "保存消息传递策略时发生错误。",
                deleteMessagingPolicyError: "删除消息传递策略时发生错误。",
                pendingDeletionMessage:  "该策略处于暂挂删除状态。不再使用该策略时，会将其删除。",
                tooltip: {
                	discardOldestMessages: "最大消息数行为设置为<em>废弃旧消息</em>。" +
                			"即使发布者接收传递确认，此设置也会导致某些消息未传递到订户。" +
                			"即使发布者和订户都请求了可靠的消息传递，消息也会废弃。"
                }
			},
			messageProtocols: {
				title: "消息传递协议",
				subtitle: "消息传递协议用于在客户机与 ${IMA_PRODUCTNAME_FULL} 之间发送消息。",
				tagline: "可用的消息传递协议及其功能。",
				messageProtocolsList: {
					name: "协议名称",
					topic: "主题",
					shared: "全球共享的预订",
					queue: "队列",
					browse: "浏览"
				}
			},
			messageHubs: {
				title: "消息中心",
				subtitle: "系统管理员和消息传递管理员可以定义、编辑或删除消息中心。" +
						  "消息中心是用于分组相关端点、连接策略并对策略进行消息传递的组织配置对象。<br /><br />" +
						  "端点是客户机应用程序可以连接到的端口。端点必须至少具有一个连接策略和一个消息传递策略。" +
						  "连接策略授权客户机连接到端点，而消息传递策略授权客户机在连接到端点后可即刻执行特定的消息传递操作。",
				tagline: "定义、编辑或删除消息中心。",
				defineAMessageHub: "定义消息中心",
				editAMessageHub: "编辑消息中心",
				defineAnEndpoint: "定义端点",
				endpointTabTagline: "端点是客户机应用程序可以连接到的端口。" +
						"端点必须至少具有一个连接策略和一个消息传递策略。",
				messagingPolicyTabTagline: "消息传递策略允许您控制客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题、队列或全局共享的预订。" +
						"每个端点必须至少具有一个消息传递策略。",
				connectionPolicyTabTagline: "连接策略授权客户机连接到 ${IMA_PRODUCTNAME_FULL} 端点。" +
						"每个端点必须至少具有一个连接策略。",						
                retrieveMessageHubsError: "检索消息中心时发生错误。",
                saveMessageHubError: "保存消息中心时发生错误。",
                deleteMessageHubError: "删除消息中心时发生错误。",
                messageHubNotFoundError: "找不到消息中心。可能已被另一个用户删除。",
                removeNotAllowedDialog: {
                	title: "不允许移除",
                	content: "消息中心无法移除，因为它包含一个或多个端点。" +
                			 "编辑消息中心以删除端点，然后重试。",
                	closeButton: "关闭"
                },
                addDialog: {
                	title: "添加消息中心",
                	instruction: "定义消息中心以管理服务器连接。",
                	saveButton: "保存",
                	cancelButton: "取消",
                	name: "名称：",
                	description: "描述："
                },
                editDialog: {
                	title: "编辑消息中心属性",
                	instruction: "编辑消息中心的名称和描述。"
                },                
				messageHubList: {
					name: "消息中心",
					description: "描述",
					metricLabel: "端点",
					removeDialog: {
						title: "删除消息中心",
						content: "确定要删除此消息中心吗？"
					}
				},
				endpointList: {
					name: "端点",
					description: "描述",
					connectionPolicies: "连接策略",
					messagingPolicies: "消息传递策略",
					port: "端口",
					enabled: "已启用",
					status: "状态",
					up: "启动",
					down: "关闭",
					removeDialog: {
						title: "删除端点",
						content: "确定要删除此端点吗？"
					}
				},
				messagingPolicyList: {
					name: "消息传递策略",
					description: "描述",					
					metricLabel: "端点",
					destinationLabel: "目标",
					maxMessagesLabel: "最大消息数",
					disconnectedClientNotificationLabel: "断开连接的客户机通知",
					actionsLabel: "权限",
					useCountLabel: "使用计数",
					unknown: "未知",
					removeDialog: {
						title: "删除消息传递策略",
						content: "确定要删除此消息传递策略吗？"
					},
					deletePendingDialog: {
						title: "删除暂挂",
						content: "已收到删除请求，但是此时无法删除该策略。" +
							"大约有 {0} 个订户或生产者正在使用该策略。" +
							"不再使用该策略时，会将其删除。",
						contentUnknown: "已收到删除请求，但是此时无法删除该策略。" +
						"订户或生产者可能正在使用该策略。" +
						"不再使用该策略时，会将其删除。",
						closeButton: "关闭"
					},
					deletePendingTooltip: "不再使用该策略时，会将其删除。"
				},	
				connectionPolicyList: {
					name: "连接策略",
					description: "描述",					
					endpoints: "端点",
					removeDialog: {
						title: "删除连接策略",
						content: "确定要删除此连接策略吗？"
					}
				},				
				messageHubDetails: {
					backLinkText: "返回到消息中心",
					editLink: "编辑",
					endpointsTitle: "端点",
					endpointsTip: "为此消息中心配置端点和连接策略",
					messagingPoliciesTitle: "消息传递策略",
					messagingPoliciesTip: "为此消息中心配置消息传递策略",
					connectionPoliciesTitle: "连接策略",
					connectionPoliciesTip: "为此消息中心配置连接策略"
				},
				hovercard: {
					name: "姓名",
					description: "描述",
					endpoints: "端点",
					connectionPolicies: "连接策略",
					messagingPolicies: "消息传递策略",
					warning: "警告"
				}
			},
			referencedObjectGrid: {
				applied: "已应用",
				name: "姓名"
			},
			savingProgress: "正在保存...",
			savingFailed: "保存失败。",
			deletingProgress: "正在删除...",
			deletingFailed: "删除失败。",
			refreshingGridMessage: "正在更新...",
			noItemsGridMessage: "没有要显示的项目",
			testing: "正在测试...",
			testTimedOut: "完成测试连接耗时过长。",
			testConnectionFailed: "测试连接失败",
			testConnectionSuccess: "测试连接成功",
			dialog: {
				saveButton: "保存",
				deleteButton: "删除",
				deleteContent: "确定要删除该记录吗？",
				cancelButton: "取消",
				closeButton: "关闭",
				testButton: "测试连接",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingContent: "正在测试到 {0} 的连接...",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingFailedContent: "测试到 {0} 的连接失败。",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingSuccessContent: "测试到 {0} 的连接成功。"
			},
			updating: "正在更新...",
			invalidName: "已存在同名记录。",
			filterHeadingConnection: "要将使用该策略的连接限制为特定的客户机，请指定一个或多个以下过滤条件。" +
					"例如，选择<em>组标识</em>以将该策略限制为特定组的成员。" +
					"该策略只允许在所有指定的过滤条件均为 true 时访问。",
			filterHeadingMessaging: "要将该策略中定义的消息传递操作限制为特定的客户机，请指定一个或多个以下过滤条件。" +
					"例如，选择<em>组标识</em>以将该策略限制为特定组的成员。" +
					"该策略只允许在所有指定的过滤条件均为 true 时访问。",
			settingsHeadingMessaging: "指定准许客户机访问的资源和消息传递操作：",
			mqConnectivity: {
				title: "MQ Connectivity",
				subtitle: "配置与一个或多个 WebSphere MQ 队列管理器的连接。"				
			},
			connectionProperties: {
				title: "队列管理器连接属性",
				tagline: "定义、编辑或删除有关服务器如何连接到队列管理器的信息。",
				retrieveError: "检索队列管理器连接属性时发生错误。",
				grid: {
					name: "姓名",
					qmgr: "队列管理器",
					connName: "连接名称",
					channel: "渠道名称",
					description: "描述",
					SSLCipherSpec: "SSL 密码规范",
					status: "状态"
				},
				dialog: {
					instruction: "与 MQ 连接需要以下队列管理器连接详细信息。",
					nameInvalid: "名称不能具有前导或尾部空格。",
					connTooltip: "采用 IP 地址或主机名和端口号的形式输入逗号分隔的连接名称列表，如 224.0.138.177(1414)",
					connInvalid: "连接名称包含无效字符。",
					qmInvalid: "队列管理器名称只能由字母、数字和四个特殊字符 ._ % 和 / 组成",
					qmTooltip: "输入队列管理器名称，该名称只能由字母、数字和四个特殊字符 . _ % 和 / 组成",
				    channelTooltip: "输入渠道名称，该名称只能由字母、数字和四个特殊字符 . _ % 和 / 组成",
					channelInvalid: "渠道名称只能由字母、数字和四个特殊字符 . _ % 和 / 组成",
					activeRuleTitle: "不允许编辑",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailOne: "此队列管理器连接正由已启用的目标映射规则 {0} 使用，因此不能编辑。",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailMany: "此队列管理器连接正由以下已启用的目标映射规则使用，因此不能编辑：{0}。",
					SSLCipherSpecTooltip: "输入 SSL 连接的密码规范，其最大长度为 32 个字符。"
				},
				addDialog: {
					title: "添加队列管理器连接"
				},
				removeDialog: {
					title: "删除队列管理器连接",
					content: "确定要删除此记录吗？"
				},
				editDialog: {
					title: "编辑队列管理器连接"
				},
                removeNotAllowedDialog: {
                	title: "不允许移除",
					// TRNOTE {0} is a user provided name of a rule.
                	contentOne: "此队列管理器连接正由目标映射规则 {0} 使用，因此不能移除。",
					// TRNOTE {0} is a list of user provided names of rules.
                	contentMany: "此队列管理器连接正由以下目标映射规则使用，因此不能移除：{0}。",
                	closeButton: "关闭"
                }
			},
			destinationMapping: {
				title: "目标映射规则",
				tagline: "系统管理员和消息传递管理员可以定义、编辑或删除规则，这些规则控制将哪些消息转发到队列管理器以及从队列管理器转发哪些消息。",
				note: "必须先禁用规则然后才能删除。",
				retrieveError: "检索目标映射规则时发生错误。",
				disableRulesConfirmationDialog: {
					text: "确定要禁用该规则吗？",
					info: "禁用规则会停止规则，这会导致丢失已缓冲的消息以及当前正发送的任何消息。",
					buttonLabel: "禁用规则"
				},
				leadingBlankConfirmationDialog: {
					textSource: "<em>源</em>具有前导空白。确定要保存这个具有前导空白的规则吗？",
					textDestination: "<em>目标</em>具有前导空白。确定要保存这个具有前导空白的规则吗？",
					textBoth: "<em>源</em>和<em>目标</em>具有前导空白。确定要保存这个具有前导空白的规则吗？",
					info: "主题可以具有前导空白，但一般没有前导空白。检查主题字符串以确保其正确性。",
					buttonLabel: "保存规则"
				},
				grid: {
					name: "姓名",
					type: "规则类型",
					source: "来源",
					destination: "目标",
					none: "无",
					all: "全部",
					enabled: "已启用",
					associations: "关联",
					maxMessages: "最大消息数",
					retainedMessages: "保留的消息"
				},
				state: {
					enabled: "已启用",
					disabled: "已禁用",
					pending: "暂挂中"
				},
				ruleTypes: {
					type1: "${IMA_PRODUCTNAME_SHORT} 主题到 MQ 队列",
					type2: "${IMA_PRODUCTNAME_SHORT} 主题到 MQ 主题",
					type3: "MQ 队列到 ${IMA_PRODUCTNAME_SHORT} 主题",
					type4: "MQ 主题到 ${IMA_PRODUCTNAME_SHORT} 主题",
					type5: "${IMA_PRODUCTNAME_SHORT} 主题子树到 MQ 队列",
					type6: "${IMA_PRODUCTNAME_SHORT} 主题子树到 MQ 主题",
					type7: "${IMA_PRODUCTNAME_SHORT} 主题子树到 MQ 主题子树",
					type8: "MQ 主题子树到 ${IMA_PRODUCTNAME_SHORT} 主题",
					type9: "MQ 主题子树到 ${IMA_PRODUCTNAME_SHORT} 主题子树",
					type10: "${IMA_PRODUCTNAME_SHORT} 队列到 MQ 队列",
					type11: "${IMA_PRODUCTNAME_SHORT} 队列到 MQ 主题",
					type12: "MQ 队列到 ${IMA_PRODUCTNAME_SHORT} 队列",
					type13: "MQ 主题到 ${IMA_PRODUCTNAME_SHORT} 队列",
					type14: "MQ 主题子树到 ${IMA_PRODUCTNAME_SHORT} 队列"
				},
				sourceTooltips: {
					type1: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 主题",
					type2: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 主题",
					type3: "输入要从其映射的 MQ 队列。值可以包含 a-z、A-Z 和 0-9 范围内的字符以及以下任意字符：% . /  _",
					type4: "输入要从其映射的 MQ 主题",
					type5: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 主题子树，例如 MessageGatewayRoot/Level2",
					type6: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 主题子树，例如 MessageGatewayRoot/Level2",
					type7: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 主题子树，例如 MessageGatewayRoot/Level2",
					type8: "输入要从其映射的 MQ 主题子树，例如 MQRoot/Layer1",
					type9: "输入要从其映射的 MQ 主题子树，例如 MQRoot/Layer1",
					type10: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 队列。" +
							"该值不得含有前导空格或尾部空格，并且不得包含控制字符、逗号、双引号、反斜杠或等号。" +
							"第一个字符不能为数字、引号或以下任何特殊字符：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type11: "输入要从其映射的 ${IMA_PRODUCTNAME_SHORT} 队列。" +
							"该值不得含有前导空格或尾部空格，并且不得包含控制字符、逗号、双引号、反斜杠或等号。" +
							"第一个字符不能为数字、引号或以下任何特殊字符：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type12: "输入要从其映射的 MQ 队列。值可以包含 a-z、A-Z 和 0-9 范围内的字符以及以下任意字符：% . /  _",
					type13: "输入要从其映射的 MQ 主题",
					type14: "输入要从其映射的 MQ 主题子树，例如 MQRoot/Layer1"
				},
				targetTooltips: {
					type1: "输入要映射到的 MQ 队列。值可以包含 a-z、A-Z 和 0-9 范围内的字符以及以下任意字符：% . /  _",
					type2: "输入要映射到的 MQ 主题",
					type3: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 主题",
					type4: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 主题",
					type5: "输入要映射到的 MQ 队列。值可以包含 a-z、A-Z 和 0-9 范围内的字符以及以下任意字符：% . /  _",
					type6: "输入要映射到的 MQ 主题",
					type7: "输入要映射到的 MQ 主题子树，例如 MQRoot/Layer1",
					type8: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 主题",
					type9: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 主题子树，例如 MessageGatewayRoot/Level2",
					type10: "输入要映射到的 MQ 队列。值可以包含 a-z、A-Z 和 0-9 范围内的字符以及以下任意字符：% . /  _",
					type11: "输入要映射到的 MQ 主题",
					type12: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 队列。" +
							"该值不得含有前导空格或尾部空格，并且不得包含控制字符、逗号、双引号、反斜杠或等号。" +
							"第一个字符不能为数字、引号或以下任何特殊字符：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type13: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 队列。" +
							"该值不得含有前导空格或尾部空格，并且不得包含控制字符、逗号、双引号、反斜杠或等号。" +
							"第一个字符不能为数字、引号或以下任何特殊字符：! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type14: "输入要映射到的 ${IMA_PRODUCTNAME_SHORT} 队列。" +
							"该值不得含有前导空格或尾部空格，并且不得包含控制字符、逗号、双引号、反斜杠或等号。" +
							"第一个字符不能为数字、引号或以下任何特殊字符：! # $ % &amp; ( ) * + , - . / : ; < = > ? @"
				},
				associated: "已关联",
				associatedQMs: "关联的队列管理器连接：",
				associatedMessages: {
					errorMissing: "必须选择一个队列管理器连接，",
					errorRetained: "保留的消息设置允许单个队列管理器连接关联。" +
							"将保留的消息更改为<em>无</em>或移除某些关联。"
				},
				ruleTypeMessage: "保留的消息设置允许具有主题或主题子树目标的规则类型。" +
							"将保留的消息更改为<em>无</em>或选择其他规则类型。",
				status: {
					active: "活动",
					starting: "正在启动",
					inactive: "不活动"
				},
				dialog: {
					instruction: "目标映射规则定义消息的移动方向，以及源和目标对象的性质。",
					nameInvalid: "名称不能具有前导或尾部空格。",
					noQmgrsTitle: "无队列管理器连接",
					noQmrsDetail: "必须先定义队列管理器连接，然后才能定义目标映射规则。",
					maxMessagesTooltip: "指定目标上可缓冲的最大消息数。",
					maxMessagesInvalid: "最大消息数必须是 1 和 20,000,000 之间的数字（包括二者）。",
					retainedMessages: {
						label: "保留的消息",
						none: "无",
						all: "全部",
						tooltip: {
							basic: "指定将哪些消息作为保留的消息转发至主题。",
							disabled4Type: "当目标是队列时，保留的消息必须为<em>无</em>。",
							disabled4Associations: "当选择了多个队列管理器连接时，保留的消息必须为<em>无</em>。",
							disabled4Both: "当目标是队列或选择了多个队列管理器连接时，保留的消息必须为<em>无</em>。"
						}
					}
				},
				addDialog: {
					title: "添加目标映射规则"
				},
				removeDialog: {
					title: "删除目标映射规则",
					content: "确定要删除此记录吗？"
				},
                removeNotAllowedDialog: {
                	title: "不允许移除",
                	content: "目标映射规则已启用，因此不能移除。" +
                			 "使用“其他操作”菜单禁用该规则，然后重试。",
                	closeButton: "关闭"
                },
				editDialog: {
					title: "编辑目标映射规则",
					restrictedEditingTitle: "规则已启用时编辑受限。",
					restrictedEditingContent: "在规则已启用时可以编辑受限属性。" +
							"要编辑其他属性，请禁用该规则，编辑属性，并保存更改。" +
							"禁用规则会停止规则，这会导致丢失已缓冲的消息以及当前正发送的任何消息。" +
							"规则保持禁用状态，直至您重新启用。"
				},
				action: {
					Enable: "启用规则",
					Disable: "禁用规则",
					Refresh: "刷新状态"
				}
			},
			sslkeyrepository: {
				title: "SSL 密钥存储库",
				tagline: "系统管理员和消息传递管理员可以上载和下载 SSL 密钥存储库和密码文件。" +
						 "SSL 密钥存储库和密码文件用于保护服务器与队列管理器之间的连接。",
                uploadButton: "上载",
                downloadLink: "下载",
                deleteButton: "删除",
                lastUpdatedLabel: "SSL 密钥存储库文件上次更新时间：",
                noFileUpdated: "从不",
                uploadFailed: "上载失败。",
                deletingFailed: "删除失败。",                
                dialog: {
                	uploadTitle: "上载 SSL 密钥存储库文件",
                	deleteTitle: "删除 SSL 密钥存储库文件",
                	deleteContent: "如果 MQConnectivity 服务正在运行，将重新启动此服务。确定要删除 SSL 密钥存储库文件？",
                	keyFileLabel: "密钥数据库文件：",
                	passwordFileLabel: "密码存储文件：",
                	browseButton: "浏览...",
					browseHint: "选择文件...",
					savingProgress: "正在保存...",
					deletingProgress: "正在删除...",
                	saveButton: "保存",
                	deleteButton: "确定",
                	cancelButton: "取消",
                	keyRepositoryError:  "SSL 密钥存储库文件必须是 .kdb 文件。",
                	passwordStashError:  "密码存储文件必须是 .sth 文件。",
                	keyRepositoryMissingError: "需要 SSL 密钥存储库文件。",
                	passwordStashMissingError: "需要密码存储文件。"
                }
			},
			externldap: {
				subTitle: "LDAP 配置",
				tagline: "如果启用 LDAP，它将用于服务器用户和组。",
				itemName: "LDAP 连接",
				grid: {
					ldapname: "LDAP 名称",
					url: "URL",
					userid: "用户标识",
					password: "密码"
				},
				enableButton: {
					enable: "启用 LDAP",
					disable: "禁用 LDAP"
				},
				dialog: {
					ldapname: "LDAP 对象名称",
					url: "URL",
					certificate: "证书",
					checkcert: "检查服务器证书",
					checkcertTStoreOpt: "使用消息传递服务器信任库",
					checkcertDisableOpt: "禁用证书验证",
					checkcertPublicTOpt: "使用公用信任库",
					timeout: "超时",
					enableCache: "启用高速缓存",
					cachetimeout: "高速缓存超时",
					groupcachetimeout: "组高速缓存超时",
					ignorecase: "忽略大小写",
					basedn: "基本 DN",
					binddn: "绑定 DN",
					bindpw: "绑定密码",
					userSuffix: "用户后缀",
					groupSuffix: "组后缀",
					useridmap: "用户标识映射",
					groupidmap: "组标识映射",
					groupmemberidmap: "组成员标识映射",
					nestedGroupSearch: "包括嵌套的组",
					tooltip: {
						url: "指向 LDAP 服务器的 URL。格式为：<br/> &lt;protocol&gt;://&lt;server&nbsp;ip&gt;:&lt;port&gt;。",
						checkcert: "指定如何检查 LDAP 服务器证书。",
						basedn: "基本专有名称。",
						binddn: "绑定到 LDAP 时使用的专有名称。对于匿名绑定，保留为空。",
						bindpw: "绑定到 LDAP 时使用的密码。对于匿名绑定，保留为空。",
						userSuffix: "要搜索的用户专有名称后缀。" +
									"如果未指定，使用用户标识映射搜索用户专有名称，然后与该专有名称绑定。",
						groupSuffix: "组专有名称后缀。",
						useridmap: "要将用户标识映射到的属性。",
						groupidmap: "要将组标识映射到的属性。",
						groupmemberidmap: "要将组成员标识映射到的属性。",
						timeout: "LDAP 调用的超时，以秒计。",
						enableCache: "指定凭证是否应高速缓存。",
						cachetimeout: "高速缓存超时，以秒计。",
						groupcachetimeout: "组高速缓存超时，以秒计。",
						nestedGroupSearch: "如果选中，将在用户的组成员资格搜索中包括所有嵌套组。",
						testButtonHelp: "测试与 LDAP 服务器的连接。您必须指定 LDAP 服务器 URL，并可以选择指定证书、绑定 DN 和绑定密码值。"
					},
					secondUnit: "秒",
					browseHint: "选择文件...",
					browse: "浏览...",
					clear: "清除",
					connSubHeading: "LDAP 连接设置",
					invalidTimeout: "超时必须是 1 和 60 之间的数字（包括二者）。",
					invalidCacheTimeout: "超时必须是 1 和 60 之间的数字（包括二者）。",
					invalidGroupCacheTimeout: "超时必须是 1 和 86,400 之间的数字（包括二者）。",
					certificateRequired: "如果已指定 ldaps url 并已使用信任库，那么需要证书",
					urlThemeError: "输入使用 ldap 或 ldaps 协议的有效 URL。"
				},
				addDialog: {
					title: "添加 LDAP 连接",
					instruction: "配置 LDAP 连接。"
				},
				editDialog: {
					title: "编辑 LDAP 连接"
				},
				removeDialog: {
					title: "删除 LDAP 连接"
				},
				warnOnlyOneLDAPDialog: {
					title: "LDAP 连接已定义",
					content: "只能指定一个 LDAP 连接",
					closeButton: "关闭"
				},
				restartInfoDialog: {
					title: "LDAP 设置已更改",
					content: "下次认证或授权客户机或连接时将使用新的 LDAP 设置。",
					closeButton: "关闭"
				},
				enableError: "启用/禁用外部 LDAP 配置时发生错误。",				
				retrieveError: "检索 LDAP 配置时发生错误。",
				saveSuccess: "保存成功。必须重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器才能使更改生效。"
			},
			messagequeues: {
				title: "消息队列",
				subtitle: "消息队列用于点到点消息传递中。"				
			},
			queues: {
				title: "队列",
				tagline: "定义、编辑或删除消息队列。",
				retrieveError: "检索消息队列列表时发生错误。",
				grid: {
					name: "姓名",
					allowSend: "允许发送",
					maxMessages: "最大消息数",
					concurrentConsumers: "并发使用者",
					description: "描述"
				},
				dialog: {	
					instruction: "定义用于消息传递应用程序的队列。",
					nameInvalid: "名称不能具有前导或尾部空格。",
					maxMessagesTooltip: "指定队列上可存储的最大消息数。",
					maxMessagesInvalid: "最大消息数必须是 1 和 20,000,000 之间的数字（包括二者）。",
					allowSendTooltip: "指定应用程序是否可以向该队列发送消息。该属性不会影响应用程序从该队列接收消息的能力。",
					concurrentConsumersTooltip: "指定队列是否允许多个并发使用者。如果清除该复选框，那么该队列上只允许一个使用者。",
					performanceLabel: "性能属性"
				},
				addDialog: {
					title: "添加队列"
				},
				removeDialog: {
					title: "删除队列",
					content: "确定要删除此记录吗？"
				},
				editDialog: {
					title: "编辑队列"
				}	
			},
			messagingTester: {
				title: "Messaging Tester 样本应用程序",
				tagline: "Messaging Tester 是一个简单的 HTML5 样本应用程序，它使用 MQTT over WebSocket 来模拟与 ${IMA_PRODUCTNAME_SHORT} 服务器交互的多个客户机。" +
						 "这些客户机一旦连接到服务器，它们就可以对这三个主题进行发布/预订。",
				enableSection: {
					title: "1. 启用端点 DemoMqttEndpoint",
					tagline: "Messaging Tester 样本应用程序必须连接到不受保护的 ${IMA_PRODUCTNAME_SHORT} 端点。您可以使用 DemoMqttEndpoint。"					
				},
				downloadSection: {
					title: "2. 下载并运行 Messaging Tester 样本应用程序",
					tagline: "单击此链接以下载 MessagingTester.zip。解压缩该文件，然后在支持 WebSocket 的浏览器中打开 index.html。" +
							 "按照该网页上的指示信息来验证 ${IMA_PRODUCTNAME_SHORT} 是否为 MQTT 消息传递做好准备。",
					linkLabel: "下载 MessagingTester.zip"
				},
				disableSection: {
					title: "3. 禁用端点 DemoMqttEndpoint",
					tagline: "完成 ${IMA_PRODUCTNAME_SHORT} MQTT 消息传递验证时，禁用端点 DemoMqttEndpoint 以防止在未经授权的情况下访问 ${IMA_PRODUCTNAME_SHORT}。"					
				},
				endpoint: {
					// TRNOTE {0} is replaced with the name of the endpoint
					label: "{0} 状态",
					state: {
						enabled: "已启用",					
						disabled: "已禁用",
						missing: "丢失",
						down: "关闭"
					},
					linkLabel: {
						enableLinkLabel: "启用",
						disableLinkLabel: "禁用"						
					},
					missingMessage: "如果另一个不受保护的端点不可用，请创建一个。",
					retrieveEndpointError: "检索端点配置时发生错误。",					
					retrieveEndpointStatusError: "检索端点状态时发生错误。",
					saveEndpointError: "设置端点状态时发生错误。"
				}
			}
		},
		protocolsLabel: "协议",

		// Messaging policy types
		topicPoliciesTitle: "主题策略",
		subscriptionPoliciesTitle: "预订策略",
		queuePoliciesTitle: "队列策略",
		// Messaging policy dialog strings
		addTopicMPTitle: "添加主题策略",
	    editTopicMPTitle: "编辑主题策略",
	    viewTopicMPTitle: "查看主题策略",
		removeTopicMPTitle: "删除主题策略",
		removeTopicMPContent: "确定要删除此主题策略吗？",
		topicMPInstruction: "主题策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题。" +
					     "每个端点至少应有一个主题策略、预订策略或队列策略。",
		addSubscriptionMPTitle: "添加预订策略",
		editSubscriptionMPTitle: "编辑预订策略",
		viewSubscriptionMPTitle: "查看预订策略",
		removeSubscriptionMPTitle: "删除预订策略",
		removeSubscriptionMPContent: "确定要删除此预订策略吗？",
		subscriptionMPInstruction: "预订策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些全球共享的预订。" +
					     "在全球共享的预订中，可在多个订户间共享从持久主题预订接收消息的工作。每个端点至少应有一个主题策略、预订策略或队列策略。",
		addQueueMPTitle: "添加队列策略",
		editQueueMPTitle: "编辑队列策略",
		viewQueueMPTitle: "查看队列策略",
		removeQueueMPTitle: "删除队列策略",
		removeQueueMPContent: "确定要删除此队列策略吗？",
		queueMPInstruction: "队列策略授权已连接的客户机执行特定的消息传递操作，例如，客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些队列。" +
					     "每个端点至少应有一个主题策略、预订策略或队列策略。",
		// Messaging and Endpoint dialog strings
		policyTypeTitle: "策略类型",
		policyTypeName_topic: "主题",
		policyTypeName_subscription: "全球共享的预订",
		policyTypeName_queue: "队列",
		// Messaging policy type names that are embedded in other strings
		policyTypeShortName_topic: "主题",
		policyTypeShortName_subscription: "预订",
		policyTypeShortName_queue: "队列",
		policyTypeTooltip_topic: "应用此策略的主题。使用星号 (*) 时请务必小心。" +
		    "星号匹配 0 个或更多个字符，包括斜杠 (/)。因此，可匹配主题树中的多个级别。",
		policyTypeTooltip_subscription: "应用此策略的全球共享的持久预订。使用星号 (*) 时请务必小心。" +
			"星号匹配 0 个或更多字符。",
	    policyTypeTooltip_queue: "应用此策略的队列。使用星号 (*) 时请务必小心。" +
			"星号匹配 0 个或更多字符。",
	    topicPoliciesTagline: "主题策略允许您控制客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些主题。",
		subscriptionPoliciesTagline: "预订策略允许您控制客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些全球共享的持久预订。",
	    queuePoliciesTagline: "队列策略允许您控制客户机可通过 ${IMA_PRODUCTNAME_FULL} 访问哪些队列。",
	    // Additional MQConnectivity queue manager connection dialog properties
	    channelUser: "通道用户",
	    channelPassword: "通道用户密码",
	    channelUserTooltip: "如果您的队列管理器已配置为对通道用户进行认证，那么必须设置该值。",
	    channelPasswordTooltip: "如果指定通道用户，还必须设置此值。",
	    // Additional LDAP dialog properties
	    emptyLdapURL: "未设置 LDAP URL",
		externalLDAPOnlyTagline: "为用于 ${IMA_PRODUCTNAME_SHORT} 服务器用户和组的 LDAP 存储库配置连接属性。", 
		// TRNNOTE: Do not translate bindPasswordHint.  This is a place holder string.
		bindPasswordHint: "********",
		clearBindPasswordLabel: "清除",
		resetLdapButton: "重置",
		resetLdapTitle: "重置 LDAP 配置",
		resetLdapContent: "所有 LDAP 配置设置将重置为缺省值。",
		resettingProgress: "正在重置...",
		// New SSL Key repository dialog strings
		sslkeyrepositoryDialogUploadContent: "如果 MQConnectivity 服务正在运行，将在上载存储库文件后重新启动此服务。",
		savingRestartingProgress: "正在保存并重新启动...",
		deletingRestartingProgress: "正在删除并重新启动..."
});
