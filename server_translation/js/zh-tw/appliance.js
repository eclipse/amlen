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
				title: "Web UI 使用者管理",
				tagline: "將 Web UI 的存取權提供給使用者。",
				usersSubTitle: "Web UI 使用者",
				webUiUsersTagline: "系統管理者可以定義、編輯或刪除 Web UI 使用者。",
				itemName: "使用者",
				itemTooltip: "",
				addDialogTitle: "新增使用者",
				editDialogTitle: "編輯使用者",
				removeDialogTitle: "刪除使用者",
				resetPasswordDialogTitle: "重設密碼",
				// TRNOTE: Do not translate addGroupDialogTitle, editGroupDialogTitle, removeGroupDialogTitle
				//addGroupDialogTitle: "Add Group",
				//editGroupDialogTitle: "Edit Group",
				//removeGroupDialogTitle: "Delete Group",
				grid: {
					userid: "使用者 ID",
					// TRNOTE: Do not translate groupid
					//groupid: "Group ID",
					description: "說明",
					groups: "角色",
					noEntryMessage: "無項目可顯示",
					// TRNOTE: Do not translate ldapEnabledMessage
					//ldapEnabledMessage: "Messaging users and groups configured on the appliance are disabled because an external LDAP connection is enabled.",
					refreshingLabel: "更新中..."
				},
				dialog: {
					useridTooltip: "輸入使用者 ID，不得有前導空格或尾端空格。",
					applianceUserIdTooltip: "輸入使用者 ID，它必須僅包含字母 A-Za-z、數字 0-9 以及四個特殊字元 - _ . 及 +",					
					useridInvalid: "使用者 ID 不得有前導空格或尾端空格。",
					applianceInvalid: "使用者 ID 必須僅包含字母 A-Za-z、數字 0-9 以及四個特殊字元 - _ . 及 +",					
					// TRNOTE: Do not translate groupidTooltip, groupidInvalid
					//groupidTooltip: "Enter the Group ID, which must not have leading or trailing spaces.",
					//groupidInvalid: "The Group ID must not have leading or trailing spaces.",
					SystemAdministrators: "系統管理者",
					MessagingAdministrators: "傳訊管理者",
					Users: "使用者",
					applianceUserInstruction: "「系統管理者」可以建立、更新或刪除其他 Web UI 管理者與 Web UI 使用者。",
					// TRNOTE: Do not translate messagingUserInstruction, messagingGroupInstruction
					//messagingUserInstruction: "Messaging users can be added to messaging policies to control the messaging resources that a user can access.",
					//messagingGroupInstruction: "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.",				                    
					password1: "密碼：",
					password2: "確認密碼：",
					password2Invalid: "密碼不符。",
					passwordNoSpaces: "密碼不得有前導空格或尾端空格。",
					saveButton: "儲存",
					cancelButton: "取消",
					refreshingGroupsLabel: "正在擷取群組..."
				},
				returnCodes: {
					CWLNA5056: "部分設定無法儲存"
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
				title: "刪除項目",
				content: "您確定要刪除此記錄嗎？",
				deleteButton: "刪除",
				cancelButton: "取消"
			},
			savingProgress: "儲存中…",
			savingFailed: "儲存失敗。",
			deletingProgress: "刪除中...",
			deletingFailed: "刪除失敗。",
			testingProgress: "測試中...",
			uploadingProgress: "上傳中...",
			dialog: { 
				saveButton: "儲存",
				cancelButton: "取消",
				testButton: "測試連線"
			},
			invalidName: "已存在具有該名稱的記錄。",
			invalidChars: "需要有效的主機名稱或 IP 位址。",
			invalidRequired: "需要輸入值。",
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
				nothingToTest: "必須提供 IPv4 或 IPv6 位址，才能測試連線。",
				testConnFailed: "測試連線失敗",
				testConnSuccess: "測試連線成功",
				testFailed: "測試失敗",
				testSuccess: "測試成功"
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
				title: "安全設定",
				tagline: "匯入金鑰及憑證，以保護伺服器連線的安全。",
				securityProfilesSubTitle: "安全設定檔",
				securityProfilesTagline: "系統管理者可以定義、編輯或刪除識別用於驗證用戶端之方法的安全設定檔。",
				certificateProfilesSubTitle: "憑證設定檔",
				certificateProfilesTagline: "系統管理者可以定義、編輯或刪除驗證使用者身分的憑證設定檔。",
				ltpaProfilesSubTitle: "LTPA 設定檔",
				ltpaProfilesTagline: "系統管理者可以定義、編輯或刪除小型認證機構 (LTPA) 設定檔，以便能夠在多部伺服器之間使用單一登入。",
				oAuthProfilesSubTitle: "OAuth 設定檔",
				oAuthProfilesTagline: "系統管理者可以定義、編輯或刪除 OAuth 設定檔，以便能夠在多部伺服器之間使用單一登入。",
				systemWideSubTitle: "全伺服器的安全設定",
				useFips: "將 FIPS 140-2 設定檔用於安全傳訊通訊",
				useFipsTooltip: "選取此選項可使用符合使用安全設定檔保證端點安全之聯邦資訊存取安全標準 (FIPS) 140-2 的加密法。" +
				                "變更此設定需要重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器。",
				retrieveFipsError: "在擷取 FIPS 設定時發生錯誤。",
				fipsDialog: {
					title: "設定 FIPS",
					content: "必須重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器，才能使此變更生效。您確定要變更此設定嗎？"	,
					okButton: "設定並重新啟動",
					cancelButton: "取消變更",
					failed: "在變更 FIPS 設定時發生錯誤。",
					stopping: "正在停止 ${IMA_PRODUCTNAME_SHORT} 伺服器...",
					stoppingFailed: "停止 ${IMA_PRODUCTNAME_SHORT} 伺服器時發生錯誤。",
					starting: "正在啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器...",
					startingFailed: "啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器時發生錯誤。"
				}
			},
			securityProfiles: {
				grid: {
					name: "名稱",
					mprotocol: "最低通訊協定方法",
					ciphers: "密碼",
					certauth: "使用用戶端憑證",
					clientciphers: "使用用戶端的密碼",
					passwordAuth: "使用密碼鑑別",
					certprofile: "憑證設定檔",
					ltpaprofile: "LTPA 設定檔",
					oAuthProfile: "OAuth 設定檔",
					clientCertsButton: "授信憑證"
				},
				trustedCertMissing: "授信憑證遺失",
				retrieveError: "在擷取安全設定檔時發生錯誤。",
				addDialogTitle: "新增安全設定檔",
				editDialogTitle: "編輯安全設定檔",
				removeDialogTitle: "刪除安全設定檔",
				dialog: {
					nameTooltip: "名稱由最多 32 個英數字元組成。第一個字元不得是數字。",
					mprotocolTooltip: "在連接時允許的通訊協定方法最低層次。",
					ciphersTooltip: "<strong>最好：&nbsp;</strong>選取伺服器及用戶端支援的最安全密碼。<br />" +
							        "<strong>快：&nbsp;</strong>選取伺服器及用戶端支援的最快高安全密碼。<br />" +
							        "<strong>中：&nbsp;</strong>選取用戶端支援的最快中或高安全密碼。",
				    ciphers: {
				    	best: "最好",
				    	fast: "快",
				    	medium: "中"
				    },
					certauthTooltip: "選擇是否鑑別用戶端憑證。",
					clientciphersTooltip: "選擇在連接時是否容許用戶端判定要使用的密碼組合。",
					passwordAuthTooltip: "選擇在連接時是否需要有效的使用者 ID 及密碼。",
					passwordAuthTooltipDisabled: "無法修改，因為選取了 LTPA 或 OAuth 設定檔。若要修改，請將 LTPA 或 OAuth 設定檔變更為「<em>選擇一個</em>」",
					certprofileTooltip: "選取現有的憑證設定檔與此安全設定檔關聯。",
					ltpaprofileTooltip: "選取現有的 LTPA 設定檔與此安全設定檔關聯。" +
					                    "如果選取 LTPA 設定檔，則還必須選取「使用密碼鑑別」。",
					ltpaprofileTooltipDisabled: "無法選取 LTPA 設定檔，因為選取了 OAuth 設定檔，或未選取「使用密碼鑑別」。",
					oAuthProfileTooltip: "選取現有的 OAuth 設定檔與此安全設定檔關聯。" +
                    					  "如果選取 OAuth 設定檔，則還必須選取「使用密碼鑑別」。",
                    oAuthProfileTooltipDisabled: "無法選取 OAuth 設定檔，因為選取了 LTPA 設定檔，或未選取「使用密碼鑑別」。",
					noCertProfilesTitle: "無憑證設定檔",
					noCertProfilesDetails: "選取「<em>使用 TLS</em>」時，只有在先定義憑證設定檔之後才能定義安全設定檔。",
					chooseOne: "選擇一個"
				},
				certsDialog: {
					title: "授信憑證",
					instruction: "針對安全設定檔，將憑證上傳至用戶端憑證信任儲存庫，或從中刪除憑證",
					securityProfile: "安全設定檔",
					certificate: "憑證",
					browse: "瀏覽",
					browseHint: "選取檔案...",
					upload: "上傳",
					close: "關閉",
					remove: "刪除",
					removeTitle: "刪除憑證 ",
					removeContent: "確定要刪除此憑證嗎？",
					retrieveError: "在擷取憑證時發生錯誤。",
					uploadError: "在上傳憑證檔時發生錯誤。",
					deleteError: "在刪除憑證檔時發生錯誤。"	
				}
			},
			certificateProfiles: {
				grid: {
					name: "名稱",
					certificate: "憑證",
					privkey: "私密金鑰",
					certexpiry: "憑證期限",
					noDate: "擷取日期時發生錯誤..."
				},
				retrieveError: "在擷取憑證設定檔時發生錯誤。",
				uploadCertError: "在上傳憑證檔時發生錯誤。",
				uploadKeyError: "在上傳金鑰檔時發生錯誤。",
				uploadTimeoutError: "在上傳憑證或金鑰檔時發生錯誤。",
				addDialogTitle: "新增憑證設定檔",
				editDialogTitle: "編輯憑證設定檔",
				removeDialogTitle: "刪除憑證設定檔",
				dialog: {
					certpassword: "憑證密碼：",
					keypassword: "金鑰密碼：",
					browse: "瀏覽...",
					certificate: "憑證：",
					certificateTooltip: "", // "Upload a certificate"
					privkey: "私密金鑰：",
					privkeyTooltip: "", // "Upload a private key"
					browseHint: "選取檔案...",
					duplicateFile: "已存在具有該名稱的檔案。",
					identicalFile: "憑證或金鑰不能是相同的檔案。",
					noFilesToUpload: "若要更新憑證設定檔，請至少選取一個要上傳的檔案。"
				}
			},
			ltpaProfiles: {
				addDialogTitle: "新增 LTPA 設定檔",
				editDialogTitle: "編輯 LTPA 設定檔",
				instruction: "在安全設定檔中使用 LTPA（小型認證機構）設定檔，以在多個伺服器之間啟用單一登入。",
				removeDialogTitle: "刪除 LTPA 設定檔",				
				keyFilename: "金鑰檔名",
				keyFilenameTooltip: "包含匯出之 LTPA 金鑰的檔案。",
				browse: "瀏覽...",
				browseHint: "選取檔案...",
				password: "密碼",
				saveButton: "儲存",
				cancelButton: "取消",
				retrieveError: "在擷取 LTPA 設定檔時發生錯誤。",
				duplicateFileError: "已存在具有該名稱的金鑰檔。",
				uploadError: "在上傳金鑰檔時發生錯誤。",
				noFilesToUpload: "若要更新 LTPA 設定檔，請選取要上傳的檔案。"
			},
			oAuthProfiles: {
				addDialogTitle: "新增 OAuth 設定檔",
				editDialogTitle: "編輯 OAuth 設定檔",
				instruction: "在安全設定檔中使用 OAuth 設定檔，以在多個伺服器之間啟用單一登入。",
				removeDialogTitle: "刪除 OAuth 設定檔",	
				resourceUrl: "資源 URL",
				resourceUrlTooltip: "用於驗證存取記號的授權伺服器 URL。URL 必須包括通訊協定。通訊協定可以是 HTTP 或 HTTPS。",
				keyFilename: "OAuth 伺服器憑證",
				keyFilenameTooltip: "包含用於連接至 OAuth 伺服器的 SSL 憑證之檔案的名稱。",
				browse: "瀏覽...",
				reset: "重設",
				browseHint: "選取檔案...",
				authKey: "授權金鑰",
				authKeyTooltip: "包含存取記號的金鑰名稱。預設名稱為 <em>access_token</em>。",
				userInfoUrl: "使用者資訊 URL",
				userInfoUrlTooltip: "用於擷取使用者資訊的授權伺服器 URL。URL 必須包括通訊協定。通訊協定可以是 HTTP 或 HTTPS。",
				userInfoKey: "使用者資訊金鑰",
				userInfoKeyTooltip: "用於擷取使用者資訊的金鑰名稱。",
				saveButton: "儲存",
				cancelButton: "取消",
				retrieveError: "在擷取 OAuth 設定檔時發生錯誤。",
				duplicateFileError: "已存在具有該名稱的金鑰檔。",
				uploadError: "在上傳金鑰檔時發生錯誤。",
				urlThemeError: "輸入使用 http 或 https 通訊協定的有效 URL。",
				inUseBySecurityPolicyMessage: "無法刪除 OAuth 設定檔，因為下列安全設定檔正在使用該設定檔：{0}。",			
				inUseBySecurityPoliciesMessage: "無法刪除 OAuth 設定檔，因為下列安全設定檔正在使用該設定檔：{0}。"			
			},			
			systemControl: {
				pageTitle: "伺服器控制",
				appliance: {
					subTitle: "伺服器",
					tagLine: "影響 ${IMA_PRODUCTNAME_FULL} 伺服器的設定和動作。",
					hostname: "伺服器名稱",
					hostnameNotSet: "（無）",
					retrieveHostnameError: "擷取伺服器名稱時發生錯誤。",
					hostnameError: "儲存伺服器名稱時發生錯誤。",
					editLink: "編輯",
	                viewLink: "檢視",
					firmware: "韌體版本",
					retrieveFirmwareError: "在擷取韌體版本時發生錯誤。",
					uptime: "執行時間",
					retrieveUptimeError: "在擷取執行時間時發生錯誤。",
					// TRNOTE Do not translate locateLED, locateLEDTooltip
					//locateLED: "Locate LED",
					//locateLEDTooltip: "Locate your appliance by turning on the blue LED.",
					locateOnLink: "開啟",
					locateOffLink: "關閉",
					locateLedOnError: "在開啟定位 LED 指示燈時發生錯誤。",
					locateLedOffError: "在關閉定位 LED 指示燈時發生錯誤。",
					locateLedOnSuccess: "定位 LED 指示燈已開啟。",
					locateLedOffSuccess: "定位 LED 指示燈已關閉。",
					restartLink: "重新啟動伺服器",
					restartError: "重新啟動伺服器時發生錯誤。",
					restartMessage: "已提出要重新啟動伺服器的要求。",
					restartMessageDetails: "伺服器可能需要幾分鐘才能重新啟動。當伺服器重新啟動時，使用者介面無法使用。",
					shutdownLink: "關閉伺服器",
					shutdownError: "關閉伺服器時發生錯誤。",
					shutdownMessage: "已提出要關閉伺服器的要求。",
					shutdownMessageDetails: "伺服器可能需要幾分鐘才能關閉。當伺服器關閉時，使用者介面無法使用。" +
					                        "若要重新開啟伺服器，請按下伺服器上的電源按鈕。",
// TRNOTE Do not translate sshHost, sshHostSettings, sshHostStatus, sshHostTooltip, sshHostStatusTooltip,
// retrieveSSHHostError, sshSaveError
//					sshHost: "SSH Host",
//				    sshHostSettings: "Settings",
//				    sshHostStatus: "Status",
//				    sshHostTooltip: "Set the host IP address or addresses that can access the IBM IoT MessageSight SSH console.",
//				    sshHostStatusTooltip: "The host IP addresses that the SSH service is currently using.",
//				    retrieveSSHHostError: "An error occurred while retrieving the SSH host setting and status.",
//				    sshSaveError: "An error occurred while configuring IBM IoT MessageSight to allow SSH connections on the address or addresses provided.",

				    licensedUsageLabel: "授權用途",
				    licensedUsageRetrieveError: "擷取授權用途設定時發生錯誤。",
                    licensedUsageSaveError: "儲存授權用途設定時發生錯誤。",
				    licensedUsageValues: {
				        developers: "開發者",
				        nonProduction: "非正式作業",
				        production: "正式作業"
				    },
				    licensedUsageDialog: {
				        title: "變更授權用途",
				        instruction: "變更伺服器的授權使用情況。" +
				        		"如果您擁有「授權證明」(POE)，則所選授權用途應該符合 POE 中所述的程式授權用途，即「正式作業」或「非正式作業」。" +
				        		"如果 POE 指定程式為「非正式作業」，則應該選取「非正式作業」。" +
				        		"如果在 POE 中未進行任何指定，則會假設您的用途為「正式作業」，因此應該選取「正式作業」。" +
				        		"如果您沒有授權證明，則應該選取「開發者」。",
				        content: "當您變更伺服器的授權使用情況時，伺服器會重新啟動。" +
				        		"您將從使用者介面登出，要等到伺服器重新啟動後，才能使用使用者介面。" +
				        		"伺服器可能需要幾分鐘才能重新啟動。" +
				        		"伺服器重新啟動之後，您必須登入 Web UI 並接受新授權。",
				        saveButton: "儲存並重新啟動伺服器",
				        cancelButton: "取消"
				    },
					restartDialog: {
						title: "重新啟動伺服器",
						content: "您確定要重新啟動伺服器嗎？" +
								"您將從使用者介面登出，要等到伺服器重新啟動後，才能使用使用者介面。" +
								"伺服器可能需要幾分鐘才能重新啟動。",
						okButton: "重新啟動",
						cancelButton: "取消"												
					},
					shutdownDialog: {
						title: "關閉伺服器",
						content: "您確定要關閉伺服器嗎？" +
								 "您將從使用者介面登出，要等到伺服器重新開啟後，才能使用使用者介面。" +
								 "若要重新開啟伺服器，請按下伺服器上的電源按鈕。",
						okButton: "關閉",
						cancelButton: "取消"												
					},

					hostnameDialog: {
						title: "編輯伺服器名稱",
						instruction: "請指定伺服器名稱，最多 1024 個字元。",
						invalid: "輸入的值無效。伺服器名稱可以包含有效的 UTF-8 字元。",
						tooltip: "輸入伺服器名稱。伺服器名稱可以包含有效的 UTF-8 字元。",
						hostname: "伺服器名稱",
						okButton: "儲存",
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
					subTitle: "${IMA_PRODUCTNAME_FULL} 伺服器",
					tagLine: "影響 ${IMA_PRODUCTNAME_SHORT} 伺服器的設定和動作。",
					status: "狀態",
					retrieveStatusError: "在擷取狀態時發生錯誤。",
					stopLink: "停止伺服器",
					forceStopLink: "強制伺服器停止",
					startLink: "啟動伺服器",
					starting: "啟動中",
					unknown: "不明",
					cleanStore: "Maintenance 模式（正在執行清除儲存庫）",
					startError: "啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器時發生錯誤。",
					stopping: "停止中",
					stopError: "停止 ${IMA_PRODUCTNAME_SHORT} 伺服器時發生錯誤。",
					stopDialog: {
						title: "停止 ${IMA_PRODUCTNAME_FULL} 伺服器",
						content: "您確定要停止 ${IMA_PRODUCTNAME_SHORT} 伺服器嗎？",
						okButton: "關閉",
						cancelButton: "取消"						
					},
					forceStopDialog: {
						title: "強制停止 ${IMA_PRODUCTNAME_FULL} 伺服器",
						content: "您確定要強制停止 ${IMA_PRODUCTNAME_SHORT} 伺服器嗎？強制停止是最後的手段，因為它可能會導致訊息遺失。",
						okButton: "強制停止",
						cancelButton: "取消"						
					},

					memory: "記憶體"
				},
				mqconnectivity: {
					subTitle: "MQ Connectivity 服務",
					tagLine: "影響 MQ Connectivity 服務的設定及動作。",
					status: "狀態",
					retrieveStatusError: "在擷取狀態時發生錯誤。",
					stopLink: "停止服務",
					startLink: "啟動服務",
					starting: "啟動中",
					unknown: "不明",
					startError: "在啟動 MQ Connectivity 服務時發生錯誤。",
					stopping: "停止中",
					stopError: "在停止 MQ Connectivity 服務時發生錯誤。",
					started: "執行中",
					stopped: "已停止",
					stopDialog: {
						title: "停止 MQ Connectivity 服務",
						content: "您確定要停止 MQ Connectivity 服務嗎？停止服務可能會導致訊息遺失。",
						okButton: "停止",
						cancelButton: "取消"						
					}
				},
				runMode: {
					runModeLabel: "執行模式",
					cleanStoreLabel: "清除儲存庫",
				    runModeTooltip: "指定 ${IMA_PRODUCTNAME_SHORT} 伺服器的執行模式。一旦選取並確認新的執行模式，在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器之前，新的執行模式不會生效。",
				    runModeTooltipDisabled: "因為 ${IMA_PRODUCTNAME_SHORT} 伺服器的狀態而無法變更執行模式。",
				    cleanStoreTooltip: "指出在 ${IMA_PRODUCTNAME_SHORT} 伺服器重新啟動時，是否要執行清除儲存庫動作。只有在 ${IMA_PRODUCTNAME_SHORT} 伺服器處於維護模式時，才能選取清除儲存庫動作。",
					// TRNOTE {0} is a mode, such as production or maintenance.
				    setRunModeError: "嘗試將 ${IMA_PRODUCTNAME_SHORT} 伺服器的執行模式變更為 {0} 時發生錯誤。",
					retrieveRunModeError: "擷取 ${IMA_PRODUCTNAME_SHORT} 伺服器的執行模式時發生錯誤。",
					runModeDialog: {
						title: "確認執行模式變更",
						// TRNOTE {0} is a mode, such as production or maintenance.
						content: "您確定要將 ${IMA_PRODUCTNAME_SHORT} 伺服器的執行模式變更為 <strong>{0}</strong> 嗎？選取新的執行模式後，必須先重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器，新的執行模式才會生效。",
						okButton: "確認",
						cancelButton: "取消"												
					},
					cleanStoreDialog: {
						title: "確認清除儲存庫動作變更",
						contentChecked: "您確定要在伺服器重新啟動期間執行<strong>清除儲存庫</strong>動作嗎？" +
								 "如果設定了<strong>清除儲存庫</strong>動作，必須重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器才能執行該動作。" +
						         "執行<strong>清除儲存庫</strong>動作將會導致持續保存的傳訊資料遺失。<br/><br/>",
						content: "您確定要取消<strong>清除儲存庫</strong>動作嗎？",
						okButton: "確認",
						cancelButton: "取消"												
					}
				},
				storeMode: {
					storeModeLabel: "儲存庫模式",
				    storeModeTooltip: "指定 ${IMA_PRODUCTNAME_SHORT} 伺服器的儲存庫模式。一旦選取並確認新的儲存庫模式，即會重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器並執行清除儲存庫動作。",
				    storeModeTooltipDisabled: "因為 ${IMA_PRODUCTNAME_SHORT} 伺服器的狀態而無法變更儲存庫模式。",
				    // TRNOTE {0} is a mode, such as production or maintenance.
				    setStoreModeError: "嘗試將 ${IMA_PRODUCTNAME_SHORT} 伺服器的儲存庫模式變更為 {0} 時發生錯誤。",
					retrieveStoreModeError: "擷取 ${IMA_PRODUCTNAME_SHORT} 伺服器的儲存庫模式時發生錯誤。",
					storeModeDialog: {
						title: "確認儲存庫模式變更",
						// TRNOTE {0} is a mode, such as persistent or memory only.
						content: "您確定要將 ${IMA_PRODUCTNAME_SHORT} 伺服器的儲存庫模式變更為 <strong>{0}</strong> 嗎？" + 
								"如果變更儲存庫模式，則會重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器並執行清除儲存庫動作。" +
								"執行清除儲存庫動作將會導致持續保存的傳訊資料遺失。變更儲存庫模式時，必須執行清除儲存庫動作。",
						okButton: "確認",
						cancelButton: "取消"												
					}
				},
				logLevel: {
					subTitle: "設定記載層次",
					tagLine: "設定您要在伺服器日誌中看到的訊息層次。" +
							 "可以從<a href='monitoring.jsp?nav=downloadLogs'>監視 > 下載日誌</a>下載日誌。",
					logLevel: "預設日誌",
					connectionLog: "連線日誌",
					securityLog: "安全日誌",
					adminLog: "管理日誌",
					mqconnectivityLog: "MQ Connectivity 日誌",
					tooltip: "記載層次會控制預設日誌檔中的訊息類型。" +
							 "此日誌檔顯示有關伺服器作業的所有日誌項目。" +
							 "其他日誌中顯示的高嚴重性項目也會記載於此日誌中。" +
							 "「下限」設定將僅包括最重要的項目。" +
							 "「上限」設定將導致包括所有項目。" +
							 "如需記載層次的相關資訊，請參閱文件。",
					connectionTooltip: "記載層次會控制連線日誌檔中的訊息類型。" +
							           "此日誌檔顯示與連線相關的日誌項目。" +
							           "這些項目可用作連線的審核日誌，還會指出連線錯誤。" +
							           "「下限」設定將僅包括最重要的項目。「上限」設定將導致包括所有項目。" +
					                   "如需記載層次的相關資訊，請參閱文件。",
					securityTooltip: "記載層次會控制安全日誌檔中的訊息類型。" +
							         "此日誌檔顯示與鑑別及權限相關的日誌項目。" +
							         "此日誌可用作安全相關項目的審核日誌。" +
							         "「下限」設定將僅包括最重要的項目。「上限」設定將導致包括所有項目。" +
					                 "如需記載層次的相關資訊，請參閱文件。",
					adminTooltip: "記載層次會控制管理日誌檔中的訊息類型。" +
							      "此日誌檔顯示與從 ${IMA_PRODUCTNAME_FULL} Web UI 或指令行執行管理指令相關的日誌項目。" +
							      "它會記錄伺服器配置的所有變更，以及嘗試的變更。" +
								  "「下限」設定將僅包括最重要的項目。" +
								  "「上限」設定將導致包括所有項目。" +
					              "如需記載層次的相關資訊，請參閱文件。",
					mqconnectivityTooltip: "記載層次會控制 MQ Connectivity 日誌檔中的訊息類型。" +
							               "此日誌檔顯示 MQ Connectivity 功能相關的日誌項目。" +
							               "這些項目包括連接至佇列管理程式時發現的問題。" +
							               "此日誌有助於診斷 ${IMA_PRODUCTNAME_FULL} 與 WebSphere<sup>&reg;</sup> MQ 的連線問題。" +
										   "「下限」設定將僅包括最重要的項目。" +
										   "「上限」設定將導致包括所有項目。" +
										   "如需記載層次的相關資訊，請參閱文件。",										
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log										   
					retrieveLogLevelError: "擷取 {0} 層次時發生錯誤。",
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log
					setLogLevelError: "設定 {0} 層次時發生錯誤。",
					levels: {
						MIN: "下限",
						NORMAL: "正常",
						MAX: "上限"
					},
					serverNotAvailable: "當 ${IMA_PRODUCTNAME_FULL} 伺服器停止時，無法設定記載層次。",
					serverCleanStore: "記載層次在執行清除儲存庫時無法設定。",
					serverSyncState: "記載層次在主要節點同步化時無法設定。",
					mqNotAvailable: "MQ Connectivity 記載層次在 MQ Connectivity 服務停止時無法設定。",
					mqStandbyState: "MQ Connectivity 記載層次在伺服器待命時無法設定。"
				}
			},
			highAvailability: {
				title: "高可用性",
				tagline: "配置兩個 ${IMA_PRODUCTNAME_SHORT} 伺服器作為節點的高可用性配對。",
				stateLabel: "現行的高可用性狀態：",
				statePrimary: "主要",
				stateStandby: "待命",
				stateIndeterminate: "不確定",
				error: "高可用性發生錯誤",
				save: "儲存",
				saving: "儲存中…",
				success: "儲存成功。必須重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器及 MQ Connectivity 服務，才能使變更生效。",
				saveFailed: "更新高可用性設定時發生錯誤。",
				ipAddrValidation : "指定 IPv4 位址或 IPv6 位址，例如 192.168.1.0",
				discoverySameAsReplicationError: "探索位址必須不同於抄寫位址。",
				AutoDetect: "自動偵測",
				StandAlone: "獨立式",
				notSet: "未設定",
				yes: "是",
				no: "否",
				config: {
					title: "配置",
					tagline: "已顯示高可用性配置。" +
					         "在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器之前，對配置所做的變更不會生效。",
					editLink: "編輯",
					enabledLabel: "已啟用高可用性：",
					haGroupLabel: "高可用性群組：",
					startupModeLabel: "啟動模式",
					replicationInterfaceLabel: "抄寫位址：",
					discoveryInterfaceLabel: "探索位址：",
					remoteDiscoveryInterfaceLabel: "遠端探索位址：",
					discoveryTimeoutLabel: "探索逾時：",
					heartbeatLabel: "活動訊號逾時：",
					seconds: "{0} 秒",
					secondsDefault: "{0} 秒（預設值）",
					preferedPrimary: "自動偵測（偏好的主要）"
				},
				restartInfoDialog: {
					title: "需要重新啟動",
					content: "您的變更已儲存，但在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器之前不會生效。<br /><br />" +
							 "若要立即重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器，請按一下<b>立即重新啟動</b>。否則，請按一下<b>稍後重新啟動</b>。",
					closeButton: "稍後重新啟動",
					restartButton: "立即重新啟動",
					stopping: "正在停止 ${IMA_PRODUCTNAME_SHORT} 伺服器...",
					stoppingFailed: "停止 ${IMA_PRODUCTNAME_SHORT} 伺服器時發生錯誤。" +
								"使用「伺服器控制」頁面來重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器。",
					starting: "正在啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器..."
//					startingFailed: "An error occurred while starting the IBM IoT MessageSight server. " +
//								"Use the System Control page to start the IBM IoT MessageSight server."
				},
				confirmGroupNameDialog: {
					title: "確認群組名稱變更",
					content: "目前只有一個節點在作用中。如果群組名稱變更，則您可能需要手動編輯其他節點的群組名稱。" +
					         "您確定要變更群組名稱嗎？",
					closeButton: "取消",
					confirmButton: "確認"
				},
				dialog: {
					title: "編輯高可用性配置",
					saveButton: "儲存",
					cancelButton: "取消",
					instruction: "啟用高可用性並設定高可用性群組，以決定與此伺服器配對的伺服器。" + 
								 "或者停用「高可用性」。在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器之前，高可用性配置變更不會生效。",
					haEnabledLabel: "已啟用高可用性：",
					startupMode: "啟動模式",
					haEnabledTooltip: "工具提示文字",
					groupNameLabel: "高可用性群組：",
					groupNameToolTip: "群組用於自動配置伺服器以彼此配對。" +
					                  "兩部伺服器上的這個值必須相同。" +
					                  "此值的長度上限為 128 個字元。",
					advancedSettings: "進階設定",
					advSettingsTagline: "若高可用性配置要使用特定的設定，可手動配置下列設定。" +
										"在變更這些設定之前，請參閱 ${IMA_PRODUCTNAME_FULL} 文件以取得詳細指示和建議。",
					nodePreferred: "當兩個節點在自動偵測模式下啟動時，此節點為偏好的主要節點。",
				    localReplicationInterface: "本端抄寫位址：",
				    localDiscoveryInterface: "本端探索位址：",
				    remoteDiscoveryInterface: "遠端探索位址：",
				    discoveryTimeout: "探索逾時：",
				    heartbeatTimeout: "活動訊號逾時：",
				    startModeTooltip: "節點啟動時的模式。節點可以設定為自動偵測模式或獨立式模式。" +
				    				  "在自動偵測模式中，必須啟動兩個節點。" +
				    				  "節點會自動嘗試彼此偵測，並建立 HA 配對。" +
				    				  "僅在啟動單個節點時使用獨立式模式。",
				    localReplicationInterfaceTooltip: "在 HA 配對中的本端節點上用於 HA 抄寫的 NIC IP 位址。" +
				    								  "您可以選擇要指派的 IP 位址。" +
				    								  "但是，本端抄寫位址必須在與本端探索位址不同的子網路上。",
				    localDiscoveryInterfaceTooltip: "在 HA 配對中的本端節點上用於 HA 探索的 NIC IP 位址。" +
				    							    "您可以選擇要指派的 IP 位址。" +
				    								"但是，本端探索位址必須在與本端抄寫位址不同的子網路上。",
				    remoteDiscoveryInterfaceTooltip: "在 HA 配對中的遠端節點上用於 HA 探索的 NIC IP 位址。" +
				    								 "您可以選擇要指派的 IP 位址。" +
				    								 "但是，遠端探索位址必須在與本端抄寫位址不同的子網路上。",
				    discoveryTimeoutTooltip: "在自動偵測模式下啟動的伺服器必須連接至 HA 配對中的另一個伺服器的時間（以秒為單位）。" +
				    					     "有效範圍為 10 - 2147483647。預設值為 600。" +
				    						 "如果未在該時間內進行連線，則伺服器會在維護模式下啟動。",
				    heartbeatTimeoutTooltip: "伺服器必須判定 HA 配對中的另一個伺服器是否失敗的時間（以秒為單位）。" + 
				    						 "有效範圍為 1 - 2147483647。預設值是 10。" +
				    						 "如果主要伺服器未在此時間內收到待命伺服器的活動訊號，它會繼續擔任主要伺服器，但已停止資料同步化程序。" +
				    						 "如果待命伺服器未在此時間內收到主要伺服器的活動訊號，待命伺服器會變成主要伺服器。",
				    seconds: "秒",
				    nicsInvalid: "本端抄寫位址、本端探索位址及遠端探索位址皆必須提供有效值。" +
				    			 "或者，本端抄寫位址、本端探索位址及遠端探索位址可能皆未提供值。",
				    securityRiskTitle: "安全風險",
				    securityRiskText: "您確定要啟用高可用性，而不指定抄寫及探索位址嗎？" +
				    		"如果您對伺服器的網路環境沒有完全控制權，則會多重播送該伺服器的相關資訊，且您的伺服器可能會與使用相同高可用性群組名稱的未授信伺服器配對。" +
				    		"<br /><br />" +
				    		"您可以透過指定抄寫及探索位址來消除此安全風險。"
				},
				status: {
					title: "狀態",
					tagline: "顯示高可用性狀態。" +
					         "如果狀態與配置不符，可能需要重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器，以使最新的配置變更生效。",
					editLink: "編輯",
					haLabel: "高可用性：",
					haGroupLabel: "高可用性群組：",
					haNodesLabel: "作用中節點",
					pairedWithLabel: "前次已配對：",
					replicationInterfaceLabel: "抄寫位址：",
					discoveryInterfaceLabel: "探索位址：",
					remoteDiscoveryInterfaceLabel: "遠端探索位址：",
					remoteWebUI: "<a href='{0}' target=_'blank'>遠端 Web UI</a>",
					enabled: "已啟用高可用性。",
					disabled: "已停用高可用性"
				},
				startup: {
					subtitle: "節點啟動模式",
					tagline: "節點可以在自動偵測模式或獨立式模式下設定。在自動偵測模式中，必須啟動兩個節點。" +
							 "節點將會自動嘗試彼此偵測，並建立高可用性配對。獨立式模式應該僅在啟動單一節點時使用。",
					mode: "啟動模式",
					modeAuto: "自動偵測",
					modeAlone: "獨立式",
					primary: "當兩個節點在自動偵測模式下啟動時，此節點為偏好的主要節點。"
				},
				remote: {
					subtitle: "遠端節點探索位址",
					tagline: "高可用性配對中遠端節點上探索位址的 IPv4 位址或 IPv6 位址。",
					discoveryInt: "探索位址：",
					discoveryIntTooltip: "輸入用於配置遠端節點探索位址的 IPv4 位址或 IPv6 位址。"
				},
				local: {
					subtitle: "高可用性 NIC 位址",
					tagline: "本端節點抄寫及探索位址的 IPv4 位址或 IPv6 位址。",
					replicationInt: "抄寫位址：",
					replicationIntTooltip: "輸入用於配置本端節點抄寫位址的 IPv4 位址或 IPv6 位址。",
					replicationError: "高可用性抄寫位址無法使用",
					discoveryInt: "探索位址：",
					discoveryIntTooltip: "輸入用於配置本端節點探索位址的 IPv4 位址或 IPv6 位址。"
				},
				timeouts: {
					subtitle: "逾時",
					tagline: "當高可用性啟用節點在自動偵測模式下啟動時，它會嘗試與高可用性配對中的其他節點通訊。" +
							 "如果在探索逾時期間無法建立連線，則節點會在 Maintenance 模式下啟動。",
					tagline2: "活動訊號逾時用於偵測高可用性配對中的其他節點是否已失敗。" +
							  "如果主要節點沒有從待命節點接收到活動訊號，它會繼續作為主要節點工作，但資料同步化程序會停止。" +
							  "如果待命節點沒有從主要節點接收到活動訊號，待命節點會變成主要節點。",
					discoveryTimeout: "探索逾時：",
					discoveryTimeoutTooltip: "節點嘗試連接至配對中其他節點的秒數（介於 10 與 2,147,483,647 之間）。",
					discoveryTimeoutValidation: "輸入介於 10 與 2,147,483,647 之間的數字。",
					discoveryTimeoutError: "探索逾時無法使用",
					heartbeatTimeout: "活動訊號逾時：",
					heartbeatTimeoutTooltip: "介於 1 與 2,147,483,647 之間的節點活動訊號秒數。",
					heartbeatTimeoutValidation: "輸入介於 1 與 2,147,483,647 之間的數字。",
					seconds: "秒"
				}
			},
			webUISecurity: {
				title: "Web UI 設定",
				tagline: "配置 Web UI 的設定。",
				certificate: {
					subtitle: "Web UI 憑證",
					tagline: "配置用於保證與 Web UI 通訊安全的憑證。"
				},
				timeout: {
					subtitle: "階段作業逾時值與 LTPA 記號有效期限",
					tagline: "這些設定會根據瀏覽器與伺服器之間的活動以及總登入時間控制使用者登入階段作業。" +
							 "變更階段作業閒置逾時或 LTPA 記號有效期限，可能會使所有的作用中登入階段作業失效。",
					inactivity: "階段作業閒置逾時",
					inactivityTooltip: "階段作業閒置逾時會控制當瀏覽器與伺服器之間沒有活動時，使用者所能保持登入的時間。" +
							           "閒置逾時必須小於或等於 LTPA 記號有效期限。",
					expiration: "LTPA 記號有效期限",
					expirationTooltip: "小型認證機構記號有效期限會控制使用者可以保持登入的時間量，而無論瀏覽器與伺服器之間有無活動。" +
							           "有效期限必須大於或等於階段作業閒置逾時。如需相關資訊，請參閱文件。", 
					unit: "分鐘",
					save: "儲存",
					saving: "儲存中…",
					success: "儲存成功。請先登出，然後再登入。",
					failed: "儲存失敗",
					invalid: {
						inactivity: "階段作業閒置逾時必須是介於 1 與 LTPA 記號有效期限值之間的數字。",
						expiration: "LTPA 記號有效期限值必須是介於階段作業閒置逾時值與 1440 之間的數字。"
					},
					getExpirationError: "在取得 LTPA 記號有效期限時發生錯誤。",
					getInactivityError: "在取得階段作業閒置逾時時發生錯誤。",
					setExpirationError: "在設定 LTPA 記號有效期限時發生錯誤。",
					setInactivityError: "在設定階段作業閒置逾時時發生錯誤。"
				},
				cacheauth: {
					subtitle: "鑑別快取逾時",
					tagline: "鑑別快取逾時可指定已鑑別認證在快取中的期限。" +
					"較大的值可能會增加安全風險，因為已撤銷的認證仍會在快取中保持較長時間。" +
					"較小的值可能會影響效能，因為使用者登錄必須存取更頻繁。",
					cacheauth: "鑑別快取逾時",
					cacheauthTooltip: "鑑別快取逾時會指出在移除快取中的項目之前，其存在快取中的時間量。" +
							"此值應介於 1 與 3600 秒（1 小時）之間，不應大於 LTPA 記號有效期限值的三分之一。" +
							"如需進一步資訊，請參閱相關文件。", 
					unit: "秒",
					save: "儲存",
					saving: "儲存中…",
					success: "儲存成功",
					failed: "儲存失敗",
					invalid: {
						cacheauth: "鑑別快取逾時值必須是介於 1 與 3600 之間的數字。"
					},
					getCacheauthError: "取得鑑別快取逾時時發生錯誤。",
					setCacheauthError: "設定鑑別快取逾時時發生錯誤。"
				},
				readTimeout: {
					subtitle: "HTTP 讀取逾時",
					tagline: "首次讀取發生之後，在 Socket 上等待讀取要求完成的時間量上限。",
					readTimeout: "HTTP 讀取逾時",
					readTimeoutTooltip: "介於 1 與 300 秒（5 分鐘）之間的值。", 
					unit: "秒",
					save: "儲存",
					saving: "儲存中…",
					success: "儲存成功",
					failed: "儲存失敗",
					invalid: {
						readTimeout: "HTTP 讀取逾時值必須是介於 1 與 300 之間的數字。"
					},
					getReadTimeoutError: "取得 HTTP 讀取逾時時發生錯誤。",
					setReadTimeoutError: "設定 HTTP 讀取逾時時發生錯誤。"
				},
				port: {
					subtitle: "IP 位址及埠",
					tagline: "選取用於 Web UI 通訊的 IP 位址及埠。變更其中一個值可能需要您以新 IP 位址及埠登入。" +
							 "<em>不建議選取 IP 位址 All，因為這可能會在網際網路上公開 Web UI。</em>",
					ipAddress: "IP 位址",
					all: "All",
					interfaceHint: "介面：{0}",
					port: "埠",
					save: "儲存",
					saving: "儲存中…",
					success: "儲存成功。可能需要幾分鐘變更才能生效。請先關閉瀏覽器，然後再登入。",
					failed: "儲存失敗",
					tooltip: {
						port: "輸入可用的埠。有效埠是介於 1（含）與 8999（含）之間、9087 或介於 9100（含）與 65535（含）之間。"
					},
					invalid: {
						interfaceDown: "介面 {0} 報告其已關閉。請檢查介面或選取不同的位址。",
						interfaceUnknownState: "查詢 {0} 的狀態時發生錯誤。請檢查介面或選取不同的位址。",
						port: "埠號必須是介於 1（含）與 8999（含）之間、9087 或介於 9100（含）與 65535（含）之間的數字。"
					},					
					getIPAddressError: "在取得 IP 位址時發生錯誤。",
					getPortError: "在取得埠時發生錯誤。",
					setIPAddressError: "在設定 IP 位址時發生錯誤。",
					setPortError: "在設定埠時發生錯誤。"					
				},
				certificate: {
					subtitle: "安全憑證",
					tagline: "上傳及取代 Web UI 使用的憑證。",
					addButton: "新增憑證",
					editButton: "編輯憑證",
					addTitle: "取代 Web UI 憑證",
					editTitle: "編輯 Web UI 憑證",
					certificateLabel: "憑證：",
					keyLabel: "私密金鑰：",
					expiryLabel: "憑證期限：",
					defaultExpiryDate: "警告！預設的憑證正在使用中且應該取代",
					defaultCert: "預設值",
					success: "可能需要幾分鐘變更才能生效。請先關閉瀏覽器，然後再登入。"
				},
				service: {
					subtitle: "服務",
					tagline: "設定追蹤層次以便為 Web UI 產生支援資訊。",
					traceEnabled: "啟用追蹤",
					traceSpecificationLabel: "追蹤規格",
					traceSpecificationTooltip: "「IBM 支援中心」提供的追蹤規格字串。",
				    save: "儲存",
				    getTraceError: "在取得追蹤規格時發生錯誤。",
				    setTraceError: "在設定追蹤規格時發生錯誤。",
				    clearTraceError: "在清除追蹤規格時發生錯誤。",
					saving: "儲存中…",
					success: "儲存成功。",
					failed: "儲存失敗"
				}
			}
		},
        platformDataRetrieveError: "擷取平台資料時發生錯誤。",
        // Strings for client certs that can be used for certificate authentication
		clientCertsTitle: "用戶端憑證",
		clientCertsButton: "用戶端憑證",
        stopHover: "關閉伺服器，並阻止對伺服器的所有存取。",
        restartLink: "重新啟動伺服器",
		restartMaintOnLink: "啟動維護",
		restartMaintOnHover: "重新啟動伺服器，並在維護模式下執行它。",
		restartMaintOffLink: "停止維護",
		restartMaintOffHover: "重新啟動伺服器，並在正式作業模式下執行它。",
		restartCleanStoreLink: "清除儲存庫",
		restartCleanStoreHover: "清除伺服器儲存庫，並重新啟動伺服器。",
        restartDialogTitle: "重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器",
        restartMaintOnDialogTitle: "在維護模式下重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器",
        restartMaintOffDialogTitle: "在正式作業模式下重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器",
        restartCleanStoreDialogTitle: "清除 ${IMA_PRODUCTNAME_SHORT} 伺服器儲存庫",
        restartDialogContent: "您確定要重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器嗎？重新啟動伺服器會中斷所有傳訊。",
        restartMaintOnDialogContent: "您確定要在維護模式下重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器嗎？在維護模式下啟動伺服器會停止所有傳訊。",
        restartMaintOffDialogContent: "您確定要在正式作業模式下重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器嗎？在正式作業模式下啟動伺服器會重新啟動傳訊。",
        restartCleanStoreDialogContent: "執行<strong>清除儲存庫</strong>動作將會導致持續保存的傳訊資料遺失。<br/><br/>" +
        	"您確定要執行<strong>清除儲存庫</strong>動作嗎？",
        restartOkButton: "重新啟動",        
        cleanOkButton: "清除儲存庫並重新啟動",
        restarting: "正在重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器...",
        restartingFailed: "重新啟動 ${IMA_PRODUCTNAME_SHORT} 伺服器時發生錯誤。",
        remoteLogLevelTagLine: "設定您要在伺服器日誌中看到的訊息層次。", 
        errorRetrievingTrustedCertificates: "擷取授信憑證時發生錯誤。",

        // New page title and tagline:  Admin Endpoint
        adminEndpointPageTitle: "管理端點配置",
        adminEndpointPageTagline: "配置管理端點及建立配置原則。" +
        		"Web UI 連接至管理端點來執行管理和監視作業。" +
        		"配置原則控制允許使用者執行哪些管理及監視作業。",
        // HA config strings
        haReplicationPortLabel: "抄寫埠：",
		haExternalReplicationInterfaceLabel: "外部抄寫位址：",
		haExternalReplicationPortLabel: "外部抄寫埠：",
        haRemoteDiscoveryPortLabel: "遠端探索埠：",
        haReplicationPortTooltip: "HA 配對中的本端節點上的 HA 抄寫所使用的埠號。",
		haReplicationPortInvalid: "抄寫埠無效。" +
				"有效範圍為 1024 - 65535。預設值是 0，允許在連線時隨機選取可用的埠。",
		haExternalReplicationInterfaceTooltip: "HA 抄寫所應使用的 NIC 的 IPv4 或 IPv6 位址。",
		haExternalReplicationPortTooltip: "HA 配對中的本端節點上的 HA 抄寫所使用的外部埠號。",
		haExternalReplicationPortInvalid: "外部抄寫埠無效。" +
				"有效範圍為 1024 - 65535。預設值是 0，允許在連線時隨機選取可用的埠。",
        haRemoteDiscoveryPortTooltip: "HA 配對中的遠端節點上的 HA 探索所使用的埠號。",
		haRemoteDiscoveryPortInvalid: "遠端探索埠無效。" +
				 "有效範圍為 1024 - 65535。預設值是 0，允許在連線時隨機選取可用的埠。",
		haReplicationAndDiscoveryBold: "抄寫及探索位址",
		haReplicationAndDiscoveryNote: "（如果設定一個，必須全部設定）",
		haUseSecuredConnectionsLabel: "使用安全連線：",
		haUseSecuredConnectionsTooltip: "請使用 TLS 作為 HA 對組的節點間的探索與資料通道。",
		haUseSecuredConnectionsEnabled: "已啟用安全連線",
		haUseSecuredConnectionsDisabled: "已停用安全連線",
		mqConnectivityEnableLabel: "啟用 MQ Connectivity",
		savingProgress: "儲存中…",
	    setMqConnEnabledError: "設定 MQConnectivity 已啟用狀態時發生錯誤。",
	    allowCertfileOverwriteLabel: "改寫",
		allowCertfileOverwriteTooltip: "容許改寫憑證和私密金鑰。",
		stopServerWarningTitle: "停止動作將導致無法從 Web UI 重新啟動！",
		stopServerWarning: "停止伺服器會阻止透過 REST 要求和 Web UI 的所有傳訊和所有管理存取。<b>為了避免無法從 Web UI 存取伺服器，請使用「重新啟動伺服器」鏈結。</b>",
		// New Web UI user dialog strings to allow French translation to insert a space before colon
		webuiuserLabel: "使用者 ID︰",
		webuiuserdescLabel: "說明：",
		webuiuserroleLabel: "角色："

});
