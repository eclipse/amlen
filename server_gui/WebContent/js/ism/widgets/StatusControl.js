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
define(["dojo/_base/declare",
        "dojo/_base/lang",
        'dojo/aspect',
        'dijit/registry',
		'dijit/Menu',
		'dijit/MenuItem',
		"dijit/PopupMenuBarItem",
		'ism/config/Resources',
		'ism/IsmUtils',
        "dojo/i18n!ism/nls/home",
        "dojo/i18n!ism/nls/appliance"],
        function(declare, lang, aspect, registry, Menu, MenuItem, PopupMenuBarItem, Resources, Utils, nls, nlsa){
	
	/**
	 * Regularly monitors the server status, displaying it in a menu.
	 */
	var StatusControl = declare("ism.widgets.StatusControl", null , {	
	
		STATUS: {
			NORMAL: { value: 0, name: "normal", badge: "" },
			INFO: { value: 1, name: "info", badge: "<span class='idxBadgeInformation'>i</span>"},
			WARN: { value: 2, name: "warn", badge: "<span class='idxBadgeWarning'>!</span>" },
			ERROR: { value: 3, name: "error", badge: "<span class='idxBadgeError'>X</span>" }
		},
		
		statusUpdateInterval: 30000, // milliseconds
		initialUpdateInterval: 5000, // milliseconds
		messagingStatusItem: null,
		overallStatusItem: null,
		overallStatus: null,
		haRoleItem: null,
		currentHaRole: null,
		clusterItem: null,
		currentClusterStatus: null,
		currentClusterName: null,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);

			this.overallStatus = this.STATUS.NORMAL;
			this.currentClusterName = ism.user.getClusterName();
		},
		
		startMonitoring: function() {
			console.debug(this.declaredClass + ".startMonitoring");

			var menu = new Menu();
			aspect.after(menu,"onOpen", function() {
				dojo.publish("lastAccess", {requestName: "StatusControl.onOpen"});
			});
			this.messagingStatusItem = new MenuItem({ disabled: true, id: "statusMenu_ismServer", title:"", label: nls.home.statusControl.ismServer + " " + nls.home.statusControl.pending});
			menu.addChild(this.messagingStatusItem);
			this.haRoleItem = new MenuItem({ disabled: true, id: "statusMenu_haRole", title:"", label: nls.home.statusControl.haRole + " " + nls.home.statusControl.pending});
			menu.addChild(this.haRoleItem);
			
			this.clusterItem = new MenuItem({ disabled: true, id: "statusMenu_cluster", title:"", label: nls.statusControl_cluster + " " + nls.home.statusControl.pending});
			menu.addChild(this.clusterItem);
			
			this.overallStatusItem = new PopupMenuBarItem({ label: nls.home.statusControl.label, popup: menu, 'class': "ismStatusMenu", id: "ismStatusMenu" });
			
			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				this.showStatus(data);
			}));
			
		
			// schedule first update
			setTimeout(lang.hitch(this, function(){
				this.updateStatus();
			}), this.initialUpdateInterval);
			
			return this.overallStatusItem;
		},

		showStatus: function(data) {
			var statusRC = data.RC;
			var haRole = data.harole;
			
			if (haRole) {
				var pctSyncCompletion = data.HighAvailability ? data.HighAvailability.PctSyncCompletion : -1;
				this.showHARole(haRole, pctSyncCompletion);
			}
			
			if(data.Cluster) {
				var clusterstatus = data.Cluster.Status ? data.Cluster.Status : "";
				var clustername = data.Cluster.Name ? data.Cluster.Name : undefined;
				if (clustername !== this.currentClusterName) {
					ism.user.updateClusterName(ism.user.server, this.currentClusterName, clustername);
					this.currentClusterName = clustername;
				}
				this.showClusterStatus(clustername, clusterstatus);
			}
			
			var rc = "rc" + statusRC;
			var niceStatus = nls.home.status[rc];
			var newOverallStatus = this.STATUS.NORMAL;
			if (!niceStatus) {
				niceStatus = nls.home.status.unknown;
			}					
			
		    if (Utils.isErrorMode(statusRC) || Utils.isHAErrorMode(this.currentHaRole)) {
				newOverallStatus = this.STATUS.ERROR;
			} else if (Utils.isWarnMode(statusRC) || Utils.isHAWarnMode(this.currentHaRole)) {
				newOverallStatus = this.STATUS.WARN;
			} else if (Utils.isPrimarySynchronizing(haRole)) {
				newOverallStatus = this.STATUS.INFO;
			}
			
			this.messagingStatusItem.set("label", nls.home.statusControl.ismServer + " " + niceStatus);						
			if (this.overallStatus != newOverallStatus) {
				console.debug("change in status to: "+newOverallStatus.name);
				this.overallStatus = newOverallStatus;
				this.messagingStatusItem.set("title","");
				this.overallStatusItem.set("label", nls.home.statusControl.label + newOverallStatus.badge);
				if (newOverallStatus == this.STATUS.ERROR) {
					if (this.isAdmin) {
						this.messagingStatusItem.set("title", nls.home.statusControl.serverNotAvailable);
					} else {
						this.messagingStatusItem.set("title", nls.home.statusControl.serverNotAvailableNonAdmin);						
					}
				} 				
			}			
		},
	
		showHARole: function(haRole, pctSyncCompletion) {					
			if (this.currentHaRole != haRole || pctSyncCompletion > -1) {
				console.debug("change in HA Role to: "+haRole);
				this.currentHaRole = haRole;
				var niceStatus = nls.home.role[haRole];
				if (!niceStatus) {
					niceStatus = nls.home.role.unknown;
				} else if (Utils.isPrimarySynchronizing(haRole)) {
					niceStatus = lang.replace(niceStatus, [pctSyncCompletion]); 
				}				
				this.haRoleItem.set("title","");
				this.haRoleItem.set("label", nls.home.statusControl.haRole + " " + niceStatus);		
			}			
		},
		
		showClusterStatus: function(clustername, clusterstatus) {					
			if (clusterstatus && this.currentClusterStatus != clusterstatus) {
				console.debug("change cluster status to: "+clusterstatus);
				this.currentClusterStatus = clusterstatus;
				var clStatus = nls["clusterStatus_"+clusterstatus];
				if (!clStatus) {
					if (clusterstatus.toLowerCase() === "standby") {
						// Reuse appliance string because it is too late to do additional
						// translations.
						clStatus = nlsa.appliance.highAvailability.stateStandby;
					} else {
						clStatus = nls.clusterStatus_Unknown;
					}
				} 				
				this.clusterItem.set("title","");
				this.clusterItem.set("label", nls.statusControl_cluster + " " + clStatus + (clustername ? ("(" + clustername + ")") : ""));		
			}			
		},
		
		updateStatus: function() {
		    var _this = this;
			var scheduleUpdate = function() {
                setTimeout(lang.hitch(this, function(){
                    _this.updateStatus();
                }), _this.statusUpdateInterval);             			    
			};
            // the change in status will be published on the subscribed topic, so we just need to schedule
			// the next update here, whether it was successful or errored out
			Utils.updateStatus(scheduleUpdate, scheduleUpdate);
		}					
	});
	
	return StatusControl;
	
});
