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
		'dojo/text!ism/controller/content/templates/ReadTimeout.html'
		], function(declare, lang, xhr, json, domConstruct, number, _Widget, _TemplatedMixin, 
				_WidgetsInTemplateMixin, TextBox, Resources, ToggleContainer, Utils, nls, template) {

	var ReadTimeout = declare("ism.controller.content.ReadTimeout", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
        Utils: Utils,
        
        // template strings
        readTimeoutLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.readTimeout.readTimeout),
        unitLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.readTimeout.unit),
        invalidReadTimeout: Utils.escapeApostrophe(nls.appliance.webUISecurity.readTimeout.invalid.readTimeout),
        readTimeoutTooltip: Utils.escapeApostrophe(nls.appliance.webUISecurity.readTimeout.readTimeoutTooltip),
		startingValues: {},

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
		
		startup: function(args) {

			this.inherited(arguments);

			var t = this;
			var readTimeoutField = this.readTimeoutField;
			var readTimeoutValid = true;
			var startingValues = this.startingValues;
	
		    var saveButton = this.saveButton;
			saveButton.set('disabled', true);			
			
			
			startingValues.readTimeout = "";
			readTimeoutField.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			readTimeoutField.isValid = function(focused) {
				var value = this.get("value");
				if (value === null || value === undefined || value === "") {
					saveButton.set('disabled', true);
					return false;
				}
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				var max = 300;
				if (isNaN(intValue) || intValue < 1 || intValue > max) {
					saveButton.set('disabled', true);
					readTimeoutValid = false;
					return false;
				} else {
					readTimeoutValid = true;
					// Don't enable the save button if the value has not changed
					if (startingValues.readTimeout != value) {
						saveButton.set('disabled', false);					
					} else {
						saveButton.set('disabled', true);	
					}
					return true;
				}
				
			};
			

			this._initReadTimeoutField();
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.webui.nav.webuiSecuritySettings.topic, lang.hitch(this, function(message){
				this._initReadTimeoutField();
			}));	

					
		},
				
		saveReadTimeout: function() {
			if (this.readTimeout.validate()) {
				this._updateSaveOutput(this, "saving");
				this._saveReadTimeoutField();
			}
		},
		


		
		_initReadTimeoutField: function() {
			  var readTimeoutField = this.readTimeoutField;
			  var startingValues = this.startingValues;
	          dojo.xhrGet({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('httpOptions_readTimeout'),
				    handleAs: "json",
				    preventCache: true,
	                load: function(data) {
	                	var timeout = data['httpOptions_readTimeout'];
	                	if (timeout) {
	                		// remove the s that represents seconds
	                		timeout = timeout.replace("s" , "");
                     		startingValues.readTimeout = timeout;	                		
                     		readTimeoutField.set('value', timeout);
	                	}
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.readTimeout.getReadTimeoutError, error, ioargs);
					}
			 }); 			
		},


		_saveReadTimeoutField: function() {
			 var t = this;
			 var value = number.parse(t.readTimeoutField.get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
			 if (t.startingValues.readTimeout == value) {
				 console.debug("ReadTimeout value unchanged");
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
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('httpOptions_readTimeout'),
				    handleAs: "json",
	  				putData: JSON.stringify(data),
	                load: function(data) {
	                	t.startingValues.readTimeout = value;
	   				 	t._updateSaveOutput(t, "success");
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.readTimeout.setReadTimeoutError, error, ioargs);
						t._updateSaveOutput(t, "failed");
					}
			 }); 			
		},
		
		_updateSaveOutput: function (t, success) {
			switch (success) {
			case "saving":
    			t.changeReadTimeoutOutputText.innerHTML = nls.appliance.webUISecurity.readTimeout.saving;
    			t.changeReadTimeoutOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;			
			case "success":
    			t.changeReadTimeoutOutputText.innerHTML = nls.appliance.webUISecurity.readTimeout.success;
    			t.changeReadTimeoutOutputImage.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;
			case "failed":				
    			t.changeReadTimeoutOutputText.innerHTML = nls.appliance.webUISecurity.readTimeout.failed;
    			t.changeReadTimeoutOutputImage.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>";
    			t.saveButton.set('disabled', false);    			
    			break;
			default:
    			t.changeReadTimeoutOutputText.innerHTML = "";
    			t.changeReadTimeoutOutputImage.innerHTML = "";	
    			break;
    			
			} 
		}
						
	});
	
	var WebUIReadTimeout = declare("ism.controller.content.WebUIReadTimeout", [ToggleContainer], {
	
		startsOpen: true,
		contentId: "webUIReadTimeout",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.appliance.webUISecurity.readTimeout.subtitle;
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
				"<span class='tagline'>" + nls.appliance.webUISecurity.readTimeout.tagline + "</span><br />",
				"<div id='"+this.contentId+"'></div>"
			].join("\n");
			domConstruct.place(nodeText, this.toggleNode);
			
			var formId = this.contentId+'Form';
			this.readTimeout = new ReadTimeout({formId: formId}, this.contentId);
			this.readTimeout.startup();					
		}
	});

	return WebUIReadTimeout;
});
