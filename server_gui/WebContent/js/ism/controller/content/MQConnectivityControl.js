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
	'dijit/_Widget',
	'dijit/_TemplatedMixin',
	'dijit/_WidgetsInTemplateMixin',		
	'dijit/layout/ContentPane',
	'dijit/form/Button',
    'dojo/text!ism/controller/content/templates/MQConnectivityControl.html',
	'dojo/i18n!ism/nls/appliance',
	'ism/IsmUtils',
	'ism/Utils',
	'ism/widgets/Dialog',
	'ism/config/Resources'
	], function(declare, lang, dom, domStyle, domConstruct, number,  _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, 
			ContentPane, Button, template, nls, IsmUtils, Utils, Dialog, Resources) {
 
    var MQConnectivityControl = declare("ism.controller.content.MQConnectivityControl", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        
    	templateString: template,
    	nls: nls,
    	stopDialog: null,
    	adminErrorDisplayed: false,
    	starting: false,
    	updateInterval: 30000, // milliseconds
    	enableLabel: nls.mqConnectivityEnableLabel,
    	enabled: false,
        started: undefined,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function(args) {
			this.inherited(arguments);
			// initial status fetch should request scheduling of another update
			this._initMQConnectivityData(true);
			this._createConfirmationDialogs();
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.appliance.nav.systemControl.topic, lang.hitch(this, function(message){
				this._initMQConnectivityData(false);
			}));

		},
		
		_initMQConnectivityData: function(scheduleUpdate) {
			this._fetchStatus(this.id, scheduleUpdate);
		},
		
		_createConfirmationDialogs: function() {
			var statusId = this.id + "_status";
			var self = this;
			
			// stop confirmation dialog
			var stopDialogId = this.id+"_stopDialog";
			domConstruct.place("<div id='"+stopDialogId+"'></div>", this.id);
			var stopTitle = nls.appliance.systemControl.mqconnectivity.stopDialog.title;
			this.stopDialog = new Dialog({
				id: stopDialogId,
				title: stopTitle,
				content: "<div>" + nls.appliance.systemControl.mqconnectivity.stopDialog.content + "</div>",
				buttons: [
					new Button({
						id: stopDialogId + "_button_ok",
						label: nls.appliance.systemControl.mqconnectivity.stopDialog.okButton,
						onClick: function() { 
							self._setStarting(false);
							dojo.byId(statusId).innerHTML = nls.appliance.systemControl.mqconnectivity.stopping; 
							IsmUtils.stopMQConnectivity(function(data) {
								self._fetchStatus(self.id, false);
								dijit.byId(stopDialogId).hide();
							}, function(error) {
								self.stopDialog.showErrorFromPromise(nls.appliance.systemControl.mqconnectivity.stopError, error);
								self._fetchStatus(self.id, false);
							});
							
						}
					})
				],
				closeButtonLabel: nls.appliance.systemControl.mqconnectivity.stopDialog.cancelButton
			}, stopDialogId);
			this.stopDialog.dialogId = stopDialogId;

		},

		_fetchStatus: function(id, scheduleUpdate) {
			var t = this;
			IsmUtils.getMQConnectivityStatus(
				function(data) {
					t._updateServerStatus(data, t, id, scheduleUpdate);
				},
				function(error) {	
			    	console.debug("error: "+error);
					var status = { Status:  "UNKNOWN" };
					t._updateServerStatus(status, t, id, scheduleUpdate);
				}
			);
		},		
		
		_updateServerStatus: function(data, t, id, scheduleUpdate) {
			var startLink = dom.byId(id + "_start");
			var stopLink = dom.byId(id + "_stop");

			// handle special case of starting
			if (t.starting) {
				domStyle.set(startLink, "display", "none");
				domStyle.set(stopLink, "display", "none");
				if (data.MQConnectivity.Status.toLowerCase() === "inactive" || data.MQConnectivity.Status === "UNKNOWN") {
					console.debug("_updateServerStatus returning without modifying page because starting is true and status is unknownOrStopped");
					setTimeout(function(){
						dijit.byId(id)._fetchStatus(id, false);
					}, 5000);
					return;
				} else {
					t._setStarting(false);
				}
			}
			
			if (data.MQConnectivity) {
				t.enabled = data.MQConnectivity.Enabled === undefined ? false : data.MQConnectivity.Enabled;
			}
			t.field_MQConnectivityEnabled.set("checked", t.enabled,"false");

			var modeString = "";
			if (t.enabled) {
			if (data.MQConnectivity.Status.toLowerCase() === "active") {
				domStyle.set(startLink, "display", "none");
				domStyle.set(stopLink, "display", "inline");
				modeString =  nls.appliance.systemControl.mqconnectivity.started;
				t.started = true;
			} else {
				t.started = false;
				domStyle.set(startLink, "display", "inline");
				domStyle.set(stopLink, "display", "none");
				if (data.MQConnectivity.Status.toLowerCase() === "inactive") {
					modeString =  nls.appliance.systemControl.mqconnectivity.stopped;
				} else {
					modeString =  nls.appliance.systemControl.mqconnectivity.unknown;
				}
			}	
			} else {
				domStyle.set(startLink, "display", "none");
				domStyle.set(stopLink, "display", "none");				
				// This should only happen due to timing issues.  If enabled is still false, then
				// the service status should always be inactive.
				if (data.MQConnectivity && data.MQConnectivity.Status) {
					if (data.MQConnectivity.Status.toLowerCase() === "active") {
						modeString =  nls.appliance.systemControl.mqconnectivity.started;
						t.started = true;
					}
					else if (data.MQConnectivity.Status.toLowerCase() === "inactive") {
						modeString =  nls.appliance.systemControl.mqconnectivity.stopped;
						t.started = false;
					} else {
						modeString =  nls.appliance.systemControl.mqconnectivity.unknown;
						t.started = false;
					}
				} else {
					modeString =  nls.appliance.systemControl.mqconnectivity.unknown;
					t.started = false;
				}
			}
								
			dojo.byId(id+"_status").innerHTML = modeString;
			
			if (scheduleUpdate) {
				// do not allow every update to schedule another one
				setTimeout(lang.hitch(this, function(){
					this._fetchStatus(id, true);
				}), this.updateInterval);
			}
		},
		
		
		
		_setStarting: function(starting) {
			if (starting === this.starting) {
				console.debug("_setStarting called but already set to " + starting);
				return;
			}

			if (starting) {
				// Disable the checkbox until processing is finished
				this.field_MQConnectivityEnabled.set("disabled", true);
				// Hide stop/start links until processing is finished
				domStyle.set(this.id + "_start", "display", "none");
				domStyle.set(this.id + "_stop", "display", "none");
				this.starting = true;				
				dojo.byId(this.id+"_status").innerHTML = nls.appliance.systemControl.mqconnectivity.starting;
				
				// set a timer so it doesn't stay in starting mode for more than a minute
				this.timeoutID = window.setTimeout(lang.hitch(this, function(){
					this._setStarting(false);
					this._fetchStatus(this.id);
				}), 60000);
				
			} else {
				this.starting = false;
				// Re-enable the checkbox after processing is finished
				this.field_MQConnectivityEnabled.set("disabled", false);
								
				// clear any timer
				if (this.timeoutID) {
					window.clearTimeout(this.timeoutID);
					delete(this.timeoutID);
				}								
			}
		},
		
		setMQConnectivityEnabled: function(value) {
			console.debug("changed MQConnectivityEnabled");
			if (value == this.enabled) {
				// not changing, so do nothing...
				return;
			}
			var statusId = this.id + "_status";
			this.mqConnEnabledSavingDiv.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.savingProgress;
			var _this = this;
			if (value === true) {
				_this._setStarting(true);
			}			
			if (value === false) {
				domStyle.set(_this.id +"_start", "display", "none");
				domStyle.set(_this.id + "_stop", "display", "none");
				if (_this.started === true) {
					dojo.byId(statusId).innerHTML = nls.appliance.systemControl.mqconnectivity.stopping; 
				}
			}
			IsmUtils.setMQConnectivityEnabled(value, function() {
				_this._fetchStatus(_this.id, true);
				_this.mqConnEnabledSavingDiv.innerHTML = "";
			}, function (error) {
				_this.mqConnEnabledSavingDiv.innerHTML = "";
				Utils.showPageError(nls.setMqConnEnabledError, error);
				_this.field_MQConnectivityEnabled.set('value', _this.enabled, false);
			});
		},
		
		start: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();
		    
			console.debug("Start clicked");
			var id = this.id;
			var self = this;
			self._setStarting(true);
			IsmUtils.startMQConnectivity(function(data) {
				// wait 5 seconds, then fetch status
				setTimeout(function(){
					self._fetchStatus(id, false);
				}, 5000);
			}, function(error) {
				self._setStarting(false);
				Utils.showPageError(nls.appliance.systemControl.mqconnectivity.startError, error);
				dojo.byId(id+"_status").innerHTML = nls.appliance.systemControl.mqconnectivity.unknown;					
			});
		},		
		
		stop: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();
		    
			console.debug("Stop clicked");
			this.stopDialog.show();			
		}
		
    });
    
    return MQConnectivityControl;
 
});
