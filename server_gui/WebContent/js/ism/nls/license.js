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
	root : ({

		license : {
			heading : "License Agreement",
			languageSelection: "License Language Selection",
			intro : "By clicking on the <q>I Agree</q> button below, you agree to the terms of the licenses, non-IBM terms, and warranty information. " +
					"If you do not agree, click the <q>I do not Agree</q> button.",
			introAccepted: "The terms of the licenses, non-IBM terms, and warranty information has been accepted.",
			nonibm : {
				link : "Read Software Non-IBM terms",
				title : "Software Non-IBM terms"
			},
			notices : {
				link : "Read Software Notices",
				title : "Software Notices"
			},
			laMachineCode : {
				link : "Read IBM License Agreement for Machine Code"
			},
			hardwareNonibm : {
				link : "Read Hardware Non-IBM terms"
			},
			hardwareNotices : {
				link : "Read Hardware Notices"
			},
			warranty : {
				info : "For warranty information, see the <em>Warranty Information</em> publication available in your product packaging."
			},
			
			accept : "I Agree",
			decline : "I do not Agree",
			print : "Print license",
			error : "Error: The license could not be loaded in the requested language.",
			acceptDialog : {
				title : "License Accepted",
				content : "The license has been accepted.  The ${IMA_SVR_COMPONENT_NAME_FULL} is starting."
			},
			declineDialog : {
				title : "License Declined",
				content : "If you do not accept the license agreement, non-IBM terms, and warranty information, you should return the product to the point of acquisition and obtain a refund, if applicable."
			},
			loading: "Loading...",
			lang : {
				en : "English (en)",
				zh : "Chinese Simplified (zh)",
				zh_TW : "Chinese Traditional (zh_TW)",
				cs : "Czech (cs)",
				fr : "French (fr)",
				de : "German (de)",
				el : "Greek (el)",
				in_ : "Indonesian (in)", // "in" is reserved
				it : "Italian (it)",
				ja : "Japanese (ja)",
				ko : "Korean (ko)",
				lt : "Lithuanian (lt)",
				pl : "Polish (pl)",
				pt : "Portuguese (pt)",
				ru : "Russian (ru)",
				sl : "Slovenian (sl)",
				es : "Spanish (es)",
				tr : "Turkish (tr)"
			}
		},
		licenseNotAcceptedTitle: "The ${IMA_PRODUCTNAME_FULL} license agreement has not been accepted.",
		licenseNotAcceptedContent: "${IMA_PRODUCTNAME_FULL} is not fully functional until a system administrator accepts the license agreement."

	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
