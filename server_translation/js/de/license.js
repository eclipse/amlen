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
			heading : "Lizenzvereinbarung",
			languageSelection: "Auswahl der Lizenzsprache",
			intro : "Indem Sie auf die folgende Schaltfläche <q>Ich stimme zu</q> klicken, stimmen Sie den Bedingungen der Lizenzen, den Bedingungen anderer Anbieter und den Gewährleistungsinformationen zu." +
					"Wenn Sie den Bedingungen nicht zustimmen, klicken Sie auf die Schaltfläche <q>Ich stimme nicht zu</q>.",
			introAccepted: "Die Bedingungen der Lizenzen, die Bedingungen anderer Anbieter und die Gewährleistungsinformationen wurden akzeptiert.",
			nonibm : {
				link : "Bedingungen anderer Anbieter für Software lesen",
				title : "Bedingungen anderer Anbieter für Software"
			},
			notices : {
				link : "Bemerkungen zur Software lesen",
				title : "Bemerkungen zur Software"
			},
			laMachineCode : {
				link : "IBM Lizenzvereinbarung für Maschinencode lesen"
			},
			hardwareNonibm : {
				link : "Bedingungen anderer Anbieter für Hardware lesen"
			},
			hardwareNotices : {
				link : "Bemerkungen zur Hardware lesen"
			},
			warranty : {
				info : "Die Gewährleistungsinformationen finden Sie in der Veröffentlichung <em>Warranty Information</em> in Ihrem Produktpaket."
			},
			accept : "Ich stimme zu",
			decline : "Ich stimme nicht zu",
			print : "Lizenz drucken",
			error : "Fehler: Die Lizenz konnte nicht in der angeforderten Sprache geladen werden.",
			acceptDialog : {
				title : "Lizenz akzeptiert",
				content : "Die Lizenz wurde akzeptiert. Der ${IMA_PRODUCTNAME_FULL}-Server wird gestartet."
			},
			declineDialog : {
				title : "Lizenz abgelehnt",
				content : "Wenn Sie die Lizenzvereinbarung, die Bedingungen anderer Anbieter und die Gewährleistungsinformationen nicht akzeptieren, sollten Sie das Produkt zurückgeben und ggf. eine Rückerstattung fordern."
			},
			loading: "Ladevorgang läuft...",
			lang : {
				en : "Englisch (en)",
				zh : "Vereinfachtes Chinesisch (zh)",
				zh_TW : "Traditionelles Chinesisch (zh_TW)",
				cs : "Tschechisch (cs)",
				fr : "Französisch (fr)",
				de : "Deutsch (de)",
				el : "Griechisch (el)",
				in_ : "Indonesisch (in)", // "in" is reserved
				it : "Italienisch (it)",
				ja : "Japanisch (ja)",
				ko : "Koreanisch (ko)",
				lt : "Litauisch (lt)",
				pl : "Polnisch (pl)",
				pt : "Portugiesisch (pt)",
				ru : "Russisch (ru)",
				sl : "Slowenisch (sl)",
				es : "Spanisch (es)",
				tr : "Türkisch (tr)"
			}
		},
		licenseNotAcceptedTitle: "Die ${IMA_PRODUCTNAME_FULL}-Lizenzvereinbarung wurde nicht akzeptiert.",
		licenseNotAcceptedContent: "${IMA_PRODUCTNAME_FULL} ist erst vollständig funktionsfähig, nachdem ein Systemadministrator die Lizenzvereinbarung akzeptiert hat."

});
