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
define(['dojo/_base/declare', 'dojo/i18n!ism/nls/home', 'dojo/i18n!ism/nls/strings', 'dojo/i18n!ism/nls/messaging', 'ism/Utils'], 
        function(declare, nls_home, nls, nls_msg, Utils) {
    
    var Resources = {
        roles: {
                messagingAdmin: "MessagingAdministrator",
                applianceAdmin: "SystemAdministrator",
                observer: "User"
            },          
        pages: {
            login: {
                uri: "login.jsp"
            },
			licenseAccept: {
				name: "licenseAccept",
				uri: "licenseAccept.jsp",
				roles: ["all"],
				nav: {
					licenseAccept: {
						name: "licenseAccept",
						topic: "action/license/license",
						uri: "js/ism/templates/license/licenseAccept.html",
						roles: ["all"]
					}
				},
				accepttopic: "ism/action/license/accept"
			},
            license: {
                name: "license",
                uri: "license.jsp",
                roles: ["all"],
                nav: {
                	Developers: {
                        name: "Developers",
                        topic: "action/license/license",
                        uri: "js/ism/templates/license/license.html",
                        roles: ["all"]
                    },
                    NonProduction: {
                        name: "license",
                        topic: "action/license/license",
                        uri: "js/ism/templates/license/license.html",
                        roles: ["all"]
                    },
                    Production: {
                        name: "license",
                        topic: "action/license/license",
                        uri: "js/ism/templates/license/license.html",
                        roles: ["all"]
                    }
                }
            },
            firstserver: {
                name: "firstserver",
                uri: "firstserver.jsp",
                roles: ["all"],
                nav: {
                    firstserver: {
                        name: "firstserver",
                        topic: "action/firstserver/firstserver",
                        uri: "js/ism/templates/firstserver/firstserver.html",
                        roles: ["all"]
                    }
                },
//                testtopic: "ism/action/firstserver/testconn",
                savetopic: "ism/action/firstserver/save"
            },
            home: {
                name: "home",
                uri: "index.jsp",
                roles: ["all"],
                nav: {
                    home: {
                        name: nls.global.home,
                        topic: "action/home/home",
                        uri: "js/ism/templates/home/flex.html",
                        roles: ["all"]
                    }
                }
            },
            messaging: {
                name: "messaging",
                uri: "messaging.jsp",
                roles: ["MessagingAdministrator", "SystemAdministrator"],
                nav: {
                    messageHubs: {
                        name: nls.globalMenuBar.messaging.messageHubs,
                        topic: "action/messaging/messageHubs",
                        uri: "js/ism/templates/messaging/messageHubs.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    messageHubDetails: {
                        name: nls.globalMenuBar.messaging.messageHubDetails,
                        excludeFromMenu: true,
                        topic: "action/messaging/messageHubDetails",
                        uri: "js/ism/templates/messaging/messageHubDetails.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1, 9], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    messagequeues: {
                        name: nls.globalMenuBar.messaging.messagequeues,
                        topic: "action/messaging/messagequeues",
                        uri: "js/ism/templates/messaging/messagequeues.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1 ], // needs the ISM server to be running (since this requires the Engine, can't use maintenance mode)
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    mqConnectivity: {
                        name: nls.globalMenuBar.messaging.mqConnectivity,
                        topic: "action/messaging/mqConnectivity",
                        uri: "js/ism/templates/messaging/mqConnectivity.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    messageProtocols: {
                        name: nls.globalMenuBar.messaging.messageProtocols,
                        topic: "action/messaging/messageProtocols",
                        uri: "js/ism/templates/messaging/messageProtocols.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    messagingTester: {
                        name: nls.globalMenuBar.messaging.messagingTester,
                        topic: "action/messaging/messagingTester",
                        uri: "js/ism/templates/messaging/messagingTester.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1, 9 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    }
                },
                messageHubTabs: {
                    connectionPolicies: {
                        id: "ismMHConnectionPolicies",
                        title: nls_msg.messaging.messageHubs.messageHubDetails.connectionPoliciesTitle,
                        tooltip: nls_msg.messaging.messageHubs.messageHubDetails.connectionPoliciesTip,
                        classname: "ism.controller.content.ConnectionPoliciesPane"                              
                    },                                              
                    messagingPolicies: {
                        id: "ismMHMessagingPolicies",
                        title: nls_msg.messaging.messageHubs.messageHubDetails.messagingPoliciesTitle,
                        tooltip: nls_msg.messaging.messageHubs.messageHubDetails.messagingPoliciesTip,
                        classname: "ism.controller.content.MessagingPoliciesPane"                               
                    },
                    endpoints: {
                        id: "ismMHEndpoints",
                        title: nls_msg.messaging.messageHubs.messageHubDetails.endpointsTitle,
                        tooltip: nls_msg.messaging.messageHubs.messageHubDetails.endpointsTip,
                        classname: "ism.controller.content.EndpointsPane"                               
                    }                   
                }
            },
            monitoring: {
                name: "monitoring",
                uri: "monitoring.jsp",
                roles: ["all"],
                nav: {
                    connectionStatistics: {
                        name: nls.globalMenuBar.monitoring.connectionStatistics,
                        topic: "action/monitoring/connectionStatistics",
                        uri: "js/ism/templates/monitoring/connectionStatistics.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },                  
                    endpointStatistics: {
                        name: nls.globalMenuBar.monitoring.endpointStatistics,
                        topic: "action/monitoring/endpointStatistics",
                        uri: "js/ism/templates/monitoring/endpointStatistics.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },      
                    queueMonitor: {
                        name: nls.globalMenuBar.monitoring.queueMonitor,
                        topic: "action/monitoring/queueMonitor",
                        uri: "js/ism/templates/monitoring/queueMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },      
                    topicMonitor: {
                        name: nls.globalMenuBar.monitoring.topicMonitor,
                        topic: "action/monitoring/topicMonitor",
                        uri: "js/ism/templates/monitoring/topicMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },      
                    mqttClientMonitor: {
                        name: nls.globalMenuBar.monitoring.mqttClientMonitor,
                        topic: "action/monitoring/mqttClientMonitor",
                        uri: "js/ism/templates/monitoring/mqttClientMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    subscriptionMonitor: {
                        name: nls.globalMenuBar.monitoring.subscriptionMonitor,
                        topic: "action/monitoring/subscriptionMonitor",
                        uri: "js/ism/templates/monitoring/subscriptionMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    transactionMonitor: {
                        name: nls.globalMenuBar.monitoring.transactionMonitor,
                        topic: "action/monitoring/transactionMonitor",
                        uri: "js/ism/templates/monitoring/transactionMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    destinationMappingRuleMonitor: {
                        name: nls.globalMenuBar.monitoring.destinationMappingRuleMonitor,
                        topic: "action/monitoring/destinationMappingRuleMonitor",
                        uri: "js/ism/templates/monitoring/destinationMappingRuleMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                        // Set to false for Beta
                    },      
                    applianceMonitor: {
                        name: nls.globalMenuBar.monitoring.applianceMonitor,
                        topic: "action/monitoring/applianceMonitor",
                        uri: "js/ism/templates/monitoring/applianceMonitor.html",
                        roles: ["all"],
                        availability: [ 1 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    }
//                    snmpSettings: {
//                        name: nls.globalMenuBar.monitoring.snmpSettings,
//                        topic: "action/monitoring/snmpSettings",
//                        uri: "js/ism/templates/monitoring/snmpSettings.html",
//                        roles: ["MessagingAdministrator", "SystemAdministrator"],
//                    }
                }
            },
            appliance: {
                name: "appliance",
                uri: "appliance.jsp",
                roles: ["SystemAdministrator"],
                nav: {
                    securitySettings: {
                        name: nls.globalMenuBar.appliance.securitySettings,
                        topic: "action/appliance/securitySettings",
                        uri: "js/ism/templates/appliance/securitySettings.html",
                        roles: ["SystemAdministrator"],
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance mode
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },                  
                    userAdministration: {
                        name: nls.globalMenuBar.messaging.userAdministration,
                        topic: "action/messaging/userAdministration",
                        uri: "js/ism/templates/appliance/externalLDAP.html",
                        roles: ["MessagingAdministrator", "SystemAdministrator"],
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    systemControl: {
                        name: nls.globalMenuBar.appliance.systemControl,
                        topic: "action/appliance/systemControl",
                        uri: "js/ism/templates/appliance/remoteSystemControl.html",
                        roles: ["SystemAdministrator"],
                        availability: [ 1, 9, 10 ] // needs the ISM server to be running or in maintenance mode or Standby
                    },
                    adminEndpoint: {
                        name: nls.adminEndpoint,
                        topic: "action/appliance/adminEndpoint",
                        uri: "js/ism/templates/appliance/adminEndpoint.html",
                        roles: ["SystemAdministrator"],
                        availability: [ 1, 9, 10 ] // needs the ISM server to be running or in maintenance node or Standby
                    },
                    highAvailability: {
                        name: nls.globalMenuBar.appliance.highAvailability,
                        topic: "action/appliance/highAvailability",
                        uri: "js/ism/templates/appliance/highAvailability.html",
                        roles: ["SystemAdministrator"],
                        availability: [ 1, 9, 10 ] // needs the ISM server to be running or in maintenance node or Standby
                    }
                }
            },
            cluster: {
                name: "cluster",
                uri: "cluster.jsp",
                roles: ["all"],
                nav: {
                    cluster: {
                        name: nls.clusterMembership,  
                        topic: "action/cluster/membership",
                        uri: "js/ism/templates/cluster/membership.html",
                        roles: ["SystemAdministrator"],
                        availability: [ 1, 9, 10 ]
                    },
                    clusterStats: {
                        name: nls.clusterStatus,  
                        topic: "action/cluster/statistics",
                        uri: "js/ism/templates/cluster/statistics.html",
                        roles: ["all"],
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"], // HA Role must be primary or HA must be disabled
                        cluster: ["Active"] // Cluster status must be in Active state
                    }
                }
            },
            webui: {
            	name: "webui",
            	uri: "webui.jsp",
            	roles: ["SystemAdministrator"],
            	nav: {
                    users: {
                        name: nls.globalMenuBar.appliance.users,
                        topic: "action/appliance/users",
                        uri: "js/ism/templates/webui/users.html",
                        roles: ["SystemAdministrator"]                    },
                    webuiSecuritySettings: {
                        name: nls.globalMenuBar.appliance.webuiSecuritySettings,
                        topic: "action/appliance/webuiSecuritySettings",
                        uri: "js/ism/templates/webui/webUISecurity.html",
                        roles: ["SystemAdministrator"]
                    }
            	}
            }
        },
        /*
         * The flex dashboard consists of horizontal sections, each containing a set of tiles. Each
         * tile has a set of properties, including id, title, description, widget, and widget options.
         * By default, the flex dashboard will show the sections defined in flex.defaultSections, and
         * each section will show the tiles defined in flex[sectionName].defaultTiles. In the future, 
         * we may allow the user to choose from the set of available tiles and sections to customize 
         * their dashboard.  The customized section and tile lists could then be saved in userPreferences.
         */
        flexDashboard: {
            title: nls_home.home.dashboards.flex.title,
            titleWithNodeName: nls_home.home.dashboards.flex.titleWithNodeName,
            roles: ["all"],
            // refreshRateMillis for dataSets that server multiple tiles go here
            serverDataInterval: 5000,  // because the endpoint history is available in 5 second intervals, not 3
            throughputDataInterval: 5000,
            duration: 600,     // start with last 10 minutes of history
            numPoints: 120,    // show 10 minutes
            availableSections: {
                d3QS: {
                    id: "ismFlex_d3_quickStats",
                    title: nls_home.home.dashboards.flex.quickStats.title,
                    styleClass: "flexTile_innerBorder",
                    // the available tiles (for future user customization)
                    availableTiles: {
                        activeConnectionsQS: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.activeConnectionsQS.title,
                            widget: "ism.controller.content.BigStat",
                            options: {
                                id: "ismFlex_activeConnectionsQS",
                                description: nls_home.home.dashboards.flex.activeConnectionsQS.description,
                                dataSet: "serverData",
                                dataItem: "connVolume",                             
                                legend: nls_home.home.dashboards.flex.activeConnectionsQS.legend,
                                type: "number"
                            }
                        },
                        throughputQS: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.throughputQS.title,
                            widget: "ism.controller.content.BigStat",
                            options: {
                                id: "ismFlex_throughputQS",
                                description: nls_home.home.dashboards.flex.throughputQS.description,
                                dataSet: "throughputData",
                                dataItem: "currentMsgs",
                                legend: nls_home.home.dashboards.flex.throughputQS.legend,
                                type: "number"
                            }
                        },
                        uptimeQS: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.uptimeQS.title,
                            widget: "ism.controller.content.BigStat",
                            options: {
                                id: "ismFlex_uptimeQS",
                                description: nls_home.home.dashboards.flex.uptimeQS.description,
                                dataSet: "uptimeData",
                                dataItem: "uptime",
                                legend: nls_home.home.dashboards.flex.uptimeQS.legend,
                                type: "time",
                                iconClass: "ismIconBigGreenUpArrow",
                                refreshRateMillis: 3000
                            }
                        },
                        applianceQS: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.applianceQS.title,
                            widget: "ism.controller.content.SystemResourcesPane",
                            options: {
                                id: "ismFlex_applianceQS",
                                description: nls_home.home.dashboards.flex.applianceQS.description,
                                storeData: {  // define which store statistics to show a bar for
                                    DiskUsedPercent: {
                                        title: nls_home.home.dashboards.flex.applianceQS.bars.disk.label,
                                        thresholdPercents: {
                                            warn:  80,
                                            alert: 90
                                        },
                                        warningThresholdText: nls_home.home.dashboards.flex.applianceQS.bars.disk.warningThresholdText,
                                        alertThresholdText: nls_home.home.dashboards.flex.applianceQS.bars.disk.alertThresholdText
                                    }                                       
                                },
                                memoryData: { // define which memory statistics to show a bar for
                                    MemoryFreePercent: {
                                        title: nls_home.home.dashboards.flex.applianceQS.bars.mem.label,
                                        invert: true,  // show the MemoryUsed instead of free
                                        thresholdPercents: {
                                            warn: 65,
                                            alert: 85
                                        },
                                        warningThresholdText: nls_home.home.dashboards.flex.applianceQS.bars.mem.warningThresholdText,
                                        alertThresholdText: nls_home.home.dashboards.flex.applianceQS.bars.mem.alertThresholdText                               
                                    }                                       
                                },
                                refreshRateMillis: 5000  // set to < 0 to disable
                            }
                        }
                    },
                    // the default tiles to show in order
                    defaultTiles: ["activeConnectionsQS", "throughputQS", "uptimeQS", "applianceQS"],                           
                    clusterTiles: ["throughputQS"]
                }, /* end quickStats*/

                d3LiveCharts: {
                    id: "ismFlex_d3_liveCharts",
                    title: nls_home.home.dashboards.flex.liveCharts.title,
                    styleClass: "flexTile_zeroPadding",
                    // the available tiles (for future user customization)
                    availableTiles: {
                        connections: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.connections.title,
                            widget: "ism.controller.content.FlexChartD3",
                            options: {
                                id: "ismFlex_connections",
                                description: nls_home.home.dashboards.flex.connections.description,
                                type: "D3", //"Areas",
                                dataSet: "connectionData",
                                dataItem: "ActiveConnections",
                                fullRefresh: true,
                                refreshRateMillis: 5000,
                                //historicalDataFunction: "getEndpointHistory",
                                //historicalDataMetric: "ActiveConnections", // for the historical data prepopulation
                                duration: 600,     // start with last 10 minutes of history
                                numPoints: 120,    // show 10 minutes
                                startingColor: "blue",  
                                chartWidth: "600px",
                                chartHeight: "300px",
                                margin: {left: 80, top: 25, right: 60, bottom: 40},                                 
                                xAxisOptions: {
                                    ticks: {unit: "minute", value: 2}, // tick every 2 minutes
                                    minorTicks: {unit: "second", value: 30} // minor tick every 30 seconds
                                },
                                yAxisOptions: {
                                    integers: true,
                                    legend: nls_home.home.dashboards.flex.connections.legend.y  // d3
                                }
                            }
                        },
                        throughput: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.throughput.title,
                            widget: "ism.controller.content.FlexChartD3",
                            options: {
                                id: "ismFlex_throughput",
                                description: nls_home.home.dashboards.flex.throughput.description,
                                type: "D3",
                                stackedAreas: true,
                                dataSet: "throughputData",
                                dataItem: ["MsgsRead", "MsgsWrite", "ClMsgsRead", "ClMsgsWrite"],
                                fullRefresh: true, // refresh the whole thing every time
                                //historicalDataFunction: "getThroughputHistory",                               
                                //historicalDataMetric: "Msgs",    // for the historical data prepopulation
                                duration: 600,     // start with last 10 minutes of history
                                numPoints: 120,    // show 10 minutes
                                colors: {MsgsRead:"purple", MsgsWrite: "teal", ClMsgsRead:"magenta50", ClMsgsWrite: "green"},  
                                chartWidth: "600px",
                                chartHeight: "300px",
                                margin: {left: 80, top: 25, right: 30, bottom: 40},
                                legend: {
                                    title: nls_home.home.dashboards.flex.throughput.legend.title,
                                    MsgsRead: nls_home.home.dashboards.flex.throughput.legend.incoming,
                                    MsgsWrite: nls_home.home.dashboards.flex.throughput.legend.outgoing,
                                    ClMsgsRead: nls_home.clusterLegendIncoming,
                                    ClMsgsWrite: nls_home.clusterLegendOutgoing,
                                    hover: {
                                        MsgsRead: nls_home.home.dashboards.flex.throughput.legend.hover.incoming,
                                        MsgsWrite: nls_home.home.dashboards.flex.throughput.legend.hover.outgoing,                                       
                                        ClMsgsRead: nls_home.clusterHoverIncoming,
                                        ClMsgsWrite: nls_home.clusterHoverOutgoing
                                    },
                                    subgroup: [{
                                        label: nls_home.clusterThroughputLabel,
                                        items: ["ClMsgsRead","ClMsgsWrite"]
                                    },
                                    {
                                        label: nls_home.clientThroughputLabel,
                                        items: ["MsgsRead","MsgsWrite"]
                                    }]
                                },
                                legenddetail: {
                                    MsgsRead: nls_home.clientThroughputLabel,
                                    MsgsWrite: nls_home.clientThroughputLabel,                                       
                                    ClMsgsRead: nls_home.clusterThroughputLabel,
                                    ClMsgsWrite: nls_home.clusterThroughputLabel                              	
                                },
                                xAxisOptions: {
                                    ticks: {unit: "minute", value: 2}, // tick every 2 minutes
                                    minorTicks: {unit: "second", value: 30} // minor tick every 30 seconds
                                },
                                yAxisOptions: {
                                    legend: nls_home.home.dashboards.flex.throughput.legend.y
                                }
                            }
                        }
                    },
                    // the default tiles to show in order
                    defaultTiles: ["connections", "throughput", "disk"],                         
                    clusterTiles: ["throughput"]
                }, /* end d3 liveCharts*/
                d3Mem: {
                    id: "ismFlex_d3_mem",
                    title: nls_home.home.dashboards.flex.applianceResources.title,
                    styleClass: "flexTile_zeroPadding",
                    // the available tiles (for future user customization)
                    availableTiles: {
                        memoryDetail: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.memoryDetail.title,
                            widget: "ism.controller.content.FlexChartD3",
                            options: {
                                id: "ismFlex_memoryDetailHistory",
                                description: nls_home.home.dashboards.flex.memoryDetail.description,
                                type: "D3",
                                stackedAreas: true,
                                dataSet: "memoryDetailHistoryData",
                                dataItem: ["MessagePayloads", "PublishSubscribe", "Destinations", "CurrentActivity","ClientStates","System"],
                                fullRefresh: true, // refresh the whole thing every time                                
                                refreshRateMillis: 60000, // 1 minute                               
                                duration: 86400,          // 24 hours
                                numPoints: 1440,          // show 24 hours, every 1 minute
                                colors: {"MessagePayloads":"orange", "PublishSubscribe":"yellow", "Destinations":"green", "CurrentActivity":"teal","ClientStates":"blue","System":"blue50"},
                                chartWidth: "1000px",
                                chartHeight: "360px",
                                margin: {left: 80, top: 25, right: 30, bottom: 100}, //{l: 80, t: 20, r: 30, b: 40},
                                brushMargin: {left: 80, top: 290, right: 30, bottom: 50}, // show D3 brush                              
                                legend: {
                                    x: nls_home.home.dashboards.flex.memoryDetail.legend.x,
                                    y: nls_home.home.dashboards.flex.memoryDetail.legend.y,
                                    title: nls_home.home.dashboards.flex.memoryDetail.legend.title,
                                    System: nls_home.home.dashboards.flex.memoryDetail.legend.system,
                                    MessagePayloads: nls_home.home.dashboards.flex.memoryDetail.legend.messagePayloads,
                                    PublishSubscribe: nls_home.home.dashboards.flex.memoryDetail.legend.publishSubscribe,
                                    Destinations: nls_home.home.dashboards.flex.memoryDetail.legend.destinations,
                                    CurrentActivity: nls_home.home.dashboards.flex.memoryDetail.legend.currentActivity,
                                    ClientStates: nls_home.home.dashboards.flex.memoryDetail.legend.clientStates,
                                    hover: {
                                        System: nls_home.home.dashboards.flex.memoryDetail.legend.hover.system,
                                        MessagePayloads: nls_home.home.dashboards.flex.memoryDetail.legend.hover.messagePayloads,
                                        PublishSubscribe: nls_home.home.dashboards.flex.memoryDetail.legend.hover.publishSubscribe,
                                        Destinations: nls_home.home.dashboards.flex.memoryDetail.legend.hover.destinations,
                                        CurrentActivity: nls_home.home.dashboards.flex.memoryDetail.legend.hover.currentActivity,
                                        ClientStates: nls_home.home.dashboards.flex.memoryDetail.legend.hover.clientStates                                      
                                    }
                                },                              
                                xAxisOptions: {
                                    // x-axis options
                                    ticks: 8, // tick every 3 hours
                                    minorTicks: 24 // minor tick every 1 hour
                                },
                                yAxisOptions: {
                                    // y-axis options
                                    max: 100,
                                    legend: nls_home.home.dashboards.flex.memoryDetail.legend.y 
                                }
                            }
                        }
                    },
                    // the default tiles to show in order
                    defaultTiles: ["memoryDetail"],  
                    clusterTiles: []
                },
                d3StoreMemory: {
                    id: "ismFlex_d3_store",
                    title: nls_home.home.dashboards.flex.resourceDetails.title,
                    styleClass: "flexTile_innerBorder_tight",
                    // the available tiles (for future user customization)
                    availableTiles: {
                        storeMemory: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.disk.title,
                            widget: "ism.controller.content.FlexChartD3",
                            hideOnVirtual: false,
                            options: {
                                id: "ismFlex_storeMemory",
                                description: nls_home.home.dashboards.flex.storeMemory.description,
                                type: "D3", 
                                stackedAreas: true,
                                dataSet: "storeDetailHistoryData",
                                dataItem: ["MQConnectivity","Topics","Subscriptions","Queues","ClientStates","Transactions","Pool1System"],
                                warnMessage: "Pool1LimitMessage",
                                colors: {"MQConnectivity":"purple","Topics":"orange","Subscriptions":"yellow", "Queues":"green", "ClientStates":"teal","Transactions":"blue","Pool1System":"blue50"},
                                horizontalLine: {
                                    yValue: "Pool1Limit",
                                    color: "blue",
                                    width: 2,
                                    legend: nls_home.home.dashboards.flex.storeMemory.legend.recordLimit,
                                    legendHover: nls_home.home.dashboards.flex.storeMemory.legend.hover.recordLimit
                                },
                                legend: {
                                    title: nls_home.home.dashboards.flex.storeMemory.legend.pool1Title,
                                    Pool1System: nls_home.home.dashboards.flex.storeMemory.legend.system,
                                    MQConnectivity: nls_home.home.dashboards.flex.storeMemory.legend.MQConnectivity,
                                    Transactions: nls_home.home.dashboards.flex.storeMemory.legend.Transactions,
                                    Topics: nls_home.home.dashboards.flex.storeMemory.legend.Topics,
                                    Subscriptions: nls_home.home.dashboards.flex.storeMemory.legend.Subscriptions,
                                    Queues: nls_home.home.dashboards.flex.storeMemory.legend.Queues,                                        
                                    ClientStates: nls_home.home.dashboards.flex.storeMemory.legend.ClientStates,                                        
                                    hover: {
                                        Pool1System: nls_home.home.dashboards.flex.storeMemory.legend.hover.system,
                                        IncomingMessageAcks: nls_home.home.dashboards.flex.storeMemory.legend.hover.IncomingMessageAcks,
                                        MQConnectivity: nls_home.home.dashboards.flex.storeMemory.legend.hover.MQConnectivity,
                                        Transactions: nls_home.home.dashboards.flex.storeMemory.legend.hover.Transactions,
                                        Topics: nls_home.home.dashboards.flex.storeMemory.legend.hover.Topics,
                                        Subscriptions: nls_home.home.dashboards.flex.storeMemory.legend.hover.Subscriptions,
                                        Queues: nls_home.home.dashboards.flex.storeMemory.legend.hover.Queues,                                      
                                        ClientStates: nls_home.home.dashboards.flex.storeMemory.legend.hover.ClientStates                                       
                                    },
                                    subgroup: {
                                        label: nls_home.recordsLabel,
                                        items: ["MQConnectivity","Topics","Subscriptions","Queues","ClientStates","Transactions"]
                                    }
                                },                              
                                fullRefresh: true, // refresh the whole thing every time
                                refreshRateMillis: 60000, // 1 minute                               
                                duration: 86400,          // 24 hours
                                numPoints: 1440,          // show 24 hours, every 1 minute
                                chartWidth: "600px",
                                chartHeight: "360px",
                                margin: {left: 80, top: 25, right: 30, bottom: 110},
                                brushMargin: {left: 80, top: 290, right: 30, bottom: 60}, // show D3 brush                              
                                xAxisOptions: {
                                    // x-axis options
                                    ticks: 6, // tick every 4 hours
                                    minorTicks: 24 // minor tick every 1 hour
                                },
                                yAxisOptions: {
                                    // y-axis options
                                    max: 100,
                                    legend: nls_home.home.dashboards.flex.storeMemory.legend.y 
                                }
                            }
                        },
                        incomingMessageAckMemory: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.storeMemory.legend.IncomingMessageAcks,
                            widget: "ism.controller.content.FlexChartD3",
                            options: {
                                id: "ismFlex_incomingMessageAckMemory",
                                description:  nls_home.home.dashboards.flex.storeMemory.legend.hover.IncomingMessageAcks,
                                type: "D3", //"Areas",
                                stackedAreas: true,
                                dataSet: "storeDetailHistoryData",
                                dataItem: ["IncomingMessageAcks","Pool2System"],
                                colors: {"IncomingMessageAcks":"blue","Pool2System":"blue50"},
                                legend: {
                                    title: nls_home.home.dashboards.flex.storeMemory.legend.pool2Title,
                                    Pool2System: nls_home.home.dashboards.flex.storeMemory.legend.system,
                                    IncomingMessageAcks: nls_home.home.dashboards.flex.storeMemory.legend.IncomingMessageAcks,
                                    hover: {
                                        Pool2System: nls_home.home.dashboards.flex.storeMemory.legend.hover.system,
                                        IncomingMessageAcks: nls_home.home.dashboards.flex.storeMemory.legend.hover.IncomingMessageAcks
                                    }
                                },                              
                                fullRefresh: true, // refresh the whole thing every time
                                refreshRateMillis: 60000, // 1 minute                               
                                duration: 86400,          // 24 hours
                                numPoints: 1440,          // show 24 hours, every 1 minute
                                chartWidth: "600px",
                                chartHeight: "360px",
                                margin: {left: 80, top: 25, right: 30, bottom: 110},
                                brushMargin: {left: 80, top: 290, right: 30, bottom: 60}, // show D3 brush                              
                                xAxisOptions: {
                                    // x-axis options
                                    ticks: 6, // tick every 4 hours
                                    minorTicks: 24 // minor tick every 1 hour
                                },
                                yAxisOptions: {
                                    // y-axis options
                                    max: 100,
                                    legend: nls_home.home.dashboards.flex.storeMemory.legend.y 
                                }
                            }
                        }                       
                    },
                    // the default tiles to show in order
                    defaultTiles: ["storeMemory", "incomingMessageAckMemory"],                           
                    clusterTiles: []
                }, /* end d3StoreMemory */
                d3StoreDisk: {
                    id: "ismFlex_d3_storeDisk",
                    title: nls_home.home.dashboards.flex.diskResourceDetails.title,
                    styleClass: "flexTile_zeroPadding",
                    // the available tiles (for future user customization)
                    availableTiles: {
                        disk: {
                            roles: ["all"],
                            title: nls_home.home.dashboards.flex.disk.title,
                            widget: "ism.controller.content.FlexChartD3",
                            options: {
                                id: "ismFlex_diskHistory",
                                description: nls_home.home.dashboards.flex.disk.description,
                                type: "D3", //"Areas",
                                dataSet: "storeHistoryData",
                                dataItem: "DiskUsedPercent",
                                fullRefresh: true, // refresh the whole thing every time
                                refreshRateMillis: 60000, // 1 minute                               
                                duration: 86400,          // 24 hours
                                numPoints: 1440,          // show 24 hours, every 1 minute
                                startingColor: "magenta",   
                                chartWidth: "600px",
                                chartHeight: "360px",
                                margin: {left: 80, top: 25, right: 60, bottom: 110}, //{l: 80, t: 20, r: 30, b: 40},
                                brushMargin: {left: 80, top: 290, right: 60, bottom: 60}, // show D3 brush                              
                                xAxisOptions: {
                                    // x-axis options
                                    ticks: 6, // tick every 4 hours
                                    minorTicks: 24 // minor tick every 1 hour                                   
                                },
                                yAxisOptions: {
                                    // y-axis options
                                    max: 100,
                                    legend: nls_home.home.dashboards.flex.disk.legend.y 
                                }
                            }
                        }
                    },
                    // the default tiles to show in order
                    defaultTiles: ["disk"],                          
                    clusterTiles: []
                } /* end d3StoreDisk */             
            },
            // the default sections to show in order
            defaultSections: ["d3QS","d3LiveCharts","d3Mem", "d3StoreMemory", "d3StoreDisk"],
            clusterSections: ["d3LiveCharts"]
        },

        restoreTasks: {  // used to determine whether to show the Restore Tasks menu item in the Help menu
            roles: ["SystemAdministrator", "MessagingAdministrator"]        
        },
        actions: {
            deleteMqttClient: {  // on MQTT Clients monitoring page
                roles: ["SystemAdministrator", "MessagingAdministrator"]
            },
            deleteDurableSubscription: {  // on Subscriptions monitoring page
                roles: ["SystemAdministrator", "MessagingAdministrator"]
            },
            configureTopicMonitor: { // on Topic monitoring page
                roles: ["SystemAdministrator", "MessagingAdministrator"]
            },
            processTransactions: { // on Transaction monitoring page
                roles: ["SystemAdministrator", "MessagingAdministrator"]
            },
            editMessagingPolicy: { // on Subscription monitoring page
                roles: ["SystemAdministrator", "MessagingAdministrator"]                
            }
        },
        tasks: {  // used to populate the common tasks section of the home page
            messagingTester: {
                roles: ["SystemAdministrator", "MessagingAdministrator"],
                id: "ismTaskMessagingTester",
                cssClass: "mqttIcon",
                heading: nls_home.home.tasks.messagingTester.heading,
                description: nls_home.home.tasks.messagingTester.description,
                links: [
                    {
                        uri: "messaging.jsp?nav=messagingTester", 
                        label: nls_home.home.tasks.messagingTester.links.messagingTester,
                        availability: [ 1, 9 ], // needs the ISM server to be running
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    }
                ]
            },
            security: {
                roles: ["SystemAdministrator"],
                id: "ismTaskSecurity",
                cssClass: "securityIcon",
                heading: nls_home.home.tasks.security.heading,
                description: nls_home.home.tasks.security.description,
                links: [
                    {
                        uri: "webui.jsp?nav=webuiSecuritySettings",
                        label: nls_home.home.tasks.security.links.webuiSettings
                    }, 
                    {
                        uri: "appliance.jsp?nav=securitySettings",
                        label: nls_home.home.tasks.security.links.keysAndCerts,
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled                     
                    } 
                ]
            },          
            users: {
                roles: ["SystemAdministrator"],
                id: "ismTaskUsers",
                cssClass: "usersIcon",
                heading: nls_home.home.tasks.users.heading,
                description: nls_home.home.tasks.users.description,
                links: [
                    {
                        uri: "webui.jsp?nav=users",
                        label: nls_home.home.tasks.users.links.users,
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled                     
                    } 
                ]
            },
//            messagingUsers: {
//                roles: ["MessagingAdministrator"],
//                id: "ismTaskMessagingUsers",
//                subsetOf: "ismTaskUsers",
//                cssClass: "usersIcon",
//                heading: nls_home.home.tasks.messagingUsers.heading,
//                description: nls_home.home.tasks.messagingUsers.description,
//                links: [
//                    {
//                        uri: "messaging.jsp?nav=userAdministration",
//                        label: nls_home.home.tasks.messagingUsers.links.userGroups,
//                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
//                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
//                    } 
//                ]
//            },          
            connections: {
                roles: ["SystemAdministrator","MessagingAdministrator"],
                id: "ismTaskConnections",
                cssClass: "portsIcon",
                heading: nls_home.home.tasks.connections.heading,
                description: nls_home.home.tasks.connections.description,
                links: [
                    {
                        uri: "messaging.jsp?nav=messageHubs",
                        label: nls_home.home.tasks.connections.links.listeners,
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    {
                        uri: "messaging.jsp?nav=mqConnectivity",
                        label: nls_home.home.tasks.connections.links.mqconnectivity,
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    },
                    {
                        uri: "appliance.jsp?nav=userAdministration",
                        label: nls_home.home.tasks.users.links.userGroups,
                        availability: [ 1, 9 ], // needs the ISM server to be running or in maintenance node
                        ha: ["PRIMARY", "HADISABLED"] // HA Role must be primary or HA must be disabled
                    } 
                ]
            }
        },
        preferences: {
            uploadedTopic: "preferencesUploaded",
            updatedTopic: "preferencesUpdated"
        },
        errorhandling: Utils.errorhandling,
        serverStatus: {
            statusChangedTopic: "global/serverStatus",
            adminEndpointStatusChangedTopic: "global/adminEndpointStatusChanged",  // service/status result is published on this topic
            clusterStatusChangedTopic: "global/clusterStatusChanged"  // monitor/Cluster result is published on this topic
        },
        mqConnectivityStatus: {
            statusChangedTopic: "global/mqConnectivityStatus"
        },
        snmpStatus: {
            statusChangedTopic: "global/snmpStatus"
        },
        monitoring: {
            stopMonitoringTopic: "global/monitoring/stop",
            reinitMonitoringTopic: "global/monitoring/reinit",
            serverDataTopic: "global/monitoring/serverData",
            storeDataTopic: "global/monitoring/storeData",
            memoryDataTopic: "global/monitoring/memoryData",
            memoryHistoryDataTopic: "global/monitoring/memoryHistoryData",
            memoryDetailHistoryDataTopic: "global/monitoring/memoryDetailHistoryData",          
            storeHistoryDataTopic: "global/monitoring/storeHistoryData",
            storeDetailHistoryDataTopic: "global/monitoring/storeDetailHistoryData",                        
            uptimeDataTopic: "global/monitoring/uptimeData",
            connectionDataTopic: "global/monitoring/connectionData",
            throughputDataTopic: "global/monitoring/throughputData",
            snmpCommunitiesTopic: "global/monitoring/snmpCommunitiesChanged",
            snmpTrapSubscribersTopic: "global/monitoring/snmpTrapSubscribersChanged"
        },
        contextChange: {
            nodeNameTopic: "global/contextChange/nodeName",
            serverUIDTopic: "global/contextChange/serverUID"
        }
    };
    
    return Resources;
});
