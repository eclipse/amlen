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
		'dojo/number',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',		
		'ism/widgets/TextBox',
		'ism/config/Resources',
		'ism/widgets/ToggleContainer',
		'ism/IsmUtils',
		'dojo/i18n!ism/nls/appliance',
		'dojo/text!ism/controller/content/templates/TimeoutsForm.html',
		'dojo/dom-style',
		'dojo/dom-geometry'
		], function(declare, lang, xhr, json, domConstruct, number, _Widget, _TemplatedMixin, 
				_WidgetsInTemplateMixin, TextBox, Resources, ToggleContainer, Utils, nls, template,
				domStyle, domGeom) {

	var TimeoutsForm = declare("ism.controller.content.TimeoutsForm", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
		startingValues: {},
		Utils: Utils,
		
		// template strings
		inactivityLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.inactivity),
        unitLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.unit),
        invalidInactivity: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.invalid.inactivity),
        inactivityTooltip: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.inactivityTooltip),
        expirationLabel: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.expiration),
        invalidExpiration: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.invalid.expiration),
        expirationTooltip: Utils.escapeApostrophe(nls.appliance.webUISecurity.timeout.expirationTooltip),

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
		
		startup: function(args) {
			this.inherited(arguments);

			var t = this;
			var expirationField = this.expirationField;
			var expirationValid = true;
			var inactivityField = this.inactivityField;
			var inactivityValid = true;
			var startingValues = this.startingValues;

			var saveButton = this.saveButton;
			saveButton.set('disabled', true);			
			
			startingValues.inactivity = "";
			inactivityField.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));			
			inactivityField.isValid = function(focused) {
				t._updateSaveOutput(t);
				var value = this.get("value");
				var expirationValue = expirationField.get("value");
				if (value === null || value === undefined || value === "") {
					saveButton.set('disabled', true);
					inactivityValid = false;
					return false;
				}
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				var max = 1440;
				if (expirationValue !== null && expirationValue !== undefined && expirationValid) {
					var expIntValue = number.parse(expirationValue, {places: 0, locale: ism.user.localeInfo.localeString});
					if (!isNaN(expIntValue) && expIntValue > 0 && expIntValue <= max) {
						max = expIntValue;
					}
				}
				if (isNaN(intValue) || intValue < 1 || intValue > max) {
					saveButton.set('disabled', true);
					inactivityValid = false;					
					return false;
				}
				inactivityValid = true;
				if (!expirationValid) {
					expirationField.validate();
				}
				if (expirationValid && startingValues.inactivity != value) {
					saveButton.set('disabled', false);					
				}
				return true;
			};
			
			startingValues.expiration = "";
			expirationField.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));
			expirationField.isValid = function(focused) {
				var value = this.get("value");
				if (value === null || value === undefined || value === "") {
					saveButton.set('disabled', true);
					return false;
				}				
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				var max = 1440;
				var min = 1;
				var inactivityValue = inactivityField.get('value');
				if (inactivityValue !== null && inactivityValue !== undefined && inactivityValid) {
					var inactivityIntValue = number.parse(inactivityValue, {places: 0, locale: ism.user.localeInfo.localeString});
					if (!isNaN(inactivityIntValue) && inactivityIntValue > 0) {
						min = inactivityIntValue;
					}					
				}
				if (isNaN(intValue) || intValue < min || intValue > max) {
					saveButton.set('disabled', true);
					expirationValid = false;
					return false;
				}
				expirationValid = true;
				if (!inactivityValid) {
					inactivityField.validate();
				}
				if (inactivityValid && (startingValues.expiration != value || startingValues.inactivity != inactivityValue)) {
					saveButton.set('disabled', false);					
				}
								
				return true;
			};
			
			this._initInactivityField();
			this._initExpirationField();
			
			var labelWidth = this._getLabelWidth(this.inactivityField);
			var width2 = this._getLabelWidth(this.expirationField);
			
			if (labelWidth >= width2) {
				labelWidth = width2;
			} 
			
			this.inactivityField.set("labelWidth", labelWidth);
			this.expirationField.set("labelWidth", labelWidth);	
			
			// IDX 1.4.2 appears to have a bug in _CompositeMixin. It is setting
			// visibility to hidden and not clearing it again. This occurs in the parent startup.
			domStyle.set(this.inactivityField.domNode, {visibility: ""});
			domStyle.set(this.expirationField.domNode, {visibility: ""});
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.webui.nav.webuiSecuritySettings.topic, lang.hitch(this, function(message){
				this._initInactivityField();
				this._initExpirationField();
			}));	
					
		},
		
		_getLabelWidth: function(widget) {
			
			var node = widget.domNode;
			var computedStyle = domStyle.get(node);
		    var box = domGeom.getContentBox(node, computedStyle);
		    var fieldWrapNode = widget.fieldWrap;
		    var fieldWrapComputedStyle = domStyle.get(fieldWrapNode);
		    var fieldWrapBox = domGeom.getContentBox(fieldWrapNode, fieldWrapComputedStyle);
			
			var labelWidth = box.w - fieldWrapBox.w + 30; // make sure there is room for the help icon	
			
			return labelWidth;			
		},
				
		saveTimeouts: function() {
			if (this.timeoutsForm.validate()) {
				this._updateSaveOutput(this, "saving");
				this._saveInactivityField();			
			}
		},
		

		_initInactivityField: function() {
			  var inactivityField = this.inactivityField;
			  var startingValues = this.startingValues;
	          dojo.xhrGet({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('httpSession_invalidationTimeout'),
				    handleAs: "json",
				    preventCache: true,
	                load: function(data) {
	                	var timeout = data['httpSession_invalidationTimeout'];
	                	if (timeout) {
	                		var value = timeout/60;
	                		startingValues.inactivity = value;	                		
	                		inactivityField.set('value', value);
	                	}
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.timeout.getInactivityError, error, ioargs);
					}
			 }); 			
		},
		
		_initExpirationField: function() {
			  var expirationField = this.expirationField;
			  var startingValues = this.startingValues;
	          dojo.xhrGet({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('ltpa_expiration'),
				    handleAs: "json",
				    preventCache: true,
	                load: function(data) {
	                	var expiration = data['ltpa_expiration'];
	                	if (expiration) {
	                		startingValues.expiration = expiration;	                		
	                		expirationField.set('value', expiration);
	                	}
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.timeout.getExpirationError, error, ioargs);
					}
			 }); 			
		},

		_saveInactivityField: function() {
			 var t = this;
			 var value = number.parse(t.inactivityField.get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
			 if (t.startingValues.inactivity == value) {
				 console.debug("Inactivity value unchanged");
				 t._saveExpirationField(t, "success");
				 return;
			 }
			 var data = {};
			 data.value = "" + value * 60;
	         dojo.xhrPut({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('httpSession_invalidationTimeout'),
				    handleAs: "json",
	  				putData: JSON.stringify(data),
	                load: function(data) {
	                	t.startingValues.inactivity = value;
	                	t._saveExpirationField(t, "success");
	                	ism.user.setSessionTimeout(value * 60);
	                },
	                error: function(error,ioargs) {	                	
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.timeout.setInactivityError, error, ioargs);
						t._saveExpirationField(t, "failed");
					}
			 }); 			
		},

		_saveExpirationField: function(t, success) {
			 var value = number.parse(t.expirationField.get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
			 if (t.startingValues.expiration == value) {
				 console.debug("Expiration value unchanged");
				 t._updateSaveOutput(t, success);
				 return;
			 }
			 var data = {};
			 data.value = "" + value;

             dojo.xhrPut({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('ltpa_expiration'),
				    handleAs: "json",
	  				putData: JSON.stringify(data),
	                load: function(data) {
	                	t.startingValues.expiration = value;
	   				 	t._updateSaveOutput(t, success);
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.timeout.setExpirationError, error, ioargs);
						t._updateSaveOutput(t, "failed");
					}
			 }); 			
		},
		
		_updateSaveOutput: function (t, success) {
			switch (success) {
			case "saving":
    			t.changeTimeoutsOutputText.innerHTML = nls.appliance.webUISecurity.timeout.saving;
    			t.changeTimeoutsOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;			
			case "success":
    			t.changeTimeoutsOutputText.innerHTML = nls.appliance.webUISecurity.timeout.success;
    			t.changeTimeoutsOutputImage.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;
			case "failed":				
    			t.changeTimeoutsOutputText.innerHTML = nls.appliance.webUISecurity.timeout.failed;
    			t.changeTimeoutsOutputImage.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>";
    			t.saveButton.set('disabled', false);    			
    			break;
			default:
    			t.changeTimeoutsOutputText.innerHTML = "";
    			t.changeTimeoutsOutputImage.innerHTML = "";	
    			break;
    			
			} 
		}
						
	});
	
	var WebUITimeouts = declare("ism.controller.content.WebUITimeouts", [ToggleContainer], {
	
		startsOpen: true,
		contentId: "webUITimeouts",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.appliance.webUISecurity.timeout.subtitle;
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
				"<span class='tagline'>" + nls.appliance.webUISecurity.timeout.tagline + "</span><br />",
				"<div id='"+this.contentId+"'></div>"
			].join("\n");
			domConstruct.place(nodeText, this.toggleNode);
			
			var formId = this.contentId+'Form';
			this.timeoutsForm = new TimeoutsForm({formId: formId}, this.contentId);
			this.timeoutsForm.startup();					
		}
	});

	return WebUITimeouts;
});
