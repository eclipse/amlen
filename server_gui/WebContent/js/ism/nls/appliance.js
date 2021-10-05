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

		// ------------------------------------------------------------------------
		// Appliance
		// ------------------------------------------------------------------------
		appliance : {
			users: {
				title: "Web UI User Management",
				tagline: "Give users access to the Web UI.",
				usersSubTitle: "Web UI Users",
				webUiUsersTagline: "System administrators can define, edit, or delete Web UI users.",
				itemName: "User",
				itemTooltip: "",
				addDialogTitle: "Add User",
				editDialogTitle: "Edit User",
				removeDialogTitle: "Delete User",
				resetPasswordDialogTitle: "Reset Password",
				// TRNOTE: Do not translate addGroupDialogTitle, editGroupDialogTitle, removeGroupDialogTitle
				//addGroupDialogTitle: "Add Group",
				//editGroupDialogTitle: "Edit Group",
				//removeGroupDialogTitle: "Delete Group",
				grid: {
					userid: "User ID",
					// TRNOTE: Do not translate groupid
					//groupid: "Group ID",
					description: "Description",
					groups: "Role",
					noEntryMessage: "No items to display",
					// TRNOTE: Do not translate ldapEnabledMessage
					//ldapEnabledMessage: "Messaging users and groups configured on the appliance are disabled because an external LDAP connection is enabled.",
					refreshingLabel: "Updating..."
				},
				dialog: {
					useridTooltip: "Enter the User ID, which must not have leading or trailing spaces.",
					applianceUserIdTooltip: "Enter the User ID, which must consist of only the letters A-Za-z, numbers 0-9, and the four special characters - _ . and +",					
					useridInvalid: "The User ID must not have leading or trailing spaces.",
					applianceInvalid: "The User ID must consist of only the letters A-Za-z, numbers 0-9, and the four special characters - _ . and +",					
					// TRNOTE: Do not translate groupidTooltip, groupidInvalid
					//groupidTooltip: "Enter the Group ID, which must not have leading or trailing spaces.",
					//groupidInvalid: "The Group ID must not have leading or trailing spaces.",
					SystemAdministrators: "System Administrators",
					MessagingAdministrators: "Messaging Administrators",
					Users: "Users",
					applianceUserInstruction: "System Administrators can create, update or delete other Web UI administrators and Web UI users.",
					// TRNOTE: Do not translate messagingUserInstruction, messagingGroupInstruction
					//messagingUserInstruction: "Messaging users can be added to messaging policies to control the messaging resources that a user can access.",
					//messagingGroupInstruction: "Messaging groups can be added to messaging policies to control the messaging resources that a group of users can access.",				                    
					password1: "Password:",
					password2: "Confirm Password:",
					password2Invalid: "The passwords do not match.",
					passwordNoSpaces: "The password must not have leading or trailing spaces.",
					saveButton: "Save",
					cancelButton: "Cancel",
					refreshingGroupsLabel: "Retrieving Groups..."
				},
				returnCodes: {
					CWLNA5056: "Some settings could not be saved"
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
				title: "Delete entry",
				content: "Are you sure you want to delete this record?",
				deleteButton: "Delete",
				cancelButton: "Cancel"
			},
			savingProgress: "Saving...",
			savingFailed: "Save failed.",
			deletingProgress: "Deleting...",
			deletingFailed: "Delete failed.",
			testingProgress: "Testing...",
			uploadingProgress: "Uploading...",
			dialog: { 
				saveButton: "Save",
				cancelButton: "Cancel",
				testButton: "Test connection"
			},
			invalidName: "A record with that name already exists.",
			invalidChars: "A valid host name or IP address is required.",
			invalidRequired: "A value is required.",
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
				nothingToTest: "An IPv4 or IPv6 address must be provided to test the connection.",
				testConnFailed: "Test connection failed",
				testConnSuccess: "Test connection succeeded",
				testFailed: "Test failed",
				testSuccess: "Test succeeded"
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
				title: "Security Settings",
				tagline: "Import keys and certificates to secure connections to the server.",
				securityProfilesSubTitle: "Security Profiles",
				securityProfilesTagline: "System administrators can define, edit, or delete security profiles that identify the methods used to verify clients.",
				certificateProfilesSubTitle: "Certificate Profiles",
				certificateProfilesTagline: "System administrators can define, edit, or delete certificate profiles that verify user identities.",
				ltpaProfilesSubTitle: "LTPA Profiles",
				ltpaProfilesTagline: "System administrators can define, edit, or delete lightweight third party authentication (LTPA) profiles to enable single sign on across multiple servers.",
				oAuthProfilesSubTitle: "OAuth Profiles",
				oAuthProfilesTagline: "System administrators can define, edit, or delete OAuth profiles to enable single sign on across multiple servers.",
				systemWideSubTitle: "Server-wide Security Settings",
				useFips: "Use FIPS 140-2 profile for secure messaging communications",
				useFipsTooltip: "Select this option to use cryptography that complies with Federal Information Processing Standards (FIPS) 140-2 for endpoints secured with a security profile. " +
				                "Changes to this setting require a restart of the ${IMA_SVR_COMPONENT_NAME}.",
				retrieveFipsError: "An error occurred while retrieving the FIPS setting.",
				fipsDialog: {
					title: "Set FIPS",
					content: "The ${IMA_SVR_COMPONENT_NAME} must be restarted for this change to take effect. Are you sure you want to change this setting?"	,
					okButton: "Set and Restart",
					cancelButton: "Cancel Change",
					failed: "An error occurred while changing the FIPS setting.",
					stopping: "Stopping the ${IMA_SVR_COMPONENT_NAME}...",
					stoppingFailed: "An error occurred while stopping the ${IMA_SVR_COMPONENT_NAME}.",
					starting: "Starting the ${IMA_SVR_COMPONENT_NAME}...",
					startingFailed: "An error occurred while starting the ${IMA_SVR_COMPONENT_NAME}."
				}
			},
			securityProfiles: {
				grid: {
					name: "Name",
					mprotocol: "Minimum Protocol Method",
					ciphers: "Ciphers",
					certauth: "Use Client Certificates",
					clientciphers: "Use Client's Ciphers",
					passwordAuth: "Use Password Authentication",
					certprofile: "Certificate Profile",
					ltpaprofile: "LTPA Profile",
					oAuthProfile: "OAuth Profile",
					clientCertsButton: "Trusted Certificates"
				},
				trustedCertMissing: "Trusted certificate missing",
				retrieveError: "An error occurred retrieving security profiles.",
				addDialogTitle: "Add Security Profile",
				editDialogTitle: "Edit Security Profile",
				removeDialogTitle: "Delete Security Profile",
				dialog: {
					nameTooltip: "A name consisting of at most 32 alphanumeric characters. The first character must not be a number.",
					mprotocolTooltip: "Lowest level of protocol method permitted when connecting.",
					ciphersTooltip: "<strong>Best:&nbsp;</strong> Select the most secure cipher supported by the server and the client.<br />" +
							        "<strong>Fast:&nbsp;</strong> Select the fastest high security cipher supported by the server and the client.<br />" +
							        "<strong>Medium:&nbsp;</strong> Select the fastest medium or high security cipher supported by the client.",
				    ciphers: {
				    	best: "Best",
				    	fast: "Fast",
				    	medium: "Medium"
				    },
					certauthTooltip: "Choose whether to authenticate the client certificate.",
					clientciphersTooltip: "Choose whether to allow the client to determine the cipher suite to use when connecting.",
					passwordAuthTooltip: "Choose whether to require a valid user ID and password when connecting.",
					passwordAuthTooltipDisabled: "Cannot be modified because an LTPA or OAuth Profile is selected. To modify, change the LTPA or OAuth Profile to <em>Choose One</em>",
					certprofileTooltip: "Select an existing certificate profile to associate with this security profile.",
					ltpaprofileTooltip: "Select an existing LTPA profile to associate with this security profile. " +
					                    "If an LTPA profile is selected, then Use Password Authentication must also be selected.",
					ltpaprofileTooltipDisabled: "An LTPA profile cannot be selected because an OAuth profile is selected or Use Password Authentication is not selected.",
					oAuthProfileTooltip: "Select an existing OAuth profile to associate with this security profile. " +
                    					  "If an OAuth profile is selected, then Use Password Authentication must also be selected.",
                    oAuthProfileTooltipDisabled: "An OAuth profile cannot be selected because an LTPA profile is selected or Use Password Authentication is not selected.",
					noCertProfilesTitle: "No Certificate Profiles",
					noCertProfilesDetails: "When <em>Use TLS</em> is selected, a security profile can be defined only after a certificate profile has first been defined.",
					chooseOne: "Choose One"
				},
				certsDialog: {
					title: "Trusted Certificates",
					instruction: "Upload certificates to or delete certificates from the client certificate truststore for a security profile",
					securityProfile: "Security Profile",
					certificate: "Certificate",
					browse: "Browse",
					browseHint: "Select a file...",
					upload: "Upload",
					close: "Close",
					remove: "Delete",
					removeTitle: "Delete Certificate",
					removeContent: "Are you sure you want to delete this certificate?",
					retrieveError: "An error occurred retrieving certificates.",
					uploadError: "An error occurred uploading the certificate file.",
					deleteError: "An error occurred deleting the certificate file."	
				}
			},
			certificateProfiles: {
				grid: {
					name: "Name",
					certificate: "Certificate",
					privkey: "Private Key",
					certexpiry: "Certificate Expiry",
					noDate: "Error retrieving date..."
				},
				retrieveError: "An error occurred retrieving certificate profiles.",
				uploadCertError: "An error occurred uploading the certificate file.",
				uploadKeyError: "An error occurred uploading the key file.",
				uploadTimeoutError: "An error occurred uploading the certificate or key file.",
				addDialogTitle: "Add Certificate Profile",
				editDialogTitle: "Edit Certificate Profile",
				removeDialogTitle: "Delete Certificate Profile",
				dialog: {
					certpassword: "Certificate Password:",
					keypassword: "Key Password:",
					browse: "Browse...",
					certificate: "Certificate:",
					certificateTooltip: "", // "Upload a certificate"
					privkey: "Private Key:",
					privkeyTooltip: "", // "Upload a private key"
					browseHint: "Select a file...",
					duplicateFile: "A file with that name already exists.",
					identicalFile: "The certificate or key cannot be the same file.",
					noFilesToUpload: "To update the certificate profile, select at least one file to upload."
				}
			},
			ltpaProfiles: {
				addDialogTitle: "Add LTPA Profile",
				editDialogTitle: "Edit LTPA Profile",
				instruction: "Use an LTPA (Lightweight third-party authentication) profile in a security profile to enable single sign on across multiple servers.",
				removeDialogTitle: "Delete LTPA Profile",				
				keyFilename: "Key Filename",
				keyFilenameTooltip: "The file containing the exported LTPA key.",
				browse: "Browse...",
				browseHint: "Select a file...",
				password: "Password",
				saveButton: "Save",
				cancelButton: "Cancel",
				retrieveError: "An error occurred retrieving LTPA profiles.",
				duplicateFileError: "A key file with that name already exists.",
				uploadError: "An error occurred uploading the key file.",
				noFilesToUpload: "To update the LTPA profile, select a file to upload."
			},
			oAuthProfiles: {
				addDialogTitle: "Add OAuth Profile",
				editDialogTitle: "Edit OAuth Profile",
				instruction: "Use an OAuth profile in a security profile to enable single sign on across multiple servers.",
				removeDialogTitle: "Delete OAuth Profile",	
				resourceUrl: "Resource URL",
				resourceUrlTooltip: "The authorization server URL to use to validate the access token. The URL must include the protocol. The protocol can be either http or https.",
				keyFilename: "OAuth Server Certificate",
				keyFilenameTooltip: "The name of the file that contains the SSL certificate that is used to connect to the OAuth server.",
				browse: "Browse...",
				reset: "Reset",
				browseHint: "Select a file...",
				authKey: "Authorization Key",
				authKeyTooltip: "The name of the key that contains the access token. The default name is <em>access_token</em>.",
				userInfoUrl: "User Info URL",
				userInfoUrlTooltip: "The authorization server URL to use to retrieve the user information. The URL must include the protocol. The protocol can be either http or https.",
				userInfoKey: "User Info Key",
				userInfoKeyTooltip: "The name of the key that is used to retrieve the user information.",
				saveButton: "Save",
				cancelButton: "Cancel",
				retrieveError: "An error occurred retrieving OAuth profiles.",
				duplicateFileError: "A key file with that name already exists.",
				uploadError: "An error occurred uploading the key file.",
				urlThemeError: "Enter a valid URL with a protocol of http or https.",
				inUseBySecurityPolicyMessage: "The OAuth profile cannot be deleted because it is in use by the following Security Profile: {0}.",			
				inUseBySecurityPoliciesMessage: "The OAuth profile cannot be deleted because it is in use by the following Security Profiles: {0}."			
			},			
			systemControl: {
				pageTitle: "Server Control",
				appliance: {
					subTitle: "Server",
					tagLine: "Settings and actions that affect the ${IMA_SVR_COMPONENT_NAME_FULL}.",
					hostname: "Server name",
					hostnameNotSet: "(none)",
					retrieveHostnameError: "An error occurred while retrieving the server name.",
					hostnameError: "An error occurred while saving the server name.",
					editLink: "Edit",
	                viewLink: "View",
					firmware: "Firmware Version",
					retrieveFirmwareError: "An error occurred while retrieving the firmware version.",
					uptime: "Uptime",
					retrieveUptimeError: "An error occurred while retrieving the uptime.",
					// TRNOTE Do not translate locateLED, locateLEDTooltip
					//locateLED: "Locate LED",
					//locateLEDTooltip: "Locate your appliance by turning on the blue LED.",
					locateOnLink: "Turn On",
					locateOffLink: "Turn Off",
					locateLedOnError: "An error occurred while turning the Locate LED indicator on.",
					locateLedOffError: "An error occurred while turning the Locate LED indicator off.",
					locateLedOnSuccess: "The Locate LED indicator was turned on.",
					locateLedOffSuccess: "The Locate LED indicator was turned off.",
					restartLink: "Restart the server",
					restartError: "An error occurred while restarting the server.",
					restartMessage: "The request to restart the server was submitted.",
					restartMessageDetails: "The server might take several minutes to restart. The user interface is unavailable while the server restarts.",
					shutdownLink: "Shut down the server",
					shutdownError: "An error occurred while shutting down the server.",
					shutdownMessage: "The request to shutdown the server was submitted.",
					shutdownMessageDetails: "The server might take several minutes to shutdown. The user interface is unavailable while the server is shut down. " +
					                        "To turn the server back on, press the power button on the servers.",
// TRNOTE Do not translate sshHost, sshHostSettings, sshHostStatus, sshHostTooltip, sshHostStatusTooltip,
// retrieveSSHHostError, sshSaveError
//					sshHost: "SSH Host",
//				    sshHostSettings: "Settings",
//				    sshHostStatus: "Status",
//				    sshHostTooltip: "Set the host IP address or addresses that can access the IBM IoT MessageSight SSH console.",
//				    sshHostStatusTooltip: "The host IP addresses that the SSH service is currently using.",
//				    retrieveSSHHostError: "An error occurred while retrieving the SSH host setting and status.",
//				    sshSaveError: "An error occurred while configuring IBM IoT MessageSight to allow SSH connections on the address or addresses provided.",

				    licensedUsageLabel: "Licensed Usage",
				    licensedUsageRetrieveError: "An error occurred while retrieving the licensed usage setting.",
                    licensedUsageSaveError: "An error occurred while saving the licensed usage setting.",
				    licensedUsageValues: {
				        developers: "Developers",
				        nonProduction: "Non-Production",
				        production: "Production"
				    },
				    licensedUsageDialog: {
				        title: "Change Licensed Usage",
				        instruction: "Change the licensed usage of the server. " +
				        		"If you have a Proof of Entitlement (POE), the licensed usage that you select should match the authorized usage for the program as stated in the POE, either Production or Non-Production. " +
				        		"If the POE designates the Program as Non-Production, then Non-Production should be selected. " +
				        		"If there is no designation on the POE, then your usage is assumed to be Production, and Production should be selected. " +
				        		"If you do not have a proof of entitlement, then Developers should be selected.",
				        content: "When you change the licensed usage of the server, the server will restart. " +
				        		"You will be logged out of the user interface, and the user interface will be unavailable until the server restarts. " +
				        		"The server might take several minutes to restart. " +
				        		"After the server restarts, you must log in to the Web UI and accept the new license.",
				        saveButton: "Save and Restart Server",
				        cancelButton: "Cancel"
				    },
				    
					restartDialog: {
						title: "Restart the Server",
						content: "Are you sure that you want to restart the server? " +
								"You will be logged out of the user interface, and the user interface will be unavailable until the server restarts. " +
								"The server might take several minutes to restart.",
						okButton: "Restart",
						cancelButton: "Cancel"												
					},
					shutdownDialog: {
						title: "Shut Down the Server",
						content: "Are you sure that you want to shut down the server? " +
								 "You will be logged out of the user interface, and the user interface will be unavailable until the server is turned back on. " +
								 "To turn the server back on, press the power button on the server.",
						okButton: "Shut down",
						cancelButton: "Cancel"												
					},

					hostnameDialog: {
						title: "Edit Server Name",
						instruction: "Specify a server name of up to 1024 character.",
						invalid: "The value entered is not valid. The server name can contain valid UTF-8 characters.",
						tooltip: "Enter the server name.  The server name can contain valid UTF-8 characters.",
						hostname: "Server Name",
						okButton: "Save",
						cancelButton: "Cancel"												
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
					subTitle: "${IMA_SVR_COMPONENT_NAME_FULL}",
					tagLine: "Settings and actions that affect the ${IMA_SVR_COMPONENT_NAME}.",
					status: "Status",
					retrieveStatusError: "An error occurred while retrieving the status.",
					stopLink: "Stop the server",
					forceStopLink: "Force the server to stop",
					startLink: "Start the server",
					starting: "Starting",
					unknown: "Unknown",
					cleanStore: "Maintenance mode (Clean store in progress)",
					startError: "An error occurred while starting the ${IMA_SVR_COMPONENT_NAME}.",
					stopping: "Stopping",
					stopError: "An error occurred while stopping the ${IMA_SVR_COMPONENT_NAME}.",
					stopDialog: {
						title: "Stop the ${IMA_SVR_COMPONENT_NAME_FULL}",
						content: "Are you sure that you want to stop the ${IMA_SVR_COMPONENT_NAME}?",
						okButton: "Shut down",
						cancelButton: "Cancel"						
					},
					forceStopDialog: {
						title: "Force ${IMA_SVR_COMPONENT_NAME_FULL} to stop",
						content: "Are you sure that you want to force the ${IMA_SVR_COMPONENT_NAME} to stop? Forcing a stop is a last resort as it might cause loss of messages.",
						okButton: "Force Stop",
						cancelButton: "Cancel"						
					},

					memory: "Memory"
				},
				mqconnectivity: {
					subTitle: "MQ Connectivity Service",
					tagLine: "Settings and actions that affect the MQ Connectivity service.",
					status: "Status",
					retrieveStatusError: "An error occurred while retrieving the status.",
					stopLink: "Stop the service",
					startLink: "Start the service",
					starting: "Starting",
					unknown: "Unknown",
					startError: "An error occurred while starting the MQ Connectivity service.",
					stopping: "Stopping",
					stopError: "An error occurred while stopping the MQ Connectivity service.",
					started: "Running",
					stopped: "Stopped",
					stopDialog: {
						title: "Stop the MQ Connectivity Service",
						content: "Are you sure that you want to stop the MQ Connectivity service?  Stopping the service might cause loss of messages.",
						okButton: "Stop",
						cancelButton: "Cancel"						
					}
				},
				runMode: {
					runModeLabel: "Run Mode",
					cleanStoreLabel: "Clean Store",
				    runModeTooltip: "Specify the run mode of the ${IMA_SVR_COMPONENT_NAME}. Once a new run mode is selected and confirmed the new run mode will not take effect until the ${IMA_SVR_COMPONENT_NAME} is restarted.",
				    runModeTooltipDisabled: "Changing the run mode is not available because of the status of the ${IMA_SVR_COMPONENT_NAME}.",
				    cleanStoreTooltip: "Indicate if you would like the clean store action to execute when the ${IMA_SVR_COMPONENT_NAME} is restarted. The clean store action can only be selected when the ${IMA_SVR_COMPONENT_NAME} is in maintenance mode.",
					// TRNOTE {0} is a mode, such as production or maintenance.
				    setRunModeError: "An error occurred while attempting to change the run mode of the ${IMA_SVR_COMPONENT_NAME} to {0}.",
					retrieveRunModeError: "An error occurred while retrieving the run mode of the ${IMA_SVR_COMPONENT_NAME}.",
					runModeDialog: {
						title: "Confirm run mode change",
						// TRNOTE {0} is a mode, such as production or maintenance.
						content: "Are you sure that you want to change the run mode of the ${IMA_SVR_COMPONENT_NAME} to <strong>{0}</strong>? When a new run mode is selected the ${IMA_SVR_COMPONENT_NAME} must be restarted before the new run mode will take effect.",
						okButton: "Confirm",
						cancelButton: "Cancel"												
					},
					cleanStoreDialog: {
						title: "Confirm clean store action change",
						contentChecked: "Are you sure that you want the <strong>clean store</strong> action to execute during server restart? " +
								 "If the <strong>clean store</strong> action is set the ${IMA_SVR_COMPONENT_NAME} must be restarted to execute the action. " +
						         "Execution of the <strong>clean store</strong> action will result in the loss of persisted messaging data.<br/><br/>",
						content: "Are you sure that you want to cancel the <strong>clean store</strong> action?",
						okButton: "Confirm",
						cancelButton: "Cancel"												
					}
				},
				storeMode: {
					storeModeLabel: "Store Mode",
				    storeModeTooltip: "Specify the store mode of the ${IMA_SVR_COMPONENT_NAME}. Once a new store mode is selected and confirmed the ${IMA_SVR_COMPONENT_NAME} will be restarted and a clean store will be performed.",
				    storeModeTooltipDisabled: "Changing the store mode is not available because of the status of the ${IMA_SVR_COMPONENT_NAME}.",
				    // TRNOTE {0} is a mode, such as production or maintenance.
				    setStoreModeError: "An error occurred while attempting to change the store mode of the ${IMA_SVR_COMPONENT_NAME} to {0}.",
					retrieveStoreModeError: "An error occurred while retrieving the store mode of the ${IMA_SVR_COMPONENT_NAME}.",
					storeModeDialog: {
						title: "Confirm store mode change",
						// TRNOTE {0} is a mode, such as persistent or memory only.
						content: "Are you sure that you want to change the store mode of the ${IMA_SVR_COMPONENT_NAME} to <strong>{0}</strong>? " + 
								"If the store mode is changed, the ${IMA_SVR_COMPONENT_NAME} will be restarted and a clean store action will be executed. " +
								"Execution of the clean store action will result in the loss of persisted messaging data. When the store mode is changed, a clean store must be performed.",
						okButton: "Confirm",
						cancelButton: "Cancel"												
					}
				},
				logLevel: {
					subTitle: "Set Log Level",
					tagLine: "Set the level of messages that you want to see in the server logs.  " +
							 "The logs can be downloaded from <a href='monitoring.jsp?nav=downloadLogs'>Monitoring > Download Logs</a>.",
					logLevel: "Default Log",
					connectionLog: "Connection Log",
					securityLog: "Security Log",
					adminLog: "Administration Log",
					mqconnectivityLog: "MQ Connectivity Log",
					tooltip: "The log level controls the types of messages in the Default log file. " +
							 "This log file shows all log entries about the operation of the server. " +
							 "High severity entries that are shown in other logs are also logged in this log. " +
							 "A setting of Minimum will include only the most important entries. " +
							 "A setting of Maximum will result in all entries being included. " +
							 "Refer to the documentation for more information about log levels.",
					connectionTooltip: "The log level controls the types of messages in the Connection log file. " +
							           "This log file shows log entries that are related to connections. " +
							           "These entries can be used as an audit log for connections, and also indicates errors with connections. " +
							           "A setting of Minimum will include only the most important entries. A setting of Maximum will result in all entries being included. " +
					                   "Refer to the documentation for more information about log levels. ",
					securityTooltip: "The log level controls the types of messages in the Security log file. " +
							         "This log file shows log entries that are related to authentication and authorization. " +
							         "This log can be used as an audit log for security-related items. " +
							         "A setting of Minimum will include only the most important entries. A setting of Maximum will result in all entries being included. " +
					                 "Refer to the documentation for more information about log levels.",
					adminTooltip: "The log level controls the types of messages in the Admin log file. " +
							      "This log file shows log entries that are related to running administration commands from either the ${IMA_PRODUCTNAME_FULL} Web UI or the command line. " +
							      "It records all changes to the configuration of the server, and attempted changes. " +
								  "A setting of Minimum will include only the most important entries. " +
								  "A setting of Maximum will result in all entries being included. " +
					              "Refer to the documentation for more information about log levels.",
					mqconnectivityTooltip: "The log level controls the types of messages in the MQ Connectivity log file. " +
							               "This log file shows log entries that are related to MQ Connectivity function. " +
							               "These entries include problems found connecting to queue managers. " +
							               "The log can help with diagnosing problems in connecting ${IMA_PRODUCTNAME_FULL} to WebSphere<sup>&reg;</sup> MQ. " +
										   "A setting of Minimum will include only the most important entries. " +
										   "A setting of Maximum will result in all entries being included. " +
										   "Refer to the documentation for more information about log levels.",										
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log										   
					retrieveLogLevelError: "An error occurred while retrieving the {0} level.",
					// TRNOTE {0} is a log the user is setting the log level for, such as Connection Log
					setLogLevelError: "An error occurred while setting the {0} level.",
					levels: {
						MIN: "Minimum",
						NORMAL: "Normal",
						MAX: "Maximum"
					},
					serverNotAvailable: "The logging level cannot be set while the ${IMA_SVR_COMPONENT_NAME_FULL} is stopped.",
					serverCleanStore: "The logging level cannot be set while a clean store is in progress.",
					serverSyncState: "The logging level cannot be set while the primary node is synchronizing.",
					mqNotAvailable: "The MQ Connectivity log level cannot be set while the MQ Connectivity service is stopped.",
					mqStandbyState: "The MQ Connectivity log level cannot be set while the server is in standby."
				}
			},
			highAvailability: {
				title: "High Availability",
				tagline: "Configure two ${IMA_SVR_COMPONENT_NAME}s to act as a high availability pair of nodes.",
				stateLabel: "Current High Availability Status:",
				statePrimary: "Primary",
				stateStandby: "Standby",
				stateIndeterminate: "Indeterminate",
				error: "High availability encountered an error",
				save: "Save",
				saving: "Saving...",
				success: "Save successful. The ${IMA_SVR_COMPONENT_NAME} and MQ Connectivity service must be restarted for the changes to take effect.",
				saveFailed: "An error occured while updating the high availability settings.",
				ipAddrValidation : "Specify an IPv4 address or IPv6 address, for example 192.168.1.0",
				discoverySameAsReplicationError: "The discovery address must be different from the replication address.",
				AutoDetect: "Auto-detect",
				StandAlone: "Stand-alone",
				notSet: "Not set",
				yes: "Yes",
				no: "No",
				config: {
					title: "Configuration",
					tagline: "The high availablity configuration is shown. " +
					         "Changes to the configuration do not take effect until the ${IMA_SVR_COMPONENT_NAME} is restarted.",
					editLink: "Edit",
					enabledLabel: "High Availability Enabled:",
					haGroupLabel: "High Availability Group:",
					startupModeLabel: "Startup Mode:",
					replicationInterfaceLabel: "Replication Address:",
					discoveryInterfaceLabel: "Discovery Address:",
					remoteDiscoveryInterfaceLabel: "Remote Discovery Address:",
					discoveryTimeoutLabel: "Discovery Timeout:",
					heartbeatLabel: "Heartbeat Timeout:",
					seconds: "{0} seconds",
					secondsDefault: "{0} seconds (default)",
					preferedPrimary: "Auto-detect (preferred primary)"
				},
				restartInfoDialog: {
					title: "Restart Required",
					content: "Your changes were saved, but will not take effect until the ${IMA_SVR_COMPONENT_NAME} is restarted.<br /><br />" +
							 "To restart the ${IMA_SVR_COMPONENT_NAME} now, click <b>Restart Now</b>. Otherwise, click <b>Restart Later</b>.",
					closeButton: "Restart Later",
					restartButton: "Restart Now",
					stopping: "Stopping the ${IMA_SVR_COMPONENT_NAME}...",
					stoppingFailed: "An error occurred while stopping the ${IMA_SVR_COMPONENT_NAME}. " +
								"Use the Server Control page to restart the I${IMA_SVR_COMPONENT_NAME}.",
					starting: "Starting the ${IMA_SVR_COMPONENT_NAME}..."
//					startingFailed: "An error occurred while starting the IBM IoT MessageSight server. " +
//								"Use the System Control page to start the IBM IoT MessageSight server."
				},
				confirmGroupNameDialog: {
					title: "Confirm Group Name Change",
					content: "Only one node is currently active. If group name is changed, then you might need to manually edit the group name for other nodes. " +
					         "Are you sure that you want to change the group name?",
					closeButton: "Cancel",
					confirmButton: "Confirm"
				},
				dialog: {
					title: "Edit High Availability Configuration",
					saveButton: "Save",
					cancelButton: "Cancel",
					instruction: "Enable high availability and set a high availability group to determine which server this server is paired with. " + 
								 "Alternatively, disable high availability. High availability configuration changes do not take effect until you restart the ${IMA_SVR_COMPONENT_NAME}.",
					haEnabledLabel: "High Availablility Enabled:",
					startupMode: "Startup Mode:",
					haEnabledTooltip: "tool tip text",
					groupNameLabel: "High Availability Group:",
					groupNameToolTip: "Group is used to automatically configure servers to pair with each other. " +
					                  "The value must be the same on both servers. " +
					                  "The maximum length of this value is 128 characters. ",
					advancedSettings: "Advanced Settings",
					advSettingsTagline: "If you want particular settings for your high availability configuration, you can manually configure the following settings. " +
										"Refer to the ${IMA_PRODUCTNAME_FULL} documentation for detailed instructions and recommendations before you change these settings.",
					nodePreferred: "When both nodes start in auto-detect mode, this node is the preferred primary node.",
				    localReplicationInterface: "Local Replication Address:",
				    localDiscoveryInterface: "Local Discovery Address:",
				    remoteDiscoveryInterface: "Remote Discovery Address:",
				    discoveryTimeout: "Discovery Timeout:",
				    heartbeatTimeout: "Heartbeat Timeout:",
				    startModeTooltip: "The mode that the node starts in. A node can be set either to auto-detect mode or to stand-alone mode. " +
				    				  "In auto-detect mode, two nodes must be started. " +
				    				  "The nodes automatically try to detect one another and establish an HA pair. " +
				    				  "Use stand-alone mode only when you are starting a single node.",
				    localReplicationInterfaceTooltip: "The IP address of the  NIC that is used for HA replication on the local node in the HA pair. " +
				    								  "You can choose the IP address that you want to assign. " +
				    								  "However, the local replication address must be on a different subnet to the local discovery address.",
				    localDiscoveryInterfaceTooltip: "The IP address of the  NIC that is used for HA discovery on the local node in the HA pair. " +
				    							    "You can choose the IP address that you want to assign. " +
				    								"However, the local discovery address must be on a different subnet to the local replication address.",
				    remoteDiscoveryInterfaceTooltip: "The IP address of the NIC that is used for HA discovery on the remote node in the HA pair. " +
				    								 "You can choose the IP address that you want to assign. " +
				    								 "However, the remote discovery address must be on a different subnet to the local replication address.",
				    discoveryTimeoutTooltip: "The time, in seconds, within which a server that is started in auto-detect mode must connect to the other server in the HA pair. " +
				    					     "The valid range is 10 - 2147483647. The default is 600. " +
				    						 "If the connection is not made within that time, the server starts in maintenance mode.",
				    heartbeatTimeoutTooltip: "The time, in seconds, within which a server must determine whether the other server in the HA pair fails. " + 
				    						 "The valid range is 1 - 2147483647. The default is 10. " +
				    						 "If the primary server does not receive a heartbeat from the standby server within this time, it continues to work as the primary server but the data synchronization process is stopped. " +
				    						 "If the standby server does not receive a heartbeat from the primary server within this time, the standby server becomes the primary server.",
				    seconds: "Seconds",
				    nicsInvalid: "Local replication address, local discovery address and remote discovery address must all have valid values provided. " +
				    			 "Alternatively, local replication address, local discovery address and remote discovery address may have no values provided. ",
				    securityRiskTitle: "Security Risk",
				    securityRiskText: "Are you sure you want to enable high availability without specifying replication and discovery addresses? " +
				    		"If you do not have complete control over the network environment of your server, information about your server will be multicast and your server might pair with an untrusted server that uses the same high availability group name." +
				    		"<br /><br />" +
				    		"You can eliminate this security risk by specifying replication and discovery addresses."
				},
				status: {
					title: "Status",
					tagline: "The high availablity status is shown. " +
					         "If the status does not match the configuration, the ${IMA_SVR_COMPONENT_NAME} might need to be restarted for recent configuration changes to take effect.",
					editLink: "Edit",
					haLabel: "High Availability:",
					haGroupLabel: "High Availability Group:",
					haNodesLabel: "Active Nodes",
					pairedWithLabel: "Last Paired With:",
					replicationInterfaceLabel: "Replication Address:",
					discoveryInterfaceLabel: "Discovery Address:",
					remoteDiscoveryInterfaceLabel: "Remote Discovery Address:",
					remoteWebUI: "<a href='{0}' target=_'blank'>Remote Web UI</a>",
					enabled: "High Availability is enabled.",
					disabled: "High Availability is disabled"
				},
				startup: {
					subtitle: "Node Startup Mode",
					tagline: "A node can be set in either auto-detect or stand-alone mode. In auto-detect mode, two nodes must be started. " +
							 "The nodes will automatically try to detect one another and establish a high availability pair. Stand-alone mode should be used only when starting a single node.",
					mode: "Startup Mode:",
					modeAuto: "Auto-detect",
					modeAlone: "Stand-alone",
					primary: "When both nodes start in auto-detect mode, this node is the preferred primary node."
				},
				remote: {
					subtitle: "Remote Node Discovery Address",
					tagline: "The IPv4 address or IPv6 address of the discovery address on the remote node in the high availability pair.",
					discoveryInt: "Discovery Address:",
					discoveryIntTooltip: "Enter IPv4 address or IPv6 address for configuring remote node discovery address."
				},
				local: {
					subtitle: "High Availability NIC Address",
					tagline: "The IPv4 addresses or IPv6 addresses of the replication and discovery addresses of the local node.",
					replicationInt: "Replication Address:",
					replicationIntTooltip: "Enter IPv4 address or IPv6 address for configuring replication address of the local node.",
					replicationError: "High availability replication address is not available",
					discoveryInt: "Discovery Address:",
					discoveryIntTooltip: "Enter IPv4 address or IPv6 address for configuring discovery address of the local node."
				},
				timeouts: {
					subtitle: "Timeouts",
					tagline: "When a  high availability enabled node is started in auto-detect mode, it tries to communicate with the other node in the high availability pair. " +
							 "If a connection cannot be made within the discovery timeout period, the node starts in maintenance mode instead.",
					tagline2: "The heartbeat timeout is used to detect if the other node in a high availability pair has failed. " +
							  "If the primary node does not receive a heartbeat from the standby node, it continues to work as the primary node but the data synchronization process is stopped. " +
							  "If the standby node does not receive a heartbeat from the primary node, the standby node becomes the primary node.",
					discoveryTimeout: "Discovery Timeout:",
					discoveryTimeoutTooltip: "Number of seconds between 10 and 2,147,483,647 for the node to try to connect to the other node in the pair.",
					discoveryTimeoutValidation: "Enter a number between 10 and 2,147,483,647.",
					discoveryTimeoutError: "Discovery Timeout is not available",
					heartbeatTimeout: "Heartbeat Timeout:",
					heartbeatTimeoutTooltip: "Number of seconds between 1 and 2,147,483,647 for the node heartbeat.",
					heartbeatTimeoutValidation: "Enter a number between 1 and 2,147,483,647.",
					seconds: "Seconds"
				}
			},
			webUISecurity: {
				title: "Web UI Settings",
				tagline: "Configure settings for the Web UI.",
				certificate: {
					subtitle: "Web UI Certificate",
					tagline: "Configure the certificate used to secure communications with the Web UI."
				},
				timeout: {
					subtitle: "Session Timeout &amp; LTPA Token Expiration",
					tagline: "These settings control the user login sessions based on activity between the browser and server as well as total log in time. " +
							 "Changing the session inactivity timeout or the LTPA token expiration might invalidate any active login sessions.",
					inactivity: "Session Inactivity Timeout",
					inactivityTooltip: "The session inactivity timeout controls how long a user can remain logged in when there is no activity between the browser and the server. " +
							           "The inactivity timeout must be less than or equal to the LTPA token expiration.",
					expiration: "LTPA Token Expiration",
					expirationTooltip: "The Lightweight Third-Party Authentication token expiration controls how long a user can remain logged in for, regardless of the activity between the browser and the server. " +
							           "The expiration must be greater than or equal to the session inactivity timeout. Refer to the documentation for more information.", 
					unit: "minutes",
					save: "Save",
					saving: "Saving...",
					success: "Save successful. Please log out and log in again.",
					failed: "Save failed",
					invalid: {
						inactivity: "The session inactivity timeout must be a number between 1 and the LTPA token expiration value.",
						expiration: "The LTPA Token Expiration value must be a number between the session inactivity timeout value and 1440."
					},
					getExpirationError: "An error occurred while getting the LTPA token expiration.",
					getInactivityError: "An error occurred while getting the session inactivity timeout.",
					setExpirationError: "An error occurred while setting the LTPA token expiration.",
					setInactivityError: "An error occurred while setting the session inactivity timeout."
				},
				cacheauth: {
					subtitle: "Authentication Cache Timeout",
					tagline: "The authentication cache timeout specifies how long an authenticated credential in the cache is valid. " +
					"Larger values might increase the security risk because a revoked credential remains in the cache for a longer period. " +
					"Smaller values might affect performance because the user registry must be accessed more frequently.",
					cacheauth: "Authentication Cache Timeout",
					cacheauthTooltip: "The authentication cache timeout states the amount of time after which an entry in the cache will be removed. " +
							"The value should be between 1 and 3600 seconds (1 hour). The value should not be more than one third of the LTPA Token expiration value. " +
							"Refer to the documentation for further information.", 
					unit: "seconds",
					save: "Save",
					saving: "Saving...",
					success: "Save successful",
					failed: "Save failed",
					invalid: {
						cacheauth: "The authentication cache timeout value must be a number between 1 and 3600."
					},
					getCacheauthError: "An error occurred while getting the authentication cache timeout.",
					setCacheauthError: "An error occurred while setting the authentication cache timeout."
				},
				readTimeout: {
					subtitle: "HTTP Read Timeout",
					tagline: "The maximum amount of time to wait for a read request to complete on a socket after the first read occurs.",
					readTimeout: "HTTP Read Timeout",
					readTimeoutTooltip: "A value between 1 and 300 seconds (5 minutes).", 
					unit: "seconds",
					save: "Save",
					saving: "Saving...",
					success: "Save successful",
					failed: "Save failed",
					invalid: {
						readTimeout: "The HTTP read timeout value must be a number between 1 and 300."
					},
					getReadTimeoutError: "An error occurred while getting the HTTP read timeout.",
					setReadTimeoutError: "An error occurred while setting the HTTP read timeout."
				},
				port: {
					subtitle: "IP Address and Port",
					tagline: "Select the IP address and port for the Web UI communication. Changing either value might require you to log in at the new IP address and port.  " +
							 "<em>Selecting \"All\" IP addresses is not recommended because it might expose the Web UI to the Internet.</em>",
					ipAddress: "IP Address",
					all: "All",
					interfaceHint: "Interface: {0}",
					port: "Port",
					save: "Save",
					saving: "Saving...",
					success: "Save successful. It might take a minute for the change to take effect.  Please close your browser and log in again.",
					failed: "Save failed",
					tooltip: {
						port: "Enter an available port. Valid ports are between 1 and 8999, 9087, or 9100 and 65535, inclusive."
					},
					invalid: {
						interfaceDown: "The interface, {0}, reports it is down. Please check the interface or select a different address.",
						interfaceUnknownState: "An error occurred while querying the state of {0}. Please check the interface or select a different address.",
						port: "The port number must be a number between 1 and 8999, 9087, or 9100 and 65535, inclusive."
					},					
					getIPAddressError: "An error occurred while getting the IP address.",
					getPortError: "An error occurred while getting the port.",
					setIPAddressError: "An error occurred while setting the IP address.",
					setPortError: "An error occurred while setting the port."					
				},
				certificate: {
					subtitle: "Security Certificate",
					tagline: "Upload and replace the certificate used by the Web UI.",
					addButton: "Add Certificate",
					editButton: "Edit Certificate",
					addTitle: "Replace Web UI Certificate",
					editTitle: "Edit Web UI Certificate",
					certificateLabel: "Certificate:",
					keyLabel: "Private Key:",
					expiryLabel: "Certificate Expiry:",
					defaultExpiryDate: "Warning! The default certificate is in use and should be replaced",
					defaultCert: "Default",
					success: "It might take a minute for the change to take effect.  Please close your browser and log in again."
				},
				service: {
					subtitle: "Service",
					tagline: "Set trace levels to generate support information for the Web UI.",
					traceEnabled: "Enable trace",
					traceSpecificationLabel: "Trace Specification",
					traceSpecificationTooltip: "The trace specification string provided by IBM Support",
				    save: "Save",
				    getTraceError: "An error occurred while getting the trace specification.",
				    setTraceError: "An error occurred while setting the trace specification.",
				    clearTraceError: "An error occurred while clearing the trace specification.",
					saving: "Saving...",
					success: "Save successful.",
					failed: "Save failed"
				}
			}
		},
        platformDataRetrieveError: "An error occurred while retrieving platform data.",
        
        // Strings for client certs that can be used for certificate authentication
		clientCertsTitle: "Client Certificates",
		clientCertsButton: "Client Certificates",
		
        stopHover: "Shut down the server and prevent all access to the server.",
        restartLink: "Restart the server",
		restartMaintOnLink: "Start maintenance",
		restartMaintOnHover: "Restart the server and run it in maintenence mode.",
		restartMaintOffLink: "Stop maintenance",
		restartMaintOffHover: "Restart the server and run it in production mode.",
		restartCleanStoreLink: "Clean store",
		restartCleanStoreHover: "Clean the server store and restart the server.",
        restartDialogTitle: "Restart the ${IMA_SVR_COMPONENT_NAME}",
        restartMaintOnDialogTitle: "Restart the ${IMA_SVR_COMPONENT_NAME} in Maintenance Mode",
        restartMaintOffDialogTitle: "Restart the ${IMA_SVR_COMPONENT_NAME} in Production Mode",
        restartCleanStoreDialogTitle: "Clean the ${IMA_SVR_COMPONENT_NAME} store",
        restartDialogContent: "Are you sure that you want to restart the ${IMA_SVR_COMPONENT_NAME}?  Restarting the server disrupts all messaging.",
        restartMaintOnDialogContent: "Are you sure that you want to restart the ${IMA_SVR_COMPONENT_NAME} in maintenance mode?  Starting the server in maintenance mode stops all messaging.",
        restartMaintOffDialogContent: "Are you sure that you want to restart the ${IMA_SVR_COMPONENT_NAME} in production mode?  Starting the server in production mode restarts messaging.",
        restartCleanStoreDialogContent: "Execution of the <strong>clean store</strong> action will result in the loss of persisted messaging data.<br/><br/>" +
        	"Are you sure that you want the <strong>clean store</strong> action to execute?",
        restartOkButton: "Restart",        
        cleanOkButton: "Clean store and restart",
        restarting: "Restarting the ${IMA_SVR_COMPONENT_NAME}...",
        restartingFailed: "An error occurred while the ${IMA_SVR_COMPONENT_NAME} was being restarted.",
        remoteLogLevelTagLine: "Set the level of messages that you want to see in the server logs.  ", 
        errorRetrievingTrustedCertificates: "An error occurred while retrieving trusted certificates.",

        // New page title and tagline:  Admin Endpoint
        adminEndpointPageTitle: "Admin Endpoint Configuration",
        adminEndpointPageTagline: "Configure the admin endpoint and create configuration policies. " +
        		"The Web UI connects to the admin endpoint to perform administrative and monitoring tasks. " +
        		"Configuration policies control which administrative and monitoring tasks a user is permitted to perform.",
        
        // HA config strings
        haReplicationPortLabel: "Replication Port:",
		haExternalReplicationInterfaceLabel: "External Replication Address:",
		haExternalReplicationPortLabel: "External Replication Port:",
        haRemoteDiscoveryPortLabel: "Remote Discovery Port:",
        haReplicationPortTooltip: "The port number used for HA replication on the local node in the HA Pair.",
		haReplicationPortInvalid: "The replication port is invalid." +
				"The valid range is 1024 - 65535. The default is 0 and allows an available port to be randomly selected at connection time.",
		haExternalReplicationInterfaceTooltip: "The IPv4 or IPv6 address of the NIC that should be used for HA replication.",
		haExternalReplicationPortTooltip: "The external port number used for HA replication on the local node in the HA Pair.",
		haExternalReplicationPortInvalid: "The external replication port is invalid." +
				"The valid range is 1024 - 65535. The default is 0 and allows an available port to be randomly selected at connection time.",
        haRemoteDiscoveryPortTooltip: "The port number used for HA discovery on the remote node in the HA Pair.",
		haRemoteDiscoveryPortInvalid: "The remote discovery port is invalid." +
				 "The valid range is 1024 - 65535. The default is 0 and allows an available port to be randomly selected at connection time.",
		haReplicationAndDiscoveryBold: "Replication and Discovery Addresses ",
		haReplicationAndDiscoveryNote: "(if one is set, all must be set)",
		haUseSecuredConnectionsLabel: "Use Secured Connections:",
		haUseSecuredConnectionsTooltip: "Use TLS for discovery and data channels between nodes of an HA Pair.",
		haUseSecuredConnectionsEnabled: "The use of secure connections is enabled",
		haUseSecuredConnectionsDisabled: "The use of secure connections is disabled",
		
		mqConnectivityEnableLabel: "Enable MQ Connectivity",
		savingProgress: "Saving...",
	    setMqConnEnabledError: "An error occurred while setting the MQConnectivity enabled state.",
	    
	    allowCertfileOverwriteLabel: "Overwrite",
		allowCertfileOverwriteTooltip: "Allow the certificate and the private key to be overwritten.",
		
		stopServerWarningTitle: "Stopping prevents restart from the Web UI!",
		stopServerWarning: "Stopping the server prevents all messaging and all administrative access via REST requests and the Web UI. <b>To avoid losing access to the server from the Web UI, use the Restart the server link.</b>",
		
		// New Web UI user dialog strings to allow French translation to insert a space before colon
		webuiuserLabel: "User ID:",
		webuiuserdescLabel: "Description:",
		webuiuserroleLabel: "Role:"

	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
