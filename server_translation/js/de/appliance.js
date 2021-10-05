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
				title: "Benutzermanagement für die Webbenutzerschnittstelle",
				tagline: "Erteilen Sie Benutzern Zugriff auf die Webbenutzerschnittstelle.",
				usersSubTitle: "Benutzer der Webbenutzerschnittstelle",
				webUiUsersTagline: "Systemadministratoren können Webbenutzerschnittstellenbenutzer definieren, bearbeiten und löschen.",
				itemName: "Benutzer",
				itemTooltip: "",
				addDialogTitle: "Benutzer hinzufügen",
				editDialogTitle: "Benutzer bearbeiten",
				removeDialogTitle: "Benutzer löschen",
				resetPasswordDialogTitle: "Kennwort zurücksetzen",
				// TRNOTE: Do not translate addGroupDialogTitle, editGroupDialogTitle, removeGroupDialogTitle
				//addGroupDialogTitle: "Add Group",
				//editGroupDialogTitle: "Edit Group",
				//removeGroupDialogTitle: "Delete Group",
				grid: {
					userid: "Benutzer-ID",
					// TRNOTE: Do not translate groupid
					//groupid: "Group ID",
					description: "Beschreibung",
					groups: "Rolle",
					noEntryMessage: "Es sind keine anzuzeigenden Elemente vorhanden.",
					// TRNOTE: Do not translate ldapEnabledMessage
					//ldapEnabledMessage: "Messaging users and groups configured on the appliance are disabled because an external LDAP connection is enabled.",
					refreshingLabel: "Aktualisierung wird durchgeführt..."
				},
				dialog: {
					useridTooltip: "Geben Sie die Benutzer-ID ein, die keine führenden und nachfolgenden Leerzeichen enthalten darf.",
					applianceUserIdTooltip: "Geben Sie die Benutzer-ID ein, die sich ausschließlich aus den Buchstaben A-Z, a-z, den Zahlen 0-9 und den vier Sonderzeichen - _ . und + zusammensetzen darf.",					
					useridInvalid: "Die Benutzer-ID darf keine führenden oder nachfolgenden Leerzeichen enthalten.",
					applianceInvalid: "Die Benutzer-ID darf sich ausschließlich aus den Buchstaben A-Z, a-z, den Zahlen 0-9 und den vier Sonderzeichen - _ . und + zusammensetzen.",					
					// TRNOTE: Do not translate groupidTooltip, groupidInvalid
					//groupidTooltip: "Enter the Group ID, which must not have leading or trailing spaces.",
					//groupidInvalid: "The Group ID must not have leading or trailing spaces.",
					SystemAdministrators: "Systemadministratoren",
					MessagingAdministrators: "Messaging-Administratoren",
					Users: "Benutzer",
					applianceUserInstruction: "Systemadministratoren können andere Webbenutzerschnittstellenadministratoren und Webbenutzerschnittstellenbenutzer erstellen, aktualisieren und löschen.",
					// TRNOTE: Do not translate messagingUserInstruction, messagingGroupInstruction
					//messagingUserInstruction: "Messaging users can be added to messaging policies to control the messaging resources that a user can access.",
					//messagingGroupInstruction: "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.",				                    
					password1: "Kennwort:",
					password2: "Kennwort bestätigen:",
					password2Invalid: "Die Kennwörter stimmen nicht überein.",
					passwordNoSpaces: "Das Kennwort darf keine führenden oder nachfolgenden Leerzeichen enthalten.",
					saveButton: "Speichern",
					cancelButton: "Abbrechen",
					refreshingGroupsLabel: "Gruppen werden abgerufen..."
				},
				returnCodes: {
					CWLNA5056: "Einige Einstellungen konnten nicht gespeichert werden."
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
				title: "Eintrag löschen",
				content: "Möchten Sie diesen Datensatz wirklich löschen?",
				deleteButton: "Löschen",
				cancelButton: "Abbrechen"
			},
			savingProgress: "Speicheroperation wird durchgeführt...",
			savingFailed: "Die Speicheroperation ist fehlgeschlagen.",
			deletingProgress: "Löschoperation wird durchgeführt...",
			deletingFailed: "Die Löschoperation ist fehlgeschlagen.",
			testingProgress: "Tests werden durchgeführt...",
			uploadingProgress: "Upload wird durchgeführt...",
			dialog: { 
				saveButton: "Speichern",
				cancelButton: "Abbrechen",
				testButton: "Verbindung testen"
			},
			invalidName: "Es ist bereits ein Datensatz mit diesem Namen vorhanden.",
			invalidChars: "Es muss ein gültiger Hostname oder eine gültige IP-Adresse eingegeben werden.",
			invalidRequired: "Es muss ein Wert eingegeben werden.",
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
				nothingToTest: "Sie müssen eine IPv4- oder IPv6-Adresse angeben, um die Verbindung zu testen.",
				testConnFailed: "Verbindungstest fehlgeschlagen",
				testConnSuccess: "Verbindungstest erfolgreich",
				testFailed: "Test fehlgeschlagen",
				testSuccess: "Test erfolgreich"
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
				title: "Sicherheitseinstellungen",
				tagline: "Importieren Sie Schlüssel und Zertifikate, um die Verbindungen zum Server zu sichern.",
				securityProfilesSubTitle: "Sicherheitsprofile",
				securityProfilesTagline: "Systemadministratoren können Sicherheitsprofile, die die zum Überprüfen von Clients verwendeten Methoden identifizieren, definieren, bearbeiten und löschen.",
				certificateProfilesSubTitle: "Zertifikatsprofile",
				certificateProfilesTagline: "Systemadministratoren können Zertifikatsprofile, die Benutzeridentitäten prüfen, definieren, bearbeiten und löschen.",
				ltpaProfilesSubTitle: "LTPA-Profile",
				ltpaProfilesTagline: "Systemadministratoren können LTPA-Profile (Lightweight Third Party Authentication) für die Aktivierung von Single Sign-on (SSO) auf mehreren Servern definieren, bearbeiten und löschen.",
				oAuthProfilesSubTitle: "OAuth-Profile",
				oAuthProfilesTagline: "Systemadministratoren können OAuth-Profile für die Aktivierung von Single Sign-on (SSO) auf mehreren Servern definieren, bearbeiten und löschen.",
				systemWideSubTitle: "Serverweite Sicherheitseinstellungen",
				useFips: "FIPS 140-2-Profil für die Sicherung der Messaging-Kommunikation verwenden",
				useFipsTooltip: "Wählen Sie diese Option aus, um eine Verschlüsselung, die mit Federal Information Processing Standards (FIPS) 140-2 konform ist, für Endpunkte zu verwenden, die mit einem Sicherheitsprofil geschützt sind." +
				                "Eine Änderung dieser Einstellung erfordert einen Neustart des ${IMA_PRODUCTNAME_SHORT}-Servers.",
				retrieveFipsError: "Beim Abrufen der FIPS-Einstellung ist ein Fehler aufgetreten.",
				fipsDialog: {
					title: "FIPS festlegen",
					content: "Der ${IMA_PRODUCTNAME_SHORT}-Server muss erneut gestartet werden, damit diese Änderung wirksam wird. Möchten Sie diese Einstellung wirklich ändern?"	,
					okButton: "Festlegen und erneut starten",
					cancelButton: "Änderung abbrechen",
					failed: "Beim Ändern der FIPS-Einstellung ist ein Fehler aufgetreten.",
					stopping: "Der ${IMA_PRODUCTNAME_SHORT}-Server wird gestoppt...",
					stoppingFailed: "Beim Stoppen des ${IMA_PRODUCTNAME_SHORT}-Server ist ein Fehler aufgetreten.",
					starting: "Der ${IMA_PRODUCTNAME_SHORT}-Server wird gestartet...",
					startingFailed: "Beim Starten des ${IMA_PRODUCTNAME_SHORT}-Server ist ein Fehler aufgetreten."
				}
			},
			securityProfiles: {
				grid: {
					name: "Name",
					mprotocol: "Minimale Protokollmethode",
					ciphers: "Verschlüsselungen",
					certauth: "Clientzertifikate verwenden",
					clientciphers: "Verschlüsselungen des Clients verwenden",
					passwordAuth: "Kennwortauthentifizierung verwenden",
					certprofile: "Zertifikatsprofil",
					ltpaprofile: "LTPA-Profil",
					oAuthProfile: "OAuth-Profil",
					clientCertsButton: "Anerkannte Zertifikate"
				},
				trustedCertMissing: "Anerkanntes Zertifikat fehlt",
				retrieveError: "Beim Abrufen der Sicherheitsprofile ist ein Fehler aufgetreten.",
				addDialogTitle: "Sicherheitsprofil hinzufügen",
				editDialogTitle: "Sicherheitsprofil bearbeiten",
				removeDialogTitle: "Sicherheitsprofil löschen",
				dialog: {
					nameTooltip: "Ein Name, der sich aus maximal 32 alphanumerischen Zeichen zusammensetzt. Das erste Zeichen darf keine Zahl sein.",
					mprotocolTooltip: "Die niedrigste Protokollebene, die beim Herstellen der Verbindung zulässig ist.",
					ciphersTooltip: "<strong>Optimal:&nbsp;</strong> Wählen Sie sicherste Verschlüsselung aus, die vom Server und vom Client unterstützt wird.<br />" +
							        "<strong>Schnell:&nbsp;</strong> Wählen Sie die schnellste Verschlüsselung mit hoher Sicherheit aus, die vom Server und vom Client unterstützt wird.<br />" +
							        "<strong>Mittel:&nbsp;</strong> Wählen Sie die schnellste Verschlüsselung mit mittlerer oder hoher Sicherheit aus, die vom Client unterstützt wird.",
				    ciphers: {
				    	best: "Optimal",
				    	fast: "Schnell",
				    	medium: "Mittel"
				    },
					certauthTooltip: "Legen Sie fest, ob das Clientzertifikat authentifiziert werden soll.",
					clientciphersTooltip: "Legen Sie fest, ob die Bestimmung der bei der Verbindungsherstellung zu verwendenden Cipher-Suite durch den Client zugelassen werden soll.",
					passwordAuthTooltip: "Legen Sie fest, ob eine gültige Benutzer-ID und ein gültiges Kennwort für die Verbindungsherstellung erforderlich sind.",
					passwordAuthTooltipDisabled: "Die Einstellung kann nicht geändert werden, weil ein LTPA- oder ein OAuth-Profil ausgewählt ist. Wenn Sie die Einstellung ändern möchten, ändern Sie das LTPA- bzw. OAuth-Profil in <em>Ein Profil auswählen</em>. ",
					certprofileTooltip: "Wählen Sie ein vorhandenes Zertifikatsprofil aus, das Sie diesem Sicherheitsprofil zuordnen möchten.",
					ltpaprofileTooltip: "Wählen Sie ein vorhandenes LTPA-Profil aus, das Sie diesem Sicherheitsprofil zuordnen möchten." +
					                    "Wenn Sie ein LTPA-Profil auswählen, müssen Sie auch die Option Kennwortauthentifizierung verwenden auswählen.",
					ltpaprofileTooltipDisabled: "Es kann kein LTPA-Profil ausgewählt werden, weil ein OAuth-Profil ausgewählt ist oder weil die Option Kennwortauthentifizierung verwenden nicht ausgewählt ist.",
					oAuthProfileTooltip: "Wählen Sie ein vorhandenes OAuth-Profil aus, das Sie diesem Sicherheitsprofil zuordnen möchten." +
                    					  "Wenn Sie ein OAuth-Profil auswählen, muss auch die Option Kennwortauthentifizierung verwenden ausgewählt werden.",
                    oAuthProfileTooltipDisabled: "Es kann kein OAuth-Profil ausgewählt werden, weil ein LTPA-Profil ausgewählt ist oder weil die Option Kennwortauthentifizierung verwenden ausgewählt ist.",
					noCertProfilesTitle: "Keine Zertifikatsprofile",
					noCertProfilesDetails: "Wenn <em>TLS verwenden</em> ausgewählt ist, kann ein Sicherheitsprofil erst nach der Definition eines Zertifikatsprofils definiert werden.",
					chooseOne: "Ein Profil auswählen"
				},
				certsDialog: {
					title: "Anerkannte Zertifikate",
					instruction: "Zertifikate für ein Sicherheitsprofil in den Truststore hochladen oder aus dem Truststore löschen",
					securityProfile: "Sicherheitsprofil",
					certificate: "Zertifikat",
					browse: "Durchsuchen",
					browseHint: "Datei auswählen...",
					upload: "Hochladen",
					close: "Schließen",
					remove: "Löschen",
					removeTitle: "Zertifikat löschen",
					removeContent: "Möchten Sie dieses Zertifikat wirklich löschen?",
					retrieveError: "Beim Abrufen der Zertifikate ist ein Fehler aufgetreten.",
					uploadError: "Beim Hochladen der Zertifikatsdatei ist ein Fehler aufgetreten.",
					deleteError: "Beim Löschen der Zertifikatsdatei ist ein Fehler aufgetreten."	
				}
			},
			certificateProfiles: {
				grid: {
					name: "Name",
					certificate: "Zertifikat",
					privkey: "Privater Schlüssel",
					certexpiry: "Verfallszeit des Zertifikats",
					noDate: "Fehler beim Abrufen des Datums..."
				},
				retrieveError: "Beim Abrufen der Zertifikatsprofile ist ein Fehler aufgetreten.",
				uploadCertError: "Beim Hochladen der Zertifikatsdatei ist ein Fehler aufgetreten.",
				uploadKeyError: "Beim Hochladen der Schlüsseldatei ist ein Fehler aufgetreten.",
				uploadTimeoutError: "Beim Hochladen der Zertifikats- bzw. Schlüsseldatei ist ein Fehler aufgetreten.",
				addDialogTitle: "Zertifikatsprofil hinzufügen",
				editDialogTitle: "Zertifikatsprofil bearbeiten",
				removeDialogTitle: "Zertifikatsprofil löschen",
				dialog: {
					certpassword: "Zertifikatskennwort:",
					keypassword: "Schlüsselkennwort:",
					browse: "Durchsuchen...",
					certificate: "Zertifikat: ",
					certificateTooltip: "", // "Upload a certificate"
					privkey: "Privater Schlüssel:",
					privkeyTooltip: "", // "Upload a private key"
					browseHint: "Datei auswählen...",
					duplicateFile: "Es ist bereits eine Datei mit diesem Namen vorhanden.",
					identicalFile: "Zertifikat und Schlüssel dürfen nicht dieselbe Datei sein.",
					noFilesToUpload: "Zum Aktualisieren des Zertifikatsprofils müssen Sie mindestens eine hochzuladende Datei auswählen."
				}
			},
			ltpaProfiles: {
				addDialogTitle: "LTPA-Profil hinzufügen",
				editDialogTitle: "LTPA-Profil bearbeiten",
				instruction: "Verwenden Sie ein LTPA-Profil (Lightweight Third-Party Authentication) in einem Sicherheitsprofil, um Single Sign-On (SSO) auf mehreren Servern zu aktivieren.",
				removeDialogTitle: "LTPA-Profil löschen",				
				keyFilename: "Name der Schlüsseldatei",
				keyFilenameTooltip: "Die Datei, die den exportierten LTPA-Schlüssel enthält.",
				browse: "Durchsuchen...",
				browseHint: "Datei auswählen...",
				password: "Kennwort",
				saveButton: "Speichern",
				cancelButton: "Abbrechen",
				retrieveError: "Beim Abrufen der LTPA-Profile ist ein Fehler aufgetreten.",
				duplicateFileError: "Es ist bereits eine Schlüsseldatei mit diesem Namen vorhanden.",
				uploadError: "Beim Hochladen der Schlüsseldatei ist ein Fehler aufgetreten.",
				noFilesToUpload: "Zum Aktualisieren des LTPA-Profils wählen Sie eine hochzuladende Datei aus."
			},
			oAuthProfiles: {
				addDialogTitle: "OAuth-Profil hinzufügen",
				editDialogTitle: "OAuth-Profil bearbeiten",
				instruction: "Verwenden Sie ein OAuth-Profil in einem Sicherheitsprofil, um Single Sign-On (SSO) auf mehreren Servern zu aktivieren.",
				removeDialogTitle: "OAuth-Profil löschen",	
				resourceUrl: "Ressourcen-URL",
				resourceUrlTooltip: "Die für die Validierung des Zugriffstokens zu verwendende Berechtigungsserver-URL. Die URL muss das Protokoll enthalten. Die gültigen Protokolle sind http und https.",
				keyFilename: "OAuth-Serverzertifikat",
				keyFilenameTooltip: "Der Name der Datei, die das SSL-Zertifikat enthält, das für die Herstellung der Verbindung zum OAuth-Server verwendet wird.",
				browse: "Durchsuchen...",
				reset: "Zurücksetzen",
				browseHint: "Datei auswählen...",
				authKey: "Berechtigungsschlüssel",
				authKeyTooltip: "Der Name des Schlüssels, der das Zugriffstoken enthält. Der Standardname ist <em>access_token</em>.",
				userInfoUrl: "URL für Benutzerinformationen",
				userInfoUrlTooltip: "Die für das Abrufen der Benutzerinformationen zu verwendende Berechtigungsserver-URL. Die URL muss das Protokoll enthalten. Die gültigen Protokolle sind http und https.",
				userInfoKey: "Schlüssel für Benutzerinformationen",
				userInfoKeyTooltip: "Der Name des Schlüssels, der für das Abrufen der Benutzerinformationen verwendet wird. ",
				saveButton: "Speichern",
				cancelButton: "Abbrechen",
				retrieveError: "Beim Abrufen der OAuth-Profile ist ein Fehler aufgetreten. ",
				duplicateFileError: "Es ist bereits eine Schlüsseldatei mit diesem Namen vorhanden.",
				uploadError: "Beim Hochladen der Schlüsseldatei ist ein Fehler aufgetreten.",
				urlThemeError: "Geben Sie eine gültige URL mit dem Protokoll http oder https ein.",
				inUseBySecurityPolicyMessage: "Das OAuth-Profil kann nicht gelöscht werden, weil es bereits vom folgenden Sicherheitsprofil verwendet wird: {0}.",			
				inUseBySecurityPoliciesMessage: "Das OAuth-Profil kann nicht gelöscht werden, weil es bereits von den folgenden Sicherheitsprofilen verwendet wird: {0}."			
			},			
			systemControl: {
				pageTitle: "Serversteuerung",
				appliance: {
					subTitle: "Server",
					tagLine: "Einstellungen und Aktionen, die sich auf den ${IMA_PRODUCTNAME_FULL}-Server auswikren.",
					hostname: "Servername",
					hostnameNotSet: "(ohne)",
					retrieveHostnameError: "Beim Abrufen des Servernamens ist ein Fehler aufgetreten.",
					hostnameError: "Beim Speichern des Servernamens ist ein Fehler aufgetreten.",
					editLink: "Bearbeiten",
	                viewLink: "Anzeigen",
					firmware: "Firmwareversion",
					retrieveFirmwareError: "Beim Abrufen der Firmwareversion ist ein Fehler aufgetreten.",
					uptime: "Betriebszeit",
					retrieveUptimeError: "Beim Abrufen der Betriebszeit ist ein Fehler aufgetreten.",
					// TRNOTE Do not translate locateLED, locateLEDTooltip
					//locateLED: "Locate LED",
					//locateLEDTooltip: "Locate your appliance by turning on the blue LED.",
					locateOnLink: "Einschalten",
					locateOffLink: "Ausschalten",
					locateLedOnError: "Beim Einschalten der Positions-LED ist ein Fehler aufgetreten. ",
					locateLedOffError: "Beim Ausschalten der Positions-LED ist ein Fehler aufgetreten.",
					locateLedOnSuccess: "Die Positions-LED wurde eingeschaltet.",
					locateLedOffSuccess: "Die Positions-LED wurde ausgeschaltet.",
					restartLink: "Server erneut starten",
					restartError: "Beim Neustart des Servers ist ein Fehler aufgetreten.",
					restartMessage: "Die Anforderung zum erneuten Starten des Servers wurde übergeben.",
					restartMessageDetails: "Der Neustart des Servers kann einige Minuten dauern. Die Benutzerschnittstelle ist während des Neustarts des Servers nicht verfügbar.",
					shutdownLink: "Server herunterfahren",
					shutdownError: "Beim Herunterfahren des Servers ist ein Fehler aufgetreten.",
					shutdownMessage: "Die Anforderung zum Herunterfahren des Servers wurde übergeben.",
					shutdownMessageDetails: "Das Herunterfahren des Servers kann einige Minuten dauern. Die Benutzerschnittstelle ist während des Herunterfahrens des Servers nicht verfügbar." +
					                        "Um den Server wieder einzuschalten, drücken Sie den Netzschalter am Server.",
// TRNOTE Do not translate sshHost, sshHostSettings, sshHostStatus, sshHostTooltip, sshHostStatusTooltip,
// retrieveSSHHostError, sshSaveError
//					sshHost: "SSH Host",
//				    sshHostSettings: "Settings",
//				    sshHostStatus: "Status",
//				    sshHostTooltip: "Set the host IP address or addresses that can access the IBM IoT MessageSight SSH console.",
//				    sshHostStatusTooltip: "The host IP addresses that the SSH service is currently using.",
//				    retrieveSSHHostError: "An error occurred while retrieving the SSH host setting and status.",
//				    sshSaveError: "An error occurred while configuring IBM IoT MessageSight to allow SSH connections on the address or addresses provided.",

				    licensedUsageLabel: "Lizenznutzung",
				    licensedUsageRetrieveError: "Beim Abrufen der Einstellung für die Lizenznutzung ist ein Fehler aufgetreten.",
                    licensedUsageSaveError: "Beim Speichern der Einstellung für die Lizenznutzung ist ein Fehler aufgetreten.",
				    licensedUsageValues: {
				        developers: "Entwickler",
				        nonProduction: "Nicht-Produktion",
				        production: "Produktion"
				    },
				    licensedUsageDialog: {
				        title: "Lizenznutzung ändern",
				        instruction: "Ändern Sie die Lizenznutzung des Servers. " +
				        		"Wenn Sie einen Berechtigungsnachweis haben, muss die ausgewählte Lizenznutzung mit der autorisierten Nutzung für das Programm gemäß Festlegung im Berechtigungsnachweis übereinstimmen, Produktion oder Nicht-Produktion." +
				        		"Wenn der Berechtigungsnachweis das Programm als Programm kennzeichnet, das nicht für den Einsatz in einer Produktionsumgebung bestimmt ist, muss Nicht-Produktion ausgewählt werden. " +
				        		"Wenn es keine Festlegung im Berechtigungsnachweis gibt, wird von einem Einsatz in einer Produktionsumgebung ausgegangen, und deshalb muss Produktion ausgewählt werden." +
				        		"Wenn Sie keinen Berechtigungsnachweis haben, muss Entwickler ausgewählt werden. ",
				        content: "Wenn Sie die Lizenznutzung des Servers ändern, wird der Server erneut gestartet. " +
				        		"Sie werden von der Benutzerschnittstelle abgemeldet, und die Benutzerschnittstelle ist während des Neustarts des Servers nicht verfügbar." +
				        		"Der Neustart des Servers kann einige Minuten dauern. " +
				        		"Nach dem Neustart des Servers müssen Sie sich an der Webbenutzerschnittstelle anmelden und die neue Lizenz akzeptieren.",
				        saveButton: "Speichern und Server erneut starten",
				        cancelButton: "Abbrechen"
				    },
					restartDialog: {
						title: "Server erneut starten",
						content: "Möchten Sie den Server wirklich erneut starten? " +
								"Sie werden von der Benutzerschnittstelle abgemeldet, und die Benutzerschnittstelle ist während des Neustarts des Servers nicht verfügbar." +
								"Der Neustart des Servers kann einige Minuten dauern. ",
						okButton: "Erneut starten",
						cancelButton: "Abbrechen"												
					},
					shutdownDialog: {
						title: "Server herunterfahren",
						content: "Möchten Sie den Server wirklich herunterfahren? " +
								 "Sie werden von der Benutzerschnittstelle abgemeldet, und die Benutzerschnittstelle ist erst wieder verfügbar, nachdem der Server wieder eingeschaltet wurde." +
								 "Um den Server wieder einzuschalten, drücken Sie den Netzschalter am Server.",
						okButton: "Herunterfahren",
						cancelButton: "Abbrechen"												
					},

					hostnameDialog: {
						title: "Servernamen bearbeiten",
						instruction: "Geben Sie einen Servernamen an, der aus maximal 1024 Zeichen besteht.",
						invalid: "Der eingegebene Wert ist nicht gültig. Der Servername darf nur gültige UTF-8-Zeichen enthalten.",
						tooltip: "Geben Sie den Servernamen ein. Der Servername darf nur gültige UTF-8-Zeichen enthalten.",
						hostname: "Servername",
						okButton: "Speichern",
						cancelButton: "Abbrechen"												
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
					subTitle: "${IMA_PRODUCTNAME_FULL}-Server",
					tagLine: "Einstellungen und Aktionen, die sich auf den ${IMA_PRODUCTNAME_SHORT}-Server auswirken.",
					status: "Status",
					retrieveStatusError: "Beim Abrufen des Status ist ein Fehler aufgetreten.",
					stopLink: "Server stoppen",
					forceStopLink: "Stoppen des Servers erzwingen",
					startLink: "Server starten",
					starting: "Startoperation wird ausgeführt",
					unknown: "Unbekannt",
					cleanStore: "Wartungsmodus (Speicherbereinigung in Bearbeitung)",
					startError: "Beim Starten des ${IMA_PRODUCTNAME_SHORT}-Server ist ein Fehler aufgetreten.",
					stopping: "Stoppoperation wird ausgeführt",
					stopError: "Beim Stoppen des ${IMA_PRODUCTNAME_SHORT}-Server ist ein Fehler aufgetreten.",
					stopDialog: {
						title: "${IMA_PRODUCTNAME_FULL}-Server stoppen",
						content: "Möchten Sie den ${IMA_PRODUCTNAME_SHORT}-Server wirklich stoppen? ",
						okButton: "Herunterfahren",
						cancelButton: "Abbrechen"						
					},
					forceStopDialog: {
						title: "Stoppen des ${IMA_PRODUCTNAME_FULL}-Servers erzwingen",
						content: "Möchten Sie das Stoppen des ${IMA_PRODUCTNAME_SHORT}-Server wirklich erzwingen? Das Erzwingen eines Stopps ist die letzte zu ergreifende Maßnahme, weil diese zu einem Verlust von Nachrichten führen kann.",
						okButton: "Stoppen erzwingen",
						cancelButton: "Abbrechen"						
					},

					memory: "Hauptspeicher"
				},
				mqconnectivity: {
					subTitle: "MQ Connectivity-Service",
					tagLine: "Einstellungen und Aktionen, die sich auf den MQ Connectivity-Service auswirken.",
					status: "Status",
					retrieveStatusError: "Beim Abrufen des Status ist ein Fehler aufgetreten.",
					stopLink: "Service stoppen",
					startLink: "Service starten",
					starting: "Startoperation wird ausgeführt",
					unknown: "Unbekannt",
					startError: "Beim Starten des MQ Connectivity-Service ist ein Fehler aufgetreten.",
					stopping: "Stoppoperation wird ausgeführt",
					stopError: "Beim Stoppen des MQ Connectivity-Service ist ein Fehler aufgetreten.",
					started: "Aktiv",
					stopped: "Gestoppt",
					stopDialog: {
						title: "MQ Connectivity-Service stoppen",
						content: "Möchten Sie den MQ Connectivity-Service wirklich stoppen? Das Stoppen des Service kann zu einem Verlust von Nachrichten führen.",
						okButton: "Stoppen",
						cancelButton: "Abbrechen"						
					}
				},
				runMode: {
					runModeLabel: "Ausführungsmodus",
					cleanStoreLabel: "Speicher bereinigen",
				    runModeTooltip: "Geben Sie den Ausführungsmodus für den ${IMA_PRODUCTNAME_SHORT}-Server an. Nach der Auswahl und Bestätigung eines neuen Ausführungsmodus wird dieser erst wirksam, wenn der ${IMA_PRODUCTNAME_SHORT}-Server erneut gestartet wird.",
				    runModeTooltipDisabled: "Der Ausführungsmodus kann wegen des Status des ${IMA_PRODUCTNAME_SHORT}-Servers nicht geändert werden.",
				    cleanStoreTooltip: "Geben Sie an, ob die Aktion zur Speicherbereinigung beim Neustart des ${IMA_PRODUCTNAME_SHORT}-Servers ausgeführt werden soll. Die Aktion Speicher bereinigen kann nur ausgewählt werden, wenn sich der ${IMA_PRODUCTNAME_SHORT}-Server im Wartungsmodus befindet.",
					// TRNOTE {0} is a mode, such as production or maintenance.
				    setRunModeError: "Beim Versuch, den Ausführungsmodus des ${IMA_PRODUCTNAME_SHORT}-Server in {0} zu ändern, ist ein Fehler aufgetreten.",
					retrieveRunModeError: "Beim Abrufen des Ausführungsmodus des ${IMA_PRODUCTNAME_SHORT}-Servers ist ein Fehler aufgetreten.",
					runModeDialog: {
						title: "Änderung des Ausführungsmodus bestätigen",
						// TRNOTE {0} is a mode, such as production or maintenance.
						content: "Möchten Sie den Ausführungsmodus des ${IMA_PRODUCTNAME_SHORT}-Servers wirklich in <strong>{0}</strong> ändern? Wenn Sie einen neuen Ausführungsmodus auswählen, muss der ${IMA_PRODUCTNAME_SHORT}-Servers erneut gestartet werden, damit der neue Ausführungsmodus wirksam wird.",
						okButton: "Bestätigen",
						cancelButton: "Abbrechen"												
					},
					cleanStoreDialog: {
						title: "Änderung der Aktion Speicher bereinigen bestätigen",
						contentChecked: "Möchten Sie wirklich, dass die Aktion <strong>Speicher bereinigen</strong> während des Neustarts des Servers ausgeführt wird? " +
								 "Wenn die Aktion <strong>Speicher bereinigen</strong> definiert wird, muss der ${IMA_PRODUCTNAME_SHORT}-Server erneut gestartet werden, damit die Aktion ausgeführt wird. " +
						         "Die Ausführung der Aktion <strong>Speicher bereinigen</strong> führt zu einem Verlust der persistent gespeicherten Messaging-Daten.<br/><br/>",
						content: "Möchten Sie die Aktion <strong>Speicher bereinigen</strong> wirklich abbrechen?",
						okButton: "Bestätigen",
						cancelButton: "Abbrechen"												
					}
				},
				storeMode: {
					storeModeLabel: "Speichermodus",
				    storeModeTooltip: "Geben Sie den Speichermodus für den ${IMA_PRODUCTNAME_SHORT}-Server an. Sobald ein neuer Speichermodus ausgewählt und bestätigt wurde, wird der ${IMA_PRODUCTNAME_SHORT}-Server erneut gestartet und eine Speicherbereinigung durchgeführt.",
				    storeModeTooltipDisabled: "Der Speichermodus kann wegen des Status des ${IMA_PRODUCTNAME_SHORT}-Servers nicht geändert werden.",
				    // TRNOTE {0} is a mode, such as production or maintenance.
				    setStoreModeError: "Beim Versuch, den Speichermodus des ${IMA_PRODUCTNAME_SHORT}-Servers in {0} zu ändern, ist ein Fehler aufgetreten.",
					retrieveStoreModeError: "Beim Abrufen des Speichermodus des ${IMA_PRODUCTNAME_SHORT}-Servers ist ein Fehler aufgetreten.",
					storeModeDialog: {
						title: "Änderung des Speichermodus bestätigen",
						// TRNOTE {0} is a mode, such as persistent or memory only.
						content: "Möchten Sie den Speichermodus des ${IMA_PRODUCTNAME_SHORT}-Servers wirklich in <strong>{0}</strong> ändern? " + 
								"Nach einer Änderung des Speichermodus wird der ${IMA_PRODUCTNAME_SHORT}-Server erneut gestartet und eine Speicherbereinigungsaktion ausgeführt." +
								"Die Ausführung der Aktion Speicher bereinigen führt zu einem Verlust der persistent gespeicherten Messaging-Daten. Wenn der Speichermodus geändert wird, muss eine Speicherbereinigung durchgeführt werden.",
						okButton: "Bestätigen",
						cancelButton: "Abbrechen"												
					}
				},
				logLevel: {
					subTitle: "Protokollebene festlegen",
					tagLine: "Legen Sie die Nachrichtenebene fest, die Sie in den Serverprotokollen anzeigen möchten.  " +
							 "Die Protokolle können über <a href='monitoring.jsp?nav=downloadLogs'>Überwachung > Protokolle herunterladen</a> heruntergeladen werden.",
					logLevel: "Standardprotokoll",
					connectionLog: "Verbindungsprotokoll",
					securityLog: "Sicherheitsprotokoll",
					adminLog: "Verwaltungsprotokoll",
					mqconnectivityLog: "MQ Connectivity-Protokoll",
					tooltip: "Die Protokollebene steuert die Typen von Nachrichten in der Standardprotokolldatei. " +
							 "In dieser Protokolldatei werden alle Protokolleinträge zum Betrieb des Servers angezeigt. " +
							 "Einträge mit hoher Wertigkeit, die in anderen Protokollen angezeigt werden, werden ebenfalls in diesem Protokoll aufgezeichnet. " +
							 "Bei der Einstellung Minimum werden nur die wichtigsten Einträge in das Protokoll aufgenommen. " +
							 "Bei der Einstellung Maximum werden alle Einträge in das Protokoll aufgenommen. " +
							 "Weitere Informationen zu den Protokollebenen finden Sie in der Dokumentation. ",
					connectionTooltip: "Die Protokollebene steuert die Typen von Nachrichten in der Verbindungsprotokolldatei. " +
							           "In dieser Protokolldatei werden Einträge zu Verbindungen angezeigt. " +
							           "Diese Einträge können als Prüfprotokoll für Verbindungen verwendet werden und zeigen auch Fehler bei Verbindungen an. " +
							           "Bei der Einstellung Minimum werden nur die wichtigsten Einträge in das Protokoll aufgenommen. Bei der Einstellung Maximum werden alle Einträge in das Protokoll aufgenommen. " +
					                   "Weitere Informationen zu den Protokollebenen finden Sie in der Dokumentation. ",
					securityTooltip: "Die Protokollebene steuert die Typen von Nachrichten in der Sicherheitsprotokolldatei. " +
							         "In dieser Protokolldatei werden Einträge zur Authentifizierung und zur Berechtigung angezeigt. " +
							         "Dieses Protokoll kann als Prüfprotokoll für sicherheitsrelevante Elemente verwendet werden. " +
							         "Bei der Einstellung Minimum werden nur die wichtigsten Einträge in das Protokoll aufgenommen. Bei der Einstellung Maximum werden alle Einträge in das Protokoll aufgenommen. " +
					                 "Weitere Informationen zu den Protokollebenen finden Sie in der Dokumentation. ",
					adminTooltip: "Die Protokollebene steuert die Typen von Nachrichten in der Verwaltungsprotokolldatei. " +
							      "In dieser Protokolldatei werden Einträge zur Ausführung von Verwaltungsbefehlen über die Webbenutzerschnittstelle oder die Befehlszeile von ${IMA_PRODUCTNAME_FULL} angezeigt." +
							      "In der Datei werden alle Änderungen aufgezeichnet, die an der Konfiguration des Geräts vorgenommen bzw. versucht wurden." +
								  "Bei der Einstellung Minimum werden nur die wichtigsten Einträge in das Protokoll aufgenommen. " +
								  "Bei der Einstellung Maximum werden alle Einträge in das Protokoll aufgenommen. " +
					              "Weitere Informationen zu den Protokollebenen finden Sie in der Dokumentation. ",
					mqconnectivityTooltip: "Die Protokollebene steuert die Typen von Nachrichten in der MQ Connectivity-Protokolldatei. " +
							               "In dieser Protokolldatei werden Einträge zur MQ Connectivity-Funktion angezeigt. " +
							               "Zu diesen Einträgen gehören Probleme, die während der Herstellung der Verbindungen zu Warteschlangenmanagern gefunden werden. " +
							               "Das Protokoll kann Ihnen bei der Diagnose von Problemen helfen, die beim Verbinden von ${IMA_PRODUCTNAME_FULL} mit WebSphere<sup>&reg;</sup> MQ auftreten. " +
										   "Bei der Einstellung Minimum werden nur die wichtigsten Einträge in das Protokoll aufgenommen. " +
										   "Bei der Einstellung Maximum werden alle Einträge in das Protokoll aufgenommen. " +
										   "Weitere Informationen zu den Protokollebenen finden Sie in der Dokumentation. ",										
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log										   
					retrieveLogLevelError: "Beim Abrufen der Ebene {0} ist ein Fehler aufgetreten. ",
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log
					setLogLevelError: "Beim Festlegen der Ebene {0} ist ein Fehler aufgetreten. ",
					levels: {
						MIN: "Minimum",
						NORMAL: "Normal",
						MAX: "Maximum"
					},
					serverNotAvailable: "Die Protokollierungsebene kann nicht definiert werden, wenn der ${IMA_PRODUCTNAME_FULL}-Server gestoppt ist.",
					serverCleanStore: "Die Protokollierungsebene kann nicht definiert werden, während die Speicherbereinigung in Bearbeitung ist.",
					serverSyncState: "Die Protokollierungsebene kann nicht definiert werden, während der primäre Knoten synchronisiert wird. ",
					mqNotAvailable: "Die MQ Connectivity-Protokollebene kann nicht definiert werden, wenn der MQ Connectivity-Service gestoppt ist.",
					mqStandbyState: "Die MQ Connectivity-Protokollebene kann nicht definiert werden, wenn sich der Server im Standby-Modus befindet."
				}
			},
			highAvailability: {
				title: "Hochverfügbarkeit",
				tagline: "Konfigurieren Sie zwei ${IMA_PRODUCTNAME_SHORT}-Server als Hochverfügbarkeitsknotenpaar.",
				stateLabel: "Aktueller Hochverfügbarkeitsstatus:",
				statePrimary: "Primär",
				stateStandby: "Standby",
				stateIndeterminate: "Unbestimmt",
				error: "In Bezug auf die Hochverfügbarkeit ist ein Fehler aufgetreten.",
				save: "Speichern",
				saving: "Speicheroperation wird durchgeführt...",
				success: "Die Speicheroperation war erfolgreich. Der ${IMA_PRODUCTNAME_SHORT}-Server und der MQ Connectivity-Service müssen erneut gestartet werden, damit die Änderungen wirksam werden.",
				saveFailed: "Beim Aktualisieren der Hochverfügbarkeitseinstellungen ist ein Fehler aufgetreten.",
				ipAddrValidation : "Geben Sie eine IPv4-Adresse oder eine IPv6-Adresse an, z. B. 192.168.1.0.",
				discoverySameAsReplicationError: "Die Erkennungsadresse darf nicht mit der Replikationsadresse übereinstimmen.",
				AutoDetect: "Automatische Erkennung",
				StandAlone: "Eigenständig",
				notSet: "Nicht definiert",
				yes: "Ja",
				no: "Nein",
				config: {
					title: "Konfiguration",
					tagline: "Die Hochverfügbarkeitskonfiguration wird angezeigt. " +
					         "Änderungen an der Konfiguration werden erst nach einem Neustart des ${IMA_PRODUCTNAME_SHORT}-Server wirksam.",
					editLink: "Bearbeiten",
					enabledLabel: "Hochverfügbarkeit aktiviert:",
					haGroupLabel: "Hochverfügbarkeitsgruppe:",
					startupModeLabel: "Startmodus:",
					replicationInterfaceLabel: "Replikationsadresse:",
					discoveryInterfaceLabel: "Erkennungsadresse:",
					remoteDiscoveryInterfaceLabel: "Ferne Erkennungsadresse:",
					discoveryTimeoutLabel: "Erkennungszeitlimit:",
					heartbeatLabel: "Heartbeatzeitlimit:",
					seconds: "{0} Sekunden",
					secondsDefault: "{0} Sekunden (Standard)",
					preferedPrimary: "Automatische Erkennung (primär bevorzugt)"
				},
				restartInfoDialog: {
					title: "Neustart erforderlich",
					content: "Ihre Änderungen wurden zwar gespeichert, werden aber erst nach einem Neustart des ${IMA_PRODUCTNAME_SHORT}-Servers wirksam.<br /><br />" +
							 "Wenn Sie den ${IMA_PRODUCTNAME_SHORT}-Server jetzt erneut starten möchten, klicken Sie auf <b>Sofort erneut starten</b>. Andernfalls klicken Sie auf <b>Später erneut starten</b>.",
					closeButton: "Später erneut starten",
					restartButton: "Sofort erneut starten",
					stopping: "Der ${IMA_PRODUCTNAME_SHORT}-Server wird gestoppt...",
					stoppingFailed: "Beim Stoppen des ${IMA_PRODUCTNAME_SHORT}-Server ist ein Fehler aufgetreten." +
								"Verwenden Sie die Seite Serversteuerung, um den ${IMA_PRODUCTNAME_SHORT}-Server erneut zu starten.",
					starting: "Der ${IMA_PRODUCTNAME_SHORT}-Server wird gestartet..."
//					startingFailed: "An error occurred while starting the IBM IoT MessageSight server. " +
//								"Use the System Control page to start the IBM IoT MessageSight server."
				},
				confirmGroupNameDialog: {
					title: "Änderung des Gruppennamens bestätigen",
					content: "Es ist momentan nur ein einziger Knoten aktiv. Wenn Sie den Gruppennamen ändern, müssen Sie den Gruppennamen für andere Knoten möglicherweise manuell bearbeiten. " +
					         "Möchten Sie den Gruppennamen wirklich ändern?",
					closeButton: "Abbrechen",
					confirmButton: "Bestätigen"
				},
				dialog: {
					title: "Hochverfügbarkeitskonfiguration bearbeiten",
					saveButton: "Speichern",
					cancelButton: "Abbrechen",
					instruction: "Aktivieren Sie die Hochverfügbarkeit und definieren Sie eine Hochverfügbarkeitsgruppe, um festzulegen, mit welchem Server dieser Server gepaart wird." + 
								 "Alternativ können Sie die Hochverfügbarkeit inaktivieren. Änderungen an der Hochverfügbarkeitskonfiguration werden erst nach einem Neustart des ${IMA_PRODUCTNAME_SHORT}-Servers wirksam.",
					haEnabledLabel: "Hochverfügbarkeit aktiviert:",
					startupMode: "Startmodus:",
					haEnabledTooltip: "QuickInfo-Test",
					groupNameLabel: "Hochverfügbarkeitsgruppe:",
					groupNameToolTip: "Die Gruppe wird verwendet, um Server, die miteinander gepaart werden sollen, automatisch zu konfigurieren. " +
					                  "Der Wert muss auf beiden Servern derselbe sein. " +
					                  "Die maximale Länge dieses Werts sind 128 Zeichen. ",
					advancedSettings: "Erweiterte Einstellungen",
					advSettingsTagline: "Wenn Sie bestimmte Einstellungen für Ihre Hochverfügbarkeitskonfiguration verwenden möchten, können Sie die folgenden Einstellungen manuell konfigurieren. " +
										"Ausführliche Anweisungen und Empfehlungen zum Ändern dieser Einstellungen finden Sie in der Dokumentation zu ${IMA_PRODUCTNAME_FULL}.",
					nodePreferred: "Wenn beide Knoten im Modus für automatische Erkennung gestartet werden, ist dieser Knoten der bevorzugte primäre Knoten.",
				    localReplicationInterface: "Lokale Replikationsadresse:",
				    localDiscoveryInterface: "Lokale Erkennungsadresse:",
				    remoteDiscoveryInterface: "Ferne Erkennungsadresse:",
				    discoveryTimeout: "Erkennungszeitlimit:",
				    heartbeatTimeout: "Heartbeatzeitlimit:",
				    startModeTooltip: "Der Modus, in dem der Knoten gestartet wird. Als Modus für einen Knoten kann der Modus für automatische Erkennung oder der eigenständige Modus definiert werden." +
				    				  "Im Modus für automatische Erkennung müssen zwei Knoten gestartet werden. " +
				    				  "Die Knoten versuchen automatisch, einander zu erkennen und ein Hochverfügbarkeitspaar zu bilden. " +
				    				  "Verwenden Sie den eigenständigen Modus nur, wenn Sie einen einzigen Knoten starten. ",
				    localReplicationInterfaceTooltip: "Die IP-Adresse der Netzschnittstellenkarte, die für die Hochverfügbarkeitsreplikation auf dem lokalen Knoten eines Hochverfügbarkeitspaars verwendet wird. " +
				    								  "Sie können die IP-Adresse, die Sie zuweisen möchten, auswählen. " +
				    								  "Die lokale Replikationsadresse muss sich jedoch in einem anderen Teilnetz als die lokale Erkennungsadresse befinden.",
				    localDiscoveryInterfaceTooltip: "Die IP-Adresse der Netzschnittstellenkarte, die für die Hochverfügbarkeitserkennung auf dem lokalen Knoten eines Hochverfügbarkeitspaars verwendet wird. " +
				    							    "Sie können die IP-Adresse, die Sie zuweisen möchten, auswählen. " +
				    								"Die lokale Erkennungsadresse muss sich jedoch in einem anderen Teilnetz als die lokale Replikationsadresse befinden.",
				    remoteDiscoveryInterfaceTooltip: "Die IP-Adresse der Netzschnittstellenkarte, die für die Hochverfügbarkeitserkennung auf dem fernen Knoten eines Hochverfügbarkeitspaars verwendet wird. " +
				    								 "Sie können die IP-Adresse, die Sie zuweisen möchten, auswählen. " +
				    								 "Die ferne Erkennungsadresse muss sich jedoch in einem anderen Teilnetz als die lokale Replikationsadresse befinden.",
				    discoveryTimeoutTooltip: "Die Zeit (in Sekunden), innerhalb derer ein Server, der im Modus für automatische Erkennung gestartet wurde, eine Verbindung zum anderen Server im Hochverfügbarkeitspaar herstellen muss. " +
				    					     "Der gültige Bereich ist 10-2147483647. Der Standardwert ist 600. " +
				    						 "Wenn die Verbindung nicht innerhalb dieser Zeit hergestellt wird, wird der Server im Wartungsmodus gestartet. ",
				    heartbeatTimeoutTooltip: "Die Zeit (in Sekunden), innerhalb derer ein Server feststellen muss, ob der andere Server im Hochverfügbarkeitspaar ausgefallen ist. " + 
				    						 "Der gültige Bereich ist 1-2147483647. Der Standardwert ist 10. " +
				    						 "Wenn der primäre Server innerhalb dieser Zeit kein Heartbeat (Überwachungssignal) vom Standby-Server empfängt, arbeitet er weiterhin als primärer Server, aber der Datensynchronisationsprozess wird gestoppt. " +
				    						 "Wenn der Standby-Server innerhalb dieser Zeit kein Heartbeat (Überwachungssignal) vom primären Server empfängt, wird der Standby-Server zum primären Server. ",
				    seconds: "Sekunden",
				    nicsInvalid: "Für die lokale Replikationsadresse, die lokale Erkennungsadresse und die ferne Erkennungsadresse müssen gültige Werte angegeben werden." +
				    			 "Alternativ darf für die lokale Replikationsadresse, die lokale Erkennungsadresse und die ferne Erkennungsadresse kein Wert angegeben werden.",
				    securityRiskTitle: "Sicherheitsrisiko",
				    securityRiskText: "Möchten Sie die Hochverfügbarkeit wirklich aktivieren, ohne Replikations- und Erkennungsadressen anzugeben? " +
				    		"Wenn Sie keine vollständige Kontrolle über die Netzumgebung Ihres Server haben, werden die Informationen zu Ihrem Server selektiv rundgesendet, und Ihr Server könnte mit einem nicht anerkannten Server gepaart werden, der denselben Hochverfügbarkeitsgruppennamen verwendet. " +
				    		"<br /><br />" +
				    		"Sie können dieses Sicherheitsrisiko ausschalten, indem Sie Replikations- und Erkennungsadressen angeben."
				},
				status: {
					title: "Status",
					tagline: "Der Hochverfügbarkeitsstatus wird angezeigt. " +
					         "Wenn der Status nicht mit der Konfiguration übereinstimmt, muss der ${IMA_PRODUCTNAME_SHORT}-Server möglicherweise erneut gestartet werden, damit die letzten Konfigurationsänderungen wirksam werden.",
					editLink: "Bearbeiten",
					haLabel: "Hochverfügbarkeit:",
					haGroupLabel: "Hochverfügbarkeitsgruppe:",
					haNodesLabel: "Aktive Knoten",
					pairedWithLabel: "Zuletzt gepaart mit:",
					replicationInterfaceLabel: "Replikationsadresse:",
					discoveryInterfaceLabel: "Erkennungsadresse:",
					remoteDiscoveryInterfaceLabel: "Ferne Erkennungsadresse:",
					remoteWebUI: "<a href='{0}' target=_'blank'>Ferne Webbenutzerschnittstelle</a>",
					enabled: "Die Hochverfügbarkeit ist aktiviert.",
					disabled: "Die Hochverfügbarkeit ist inaktiviert."
				},
				startup: {
					subtitle: "Startmodus des Knotens",
					tagline: "Ein Knoten kann in den Modus für automatische Erkennung oder in den eigenständigen Modus versetzt werden. Im Modus für automatische Erkennung müssen zwei Knoten gestartet werden. " +
							 "Die Knoten versuchen automatisch, einander zu erkennen und ein Hochverfügbarkeitspaar zu bilden. Der eigenständige Modus sollte nur verwendet werden, wenn ein einziger Knoten gestartet wird. ",
					mode: "Startmodus:",
					modeAuto: "Automatische Erkennung",
					modeAlone: "Eigenständig",
					primary: "Wenn beide Knoten im Modus für automatische Erkennung gestartet werden, ist dieser Knoten der bevorzugte primäre Knoten."
				},
				remote: {
					subtitle: "Erkennungsadresse des fernen Knotens",
					tagline: "Die IPv4-Adresse oder IPv6-Adresse der Erkennungsadresse auf dem fernen Knoten in einem Hochverfügbarkeitspaar.",
					discoveryInt: "Erkennungsadresse:",
					discoveryIntTooltip: "Geben Sie die IPv4-Adresse oder die IPv6-Adresse für die Konfiguration der Erkennungsadresse des fernen Knotens ein."
				},
				local: {
					subtitle: "NIC-Adresse für Hochverfügbarkeit",
					tagline: "Die IPv4- oder IPv6-Adressen der Replikations- und Erkennungsadressen des lokalen Knotens.",
					replicationInt: "Replikationsadresse:",
					replicationIntTooltip: "Geben Sie die IPv4- oder IPv6-Adresse für die Konfiguration der Replikationsadresse des fernen Knotens ein. ",
					replicationError: "Die Replikationsadresse für die Hochverfügbarkeit ist nicht verfügbar.",
					discoveryInt: "Erkennungsadresse:",
					discoveryIntTooltip: "Geben Sie die IPv4- oder IPv6-Adresse für die Konfiguration der Erkennungsadresse des fernen Knotens ein. "
				},
				timeouts: {
					subtitle: "Zeitlimits",
					tagline: "Wenn ein Knoten mit aktivierter Hochverfügbarkeit im Modus für automatisch Erkennung gestartet wird, versucht er, mit dem anderen Knoten im Hochverfügbarkeitspaar zu kommunizieren. " +
							 "Falls innerhalb des Erkennungszeitlimits keine Verbindung hergestellt werden kann, wird der Knoten stattdessen im Wartungsmodus gestartet. ",
					tagline2: "Das Heartbeatzeitlimit wird verwendet, um zu erkennen, ob der andere Knoten in einem Hochverfügbarkeitspaar ausgefallen ist. " +
							  "Wenn der primäre Knoten kein Heartbeat (Überwachungssignal) vom Standby-Knoten empfängt, arbeitet er weiterhin als primärer Knoten, aber der Datensynchronisationsprozess wird gestoppt. " +
							  "Wenn der Standby-Knoten kein Heartbeat (Überwachungssignal) vom primären Knoten empfängt, wird der Standby-Knoten zum primären Knoten. ",
					discoveryTimeout: "Erkennungszeitlimit:",
					discoveryTimeoutTooltip: "Die Zeit in Sekunden zwischen 10 und 2.147.483.647, innerhalb derer der Knoten versucht, eine Verbindung zum anderen Knoten im Paar herzustellen.",
					discoveryTimeoutValidation: "Geben Sie eine Zahl zwischen 10 und 2.147.483.647 ein.",
					discoveryTimeoutError: "Es ist kein Erkennungszeitlimit verfügbar.",
					heartbeatTimeout: "Heartbeatzeitlimit:",
					heartbeatTimeoutTooltip: "Die Zeit in Sekunden zwischen 1 und 2.147.483.647 für den Knotenheartbeat.",
					heartbeatTimeoutValidation: "Geben Sie eine Zahl zwischen 1 und 2.147.483.647 ein.",
					seconds: "Sekunden"
				}
			},
			webUISecurity: {
				title: "Einstellungen für die Webbenutzerschnittstelle",
				tagline: "Konfigurieren Sie Einstellungen für die Webbenutzerschnittstelle.",
				certificate: {
					subtitle: "Webbenutzerschnittstellenzertifikat",
					tagline: "Konfigurieren Sie das Zertifikat, das zum Sichern der Kommunikation mit der Webbenutzerschnittstelle verwendet werden wird."
				},
				timeout: {
					subtitle: "Sitzungszeitlimit &amp; Verfallszeit des LTPA-Tokens",
					tagline: "Diese Einstellungen steuern die Benutzeranmeldesitzungen basierend auf den Aktivitäten zwischen dem Browser und dem Server sowie der Gesamtanmeldezeit. " +
							 "Die Änderung des Zeitlimits für Sitzungsinaktivität oder der Verfallszeit des LTPA-Tokens kann dazu führen, dass aktive Anmeldesitzungen ungültig werden.",
					inactivity: "Zeitlimit für Sitzungsinaktivität",
					inactivityTooltip: "Das Zeitlimit für Sitzungsinaktivität steuert, wie lange ein Benutzer angemeldet bleiben kann, wenn keine Aktivitäten zwischen dem Browser und dem Server stattfinden. " +
							           "Das Inaktivitätszeitlimit muss kleiner-gleich der Verfallszeit des LTPA-Tokens sein.",
					expiration: "Verfallszeit des LTPA-Tokens",
					expirationTooltip: "Die Verfallszeit des LTPA-Tokens (Lightweight Third-Party Authentication) steuert, wie lange ein Benutzer angemeldet bleiben kann, unabhängig davon, ob Aktivitäten zwischen dem Browser und dem Server stattfinden oder nicht." +
							           "Die Verfallszeit muss größer-gleich dem Zeitlimit für Sitzungsinaktivität sein. Weitere Informationen finden Sie in der Dokumentation. ", 
					unit: "Minuten",
					save: "Speichern",
					saving: "Speicheroperation wird durchgeführt...",
					success: "Die Speicheroperation war erfolgreich. Melden Sie sich ab und dann wieder an.",
					failed: "Speicheroperation fehlgeschlagen",
					invalid: {
						inactivity: "Das Zeitlimit für Sitzungsinaktivität muss ein Wert zwischen 1 und dem Wert für die Verfallszeit des LTPA-Tokens sein.",
						expiration: "Der Wert für die Verfallszeit des LTPA-Tokens muss ein Wert zwischen dem Wert für das Zeitlimit für Sitzungsinaktivität und 1440 sein."
					},
					getExpirationError: "Beim Abrufen der Verfallszeit des LTPA-Tokens ist ein Fehler aufgetreten.",
					getInactivityError: "Beim Abrufen des Zeitlimits für Sitzungsinaktivität ist ein Fehler aufgetreten.",
					setExpirationError: "Beim Festlegen der Verfallszeit des LTPA-Tokens ist ein Fehler aufgetreten.",
					setInactivityError: "Beim Festlegen des Zeitlimits für Sitzungsinaktivität ist ein Fehler aufgetreten."
				},
				cacheauth: {
					subtitle: "Zeitlimit für Authentifizierungscache",
					tagline: "Das Zeitlimit für den Authentifizierungscache gibt an, wie lange ein authentifizierter Berechtigungsnachweis im Cache gültig ist. " +
					"Höhere Werte können das Sicherheitsrisiko erhöhen, weil ein widerrufener Berechtigungsnachweis länger im Cache bleibt. " +
					"Niedrigere Werte können sich auf die Leistung auswirken, weil häufiger auf die Benutzerregistry zugegriffen werden muss. ",
					cacheauth: "Zeitlimit für Authentifizierungscache",
					cacheauthTooltip: "Das Zeitlimit für den Authentifizierungscache gibt die Zeit an, nach der ein Eintrag im Cache entfernt wird. " +
							"Der Wert muss zwischen 1 und 3600 Sekunden (1 Stunde) liegen. Der Wert sollte nicht höher sein als ein Drittel des Werts für die Verfallszeit des LTPA-Tokens. " +
							"Weitere Informationen finden Sie in der Dokumentation.", 
					unit: "Sekunden",
					save: "Speichern",
					saving: "Speicheroperation wird durchgeführt...",
					success: "Sichern erfolgreich",
					failed: "Speicheroperation fehlgeschlagen",
					invalid: {
						cacheauth: "Das Zeitlimit für den Authentifizierungscache muss ein Wert zwischen 1 und 3600 sein."
					},
					getCacheauthError: "Beim Abrufen des Zeitlimits für den Authentifizierungscache ist ein Fehler aufgetreten.",
					setCacheauthError: "Beim Festlegen des Zeitlimits für den Authentifizierungscache ist ein Fehler aufgetreten."
				},
				readTimeout: {
					subtitle: "Zeitlimit für HTTP-Leseoperationen",
					tagline: "Die maximale Wartezeit für den Abschluss einer Leseanforderung an einem Socket nach der ersten Leseoperation. ",
					readTimeout: "Zeitlimit für HTTP-Leseoperationen",
					readTimeoutTooltip: "Ein Wert zwischen 1 und 300 Sekunden (5 Minuten).", 
					unit: "Sekunden",
					save: "Speichern",
					saving: "Speicheroperation wird durchgeführt...",
					success: "Sichern erfolgreich",
					failed: "Speicheroperation fehlgeschlagen",
					invalid: {
						readTimeout: "Der Wert für das Zeitlimit für HTTP-Leseoperationen muss ein Wert zwischen 1 und 300 sein."
					},
					getReadTimeoutError: "Beim Abrufen des Zeitlimits für HTTP-Leseoperationen ist ein Fehler aufgetreten.",
					setReadTimeoutError: "Beim Festlegen des Zeitlimits für HTTP-Leseoperationen ist ein Fehler aufgetreten."
				},
				port: {
					subtitle: "IP-Adresse und Port",
					tagline: "Wählen Sie die IP-Adresse und den Port für die Webbenutzerschnittstellenkommunikation aus. Wenn Sie einen der Werte ändern, müssen Sie sich möglicherweise an der neuen IP-Adresse und dem neuen Port anmelden." +
							 "<em>Die Auswahl von \"Alle\" für die IP-Adressen wird nicht empfohlen, weil die Webbenutzerschnittstelle dadurch im Internet verfügbar gemacht werden könnte.</em>",
					ipAddress: "IP-Adresse",
					all: "Alle",
					interfaceHint: "Schnittstelle: {0}",
					port: "Port",
					save: "Speichern",
					saving: "Speicheroperation wird durchgeführt...",
					success: "Die Speicheroperation war erfolgreich. Es kann eine Minute dauern, bis die Änderung wirksam wird. Schließen Sie Ihren Browser und melden Sie sich erneut an.",
					failed: "Speicheroperation fehlgeschlagen",
					tooltip: {
						port: "Geben Sie einen verfügbaren Port ein. Die gültigen Portwerte sind 1 bis 8999, 9087 oder 9100 und 65535 einschließlich."
					},
					invalid: {
						interfaceDown: "Die Schnittstelle {0} meldet, dass sie inaktiv ist. Überprüfen Sie die Schnittstelle oder wählen Sie eine andere Adresse aus. ",
						interfaceUnknownState: "Beim Abfragen des Status von {0} ist ein Fehler aufgetreten. Überprüfen Sie die Schnittstelle oder wählen Sie eine andere Adresse aus. ",
						port: "Die gültigen Portnummern sind 1 bis 8999, 9087 oder 9100 und 65535 einschließlich. "
					},					
					getIPAddressError: "Beim Abrufen der IP-Adresse ist ein Fehler aufgetreten. ",
					getPortError: "Beim Abrufen des Ports ist ein Fehler aufgetreten.",
					setIPAddressError: "Beim Festlegen der IP-Adresse ist ein Fehler aufgetreten. ",
					setPortError: "Beim Festlegen des Ports ist ein Fehler aufgetreten. "					
				},
				certificate: {
					subtitle: "Sicherheitszertifikat",
					tagline: "Laden Sie das von der Webbenutzerschnittstelle verwendete Zertifikat hoch und ersetzen Sie es.",
					addButton: "Zertifikat hinzufügen",
					editButton: "Zertifikat bearbeiten",
					addTitle: "Webbenutzerschnittstellenzertifikat ersetzen",
					editTitle: "Webbenutzerschnittstellenzertifikat bearbeiten",
					certificateLabel: "Zertifikat: ",
					keyLabel: "Privater Schlüssel:",
					expiryLabel: "Verfallszeit des Zertifikats:",
					defaultExpiryDate: "Warnung! Das Standardzertifikat ist im Gebrauch und muss ersetzt werden.",
					defaultCert: "Standard",
					success: "Es kann eine Minute dauern, bis die Änderung wirksam wird. Schließen Sie Ihren Browser und melden Sie sich erneut an."
				},
				service: {
					subtitle: "Service",
					tagline: "Legen Sie die Tracestufen für die Generierung von Unterstützungsinformationen für die Webbenutzerschnittstelle fest.",
					traceEnabled: "Trace aktivieren",
					traceSpecificationLabel: "Tracespezifikation",
					traceSpecificationTooltip: "Die vom IBM Support angegebene Tracespezifikationszeichenfolge.",
				    save: "Speichern",
				    getTraceError: "Beim Abrufen der Tracespezifikation ist ein Fehler aufgetreten. ",
				    setTraceError: "Beim Festlegen der Tracespezifikation ist ein Fehler aufgetreten. ",
				    clearTraceError: "Beim Löschen der Tracespezifikation ist ein Fehler aufgetreten. ",
					saving: "Speicheroperation wird durchgeführt...",
					success: "Die Speicheroperation war erfolgreich. ",
					failed: "Speicheroperation fehlgeschlagen"
				}
			}
		},
        platformDataRetrieveError: "Beim Abrufen der Plattformdaten ist ein Fehler aufgetreten.",
        // Strings for client certs that can be used for certificate authentication
		clientCertsTitle: "Clientzertifikate",
		clientCertsButton: "Clientzertifikate",
        stopHover: "Server herunterfahren und alle Zugriffe auf den Server verhindern",
        restartLink: "Server erneut starten",
		restartMaintOnLink: "Wartung starten",
		restartMaintOnHover: "Server erneut starten und dann im Wartungsmodus ausführen",
		restartMaintOffLink: "Wartung stoppen",
		restartMaintOffHover: "Server erneut starten und dann im Produktionsmodus ausführen",
		restartCleanStoreLink: "Speicher bereinigen",
		restartCleanStoreHover: "Serverspeicher bereinigen und Server erneut starten",
        restartDialogTitle: "${IMA_PRODUCTNAME_SHORT}-Server erneut starten",
        restartMaintOnDialogTitle: "${IMA_PRODUCTNAME_SHORT}-Server im Wartungsmodus erneut starten",
        restartMaintOffDialogTitle: "${IMA_PRODUCTNAME_SHORT}-Server im Produktionsmodus erneut starten",
        restartCleanStoreDialogTitle: "${IMA_PRODUCTNAME_SHORT}-Serverspeicher bereinigen",
        restartDialogContent: "Möchten Sie den ${IMA_PRODUCTNAME_SHORT}-Server wirklich erneut starten? Durch den Neustart des Servers wird das gesamte Messaging unterbrochen.",
        restartMaintOnDialogContent: "Möchten Sie den ${IMA_PRODUCTNAME_SHORT}-Server wirklich im Wartungsmodus erneut starten? Durch das Starten des Servers im Wartungsmodus wird das gesamte Messaging gestoppt.",
        restartMaintOffDialogContent: "Möchten Sie den ${IMA_PRODUCTNAME_SHORT}-Server wirklich im Produktionsmodus erneut starten? Durch das Starten des Servers im Produktionsmodus wird das Messaging erneut gestartet.",
        restartCleanStoreDialogContent: "Die Ausführung der Aktion <strong>Speicher bereinigen</strong> führt zu einem Verlust der persistent gespeicherten Messaging-Daten.<br/><br/>" +
        	"Möchten Sie die Aktion <strong>Speicher bereinigen</strong> wirklich ausführen?",
        restartOkButton: "Erneut starten",        
        cleanOkButton: "Speicher bereinigen und erneut starten",
        restarting: "Der ${IMA_PRODUCTNAME_SHORT}-Server wird erneut gestartet...",
        restartingFailed: "Beim erneuten Starten des ${IMA_PRODUCTNAME_SHORT}-Server ist ein Fehler aufgetreten.",
        remoteLogLevelTagLine: "Legen Sie die Nachrichtenebene fest, die Sie in den Serverprotokollen anzeigen möchten.  ", 
        errorRetrievingTrustedCertificates: "Beim Abrufen vertrauenswürdiger Zertifikate ist ein Fehler aufgetreten.",

        // New page title and tagline:  Admin Endpoint
        adminEndpointPageTitle: "Administratorendpunktkonfiguration",
        adminEndpointPageTagline: "Administratorendpunkt konfigurieren und Konfigurationsrichtlinien erstellen." +
        		"Die Webbenutzerschnittstelle stellt eine Verbindung zum Administratorendpunkt her, um Verwaltungs- und Überwachungstasks auszuführen. " +
        		"Konfigurationsrichtlinien steuern, welche Verwaltungs- und Überwachungstasks ein Benutzer ausführen darf. ",
        // HA config strings
        haReplicationPortLabel: "Replikationsport:",
		haExternalReplicationInterfaceLabel: "Externe Replikationsadresse:",
		haExternalReplicationPortLabel: "Externer Replikationsport:",
        haRemoteDiscoveryPortLabel: "Ferner Erkennungsport:",
        haReplicationPortTooltip: "Die Portnummer, die für die Hochverfügbarkeitsreplikation auf dem lokalen Knoten im Hochverfügbarkeitspaar verwendet wird.",
		haReplicationPortInvalid: "Der Replikationsport ist ungültig." +
				"Der gültige Bereich ist 1024-65535. Der Standardwert ist 0 und lässt die Zufallsauswahl eines verfügbaren Ports zur Verbindungszeit zu.",
		haExternalReplicationInterfaceTooltip: "Die IPv4- oder IPv6-Adresse der NIC, die für die Hochverfügbarkeitsreplikation verwendet werden soll.",
		haExternalReplicationPortTooltip: "Die externe Portnummer, die für die Hochverfügbarkeitsreplikation auf dem lokalen Knoten im Hochverfügbarkeitspaar verwendet wird.",
		haExternalReplicationPortInvalid: "Der externe Replikationsport ist ungültig." +
				"Der gültige Bereich ist 1024-65535. Der Standardwert ist 0 und lässt die Zufallsauswahl eines verfügbaren Ports zur Verbindungszeit zu.",
        haRemoteDiscoveryPortTooltip: "Die Portnummer, die für die Hochverfügbarkeitserkennung auf dem fernen Knoten im Hochverfügbarkeitspaar verwendet wird.",
		haRemoteDiscoveryPortInvalid: "Der ferne Erkennungsport ist ungültig." +
				 "Der gültige Bereich ist 1024-65535. Der Standardwert ist 0 und lässt die Zufallsauswahl eines verfügbaren Ports zur Verbindungszeit zu.",
		haReplicationAndDiscoveryBold: "Replikations- und Erkennungsadressen ",
		haReplicationAndDiscoveryNote: "(wenn eine definiert wird, müssen alle definiert werden)",
		haUseSecuredConnectionsLabel: "Sichere Verbindungen verwenden:",
		haUseSecuredConnectionsTooltip: "Verwenden Sie TLS für die Erkennungs- und Datenkanäle zwischen Knoten eines HA-Paars.",
		haUseSecuredConnectionsEnabled: "Die Verwendung sicherer Verbindungen ist aktiviert.",
		haUseSecuredConnectionsDisabled: "Die Verwendung sicherer Verbindungen ist inaktiviert.",
		mqConnectivityEnableLabel: "MQ Connectivity aktivieren",
		savingProgress: "Speicheroperation wird durchgeführt...",
	    setMqConnEnabledError: "Beim Festlegen des MQ Connectivity-Aktivierungsstatus ist ein Fehler aufgetreten.",
	    allowCertfileOverwriteLabel: "Überschreiben",
		allowCertfileOverwriteTooltip: "Lässt das Überschreiben des Zertifikats und des privaten Schlüssels zu.",
		stopServerWarningTitle: "Das Stoppen des Servers verhindert den Neustart über die Webbenutzerschnittstelle!",
		stopServerWarning: "Durch das Stoppen des Servers werden alle Messaging- und Verwaltungszugriffe über REST-Anforderungen und die Webbenutzerschnittstelle verhindert. <b>Verwenden Sie den Link Server erneut starten, um zu verhindern, dass Sie den Zugriff auf den Server über die Webbenutzerschnittstelle verlieren.</b>",
		// New Web UI user dialog strings to allow French translation to insert a space before colon
		webuiuserLabel: "Benutzer-ID:",
		webuiuserdescLabel: "Beschreibung:",
		webuiuserroleLabel: "Rolle:"

});
