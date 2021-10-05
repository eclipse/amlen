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
    "dojo/_base/declare",
    'dojo/_base/lang',
    'dojo/dom',
	'dijit/_Widget',
	'dijit/_TemplatedMixin',
	'dijit/_WidgetsInTemplateMixin',		
	'dijit/layout/ContentPane',    
	'dojo/data/ItemFileWriteStore',
    'dojo/text!ism/controller/content/templates/LogLevelControl.html',
	'dojo/i18n!ism/nls/appliance',
	'ism/config/Resources',
	'ism/IsmUtils'
     ], function(declare, lang, dom, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, ContentPane, ItemFileWriteStore, template, nls, Resources, Utils) {
 
	var LogLevelControl = declare("ism.controller.content.LogLevelControl", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        
    	templateString: template,
    	nls: nls,
    	logLevel: null,
    	logType: "LogLevel",
    	process: "ism",
    	lastServerStatus: undefined,
    	lastServerStatusEnableState: 1,
    	lastMQConnectivityStatusEnableState: 1,
    	logLabel: Utils.escapeApostrophe(nls.appliance.systemControl.logLevel.logLevel),
    	logTooltip: Utils.escapeApostrophe(nls.appliance.systemControl.logLevel.tooltip),
    	
    	restURI: "rest/config/loglevel",
    	domainUid: 0,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			if (this.logType != "LogLevel") {
				this.logLabel = Utils.escapeApostrophe(nls.appliance.systemControl.logLevel[this.logKey]);
				this.logTooltip = Utils.escapeApostrophe(nls.appliance.systemControl.logLevel[this.logTooltipKey]);
			}
			
			// subscribe to server status changes and enable/disable the select widget accordingly
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {	
				// save the status for MQConnectivity control to check later
				this.lastServerStatus = data;

				/* no longer blocked... See work item 70418
				// if the harole indicates this is the primary and it is synchronizing,
				// disable log level control and return
				if (Utils.isPrimarySynchronizing(data.harole)) {
					// disable any sort of control
					this.logLevelSelect.set("disabled",true);
					this.lastServerStatusEnableState = 0;
					dom.byId("LogLevelWarnArea").innerHTML = "<br/>" +
						"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
						 nls.appliance.systemControl.logLevel.serverSyncState;
					return;
				} */

				// if this is the mqconnectivity process and the role is STANDBY, 
				// disable log level control and return				
				if (this.process === "mqconnectivity" && data.harole === "STANDBY") {
					this.logLevelSelect.set("disabled",true);
					this.lastServerStatusEnableState = 0;					
					dom.byId("LogLevelWarnArea").innerHTML = "<br/>" +
						"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
						nls.appliance.systemControl.logLevel.mqStandbyState;
					return;
				}
				
				if (Utils.isErrorMode(data.RC)) { 
					this.logLevelSelect.set("disabled",true);
					this.lastServerStatusEnableState = 0;					
					dom.byId("LogLevelWarnArea").innerHTML = "<br/>" +
						"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
						nls.appliance.systemControl.logLevel.serverNotAvailable;
				} else if (Utils.isStartingInProgress(data.RC)) {
					this.logLevelSelect.set("disabled",true);
					this.lastServerStatusEnableState = 0;					
					dom.byId("LogLevelWarnArea").innerHTML = "<br/>" +
						"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
						nls.appliance.systemControl.logLevel.serverCleanStore;									
				} else if (this.logLevelSelect.get("disabled", true)){
					this.lastServerStatusEnableState = 1;
					dom.byId("LogLevelWarnArea").innerHTML = "";
					if (this.process == "mqconnectivity" && this.lastMQConnectivityStatusEnableState === 0) {
						return;
					}
					this.logLevelSelect.set("disabled",false);
					this._fetchLogLevel();
				}
			}));
			
			// subscribe to MQ Connectivity status changes and enable/disable the select widget accordingly
			dojo.subscribe(Resources.mqConnectivityStatus.statusChangedTopic, lang.hitch(this, function(data) {	
				// if this log control is not for mqconnectivity return
				if (this.process != "mqconnectivity") {
					return;
				}
				if (data.Status === "stopped" || data.Status === "UNKNOWN") {
					this.logLevelSelect.set("disabled",true);
					this.lastMQConnectivityStatusEnableState = 0;
					dom.byId("MQLogLevelWarnArea").innerHTML = "<br/>" +
						"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
						nls.appliance.systemControl.logLevel.mqNotAvailable;
				} else if (this.logLevelSelect.get("disabled", true)) {
					dom.byId("MQLogLevelWarnArea").innerHTML = "";
					this.lastMQConnectivityStatusEnableState = 1;
					if (this.lastServerStatusEnableState === 1){
						this.logLevelSelect.set("disabled",false);
						this._fetchLogLevel();
					}
				}
			}));
		},	
		
		postCreate: function(args) {
			this.inherited(arguments);
	         
            this.domainUid = ism.user.getDomainUid();

			// is this for a specific domain or the appliance as a whole?
			var domainString = this.domainUid === undefined ? "" : "/" + this.domainUid; 			
			this.restURI += domainString + "/" + this.logType;
			
			this._fetchLogLevel();
			if (this.topic) {
				// when switching to this page, refresh the field values
				var topic = dojo.string.substitute(this.topic,{Resources:Resources});
				dojo.subscribe(topic, lang.hitch(this, function(message){
					this._fetchLogLevel();
				}));					
			}
		},
		
		_fetchLogLevel: function() {
			var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.getDefaultGetOptions();
            var promise = adminEndpoint.doRequest("/configuration/"+ _this.logType, options);
            promise.then(function(data) {
            			_this.logLevel = data[_this.logType];
						_this.logLevelSelect.set("value", _this.logLevel, false);
            },
            function(error){
            	_this._handleError(lang.replace(nls.appliance.systemControl.logLevel.retrieveLogLevelError,[_this.logLabel]), error);
            });
		},
		
		_handleError: function(title, error) {
			// check to see if the server is stopped before displaying a page error
			var _this = this;
			// have we already been disabled?
			if (_this.logLevelSelect.get("disabled", true)) {
				console.debug("_handleError: already disabled, just return. Log: " + _this.logLabel);
				return;
			}
			
			if (_this.process === "ism") {
				_this._handleServerError(title, error);
			} else if (_this.process === "mqconnectivity") {
				_this._handleMQConnectivtyError(title, error);
			}
			
		},
		
		_handleServerError: function(title, error) {
			var t = this;
			Utils.updateStatus( function(data) {
			    if (Utils.isErrorMode(data.RC) || Utils.isStartingInProgress(data.RC)) {
			        console.debug("_handleError: Server in error mode, disable control. Log: " + t.logLabel);
			        t.logLevelSelect.set("disabled",true);
			        dom.byId("LogLevelWarnArea").innerHTML = "<br/>" +
			        "<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
			        nls.appliance.systemControl.logLevel.serverNotAvailable;						
			    } else {
			        Utils.showPageErrorFromPromise(title, error);						
			    }
			}, function(error) {	
			    Utils.showPageErrorFromPromiseFromPromise(title, error);
			});
		},
		
		_handleMQConnectivtyError: function(title, error) {
			var t = this;			
			Utils.getMQConnectivityStatus({
				load: function(data) {
					if (data.Status === "stopped") {
						console.debug("_handleError: Server in error mode, disable control. Log: " + t.logLabel);
						t.logLevelSelect.set("disabled",true);
						dom.byId("MQLogLevelWarnArea").innerHTML = "<br/>" +
							"<img src='css/images/msgWarning16.png' width='16' height='16' alt=''/> " +
							nls.appliance.systemControl.logLevel.mqNotAvailable;						
					} else {
						Utils.showPageErrorFromPromise(title, error);						
					}
				},
				error: function(error) {	
					Utils.showPageErrorFromPromise(title, error);
				}
			});
			
		},
		
		setLogLevel:  function() {
			var value = this.logLevelSelect.get("value");
			var logLabel = this.logLabel;
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions(_this.logType, value);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(function(data) {
				_this.logLevel = value;
			},
			function(error) {
				Utils.showPageErrorFromPrimise(lang.replace(nls.appliance.systemControl.logLevel.setLogLevelError,[logLabel]), error);
				_this.logLevelSelect.set("value", _this.logLevel, false);
			});
		}

    });
 
    return LogLevelControl;
});
