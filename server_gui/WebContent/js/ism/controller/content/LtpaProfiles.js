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
		'dojo/aspect',
		'dojo/dom',
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
		'dojo/text!ism/controller/content/templates/LtpaProfileDialog.html',
		'ism/widgets/GridFactory',
		'ism/widgets/Dialog',
		'ism/IsmUtils',
		'dojo/i18n!ism/nls/appliance',
		'dojo/i18n!ism/nls/strings',
		'ism/config/Resources',
		'dojo/keys'
		], function(declare, lang, array, aspect, dom, query, on, domConstruct,
				Memory, locale, when, manager, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Button, ltpaProfileDialogTemplate, GridFactory, Dialog, Utils, nls, nls_strings, Resources, keys) {
	
	var LtpaProfileDialog = declare("ism.controller.content.LtpaProfileDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		nls: nls,
		nls_strings: nls_strings,
		templateString: ltpaProfileDialogTemplate,	

        // template strings
        nameLabel: Utils.escapeApostrophe(nls_strings.name.label),
        nameTooltip: Utils.escapeApostrophe(nls_strings.name.tooltip),
        browseHint: Utils.escapeApostrophe(nls.appliance.ltpaProfiles.browseHint),
        browseLabel: Utils.escapeApostrophe(nls.appliance.ltpaProfiles.browse),
        privkeyLabel: Utils.escapeApostrophe(nls.appliance.ltpaProfiles.keyFilename),
        privkeyTooltip: Utils.escapeApostrophe(nls.appliance.ltpaProfiles.keyFilenameTooltip),
        keypasswordLabel: Utils.escapeApostrophe(nls.appliance.ltpaProfiles.password),
        overwriteLabel: Utils.escapeApostrophe(nls.allowCertfileOverwriteLabel),

		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_startingValues: {},
		_fileToUpload: null,
		// function to call when all files needing to be uploaded are complete
		_uploadComplete: null,
		_savedValues: null,
		_obj: null,

		// variable to keep count of uploaded files
		_uploadedFileCount: null,
		_uploadTimeout: 30000, // milliseconds
		_timeoutID: null,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
		
		postCreate: function() {		
			
			this.field_KeyFileName.isValid = function(focused) {
				var value = this.get("value");
				if (!value || value === ""){
					return false;
				}
				var unique = !(array.some(this.existingNames, function(item){ return item.toLowerCase() === value.toLowerCase(); }));
				return unique;				
			};
			this.field_KeyFileName.getErrorMessage = function() {
				var value = this.get("value");
				if (!value || value === ""){
					return nls.appliance.invalidRequired;
				}
				return nls.appliance.ltpaProfiles.duplicateFileError;
			};
			
			if ((this.add === true || Utils.configObjName.editEnabled())) {			
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
			
			// watch the form for changes between valid and invalid states
			dijit.byId(this.dialogId+"_form").watch('state', lang.hitch(this, function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId(this.dialogId+"_saveButton");
				if (newvalue) {					
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			}));
			
			// reset the form on close
			aspect.after(this.dialog, "hide", lang.hitch(this, function() {
				this._fileToUpload = null;
				this._savedValues = null;
				this._obj = null;
				dijit.byId(this.dialogId+"_form").reset();
			}));
		},

		clickFileUploaderButton: function() {
		    this.fileUploader.click();
		},

		handleFileChangeEvent: function() {                      
		    var files = this.fileUploader.files;
		    var field = this.field_KeyFileName;
		    if (files && files.length > 0 && files[0].name) {
		        var filename = files[0].name;
		        field.set("value", filename);  
		        if (field.validate()) {
		            this._fileToUpload = files[0];
		        } else {
		            this._fileToUpload = undefined;
		        }                  
		    } else {
		        field.set("value", "");
		        this._fileToUpload = undefined;
		    }           
		},                      

		handleError: function(error, errorMessage) {
			if (this._timeoutID) {
				window.clearTimeout(this._timeoutID);
			}
			if (!errorMessage) {
				errorMessage = nls.appliance.ltpaProfiles.uploadError; 
			}
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";					
			this.dialog.showErrorFromPromise(errorMessage, error);
		},

		
		// populate the dialog with the profile name only
		// REST requires both a file and a password value for update actions
		fillValues: function(values) {
			this._startingValues["Name"] = values["Name"];
			this._setFieldValue("Name", values["Name"]);
			this._fileToUpload = null;
		},
		
		show: function(names, keyfiles, rowData) {
			this.field_Name.existingNames = names;
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_KeyFileName.existingNames = keyfiles;
	        this.fileUploader.value = "";
			if (this.add === true) {
				this.field_Name.set("value", "");
				this.field_KeyFileName.set("value", "");				
				dijit.byId(this.dialogId+"_saveButton").set('disabled', true);
				dom.byId(this.dialogId+"_overwrite").innerHTML = "";
			} else if (rowData) {
				this.fillValues(rowData);
				this.field_KeyFileName.set("value", "");
				dijit.byId(this.dialogId+"_saveButton").set('disabled', false);
			}
			this.field_Password.set("value", "");
			this.dialog.show();
			this.field_Name.focus();		    
			this._uploadedFileCount = 0;
		},
		
		// check to see if anything changed, and save the record if so
		save: function(store, position, onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			var obj = store.get(position);
			var recordChanged = false;
						
			if (this._timeoutID) {
				window.clearTimeout(this._timeoutID);
			}
			var values = {};
			if (!this._startingValues) {
				this._startingValues = {};
			}
			for (var prop in this) {
				if (this.hasOwnProperty(prop) && prop.substring(0,6) == "field_") {
					var actualProp = prop.substring(6);
					values[actualProp] = this._getFieldValue(actualProp);
					if (values[actualProp] != this._startingValues[actualProp]) {
						if (values[actualProp] === "" && this._startingValues[actualProp] === undefined) {
							// consider it unchanged
							continue;
						}
						recordChanged = true;
					}
				}
			}
			
			/* allow to change password anyway? if we cannot determine how to validate password, I think we should...
			 * if (!this.add && this._startingValues["Name"] == values["Name"]) {
				// for an update with no name change, warn the user if they haven't selected any file to upload
				if (this._fileToUpload == null && this._uploadedFileCount == 0) {
					this.dialog.showWarningMessage(nls.appliance.ltpaProfiles.noFilesToUpload);
					return;
				}
			}
			*/
			
			if (this._fileToUpload) {
				recordChanged = true;
			}
			
			if (recordChanged) {
				
				this._savedValues = values;
				this._obj = obj;
				this._uploadComplete = onFinish;
				
                var _this = this;
                var adminEndpoint = ism.user.getAdminEndpoint();
				
				// is there a file to upload?
				if (this._fileToUpload) {
                    adminEndpoint.uploadFile(_this._fileToUpload, function(data) {
                        _this._uploadedFileCount++;
                        _this._fileToUpload = null;
                        console.debug("upload complete");
                        window.clearTimeout(_this._timeoutID);
                        _this._uploadComplete(values,obj);                 
                    }, function(error) {
                        _this.handleError(error, nls.appliance.ltpaProfiles.uploadError);                        
                    }); 
    				
    				// schedule timeout
    				this._timeoutID = window.setTimeout(lang.hitch(this, function(){
    					this.handleError(null, nls.appliance.ltpaProfiles.uploadError);
    				}), this._uploadTimeout);
				} else {
					onFinish(values,obj);
					return;
				}
			} else {
				console.debug("nothing changed, so no need to save");
				this.dialog.hide();
			}			
		},
		
		// set a field value according to special rules (prefixes, checkbox on/off, etc.)
		_setFieldValue: function(prop, value) {
			console.debug("setting field_"+prop+" to '"+value+"'");

			if (this["field_"+prop] && this["field_"+prop].set) {
				this["field_"+prop].set("value", value);
			} else {
				this["field_"+prop] = { value: value };
			}

		},

		// define any special rules for getting a field value (e.g. for checkboxes)
		_getFieldValue: function(prop) {
			var value;

			switch(prop) {
			case "Overwrite":
				value = this["field_Overwrite"].get("checked");
				break;
			default:
				if (this["field_"+prop].get) {
					value = this["field_"+prop].get("value");
				} else {
					value = this["field_"+prop].value;
				}
			    break;
			}
			return value;
		}		
	});

	var LtpaProfiles = declare("ism.controller.content.LtpaProfiles", [_Widget, _TemplatedMixin], {

		grid: null,
		store: null,
		retry: true,  // to prevent multiple retries
		editDialog: null,
		addDialog: null,
		removeDialog: null,
		contentId: null,
		
		addButton: null,
		deleteButton: null,
		editButton: null,
		
		uploadAction: null,
		templateString: "<div data-dojo-attach-point='mainNode'></div>",
		restURI: "rest/config/ltpaProfiles",
		domainUid: 0,
		     			
		constructor: function(args) {
			dojo.safeMixin(this, args);
		
			this.contentId = this.id + "Content";
			console.debug("contentId="+this.contentId);
			this.store = new Memory({data:[], idProperty: "id"});
		},

		postCreate: function() {
			this.inherited(arguments);
            
            this.domainUid = ism.user.getDomainUid();
			
			// Add the domain Uid to the REST API.
			this.restURI += "/" + this.domainUid;			
			this.uploadAction = Utils.getBaseUri() + this.restURI;			
			
			// start with an empty <div> for the Listeners content
			this.mainNode.innerHTML = "<div id='"+this.contentId+"'></div>";
			
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
            var adminEndpoint = ism.user.getAdminEndpoint();
            var promise = adminEndpoint.doRequest("/configuration/LTPAProfile", adminEndpoint.getDefaultGetOptions());
            promise.then(
                function(data) {
                    _this._populateProfiles(data);
                },
                function(error) {
                    if (Utils.noObjectsFound(error)) {
                        _this._populateProfiles({"LTPAProfile" : {}});
                    } else {
                        Utils.showPageErrorFromPromise(nls.appliance.ltpaProfiles.retrieveError, error);
                    }
                }
            );          
        },
        
        _populateProfiles: function(data) {
            var _this = this;
            var adminEndpoint = ism.user.getAdminEndpoint();
            var profiles = adminEndpoint.getObjectsAsArray(data, "LTPAProfile");
            // add an id property with an encoded form of the Name
            var mapped = dojo.map(profiles,function(item) {
                item.id = escape(item.Name);
                return item;
            });
            _this.store = new Memory({data: mapped, idProperty: "id"});
            _this.grid.setStore(_this.store);
            _this.triggerListeners();        			
		},
		
		_saveSettings: function(name, values, dialog, onFinish, onError) {
			var _this = this;
			if (!onFinish) { onFinish = function() {    
    			    dialog.hide();
    			};
			}
			if (!onError) { onError = function(error) { 
    			    query(".idxDialogActionBarLead", dialog.domNode)[0].innerHTML = "";
    			    dialog.showErrorFromPromise(nls.appliance.savingFailed, error);
    			};
			}
        
			var saveValues = {};
			for (var prop in values) {
				saveValues[prop] = values[prop];
			}
			delete saveValues.id;  // remove id since REST API doesn't expect it.
			 
            var adminEndpoint = ism.user.getAdminEndpoint();
            var options = adminEndpoint.createPostOptions("LTPAProfile", saveValues);
            var promise = adminEndpoint.doRequest("/configuration", options);
            promise.then(function(data) { onFinish(values); }, onError);   
		},

		_deleteEntry: function(id, name, dialog, onComplete, onError) {
	         var _this = this;
             var adminEndpoint = ism.user.getAdminEndpoint();
             var promise = adminEndpoint.doRequest("/configuration/LTPAProfile/"+encodeURIComponent(name), adminEndpoint.getDefaultDeleteOptions());
             promise.then(
                 function(data) {
                     _this.grid.store.remove(id);    
                     if (onComplete) onComplete();                       
                 }, function(error) {
                     query(".idxDialogActionBarLead",dialog.domNode)[0].innerHTML = "";
                     dialog.showErrorFromPromise(nls.appliance.deletingFailed, error);
                     if (onError) onError();                     
                 });
		},

		_initDialogs: function() {
			// summary:
			// 		create and instantiate the dialogs triggered by the grid/toolbar

		    var instruction = Utils.escapeApostrophe(nls.appliance.ltpaProfiles.instruction);
		    
			// create Edit dialog
			var editTitle = Utils.escapeApostrophe(dojo.string.substitute(this.editDialogTitle,{nls:nls}));
			var editId = "edit"+this.id+"Dialog";
			domConstruct.place("<div id='"+editId+"'></div>", this.contentId);		
			this.editDialog = new LtpaProfileDialog({dialogId: editId, dialogTitle: editTitle, 
									instruction: instruction }, editId);
			this.editDialog.startup();

			// create Add dialog
			var addTitle = Utils.escapeApostrophe(dojo.string.substitute(this.addDialogTitle,{nls:nls}));
			var addId = "add"+this.id+"Dialog";
			domConstruct.place("<div id='"+addId+"'></div>", this.contentId);
			this.addDialog = new LtpaProfileDialog({add: true, dialogId: addId, dialogTitle: addTitle, 
									instruction: instruction }, addId);
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
					{	id: "Name", name: nls_strings.name.label, field: "Name", dataType: "string", width: "300px",
						decorator: Utils.nameDecorator
					},
					{	id: "KeyFileName", name: nls.appliance.ltpaProfiles.keyFilename, field: "KeyFileName", 
						dataType: "string", width: "300px", decorator: Utils.textDecorator }
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
				var existingFiles = this.store.query({}).map(
						function(item){ return item.KeyFileName; });
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
					var profile = this._getSelectedData();
					var name = profile.Name;
					var id = this.grid.select.row.getSelected()[0];
					this.editDialog.save(this.store, id, function(data, obj) {
						var bar = query(".idxDialogActionBarLead",_this.editDialog.dialog.domNode)[0];
						bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;						
						_this._saveSettings(name,data,_this.editDialog.dialog, function(data) {
							data.Password = ""; // don't store the password
							data.id = escape(data.Name);
							if (data.Name != _this.editDialog._startingValues["Name"]) {
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
				if (dijit.byId("add"+this.id+"Dialog_form").validate()) {		
				    var _this = this;
					var bar = query(".idxDialogActionBarLead",this.addDialog.dialog.domNode)[0];					
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.appliance.savingProgress;
					this.addDialog.save(this.store, null, function(data) {
						_this._saveSettings(data.Name, data, _this.addDialog.dialog, function(data) {
							data.Password = ""; // don't store the password
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
				var _this = this;
				this._deleteEntry(id, name, this.removeDialog, function() {
					_this.removeDialog.hide();		
					// disable buttons if empty or no selection
					if (_this.grid.store.data.length === 0 || 
							_this.grid.select.row.getSelected().length === 0) {
						_this.deleteButton.set('disabled',true);
						_this.editButton.set('disabled',true);											
					}					
					_this.triggerListeners();
				}, function(error) {
					console.debug("delete failed");
					dijit.byId(_this.removeDialog.dialogId + "_saveButton").set('disabled', true);					
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
			var existingFiles = this.store.query(function(item){ return item.KeyFileName != rowData.KeyFileName; }).map(
					function(item){ return item.KeyFileName; });
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
	return LtpaProfiles;
});
