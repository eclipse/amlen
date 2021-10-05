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
        'dojo/dom-construct',
        'dojo/store/Memory',
        'dojo/number',
    	'dojo/query',
    	'dojo/date/locale',
        'dijit/_Widget',
        'dijit/_TemplatedMixin',
        'dijit/_WidgetsInTemplateMixin',	
        'dijit/form/Button',
        'ism/widgets/TextBox',
        'ism/config/Resources',
        'ism/widgets/ToggleContainer',
        'ism/controller/content/LibertyCertificateDialog',
        'ism/IsmUtils',
        'dojo/i18n!ism/nls/appliance',
        'dojo/text!ism/controller/content/templates/CertificateForm.html',
		'dojox/form/Uploader',
		'dojox/form/uploader/FileList'        
        ], function(declare, lang, xhr, json, domConstruct, Memory, number, query, locale, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, Button, TextBox, Resources, ToggleContainer, LibertyCertificateDialog,
        		Utils, nls, template, Uploader, FileList) {

	var CertificateForm = declare("ism.controller.content.CertificateForm", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,

		editDialog: null,
		addDialog: null,
		store: null,
		uploadAction: null,
		
		templateType: "liberty",
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
			this.store = new Memory({data:[], idProperty: "cert"});
			this.uploadAction = Utils.getBaseUri() + this.certificatesURI;
		},

		startup: function(args) {
			this.inherited(arguments);

			this._initDialogs();
			this._initEvents();
			this._getCertificateInfo();
		},
		
		_getCertificateInfo: function() {
			console.debug("getCertificateInfo");
			var certExpiryDate = "keystore_expirationDate";
			var certName = "user_cert_name";
			var keyName = "user_key_name";
			// Get current certificate
			dojo.xhrGet({
				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/"+encodeURIComponent(certExpiryDate),
				sync: true,
				handleAs: "json",
				preventCache: true,
				load: lang.hitch(this, function(data) {
					var value = data[certExpiryDate];
					var exp = nls.appliance.webUISecurity.certificate.defaultCert;
					if (value) {
						if (value !== "") {
							// need to convert the date
							var date = new Date(Number(value));
							exp = locale.format(date, { datePattern: "yyyy-MM-dd", timePattern: "HH:mm (z)" });
						}
					}
					this.expiryDateText.innerHTML = exp;
				}),
				error: lang.hitch(this, function(error, ioargs) {
					console.debug("error", error);
					Utils.checkXhrError(ioargs);
				})
			});
			dojo.xhrGet({
				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/"+encodeURIComponent(certName),
				handleAs: "json",
				preventCache: true,
				load: lang.hitch(this, function(data) {
					var value = data[certName];
					var cert = nls.appliance.webUISecurity.certificate.defaultCert;
					if (value) {
						if (value !== "") {
							cert = value;
						}
					}
					this.certificateNameText.innerHTML = cert;
				}),
				error: lang.hitch(this, function(error, ioargs) {
					console.debug("error", error);
					Utils.checkXhrError(ioargs);
				})
			});
			dojo.xhrGet({
				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/"+encodeURIComponent(keyName),
				handleAs: "json",
				preventCache: true,
				load: lang.hitch(this, function(data) {
					var value = data[keyName];
					var key = nls.appliance.webUISecurity.certificate.defaultCert;
					if (value) {
						if (value !== "") {
							key = value;
						}
					}
					this.keyNameText.innerHTML = key;
				}),
				error: lang.hitch(this, function(error, ioargs) {
					console.debug("error", error);
					Utils.checkXhrError(ioargs);
				})
			});
		},

		_initDialogs: function() {
			// create Add dialog
			var addTitle = Utils.escapeApostrophe(nls.appliance.webUISecurity.certificate.addTitle);
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.id);
			this.addDialog = new LibertyCertificateDialog({ add: true, dialogId: addId, dialogTitle: addTitle, uploadAction: this.uploadAction, templateType: this.templateType },
					addId);
			this.addDialog.startup();
		},
		
		_initEvents: function() {
			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving addDialog");
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {		
					var bar = query(".idxDialogActionBarLead",this.addDialog.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
					this.addDialog.save(this.store, null, lang.hitch(this, function(data) {
						console.debug("add is done, returned:", data);
						if (data && this._saveSettings(data, this.addDialog.dialog)) {
							this.addDialog.dialog.hide();
							Utils.showPageWarning(nls.appliance.webUISecurity.certificate.success);
						} else {
							console.debug("save failed");
							bar.innerHTML = "";
						}
					}));
				} else {
					console.debug("validation failed");
				}
			}));
		},

		_saveSettings: function(values, dialog) {
            console.debug("Saving...");
			
			console.debug("data="+JSON.stringify(values));

			var page = this;
			var ret = false;
			//if (value.name)
			dojo.xhrPost({
			    url: Utils.getBaseUri() + this.certificatesURI,
				handleAs: "json",
			    headers: { 
					"Content-Type": "application/json"
				},
				postData: JSON.stringify(values),
			    sync: true, // need to wait for an answer
			    load: function(data){
		    		console.debug("success");
		    		ret=true;
			    },
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);
					dialog.showXhrError(nls.appliance.savingFailed, error, ioargs);
			    	ret=false;
			    }
			  });
			return ret;
		},
		
		addCertificate: function() {
			console.debug(this.declaredClass + ".addCertificate()");
			this.addDialog.show({}, {});
		}		
	});

	var WebUICertificate = declare("ism.controller.content.WebUICertificate", [ToggleContainer], {

		startsOpen: true,
		contentId: "webUICertificate",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = Utils.escapeApostrophe(nls.appliance.webUISecurity.certificate.subtitle);
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
			                "<span class='tagline'>" + nls.appliance.webUISecurity.certificate.tagline + "</span><br />",
			                "<div id='"+this.contentId+"'></div>"
			                ].join("\n");
			domConstruct.place(nodeText, this.toggleNode);

			var formId = this.contentId+'Form';
			this.certificateForm = new CertificateForm({formId: formId, certificatesURI: this.certificatesURI}, this.contentId);
			this.certificateForm.startup();
			
		}
	});

	return WebUICertificate;
});
