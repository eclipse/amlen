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

		license : {
			heading : "ご使用条件",
			languageSelection: "ご使用条件の言語の選択",
			intro : "下の <q>同意します</q> ボタンをクリックすると、使用条件、IBM 以外の使用条件、および保証情報に同意したことになります。 " +
					"同意できない場合は、 <q>同意しません</q> ボタンをクリックしてください。",
			introAccepted: "使用条件、IBM 以外の使用条件、および保証情報が受諾されました。",
			nonibm : {
				link : "ソフトウェアの IBM 以外の条件を読む",
				title : "ソフトウェアの IBM 以外の条件"
			},
			notices : {
				link : "ソフトウェアの特記事項を読む",
				title : "ソフトウェア特記事項"
			},
			laMachineCode : {
				link : "IBM 機械コードのご使用条件を読む"
			},
			hardwareNonibm : {
				link : "ハードウェアの IBM 以外の条件を読む"
			},
			hardwareNotices : {
				link : "ハードウェアの特記事項を読む"
			},
			warranty : {
				info : "保証情報については、製品パッケージに同梱されている<em>「保証情報」</em>という資料をご覧ください。"
			},
			accept : "同意します",
			decline : "同意しません",
			print : "使用条件を印刷する",
			error : "エラー: 要求された言語で使用条件を読み込めませんでした。",
			acceptDialog : {
				title : "使用が許諾されました",
				content : "使用が許諾されました。  ${IMA_PRODUCTNAME_FULL} サーバーを開始中です。"
			},
			declineDialog : {
				title : "使用が拒否されました",
				content : "使用条件、IBM 以外の条件、および保証情報にご同意いただけない場合は、製品を取得時点の状態に戻し、代金の払い戻しを受けてください (該当する場合)。"
			},
			loading: "読み込み中...",
			lang : {
				en : "英語 (en)",
				zh : "中国語 (簡体字) (zh)",
				zh_TW : "中国語 (繁体字) (zh_TW)",
				cs : "チェコ語 (cs)",
				fr : "フランス語 (fr)",
				de : "ドイツ語 (de)",
				el : "ギリシャ語 (el)",
				in_ : "インドネシア語 (in)", // "in" is reserved
				it : "イタリア語 (it)",
				ja : "日本語	(ja)",
				ko : "韓国語 (ko)",
				lt : "リトアニア語 (lt)",
				pl : "ポーランド語 (pl)",
				pt : "ポルトガル語 (pt)",
				ru : "ロシア語 (ru)",
				sl : "スロベニア語 (sl)",
				es : "スペイン語 (es)",
				tr : "トルコ語 (tr)"
			}
		},
		licenseNotAcceptedTitle: "${IMA_PRODUCTNAME_FULL} ご使用条件が受諾されていません。",
		licenseNotAcceptedContent: "${IMA_PRODUCTNAME_FULL} はシステム管理者がご使用条件を受諾しないと完全に機能しません。"

});
