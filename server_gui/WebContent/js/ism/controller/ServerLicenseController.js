/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
        'dojo/Deferred',
        'dojo/query',
        'dijit/_base/manager',
		'ism/config/Resources',
		'ism/controller/_PageController',
		'dojo/i18n!ism/nls/license',
		'dojo/i18n!ism/nls/appliance',
		'ism/IsmUtils',
		'ism/widgets/User',
		'ism/Utils',
		'ism/controller/content/LicenseAccept',
		'ism/widgets/Dialog'
		], function(declare, Deferred, query, manager, Resources, _PageController, nls, nlsa, IsmUtils, User, Utils, 
				LicenseAccept, Dialog) {

	var ServerLicenseController = declare("ism.controller.ServerLicenceController", [_PageController], {
		// summary:
		// 		Responsible for the page life-cycle of the License Acceptance page for the server side
		//      licenses.
		
		name: "licenseAccept",
		
		constructor: function() {
		},

		init: function() {
			console.debug(this.declaredClass + ".init()");
			this.inherited(arguments);
		},

		_initLayout: function() {
			console.debug(this.declaredClass + "._initLayout()");
			this.inherited(arguments);
		},

		_initWidgets: function() {
			console.debug(this.declaredClass + "._initWidgets()");
			this.inherited(arguments);
		},

		_initEvents: function() {
			console.debug(this.declaredClass + "._initEvents()");
			this.inherited(arguments);
			
			var controller = this;
			dojo.subscribe(Resources.pages.licenseAccept.accepttopic, function(message) {
				var bar = undefined;
				ism.user.setLicenseAccept(message, function() {
					// show message about the license being accepted and the server restarting
					var acceptDialogId = "acceptLicenseDialog";
					var dialog = manager.byId(acceptDialogId);	
					if (!dialog) {
						dialog = new Dialog({
							id: acceptDialogId,
							title: nls.license.acceptDialog.title,
							content: "<div>" + nls.license.acceptDialog.content + "</div>",
							closeDialog: function() {
								IsmUtils.updateStatus();
								bar.innerHTML = "";
								controller._handleOnCancel(controller);
							}
						},acceptDialogId);
					}
					var bar = query(".idxDialogActionBarLead",dialog.domNode)[0];
				    bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nlsa.restarting;
					dialog.closeButton.set('style', 'display:none');
					dialog.show();					
					controller._waitForRestart().then(dialog.closeDialog, dialog.closeDialog);
				});
			});
		},
		
		_handleOnCancel: function(context) {		    
            if (context.licenseOnly) {
                // special accessibility version of page                                
                var widget = manager.byId("licenseAcceptance");
                if (widget) {
                    widget.licenseAcceptance(widget);
                }
                return;
            }
		    // if the redirect takes too long, IE thinks the script is misbehaving.
		    // Return and run the rest of the script later
		    var newPage = Resources.pages.home.uri;
            setTimeout(function(){
                context.redirect(newPage);
            }, 10);
		},		

		_waitForRestart: function() {		    
			var _this = this;
			var promise = new Deferred();
			IsmUtils.waitForRestart(10, function() {                      
			    promise.resolve();
			});                 
			return promise.promise;
		}
	});

	return ServerLicenseController;
});
