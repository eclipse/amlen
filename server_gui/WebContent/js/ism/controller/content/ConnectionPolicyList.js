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
		'dojo/dom-class',
		'dojo/dom-construct',
		'dojo/dom-style',		
		'dojo/store/Memory',
		'dojo/number',
		'dojo/data/ItemFileReadStore',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/registry',
		'dijit/form/Button',
		'ism/widgets/Dialog',
		'idx/form/CheckBoxSelect',
	    'idx/form/CheckBox',
		'ism/widgets/GridFactory',				
		'ism/IsmUtils',		
		'dojo/text!ism/controller/content/templates/ConnectionPolicyDialog.html',
		'dojo/i18n!ism/nls/messaging',
		'dojo/i18n!ism/nls/strings',
		'ism/config/Resources',
		'ism/widgets/MessageHubAssociationControl',
		'ism/widgets/TextArea',
		'ism/widgets/IsmQueryEngine',
		'dojo/keys',
		'dojo/i18n!ism/nls/connectionPolicy'
		], function(declare, lang, array, query, aspect, on, domClass, domConstruct, domStyle, 
				Memory, number, ItemFileReadStore, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, registry,
				Button, Dialog, CheckBoxSelect, CheckBox, GridFactory, Utils, connectionPolicyDialogTemplate, nls, nls_strings, 
				Resources, MessageHubAssociationControl, TextArea, IsmQueryEngine, keys, nls_cp) {

	/*
	 * This file defines a configuration widget that displays the list of message hubs
	 */

	var ConnectionPolicyDialog = declare("ism.controller.content.ConnectionPolicyDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog tied to the Add/Edit buttons in the ConnectionPolicyList widget
		// 
		// description:
		//		This widget pulls its structure from a template and adds functions to
		//		populate the dialog fields with a record (fillValues) and save the changes
		//		via POST.  For inputs that are more complex than a text box,
		//		_getFieldValue and _setFieldValue are defined to get/set a generic value for
		//		different input types (e.g. checkbox).
		nls: nls,
		nls_strings: nls_strings,
		nls_cp: nls_cp,
		templateString: connectionPolicyDialogTemplate,
        
        // template strings
        nameLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.name),		
		nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),		
        invalidClientIPMessage: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.invalid.clientIP),
        descriptionLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.description),
        clientIPLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.clientIP),
        invalidClientIPMessage: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.invalid.clientIP),
        IDLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.ID),
        invalidWildcard: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.invalid.wildcard),
        commonNameLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.commonName),
        clientIDLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.clientID),
        groupLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.Group),
        protocolLabel: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.protocol),
        protocolPlaceholder: Utils.escapeApostrophe(nls.messaging.connectionPolicies.dialog.selectProtocol),
        missingRequiredMessage: Utils.escapeApostrophe(nls_strings.global.missingRequiredMessage),        
        
		messageHubItem: null,
		protocolOptions: null,
		filters: ['ClientAddress', 'UserID', 'CommonNames', 'ClientID', 'GroupID', 'Protocol'],
		vars: ['${UserID}', '${GroupID}', '${ClientID}', '${CommonName}'],
		clientIdVars: ['${GroupID}', '${ClientID}'],

		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},
		
		restURI: "rest/config/connectionPolicies",
		domainUid: 0,

		constructor: function(args) {
			dojo.safeMixin(this, args);		
			this.inherited(arguments);
		},

		postCreate: function() {
            this.domainUid = ism.user.getDomainUid();

			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;			
			
			// check that the name matches the pattern and doesn't clash with an existing object
			if (this.add === true || Utils.configObjName.editEnabled()) {			
				// check that the name matches the pattern and doesn't clash with an existing object
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

			// get all the available protocols and populate the CheckBoxSelect
			this.protocolDeferrred.then(lang.hitch(this,function(data) {
				var store = Utils.getProtocolStore(data);
				this.field_Protocol.addOption(store.query({}, {sort: [{attribute: "label"}]}));
			}));
			
			// set up the onChange event handlers for filter check boxes
			// and validators for filters (if a check box is checked then
			// a value should be provided for a filter
			var t = this;
			var len = t.filters ? t.filters.length : 0;
			for (var i=0; i < len; i++) {				
				var filter = this.filters[i];
				var field = this["field_"+filter];
				field.defaultInvalidMessage = field.invalidMessage;
				var checkbox = this["checkbox_"+filter];
				if (checkbox && field) {
					checkbox.on("change", function(checked) {
						var dap = this.dojoAttachPoint;
						var prop = dap.substring(dap.indexOf("_")+1, dap.length); 
						t["field_"+prop].set("disabled", !checked);
					}, true);	
					domStyle.set(checkbox.iconNode, "display", "none");
					// create a validator for the field
					field.isValid = function(focused) {
						// a disabled field is always valid
						if (this.get("disabled")) {
							return true;
						}
						var dap = this.dojoAttachPoint;
						var prop = dap.substring(dap.indexOf("_")+1, dap.length); 						
						var value = t._getFieldValue(prop);
						if (value !== null && value !== undefined && lang.trim(value) !== "") {
							// does it have a regex?
							var matches = true;
							var pattern = this.get('pattern');
							if (pattern != undefined && pattern !== "") {
								matches = new RegExp("^(?:"+pattern+")$").test(value);
							}
							if (matches) {
								// make sure it doesn't contain any of the variables
								var clientID = prop == "ClientID";
								var vars = clientID ? t.clientIdVars : t.vars;
								var len = vars ? vars.length : 0;
								for (var vi=0; vi < len; vi++) {
									if (value.indexOf(vars[vi]) > -1) {
										matches = false;
										this.invalidMessage = clientID ? nls.messaging.connectionPolicies.dialog.invalid.clientIDvars : nls.messaging.connectionPolicies.dialog.invalid.vars;
									}
								}
							} else {
								this.invalidMessage = this.defaultInvalidMessage;
							}
							return matches;
						} 
						return false;
					};
					field.getErrorMessage = function(focused) {
						var dap = this.dojoAttachPoint;
						var prop = dap.substring(dap.indexOf("_")+1, dap.length); 						
						var value = t._getFieldValue(prop);
						if (value !== null && value !== undefined && lang.trim(value) !== "") {
							return this.invalidMessage;
						} 
						return nls_strings.global.missingRequiredMessage;
					};
					// check invalidMessage
					if (!field.hasOwnProperty("invalidMessage")) {
						field.invalidMessage = nls_strings.global.missingRequiredMessage;
					}
				}
				
				// set up onChange for description
				t.field_Description.on("change", function(args) {
					t.dialog.resize();
				}, true);
				
				// set up onChange for protocol select
				t.field_Protocol.on("change", function(args) {
					t.field_Protocol.validate();
				}, true);
				
				// set up blur for protocol select
				t.field_Protocol.on("blur", function(args) {
					t.field_Protocol.validate();
				}, true);
				
			}			
													
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

		},

		// populate the dialog with the passed-in values
		fillValues: function(values) {
			console.debug("ConnectionPolicyDialog -- fillValues", values);
			this._startingValues = values;
			//console.debug(values);
			for (var prop in values) {
				this._setFieldValue(prop, values[prop]);
			}	
		},
		
		show: function(connectionPolicy, list) {
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";						
			this.field_Name.existingNames = list;
			this._setupFields(connectionPolicy, true);
			if (connectionPolicy) {
				this.fillValues(connectionPolicy);
			}
			this.dialog.clearErrorMessage();
			this.dialog.show();
			this.field_Description.focus(); // needed to get the text area to resize
			if (this.add === true) {
				this.field_Name.focus();
			}
		},

		// check to see if anything changed, and save the record if so
		save: function(onFinish, onError) {
			var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
			bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.savingProgress;

			if (!onFinish) { 
				onFinish = lang.hitch(this,function() {	
					this.dialog.hide();
				});
			}
			if (!onError) { 	
				onError = lang.hitch(this,function(error) {	
					bar.innerHTML = "";
					this.dialog.showErrorFromPromise(nls.messaging.connectionPolicies.saveConnectionPolicyError, error);
					});
			}
			var values = {};
			var recordChanged = false;
			var filterSelected = false;
			
			// start with our fields, _startingValues doesn't always have non-required values in it!
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (values[actualProp] != this._startingValues[prop]) {
						recordChanged = true;
					}
					if (array.indexOf(this.filters, actualProp) >= 0 && 
							values[actualProp] !== null && values[actualProp] !== undefined && lang.trim(values[actualProp]) !== "") {
						filterSelected = true;
					} 
				}
			}
			
			// verify at least one filter has been selected
			if (filterSelected === false) {
				bar.innerHTML = "";				
				this.dialog.showErrorMessage(nls.messaging.connectionPolicies.dialog.invalid.filter);	
				return;
			}
			if (recordChanged) {			
				var policyName = values.Name;
				var dialog = this.dialog;
				var adminEndpoint = ism.user.getAdminEndpoint();
				var options = adminEndpoint.createPostOptions("ConnectionPolicy", values);
				var promise = adminEndpoint.doRequest("/configuration", options);
				promise.then(function(data) {
					onFinish(values);
				},
				onError);
			} else {
				dialog.hide(); // nothing to save
			}
		},
		
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("ConnectionPolicyDialog -- _setFieldValue", prop, value);
			// if there is a check box for this field, determine whether or not 
			// it should be checked
			var checkbox = this["checkbox_"+prop]; 
			if (checkbox) {
				var valueProvided = value !== null && value !== undefined && value !== "";
				this["checkbox_"+prop].set("checked", valueProvided);					
			}

			switch (prop) {
				case "Protocol":
					if (value) {
						this["field_"+prop].set("value",value.split(","));
					}
					break;					
				default:
					if (this["field_"+prop] && this["field_"+prop].set) {
						this["field_"+prop].set("value", value);
					} else if (this["field_"+prop]) {
						this["field_"+prop] = { value: value };
					}
					break;
			}
			
		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value = "";
			
			// if there is a check box for this field, is it enabled?
			var checkbox = this["checkbox_"+prop]; 
			if (checkbox) {
				if (!checkbox.get('checked')) {
					// there is a check box and it's not checked, so return an empty value
					return value;  
				}
			}
			
			switch (prop) {
				case "Protocol":
					/*
					 * get the array of options and create a comma
					 * delimited string representing selected protocols
					 */
					var arrayValue = this["field_"+prop].getOptions();
					var addSep  = false;
					var len = arrayValue ? arrayValue.length : 0;
					for (var i = 0; i < len; i++) {
						if (arrayValue[i].selected) {
							if (addSep) {
								value += ",";
							}
							value += arrayValue[i].value;
							addSep = true;
						}
					}
					break;
                case "AllowDurable":
                case "AllowPersistentMessages":
                    value = this["field_"+prop].get('checked');
                    break;					
				default:
					if (this["field_"+prop].get) {
						value = this["field_"+prop].get("value");
					} else {
						value = this["field_"+prop].value;
					}
					break;
			}
			//console.debug("returning field_"+prop+" as '"+value+"'");
			return value;
		},

		// do any adjustment of form field values necessary before showing the form.  in this
		// case, we populate the ipAddr ComboBox with valid ethernet IP addresses via REST API call
		_setupFields: function(connectionPolicy, showWarning) {
			console.debug("ConnectionPolicyDialog -- setupFields");
			// clear fields and any validation errors		
			dijit.byId(this.dialogId +"_DialogForm").reset();
		}

	});


	var ConnectionPolicyList = declare ("ism.controller.content.ConnectionPolicyList", [_Widget, _TemplatedMixin], {

		nls: nls,
		paneId: 'ismConnectionPolicyList',
		store: null,
		editDialog: null,
		removeDialog: null,
		messageHubItem: null,
		contentId: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		existingNames: [],
		detailsPage: null,

		addButton: null,
		deleteButton: null,
		editButton: null,
		queryEngine: IsmQueryEngine,
		
		restURI: "rest/config/connectionPolicies",
		domainUid: 0,

		constructor: function(args) {
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			console.debug("constructing ConnectionPolicyList");
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], idProperty: "id", queryEngine: this.queryEngine});	
		},
		
		startup: function() {
			this.inherited(arguments);
	         
			this.domainUid = ism.user.getDomainUid();
            			
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;			
						
			console.debug("startup ConnectionPolicyList");
			// start with an empty <div> for the content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
		
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
			}
		},

		_setDetailsPageAttr: function(value) {
			this.detailsPage = value;
			this.detailsPage.stores.connectionPolicies = this.store;
		},
		
		_initStore: function(reInit) {
			console.debug("initStore for ConnectionPolicies");	
			this.grid.emptyNode.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.refreshingGridMessage;
			
			var _this = this;
			
			// we want to show any connection policies who have any endpoint that is in the message hub 
			// or any connection policies with no endpoints
			
			// kick off the queries
			var endpointResults = this.detailsPage.promises.endpoints;
			var connectionPolicyResults = this.detailsPage.promises.connectionPolicies;
			
            var adminEndpoint = ism.user.getAdminEndpoint();
            adminEndpoint.getAllQueryResults([endpointResults, connectionPolicyResults], ["Endpoint", "ConnectionPolicy"], function(queryResults) {
                _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;

                var allEndpoints = queryResults.Endpoint; // this is all of them, just need the ones for this message hub
                var hubPolicyNames = [];    
                var hubPolicyRefs = [];
                var all = "";
                for (var j=0, numEndpoints=allEndpoints.length; j < numEndpoints; j++) {
                    var policyList = allEndpoints[j]['ConnectionPolicies'];                 
                    if (policyList && policyList !== "") {
                        all += policyList + ",";
                        var policies = policyList.split(',');
                        if (allEndpoints[j]['MessageHub'] == _this.messageHubItem.Name) {
                            for (var l=0, numPolicies=policies.length; l < numPolicies; l++) {
                                var hpRefsIndex = dojo.indexOf(hubPolicyNames, policies[l]);
                                var endpointName = allEndpoints[j].Name;
                                if (hpRefsIndex < 0) {
                                    hubPolicyNames.push(policies[l]);
                                    hubPolicyRefs.push([endpointName]);
                                } else {                                
                                    hubPolicyRefs[hpRefsIndex].push(endpointName);
                                }
                            }
                        }
                    }
                }
                
                var allReferredNames = all.split(',');  // every named connection policy, may contain dupes
                
                // hubPolicyNames now contains a list of all connectionPolicy names for the hub             
                var allConnectionPolicies = queryResults.ConnectionPolicy; // this is all of them, just need to ones for this message hub plus orphans
                var connectionPolicies = [];                
                _this.existingNames = [];
                var cpIndex = 0;
                for (var i=0, numConPolicies=allConnectionPolicies.length; i < numConPolicies; i++) { 
                    Utils.convertStringsToBooleans(allConnectionPolicies[i],[{property: "AllowDurable", defaultValue: true},{property: "AllowPersistentMessages", defaultValue: true}]);                    
                    var index = dojo.indexOf(hubPolicyNames, allConnectionPolicies[i].Name); 
                    allConnectionPolicies[i].associated = {checked: false, Name: allConnectionPolicies[i].Name, sortWeight: 1};
                    if ( index >= 0) {  
                        var endpointArray = hubPolicyRefs[index].sort();
                        allConnectionPolicies[i].endpoint = {list: endpointArray.join(","), Name: allConnectionPolicies[i].Name, sortWeight: endpointArray.length};  // add the endpoints to the object
                        allConnectionPolicies[i].endpointCount = endpointArray.length;
                        allConnectionPolicies[i].id = escape(allConnectionPolicies[i].Name);
                        connectionPolicies[cpIndex] = allConnectionPolicies[i];
                        cpIndex++;
                    } else if (dojo.indexOf(allReferredNames, allConnectionPolicies[i].Name) < 0) {
                        // found an orphan
                        allConnectionPolicies[i].endpoint = {list: "", Name: allConnectionPolicies[i].Name, sortWeight: 0};
                        allConnectionPolicies[i].endpointCount = 0;
                        allConnectionPolicies[i].id = escape(allConnectionPolicies[i].Name);
                        connectionPolicies[cpIndex] = allConnectionPolicies[i];
                        cpIndex++;
                    }
                    _this.existingNames[i] = allConnectionPolicies[i].Name;                  
                }

                connectionPolicies.sort(function(a,b){
                    if (a.Name < b.Name) {
                        return -1;
                    }
                    return 1;
                });         
                            
                if (_this.store.data.length === 0 && connectionPolicies.length === 0) {
                    _this.detailsPage.stores.connectionPolicies = _this.store;
                    return;
                }
                _this.store = new Memory({data: connectionPolicies, idProperty: "id", queryEngine: _this.queryEngine});
                _this.grid.setStore(_this.store);
                _this.triggerListeners();
                _this.detailsPage.stores.connectionPolicies = _this.store;
                if (reInit) {
                    _this.grid.resize();
                }                
            }, function(error) {
                _this.grid.emptyNode.innerHTML = nls.messaging.noItemsGridMessage;
                Utils.showPageErrorFromPromise(nls.messaging.connectionPolicies.retrieveConnectionPolicyError, error);                
            });
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the toolbar

			var protocolDeferrred = this.detailsPage.deferreds.protocols;

			var editId =  "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);
			this.editDialog = new ConnectionPolicyDialog({dialogId: editId, 
												  dialogTitle: nls.messaging.connectionPolicies.editDialog.title,
												  dialogInstruction: nls.messaging.connectionPolicies.editDialog.instruction,
												  protocolDeferrred:protocolDeferrred, add: false}, editId);
			this.editDialog.set("messageHubItem", this.messageHubItem);
			this.editDialog.startup();
			
			// create Add dialog
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new ConnectionPolicyDialog({dialogId: addId,
									                dialogTitle: nls.messaging.connectionPolicies.addDialog.title, 
									                dialogInstruction: nls.messaging.connectionPolicies.addDialog.instruction,
									                protocolDeferrred:protocolDeferrred, add: true}, addId);
			this.addDialog.set("messageHubItem", this.messageHubItem);
			this.addDialog.startup();			
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='removeConnectionPolicyDialog'></div>", 'ismConnectionPolicyList');
			var dialogId = "removeConnectionPolicyDialog";

			this.removeDialog = new Dialog({
				title: nls.messaging.messageHubs.connectionPolicyList.removeDialog.title,
				content: "<div>" + nls.messaging.messageHubs.connectionPolicyList.removeDialog.content + "</div>",
				style: 'width: 600px;',
				buttons: [
				          new Button({
				        	  id: dialogId + "_saveButton",
				        	  label: nls_strings.action.Ok,
				        	  onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
				          })
				          ],
				          closeButtonLabel: nls_strings.action.Cancel
			}, "removeConnectionPolicyDialog");
			this.removeDialog.dialogId = dialogId;
			this.removeDialog.save = lang.hitch(this, function(connectionPolicy, onFinish, onError) {
				
				var connectionPolicyName = connectionPolicy.Name;
				
				if (!onFinish) { onFinish = function() {this.removeDialog.hide();}; }
				if (!onError) { onError = lang.hitch(this,function(error) {	
					this.removeDialog.showErrorFromPromise(nls.messaging.connectionPolicies.deleteConnectionPolicyError, error);
					});
				}
				console.debug("Deleting: ", connectionPolicyName);
				var endpoint = ism.user.getAdminEndpoint();
	            var options = endpoint.getDefaultDeleteOptions();
	            var promise = endpoint.doRequest("/configuration/ConnectionPolicy/" + encodeURIComponent(connectionPolicyName), options);
				promise.then(onFinish, onError);
			});
			
			// create Remove not allowed dialog (remove not allowed when there are endpoints that reference the policy)
			var removeNotAllowedId = this.id+"_removeNotAllowedDialog";
			domConstruct.place("<div id='"+removeNotAllowedId+"'></div>", this.id);
			this.removeNotAllowedDialog = new Dialog({
				id: removeNotAllowedId,
				title: nls.messaging.connectionPolicies.removeNotAllowedDialog.title,
				content: "<div style='display: table;'>" +
						"<img style='display: inline-block; float: left; margin: 0 10px 0 0;' src='css/images/warning24.png' alt='' />" +
						"<span style='display: table-cell; vertical-align:middle; word-wrap:break-word;'>" + 
						nls.messaging.connectionPolicies.removeNotAllowedDialog.content + "</span><div>",
				closeButtonLabel: nls.messaging.connectionPolicies.removeNotAllowedDialog.closeButton
			}, removeNotAllowedId);

		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.messaging.messageHubs.connectionPolicyList.name, field: "Name", dataType: "string", width: "200px",
						decorator: Utils.nameDecorator
					},
					{	id: "endpoint", name: nls.messaging.messageHubs.connectionPolicyList.endpoints, field: "endpoint", filterable: false, dataType: "number", width: "80px",
						widgetsInCell: true,
						decorator: function() {	
								return "<div data-dojo-type='ism.widgets.MessageHubAssociationControl' data-dojo-props='associationType: \"endpoints\"' class='gridxHasGridCellValue'></div>";
						}
					},
	                {   id: "AllowDurable", name: nls_cp.allowDurable, field: "AllowDurable", dataType: "string", width: "90px",
	                    formatter: function(data) {
	                        return Utils.booleanDecorator(data.AllowDurable);
	                    }                         
                    },   
                    {   id: "AllowPersistentMessages", name: nls_cp.allowPersistentMessages, field: "AllowPersistentMessages", dataType: "string", width: "90px",
                        formatter: function(data) {
                            return Utils.booleanDecorator(data.AllowPersistentMessages);
                        }                         
                    },                                      
					{	id: "Description", name: nls.messaging.messageHubs.connectionPolicyList.description, field: "Description", dataType: "string", width: "200px",
						decorator: Utils.textDecorator
					}					
				];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, {paging: true, filter: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);	
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
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				console.log("selected from click: "+this.grid.select.row.getSelected());
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    console.log("clicked add button");
			    this.addDialog.show(null, this.existingNames);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				console.debug("clicked delete button");
				var policy = this.store.query({id: this.grid.select.row.getSelected()[0]})[0];
				var numEndpoints = policy.endpointCount;
				if (numEndpoints !== 0) {
					this.removeNotAllowedDialog.show();
				} else {
					query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
					this.removeDialog.show();
				}
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
				if (dijit.byId(this.editDialog.dialogId + "_DialogForm").validate()) {		
					var id = this.grid.select.row.getSelected()[0];
					var policy = this.store.query({id: id})[0];
					var policyName = policy.Name;
					this.editDialog.save(lang.hitch(this, function(data) {
						console.debug("save is done, returned:", data);
						// need to get the number of endpoints
						data.endpoint = policy.endpoint;
						data.endpointCount = policy.endpointCount;
						data.associated = {checked: false, Name: data.Name, sortWeight: 1};
						data.id = escape(data.Name);
						if (policyName != data.Name) {
							this.store.remove(id);
							this.store.add(data);
							var index = dojo.indexOf(this.existingNames, policyName);
							if (index >=0) {
								this.existingNames[index] = data.Name;
							}						
						} else {
							this.store.put(data, {id: data.id, overwrite: true});
						}
						this.triggerListeners();
						this.editDialog.dialog.hide();
					}));
				}
			}));
			
			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we clicked "save"
				console.debug("saving addDialog");
				if (dijit.byId(this.addDialog.dialogId + "_DialogForm").validate()) {		
					this.addDialog.save(lang.hitch(this, function(data) {
						console.debug("add is done, returned:", data);
						data.endpoint = {list: "", Name: data.Name, sortWeight: 0};
						data.endpointCount = 0;
						data.id = escape(data.Name);
						data.associated = {checked: false, Name: data.Name, sortWeight: 1};
						this.store.add(data);
						this.existingNames.push(data.Name);						
						this.triggerListeners();
						this.addDialog.dialog.hide();
					}));
				}
			}));

			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				// any message on this topic means that we confirmed the operation
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.messaging.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var connectionPolicy = this.store.query({id: id})[0];				
				var connectionPolicyName = connectionPolicy.Name;
				console.log("deleting " + connectionPolicyName);
				this.removeDialog.save(connectionPolicy, lang.hitch(this, function(data) {
					console.debug("remove is done, returned:", data);
					this.store.remove(id);
					var index = dojo.indexOf(this.existingNames, connectionPolicyName);
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
				}));

			}));
			
			dojo.subscribe("EndpointRemoved", lang.hitch(this, function(message) {
				 var endpointRemoved = message.Name;
				 var currentEntries = this.store.query();
				 for (var i=0, len=currentEntries.length; i < len; i++) {
					 var item = currentEntries[i];
					 var endpoints = item.endpoint.list.split(",");
					 var index = array.indexOf(endpoints, endpointRemoved); 
					 if ( index >= 0) {
						 endpoints.splice(index, 1);
						 item.endpointCount = endpoints.length;
						 item.endpoint = {list: endpoints.join(","), Name: item.Name, sortWeight: endpoints.length};
						 this.store.put(item);
					 }
				 }
				 this.triggerListeners();
				 
			}));

			dojo.subscribe("EndpointChanged", lang.hitch(this, function(message) {
				 var oldPolicies = message.oldValue.ConnectionPolicies;
				 var newPolicies = message.newValue.ConnectionPolicies;
				 if (oldPolicies == newPolicies) {
					 return;
				 }
				 if (oldPolicies) {
					 this._removeEndpointFromPolicies(message.oldValue);
				 }
				 if (newPolicies) {
					 this._addEndpointToPolicies(message.newValue);
				 }
				 this.triggerListeners();
			}));
			
			dojo.subscribe("EndpointAdded", lang.hitch(this, function(message) {				
				if (message.ConnectionPolicies) {
					this._addEndpointToPolicies(message);
					this.triggerListeners();
				}
			}));
			
			dijit.byId("ismMHTabContainer").watch("selectedChildWidget", lang.hitch(this, function(name, origVal, newVal) {
				if (newVal.id == Resources.pages.messaging.messageHubTabs.connectionPolicies.id) {
					this.grid.resize();
				}
			}));

		},
		
		_doSelect: function() {
			if (this.grid.select.row.getSelected().length > 0) {
				this.deleteButton.set('disabled',false);
				this.editButton.set('disabled',false);
			} else {
				this.deleteButton.set('disabled',true);
				this.editButton.set('disabled',true);
			}
		},
		
		_doEdit: function() {
			var connectionPolicy = this.store.query({id: this.grid.select.row.getSelected()[0]})[0];
			// create array of existing names except for the current one, so we don't clash
			var names = array.map(this.existingNames, function(item) { 
				if (item != connectionPolicy.Name) return item; 
			});			
			this.editDialog.show(connectionPolicy,names);
		},
		
		_removeEndpointFromPolicies: function(endpoint) {
			var endpointName = endpoint.Name;
			var policies = endpoint.ConnectionPolicies.split(',');
			for (var i=0, len=policies.length; i < len; i++) {
				var results = this.store.query({Name: policies[i]});
				if (!results || results.length === 0 || !results[0].endpoint) {
					continue;
				}
				var item = results[0];					
				var endpoints = item.endpoint.list.split(",");
				var index = array.indexOf(endpoints, endpointName); 
				if ( index >= 0) {
					endpoints.splice(index, 1);
					item.endpointCount = endpoints.length;
					item.endpoint = {list: endpoints.join(","), Name: item.Name, sortWeight: endpoints.length};
					this.store.put(item);
				}
			}			
		},

		_addEndpointToPolicies: function(endpoint) {
			var endpointName = endpoint.Name;
			var policies = endpoint.ConnectionPolicies.split(',');
			for (var i=0, len=policies.length; i < len; i++) {
				var results = this.store.query({Name: policies[i]});
				if (!results) {
					continue;
				}
				var item = results[0];
				var endpoints = [];
				if (item.endpoint && item.endpoint.list !== "") {
					endpoints = item.endpoint.list.split(",");
				}
				endpoints.push(endpointName);				
				endpoints.sort();
				item.endpoint = {list: endpoints.join(","), Name: item.Name, sortWeight: endpoints.length};
				item.endpointCount = endpoints.length;
				this.store.put(item);
			}			
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

	
	return ConnectionPolicyList;
});
