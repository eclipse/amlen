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
		'ism/Utils',
		'dojo/i18n!ism/nls/appliance',
		'dojo/text!ism/controller/content/templates/ServiceForm.html'
		], function(declare, lang, xhr, json, domConstruct, number, _Widget, _TemplatedMixin, 
				_WidgetsInTemplateMixin, TextBox, Resources, ToggleContainer, Utils, nls, template) {

	var ServiceForm = declare("ism.controller.content.ServiceForm", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
		startingValues: {},

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
		
		startup: function(args) {
			this.inherited(arguments);
			
			this.saveButton.set('disabled', true);
			
			this._initFields();
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.webui.nav.webuiSecuritySettings.topic, lang.hitch(this, function(message){
				this._initFields();
			}));	
					
		},
				
		_initFields: function() {
			  var traceSpecification = this.traceSpecification;
			  var traceEnabled = this.traceEnabled;
			  var startingValues = this.startingValues;
	          dojo.xhrGet({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('logging_traceSpecification'),
				    handleAs: "json",
				    preventCache: true,
	                load: function(data) {
	                	var trace = data['logging_traceSpecification'];
	                	if (!trace) {
	                		trace = "";
	                	}
                		startingValues.trace = trace;
                		startingValues.enabled = trace !== "";
                		traceSpecification.set('value', trace);	                	
	                	traceEnabled.set('checked', startingValues.enabled);
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nls.appliance.webUISecurity.service.getTraceError, error, ioargs);
					}
			 }); 			
		},
		
		saveTrace: function() {			
			if (this.serviceForm.validate()) {
				 this._updateSaveOutput(this, "saving");			
				 var data = {};
				 var traceSpecification = this.traceSpecification.get("value");
				 var traceEnabled = this.traceEnabled.get("checked");
				 data.value = traceEnabled ? traceSpecification : "";
				 var errorMessage = data.value === "" ? nls.appliance.webUISecurity.service.clearTraceError : nls.appliance.webUISecurity.service.setTraceError;
				 var t = this;
		         dojo.xhrPut({
		        	    headers: {
							"Content-Type": "application/json"
						},
					    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('logging_traceSpecification'),
					    handleAs: "json",
		  				putData: JSON.stringify(data),
		                load: function(data) {
		                	t.startingValues.trace = data.value;
		                	t.startingValues.enabled = traceEnabled;
		                	t._updateSaveOutput(t, "success");
		                },
		                error: function(error,ioargs) {	                	
							Utils.checkXhrError(ioargs);
							Utils.showPageError(errorMessage, error, ioargs);
							t._updateSaveOutput(t, "failed");
						}
				 }); 			
			}
		},
		
		setTraceEnabled: function(checked)	{
    		this.traceSpecification.set('disabled', !checked);
    		if (this.startingValues.enabled != checked) {
    			this._updateSaveOutput(this, "clear");
    		}
		},
		
		traceSpecificationChanged: function(traceSpecification) {
			if (this.startingValues.trace != traceSpecification) {
				this._updateSaveOutput(this, "clear");
			}
		},
		
		_updateSaveOutput: function (t, success) {	
			switch (success) {
			case "saving":
    			t.changeTraceOutputText.innerHTML = nls.appliance.webUISecurity.service.saving;
    			t.changeTraceOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;			
			case "success":
    			t.changeTraceOutputText.innerHTML = nls.appliance.webUISecurity.service.success;
    			t.changeTraceOutputImage.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>";
    			t.saveButton.set('disabled', true);
    			break;
			case "failed":				
    			t.changeTraceOutputText.innerHTML = nls.appliance.webUISecurity.service.failed;
    			t.changeTraceOutputImage.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>";
    			t.saveButton.set('disabled', false);    			
    			break;
			default:
    			t.changeTraceOutputText.innerHTML = "";
    			t.changeTraceOutputImage.innerHTML = "";
    			t.saveButton.set('disabled', false);    			
    			break;
    			
			} 
		}
						
	});
	
	var WebUIService = declare("ism.controller.content.WebUIService", [ToggleContainer], {
	
		startsOpen: true,
		contentId: "webUIService",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.appliance.webUISecurity.service.subtitle;
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId, false);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
				"<span class='tagline'>" + nls.appliance.webUISecurity.service.tagline + "</span><br />",
				"<div id='"+this.contentId+"'></div>"
			].join("\n");
			domConstruct.place(nodeText, this.toggleNode);
			
			var formId = this.contentId+'Form';
			this.serviceForm = new ServiceForm({formId: formId}, this.contentId);
			this.serviceForm.startup();					
		}
	});

	return WebUIService;
});
