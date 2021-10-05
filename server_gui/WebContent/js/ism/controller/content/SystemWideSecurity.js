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
    'dojo/dom-construct',
	'dojo/query',
    'ism/widgets/_TemplatedContent',
    'idx/form/CheckBox',
    'dijit/form/Button',
	'dijit/layout/ContentPane',
	'ism/IsmUtils',
	'ism/widgets/Dialog',
    'dojo/text!ism/controller/content/templates/SystemWideSecurity.html',
    'dojo/i18n!ism/nls/appliance'
], function(declare, lang, domConstruct, query, _TemplatedContent, CheckBox, Button, ContentPane, Utils, Dialog, template, nls) {
 
    return declare("ism.controller.content.SystemWideSecurity", _TemplatedContent, {
        
    	templateString: template,
    	nls: nls, 
    	fips: false,
    	fipsDialog: null,
    	domainUid: 0,
        
		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
		},
		
		postCreate: function() {
			this.inherited(arguments);
            this.domainUid = ism.user.getDomainUid();
            // Add the domain Uid to the REST API.
            this.restURI += "/" + this.domainUid;           
			
			console.debug(this.declaredClass + ".postCreate()");
			this.getFIPS();
			this._createConfirmationDialogs();
		},
		
		_createConfirmationDialogs: function() {
			var field_FIPS = this.field_FIPS;
			var self = this;
			var uri = Utils.getBaseUri() + this.restURI;
			
			// stop confirmation dialog
			var fipsDialogId = "setFipsDialogId";
			domConstruct.place("<div id='"+fipsDialogId+"'></div>", this.id);
			this.fipsDialog = new Dialog({
				id: fipsDialogId,
				style: 'width: 600px;',
				title: nls.appliance.securitySettings.fipsDialog.title,
				content: "<div>" + nls.appliance.securitySettings.fipsDialog.content + "</div>",
				buttons: [
					new Button({
						id: fipsDialogId + "_button_ok",
						label: nls.appliance.securitySettings.fipsDialog.okButton,
						onClick: function() { 
							var value =  field_FIPS.get("checked");
							console.log("value about to submit is " + value);
				            var adminEndpoint = ism.user.getAdminEndpoint();
				            var options = adminEndpoint.getDefaultPostOptions();
				            options.data = JSON.stringify({"FIPS":value});
				            var promise = adminEndpoint.doRequest("/configuration", options);
				            promise.then(
				                function(data) {
                                    // restart the ismserver
                                    self.restartIsmServer(self, function(success) {
                                        if (success) {
                                            self.fips = value;
                                        }                                       
                                    });
				                },
				                function(error) {
                                    dijit.byId(fipsDialogId).showErrorFromPromise(nls.appliance.securitySettings.fipsDialog.failed, error);                                 
				                }
				            );          
						}
					})
				],
				closeButtonLabel: nls.appliance.securitySettings.fipsDialog.cancelButton,
				onCancel: function() {
					console.log("Cancel clicked for FIPS");
					// put back the original setting
					field_FIPS.set("checked", self.fips);
				}
			}, fipsDialogId);
			this.fipsDialog.dialogId = fipsDialogId;		
		},


		getFIPS: function() {

            var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/FIPS", adminEndpoint.getDefaultGetOptions());
            promise.then(
                function(data) {
                    _this.fips = Utils.isTrue(data.FIPS, false);
                    _this.field_FIPS.set("checked", _this.fips, false);
                },
                function(error) {
                    Utils.showPageErrorFromPromise(nls.appliance.securitySettings.retrieveFipsError, error);
                }
            );		    
		},
		
		setFIPS: function(value) {
			console.debug("changed FIPS setting: "+value);
			if (value == this.fips) {
				// not changing, so do nothing...
				return;
			}
			// otherwise, show the confirmation dialog and let it handle the flow
			this.fipsDialog.show();
		},
		
		restartIsmServer: function(self, onComplete) {
		    var dialog = self.fipsDialog;
		    var bar = query(".idxDialogActionBarLead",dialog.domNode)[0];
		    bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.restarting;
		    Utils.restartMessagingServer(true, function() {                      
		        // start returned successfully
		        bar.innerHTML = "";
		        onComplete(true);
		        dialog.hide();
		    }, function(error, ioargs) {
		        // start failed
		        Utils.checkXhrError(ioargs);
		        bar.innerHTML = "";
		        dialog.showXhrError(nls.restartingFailed, error, ioargs);
		    });                 
		}		
    });
 
});
