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
define([
    'dojo/_base/declare',
    'dojo/_base/lang',
    'dojo/dom',
    'dojo/dom-style',
    'dojo/dom-construct',
    'dojo/number',
    'dojo/query',
	'dijit/_Widget',
	'dijit/_TemplatedMixin',
	'dijit/_WidgetsInTemplateMixin',		
	'dijit/layout/ContentPane',
	'dijit/form/Button',
	'dijit/form/Form',
    'dojo/text!ism/controller/content/templates/MessagingServerControl.html',
	'dojo/i18n!ism/nls/appliance',
	'dojo/i18n!ism/nls/home',
	'dojo/i18n!ism/nls/strings',
	'dojox/html/entities',
	'idx/string',
	'ism/IsmUtils',
	'ism/widgets/Dialog',
	'ism/widgets/Select',
	'ism/widgets/TextBox',
	'ism/config/Resources'
	], function(declare, lang, dom, domStyle, domConstruct, number, query, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, 
			ContentPane, Button, Form, template, nls, nlsHome, nlsStrings, entities, iString, Utils, Dialog, Select, TextBox, Resources) {
 
    var MessagingServerControl = declare("ism.controller.content.MessagingServerControl", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        
    	templateString: template,
    	nls: nls,
    	nlsHome: nlsHome,
    	restartDialog: null,
    	restartMaintOnDialog: null,
    	restartMaintOffDialog: null,
    	restartCleanStoreDialog: null,
    	stopDialog: null,
    	adminErrorDisplayed: false,
    	starting: false,
    	startTimeoutID: null,
    	stopping: false,
    	stopped: false,
    	restarting: false,
    	stopTimeoutID: null,
    	cleaningStore: false,
    	viewLicenseUri: "",
		licensedUsageValues: [],
    	servername: null,
		servernameDialog: null,
		currentStatus: {niceStatus: nlsHome.home.status.unknown, serverState: -1, unknownOrStopped: true},
    	
    	domainUid: 0,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function(args) {
			this.inherited(arguments);
			this.domainUid = ism.user.getDomainUid();
			
			this._initMessagingServerData();
			this._createConfigDialogs();
			this._createConfirmationDialogs();
			
			this.editLicensedUsageLink = dojo.byId(this.id+"_editLicensedUsage");
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.appliance.nav.systemControl.topic + ":onShow", lang.hitch(this, function(message){
				this._initMessagingServerData();
			}));	
			
			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				if (data) {
					this.currentStatus = this._updateServerStatus(data, this, this.id);
				}
			}));

		},
		
		_getServerStatus: function() {
			Utils.updateStatus();
			return this.currentStatus;
		},

		editServername: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();

			console.debug("Edit clicked");
			var servername = this.servername == nls.appliance.systemControl.appliance.hostnameNotSet ? "" : this.servername;
			this.servernameDialog.servernameField.set('value', servername);
			query(".idxDialogActionBarLead",this.servernameDialog.domNode)[0].innerHTML = "";
			this.servernameDialog.show();
		},

        editLicensedUsage: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();
            
            var licensedUsageDialog = this._getLicensedUsageDialog();
            licensedUsageDialog.licensedUsageField.set('value', this.licensedUsage);
            query(".idxDialogActionBarLead",licensedUsageDialog.domNode)[0].innerHTML = "";
            licensedUsageDialog.show();
        },

        _getLicensedUsageDialog: function() {
            
            if (this.licensedUsageDialog) {
                return this.licensedUsageDialog;
            }
            
            var licensedUsageDialogId = this.id+"_licensedUsageDialog";
            domConstruct.place("<div id='"+licensedUsageDialogId+"'></div>", this.id);
            var licensedUsageForm = new Form({id: this.id+'_licensedUsageForm'});
            var options = [];
            for (var i = 0, len=this.licensedUsageValues.length; i < len; i++) {
                var val = this.licensedUsageValues[i];
                switch (val) {
                case "Developers":
                    options.push({label: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageValues.developers), value: "Developers"});
                    break;
                case "Non-Production" :
                    options.push({label: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageValues.nonProduction), value: "Non-Production"});
                    break;
                case "Production" :
                    options.push({label: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageValues.production), value: "Production"});
                    break;
                default:
                    console.debug("unrecognized LicensedUsage value: " + val);
                    break;
                }
            }
            
            var licensedUsageField = new Select({
                    label: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageLabel)+":",
                    value: this.licensedUsage,
                    id: 'licensedUsageField',
                    width: '150px', 
                    labelAlignment: 'horizontal',
                    labelWidth: '125',
                    sortByLabel: false,
                    options: options,
                    onChange: lang.hitch(this, function() {this._changeLicensedUsage();})});
                        
            licensedUsageForm.containerNode.appendChild(domConstruct.create("div", { 
                innerHTML: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageDialog.content),
                style: "padding-bottom: 10px;"}));
            licensedUsageForm.containerNode.appendChild(licensedUsageField.domNode);
            this.licensedUsageForm = licensedUsageForm;
            
            this.licensedUsageDialog = new Dialog({
                id: licensedUsageDialogId,
                title: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageDialog.title),
                instruction: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageDialog.instruction),
                content: licensedUsageForm,
                style: 'width: 600px;', 
//                onEnter: lang.hitch(this, function() {this._saveLicensedUsage();}),
                buttons: [
                    new Button({
                        id: licensedUsageDialogId + "_button_ok",
                        label: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageDialog.saveButton)
//                        onClick: lang.hitch(this, function() {this._saveLicensedUsage();})
                    })
                ],
                closeButtonLabel: Utils.escapeApostrophe(nls.appliance.systemControl.appliance.licensedUsageDialog.cancelButton)
            }, licensedUsageDialogId);
            this.licensedUsageDialog.dialogId = licensedUsageDialogId;
            this.licensedUsageDialog.licensedUsageField = licensedUsageField;
            this.licensedUsageDialog.startup();
            // start with the save button disabled
            var button = dijit.byId(licensedUsageDialogId + "_button_ok");
            button.set('disabled',true);
            
            return this.licensedUsageDialog;
  
        },
        
        _changeLicensedUsage: function() {
            // don't allow submit if the licensed usage is unchanged, it will cause an error on the server
            var value = this.licensedUsageDialog.licensedUsageField.get("value");
            var button = dijit.byId(this.id+"_licensedUsageDialog_button_ok");
            if (this.licensedUsage == value) {
                button.set('disabled',true);
            } else {
                button.set('disabled', false);
            }
        },

// TODO: Add support for setting LicensedUsage from Web UI in V2.0 and later
//        _saveLicensedUsage: function() {
//            console.debug("Save licensedUsage clicked "+licensedUsage);
//            var licensedUsage = this.licensedUsageDialog.licensedUsageField.get("value"), 
//                form = this.licensedUsageForm;
//            if (!form.validate()) { return; }
//            var _this = this;
//            var bar = query(".idxDialogActionBarLead",_this.licensedUsageDialog.domNode)[0];
//            bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;                                      
//            dojo.xhrPut({
//                url: Utils.getBaseUri() + "rest/config/license/licensedUsage/"+encodeURIComponent(licensedUsage),
//                handleAs: "json",
//                load: lang.hitch(this,function(data) {
//                    // restart the appliance...
//                    Utils.showPageInfo(nls.appliance.systemControl.appliance.restartMessage, 
//                            nls.appliance.systemControl.appliance.restartMessageDetails);
//                    dojo.xhrPut({
//                        url: Utils.getBaseUri() + "rest/config/appliance/reboot",
//                        handleAs: "json",
//                        load: function(data) {                                                                      
//                            // if the redirect takes too long, IE thinks the script is misbehaving.
//                            // Return and run the rest of the script later
//                            setTimeout(function(){
//                                ism.user.logout();
//                            }, 10);                                 
//                        },
//                        error: function(error, ioargs) {
//                            bar.innerHTML = "";
//                            Utils.checkXhrError(ioargs);
//                            _this.licensedUsageDialog.showXhrError(nls.appliance.systemControl.appliance.restartError, error, ioargs);                
//                        }
//                    });
//
//                }),
//                error: function(error, ioargs) {
//                    Utils.checkXhrError(ioargs);
//                    bar.innerHTML = nls.appliance.savingFailed;
//                    _this.licensedUsageDialog.showXhrError(nls.appliance.systemControl.appliance.licensedUsageSaveError, error, ioargs);                
//                }
//            });
//        },

		_initMessagingServerData: function() {
			this.adminErrorDisplayed = false;
			this._fetchStatus(this.id, 1000, true);
			this._fetchServername();
		    this._fetchLicensedUsage();
		},
		
		_createConfigDialogs: function() {
			//
			// Edit servername dialog
			//
			var servernameDialogId = this.id+"_servernameDialog";
			domConstruct.place("<div id='"+servernameDialogId+"'></div>", this.id);
			var servernameForm = new Form({id: this.id+'_servernameForm'});
			var servernameField = new TextBox({
					label: nls.appliance.systemControl.appliance.hostnameDialog.hostname,
					value: this.servername,
					maxLength: 1024,
					id: 'servernameField',
					invalidMessage: nls.appliance.systemControl.appliance.hostnameDialog.invalid,
					tooltipContent: nls.appliance.systemControl.appliance.hostnameDialog.tooltip,
				    attachPoint: 'servernameField',
					width: '150px', 
					labelAlignment: 'horizontal',
					labelWidth: '110'});			
						
			servernameForm.containerNode.appendChild(servernameField.domNode);
			
			this.servernameDialog = new Dialog({
				id: servernameDialogId,
				title: nls.appliance.systemControl.appliance.hostnameDialog.title,
				instruction: nls.appliance.systemControl.appliance.hostnameDialog.instruction,
				content: servernameForm,
				onEnter: lang.hitch(this, function() {this._saveServername(servernameField.get("value"), servernameForm, servernameDialogId);}),
				buttons: [
					new Button({
						id: servernameDialogId + "_button_ok",
						label: nls.appliance.systemControl.appliance.hostnameDialog.okButton,
						onClick: lang.hitch(this, function() {this._saveServername(servernameField.get("value"), servernameForm, servernameDialogId);})
					})
				],
				closeButtonLabel: nls.appliance.systemControl.appliance.hostnameDialog.cancelButton
			}, servernameDialogId);
			this.servernameDialog.dialogId = servernameDialogId;
			this.servernameDialog.servernameField = servernameField;
			this.servernameDialog.startup();
		},
		
		_createConfirmationDialogs: function() {
			var statusId = this.id + "_status";
			var values = {};
			values.Service = "Server";
			var self = this;
			
			// stop confirmation dialog
			var stopDialogId = this.id+"_stopDialog";
			domConstruct.place("<div id='"+stopDialogId+"'></div>", this.id);
			var stopTitle = nls.appliance.systemControl.messagingServer.stopDialog.title;
			this.stopDialog = new Dialog({
				id: stopDialogId,
				title: stopTitle,
				content: "<div>" + nls.appliance.systemControl.messagingServer.stopDialog.content + "</div>",
				buttons: [
					new Button({
						id: stopDialogId + "_button_ok",
						label: nls.appliance.systemControl.messagingServer.stopDialog.okButton,
						onClick: function() { 
							        self._setStopping(true);
							        self._setStarting(false);
							        dojo.byId(statusId).innerHTML = nls.appliance.systemControl.messagingServer.stopping;
							        self._doStop(values, stopDialogId);
						    }
						})
				],
				closeButtonLabel: nls.appliance.systemControl.messagingServer.stopDialog.cancelButton
			}, stopDialogId);
			this.stopDialog.dialogId = stopDialogId;

            // restart confirmation dialog
            var restartDialogId = this.id+"_restartDialog";
            domConstruct.place("<div id='"+restartDialogId+"'></div>", this.id);
            var restartTitle = nls.restartDialogTitle;
            this.restartDialog = new Dialog({
                id: restartDialogId,
                title: restartTitle,
                content: "<div>" + nls.restartDialogContent + "</div>",
                buttons: [
                    new Button({
                        id: restartDialogId + "_button_ok",
                        label: nls.restartOkButton,
                        onClick: function() {
                        	dojo.byId(statusId).innerHTML = nls.restarting;
                        	this.set("style","display:none");
                        	self.restartDialog.closeButton.set("label",nlsStrings.action.Close);
                        	self._doRestart(true, values, restartDialogId);
                        }
                    })
                ],
                closeButtonLabel: nls.appliance.systemControl.messagingServer.stopDialog.cancelButton,
                onShow: function() {
            			query(".idxDialogActionBarLead",this.domNode)[0].innerHTML = "";
            			dijit.byId(restartDialogId + "_button_ok").set("style","display:inline");
                    	this.closeButtonLabel = nls.appliance.systemControl.messagingServer.stopDialog.cancelButton;
            			self.restartDialog.show();
                }
            }, restartDialogId);
            this.restartDialog.dialogId = restartDialogId;
			
            // start maintenance confirmation dialog
            var restartMaintOnDialogId = this.id+"_restartMaintOnDialog";
            domConstruct.place("<div id='"+restartMaintOnDialogId+"'></div>", this.id);
            var restartMaintOnTitle = nls.restartMaintOnDialogTitle;
            this.restartMaintOnDialog = new Dialog({
                id: restartMaintOnDialogId,
                title: restartMaintOnTitle,
                content: "<div>" + nls.restartMaintOnDialogContent + "</div>",
                buttons: [
                    new Button({
                        id: restartMaintOnDialogId + "_button_ok",
                        label: nls.restartOkButton,
                        onClick: function() {
                        	var rvalues = JSON.parse(JSON.stringify(values));
                        	rvalues.Maintenance = "start";
                        	dojo.byId(statusId).innerHTML = nls.restarting;
                        	this.set("style","display:none");
                        	self.restartMaintOnDialog.closeButton.set("label",nlsStrings.action.Close);
                        	self._doRestart(true, rvalues, restartMaintOnDialogId);
                        }
                    })
                ],
                closeButtonLabel: nls.appliance.systemControl.messagingServer.stopDialog.cancelButton,
                onShow: function() {
        			query(".idxDialogActionBarLead",this.domNode)[0].innerHTML = "";
        			dijit.byId(restartMaintOnDialogId + "_button_ok").set("style","display:inline");
                	this.closeButtonLabel = nls.appliance.systemControl.messagingServer.stopDialog.cancelButton;
        			self.restartMaintOnDialog.show();
            }
            }, restartMaintOnDialogId);
            this.restartMaintOnDialog.dialogId = restartMaintOnDialogId;
            
            // stop maintenance confirmation dialog
            var restartMaintOffDialogId = this.id+"_restartMaintOffDialog";
            domConstruct.place("<div id='"+restartMaintOffDialogId+"'></div>", this.id);
            var restartMaintOffTitle = nls.restartMaintOffDialogTitle;
            this.restartMaintOffDialog = new Dialog({
                id: restartMaintOffDialogId,
                title: restartMaintOffTitle,
                content: "<div>" + nls.restartMaintOffDialogContent + "</div>",
                buttons: [
                    new Button({
                        id: restartMaintOffDialogId + "_button_ok",
                        label: nls.restartOkButton,
                        onClick: function() {
                        	var rvalues = JSON.parse(JSON.stringify(values));
                        	rvalues.Maintenance = "stop";
                        	dojo.byId(statusId).innerHTML = nls.restarting;
                        	this.set("style","display:none");
                        	self.restartMaintOffDialog.closeButton.set("label",nlsStrings.action.Close);
                        	self._doRestart(true, rvalues, restartMaintOffDialogId);
                        }
                    })
                ],
                closeButtonLabel: nls.appliance.systemControl.messagingServer.stopDialog.cancelButton,
                onShow: function() {
        			query(".idxDialogActionBarLead",this.domNode)[0].innerHTML = "";
        			dijit.byId(restartMaintOffDialogId + "_button_ok").set("style","display:inline");
                	this.closeButtonLabel = nls.appliance.systemControl.messagingServer.stopDialog.cancelButton;
        			self.restartMaintOffDialog.show();
            }
            }, restartMaintOffDialogId);
            this.restartMaintOffDialog.dialogId = restartMaintOffDialogId;
            
            // clean store confirmation dialog
            var restartCleanStoreDialogId = this.id+"_restartCleanStoreDialog";
            domConstruct.place("<div id='"+restartCleanStoreDialogId+"'></div>", this.id);
            var restartCleanStoreTitle = nls.restartCleanStoreDialogTitle;
            this.restartCleanStoreDialog = new Dialog({
                id: restartCleanStoreDialogId,
                title: restartCleanStoreTitle,
                content: "<div>" + nls.restartCleanStoreDialogContent + "</div>",
                buttons: [
                    new Button({
                        id: restartCleanStoreDialogId + "_button_ok",
                        label: nls.cleanOkButton,
                        onClick: function() {
                        	var rvalues = JSON.parse(JSON.stringify(values));
                        	rvalues.CleanStore = true;
                        	dojo.byId(statusId).innerHTML = nls.restarting;
                        	this.set("style","display:none");
                        	self.restartCleanStoreDialog.closeButton.set("label",nlsStrings.action.Close);
                        	self._doRestart(true, rvalues, restartCleanStoreDialogId);
                        }
                    })
                ],
                closeButtonLabel: nls.appliance.systemControl.messagingServer.stopDialog.cancelButton,
                onShow: function() {
        			query(".idxDialogActionBarLead",this.domNode)[0].innerHTML = "";
        			dijit.byId(restartCleanStoreDialogId + "_button_ok").set("style","display:inline");
                	this.closeButtonLabel = nls.appliance.systemControl.messagingServer.stopDialog.cancelButton;
        			self.restartCleanStoreDialog.show();
            }
            }, restartCleanStoreDialogId);
            this.restartCleanStoreDialog.dialogId = restartCleanStoreDialogId;

		},

		
		_fetchStatus: function(id, timeout, preserveTimeout) {
			var t = this;
			Utils.updateStatus(
				function(data) {
					var status = t._updateServerStatus(data, t, id);
					t.currentStatus = status;
					if (timeout > 0 && !status.unknownOrStopped && !Utils.isMaintenanceMode(status.serverState) && status.serverState != 1 && status.serverState != 10) {
						var newtimeout = 5000;
						if (preserveTimeout) {
							newtimeout = timeout;
						}
						// server is in a state of flux, schedule another in 1 second
						setTimeout(function(){
							dijit.byId(id)._fetchStatus(id, newtimeout);
						}, timeout);
					}
				},
				function(error) {	
			    	console.debug("error: "+error);
					// are we restarting?
					if (t.restarting && timeout > 0) {
                        setTimeout(function(){
                            dijit.byId(id)._fetchStatus(id, 5000);
                        }, timeout);
					    return;
					}
					if (!t.currentStatus.unknownOrStopped) {
						Utils.showPageErrorFromPromise(nls.appliance.systemControl.messagingServer.retrieveStatusError, error);
					}
					dojo.byId(id+"_status").innerHTML = nls.appliance.systemControl.messagingServer.unknown;
				});
		},		
		
		_fetchServername: function() {
			var id = this.id + "_servername";
			var self = this;
			Utils.getServerName(
					function(data) {
						var servername = nls.appliance.systemControl.appliance.hostnameNotSet;
						if (data && data.ServerName !== undefined && data.ServerName !== "") {
							servername = data.ServerName;
						}
						dojo.byId(id).innerHTML = entities.encode(servername);
						self.servername = servername;
						dojo.publish(Resources.contextChange.nodeNameTopic, servername);
					},
				    function(error) {
				    	console.debug("error: "+error);
						if (self.currentStatus.unknownOrStopped) {
	                        setTimeout(function(){
	                            dijit.byId(self.id)._fetchServername();
	                        }, 1000);
						    return;
						} else {
							Utils.showPageErrorFromPromise(nls.appliance.systemControl.appliance.retrieveHostnameError, error);
						}
				    });
		},

        _fetchLicensedUsage: function() {
            var id = this.id + "_licensedUsage";
            var self = this;
            Utils.getLicensedUsage(function(data) {
                var licensedUsage = data.LicensedUsage;
                var licensedUsageDisplayString = "";
                if (licensedUsage) {
                    var luKey = licensedUsage.toLowerCase();
                    if (luKey === "non-production") {
                        luKey = "nonProduction";
                    } 
                    licensedUsageDisplayString = nls.appliance.systemControl.appliance.licensedUsageValues[luKey];
                }                
                dojo.byId(id).innerHTML = entities.encode(licensedUsageDisplayString);
                self.licensedUsage = licensedUsage;            	
                dojo.byId(self.id+"_viewLicense").href = Utils.getLicenseUrl();
            },
            function(error) {
                console.debug("error: "+error);
				if (self.currentStatus.unknownOrStopped) {
                    setTimeout(function(){
                        dijit.byId(self.id)._fetchLicensedUsage();
                    }, 1000);
				    return;
				} else {
	                Utils.showPageErrorFromPromise(nls.appliance.systemControl.appliance.licensedUsageRetrieveError, error);
				}
            });
        },		
		
        _hideLinks: function() {
			var stopLink = dom.byId(this.id + "_stop");
			var restartLink = dom.byId(this.id + "_restart");
			var restartMaintOnLink = dom.byId(this.id + "_restartMaintOn");
			var restartMaintOffLink = dom.byId(this.id + "_restartMaintOff");
			var restartCleanStoreLink = dom.byId(this.id + "_restartCleanStore");
			
			domStyle.set(stopLink, "display", "none");
            domStyle.set(restartLink, "display", "none");
            domStyle.set(restartMaintOnLink, "display", "none");
            domStyle.set(restartMaintOffLink, "display", "none");
            domStyle.set(restartCleanStoreLink, "display", "none");
        },
        
		_updateServerStatus: function(data, t, id) {
			var status = {niceStatus: nlsHome.home.status.unknown, serverState: -1, unknownOrStopped: true};
			var stopLink = dom.byId(id + "_stop");
			var restartLink = dom.byId(id + "_restart");
			var restartMaintOnLink = dom.byId(id + "_restartMaintOn");
			var restartMaintOffLink = dom.byId(id + "_restartMaintOff");
			var restartCleanStoreLink = dom.byId(id + "_restartCleanStore");

			if (data && data.Server) {
				status.serverState = data.Server.State;
				if (t.currentStatus.serverState !== status.serverState) {
					if (t.stopped && !Utils.isRunning(status.serverState)) {
						t.stopped = false;
					}
				}
			} 

			if ((t.stopped === true) || (t.restarting === true) || (t.stopping === true) ||  (!Utils.isRunning(status.serverState) && !Utils.isMaintenanceMode(status.serverState))) {				
				domStyle.set(stopLink, "display", "none");
                domStyle.set(restartLink, "display", "none");
                domStyle.set(restartMaintOnLink, "display", "none");
                domStyle.set(restartMaintOffLink, "display", "none");
                domStyle.set(restartCleanStoreLink, "display", "none");
			}
			
			var rc = "rc" + status.serverState;
			status.niceStatus = nlsHome.home.status[rc];
			status.unknownOrStopped = false;
			if (!status.niceStatus) {
				// if stopping do not set to unknown
				if (t.stopping === false) {
					status.niceStatus = nlsHome.home.status.unknown;
					status.unknownOrStopped = true;
				}
			} else if (status.niceStatus == nlsHome.home.status.unknown) {
				status.unknownOrStopped = true;
			} else if (Utils.isStopped(status.serverState)) {
				status.unknownOrStopped = true;
			} else if (Utils.isStartingInProgress(status.serverState)) {
				t._setStopping(false);
			}

			// handle special cases
			if (t.starting) {
				if (status.unknownOrStopped) {
					console.debug("_updateServerStatus returning without modifying page because starting is true and status is unknownOrStopped");
					setTimeout(function(){
//						dijit.byId(id)._fetchStatus(id, -1);
						status = t._getServerStatus();
					}, 5000);
					return status;
				} else {
					t._setStarting(false);
				}
			} else if (t.restarting) {
                if (t.stopped || status.unknownOrStopped || Utils.isStopping(status.serverState)) {
                    console.debug("_updateServerStatus returning without modifying page because restarting is true and status is unknownOrStopped");
                    setTimeout(function(){
//                        dijit.byId(id)._fetchStatus(id, -1);
						status = t._getServerStatus();
                    }, 3000);
                    return status;
                } else {
                    t._setRestarting(false);
                }
            } else if (t.stopping) {
				if (status.unknownOrStopped === false) {
					console.debug("_updateServerStatus returning without modifying page because stopping is true and status is not unknownOrStopped");
					setTimeout(function(){
//						dijit.byId(id)._fetchStatus(id, -1);
						status = t._getServerStatus();
					}, 3000);
					return status;
				} else {
					t._setStopping(false);
				}
			}
			
		    // handle special case of clean store
			if (t.cleaningStore) {
				if (status.unknownOrStopped === false) {
					console.debug("_updateServerStatus returning without modifying page because cleaningStore is true and status is not unknownOrStopped");
					setTimeout(function(){
//						dijit.byId(id)._fetchStatus(id, -1);
						status = t._getServerStatus();
					}, 3000);
					return status;
				} else {
					t._setCleaningStore(false);
				}
			}
			
			if (t.stopped === false && t.restarting === false && t.stopping === false && status.unknownOrStopped === false && !Utils.isStopping(status.serverState)) {			    
				domStyle.set(stopLink, "display", "inline");
                domStyle.set(this.id + "_restart", "display", "inline");
                if (Utils.isMaintenanceMode(status.serverState)) {
                    domStyle.set(this.id + "_restartMaintOn", "display", "none");
                    domStyle.set(this.id + "_restartMaintOff", "display", "inline");
                } else {
                    domStyle.set(this.id + "_restartMaintOn", "display", "inline");
                    domStyle.set(this.id + "_restartMaintOff", "display", "none");
                }
                domStyle.set(this.id + "_restartCleanStore", "display", "inline");
                domStyle.set(stopLink, "display", "inline");
			} else {
				domStyle.set(stopLink, "display", "none");
                domStyle.set(restartLink, "display", "none");
                domStyle.set(restartMaintOnLink, "display", "none");
                domStyle.set(restartMaintOffLink, "display", "none");
                domStyle.set(restartCleanStoreLink, "display", "none");
			}	
			
			if (!(t.restarting && Utils.isRunning(status.serverState))) {
				dojo.byId(id+"_status").innerHTML = status.niceStatus;
			}
						
			// If there is an admin state error and we haven't shown a message already, show one
			if (data && data.Server
					&& data.Server.ErrorCode !== undefined && data.Server.ErrorCode !== 0 && !t.adminErrorDisplayed) {
				t.adminErrorDisplayed = true;
				var error = data.Server.ErrorMessage ? data.Server.ErrorMessage : "";
				var details = lang.replace(nlsHome.home.status.adminErrorDetails, [data.Server.ErrorCode, error]);
				// if there is an hareason, show it instead...
				if (data.HighAvailability && data.HighAvailability.ReasonCode && nlsHome.home.haReason[data.HighAvailability.ReasonCode]) {
					var reason = nlsHome.home.haReason[data.HighAvailability.ReasonCode];
					if (data.HighAvailability.ReasonCode === 1 && data.HighAvailability.ReasonString) {
						reason = lang.replace(reason, [data.HighAvailability.ReasonString]);
					}
					if (data.HighAvailability.PrimaryLastTime && data.HighAvailability.PrimaryLastTime !== 0) {
						var formattedDate = Utils.getFormattedDateTime(new Date(data.HighAvailability.PrimaryLastTime));
						var lastPrimaryMessage = lang.replace(nlsHome.home.haReason.lastPrimaryTime, [formattedDate]);
						reason += " " + lastPrimaryMessage;
					}
					details = reason;
				}
				Utils.publishPageErrorIfUnique(nlsHome.home.status.adminError, "", details);
			} 
			
			return status;
		},
				
		_saveServername: function(servername, form, servernameDialogId) {
			console.debug("Save servername clicked "+servername);
			
			if (!form.validate()) { return; } 
			servername = iString.nullTrim(servername);
			if (!servername || servername === "") {
				servername = "(none)";
			}
			var _this = this;
			var servernameDialog = dijit.byId(servernameDialogId);
			var bar = query(".idxDialogActionBarLead",this.servernameDialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
			Utils.setServerName(servername,
					function(data) {
				        bar.innerHTML = "";
				        _this._fetchServername();				
				        servernameDialog.hide();
			},
			function(error) {
				bar.innerHTML = nls.appliance.savingFailed;
				servernameDialog.showErrorFromPromise(nls.appliance.systemControl.appliance.hostnameError, error);								
			});
		},

		_setStarting: function(starting) {
			if (starting === this.starting) {
				console.debug("_setStarting called but already set to " + starting);
				return;
			}

			if (starting) {
				this.starting = true;				
				dojo.byId(this.id+"_status").innerHTML = nls.appliance.systemControl.messagingServer.starting;
				
				// set a timer so it doesn't stay in starting mode for more than a minute
				this.startTimeoutID = window.setTimeout(lang.hitch(this, function(){
					this._setStarting(false);
					this._fetchStatus(this.id, -1);
				}), 60000);
				
			} else {
				this.starting = false;
								
				// clear any timer
				if (this.startTimeoutID) {
					window.clearTimeout(this.startTimeoutID);
					delete(this.startTimeoutID);
				}								
			}
		},
		
		_setStopping: function(stopping) {
			if (stopping === this.stopping) {
				console.debug("_setStopping called but already set to " + stopping);
				return;
			}

			if (stopping) {
				this.stopping = true;				
				dojo.byId(this.id+"_status").innerHTML = nls.appliance.systemControl.messagingServer.stopping;
				
				// set a timer so it doesn't stay in stopping mode for more than a minute
				this.stopTimeoutID = window.setTimeout(lang.hitch(this, function(){
					this._setStopping(false);
					this._fetchStatus(this.id, -1);
				}), 60000);
				
			} else {
				this.stopping = false;
								
				// clear any timer
				if (this.stopTimeoutID) {
					window.clearTimeout(this.stopTimeoutID);
					delete(this.stopTimeoutID);
				}								
			}
		},
		
        _setRestarting: function(restarting) {
            if (restarting === this.restarting) {
                console.debug("_setRetarting called but already set to " + restarting);
                return;
            }

            if (restarting) {
                this.restarting = true;               
                dojo.byId(this.id+"_status").innerHTML = nls.restarting;
                
                // set a timer so it doesn't stay in restarting mode for more than a minute
                this.restartTimeoutID = window.setTimeout(lang.hitch(this, function(){
                    this._setRestarting(false);
                    this.stopped = false;
                    this._fetchStatus(this.id, -1);
                }), 60000);
                
            } else {
                this.restarting = false;
                this.stopped = false;
                                
                // clear any timer
                if (this.restartTimeoutID) {
                    window.clearTimeout(this.restartTimeoutID);
                    delete(this.restartTimeoutID);
                }                               
            }
        },
        		
		
		_setCleaningStore: function(cleaningStore) {
			if (cleaningStore === this.cleaningStore) {
				console.debug("_setCleaningStore called but already set to " + cleaningStore);
				return;
			}

			if (cleaningStore) {
				this.cleaningStore = true;				
				dojo.byId(this.id+"_status").innerHTML = nls.appliance.systemControl.messagingServer.cleanStore;
				
			} else {
				this.cleaningStore = false;							
			}
		},
		
		_doRestart: function(waitForRestart, values, restartDialogId) {
            var bar = query(".idxDialogActionBarLead",dijit.byId(restartDialogId).domNode)[0];
            bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.restarting;
		    var self = this;
            self._setStopping(false);
            self._setStarting(false);
            self._setRestarting(true);
            self.stopped = true;
            self._hideLinks();
            Utils.restartRemoteServer(waitForRestart, values, function() {                      
                // restart returned successfully
            	self.stopped = false;
            	self._setRestarting(false);
            	// Hide action links
            	self.currentStatus = self._updateServerStatus(null, self, self.id);
                // Show server is unreachable (status will show "X")
                self._fetchStatus(self.id, -1);
                dijit.byId(restartDialogId).hide();
          }, function(error) {
              // restart failed
              self._setRestarting(false);
              dijit.byId(restartDialogId).showErrorFromPromise(nls.restartingFailed, error);
              bar.innerHTML = "";
              self._fetchStatus(self.id, -1);
          });			
		},

		_doStop: function(values, stopDialogId) {
		    var self = this;
            self._setStopping(true);
            self._setStarting(false);
            self._setRestarting(false);
            self.stopped = true;
            Utils.stopRemoteServer(values, function() {                      
                // stop returned successfully
            	self.stopped = false;
            	self._setStopping(false);
            	// Hide action links
            	self.currentStatus = self._updateServerStatus(null, self, self.id);
            	// Show server is unreachable (status will show "X")
            	self._fetchStatus(self.id, -1);
                dijit.byId(stopDialogId).hide();
          }, function(error) {
              // stop failed
              self._setStopping(false);
              dijit.byId(stopDialogId).showErrorFromPromise(nls.stopFailed, error);
              self._fetchStatus(self.id, -1);
          });			
		},
		
		restart: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();

		    console.debug("restart clicked");
		    this.restartDialog.show();         
		},
		
		restartMaintOn: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();

		    console.debug("restartMaintOn clicked");
		    this.restartMaintOnDialog.show();         
		},
		
		restartMaintOff: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();

		    console.debug("restartMaintOff clicked");
		    this.restartMaintOffDialog.show();         
		},
		
		restartCleanStore: function(event) {
		    // work around for IE event issue - stop default event
		    if (event && event.preventDefault) event.preventDefault();

		    console.debug("restartCleanStore clicked");
		    this.restartCleanStoreDialog.show();         
		},
	        		
		stop: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();

            console.debug("Stop clicked");
			this.stopDialog.show();	
			this.stopDialog.showWarningMessage(nls.stopServerWarningTitle, nls.stopServerWarning);
		}

    });
    
    return MessagingServerControl;
 
});
