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
			title : "主页",
			taskContainer : {
				title : "常用配置和定制任务",
				tagline: "常规管理任务的快速链接。",
				// TRNOTE {0} is a number from 1 - 5.
				taskCount : "（剩余 {0} 个任务）",
				links : {
					restore : "复原任务",
					restoreTitle: "将关闭的任务复原到常用配置和定制任务部分。",
					hide : "隐藏部分",
					hideTitle: "隐藏常用配置和定制任务部分。" +
							"要复原此部分，请从“帮助”菜单选择“复原主页上的任务”。"						
				}
			},
			tasks : {
				messagingTester : {
					heading: "使用 ${IMA_PRODUCTNAME_SHORT} Messaging Tester 样本应用程序验证配置",
					description: "Messaging Tester 是一个简单的 HTML5 样本应用程序，使用 MQTT over WebSocket 来验证服务器是否配置正确。",
					links: {
						messagingTester: "Messaging Tester 样本应用程序"
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
					heading : "创建用户",
					description : "为用户提供对 Web UI 的访问权。",
					links : {
						users : "Web UI 用户",
						userGroups : "LDAP 配置"						
					}
				},
				connections: {
					heading: "将 ${IMA_PRODUCTNAME_FULL} 配置为接受连接",
					description: "定义消息中心以管理服务器连接。配置 MQ Connectivity，以将 ${IMA_PRODUCTNAME_FULL} 连接到 MQ 队列管理器。配置 LDAP 以对消息传递用户进行认证。",
					links: {
						listeners: "消息中心",
						mqconnectivity: "MQ Connectivity"
					}
				},
				security : {
					heading : "保护您的环境",
					description : "控制 Web UI 侦听的接口和端口。导入密钥和证书以保护与服务器的连接。",
					links : {
						keysAndCerts : "服务器安全性",
						webuiSettings : "Web UI 设置"
					}
				}
			},
			dashboards: {
				tagline: "服务器连接和性能的概述。",
				monitoringNotAvailable: "监视由于 ${IMA_PRODUCTNAME_SHORT} 服务器的状态而不可用。",
				flex: {
					title: "服务器仪表板",
					// TRNOTE {0} is a user provided name for the server.
					titleWithNodeName: "{0} 的服务器仪表板",
					quickStats: {
						title: "快速统计"
					},
					liveCharts: {
						title: "活动连接和吞吐量"
					},
					applianceResources: {
						title: "服务器内存使用情况"
					},
					resourceDetails: {
						title: "存储内存使用情况"
					},
					diskResourceDetails: {
						title: "磁盘使用情况"
					},
					activeConnectionsQS: {
						title: "活动连接计数",
						description: "活动连接数。",
						legend: "活动连接数"
					},
					throughputQS: {
						title: "当前吞吐量",
						description: "当前吞吐量，以消息数/秒计。",
						legend: "消息数/秒"
					},
					uptimeQS: {
						title: "${IMA_PRODUCTNAME_FULL} 正常运行时间",
						description: "${IMA_PRODUCTNAME_FULL} 的状态及其运行时长。",
						legend: "${IMA_PRODUCTNAME_SHORT} 正常运行时间"
					},
					applianceQS: {
						title: "服务器资源",
						description: "持久存储磁盘空间和已使用的服务器内存的百分比。",
						bars: {
							pMem: {
								label: "持久内存"								
							},
							disk: { 
								label: "磁盘空间：",
								warningThresholdText: "资源使用率位于或高于警告阈值。",
								alertThresholdText: "资源使用率位于或高于警报阈值。"
							},
							mem: {
								label: "内存：",
								warningThresholdText: "内存使用率越来越高。内存使用率达到 85% 时，将拒绝发布内容，且连接可能断开。",
								alertThresholdText: "内存使用率过高。将拒绝发布内容，且连接可能断开。"	
							}
						},
						warningThresholdAltText: "警告图标",
						alertThresholdText: "资源使用率位于或高于警报阈值。",
						alertThresholdAltText: "警报图标"
					},
					connections: {
						title: "连接",
						description: "连接的快照，大约每五秒创建一次。",
						legend: {
							x: "时间",
							y: "连接"
						}
					},
					throughput: {
						title: "吞吐量",
						description: "每秒的消息数快照，大约每五秒创建一次。",
						legend: {
							x: "时间",						
							y: "消息数/秒",
							title: "消息数/秒",
							incoming: "入局",
							outgoing: "出局",
							hover: {
								incoming: "每秒从客户机读取的消息数快照，大约每五秒创建一次。",
								outgoing: "每秒写入客户机的消息数快照，大约每五秒创建一次。"
							}
						}
					},
					memory: {
						title: "内存",
						description: "内存使用率的快照，大约每分钟创建一次。",
						legend: {
							x: "时间",
							y: "已用内存 (%)"
						}
					},
					disk: {
						title: "磁盘",
						description: "持久存储磁盘使用率的快照，大约每分钟创建一次。",
						legend: {
							x: "时间",
							y: "已用磁盘空间 (%)"
						}						
					},
					memoryDetail: {
						title: "内存使用情况的详细信息",
						description: "针对主要消息传递资源的内存使用率快照，大约每分钟创建一次。",
						legend: {
							x: "时间",
							y: "已用内存 (%)",
							title: "消息传递内存使用情况",
							system: "服务器主机系统",
							messagePayloads: "消息有效内容",
							publishSubscribe: "发布预订",
							destinations: "目标",
							currentActivity: "当前活动",
							clientStates: "客户机状态",
							hover: {
								system: "为其他系统使用（如操作系统、连接、安全性、日志记录和管理）分配的内存。",
								messagePayloads: "为消息分配的内存。" +
										"当消息发布到多个订户时，在内存中只分配消息的一个副本。" +
										"在该类别中还分配保留消息的内存。" +
										"为断开连接或缓慢的使用者在服务器中缓冲大量消息的工作负载以及使用大量保留消息的工作负载可消耗大量消息有效内容内存。",
								publishSubscribe: "为执行发布/预订消息传递分配的内存。" +
										"服务器在该类别中分配内存以对保留消息和预订保持跟踪。" +
										"出于性能考虑，服务器维护发布/预订信息的高速缓存。" +
										"使用大量预订的工作负载以及使用大量保留消息的工作负载可以消耗大量发布/预订内存。",
								destinations: "为消息传递目标分配的内存。" +
										"该类别内存用于将消息组织为供客户机使用的队列和预订。" +
										"在服务器中缓冲大量消息的工作负载以及使用大量预订的工作负载可以消耗大量的目标内存。",
								currentActivity: "为当前活动分配的内存。" +
										"这包括会话、事务和消息确认。" +
										"在该类别中还分配用于满足监视请求的信息。" +
										"带有大量已连接客户机的复杂工作负载以及消耗大量功能资源（如事务或未确认的消息）的工作负载可消耗大量的当前活动内存。",
								clientStates: "为已连接和断开连接的客户机分配的内存。" +
										"服务器在该类别中为每个已连接的客户机分配内存。" +
										"在 MQTT 中，在尝试断开连接时必须记录连接设置 CleanSession 到 0 的客户机。" +
										"此外，从该类别分配内存以对 MQTT 中的入局和出局消息确认保持跟踪。" +
										"使用大量已连接和断开连接的客户机的工作负载可以消耗大量的客户机状态内存，尤其是使用高质量服务消息时。"								
							}
						}
					},
					storeMemory: {
						title: "持久存储内存使用情况详细信息",
						description: "持久存储内存使用情况的快照，大约每分钟创建一次。",
						legend: {
							x: "时间",
							y: "已用内存 (%)",
							pool1Title: "池 1 内存使用情况",
							pool2Title: "池 2 内存使用情况",
							system: "服务器主机系统",
							IncomingMessageAcks: "入局消息确认",
							MQConnectivity: "MQ Connectivity",
							Transactions: "事务",
							Topics: "具有保留消息的主题",
							Subscriptions: "订购",
							Queues: "队列",										
							ClientStates: "客户机状态",
							recordLimit: "记录限制",
							hover: {
								system: "由系统保留或在系统中正在使用的存储内存。",
								IncomingMessageAcks: "为确认入局消息所分配的存储内存。" +
										"服务器在此类别中分配内存给使用 CleanSession=0 设置进行连接并且正在发布服务质量为 2 的消息的 MQTT 客户机。" +
										"此内存用于确保有且仅有一次的传递。",
								MQConnectivity: "为与 IBM WebSphere MQ 队列管理器连接分配的存储内存。",
								Transactions: "为事务分配的存储内存。" +
										"服务器在此类别中为每个事务分配内存，以便在服务器重新启动时事务可完成恢复。",
								Topics: "为主题分配的存储内存。" +
										"服务器在此类别中为具有保留消息的每个主题分配内存。",
								Subscriptions: "为持久预订分配的存储内存。" +
										"在 MQTT 中，这些是针对使用 CleanSession=0 设置连接的客户机的预订。",
								Queues: "为队列分配的存储内存。" +
										"在此类别中为针对点到点消息传递创建的每个队列分配内存。",										
								ClientStates: "为断开连接时必须记住的客户机分配的存储内存。" +
										"在 MQTT 中，这些是使用 CleanSession=0 设置连接的客户机或者已连接并设置服务质量为 1 或 2 的 will 消息的客户机。",
								recordLimit: "达到记录限制后，就不能为保留消息、持久预订、队列、客户机和与 WebSphere MQ 队列管理器的连接创建记录。"
							}
						}						
					},					
					peakConnectionsQS: {
						title: "连接数峰值",
						description: "指定时间段内活动连接数的峰值。",
						legend: "连接数峰值"
					},
					avgConnectionsQS: {
						title: "平均连接数",
						description: "指定时间段内的平均活动连接数。",
						legend: "平均连接数"
					},
					peakThroughputQS: {
						title: "吞吐量峰值",
						description: "指定时间段内的每秒消息数峰值。",
						legend: "消息数峰值/秒"
					},
					avgThroughputQS: {
						title: "平均吞吐量",
						description: "指定时间段内的每秒平均消息数。",
						legend: "平均消息数/秒"
					}
				}
			},
			status: {
				rc0: "正在初始化",
				rc1: "正在运行（生产）",
				rc2: "正在停止",
				rc3: "已初始化",
				rc4: "传输已启动",
				rc5: "协议已启动",
				rc6: "存储库已启动",
				rc7: "引擎已启动",
				rc8: "消息传递已启动",
				rc9: "正在运行（维护）",
				rc10: "就绪",
				rc11: "正在启动存储",
				rc12: "维护（正在清除存储）",
				rc99: "已停止",				
				unknown: "未知",
				// TRNOTE {0} is a mode, such as production or maintenance.
				modeChanged: "服务器重新启动时将处于<em>{0}</em>方式。",
				cleanStoreSelected: "已请求<em>清除存储</em>操作。将在重新启动服务器后生效。",
				mode_0: "生产",
				mode_1: "维护",
				mode_2: "清除存储",
				adminError: "${IMA_PRODUCTNAME_FULL} 检测到错误。",
				adminErrorDetails: "错误代码为：{0}。错误字符串为：{1}"
			},
			storeStatus: {
				mode_0: "持久",
				mode_1: "仅内存"
			},
			memoryStatus: {
				ok: "确定",
				unknown: "未知",
				error: "内存检测报告了错误。",
				errorMessage: "内存检测报告了错误。请联系支持人员。"
			},
			role: {
				PRIMARY: "主节点",
				PRIMARY_SYNC: "主节点（正在同步，已完成 {0}%）",
				STANDBY: "备用节点",
				UNSYNC: "节点不同步",
				TERMINATE: "主节点终止了备用节点",
				UNSYNC_ERROR: "节点不能再同步",
				HADISABLED: "已禁用",
				UNKNOWN: "未知",
				unknown: "未知"
			},
			haReason: {
				"1": "两个节点的存储配置不允许它们充当 HA 对。" +
				     "不匹配的配置项为：{0}",
				"2": "发现时间已到期。无法与 HA 对中的另一个节点通信。",
				"3": "两个主节点已确定。",
				"4": "已标识两个未同步的节点。",
				"5": "无法确定哪个节点是主节点，因为两个节点都具有非空存储。",
				"7": "由于两个节点具有不同的组标识，因此节点无法与远程节点配对。",
				"9": "发生了 HA 服务器或内部错误",
				// TRNOTE {0} is a time stamp (date and time).
				lastPrimaryTime: "在 {0} 时，此节点是主节点。"
			},
			statusControl: {
				label: "状态",
				ismServer: "${IMA_PRODUCTNAME_FULL} 服务器：",
				serverNotAvailable: "请联系您的服务器系统管理员以启动服务器",
				serverNotAvailableNonAdmin: "请联系您的服务器系统管理员以启动服务器",
				haRole: "高可用性：",
				pending: "正在暂挂..."
			}
		},
		recordsLabel: "记录",
		// Additions for cluster status to appear in status dropdown
		statusControl_cluster: "集群：",
		clusterStatus_Active: "活动",
		clusterStatus_Inactive: "不活动",
		clusterStatus_Removed: "已移除",
		clusterStatus_Connecting: "正在连接",
		clusterStatus_None: "未配置",
		clusterStatus_Unknown: "未知",
		// Additions to throughput chart on dashboard (also appears on cluster status page)
		//TRNOTE: Remote throughput refers to messages from remote servers that are members 
		// of the same cluster as the local server.
		clusterThroughputLabel: "远程",
		// TRNOTE: Local throughput refers to messages that are received directly from
		// or that are sent directly to messaging clients that are connected to the
		// local server.
		clientThroughputLabel: "本地",
		clusterLegendIncoming: "入局",
		clusterLegendOutgoing: "出局",
		clusterHoverIncoming: "每秒从远程集群成员读取的消息数快照，大约每五秒创建一次。",
		clusterHoverOutgoing: "每秒写入远程集群成员的消息数快照，大约每五秒创建一次。"

});
