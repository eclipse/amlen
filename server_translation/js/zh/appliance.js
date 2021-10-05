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
		// Appliance
		// ------------------------------------------------------------------------
		appliance : {
			users: {
				title: "Web UI 用户管理",
				tagline: "为用户提供对 Web UI 的访问权。",
				usersSubTitle: "Web UI 用户",
				webUiUsersTagline: "系统管理员可以定义、编辑或删除 Web UI 用户。",
				itemName: "用户",
				itemTooltip: "",
				addDialogTitle: "添加用户",
				editDialogTitle: "编辑用户",
				removeDialogTitle: "删除用户",
				resetPasswordDialogTitle: "重置密码",
				// TRNOTE: Do not translate addGroupDialogTitle, editGroupDialogTitle, removeGroupDialogTitle
				//addGroupDialogTitle: "Add Group",
				//editGroupDialogTitle: "Edit Group",
				//removeGroupDialogTitle: "Delete Group",
				grid: {
					userid: "用户标识",
					// TRNOTE: Do not translate groupid
					//groupid: "Group ID",
					description: "描述",
					groups: "角色",
					noEntryMessage: "没有要显示的项目",
					// TRNOTE: Do not translate ldapEnabledMessage
					//ldapEnabledMessage: "Messaging users and groups configured on the appliance are disabled because an external LDAP connection is enabled.",
					refreshingLabel: "正在更新..."
				},
				dialog: {
					useridTooltip: "输入用户标识，该标识不能使用前导或尾部空格。",
					applianceUserIdTooltip: "输入用户标识，该标识只能由字母 A-Za-z、数字 0-9 以及四个特殊字符 - _ . 和 + 组成",					
					useridInvalid: "用户标识不能使用前导或尾部空格。",
					applianceInvalid: "用户标识只能由字母 A-Za-z、数字 0-9 以及四个特殊字符 - _ . 和 + 组成",					
					// TRNOTE: Do not translate groupidTooltip, groupidInvalid
					//groupidTooltip: "Enter the Group ID, which must not have leading or trailing spaces.",
					//groupidInvalid: "The Group ID must not have leading or trailing spaces.",
					SystemAdministrators: "系统管理员",
					MessagingAdministrators: "消息传递管理员",
					Users: "用户",
					applianceUserInstruction: "系统管理员可以创建、更新或删除其他 Web UI 管理员和 Web UI 用户。",
					// TRNOTE: Do not translate messagingUserInstruction, messagingGroupInstruction
					//messagingUserInstruction: "Messaging users can be added to messaging policies to control the messaging resources that a user can access.",
					//messagingGroupInstruction: "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.",				                    
					password1: "密码：",
					password2: "确认密码：",
					password2Invalid: "密码不匹配。",
					passwordNoSpaces: "密码不能使用前导或尾部空格。",
					saveButton: "保存",
					cancelButton: "取消",
					refreshingGroupsLabel: "正在检索组..."
				},
				returnCodes: {
					CWLNA5056: "某些设置无法保存"
				}
			},
// TRNOTE: Do not tranlate messages in networkSettings
//			networkSettings: {
//				title: "Network Settings",
//				tagline: "Information related to network settings.",
//				ethernetSubTitle: "Ethernet Interfaces",
//				ethernetTagline: "Ethernet interfaces are used for messaging and server management.",
//				haSubTitle: "High Availability Interfaces",
//				haTagline: "High availability interfaces are dedicated to replication and discovery.",
//				dnsSubTitle: "Domain Name Servers and Search Domains",
//				dnsTagline: "System administrators can define, edit, or delete DNS addresses and search domains.",
//				dnsSubTitleCCI: "Search Domains",
//				dnsTaglineCCI: "System administrators can define, edit, or delete search domains."
//			},
			removeDialog: {
				title: "删除条目",
				content: "确定要删除该记录吗？",
				deleteButton: "删除",
				cancelButton: "取消"
			},
			savingProgress: "正在保存...",
			savingFailed: "保存失败。",
			deletingProgress: "正在删除...",
			deletingFailed: "删除失败。",
			testingProgress: "正在测试...",
			uploadingProgress: "正在上载...",
			dialog: { 
				saveButton: "保存",
				cancelButton: "取消",
				testButton: "测试连接"
			},
			invalidName: "已存在同名记录。",
			invalidChars: "需要有效主机名或 IP 地址。",
			invalidRequired: "值是必需的。",
// TRNOTE: Do not translate any of the strings in ethernetInterfaces
//			ethernetInterfaces : {
//				grid : {
//					enabled : "Enabled",
//					name : "Name",
//					ipAddr : "IPv4 Address",
//					gateway : "IPv4 Default Gateway",
//					ipAddrV6 : "IPv6 Address",
//					gatewayV6 : "IPv6 Default Gateway",
//					status: "Status",
//					statusUp: "Up",
//					statusDown: "Down",
//					statusWarning: "Warning"
//				},
//				dialog: {
//					tooltip: {
//						ipAddr: "Enter the IPv4 address range using Classless Inter-Domain Routing (CIDR) notation, for example 192.168.1.0/24. Refer to the documentation for details about CIDR.",
//						gateway: "Enter the IPv4 address of the default gateway. This can only be entered when an IPv4 address has been entered",
//						ipAddrV6: "Enter the IPv6 address range using Classless Inter-Domain Routing (CIDR) notation, for example 2001:db8::/32. Refer to the documentation for details about CIDR.",
//						gatewayV6: "Enter the IPv6 address of the default gateway. This can only be entered when an IPv6 address has been entered"
//					},
//					invalid: {
//						ipAddr:  "The IPv4 address range must be specified in CIDR notation, for example 192.168.1.0/24",
//						gateway: "The gateway must be specified as an IPv4 address, for example 192.168.1.254",
//						ipAddrV6:  "The IPv6 address range must be specified in CIDR notation, for example 2001:db8::/32",
//						gatewayV6: "The gateway must be specified as an IPv6 address, for example 2001:0db8:85a3:0042:0000:8a2e:0370:7334"
//					},
//					dhcpMessage: "IP addresses for this interface are automatically assigned and cannot be edited because the interface is configured to use DHCP."
//				},
//				editDialog: {
//					title: "Edit Ethernet Interface",
//					instruction: "Ethernet interfaces are used for messaging and appliance management."
//				},
//				editHaDialog: {
//					title: "Edit High Availability Interface",
//					instruction: "High availability interfaces are dedicated to replication and discovery."
//				},
//				clearedInfoDialog: {
//					title: "Address Cleared",					
//					content: {
//						// TRNOTE {0} is an IP address. {1} is an interface name (e.g. eth0). {2} is either Ethernet Interfaces or High Availability Interfaces.
//						singular: "The following address was successfully cleared: {0}.  " +
//					    		  "If additional addresses are configured for {1}, the next address will appear in the {2} list.",									
//					    // TRNOTE {0} is a list of IP addresses. {1} is an interface name (e.g. eth0). {2} is either Ethernet Interfaces or High Availability Interfaces.					    		  
//						plural: "The following addresses were successfully cleared: {0}.  " +
//							    "If additional addresses are configured for {1}, the next address will appear in the {2} list."
//					},
//					closeButton: "Close"					
//				},
//				warnWebUIChangeDialog: {
//					title : "IP Address Warning",
//					content : "Are you sure you want to change the IP address used by the user interface? " + 
//					          "You will be redirected to the new IP address, but if any loss of connectivity occurs you must manually navigate to the new IP address. " +
//					          "You might be required to log in again.",
//					// TRNOTE {0} is an IP address.					          
//					contentDoNotProceed : "You are about to change the IP address on an Ethernet interface used by the Web UI. " + 
//					                      "To prevent loss of connectivity, make sure you change the Web UI IP address first. " + 
//					                      "To do so, please navigate to {0}",
//					// TRNOTE {0} is an interface name (e.g. eth0).
//					contentDisabled : "You are about to disable the connection used by the user interface. Disabling the connection will stop the user interface from working. " +
//							"To re-enable the connection, use the command line interface to execute the following command:  <em>edit ethernet-interface {0}</em>",
//					contentGeneric : "You are about to save changes to the connection that is used by the user interface. Changing the connection may stop the user interface from working.",
//					continueButton : "Continue with Changes",
//					cancelButton : "Cancel"
//				}
//			},
			testConnection: {
				nothingToTest: "必须提供 IPv4 或 IPv6 地址以测试连接。",
				testConnFailed: "测试连接失败",
				testConnSuccess: "测试连接成功",
				testFailed: "测试失败",
				testSuccess: "测试成功"
			},
// TRNOTE: Do not trandlate strings in dns, domain, ntp, serverLocale
//			dns: {
//				itemName: "Domain Name Server Address",
//				itemTooltip: "",
//				addDialogTitle: "Add Domain Name Server",
//				editDialogTitle: "Edit Domain Name Server",
//				removeDialogTitle: "Delete Domain Name Server"
//			},
//			domain: {
//				itemName: "Search Domain",
//				itemTooltip: "A domain name to use when resolving host names",
//				addDialogTitle: "Add Search Domain",
//				editDialogTitle: "Edit Search Domain",
//				removeDialogTitle: "Delete Search Domain"
//			},
//			ntp: {
//				itemName: "NTP Server Address",
//				itemTooltip: "",
//				addDialogTitle: "Add Network Time Protocol (NTP) Server",
//				editDialogTitle: "Edit Network Time Protocol (NTP) Server",
//				removeDialogTitle: "Delete Network Time Protocol (NTP) Server"
//			},
//			serverLocale : {
//				pageTitle: "Server Locale, Date and Time",
//				retrievingCurrentLocale: "Retrieving the server locale.",
//				retrievingCurrentTime: "Retrieving the server time.",				
//				currentLocaleTemplate: "The server locale is {0}.",
//				currentTimeTemplate: "The server time is {0}.",
//				currentTimeTemplateBadTimezone: "The server time is {0}. (Unable to format using timezone {1}.)",
//				saveButton: "Save",
//				locale: {
//					sectionTitle: "Server Locale and Timezone",
//					localeDesc: "The server locale is used for formatting server log messages. " +
//					 			"If a translation for the specified language is available, server log messages are translated. " +
//					 			"Otherwise messages are shown in English. " +
//					 			"Translations exist for the following languages: German, English, French, Japanese, Simplified Chinese, and Traditional Chinese.",
//					localeLabel: "Server locale:",
//					saveDialog: {
//						title: "Change Server Locale",
//						content: "Are you sure you want to change the IBM IoT MessageSight server locale to <strong>{0}</strong>? " +
//						         "The locale change will take affect immediately, however, the Web UI will not recognize the new locale setting until restarted. " +
//						         "The Web UI can be restarted by using the imaserver stop webui and imaserver start webui commands.", 
//						saveButton: "Save",
//						cancelButton: "Cancel",
//						saveFailed: "The server locale could not be changed."
//					}
//				},
//				dateTime: {
//					sectionTitle: "Date and Time",
//					dateTimeDesc: "The date and time are not set to synchronize with a Network Time Protocol (NTP) server. " +
//							"<strong>Changing the date or time will restart the IBM IoT MessageSight Web UI.</strong>",
//					dateTimeNtpDesc: "The date and time are set to use the configured Network Time Protocol (NTP) servers.",
//					timeZoneLabel: "Server time zone:",
//					timeLabel: "Time:",
//					dateLabel: "Date:",
//					saving: "Saving...",
//					success: "Save successful. The Web UI is restarting...",
//					failed: "Save failed",
//					cancel: "Save cancelled",
//					invalidDateOrTime: "The date or time is invalid. Correct the problem and try again.",
//					saveDialog: {
//						title: "Change Date and Time",
//						content: "Are you sure you want to change the IBM IoT MessageSight date and time? " +
//						         "Changing the date and time will restart the Web UI. You must log in again.",
//						saveButton: "Save",
//						cancelButton: "Cancel"						
//					}
//				},
//				ntp: {
//					sectionTitle: "Network Time Protocol Servers",
//					tagline: "System administrators can define, edit, or delete a server address to which the appliance can be synchronized for the time and date.",
//					ntpDesc: "The use of a Network Time Protocol (NTP) Server helps ensure accurate date and time stamps on your appliances."
//				}
//			},
			securitySettings: {
				title: "安全设置",
				tagline: "导入密钥和证书以保护与服务器的连接。",
				securityProfilesSubTitle: "安全概要文件",
				securityProfilesTagline: "系统管理员可以定义、编辑或删除确定验证客户机时使用方法的安全概要文件。",
				certificateProfilesSubTitle: "证书概要文件",
				certificateProfilesTagline: "系统管理员可以定义、编辑或删除验证用户身份的证书概要文件。",
				ltpaProfilesSubTitle: "LTPA 概要文件",
				ltpaProfilesTagline: "系统管理员可以定义、编辑或删除轻量级第三方认证 (LTPA) 概要文件以在多个服务器之间启用单点登录。",
				oAuthProfilesSubTitle: "OAuth 概要文件",
				oAuthProfilesTagline: "系统管理员可以定义、编辑或删除 OAuth 概要文件以在多个服务器之间启用单点登录。",
				systemWideSubTitle: "服务器范围安全设置",
				useFips: "使用 FIPS 140-2 概要文件保护消息传递通信安全",
				useFipsTooltip: "选择该选项，以针对使用安全概要文件保护的端点，使用符合联邦信息处理标准 (FIPS) 140-2 的密码术。" +
				                "更改此设置需要重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器。",
				retrieveFipsError: "检索 FIPS 设置时发生错误。",
				fipsDialog: {
					title: "设置 FIPS",
					content: "必须重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器才能使此更改生效。确定要更改此设置吗？"	,
					okButton: "设置并重新启动",
					cancelButton: "取消更改",
					failed: "更改 FIPS 设置时发生错误。",
					stopping: "正在停止 ${IMA_PRODUCTNAME_SHORT} 服务器...",
					stoppingFailed: "停止 ${IMA_PRODUCTNAME_SHORT} 服务器时发生错误。",
					starting: "正在启动 ${IMA_PRODUCTNAME_SHORT} 服务器...",
					startingFailed: "启动 ${IMA_PRODUCTNAME_SHORT} 服务器时发生错误。"
				}
			},
			securityProfiles: {
				grid: {
					name: "姓名",
					mprotocol: "最低协议方法",
					ciphers: "密码",
					certauth: "使用客户机证书",
					clientciphers: "使用客户机密码",
					passwordAuth: "使用密码认证",
					certprofile: "证书概要文件",
					ltpaprofile: "LTPA 概要文件",
					oAuthProfile: "OAuth 概要文件",
					clientCertsButton: "可信证书"
				},
				trustedCertMissing: "缺少可信证书",
				retrieveError: "检索安全概要文件时发生错误。",
				addDialogTitle: "添加安全概要文件",
				editDialogTitle: "编辑安全概要文件",
				removeDialogTitle: "删除安全概要文件",
				dialog: {
					nameTooltip: "名称最多包含 32 个字母数字字符。第一个字符不能是数字。",
					mprotocolTooltip: "连接时许可的最低级别协议方法。",
					ciphersTooltip: "<strong>最佳：</strong>选择服务器与客户机所支持的最安全的密码。<br />" +
							        "<strong>最快：</strong>选择服务器与客户机所支持的最快的高安全性密码。<br />" +
							        "<strong>中等：</strong>选择客户机所支持的最快的中等安全性密码或高安全性密码。",
				    ciphers: {
				    	best: "最佳",
				    	fast: "最快",
				    	medium: "中等"
				    },
					certauthTooltip: "选择是否认证客户机证书。",
					clientciphersTooltip: "选择是否允许客户机确定连接时使用的密码套件。",
					passwordAuthTooltip: "选择是否在连接时需要有效的用户标识和密码。",
					passwordAuthTooltipDisabled: "无法修改，因为已选择 LTPA 或 OAuth 概要文件。要修改，请将 LTPA 或 OAuth 概要文件更改为<em>选择一个</em>",
					certprofileTooltip: "选择现有的证书概要文件以与该安全概要文件相关联。",
					ltpaprofileTooltip: "选择现有的 LTPA 概要文件以与该安全概要文件相关联。" +
					                    "如果已选择 LTPA 概要文件，那么还必须选择“使用密码认证”。",
					ltpaprofileTooltipDisabled: "无法选择 LTPA 概要文件，因为已选择 OAuth 概要文件，或未选择“使用密码认证”。",
					oAuthProfileTooltip: "选择现有的 OAuth 概要文件以与该安全概要文件相关联。" +
                    					  "如果已选择 OAuth 概要文件，那么还必须选择“使用密码认证”。",
                    oAuthProfileTooltipDisabled: "无法选择 OAuth 概要文件，因为已选择 LTPA 概要文件，或未选择“使用密码认证”。",
					noCertProfilesTitle: "无证书概要文件",
					noCertProfilesDetails: "选择<em>使用 TLS</em> 时，只有在先定义了证书概要文件之后，才能定义安全概要文件。",
					chooseOne: "选择一个"
				},
				certsDialog: {
					title: "可信证书",
					instruction: "将证书上载到安全概要文件的客户机证书信任库或者从中删除证书",
					securityProfile: "安全概要文件",
					certificate: "证书",
					browse: "浏览",
					browseHint: "选择文件...",
					upload: "上载",
					close: "关闭",
					remove: "删除",
					removeTitle: "删除证书",
					removeContent: "是否确实删除此证书?",
					retrieveError: "检索证书时发生错误。",
					uploadError: "上载证书文件时发生错误。",
					deleteError: "删除证书文件时发生错误。"	
				}
			},
			certificateProfiles: {
				grid: {
					name: "姓名",
					certificate: "证书",
					privkey: "专用密钥",
					certexpiry: "证书期限",
					noDate: "检索日期时发生错误..."
				},
				retrieveError: "检索证书概要文件时发生错误。",
				uploadCertError: "上载证书文件时发生错误。",
				uploadKeyError: "上载密钥文件时发生错误。",
				uploadTimeoutError: "上载证书或密钥文件时发生错误。",
				addDialogTitle: "添加证书概要文件",
				editDialogTitle: "编辑证书概要文件",
				removeDialogTitle: "删除证书概要文件",
				dialog: {
					certpassword: "证书密码：",
					keypassword: "密钥密码：",
					browse: "浏览...",
					certificate: "证书：",
					certificateTooltip: "", // "Upload a certificate"
					privkey: "专用密钥：",
					privkeyTooltip: "", // "Upload a private key"
					browseHint: "选择文件...",
					duplicateFile: "已存在同名文件。",
					identicalFile: "证书或密钥文件不能同名。",
					noFilesToUpload: "要更新证书概要文件，请至少选择一个文件进行上载。"
				}
			},
			ltpaProfiles: {
				addDialogTitle: "添加 LTPA 概要文件",
				editDialogTitle: "编辑 LTPA 概要文件",
				instruction: "在安全概要文件中使用 LTPA（轻量级第三方认证）概要文件以在多个服务器之间启用单点登录。",
				removeDialogTitle: "删除 LTPA 概要文件",				
				keyFilename: "密钥文件名",
				keyFilenameTooltip: "包含已导出 LTPA 密钥的文件。",
				browse: "浏览...",
				browseHint: "选择文件...",
				password: "密码",
				saveButton: "保存",
				cancelButton: "取消",
				retrieveError: "检索 LTPA 概要文件时发生错误。",
				duplicateFileError: "已存在同名密钥文件。",
				uploadError: "上载密钥文件时发生错误。",
				noFilesToUpload: "要更新 LTPA 概要文件，请选择一个文件进行上载。"
			},
			oAuthProfiles: {
				addDialogTitle: "添加 OAuth 概要文件",
				editDialogTitle: "编辑 OAuth 概要文件",
				instruction: "在安全概要文件中使用 OAuth 概要文件以在多个服务器之间启用单点登录。",
				removeDialogTitle: "删除 OAuth 概要文件",	
				resourceUrl: "资源 URL",
				resourceUrlTooltip: "要用于验证访问令牌的授权服务器 URL。URL 必须包含协议。协议可以是 http 或 https。",
				keyFilename: "OAuth 服务器证书",
				keyFilenameTooltip: "包含用于连接到 OAuth 服务器的 SSL 证书的文件名称。",
				browse: "浏览...",
				reset: "重置",
				browseHint: "选择文件...",
				authKey: "授权密钥",
				authKeyTooltip: "包含访问令牌的密钥的名称。缺省名称为 <em>access_token</em>。",
				userInfoUrl: "用户信息 URL",
				userInfoUrlTooltip: "用于检索用户信息的授权服务器 URL。URL 必须包含协议。协议可以是 http 或 https。",
				userInfoKey: "用户信息键",
				userInfoKeyTooltip: "用于检索用户信息的键的名称。",
				saveButton: "保存",
				cancelButton: "取消",
				retrieveError: "检索 OAuth 概要文件时发生错误。",
				duplicateFileError: "已存在同名密钥文件。",
				uploadError: "上载密钥文件时发生错误。",
				urlThemeError: "输入使用 http 或 https 协议的有效 URL。",
				inUseBySecurityPolicyMessage: "无法删除 OAuth 概要文件，因为以下安全概要文件正在使用：{0}。",			
				inUseBySecurityPoliciesMessage: "无法删除 OAuth 概要文件，因为以下安全概要文件正在使用：{0}。"			
			},			
			systemControl: {
				pageTitle: "服务器控制",
				appliance: {
					subTitle: "服务器",
					tagLine: "影响 ${IMA_PRODUCTNAME_FULL} 服务器的设置和操作。",
					hostname: "服务器名称",
					hostnameNotSet: "（无）",
					retrieveHostnameError: "检索服务器名称时发生错误。",
					hostnameError: "保存服务器名称时发生错误。",
					editLink: "编辑",
	                viewLink: "查看",
					firmware: "固件版本",
					retrieveFirmwareError: "检索固件版本时发生错误。",
					uptime: "正常运行时间",
					retrieveUptimeError: "检索正常运行时间时发生错误。",
					// TRNOTE Do not translate locateLED, locateLEDTooltip
					//locateLED: "Locate LED",
					//locateLEDTooltip: "Locate your appliance by turning on the blue LED.",
					locateOnLink: "打开",
					locateOffLink: "关闭",
					locateLedOnError: "打开定位指示灯时发生错误。",
					locateLedOffError: "关闭定位指示灯时发生错误。",
					locateLedOnSuccess: "定位指示灯已打开。",
					locateLedOffSuccess: "定位指示灯已关闭。",
					restartLink: "重新启动服务器",
					restartError: "重新启动服务器时发生错误。",
					restartMessage: "已提交重新启动服务器的请求。",
					restartMessageDetails: "重新启动服务器可能需要几分钟时间。用户接口在服务器重新启动时不可用。",
					shutdownLink: "关闭服务器",
					shutdownError: "关闭服务器时发生错误。",
					shutdownMessage: "已提交关闭服务器的请求。",
					shutdownMessageDetails: "关闭服务器可能需要几分钟时间。用户接口在服务器关闭时不可用。" +
					                        "要重新打开服务器，请按服务器上的电源按钮。",
// TRNOTE Do not translate sshHost, sshHostSettings, sshHostStatus, sshHostTooltip, sshHostStatusTooltip,
// retrieveSSHHostError, sshSaveError
//					sshHost: "SSH Host",
//				    sshHostSettings: "Settings",
//				    sshHostStatus: "Status",
//				    sshHostTooltip: "Set the host IP address or addresses that can access the IBM IoT MessageSight SSH console.",
//				    sshHostStatusTooltip: "The host IP addresses that the SSH service is currently using.",
//				    retrieveSSHHostError: "An error occurred while retrieving the SSH host setting and status.",
//				    sshSaveError: "An error occurred while configuring IBM IoT MessageSight to allow SSH connections on the address or addresses provided.",

				    licensedUsageLabel: "许可用途",
				    licensedUsageRetrieveError: "检索许可用途设置时发生错误。",
                    licensedUsageSaveError: "保存许可用途设置时发生错误。",
				    licensedUsageValues: {
				        developers: "开发人员",
				        nonProduction: "非生产",
				        production: "生产"
				    },
				    licensedUsageDialog: {
				        title: "更改许可用途",
				        instruction: "更改服务器的许可用途。" +
				        		"如果您拥有权利证明 (POE)，那么您选择的许可用途应与 POE 中声明的程序授权用途（“生产”或“非生产”）相匹配。" +
				        		"如果 POE 将程序指定为“非生产”，那么应选择“非生产”。" +
				        		"如果在 POE 上未进行指定，那么认为您的用途为“生产”，并且应选择“生产”。" +
				        		"如果您没有权利证明，那么应选择“开发人员”。",
				        content: "更改服务器的许可用途时，该服务器将重新启动。" +
				        		"您将从用户界面注销，并且用户界面将不可用，直至服务器重新启动。" +
				        		"重新启动服务器可能需要几分钟时间。" +
				        		"服务器重新启动后，必须登录到 Web UI，并接受新的许可证。",
				        saveButton: "保存并重新启动服务器",
				        cancelButton: "取消"
				    },
					restartDialog: {
						title: "重新启动服务器",
						content: "确定要重新启动此服务器吗？" +
								"您将从用户界面注销，并且用户界面将不可用，直至服务器重新启动。" +
								"重新启动服务器可能需要几分钟时间。",
						okButton: "重新启动",
						cancelButton: "取消"												
					},
					shutdownDialog: {
						title: "关闭服务器",
						content: "确定要关闭此服务器吗？" +
								 "您将从用户界面注销，并且此用户界面将不可用，直至服务器重新打开。" +
								 "要重新打开服务器，请按服务器上的电源按钮。",
						okButton: "关闭",
						cancelButton: "取消"												
					},

					hostnameDialog: {
						title: "编辑服务器名称",
						instruction: "指定最多包含 1024 个字符的服务器名称。",
						invalid: "输入的值无效。服务器名称可包含有效的 UTF-8 字符。",
						tooltip: "输入服务器名称。服务器名称可包含有效的 UTF-8 字符。",
						hostname: "服务器名称",
						okButton: "保存",
						cancelButton: "取消"												
					}
// TRNOTE Do not translate strings in sshHostDialog
//					sshHostDialog: {
//						title: "Edit SSH Host",
//						// TRNOTE Do not translate ALL. It is a keyword
//						instruction: "Specify the host IP address that IBM IoT MessageSight can use for SSH connections. " +
//								     "To specify multiple addresses, use a comma-separated list. " +
//								     "To connect over SSH to any IP address that is configured on the IBM IoT MessageSight appliance, enter a value of ALL. "+
//						             "To disable SSH connections, enter a value of 127.0.0.1.",
//						sshHost: "SSH Host IP Address",
//                        // TRNOTE Do not translate ALL. It is a keyword						
//						invalid: "The value entered is not valid. Acceptable values include ALL, an IP address or list of IP addresses separated by a comma.",
//						okButton: "Save",
//						cancelButton: "Cancel"												
//					}
				},
				messagingServer: {
					subTitle: "${IMA_PRODUCTNAME_FULL} 服务器",
					tagLine: "影响 ${IMA_PRODUCTNAME_SHORT} 服务器的设置和操作。",
					status: "状态",
					retrieveStatusError: "检索状态时发生错误。",
					stopLink: "停止服务器",
					forceStopLink: "强制停止服务器",
					startLink: "启动服务器",
					starting: "正在启动",
					unknown: "未知",
					cleanStore: "维护方式（正在清除存储）",
					startError: "启动 ${IMA_PRODUCTNAME_SHORT} 服务器时发生错误。",
					stopping: "正在停止",
					stopError: "停止 ${IMA_PRODUCTNAME_SHORT} 服务器时发生错误。",
					stopDialog: {
						title: "停止 ${IMA_PRODUCTNAME_FULL} 服务器",
						content: "确定要停止 ${IMA_PRODUCTNAME_SHORT} 服务器吗？",
						okButton: "关闭",
						cancelButton: "取消"						
					},
					forceStopDialog: {
						title: "强制停止 ${IMA_PRODUCTNAME_FULL} 服务器",
						content: "确定要强制停止 ${IMA_PRODUCTNAME_SHORT} 服务器吗？强制停止是最后的手段，因为可能导致消息丢失。",
						okButton: "强制停止",
						cancelButton: "取消"						
					},

					memory: "内存"
				},
				mqconnectivity: {
					subTitle: "MQ Connectivity 服务",
					tagLine: "影响 MQ Connectivity 服务的设置和操作。",
					status: "状态",
					retrieveStatusError: "检索状态时发生错误。",
					stopLink: "停止服务",
					startLink: "启动服务",
					starting: "正在启动",
					unknown: "未知",
					startError: "启动 MQ Connectivity 服务时发生错误。",
					stopping: "正在停止",
					stopError: "停止 MQ Connectivity 服务时发生错误。",
					started: "正在运行",
					stopped: "已停止",
					stopDialog: {
						title: "停止 MQ Connectivity 服务",
						content: "确定要停止 MQ Connectivity 服务吗？停止服务可能导致消息丢失。",
						okButton: "停止",
						cancelButton: "取消"						
					}
				},
				runMode: {
					runModeLabel: "运行方式",
					cleanStoreLabel: "清除存储",
				    runModeTooltip: "指定 ${IMA_PRODUCTNAME_SHORT} 服务器的运行方式。选中并确认新的运行方式后，新的运行方式在 ${IMA_PRODUCTNAME_SHORT} 服务器重新启动后才会生效。",
				    runModeTooltipDisabled: "由于 ${IMA_PRODUCTNAME_SHORT} 服务器的状态，因此无法更改运行方式。",
				    cleanStoreTooltip: "指示是否想在 ${IMA_PRODUCTNAME_SHORT} 服务器重新启动时执行清除存储操作。仅当 ${IMA_PRODUCTNAME_SHORT} 服务器处于维护方式时才能选择清除存储操作。",
					// TRNOTE {0} is a mode, such as production or maintenance.
				    setRunModeError: "尝试将 ${IMA_PRODUCTNAME_SHORT} 服务器的运行方式更改为“{0}”时发生错误。",
					retrieveRunModeError: "检索 ${IMA_PRODUCTNAME_SHORT} 服务器的运行方式时发生错误。",
					runModeDialog: {
						title: "确认运行方式更改",
						// TRNOTE {0} is a mode, such as production or maintenance.
						content: "确定要将 ${IMA_PRODUCTNAME_SHORT} 服务器的运行方式更改为“<strong>{0}</strong>”吗？选择新的运行方式后，必须先重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器，然后新的运行方式才会生效。",
						okButton: "确认",
						cancelButton: "取消"												
					},
					cleanStoreDialog: {
						title: "确认清除存储操作更改",
						contentChecked: "确定要在服务器重新启动期间执行<strong>清除存储</strong>操作吗？" +
								 "如果设置了<strong>清除存储</strong>操作，那么必须重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器才能执行此操作。" +
						         "执行<strong>清除存储</strong>操作将导致持久存储的消息传递数据丢失。<br/><br/>",
						content: "确定要取消<strong>清除存储</strong>操作吗？",
						okButton: "确认",
						cancelButton: "取消"												
					}
				},
				storeMode: {
					storeModeLabel: "存储方式",
				    storeModeTooltip: "指定 ${IMA_PRODUCTNAME_SHORT} 服务器的存储方式。选中并确认新的存储方式后，${IMA_PRODUCTNAME_SHORT} 服务器将会重新启动并且将执行清除存储操作。",
				    storeModeTooltipDisabled: "由于 ${IMA_PRODUCTNAME_SHORT} 服务器的状态，因此无法更改存储方式。",
				    // TRNOTE {0} is a mode, such as production or maintenance.
				    setStoreModeError: "尝试将 ${IMA_PRODUCTNAME_SHORT} 服务器的存储方式更改为“{0}”时发生错误。",
					retrieveStoreModeError: "检索 ${IMA_PRODUCTNAME_SHORT} 服务器的存储方式时发生错误。",
					storeModeDialog: {
						title: "确认存储方式更改",
						// TRNOTE {0} is a mode, such as persistent or memory only.
						content: "确定要将 ${IMA_PRODUCTNAME_SHORT} 服务器的存储方式更改为“<strong>{0}</strong>”吗？" + 
								"如果更改了存储方式，那么 ${IMA_PRODUCTNAME_SHORT} 服务器将会重新启动并且将执行清除存储操作。" +
								"执行清除存储操作将导致持久存储的消息传递数据丢失。更改存储方式时，必须执行清除存储操作。",
						okButton: "确认",
						cancelButton: "取消"												
					}
				},
				logLevel: {
					subTitle: "设置日志级别",
					tagLine: "设置想要在服务器日志中看到的消息级别。" +
							 "这些日志可通过<a href='monitoring.jsp?nav=downloadLogs'>监视 > 下载日志</a>下载。",
					logLevel: "缺省日志",
					connectionLog: "连接日志",
					securityLog: "安全日志",
					adminLog: "管理日志",
					mqconnectivityLog: "MQ Connectivity 日志",
					tooltip: "日志级别控制缺省日志文件中的消息类型。" +
							 "该日志文件显示了有关服务器操作的所有日志条目。" +
							 "其他日志中显示的高严重性条目也记录在该日志中。" +
							 "最低设置仅包含最重要的条目。" +
							 "最高设置将包含全部条目。" +
							 "请参阅文档以获取有关日志级别的更多信息。",
					connectionTooltip: "日志级别控制连接日志文件中的消息类型。" +
							           "该日志文件显示了与连接相关的日志条目。" +
							           "这些条目可用作连接的审计日志，还可指示连接错误。" +
							           "最低设置仅包含最重要的条目。最高设置将包含全部条目。" +
					                   "请参阅文档以获取有关日志级别的更多信息。",
					securityTooltip: "日志级别控制安全日志文件中的消息类型。" +
							         "该日志文件显示了与认证和授权相关的日志条目。" +
							         "该日志可用作安全相关项的审计日志。" +
							         "最低设置仅包含最重要的条目。最高设置将包含全部条目。" +
					                 "请参阅文档以获取有关日志级别的更多信息。",
					adminTooltip: "日志级别控制管理日志文件中的消息类型。" +
							      "此日志文件显示与从 ${IMA_PRODUCTNAME_FULL} Web UI 或命令行运行管理命令相关的日志条目。" +
							      "它记录了对服务器配置所做的所有更改以及尝试进行的更改。" +
								  "最低设置仅包含最重要的条目。" +
								  "最高设置将包含全部条目。" +
					              "请参阅文档以获取有关日志级别的更多信息。",
					mqconnectivityTooltip: "日志级别控制 MQ Connectivity 日志文件中的消息类型。" +
							               "该日志文件显示与 MQ Connectivity 功能相关的日志条目。" +
							               "这些条目包含与队列管理器连接时发现的问题。" +
							               "该日志有助于诊断将 ${IMA_PRODUCTNAME_FULL} 连接到 WebSphere<sup>&reg;</sup> MQ 时出现的问题。" +
										   "最低设置仅包含最重要的条目。" +
										   "最高设置将包含全部条目。" +
										   "请参阅文档以获取有关日志级别的更多信息。",										
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log										   
					retrieveLogLevelError: "检索 {0} 级别时发生错误。",
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log
					setLogLevelError: "设置 {0} 级别时发生错误。",
					levels: {
						MIN: "最低",
						NORMAL: "常规",
						MAX: "最大值"
					},
					serverNotAvailable: "在 ${IMA_PRODUCTNAME_FULL} 服务器停止时，不能设置此日志记录级别。",
					serverCleanStore: "在正在执行清除存储时，不能设置此日志记录级别。",
					serverSyncState: "在主节点正在同步时，不能设置此日志记录级别。",
					mqNotAvailable: "在 MQ Connectivity 服务停止时，不能设置 MQ Connectivity 日志级别。",
					mqStandbyState: "当服务器处于待机状态时，无法设置 MQ Connectivity 日志级别。"
				}
			},
			highAvailability: {
				title: "高可用性",
				tagline: "配置两个 ${IMA_PRODUCTNAME_SHORT} 服务器，使其充当高可用性节点对。",
				stateLabel: "当前高可用性状态：",
				statePrimary: "主",
				stateStandby: "就绪",
				stateIndeterminate: "未确定",
				error: "高可用性遇到错误",
				save: "保存",
				saving: "正在保存...",
				success: "保存成功。必须重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器和 MQ Connectivity 服务才能使更改生效。",
				saveFailed: "更新高可用性设置时发生错误。",
				ipAddrValidation : "指定 IPv4 地址或 IPv6 地址，例如 192.168.1.0",
				discoverySameAsReplicationError: "发现地址必须不同于复制地址。",
				AutoDetect: "自动检测",
				StandAlone: "独立",
				notSet: "未设置",
				yes: "是",
				no: "否",
				config: {
					title: "配置",
					tagline: "显示高可用性配置。" +
					         "重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器后，对配置的更改才会生效。",
					editLink: "编辑",
					enabledLabel: "已启用高可用性",
					haGroupLabel: "高可用性组：",
					startupModeLabel: "启动方式",
					replicationInterfaceLabel: "复制地址：",
					discoveryInterfaceLabel: "发现地址：",
					remoteDiscoveryInterfaceLabel: "远程发现地址：",
					discoveryTimeoutLabel: "发现超时：",
					heartbeatLabel: "脉动信号超时：",
					seconds: "{0} 秒",
					secondsDefault: "{0} 秒（缺省值）",
					preferedPrimary: "自动检测（首选主节点）"
				},
				restartInfoDialog: {
					title: "需要重新启动",
					content: "您的更改已保存，但是在重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器后更改才会生效。<br /><br />" +
							 "要现在重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器，请单击<b>立即重新启动</b>。否则单击<b>稍后重新启动</b>。",
					closeButton: "稍后重新启动",
					restartButton: "立即重新启动",
					stopping: "正在停止 ${IMA_PRODUCTNAME_SHORT} 服务器...",
					stoppingFailed: "停止 ${IMA_PRODUCTNAME_SHORT} 服务器时发生错误。" +
								"使用“服务器控制”页面来重新启动 I${IMA_PRODUCTNAME_SHORT} 服务器。",
					starting: "正在启动 ${IMA_PRODUCTNAME_SHORT} 服务器..."
//					startingFailed: "An error occurred while starting the IBM IoT MessageSight server. " +
//								"Use the System Control page to start the IBM IoT MessageSight server."
				},
				confirmGroupNameDialog: {
					title: "确认组名更改",
					content: "当前只有一个节点处于活动状态。如果组名已更改，您可能需要手动编辑其他节点的组名。" +
					         "确定要更改组名吗？",
					closeButton: "取消",
					confirmButton: "确认"
				},
				dialog: {
					title: "编辑高可用性配置",
					saveButton: "保存",
					cancelButton: "取消",
					instruction: "启用高可用性并设置高可用性组，以确定该服务器与哪个服务器配对。" + 
								 "或者，禁用高可用性。重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器之后，高可用性配置更改才会生效。",
					haEnabledLabel: "已启用高可用性：",
					startupMode: "启动方式",
					haEnabledTooltip: "工具提示文本",
					groupNameLabel: "高可用性组：",
					groupNameToolTip: "组用于自动配置服务器以互相组对。" +
					                  "该值在两个服务器上必须相同。" +
					                  "该值的最大长度为 128 个字符。",
					advancedSettings: "高级设置",
					advSettingsTagline: "如果希望为高可用性配置进行特定设置，可以手动配置以下设置。" +
										"在更改这些设置之前，请参阅 ${IMA_PRODUCTNAME_FULL} 文档，以获取详细的指示信息和建议。",
					nodePreferred: "两个节点都以自动检测方式启动时，该节点是首选主节点。",
				    localReplicationInterface: "本地复制地址：",
				    localDiscoveryInterface: "本地发现地址：",
				    remoteDiscoveryInterface: "远程发现地址：",
				    discoveryTimeout: "发现超时：",
				    heartbeatTimeout: "脉动信号超时：",
				    startModeTooltip: "节点启动时所处的方式。节点可设置为自动检测方式或独立方式。" +
				    				  "在自动检测方式中，必须启动两个节点。" +
				    				  "节点会自动尝试检测另一个节点，并建立 HA 对。" +
				    				  "仅当启动单个节点时使用独立方式。",
				    localReplicationInterfaceTooltip: "用于 HA 对中本地节点上 HA 复制的 NIC 的 IP 地址。" +
				    								  "您可以选择想要分配的 IP 地址。" +
				    								  "但是，本地复制地址必须与本地发现地址位于不同的子网上。",
				    localDiscoveryInterfaceTooltip: "用于 HA 对中本地节点上 HA 发现的 NIC 的 IP 地址。" +
				    							    "您可以选择想要分配的 IP 地址。" +
				    								"但是，本地发现地址必须与本地复制地址位于不同的子网上。",
				    remoteDiscoveryInterfaceTooltip: "用于 HA 对中远程节点上 HA 发现的 NIC 的 IP 地址。" +
				    								 "您可以选择想要分配的 IP 地址。" +
				    								 "但是，远程发现地址必须与本地复制地址位于不同的子网上。",
				    discoveryTimeoutTooltip: "时间（以秒计），在这段时间内，以自动检测方式启动的服务器必须连接到高可用性对中的另一个服务器。" +
				    					     "有效范围为 10 - 2147483647。缺省值为 600。" +
				    						 "如果未在该时间段内完成连接，服务器会以维护方式启动。",
				    heartbeatTimeoutTooltip: "时间（以秒计），在这段时间内，服务器必须确定高可用性对中的另一个服务器是否发生故障。" + 
				    						 "有效范围为 1 - 2147483647。缺省值为 10。" +
				    						 "如果主服务器未在该时间段内收到来自备用服务器的脉动信号，它会继续充当主服务器，但是数据同步过程将停止。" +
				    						 "如果备用服务器未在该时间段内收到来自主服务器的脉动信号，那么备用服务器将成为主服务器。",
				    seconds: "秒",
				    nicsInvalid: "必须为本地复制地址、本地发现地址和远程发现地址都提供有效值。" +
				    			 "或者，也可以不为本地复制地址、本地发现地址和远程发现地址提供任何值。",
				    securityRiskTitle: "安全风险",
				    securityRiskText: "确定要在不指定复制地址和发现地址的情况下启用高可用性吗？" +
				    		"如果您不具有对服务器网络环境的完全控制权，那么将多点广播与您的服务器相关的信息，并且您的服务器可能与使用相同高可用性组名的不可信服务器配对。" +
				    		"<br /><br />" +
				    		"您可以通过指定复制地址和发现地址来消除这种安全风险。"
				},
				status: {
					title: "状态",
					tagline: "显示高可用性状态。" +
					         "如果状态与配置不匹配，那么可能需要重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器，才能使最近所做的配置更改生效。",
					editLink: "编辑",
					haLabel: "高可用性：",
					haGroupLabel: "高可用性组：",
					haNodesLabel: "活动节点",
					pairedWithLabel: "最后配对：",
					replicationInterfaceLabel: "复制地址：",
					discoveryInterfaceLabel: "发现地址：",
					remoteDiscoveryInterfaceLabel: "远程发现地址：",
					remoteWebUI: "<a href='{0}' target=_'blank'>远程 Web UI</a>",
					enabled: "已启用高可用性。",
					disabled: "已禁用高可用性。"
				},
				startup: {
					subtitle: "节点启动方式",
					tagline: "节点可设置为自动检测或独立方式。在自动检测方式中，必须启动两个节点。" +
							 "节点将自动尝试检测另一个节点，并建立高可用性对。仅当启动单个节点时应使用独立方式。",
					mode: "启动方式",
					modeAuto: "自动检测",
					modeAlone: "独立",
					primary: "两个节点都以自动检测方式启动时，该节点是首选主节点。"
				},
				remote: {
					subtitle: "远程节点发现地址",
					tagline: "高可用性对中远程节点上发现地址的 IPv4 地址或 IPv6 地址。",
					discoveryInt: "发现地址：",
					discoveryIntTooltip: "输入 IPv4 地址或 IPv6 地址以配置远程节点发现地址。"
				},
				local: {
					subtitle: "高可用性 NIC 地址",
					tagline: "本地节点的复制地址和发现地址的 IPv4 地址或 IPv6 地址。",
					replicationInt: "复制地址：",
					replicationIntTooltip: "输入 IPv4 地址或 IPv6 地址以配置本地节点的复制地址。",
					replicationError: "高可用性复制地址不可用",
					discoveryInt: "发现地址：",
					discoveryIntTooltip: "输入 IPv4 地址或 IPv6 地址以配置本地节点的发现地址。"
				},
				timeouts: {
					subtitle: "超时",
					tagline: "当已启用高可用性的节点以自动检测方式启动时，会尝试与高可用性对中的另一个节点进行通信，" +
							 "如果在发现超时时间段内无法建立连接，那么节点将改为以维护方式启动。",
					tagline2: "脉动信号超时用于检测高可用性对中的另一个节点是否已发生故障。" +
							  "如果主节点未收到来自备用节点的脉动信号，它会继续充当主节点，但是数据同步过程将停止。" +
							  "如果备用节点未收到来自主节点的脉动信号，那么备用节点成为主节点。",
					discoveryTimeout: "发现超时：",
					discoveryTimeoutTooltip: "节点尝试连接到对中另一个节点的秒数，范围为 10 到 2,147,483,647。",
					discoveryTimeoutValidation: "请输入 10 到 2,147,483,647 之间的数字。",
					discoveryTimeoutError: "发现超时不可用",
					heartbeatTimeout: "脉动信号超时：",
					heartbeatTimeoutTooltip: "节点脉动信号的秒数，范围为 1 到 2,147,483,647。",
					heartbeatTimeoutValidation: "请输入 1 到 2,147,483,647 之间的数字。",
					seconds: "秒"
				}
			},
			webUISecurity: {
				title: "Web UI 设置",
				tagline: "配置 Web UI 的设置。",
				certificate: {
					subtitle: "Web UI 证书",
					tagline: "配置用于保护与 Web UI 通信安全的证书。"
				},
				timeout: {
					subtitle: "会话超时与 LTPA 令牌到期",
					tagline: "这些设置根据浏览器和服务器之间的活动以及登录总时间，来控制用户登录会话。" +
							 "更改会话不活动超时或 LTPA 令牌到期可能使任何活动登录会话失效。",
					inactivity: "会话不活动超时",
					inactivityTooltip: "会话不活动超时将控制当浏览器和服务器之间没有任何活动时用户可保持登录状态的时长。" +
							           "不活动超时必须小于或等于 LTPA 令牌到期。",
					expiration: "LTPA 令牌到期",
					expirationTooltip: "轻量级第三方认证令牌到期控制用户可保持登录状态的时长，无论浏览器和服务器之间的活动如何。" +
							           "到期必须大于或等于会话不活动超时。有关更多信息，请参阅文档。", 
					unit: "分钟",
					save: "保存",
					saving: "正在保存...",
					success: "保存成功。请注销，然后重新登录。",
					failed: "保存失败",
					invalid: {
						inactivity: "会话不活动超时必须是 1 到 LTPA 令牌到期值之间的数字。",
						expiration: "LTPA 令牌到期值必须是会话不活动超时值到 1440 之间的数字。"
					},
					getExpirationError: "获取 LTPA 令牌到期时发生错误。",
					getInactivityError: "获取会话不活动超时时发生错误。",
					setExpirationError: "设置 LTPA 令牌到期时发生错误。",
					setInactivityError: "设置会话不活动超时时发生错误。"
				},
				cacheauth: {
					subtitle: "认证高速缓存超时",
					tagline: "认证高速缓存超时指定高速缓存中已认证凭证的有效期限。" +
					"较大的值可能增加安全风险，因为撤销的凭证会在高速缓存中保留较长时间。" +
					"较小的值可能影响性能，因为必须更频繁地访问用户注册表。",
					cacheauth: "认证高速缓存超时",
					cacheauthTooltip: "认证高速缓存超时确定移除高速缓存中条目之前的时间量。" +
							"该值范围为 1 到 3600 秒（1 小时）。该值不应超过 LTPA 令牌到期值的 1/3。" +
							"请参阅文档以获取更多信息。", 
					unit: "秒",
					save: "保存",
					saving: "正在保存...",
					success: "保存成功",
					failed: "保存失败",
					invalid: {
						cacheauth: "认证高速缓存超时值必须是 1 到 3600 之间的数字。"
					},
					getCacheauthError: "获取认证高速缓存超时时发生错误。",
					setCacheauthError: "设置认证高速缓存超时时发生错误。"
				},
				readTimeout: {
					subtitle: "HTTP 读超时",
					tagline: "在发生第一次读之后等待套接字上的读请求完成的最大时间量。",
					readTimeout: "HTTP 读超时",
					readTimeoutTooltip: "该值必须在 1 到 300 秒（5 分钟）之间。", 
					unit: "秒",
					save: "保存",
					saving: "正在保存...",
					success: "保存成功",
					failed: "保存失败",
					invalid: {
						readTimeout: "HTTP 读超时值必须是 1 到 300 之间的数字。"
					},
					getReadTimeoutError: "获取 HTTP 读超时时发生错误。",
					setReadTimeoutError: "设置 HTTP 读超时时发生错误。"
				},
				port: {
					subtitle: "IP 地址和端口",
					tagline: "为 Web UI 通信选择 IP 地址和端口。更改任一值可能需要您使用新的 IP 地址和端口进行登录。" +
							 "<em>不建议选择“全部”IP 地址，因为这样可能导致将 Web UI 公开到互联网。</em>",
					ipAddress: "IP 地址",
					all: "全部",
					interfaceHint: "接口：{0}",
					port: "端口",
					save: "保存",
					saving: "正在保存...",
					success: "保存成功。更改生效可能需要几分钟时间。请关闭浏览器然后重新登录。",
					failed: "保存失败",
					tooltip: {
						port: "请输入可用端口。有效的端口在 1 和 8999、9087 或 9100 和 65535 之间（包括这些值）。"
					},
					invalid: {
						interfaceDown: "接口 {0} 报告已关闭。请检查该接口或选择其他地址。",
						interfaceUnknownState: "查询 {0} 的状态时发生错误。请检查该接口或选择其他地址。",
						port: "端口号必须在 1 和 8999、9087 或 9100 和 65535 之间（包括这些值）。"
					},					
					getIPAddressError: "获取 IP 地址时发生错误。",
					getPortError: "获取端口时发生错误。",
					setIPAddressError: "设置 IP 地址时发生错误。",
					setPortError: "设置端口时发生错误。"					
				},
				certificate: {
					subtitle: "安全证书",
					tagline: "上载并替换 Web UI 使用的证书",
					addButton: "添加证书",
					editButton: "编辑证书",
					addTitle: "替换 Web UI 证书",
					editTitle: "编辑 Web UI 证书",
					certificateLabel: "证书：",
					keyLabel: "专用密钥：",
					expiryLabel: "证书期限：",
					defaultExpiryDate: "警告！缺省证书正在使用，应进行替换",
					defaultCert: "缺省",
					success: "更改生效可能需要几分钟时间。请关闭浏览器然后重新登录。"
				},
				service: {
					subtitle: "服务",
					tagline: "设置跟踪级别以生成针对 Web UI 的支持信息。",
					traceEnabled: "启用跟踪",
					traceSpecificationLabel: "跟踪规范",
					traceSpecificationTooltip: "IBM 支持人员提供的跟踪规范字符串",
				    save: "保存",
				    getTraceError: "获取跟踪规范时发生错误。",
				    setTraceError: "设置跟踪规范时发生错误。",
				    clearTraceError: "清除跟踪规范时发生错误。",
					saving: "正在保存...",
					success: "保存成功。",
					failed: "保存失败"
				}
			}
		},
        platformDataRetrieveError: "检索平台数据时出错。",
        // Strings for client certs that can be used for certificate authentication
		clientCertsTitle: "客户机证书",
		clientCertsButton: "客户机证书",
        stopHover: "关闭服务器，并阻止对服务器的所有访问。",
        restartLink: "重新启动服务器",
		restartMaintOnLink: "启动维护",
		restartMaintOnHover: "重新启动服务器并以维护方式运行。",
		restartMaintOffLink: "停止维护",
		restartMaintOffHover: "重新启动服务器并以生产方式运行。",
		restartCleanStoreLink: "清除存储",
		restartCleanStoreHover: "清除服务器存储，然后重新启动服务器。",
        restartDialogTitle: "重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器",
        restartMaintOnDialogTitle: "以维护方式重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器",
        restartMaintOffDialogTitle: "以生产方式重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器",
        restartCleanStoreDialogTitle: "清除 ${IMA_PRODUCTNAME_SHORT} 服务器存储器",
        restartDialogContent: "确定要重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器吗？重新启动服务器会中断所有消息传递。",
        restartMaintOnDialogContent: "确定要以维护方式重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器吗？以维护方式启动服务器会停止所有消息传递。",
        restartMaintOffDialogContent: "确定要以生产方式重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器吗？以生产方式启动服务器会重新启动消息传递。",
        restartCleanStoreDialogContent: "执行<strong>清除存储</strong>操作将导致持久存储的消息传递数据丢失。<br/><br/>" +
        	"确定要执行<strong>清除存储</strong>操作吗？",
        restartOkButton: "重新启动",        
        cleanOkButton: "清除存储并重新启动",
        restarting: "正在重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器...",
        restartingFailed: "重新启动 ${IMA_PRODUCTNAME_SHORT} 服务器时发生错误。",
        remoteLogLevelTagLine: "设置想要在服务器日志中看到的消息级别。", 
        errorRetrievingTrustedCertificates: "检索可信证书时发生错误。",

        // New page title and tagline:  Admin Endpoint
        adminEndpointPageTitle: "管理端点配置",
        adminEndpointPageTagline: "配置管理端点并创建配置策略。" +
        		"Web UI 可连接到管理端点，以执行管理任务和监视任务。" +
        		"配置策略可控制允许用户执行的管理任务和监视任务。",
        // HA config strings
        haReplicationPortLabel: "复制端口：",
		haExternalReplicationInterfaceLabel: "外部复制地址：",
		haExternalReplicationPortLabel: "外部复制端口：",
        haRemoteDiscoveryPortLabel: "远程发现端口：",
        haReplicationPortTooltip: "用于 HA 对中本地节点上 HA 复制的端口号。",
		haReplicationPortInvalid: "复制端口无效。" +
				"有效范围为 1024 - 65535。缺省值为 0，允许在连接时随机选择可用端口。",
		haExternalReplicationInterfaceTooltip: "应用于 HA 复制的 NIC 的 IPv4 或 IPv6 地址。",
		haExternalReplicationPortTooltip: "用于 HA 对中本地节点上 HA 复制的外部端口号。",
		haExternalReplicationPortInvalid: "外部复制端口无效。" +
				"有效范围为 1024 - 65535。缺省值为 0，允许在连接时随机选择可用端口。",
        haRemoteDiscoveryPortTooltip: "用于 HA 对中远程节点上 HA 发现的端口号。",
		haRemoteDiscoveryPortInvalid: "远程发现端口无效。" +
				 "有效范围为 1024 - 65535。缺省值为 0，允许在连接时随机选择可用端口。",
		haReplicationAndDiscoveryBold: "复制和发现地址",
		haReplicationAndDiscoveryNote: "（如果设置了其中一个，那么必须全部设置）",
		haUseSecuredConnectionsLabel: "使用安全连接：",
		haUseSecuredConnectionsTooltip: "对 HA 对的节点间的发现和数据通道使用 TLS。",
		haUseSecuredConnectionsEnabled: "启用安全连接",
		haUseSecuredConnectionsDisabled: "禁用安全连接",
		mqConnectivityEnableLabel: "启用 MQ Connectivity",
		savingProgress: "正在保存...",
	    setMqConnEnabledError: "设置 MQConnectivity 已启用状态时发生错误。",
	    allowCertfileOverwriteLabel: "覆盖",
		allowCertfileOverwriteTooltip: "允许覆盖证书和专用密钥。",
		stopServerWarningTitle: "停止操作会阻止从 Web UI 重新启动！",
		stopServerWarning: "停止服务器会通过 REST 请求和 Web UI 阻止所有消息传递和所有管理访问。<b>为避免丧失从 Web UI 访问服务器的权限，请使用“重新启动服务器”链接。</b>",
		// New Web UI user dialog strings to allow French translation to insert a space before colon
		webuiuserLabel: "用户标识：",
		webuiuserdescLabel: "描述：",
		webuiuserroleLabel: "角色："

});
