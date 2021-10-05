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
        'dojo/_base/config',
        'dojo/_base/array',
        'dojo/Deferred',
        'dojo/dom',
        'dojo/has',
        'dojo/html',
        'dojo/when',
		'dijit/_base/manager',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/_Widget',
		'dijit/form/SimpleTextarea',
		'ism/widgets/Dialog',
		'dijit/form/Button',
		'idx/form/Select',
		'ism/config/Resources',
		'ism/widgets/License',
		'dojo/i18n!ism/nls/license',
		'dojo/i18n!ism/nls/appliance',
		'dojo/text!ism/controller/content/templates/LicenseAccept.html',
		'ism/Utils',
		'ism/IsmUtils',
		'idx/string'
], function(declare, config, array, Deferred, dom, has, html, when, manager, _TemplatedMixin, _WidgetsInTemplateMixin,
		_Widget, SimpleTextarea, Dialog, Button, Select, Resources, License, nls, nlsa, template, Utils, IsmUtils, iString) {

	var LicenseAcceptForm = declare("ism.controller.content.LicenseAccept", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {

		STATUS_LICENSE_VALID: 0,
	
		nls: nls,
		templateString: template,
		softwareNonibmUrl: null,
		softwareNoticesUrl: null,
		laMachineCodeUrl: null,
	    hardwareNonibmUrl: null,
	    hardwareNoticesUrl: null,
	    isVirtual: false,
	    normalPageId: "li_licensePresentPage",
	    nonAuthPageId: "li_licensePresentPage_NotAuthorized",
	    licensedUsage: {},
	    luRequest: new Deferred(),

		_displayedLang: null,
		
		constructor: function (args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			this.serverName = ism.user.serverName;
		},
		
		postCreate: function() {	
			console.debug(this.declaredClass + ".postCreate()");
			
            var _this = this;
	        // handle special case of a non system admin user logging in when the license is not accepted
            if (!ism.user.isLicenseAccepted() && array.indexOf(ism.user.roles,"SystemAdministrator") < 0) {
                dojo.byId(this.normalPageId).style.display = 'none';
                dojo.byId(this.nonAuthPageId).style.display = 'block';
                this.luRequest.cancel();
            } else {
    			if (ism.user.isLicenseAccepted()) {
    				this.luRequest.cancel();
    				window.location = Resources.pages.home.uri;
    			}
                IsmUtils.getLicensedUsage(
                    	function(data) {
                    		_this.licensedUsage = data;
                    		_this.luRequest.resolve();
                    	},
                    	function(error) {
                    		Utils.showPageErrorFromPromise(nlsa.appliance.systemControl.licensedUsageRetrieveError, error);
                    		_this.luRequest.reject();
                    	});
            }
            
            when(this.luRequest, function() {
            	dijit.byId("li_acceptButton").set('disabled', false);
    			var defaultLang = config.locale;
    			if (iString.startsWith(defaultLang, "zh-tw")) {
    				defaultLang = "zh_TW";
    			} else {
    				defaultLang = defaultLang.substring(0,2);
    			}
    			if (_this.langChoice.get("value") === defaultLang) {
    				_this.changeLanguage(defaultLang); // get the content loaded
    			} else {
    				_this.langChoice.set("value", defaultLang);
    			}
           });
		},
		
		startup: function() {
			console.debug(this.declaredClass + ".startup()");

			this.inherited(arguments);
		},
		
		changeLanguage: function(lang) {
			console.debug(this.declaredClass + ".changeLanguage() lang="+lang);
			var page = this;
			
			// Messaging server licenses
			var adminEndpoint = ism.user.getAdminEndpoint();
			var promise = adminEndpoint.doLicenseRequest(lang, this.licensedUsage.LicensedUsage);
			promise.then(function(data) {
			    var srvLicenseNode = dom.byId("li_licenseContent");
			    var htmlText = data.replace(/(\r\n)/g,"");
			    if(has('webkit') || has('ff')) {
			    	htmlText = htmlText.replace(/(>)/g,">\n");
			    }
			    html.set(srvLicenseNode, htmlText, { extractContent: true });
				page._displayedLang = lang;
			}, function(error) {
				console.debug("failure on retrieving license from messaging server");
				page.licenseContent.set("value", nls.license.error);
			});
		},
		
		accept: function() {
			console.debug(this.declaredClass + ".accept()");
			dojo.publish(Resources.pages.licenseAccept.accepttopic, this.licensedUsage);
		},
		
		decline: function() {
			console.debug(this.declaredClass + ".decline()");
			var declineDialogId = "declineLicenseDialog";
			var dialog = manager.byId(declineDialogId);
			if (!dialog) {
				dialog = new Dialog({
					id: declineDialogId,
					title: nls.license.declineDialog.title,
					content: "<div>" + nls.license.declineDialog.content + "</div>"
				},declineDialogId);
			}
			dialog.show();
		},
		
		print: function() {
			console.debug(this.declaredClass + ".print()");

			// get license contents from text widget
			var content = this.licenseContent.get("value");
			// ensure line breaks are in the right places
			content = content.replace(/\n/g,'<br/>');
						
			var iframe = dojo.byId('printLicenseIFrame');
			// handle browser differences
			var printDoc = (iframe.contentWindow || iframe.contentDocument);
			if (printDoc.document) {
				printDoc = printDoc.document;
			}
			
			// write HTML document to be printed
			printDoc.write("<html><head><title>"+nls.license.heading+"</title>");
			printDoc.write("</head><body onload='this.focus(); this.print();'>");
			printDoc.write(content + "</body></html>");
			printDoc.close();
		}		
	});
	return LicenseAcceptForm;
});
