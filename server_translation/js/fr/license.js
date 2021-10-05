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
			heading : "Contrat de licence",
			languageSelection: "Sélection de la licence de langue",
			intro : "En cliquant sur le bouton <q>J'accepte</q> ci-dessous, vous acceptez les termes des licences, les termes non IBM et les informations de garantie. " +
					"Si vous ne les acceptez pas, cliquez sur le bouton <q>Je n'accepte pas</q> .",
			introAccepted: "Les termes des licences, les termes non IBM et les informations de garantie ont été acceptés.",
			nonibm : {
				link : "Lire les termes des logiciels non IBM",
				title : "Termes des logiciels non IBM"
			},
			notices : {
				link : "Lire les notices des logiciels",
				title : "Notices des logiciels"
			},
			laMachineCode : {
				link : "Lire le contrat de licence IBM pour Machine Code"
			},
			hardwareNonibm : {
				link : "Lire les termes des matériels non IBM"
			},
			hardwareNotices : {
				link : "Lire les notices des matériels"
			},
			warranty : {
				info : "Pour les informations de garantie, voir les <em>informations sur la garantie</em> disponibles avec votre produit."
			},
			accept : "J'accepte",
			decline : "Je n'accepte pas",
			print : "Imprimer la licence",
			error : "Erreur : La licence n'a pas pu être chargée dans la langue demandée.",
			acceptDialog : {
				title : "Licence acceptée",
				content : "La licence a été acceptée.  Le serveur ${IMA_PRODUCTNAME_FULL} démarre."
			},
			declineDialog : {
				title : "Licence rejetée",
				content : "Si vous n'acceptez pas le contrat de licence, les termes non IBM et les informations de garantie, vous devez renvoyer le produit au point de vente où vous l'avez acheté et demandez un remboursement, si cela est possible."
			},
			loading: "Chargement...",
			lang : {
				en : "Anglais (en)",
				zh : "Chinois simplifié (zh)",
				zh_TW : "Chinois traditionnel (zh_TW)",
				cs : "Tchèque (cs)",
				fr : "Français (fr)",
				de : "Allemand (de)",
				el : "Grec (el)",
				in_ : "Indonésien (in)", // "in" is reserved
				it : "Italien (it)",
				ja : "Japonais (ja)",
				ko : "Coréen (ko)",
				lt : "Lituanien (lt)",
				pl : "Polonais (pl)",
				pt : "Portugais (pt)",
				ru : "Russe (ru)",
				sl : "Slovène (sl)",
				es : "Espagnol (es)",
				tr : "Turc (tr)"
			}
		},
		licenseNotAcceptedTitle: "Le contrat de licence d'${IMA_PRODUCTNAME_FULL} n'a pas été accepté.",
		licenseNotAcceptedContent: "${IMA_PRODUCTNAME_FULL} ne sera pas entièrement fonctionnel tant qu'un administrateur système n'aura pas accepté le contrat de licence."

});
