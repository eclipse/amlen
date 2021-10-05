/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
		'dojo/aspect',
        'dojo/_base/json',
		'dojo/Deferred',
        'dojo/dom-construct',
		'dojo/query',
        'dojo/number',
        'dojo/dom',
        'dojo/on',
        'dojo/dom-style',
		'dojo/when',
        "dijit/registry",
        'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
	    'dijit/form/Button',
        'ism/widgets/TextBox',
        'ism/config/Resources',
        'ism/IsmUtils',
		'ism/widgets/Dialog',
		'ism/controller/content/ClusterMembershipDialog',
        'idx/string',
        'dojo/i18n!ism/nls/clusterMembership',
        'dojo/text!ism/controller/content/templates/ClusterMembership.html'
        ], function(declare, lang, aspect, json, Deferred, domConstruct, query, number, dom, on, domStyle, when, registry, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
        		Button, TextBox, Resources, Utils, Dialog, ClusterMembershipDialog, iString, nls, template) {	

    var ClusterMembership = declare("ism.controller.content.ClusterMembership",  [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		
		// 
		// description:
		//	
		nls: nls,
		templateString: template,
		restURI: Utils.getBaseUri() + "rest/config/cluster/clusterMembership/", 
		defaultControlPort: 9104,
		defaultMessagingPort: 9105,
		defaultDiscoveryPort: 9106,
        domainUid: 0,
        clusterMembership: {},
        lastClusterState: undefined,
        status: {
            configured: false,
            enabled: false,
            member: false,
            state: "unknown",
            tagline: nls.statusNotConfigured,
            membership: nls.notConfigured},
        defaults: {
            ControlExternalAddress: "ControlAddress",
            ControlExternalPort: "ControlPort",
            MessagingAddress: "ControlAddress",
            MessagingExternalAddress: "MessagingAddress",
            MessagingExternalPort: "MessagingPort"
        },
        haInfo: {
        	active: false,
        	standby: false,
        	primary: false
        },
        waitingForRestart: false,

		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);	
		},

		postCreate: function(args) {			
			this.inherited(arguments);
			this.domainUid = ism.user.getDomainUid();
            // Add the domain Uid to the REST API.
            this.restURI += this.domainUid;
			
			// initialize by getting the configuration and ha role
			this._initStatusAndConfig();
			this._initDialogs();
			this._initEvents();			
		
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.cluster.nav.cluster.topic, lang.hitch(this, function(message){
				this._initStatusAndConfig();
			}));
			
	         // subscribe to server status changes
            dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
                // Cluster information in status:  ClusterStatus, ClusterName, ServerName, ConnectedServers, DisconnectedServers
                if (!data || !data.Cluster) {
                    return;
                }
                var status = data.Cluster;
                
                if (data.HighAvailability && data.HighAvailability.Status) {
                	if (data.HighAvailability.Status.toLowerCase() === "active") {
                		this.haInfo.active = true;
                		if (data.HighAvailability.NewRole) {
                			if (data.HighAvailability.NewRole.toLowerCase() === "standby") {
                				this.haInfo.standby = true;
                				this.haInfo.primary = false;
                			} else if (data.HighAvailability.NewRole.toLowerCase() === "primary") {
                				this.haInfo.standby = false;
                				this.haInfo.primary = true;
                			} else {
                				this.haInfo.standby = false;
                				this.haInfo.primary = true;                				
                			}
                		}
                	} else {
                		this.haInfo = {active: false, standby: false, primary: false};
                	}
                }
                this.haStandby = data.HighAvailability && data.HighAvailability.NewRole ? data.HighAvailability.NewRole === "STANDBY" : false;
                if (status.Status && status.Status.toLowerCase() != this.lastClusterState) {
                	var clstate = status.Status.toLowerCase();
                	var needreload = ((this.lastClusterState !== undefined) && (!this.waitingForRestart));
                    this.lastClusterState = clstate;
                    this.updateStatusSection(status);
                    if (needreload) {
                    	window.location.reload(true);
                    }
                }
            }));

			
		},
		
		_initStatusAndConfig: function() {
		    var _this = this;
		    Utils.updateStatus();
            
		    var endpoint = ism.user.getAdminEndpoint();
            var options = endpoint.getDefaultGetOptions();
            var promise = endpoint.doRequest("/configuration/ClusterMembership", options);
            promise.then(
                function(data) {
                    var clusterMembership = data.ClusterMembership;
                    if (!clusterMembership) {
                        Utils.showPageError(nls.errorGettingClusterMembership);
                        return;
                    }
                    // handle booleans that came in as strings                    
                    Utils.convertStringsToBooleans(clusterMembership, [{property: "EnableClusterMembership", defaultValue: false},
                                                                       {property: "UseMulticastDiscovery", defaultValue: true}, 
                                                                       {property: "MessagingUseTLS", defaultValue: false}]);                    
                    _this.clusterMembership = clusterMembership;
                    _this._updatePage();                        
                },
                function(error) {
                    Utils.showPageErrorFromPromise(nls.errorGettingClusterMembership, error);
                }
            );
		},
		
		join: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();
            var _this = this;
            var promise =  _this.setEnabledState(true);
            promise.then(function() {
                _this.restartInfoDialog.show();
            }, function(err) {
                Utils.showPageErrorFromPromise(nls.saveClusterMembershipError, err);
                _this._initStatusAndConfig();
            });
		},
		
		leave: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();
            this.confirmLeave(Utils.updateStatus());
		},
		
		confirmLeave: function(onFinish) {
		    
		    if (!this.confirmLeaveDialog) {
		        // create restart info dialog
		        domConstruct.place("<div id='confirmLeave"+this.id+"Dialog'></div>", this.id);
		        var confirmLeaveDialogId = "confirmLeave"+this.id+"Dialog";
		        var _this = this;
		        this.confirmLeaveDialog = new Dialog({
		            id: confirmLeaveDialogId,
		            title: nls.leaveTitle + " (" + nls.restartTitle + ")",
		            content: "<div>" + nls.leaveContent + "</div>",
		            closeButtonLabel: nls.cancelButton,
		            onFinishHandler: onFinish,
		            buttons: [
		                      new Button({
		                          id: confirmLeaveDialogId + "_button_leave",
		                          label: nls.leaveButton + " (" + nls.restartButton + ")",
		                          onClick: lang.hitch(this, function() {
		                        	  //this.restartInfoDialog.show();
		                              this._leave(this.confirmLeaveDialog.onFinishHandler);
		                          })
		                      })
		                      ],
		                      style: 'width: 600px;'
		        }, confirmLeaveDialogId);
		        this.confirmLeaveDialog.dialogId = confirmLeaveDialogId;
		        this.confirmLeaveDialog.leaveButton = dijit.byId(confirmLeaveDialogId + "_button_leave");
		    } else {
		        var bar = query(".idxDialogActionBarLead",this.confirmLeaveDialog.domNode)[0];
		        bar.innerHTML = "";
		    }
		    this.confirmLeaveDialog.onFinishHandler = onFinish;
		    this.confirmLeaveDialog.show();
		},
		
		_leave: function(onFinish) {
		    var _this = this;
		    var bar = query(".idxDialogActionBarLead",this.confirmLeaveDialog.domNode)[0];
		    bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;

            var promise =  _this.setEnabledState(false);
            promise.then(function() {
                _this.status.enabled = false;
                _this.status.member = false;
                _this.updateStatusSection();
                bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.restarting;
                _this.waitingForRestart = true;
                if (onFinish) {
                	onFinish();
                }
                var t = _this;
                _this._waitForRestart(t.confirmLeaveDialog).then(function(){
                	t.confirmLeaveDialog.hide();
                	t.waitingForRestart = false;
                });
            }, function(err) {
                bar.innerHTML = "";
                _this.confirmLeaveDialog.showErrorFromPromise(nls.saveClusterMembershipError, err);                
                _this._initStatusAndConfig();
            });		    
		},
		
		editConfig: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();
		    this.editDialog.show(this.clusterMembership, this.status, this.haInfo);
		},	        
		
		resetConfig: function(event) {
		    
		    if (!this.resetInfoDialog) {
    	          // create restart info dialog
                domConstruct.place("<div id='resetInfo"+this.id+"Dialog'></div>", this.id);
                var resetDialogId = "resetInfo"+this.id+"Dialog";
                this.resetInfoDialog = new Dialog({
                    id: resetDialogId,
                    title: nls.resetTitle,
                    content: "<div>" + nls.resetContent + "</div>",
                    closeButtonLabel: nls.cancelButton,
                    buttons: [
                        new Button({
                            id: resetDialogId + "_button_reset",
                            label: nls.resetButton,
                            onClick: lang.hitch(this, function() {
                                this._reset();
                            })
                        })
                    ],
                    style: 'width: 600px;'
                }, resetDialogId);
                this.resetInfoDialog.dialogId = resetDialogId;
                this.resetInfoDialog.resetButton = dijit.byId(resetDialogId + "_button_reset");
		    }
		    this.resetInfoDialog.show();
		},
		
		_reset: function() {
            var dialog = this.resetInfoDialog;
            var bar = query(".idxDialogActionBarLead",dialog.domNode)[0];
            var _this = this;
            bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.resetting;
            var values = {
                    EnableClusterMembership: false,
                    ClusterName: "", 
                    UseMulticastDiscovery: true,
                    MulticastDiscoveryTTL: 1,
                    DiscoveryServerList: null, 
                    ControlAddress: null,
                    ControlPort: _this.defaultControlPort,
                    ControlExternalAddress: null, 
                    ControlExternalPort: null, 
                    MessagingAddress: null, 
                    MessagingPort: _this.defaultMessagingPort,
                    MessagingExternalAddress: null, 
                    MessagingExternalPort: null,
                    MessagingUseTLS: false,
                    DiscoveryTime: 10,
                    DiscoveryPort: _this.defaultDiscoveryPort};
            
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("ClusterMembership", values);
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(
                function(data) {
                    bar.innerHTML = "";
                    dialog.hide();
                    _this._resetPage(values);
                    _this._initStatusAndConfig();  
                },
                function(error) {
                    Utils.showPageErrorFromPromise(nls.saveClusterMembershipError, error);
                    bar.innerHTML = "";
                    dialog.hide();
                }
            );            
		},

		_updatePage: function() {
		    this.updateStatusSection();
		    this.updateConfigurationSection();
		},
		
		_resetPage: function(initialValues) {
		    if (!initialValues) {
		        return;
		    }
		    for (var prop in initialValues) {
		        if (initialValues[prop] === "") {
		            initialValues[prop] = nls.notSet;
		        }
		    }
		    this.updateConfigurationSection(initialValues);
		},
		
		updateStatusSection: function(clusterStatus) {
            var status = this.status;
            var cm = this.clusterMembership;
            var wasMember = status.member === true;
            status.enabled = cm.EnableClusterMembership === true;
            status.configured = this._haveValues([cm.ClusterName, cm.ControlAddress]);
            status.member = status.enabled;
            if (clusterStatus && clusterStatus.Status) {
                // set member and state
                status.state = clusterStatus.Status.toLowerCase();
                switch (status.state) {
                case "inactive":
                case "removed":
                    status.member = false;                    
                    break;
                case "discover":
                case "active":
                    status.member = true;
                    break;
                case "error": 
                case "initializing":
                case "unavailable" :
                    console.debug("ClusterStatus is " + status.state);
                    break;
                default:
                    status.state = "unknown";
                    break;
                }
                this.stateValue.innerHTML = nls[status.state];
            }
            var displayJoin = 'none';            
            var displayLeave = 'none';
            var joinText = "";
            if (status.configured) {
                var clusterName = cm.ClusterName;
                if (status.enabled && status.member) {
                    status.tagline = lang.replace(nls.statusMember, [clusterName]);
                    status.membership = lang.replace(nls.member, [clusterName]);
                    if (!this.haInfo.standby) {
                        displayLeave = 'inline';
                    }
                } else {
                    status.tagline = lang.replace(nls.statusNotAMember, [clusterName]);                                         
                    status.membership = nls.notAMember;
                    displayJoin = 'inline';
                    joinText = lang.replace(nls.joinLink, [clusterName]);
                }
            } else {
                status.tagline = nls.statusNotConfigured;                                          
                status.membership = nls.notConfigured;
            }
            
            this.statusTagline.innerHTML = status.tagline;
            this.statusValue.innerHTML = status.membership;
            this.joinLink.style.display = displayJoin;
            // if join is showing, also show reset
            this.resetLink.style.display = displayJoin;
            this.joinLink.innerHTML = joinText;
            this.leaveLink.style.display = displayLeave;           		   
		},
		
		updateConfigurationSection: function(cm) {
		    if (!cm) {
		        cm = this.clusterMembership;
		    }
		    var value, prop;
            for (prop in cm) {
                value = cm[prop];
                this._updateTd(prop, value);                
            }
            for (prop in this.defaults) {                
                if (!this._hasValue(cm[prop])) {
                    value = cm[this.defaults[prop]];
                    if (value === undefined && this.defaults[this.defaults[prop]]) {
                        value = cm[this.defaults[this.defaults[prop]]];
                    }
                    this._updateTd(prop, value);                                    
                 }
            }
		},
		
		_updateTd: function(prop, value) {
            var td = this[prop];
            if (td !== undefined && value !== undefined) {
                if (prop == "UseMulticastDiscovery" || prop == "MessagingUseTLS") {
                    var icon = value ? "check.png" : "cross.png";
                    var title = value ? nls.yes : nls.no;
                    value = "<img src='css/icons/images/"+icon+"' width='10' height='10' alt=''/> " + title;
                }
                if (prop === "DiscoveryTime" && value !== undefined) {
                	value = lang.replace(nls.seconds,[value]);
                }
                td.innerHTML = value;
            }		    
		},
		
		_haveValues: function(propArray) {
		    if (!propArray) return false;
		    for (var i = 0, len = propArray.length; i < len; i++) {
		        if (!this._hasValue(propArray[i])) {
		            return false;
		        }
		    }
		    return true;
		},
		
		_hasValue: function(prop) {
		    return prop !== undefined && prop !== "" && prop !== -1;
		},
		
		setEnabledState: function(/*boolean*/ enabled) {
		    var values = this.clusterMembership;
		    values.EnableClusterMembership = enabled;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("ClusterMembership", values);
            return adminEndpoint.doRequest("/configuration", options);
		},
		
		_initDialogs: function() {
			
			var editId = "edit"+this.id+"Dialog";
			var editDialogNode = editId + "Node";
        	var unit = lang.replace(nls.seconds,[""]);
			domConstruct.place("<div id='"+editDialogNode+"'></div>", this.id);
			
			this.editDialog = new ClusterMembershipDialog({dialogId: editId, parent: this, unit: unit}, editDialogNode);
			this.editDialog.startup();
			
			// create restart info dialog
			domConstruct.place("<div id='restartInfo"+this.id+"Dialog'></div>", this.id);
			var restartDialogId = "restartInfo"+this.id+"Dialog";
			this.restartInfoDialog = new Dialog({
				id: restartDialogId,
				title: nls.restartTitle,
				content: "<div>" + nls.restartContent + "</div>",
				closeButtonLabel: nls.cancelButton,
				buttons: [
					new Button({
						id: restartDialogId + "_button_restart",
						label: nls.restartButton,
						onClick: lang.hitch(this, function() {
							this._restart();
						})
					})
				],
				style: 'width: 600px;'
			}, restartDialogId);
			this.restartInfoDialog.dialogId = restartDialogId;
			this.restartInfoDialog.restartButton = dijit.byId(restartDialogId + "_button_restart");
		},
		
		_restart: function(dialog) {		    
		    if (!dialog) {
		        dialog = this.restartInfoDialog;		    
		    }
			var bar = query(".idxDialogActionBarLead",dialog.domNode)[0];
			var _this = this;
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.restarting;
			Utils.restartMessagingServer(true, function() {                      
			    bar.innerHTML = "";
			    dialog.hide();
			    Utils.updateStatus();
			    _this._initStatusAndConfig();
			}, function(error, ioargs) {
                Utils.showPageError(nls.restartingFailed, error, ioargs);
			});                 
		},
		
		_waitForRestart: function() {		    
			var _this = this;
			var promise = new Deferred();
			Utils.waitForRestart(10, function() {                      
			    Utils.updateStatus();
			    _this._initStatusAndConfig();
			    promise.resolve();
			});                 
			return promise.promise;
		},

		_showRestartRequired: function() {
		    if (!this.restartRequiredDialog) {
	            // create restart required dialog
	            domConstruct.place("<div id='restartRequired"+this.id+"Dialog'></div>", this.id);
	            var restartRequiredDialogId = "restartRequired"+this.id+"Dialog";
	            this.restartRequiredDialog = new Dialog({
	                id: restartRequiredDialogId,
	                title: nls.restartRequiredTitle,
	                content: "<div>" + nls.restartRequiredContent + "</div>",
	                closeButtonLabel: nls.restartLaterButton,
	                buttons: [
	                    new Button({
	                        id: restartRequiredDialogId + "_button_restart",
	                        label: nls.restartNowButton,
	                        onClick: lang.hitch(this, function() {
	                            var dialog = dijit.byId(restartRequiredDialogId);
	                            this._restart(dialog);
	                        })
	                    })
	                ],
	                style: 'width: 600px;'
	            }, restartRequiredDialogId);
	            this.restartRequiredDialog.dialogId = restartRequiredDialogId;
	            this.restartRequiredDialog.restartButton = dijit.byId(restartRequiredDialogId + "_button_restart");		        
		    }
		    this.restartRequiredDialog.show();
		},
		
		_initEvents: function() {		
		    var _this = this;
			dojo.subscribe(_this.editDialog.dialogId + "_saveButton", function(message) {
				if (_this.editDialog.form.validate()) {
				    _this.editDialog.save(function(values) {
				        _this.editDialog.hide();
				        _this._initStatusAndConfig();
				        if (_this.clusterMembership.EnableClusterMembership === true) {
				            _this._showRestartRequired();
				        }
				    });
				}
			});
		}
	});
	
	return ClusterMembership;
});
