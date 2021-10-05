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
		// Messaging
		// ------------------------------------------------------------------------
		messaging : {
			title : "Messaging",
			users: {
				title: "User Authentication",
				tagline: "Configure LDAP to authenticate messaging server users."
			},
			endpoints: {
				title: "Endpoint Configuration",
				listenersSubTitle: "Endpoints",
				endpointsSubTitle: "Define an Endpoint",
		 		form: {
					enabled: "Enabled",
					name: "Name",
					description: "Description",
					port: "Port",
					ipAddr: "IP Address",
					all: "All",
					protocol: "Protocol",
					security: "Security",
					none: "None",
					securityProfile: "Security Profile",
					connectionPolicies: "Connection Policies",
					connectionPolicy: "Connection Policy",
					messagingPolicies: "Messaging Policies",
					messagingPolicy: "Messaging Policy",
					destinationType: "Destination Type",
					destination: "Destination",
					maxMessageSize: "Max Message Size",
					selectProtocol: "Select Protocol",
					add: "Add",
					tooltip: {
						description: "",
						port: "Enter an available port. Valid ports are between 1 and 65535, inclusive.",
						connectionPolicies: "Add at least one connection policy. A connection policy authorizes clients to connect to the endpoint. " +
								"Connection Policies are evaluated in the order shown. To change the order, use the up and down arrows on the toolbar.",
						messagingPolicies: "Add at least one messaging policy. " +
								"A messaging policy authorizes connected clients to perform specific messaging actions, such as which topics or queues the client can access on ${IMA_PRODUCTNAME_FULL}. " +
								"Messaging Policies are evaluated in the order shown. To change the order, use the up and down arrows on the toolbar.",									
						maxMessageSize: "Maximum message size, in KB. Valid values are between 1 and 262,144 inclusive.",
						protocol: "Specify valid protocols for this endpoint."
					},
					invalid: {						
						port: "The port number must be a number between 1 and 65535, inclusive.",
						maxMessageSize: "The maximum message size must be a number between 1 and 262,144 inclusive.",
						ipAddr: "A valid IP address is required.",
						security: "A value is required."
					},
					duplicateName: "A record with that name already exists."
				},
				addDialog: {
					title: "Add Endpoint",
					instruction: "An endpoint is a port that client applications can connect to.  An endpoint must have at least one connection policy and one messaging policy."
				},
				removeDialog: {
					title: "Delete Endpoint",
					content: "Are you sure that you want to delete this record?"
				},
				editDialog: {
					title: "Edit Endpoint",
					instruction: "An endpoint is a port that client applications can connect to.  An endpoint must have at least one connection policy and one messaging policy."
				},
				addProtocolsDialog: {
					title: "Add Protocols to the Endpoint",
					instruction: "Add protocols that are allowed to connect to this endpoint. At least one protocol must be selected.",
					allProtocolsCheckbox: "All protocols are valid for this endpoint.",
					protocolSelectErrorTitle: "No protocol selected.",
					protocolSelectErrorMsg: "At least one protocol must be selected. Either specify that all protocols be added or select specific protocls from the protocol list."
				},
				addConnectionPoliciesDialog: {
					title: "Add Connection Policies to the Endpoint",
					instruction: "A connection policy authorizes clients to connect to ${IMA_PRODUCTNAME_FULL} endpoints. " +
						"Each endpoint must have at least one connection policy."
				},
				addMessagingPoliciesDialog: {
					title: "Add Messaging Policies to the Endpoint",
					instruction: "A messaging policy allows you to control what topics, queues, or global-shared subscriptions a client can access on ${IMA_PRODUCTNAME_FULL}. " +
							"Each endpoint must have at least one messaging policy. " +
							"If an endpoint has a messaging policy for a global-shared subscription, it must also have a messaging policy for the subscribed topics."
				},
                retrieveEndpointError: "An error occurred while retrieving the Endpoints.",
                saveEndpointError: "An error occurred while saving the endpoint.",
                deleteEndpointError: "An error occurred while deleting the endpoint.",
                retrieveSecurityProfilesError: "An error occurred while retrieving the Security Policies."
			},
			connectionPolicies: {
				title: "Connection Policies",
				grid: {
					applied: "Applied",
					name: "Name"
				},
		 		dialog: {
					name: "Name",
					protocol: "Protocol",
					description: "Description",
					clientIP: "Client IP Address",  
					clientID: "Client ID",
					ID: "User ID",
					Group: "Group ID",
					selectProtocol: "Select Protocol",
					commonName: "Certificate Common Name",
					protocol: "Protocol",
					tooltip: {
						name: "",
						filter: "You must specify at least one of the filter fields. " +
								"A single asterisk (*) may be used as the last character in most filters to match 0 or more characters. " +
								"The Client IP Address may contain an asterisk(*) or a comma-delimited list of IPv4 or IPv6 addresses or ranges, for example: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"IPv6 addresses must be enclosed in square brackets, for example: [2001:db8::]."
					},
					invalid: {						
						filter: "You must specify at least one of the filter fields.",
						wildcard: "A single asterisk (*) may be used at the end of the value to match 0 or more characters.",
						vars: "Must not contain the substitution variable ${UserID}, ${GroupID}, ${ClientID}, or ${CommonName}.",
						clientIDvars: "Must not contain the substitution variable ${GroupID} or ${ClientID}.",
						clientIP: "The Client IP Address must contain an asterisk(*) or a comma-delimited list of IPv4 or IPv6 addresses or ranges for example: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"IPv6 addresses must be enclosed in square brackets, for example: [2001:db8::]."
					}										
				},
				addDialog: {
					title: "Add Connection Policy",
					instruction: "A connection policy authorizes clients to connect to ${IMA_PRODUCTNAME_FULL} endpoints. Each endpoint must have at least one connection policy."
				},
				removeDialog: {
					title: "Delete Connection Policy",
					instruction: "A connection policy authorizes clients to connect to ${IMA_PRODUCTNAME_FULL} endpoints. Each endpoint must have at least one connection policy.",
					content: "Are you sure that you want to delete this record?"
				},
                removeNotAllowedDialog: {
                	title: "Remove not Allowed",
                	content: "The connection policy cannot be removed because it is being used by one or more endpoints. " +
                			 "Remove the connection policy from all endpoints, then try again.",
                	closeButton: "Close"
                },								
				editDialog: {
					title: "Edit Connection Policy",
					instruction: "A connection policy authorizes clients to connect to ${IMA_PRODUCTNAME_FULL} endpoints. Each endpoint must have at least one connection policy."
				},
                retrieveConnectionPolicyError: "An error occurred while retrieving the connection policies.",
                saveConnectionPolicyError: "An error occurred while saving the connection policy.",
                deleteConnectionPolicyError: "An error occurred while deleting the connection policy."
 			},
			messagingPolicies: {
				title: "Messaging Policies",
				listSeparator : ",",
		 		dialog: {
					name: "Name",
					description: "Description",
					destinationType: "Destination Type",
					destinationTypeOptions: {
						topic: "Topic",
						subscription: "Global-shared Subscription",
						queue: "Queue"
					},
					topic: "Topic",
					queue: "Queue",
					selectProtocol: "Select Protocol",
					destination: "Destination",
					maxMessages: "Max Messages",
					maxMessagesBehavior: "Max Messages Behavior",
					maxMessagesBehaviorOptions: {
						RejectNewMessages: "Reject new messages",
						DiscardOldMessages: "Discard old messages"
					},
					action: "Authority",
					actionOptions: {
						publish: "Publish",
						subscribe: "Subscribe",
						send: "Send",
						browse: "Browse",
						receive: "Receive",
						control: "Control"
					},
					clientIP: "Client IP Address",  
					clientID: "Client ID",
					ID: "User ID",
					Group: "Group ID",
					commonName: "Certificate Common Name",
					protocol: "Protocol",
					disconnectedClientNotification: "Disconnected Client Notification",
					subscriberSettings: "Subscriber Settings",
					publisherSettings: "Publisher Settings",
					senderSettings: "Sender Settings",
					maxMessageTimeToLive: "Max Message Time To Live",
					unlimited: "unlimited",
					unit: "seconds",
					// TRNOTE {0} is formatted an integer number.
					maxMessageTimeToLiveValueString: "{0} seconds",
					tooltip: {
						name: "",
						destination: "Message topic, queue or global-shared subscription this policy applies to. Use asterisk (*) with caution. " +
							"An asterisk matches 0 or more characters, including slash (/). Therefore, it can match multiple levels in a topic tree.",
						maxMessages: "The maximum number of messages to keep for a subscription. Must be a number between 1 and 20,000,000",
						maxMessagesBehavior: "The behavior applied when the buffer for a subscription is full. " +
								"That is, when the number of messages in the buffer for a subscription reaches the Max Messages value.<br />" +
								"<strong>Reject new messages:&nbsp;</strong>  While the buffer is full, new messages are rejected.<br />" +
								"<strong>Discard old messages:&nbsp;</strong>  When the buffer is full and a new message arrives, the oldest un-delivered messages are discarded.",
						discardOldestMessages: "This setting can result in some messages not being delivered to subscribers, even if the publisher receives acknowledgement of the delivery. " +
								"The messages are discarded even if both the publisher and the subscriber requested reliable messaging.",
						action: "Actions permitted by this policy",
						filter: "You must specify at least one of the filter fields. " +
								"A single asterisk (*) may be used as the last character in most filters to match 0 or more characters. " +
								"The Client IP Address may contain an asterisk(*) or a comma-delimited list of IPv4 or IPv6 addresses or ranges, for example: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								"IPv6 addresses must be enclosed in square brackets, for example: [2001:db8::].",
				        disconnectedClientNotification: "Specifies whether notification messages are published for disconnected MQTT clients on arrival of a message. " +
				        		"Notifications are only enabled for MQTT CleanSession=0 clients.",
				        protocol: "Enable filtering of protocols for the messaging policy. If enabled one or more protocols must be specified. If not enabled all protocols will be allowed for the messaging policy.",
				        destinationType: "The type of destination to allow access to. " +
				        		"To allow access to a global-shared subscription, two messaging policies are required:"+
				        		"<ul><li>A messaging policy with destination type <em>topic</em>, which allows access to one or more topics.</li>" +
				        		"<li>A messaging policy with destination type <em>global-shared subscription</em>, which allows access to the global-shared durable subscription on those topics.</li></ul>",
						action: {
							topic: "<dl><dt>Publish:</dt><dd>Allows clients to publish messages to the topic that is specified in the messaging policy.</dd>" +
							       "<dt>Subscribe:</dt><dd>Allows clients to subscribe to the topic that is specified in the messaging policy.</dd></dl>",
							queue: "<dl><dt>Send:</dt><dd>Allows clients to send messages to the queue that is specified in the messaging policy.</dd>" +
								    "<dt>Browse:</dt><dd>Allows clients to browse the queue that is specified in the messaging policy.</dd>" +
								    "<dt>Receive:</dt><dd>Allows clients to receive messages from the queue that is specified in the messaging policy.</dd></dl>",
							subscription:  "<dl><dt>Receive:</dt><dd>Allows clients to receive messages from the global-shared subscription that is specified in the messaging policy.</dd>" +
									"<dt>Control:</dt><dd>Allows clients to create and delete the global-shared subscription that is specified in the messaging policy.</dd></dl>"
							},
						maxMessageTimeToLive: "The maximum number of seconds a message published under the policy can live for. " +
								"If the publisher specifies a smaller expiration value, the publisher value is used. " +
								"Valid values are <em>unlimited</em> or 1 - 2,147,483,647 seconds. The value <em>unlimited</em> sets no maximum.",
						maxMessageTimeToLiveSender: "The maximum number of seconds a message sent under the policy can live for. " +
								"If the sender specifies a smaller expiration value, the sender value is used. " +
								"Valid values are <em>unlimited</em> or 1 - 2,147,483,647 seconds. The value <em>unlimited</em> sets no maximum."
								
								
					},
					invalid: {						
						maxMessages: "Must be a number between 1 and 20,000,000.",                       
						filter: "You must specify at least one of the filter fields.",
						wildcard: "A single asterisk (*) may be used at the end of the value to match 0 or more characters.",
						vars: "Must not contain the substitution variable ${UserID}, ${GroupID}, ${ClientID}, or ${CommonName}.",
						clientIP: "The Client IP Address must contain an asterisk(*) or a comma-delimited list of IPv4 or IPv6 addresses or ranges, for example: 9.3.41.32,9.41.0.0,10.10.1.1-10.10.1.100. " +
								  "IPv6 addresses must be enclosed in square brackets, for example: [2001:db8::].",
					    subDestination: "Client ID variable substitution is not allowed when the destination type is <em>global-shared subscription</em>.",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    protocol: "The protocol(s) {0} are not valid for the destination type {1}.",
					    // TRNOTE {0} is one or more protocols, such as JMS,MQTT. {1} is one of Topic, Queue, or Global-shared Subscription.
					    destinationType: "The protocol(s) {0} that are specified for the protocol filter are not valid for the destination type {1}.",
					    maxMessageTimeToLive: "Must be <em>unlimited</em> or a number between 1 and 2,147,483,647."
					}					
				},
				addDialog: {
					title: "Add Messaging Policy",
					instruction: "A messaging policy authorizes connected clients to perform specific messaging actions, such as which topics, queues, or global-shared subscriptions the client can access on ${IMA_PRODUCTNAME_FULL}. " +
							     "In a global-shared subscription, the work of receiving messages from a durable topic subscription is shared between multiple subscribers. Each endpoint must have at least one messaging policy."
				},
				removeDialog: {
					title: "Delete Messaging Policy",
					instruction: "A messaging policy authorizes connected clients to perform specific messaging actions, such as which topics, queues, or global-shared subscriptions the client can access on ${IMA_PRODUCTNAME_FULL}. " +
							"In a global-shared subscription, the work of receiving messages from a durable topic subscription is shared between multiple subscribers. Each endpoint must have at least one messaging policy.",
					content: "Are you sure that you want to delete this record?"
				},
                removeNotAllowedDialog: {
                	title: "Remove not Allowed",
                	// TRNOTE: {0} is the messaging policy type where the value can be topic, queue or subscription.
                	content: "The {0} policy cannot be removed because it is being used by one or more endpoints. " +
                			 "Remove the {0} policy from all endpoints, then try again.",
                	closeButton: "Close"
                },				
				editDialog: {
					title: "Edit Messaging Policy",
					instruction: "A messaging policy authorizes connected clients to perform specific messaging actions, such as which topics, queues, or global-shared subscriptions the client can access on ${IMA_PRODUCTNAME_FULL}. " +
							     "In a global-shared subscription, the work of receiving messages from a durable topic subscription is shared between multiple subscribers. Each endpoint must have at least  one messaging policy. "
				},
				viewDialog: {
					title: "View Messaging Policy"
				},		
				confirmSaveDialog: {
					title: "Save Messaging Policy",
					// TRNOTE: {0} is a numeric count, {1} is a table of properties and values.
					content: "This policy is in use by approximately {0} subscribers or producers. " +
							"Clients authorized by this policy will use the following new settings: {1}" +
							"Are you sure you want to change this policy?",
					// TRNOTE: {1} is a table of properties and values.							
					contentUnknown: "This policy might be in use by subscribers or producers. " +
							"Clients authorized by this policy will use the following new settings: {1}" +
							"Are you sure you want to change this policy?",
					saveButton: "Save",
					closeButton: "Cancel"
				},
                retrieveMessagingPolicyError: "An error occurred while retrieving the messaging policies.",
                retrieveOneMessagingPolicyError: "An error occurred while retrieving the messaging policy.",
                saveMessagingPolicyError: "An error occurred while saving the messaging policy.",
                deleteMessagingPolicyError: "An error occurred while deleting the messaging policy.",
                pendingDeletionMessage:  "This policy is pending deletion.  It will be deleted when it is no longer in use.",
                tooltip: {
                	discardOldestMessages: "Max Messages Behavior is set to <em>Discard Old Messages</em>. " +
                			"This setting can result in some messages not being delivered to subscribers, even if the publisher receives acknowledgement of the delivery. " +
                			"The messages are discarded even if both the publisher and the subscriber requested reliable messaging."
                }
			},
			messageProtocols: {
				title: "Messaging Protocols",
				subtitle: "Messaging protocols are used to send messages between clients and ${IMA_PRODUCTNAME_FULL}.",
				tagline: "Available messaging protocols and their capabilities.",
				messageProtocolsList: {
					name: "Protocol Name",
					topic: "Topic",
					shared: "Global-shared Subscription",
					queue: "Queue",
					browse: "Browse"
				}
			},
			messageHubs: {
				title: "Message Hubs",
				subtitle: "System Administrators and Messaging Administrators can define, edit, or delete message hubs. " +
						  "A message hub is an organizational configuration object that groups related endpoints, connection policies, and messaging policies. <br /><br />" +
						  "An endpoint is a port that client applications can connect to. An endpoint must have at least one connection policy and one messaging policy. " +
						  "The connection policy authorizes clients to connect to the endpoint, while the messaging policy authorizes clients for specific messaging actions once connected to the endpoint. ",
				tagline: "Define, edit, or delete a message hub.",
				defineAMessageHub: "Define a Message Hub",
				editAMessageHub: "Edit a Message Hub",
				defineAnEndpoint: "Define an Endpoint",
				endpointTabTagline: "An endpoint is a port that client applications can connect to.  " +
						"An endpoint must have at least one connection policy and one messaging policy.",
				messagingPolicyTabTagline: "A messaging policy allows you to control what topics, queues, or global-shared subscriptions a client can access on ${IMA_PRODUCTNAME_FULL}. " +
						"Each endpoint must have at least one messaging policy.",
				connectionPolicyTabTagline: "A connection policy authorizes clients to connect to ${IMA_PRODUCTNAME_FULL} endpoints. " +
						"Each endpoint must have at least one connection policy.",						
                retrieveMessageHubsError: "An error occurred while retrieving the message hubs.",
                saveMessageHubError: "An error occurred while saving the message hub.",
                deleteMessageHubError: "An error occurred while deleting the message hub.",
                messageHubNotFoundError: "The message hub cannot be found. It might have been deleted by another user.",
                removeNotAllowedDialog: {
                	title: "Remove not Allowed",
                	content: "The message hub cannot be removed because it contains one or more endpoints. " +
                			 "Edit the message hub to delete the endpoints, then try again.",
                	closeButton: "Close"
                },
                addDialog: {
                	title: "Add Message Hub",
                	instruction: "Define a message hub to manage server connections.",
                	saveButton: "Save",
                	cancelButton: "Cancel",
                	name: "Name:",
                	description: "Description:"
                },
                editDialog: {
                	title: "Edit Message Hub Properties",
                	instruction: "Edit the name and description of the message hub."
                },                
				messageHubList: {
					name: "Message Hub",
					description: "Description",
					metricLabel: "Endpoints",
					removeDialog: {
						title: "Delete Message Hub",
						content: "Are you sure that you want to delete this message hub?"
					}
				},
				endpointList: {
					name: "Endpoint",
					description: "Description",
					connectionPolicies: "Connection Policies",
					messagingPolicies: "Messaging Policies",
					port: "Port",
					enabled: "Enabled",
					status: "Status",
					up: "Up",
					down: "Down",
					removeDialog: {
						title: "Delete Endpoint",
						content: "Are you sure that you want to delete this endpoint?"
					}
				},
				messagingPolicyList: {
					name: "Messaging Policy",
					description: "Description",					
					metricLabel: "Endpoints",
					destinationLabel: "Destination",
					maxMessagesLabel: "Max Messages",
					disconnectedClientNotificationLabel: "Disconnected Client Notification",
					actionsLabel: "Authority",
					useCountLabel: "Use Count",
					unknown: "Unknown",
					removeDialog: {
						title: "Delete Messaging Policy",
						content: "Are you sure that you want to delete this messaging policy?"
					},
					deletePendingDialog: {
						title: "Delete Pending",
						content: "The delete request was received, but the policy cannot be deleted at this time. " +
							"The policy is in use by approximately {0} subscribers or producers. " +
							"The policy will be deleted when it is no longer in use.",
						contentUnknown: "The delete request was received, but the policy cannot be deleted at this time. " +
						"The policy might be in use by subscribers or producers. " +
						"The policy will be deleted when it is no longer in use.",
						closeButton: "Close"
					},
					deletePendingTooltip: "This policy will be deleted when it is no longer in use."
				},	
				connectionPolicyList: {
					name: "Connection Policy",
					description: "Description",					
					endpoints: "Endpoints",
					removeDialog: {
						title: "Delete Connection Policy",
						content: "Are you sure that you want to delete this connection policy?"
					}
				},				
				messageHubDetails: {
					backLinkText: "Return to Message Hubs",
					editLink: "Edit",
					endpointsTitle: "Endpoints",
					endpointsTip: "Configure endpoints and connection policies for this message hub",
					messagingPoliciesTitle: "Messaging Policies",
					messagingPoliciesTip: "Configure messaging policies for this message hub",
					connectionPoliciesTitle: "Connection Policies",
					connectionPoliciesTip: "Configure connection policies for this message hub"
				},
				hovercard: {
					name: "Name",
					description: "Description",
					endpoints: "Endpoints",
					connectionPolicies: "Connection Policies",
					messagingPolicies: "Messaging Policies",
					warning: "Warning"
				}
			},
			referencedObjectGrid: {
				applied: "Applied",
				name: "Name"
			},
			savingProgress: "Saving...",
			savingFailed: "Save failed.",
			deletingProgress: "Deleting...",
			deletingFailed: "Delete failed.",
			refreshingGridMessage: "Updating...",
			noItemsGridMessage: "No items to display",
			testing: "Testing...",
			testTimedOut: "The test connection is taking too long to complete.",
			testConnectionFailed: "Test connection failed",
			testConnectionSuccess: "Test connection succeeded",
			dialog: {
				saveButton: "Save",
				deleteButton: "Delete",
				deleteContent: "Are you sure you want to delete this record?",
				cancelButton: "Cancel",
				closeButton: "Close",
				testButton: "Test connection",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingContent: "Testing connection to {0}...",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingFailedContent: "Testing connection to {0} failed.",
				// TRNOTE {0} is a remote resource, such as the name of a configuration object or a URL to an LDAP repository
				testingSuccessContent: "Testing connection to {0} succeeded."
			},
			updating: "Updating...",
			invalidName: "A record with that name already exists.",
			filterHeadingConnection: "To restrict connections using this policy to specific clients, specify one or more of the following filters. " +
					"For example, select <em>Group ID</em> to restrict this policy to members of a particular group. " +
					"The policy allows access only when all of the specified filters are true:",
			filterHeadingMessaging: "To restrict the messaging actions that are defined in this policy to specific clients, specify one or more of the following filters. " +
					"For example, select <em>Group ID</em> to restrict this policy to members of a particular group. " +
					"The policy allows access only when all of the specified filters are true:",
			settingsHeadingMessaging: "Specify the resources and messaging actions that the client is permitted to access:",
			mqConnectivity: {
				title: "MQ Connectivity",
				subtitle: "Configure connections to one or more WebSphere MQ queue managers."				
			},
			connectionProperties: {
				title: "Queue Manager Connection Properties",
				tagline: "Define, edit, or delete information about how the server connects to the queue managers.",
				retrieveError: "An error occurred retrieving queue manager connection properties.",
				grid: {
					name: "Name",
					qmgr: "Queue Manager",
					connName: "Connection Name",
					channel: "Channel Name",
					description: "Description",
					SSLCipherSpec: "SSL CipherSpec",
					status: "Status"
				},
				dialog: {
					instruction: "Connectivity with MQ requires the following queue manager connection details.",
					nameInvalid: "The name cannot have leading or trailing spaces",
					connTooltip: "Enter a comma separated list of connection names in the form of IP address or host name, and port number, for example 224.0.138.177(1414)",
					connInvalid: "The connection name contains invalid characters",
					qmInvalid: "The queue manager name must consist of only letters and numbers and the four special characters . _ % and /",
					qmTooltip: "Enter a queue manager name consisting of only letters, numbers, and the four special characters . _ % and /",
				    channelTooltip: "Enter a channel name consisting of only letters, numbers, and the four special characters . _ % and /",
					channelInvalid: "The channel name must consist of only letters and numbers and the four special characters . _ % and /",
					activeRuleTitle: "Edit not allowed",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailOne: "The queue manager connection cannot be edited because it is being used by enabled destination mapping rule {0}.",
					// TRNOTE {0} is a user provided name of a rule.
					activeRuleDetailMany: "The queue manager connection cannot be edited because it is being used by the following enabled destination mapping rules: {0}.",
					SSLCipherSpecTooltip: "Enter a cipher specification for an SSL connection, with a maximum length of 32 characters"
				},
				addDialog: {
					title: "Add Queue Manager Connection"
				},
				removeDialog: {
					title: "Delete Queue Manager Connection",
					content: "Are you sure that you want to delete this record?"
				},
				editDialog: {
					title: "Edit Queue Manager Connection"
				},
                removeNotAllowedDialog: {
                	title: "Remove not Allowed",
					// TRNOTE {0} is a user provided name of a rule.
                	contentOne: "The queue manager connection cannot be removed because it is being used by destination mapping rule {0}.",
					// TRNOTE {0} is a list of user provided names of rules.
                	contentMany: "The queue manager connection cannot be removed because it is being used by the following destination mapping rules: {0}.",
                	closeButton: "Close"
                }
			},
			destinationMapping: {
				title: "Destination Mapping Rules",
				tagline: "System Administrators and Messaging Administrators can define, edit, or delete rules that govern which messages are forwarded to and from queue managers.",
				note: "Rules must be disabled before they can be deleted.",
				retrieveError: "An error occurred retrieving destination mapping rules.",
				disableRulesConfirmationDialog: {
					text: "Are you sure that you want to disable this rule?",
					info: "Disabling the rule stops the rule, which results in the loss of buffered messages and any messages that are currently being sent.",
					buttonLabel: "Disable Rule"
				},
				leadingBlankConfirmationDialog: {
					textSource: "The <em>source</em> has a leading blank. Are you sure that you want to save this rule with a leading blank?",
					textDestination: "The <em>destination</em> has a leading blank. Are you sure that you want to save this rule with a leading blank?",
					textBoth: "The <em>source</em> and <em>destination</em> have leading blanks. Are you sure that you want to save this rule with leading blanks?",
					info: "Topics are permitted to have leading blanks, but generally do not.  Check the topic string to ensure it is correct.",
					buttonLabel: "Save Rule"
				},
				grid: {
					name: "Name",
					type: "Rule Type",
					source: "Source",
					destination: "Destination",
					none: "None",
					all: "All",
					enabled: "Enabled",
					associations: "Associations",
					maxMessages: "Max Messages",
					retainedMessages: "Retained Messages"
				},
				state: {
					enabled: "Enabled",
					disabled: "Disabled",
					pending: "Pending"
				},
				ruleTypes: {
					type1: "${IMA_PRODUCTNAME_SHORT} topic to MQ queue",
					type2: "${IMA_PRODUCTNAME_SHORT} topic to MQ topic",
					type3: "MQ queue to ${IMA_PRODUCTNAME_SHORT} topic",
					type4: "MQ topic to ${IMA_PRODUCTNAME_SHORT} topic",
					type5: "${IMA_PRODUCTNAME_SHORT} topic subtree to MQ queue",
					type6: "${IMA_PRODUCTNAME_SHORT} topic subtree to MQ topic",
					type7: "${IMA_PRODUCTNAME_SHORT} topic subtree to MQ topic subtree",
					type8: "MQ topic subtree to ${IMA_PRODUCTNAME_SHORT} topic",
					type9: "MQ topic subtree to ${IMA_PRODUCTNAME_SHORT} topic subtree",
					type10: "${IMA_PRODUCTNAME_SHORT} queue to MQ queue",
					type11: "${IMA_PRODUCTNAME_SHORT} queue to MQ topic",
					type12: "MQ queue to ${IMA_PRODUCTNAME_SHORT} queue",
					type13: "MQ topic to ${IMA_PRODUCTNAME_SHORT} queue",
					type14: "MQ topic subtree to ${IMA_PRODUCTNAME_SHORT} queue"
				},
				sourceTooltips: {
					type1: "Enter the ${IMA_PRODUCTNAME_SHORT} topic to map from",
					type2: "Enter the ${IMA_PRODUCTNAME_SHORT} topic to map from",
					type3: "Enter the MQ queue to map from. The value can contain characters in the ranges a-z, A-Z, and 0-9, and any of the following characters: % . /  _",
					type4: "Enter the MQ topic to map from",
					type5: "Enter the ${IMA_PRODUCTNAME_SHORT} topic subtree to map from, for example MessageGatewayRoot/Level2",
					type6: "Enter the ${IMA_PRODUCTNAME_SHORT} topic subtree to map from, for example MessageGatewayRoot/Level2",
					type7: "Enter the ${IMA_PRODUCTNAME_SHORT} topic subtree to map from, for example MessageGatewayRoot/Level2",
					type8: "Enter the MQ topic subtree to map from, for example MQRoot/Layer1",
					type9: "Enter the MQ topic subtree to map from, for example MQRoot/Layer1",
					type10: "Enter the ${IMA_PRODUCTNAME_SHORT} queue to map from. " +
							"The value must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. " +
							"The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type11: "Enter the ${IMA_PRODUCTNAME_SHORT} queue to map from. " +
							"The value must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. " +
							"The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type12: "Enter the MQ queue to map from. The value can contain characters in the ranges a-z, A-Z, and 0-9, and any of the following characters: % . /  _",
					type13: "Enter the MQ topic to map from",
					type14: "Enter the MQ topic subtree to map from, for example MQRoot/Layer1"
				},
				targetTooltips: {
					type1: "Enter the MQ queue to map to. The value can contain characters in the ranges a-z, A-Z, and 0-9, and any of the following characters: % . /  _",
					type2: "Enter the MQ topic to map to",
					type3: "Enter the ${IMA_PRODUCTNAME_SHORT} topic to map to",
					type4: "Enter the ${IMA_PRODUCTNAME_SHORT} topic to map to",
					type5: "Enter the MQ queue to map to. The value can contain characters in the ranges a-z, A-Z, and 0-9, and any of the following characters: % . /  _",
					type6: "Enter the MQ topic to map to",
					type7: "Enter the MQ topic subtree to map to, for example MQRoot/Layer1",
					type8: "Enter the ${IMA_PRODUCTNAME_SHORT} topic to map to",
					type9: "Enter the ${IMA_PRODUCTNAME_SHORT} topic subtree to map to, for example MessageGatewayRoot/Level2",
					type10: "Enter the MQ queue to map to. The value can contain characters in the ranges a-z, A-Z, and 0-9, and any of the following characters: % . /  _",
					type11: "Enter the MQ topic to map to",
					type12: "Enter the ${IMA_PRODUCTNAME_SHORT} queue to map to. " +
							"The value must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. " +
							"The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type13: "Enter the ${IMA_PRODUCTNAME_SHORT} queue to map to. " +
							"The value must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. " +
							"The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @",
					type14: "Enter the ${IMA_PRODUCTNAME_SHORT} queue to map to. " +
							"The value must not have leading or trailing spaces and cannot contain control characters, commas, double quotation marks, backslashes or equal signs. " +
							"The first character must not be a number, quotation mark, or any of the following special characters: ! # $ % &amp; ( ) * + , - . / : ; < = > ? @"
				},
				associated: "Associated",
				associatedQMs: "Associated queue manager connections:",
				associatedMessages: {
					errorMissing: "A queue manager connection must be selected.",
					errorRetained: "The retained messages setting allows a single queue manager connection association. " +
							"Change retained messages to <em>None</em> or remove some associations."
				},
				ruleTypeMessage: "The retained messages setting allows rule types with topic or topic subtree destinations. " +
							"Change retained messages to <em>None</em> or select a different rule type.",
				status: {
					active: "Active",
					starting: "Starting",
					inactive: "Inactive"
				},
				dialog: {
					instruction: "Destination mapping rules define the direction in which messages are moved, and the nature of the source and target objects",
					nameInvalid: "The name cannot have leading or trailing spaces",
					noQmgrsTitle: "No Queue Manager Connections",
					noQmrsDetail: "A destination mapping rule cannot be defined without first defining a queue manager connection.",
					maxMessagesTooltip: "Specify the maximum number of messages that can be buffered on the destination.",
					maxMessagesInvalid: "The maximum messages must be a number between 1 and 20,000,000 inclusive.",
					retainedMessages: {
						label: "Retained Messages",
						none: "None",
						all: "All",
						tooltip: {
							basic: "Specify which messages are forwarded to a topic as retained messages.",
							disabled4Type: "Retained messages must be <em>None</em> when the destination is a queue.",
							disabled4Associations: "Retained messages must be <em>None</em> when multiple queue manager connections are selected.",
							disabled4Both: "Retained messages must be <em>None</em> when the destination is a queue, or when multiple queue manager connections are selected."
						}
					}
				},
				addDialog: {
					title: "Add Destination Mapping Rule"
				},
				removeDialog: {
					title: "Delete Destination Mapping Rule",
					content: "Are you sure that you want to delete this record?"
				},
                removeNotAllowedDialog: {
                	title: "Remove not Allowed",
                	content: "The destination mapping rule cannot be removed because it is enabled.  " +
                			 "Disable the rule using the 'Other Actions' menu, then try again.",
                	closeButton: "Close"
                },
				editDialog: {
					title: "Edit Destination Mapping Rule",
					restrictedEditingTitle: "Editing is limited while the rule is enabled",
					restrictedEditingContent: "You can edit limited properties while the rule is enabled. " +
							"To edit additional properties, disable the rule, edit the properties, and save your changes. " +
							"Disabling the rule stops the rule, which results in the loss of buffered messages and any messages that are currently being sent. " +
							"The rule remains disabled until you enable it again."
				},
				action: {
					Enable: "Enable Rule",
					Disable: "Disable Rule",
					Refresh: "Refresh Status"
				}
			},
			sslkeyrepository: {
				title: "SSL Key Repository",
				tagline: "System Administrators and Messaging Administrators can upload and download an SSL key repository and password file. " +
						 "The SSL key repository and password file are used to secure the connection between the server and the queue manager.",
                uploadButton: "Upload",
                downloadLink: "Download",
                deleteButton: "Delete",
                lastUpdatedLabel: "SSL Key Repository Files Last Updated:",
                noFileUpdated: "Never",
                uploadFailed: "Upload failed.",
                deletingFailed: "Delete failed.",                
                dialog: {
                	uploadTitle: "Upload SSL Key Repository Files",
                	deleteTitle: "Delete SSL Key Repository Files",
                	deleteContent: "If the MQConnectivity service is running, it will be restarted. Are you sure that you want to delete the SSL Key Repository Files?",
                	keyFileLabel: "Key Database File:",
                	passwordFileLabel: "Password Stash File:",
                	browseButton: "Browse...",
					browseHint: "Select a file...",
					savingProgress: "Saving...",
					deletingProgress: "Deleting...",
                	saveButton: "Save",
                	deleteButton: "OK",
                	cancelButton: "Cancel",
                	keyRepositoryError:  "The SSL key repository file must be a .kdb file.",
                	passwordStashError:  "The password stash file must be a .sth file.",
                	keyRepositoryMissingError: "A SSL key repository file is required.",
                	passwordStashMissingError: "A password stash file is required."
                }
			},
			externldap: {
				subTitle: "LDAP Configuration",
				tagline: "If LDAP is enabled, it will be used for server users and groups.",
				itemName: "LDAP Connection",
				grid: {
					ldapname: "LDAP Name",
					url: "URL",
					userid: "User ID",
					password: "Password"
				},
				enableButton: {
					enable: "Enable LDAP",
					disable: "Disable LDAP"
				},
				dialog: {
					ldapname: "LDAP Object Name",
					url: "URL",
					certificate: "Certificate",
					checkcert: "Check Server Certificate",
					checkcertTStoreOpt: "Use messaging server trust store",
					checkcertDisableOpt: "Disable certificate verification",
					checkcertPublicTOpt: "Use public trust store",
					timeout: "Timeout",
					enableCache: "Enable Cache",
					cachetimeout: "Cache Timeout",
					groupcachetimeout: "Group Cache Timeout",
					ignorecase: "Ignore Case",
					basedn: "BaseDN",
					binddn: "BindDN",
					bindpw: "Bind Password",
					userSuffix: "User Suffix",
					groupSuffix: "Group Suffix",
					useridmap: "User ID Map",
					groupidmap: "Group ID Map",
					groupmemberidmap: "Group Member ID Map",
					nestedGroupSearch: "Include Nested Groups",
					tooltip: {
						url: "The URL that points to the LDAP Server. The format is:<br/> &lt;protocol&gt;://&lt;server&nbsp;ip&gt;:&lt;port&gt;.",
						checkcert: "Specify how to check the LDAP server certificate.",
						basedn: "Base distinguished name.",
						binddn: "Distinguished name to use when binding to LDAP. For anonymous bind, leave blank.",
						bindpw: "Password to use when binding to LDAP. For anonymous bind, leave blank.",
						userSuffix: "User distinguished name suffix to search for. " +
									"If not specified, search for the user distinguished name using the user ID map, then bind with that distinguished name.",
						groupSuffix: "Group distinguished name suffix.",
						useridmap: "Property to map a user ID to.",
						groupidmap: "Property to map a group ID to.",
						groupmemberidmap: "Property to map a group member ID to.",
						timeout: "Timeout for LDAP calls in seconds.",
						enableCache: "Specify if credentials should be cached.",
						cachetimeout: "Cache timeout in seconds.",
						groupcachetimeout: "Group cache timeout in seconds.",
						nestedGroupSearch: "If checked, include all nested groups in the search for the group membership of a user.",
						testButtonHelp: "Tests the connection to the LDAP server. You must specify the LDAP server URL, and can optionally specify certificate, BindDN and BindPassword values."
					},
					secondUnit: "seconds",
					browseHint: "Select a file...",
					browse: "Browse...",
					clear: "Clear",
					connSubHeading: "LDAP Connection Settings",
					invalidTimeout: "The timeout must be a number between 1 and 60 inclusive.",
					invalidCacheTimeout: "The timeout must be a number between 1 and 60 inclusive.",
					invalidGroupCacheTimeout: "The timeout must be a number between 1 and 86,400 inclusive.",
					certificateRequired: "A certificate is required when an ldaps url is specified and the truststore is used",
					urlThemeError: "Enter a valid URL with a protocol of ldap or ldaps."
				},
				addDialog: {
					title: "Add LDAP Connection",
					instruction: "Configure an LDAP Connection."
				},
				editDialog: {
					title: "Edit LDAP Connection"
				},
				removeDialog: {
					title: "Delete LDAP Connection"
				},
				warnOnlyOneLDAPDialog: {
					title: "LDAP Connection already defined",
					content: "Only one LDAP Connection can be specified",
					closeButton: "Close"
				},
				restartInfoDialog: {
					title: "LDAP Settings Changed",
					content: "The new LDAP settings will be used the next time a client or connection is authenticated or authorized.",
					closeButton: "Close"
				},
				enableError: "An error occurred while enabling/disabling the external LDAP configuration.",				
				retrieveError: "An error occurred retrieving LDAP configuration.",
				saveSuccess: "Save successful. The ${IMA_SVR_COMPONENT_NAME} must be restarted for the changes to take effect."
			},
			messagequeues: {
				title: "Message Queues",
				subtitle: "Message Queues are used in point-to-point messaging."				
			},
			queues: {
				title: "Queues",
				tagline: "Define, edit, or delete a message queue.",
				retrieveError: "An error occurred retrieving the list of message queues.",
				grid: {
					name: "Name",
					allowSend: "Allow Send",
					maxMessages: "Maximum Messages",
					concurrentConsumers: "Concurrent Consumers",
					description: "Description"
				},
				dialog: {	
					instruction: "Define a queue for use with your messaging application.",
					nameInvalid: "The name cannot have leading or trailing spaces",
					maxMessagesTooltip: "Specify the maximum number of messages that can be stored on the queue.",
					maxMessagesInvalid: "The maximum messages must be a number between 1 and 20,000,000 inclusive.",
					allowSendTooltip: "Specify whether applications can send messages to this queue. This property does not affect the ability of applications to receive messages from this queue.",
					concurrentConsumersTooltip: "Specify whether the queue allows multiple concurrent consumers. If the check box is cleared, only one consumer is allowed on the queue.",
					performanceLabel: "Performance Properties"
				},
				addDialog: {
					title: "Add Queue"
				},
				removeDialog: {
					title: "Delete Queue",
					content: "Are you sure that you want to delete this record?"
				},
				editDialog: {
					title: "Edit Queue"
				}	
			},
			messagingTester: {
				title: "Messaging Tester Sample Application",
				tagline: "Messaging Tester is a simple HTML5 sample application that uses MQTT over WebSockets to simulate several clients interacting with the ${IMA_SVR_COMPONENT_NAME}. " +
						 "Once these clients are connected to the server, they can publish/subscribe on three topics.",
				enableSection: {
					title: "1. Enable the endpoint DemoMqttEndpoint",
					tagline: "The Messaging Tester sample application must connect to an unsecured ${IMA_PRODUCTNAME_SHORT} endpoint.  You can use DemoMqttEndpoint."					
				},
				downloadSection: {
					title: "2. Download and Run the Messaging Tester Sample Application",
					tagline: "Click on the link to download MessagingTester.zip.  Extract the files, then open index.html in a browser that supports WebSockets. " +
							 "Follow the instructions on the web page to verify that ${IMA_PRODUCTNAME_SHORT} is ready for MQTT messaging.",
					linkLabel: "Download MessagingTester.zip"
				},
				disableSection: {
					title: "3. Disable the endpoint DemoMqttEndpoint",
					tagline: "When you are finished verifying ${IMA_PRODUCTNAME_SHORT} MQTT messaging, disable the endpoint DemoMqttEndpoint to prevent unauthorized access to ${IMA_PRODUCTNAME_SHORT}."					
				},
				endpoint: {
					// TRNOTE {0} is replaced with the name of the endpoint
					label: "{0} Status",
					state: {
						enabled: "Enabled",					
						disabled: "Disabled",
						missing: "Missing",
						down: "Down"
					},
					linkLabel: {
						enableLinkLabel: "Enable",
						disableLinkLabel: "Disable"						
					},
					missingMessage: "If another unsecured endpoint is unavailable, create one.",
					retrieveEndpointError: "An error occurred while retrieving the endpoint configuration.",					
					retrieveEndpointStatusError: "An error occurred while retrieving the endpoint status.",
					saveEndpointError: "An error occurred while setting the endpoint state."
				}
			}
		},
		protocolsLabel: "Protocols",

		
		// Messaging policy types
		topicPoliciesTitle: "Topic Policies",
		subscriptionPoliciesTitle: "Subscription Policies",
		queuePoliciesTitle: "Queue Policies",
			
		// Messaging policy dialog strings
		addTopicMPTitle: "Add Topic Policy",
	    editTopicMPTitle: "Edit Topic Policy",
	    viewTopicMPTitle: "View Topic Policy",
		removeTopicMPTitle: "Delete Topic Policy",
		removeTopicMPContent: "Are you sure that you want to delete this topic policy?",
		topicMPInstruction: "A topic policy authorizes connected clients to perform specific messaging actions, such as which topics the client can access on ${IMA_PRODUCTNAME_FULL}. " +
					     "Each endpoint must have at least one topic policy, subscription policy, or queue policy.",
			
		addSubscriptionMPTitle: "Add Subscription Policy",
		editSubscriptionMPTitle: "Edit Subscription Policy",
		viewSubscriptionMPTitle: "View Subscription Policy",
		removeSubscriptionMPTitle: "Delete Subscription Policy",
		removeSubscriptionMPContent: "Are you sure that you want to delete this subscription policy?",
		subscriptionMPInstruction: "A subscription policy authorizes connected clients to perform specific messaging actions, such as which global-shared subscriptions the client can access on ${IMA_PRODUCTNAME_FULL}. " +
					     "In a global-shared subscription, the work of receiving messages from a durable topic subscription is shared between multiple subscribers. Each endpoint must have at least one topic policy, subscription policy, or queue policy.",
			
		addQueueMPTitle: "Add Queue Policy",
		editQueueMPTitle: "Edit Queue Policy",
		viewQueueMPTitle: "View Queue Policy",
		removeQueueMPTitle: "Delete Queue Policy",
		removeQueueMPContent: "Are you sure that you want to delete this queue policy?",
		queueMPInstruction: "A queue policy authorizes connected clients to perform specific messaging actions, such as which queues the client can access on ${IMA_PRODUCTNAME_FULL}. " +
					     "Each endpoint must have at least one topic policy, subscription policy, or queue policy.",
		
		// Messaging and Endpoint dialog strings
		policyTypeTitle: "Policy Type",
		policyTypeName_topic: "Topic",
		policyTypeName_subscription: "Global-shared Subscription",
		policyTypeName_queue: "Queue",
		
		// Messaging policy type names that are embedded in other strings
		policyTypeShortName_topic: "topic",
		policyTypeShortName_subscription: "subscription",
		policyTypeShortName_queue: "queue",
		
		policyTypeTooltip_topic: "Topic that this policy applies to. Use asterisk (*) with caution. " +
		    "An asterisk matches 0 or more characters, including slash (/). Therefore, it can match multiple levels in a topic tree.",
		policyTypeTooltip_subscription: "Global-shared durable subscription that this policy applies to. Use asterisk (*) with caution. " +
			"An asterisk matches 0 or more characters.",
	    policyTypeTooltip_queue: "Queue that this policy applies to. Use asterisk (*) with caution. " +
			"An asterisk matches 0 or more characters.",
			
	    topicPoliciesTagline: "A topic policy allows you to control which topics a client can access on ${IMA_PRODUCTNAME_FULL}.",
		subscriptionPoliciesTagline: "A subscription policy allows you to control which global-shared durable subscriptions a client can access on ${IMA_PRODUCTNAME_FULL}.",
	    queuePoliciesTagline: "A queue policy allows you to control which queues a client can access on ${IMA_PRODUCTNAME_FULL}.",
	    
	    // Additional MQConnectivity queue manager connection dialog properties
	    channelUser: "Channel User",
	    channelPassword: "Channel User Password",
	    channelUserTooltip: "If your queue manager is configured to authenticate channel users, you must set this value.",
	    channelPasswordTooltip: "If you specify a channel user, you must also set this value.",
	    
	    // Additional LDAP dialog properties
	    emptyLdapURL: "The LDAP URL is not set",
		externalLDAPOnlyTagline: "Configure the connection properties for the LDAP repository that is used for ${IMA_SVR_COMPONENT_NAME} users and groups.", 
		// TRNNOTE: Do not translate bindPasswordHint.  This is a place holder string.
		bindPasswordHint: "********",
		clearBindPasswordLabel: "Clear",
		resetLdapButton: "Reset",
		resetLdapTitle: "Reset LDAP Configuration",
		resetLdapContent: "All LDAP configuration settings will be reset to default values.",
		resettingProgress: "Resetting...",
		
		// New SSL Key repository dialog strings
		sslkeyrepositoryDialogUploadContent: "If the MQConnectivity service is running, it will be restarted after the repository files are uploaded.",
		savingRestartingProgress: "Saving and restarting...",
		deletingRestartingProgress: "Deleting and restarting..."
	}),
	
	  "zh": true,
	  "zh-tw": true,
	  "ja": true,
	  "fr": true,
	  "de": true

});
