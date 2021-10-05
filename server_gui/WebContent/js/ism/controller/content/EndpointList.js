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
		'dojo/_base/array',
		'dojo/query',
		'dojo/aspect',
		'dojo/on',
		'dojo/when',
		'dojo/dom-attr',
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/store/Memory',
		'dojo/data/ItemFileWriteStore',
		'dojox/html/entities',
		'dojo/number',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/registry',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'idx/form/CheckBoxSelect',
		'ism/controller/content/ReferencedObjectGrid',
		'ism/controller/content/OrderedReferencedObjectGrid',
		'ism/widgets/GridFactory',
		'ism/IsmUtils',
		'ism/MonitoringUtils',
		'dojo/text!ism/controller/content/templates/EndpointDialog.html',
		'dojo/text!ism/controller/content/templates/AddPoliciesDialog.html',
		'dojo/text!ism/controller/content/templates/AddProtocolsDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/messages',
		'ism/config/Resources',
		'ism/widgets/MessageHubAssociationControl',
		'ism/widgets/TextBox',
		'ism/widgets/TextArea',
		'ism/widgets/IsmQueryEngine',
		'dojo/keys',
		'ism/widgets/NoticeIcon',
		'idx/form/ComboBox'
		], function(declare, lang, array, query, aspect, on, when, domAttr, domClass, domConstruct, domStyle, Memory,
				ItemFileWriteStore, entities, number, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, registry,
				Button, Dialog, CheckBoxSelect, ReferencedObjectGrid, OrderedReferencedObjectGrid, GridFactory, Utils,
				MonitoringUtils, endpointDialog, addPoliciesDialog,	addProtocolsDialog, nls, nls_strings, nls_messages,
				Resources, MessageHubAssociationControl, TextBox, TextArea, IsmQueryEngine, keys, NoticeIcon,ComboBox) {

	/*
	 * This file defines a configuration widget that displays the list of endpoints
	 */
	var AddProtocolsDialog = declare("ism.controller.content.AddProtocolsDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the selection of protocols for an endpoint.

		nls: nls,
		templateString: addProtocolsDialog,
		dialogId: undefined,
		additionalColumns: undefined,
		protocolStore: null,
		protocolDeferrred: null,
		onSave: undefined,
		endpointDialog: null,
		protocolsGrid: null,
		pageAndFilter: false,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {

			// get store for protocols and initialize select widget
			this.protocolDeferrred.then(lang.hitch(this,function(data) {
				this.protocolStore = Utils.getProtocolStore(data);
				var items = this.protocolStore.query({}, {sort: [{attribute: "label"}]});
				// if more than 10 protocols enable filtering and pagination
				if (items.total > 10) {
					this.pageAndFilter = true;
				}
				this.protocolStore = new dojo.store.Memory({data:items, idProperty: "Name", queryEngine: IsmQueryEngine});

				// create the protocols grid
				this.protocolsGrid = new ReferencedObjectGrid({idPrefix: this.dialogId + "_protocols_",
															   filterEnabled: this.pageAndFilter,
															   pagingEnabled: this.pageAndFilter,
															   disabled: true,
															   appliedLabel: nls.messaging.endpoints.form.add,
															   nameLabel: this.protocolNameLabel}, this.dialogId + "_protocols");
				this.protocolsGrid.startup();

			}));


			dojo.subscribe(this.dialogId + "_saveButton", lang.hitch(this, function(message) {

				/*
				 * If all checkbox is not checked make sure at least one
				 * protocol is selected.
                 */
				if (!this.checkbox_AllProtocols.checked) {
					var values = this.protocolsGrid.getValue();
					if (!values || values.length === 0) {
						var errorTitle = nls.messaging.endpoints.addProtocolsDialog.protocolSelectErrorTitle;
						var errorMsg = nls.messaging.endpoints.addProtocolsDialog.protocolSelectErrorMsg;
						this.dialog.showErrorMessage(errorTitle, errorMsg);
						return;
					}
				}

				this._updateProtocols();
				this.dialog.hide();

			}));

			// set up onChange for description
			this.checkbox_AllProtocols.on("change", lang.hitch(this, function(checked) {
				// enable or disable checkboxes in grid based on all protocol checkbox
				this._updateCheckBoxes(checked);
			}));

		},

		// method to disable or enable checkboxes in grid
		_updateCheckBoxes: function(allSelected) {
			var updatedItems = [];
			var items = this.protocolStore.query({}).forEach(function(item){
							item.associated.disabled = allSelected;
							updatedItems.push(item);
						});
			this.protocolsGrid.grid.model.clearCache();
			this.protocolsGrid.grid.model.store.setData(updatedItems);
			this.protocolsGrid.grid.body.refresh();
		},

		show: function(selectedProtocols, endpointDialog, onSave ) {
			// clear fields and any validation errors
			this.endpointDialog = endpointDialog;
			dijit.byId(this.dialogId +"_DialogForm").reset();
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.onSave = onSave ? onSave : undefined;

			// check if all checkbox should be seleceted
			var allChecked = selectedProtocols.toLowerCase() === "all" ? true : false;
			this.checkbox_AllProtocols.set("checked", allChecked);
			this._updateCheckBoxes(allChecked);

			this.dialog.show();

			// must be done after the dialog has been shown
			this.protocolsGrid.populateGrid(this.dialog, this.protocolStore, selectedProtocols, false);
			this.dialog.resize();

		},

		/**
		 * returns an array of the names of the selected protocols
		 */
		_getSelectedProtocols: function() {
			return this.protocolsGrid.getValue();
		},

		_updateProtocols: function() {
			if (this.onSave) {
				if (this.checkbox_AllProtocols.checked) {
					this.onSave("All", this.endpointDialog);
				} else {
					this.onSave(this.protocolsGrid.getValue(), this.endpointDialog);
				}
			}
		}

	});

	var AddPoliciesDialog = declare("ism.controller.content.AddPoliciesDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied the the Add button in the EndpointDialog widget

		nls: nls,
		templateString: addPoliciesDialog,
		dialogId: undefined,
		additionalColumns: undefined,
		selectedPolicies: undefined,
		onSave: undefined,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {
			if (this.additionalColumns) {
				this.policies.set("additionalColumns", this.additionalColumns);
			}
			dojo.subscribe(this.dialogId + "_saveButton", lang.hitch(this, function(message) {
				this._updatePolicies();
				this.dialog.hide();
			}));



		},

		show: function(policies, policyStore, onSave) {
			// clear fields and any validation errors
			dijit.byId(this.dialogId +"_DialogForm").reset();
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.onSave = onSave ? onSave : undefined;
			this.dialog.show();

			// must be done after the dialog has been shown
			this.policies.populateGrid(this.dialog,policyStore, policies, true);
			this.dialog.resize();
		},

		/**
		 * returns an array of the names of the selected policies
		 */
		_getSelectedPoliciesAttr: function() {
			return this.policies.getValue();
		},

		_updatePolicies: function() {
			if (this.onSave) {
				this.onSave(this.policies.getValue());
			}
		}

	});

	var EndpointDialog = declare("ism.controller.content.EndpointDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add or Edit button in the Listeners widget
		//
		// description:
		//		This widget pulls its structure from a template and adds functions to
		//		populate the dialog fields with a record (fillValues) and save the changes
		//		via XHR put (save).  For inputs that are more complex than a text box,
		//		_getFieldValue and _setFieldValue are defined to get/set a generic value for
		//		different input types (e.g. checkbox).
		nls: nls,
		nls_strings: nls_strings,
		templateString: endpointDialog,

        // template strings
        nameLabel: Utils.escapeApostrophe(nls.messaging.messagingPolicies.dialog.name),
        nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        descriptionLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.description),
        portLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.port),
		portTooltip: Utils.escapeApostrophe(nls.messaging.endpoints.form.tooltip.port),
		portInvalid: Utils.escapeApostrophe(nls.messaging.endpoints.form.invalid.port),
		ipAddrLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.ipAddr),
		maxMessageSizeLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.maxMessageSize),
		maxMessageSizeTooltip: Utils.escapeApostrophe(nls.messaging.endpoints.form.tooltip.maxMessageSize),
		maxMessageSizeInvalid: Utils.escapeApostrophe(nls.messaging.endpoints.form.invalid.maxMessageSize),
		securityProfileLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.securityProfile),
		connectionPoliciesTitle: Utils.escapeApostrophe(nls.messaging.endpoints.form.connectionPolicies),
		connectionPoliciesTooltip: Utils.escapeApostrophe(nls.messaging.endpoints.form.tooltip.connectionPolicies),
		connectionPolicyLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.connectionPolicy),
        messagingPoliciesTitle: Utils.escapeApostrophe(nls.messaging.endpoints.form.messagingPolicies),
        messagingPoliciesTooltip: Utils.escapeApostrophe(nls.messaging.endpoints.form.tooltip.messagingPolicies),
        messagingPolicyLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.messagingPolicy),
		all:  Utils.escapePsuedoTranslation(nls.messaging.endpoints.form.all),

        TopicPolicies: null,
		SubscriptionPolicies: null,
		QueuePolicies: null,
        messageHubItem: null,
		selectedProtocols: null,
		_non_eth_ip_added: false,
		additionalConnectionPolicyColumns:
			[{ id: "Description", name: nls.messaging.endpoints.form.description, field: "Description", dataType: "string", width: "310px",
				decorator: Utils.textDecorator }],
		additionalMessagingPolicyColumns:
			[{	id: "DestinationType", name: nls.policyTypeTitle, field: "policyTypeName", dataType: "string", width: "150px",
				formatter: function(data) {
					// create a new column to allow proper sorting since sort works on the model
					data.policyTypeName = nls["policyTypeName_" + data.policyType.toLowerCase()];
					return data.policyTypeName;
				}
			},
			{  id: "Destination", name: nls.messaging.endpoints.form.destination, field: "Destination", dataType: "string", width: "150px",
				formatter: function(data) {
					return data[data.policyType];
				},
				decorator: Utils.textDecorator
			}],

		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},

		restURI: "rest/config/endpoints",
		domainUid: 0,

		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {

		    this.domainUid = ism.user.getDomainUid();

			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;

			if (this.add === true || Utils.configObjName.editEnabled()) {
				// check that the userid matches the pattern and doesn't clash with an existing object
				this.field_Name.isValid = function(focused) {
					var value = this.get("value");
					var validate =  Utils.configObjName.validate(value, this.existingNames, focused);
					if (!validate.valid) {
						this.invalidMessage = validate.message;
						return false;
					}
					return true;
				};
			} else {
				this.field_Name.set('readOnly', true);
			}

			this.field_ConnectionPolicies.set("additionalColumns", this.additionalConnectionPolicyColumns);
			this.field_ConnectionPolicies.set("addDialog", this.addConnectionPoliciesDialog);
			this.field_MessagingPolicies.set("additionalColumns", this.additionalMessagingPolicyColumns);
			this.field_MessagingPolicies.set("addDialog", this.addMessagingPoliciesDialog);

			this.field_Port.isValid = function(focused) {
				var value = this.get("value");
				var intValue = number.parse(value, {places: 0});
				if (isNaN(intValue) || intValue < 1 || intValue > 65535) {
					this.invalidMessage = nls.messaging.endpoints.form.invalid.port;
					return false;
				}
				return true;
			};

			this.field_MaxMessageSize.set('pattern', number.regexp({locale: ism.user.localeInfo.localeString}));

			this.field_MaxMessageSize.isValid = function(focused) {
				var value = this.get("value");
				var intValue = number.parse(value, {places: 0, locale: ism.user.localeInfo.localeString});
				if (isNaN(intValue) || intValue < 1 || intValue > 262144) {
					this.invalidMessage = nls.messaging.endpoints.form.invalid.maxMessageSize;
					return false;
				}
				return true;
			};

			var t = this;
			// set up onChange for description
			t.field_Description.on("change", function(args) {
				t.dialog.resize();
			}, true);

			/* watch the form for changes between valid and invalid states
			var saveId = this.dialogId + "_saveButton";
			dijit.byId(this.dialogId +"_DialogForm").watch('state', function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(saveId);
				if (newvalue) {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			});*/

			this._setupFields();

		},

		_setMessageHubItemAttr: function(value) {
			this.messageHubItem = value;
		},


		show: function(endpoint, list, stores) {
			// clear fields and any validation errors
			dijit.byId(this.dialogId +"_DialogForm").reset();
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";

			this._setProtocolValue("All", this);
			this._checkIPAddress(endpoint);

			this.field_Name.existingNames = list;
			if (endpoint) {
				this.fillValues(endpoint);
			} else {
			    // set the default IP address to all
			    this._setFieldValue("Interface", "All");
			}
			var connectionPolicies = endpoint ? endpoint.ConnectionPolicies : "";
			var mplist = endpoint ? Utils.getMessagingPolicies(endpoint) : undefined;
			var messagingPolicies = (mplist && mplist.count > 0) ? mplist.Id.join(",") : "";

			this.dialog.clearErrorMessage();
			this.dialog.show();

			this.field_Description.focus(); // needed to get the text area to resize
			if (this.add === true) {
				this.field_Name.focus();
			} else {
				this.field_Enabled.focus(); // TODO post beta this.field_Name.focus();
			}

			var connectionPolicyStore = stores.connectionPolicies;
			var messagingPolicyStore = stores.messagingPolicies;

			// must be done after the dialog has been shown
			this.field_ConnectionPolicies.populateGrid(this.dialog, connectionPolicyStore, connectionPolicies);
			this.field_MessagingPolicies.populateMultiStoreGrid(this.dialog, messagingPolicyStore, messagingPolicies);
			this.dialog.resize();
		},

		editProtocol: function(event) {
            // work around for IE event issue - stop default event
            if (event && event.preventDefault) event.preventDefault();

			this.addProtocolsDialog.show(this.selectedProtocols, this, this._setProtocolValue);
		},

		_setProtocolValue: function(protocolSetting, _this) {

			// get the displayed string - at most two addresses
			var protocolSettingArray = [];
			if (lang.isArray(protocolSetting)) {
				protocolSettingArray = protocolSetting;
				_this.selectedProtocols = protocolSetting.join();
			} else {
				protocolSettingArray = protocolSetting.split(",");
				_this.selectedProtocols = protocolSetting;
			}

			var protocolValue = protocolSettingArray[0];
			if (protocolSettingArray.length > 2) {
				protocolValue = protocolSettingArray[0] + ", " + protocolSettingArray[1] + ", ...";
			} else if (protocolSettingArray.length === 2) {
				protocolValue = protocolSettingArray[0] + ", " + protocolSettingArray[1];
			}

			var protocolHoverValue = protocolSettingArray.join(", ");

			if (protocolValue === "All") {
				protocolValue = nls.messaging.endpoints.form.all;
				protocolHoverValue = nls.messaging.endpoints.form.all;
			}

			// set displayed value and hover tool tip
			dojo.byId(_this.dialogId + "_protocols_Value").innerHTML = entities.encode(protocolValue);
			dojo.byId(_this.dialogId + "_protocols_Value_Hover").innerHTML = entities.encode(protocolHoverValue);

		},

		// populate the dialog with the passed-in values
		fillValues: function(values) {
			console.debug("EndpointDialog -- fillValues", values);
			this._startingValues = values;
			for (var prop in values) {
				if (prop === "ConnectionPolicies" || prop === "MessagingPolicies") {
					// this are handled in populateGrid
					continue;
				}
				if (prop === "Protocol") {
				    // use protocol selection dialog
					this._setProtocolValue(values[prop], this);
					continue;
				}
				this._setFieldValue(prop, values[prop]);
			}
		},


		// check to see if anything changed, and save the record if so
		save: function(onFinish, onError) {
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;

			if (!onFinish) { onFinish = lang.hitch(this,function() {
				this.dialog.hide();
				});
			}
			if (!onError) { 	onError = lang.hitch(this,function(error) {
				bar.innerHTML = "";
				this.dialog.showErrorFromPromise(nls.messaging.endpoints.saveEndpointError, error);
				});
			}

			var values = {};
			var recordChanged = false;
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var i = 0; i < this._attachPoints.length; i++) {
		    	var prop = this._attachPoints[i];
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					if (actualProp === "MessagingPolicies")
						continue;
					values[actualProp] = this._getFieldValue(actualProp);
					if (values[actualProp] != this._startingValues[actualProp]) {
						recordChanged = true;
					}
				}
			}

			values.TopicPolicies = this._getPolicyNamesByType("Topic", this.field_MessagingPolicies);
			values.SubscriptionPolicies = this._getPolicyNamesByType("Subscription", this.field_MessagingPolicies);
			values.QueuePolicies = this._getPolicyNamesByType("Queue", this.field_MessagingPolicies);

			if (values.TopicPolicies != this._startingValues["TopicPolicies"]
			     || values.SubscriptionPolicies != this._startingValues["SubscriptionPolicies"]
			     || values.QueuePolicies != this._startingValues["QueuePolicies"]) {
				recordChanged = true;
			}

			values.MessageHub = this.messageHubItem.Name;

			// surround ipv6 addresses in brackets
			var address = values.Interface;
			if (address != "All" && (address.match(/[A-Fa-f]/) || address.match(/[:]/))) {
				values.Interface = "[" + address + "]";
			}

			// set protocol string
			values.Protocol = this.selectedProtocols;

            if (values.Protocol != this._startingValues["Protocol"]) {
                recordChanged = true;
            }

			var dialog = this.dialog;

			if (recordChanged) {
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.createPostOptions("Endpoint", values);
				var promise = adminEndpoint.doRequest("/configuration", options);
				promise.then(function(data) {
					onFinish(values);
				},
				onError);
			} else {
				dialog.hide(); // nothing to save
			}
		},


		_getPolicyNamesByType: function(type, allpolicies) {
			var policies = allpolicies.store.query({policyType: type});
			var policyName = [];
			if (policies && policies.length > 0) {
				var polid = allpolicies.value;
				var idx = 0;
		        for (var i = 0; i < polid.length && idx < policies.length; i++) {
				    for (var j = 0; j < policies.length; j++) {
			        	if ((escape(polid[i]) === policies[j].id) || (polid[i] === policies[j].id)) {
			        		policyName[idx] = policies[j].Name;
			        		idx++;
			        		break;
			        	}
			        }
				}
			    return policyName.join(",");
		    }
			return null;
		},

		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("EditEndpointDialog -- _setFieldValue", prop, value);
			switch (prop) {
				case "Enabled":
					if (value === true || value === false) {
						this["field_"+prop].set("checked", value);
					} else if (value.toString().toLowerCase() == "true") {
						this["field_"+prop].set("checked", true);
					} else {
						this["field_"+prop].set("checked", false);
					}
					break;
				case "ConnectionPolicies":
				case "MessagingPolicies":
					if (value) {
						this["field_"+prop].set("value", value.split(","));
					}
					break;
				case "MaxMessageSize":
					// take off the KB at the end of the string
					var index = value.indexOf("KB");
					if (index != -1) {
						value = value.substr(0, index);
					}
					// take off the MB at the end of the string and convert MB to KB
					index = value.indexOf("MB");
					if (index != -1) {
						value = value.substr(0, index);
						value = value * 1024;
					}
					this["field_MaxMessageSize"]._setValueAttr(number.format(value, {places: 0, locale: ism.user.localeInfo.localeString}));
					break;
				case "Interface":
				    var isAll = false;
					if (value && value.toLowerCase() == "all") {
                        isAll = true;
                        value = nls.messaging.endpoints.form.all;
					}
					if (!isAll && (value.match(/[A-Fa-f]/) || value.match(/[:]/))) {
						// remove the [, ] characters that wrap this address
						value = value.substr(1, value.length - 2);
					}
					this["field_Interface"].set("value", value);
					break;
				default:
					if (this["field_"+prop] && this["field_"+prop].set) {
						this["field_"+prop].set("value", value);
					} else {
						this["field_"+prop] = { value: value };
					}
					break;
			}
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value = "";
			switch (prop) {
				case "Enabled":
					value = this["field_"+prop].checked;
					if (value == "off") { value = false; }
					if (value == "on") { value = true; }
					break;
				case "SecurityProfile":
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					// we want to map a "None" selection to an empty string
					if (value == "None") {
						value = "";
					}
					break;
				case "Interface":
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					if (value == nls.messaging.endpoints.form.all) {
					    value = "All";
					} else if (value.indexOf("/") > 0) {
					    // we want to trim off the subnet mask from the IP address
						value = value.substring(0, value.indexOf("/"));
					}
					break;
				case "MaxMessageSize":
					value = number.parse(this["field_MaxMessageSize"].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
					value = value + "KB";
					break;
				case "Port":
					value = number.parse(this["field_Port"].get("value"), {places: 0, locale: ism.user.localeInfo.localeString});
					break;
				case "ConnectionPolicies":
				case "MessagingPolicies":
					value = this["field_"+prop].getValueAsString();
					break;
				default:
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					break;
			}
			console.debug("EndpointDialog -- _getFieldValue", prop, value);
			return value;
		},

		_checkIPAddress: function(endpoint) {

			// have we already added a special address for a non- ethernet-interface?
			if (this._non_eth_ip_added) {
				if (endpoint && this._non_eth_ip_added == endpoint.Interface) {
					return; // nothing to do
				}
				// delete it
				this.field_Interface.store.query({name: this._non_eth_ip_added}).then(lang.hitch(this, function(items) {
					if (items && items.length > 0) {
						this.field_Interface.store.deleteItem(items[0]);
						this.field_Interface.store.save();
						this._non_eth_ip_added = undefined;
					}
				}));
			}
			if (endpoint && endpoint.Interface && endpoint.Interface.toLowerCase() != "all") {
				// check to see if the address is in the store
				this.field_Interface.store.query({name: endpoint.Interface}).then(lang.hitch(this, function(items) {
					if (!items || items.length === 0) {
						// not there, so add it
						this.field_Interface.store.newItem({name: endpoint.Interface});
						this._non_eth_ip_added = endpoint.Interface;
						this.field_Interface.store.save();
					}
				}));
			}
		},

		// do any adjustment of form field values necessary before showing the form.  in this
		// case, we populate the Interface ComboBox with valid ethernet IP addresses via REST API call
		_setupFields: function() {
			console.debug("setupFields");

			// clear fields and any validation errors
			dijit.byId(this.dialogId +"_DialogForm").reset();

			// Populate IP addresses
			var addAddrs = lang.hitch(this, function() {
				var _this = this;
//                var adminEndpoint = ism.user.getAdminEndpoint();
//                var options = adminEndpoint.getDefaultGetOptions();
//                var promise = adminEndpoint.doRequest("/configuration/Endpoint", options);
// TODO: FIX THIS!  The code here to manipulate the data object
//       is really the code for the old JAX-RS API.
//       We need a new server REST API for this.  And the
//       code should be updated accordingly when the new REST API
//       is available.
// THIS CAPABILITY (PROVDING A LIST OF VALID INTERFACES) WILL NEED TO BE ADDED POST-2.0 GA
//                 promise.then(function(data) {
//						console.debug("adding addrs", data);
						_this.field_Interface.store.newItem({ name: nls.messaging.endpoints.form.all, label: nls.messaging.endpoints.form.all });
//						if (data.interfaces) {
//							var ipv4 = [];
//							var ipv6 = [];
//							var removeInterfaces = [];
//							for (var iface in data.interfaces) {
//								if (data.interfaces[iface].members) {
//									for (var memberIndex=0, membersLen=data.interfaces[iface].members.length; memberIndex < membersLen; memberIndex++) {
//										var member = data.interfaces[iface].members[memberIndex];
//										removeInterfaces.push(member);
//									}
//								}
//							}
//							for (var i in data.interfaces) {
//								if (removeInterfaces.indexOf(data.interfaces[i]) > -1) {
//									continue;
//								}
//								var adminState = data.interfaces[i].adminState ? data.interfaces[i].adminState.toLowerCase() : "";
//								if (adminState != "enabled") {
//									continue;
//								}
//								if (data.interfaces[i].ipaddress) {
//									for (var j=0, addrLen=data.interfaces[i].ipaddress.length; j < addrLen; j++) {
//										var address = data.interfaces[i].ipaddress[j];
//										// we want to trim off the subnet mask from the IP address
//										if (address.indexOf("/") > 0) {
//											address = address.substring(0, address.indexOf("/"));
//										}
//										if (address.match(/[A-Fa-f]/) || address.match(/[:]/)) {
//											if (array.indexOf(ipv6, address) < 0) {
//												ipv6.push(address);
//											}
//										} else if (array.indexOf(ipv4, address) < 0) {
//											ipv4.push(address);
//										}
//									}
//								}
//							}
//
//							ipv4.sort();
//							ipv6.sort();
//
//							for (var l=0, ipv4Len=ipv4.length; l < ipv4Len; l++) {
//								_this.field_Interface.store.newItem({ name: ipv4[l], label: ipv4[l] });
//							}
//							for (var k=0, ipv6Len=ipv6.length; k < ipv6Len; k++) {
//								_this.field_Interface.store.newItem({ name: ipv6[k], label: ipv6[k] });
//							}
//						}
                        _this.field_Interface.store.save();
//                },
//                function(error) {
//					console.debug("error", error);
//                });
			});
			this.field_Interface.store.fetch({
				onComplete: lang.hitch(this, function(items, request) {
					console.debug("deleting items");
					var len = items ? items.length : 0;
					for (var i=0; i < len; i++) {
						this.field_Interface.store.deleteItem(items[i]);
					}
					this.field_Interface.store.save({ onComplete: addAddrs() });
				})
			});

			// Populate security profile
			var addProfiles = lang.hitch(this, function() {
			    var _this = this;
                var adminEndpoint = ism.user.getAdminEndpoint();
                var promise = adminEndpoint.doRequest("/configuration/SecurityProfile", adminEndpoint.getDefaultGetOptions());
                promise.then(
                        function(data) {
                            _this._populateSecurityProfiles(data);
                        },
                        function(error) {
                            if (Utils.noObjectsFound(error)) {
                                _this._populateSecurityProfiles({"SecurityProfile" : {}});
                            } else {
                                _this.dialog.showErrorFromPromise(nls.getSecurityProfilesError, error);
                            }
                        }
                );
			});
			this.field_SecurityProfile.store.fetch({
				onComplete: lang.hitch(this, function(items, request) {
					console.debug("deleting items");
					var len = items ? items.length : 0;
					for (var i=0; i < len; i++) {
						this.field_SecurityProfile.store.deleteItem(items[i]);
					}
					this.field_SecurityProfile.store.save({ onComplete: addProfiles() });
				})
			});
		},

		_populateSecurityProfiles: function(data) {
            this.field_SecurityProfile.store.newItem({ name: "None", label: nls.messaging.endpoints.form.none });
            var bChanged = false;
            var endpoint = ism.user.getAdminEndpoint();
            var profiles = endpoint.getObjectsAsArray(data, "SecurityProfile");
            var len = profiles ? profiles.length : 0;
            for (var i=0; i < len; i++) {
                var profile = profiles[i];
                if (profile.Name) {
                    this.field_SecurityProfile.store.newItem({ name: profile.Name, label: dojox.html.entities.encode(profile.Name) });
                    bChanged = true;
                }
            }
            if (bChanged) {
                this.field_SecurityProfile.store.save();
            }
        }
	});

	var EndpointList = declare ("ism.controller.content.EndpointList", [_Widget, _TemplatedMixin], {

		nls: nls,
		paneId: 'ismEndpointList',
		store: null,
		queryEngine: IsmQueryEngine,
		editDialog: null,
		removeDialog: null,
		removeDialogId: null,
		messageHubItem: null,
		contentId: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		existingNames: [],
		detailsPage: null,
		messagingPolicyStore: null,

		addButton: null,
		deleteButton: null,
		editButton: null,

		restURI: "rest/config/endpoints",
		domainUid: 0,
		inMaintenanceMode: false,
		storeInitialized: false,

		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug("constructing EndpointList");
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], idProperty: "id", queryEngine: this.queryEngine});
		},

		startup: function() {
			this.inherited(arguments);

            this.domainUid = ism.user.getDomainUid();

			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;

			// start with an empty <div> for the content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";

			Utils.updateStatus();
	         // subscribe to server status changes
            dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
            	if (!data || !data.Server || !data.Server.State) {
            		return;
            	}
            	var inMaintenanceMode = Utils.isMaintenanceMode(data.Server.State);
            	var modeChanged = (inMaintenanceMode !== this.inMaintenanceMode);
            	this.inMaintenanceMode = Utils.isMaintenanceMode(data.Server.State);

            	if (modeChanged && this.storeInitialized) {
            		this._initStore(true);
            	}
            }));

			this._initGrid();
			this._initStore(false);
			this._initDialogs();
			this._initEvents();
		},

		_setMessageHubItemAttr: function(value) {
			var currentName = this.messageHubItem ? this.messageHubItem.Name : null;
			this.messageHubItem = value;
			if (currentName && currentName != value.Name) {
				// we have a new item, we need to refresh the grid's store
				this._initStore(true);
				if (this.addDialog) {
					this.addDialog.set("messageHubItem", this.messageHubItem);
				}
				if (this.editDialog) {
					this.editDialog.set("messageHubItem", this.messageHubItem);
				}

			}
		},

		_setDetailsPageAttr: function(value) {
			this.detailsPage = value;
			this.detailsPage.stores.endpoints = this.store;
		},

		_initStore: function(reInit) {
			this.storeInitialized = true;
			console.debug("initStore for Endpoints");
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;

			var _this = this;

			// kick off the queries
			var endpointResults = this.detailsPage.promises.endpoints;
			var endpointStatusResults = MonitoringUtils.getEndpointStatusDeferred();

			// handle the query results
			var dl = new dojo.DeferredList([endpointResults, endpointStatusResults]);

			dl.then(lang.hitch(this,function(results) {
		    	_this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;

				if (results[0][0] === false) {
				    if (Utils.noObjectsFound(results[0][1])) {
				        results[0][1] = {};
				        results[0][1].Endpoint = {};
				    } else {
				        Utils.showPageErrorFromPromise(nls.messaging.endpoints.retrieveEndpointError, results[0][1]);
				        return;
				    }
				} else if ((results[1][0] === false) && !_this.inMaintenanceMode) {
					Utils.showPageErrorFromPromise(nls.messaging.endpoints.retrieveEndpointError, results[1][1]);
					return;
				} else if ((results[1][0] === false) && _this.inMaintenanceMode) {
					// If we are in maintenance mode, we still need the endpoint list but we cannot present
					// status data for the endpoints because there is no access to monitoring.
			        results[1][1] = {};
			        results[1][1].Endpoint = {};
				}

				results[1][1] = results[1][1].Endpoint;
				var endpointStatus = dojo.map(results[1][1], function(item, index){
					item.id = escape(item.Name);
					return item;
				});
				var statusStore = new Memory({data: endpointStatus, idProperty: "id"});

				var adminEndpoint = ism.user.getAdminEndpoint();
				var allEndpoints = adminEndpoint.getObjectsAsArray(results[0][1], "Endpoint"); // this is all of them, just need the ones for this message hub
				var endpoints = [];
				var epIndex = 0;
				this.existingNames = [];
				for (var j=0, len=allEndpoints.length; j < len; j++) {
					if (allEndpoints[j]['MessageHub'] == this.messageHubItem.Name) {
					    Utils.convertStringsToBooleans(allEndpoints[j], [{property: "Enabled", defaultValue: true}]);
						endpoints[epIndex] = allEndpoints[j];
						epIndex++;
					}
					this.existingNames[j] = allEndpoints[j].Name;
				}

				endpoints.sort(function(a,b){
					if (a.Name < b.Name) {
						return -1;
					}
					return 1;
				});

				var items = [];
				dojo.forEach(endpoints, lang.hitch(this, function(endpoint, i) {
					endpoint.id = escape(endpoint.Name);
					var mplist = Utils.getMessagingPolicies(endpoint);
					endpoint.mp = {list: mplist.Name.join(","), Name: endpoint.Name, sortWeight: mplist.count};
					endpoint.cp = {list: endpoint.ConnectionPolicies, Name: endpoint.Name, sortWeight: endpoint.ConnectionPolicies.split(",").length};
					endpoint.status = statusStore.query({id: endpoint.id})[0];
					items.push(endpoint);
				}));

				if (_this.store.data.length === 0 && items.length === 0) {
					_this.detailsPage.stores.endpoints = _this.store;
					return; // store was and still is empty
				}
				_this.store = new Memory({data: items, idProperty: "id", queryEngine: _this.queryEngine});
				_this.grid.setStore(_this.store);
				_this.triggerListeners();
				_this.detailsPage.stores.endpoints = _this.store;
				if (reInit) {
					_this.grid.resize();
				}
			}));
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the toolbar

			var protocolDeferrred = this.detailsPage.deferreds.protocols;

			// create Add protocols dialog
			var addProtocolsId = "addProtocols"+this.id+"Dialog";
			domConstruct.place("<div id='"+addProtocolsId+"'></div>", this.contentId);
			this.addProtocolsDialog = new AddProtocolsDialog({dialogId: addProtocolsId,
									                dialogTitle: Utils.escapeApostrophe(nls.messaging.endpoints.addProtocolsDialog.title),
									                dialogInstruction: Utils.escapeApostrophe(nls.messaging.endpoints.addProtocolsDialog.instruction),
									                protocolNameLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.protocol),
									                protocolDeferrred:protocolDeferrred
									                }, addProtocolsId);
			this.addProtocolsDialog.startup();

			// create Add connection policy dialog
			var addConnectionPoliciesId = "addConnectionPolicies"+this.id+"Dialog";
			domConstruct.place("<div id='"+addConnectionPoliciesId+"'></div>", this.contentId);
			this.addConnectionPoliciesDialog = new AddPoliciesDialog({dialogId: addConnectionPoliciesId,
									                dialogTitle: Utils.escapeApostrophe(nls.messaging.endpoints.addConnectionPoliciesDialog.title),
									                dialogInstruction: Utils.escapeApostrophe(nls.messaging.endpoints.addConnectionPoliciesDialog.instruction),
									                policyNameLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.connectionPolicy),
									                additionalColumns: [{ id: "Description", name: Utils.escapeApostrophe(nls.messaging.endpoints.form.description), field: "Description", dataType: "string", width: "310px",
									    				decorator: Utils.textDecorator }]
									                }, addConnectionPoliciesId);
			this.addConnectionPoliciesDialog.startup();

			// create Add messaging policy dialog
			var addMessagingPoliciesId = "addMessagingPolicies"+this.id+"Dialog";
			domConstruct.place("<div id='"+addMessagingPoliciesId+"'></div>", this.contentId);
			this.addMessagingPoliciesDialog = new AddPoliciesDialog({dialogId: addMessagingPoliciesId,
									                dialogTitle: Utils.escapeApostrophe(nls.messaging.endpoints.addMessagingPoliciesDialog.title),
									                dialogInstruction: Utils.escapeApostrophe(nls.messaging.endpoints.addMessagingPoliciesDialog.instruction),
									                policyNameLabel: Utils.escapeApostrophe(nls.messaging.endpoints.form.messagingPolicy),
									                additionalColumns:	[{	id: "DestinationType", name: Utils.escapeApostrophe(nls.policyTypeTitle), field: "policyTypeName", dataType: "string", width: "150px",
										                	formatter: function(data) {
										    					// create a new column to allow proper sorting since sort works on the model
										    					data.policyTypeName = nls["policyTypeName_" + data.policyType.toLowerCase()];
										    					return data.policyTypeName;
										                	}
										                },
										                {  id: "Destination", name: Utils.escapeApostrophe(nls.messaging.endpoints.form.destination), field: "Destination", dataType: "string", width: "150px",
										    				formatter: function(data) {
										    					return data[data.policyType];
										    				},
										                	decorator: Utils.textDecorator
										                }]
									                }, addMessagingPoliciesId);
			this.addMessagingPoliciesDialog.startup();

			// create Edit dialog
			var editId =  "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);
			this.editDialog = new EndpointDialog({dialogId: editId,
												  dialogTitle: Utils.escapeApostrophe(nls.messaging.endpoints.editDialog.title),
												  dialogInstruction: Utils.escapeApostrophe(nls.messaging.endpoints.editDialog.instruction),
												  addProtocolsDialog: this.addProtocolsDialog,
												  addConnectionPoliciesDialog: this.addConnectionPoliciesDialog,
												  addMessagingPoliciesDialog: this.addMessagingPoliciesDialog,
												   add: false}, editId);
			this.editDialog.set("messageHubItem", this.messageHubItem);
			this.editDialog.startup();

			// create Add dialog
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new EndpointDialog({dialogId: addId,
									                dialogTitle: Utils.escapeApostrophe(nls.messaging.endpoints.addDialog.title),
									                dialogInstruction: Utils.escapeApostrophe(nls.messaging.endpoints.addDialog.instruction),
									                addProtocolsDialog: this.addProtocolsDialog,
									                addConnectionPoliciesDialog: this.addConnectionPoliciesDialog,
													addMessagingPoliciesDialog: this.addMessagingPoliciesDialog,
													 add: true}, addId);
			this.addDialog.set("messageHubItem", this.messageHubItem);
			this.addDialog.startup();

			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeEndpointDialog'></div>", 'ismEndpointList');
			var dialogId = "removeEndpointDialog";

			this.removeDialog = new Dialog({
				title: Utils.escapeApostrophe(nls.messaging.messageHubs.endpointList.removeDialog.title),
				content: "<div>" + Utils.escapeApostrophe(nls.messaging.messageHubs.endpointList.removeDialog.content) + "</div>",
				buttons: [
				          new Button({
				        	  id: dialogId + "_saveButton",
				        	  label: nls_strings.action.Ok,
				        	  onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
				          })
				          ],
				          closeButtonLabel: nls_strings.action.Cancel
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.save = lang.hitch(this, function(endpointName, onFinish, onError) {
				if (!onFinish) { onFinish = function() {this.removeDialog.hide();}; }
				if (!onError) { 	onError = lang.hitch(this,function(error) {
					this.removeDialog.showErrorFromPromise(nls.messaging.endpoints.deleteEndpointError, error);
					});
				}
				console.debug("Deleting: ", endpointName);
				var adminEndpoint = ism.user.getAdminEndpoint();
	            var options = adminEndpoint.getDefaultDeleteOptions();
	            var promise = adminEndpoint.doRequest("/configuration/Endpoint/" + encodeURIComponent(endpointName), options);
	            promise.then(onFinish, onError);
			});
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.messaging.messageHubs.endpointList.name, field: "Name", dataType: "string", width: "200px",
						decorator: Utils.nameDecorator
					},
					{   id: "Port", name: nls.messaging.messageHubs.endpointList.port, field: "Port", dataType: "number", width: "50px" },
					{   id: "Enabled", name: nls.messaging.messageHubs.endpointList.enabled, field: "Enabled", dataType: "string", width: "70px",
					    widgetsInCell: true,
					    decorator: function() {
					    	var template = "<div class='gridxHasGridCellValue'>" +
			    		        "<div style='text-align:center' data-dojo-type='ism.widgets.NoticeIcon' data-dojo-attach-point='enabledIcon'>" +
			    		        "</div></div>";
				    		return template;
					    },
					    setCellValue: function(gridData, storeData, cellWidget){
					    	// keep track of the value from the store
					    	var enabled = false;
							if (storeData === true || storeData === false) {
								enabled = storeData;
							} else if (storeData.toLowerCase() == "true") {
								enabled = true;
							}
					    	var iconClass = enabled ? "ismIconChecked": "ismIconUnchecked";
					    	cellWidget.enabledIcon.set("iconClass", iconClass);
					    	cellWidget.enabledIcon.set("hoverHelpText", gridData);
					    },
						formatter: function(data) {
							// the filter operates on formatted data, so have it return the altText
							var altText = "";
							if (data.Enabled) {
								var cellData = data.Enabled;
								var enabled = false;
								if (cellData === true || cellData === false) {
									enabled = cellData;
								} else if (	cellData.toLowerCase() == "true") {
									enabled = true;
								}

								if (enabled === false) {
									altText = nls.messaging.destinationMapping.state.disabled;
								} else {
									altText = nls.messaging.destinationMapping.state.enabled;
								}
							}
							return altText;
						}
					},
					{   id: "status", name: nls.messaging.messageHubs.endpointList.status, field: "status", dataType: "string", width: "70px",
					    widgetsInCell: true,
					    decorator: function() {
					    	var template = "<div class='gridxHasGridCellValue'>" +
			    		            "<div style='text-align:center' data-dojo-type='ism.widgets.NoticeIcon' data-dojo-attach-point='statusIcon'>" +
				    		        "</div></div>";
				    		return template;
					    },
					    setCellValue: function(gridData, storeData, cellWidget){
					    	var iconClass = "";
					    	if (storeData) {
						    	iconClass = storeData.Enabled === true ? "ismIconEnabled": "ismIconDisabled";
					    	}
						    cellWidget.statusIcon.set("iconClass", iconClass);
						    cellWidget.statusIcon.set("hoverHelpText", gridData);

					    },
						formatter: function(data) {
							// the filter operates on formatted data, so have it return the altText
							var altText = "";
							var sortWeight = 0;
							if (data.status) {
								var cellData = data.status;
								if (cellData) {
									if (cellData.Enabled === true) {
										altText = nls.messaging.messageHubs.endpointList.up;
										sortWeight = 1;
									} else {
										altText = nls.messaging.messageHubs.endpointList.down;
										if (cellData.LastErrorCode && cellData.LastErrorCode !== "0") {
											var message = nls_messages.returnCodes["rc_"+cellData.LastErrorCode];
											var messageID = nls_messages.returnCodes.prefix + cellData.LastErrorCode;
											if (message) {
												altText += " [" + messageID + ": " + message + "]";
											} else {
												altText += " [" + nls_messages.lastErrorCode + " " + messageID +  "]";
											}
										}
									}
								}
								data.status.sortWeight = sortWeight;  // sort the column based on numeric value of Enabled
							}
							return altText;
						}
					},
					{	id: "connectionPolicyCount", name: nls.messaging.messageHubs.endpointList.connectionPolicies, field: "cp", filterable: false, dataType: "string", width: "80px",
							widgetsInCell: true,
							decorator: function() {
									return "<div data-dojo-type='ism.widgets.MessageHubAssociationControl' data-dojo-props='associationType: \"connectionPolicies\"' class='gridxHasGridCellValue'></div>";
							}
					},
					{	id: "messagingPolicyCount", name: nls.messaging.messageHubs.endpointList.messagingPolicies, field: "mp", filterable: false, dataType: "string", width: "80px",
						widgetsInCell: true,
						decorator: function() {
								return "<div data-dojo-type='ism.widgets.MessageHubAssociationControl' data-dojo-props='associationType: \"messagingPolicies\"' class='gridxHasGridCellValue'></div>";
						}
					},

					{	id: "Description", name: nls.messaging.messageHubs.endpointList.description, field: "Description", dataType: "string", width: "200px",
						decorator: Utils.textDecorator
					}
				];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, {paging: true, filter: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);
		},

		_getSelectedData: function() {
			// summary:
			// 		get the row data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).data();
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh().then(lang.hitch(this, function() {
				this.grid.resize();
			}));

		},

		_initEvents: function() {
			// summary:
			// 		init events

			aspect.after(this.grid, "onRowClick", lang.hitch(this, function(event) {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));

			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    this.addDialog.show(null, this.existingNames, this.detailsPage.stores);
			}));

			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				this.endpointToDelete = unescape(this.grid.select.row.getSelected()[0]);
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				this.removeDialog.show();
			}));

			on(this.editButton, "click", lang.hitch(this, function() {
				console.debug("clicked edit button");
				this._doEdit();

			}));

			aspect.after(this.grid, "onRowDblClick", lang.hitch(this, function(event) {
				// if the edit button is enabled, pretend it was clicked
				if (!this.editButton.get('disabled')) {
					this._doEdit();
				}
			}));

		    aspect.after(this.grid, "onRowKeyPress", lang.hitch(this, function(event) {
		    	// if the enter key was pressed on a row and the edit button is enabled,
		    	// pretend it was clicked
				if (event.keyCode == keys.ENTER && !this.editButton.get('disabled')) {
					this._doEdit();
				} else if (event.charCode == keys.SPACE) {
					this._doSelect();
				}
			}), true);

			dojo.subscribe(this.editDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				if (dijit.byId(this.editDialog.dialogId+"_DialogForm").validate()) {
					var id = this.grid.select.row.getSelected()[0];
					var endpointName = unescape(id);
					var result = this.grid.store.query({id: id});
					this.editDialog.save(lang.hitch(this, function(data) {
						console.debug("save is done, returned:", data);
						var endpoint = null;
						if (result) {
							endpoint = result[0];
						}
						data.id = escape(data.Name);
						var mplist = Utils.getMessagingPolicies(data);
						data.mp = {list: mplist.Name.join(","), Name: data.Name, sortWeight: mplist.count};
						data.cp = {list: data.ConnectionPolicies, Name: data.Name, sortWeight: data.ConnectionPolicies.split(",").length};
						var _this = this;
						when(MonitoringUtils.getNamedEndpointStatusDeferred(data.Name), function(sdata) {
							if(sdata) {
								data.status = sdata.Endpoint[0];
							}
							if (endpointName != data.Name) {
								_this.store.remove(id);
								_this.store.add(data);
								var index = dojo.indexOf(_this.existingNames, endpointName);
								if (index >=0) {
									_this.existingNames[index] = data.Name;
								}
							} else {
								_this.store.put(data);
							}
							_this.triggerListeners();

							_this.editDialog.dialog.hide();
							dojo.publish("EndpointChanged", {oldValue: endpoint, newValue: data});
							}, function(error) {
								data.status = { error: error };
							});
					}));
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving addDialog");
				if (dijit.byId(this.addDialog.dialogId+"_DialogForm").validate()) {
					this.addDialog.save(lang.hitch(this, function(data) {
						console.debug("add is done, returned:", data);
						data.id = escape(data.Name);
						var mplist = Utils.getMessagingPolicies(data);
						data.mp = {list: mplist.Id.join(","), Name: data.Name, sortWeight: mplist.count};
						data.cp = {list: data.ConnectionPolicies, Name: data.Name, sortWeight: data.ConnectionPolicies.split(",").length};
						var _this = this;
						when(MonitoringUtils.getNamedEndpointStatusDeferred(data.Name), function(sdata) {
							if(sdata) {
								data.status = sdata.Endpoint[0];
							}
							_this.store.add(data);
							_this.triggerListeners();
							_this.existingNames.push(data.Name);
							_this.addDialog.dialog.hide();
							dojo.publish("EndpointAdded", data);
						}, function(error) {
							data.status = { error: error };
						});
					}));
				}
			}));

			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.deletingProgress;
				var endpoint = this._getSelectedData();
				var id = this.grid.select.row.getSelected()[0];
				var endpointName = endpoint.Name;
				console.log("deleting " + endpointName);
				this.removeDialog.save(endpointName, lang.hitch(this, function(data) {
					console.debug("remove is done, returned:", data);
					this.store.remove(id);
					var index = dojo.indexOf(this.existingNames, endpointName);
					if (index >= 0) {
						this.existingNames.splice(index, 1);
					}
					if (this.store.data.length === 0 ||
							this.grid.select.row.getSelected().length === 0) {
						this.deleteButton.set('disabled',true);
						this.editButton.set('disabled',true);
					}
					this.removeDialog.hide();
					bar.innerHTML = "";
					endpoint.messageHub = this.messageHubItem.Name;
					dojo.publish("EndpointRemoved", endpoint);

				}));
			}));

			dijit.byId("ismMHTabContainer").watch("selectedChildWidget", lang.hitch(this, function(name, origVal, newVal) {
				if (newVal.id == Resources.pages.messaging.messageHubTabs.endpoints.id) {
					this.grid.resize();
				}
			}));

		},

		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				//console.log("row count: "+this.grid.rowCount());
				this.deleteButton.set('disabled',false);
				this.editButton.set('disabled',false);
			} else {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);
			}
		},

		_doEdit: function(){
			var endpoint = this.grid.store.query({id: this.grid.select.row.getSelected()[0]})[0];
			// create array of existing names except for the current one, so we don't clash
			var names = array.map(this.existingNames, function(item) {
				if (item != endpoint.Name) return item;
			});
			this.editDialog.show(endpoint, names, this.detailsPage.stores);
		},

		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");

			// update grid quick filter if there is one
			if (this.grid.quickFilter) {
				this.grid.quickFilter.apply();
			}
			// re-sort
			if (this.grid.sort) {
				var sortData = this.grid.sort.getSortData();
				if (sortData && sortData !== []) {
					this.grid.model.clearCache();
					this.grid.sort.sort(sortData);
				}
			}
		}
	});

	return EndpointList;
});
