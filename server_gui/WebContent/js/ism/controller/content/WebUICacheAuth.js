/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
		'dojo/number',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',		
		'ism/widgets/TextBox',
		'ism/config/Resources',
		'ism/widgets/ToggleContainer',
		'ism/IsmUtils',
		'dojo/i18n!ism/nls/appliance',
		'dojo/text!ism/controller/content/templates/CacheAuth.html'
		], function(declare, lang, xhr, json, domConstruct, number, _Widget, _TemplatedMixin, 
				_WidgetsInTemplateMixin, TextBox, Resources, ToggleContainer, Utils, nls, template) {

	var CacheAuth = declare("ism.controller.content.CacheAuth", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
        Utils: Utils,
        
        // template strings
        cacheauthLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.cacheauth.cacheauth),
        unitLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.cacheauth.unit),
        invalidCacheauth: Utils.escapeApostrophe(nls.appliance.webUISecurity.cacheauth.invalid.cacheauth),
        cacheauthTooltip: Utils.escapeApostrophe(nls.appliance.webUISecurity.cacheauth.cacheauthTooltip),
		startingValues: {},

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
		
		startup: function(args) {

			this.inherited(arguments);

			var t = this;
			var cacheauthField = this.cacheauthField;
			var cacheauthValid = true;
			var startingValues = this.startingValues;
	
		    var saveButton = this.saveButton;
			saveButton.set('disabled', true);			
			
			
			startingValues.cacheauth = "";
			cacheauthField.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			cacheauthField.isValid = function(focused) {
				var value = this.get("value");
				if (value === null || value === undefined || value === "") {
					saveButton.set('disabled', true);
					return false;
				}
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				var max = 3600;
				if (isNaN(intValue) || intValue < 1 || intValue > max) {
					saveButton.set('disabled', true);
					cacheauthValid = false;
					return false;
				} else {
					cacheauthValid = true;
					// Don't enable the save button if the value has not changed
					if (startingValues.cacheauth != value) {
						saveButton.set('disabled', false);					
					} else {
						saveButton.set('disabled', true);	
					}
					return true;
				}
				
			};
			

			this._initCacheauthField();
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.webui.nav.webuiSecuritySettings.topic, lang.hitch(this, function(message){
				this._initCacheauthField();
			}));	

					
		},
				
		saveCacheauth: function() {
			if (this.cacheauth.validate()) {
				this._updateSaveOutput(this, "saving");
				this._saveCacheauthField();
			}
		},
		


		
		_initCacheauthField: function() {
			  var cacheauthField = this.cacheauthField;
			  var startingValues = this.startingValues;
	          dojo.xhrGet({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('authCache_timeout'),
				    handleAs: "json",
				    preventCache: true,
	                load: function(data) {
	                	var cacheauth = data['authCache_timeout'];
	                	if (cacheauth) {
	                		// remove the s that represents seconds
	                		cacheauth = cacheauth.replace("s" , "");
                     		startingValues.cacheauth = cacheauth;	                		
	                		cacheauthField.set('value', cacheauth);
	                	}
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.cacheauth.getCacheauthError, error, ioargs);
					}
			 }); 			
		},


		_saveCacheauthField: function() {
			 var t = this;
			 var value = number.parse(t.cacheauthField.get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
			 if (t.startingValues.cacheauth == value) {
				 console.debug("Cacheauth value unchanged");
				 t._updateSaveOutput(t);
				 return;
			 }
			 var data = {};
			 // Add the s to represent seconds before the value is saved
			 value = value + "s";
			 data.value = "" + value;

             dojo.xhrPut({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('authCache_timeout'),
				    handleAs: "json",
	  				putData: JSON.stringify(data),
	                load: function(data) {
	                	t.startingValues.cacheauth = value;
	   				 	t._updateSaveOutput(t, "success");
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.cacheauth.setCacheauthError, error, ioargs);
						t._updateSaveOutput(t, "failed");
					}
			 }); 			
		},
		
		_updateSaveOutput: function (t, success) {
			switch (success) {
			case "saving":
    			t.changeCacheauthOutputText.innerHTML = nls.appliance.webUISecurity.cacheauth.saving;
    			t.changeCacheauthOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;			
			case "success":
    			t.changeCacheauthOutputText.innerHTML = nls.appliance.webUISecurity.cacheauth.success;
    			t.changeCacheauthOutputImage.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;
			case "failed":				
    			t.changeCacheauthOutputText.innerHTML = nls.appliance.webUISecurity.cacheauth.failed;
    			t.changeCacheauthOutputImage.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>";
    			t.saveButton.set('disabled', false);    			
    			break;
			default:
    			t.changeCacheauthOutputText.innerHTML = "";
    			t.changeCacheauthOutputImage.innerHTML = "";	
    			break;
    			
			} 
		}
						
	});
	
	var WebUICacheAuth = declare("ism.controller.content.WebUICacheAuth", [ToggleContainer], {
	
		startsOpen: true,
		contentId: "webUICacheAuth",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.appliance.webUISecurity.cacheauth.subtitle;
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
				"<span class='tagline'>" + nls.appliance.webUISecurity.cacheauth.tagline + "</span><br />",
				"<div id='"+this.contentId+"'></div>"
			].join("\n");
			domConstruct.place(nodeText, this.toggleNode);
			
			var formId = this.contentId+'Form';
			this.cacheauth = new CacheAuth({formId: formId}, this.contentId);
			this.cacheauth.startup();					
		}
	});

	return WebUICacheAuth;
});
