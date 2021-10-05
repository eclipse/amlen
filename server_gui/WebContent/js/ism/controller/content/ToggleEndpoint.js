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
    'dojo/on',
    'dojo/when',
	'dijit/_Widget',
	'dijit/_TemplatedMixin',
	'dijit/_WidgetsInTemplateMixin',		
    'dojo/text!ism/controller/content/templates/ToggleEndpoint.html',
	'dojo/i18n!ism/nls/messaging',
	'dojo/i18n!ism/nls/messages',
	'ism/IsmUtils',
	'ism/MonitoringUtils',
	'ism/config/Resources'
     ], function(declare, lang, on, when, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
    		  template, nls, nls_messages, Utils, MonitoringUtils, Resources) {
 
    var ToggleEndpoint = declare("ism.controller.content.ToggleEndpoint", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
        
    	templateString: template,
    	nls: nls,
    	endpointLabel: undefined,
    	state: undefined,
    	stateMessage: "",
    	missingMessage: "",
    	endpointData: {},
    	endpointToggledTopic: "ism.controller.content.ToggleEndpoint.endpointToggled",
    	domainUid: 0,
    	    	
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			this.endpointLabel = lang.replace(nls.messaging.messagingTester.endpoint.label, [this.endpoint]);
			this.linkLabel = this.enable ? nls.messaging.messagingTester.endpoint.linkLabel.enableLinkLabel : nls.messaging.messagingTester.endpoint.linkLabel.disableLinkLabel;			
		},
		
		postCreate: function() {
            
            this.domainUid = ism.user.getDomainUid();

		    this._initEndpointState();	
			if (this.topic) {
				// when switching to this page, refresh the field values
				var topic = dojo.string.substitute(this.topic,{Resources:Resources});
				dojo.subscribe(topic, lang.hitch(this, function(message){
					this._initEndpointState();
				}));					
			}
			
			// when state changed, refresh the field values
			dojo.subscribe(this.endpointToggledTopic, lang.hitch(this, function(message){
				this._initEndpointState();
			}));					
			
			// when the link is clicked, do the toggle
			on(this.toggleLink, "click", lang.hitch(this, function(event) {
                // work around for IE event issue - stop default event
                if (event && event.preventDefault) event.preventDefault();
			    
				this.toggle();
			}));


		},
		
		_initEndpointState: function() {
			var t = this;
			t.stateMessage = "";
			var toggleLinkDisplay = 'none';
			t._updateSaveOutput(t, "updating");
			var adminEndpoint = ism.user.getAdminEndpoint();
			var promise = adminEndpoint.doRequest("/configuration/Endpoint/"+ encodeURIComponent(t.endpoint), adminEndpoint.getDefaultGetOptions());
            promise.then(
                    function(data) {                        
                        t.endpointData = adminEndpoint.getNamedObject(data, "Endpoint",t.endpoint);
                        var enabled = (t.endpointData.Enabled && (t.endpointData.Enabled === true || t.endpointData.Enabled.toLowerCase() === "true")) ? true : false;
                        var status = {};
                        when(MonitoringUtils.getNamedEndpointStatusDeferred(t.endpoint), function(sdata) {
                        	if (sdata) {
                        		status = sdata.Endpoint[0];
                        	}
                            if (status.Enabled === true) {
                                // it's up
                                t.state = "enabled";
                                if (!t.enable) {
                                    toggleLinkDisplay = 'inline';
                                }
                            } else if (status.Enabled === false){
                                // it's down                            
                                t.state = "down";
                                if (status.LastErrorCode && status.LastErrorCode !== "0") {
                                    var message = nls_messages.returnCodes["rc_"+status.LastErrorCode];
                                    var messageID = nls_messages.returnCodes.prefix + status.LastErrorCode;
                                    if (message) {
                                        t.stateMessage = messageID + ": " + message;
                                    } else {
                                        t.stateMessage = nls_messages.lastErrorCode + " " + messageID;
                                    }
                                } else if (!enabled) {
                                    // Since it's configured state is disabled, show as disabled, not down.
                                    t.state = "disabled";
                                    if (t.enable) {
                                        toggleLinkDisplay = 'inline';
                                    }
                                }
                           }
                           t.updateUI(toggleLinkDisplay);
                        }, function (error) {
                            // error retrieving status
                        	status = { error: error };
                            Utils.showPageErrorFromPromise(nls.messaging.messagingTester.endpoint.retrieveEndpointStatusError, status.error);
                            return;                       
                        });                    
                    },
                    function(error) {
                        if (Utils.noObjectsFound(error)) {
                            // the endpoint doesn't exist
                            t.state = "missing";
                            if (t.missingMessage) {
                                t.stateMessage = dojo.string.substitute(t.missingMessage, {nls:nls});
                            }
                            t.endpointData = {};    
                            t.updateUI('none');
                        } else {
                            t._updateSaveOutput(t, "clear");
                            Utils.showPageErrorFromPromise(nls.messaging.messagingTester.endpoint.retrieveEndpointError, error);
                        }
                    }
            );          			
		},
		
		updateUI: function(toggleLinkDisplay) {
			var t = this;
			if (!toggleLinkDisplay) {
				toggleLinkDisplay = 'none';
			}
			t._updateSaveOutput(t, "clear");
			var stateText = nls.messaging.messagingTester.endpoint.state[t.state];
			if (t.stateMessage !== "") {
				stateText += "<span style='font-weight: normal; padding: 0 0 0 10px'>" + t.stateMessage + "</span>";
			}
			var iconClass = undefined;
			switch (t.state) {
			case "enabled": 
				iconClass = 'ismIconEnabled';
				break;
			case "disabled":
				iconClass = 'ismIconUnchecked';
				break;
			case "down":
				iconClass = 'ismIconDisabled';
				break;
			case "missing":
				iconClass = 'ismIconWarning';
				break;
			default:
				break;
			}
			if (iconClass) {
				stateText = "<div class='" + iconClass + "' style='display:inline-block; padding: 0 10px 0 0;'>&nbsp;</div>" + stateText;
			}
			t.endpointState.innerHTML = stateText;
			t.toggleLink.style.display = toggleLinkDisplay;
		},
				
		toggle: function() {
			var _this = this;
			var values = _this.endpointData;
			values.Enabled = _this.enable;
			_this._updateSaveOutput(_this, "saving");
			var adminEndpoint = ism.user.getAdminEndpoint();
			var options = adminEndpoint.createPostOptions("Endpoint", values);
			var promise = adminEndpoint.doRequest("/configuration", options);
			promise.then(
					function(data) {
						dojo.publish(_this.endpointToggledTopic);
					},
					function(error) {
						Utils.showPageErrorFromPromise(nls.messaging.messagingTester.endpoint.saveEndpointError, error);
						dojo.publish(_this.endpointToggledTopic);
					}
            );
		},
		
		_updateSaveOutput: function (t, success) {
			switch (success) {
			case "saving":
    			t.toggleStateOutputText.innerHTML = nls.messaging.savingProgress;
    			t.toggleStateOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
    			break;			
			case "updating":
    			t.toggleStateOutputText.innerHTML = nls.messaging.refreshingGridMessage;
    			t.toggleStateOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
    			break;
			default:
    			t.toggleStateOutputText.innerHTML = "";
    			t.toggleStateOutputImage.innerHTML = "";	
    			break;    	
			} 
		}
		
    });
    
    return ToggleEndpoint;
 
});
