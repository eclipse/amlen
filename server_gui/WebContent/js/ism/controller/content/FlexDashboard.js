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
define(['dojo/_base/declare',
        'dojo/_base/lang',
		'dojo/_base/xhr',
		'dojo/_base/json',
		'dojo/date',
		'dojo/dom',
		'dojo/dom-class',
		'dojo/dom-style',
		'dojo/dom-construct',
		'dojox/html/entities',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',				
		'ism/widgets/ToggleContainer',
		'idx/widget/SingleMessage',
		'ism/config/Resources',
		'dojo/i18n!ism/nls/home',
		'dojo/i18n!ism/nls/clusterStatistics',
		'ism/controller/content/BigStat',
		'ism/controller/content/SystemResourcesPane',		
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/text!ism/controller/content/templates/FlexDashboardSection.html',
		'dojo/text!ism/controller/content/templates/ChartSection.html',
		'ism/controller/content/FlexChartD3'
		], function(declare, lang, xhr, json, date, dom, domClass, domStyle, domConstruct, entities,
				    _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, ToggleContainer, SingleMessage,
				    Resources, nls, nlsClusterStats, BigStat, SystemResourcesPane, Utils, MonitoringUtils, sectionTemplate, chartTemplate, FlexChartD3) {
	
	/**
	 * Creates a section of the flex dashboard, based on the section definition provided on the
	 * constructor.  Sections are defined in Resources.flexDashboard.availableSections.
	 */
	var FlexDashboardSection = declare("ism.controller.content.FlexDashboardSection", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {

		title: "title is undefined",
		styleClass: "flexTile_innerBorder",
		availableTiles: {},
		defaultTiles: [],
		nls: nls,
		//templateString: sectionTemplate,
		sectionContainerId: undefined,
		flexDashboard: undefined,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);			
			this.inherited(arguments);
			this.sectionContainerId = this.id + "_sectionContainer";
		},		

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate() for " + this.id);
			this.inherited(arguments);
		},

		startup: function() {
			console.debug(this.declaredClass + ".startup() for " + this.id);
			this.inherited(arguments);	
			this._createSection();
		},
		
		_createSection: function() {
			// Future TODO: see if there are user specific tiles. For now
			// we use the defaultTiles
			var tiles = this.defaultTiles;
			var lastTile = tiles.length - 1;
			var isVirtual = ism.user.isVirtual();
			var isTight = this.styleClass.indexOf("_tight") > -1;
			var len = tiles ? tiles.length : 0;
			for (var i=0; i < len; i++) {
				var tileDefinition = this.availableTiles[tiles[i]];
				if (!tileDefinition) {
					console.debug("couldn't find tile " + tiles[i]);
					continue;
				}
				if (isVirtual && tileDefinition.hideOnVirtual) {
					continue;
				}				
				// create a cell for the tile
				var cssClass = this.styleClass;
				if (cssClass !== "flexTile_zeroPadding") {				    
				    if (i == lastTile) {
				       cssClass = isTight ? "flexTile_noBorder_tight" : "flexTile_noBorder";
				    } else if (i < 1 && !isTight) {
						cssClass += " flexTile_firstTile";
					}
				}
				var tileId = this.id + "_tile" + i;
				domConstruct.place("<td class='"+cssClass+"' id='"+tileId+"'></td>", this.sectionContainerId);
				// instantiate the widget for the tile
				var klass = lang.getObject(tileDefinition.widget);
				var tileInstance = new klass(tileDefinition.options);
				tileInstance.set("flexDashboard", this.flexDashboard);
				// place it in the cell and start it up
				domConstruct.place(tileInstance.domNode, tileId);
				tileInstance.startup();
			}
		}
		
	});

	/**
	 * Creates the flex dashboard by instantiating a FlexDashboardSection for each desired
	 * section, as defined in Resources.flexDashboard.
	 */
	var FlexDashboard = declare("ism.controller.content.FlexDashboard", [ToggleContainer], {

		nls: nls,
		dashboardKey: "flex",
		nodeName: undefined,
		timeout: 3000,
		lastServerStatus: 99,
		lastHaRole: null,
		serverUpTimeInSeconds: null,
		serverUpTime: null,
		timeOffset: 0,
		isStopped: true,
		storeDataInterval: -1,
		memoryDataInterval: -1,
		memoryHistoryDataInterval: -1,
		memoryDetailHistoryDataInterval: -1,		
		serverDataInterval: -1,
		uptimeDataInterval: -1,
		historyDataInterval: -1,
		throughputDataInterval: -1,
		duration: Resources.flexDashboard.duration,
		numPoints: Resources.flexDashboard.numPoints,
		storeHistoryDataInterval: -1,
		storeDetailHistoryDataInterval: -1,
		monitorHandles: {},
		warningMessage: undefined,
		sectionContainerDivId: undefined,
		historicalSectionTitleNode: undefined,
		sectionTemplate: sectionTemplate,
		chartTemplate: chartTemplate,
		clusterChartTitle: nls.home.dashboards.flex.throughput.title,
		
		domainUid: 0,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.triggerId = "dashboard";
			if (!this.isClusterStats) {
				this.title = Resources.flexDashboard.title;
			}
			
			this.noToggle = true;
			
		    this.domainUid = ism.user.getDomainUid();
					
			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				this._handleServerStatus(data);
			}));
			
	         Utils.updateStatus(); // Request the status so we can get the uptime	            
		},

		postCreate: function() {
			this.inherited(arguments);
			if (!this.isClusterStats) {
				this.toggleNode.innerHTML = "<span class='tagline'>"+nls.home.dashboards.tagline+"</span><br />";		
			}
			this._initDashboardContent();			
			this._startMonitoring();		
		},
		
		
		/**
		 * To turn monitoring of serverData on, set the interval to any positive number. Individual
		 * tiles are not allowed to specify the interval because this metric
		 * is used by multiple tiles.  To turn it off, which should normally
		 * not be done by an individual tile, set to -1.
		 */
		_setServerDataIntervalAttr: function(/*Number*/ interval) {
			if (interval === -1) {
				this.serverDataInterval = -1;
			} else {
				this.serverDataInterval = Resources.flexDashboard.serverDataInterval;				
			}
		},
		
		/**
		 * To turn monitoring of historyData on, set the interval any positive number. Individual
		 * tiles are not allowed to specify the interval because this metric
		 * is used by multiple tiles.  To turn it off, which should normally
		 * not be done by an individual tile, set to -1.
		 */
		_setHistoryDataIntervalAttr: function(/*Number*/ interval) {
			if (interval === -1) {
				this.historyDataInterval = -1;
			} else {
				this.historyDataInterval = Resources.flexDashboard.historyDataInterval;				
			}
		},
		
		_handleServerStatus: function(data) {
			console.debug(this.declaredClass + "._handleServerStatus("+data.RC+")");
			var statusRC = data.RC;
			var harole = data.harole;
			if (this.lastServerStatus != statusRC || this.lastHaRole != harole) {
				console.debug("Server status changed");
				if (Utils.isWarnMode(statusRC) || Utils.isErrorMode(statusRC) || Utils.isPrimarySynchronizing(harole)) {
					console.debug("Server down/warn, stopping charts");
					this.isStopped = true;
					this._stopMonitoring();
					this._showWarningMessage(nls.home.dashboards.monitoringNotAvailable);
					MonitoringUtils.clearMonitoringPageError();
				} else if (this.lastServerStatus !== null && this.lastServerStatus !== undefined) {
					console.debug("Server up, resuming chart");
					this.isStopped = false;
					this._hideWarningMessage();
					MonitoringUtils.clearMonitoringPageError();
					dojo.publish(Resources.monitoring.reinitMonitoringTopic, null);
					this._startMonitoring();					
				}
				this.lastServerStatus = statusRC;
				this.lastHaRole = harole;
			}
			var uptime = data.Server && data.Server.UpTimeSeconds !== undefined ? data.Server.UpTimeSeconds : undefined;
			if (!this.isStopped && uptime) {				
				this.serverUpTimeInSeconds = uptime;
				this.serverUpTime = date.add(new Date(), "second", -1*uptime);
				this._monitorUptime();
			}
			
// TODO: We should not show the dashboard when we are not connected to the server.  In the meantime, assure
//       the name that appears for the dashboard is the currently selected serverName
			var name = data.Server && data.Server.Name ? data.Server.Name : ism.user.serverName;
			if (!this.isClusterStats) {
				if (this.toggleTitle && (name !== undefined)) {
		              this.nodeName = name;
		              this.toggleTitle.innerHTML = lang.replace(Resources.flexDashboard.titleWithNodeName, [entities.encode(name)]);
				}
			} else {
				if (this.toggleTitle) {
					this.toggleTitle.innerHTML = "<div>"+nlsClusterStats.chartTitle+"</div>";
				}
			}
//			// update server name
//			var name = data.Server && data.Server.Name && data.Server.Name !== "(null)"? data.Server.Name : "";			
//            if (this.toggleTitle && name != this.nodeName) {
//                this.nodeName = name;
//                this.toggleTitle.innerHTML = lang.replace(Resources.flexDashboard.titleWithNodeName, [entities.encode(name)]);
//            }
		},
		
		_updateTitle: function() {
            var t = this;

//            // If we are in clusterStats, use throughput title instead of standard title
//            if (t.clusterStats) {
//            	
//            }
		    // if we have a serverName, use it
		    if (ism.user.serverName) {
		        t.nodeName = ism.user.serverName;
                if (t.toggleTitle) {                
                    t.toggleTitle.innerHTML = lang.replace(Resources.flexDashboard.titleWithNodeName, [entities.encode(t.nodeName)]);
                }
		        return;
		    }

		},

		_initDashboardContent: function() {
			if (!this.isClusterStats) {
				this._updateTitle();
			}
			var flexDashboard = Resources.flexDashboard; 
			var sections = flexDashboard.defaultSections;
//			if (this.defaultSections)
//				sections = this.defaultSections;
			if (this.isClusterStats) {
				sections = flexDashboard.clusterSections;
			}
			var dashboardDivId = this.id+"_dashboardDiv";
			domConstruct.place("<div id='"+dashboardDivId+"' style='width:100%'></div>", this.toggleNode);
			this.warnAreaId = this.id+"_warnArea";
			domConstruct.place("<span id='"+this.warnAreaId+"'></span>", dashboardDivId);
			this.sectionContainerDivId = this.id+"_sectionContainerDiv";
			domConstruct.place("<div id='"+this.sectionContainerDivId+"' style='width:100%; display:table;'></div>", dashboardDivId);
			var isVirtual = ism.user.isVirtual();
			var len = sections ? sections.length : 0;
			for (var i=0; i < len; i++) {
				var sectionDefinition = flexDashboard.availableSections[sections[i]];
				if (!sectionDefinition) {
					console.debug("couldn't find section " + sections[i]);
					continue;
				}
				if (this.isClusterStats) {
					sectionDefinition.defaultTiles = sectionDefinition.clusterTiles;
					sectionDefinition.templateString = chartTemplate;
					sectionDefinition.title = this.clusterChartTitle;
				} else {
					sectionDefinition.templateString = sectionTemplate;
				}
				if (isVirtual && sectionDefinition.hideOnVirtual) {
					continue;
				}								
				var sectionId = this.id + "section" + i;
				domConstruct.place("<div id='"+sectionId+"'></div>", this.sectionContainerDivId);
				var section = new FlexDashboardSection(sectionDefinition, sectionId);
				section.set("flexDashboard", this);	
				section.startup();
			}
		},
		
		_startMonitoring: function() {
			if (this.isStopped) {
				return;
			}
			this._monitorThroughputData();
			if (!this.isClusterStats) {
				this._monitorConnectionData();
				this._monitorStoreData();
				this._monitorMemoryData();
				this._monitorServerData();
				this._monitorUptime();
				this._monitorMemoryHistoryData();
				this._monitorStoreHistoryData();
				this._monitorMemoryDetailHistoryData();	
				this._monitorStoreDetailHistoryData();
			}
		},
		
		_stopMonitoring: function() {
			dojo.publish(Resources.monitoring.stopMonitoringTopic, null);
			for (var handle in this.monitorHandles) {
				window.clearTimeout(this.monitorHandles[handle]);
			}
		},
		
		_monitorStoreData: function() {
			if (this.isStopped || this.storeDataInterval < 0) {
				return;
			}
			var storeData = MonitoringUtils.getStoreData();
			storeData.then(lang.hitch(this,function(data) {
	    		if (!this.isStopped) {
					dojo.publish(Resources.monitoring.storeDataTopic, data.Store);
					this.monitorHandles.storeData = setTimeout(lang.hitch(this,function(){
						this._monitorStoreData();
					}), this.storeDataInterval);
	    		}				
			}), function(err) {
				if (storeData.ioArgs) {
					var result = Utils.checkXhrError(storeData.ioArgs);
					if (result && result.reloading) {
						console.log("getStoreData onError returning because we are reloading");
						return;
					}
				}
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},
		
		_monitorMemoryData: function() {
			if (this.isStopped || this.memoryDataInterval < 0) {
				return;
			}
			var memoryData = MonitoringUtils.getMemoryData();
			memoryData.then(lang.hitch(this,function(data) {
	    		if (!this.isStopped) {
					dojo.publish(Resources.monitoring.memoryDataTopic, data.Memory);
					this.monitorHandles.memoryData = setTimeout(lang.hitch(this,function(){
						this._monitorMemoryData();
					}), this.memoryDataInterval);
	    		}				
			}), function(err) {
				if (memoryData.ioArgs) {
					var result = Utils.checkXhrError(memoryData.ioArgs);
					if (result && result.reloading) {
						console.log("getMemoryData onError returning because we are reloading");
						return;
					}

				}					
				MonitoringUtils.showMonitoringPageError(err);
				return;
			});
		},

		// Note: currently assumes we are getting MemoryUsedPercent only
		_monitorMemoryHistoryData: function() {
			if (this.memoryHistoryDataInterval < 0) {
				return;
			}

			MonitoringUtils.getMemoryHistory(86400, { metric: "MemoryUsedPercent", onComplete: lang.hitch(this, function(data) {
				dojo.publish(Resources.monitoring.memoryHistoryDataTopic, data);
				if (this.isStopped) {
	    			return;
	    		}
				this.monitorHandles.memoryHistoryData = setTimeout(lang.hitch(this,function(){
					this._monitorMemoryHistoryData();
				}), this.memoryHistoryDataInterval);
			})});			
		},
		
		_monitorStoreDetailHistoryData: function() {
			if (this.storeDetailHistoryDataInterval < 0) {
				return;
			}

			MonitoringUtils.getStoreDetailHistory(86400, { onComplete: lang.hitch(this, function(data) {
				if (this.isStopped) {
	    			return;
	    		}				
                dojo.publish(Resources.monitoring.storeDetailHistoryDataTopic, data);
				this.monitorHandles.storeDetailHistoryData = setTimeout(lang.hitch(this,function(){
					this._monitorStoreDetailHistoryData();
				}), this.storeDetailHistoryDataInterval);
			})});			
		},		

		_monitorMemoryDetailHistoryData: function() {
			if (this.memoryDetailHistoryDataInterval < 0) {
				return;
			}

			MonitoringUtils.getMemoryDetailHistory(86400, { onComplete: lang.hitch(this, function(data) {
				if (this.isStopped) {
	    			return;
	    		}				
                dojo.publish(Resources.monitoring.memoryDetailHistoryDataTopic, data);
				this.monitorHandles.memoryDetailHistoryData = setTimeout(lang.hitch(this,function(){
					this._monitorMemoryDetailHistoryData();
				}), this.memoryDetailHistoryDataInterval);
			})});			
		},		
		
		// Note: currently assumes we are getting StoreDiskUsagePct only
		_monitorStoreHistoryData: function() {
			if (this.storeHistoryDataInterval < 0) {
				return;
			}

			MonitoringUtils.getStoreHistory(86400, { metric: "DiskUsedPercent,MemoryUsedPercent", onComplete: lang.hitch(this, function(data) {
				if (this.isStopped) {
	    			return;
	    		}				
                dojo.publish(Resources.monitoring.storeHistoryDataTopic, data);
				this.monitorHandles.storeHistoryData = setTimeout(lang.hitch(this,function(){
					this._monitorStoreHistoryData();
				}), this.storeHistoryDataInterval);
			})});			
		},	

		_monitorServerData: function() {
			if (this.isStopped || this.serverDataInterval < 0) {
				return;
			}
	        MonitoringUtils.startStopServerMonitoring(true, this.serverDataInterval);	                
		},
	
		_monitorUptime: function() {
			if (this.isStopped || this.uptimeDataInterval < 0 || !this.serverUpTime) {
				return;
			}
			
			this.serverUpTimeInSeconds = date.difference(this.serverUpTime, new Date(), "second");
			dojo.publish(Resources.monitoring.uptimeDataTopic, {uptime: this.serverUpTimeInSeconds});
			if (this.serverUpTimeInSeconds < 3600) { // if less than 1 hour, update every second instead of every 30 seconds
				this.monitorHandles.uptimeData = setTimeout(lang.hitch(this,function(){				
					this._monitorUptime();
				}), 1000);
			}
		},
		
		_monitorThroughputData: function() {
			if (this.throughputDataInterval < 0) {
				return;
			}
			
	        MonitoringUtils.getThroughputHistory(this.duration, { metric: "Msgs", numPoints: this.numPoints, onComplete: lang.hitch(this, function(data) {
                if (data.error || (data.RC !== undefined && data.RC !== "0")) {
                    var error = data.error ? data.error : data;
                    MonitoringUtils.showMonitoringPageError(error);
                    return;
                }
                if (!this.isStopped) {
                    dojo.publish(Resources.monitoring.throughputDataTopic, data);
                    this.monitorHandles.throughputData = setTimeout(lang.hitch(this,function(){
                        this._monitorThroughputData();
                    }), this.throughputDataInterval);
                }
			})});
		},
		
	    _monitorConnectionData: function() {
	        if (this.connectionDataInterval < 0) {
	            return;
	        }

	        MonitoringUtils.getEndpointHistory(this.duration, { metric: "ActiveConnections", numPoints: this.numPoints, onComplete: lang.hitch(this, function(data) {
	            if (data.error || (data.RC !== undefined && data.RC !== "0")) {
	                var error = data.error ? data.error : data;
	                MonitoringUtils.showMonitoringPageError(error);
	                return;
	            }	            
                if (!this.isStopped) {
                    dojo.publish(Resources.monitoring.connectionDataTopic, data);
                    this.monitorHandles.connectionData = setTimeout(lang.hitch(this,function(){
                        this._monitorConnectionData();
                    }), this.connectionDataInterval);
                }
	        })});
        },

		_showWarningMessage: function(message) {
			if (this.warningMessage) {
				return;
			}			
			var messageDiv = domConstruct.create("div", null, this.warnAreaId, "first");
			this.warningMessage = new SingleMessage({
				type: 'warning',
				title: message,
				showRefresh: false,
				showAction: false,
				showDetails: false,
				showDetailsLink: false,
				dateFormat: {selector: 'time', timePattern: ism.user.localeInfo.timeFormatString},
				style: 'width: 98%'
			}, messageDiv);
			// scroll to top of attachPoint
			var node = dom.byId(this.warnAreaId);
			if (node) {
				node.scrollTop = 0;
			}
		},
		
		_hideWarningMessage: function() {
			if (this.warningMessage) {
				console.debug("destroying warning message as requested by _hideWarningMessage");
				this.warningMessage.destroy();
				this.warningMessage = undefined;
			}
		}

	});

	return FlexDashboard;
});
