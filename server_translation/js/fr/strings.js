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
		// Global Definitions
		// ------------------------------------------------------------------------
		global : {
			productName : "${IMA_PRODUCTNAME_FULL}", // Do_Not_Translate
			productNameTM: "${IMA_PRODUCTNAME_FULL_HTML}",	// Do_Not_Translate 		
			webUIMainTitle: "Interface utilisateur Web d'${IMA_PRODUCTNAME_FULL}",
			node: "Noeud",
			// TRNOTE {0} is replaced with a user ID
			greeting: "{0}",    
			license: "Contrat de licence",
			menuContent: "Contenu de la sélection de menu",
			home : "Accueil",
			messaging : "Messagerie",
			monitoring : "Contrôle",
			appliance : "Serveur",
			login: "Connexion",
			logout: "Déconnexion",
			changePassword: "Modifier le mot de passe",
			yes: "Oui",
			no: "Non",
			all: "Tous",
			trueValue: "Vrai",
			falseValue: "Faux",
			days: "jours",
			hours: "heures",
			minutes: "minutes",
			// dashboard uptime, which shows a very short representation of elapsed time
			// TRNOTE e.g. '2d 1h' for 2 days and 1 hour
			daysHoursShort: "{0}j {1}h",  
			// TRNOTE e.g. '2h 30m' for 2 hours and 30 minutes
			hoursMinutesShort: "{0}h {1}m",
			// TRNOTE e.g. '1m 30s' for 1 minute and 30 seconds
			minutesSecondsShort: "{0}m {1}s",
			// TRNOTE  e.g. '30s' for 30 seconds
			secondsShort: "{0}s", 
			notAvailable: "NA",
			missingRequiredMessage: "Une valeur est requise.",
			pageNotAvailable: "Cette page n'est pas disponible en raison du statut du serveur ${IMA_PRODUCTNAME_SHORT}.",
			pageNotAvailableServerDetail: "Pour pouvoir utiliser cette page, le serveur ${IMA_PRODUCTNAME_SHORT} doit être en cours d'exécution en mode production.",
			pageNotAvailableHAroleDetail: "Pour pouvoir utiliser cette page, le serveur ${IMA_PRODUCTNAME_SHORT} doit être le serveur principal et ne pas être en cours de synchronisation, ou la haute disponibilité doit être désactivée. "			
		},
		name: {
			label: "Nom",
			tooltip: "Le nom ne doit pas commencer ou finir par des espaces et ne peut pas contenir de caractère de contrôle, de virgule, de guillemets, de barre oblique  inversée ou de signe égal. " +
					 "Le premier caractère ne doit pas être un nombre, une apostrophe ni l'un des caractères spéciaux suivants : ! # $ % &amp; ( ) * + , - . / : ; &lt; = &gt; ? @",
			invalidSpaces: "Le nom ne peut pas commencer ou finir par des espaces.",
			noSpaces: "Le nom ne peut pas contenir d'espace.",
			invalidFirstChar: "Le premier caractère ne peut pas être un nombre, un caractère de contrôle ou l'un des caractères spéciaux suivants : ! &quot; # $ % &amp; ' ( ) * + , - . / : ; &lt; = &gt; ? @",
			// disallow ",=\
			invalidChar: "Le nom ne peut pas contenir de caractère de contrôle ou l'un des caractères spéciaux suivants : &quot; , = \\ ",
			duplicateName: "Un enregistrement du même nom existe déjà.",
			unicodeAlphanumericOnly: {
				invalidChar: "Le nom ne doit être composé que de caractères alphanumériques.",
				invalidFirstChar: "Le premier caractère ne doit pas être un nombre."
			}
		},
		// ------------------------------------------------------------------------
		// Navigation (menu) items
		// ------------------------------------------------------------------------
		globalMenuBar: {
			messaging : {
				messageProtocols: "Protocoles de messagerie",
				userAdministration: "Authentification d'utilisateur",
				messageHubs: "Concentrateurs de messages",
				messageHubDetails: "Détails du concentrateur de messages",
				mqConnectivity: "MQ Connectivity",
				messagequeues: "Files d'attente de messages",
				messagingTester: "Exemple d'application"
			},
			monitoring: {
				connectionStatistics: "Connexions",
				endpointStatistics: "Noeuds finaux",
				queueMonitor: "Files d'attente",
				topicMonitor: "Rubriques",
				mqttClientMonitor: "Clients MQTT déconnectés",
				subscriptionMonitor: "Abonnements",
				transactionMonitor: "Transactions",
				destinationMappingRuleMonitor: "MQ Connectivity",
				applianceMonitor: "Serveur",
				downloadLogs: "Journaux de téléchargement",
				snmpSettings: "Paramètres SNMP"
			},
			appliance: {
				users: "Utilisateurs de l'interface utilisateur Web",
				networkSettings: "Paramètres réseau",
			    locale: "Environnement local, date et heure",
			    securitySettings: "Paramètres de sécurité",
			    systemControl: "Contrôle du serveur",
			    highAvailability: "Haute disponibilité",
			    webuiSecuritySettings: "Paramètres de l'interface utilisateur Web"
			}
		},

		// ------------------------------------------------------------------------
		// Help
		// ------------------------------------------------------------------------
		helpMenu : {
			help : "Aide",
			homeTasks : "Restaurer des tâches sur la page d'accueil",
			about : {
				linkTitle : "A propos de",
				dialogTitle: "A propos d'${IMA_PRODUCTNAME_FULL}",
				// TRNOTE: {0} is the license type which is Developers, Non-Production or Production
				viewLicense: "Afficher le contrat de licence {0}",
				iconTitle: "${IMA_PRODUCTNAME_FULL_HTML} version ${ISM_VERSION_ID}"
			}
		},

		// ------------------------------------------------------------------------
		// Change Password dialog
		// ------------------------------------------------------------------------
		changePassword: {
			dialog: {
				title: "Modifier le mot de passe",
				currpasswd: "Mot de passe actuel :",
				newpasswd: "Nouveau mot de passe :",
				password2: "Confirmation du mot de passe :",
				password2Invalid: "Les mots de passe ne correspondent pas.",
				savingProgress: "Enregistrement en cours...",
				savingFailed: "L'enregistrement a échoué."
			}
		},

		// ------------------------------------------------------------------------
		// Message Level Descriptions
		// ------------------------------------------------------------------------
		level : {
			Information : "Informations",
			Warning : "Avertissement",
			Error : "Erreur",
			Confirmation : "Confirmation",
			Success : "Réussite"
		},

		action : {
			Ok : "OK",
			Close : "Fermer",
			Cancel : "Annuler",
			Save: "Enregistrer",
			Create: "Créer",
			Add: "Ajouter",
			Edit: "Modifier",
			Delete: "Supprimer",
			MoveUp: "Déplacer vers le haut",
			MoveDown: "Déplacer vers le bas",
			ResetPassword: "Redéfinition de mot de passe",
			Actions: "Actions",
			OtherActions: "Autres actions",
			View: "Afficher",
			ResetColWidth: "Réinitialiser les largeurs de colonne",
			ChooseColumns: "Choisir des colonnes visibles",
			ResetColumns: "Réinitialiser les colonnes visibles"
		},
		// new pages and tabs need to go here
        cluster: "Cluster",
        clusterMembership: "Rejoindre/Quitter",
        adminEndpoint: "Noeud final d'administration",
        firstserver: "Se connecter à un serveur",
        portInvalid: "Le numéro de port doit être un nombre compris entre 1 et 65535.",
        connectionFailure: "Connexion impossible au serveur ${IMA_PRODUCTNAME_FULL}.",
        clusterStatus: "Statut",
        webui: "Interface utilisateur Web",
        licenseType_Devlopers: "Développeurs",
        licenseType_NonProd: "Hors production",
        licenseType_Prod: "Production",
        licenseType_Beta: "Bêta"
});
