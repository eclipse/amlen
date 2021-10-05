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
			title : "メッセージング",
			users: {
				title: "ユーザー認証",
				tagline: "メッセージング・サーバー・ユーザーを認証するように LDAP を構成します。"
			},
			endpoints: {
				title: "エンドポイント構成",
				listenersSubTitle: "エンドポイント",
				endpointsSubTitle: "エンドポイントの定義",
		 		form: {
					enabled: "使用可能",
					name: "名前",
					description: "説明",
					port: "ポート",
					ipAddr: "IP アドレス",
					all: "すべて",
					protocol: "プロトコル",
					security: "セキュリティー",
					none: "なし",
					securityProfile: "セキュリティー・プロファイル",
					connectionPolicies: "接続ポリシー",
					connectionPolicy: "接続ポリシー",
					messagingPolicies: "メッセージング・ポリシー",
					messagingPolicy: "メッセージング・ポリシー",
					destinationType: "宛先タイプ",
					destination: "宛先",
					maxMessageSize: "最大メッセージ・サイズ",
					selectProtocol: "プロトコルの選択",
					add: "追加",
					tooltip: {
						description: "",
						port: "使用可能なポートを入力してください。 有効なポートは 1 から 65535 までです。",
						connectionPolicies: "接続ポリシーを少なくとも 1 つ追加してください。 接続ポリシーは、エンドポイントに接続する権限をクライアントに付与します。 " +
								"接続ポリシーは、示された順序で評価されます。 順序を変更するには、ツールバーの上矢印と下矢印を使用します。",
						messagingPolicies: "メッセージング・ポリシーを少なくとも 1 つ追加してください。 " +
								"メッセージング・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのトピックまたはキューにアクセスできるかを決定します。 " +
								"メッセージング・ポリシーは、示された順序で評価されます。 順序を変更するには、ツールバーの上矢印と下矢印を使用します。",									
						maxMessageSize: "最大メッセージ・サイズ (KB 単位)。 有効な値は 1 から 262,144 までです。",
						protocol: "このエンドポイントの有効なプロトコルを指定してください。"
					},
					invalid: {						
						port: "ポート番号は、1 から 65535 までの数値でなければなりません。",
						maxMessageSize: "最大メッセージ・サイズは、1 から 262,144 までの数値でなければなりません。",
						ipAddr: "有効な IP アドレスが必要です。",
						security: "値が必要です。"
					},
					duplicateName: "その名前のレコードはすでに存在します。"
				},
				addDialog: {
					title: "エンドポイントの追加",
					instruction: "エンドポイントは、クライアント・アプリケーションが接続できるポートです。  エンドポイントには、少なくとも 1 つの接続ポリシーと 1 つのメッセージング・ポリシーが必要です。"
				},
				removeDialog: {
					title: "エンドポイントの削除",
					content: "このレコードを削除してもよろしいですか?"
				},
				editDialog: {
					title: "エンドポイントの編集",
					instruction: "エンドポイントは、クライアント・アプリケーションが接続できるポートです。  エンドポイントには、少なくとも 1 つの接続ポリシーと 1 つのメッセージング・ポリシーが必要です。"
				},
				addProtocolsDialog: {
					title: "エンドポイントへのプロトコルの追加",
					instruction: "このエンドポイントへの接続を許可されるプロトコルを追加します。 プロトコルを少なくとも 1 つ選択する必要があります。",
					allProtocolsCheckbox: "このエンドポイントではすべてのプロトコルが有効です。",
					protocolSelectErrorTitle: "プロトコルが選択されていません。",
					protocolSelectErrorMsg: "プロトコルを少なくとも 1 つ選択する必要があります。 すべてのプロトコルを追加することを指定するか、プロトコル・リストから特定のプロトコルを選択してください。"
				},
				addConnectionPoliciesDialog: {
					title: "エンドポイントへの接続ポリシーの追加",
					instruction: "接続ポリシーは、${IMA_PRODUCTNAME_FULL} エンドポイントに接続する権限をクライアントに付与します。 " +
						"各エンドポイントには、少なくとも 1 つの接続ポリシーが必要です。"
				},
				addMessagingPoliciesDialog: {
					title: "エンドポイントへのメッセージング・ポリシーの追加",
					instruction: "メッセージング・ポリシーを使用することにより、${IMA_PRODUCTNAME_FULL} 上でクライアントがアクセスできるトピック、キュー、またはグローバル共有サブスクリプションを制御できます。 " +
							"各エンドポイントには、少なくとも 1 つのメッセージ・ポリシーが必要です。 " +
							"グローバル共有サブスクリプションのためのメッセージング・ポリシーがエンドポイントにある場合は、サブスクライブされるトピックのためのメッセージング・ポリシーも必要です。"
				},
                retrieveEndpointError: "エンドポイントの取得中にエラーが発生しました。",
                saveEndpointError: "エンドポイントの保管中にエラーが発生しました。",
                deleteEndpointError: "エンドポイントの削除中にエラーが発生しました。",
                retrieveSecurityProfilesError: "セキュリティー・ポリシーの取得中にエラーが発生しました。"
			},
			connectionPolicies: {
				title: "接続ポリシー",
				grid: {
					applied: "適用済み",
					name: "名前"
				},
		 		dialog: {
					name: "名前",
					protocol: "プロトコル",
					description: "説明",
					clientIP: "クライアント IP アドレス",  
					clientID: "クライアント ID",
					ID: "ユーザー ID",
					Group: "グループ ID",
					selectProtocol: "プロトコルの選択",
					commonName: "証明書共通名",
					protocol: "プロトコル",
					tooltip: {
						name: "",
						filter: "フィルター・フィールドを少なくとも 1 つ指定する必要があります。 " +
								"ほとんどのフィルターでは、0 個以上の文字にマッチングする単一のアスタリスク (*) を最後の文字に使用することができます。 " +
								"クライアント IP アドレスには、アスタリスク (*) か、IPv4 または IPv6 のアドレス (範囲指定も可能) のコンマ区切りリストを使用できます (例: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100)。 " +
								"IPv6 アドレスは大括弧で囲む必要があります (例: [2001:db8::])。"
					},
					invalid: {						
						filter: "フィルター・フィールドを少なくとも 1 つ指定する必要があります。",
						wildcard: "0 個以上の文字にマッチングする単一のアスタリスク (*) を値の終わりに使用することができます。",
						vars: "置換変数 ${UserID}、${GroupID}、${ClientID}、${CommonName} を含めてはなりません。",
						clientIDvars: "置換変数 ${GroupID} と ${ClientID} はいずれも含めてはなりません。",
						clientIP: "クライアント IP アドレスには、アスタリスク (*) か、IPv4 または IPv6 のアドレス (範囲指定も可能) のコンマ区切りリストが含まれていなければなりません (例: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100)。 " +
								"IPv6 アドレスは大括弧で囲む必要があります (例: [2001:db8::])。"
					}										
				},
				addDialog: {
					title: "接続ポリシーの追加",
					instruction: "接続ポリシーは、${IMA_PRODUCTNAME_FULL} エンドポイントに接続する権限をクライアントに付与します。 各エンドポイントには、少なくとも 1 つの接続ポリシーが必要です。"
				},
				removeDialog: {
					title: "接続ポリシーの削除",
					instruction: "接続ポリシーは、${IMA_PRODUCTNAME_FULL} エンドポイントに接続する権限をクライアントに付与します。 各エンドポイントには、少なくとも 1 つの接続ポリシーが必要です。",
					content: "このレコードを削除してもよろしいですか?"
				},
                removeNotAllowedDialog: {
                	title: "削除は許可されていません",
                	content: "接続ポリシーを削除できません。1 つ以上のエンドポイントによって使用されているためです。 " +
                			 "接続ポリシーをすべてのエンドポイントから削除してから、再試行してください。",
                	closeButton: "閉じる"
                },								
				editDialog: {
					title: "接続ポリシーの編集",
					instruction: "接続ポリシーは、${IMA_PRODUCTNAME_FULL} エンドポイントに接続する権限をクライアントに付与します。 各エンドポイントには、少なくとも 1 つの接続ポリシーが必要です。"
				},
                retrieveConnectionPolicyError: "接続ポリシーの取得中にエラーが発生しました。",
                saveConnectionPolicyError: "接続ポリシーの保管中にエラーが発生しました。",
                deleteConnectionPolicyError: "接続ポリシーの削除中にエラーが発生しました。"
 			},
			messagingPolicies: {
				title: "メッセージング・ポリシー",
				listSeparator : ",",
		 		dialog: {
					name: "名前",
					description: "説明",
					destinationType: "宛先タイプ",
					destinationTypeOptions: {
						topic: "トピック",
						subscription: "グローバル共有サブスクリプション",
						queue: "キュー"
					},
					topic: "トピック",
					queue: "キュー",
					selectProtocol: "プロトコルの選択",
					destination: "宛先",
					maxMessages: "最大メッセージ数",
					maxMessagesBehavior: "最大メッセージ数の動作",
					maxMessagesBehaviorOptions: {
						RejectNewMessages: "新しいメッセージを拒否する",
						DiscardOldMessages: "古いメッセージを廃棄する"
					},
					action: "権限",
					actionOptions: {
						publish: "パブリッシュ",
						subscribe: "サブスクライブ",
						send: "送信",
						browse: "参照",
						receive: "受信",
						control: "制御"
					},
					clientIP: "クライアント IP アドレス",  
					clientID: "クライアント ID",
					ID: "ユーザー ID",
					Group: "グループ ID",
					commonName: "証明書共通名",
					protocol: "プロトコル",
					disconnectedClientNotification: "切断済みクライアント通知",
					subscriberSettings: "サブスクライバー設定",
					publisherSettings: "パブリッシャー設定",
					senderSettings: "送信側設定",
					maxMessageTimeToLive: "メッセージ最大存続時間",
					unlimited: "制限なし",
					unit: "秒",
					// TRNOTE {0} is formatted an integer number.
					maxMessageTimeToLiveValueString: "{0} 秒",
					tooltip: {
						name: "",
						destination: "このポリシーが適用されるメッセージ・トピック、キュー、またはグローバル共有サブスクリプション。 アスタリスク (*) は慎重に使用してください。 " +
							"アスタリスクは 0 個以上の文字 (スラッシュを含む) とマッチングします。 そのため、トピック・ツリー内の複数のレベルにマッチングする可能性があります。",
						maxMessages: "サブスクリプションのために保持するメッセージの最大数。 1 から 20,000,000 までの数値でなければなりません。",
						maxMessagesBehavior: "サブスクリプション用バッファーが満杯になったときに適用される動作。 " +
								"つまり、サブスクリプション用バッファー内のメッセージ数が「最大メッセージ数」の値に達したときの動作です。<br />" +
								"<strong>新しいメッセージを拒否する:&nbsp;</strong>  バッファーが満杯のとき、新しいメッセージは拒否されます。<br />" +
								"<strong>古いメッセージを廃棄する:&nbsp;</strong>  バッファーが満杯のときに新しいメッセージが到着すると、最も古い未送信メッセージが廃棄されます。",
						discardOldestMessages: "この設定では、パブリッシャーが送達の確認応答を受け取る場合でも、一部のメッセージがサブスクライバーに送達されない可能性があります。 " +
								"パブリッシャーとサブスクライバーの両方が高信頼性メッセージングを要求した場合でも、メッセージは廃棄されます。",
						action: "このポリシーが許可するアクション",
						filter: "フィルター・フィールドを少なくとも 1 つ指定する必要があります。 " +
								"ほとんどのフィルターでは、0 個以上の文字にマッチングする単一のアスタリスク (*) を最後の文字に使用することができます。 " +
								"クライアント IP アドレスには、アスタリスク (*) か、IPv4 または IPv6 のアドレス (範囲指定も可能) のコンマ区切りリストを使用できます (例: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100)。 " +
								"IPv6 アドレスは大括弧で囲む必要があります (例: [2001:db8::])。",
				        disconnectedClientNotification: "メッセージの到着時に、接続されていない MQTT クライアントに対して通知メッセージを発行するかどうかを指定します。 " +
				        		"通知は MQTT CleanSession=0 が設定されているクライアントに対してのみ使用可能です。",
				        protocol: "メッセージング・ポリシーのプロトコルのフィルタリングを使用可能にします。 使用可能にした場合は、プロトコルを 1 つ以上指定する必要があります。 使用可能にしない場合、メッセージング・ポリシーですべてのプロトコルが許可されます。",
				        destinationType: "アクセスを許可する宛先のタイプ。 " +
				        		"グローバル共有サブスクリプションへのアクセスを許可するには、次の 2 つのメッセージング・ポリシーが必要です。"+
				        		"<ul><li>宛先タイプが<em>トピック</em> であるメッセージング・ポリシー。1 つ以上のトピックへのアクセスを許可します。</li>" +
				        		"<li>宛先タイプが<em>グローバル共有サブスクリプション</em> であるメッセージング・ポリシー。それらのトピックに対するグローバル共有の永続サブスクリプションへのアクセスを許可します。</li></ul>",
						action: {
							topic: "<dl><dt>パブリッシュ:</dt><dd>メッセージング・ポリシーで指定されたトピックにクライアントがメッセージをパブリッシュすることを許可します。</dd>" +
							       "<dt>サブスクライブ:</dt><dd>メッセージング・ポリシーに指定されたトピックに、クライアントがサブスクライブできるようにします。</dd></dl>",
							queue: "<dl><dt>送信:</dt><dd>メッセージング・ポリシーに指定されたキューに、クライアントがメッセージを送信できるようにします。</dd>" +
								    "<dt>参照:</dt><dd>メッセージング・ポリシーで指定されたキューをクライアントが参照することを許可します。</dd>" +
								    "<dt>受信:</dt><dd>メッセージング・ポリシーに指定されたキューから、クライアントがメッセージを受信できるようにします。</dd></dl>",
							subscription:  "<dl><dt>受信:</dt><dd>メッセージング・ポリシーで指定されたグローバル共有サブスクリプションからクライアントがメッセージを受信することを許可します。</dd>" +
									"<dt>制御:</dt><dd>メッセージング・ポリシーで指定されたグローバル共有サブスクリプションをクライアントが作成および削除することを許可します。</dd></dl>"
							},
						maxMessageTimeToLive: "このポリシーを使用してパブリッシュされたメッセージが存続できる最大秒数。 " +
								"パブリッシャーで指定した有効期限値の方がこれより小さい場合は、パブリッシャーの値が使用されます。 " +
								"有効な値は、<em>制限なし</em> または 1 から 2,147,483,647 秒までです。 値を<em>制限なし</em> にすると、最大が設定されません。",
						maxMessageTimeToLiveSender: "このポリシーを使用して送信されたメッセージが存続できる最大秒数。 " +
								"送信側で指定した有効期限値の方がこれより小さい場合は、送信側の値が使用されます。 " +
								"有効な値は、<em>制限なし</em> または 1 から 2,147,483,647 秒までです。 値を<em>制限なし</em> にすると、最大が設定されません。"
					},
					invalid: {						
						maxMessages: "1 から 20,000,000 までの数値でなければなりません。",                       
						filter: "フィルター・フィールドを少なくとも 1 つ指定する必要があります。",
						wildcard: "0 個以上の文字にマッチングする単一のアスタリスク (*) を値の終わりに使用することができます。",
						vars: "置換変数 ${UserID}、${GroupID}、${ClientID}、${CommonName} を含めてはなりません。",
						clientIP: "クライアント IP アドレスには、アスタリスク (*) か、IPv4 または IPv6 のアドレス (範囲指定も可能) のコンマ区切りリストが含まれていなければなりません (例: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100)。 " +
								  "IPv6 アドレスは大括弧で囲む必要があります (例: [2001:db8::])。",
					    subDestination: "宛先タイプが<em>グローバル共有サブスクリプション</em> の場合、クライアント ID の変数置換は許可されません。",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    protocol: "プロトコル {0} は、宛先タイプ {1} には無効です。",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    destinationType: "プロトコル・フィルターとして指定されたプロトコル {0} は、宛先タイプ {1} には無効です。",
					    maxMessageTimeToLive: "<em>制限なし</em> か、1 から 2,147,483,647 までの数値でなければなりません。"
					}					
				},
				addDialog: {
					title: "メッセージング・ポリシーの追加",
					instruction: "メッセージング・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのトピック、キュー、またはグローバル共有サブスクリプションにアクセスできるかを決定します。 " +
							     "グローバル共有サブスクリプションでは、永続トピック・サブスクリプションからメッセージを受け取る処理が、複数のサブスクライバー間で共有されます。 各エンドポイントには、少なくとも 1 つのメッセージ・ポリシーが必要です。"
				},
				removeDialog: {
					title: "メッセージング・ポリシーの削除",
					instruction: "メッセージング・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのトピック、キュー、またはグローバル共有サブスクリプションにアクセスできるかを決定します。 " +
							"グローバル共有サブスクリプションでは、永続トピック・サブスクリプションからメッセージを受け取る処理が、複数のサブスクライバー間で共有されます。 各エンドポイントには、少なくとも 1 つのメッセージ・ポリシーが必要です。",
					content: "このレコードを削除してもよろしいですか?"
				},
                removeNotAllowedDialog: {
                	title: "削除は許可されていません",
                	// TRNOTE: {0} is the messaging policy type where the value can be topic, queue or subscription.
                	content: "{0} ポリシーを削除できません。1 つ以上のエンドポイントによって使用されているためです。 " +
                			 "{0} ポリシーをすべてのエンドポイントから削除してから、再試行してください。",
                	closeButton: "閉じる"
                },				
				editDialog: {
					title: "メッセージング・ポリシーの編集",
					instruction: "メッセージング・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのトピック、キュー、またはグローバル共有サブスクリプションにアクセスできるかを決定します。 " +
							     "グローバル共有サブスクリプションでは、永続トピック・サブスクリプションからメッセージを受け取る処理が、複数のサブスクライバー間で共有されます。 各エンドポイントには、少なくとも 1 つのメッセージ・ポリシーが必要です。 "
				},
				viewDialog: {
					title: "メッセージング・ポリシーの表示"
				},		
				confirmSaveDialog: {
					title: "メッセージング・ポリシーの保管",
					// TRNOTE: {0} is a numeric count, {1} is a table of properties and values.
					content: "このポリシーは、約 {0} 個のサブスクライバーまたはプロデューサーで使用中です。 " +
							"このポリシーによって許可されたクライアントは、次の新規設定を使用します: {1}" +
							"このポリシーを変更しますか?",
					// TRNOTE: {1} is a table of properties and values.							
					contentUnknown: "このポリシーは、サブスクライバーまたはプロデューサーで使用中の可能性があります。 " +
							"このポリシーによって許可されたクライアントは、次の新規設定を使用します: {1}" +
							"このポリシーを変更しますか?",
					saveButton: "保管",
					closeButton: "キャンセル"
				},
                retrieveMessagingPolicyError: "メッセージング・ポリシーの取得中にエラーが発生しました。",
                retrieveOneMessagingPolicyError: "メッセージング・ポリシーの取得中にエラーが発生しました。",
                saveMessagingPolicyError: "メッセージング・ポリシーの保管中にエラーが発生しました。",
                deleteMessagingPolicyError: "メッセージング・ポリシーの削除中にエラーが発生しました。",
                pendingDeletionMessage:  "このポリシーは削除保留中です。  使用されなくなった時点で削除されます。",
                tooltip: {
                	discardOldestMessages: "「最大メッセージ数の動作」が<em>古いメッセージを廃棄する</em> に設定されています。 " +
                			"この設定では、パブリッシャーが送達の確認応答を受け取る場合でも、一部のメッセージがサブスクライバーに送達されない可能性があります。 " +
                			"パブリッシャーとサブスクライバーの両方が高信頼性メッセージングを要求した場合でも、メッセージは廃棄されます。"
                }
			},
			messageProtocols: {
				title: "メッセージング・プロトコル",
				subtitle: "メッセージング・プロトコルは、クライアントと ${IMA_PRODUCTNAME_FULL} の間のメッセージの送信に使用されます。",
				tagline: "使用可能なメッセージング・プロトコルとそれらの機能。",
				messageProtocolsList: {
					name: "プロトコル名",
					topic: "トピック",
					shared: "グローバル共有サブスクリプション",
					queue: "キュー",
					browse: "参照"
				}
			},
			messageHubs: {
				title: "メッセージング・ハブ",
				subtitle: "システム管理者とメッセージング管理者は、メッセージング・ハブを定義、編集、または削除することができます。 " +
						  "メッセージング・ハブは、関連したエンドポイント、接続ポリシー、およびメッセージング・ポリシーをグループ化した組織構成オブジェクトです。 <br /><br />" +
						  "エンドポイントは、クライアント・アプリケーションが接続できるポートです。 エンドポイントには、少なくとも 1 つの接続ポリシーと 1 つのメッセージング・ポリシーが必要です。 " +
						  "接続ポリシーは、エンドポイントに接続する権限をクライアントに付与します。これに対してメッセージング・ポリシーは、いったんエンドポイントに接続されたクライアントに、特定のメッセージング・アクションの権限を付与します。 ",
				tagline: "メッセージング・ハブを定義、編集、または削除します。",
				defineAMessageHub: "メッセージング・ハブの定義",
				editAMessageHub: "メッセージング・ハブの編集",
				defineAnEndpoint: "エンドポイントの定義",
				endpointTabTagline: "エンドポイントは、クライアント・アプリケーションが接続できるポートです。  " +
						"エンドポイントには、少なくとも 1 つの接続ポリシーと 1 つのメッセージング・ポリシーが必要です。",
				messagingPolicyTabTagline: "メッセージング・ポリシーを使用することにより、${IMA_PRODUCTNAME_FULL} 上でクライアントがアクセスできるトピック、キュー、またはグローバル共有サブスクリプションを制御できます。 " +
						"各エンドポイントには、少なくとも 1 つのメッセージ・ポリシーが必要です。",
				connectionPolicyTabTagline: "接続ポリシーは、${IMA_PRODUCTNAME_FULL} エンドポイントに接続する権限をクライアントに付与します。 " +
						"各エンドポイントには、少なくとも 1 つの接続ポリシーが必要です。",						
                retrieveMessageHubsError: "メッセージング・ハブの取得中にエラーが発生しました。",
                saveMessageHubError: "メッセージング・ハブの保管中にエラーが発生しました。",
                deleteMessageHubError: "メッセージング・ハブの削除中にエラーが発生しました。",
                messageHubNotFoundError: "メッセージング・ハブが見つかりません。 別のユーザーによって削除された可能性があります。",
                removeNotAllowedDialog: {
                	title: "削除は許可されていません",
                	content: "メッセージング・ハブを削除できません。エンドポイントが 1 つ以上含まれているためです。 " +
                			 "メッセージング・ハブを編集してエンドポイントを削除してから、再試行してください。",
                	closeButton: "閉じる"
                },
                addDialog: {
                	title: "メッセージング・ハブの追加",
                	instruction: "サーバー接続を管理するためのメッセージング・ハブを定義します。",
                	saveButton: "保管",
                	cancelButton: "キャンセル",
                	name: "名前:",
                	description: "説明:"
                },
                editDialog: {
                	title: "メッセージング・ハブ・プロパティーの編集",
                	instruction: "メッセージング・ハブの名前と説明を編集します。"
                },                
				messageHubList: {
					name: "メッセージング・ハブ",
					description: "説明",
					metricLabel: "エンドポイント",
					removeDialog: {
						title: "メッセージング・ハブの削除",
						content: "このメッセージング・ハブを削除してもよろしいですか?"
					}
				},
				endpointList: {
					name: "エンドポイント",
					description: "説明",
					connectionPolicies: "接続ポリシー",
					messagingPolicies: "メッセージング・ポリシー",
					port: "ポート",
					enabled: "使用可能",
					status: "状況",
					up: "上へ",
					down: "下へ",
					removeDialog: {
						title: "エンドポイントの削除",
						content: "このエンドポイントを削除してもよろしいですか?"
					}
				},
				messagingPolicyList: {
					name: "メッセージング・ポリシー",
					description: "説明",					
					metricLabel: "エンドポイント",
					destinationLabel: "宛先",
					maxMessagesLabel: "最大メッセージ数",
					disconnectedClientNotificationLabel: "切断済みクライアント通知",
					actionsLabel: "権限",
					useCountLabel: "使用数",
					unknown: "不明",
					removeDialog: {
						title: "メッセージング・ポリシーの削除",
						content: "このメッセージング・ポリシーを削除してもよろしいですか?"
					},
					deletePendingDialog: {
						title: "削除保留中",
						content: "削除要求を受信しましたが、現時点ではポリシーを削除できません。 " +
							"ポリシーは、約 {0} 個のサブスクライバーまたはプロデューサーで使用中です。 " +
							"ポリシーは、使用されなくなった時点で削除されます。",
						contentUnknown: "削除要求を受信しましたが、現時点ではポリシーを削除できません。 " +
						"ポリシーは、サブスクライバーまたはプロデューサーで使用中の可能性があります。 " +
						"ポリシーは、使用されなくなった時点で削除されます。",
						closeButton: "閉じる"
					},
					deletePendingTooltip: "このポリシーは、使用されなくなった時点で削除されます。"
				},	
				connectionPolicyList: {
					name: "接続ポリシー",
					description: "説明",					
					endpoints: "エンドポイント",
					removeDialog: {
						title: "接続ポリシーの削除",
						content: "この接続ポリシーを削除してもよろしいですか?"
					}
				},				
				messageHubDetails: {
					backLinkText: "メッセージング・ハブに戻る",
					editLink: "編集",
					endpointsTitle: "エンドポイント",
					endpointsTip: "このメッセージング・ハブのエンドポイントと接続ポリシーを構成",
					messagingPoliciesTitle: "メッセージング・ポリシー",
					messagingPoliciesTip: "このメッセージング・ハブのメッセージング・ポリシーを構成",
					connectionPoliciesTitle: "接続ポリシー",
					connectionPoliciesTip: "このメッセージング・ハブの接続ポリシーを構成"
				},
				hovercard: {
					name: "名前",
					description: "説明",
					endpoints: "エンドポイント",
					connectionPolicies: "接続ポリシー",
					messagingPolicies: "メッセージング・ポリシー",
					warning: "警告"
				}
			},
			referencedObjectGrid: {
				applied: "適用済み",
				name: "名前"
			},
			savingProgress: "保管中...",
			savingFailed: "保管に失敗しました。",
			deletingProgress: "削除中...",
			deletingFailed: "削除に失敗しました。",
			refreshingGridMessage: "更新中...",
			noItemsGridMessage: "表示する項目がありません",
			testing: "テスト中...",
			testTimedOut: "テスト接続の完了に時間がかかりすぎています。",
			testConnectionFailed: "テスト接続に失敗しました",
			testConnectionSuccess: "テスト接続に成功しました。",
			dialog: {
				saveButton: "保管",
				deleteButton: "削除",
				deleteContent: "このレコードを削除してもよろしいですか?",
				cancelButton: "キャンセル",
				closeButton: "閉じる",
				testButton: "接続のテスト",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingContent: "{0} への接続をテストしています...",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingFailedContent: "{0} への接続のテストが失敗しました。",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingSuccessContent: "{0} への接続のテストが成功しました。"
			},
			updating: "更新中...",
			invalidName: "その名前のレコードはすでに存在します。",
			filterHeadingConnection: "このポリシーを使用して接続を特定のクライアントに制限するには、以下のフィルターを 1 つ以上指定します。 " +
					"例えば、このポリシーを特定のグループのメンバーに制限するには、<em>グループ ID</em> を選択します。 " +
					"このポリシーは、指定されたフィルターのすべてに当てはまる場合にのみアクセスが許可されます。",
			filterHeadingMessaging: "このポリシーで定義されたメッセージング・アクションを特定のクライアントに制限するには、以下のフィルターを 1 つ以上指定します。 " +
					"例えば、このポリシーを特定のグループのメンバーに制限するには、<em>グループ ID</em> を選択します。 " +
					"このポリシーは、指定されたフィルターのすべてに当てはまる場合にのみアクセスが許可されます。",
			settingsHeadingMessaging: "クライアントがアクセスすることを許可されるリソースとメッセージング・アクションを指定します:",
			mqConnectivity: {
				title: "MQ Connectivity",
				subtitle: "1 つ以上の WebSphere MQ キュー・マネージャーへの接続を構成します。"				
			},
			connectionProperties: {
				title: "キュー・マネージャー接続プロパティー",
				tagline: "サーバーがキュー・マネージャーに接続する方法に関する情報を、定義、編集、または削除することができます。",
				retrieveError: "キュー・マネージャー接続プロパティーの取得中にエラーが発生しました。",
				grid: {
					name: "名前",
					qmgr: "キュー・マネージャー",
					connName: "接続名",
					channel: "チャネル名",
					description: "説明",
					SSLCipherSpec: "SSL CipherSpec",
					status: "状況"
				},
				dialog: {
					instruction: "MQ との接続には以下のキュー・マネージャー接続の詳細が必要になります。",
					nameInvalid: "名前の先頭および末尾をスペースにすることはできません",
					connTooltip: "IP アドレスまたはホスト名とポート番号の形式の接続名 (例えば 224.0.138.177(1414)) のコンマ区切りリストを入力してください",
					connInvalid: "接続名に無効文字が含まれています",
					qmInvalid: "キュー・マネージャー名で使用できるのは、文字、数字、および 4 つの特殊文字 . _ % / のみです",
					qmTooltip: "文字、数字、および 4 つの特殊文字 . _ % / のみで構成されたキュー・マネージャー名を入力してください",
				    channelTooltip: "文字、数字、および 4 つの特殊文字 . _ % / のみで構成されたチャネル名を入力してください",
					channelInvalid: "チャネル名で使用できるのは、文字、数字、および 4 つの特殊文字 . _ % / のみです",
					activeRuleTitle: "編集は許可されていません",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailOne: "キュー・マネージャー接続を編集できません。使用可能になっている宛先マッピング・ルール {0} によって使用されているためです。",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailMany: "キュー・マネージャー接続を編集できません。使用可能になっている次の宛先マッピング・ルールによって使用されているためです: {0}。",
					SSLCipherSpecTooltip: "SSL 接続の暗号仕様を入力してください (最大長 32 文字)"
				},
				addDialog: {
					title: "キュー・マネージャー接続の追加"
				},
				removeDialog: {
					title: "キュー・マネージャー接続の削除",
					content: "このレコードを削除してもよろしいですか?"
				},
				editDialog: {
					title: "キュー・マネージャー接続の編集"
				},
                removeNotAllowedDialog: {
                	title: "削除は許可されていません",
					// TRNOTE {0} is a user provided name of a rule.
                	contentOne: "キュー・マネージャー接続を削除できません。宛先マッピング・ルール {0} によって使用されているためです。",
					// TRNOTE {0} is a list of user provided names of rules.
                	contentMany: "キュー・マネージャー接続を削除できません。次の宛先マッピング・ルールによって使用されているためです: {0}。",
                	closeButton: "閉じる"
                }
			},
			destinationMapping: {
				title: "宛先マッピング・ルール",
				tagline: "システム管理者とメッセージング管理者は、キュー・マネージャーとの間で転送されるメッセージを規定するルールを、定義、編集、または削除することができます。",
				note: "ルールを削除するには、その前にルールを使用不可に設定しておく必要があります。",
				retrieveError: "宛先マッピング・ルールの取得中にエラーが発生しました。",
				disableRulesConfirmationDialog: {
					text: "このルールを使用不可に設定してもよろしいですか?",
					info: "ルールを使用不可に設定するとルールが停止します。その結果、バッファーに入れられたメッセージと現在送信中のメッセージが失われることになります。",
					buttonLabel: "ルールの使用不可化"
				},
				leadingBlankConfirmationDialog: {
					textSource: "<em>ソース</em> に先行ブランクがあります。 先行ブランクのあるこのルールを保管しますか?",
					textDestination: "<em>宛先</em> に先行ブランクがあります。 先行ブランクのあるこのルールを保管しますか?",
					textBoth: "<em>ソース</em> と<em>宛先</em> に先行ブランクがあります。 先行ブランクのあるこのルールを保管しますか?",
					info: "トピックでの先行ブランクの使用は許可されていますが、通常は使用しません。  トピック・ストリングを調べて正しいことを確認してください。",
					buttonLabel: "ルールの保管"
				},
				grid: {
					name: "名前",
					type: "ルール・タイプ",
					source: "ソース",
					destination: "宛先",
					none: "なし",
					all: "すべて",
					enabled: "使用可能",
					associations: "関連付け",
					maxMessages: "最大メッセージ数",
					retainedMessages: "保存メッセージ"
				},
				state: {
					enabled: "使用可能",
					disabled: "使用不可",
					pending: "保留"
				},
				ruleTypes: {
					type1: "${IMA_PRODUCTNAME_SHORT} トピックから MQ キューへ",
					type2: "${IMA_PRODUCTNAME_SHORT} トピックから MQ トピックへ",
					type3: "MQ キューから ${IMA_PRODUCTNAME_SHORT} トピックへ",
					type4: "MQ トピックから ${IMA_PRODUCTNAME_SHORT} トピックへ",
					type5: "${IMA_PRODUCTNAME_SHORT} トピック・サブツリーから MQ キューへ",
					type6: "${IMA_PRODUCTNAME_SHORT} トピック・サブツリーから MQ トピックへ",
					type7: "${IMA_PRODUCTNAME_SHORT} トピック・サブツリーから MQ トピック・サブツリーへ",
					type8: "MQ トピック・サブツリーから ${IMA_PRODUCTNAME_SHORT} トピックへ",
					type9: "MQ トピック・サブツリーから ${IMA_PRODUCTNAME_SHORT} トピック・サブツリーへ",
					type10: "${IMA_PRODUCTNAME_SHORT} キューから MQ キューへ",
					type11: "${IMA_PRODUCTNAME_SHORT} キューから MQ トピックへ",
					type12: "MQ キューから ${IMA_PRODUCTNAME_SHORT} キューへ",
					type13: "MQ トピックから ${IMA_PRODUCTNAME_SHORT} キューへ",
					type14: "MQ トピック・サブツリーから ${IMA_PRODUCTNAME_SHORT} キューへ"
				},
				sourceTooltips: {
					type1: "マップ元の ${IMA_PRODUCTNAME_SHORT} トピックを入力してください",
					type2: "マップ元の ${IMA_PRODUCTNAME_SHORT} トピックを入力してください",
					type3: "マップ元の MQ キューを入力してください。 値には、a から z、A から Z、0 から 9 の文字と、次の文字を使用することができます。% . /  _",
					type4: "マップ元の MQ トピックを入力してください",
					type5: "マップ元の ${IMA_PRODUCTNAME_SHORT} トピック・サブツリーを入力してください (例えば MessageGatewayRoot/Level2)",
					type6: "マップ元の ${IMA_PRODUCTNAME_SHORT} トピック・サブツリーを入力してください (例えば MessageGatewayRoot/Level2)",
					type7: "マップ元の ${IMA_PRODUCTNAME_SHORT} トピック・サブツリーを入力してください (例えば MessageGatewayRoot/Level2)",
					type8: "マップ元の MQ トピック・サブツリーを入力してください (例えば MQRoot/Layer1)",
					type9: "マップ元の MQ トピック・サブツリーを入力してください (例えば MQRoot/Layer1)",
					type10: "マップ元の ${IMA_PRODUCTNAME_SHORT} キューを入力してください。 " +
							"値の先頭と末尾にはスペースがあってはなりません。また、値には、制御文字、コンマ、二重引用符、円記号、または等号は使用できません。 " +
							"先頭文字は、数字、引用符、または次のいずれかの特殊文字であってはなりません。! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type11: "マップ元の ${IMA_PRODUCTNAME_SHORT} キューを入力してください。 " +
							"値の先頭と末尾にはスペースがあってはなりません。また、値には、制御文字、コンマ、二重引用符、円記号、または等号は使用できません。 " +
							"先頭文字は、数字、引用符、または次のいずれかの特殊文字であってはなりません。! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type12: "マップ元の MQ キューを入力してください。 値には、a から z、A から Z、0 から 9 の文字と、次の文字を使用することができます。% . /  _",
					type13: "マップ元の MQ トピックを入力してください",
					type14: "マップ元の MQ トピック・サブツリーを入力してください (例えば MQRoot/Layer1)"
				},
				targetTooltips: {
					type1: "マップ先の MQ キューを入力してください。 値には、a から z、A から Z、0 から 9 の文字と、次の文字を使用することができます。% . /  _",
					type2: "マップ先の MQ トピックを入力してください",
					type3: "マップ先の ${IMA_PRODUCTNAME_SHORT} トピックを入力してください",
					type4: "マップ先の ${IMA_PRODUCTNAME_SHORT} トピックを入力してください",
					type5: "マップ先の MQ キューを入力してください。 値には、a から z、A から Z、0 から 9 の文字と、次の文字を使用することができます。% . /  _",
					type6: "マップ先の MQ トピックを入力してください",
					type7: "マップ先の MQ トピック・サブツリーを入力してください (例えば MQRoot/Layer1)",
					type8: "マップ先の ${IMA_PRODUCTNAME_SHORT} トピックを入力してください",
					type9: "マップ先の ${IMA_PRODUCTNAME_SHORT} トピック・サブツリーを入力してください (例えば MessageGatewayRoot/Level2)",
					type10: "マップ先の MQ キューを入力してください。 値には、a から z、A から Z、0 から 9 の文字と、次の文字を使用することができます。% . /  _",
					type11: "マップ先の MQ トピックを入力してください",
					type12: "マップ先の ${IMA_PRODUCTNAME_SHORT} キューを入力してください。 " +
							"値の先頭と末尾にはスペースがあってはなりません。また、値には、制御文字、コンマ、二重引用符、円記号、または等号は使用できません。 " +
							"先頭文字は、数字、引用符、または次のいずれかの特殊文字であってはなりません。! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type13: "マップ先の ${IMA_PRODUCTNAME_SHORT} キューを入力してください。 " +
							"値の先頭と末尾にはスペースがあってはなりません。また、値には、制御文字、コンマ、二重引用符、円記号、または等号は使用できません。 " +
							"先頭文字は、数字、引用符、または次のいずれかの特殊文字であってはなりません。! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type14: "マップ先の ${IMA_PRODUCTNAME_SHORT} キューを入力してください。 " +
							"値の先頭と末尾にはスペースがあってはなりません。また、値には、制御文字、コンマ、二重引用符、円記号、または等号は使用できません。 " +
							"先頭文字は、数字、引用符、または次のいずれかの特殊文字であってはなりません。! # $ % &amp; ( ) * + , - . / : ; < = > ? @"
				},
				associated: "関連付け済み",
				associatedQMs: "関連付けられたキュー・マネージャー接続:",
				associatedMessages: {
					errorMissing: "キュー・マネージャー接続を選択する必要があります。",
					errorRetained: "保存メッセージの設定では、キュー・マネージャー接続の関連付けを 1 つ行うことができます。 " +
							"保存メッセージを<em>なし</em> に変更するか、いくつかの関連付けを削除してください。"
				},
				ruleTypeMessage: "保存メッセージの設定では、宛先がトピックまたはトピック・サブツリーのルール・タイプが許可されます。 " +
							"保存メッセージを<em>なし</em> に変更するか、異なるルール・タイプを選択してください。",
				status: {
					active: "アクティブ",
					starting: "始動中",
					inactive: "非アクティブ"
				},
				dialog: {
					instruction: "宛先マッピング・ルールは、メッセージの移動方向と、ソース・オブジェクトおよびターゲット・オブジェクトの性質を定義します。",
					nameInvalid: "名前の先頭および末尾をスペースにすることはできません",
					noQmgrsTitle: "キュー・マネージャー接続がありません",
					noQmrsDetail: "まずキュー・マネージャー接続を定義しなければ、宛先マッピング・ルールを定義できません。",
					maxMessagesTooltip: "宛先のバッファーに入れることができるメッセージの最大数を指定します。",
					maxMessagesInvalid: "最大メッセージ数は、1 から 20,000,000 までの数値でなければなりません。",
					retainedMessages: {
						label: "保存メッセージ",
						none: "なし",
						all: "すべて",
						tooltip: {
							basic: "保存メッセージとしてトピックに転送するメッセージを指定します。",
							disabled4Type: "宛先がキューの場合、保存メッセージは<em>なし</em> でなければなりません。",
							disabled4Associations: "キュー・マネージャー接続が複数選択されている場合、保存メッセージは<em>なし</em> でなければなりません。",
							disabled4Both: "宛先がキューの場合、またはキュー・マネージャー接続が複数選択されている場合、保存メッセージは<em>なし </em> でなければなりません。"
						}
					}
				},
				addDialog: {
					title: "宛先マッピング・ルールの追加"
				},
				removeDialog: {
					title: "宛先マッピング・ルールの削除",
					content: "このレコードを削除してもよろしいですか?"
				},
                removeNotAllowedDialog: {
                	title: "削除は許可されていません",
                	content: "宛先マッピング・ルールは、使用可能になっているため、削除できません。  " +
                			 "「その他のアクション」メニューを使用してルールを使用不可にしてから、再試行してください。",
                	closeButton: "閉じる"
                },
				editDialog: {
					title: "宛先マッピング・ルールの編集",
					restrictedEditingTitle: "ルールが使用可能になっている間、編集は制限されます",
					restrictedEditingContent: "ルールが使用可能になっている間は、限られたプロパティーを編集できます。 " +
							"その他のプロパティーを編集するには、ルールを使用不可に設定し、プロパティーを編集してから変更を保管します。 " +
							"ルールを使用不可に設定するとルールが停止します。その結果、バッファーに入れられたメッセージと現在送信中のメッセージが失われることになります。 " +
							"ルールは再度使用可能にするまで使用不可のままになります。"
				},
				action: {
					Enable: "ルールの使用可能化",
					Disable: "ルールの使用不可化",
					Refresh: "状況を最新表示"
				}
			},
			sslkeyrepository: {
				title: "SSL 鍵リポジトリー",
				tagline: "システム管理者とメッセージング管理者は、SSL 鍵リポジトリーとパスワード・ファイルのアップロードとダウンロードを行えます。 " +
						 "SSL 鍵リポジトリーとパスワード・ファイルは、サーバーとキュー・マネージャーの間の接続を保護するために使用されます。",
                uploadButton: "アップロード",
                downloadLink: "ダウンロード",
                deleteButton: "削除",
                lastUpdatedLabel: "SSL 鍵リポジトリー・ファイルの最終更新日時:",
                noFileUpdated: "なし",
                uploadFailed: "アップロードに失敗しました。",
                deletingFailed: "削除に失敗しました。",                
                dialog: {
                	uploadTitle: "SSL 鍵リポジトリー・ファイルのアップロード",
                	deleteTitle: "SSL 鍵リポジトリー・ファイルの削除",
                	deleteContent: "MQConnectivity サービスが実行中の場合は、再始動されます。 SSL 鍵リポジトリー・ファイルを削除してもよろしいですか?",
                	keyFileLabel: "鍵データベース・ファイル:",
                	passwordFileLabel: "パスワード・スタッシュ・ファイル:",
                	browseButton: "参照...",
					browseHint: "ファイルの選択...",
					savingProgress: "保管中...",
					deletingProgress: "削除中...",
                	saveButton: "保管",
                	deleteButton: "OK",
                	cancelButton: "キャンセル",
                	keyRepositoryError:  "SSL 鍵リポジトリー・ファイルは .kdb ファイルでなければなりません。",
                	passwordStashError:  "パスワード・スタッシュ・ファイルは .sth ファイルでなければなりません。",
                	keyRepositoryMissingError: "SSL 鍵リポジトリー・ファイルが必要です。",
                	passwordStashMissingError: "パスワード・スタッシュ・ファイルが必要です。"
                }
			},
			externldap: {
				subTitle: "LDAP 構成",
				tagline: "LDAP が使用可能な場合、それがサーバー・ユーザーおよびグループに使用されます。",
				itemName: "LDAP 接続",
				grid: {
					ldapname: "LDAP 名",
					url: "URL",
					userid: "ユーザー ID",
					password: "パスワード"
				},
				enableButton: {
					enable: "LDAP の使用可能化",
					disable: "LDAP の使用不可化"
				},
				dialog: {
					ldapname: "LDAP オブジェクト名",
					url: "URL",
					certificate: "証明書",
					checkcert: "サーバー証明書の確認",
					checkcertTStoreOpt: "メッセージ・サーバーのトラストストアを使用",
					checkcertDisableOpt: "証明書の検証を無効化",
					checkcertPublicTOpt: "パブリック・トラストストアを使用",
					timeout: "タイムアウト",
					enableCache: "キャッシュの使用可能化",
					cachetimeout: "キャッシュ・タイムアウト",
					groupcachetimeout: "グループ・キャッシュ・タイムアウト",
					ignorecase: "大/小文字の無視",
					basedn: "BaseDN",
					binddn: "BindDN",
					bindpw: "バインド・パスワード",
					userSuffix: "ユーザー・サフィックス",
					groupSuffix: "グループ・サフィックス",
					useridmap: "ユーザー ID マップ",
					groupidmap: "グループ ID マップ",
					groupmemberidmap: "グループ・メンバー ID マップ",
					nestedGroupSearch: "ネストされたグループを含める",
					tooltip: {
						url: "LDAP サーバーを指す URL。 形式は次のとおりです。<br/> &lt;protocol&gt;://&lt;server&nbsp;ip&gt;:&lt;port&gt;.",
						checkcert: "LDAP サーバー証明書を確認する方法を指定します。",
						basedn: "基本識別名。",
						binddn: "LDAP へのバインド時に使用する識別名。 匿名バインドの場合はブランクのままにします。",
						bindpw: "LDAP へのバインド時に使用するパスワード。 匿名バインドの場合はブランクのままにします。",
						userSuffix: "検索するユーザー識別名の接尾部。 " +
									"指定しない場合は、ユーザー ID マップを使用してユーザー識別名を検索した後、その識別名でバインドします。",
						groupSuffix: "グループ識別名の接尾部。",
						useridmap: "ユーザー ID のマップ先のプロパティー。",
						groupidmap: "グループ ID のマップ先のプロパティー。",
						groupmemberidmap: "グループ・メンバー ID のマップ先のプロパティー。",
						timeout: "LDAP 呼び出しのタイムアウト (秒単位)。",
						enableCache: "資格情報をキャッシュに入れるかどうかを指定します。",
						cachetimeout: "キャッシュのタイムアウト (秒単位)。",
						groupcachetimeout: "グループ・キャッシュのタイムアウト (秒単位)。",
						nestedGroupSearch: "チェック・マークを付けた場合は、ユーザーのグループ・メンバーシップの検索で、ネストされたグループがすべて含まれます。",
						testButtonHelp: "LDAP サーバーへの接続をテストします。 LDAP サーバーの URL を指定する必要があります。必要に応じて、証明書、BindDN、および BindPassword の値を指定できます。"
					},
					secondUnit: "秒",
					browseHint: "ファイルの選択...",
					browse: "参照...",
					clear: "クリア",
					connSubHeading: "LDAP 接続設定",
					invalidTimeout: "タイムアウトは、1 から 60 までの数値でなければなりません。",
					invalidCacheTimeout: "タイムアウトは、1 から 60 までの数値でなければなりません。",
					invalidGroupCacheTimeout: "タイムアウトは、1 から 86,400 までの数値でなければなりません。",
					certificateRequired: "LDAPS URL を指定し、トラストストアを使用する場合は、証明書が必要です",
					urlThemeError: "プロトコル LDAP または LDAPS を使用する有効な URL を入力してください。"
				},
				addDialog: {
					title: "LDAP 接続の追加",
					instruction: "LDAP 接続を構成します。"
				},
				editDialog: {
					title: "LDAP 接続の編集"
				},
				removeDialog: {
					title: "LDAP 接続の削除"
				},
				warnOnlyOneLDAPDialog: {
					title: "LDAP 接続は既に定義されています",
					content: "指定できる LDAP 接続は 1 つだけです",
					closeButton: "閉じる"
				},
				restartInfoDialog: {
					title: "LDAP 設定が変更されました",
					content: "次にクライアントまたは接続が認証または許可された時点で、新しい LDAP 設定値が使用されます。",
					closeButton: "閉じる"
				},
				enableError: "外部 LDAP 構成の使用可能化/使用不可化中にエラーが発生しました。",				
				retrieveError: "LDAP 構成の取得中にエラーが発生しました。",
				saveSuccess: "正常に保管されました。 変更を有効にするには、${IMA_PRODUCTNAME_SHORT} サーバーを再始動する必要があります。"
			},
			messagequeues: {
				title: "メッセージ・キュー",
				subtitle: "Point-to-Point メッセージングでは、メッセージ・キューが使用されます。"				
			},
			queues: {
				title: "キュー",
				tagline: "メッセージ・キューを定義、編集、または削除します。",
				retrieveError: "メッセージ・キューのリストを取得中にエラーが発生しました。",
				grid: {
					name: "名前",
					allowSend: "送信の許可",
					maxMessages: "最大メッセージ数",
					concurrentConsumers: "同時コンシューマー",
					description: "説明"
				},
				dialog: {	
					instruction: "メッセージング・アプリケーションで使用するキューを定義します。",
					nameInvalid: "名前の先頭および末尾をスペースにすることはできません",
					maxMessagesTooltip: "キューに保管できるメッセージの最大数を指定します。",
					maxMessagesInvalid: "最大メッセージ数は、1 から 20,000,000 までの数値でなければなりません。",
					allowSendTooltip: "アプリケーションがこのキューにメッセージを送信できるかどうかを指定します。 アプリケーションがこのキューからメッセージを受信できるかどうかは、このプロパティーに左右されません。",
					concurrentConsumersTooltip: "複数の同時コンシューマーをキューが許可するかどうかを指定します。 チェック・ボックスがクリアされている場合、キューで許可されるコンシューマーは 1 つだけです。",
					performanceLabel: "パフォーマンス・プロパティー"
				},
				addDialog: {
					title: "キューの追加"
				},
				removeDialog: {
					title: "キューの削除",
					content: "このレコードを削除してもよろしいですか?"
				},
				editDialog: {
					title: "キューの編集"
				}	
			},
			messagingTester: {
				title: "Messaging Tester サンプル・アプリケーション",
				tagline: "Messaging Tester は単純な HTML5 サンプル・アプリケーションであり、MQTT over WebSockets を使用して、${IMA_PRODUCTNAME_SHORT} サーバーとやり取りするいくつかのクライアントをシミュレートします。 " +
						 "これらのクライアントは、サーバーに接続すると、3 つのトピックにパブリッシュ/サブスクライブできます。",
				enableSection: {
					title: "1. エンドポイント DemoMqttEndpoint を使用可能にします",
					tagline: "Messaging Tester サンプル・アプリケーションは、無保護の ${IMA_PRODUCTNAME_SHORT} エンドポイントに接続する必要があります。  DemoMqttEndpoint を使用できます。"					
				},
				downloadSection: {
					title: "2. Messaging Tester サンプル・アプリケーションをダウンロードして実行します",
					tagline: "リンクをクリックして MessagingTester.zip をダウンロードします。  ファイルを解凍し、WebSockets をサポートするブラウザーで index.html を開きます。 " +
							 "Web ページ上の説明に従って、${IMA_PRODUCTNAME_SHORT} が MQTT メッセージングを行える状態であることを確認します。",
					linkLabel: "MessagingTester.zip のダウンロード"
				},
				disableSection: {
					title: "3. エンドポイント DemoMqttEndpoint を使用不可にします",
					tagline: "${IMA_PRODUCTNAME_SHORT} での MQTT メッセージングの検証を終了したら、${IMA_PRODUCTNAME_SHORT} への無許可アクセスを防ぐために、エンドポイント DemoMqttEndpoint を使用不可にします。"					
				},
				endpoint: {
					// TRNOTE {0} is replaced with the name of the endpoint
					label: "{0} の状況",
					state: {
						enabled: "使用可能",					
						disabled: "使用不可",
						missing: "欠落",
						down: "下へ"
					},
					linkLabel: {
						enableLinkLabel: "使用可能にする",
						disableLinkLabel: "使用不可にする"						
					},
					missingMessage: "使用できる別の無保護エンドポイントがない場合は、無保護エンドポイントを作成してください。",
					retrieveEndpointError: "エンドポイント構成の取得中にエラーが発生しました。",					
					retrieveEndpointStatusError: "エンドポイント状況の取得中にエラーが発生しました。",
					saveEndpointError: "エンドポイント状態の設定中にエラーが発生しました。"
				}
			}
		},
		protocolsLabel: "プロトコル",

		// Messaging policy types
		topicPoliciesTitle: "トピック・ポリシー",
		subscriptionPoliciesTitle: "サブスクリプション・ポリシー",
		queuePoliciesTitle: "キュー・ポリシー",
		// Messaging policy dialog strings
		addTopicMPTitle: "トピック・ポリシーの追加",
	    editTopicMPTitle: "トピック・ポリシーの編集",
	    viewTopicMPTitle: "トピック・ポリシーの表示",
		removeTopicMPTitle: "トピック・ポリシーの削除",
		removeTopicMPContent: "このトピック・ポリシーを削除してもよろしいですか?",
		topicMPInstruction: "トピック・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのトピックにアクセスできるかを決定します。 " +
					     "各エンドポイントには、少なくとも 1 つのトピック・ポリシー、サブスクリプション・ポリシー、またはキュー・ポリシーが必要です。",
		addSubscriptionMPTitle: "サブスクリプション・ポリシーの追加",
		editSubscriptionMPTitle: "サブスクリプション・ポリシーの編集",
		viewSubscriptionMPTitle: "サブスクリプション・ポリシーの表示",
		removeSubscriptionMPTitle: "サブスクリプション・ポリシーの削除",
		removeSubscriptionMPContent: "このサブスクリプション・ポリシーを削除してもよろしいですか?",
		subscriptionMPInstruction: "サブスクリプション・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのグローバル共有サブスクリプションにアクセスできるかを決定します。 " +
					     "グローバル共有サブスクリプションでは、永続トピック・サブスクリプションからメッセージを受け取る処理が、複数のサブスクライバー間で共有されます。 各エンドポイントには、少なくとも 1 つのトピック・ポリシー、サブスクリプション・ポリシー、またはキュー・ポリシーが必要です。",
		addQueueMPTitle: "キュー・ポリシーの追加",
		editQueueMPTitle: "キュー・ポリシーの編集",
		viewQueueMPTitle: "キュー・ポリシーの表示",
		removeQueueMPTitle: "キュー・ポリシーの削除",
		removeQueueMPContent: "このキュー・ポリシーを削除してもよろしいですか?",
		queueMPInstruction: "キュー・ポリシーは、特定のメッセージング・アクションを実行する権限を、接続済みクライアントに付与します。例えば、${IMA_PRODUCTNAME_FULL} 上でクライアントがどのキューにアクセスできるかを決定します。 " +
					     "各エンドポイントには、少なくとも 1 つのトピック・ポリシー、サブスクリプション・ポリシー、またはキュー・ポリシーが必要です。",
		// Messaging and Endpoint dialog strings
		policyTypeTitle: "ポリシー・タイプ",
		policyTypeName_topic: "トピック",
		policyTypeName_subscription: "グローバル共有サブスクリプション",
		policyTypeName_queue: "キュー",
		// Messaging policy type names that are embedded in other strings
		policyTypeShortName_topic: "トピック",
		policyTypeShortName_subscription: "サブスクリプション",
		policyTypeShortName_queue: "キュー",
		policyTypeTooltip_topic: "このポリシーが適用されるトピック。 アスタリスク (*) は慎重に使用してください。 " +
		    "アスタリスクは 0 個以上の文字 (スラッシュを含む) とマッチングします。 そのため、トピック・ツリー内の複数のレベルにマッチングする可能性があります。",
		policyTypeTooltip_subscription: "このポリシーが適用されるグローバル共有永続サブスクリプション。 アスタリスク (*) は慎重に使用してください。 " +
			"アスタリスクは、0 個以上の文字と一致します。",
	    policyTypeTooltip_queue: "このポリシーが適用されるキュー。 アスタリスク (*) は慎重に使用してください。 " +
			"アスタリスクは、0 個以上の文字と一致します。",
	    topicPoliciesTagline: "トピック・ポリシーを使用することにより、${IMA_PRODUCTNAME_FULL} 上でクライアントがアクセスできるトピックを制御できます。",
		subscriptionPoliciesTagline: "サブスクリプション・ポリシーを使用することにより、${IMA_PRODUCTNAME_FULL} 上でクライアントがアクセスできるグローバル共有永続サブスクリプションを制御できます。",
	    queuePoliciesTagline: "キュー・ポリシーを使用することにより、${IMA_PRODUCTNAME_FULL} 上でクライアントがアクセスできるキューを制御できます。",
	    // Additional MQConnectivity queue manager connection dialog properties
	    channelUser: "チャネル・ユーザー",
	    channelPassword: "チャネル・ユーザー・パスワード",
	    channelUserTooltip: "キュー・マネージャーがチャネル・ユーザーを認証するように構成されている場合には、この値の設定が必要です。",
	    channelPasswordTooltip: "チャネル・ユーザーを指定した場合は、この値も設定してください。",
	    // Additional LDAP dialog properties
	    emptyLdapURL: "LDAP URL が設定されていません。",
		externalLDAPOnlyTagline: "${IMA_PRODUCTNAME_SHORT} サーバーのユーザーおよびグループに使用される LDAP リポジトリーの接続プロパティーを構成します。", 
		// TRNNOTE: Do not translate bindPasswordHint.  This is a place holder string.
		bindPasswordHint: "********",
		clearBindPasswordLabel: "クリア",
		resetLdapButton: "リセット",
		resetLdapTitle: "LDAP 構成のリセット",
		resetLdapContent: "すべての LDAP 構成設定がデフォルト値にリセットされます。",
		resettingProgress: "リセットしています...",
		// New SSL Key repository dialog strings
		sslkeyrepositoryDialogUploadContent: "MQConnectivity サービスが実行中の場合は、リポジトリー・ファイルのアップロード後に再始動されます。",
		savingRestartingProgress: "保管して再始動しています...",
		deletingRestartingProgress: "削除して再始動しています..."
});
