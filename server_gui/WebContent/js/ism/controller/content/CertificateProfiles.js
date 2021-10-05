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
		'dojo/aspect',
		'dojo/query',
		'dojo/on',
		'dojo/dom-construct',
		'dojo/store/Memory',
		'dojo/date/locale',
		'dojo/when',
		'dijit/_base/manager',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/form/Button',
		'ism/controller/content/CertificateProfileDialog',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/IsmUtils',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/strings',
		'ism/config/Resources',
		'dojo/keys'
		], function(declare, lang, aspect, query, on, domConstruct,
				Memory, locale, when, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, CertificateProfileDialog, GridFactory, Dialog, Utils, nls, nls_strings, Resources, keys) {

	return declare("ism.controller.content.CertificateProfiles", [_Widget, _TemplatedMixin], {

		grid: null,
		store: null,
		retry: true,  // to prevent multiple retries
		editDialog: null,
		addDialog: null,
		removeDialog: null,
		contentId: null,
		timeOffset: null,
		
		addButton: null,
		deleteButton: null,
		editButton: null,
		
		uploadAction: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		
		domainUid: 0,
		     			
		constructor: function(args) {
			dojo.safeMixin(this, args);
		
			this.contentId = this.id + "Content";
			this.store = new Memory({data:[], idProperty: "name"});
			
		},

		postCreate: function() {
			this.inherited(arguments);
			
		    this.domainUid = ism.user.getDomainUid();
						
			this.certificatesURI += "/" + this.domainUid;
			this.uploadAction = Utils.getBaseUri() + this.certificatesURI;

			// start with an empty <div> for the Listeners content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			
			Utils.updateStatus(function(data) {
				console.debug("TIME");
				if (data.dateTime) {
					var appTime = new Date(data.dateTime);
					this.timeOffset = dojo.date.difference(new Date(), appTime, "second");
				}
			},
			function(error) {
				console.debug("Error: ", error);
			});
			
			// when switching to this page, refresh the field values
			dojo.subscribe(Resources.pages.appliance.nav.securitySettings.topic, lang.hitch(this, function(message){
				this._initStore();
			}));	
			dojo.subscribe(Resources.pages.appliance.nav.securitySettings.topic+":onShow", lang.hitch(this, function(message){
				console.debug(this.declaredClass + " refreshing grid");					
				this._refreshGrid();
			}));	
			

			this._initGrid();
			this._initStore();
			this._initDialogs();
			this._initEvents();
		},

		_initStore: function() {
			// summary:
			// 		get the initial records from the server
			var _this = this;
            // get the certificate profiles
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/CertificateProfile", adminEndpoint.getDefaultGetOptions());
            promise.then(
                    function(data) {
                        _this._populateProfiles(data);
                    },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                        _this._populateProfiles({"CertificateProfile" : {}});
                    } else {
                        Utils.showPageErrorFromPromise(nls.appliance.certificateProfiles.retrieveError, error);
                    }
                }
            );			
		},
		
		_populateProfiles:  function(data) {
		    var _this = this;
		    var adminEndpoint = ism.user.getAdminEndpoint();
		    var profiles = adminEndpoint.getObjectsAsArray(data, "CertificateProfile");
            var mapped = dojo.map(profiles, function(item){
                item.id = escape(item.Name);
                return item;
            });
            _this.store = new Memory({data: mapped, idProperty: "id"});
            _this.grid.setStore(_this.store);
            _this.triggerListeners();
		},

		
		_saveSettings: function(values, dialog, onFinish, onError) {		    
		    var _this = this;

		    if (!onFinish) { onFinish = function() {    
		            _this.dialog.hide();
		        };
		    }
		    if (!onError) { onError = function(error) { 
		            query(".idxDialogActionBarLead", dialog.domNode)[0].innerHTML = "";
		            dialog.showErrorFromPromise(nls.appliance.savingFailed, error);
		        };
		    }

		    delete values.id;  // remove id since REST API doesn't expect it.
		    delete values.ExpirationDate; // remove ExpirationDate - returned by REST GET
		                                  // but not accepted on POST

		    var adminEndpoint = ism.user.getAdminEndpoint();
		    var options = adminEndpoint.createPostOptions("CertificateProfile", values);
		    var promise = adminEndpoint.doRequest("/configuration", options);
		    promise.then(
	            function(data) {
	                console.dir(data);
	                // retrieve the certificate profile, hopefully is has an ExpirationData in it...
	                var promise2 = adminEndpoint.doRequest("/configuration/CertificateProfile/"+encodeURIComponent(values.Name), adminEndpoint.getDefaultGetOptions());
	                promise2.then(function(data) {
	                    var profile = adminEndpoint.getNamedObject(data, "CertificateProfile", values.Name);
	                    onFinish(profile);
	                }, onError);          
	            },
	            onError
		    );			
			return;
		},
		
		_deleteEntry: function(id, name, dialog, onFinish, onError) {
            var _this = this;

            if (!onFinish) { onFinish = function() {    
                    _this.dialog.hide();
                    _this.grid.store.remove(id);
                };
            }
            if (!onError) { onError = function(error) { 
                    query(".idxDialogActionBarLead", dialog.domNode)[0].innerHTML = "";
                    dialog.showErrorFromPromise(nls.appliance.deletingFailed, error);
                };
            }

            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/CertificateProfile/"+encodeURIComponent(name), adminEndpoint.getDefaultDeleteOptions());
            promise.then(onFinish, onError);
            return;
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar
						
			// create Edit dialog
			console.debug("id="+this.id+" contentId="+this.contentId);
			console.debug("editDialogTitle="+this.editDialogTitle);
			var editTitle = dojo.string.substitute(this.editDialogTitle,{nls:nls});
			var editId = "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);
			
			this.editDialog = new CertificateProfileDialog({dialogId: editId, dialogTitle: editTitle, uploadAction: this.uploadAction }, editId);
			this.editDialog.startup();

			// create Add dialog
			var addTitle = dojo.string.substitute(this.addDialogTitle,{nls:nls});
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new CertificateProfileDialog({add: true, dialogId: addId, dialogTitle: addTitle, uploadAction: this.uploadAction },
					addId);
			this.addDialog.startup();
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.contentId);
			var dialogId = "remove"+this.id+"Dialog";
			var title = dojo.string.substitute(this.removeDialogTitle,{nls:nls});
			this.removeDialog = new Dialog({
				id: dialogId,
				title: title,
				content: "<div>" + nls.appliance.removeDialog.content + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_saveButton",
						label: nls.appliance.removeDialog.deleteButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.appliance.removeDialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
		},

		_initGrid: function() {
			// summary:
			// 		create and instantiate the grid
			var gridId = this.id + "Grid";
			var structure = [
					{	id: "Name", name: nls.appliance.certificateProfiles.grid.name, field: "Name", dataType: "string", width: "160px",
						decorator: Utils.nameDecorator
					},
					{	id: "Certificate", name: nls.appliance.certificateProfiles.grid.certificate, field: "Certificate", dataType: "string", width: "160px" },
					{	id: "Key", name: nls.appliance.certificateProfiles.grid.privkey, field: "Key", dataType: "string", width: "160px" },
					{	id: "ExpirationDate", name: nls.appliance.certificateProfiles.grid.certexpiry, field: "ExpirationDate", dataType: "string", width: "262px",
						decorator: lang.hitch(this, function(cellData) {
						    if (cellData && cellData !== "") {
    							var certDate = locale.parse(cellData, { datePattern: "yyyy-MM-dd", timePattern: "HH:mm (z)" });
    							var appDate = dojo.date.add(new Date(), "second", this.timeOffset);
    							if (dojo.date.compare(appDate, certDate) > 0) {
    								return "<span style='color: red; vertical-align: middle;'>" + cellData + "</span>" +
    								"<span style='padding-left: 10px;'>" +
    								"<img style='vertical-align: middle; width=16px;height=16px' src='css/images/msgWarning16.png' alt='"+ nls_strings.level.Warning +"' /></span>";		
    							}
						    }
							return cellData;							
						})
					}
				];
			this.grid = GridFactory.createGrid(gridId, this.contentId, this.store, structure, { filter: true, paging: true, heightTrigger: 0});
			this.addButton = GridFactory.addToolbarButton(this.grid, GridFactory.ADD_BUTTON, true);
			this.deleteButton = GridFactory.addToolbarButton(this.grid, GridFactory.DELETE_BUTTON, false);
			this.editButton = GridFactory.addToolbarButton(this.grid, GridFactory.EDIT_BUTTON, false);			
		},

		_getSelectedData: function() {
			// summary:
			// 		get the row data object for the current selection
			return this.grid.row(this.grid.select.row.getSelected()[0]).rawData();
		},

		_refreshGrid: function() {
			// summary:
			// 		refresh the contents of the grid
			this.grid.body.refresh();
			this.deleteButton.set('disabled',true);
			this.editButton.set('disabled',true);
		},

		_initEvents: function() {
			// summary:
			// 		init events connecting the grid, toolbar, and dialog
			
			aspect.after(this.grid, "onRowClick", lang.hitch(this, function() {
				this._doSelect();
			}));
			
			on(this.addButton, "click", lang.hitch(this, function() {
			    // create array of existing names so we don't clash
				var existingNames = this.store.query({}).map(
						function(item){ return item.Name; });
				var existingCerts = this.store.query({}).map(
						function(item){ return item.Certificate; });
				var existingKeys = this.store.query({}).map(
						function(item){ return item.Key; });
				var existingFiles = existingCerts.concat(existingKeys);
			    this.addDialog.show(existingNames, existingFiles);
			}));
			
			on(this.deleteButton, "click", lang.hitch(this, function() {
				query(".idxDialogActionBarLead",this.removeDialog.domNode)[0].innerHTML = "";
				dijit.byId(this.removeDialog.dialogId + "_saveButton").set('disabled', false);
				this.removeDialog.show();
			}));
					
			on(this.editButton, "click", lang.hitch(this, function() {
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
				// any message on this topic means that we clicked "save"
				if (dijit.byId("edit"+this.id+"Dialog_form").validate()) {
				    var _this = this;
					var profile = _this._getSelectedData();
					var name = profile.Name;
					var id = _this.grid.select.row.getSelected()[0];
					_this.editDialog.save(_this.store, id, function(data, obj) {
						console.debug("edit is done, returned:", data);
						var bar = query(".idxDialogActionBarLead",_this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;						
						_this._saveSettings(data,_this.editDialog.dialog, function(data){
							data.id = escape(data.Name);
							if (data.Name != _this.editDialog._startingValues.Name) {
								console.debug("name changed, removing old name");
								_this.store.remove(id);
								_this.store.add(data);		
							} else {
								_this.store.put(data,{overwrite: true});					
							}
							_this.editDialog.dialog.hide();
							_this.triggerListeners();
						});
					});
				}
			}));

			dojo.subscribe(this.addDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
			    // any message on this topic means that we clicked "save"
			    console.debug("saving addDialog");
			    if (dijit.byId("add"+this.id+"Dialog_form").validate()) {
			        var _this = this;
			        var bar = query(".idxDialogActionBarLead",_this.addDialog.dialog.domNode)[0];
			        bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
			        _this.addDialog.save(_this.store, null, function(data) {
			            _this._saveSettings(data, _this.addDialog.dialog, function(data){
			                data.id = escape(data.Name);
			                when(_this.store.add(data), function(){
			                    _this.addDialog.dialog.hide();
			                    _this.triggerListeners();								
			                });
			            });
			        });
			    }
		    }));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				console.debug("remove is done");
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.deletingProgress;
				var id = this.grid.select.row.getSelected()[0];
				var name = this._getSelectedData().Name;
				var dialog = this.removeDialog;
				var _this = this;
				this._deleteEntry(id, name, dialog, function (data) {
				    dialog.hide();
                    _this.grid.store.remove(id);
					// disable buttons if empty or no selection
					if (_this.grid.store.data.length === 0 || 
							_this.grid.select.row.getSelected().length === 0) {
						_this.deleteButton.set('disabled',true);
						_this.editButton.set('disabled',true);											
					}					
					_this.triggerListeners();
				}, function(error) {
					console.debug("delete failed");
					bar.innerHTML = "";
					dialog.showErrorFromPromise(nls.appliance.deletingFailed, error);
					dijit.byId(dialog.dialogId + "_saveButton").set('disabled', true);					
				});
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
			var rowData = this._getSelectedData();

			var existingNames = this.store.query(function(item){ return item.Name != rowData.Name; }).map(
					function(item){ return item.Name; });
			var existingCerts = this.store.query(function(item){ return item.Certificate != rowData.Certificate; }).map(
					function(item){ return item.Certificate; });
			var existingKeys = this.store.query(function(item){ return item.Key != rowData.Key; }).map(
					function(item){ return item.Key; });
			var existingFiles = existingCerts.concat(existingKeys);
			this.editDialog.show(existingNames, existingFiles, rowData);
		},
		
		// This method can be used by other widgets with a aspect.after call to be notified when
		// the store gets updated 
		triggerListeners: function() {
			console.debug(this.declaredClass + ".triggerListeners()");
			
			// update grid quick filter if there is one
			if (this.grid.quickFilter) {
				this.grid.quickFilter.apply();				
			}
		}
	});

});
