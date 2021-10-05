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
				title: "Gestion des utilisateurs de l'interface utilisateur Web",
				tagline: "Attribuez aux utilisateurs l'accès à l'interface utilisateur Web.",
				usersSubTitle: "Utilisateurs de l'interface utilisateur Web",
				webUiUsersTagline: "Les administrateurs système peuvent définir, éditer ou supprimer des utilisateurs d'interface utilisateur Web.",
				itemName: "Utilisateur",
				itemTooltip: "",
				addDialogTitle: "Ajout d'utilisateur",
				editDialogTitle: "Edition d'utilisateur",
				removeDialogTitle: "Suppression d'utilisateur",
				resetPasswordDialogTitle: "Redéfinition de mot de passe",
				// TRNOTE: Do not translate addGroupDialogTitle, editGroupDialogTitle, removeGroupDialogTitle
				//addGroupDialogTitle: "Add Group",
				//editGroupDialogTitle: "Edit Group",
				//removeGroupDialogTitle: "Delete Group",
				grid: {
					userid: "ID utilisateur",
					// TRNOTE: Do not translate groupid
					//groupid: "Group ID",
					description: "Description",
					groups: "Rôle",
					noEntryMessage: "Aucun élément à afficher",
					// TRNOTE: Do not translate ldapEnabledMessage
					//ldapEnabledMessage: "Messaging users and groups configured on the appliance are disabled because an external LDAP connection is enabled.",
					refreshingLabel: "Mise à jour en cours..."
				},
				dialog: {
					useridTooltip: "Entrez l'ID utilisateur, qui ne doit pas commencer ou finir par un espace.",
					applianceUserIdTooltip: "Entrez l'ID utilisateur, qui ne doit comporter que des lettres allant de A à Z et de a à z, des nombres compris entre 0 et 9 et les quatre caractères spéciaux - _ . et +",					
					useridInvalid: "L'ID utilisateur ne doit pas commencer ou finir par un espace.",
					applianceInvalid: "L'ID utilisateur doit comporter uniquement des lettres allant de A à Z et de a à z, des nombres compris entre 0 et 9 et les quatre caractères spéciaux - _ . et +",					
					// TRNOTE: Do not translate groupidTooltip, groupidInvalid
					//groupidTooltip: "Enter the Group ID, which must not have leading or trailing spaces.",
					//groupidInvalid: "The Group ID must not have leading or trailing spaces.",
					SystemAdministrators: "Administrateurs système",
					MessagingAdministrators: "Administrateurs de messagerie",
					Users: "Utilisateurs",
					applianceUserInstruction: "Les administrateurs système peuvent créer, mettre à jour ou supprimer d'autres administrateurs de l'interface utilisateur Web et des utilisateurs de l'interface utilisateur Web.",
					// TRNOTE: Do not translate messagingUserInstruction, messagingGroupInstruction
					//messagingUserInstruction: "Messaging users can be added to messaging policies to control the messaging resources that a user can access.",
					//messagingGroupInstruction: "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.",				                    
					password1: "Mot de passe :",
					password2: "Confirmation du mot de passe :",
					password2Invalid: "Les mots de passe ne correspondent pas.",
					passwordNoSpaces: "Le mot de passe ne doit pas commencer ou finir par un espace.",
					saveButton: "Enregistrer",
					cancelButton: "Annuler",
					refreshingGroupsLabel: "Extraction de groupes..."
				},
				returnCodes: {
					CWLNA5056: "Certains paramètres n'ont pas pu être enregistrés."
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
				title: "Supprimer une entrée",
				content: "Voulez-vous vraiment supprimer cet enregistrement ?",
				deleteButton: "Supprimer",
				cancelButton: "Annuler"
			},
			savingProgress: "Enregistrement en cours...",
			savingFailed: "L'enregistrement a échoué.",
			deletingProgress: "Suppression en cours...",
			deletingFailed: "La suppression a échoué.",
			testingProgress: "Test en cours...",
			uploadingProgress: "Téléchargement en cours...",
			dialog: { 
				saveButton: "Enregistrer",
				cancelButton: "Annuler",
				testButton: "Tester la connexion"
			},
			invalidName: "Un enregistrement du même nom existe déjà.",
			invalidChars: "Un nom d'hôte ou une adresse IP valide est requis.",
			invalidRequired: "Une valeur est requise.",
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
				nothingToTest: "Vous devez fournir une adresse IPv4 ou IPv6 pour tester la connexion.",
				testConnFailed: "Le test de connexion a échoué.",
				testConnSuccess: "Le test de connexion a réussi.",
				testFailed: "Echec du test",
				testSuccess: "Le test a abouti."
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
				title: "Paramètres de sécurité",
				tagline: "Importez des clés et des certificats pour sécuriser les connexions au serveur.",
				securityProfilesSubTitle: "Profils de sécurité",
				securityProfilesTagline: "Les administrateurs système peuvent définir, modifier ou supprimer des profils de sécurité qui identifient les méthodes utilisées pour vérifier les clients.",
				certificateProfilesSubTitle: "Profils de certificat",
				certificateProfilesTagline: "Les administrateurs système peuvent définir, modifier ou supprimer des profils de certificat qui vérifient les identités utilisateur.",
				ltpaProfilesSubTitle: "Profils LTPA",
				ltpaProfilesTagline: "Les administrateurs système peuvent définir, modifier ou supprimer des profils LTPA (Lightweight Third Party Authentication) pour activer la connexion unique sur plusieurs serveurs.",
				oAuthProfilesSubTitle: "Profils OAuth",
				oAuthProfilesTagline: "Les administrateurs système peuvent définir, modifier ou supprimer des profils OAuth pour activer la connexion unique sur plusieurs serveurs.",
				systemWideSubTitle: "Paramètres de sécurité à l'échelle du serveur",
				useFips: "Utiliser le profil FIPS 140-2 pour sécuriser les communications de la messagerie",
				useFipsTooltip: "Sélectionnez cette option pour utiliser une cryptographie conforme à la norme FIPS (Federal Information Processing Standards) 140-2 pour les noeuds finaux sécurisés avec un profil de sécurité. " +
				                "Un redémarrage du serveur ${IMA_PRODUCTNAME_SHORT} est requis après la modification de ce paramètre.",
				retrieveFipsError: "Une erreur s'est produite lors de l'extraction du paramètre FIPS.",
				fipsDialog: {
					title: "Définir FIPS",
					content: "Le serveur ${IMA_PRODUCTNAME_SHORT} doit être redémarré pour que cette modification prenne effet. Voulez-vous vraiment modifier ce paramètre ?"	,
					okButton: "Définir et redémarrer",
					cancelButton: "Annuler la modification",
					failed: "Une erreur s'est produite lors de la modification du paramètre FIPS.",
					stopping: "Arrêt du serveur ${IMA_PRODUCTNAME_SHORT} en cours...",
					stoppingFailed: "Une erreur s'est produite lors de l'arrêt du serveur ${IMA_PRODUCTNAME_SHORT}.",
					starting: "Démarrage du serveur ${IMA_PRODUCTNAME_SHORT} en cours...",
					startingFailed: "Une erreur s'est produite lors du démarrage du serveur ${IMA_PRODUCTNAME_SHORT}."
				}
			},
			securityProfiles: {
				grid: {
					name: "Nom",
					mprotocol: "Méthode de protocole minimum",
					ciphers: "Chiffrements",
					certauth: "Utiliser des certificats client",
					clientciphers: "Utiliser les chiffrements client",
					passwordAuth: "Utiliser l'authentification par mot de passe",
					certprofile: "Profil de certificat",
					ltpaprofile: "Profil LTPA",
					oAuthProfile: "Profil OAuth",
					clientCertsButton: "Certificats de confiance"
				},
				trustedCertMissing: "Certificat de confiance manquant",
				retrieveError: "Une erreur s'est produite lors de l'extraction des profils de sécurité.",
				addDialogTitle: "Ajouter un profil de sécurité",
				editDialogTitle: "Modifier un profil de sécurité",
				removeDialogTitle: "Supprimer un profil de sécurité",
				dialog: {
					nameTooltip: "Nom composé de 32 caractères alphanumériques maximum. Le premier caractère ne doit pas être un nombre.",
					mprotocolTooltip: "Niveau de méthode de protocole autorisé le plus bas lors de la connexion.",
					ciphersTooltip: "<strong>Meilleure solution :&nbsp;</strong> Sélectionnez le chiffrement le plus sécurisé pris en charge par le serveur et le client.<br />" +
							        "<strong>Solution la plus rapide :&nbsp;</strong> Sélectionnez le chiffrement à haute sécurité le plus rapide pris en charge par le serveur et le client.<br />" +
							        "<strong>Solution moyenne :&nbsp;</strong> Sélectionnez le chiffrement de sécurité moyenne ou élevée le plus rapide pris en charge par le client.",
				    ciphers: {
				    	best: "Best",
				    	fast: "Fast",
				    	medium: "Medium"
				    },
					certauthTooltip: "Indiquez si le certificat client doit être authentifié.",
					clientciphersTooltip: "Indiquez si le client doit être autorisé à déterminer la suite de chiffrement à utiliser lors de la connexion.",
					passwordAuthTooltip: "Indiquez si un ID utilisateur et un mot de passe valides sont requis à la connexion.",
					passwordAuthTooltipDisabled: "Impossible à modifier car un profil LTPA ou OAuth est sélectionné. Pour modifier, changer le profil LTPA ou OAuth en <em>Choisir</em>.",
					certprofileTooltip: "Sélectionnez un profil de certificat existant à associer à ce profil de sécurité.",
					ltpaprofileTooltip: "Sélectionnez un profil LTPA existant à associer à ce profil de sécurité. " +
					                    "Si un profil LTPA est sélectionné, Utiliser l'authentification par mot de passe doit également être sélectionné.",
					ltpaprofileTooltipDisabled: "Impossible de sélectionner un profil LTPA car un profil OAuth est sélectionné ou Utiliser l'authentification par mot de passe n'est pas sélectionné.",
					oAuthProfileTooltip: "Sélectionnez un profil OAuth existant à associer à ce profil de sécurité. " +
                    					  "Si un profil OAuth est sélectionné, Utiliser l'authentification par mot de passe doit également être sélectionné.",
                    oAuthProfileTooltipDisabled: "Impossible de sélectionner un profil OAuth car un profil LTPA est sélectionné ou Utiliser l'authentification par mot de passe n'est pas sélectionné.",
					noCertProfilesTitle: "Aucun profil de certificat",
					noCertProfilesDetails: "Lorsque l'option <em>Utiliser TLS</em> est sélectionnée, un profil de sécurité ne peut être défini qu'après la définition d'un profil de certificat.",
					chooseOne: "Choisir"
				},
				certsDialog: {
					title: "Certificats de confiance",
					instruction: "Téléchargez ou supprimez des certificats dans le magasin de clés de confiance des certificats client pour un profil de sécurité",
					securityProfile: "Profil de sécurité",
					certificate: "Certificat",
					browse: "Parcourir",
					browseHint: "Sélection d'un fichier...",
					upload: "Télécharger",
					close: "Fermer",
					remove: "Supprimer",
					removeTitle: "Supprimer le certificat",
					removeContent: "Voulez-vous vraiment supprimer ce certificat ?",
					retrieveError: "Une erreur s'est produite lors de l'extraction des certificats.",
					uploadError: "Une erreur s'est produite lors du téléchargement du fichier certificat.",
					deleteError: "Une erreur s'est produite lors de la suppression du fichier certificat."	
				}
			},
			certificateProfiles: {
				grid: {
					name: "Nom",
					certificate: "Certificat",
					privkey: "Clé privée",
					certexpiry: "Expiration du certificat",
					noDate: "Erreur lors de l'extraction de la date..."
				},
				retrieveError: "Une erreur s'est produite lors de l'extraction des profils de certificat.",
				uploadCertError: "Une erreur s'est produite lors du téléchargement du fichier certificat.",
				uploadKeyError: "Une erreur s'est produite lors du téléchargement du fichier de clés.",
				uploadTimeoutError: "Une erreur s'est produite lors du téléchargement du fichier certificat ou de clés.",
				addDialogTitle: "Ajouter un profil de certificat",
				editDialogTitle: "Modifier un profil de certificat",
				removeDialogTitle: "Supprimer un profil de certificat",
				dialog: {
					certpassword: "Mot de passe du certificat :",
					keypassword: "Mot de passe de la clé :",
					browse: "Parcourir...",
					certificate: "Certificat :",
					certificateTooltip: "", // "Upload a certificate"
					privkey: "Clé privée :",
					privkeyTooltip: "", // "Upload a private key"
					browseHint: "Sélection d'un fichier...",
					duplicateFile: "Un fichier du même nom existe déjà.",
					identicalFile: "Le certificat ou la clé ne peut pas être dans le même fichier.",
					noFilesToUpload: "Pour mettre à jour le profil de certificat, sélectionnez au moins un fichier à télécharger."
				}
			},
			ltpaProfiles: {
				addDialogTitle: "Ajouter un profil LTPA",
				editDialogTitle: "Modifier un profil LTPA",
				instruction: "Utilisez un profil LTPA (Lightweight third-party authentication) dans un profil de sécurité pour activer la connexion unique sur plusieurs serveurs.",
				removeDialogTitle: "Supprimer un profil LTPA",				
				keyFilename: "Nom du fichier de clés",
				keyFilenameTooltip: "Fichier contenant la clé LTPA exportée.",
				browse: "Parcourir...",
				browseHint: "Sélection d'un fichier...",
				password: "Mot de passe",
				saveButton: "Enregistrer",
				cancelButton: "Annuler",
				retrieveError: "Une erreur s'est produite lors de l'extraction de profils LTPA.",
				duplicateFileError: "Un fichier de clés du même nom existe déjà.",
				uploadError: "Une erreur s'est produite lors du téléchargement du fichier de clés.",
				noFilesToUpload: "Pour mettre à jour le profil LTPA, sélectionnez un fichier à télécharger."
			},
			oAuthProfiles: {
				addDialogTitle: "Ajouter un profil OAuth",
				editDialogTitle: "Modifier un profil OAuth",
				instruction: "Utilisez un profil OAuth dans un profil de sécurité pour activer la connexion unique sur plusieurs serveurs.",
				removeDialogTitle: "Supprimer un profil OAuth",	
				resourceUrl: "URL de ressource",
				resourceUrlTooltip: "Adresse URL du serveur d'autorisations à utiliser pour valider le jeton d'accès. L'URL doit inclure le protocole. Le protocole peut être  HTTP ou HTTP sur SSL.",
				keyFilename: "Certificat du serveur OAuth",
				keyFilenameTooltip: "Nom du fichier contenant le certificat SSL utilisé pour se connecter au serveur OAuth.",
				browse: "Parcourir...",
				reset: "Réinitialiser",
				browseHint: "Sélection d'un fichier...",
				authKey: "Clé d'autorisation",
				authKeyTooltip: "Nom de la clé contenant le jeton d'accès. Le nom par défaut est <em>access_token</em>.",
				userInfoUrl: "URL des informations utilisateur",
				userInfoUrlTooltip: "Adresse URL du serveur d'autorisations à utiliser pour extraire les informations utilisateur. L'URL doit inclure le protocole. Le protocole peut être  HTTP ou HTTP sur SSL.",
				userInfoKey: "Clé des informations utilisateur",
				userInfoKeyTooltip: "Nom de la clé utilisée pour extraire les informations utilisateur.",
				saveButton: "Enregistrer",
				cancelButton: "Annuler",
				retrieveError: "Une erreur s'est produite lors de l'extraction de profils OAuth.",
				duplicateFileError: "Un fichier de clés du même nom existe déjà.",
				uploadError: "Une erreur s'est produite lors du téléchargement du fichier de clés.",
				urlThemeError: "Entrez une URL valide avec un protocole HTTP ou HTTP sur SSL.",
				inUseBySecurityPolicyMessage: "Le profil OAuth ne peut pas être supprimé car il est en cours d'utilisation par le profil de sécurité suivant : {0}.",			
				inUseBySecurityPoliciesMessage: "Le profil OAuth ne peut pas être supprimé car il est en cours d'utilisation par les profils de sécurité suivants : {0}."			
			},			
			systemControl: {
				pageTitle: "Contrôle du serveur",
				appliance: {
					subTitle: "Serveur",
					tagLine: "Paramètres et actions ayant un impact sur le serveur ${IMA_PRODUCTNAME_FULL}.",
					hostname: "Nom du serveur",
					hostnameNotSet: "(aucun)",
					retrieveHostnameError: "Une erreur s'est produite lors de l'extraction du nom de serveur.",
					hostnameError: "Une erreur s'est produite lors de la sauvegarde du nom de serveur.",
					editLink: "Modifier",
	                viewLink: "Afficher",
					firmware: "Version du microcode",
					retrieveFirmwareError: "Une erreur s'est produite lors de l'extraction de la version du microprogramme.",
					uptime: "Durée d'activité",
					retrieveUptimeError: "Une erreur s'est produite lors de l'extraction de la durée d'activité.",
					// TRNOTE Do not translate locateLED, locateLEDTooltip
					//locateLED: "Locate LED",
					//locateLEDTooltip: "Locate your appliance by turning on the blue LED.",
					locateOnLink: "Mettre sous tension",
					locateOffLink: "Mettre hors tension",
					locateLedOnError: "Une erreur s'est produite lors de la mise sous tension de l'indicateur du voyant de localisation.",
					locateLedOffError: "Une erreur s'est produite lors de la mise hors tension de l'indicateur du voyant de localisation.",
					locateLedOnSuccess: "L'indicateur du voyant de localisation a été mis sous tension.",
					locateLedOffSuccess: "L'indicateur du voyant de localisation a été mis hors tension.",
					restartLink: "Redémarrer le serveur",
					restartError: "Une erreur est survenue lors du redémarrage du serveur.",
					restartMessage: "La demande de redémarrage du serveur a été soumise.",
					restartMessageDetails: "Le redémarrage du serveur peut prendre plusieurs minutes. L'interface utilisateur n'est pas disponible pendant le redémarrage du serveur.",
					shutdownLink: "Arrêter le serveur",
					shutdownError: "Une erreur est survenue lors de l'arrêt du serveur.",
					shutdownMessage: "La demande d'arrêt du serveur a été soumise.",
					shutdownMessageDetails: "L'arrêt du serveur peut prendre plusieurs minutes. L'interface utilisateur n'est pas disponible pendant l'arrêt du serveur. " +
					                        "Pour remettre le serveur sous tension, appuyez sur le bouton d'alimentation du serveur.",
// TRNOTE Do not translate sshHost, sshHostSettings, sshHostStatus, sshHostTooltip, sshHostStatusTooltip,
// retrieveSSHHostError, sshSaveError
//					sshHost: "SSH Host",
//				    sshHostSettings: "Settings",
//				    sshHostStatus: "Status",
//				    sshHostTooltip: "Set the host IP address or addresses that can access the IBM IoT MessageSight SSH console.",
//				    sshHostStatusTooltip: "The host IP addresses that the SSH service is currently using.",
//				    retrieveSSHHostError: "An error occurred while retrieving the SSH host setting and status.",
//				    sshSaveError: "An error occurred while configuring IBM IoT MessageSight to allow SSH connections on the address or addresses provided.",

				    licensedUsageLabel: "Utilisation sous licence",
				    licensedUsageRetrieveError: "Une erreur s'est produite lors de l'extraction du paramètre d'utilisation sous licence.",
                    licensedUsageSaveError: "Une erreur s'est produite lors de la sauvegarde du paramètre d'utilisation sous licence.",
				    licensedUsageValues: {
				        developers: "Développeurs",
				        nonProduction: "Hors production",
				        production: "Production"
				    },
				    licensedUsageDialog: {
				        title: "Modification de l'utilisation sous licence",
				        instruction: "Modifiez l'utilisation sous licence du serveur. " +
				        		"Si vous disposez d'une autorisation d'utilisation du logiciel, l'utilisation sous licence que vous sélectionnez doit correspondre à l'utilisation autorisée du programme, conformément à cette autorisation d'utilisation du logiciel, à savoir Production ou Hors production. " +
				        		"Si l'autorisation d'utilisation du logiciel désigne le programme comme un programme hors production, Hors production doit être sélectionné. " +
				        		"Si aucune désignation n'apparaît dans l'autorisation d'utilisation du logiciel, votre utilisation est supposée être en production et Production doit être sélectionné. " +
				        		"Si vous ne possédez pas d'autorisation d'utilisation du logiciel, Développeurs doit être sélectionné.",
				        content: "Lorsque vous modifiez l'utilisation sous licence du serveur, le serveur redémarre. " +
				        		"Vous êtes déconnecté de l'interface utilisateur et l'interface utilisateur est indisponible jusqu'à ce que le serveur soit redémarré. " +
				        		"Le redémarrage du serveur peut prendre plusieurs minutes. " +
				        		"Une fois le serveur redémarré, vous devez vous connecter à l'interface utilisateur Web et accepter la nouvelle licence.",
				        saveButton: "Sauvegarder et redémarrer le serveur",
				        cancelButton: "Annuler"
				    },
					restartDialog: {
						title: "Redémarrage du serveur",
						content: "Voulez-vous vraiment redémarrer le serveur ? " +
								"Vous êtes déconnecté de l'interface utilisateur et l'interface utilisateur est indisponible jusqu'à ce que le serveur soit redémarré. " +
								"Le redémarrage du serveur peut prendre plusieurs minutes.",
						okButton: "Redémarrer",
						cancelButton: "Annuler"												
					},
					shutdownDialog: {
						title: "Arrêt du serveur",
						content: "Voulez-vous vraiment arrêter le serveur ? " +
								 "Vous allez être déconnecté de l'interface utilisateur et l'interface utilisateur sera indisponible jusqu'au redémarrage du serveur. " +
								 "Pour remettre le serveur sous tension, appuyez sur le bouton d'alimentation du serveur.",
						okButton: "Arrêter",
						cancelButton: "Annuler"												
					},

					hostnameDialog: {
						title: "Edition du nom de serveur",
						instruction: "Spécifiez un nom de serveur ne comportant pas plus de 1024 caractères.",
						invalid: "La valeur entrée n'est pas valide. Le nom de serveur peut contenir des caractères UTF-8 valides.",
						tooltip: "Entrez le nom du serveur.  Le nom de serveur peut contenir des caractères UTF-8 valides.",
						hostname: "Nom du serveur",
						okButton: "Enregistrer",
						cancelButton: "Annuler"												
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
					subTitle: "Serveur ${IMA_PRODUCTNAME_FULL}",
					tagLine: "Paramètres et actions affectant le serveur ${IMA_PRODUCTNAME_SHORT}.",
					status: "Statut",
					retrieveStatusError: "Une erreur s'est produite lors de l'extraction du statut.",
					stopLink: "Arrêter le serveur",
					forceStopLink: "Forcer le serveur à s'arrêter",
					startLink: "Démarrer le serveur",
					starting: "Démarrage en cours",
					unknown: "Inconnu",
					cleanStore: "Mode maintenance (nettoyage du magasin en cours)",
					startError: "Une erreur s'est produite lors du démarrage du serveur ${IMA_PRODUCTNAME_SHORT}.",
					stopping: "Arrêt",
					stopError: "Une erreur s'est produite lors de l'arrêt du serveur ${IMA_PRODUCTNAME_SHORT}.",
					stopDialog: {
						title: "Arrêter le serveur ${IMA_PRODUCTNAME_FULL}",
						content: "Voulez-vous vraiment arrêter le serveur ${IMA_PRODUCTNAME_SHORT} ?",
						okButton: "Arrêter",
						cancelButton: "Annuler"						
					},
					forceStopDialog: {
						title: "Forcer l'arrêt du serveur ${IMA_PRODUCTNAME_FULL}",
						content: "Voulez-vous vraiment forcer l'arrêt du serveur ${IMA_PRODUCTNAME_SHORT} ? Cette option est à utiliser en dernier recours car elle peut entraîner la perte de messages.",
						okButton: "Forcer l'arrêt",
						cancelButton: "Annuler"						
					},

					memory: "Mémoire"
				},
				mqconnectivity: {
					subTitle: "Service MQ Connectivity",
					tagLine: "Paramètres et actions affectant le service MQ Connectivity.",
					status: "Statut",
					retrieveStatusError: "Une erreur s'est produite lors de l'extraction du statut.",
					stopLink: "Arrêter le service",
					startLink: "Démarrer le service",
					starting: "Démarrage en cours",
					unknown: "Inconnu",
					startError: "Une erreur s'est produite lors du démarrage du service MQ Connectivity.",
					stopping: "Arrêt",
					stopError: "Une erreur s'est produite lors de l'arrêt du service MQ Connectivity.",
					started: "Exécution",
					stopped: "Arrêté",
					stopDialog: {
						title: "Arrêter le service MQ Connectivity",
						content: "Voulez-vous vraiment arrêter le service MQ Connectivity ?  L'arrêt du service peut provoquer la perte de messages.",
						okButton: "Arrêter",
						cancelButton: "Annuler"						
					}
				},
				runMode: {
					runModeLabel: "Mode exécution",
					cleanStoreLabel: "Nettoyer le magasin",
				    runModeTooltip: "Indiquez le mode d'exécution du serveur ${IMA_PRODUCTNAME_SHORT}. Lorsqu'un nouveau mode d'exécution est sélectionné et confirmé, il ne prend effet qu'une fois le serveur ${IMA_PRODUCTNAME_SHORT} redémarré.",
				    runModeTooltipDisabled: "Le changement du mode d'exécution n'est pas disponible en raison du statut du serveur ${IMA_PRODUCTNAME_SHORT}.",
				    cleanStoreTooltip: "Indiquez si vous souhaitez que l'action Nettoyage du magasin s'exécute lorsque le serveur ${IMA_PRODUCTNAME_SHORT} est redémarré. L'action Nettoyage du magasin ne peut être sélectionnée que lorsque le serveur ${IMA_PRODUCTNAME_SHORT} est en mode maintenance.",
					// TRNOTE {0} is a mode, such as production or maintenance.
				    setRunModeError: "Une erreur s'est produite lors de la tentative de changement de mode d'exécution du serveur ${IMA_PRODUCTNAME_SHORT} en {0}.",
					retrieveRunModeError: "Une erreur s'est produite lors de l'extraction du mode d'exécution du serveur ${IMA_PRODUCTNAME_SHORT}.",
					runModeDialog: {
						title: "Confirmation du changement de mode d'exécution",
						// TRNOTE {0} is a mode, such as production or maintenance.
						content: "Voulez-vous vraiment changer le mode d'exécution du serveur ${IMA_PRODUCTNAME_SHORT} en <strong>{0}</strong> ? Lorsqu'un nouveau mode d'exécution est sélectionné, le serveur ${IMA_PRODUCTNAME_SHORT} doit être redémarré pour qu'il puisse prendre effet.",
						okButton: "Confirmer",
						cancelButton: "Annuler"												
					},
					cleanStoreDialog: {
						title: "Confirmation du changement de l'action Nettoyer le magasin",
						contentChecked: "Voulez-vous vraiment que l'action <strong>Nettoyer le magasin</strong> s'exécute lors du redémarrage du serveur ? " +
								 "Si l'action <strong>Nettoyage du magasin</strong> est définie, le serveur ${IMA_PRODUCTNAME_SHORT} doit être redémarré pour exécuter l'action. " +
						         "L'exécution de l'action <strong>Nettoyer le magasin</strong> entraîne la perte des données de messagerie conservées.<br/><br/>",
						content: "Voulez-vous vraiment annuler l'action <strong>Nettoyer le magasin</strong> ?",
						okButton: "Confirmer",
						cancelButton: "Annuler"												
					}
				},
				storeMode: {
					storeModeLabel: "Mode de stockage",
				    storeModeTooltip: "Indiquez le mode de stockage du serveur ${IMA_PRODUCTNAME_SHORT}. Une fois qu'un nouveau mode de stockage a été sélectionné et confirmé, le serveur ${IMA_PRODUCTNAME_SHORT} redémarre et un nettoyage du magasin est effectué.",
				    storeModeTooltipDisabled: "Le changement du mode de stockage n'est pas disponible en raison du statut du serveur ${IMA_PRODUCTNAME_SHORT}.",
				    // TRNOTE {0} is a mode, such as production or maintenance.
				    setStoreModeError: "Une erreur s'est produite lors de la tentative de changement de mode de stockage du serveur ${IMA_PRODUCTNAME_SHORT} en {0}.",
					retrieveStoreModeError: "Une erreur s'est produite lors de l'extraction du mode de stockage du serveur ${IMA_PRODUCTNAME_SHORT}.",
					storeModeDialog: {
						title: "Confirmation du changement de mode de stockage",
						// TRNOTE {0} is a mode, such as persistent or memory only.
						content: "Voulez-vous vraiment changer le mode de stockage du serveur ${IMA_PRODUCTNAME_SHORT} en <strong>{0}</strong> ? " + 
								"Si le mode de stockage est modifié, le serveur ${IMA_PRODUCTNAME_SHORT} est redémarré et une nouvelle action de nettoyage de magasin est effectuée. " +
								"L'exécution de l'action Nettoyer le magasin entraîne la perte des données de messagerie conservées. Lorsque le mode de stockage est modifié, un nettoyage de magasin doit être effectué.",
						okButton: "Confirmer",
						cancelButton: "Annuler"												
					}
				},
				logLevel: {
					subTitle: "Définir le niveau de journalisation",
					tagLine: "Définissez le niveau de messages que vous souhaitez voir dans les journaux serveur.  " +
							 "Les journaux peuvent être téléchargés depuis <a href='monitoring.jsp?nav=downloadLogs'>Surveillance > Téléchargement des journaux</a>.",
					logLevel: "Journal par défaut",
					connectionLog: "Journal de connexion",
					securityLog: "Journal de sécurité",
					adminLog: "Journal d'administration",
					mqconnectivityLog: "Journal MQ Connectivity",
					tooltip: "Le niveau de journalisation contrôle les types de messages dans le fichier journal par défaut. " +
							 "Ce fichier journal affiche toutes les entrées de journal sur le fonctionnement du serveur. " +
							 "Les entrées de gravité élevée présentées dans d'autres journaux sont également consignées dans ce journal. " +
							 "Un paramètre Minimum n'inclut que les entrées les plus importantes. " +
							 "Un paramètre Maximum inclut toutes les entrées. " +
							 "Pour plus d'informations sur les niveaux de journalisation, voir la documentation.",
					connectionTooltip: "Le niveau de journalisation contrôle les types de messages dans le fichier journal de connexion. " +
							           "Ce fichier journal affiche les entrées de journal relatives aux connexions. " +
							           "Ces entrées peuvent être utilisées comme journal d'audit pour les connexions et indiquent également les erreurs liées aux connexions. " +
							           "Un paramètre Minimum n'inclut que les entrées les plus importantes. Un paramètre Maximum inclut toutes les entrées. " +
					                   "Pour plus d'informations sur les niveaux de journalisation, voir la documentation. ",
					securityTooltip: "Le niveau de journalisation contrôle les types de messages dans le fichier journal de sécurité. " +
							         "Ce fichier journal affiche les entrées de journal relatives à l'authentification et l'autorisation. " +
							         "Ce journal peut être utilisé comme journal d'audit pour les éléments liés à la sécurité. " +
							         "Un paramètre Minimum n'inclut que les entrées les plus importantes. Un paramètre Maximum inclut toutes les entrées. " +
					                 "Pour plus d'informations sur les niveaux de journalisation, voir la documentation.",
					adminTooltip: "Le niveau de journalisation contrôle les types de messages dans le fichier journal d'administration. " +
							      "Ce fichier journal affiche les entrées de journal relatives à l'exécution des commandes d'administration à partir de l'interface utilisateur Web d'${IMA_PRODUCTNAME_FULL} ou de la ligne de commande. " +
							      "Il enregistre toutes les modifications apportées à la configuration du serveur ainsi que les modifications tentées. " +
								  "Un paramètre Minimum n'inclut que les entrées les plus importantes. " +
								  "Un paramètre Maximum inclut toutes les entrées. " +
					              "Pour plus d'informations sur les niveaux de journalisation, voir la documentation.",
					mqconnectivityTooltip: "Le niveau de journalisation contrôle les types de messages dans le fichier journal MQ Connectivity. " +
							               "Ce fichier journal affiche les entrées journal relatives à la fonction MQ Connectivity. " +
							               "Ces entrées incluent les problèmes détectés lors de la connexion à des gestionnaires de files d'attente. " +
							               "Le journal peut vous aider à diagnostiquer les problèmes de connexion d'${IMA_PRODUCTNAME_FULL} à WebSphere<sup>&reg;</sup> MQ. " +
										   "Un paramètre Minimum n'inclut que les entrées les plus importantes. " +
										   "Un paramètre Maximum inclut toutes les entrées. " +
										   "Pour plus d'informations sur les niveaux de journalisation, voir la documentation.",										
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log										   
					retrieveLogLevelError: "Une erreur s'est produite lors de l'extraction du niveau {0}.",
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log
					setLogLevelError: "Une erreur s'est produite lors de la définition du niveau {0}.",
					levels: {
						MIN: "Minimum",
						NORMAL: "Normal",
						MAX: "Maximum"
					},
					serverNotAvailable: "Le niveau de journalisation ne peut pas être défini lorsque le serveur ${IMA_PRODUCTNAME_FULL} est arrêté.",
					serverCleanStore: "Le niveau de journalisation ne peut pas être défini lorsqu'un nettoyage de magasin est en cours.",
					serverSyncState: "Le niveau de journalisation ne peut pas être défini lorsque le noeud principal est en cous de synchronisation.",
					mqNotAvailable: "Le niveau de journalisation de MQ Connectivity ne peut pas être défini lorsque le service MQ Connectivity est arrêté.",
					mqStandbyState: "Le niveau de journalisation de MQ Connectivity ne peut pas être défini lorsque le serveur est en veille."
				}
			},
			highAvailability: {
				title: "Haute disponibilité",
				tagline: "Configurez deux serveurs ${IMA_PRODUCTNAME_SHORT} pour qu'ils agissent comme une paire de noeuds à haute disponibilité.",
				stateLabel: "Statut actuel de la haute disponibilité :",
				statePrimary: "Principal",
				stateStandby: "De secours",
				stateIndeterminate: "Indéterminé",
				error: "La haute disponibilité a rencontré une erreur.",
				save: "Enregistrer",
				saving: "Enregistrement en cours...",
				success: "L'enregistrement a abouti. Le serveur ${IMA_PRODUCTNAME_SHORT} et le service de connectivité MQ doivent être redémarrés pour que les modifications prennent effet.",
				saveFailed: "Une erreur s'est produite lors de la mise à jour des paramètres de la haute disponibilité.",
				ipAddrValidation : "Indiquez une adresse IPv4 ou IPv6, par exemple 192.168.1.0",
				discoverySameAsReplicationError: "L'adresse de reconnaissance doit être différente de l'adresse de réplication.",
				AutoDetect: "Détection automatique",
				StandAlone: "Version autonome",
				notSet: "Non défini",
				yes: "Oui",
				no: "Non",
				config: {
					title: "Configuration",
					tagline: "La configuration de la haute disponibilité est affichée. " +
					         "Les modifications apportées à la configuration ne prennent effet qu'une fois le serveur ${IMA_PRODUCTNAME_SHORT} redémarré.",
					editLink: "Modifier",
					enabledLabel: "Haute disponibilité activée :",
					haGroupLabel: "Groupe de haute disponibilité :",
					startupModeLabel: "Mode de démarrage :",
					replicationInterfaceLabel: "Adresse de réplication :",
					discoveryInterfaceLabel: "Adresse de reconnaissance :",
					remoteDiscoveryInterfaceLabel: "Adresse de reconnaissance distante :",
					discoveryTimeoutLabel: "Délai de reconnaissance :",
					heartbeatLabel: "Délai du signal de présence :",
					seconds: "{0} secondes",
					secondsDefault: "{0} secondes (par défaut)",
					preferedPrimary: "Détection automatique (principale recommandée)"
				},
				restartInfoDialog: {
					title: "Redémarrage requis",
					content: "Vos modifications ont été enregistrées mais ne prendront effet qu'une fois le serveur ${IMA_PRODUCTNAME_SHORT} redémarré.<br /><br />" +
							 "Pour redémarrer le serveur ${IMA_PRODUCTNAME_SHORT} maintenant, cliquez sur <b>Redémarrer maintenant</b>. Sinon, cliquez sur <b>Redémarrer ultérieurement</b>.",
					closeButton: "Redémarrer ultérieurement",
					restartButton: "Redémarrer maintenant",
					stopping: "Arrêt du serveur ${IMA_PRODUCTNAME_SHORT} en cours...",
					stoppingFailed: "Une erreur s'est produite lors de l'arrêt du serveur ${IMA_PRODUCTNAME_SHORT}. " +
								"Utilisez la page Contrôle du serveur pour redémarrer le serveur I${IMA_PRODUCTNAME_SHORT}.",
					starting: "Démarrage du serveur ${IMA_PRODUCTNAME_SHORT} en cours..."
//					startingFailed: "An error occurred while starting the IBM IoT MessageSight server. " +
//								"Use the System Control page to start the IBM IoT MessageSight server."
				},
				confirmGroupNameDialog: {
					title: "Confirmer le changement de nom de groupe",
					content: "Un seul noeud est actuellement actif. Si le nom de groupe est modifié, vous devrez peut-être modifier manuellement le nom de groupe des autres noeuds. " +
					         "Voulez-vous vraiment changer le nom de groupe ?",
					closeButton: "Annuler",
					confirmButton: "Confirmer"
				},
				dialog: {
					title: "Modification de la configuration à haute disponibilité",
					saveButton: "Enregistrer",
					cancelButton: "Annuler",
					instruction: "Activez la haute disponibilité et définissez un groupe de haute disponibilité pour déterminer à quel serveur associer ce serveur. " + 
								 "Sinon, désactivez la haute disponibilité. Les modifications apportées à la configuration à haute disponibilité ne prennent effet que lorsque vous redémarrez le serveur ${IMA_PRODUCTNAME_SHORT}.",
					haEnabledLabel: "Haute disponibilité activée :",
					startupMode: "Mode de démarrage :",
					haEnabledTooltip: "Texte de l'infobulle",
					groupNameLabel: "Groupe de haute disponibilité :",
					groupNameToolTip: "Le groupe est utilisé pour configurer automatiquement des serveurs à associer les uns aux autres. " +
					                  "La valeur doit être la même sur les deux serveurs. " +
					                  "La longueur maximale de cette valeur est de 128 caractères. ",
					advancedSettings: "Paramètres avancés",
					advSettingsTagline: "Si vous souhaitez des paramètres particuliers pour votre configuration à haute disponibilité, vous pouvez configurer manuellement les paramètres suivants. " +
										"Consultez la documentation d'${IMA_PRODUCTNAME_FULL} pour des instructions détaillées et des recommandations avant de modifier ces paramètres.",
					nodePreferred: "Lorsque les deux noeuds démarrent en mode de détection automatique, ce noeud est le noeud principal favori.",
				    localReplicationInterface: "Adresse de réplication locale :",
				    localDiscoveryInterface: "Adresse de reconnaissance locale :",
				    remoteDiscoveryInterface: "Adresse de reconnaissance distante :",
				    discoveryTimeout: "Délai de reconnaissance :",
				    heartbeatTimeout: "Délai du signal de présence :",
				    startModeTooltip: "Mode dans lequel le noeud démarre. Un noeud peut être défini soit en mode de détection automatique, soit en mode autonome. " +
				    				  "En mode détection automatique, les deux noeuds doivent être démarrés. " +
				    				  "Les noeuds essaient automatiquement de se détecter mutuellement et d'établir une paire à haute disponibilité. " +
				    				  "N'utilisez le mode autonome que si vous démarrez un seul noeud.",
				    localReplicationInterfaceTooltip: "Adresse IP de la carte d'interface réseau utilisée pour la réplication à haute disponibilité sur le noeud local dans la paire à haute disponibilité. " +
				    								  "Vous pouvez choisir l'adresse IP que vous souhaitez affecter. " +
				    								  "Toutefois, l'adresse de réplication locale doit se trouver sur un sous-réseau différent de celui de l'adresse de reconnaissance locale.",
				    localDiscoveryInterfaceTooltip: "Adresse IP de la carte d'interface réseau utilisée pour la reconnaissance à haute disponibilité sur le noeud local dans la paire à haute disponibilité. " +
				    							    "Vous pouvez choisir l'adresse IP que vous souhaitez affecter. " +
				    								"Toutefois, l'adresse de reconnaissance locale doit se trouver sur un sous-réseau différent de celui de l'adresse de réplication locale.",
				    remoteDiscoveryInterfaceTooltip: "Adresse IP de la carte d'interface réseau utilisée pour la reconnaissance à haute disponibilité sur le noeud distant dans la paire à haute disponibilité. " +
				    								 "Vous pouvez choisir l'adresse IP que vous souhaitez affecter. " +
				    								 "Toutefois, l'adresse de reconnaissance distante doit se trouver sur un sous-réseau différent de celui de l'adresse de réplication locale.",
				    discoveryTimeoutTooltip: "Délai, en secondes, pendant lequel un serveur démarré en mode détection automatique doit se connecter à l'autre serveur dans la paire à haute disponibilité. " +
				    					     "La plage valide est comprise entre 10 et 2147483647. La valeur par défaut est de 600. " +
				    						 "Si la connexion n'est pas établie dans ce délai, le serveur démarre en mode maintenance.",
				    heartbeatTimeoutTooltip: "Délai, en secondes, pendant lequel un serveur doit déterminer si l'autre serveur de la paire à haute disponibilité est défaillant. " + 
				    						 "La plage valide est comprise entre 1 et 2147483647. La valeur par défaut est de 10. " +
				    						 "Si le serveur principal ne reçoit pas de signal de présence du serveur de secours dans ce délai, il continue à fonctionner en tant que serveur principal mais le processus de synchronisation des données est arrêté. " +
				    						 "Si le serveur de secours ne reçoit pas de signal de présence du serveur principal, le serveur de secours devient le serveur principal.",
				    seconds: "secondes",
				    nicsInvalid: "L'adresse de réplication locale, l'adresse de reconnaissance locale et l'adresse de reconnaissance distante doivent toutes avoir des valeurs valides. " +
				    			 "Sinon, l'adresse de réplication locale, l'adresse de reconnaissance locale et l'adresse de reconnaissance distante peuvent ne pas avoir de valeurs fournies. ",
				    securityRiskTitle: "Risque de sécurité",
				    securityRiskText: "Voulez-vous vraiment activer la haute disponibilité sans spécifier d'adresses de réplication et de reconnaissance ? " +
				    		"Si vous ne disposez pas d'un contrôle complet sur l'environnement réseau de votre serveur, les informations sur votre serveur seront en multidiffusion et il se peut que votre serveur soit associé à un serveur non sécurisé qui utilise le même nom de groupe de haute disponibilité." +
				    		"<br /><br />" +
				    		"Vous pouvez éliminer ce risque de sécurité en spécifiant les adresses de réplication et de reconnaissance."
				},
				status: {
					title: "Statut",
					tagline: "La statut de la haute disponibilité est affiché. " +
					         "Si le statut ne correspond pas à la configuration, il est possible que le serveur ${IMA_PRODUCTNAME_SHORT} doive être redémarré pour que les modifications récentes apportées à la configuration prennent effet.",
					editLink: "Modifier",
					haLabel: "Haute disponibilité :",
					haGroupLabel: "Groupe de haute disponibilité :",
					haNodesLabel: "Noeuds actifs",
					pairedWithLabel: "Dernier appariement avec :",
					replicationInterfaceLabel: "Adresse de réplication :",
					discoveryInterfaceLabel: "Adresse de reconnaissance :",
					remoteDiscoveryInterfaceLabel: "Adresse de reconnaissance distante :",
					remoteWebUI: "<a href='{0}' target=_'blank'>Interface utilisateur Web distant</a>",
					enabled: "La haute disponibilité est activée.",
					disabled: "La haute disponibilité est désactivée."
				},
				startup: {
					subtitle: "Mode de démarrage du noeud",
					tagline: "Un noeud peut être défini soit en mode de détection automatique, soit en mode autonome. En mode détection automatique, les deux noeuds doivent être démarrés. " +
							 "Les noeuds essaieront automatiquement de se détecter mutuellement et d'établir une paire à haute disponibilité. Le mode autonome doit uniquement être utilisé lors du démarrage d'un noeud unique.",
					mode: "Mode de démarrage :",
					modeAuto: "Détection automatique",
					modeAlone: "Version autonome",
					primary: "Lorsque les deux noeuds démarrent en mode de détection automatique, ce noeud est le noeud principal favori."
				},
				remote: {
					subtitle: "Adresse de reconnaissance de noeud distant",
					tagline: "Adresse IPv4 ou IPv6 de l'adresse de reconnaissance sur le noeud distant dans la paire à haute disponibilité.",
					discoveryInt: "Adresse de reconnaissance :",
					discoveryIntTooltip: "Entrez l'adresse IPv4 ou IPv6 pour la configuration de l'adresse de reconnaissance du noeud distant."
				},
				local: {
					subtitle: "Adresse de la carte d'interface réseau à haute disponibilité",
					tagline: "Adresses IPv4 ou IPv6 des adresses de réplication et de reconnaissance du noeud local.",
					replicationInt: "Adresse de réplication :",
					replicationIntTooltip: "Entrez l'adresse IPv4 ou IPv6 pour la configuration de l'adresse de réplication du noeud local.",
					replicationError: "L'adresse de réplication à haute disponibilité n'est pas disponible.",
					discoveryInt: "Adresse de reconnaissance :",
					discoveryIntTooltip: "Entrez l'adresse IPv4 ou IPv6 pour la configuration de l'adresse de reconnaissance du noeud local."
				},
				timeouts: {
					subtitle: "Délais d'expiration",
					tagline: "Lorsqu'un noeud activé pour la haute disponibilité est démarré en mode de détection automatique, il tente de communiquer avec l'autre noeud dans la paire à haute disponibilité. " +
							 "Si une connexion ne peut pas être établie pendant le délai d'expiration de reconnaissance, le noeud démarre alors en mode maintenance.",
					tagline2: "Le délai d'expiration du signal de présence est utilisé pour détecter si l'autre noeud d'une paire à haute disponibilité a échoué. " +
							  "Si le noeud principal ne reçoit pas de signal de présence du noeud de secours, il continue à travailler en tant que noeud principal mais le processus de synchronisation des données est arrêté. " +
							  "Si le noeud de secours ne reçoit pas de signal de présence du noeud principal, le noeud de secours devient noeud principal.",
					discoveryTimeout: "Délai de reconnaissance :",
					discoveryTimeoutTooltip: "Nombre de secondes compris entre 10 et 2,147,483,647 pendant lequel le noeud tente de se connecter à l'autre noeud dans la paire.",
					discoveryTimeoutValidation: "Entrez un nombre compris entre 10 et 2,147,483,647.",
					discoveryTimeoutError: "Le délai de reconnaissance n'est pas disponible.",
					heartbeatTimeout: "Délai du signal de présence :",
					heartbeatTimeoutTooltip: "Nombre de secondes compris entre 1 et 2,147,483,647 pendant lequel le signal de présence du noeud peut être détecté.",
					heartbeatTimeoutValidation: "Entrez un nombre compris entre 1 et 2,147,483,647.",
					seconds: "secondes"
				}
			},
			webUISecurity: {
				title: "Paramètres de l'interface utilisateur Web",
				tagline: "Configurez des paramètres pour l'interface utilisateur Web.",
				certificate: {
					subtitle: "Certificat de l'interface utilisateur Web",
					tagline: "Configurez le certificat utilisé pour sécuriser des communications avec l'interface utilisateur Web."
				},
				timeout: {
					subtitle: "Délai d'attente de session &amp; Expiration du jeton LTPA",
					tagline: "Ces paramètres contrôlent les sessions de connexion utilisateur basées sur l'activité entre le navigateur et le serveur ainsi que la durée totale de la connexion. " +
							 "La modification du délai d'inactivité de session ou de l'expiration du jeton LTPA peut invalider toutes les sessions de connexion actives.",
					inactivity: "Délai d'inactivité de session",
					inactivityTooltip: "Le délai d'inactivité de session contrôle la durée pendant laquelle un utilisateur peut rester connecté en cas d'inactivité entre le  navigateur et le serveur. " +
							           "Le délai d'inactivité doit être inférieur ou égal à l'expiration du jeton LTPA.",
					expiration: "Expiration du jeton LTPA",
					expirationTooltip: "L'expiration du jeton LTPA (Lightweight Third-Party Authentication) contrôle la durée pendant laquelle un utilisateur peut rester connecté, quelle que soit l'activité entre le navigateur et le serveur. " +
							           "Le délai d'expiration doit être supérieur ou égal au délai d'inactivité de session. Pour plus d'informations, voir la documentation.", 
					unit: "minutes",
					save: "Enregistrer",
					saving: "Enregistrement en cours...",
					success: "L'enregistrement a abouti. Veuillez vous déconnecter, puis vous reconnecter.",
					failed: "L'enregistrement a échoué.",
					invalid: {
						inactivity: "Le délai d'inactivité de session doit être un nombre compris entre 1 et la valeur d'expiration du jeton LTPA.",
						expiration: "La valeur d'expiration du jeton LTPA doit être un nombre compris entre la valeur d'expiration de l'inactivité de session et 1440."
					},
					getExpirationError: "Une erreur s'est produite lors de l'obtention du délai d'expiration du jeton LTPA.",
					getInactivityError: "Une erreur s'est produite lors de l'obtention du délai d'inactivité de session.",
					setExpirationError: "Une erreur s'est produite lors de la définition du délai d'expiration du jeton LTPA.",
					setInactivityError: "Une erreur s'est produite lors de la définition du délai d'inactivité de session."
				},
				cacheauth: {
					subtitle: "Délai d'expiration de la mémoire cache d’authentification",
					tagline: "Le délai d'expiration du cache d'authentification indique combien de temps des données d'identification authentifiées dans le cache sont valides. " +
					"Des valeurs plus élevées peuvent accroître le risque de sécurité car des données d'identification révoquées restent en cache plus longtemps. " +
					"Des valeurs plus faibles peuvent affecter les performances car le registre d'utilisateurs est sollicité plus fréquemment.",
					cacheauth: "Délai d'expiration de la mémoire cache d’authentification",
					cacheauthTooltip: "Le délai d'expiration du cache d'authentification indique le délai après lequel une entrée du cache est supprimé. " +
							"La valeur doit être comprise entre 1 et 3600 secondes (1 heure). La valeur ne doit pas être supérieure à un tiers de la valeur d'expiration du jeton LTPA. " +
							"Pour plus d'informations, voir la documentation.", 
					unit: "secondes",
					save: "Enregistrer",
					saving: "Enregistrement en cours...",
					success: "L'enregistrement a abouti.",
					failed: "L'enregistrement a échoué.",
					invalid: {
						cacheauth: "La valeur d'expiration du cache d'authentification doit être un nombre compris entre 1 et 3600."
					},
					getCacheauthError: "Une erreur s'est produite lors de l'obtention de la valeur d'expiration du cache d'authentification.",
					setCacheauthError: "Une erreur s'est produite lors de la définition de la valeur d'expiration du cache d'authentification."
				},
				readTimeout: {
					subtitle: "Délai d'expiration de lecture HTTP",
					tagline: "Durée d'attente maximale pour l'achèvement d'une demande de lecture sur un socket après la première lecture.",
					readTimeout: "Délai d'expiration de lecture HTTP",
					readTimeoutTooltip: "Valeur comprise entre 1 et 300 secondes (5 minutes).", 
					unit: "secondes",
					save: "Enregistrer",
					saving: "Enregistrement en cours...",
					success: "L'enregistrement a abouti.",
					failed: "L'enregistrement a échoué.",
					invalid: {
						readTimeout: "La valeur de délai d'expiration de lecture HTTP doit être comprise entre 1 et 300."
					},
					getReadTimeoutError: "Une erreur s'est produite lors de l'obtention du délai d'expiration de lecture HTTP.",
					setReadTimeoutError: "Une erreur s'est produite lors de la définition du délai d'expiration de lecture HTTP."
				},
				port: {
					subtitle: "Adresse IP et port",
					tagline: "Sélectionnez l'adresse IP et le port pour la communication de l'interface utilisateur Web. La modification de l'une des valeurs peut nécessiter que vous vous connectiez à la nouvelle adresse IP ou au nouveau port.  " +
							 "<em>La sélection de toutes les adresses IP (\"All\") n'est pas conseillée car cela peut exposer l'interface utilisateur Web à Internet.</em>",
					ipAddress: "Adresse IP",
					all: "Tous",
					interfaceHint: "Interface : {0}",
					port: "Port",
					save: "Enregistrer",
					saving: "Enregistrement en cours...",
					success: "L'enregistrement a abouti. Il est possible qu'il faille attendre une minute pour que la modification prenne effet.  Veuillez fermer votre navigateur et vous connecter à nouveau.",
					failed: "L'enregistrement a échoué.",
					tooltip: {
						port: "Entrez un port disponible. Les ports valides sont compris entre 1 et 8999, 9087 ou 9100 et 65535, inclus."
					},
					invalid: {
						interfaceDown: "L'interface {0} indique qu'elle s'est arrêtée. Veuillez vérifier l'interface ou sélectionner une autre adresse.",
						interfaceUnknownState: "Une erreur s'est produite lors de la demande de l'état de {0}. Veuillez vérifier l'interface ou sélectionner une autre adresse.",
						port: "Le numéro de port doit être compris entre 1 et 8999, 9087 ou 9100 et 65535, inclus."
					},					
					getIPAddressError: "Une erreur s'est produite lors de l'obtention de l'adresse IP.",
					getPortError: "Une erreur s'est produite lors de l’obtention du port.",
					setIPAddressError: "Une erreur s'est produite lors de la définition de l'adresse IP.",
					setPortError: "Une erreur s'est produite lors de la définition du port."					
				},
				certificate: {
					subtitle: "Certificat de sécurité",
					tagline: "Téléchargez et remplacez le certificat utilisé par l'interface utilisateur Web.",
					addButton: "Ajouter un certificat",
					editButton: "Editer un certificat",
					addTitle: "Remplacer le certificat de l'interface utilisateur Web",
					editTitle: "Editer le certificat de l'interface utilisateur Web",
					certificateLabel: "Certificat :",
					keyLabel: "Clé privée :",
					expiryLabel: "Expiration du certificat :",
					defaultExpiryDate: "Avertissement ! Le certificat par défaut est en cours d'utilisation et doit être remplacé.",
					defaultCert: "Valeur par défaut",
					success: "Il est possible qu'il faille attendre une minute pour que la modification prenne effet.  Veuillez fermer votre navigateur et vous connecter à nouveau."
				},
				service: {
					subtitle: "Maintenance",
					tagline: "Définissez les niveaux de trace pour générer des informations de support pour l'interface utilisateur Web.",
					traceEnabled: "Activer la trace",
					traceSpecificationLabel: "Spécification de trace",
					traceSpecificationTooltip: "Chaîne de spécification de trace fournie par le support IBM",
				    save: "Enregistrer",
				    getTraceError: "Une erreur s'est produite lors de l'obtention de la spécification de trace.",
				    setTraceError: "Une erreur s'est produite lors de la définition de la spécification de trace.",
				    clearTraceError: "Une erreur s'est produite lors de l'annulation de la spécification de trace.",
					saving: "Enregistrement en cours...",
					success: "L'enregistrement a abouti.",
					failed: "L'enregistrement a échoué."
				}
			}
		},
        platformDataRetrieveError: "Une erreur s'est produite lors de l'extraction des données de plateforme.",
        // Strings for client certs that can be used for certificate authentication
		clientCertsTitle: "Certificats client",
		clientCertsButton: "Certificats client",
        stopHover: "Arrêtez le serveur et empêchez tout accès au serveur.",
        restartLink: "Redémarrer le serveur",
		restartMaintOnLink: "Démarrer la maintenance",
		restartMaintOnHover: "Redémarrez le serveur en mode maintenance.",
		restartMaintOffLink: "Arrêter la maintenance",
		restartMaintOffHover: "Redémarrez le serveur en mode production.",
		restartCleanStoreLink: "Nettoyer le magasin",
		restartCleanStoreHover: "Nettoyez le magasin du serveur et redémarrez le serveur.",
        restartDialogTitle: "Redémarrez le serveur ${IMA_PRODUCTNAME_SHORT}.",
        restartMaintOnDialogTitle: "Redémarrez le serveur ${IMA_PRODUCTNAME_SHORT} en mode maintenance.",
        restartMaintOffDialogTitle: "Redémarrez le serveur ${IMA_PRODUCTNAME_SHORT} en mode production.",
        restartCleanStoreDialogTitle: "Nettoyez le magasin du serveur ${IMA_PRODUCTNAME_SHORT}",
        restartDialogContent: "Voulez-vous vraiment redémarrer le serveur ${IMA_PRODUCTNAME_SHORT} ?  Le redémarrage du serveur interrompt la messagerie.",
        restartMaintOnDialogContent: "Voulez-vous vraiment redémarrer le serveur ${IMA_PRODUCTNAME_SHORT} en mode maintenance ?  Le démarrage du serveur en mode maintenance arrête la messagerie.",
        restartMaintOffDialogContent: "Voulez-vous vraiment redémarrer le serveur ${IMA_PRODUCTNAME_SHORT} en mode production ?  Le démarrage du serveur en mode production redémarre la messagerie.",
        restartCleanStoreDialogContent: "L'exécution de l'action <strong>Nettoyer le magasin</strong> entraîne la perte des données de messagerie conservées.<br/><br/>" +
        	"Voulez-vous vraiment exécuter l'action <strong>Nettoyer le magasin</strong> ?",
        restartOkButton: "Redémarrer",        
        cleanOkButton: "Nettoyer le magasin et redémarrer",
        restarting: "Redémarrage du serveur ${IMA_PRODUCTNAME_SHORT} en cours...",
        restartingFailed: "Une erreur est survenue lors du redémarrage du serveur ${IMA_PRODUCTNAME_SHORT}.",
        remoteLogLevelTagLine: "Définissez le niveau de messages que vous souhaitez voir dans les journaux serveur.  ", 
        errorRetrievingTrustedCertificates: "Une erreur est survenue lors de l'extraction des certificats de confiance.",

        // New page title and tagline:  Admin Endpoint
        adminEndpointPageTitle: "Configuration du noeud final d'administration",
        adminEndpointPageTagline: "Configurez le noeud final d'administration et créez des règles de configuration. " +
        		"L'interface utilisateur Web se connecte au noeud final d'administration pour effectuer des tâches d'administration et de surveillance. " +
        		"Les règles de configuration contrôlent les tâches d'administration et de surveillance qu'un utilisateur peut effectuer.",
        // HA config strings
        haReplicationPortLabel: "Port de réplication :",
		haExternalReplicationInterfaceLabel: "Adresse de réplication externe :",
		haExternalReplicationPortLabel: "Port de réplication externe :",
        haRemoteDiscoveryPortLabel: "Port de reconnaissance distante :",
        haReplicationPortTooltip: "Numéro de port utilisé pour la réplication à haute disponibilité sur le noeud local dans la paire à haute disponibilité.",
		haReplicationPortInvalid: "Le port de réplication n'est pas valide." +
				"La plage valide va de 1024 à 65535. La valeur par défaut est 0 et permet la sélection aléatoire d'un port disponible lors de la connexion.",
		haExternalReplicationInterfaceTooltip: "Adresse IPv4 ou IPv6 de la carte d'interface réseau à utiliser pour la réplication à haute disponibilité.",
		haExternalReplicationPortTooltip: "Numéro de port externe utilisé pour la réplication à haute disponibilité sur le noeud local dans la paire à haute disponibilité.",
		haExternalReplicationPortInvalid: "Le port de réplication externe n'est pas valide." +
				"La plage valide va de 1024 à 65535. La valeur par défaut est 0 et permet la sélection aléatoire d'un port disponible lors de la connexion.",
        haRemoteDiscoveryPortTooltip: "Numéro de port utilisé pour la reconnaissance à haute disponibilité sur le noeud distant dans la paire à haute disponibilité.",
		haRemoteDiscoveryPortInvalid: "Le port de reconnaissance distante n'est pas valide." +
				 "La plage valide va de 1024 à 65535. La valeur par défaut est 0 et permet la sélection aléatoire d'un port disponible lors de la connexion.",
		haReplicationAndDiscoveryBold: "Adresses de réplication et de reconnaissance ",
		haReplicationAndDiscoveryNote: "(si l'une est définie, l'autre doit l'être également)",
		haUseSecuredConnectionsLabel: "Utiliser des connexions sécurisées :",
		haUseSecuredConnectionsTooltip: "Utilisez le protocole TLS pour la reconnaissance et les canaux de données entre les noeuds d'une paire à haute disponibilité.",
		haUseSecuredConnectionsEnabled: "L'utilisation de connexions sécurisées est activée",
		haUseSecuredConnectionsDisabled: "L'utilisation de connexions sécurisées est désactivée",
		mqConnectivityEnableLabel: "Activer MQ Connectivity",
		savingProgress: "Enregistrement en cours...",
	    setMqConnEnabledError: "Une erreur s'est produite lors de la définition de l'état activé de MQ Connectivity.",
	    allowCertfileOverwriteLabel: "Remplacer",
		allowCertfileOverwriteTooltip: "Autorisez le remplacement du certificat et de la clé privée.",
		stopServerWarningTitle: "L'arrêt empêche le redémarrage depuis l'interface utilisateur Web.",
		stopServerWarning: "L'arrêt du serveur empêche toute messagerie et tout accès administrateur via les demandes REST et l'interface utilisateur Web. <b>Pour éviter de perdre l'accès au serveur depuis l'interface utilisateur Web, utilisez le lien Redémarrer le serveur.</b>",
		// New Web UI user dialog strings to allow French translation to insert a space before colon
		webuiuserLabel: "ID utilisateur :",
		webuiuserdescLabel: "Description :",
		webuiuserroleLabel: "Rôle :"

});
