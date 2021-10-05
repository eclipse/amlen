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
        'dojo/_base/config',
        'dojo/_base/array',
        'dojo/dom',
        'dojo/has',
        'dojo/html',
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
		'dojo/text!ism/controller/content/templates/LicensePresent.html',
		'ism/Utils',
		'idx/string'
], function(declare, config, array, dom, has, html, manager, _TemplatedMixin, _WidgetsInTemplateMixin, _Widget, SimpleTextarea,
		Dialog, Button, Select, Resources, License, nls, template, Utils, iString) {

	var LicenseForm = declare("ism.controller.content.LicensePresent", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {

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

		_displayedLang: null,
		
		constructor: function (args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);

			if (this.softwareNoticesRestURI !== undefined) {
				this.softwareNoticesUrl= Utils.getBaseUri() + this.softwareNoticesRestURI;
			}

			if (this.softwareNonibmRestURI !== undefined) {
				this.softwareNonibmUrl= Utils.getBaseUri() + this.softwareNonibmRestURI;
			}
		},
		
		postCreate: function() {	
			console.debug(this.declaredClass + ".postCreate()");
		},
		
		startup: function() {
			console.debug(this.declaredClass + ".startup()");

			this.inherited(arguments);
			this.type = ism.user.getLicenseType();

			var defaultLang = config.locale;
			if (iString.startsWith(defaultLang, "zh-tw")) {
				defaultLang = "zh_TW";
			} else {
				defaultLang = defaultLang.substring(0,2);
			}
			if (this.langChoice.get("value") === defaultLang) {
				this.changeLanguage(defaultLang); // get the content loaded
			} else {
				this.langChoice.set("value", defaultLang);
			}
		},
		
		changeLanguage: function(lang) {
			console.debug(this.declaredClass + ".changeLanguage() lang="+lang);
			var page = this;
			
			// Web UI licenses
			License.getLicense(this.type, lang,{
				load: function(msg) {
				    var licenseNode = dojo.byId("li_licenseContent");
					dojo.attr(licenseNode, "lang", lang.toLowerCase().replace('_','-'));
					page.licenseContent.set("value",msg);
		    		page._displayedLang = lang;
				},
				error: function(msg, ioargs) {
		    		page.licenseContent.set("value",msg);	
					Utils.checkXhrError(ioargs);
				}
		    });
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
	return LicenseForm;
});
