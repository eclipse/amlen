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
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/_Widget',
		"dijit/focus",
		'dijit/form/Button',
		'dijit/form/Form',
		"dijit/registry",
		"dojo/dom-style",
		'ism/widgets/TextBox',
		'dojox/layout/TableContainer',
		'ism/config/Resources',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/login',
		"dojo/io-query",
		"dojo/query",
		"dojo/dom-geometry",
		'dojo/text!ism/widgets/templates/LoginForm.html',
		'idx/string',
		'ism/IsmUtils'
], function(declare, lang, _TemplatedMixin, _WidgetsInTemplateMixin, _Widget, Focus, Button, Form, registry, domStyle, TextBox, TableContainer, Resources, 
				nlsglobal, nls, ioquery, query, domGeom, template, iString, Utils) {

	var LoginForm = declare("ism.widgets.LoginForm", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {

		nls: nls,
		nlsglobal: nlsglobal,
		templateString: template,
		nodeName: "",
		windowTitle: Utils.escapeApostrophe(nlsglobal.global.webUIMainTitle + " - " + nlsglobal.global.login),
		productNameTM: Utils.escapeApostrophe(nls.loginForm.productNameTM),
		trademarks: Utils.escapeApostrophe(nls.loginForm.trademarks),
		copyright: Utils.escapeApostrophe(nls.loginForm.copyright),
		
		constructor: function(args) {
		    dojo.safeMixin(this, args);
	        this.inherited(arguments);
	    },

		startup: function() {
			this.inherited(arguments);

			var uri = window.location.href;
			var queryString = uri.substring(uri.indexOf("?") + 1, uri.length);
			if (typeof(ioquery.queryToObject(queryString).error) != "undefined") {
				query(".loginError").style("display", "block");
				this.windowTitle = nlsglobal.global.webUIMainTitle + " - " + nls.loginForm.errorTitle;
			}

			if (this.nodeName !== "") {
				this.windowTitle = this.nodeName + ": " + this.windowTitle;
			}
			
			var userIdWidget = registry.byId("login_userIdInput");
			var passwordWidget = registry.byId("login_passwordInput");
			
			// IDX 1.4.2 appears to have a bug in _CompositeMixin. It is setting
			// visibility to hidden and not clearing it again. This occurs in the parent startup.
			domStyle.set(userIdWidget.domNode, {visibility: ""});
			domStyle.set(passwordWidget.domNode, {visibility: ""});
			
			var labelWidth = this._getLabelWidth(passwordWidget);
			var userWidth = this._getLabelWidth(userIdWidget);
			
			if (userWidth >= labelWidth) {
				labelWidth = userWidth;
			} 
			
			passwordWidget.set("labelWidth", labelWidth);
			userIdWidget.set("labelWidth", labelWidth);
			
			window.document.title = this.windowTitle;
		},
		
		_getLabelWidth: function(inputWidget) {
			
			var node = inputWidget.domNode;

			var computedStyle = domStyle.get(node);
		    var box = domGeom.getContentBox(node, computedStyle);
			
			var fieldWidth = inputWidget.get("fieldWidth");
			var labelWidth = box.w - fieldWidth - 10;	
			
			return labelWidth;
			
		},
		
		login: function() {
			if (this.loginForm.validate()) {
				var processedUser = this.userId.get('value').replace('\\', '\\\\\\').replace(',', '\\\\,').replace('+', '\\\\+').replace('"', '\\\\"').replace('<', '\\\\<').replace('>', '\\\\>').replace(';', '\\\\;');			
				dojo.byId("login_processedUsername").value = processedUser;				
				return true;
			}
			if (iString.nullTrim(this.userId.get('value')) === null) {
				this.userId.focus();
			} else if (iString.nullTrim(this.password.get('value')) === null) {
				this.password.focus();
			}
			return false;
		}
	});
	return LoginForm;
});
