/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
		'dojo/aspect',
        'dojo/dom-construct',
        'dojo/dom-attr',
        'dojo/number',
    	'dojo/query',
    	'dojo/date/locale',
		'dojo/on',
		'dojox/form/Uploader',
		'dojox/form/uploader/FileList',
        'dijit/_Widget',
        'dijit/_TemplatedMixin',
        'dijit/_WidgetsInTemplateMixin',	
        'dijit/form/Button',
		'ism/widgets/Dialog',
        'ism/widgets/TextBox',
        'ism/config/Resources',
		'ism/widgets/_TemplatedContent',
        'ism/IsmUtils',
        'dojo/i18n!ism/nls/messaging',
        'dojo/i18n!ism/nls/appliance',
        'dojo/text!ism/controller/content/templates/SSLKeyForm.html',
		'dojo/text!ism/controller/content/templates/SSLKeyRepositoryDialog.html'
        ], function(declare, lang, xhr, json, aspect, domConstruct, domAttr, number, query, locale, on, Uploader, FileList, _Widget, _TemplatedMixin, 
        		_WidgetsInTemplateMixin, Button, Dialog, TextBox, Resources, _TemplatedContent,	Utils, nls, nlsAppl, template, SSLKeyRepositoryDialog) {

	var SSLKeyRepositoryAddDialog = declare("ism.controller.content.SSLKeyRepositoryAddDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Form that allows you to define a new message hub
		// 
		// description:
		//		
		nls: nls,
		templateString: SSLKeyRepositoryDialog,	

		// the unique field that identifies a record on server XHR requests
		keyField: "id",
		
		// when the dialog is first populated with a record, we save
		// the starting values for comparison on save
		_sslKeyToUpload: null,
		_passwordToUpload: null,
		_files: {},
		// function to call when all files needing to be uploaded are complete
		uploadComplete: null,
		_savedValues: null,

		// variable to keep count of uploaded files
		uploadedFileCount: null,
		uploadTimeout: 30000, // milliseconds
		timeoutID: null,
		
        // template strings
		keyFileLabel: Utils.escapeApostrophe(nls.messaging.sslkeyrepository.dialog.keyFileLabel),		
        browseHint: Utils.escapeApostrophe(nls.messaging.sslkeyrepository.dialog.browseHint),     
        browseButton: Utils.escapeApostrophe(nls.messaging.sslkeyrepository.dialog.browseButton),     
        passwordFileLabel: Utils.escapeApostrophe(nls.messaging.sslkeyrepository.dialog.passwordFileLabel),  
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},

		postCreate: function() {		
			console.debug(this.declaredClass + ".postCreate()");
			
			this.field_keyFile.isValid = lang.hitch(this, function() {
				var value = this.field_keyFile.get("value");
				if (value && value !== "") {
					return value.toLowerCase().match(/.kdb$/);
				}
				return false;
			});
			this.field_keyFile.getErrorMessage = lang.hitch(this, function() {
				var value = this.field_keyFile.get("value");
				if (value && value !== "") {
					return nls.messaging.sslkeyrepository.dialog.keyRepositoryError;
				}
				return nls.messaging.sslkeyrepository.dialog.keyRepositoryMissingError;
			});
			
			this.field_passwordFile.isValid = lang.hitch(this, function() {
				var value = this.field_passwordFile.get("value");
				if (value && value !== "") {
					return value.toLowerCase().match(/.sth$/);
				}
				return false;
			});
			this.field_passwordFile.getErrorMessage = lang.hitch(this, function() {
				var value = this.field_passwordFile.get("value");
				if (value && value !== "") {
					return nls.messaging.sslkeyrepository.dialog.passwordStashError;
				}
				return nls.messaging.sslkeyrepository.dialog.passwordStashMissingError;
			});
			
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
				this._sslKeyToUpload = null;
				this._passwordToUpload = null;
				this._savedValues = null;
				this.keyFile.value = '';
				this.passwordFile.value = '';
				dijit.byId(this.dialogId+"_form").reset();
			}));
		},
		
		clickKeyFileBrowseButton: function() {
		    this.keyFile.click();
		},
		
		clickPasswordFileBrowseButton: function() {
		    this.passwordFile.click();
		},

		keyFileChanged: function() {
            this.handleFileChangeEvent(this.keyFile.files, 'keyFile');
		},
		
		passwordFileChanged: function() {
		    this.handleFileChangeEvent(this.passwordFile.files, 'passwordFile');
		},
		
		/**
		 * Handle the onchange event that occurs when the user selects the file to upload
		 * @param files  The FileList containing the file to upload
		 * @param fileType The type of file we are uploading (Certificate or Key)
		 */
		handleFileChangeEvent: function(/*FileList*/files, /*String*/fileType) {
		    var field = this["field_"+fileType];
		    if (!field) {
		        console.debug("SSLKeyRepository.handleFileChangeEvent called with unrecognized fileType " + fileType);
		        return;
		    }
            if (files && files.length > 0 && files[0].name) {
                var filename = files[0].name;
                field.set("value", filename);  
                if (field.validate()) {
                    this._files[fileType] = files[0];
                }                   
            } else {
                console.debug("SSLKeyRepository.handleFileChangeEvent called without any files " + console.debug(JSON.stringify(files)));                
            }		    
		},						
		 		
		handleError: function(evt, errorMessage) {
			if (this.timeoutID) {
				window.clearTimeout(this.timeoutID);
			}
			if (!errorMessage) {
				errorMessage = nlsAppl.appliance.certificateProfiles.uploadTimeoutError; 
			}
			var message = evt && evt.message ? evt.message.replace(/\n/g, "<br />") : null;
			var code = evt && evt.code ? evt.code : null;	
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";					
			this.dialog.showErrorMessage(errorMessage, message, code);
		},

		
		show: function() {
			if (this.hideName) {
				this.field_Name.domNode.style.display = 'none';
			}
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			dijit.byId(this.dialogId+"_saveButton").set('disabled',true);
			this.dialog.show();   
		},

		save: function(onFinish) {
			if (!onFinish) { onFinish = function() {}; }
			
			this.uploadedFileCount = 0;
			
			if (this.timeoutID) {
				window.clearTimeout(this.timeoutID);
			}
			
			this._savedValues = {
				MQSSLKey: this._files.keyFile.name,	
				MQStashPassword: this._files.passwordFile.name
			};
			
			this.uploadComplete = onFinish;
			
			var _this = this;
			var adminEndpoint = ism.user.getAdminEndpoint();
			
			if (_this._files.keyFile && _this._files.passwordFile) {
				console.debug("uploading key database");
				adminEndpoint.uploadFile(_this._files.keyFile, function(data) {
	                _this.uploadedFileCount++;
	                console.debug("Key upload complete");
	                
	                var t = _this;
	    		    adminEndpoint.uploadFile(t._files.passwordFile, function(data) {
	                    t.uploadedFileCount++;
	                    console.debug("Stash password upload complete");
	                    window.clearTimeout(t.timeoutID);
	                    t.uploadComplete(t._savedValues);
	                }, function(error) {
	                    t.handleError(error, nls.appliance.certificateProfiles.uploadKeyError);                        
	                }); 
				});

			}
			// schedule timeout
			this.timeoutID = window.setTimeout(lang.hitch(this, function(){
				this.handleError(null, nlsAppl.appliance.certificateProfiles.uploadTimeoutError);
			}), this.uploadTimeout);
		}

	});

	var SSLKeyRepository = declare("ism.controller.content.SSLKeyRepository", _TemplatedContent, {

		nls: nls,
		templateString: template,
		
		uploadDialog: null,
		removeDialog: null,
		mqConnEnabled: undefined,
		mqConnRunning: undefined,
		sslURI: null,
		retry: true,  // to prevent multiple retries
		
		domainUid: 0,

		constructor: function(args) {			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			
		},

		postCreate: function() {
			this.inherited(arguments);
            
			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				if (data && data.MQConnectivity) {
					this.mqConnEnabled = data.MQConnectivity.Enabled;
					if (this.mqConnEnabled) {
						this.mqConnRunning = data.MQConnectivity.Status === "Active" ? true : false;
					}
				}
			}));
            
            this.domainUid = ism.user.getDomainUid();

			this.uploadURI += "/" + this.domainUid;
			this.sslURI = Utils.getBaseUri() + this.uploadURI;

			this.uploadButton.set('disabled', true);
			this.deleteButton.set('disabled', true);
			
			this._updateModified();
			this._initDialogs();
			this._initEvents();
		},

		_updateModified: function(onFinish, onError) {
			// summary:
			// 		get the initial records from the server
			var page = this;
		    var adminEndpoint = ism.user.getAdminEndpoint();
		    var promise = adminEndpoint.doRequest("/configuration/MQCertificate", adminEndpoint.getDefaultGetOptions());
		    promise.then(function(data){
		    	console.debug("ssl key rest GET returned", data);
		    	page.uploadButton.set('disabled', true);
				page.deleteButton.set('disabled', false);
				if (onFinish) {
					onFinish();
				}
		    },
		    function(error) {
		    	console.debug("error: "+error);
		    	if (onError) {
		    		onError(error);
		    	}
		    	page.uploadButton.set('disabled', false);
		    });
		},
		
		_completeUpload: function(values, dialog, onFinish, onError) {
			console.debug("Completing Upload... ");
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
		    var adminEndpoint = ism.user.getAdminEndpoint();
		    var options = adminEndpoint.createPostOptions("MQCertificate", values);
		    var promise = adminEndpoint.doRequest("/configuration", options);
		    promise.then(
	            function(data) {
			    	console.debug("upload POST returned ", JSON.stringify(data));
			    	page.uploadButton.set('disabled', true);
			    	page.deleteButton.set('disabled', false);
			    	page.keyFile = undefined;
			    	page.passwordFile = undefined;
			    	// Successful upload with GET and post an error 
			    	// if GET fails.
					page._updateModified(function() {
						if (onFinish) {
							onFinish();
						}
					}, function(error) {
						dialog.showErrorFromPromise(nls.messaging.sslkeyrepository.uploadFailed, error);
						if (onError) {
							onError(error);
						}
					});
	            },
			    function(error) {
			    	console.debug("error: "+error);
			    	page.keyFile = undefined;
			    	page.passwordFile = undefined;
					dialog.showErrorFromPromise(nls.messaging.sslkeyrepository.uploadFailed, error);
			    }
		    );
		},

		_deleteFiles: function(dialog, onFinish, onError) {
            console.debug("Deleting... ");
						
			var page = this;
		    var adminEndpoint = ism.user.getAdminEndpoint();
		    var promise = adminEndpoint.doRequest("/configuration/MQCertificate", adminEndpoint.getDefaultDeleteOptions());

		    promise.then(function(data){
			    	console.debug("ssl key repository rest DEL returned "+data);
					page.uploadButton.set('disabled', false);
					page.deleteButton.set('disabled', true);
					if (onFinish) {
						onFinish();
					}
			    },
			    function(error) {
			    	console.debug("error: "+error);
					dialog.showErrorFromPromise(nls.messaging.sslkeyrepository.deletingFailed, error);
					if (onError) {
						onError(error);
					}
			    });
		},

		_waitForRestart: function(t, dialog, bar, retry, retryCount) {
        	Utils.updateStatus();
			if (t.mqConnEnabled  && retryCount < 15) {
				if (t.mqConnRunning) {
					dialog.hide(); 
					bar.innerHTML = "";
					return;
				} else if (retry) {
					retryCount++;
					// Wait 2 more seconds and try again
	                setTimeout(function(){
	                    t._waitForRestart(t, dialog, bar, true, retryCount);
	                }, 2000);                   
				} 
			} else {
				console.debug("closing dialog before restart completes");
				dialog.hide(); 
				bar.innerHTML = "";				
			}
		},
		
		_initDialogs: function() {
			// create Add dialog
			var uploadTitle = nls.messaging.sslkeyrepository.dialog.uploadTitle;
			var uploadId = "upload"+this.id+"Dialog";
			domConstruct.place("<div id='"+uploadId+"'></div>", this.id);
			this.uploadDialog = new SSLKeyRepositoryAddDialog(
					             { dialogId: uploadId, 
					               dialogTitle: uploadTitle,
					               dialogInstr: "<div>" + nls.sslkeyrepositoryDialogUploadContent + "</div>",
					               uploadAction: this.sslURI },
					uploadId);
			this.uploadDialog.startup();
			
			// create Remove dialog (confirmation)
			domConstruct.place("<div id='remove"+this.id+"Dialog'></div>", this.id);
			var dialogId = "remove"+this.id+"Dialog";
			var delTitle = nls.messaging.sslkeyrepository.dialog.deleteTitle;
			this.removeDialog = new Dialog({
				id: dialogId,
				title: delTitle,
				content: "<div>" + nls.messaging.sslkeyrepository.dialog.deleteContent + "</div>",
				buttons: [
					new Button({
						id: dialogId + "_button_ok",
						label: nls.messaging.sslkeyrepository.dialog.deleteButton,
						onClick: function() { dojo.publish(dialogId + "_saveButton", ""); }
					})
				],
				closeButtonLabel: nls.messaging.sslkeyrepository.dialog.cancelButton
			}, dialogId);
			this.removeDialog.dialogId = dialogId;
		},
		
		_initEvents: function() {
			dojo.subscribe(this.uploadDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				Utils.updateStatus();
				// any message on this topic means that we clicked "save"
				console.debug("saving uploadDialog");

				var _this = this;
				if (dijit.byId("upload"+_this.id+"Dialog_form").validate()) {		
					var bar = query(".idxDialogActionBarLead",_this.uploadDialog.dialog.domNode)[0];
					var progressMsg = (_this.mqConnEnabled && _this.mqConnRunning) ? nls.savingRestartingProgress : nls.messaging.sslkeyrepository.dialog.savingProgress;
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + progressMsg;
					_this.uploadDialog.save(lang.hitch(_this, function(data) {
						console.debug("upload is done, files uploaded: ", JSON.stringify(data));
						_this._completeUpload(data, _this.uploadDialog.dialog, function() {
							if (_this.mqConnEnabled && _this.mqConnRunning) {
								var t = _this;
								console.debug("restart MQConn service");
								_this.mqConnRunning = false;
								Utils.restartMQConnectivity(function() { 
									// Wait before checking restart status
					                setTimeout(function(){
										t._waitForRestart(t, t.uploadDialog.dialog, bar, true, 0);
					                }, 2000);                   

								},
								function(error) {									
									console.debug("upload failed");
									bar.innerHTML = "";
								});
							} else {
								_this.uploadDialog.dialog.hide();
								bar.innerHTML = "";
							}
						}, function(error) {
							console.debug("upload failed");
							bar.innerHTML = "";
						});
					}));
				} else {
					console.debug("validation failed");
				}
			}));
			
			dojo.subscribe(this.removeDialog.dialogId + "_saveButton", lang.hitch(this, function(message) {
				Utils.updateStatus();
				var bar = query(".idxDialogActionBarLead",this.removeDialog.domNode)[0];
				var progressMsg = (this.mqConnEnabled && this.mqConnRunning) ? nls.deletingRestartingProgress : nls.messaging.sslkeyrepository.dialog.deletingProgress;
				bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + progressMsg;
				var _this = this;
				this._deleteFiles(this.removeDialog, function() {
					if (_this.mqConnEnabled && _this.mqConnRunning) {
						var t = _this;
						console.debug("restart MQConn service");
						_this.mqConnRunning = false;
						Utils.restartMQConnectivity(function() { 
							// Wait before checking restart status
			                setTimeout(function(){
								t._waitForRestart(t, t.removeDialog, bar, true, 0);
			                }, 2000);                   
						},
						function(error) {									
							console.debug("delete failed");
							bar.innerHTML = "";
						});
					} else {
						_this.removeDialog.hide();
						bar.innerHTML = "";
					}
				}, function() {
					console.debug("delete failed");
					bar.innerHTML = "";
				});
			}));
		},
		
		uploadSSLKey: function() {
			console.debug("Clicked Upload...");
			this.uploadDialog.show();
		},
		
		deleteSSLKey: function() {
			console.debug("Clicked Delete...");
			this.removeDialog.show();
		}
		
	});

	return SSLKeyRepository;
});
