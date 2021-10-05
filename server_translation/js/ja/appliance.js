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
				title: "Web UI ユーザー管理",
				tagline: "Web UI へのアクセス権限をユーザーに付与します。",
				usersSubTitle: "Web UI ユーザー",
				webUiUsersTagline: "システム管理者は、Web UI ユーザーを定義、編集、または削除できます。",
				itemName: "ユーザー",
				itemTooltip: "",
				addDialogTitle: "ユーザーの追加",
				editDialogTitle: "ユーザーの編集",
				removeDialogTitle: "ユーザーの削除",
				resetPasswordDialogTitle: "パスワードの再設定",
				// TRNOTE: Do not translate addGroupDialogTitle, editGroupDialogTitle, removeGroupDialogTitle
				//addGroupDialogTitle: "Add Group",
				//editGroupDialogTitle: "Edit Group",
				//removeGroupDialogTitle: "Delete Group",
				grid: {
					userid: "ユーザー ID",
					// TRNOTE: Do not translate groupid
					//groupid: "Group ID",
					description: "説明",
					groups: "ロール",
					noEntryMessage: "表示する項目がありません",
					// TRNOTE: Do not translate ldapEnabledMessage
					//ldapEnabledMessage: "Messaging users and groups configured on the appliance are disabled because an external LDAP connection is enabled.",
					refreshingLabel: "更新中..."
				},
				dialog: {
					useridTooltip: "ユーザー ID を入力します。先頭と末尾にはスペースがあってはなりません。",
					applianceUserIdTooltip: "ユーザー ID を入力します。使用できるのは文字 (A-Z、a-z)、数字 (0-9)、および 4 つの特殊文字 (- _ . +) のみです。",					
					useridInvalid: "ユーザー ID の先頭と末尾にはスペースがあってはなりません。",
					applianceInvalid: "ユーザー ID に使用できるのは、文字 (A-Z、a-z)、数字 (0-9)、および 4 つの特殊文字 (- _ . +) のみです。",					
					// TRNOTE: Do not translate groupidTooltip, groupidInvalid
					//groupidTooltip: "Enter the Group ID, which must not have leading or trailing spaces.",
					//groupidInvalid: "The Group ID must not have leading or trailing spaces.",
					SystemAdministrators: "システム管理者",
					MessagingAdministrators: "メッセージング管理者",
					Users: "ユーザー",
					applianceUserInstruction: "システム管理者は、他の Web UI 管理者および Web UI ユーザーの作成、更新、削除を行えます。",
					// TRNOTE: Do not translate messagingUserInstruction, messagingGroupInstruction
					//messagingUserInstruction: "Messaging users can be added to messaging policies to control the messaging resources that a user can access.",
					//messagingGroupInstruction: "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.",				                    
					password1: "パスワード:",
					password2: "パスワードの確認:",
					password2Invalid: "パスワードが一致しません。",
					passwordNoSpaces: "パスワードの先頭と末尾にはスペースがあってはなりません。",
					saveButton: "保管",
					cancelButton: "キャンセル",
					refreshingGroupsLabel: "グループを取得中..."
				},
				returnCodes: {
					CWLNA5056: "一部の設定を保管できませんでした"
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
				title: "項目の削除",
				content: "このレコードを削除してもよろしいですか?",
				deleteButton: "削除",
				cancelButton: "キャンセル"
			},
			savingProgress: "保管中...",
			savingFailed: "保管に失敗しました。",
			deletingProgress: "削除中...",
			deletingFailed: "削除に失敗しました。",
			testingProgress: "テスト中...",
			uploadingProgress: "アップロード中...",
			dialog: { 
				saveButton: "保管",
				cancelButton: "キャンセル",
				testButton: "接続のテスト"
			},
			invalidName: "その名前のレコードはすでに存在します。",
			invalidChars: "有効なホスト名または IP アドレスが必要です。",
			invalidRequired: "値が必要です。",
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
				nothingToTest: "接続をテストするには、IPv4 アドレスまたは IPv6 アドレスを指定する必要があります。",
				testConnFailed: "テスト接続に失敗しました",
				testConnSuccess: "テスト接続に成功しました。",
				testFailed: "テストに失敗しました",
				testSuccess: "テストに成功しました"
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
				title: "セキュリティー設定",
				tagline: "鍵と証明書をインポートして、サーバーへの接続を保護します。",
				securityProfilesSubTitle: "セキュリティー・プロファイル",
				securityProfilesTagline: "システム管理者は、クライアントの検証方法を指定するセキュリティー・プロファイルを定義、編集、または削除できます。",
				certificateProfilesSubTitle: "証明書プロファイル",
				certificateProfilesTagline: "システム管理者は、ユーザー ID を検証する証明書プロファイルを定義、編集、または削除できます。",
				ltpaProfilesSubTitle: "LTPA プロファイル",
				ltpaProfilesTagline: "システム管理者は、複数サーバーでのシングル・サインオンを有効にする Lightweight Third Party Authentication (LTPA) プロファイルを定義、編集、または削除できます。",
				oAuthProfilesSubTitle: "OAuth プロファイル",
				oAuthProfilesTagline: "システム管理者は、複数サーバーでのシングル・サインオンを有効にする OAuth プロファイルを定義、編集、または削除できます。",
				systemWideSubTitle: "サーバー全体のセキュリティー設定",
				useFips: "メッセージング通信の保護に FIPS 140-2 プロファイルを使用",
				useFipsTooltip: "セキュリティー・プロファイルで保護されたエンドポイントに対して連邦情報処理標準 (FIPS) 140-2 に準拠する暗号方式を使用する場合に、このオプションを選択します。 " +
				                "この設定を変更した場合は、${IMA_PRODUCTNAME_SHORT} サーバーを再始動する必要があります。",
				retrieveFipsError: "FIPS 設定の取得中にエラーが発生しました。",
				fipsDialog: {
					title: "FIPS の設定",
					content: "この変更を有効にするには、${IMA_PRODUCTNAME_SHORT} サーバーを再始動する必要があります。 この設定を変更しますか?"	,
					okButton: "設定して再始動",
					cancelButton: "変更のキャンセル",
					failed: "FIPS 設定の変更中にエラーが発生しました。",
					stopping: "${IMA_PRODUCTNAME_SHORT} サーバーの停止中です...",
					stoppingFailed: "${IMA_PRODUCTNAME_SHORT} サーバーの停止中にエラーが発生しました。",
					starting: "${IMA_PRODUCTNAME_SHORT} サーバーの始動中です...",
					startingFailed: "${IMA_PRODUCTNAME_SHORT} サーバーの始動中にエラーが発生しました。"
				}
			},
			securityProfiles: {
				grid: {
					name: "名前",
					mprotocol: "最小プロトコル方式",
					ciphers: "暗号",
					certauth: "クライアント証明書の使用",
					clientciphers: "クライアントの暗号を使用",
					passwordAuth: "パスワード認証の使用",
					certprofile: "証明書プロファイル",
					ltpaprofile: "LTPA プロファイル",
					oAuthProfile: "OAuth プロファイル",
					clientCertsButton: "トラステッド証明書"
				},
				trustedCertMissing: "トラステッド証明書がありません",
				retrieveError: "セキュリティー・プロファイルの取得中にエラーが発生しました。",
				addDialogTitle: "セキュリティー・プロファイルの追加",
				editDialogTitle: "セキュリティー・プロファイルの編集",
				removeDialogTitle: "セキュリティー・プロファイルの削除",
				dialog: {
					nameTooltip: "最大 32 文字の英数字で構成される名前。 先頭文字は数字であってはなりません。",
					mprotocolTooltip: "接続時に許可される最低レベルのプロトコル方式です。",
					ciphersTooltip: "<strong>最高:&nbsp;</strong> サーバーおよびクライアントでサポートされる最も安全な暗号を選択します。<br />" +
							        "<strong>高速:&nbsp;</strong>  サーバーおよびクライアントでサポートされる最も高速でセキュリティー・レベルの高い暗号を選択します。<br />" +
							        "<strong>中:&nbsp;</strong> クライアントでサポートされる、最も高速でセキュリティー・レベルが中程度以上の暗号を選択します。",
				    ciphers: {
				    	best: "最高",
				    	fast: "高速",
				    	medium: "中"
				    },
					certauthTooltip: "クライアント証明書を認証するかどうかを選択します。",
					clientciphersTooltip: "接続時に使用する暗号スイートをクライアントが決定できるかどうかを選択します。",
					passwordAuthTooltip: "接続時に有効なユーザー ID とパスワードを要求するかどうかを選択します。",
					passwordAuthTooltipDisabled: "LTPA プロファイルまたは OAuth プロファイルが選択されているため変更できません。 変更するには、LTPA プロファイルまたは OAuth プロファイルを<em>「いずれかを選択」</em>に変更してください。",
					certprofileTooltip: "このセキュリティー・プロファイルに関連付ける既存の証明書プロファイルを選択します。",
					ltpaprofileTooltip: "このセキュリティー・プロファイルに関連付ける既存の LTPA プロファイルを選択します。 " +
					                    "LTPA プロファイルを選択する場合、「パスワード認証の使用」も選択されている必要があります。",
					ltpaprofileTooltipDisabled: "OAuth プロファイルが選択されているか、「パスワード認証の使用」が選択されていないため、LTPA プロファイルを選択できません。",
					oAuthProfileTooltip: "このセキュリティー・プロファイルに関連付ける既存の OAuth プロファイルを選択します。 " +
                    					  "OAuth プロファイルを選択する場合は、「パスワード認証の使用」も選択する必要があります。",
                    oAuthProfileTooltipDisabled: "LTPA プロファイルが選択されているか、「パスワード認証の使用」が選択されていないため、OAuth プロファイルを選択できません。",
					noCertProfilesTitle: "証明書プロファイルがありません",
					noCertProfilesDetails: "<em>「TLS の使用」</em>を選択すると、証明書プロファイルをまず定義してからでないと、セキュリティー・プロファイルを定義できなくなります。",
					chooseOne: "いずれかを選択"
				},
				certsDialog: {
					title: "トラステッド証明書",
					instruction: "セキュリティー・プロファイルのクライアント証明書トラストストアに証明書をアップロードするか、このトラストストアから証明書を削除します。",
					securityProfile: "セキュリティー・プロファイル",
					certificate: "証明書",
					browse: "参照",
					browseHint: "ファイルの選択...",
					upload: "アップロード",
					close: "閉じる",
					remove: "削除",
					removeTitle: "証明書の削除",
					removeContent: "この証明書を削除しますか?",
					retrieveError: "証明書の取得中にエラーが発生しました。",
					uploadError: "証明書ファイルのアップロード中にエラーが発生しました。",
					deleteError: "証明書ファイルの削除中にエラーが発生しました。"	
				}
			},
			certificateProfiles: {
				grid: {
					name: "名前",
					certificate: "証明書",
					privkey: "秘密鍵",
					certexpiry: "証明書の有効期限",
					noDate: "日付を取得中にエラーが発生しました..."
				},
				retrieveError: "証明書プロファイルの取得中にエラーが発生しました。",
				uploadCertError: "証明書ファイルのアップロード中にエラーが発生しました。",
				uploadKeyError: "鍵ファイルのアップロード中にエラーが発生しました。",
				uploadTimeoutError: "証明書ファイルまたは鍵ファイルのアップロード中にエラーが発生しました。",
				addDialogTitle: "証明書プロファイルの追加",
				editDialogTitle: "証明書プロファイルの編集",
				removeDialogTitle: "証明書プロファイルの削除",
				dialog: {
					certpassword: "証明書パスワード:",
					keypassword: "鍵パスワード:",
					browse: "参照...",
					certificate: "証明書:",
					certificateTooltip: "", // "Upload a certificate"
					privkey: "秘密鍵:",
					privkeyTooltip: "", // "Upload a private key"
					browseHint: "ファイルの選択...",
					duplicateFile: "この名前のファイルは既に存在しています。",
					identicalFile: "証明書または鍵を同じファイルにすることはできません。",
					noFilesToUpload: "証明書プロファイルを更新するには、アップロードするファイルを 1 つ以上選択してください。"
				}
			},
			ltpaProfiles: {
				addDialogTitle: "LTPA プロファイルの追加",
				editDialogTitle: "LTPA プロファイルの編集",
				instruction: "複数サーバーへのシングル・サインオンを有効にするには、セキュリティー・プロファイルで LTPA (Lightweight Third Party Authentication) プロファイルを使用します。",
				removeDialogTitle: "LTPA プロファイルの削除",				
				keyFilename: "鍵ファイル名",
				keyFilenameTooltip: "エクスポートされた LTPA 鍵が含まれるファイル。",
				browse: "参照...",
				browseHint: "ファイルの選択...",
				password: "パスワード",
				saveButton: "保管",
				cancelButton: "キャンセル",
				retrieveError: "LTPA プロファイルの取得中にエラーが発生しました。",
				duplicateFileError: "この名前の鍵ファイルは既に存在しています。",
				uploadError: "鍵ファイルのアップロード中にエラーが発生しました。",
				noFilesToUpload: "LTPA プロファイルを更新するには、アップロードするファイルを選択してください。"
			},
			oAuthProfiles: {
				addDialogTitle: "OAuth プロファイルの追加",
				editDialogTitle: "OAuth プロファイルの編集",
				instruction: "複数サーバーへのシングル・サインオンを有効にするには、セキュリティー・プロファイルで OAuth プロファイルを使用してください。",
				removeDialogTitle: "OAuth プロファイルの削除",	
				resourceUrl: "リソース URL",
				resourceUrlTooltip: "アクセス・トークンの検証に使用する許可サーバーの URL。 URL にはプロトコルが含まれている必要があります。 プロトコルには HTTP または HTTPS のいずれかを使用できます。",
				keyFilename: "OAuth サーバー証明書",
				keyFilenameTooltip: "OAuth サーバーへの接続に使用される SSL 証明書を含むファイルの名前。",
				browse: "参照...",
				reset: "リセット",
				browseHint: "ファイルの選択...",
				authKey: "許可鍵",
				authKeyTooltip: "アクセス・トークンを含む鍵の名前。 デフォルト名は <em>access_token</em> です。",
				userInfoUrl: "ユーザー情報 URL",
				userInfoUrlTooltip: "ユーザー情報を取得するために使用する許可サーバーの URL。 URL にはプロトコルが含まれている必要があります。 プロトコルには HTTP または HTTPS のいずれかを使用できます。",
				userInfoKey: "ユーザー情報鍵",
				userInfoKeyTooltip: "ユーザー情報を取得するために使用する鍵の名前。",
				saveButton: "保管",
				cancelButton: "キャンセル",
				retrieveError: "OAuth プロファイルの取得中にエラーが発生しました。",
				duplicateFileError: "この名前の鍵ファイルは既に存在しています。",
				uploadError: "鍵ファイルのアップロード中にエラーが発生しました。",
				urlThemeError: "HTTP または HTTPS プロトコルを使用して有効な URL を入力してください。",
				inUseBySecurityPolicyMessage: "次のセキュリティー・プロファイルにより使用されているため、OAuth プロファイルを削除できません: {0}。",			
				inUseBySecurityPoliciesMessage: "次のセキュリティー・プロファイルにより使用されているため、OAuth プロファイルを削除できません: {0}。"			
			},			
			systemControl: {
				pageTitle: "サーバー制御",
				appliance: {
					subTitle: "サーバー",
					tagLine: "${IMA_PRODUCTNAME_FULL} サーバーに影響する設定およびアクション。",
					hostname: "サーバー名",
					hostnameNotSet: "(なし)",
					retrieveHostnameError: "サーバー名の取得中にエラーが発生しました。",
					hostnameError: "サーバー名の保管中にエラーが発生しました。",
					editLink: "編集",
	                viewLink: "表示",
					firmware: "ファームウェア・バージョン",
					retrieveFirmwareError: "ファームウェア・バージョンの取得中にエラーが発生しました。",
					uptime: "実行時間",
					retrieveUptimeError: "実行時間の取得中にエラーが発生しました。",
					// TRNOTE Do not translate locateLED, locateLEDTooltip
					//locateLED: "Locate LED",
					//locateLEDTooltip: "Locate your appliance by turning on the blue LED.",
					locateOnLink: "点灯",
					locateOffLink: "消灯",
					locateLedOnError: "位置特定 LED インジケーターの点灯時にエラーが発生しました。",
					locateLedOffError: "位置特定 LED インジケーターの消灯時にエラーが発生しました。",
					locateLedOnSuccess: "位置特定 LED インジケーターが点灯しました。",
					locateLedOffSuccess: "位置特定 LED インジケーターが消灯しました。",
					restartLink: "サーバーの再始動",
					restartError: "サーバーの再始動中にエラーが発生しました。",
					restartMessage: "サーバーの再始動要求が実行依頼されました。",
					restartMessageDetails: "サーバーの再始動には数分かかる場合があります。 サーバーの再始動中は、ユーザー・インターフェースを使用できません。",
					shutdownLink: "サーバーのシャットダウン",
					shutdownError: "サーバーのシャットダウン中にエラーが発生しました。",
					shutdownMessage: "サーバーのシャットダウン要求が実行依頼されました。",
					shutdownMessageDetails: "サーバーのシャットダウンには数分かかる場合があります。 サーバーのシャットダウン中は、ユーザー・インターフェースを使用できません。 " +
					                        "サーバーを再度始動するには、サーバーの電源ボタンを押します。",
// TRNOTE Do not translate sshHost, sshHostSettings, sshHostStatus, sshHostTooltip, sshHostStatusTooltip,
// retrieveSSHHostError, sshSaveError
//					sshHost: "SSH Host",
//				    sshHostSettings: "Settings",
//				    sshHostStatus: "Status",
//				    sshHostTooltip: "Set the host IP address or addresses that can access the IBM IoT MessageSight SSH console.",
//				    sshHostStatusTooltip: "The host IP addresses that the SSH service is currently using.",
//				    retrieveSSHHostError: "An error occurred while retrieving the SSH host setting and status.",
//				    sshSaveError: "An error occurred while configuring IBM IoT MessageSight to allow SSH connections on the address or addresses provided.",

				    licensedUsageLabel: "ライセンスで許可された用途",
				    licensedUsageRetrieveError: "ライセンスで許可された用途の設定を取得している間にエラーが発生しました。",
                    licensedUsageSaveError: "ライセンスで許可された用途の設定を保管している間にエラーが発生しました。",
				    licensedUsageValues: {
				        developers: "開発者",
				        nonProduction: "非実動",
				        production: "実動"
				    },
				    licensedUsageDialog: {
				        title: "ライセンスで許可された用途の変更",
				        instruction: "ライセンスで許可されたサーバーの用途を変更します。 " +
				        		"ライセンス証書 (POE) がある場合は、許可されているプログラム使用法に関する POE の記載内容どおりに、ライセンスで許可された用途として「実動」と「非実動」のいずれかを選択する必要があります。 " +
				        		"プログラムが「非実動」と POE に指定されている場合は、「非実動」を選択してください。 " +
				        		"POE に指定がない場合、用途は「実動」と見なされるので、「実動」を選択してください。 " +
				        		"ライセンス証書がない場合は、「開発者」を選択してください。",
				        content: "ライセンスで許可されたサーバーの用途を変更すると、サーバーが再始動します。 " +
				        		"ユーザー・インターフェースからログアウトし、サーバーの再始動中はユーザー・インターフェースを使用できなくなります。 " +
				        		"サーバーの再始動には数分かかる場合があります。 " +
				        		"サーバーが再始動した後、Web UI にログインし、新しいライセンスを受け入れる必要があります。",
				        saveButton: "保管してサーバーを再始動",
				        cancelButton: "キャンセル"
				    },
					restartDialog: {
						title: "サーバーの再始動",
						content: "サーバーを再始動しますか? " +
								"ユーザー・インターフェースからログアウトし、サーバーの再始動中はユーザー・インターフェースを使用できなくなります。 " +
								"サーバーの再始動には数分かかる場合があります。",
						okButton: "再始動",
						cancelButton: "キャンセル"												
					},
					shutdownDialog: {
						title: "サーバーのシャットダウン",
						content: "サーバーをシャットダウンしますか? " +
								 "ユーザー・インターフェースからログアウトし、サーバーが再始動するまでユーザー・インターフェースを使用できなくなります。 " +
								 "サーバーを再度始動するには、サーバーの電源ボタンを押します。",
						okButton: "シャットダウン",
						cancelButton: "キャンセル"												
					},

					hostnameDialog: {
						title: "サーバー名の編集",
						instruction: "サーバー名を 1024 文字以内で指定します。",
						invalid: "入力された値は無効です。 サーバー名には、有効な UTF-8 文字を使用することができます。",
						tooltip: "サーバー名を入力します。  サーバー名には、有効な UTF-8 文字を使用することができます。",
						hostname: "サーバー名",
						okButton: "保管",
						cancelButton: "キャンセル"												
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
					subTitle: "${IMA_PRODUCTNAME_FULL} サーバー",
					tagLine: "${IMA_PRODUCTNAME_SHORT} サーバーに影響する設定およびアクション。",
					status: "状況",
					retrieveStatusError: "状況の取得中にエラーが発生しました。",
					stopLink: "サーバーの停止",
					forceStopLink: "サーバーの強制停止",
					startLink: "サーバーの始動",
					starting: "始動中",
					unknown: "不明",
					cleanStore: "保守モード (ストア消去の進行中)",
					startError: "${IMA_PRODUCTNAME_SHORT} サーバーの始動中にエラーが発生しました。",
					stopping: "停止中",
					stopError: "${IMA_PRODUCTNAME_SHORT} サーバーの停止中にエラーが発生しました。",
					stopDialog: {
						title: "${IMA_PRODUCTNAME_FULL} サーバーの停止",
						content: "${IMA_PRODUCTNAME_SHORT} サーバーを停止しますか?",
						okButton: "シャットダウン",
						cancelButton: "キャンセル"						
					},
					forceStopDialog: {
						title: "${IMA_PRODUCTNAME_FULL} サーバーの強制停止",
						content: "${IMA_PRODUCTNAME_SHORT} サーバーを強制停止しますか? メッセージが失われる可能性があるため、強制停止は最終手段として使用してください。",
						okButton: "強制停止",
						cancelButton: "キャンセル"						
					},

					memory: "メモリー"
				},
				mqconnectivity: {
					subTitle: "MQ Connectivity サービス",
					tagLine: "MQ Connectivity サービスに影響を与える設定およびアクション。",
					status: "状況",
					retrieveStatusError: "状況の取得中にエラーが発生しました。",
					stopLink: "サービスの停止",
					startLink: "サービスの始動",
					starting: "始動中",
					unknown: "不明",
					startError: "MQ Connectivity サービスの始動中にエラーが発生しました。",
					stopping: "停止中",
					stopError: "MQ Connectivity サービスの停止中にエラーが発生しました。",
					started: "実行中",
					stopped: "停止",
					stopDialog: {
						title: "MQ Connectivity サービスの停止",
						content: "MQ Connectivity サービスを停止しますか?  サービスを停止すると、メッセージが失われる可能性があります。",
						okButton: "停止",
						cancelButton: "キャンセル"						
					}
				},
				runMode: {
					runModeLabel: "実行モード",
					cleanStoreLabel: "ストア消去",
				    runModeTooltip: "${IMA_PRODUCTNAME_SHORT} サーバーの実行モードを指定します。 新しい実行モードが選択され、確認されても、${IMA_PRODUCTNAME_SHORT} サーバーが再始動するまで新しい実行モードは有効になりません。",
				    runModeTooltipDisabled: "${IMA_PRODUCTNAME_SHORT} サーバーの状況により、実行モードの変更は行えません。",
				    cleanStoreTooltip: "${IMA_PRODUCTNAME_SHORT} サーバーの再始動時にストア消去アクションを実行するかどうかを指定します。 ストア消去アクションは、${IMA_PRODUCTNAME_SHORT} サーバーが保守モードの場合にのみ選択できます。",
					// TRNOTE {0} is a mode, such as production or maintenance.
				    setRunModeError: "${IMA_PRODUCTNAME_SHORT} サーバーの実行モードを{0}に変更しようとしたときにエラーが発生しました。",
					retrieveRunModeError: "${IMA_PRODUCTNAME_SHORT} サーバーの実行モードの取得中にエラーが発生しました。",
					runModeDialog: {
						title: "実行モードの変更の確認",
						// TRNOTE {0} is a mode, such as production or maintenance.
						content: "${IMA_PRODUCTNAME_SHORT} サーバーの実行モードを<strong>{0}</strong>に変更しますか? 新しい実行モードを選択した場合、その新しい実行モードを有効にするには、${IMA_PRODUCTNAME_SHORT} サーバーを再始動する必要があります。",
						okButton: "確認",
						cancelButton: "キャンセル"												
					},
					cleanStoreDialog: {
						title: "ストア消去アクションの変更の確認",
						contentChecked: "サーバーの再始動中に<strong>ストア消去</strong>アクションを実行しますか? " +
								 "<strong>ストア消去</strong>アクションを設定した場合、アクションを実行するには、${IMA_PRODUCTNAME_SHORT} サーバーを再始動する必要があります。 " +
						         "<strong>ストア消去</strong>アクションを実行すると、永続メッセージング・データが失われます。<br/><br/>",
						content: "<strong>ストア消去</strong>アクションをキャンセルしますか?",
						okButton: "確認",
						cancelButton: "キャンセル"												
					}
				},
				storeMode: {
					storeModeLabel: "ストア・モード",
				    storeModeTooltip: "${IMA_PRODUCTNAME_SHORT} サーバーのストア・モードを指定します。 新しいストア・モードを選択して確認すると、${IMA_PRODUCTNAME_SHORT} サーバーが再始動し、ストア消去が実行されます。",
				    storeModeTooltipDisabled: "${IMA_PRODUCTNAME_SHORT} サーバーの状況が原因でストア・モードを変更できません。",
				    // TRNOTE {0} is a mode, such as production or maintenance.
				    setStoreModeError: "${IMA_PRODUCTNAME_SHORT} サーバーのストア・モードを {0} に変更しようとしたときにエラーが発生しました。",
					retrieveStoreModeError: "${IMA_PRODUCTNAME_SHORT} サーバーのストア・モードの取得中にエラーが発生しました。",
					storeModeDialog: {
						title: "ストア・モードの変更の確認",
						// TRNOTE {0} is a mode, such as persistent or memory only.
						content: "${IMA_PRODUCTNAME_SHORT} サーバーのストア・モードを <strong>{0}</strong> に変更しますか? " + 
								"ストア・モードを変更すると、${IMA_PRODUCTNAME_SHORT} サーバーが再始動し、ストア消去アクションが実行されます。 " +
								"ストア消去アクションを実行すると、永続メッセージング・データが失われます。 ストア・モードを変更したら、ストア消去を実行する必要があります。",
						okButton: "確認",
						cancelButton: "キャンセル"												
					}
				},
				logLevel: {
					subTitle: "ログ・レベルの設定",
					tagLine: "サーバー・ログに表示するメッセージのレベルを設定します。  " +
							 "ログは、<a href='monitoring.jsp?nav=downloadLogs'>「モニター」>「ログのダウンロード」</a>からダウンロードできます。",
					logLevel: "デフォルト・ログ",
					connectionLog: "接続ログ",
					securityLog: "セキュリティー・ログ",
					adminLog: "管理ログ",
					mqconnectivityLog: "MQ Connectivity ログ",
					tooltip: "このログ・レベルは、デフォルト・ログ・ファイルでのメッセージのタイプを制御します。 " +
							 "このログ・ファイルには、サーバーの操作に関するすべてのログ項目が示されます。 " +
							 "他のログに示される重大度が高い項目も、このログに記録されます。 " +
							 "「最小」に設定すると、最も重要な項目のみが含まれます。 " +
							 "「最大」に設定すると、すべての項目が含まれます。 " +
							 "ログ・レベルについて詳しくは、資料を参照してください。",
					connectionTooltip: "このログ・レベルは、接続ログ・ファイルでのメッセージのタイプを制御します。 " +
							           "このログ・ファイルには、接続に関するログ項目が示されます。 " +
							           "これらの項目は、接続の監査ログとして使用できます。また、接続のエラーも示します。 " +
							           "「最小」に設定すると、最も重要な項目のみが含まれます。 「最大」に設定すると、すべての項目が含まれます。 " +
					                   "ログ・レベルについて詳しくは、資料を参照してください。 ",
					securityTooltip: "このログ・レベルは、セキュリティー・ログ・ファイルでのメッセージのタイプを制御します。 " +
							         "このログ・ファイルには、認証および許可に関するログ項目が示されます。 " +
							         "このログは、セキュリティー関連項目の監査ログとして使用できます。 " +
							         "「最小」に設定すると、最も重要な項目のみが含まれます。 「最大」に設定すると、すべての項目が含まれます。 " +
					                 "ログ・レベルについて詳しくは、資料を参照してください。",
					adminTooltip: "このログ・レベルは、管理ログ・ファイルでのメッセージのタイプを制御します。 " +
							      "このログ・ファイルには、${IMA_PRODUCTNAME_FULL} Web UI またはコマンド・ラインのいずれかから実行される管理コマンドに関するログ項目が示されます。 " +
							      "ここには、サーバーの構成に対する変更および試みられた変更がすべて記録されます。 " +
								  "「最小」に設定すると、最も重要な項目のみが含まれます。 " +
								  "「最大」に設定すると、すべての項目が含まれます。 " +
					              "ログ・レベルについて詳しくは、資料を参照してください。",
					mqconnectivityTooltip: "このログ・レベルは、MQ Connectivity ログ・ファイルでのメッセージのタイプを制御します。 " +
							               "このログ・ファイルには、MQ Connectivity の機能に関するログ項目が示されます。 " +
							               "これらの項目には、キュー・マネージャーへの接続時に検出された問題が含まれます。 " +
							               "このログを使用して、${IMA_PRODUCTNAME_FULL} を WebSphere<sup>&reg;</sup> MQ に接続する際の問題を診断できます。 " +
										   "「最小」に設定すると、最も重要な項目のみが含まれます。 " +
										   "「最大」に設定すると、すべての項目が含まれます。 " +
										   "ログ・レベルについて詳しくは、資料を参照してください。",										
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log										   
					retrieveLogLevelError: "{0}のレベルの取得中にエラーが発生しました。",
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log
					setLogLevelError: "{0}のレベルの設定中にエラーが発生しました。",
					levels: {
						MIN: "最小",
						NORMAL: "通常",
						MAX: "最大"
					},
					serverNotAvailable: "${IMA_PRODUCTNAME_FULL} サーバーの停止中はロギング・レベルを設定できません。",
					serverCleanStore: "ストア消去の進行中はロギング・レベルを設定できません。",
					serverSyncState: "1 次ノードの同期中はロギング・レベルを設定できません。",
					mqNotAvailable: "MQ Connectivity サービスの停止中は、MQ Connectivity ログ・レベルを設定できません。",
					mqStandbyState: "サーバーが待機中は MQ Connectivity ログ・レベルを設定できません。"
				}
			},
			highAvailability: {
				title: "高可用性",
				tagline: "2 つの ${IMA_PRODUCTNAME_SHORT} サーバーが、高可用性ノード・ペアとして動作するように構成します。",
				stateLabel: "現在の高可用性状況:",
				statePrimary: "プライマリー",
				stateStandby: "スタンバイ",
				stateIndeterminate: "不確定",
				error: "高可用性でエラーが発生しました",
				save: "保管",
				saving: "保管中...",
				success: "正常に保管されました。 変更を有効にするには、${IMA_PRODUCTNAME_SHORT} サーバーと MQ Connectivity サービスを再始動する必要があります。",
				saveFailed: "高可用性設定の更新中にエラーが発生しました。",
				ipAddrValidation : "IPv4 アドレスまたは IPv6 アドレスを指定します (例えば、192.168.1.0)。",
				discoverySameAsReplicationError: "ディスカバリー・アドレスは、複製アドレスと異なっている必要があります。",
				AutoDetect: "自動検出",
				StandAlone: "スタンドアロン",
				notSet: "未設定",
				yes: "はい",
				no: "いいえ",
				config: {
					title: "構成",
					tagline: "高可用性構成を示します。 " +
					         "構成の変更は、${IMA_PRODUCTNAME_SHORT} サーバーが再始動するまで有効になりません。",
					editLink: "編集",
					enabledLabel: "高可用性が有効:",
					haGroupLabel: "高可用性グループ:",
					startupModeLabel: "始動モード:",
					replicationInterfaceLabel: "複製アドレス:",
					discoveryInterfaceLabel: "ディスカバリー・アドレス:",
					remoteDiscoveryInterfaceLabel: "リモート・ディスカバリー・アドレス:",
					discoveryTimeoutLabel: "ディスカバリー・タイムアウト:",
					heartbeatLabel: "ハートビート・タイムアウト:",
					seconds: "{0} 秒",
					secondsDefault: "{0} 秒 (デフォルト)",
					preferedPrimary: "自動検出 (優先 1 次)"
				},
				restartInfoDialog: {
					title: "再起動が必要",
					content: "変更は保管されましたが、${IMA_PRODUCTNAME_SHORT} サーバーが再始動するまで有効になりません。<br /><br />" +
							 "${IMA_PRODUCTNAME_SHORT} サーバーをすぐに再始動する場合は、<b>「今すぐ再始動」</b>をクリックします。 そうでない場合は、<b>「後で再始動」</b>をクリックします。",
					closeButton: "後で再始動",
					restartButton: "今すぐ再始動",
					stopping: "${IMA_PRODUCTNAME_SHORT} サーバーの停止中です...",
					stoppingFailed: "${IMA_PRODUCTNAME_SHORT} サーバーの停止中にエラーが発生しました。 " +
								"「サーバー制御」ページを使用して、${IMA_PRODUCTNAME_SHORT} サーバーを再始動してください。",
					starting: "${IMA_PRODUCTNAME_SHORT} サーバーの始動中です..."
//					startingFailed: "An error occurred while starting the IBM IoT MessageSight server. " +
//								"Use the System Control page to start the IBM IoT MessageSight server."
				},
				confirmGroupNameDialog: {
					title: "グループ名変更の確認",
					content: "現在アクティブなノードは 1 つのみです。 グループ名を変更した場合、他のノードについてグループ名の手動による編集が必要なことがあります。 " +
					         "グループ名を変更しますか?",
					closeButton: "キャンセル",
					confirmButton: "確認"
				},
				dialog: {
					title: "高可用性構成の編集",
					saveButton: "保管",
					cancelButton: "キャンセル",
					instruction: "高可用性を有効にし、このサーバーとペアにするサーバーを決定するように高可用性グループを設定します。 " + 
								 "または、高可用性を無効にします。 高可用性の構成変更は、${IMA_PRODUCTNAME_SHORT} サーバーを再始動するまで有効になりません。",
					haEnabledLabel: "高可用性が有効:",
					startupMode: "始動モード:",
					haEnabledTooltip: "ツールチップ・テキスト",
					groupNameLabel: "高可用性グループ:",
					groupNameToolTip: "グループは、サーバーが互いにペアとなるよう自動的に構成するために使用します。 " +
					                  "この値は、両方のサーバーで同じでなければなりません。 " +
					                  "この値の最大長は 128 文字です。 ",
					advancedSettings: "高度な設定",
					advSettingsTagline: "高可用性構成に特定の設定が必要な場合、次の設定を手動で構成できます。 " +
										"これらの設定を変更する前に、${IMA_PRODUCTNAME_FULL} の資料で詳細な手順および推奨事項を確認してください。",
					nodePreferred: "両方のノードが自動検出モードで始動している場合、このノードが優先 1 次ノードです。",
				    localReplicationInterface: "ローカル複製アドレス:",
				    localDiscoveryInterface: "ローカル・ディスカバリー・アドレス:",
				    remoteDiscoveryInterface: "リモート・ディスカバリー・アドレス:",
				    discoveryTimeout: "ディスカバリー・タイムアウト:",
				    heartbeatTimeout: "ハートビート・タイムアウト:",
				    startModeTooltip: "ノードの始動時のモード。 ノードは、自動検出モードまたはスタンドアロン・モードのいずれかに設定できます。 " +
				    				  "自動検出モードでは、2 つのノードを始動する必要があります。 " +
				    				  "ノードは自動的に互いを検出して HA ペアを確立しようとします。 " +
				    				  "スタンドアロン・モードは、1 つのノードを始動する場合にのみ使用してください。",
				    localReplicationInterfaceTooltip: "HA ペアのローカル・ノードでの HA 複製に使用される NIC の IP アドレス。 " +
				    								  "割り当てる IP アドレスを選択できます。 " +
				    								  "ただし、ローカル複製アドレスは、ローカル・ディスカバリー・アドレスとは異なるサブネット上にある必要があります。",
				    localDiscoveryInterfaceTooltip: "HA ペアのローカル・ノードでの HA ディスカバリーに使用される NIC の IP アドレス。 " +
				    							    "割り当てる IP アドレスを選択できます。 " +
				    								"ただし、ローカル・ディスカバリー・アドレスは、ローカル複製アドレスとは異なるサブネット上にある必要があります。",
				    remoteDiscoveryInterfaceTooltip: "HA ペアのリモート・ノードでの HA ディスカバリーに使用される NIC の IP アドレス。 " +
				    								 "割り当てる IP アドレスを選択できます。 " +
				    								 "ただし、リモート・ディスカバリー・アドレスは、ローカル複製アドレスとは異なるサブネット上にある必要があります。",
				    discoveryTimeoutTooltip: "自動検出モードで始動したサーバーが、HA ペアのもう 1 つのサーバーにその時間内に接続する必要がある秒単位の時間。 " +
				    					     "有効範囲は 10 から 2147483647 です。 デフォルトは 600 です。 " +
				    						 "この時間内に接続が完了しない場合、サーバーは保守モードで始動します。",
				    heartbeatTimeoutTooltip: "HA ペアの他のサーバーに障害が発生したかをサーバーが判断する時間 (秒単位) です。 " + 
				    						 "有効範囲は 1 から 2147483647 です。 デフォルトは 10 です。 " +
				    						 "1 次サーバーがこの時間内にスタンバイ・サーバーからハートビートを受信しない場合、引き続き 1 次サーバーとして機能しますが、データ同期化処理は停止されます。 " +
				    						 "スタンバイ・サーバーがこの時間内に 1 次サーバーからハートビートを受信しない場合、スタンバイ・サーバーが 1 次サーバーになります。",
				    seconds: "秒数",
				    nicsInvalid: "ローカル複製アドレス、ローカル・ディスカバリー・アドレス、およびリモート・ディスカバリー・アドレスのすべてに、それぞれ有効な値を指定する必要があります。 " +
				    			 "あるいは、ローカル複製アドレス、ローカル・ディスカバリー・アドレス、およびリモート・ディスカバリー・アドレスのいずれにも値を指定しないという方法もあります。 ",
				    securityRiskTitle: "セキュリティー・リスク",
				    securityRiskText: "複製アドレスとディスカバリー・アドレスを指定せずに高可用性を有効にしますか? " +
				    		"サーバーのネットワーク環境を完全に制御していないと、サーバーの情報がマルチキャストされることになり、結果としてこのサーバーが、同じ高可用性グループ名を使用する信頼できないサーバーとペアを成す恐れがあります。" +
				    		"<br /><br />" +
				    		"複製アドレスとディスカバリー・アドレスを指定することにより、このセキュリティー・リスクを除去できます。"
				},
				status: {
					title: "状況",
					tagline: "高可用性の状況を示します。 " +
					         "状況が構成と一致しない場合、最新の構成変更を有効にするために ${IMA_PRODUCTNAME_SHORT} サーバーの再始動が必要な場合があります。",
					editLink: "編集",
					haLabel: "高可用性:",
					haGroupLabel: "高可用性グループ:",
					haNodesLabel: "アクティブ・ノード",
					pairedWithLabel: "最後のペア:",
					replicationInterfaceLabel: "複製アドレス:",
					discoveryInterfaceLabel: "ディスカバリー・アドレス:",
					remoteDiscoveryInterfaceLabel: "リモート・ディスカバリー・アドレス:",
					remoteWebUI: "<a href='{0}' target=_'blank'>リモート Web UI</a>",
					enabled: "高可用性が有効です。",
					disabled: "高可用性が無効です。"
				},
				startup: {
					subtitle: "ノード始動モード",
					tagline: "ノードは、自動検出モードまたはスタンドアロン・モードのいずれかに設定できます。 自動検出モードでは、2 つのノードを始動する必要があります。 " +
							 "ノードは自動的に互いを検出して、高可用性ペアを確立しようとします。 スタンドアロン・モードは、1 つのノードを始動する場合にのみ使用する必要があります。",
					mode: "始動モード:",
					modeAuto: "自動検出",
					modeAlone: "スタンドアロン",
					primary: "両方のノードが自動検出モードで始動している場合、このノードが優先 1 次ノードです。"
				},
				remote: {
					subtitle: "リモート・ノード・ディスカバリー・アドレス",
					tagline: "高可用性ペアのリモート・ノード上にあるディスカバリー・アドレスの IPv4 アドレスまたは IPv6 アドレス。",
					discoveryInt: "ディスカバリー・アドレス:",
					discoveryIntTooltip: "リモート・ノード・ディスカバリー・アドレスを構成するための IPv4 アドレスまたは IPv6 アドレスを入力します。"
				},
				local: {
					subtitle: "高可用性 NIC アドレス",
					tagline: "ローカル・ノードの複製アドレスおよびディスカバリー・アドレスの IPv4 アドレスまたは IPv6 アドレス。",
					replicationInt: "複製アドレス:",
					replicationIntTooltip: "ローカル・ノードの複製アドレスを構成するための IPv4 アドレスまたは IPv6 アドレスを入力します。",
					replicationError: "高可用性複製アドレスを使用できません。",
					discoveryInt: "ディスカバリー・アドレス:",
					discoveryIntTooltip: "ローカル・ノードのディスカバリー・アドレスを構成するための IPv4 アドレスまたは IPv6 アドレスを入力します。"
				},
				timeouts: {
					subtitle: "タイムアウト",
					tagline: "高可用性が有効なノードが自動検出モードで始動した場合、このノードは高可用性ペアのもう 1 つのノードと通信しようとします。 " +
							 "ディスカバリー・タイムアウト期間内に接続できなかった場合、ノードは代わりに保守モードで始動します。",
					tagline2: "ハートビート・タイムアウトは、高可用性ペアのもう 1 つのノードが失敗したかどうかを検出するために使用されます。 " +
							  "1 次ノードがスタンバイ・ノードからハートビートを受信しない場合、引き続き 1 次ノードとして機能しますが、データ同期化処理は停止されます。 " +
							  "スタンバイ・ノードが 1 次ノードからハートビートを受信しない場合、スタンバイ・ノードが 1 次ノードになります。",
					discoveryTimeout: "ディスカバリー・タイムアウト:",
					discoveryTimeoutTooltip: "ノードがペアのもう 1 つのノードへの接続を試行する 10 から 2,147,483,647 までの秒数。",
					discoveryTimeoutValidation: "10 から 2,147,483,647 の間の数値を入力してください。",
					discoveryTimeoutError: "ディスカバリー・タイムアウトを使用できません",
					heartbeatTimeout: "ハートビート・タイムアウト:",
					heartbeatTimeoutTooltip: "ノード・ハートビートの 1 から 2,147,483,647 までの秒数。",
					heartbeatTimeoutValidation: "1 から 2,147,483,647 の間の数値を入力してください。",
					seconds: "秒数"
				}
			},
			webUISecurity: {
				title: "Web UI 設定",
				tagline: "Web UI 設定の構成",
				certificate: {
					subtitle: "Web UI 証明書",
					tagline: "Web UI との通信を保護するために使用する証明書を構成します。"
				},
				timeout: {
					subtitle: "セッション・タイムアウトと LTPA トークンの有効期限",
					tagline: "これらの設定は、ブラウザーとサーバー間のアクティビティーおよび合計ログイン時間に基づいてユーザー・ログイン・セッションを制御します。 " +
							 "セッション非アクティブ・タイムアウトまたは LTPA トークンの有効期限を変更すると、アクティブなログイン・セッションが無効になる場合があります。",
					inactivity: "セッション非アクティブ・タイムアウト",
					inactivityTooltip: "セッション非アクティブ・タイムアウトは、ブラウザーとサーバー間にアクティビティーがない場合に、ユーザーがログイン状態を保持できる時間を制御します。 " +
							           "非アクティブ・タイムアウトは、LTPA トークンの有効期限以下である必要があります。",
					expiration: "LTPA トークンの有効期限",
					expirationTooltip: "LTPA トークンの有効期限は、ブラウザーとサーバー間にアクティビティーがあるかどうかにかかわらず、ユーザーがログイン状態を保持できる時間を制御します。 " +
							           "有効期限は、セッション非アクティブ・タイムアウト以上である必要があります。 詳しくは、資料を参照してください。", 
					unit: "分",
					save: "保管",
					saving: "保管中...",
					success: "正常に保管されました。 ログアウトしてから再度ログインしてください。",
					failed: "保管に失敗しました",
					invalid: {
						inactivity: "セッション非アクティブ・タイムアウトは、1 から LTPA トークンの有効期限値までの数値である必要があります。",
						expiration: "LTPA トークンの有効期限値は、セッション非アクティブ・タイムアウトから 1440 までの数値である必要があります。"
					},
					getExpirationError: "LTPA トークンの有効期限の取得中にエラーが発生しました。",
					getInactivityError: "セッション非アクティブ・タイムアウトの取得中にエラーが発生しました。",
					setExpirationError: "LTPA トークンの有効期限の設定中にエラーが発生しました。",
					setInactivityError: "セッション非アクティブ・タイムアウトの設定中にエラーが発生しました。"
				},
				cacheauth: {
					subtitle: "認証キャッシュ・タイムアウト",
					tagline: "認証キャッシュ・タイムアウトは、キャッシュ内の認証済みの資格情報が有効である時間を指定します。 " +
					"値を大きくすると、失効した資格情報がより長い時間キャッシュに保持されるため、セキュリティー・リスクが増大する恐れがあります。 " +
					"値を小さくすると、ユーザー・レジストリーにより頻繁にアクセスしなければならないため、パフォーマンスに影響する可能性があります。",
					cacheauth: "認証キャッシュ・タイムアウト",
					cacheauthTooltip: "認証キャッシュ・タイムアウトは、キャッシュ内の項目が削除されるまでの時間を示します。 " +
							"値は 1 から 3600 秒 (1 時間) までにする必要があります。 値は、LTPA トークンの有効期限値の 3 分の 1 以下である必要があります。 " +
							"詳しくは、資料を参照してください。", 
					unit: "秒",
					save: "保管",
					saving: "保管中...",
					success: "正常に保管されました",
					failed: "保管に失敗しました",
					invalid: {
						cacheauth: "認証キャッシュ・タイムアウトの値は、1 から 3600 までの数値である必要があります。"
					},
					getCacheauthError: "認証キャッシュ・タイムアウトの取得中にエラーが発生しました。",
					setCacheauthError: "認証キャッシュ・タイムアウトの設定中にエラーが発生しました。"
				},
				readTimeout: {
					subtitle: "HTTP 読み取りタイムアウト",
					tagline: "最初の読み取りが発生した後に、ソケットで読み取り要求が完了するのを待つ時間の最大の長さ。",
					readTimeout: "HTTP 読み取りタイムアウト",
					readTimeoutTooltip: "1 から 300 秒 (5 分) までの値。", 
					unit: "秒",
					save: "保管",
					saving: "保管中...",
					success: "正常に保管されました",
					failed: "保管に失敗しました",
					invalid: {
						readTimeout: "HTTP 読み取りタイムアウト値は、1 から 300 までの数値である必要があります。"
					},
					getReadTimeoutError: "HTTP 読み取りタイムアウトの取得中にエラーが発生しました。",
					setReadTimeoutError: "HTTP 読み取りタイムアウトの設定中にエラーが発生しました。"
				},
				port: {
					subtitle: "IP アドレスおよびポート",
					tagline: "Web UI 通信用の IP アドレスとポートを選択します。 このいずれかの値を変更した場合、新しい IP アドレスとポートでログインする必要が生じる場合があります。  " +
							 "<em>すべての IP アドレスを選択すると Web UI がインターネットに公開される可能性があるため、これはお勧めしません。</em>",
					ipAddress: "IP アドレス",
					all: "すべて",
					interfaceHint: "インターフェース: {0}",
					port: "ポート",
					save: "保管",
					saving: "保管中...",
					success: "正常に保管されました。 変更が有効になるまでに数分かかる場合があります。  ブラウザーを閉じ、再度ログインしてください。",
					failed: "保管に失敗しました",
					tooltip: {
						port: "使用可能なポートを入力してください。 有効なポートは、1 から 8999、9087、または 9100 から 65535 までです。"
					},
					invalid: {
						interfaceDown: "インターフェース ({0}) が停止中であることを報告しています。 インターフェースを確認するか、別のアドレスを選択してください。",
						interfaceUnknownState: "{0} の状態を照会中にエラーが発生しました。 インターフェースを確認するか、別のアドレスを選択してください。",
						port: "ポート番号は、1 から 8999、9087、または 9100 から 65535 までの数値である必要があります。"
					},					
					getIPAddressError: "IP アドレスの取得中にエラーが発生しました。",
					getPortError: "ポートの取得中にエラーが発生しました。",
					setIPAddressError: "IP アドレスの設定中にエラーが発生しました。",
					setPortError: "ポートの設定中にエラーが発生しました。"					
				},
				certificate: {
					subtitle: "セキュリティー証明書",
					tagline: "Web UI により使用される証明書をアップロードして置換します。",
					addButton: "証明書の追加",
					editButton: "証明書の編集",
					addTitle: "Web UI 証明書の置換",
					editTitle: "Web UI 証明書の編集",
					certificateLabel: "証明書:",
					keyLabel: "秘密鍵:",
					expiryLabel: "証明書の有効期限:",
					defaultExpiryDate: "警告: デフォルトの証明書が使用されており、これを置換する必要があります。",
					defaultCert: "デフォルト",
					success: "変更が有効になるまでに数分かかる場合があります。  ブラウザーを閉じ、再度ログインしてください。"
				},
				service: {
					subtitle: "サービス",
					tagline: "Web UI のサポート情報を生成するトレース・レベルを設定します。",
					traceEnabled: "トレースの有効化",
					traceSpecificationLabel: "トレース仕様",
					traceSpecificationTooltip: "IBM サポートにより提供されるトレース仕様ストリング",
				    save: "保管",
				    getTraceError: "トレース仕様の取得中にエラーが発生しました。",
				    setTraceError: "トレース仕様の設定中にエラーが発生しました。",
				    clearTraceError: "トレース仕様の消去中にエラーが発生しました。",
					saving: "保管中...",
					success: "正常に保管されました。",
					failed: "保管に失敗しました"
				}
			}
		},
        platformDataRetrieveError: "プラットフォーム・データの取得中にエラーが発生しました。",
        // Strings for client certs that can be used for certificate authentication
		clientCertsTitle: "クライアント証明書",
		clientCertsButton: "クライアント証明書",
        stopHover: "サーバーをシャットダウンし、サーバーへのすべてのアクセスを停止します。",
        restartLink: "サーバーの再始動",
		restartMaintOnLink: "保守の開始",
		restartMaintOnHover: "サーバーを再始動して保守モードで実行します。",
		restartMaintOffLink: "保守の停止",
		restartMaintOffHover: "サーバーを再始動して実動モードで実行します。",
		restartCleanStoreLink: "ストア消去",
		restartCleanStoreHover: "サーバー・ストアを消去し、サーバーを再始動します。",
        restartDialogTitle: "${IMA_PRODUCTNAME_SHORT} サーバーの再始動",
        restartMaintOnDialogTitle: "${IMA_PRODUCTNAME_SHORT} サーバーを保守モードで再始動します",
        restartMaintOffDialogTitle: "${IMA_PRODUCTNAME_SHORT} サーバーを実動モードで再始動します",
        restartCleanStoreDialogTitle: "${IMA_PRODUCTNAME_SHORT} サーバー・ストアの消去",
        restartDialogContent: "${IMA_PRODUCTNAME_SHORT} サーバーを再始動しますか?  サーバーを再始動すると、すべてのメッセージングが中断されます。",
        restartMaintOnDialogContent: "${IMA_PRODUCTNAME_SHORT} サーバーを保守モードで再始動しますか?  サーバーを保守モードで開始すると、すべてのメッセージングが停止されます。",
        restartMaintOffDialogContent: "${IMA_PRODUCTNAME_SHORT} サーバーを実動モードで再始動しますか?  サーバーを実動モードで開始すると、メッセージングが再始動されます。",
        restartCleanStoreDialogContent: "<strong>ストア消去</strong>アクションを実行すると、永続メッセージング・データが失われます。<br/><br/>" +
        	"<strong>ストア消去</strong>アクションを実行しますか?",
        restartOkButton: "再始動",        
        cleanOkButton: "ストアを消去して再始動",
        restarting: "${IMA_PRODUCTNAME_SHORT} サーバーの再始動中です...",
        restartingFailed: "${IMA_PRODUCTNAME_SHORT} サーバーの再始動中にエラーが発生しました。",
        remoteLogLevelTagLine: "サーバー・ログに表示するメッセージのレベルを設定します。  ", 
        errorRetrievingTrustedCertificates: "トラステッド証明書の取得中にエラーが発生しました。",

        // New page title and tagline:  Admin Endpoint
        adminEndpointPageTitle: "管理エンドポイント構成",
        adminEndpointPageTagline: "管理エンドポイントを構成し、構成ポリシーを作成します。 " +
        		"Web UI は、管理エンドポイントに接続して管理タスクおよびモニター・タスクを実行します。 " +
        		"構成ポリシーでは、ユーザーがどの管理タスクおよびモニター・タスクの実行を許可されるかを制御します。",
        // HA config strings
        haReplicationPortLabel: "複製ポート:",
		haExternalReplicationInterfaceLabel: "外部複製アドレス:",
		haExternalReplicationPortLabel: "外部複製ポート:",
        haRemoteDiscoveryPortLabel: "リモート・ディスカバリー・ポート:",
        haReplicationPortTooltip: "HA ペアのローカル・ノードでの HA 複製に使用されるポート番号。",
		haReplicationPortInvalid: "複製ポートが無効です。" +
				"有効範囲は 1024 から 65535 です。 デフォルトは 0 で、使用可能なポートを接続時にランダムに選択できます。",
		haExternalReplicationInterfaceTooltip: "HA 複製に使用される NIC の IPv4 アドレスまたは IPv6 アドレス。",
		haExternalReplicationPortTooltip: "HA ペアのローカル・ノードでの HA 複製に使用される外部ポート番号。",
		haExternalReplicationPortInvalid: "外部複製ポートが無効です。" +
				"有効範囲は 1024 から 65535 です。 デフォルトは 0 で、使用可能なポートを接続時にランダムに選択できます。",
        haRemoteDiscoveryPortTooltip: "HA ペアのリモート・ノードでの HA ディスカバリーに使用されるポート番号。",
		haRemoteDiscoveryPortInvalid: "リモート・ディスカバリー・ポートが無効です。" +
				 "有効範囲は 1024 から 65535 です。 デフォルトは 0 で、使用可能なポートを接続時にランダムに選択できます。",
		haReplicationAndDiscoveryBold: "複製アドレスおよびディスカバリー・アドレス ",
		haReplicationAndDiscoveryNote: "(1 つを設定したら、すべてを設定する必要があります)",
		haUseSecuredConnectionsLabel: "セキュア接続の使用:",
		haUseSecuredConnectionsTooltip: "HA ペアのノード間でディスカバリーおよびデータ・チャネルに TLS を使用します。",
		haUseSecuredConnectionsEnabled: "セキュア接続の使用が有効です。",
		haUseSecuredConnectionsDisabled: "セキュア接続の使用が無効です。",
		mqConnectivityEnableLabel: "MQ Connectivity の使用可能化",
		savingProgress: "保管中...",
	    setMqConnEnabledError: "MQConnectivity 使用可能状態の設定中にエラーが発生しました。",
	    allowCertfileOverwriteLabel: "上書き",
		allowCertfileOverwriteTooltip: "証明書と秘密鍵の上書きを許可します。",
		stopServerWarningTitle: "停止すると、Web UI による再始動が行われません。",
		stopServerWarning: "サーバーを停止すると、すべてのメッセージングと、REST 要求および Web UI によるすべての管理アクセスが停止されます。 <b>Web UI からサーバーへのアクセスを失わないようにするには、「サーバーの再始動」リンクを使用してください。</b>",
		// New Web UI user dialog strings to allow French translation to insert a space before colon
		webuiuserLabel: "ユーザー ID:",
		webuiuserdescLabel: "説明:",
		webuiuserroleLabel: "ロール:"

});
