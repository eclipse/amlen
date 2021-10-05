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
        'ism/IsmUtils',
        'dojo/i18n!ism/nls/appliance',
        'dojo/text!ism/controller/content/templates/PortForm.html',
    	'idx/form/ComboBox'
        ], function(declare, lang, xhr, json, domConstruct, number, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, TextBox, Resources, ToggleContainer, Utils, IsmUtils, nls, template) {

	var PortForm = declare("ism.controller.content.PortForm", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		templateString: template,
		startingValues: {'port':'', 'ipAddr': ''},
		all: IsmUtils.escapePsuedoTranslation(nls.appliance.webUISecurity.port.all),

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		startup: function(args) {
			this.inherited(arguments);

			var t = this;
			var ipAddr = this.field_ipAddr;		
			var port = this.field_port;

			port.isValid = function(focused) {
				console.debug("validate");
				t._updateSaveOutput(this, "clear");
				var value = this.get("value");
				if (value === null || value === undefined || value === "") {
					t.portIsValid = false;
				} else {
					var intValue = number.parse(value, {places: 0});
					if (isNaN(intValue) || intValue < 1 || intValue > 65535) {
						t.portIsValid = false;
					} else if ((intValue < 9000)||(intValue > 9099)||(intValue == 9087)){
						t.portIsValid = true;
					} else {
						t.portIsValid = false;
					}
				}
				t._updateSaveButton(t);
				return t.portIsValid;
			};
			

			this._setupFields();
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.webui.nav.webuiSecuritySettings.topic, lang.hitch(this, function(message){
				this._setupFields();
			}));	

			
		},

		updateInterfaceHint: function() {
			this._updateSaveOutput(this, "clear");
			this._updateSaveButton(this);
			var value = this.field_ipAddr.get('value');
			this.field_ipAddr.store.fetch({
				query: { id: value },
				onComplete: lang.hitch(this, function(results) {
						var item = results[0];
						if (item) {
							var intLabel = item.ethernetInterface[0] == "All" ? nls.appliance.webUISecurity.port.all : item.ethernetInterface[0]; 							
						}
				})
			});			
		},

		savePort: function() {
			if (this.portForm.validate()) {
				this._saveFields();
			}
		},
		
		_updateSaveButton: function(t) {
			if (!t.saveButton) {
				// too early
				return;
			}
			var port = t.field_port.get('value');
			if (t.portIsValid && port !== "") {
				var ipAddr = t.field_ipAddr.get('value');				
				if (t.startingValues.port != port || t.startingValues.ipAddr != ipAddr) {
					t.saveButton.set('disabled', false);
				} else {
					t.saveButton.set('disabled', true);
				}
			} else {
				t.saveButton.set('disabled', true);
			}
		},

		// do any adjustment of form field values necessary before showing the form.  in this
		// case, we populate the ipAddr ComboBox with valid ethernet IP addresses via REST API call
		_setupFields: function() {
			console.debug("setupFields");
			
			// Populate IP addresses
			var addAddrs = lang.hitch(this, function() {
				dojo.xhrGet({
					url: Utils.getBaseUri() + "rest/config/appliance/network-interfaces/",
					handleAs: "json",
					preventCache: true,
					load: lang.hitch(this, function(data) {
						console.debug("adding addrs", data);				
						this.field_ipAddr.store.newItem({ name: nls.appliance.webUISecurity.port.all, id: "All", ethernetInterface: "All"});
						if (data.interfaces) {
							var ipv4 = [];
							var ipv6 = [];
							for (var i in data.interfaces) {
								var ethernetInterface = i;								
								if (data.interfaces[i].ipaddress) {
									var address = data.interfaces[i].ipaddress;
										// we want to trim off the subnet mask from the IP address
										if (address.indexOf("/") > 0) {
											address = address.substring(0, address.indexOf("/"));
										}
										if (address.match(/[A-Fa-f]/) || address.match(/[:]/)) {
											ipv6.push({name: address, id: "["+address+"]", ethernetInterface: ethernetInterface });	
										} else {
											ipv4.push({name: address, id: address, ethernetInterface: ethernetInterface });
										}
								}
							}
							for (var l=0, ipv4Len=ipv4.length; l < ipv4Len; l++) {
								this.field_ipAddr.store.newItem(ipv4[l]);
							}
							for (var k=0, ipv6Len=ipv6.length; k < ipv6Len; k++) {
								this.field_ipAddr.store.newItem(ipv6[k]);
							}
							this.field_ipAddr.store.save();
							
							// now get the current values
							this._initFields();			
						}
					}),
					error: lang.hitch(this, function(error, ioargs) {
						console.debug("error", error);
						Utils.checkXhrError(ioargs);
					})
				});
			});
			this.field_ipAddr.store.fetch({
				onComplete: lang.hitch(this, function(items, request) {
					console.debug("deleting items");
					var len = items ? items.length : 0;
					for (var i=0; i < len; i++) {
						this.field_ipAddr.store.deleteItem(items[i]);
					}
					this.field_ipAddr.store.save({ onComplete: addAddrs() });
				})
			});
		},

		_initFields: function() {
			var ipAddr = this.field_ipAddr;
			var port = this.field_port;
			
			this.saveButton.set('disabled', true);
			this.portIsValid = true;

			var startingValues = this.startingValues;
			dojo.xhrGet({
				headers: {
					"Content-Type": "application/json"
				},
				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('default_http_host'),
				handleAs: "json",
				preventCache: true,
				load: function(data) {
					var value = data['default_http_host'];
					if (value) {
						if (value == "*") {
							value = nls.appliance.webUISecurity.port.all;
						}
						startingValues.ipAddr = value;	                		
						ipAddr.set('value', value);
					}
				},
				error: function(error,ioargs) {
					Utils.checkXhrError(ioargs);
					Utils.showPageError(nls.appliance.webUISecurity.port.getIPAddressError, error, ioargs);
				}
			});
			dojo.xhrGet({
				headers: {
					"Content-Type": "application/json"
				},
				url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('default_https_port'),
				handleAs: "json",
				preventCache: true,
				load: function(data) {
					var value = data['default_https_port'];
					if (value) {
						startingValues.port = value;	                		
						port.set('value', value);
					}
				},
				error: function(error,ioargs) {
					Utils.checkXhrError(ioargs);
					Utils.showPageError(nls.appliance.webUISecurity.port.getPortError, error, ioargs);
				}
			}); 				          
		},

		_saveFields: function() {
			var t = this;
			var changed = false;
			var ipAddr = t.field_ipAddr.get("value");
			var port = t.field_port.get("value");
			var deferreds = [];
			var failedMessages = [];
			if (t.startingValues.ipAddr != ipAddr ) {
				failedMessages.push(nls.appliance.webUISecurity.port.setIPAddressError);
				var value = ipAddr == "All" ? "*" : ipAddr;
				deferreds.push(
						dojo.xhrPut({
							headers: { "Content-Type": "application/json"},
							url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('default_http_host'),
							handleAs: "json",
							putData: JSON.stringify({"value" : value})
						}));
			} 
			if (t.startingValues.port != port ) {
				failedMessages.push(nls.appliance.webUISecurity.port.setPortError);
				deferreds.push(
						dojo.xhrPut({
							headers: { "Content-Type": "application/json"},
							url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('default_https_port'),
							handleAs: "json",
							putData: JSON.stringify({"value" : ""+port})
						}));
			}

			if (deferreds.length === 0) {
				// shouldn't have gotten here...
				t._updateSaveOutput(t, "clear");
				console.debug("nothing changed");
				return;
			}

			t._updateSaveOutput(t, "saving");

			// handle the results
			var dl = new dojo.DeferredList(deferreds);			
			dl.then(lang.hitch(this,function(results) {
				var success = true;
				var len = deferreds ? deferreds.length : 0;
				for (var i=0; i < len; i++) {
					if (results[i][0] === false) {
						var ioargs = results[0][1];
						Utils.checkXhrError(ioargs);
						Utils.showPageError(failedMessages[i], ioargs.responseText, ioargs);
						success = false;
					}
				}
				if (success === true) {
					t.startingValues.port = port;
					t.startingValues.ipAddr = ipAddr;
					t._updateSaveOutput(t, "success");						
				} else {
					t._updateSaveOutput(t, "failed");
				}
			}));
		},

		_updateSaveOutput: function (t, state, message) {
			if (!t.changePortOutputText) {
				// not ready yet...
				return;
			}
			switch (state) {
			case "saving":
				t.changePortOutputText.innerHTML = message ? message : nls.appliance.webUISecurity.port.saving;
				t.changePortOutputImage.innerHTML = "<img src=\"css/images/loading_wheel.gif\" alt=''/>";
				t.saveButton.set('disabled', true);
				break;	
			case "success":
				t.changePortOutputText.innerHTML = message ? message : nls.appliance.webUISecurity.port.success;
				t.changePortOutputImage.innerHTML = "<img src=\"css/images/msgSuccess16.png\" alt=''/>";				
				t.saveButton.set('disabled', true);
				break;					
			case "failed":				
				t.changePortOutputText.innerHTML  = message ? message : nls.appliance.webUISecurity.port.failed;
				t.changePortOutputImage.innerHTML = "<img src=\"css/images/msgError16.png\" alt=''/>";
				t.saveButton.set('disabled', false);    			
				break;
			default:
				t.changePortOutputText.innerHTML = "";
				t.changePortOutputImage.innerHTML = "";
				break;
			} 
		}						
	});

	var WebUIPort = declare("ism.controller.content.WebUIPort", [ToggleContainer], {

		startsOpen: true,
		contentId: "webUIPort",

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.appliance.webUISecurity.port.subtitle;
			this.triggerId = this.contentId+"_trigger";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId);
		},

		postCreate: function() {
			this.inherited(arguments);			
			this.toggleNode.innerHTML = "";

			var nodeText = [
			                "<span class='tagline'>" + nls.appliance.webUISecurity.port.tagline + "</span><br />",
			                "<div id='"+this.contentId+"'></div>"
			                ].join("\n");
			domConstruct.place(nodeText, this.toggleNode);

			var formId = this.contentId+'Form';
			this.portForm = new PortForm({formId: formId}, this.contentId);
			this.portForm.startup();					
		}
	});

	return WebUIPort;
});
