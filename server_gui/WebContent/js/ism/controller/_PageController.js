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
		'dojo/_base/fx',
		'dojo/_base/config',
		'dojo/aspect',
		'dojo/dom',
		'dojo/dom-class',		
		'dojo/dom-construct',
		'dojo/dom-style',
		'dojo/fx',
		'dojo/on',
		'dojo/query',
		'dojo/Deferred',
		'dojo/when',
		'dojo/promise/tracer',
		'dijit/_base/manager',
		'dijit/form/Button',
		'dijit/layout/ContentPane',
		'dijit/layout/StackContainer',
		'dijit/form/Form',
		'dijit/Menu',
		'dijit/MenuItem',
		'dijit/registry',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'idx/app/Header',
		'idx/app/HighLevelTemplate',
		'idx/layout/MenuTabController',
		'dijit/MenuBar',
		'dijit/MenuBarItem',
		'ism/widgets/Dialog',
		'idx/widget/SingleMessage',
		'ism/config/Resources',
		'ism/widgets/User',
		'ism/widgets/TextBox',
		'ism/widgets/StatusControl',
		'ism/widgets/ServerControl',
		'ism/IsmUtils',
		'dojo/text!ism/controller/content/templates/ChangePasswordDialog.html',
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/appliance'
		], function(declare, lang, array, baseFx, config, aspect, dom, domClass, domConstruct, domStyle, fx, on, query, Deferred, when, tracer, 
				manager, Button, ContentPane, StackContainer, Form, Menu, MenuItem, registry, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin,
				Header, HighLevelTemplate, MenuTabController, MenuBar, MenuBarItem, Dialog, SingleMessage, Resources, User, TextBox,
				StatusControl, ServerControl, Utils, changePasswordDialog, nls, nlsapp) {

	var ChangePasswordDialog = declare("ism.controller.content._PageController.ChangePasswordDialog", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		//		Dialog for changing password
		nls: nls,
		templateString: changePasswordDialog,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.inherited(arguments);
		},
	
		postCreate: function() {
			// Apply changes if form is valid
			dojo.subscribe("changePasswordDialog_saveButton", lang.hitch(this, function(message) {
				this.saveButtonValidate = true;
				if (this.changepasswd_form.validate()) {
					var bar = query(".idxDialogActionBarLead",this.dialog.domNode)[0];
					bar.innerHTML = "<img src='css/images/loading_wheel.gif' width='10' height='10' alt=''/> " + nls.changePassword.dialog.savingProgress;
					if (this.save(this.dialog)) {
						this.dialog.hide();						
					} else {
						bar.innerHTML = "";
					}
				}
				this.saveButtonValidate = false;
			}));
			
			this.field_passwd1.runValidate = true;
			this.field_passwd2.runValidate = true;
			this.field_passwd1.hasInput = false;
			this.field_passwd2.hasInput = false;
			this.field_passwd1.forceFocusValidate = false;
			this.field_passwd2.forceFocusValidate = false;
			this.saveButtonValidate = false;
			
			// set validator to check that the passwords match
			this.field_passwd1.validator = lang.hitch(this,function() {
				var value = this.field_passwd1.get("value");
				if (value && value !== "") {
					this.field_passwd1.hasInput = true;
				}
				var p2Value = this.field_passwd2.get("value");
				if (p2Value && p2Value !== "" || this.saveButtonValidate || this.field_passwd1.forceFocusValidate){
					if (!value || value === ""){
						return false;
					}
					var passwordMatch = (p2Value == value);
					var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
					if (!spaces) {
						return false;
					}
					var valid = passwordMatch && spaces; 
					if (passwordMatch) {
						this.field_passwd1.runValidate = false;
						if (this.field_passwd2.runValidate) {
							dijit.byId("changePasswordDialog_passwd2").validate(true);
						}
						this.field_passwd2.runValidate = true;
						this.field_passwd1.runValidate = true;
					}
					if (this.field_passwd2.get("value") === "") {
						if (valid) {
							dijit.byId("changePasswordDialog_passwd2").validate(true);
						}
						var button = dijit.byId("changePasswordDialog_saveButton").set('disabled',true);
						valid = true;
					}
					return valid;
				}
				return !this.field_passwd2.hasInput;
			});
			this.field_passwd1.getErrorMessage = lang.hitch(this, function() {
				var value = this.field_passwd1.get("value");
				if (!value || value === "") {
					return nls.global.missingRequiredMessage;
				}
				var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
				if (!spaces) {
					return nlsapp.appliance.users.dialog.passwordNoSpaces;
				}
				return nlsapp.appliance.users.dialog.password2Invalid;
			});
			
			this.field_passwd2.validator = lang.hitch(this,function() {
				var value = this.field_passwd2.get("value");
				if (value && value !== "") {
					this.field_passwd2.hasInput = true;
				}
				var p1Value = this.field_passwd1.get("value");
				if (p1Value && p1Value !== "" || this.saveButtonValidate || this.field_passwd2.forceFocusValidate){
					if (!value || value === ""){
						return false;
					}
					var passwordMatch = (p1Value == value);
					var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
					if (!spaces) {
						return false;
					}
					var valid = passwordMatch && spaces; 
					if (passwordMatch) {
						this.field_passwd2.runValidate = false;
						if (this.field_passwd1.runValidate) {
							dijit.byId("changePasswordDialog_passwd1").validate(true);
						}
						this.field_passwd1.runValidate = true;
						this.field_passwd2.runValidate = true;
					}
					if (this.field_passwd1.get("value") === "") {
						if (valid) {
							dijit.byId("changePasswordDialog_passwd1").validate(true);
						}
						var button = dijit.byId("changePasswordDialog_saveButton").set('disabled',true);
						valid = true;
					}
					return valid;
				}
				return !this.field_passwd1.hasInput;
			});
			this.field_passwd2.getErrorMessage = lang.hitch(this, function() {
				var value = this.field_passwd2.get("value");
				if (!value || value === ""){
					return nls.global.missingRequiredMessage;
				}
				var spaces = new RegExp("^(?:|[^\\s]|[^\\s].*[^\\s])$").test(value);
				if (!spaces) {
					return nlsapp.appliance.users.dialog.passwordNoSpaces;
				}
				return nlsapp.appliance.users.dialog.password2Invalid;
			});
			
			// watch the form for changes between valid and invalid states
			this.changepasswd_form.watch('state', function(property, oldvalue, newvalue) {
				// enable/disable the Save button accordingly
				var button = dijit.byId("changePasswordDialog_saveButton");
				if (newvalue) {
					button.set('disabled',true);
				} else {
					button.set('disabled',false);
				}
			});
			
			on(this.field_passwd1, "focus", lang.hitch(this, function() {
				this.field_passwd1.forceFocusValidate = true;
			}));
			
			on(this.field_passwd2, "focus", lang.hitch(this, function() {
				this.field_passwd2.forceFocusValidate = true;
			}));
		},
		
		show: function() {
			console.debug(this.declaredClass + ".show()");
			query(".idxDialogActionBarLead",this.dialog.domNode)[0].innerHTML = "";
			this.field_currpasswd.set("value", "");
			this.field_passwd1.set("value", "");
			this.field_passwd2.set("value", "");
			this.field_passwd1.hasInput = false;
			this.field_passwd2.hasInput = false;
			this.field_passwd1.forceFocusValidate = false;
			this.field_passwd2.forceFocusValidate = false;
			dijit.byId("changepasswd_form").reset();
			dijit.byId("changePasswordDialog_saveButton").set('disabled',true);
			this.dialog.show();
		    this.field_currpasswd.focus();
		},
		
		// save the record
		save: function(dialog) {
			console.debug("Saving");
			var values = { id: ism.user.displayName, oldpassword: this.field_currpasswd.get("value"),
					password: this.field_passwd1.get("value") };
			return this._changePasswd(values,dialog);
		},

		_changePasswd: function(values,dialog) {
            console.debug("Password change... ");
			
			console.debug("data="+JSON.stringify(values));
			
			var page = this;
			var ret = false;
			dojo.xhrPut({
			    url: Utils.getBaseUri() + "rest/config/userregistry/webui/password",
			    handleAs: "json",
			    headers: { 
					"Content-Type": "application/json"
				},
				postData: JSON.stringify(values),
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.debug("passwd rest POST returned "+data);
			    	//console.debug("success");
			    	ret=true;
			    },
			    error: function(error, ioargs) {
			    	console.debug("error: "+error);
					Utils.checkXhrError(ioargs);					
					dialog.showXhrError(nls.changePassword.dialog.savingFailed, error, ioargs);
			    	ret=false;
			    }
			  });
			return ret;
		}
	});

	var PageController = declare("ism.controller._PageController", null, {
		// summary:
		// 		Base object for all page controllers.  Generates header, etc.
		
		name: "defaultPage",
		relType: "GA",

		passwordDialog: null,
		redirectPage: null,
		
		nodeName: "",
		windowTitle: nls.global.webUIMainTitle,
		tabTitle: "",
		pageTitle: "",
		
		status: null,
		lastServerStatus: null,
		pageDisabled: undefined,
		certErrorMessage: undefined,
		server: undefined,
		serverName: undefined,
		serverControl: null,
		
		changePasswordMenuItem: undefined,
		lastAccess: undefined,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			console.log("nodeName: " + this.nodeName);
			
			var originalXhr = dojo.xhr; 
			dojo.xhr = function(method,args,hasBody) {
			  args.headers = args.header || {"Content-Type": "application/json"};
			  args.headers["X-Session-Verify"] = dojo.cookie("xsrfToken");
			  return originalXhr(method,args,hasBody); // fire the standard XHR function
			};
			
			var vars = Utils.getUrlVars();
			if (vars["server"]) {
				this.server = decodeURIComponent(vars["server"]);
				if (vars["serverName"]) {
					this.serverName = decodeURIComponent(vars["serverName"]);					
				}
			}			
			
			this.init();
		},
		

		init: function() {
			// summary:
			// 		Extend to perform specific setup actions for the page
			
			console.debug(this.declaredClass + ".init()");
			
			// Create a new User object
			ism.user = new User({nodeName: this.nodeName, server: this.server, serverName: this.serverName});
			var from = document.referrer;
			if (!from) {
				from = "";
			}
			
			var _this = this;
			aspect.after(dojo, "xhrGet", function(){
				var now = Date.now();
				if (_this.lastAccess) {
					if (now - _this.lastAccess > 59999) {
						_this.lastAccess = now;
						ism.user.setLastAccess(now);
					}
				} else {
					dojo.publish("lastAccess", {requestName: "xhrRequest", requestTime: now});
				}
			});
			aspect.after(dojo, "xhrPut", function(){
				dojo.publish("lastAccess", {requestName: "xhrRequest", requestTime: Date.now()});
			});
			
			if (!ism.user.hasAuth()) {
				// Redirecting to the login page doesn't work if the login page is in the browser's cache.
				// When we do that, the XhrGet to get the auth info is redirected by Liberty to the
				// login page, and then Liberty sends the user to the referrer, which at that point is the
				// REST request to get the auth info, not what we want!
				//
				// Instead, force a reload of the page the user is requesting to let Liberty help us out
				// with authentication.
				
				//this.redirect(Resources.pages.login.uri);				
				window.location.reload(true);															
			} else if (!ism.user.isFirstServerConfigured() && this.name != Resources.pages.firstserver.name) {
				this.redirect(Resources.pages.firstserver.uri);
			// Saving the first server reinitializes the list of servers.  So assure we cannot get to this
			// page by mistake.
			} else if (ism.user.isFirstServerConfigured() && this.name == Resources.pages.firstserver.name) {
				this.redirect(Resources.pages.home.uri);
			} else if ((ism.user.isLicenseAccepted() === true) && (this.name == Resources.pages.licenseAccept.name)) {
				this.redirect(Resources.pages.home.uri);
			} else if (!ism.user.hasPermission(Resources.pages[this.name])) {
					// TODO replace this with an appropriate dialog box
					alert("You do not have permission to view this page: " + this._getPageName(this.name));
				this.redirect(Resources.pages.home.uri);
			}
			else {
				this.tabTitle = "";
				if (this.name != "defaultPage") {
					this.tabTitle = " - " +  this._getPageName(this.name);		
				}
				
				// first we should see if we need an sso token for a remote server
				var adminEndpoint = ism.user.getAdminEndpoint();
				if (adminEndpoint) {
				    // synchronous call to get or clear the sso cookie
				    if (adminEndpoint.sendCredentials) {
				        ism.user.getSSOToken(true); 
				    } else {
				        ism.user.clearSSOToken();
				    }
				}
				
				// kick off a get of the status
				var promise = undefined;
				var domainUid = ism.user.getDomainUid();
				if (ism.user.isFirstServerConfigured() && (this.name != Resources.pages.firstserver.name) && (this.name != Resources.pages.license.name)) {
				    promise = 	Utils.getStatus();			
				}
				
				// when the status result is back, handle it
				var pageReady = new Deferred();
				if (promise) {
					// Dummy status to disable menus until new status is available
					_this._enableDisableBasedOnStatus({RC: -1, harole: "UNKNOWN"}, false);
					promise.then(function(data) {
		                Utils.supplementStatus(data);
						data.harole = data.harole ? data.harole : "UNKNOWN";
						if(data && data.Server && data.Server.Name) {
							dojo.publish(Resources.contextChange.nodeNameTopic, data.Server.Name);
							if (data.Server.ErrorCode && data.Server.ErrorCode === 387) {
								ism.user.setLicenseAccepted(false);
							} else if (data.Server.ErrorCode !== undefined) {
								ism.user.setLicenseAccepted(true);
							}
						}
						if (_this.name.indexOf("license") < 0 && ism.user.isLicenseAccepted() !== true) {
							_this.redirect(Resources.pages.licenseAccept.uri);
						    pageReady.cancel();
						} else {
							_this._initLayout();
							_this._initWidgets();
							_this._initEvents();
							_this.lastStatusData = data;
							_this._enableDisableBasedOnStatus(data, false);
							_this.status.showStatus(data);						
						}
					}, function (error) {
						_this._initLayout();
						_this._initWidgets();
						_this._initEvents();
					    // check to see if it's a certificate issue
					    var message = Utils.checkForCertificateError(error);
					    if (message !== null) {
					        // wait for the page to be ready
					        pageReady.then(function() {
					            // make it async
					            window.setTimeout(function() {
					                // queue up the error message
					                _this.certErrorMessage = _this.showErrorMessage(nls.connectionFailure, message, null, true); }, 100);			                    					            
					        });
					    }
					});				
				} else {
					this._initLayout();
					this._initWidgets();
					this._initEvents();
				}
			}
			
			var pageReadyCallback = function () {
				// startup the HighLevelTemplate (top-level widget on page)
				var hlt = registry.byId("template");
				if (hlt) {
					hlt.startup();
					// the minwidth is cause scroll bar issues and it's hardcoded in HighLevelTemplate!
					domStyle.set(hlt.domNode, "minWidth", "0");
					if (window.innerWidth < 980) {
						window.setTimeout(lang.hitch(this, function(){hlt.resize();}), 100);					
					}
									
					// override HighLevelTemplate setting role to main for region center
				    var menuContainer = registry.byId("CustomStackContainer_0");
				    dijit.setWaiRole(menuContainer, "navigation");
				}
			    
			    pageReady.resolve();				
			};
			
			when(promise, pageReadyCallback, pageReadyCallback);
			
			/**
			 * IDX defect 14109 - Update recommended by IDX dev team.
			 * remove default handles
			 **/
			var menuController = dijit.byId("idx_layout_MenuTabController_0");
			if (menuController) {
				var kids = menuController.getChildren();
				for (var i=0, len=kids.length; i < len; i++) {
				    var tab = kids[i];
				    if(tab.page.id in menuController._handles){
				        var handles=menuController._handles[tab.page.id];
				        handles.forEach(function(handle){
				            handle.remove();
				        });
				    }
				}
			}
		},
		
		_getPageName: function(page) {
		    if (nls.global[page]) {
		        return nls.global[page];
		    }
		    return nls[page];
		},

		_initLayout: function() {
			// summary:
			// 		Defines the base layout for any application page.
			// description: 
			// 		The _PageController will generate the page header (title, menu, links, etc.), 
			// 		and leave the page content specification to extending widgets.
			
			console.debug(this.declaredClass + "._initLayout()");

			var controller = this;

			var template = new HighLevelTemplate({ style: "width: 100%; height: 100%; "}, "template");

			// define the user actions menu (accessed by clicking on username message)
			var userActions = new Menu();
			aspect.after(userActions,"onOpen", function() {
				dojo.publish("lastAccess", {requestName: "userActions.onOpen"});
			});
			var item = new MenuItem({id: "userActions_logout", label: nls.global.logout, title: "" });
			item.onClick = function() {
				ism.user.logout();
			};
			userActions.addChild(item);
			
			// don't show change password on license page
			if (this.name != Resources.pages.license.name) {
				var passwdItem = new MenuItem({id: "userActions_changePassword",  label: nls.global.changePassword, title: "" });
				passwdItem.onClick = lang.hitch(this,function() {
					if (!this.passwordDialog) {
						var changePasswdDialogId = "changePasswordDialog";
						this.passwordDialog = new ChangePasswordDialog({dialogId: changePasswdDialogId},changePasswdDialogId);
					}
					this.passwordDialog.show();
				    dojo.publish("lastAccess", {requestName: "passwdItem.onClick"});
				});
				userActions.addChild(passwdItem);
				this.changePasswordMenuItem = passwdItem;
			}
			
			var user = {
				displayName: ism.user.displayName,
				displayImage: ism.user.displayImage,
				messageName: function() { return ism.user.getHeaderMessage(); },
				actions: userActions
			};

			// define the help menu
			var helpMenu = new Menu();
			helpMenu.addChild(new MenuItem({
				id: "helpMenu_help",
				label: nls.helpMenu.help, title: "", 
				onClick: function() { window.open(config.infocenterURL,"infocenter"); }
			}));
			aspect.after(helpMenu,"onOpen", function() {
				dojo.publish("lastAccess", {requestName: "helpMenu.onOpen"});
			});
			
			// Add Restore Tasks item if this user has permission to run it and it's not the license page
			if (this.name != Resources.pages.license.name && ism.user.hasPermission(Resources.restoreTasks)) {
				helpMenu.addChild(new MenuItem({
					id: "helpMenu_restoreTasks",
					label: nls.helpMenu.homeTasks, title: "",
					onClick: function() { 
						ism.user.setHomeTasksVisible(true);
						controller.redirect(Resources.pages.home.uri);
					}
				}));
			}
			
			// Add About dialog for everyone
			helpMenu.addChild(new MenuItem({
				id: "helpMenu_about",
				label: nls.helpMenu.about.linkTitle, title: "",
				onClick: lang.hitch(this, function() { 
					this._showAboutDialog();
				}) 
			}));

			declare("CustomStackContainer",StackContainer, {
				selectChild: function(page, animate) {
					return this.inherited(arguments);
				}
			});
			
			// create a StackContainer which will display the page links in the header
			var stackContainer = new CustomStackContainer({ region: "center", "aria-label": nls.global.menuContent, style: "display: none"});
			var getNavMenuByName = function(name) {
				var actions = controller._getPageActions(name);
				if (Object.keys(actions).length <= 1) {
					return null;
				}
				else {
					var menu = new Menu();
					for (var key in actions) {
						if (ism.user.hasPermission(actions[key])) {
							var msg = { topic: actions[key].topic, page: name, subpage: key };
							
							// don't show items with excludeFromMenu flag;
							if (actions[key].excludeFromMenu !== true) {
								if (name == controller.name) {
									// if we are navigating to a sub-page of the current page, publish
									// a message with the new sub-page in order to trigger the fade-in mechanism
									menu.addChild(new MenuItem({ 
										id: "navMenuItem_" + actions[key].topic,
										label: actions[key].name, title: "", 
										onClick: lang.hitch(msg, function() {
											dojo.publish(this.topic, { page: this.subpage });
										    dojo.publish("lastAccess",{requestName: this.subpage+".onClick"});
										})
									}));
								} else {
									// if we are navigating to a different page completely, redirect and pass
									// the new sub-page in through the URL
									menu.addChild(new MenuItem({
										id: "navMenuItem_" + actions[key].topic,
										label: actions[key].name, title: "", 
										onClick: lang.hitch(msg, function() {
											var params = "?nav=" + this.subpage;
											if (ism.user.server) {
												params += "&server=" + encodeURIComponent(ism.user.server) + 
													"&serverName=" + encodeURIComponent(ism.user.serverName);
											}
											controller.redirect(Resources.pages[this.page].uri + params);
										})
									}));
								}
							}
						}
					}
					// when we close a menu, re-select the menu tab for the current page after a short time... IF we aren't 
					//   selecting another menu tab already
					aspect.after(menu, "onClose", function() {
						controller._tabMenuOpen = false;
						setTimeout(function() {
							if (!controller._tabMenuOpen) {
								dijit.byId("CustomStackContainer_0").selectChild(dijit.byId("navMenu_" + controller.name));
							}
						}, 500);
					});
					aspect.after(menu, "onOpen", function() {
						controller._tabMenuOpen = true;
					});
					return menu;
				}
			};
			var getContentPaneByName = function(name) {
				var menu = getNavMenuByName(name);
				if (menu) {
					var pane = new ContentPane({ id: "navMenu_" + name, title: controller._getPageName(name), lookupName: name, selected: name == controller.name, popup: menu, alwaysShowMenu: true });
					return pane;
				} else {
					return new ContentPane({ id: "navMenu_" + name, title:  controller._getPageName(name), lookupName: name, selected: name == controller.name });
				}
			};
			
			// capture a link selection (via the default published message) and redirect to the associated URI
			// for links that do not have a sub-menu popup
			stackContainer.subscribe(stackContainer.id + "-selectChild", lang.hitch(controller, function(page) {
				if (Object.keys(this._getPageActions(page.lookupName)).length <= 1 && 
						page.lookupName != controller.name) {
					// this page doesn't have sub-pages, so go straight there
					var uri = Resources.pages[page.lookupName].uri;
					if (ism.user.server) {
					    var server = encodeURIComponent(ism.user.server);
						uri += (uri.indexOf("?") < 0) ? "?server="+server : "&server="+server;
						uri += "&serverName=" + encodeURIComponent(ism.user.serverName);
					}
					this.redirect(uri);
				} 
				// we don't do anything on clicks to the top of a menu... only on sub-menu clicks
			}));
			
			for (var key in Resources.pages) {
				var showPage = true;
				showPage = showPage && ism.user.hasPermission(Resources.pages[key]);
				showPage = showPage && (key.indexOf("license") < 0);
				showPage = showPage && (key != Resources.pages.firstserver.name);
				if (showPage) {
					stackContainer.addChild(getContentPaneByName(key));
				}
			}

			// Don't show secondary tabs on License page or on FirstServer page
			var stackId;
			if ((this.name.indexOf("license") < 0) && (this.name != Resources.pages.firstserver.name)) {
				stackId = stackContainer.id;
			}
			
			// Don't show status or server menu on License page or on FirstServer page
			var topNavMenu;
			if ((this.name === Resources.pages.licenseAccept.name) || ((this.name != Resources.pages.license.name) && (this.name != Resources.pages.firstserver.name))) {
			    topNavMenu = new MenuBar();
				this.status = new StatusControl({ isAdmin: ism.user.hasPermission(Resources.pages.appliance)});
				topNavMenu.addChild(this.status.startMonitoring());

   				this.serverControl = new ServerControl();
   				topNavMenu.addChild(this.serverControl.get("serverMenuBarItem"));
   				this.serverControl.startMonitoring();
			}
								
			this.header = new Header({
				region: "top",
				primaryTitle: this.getPrimaryTitle(),
				primaryBannerType: "thin",
				user: user,
				secondaryBannerType: "blue",
				contentTabsInline: true,
				layoutType: "variable",
				help: helpMenu,
				contentContainer: stackId,
				navigation: topNavMenu
			}, "header");

			if ((this.name.indexOf("license") < 0) && (this.name != Resources.pages.firstserver.name)) {
				this.header.domNode.style.height = "86px"; // required to give Header a non-zero height (except for License or FirstServer page)
			}
						
			dijit.setWaiRole(this.header,"banner"); // set role for accessibility
			// Set this min width for the banner so that the user actions/help menus do not
			// overlap the server/status menues when the width of the window is made smaller.
			this.header.domNode.style.minWidth = "850px";
			template.addChild(stackContainer);
			new ContentPane({ region: "center" }, "main");
		},
		
		getPrimaryTitle: function () {
			var primaryTitle = nls.global.productNameTM;
			/* don't show the node name when managing a remote server, it's confusing			
			if (this.nodeName !== "" && !ism.user.isRemote()) {
				var fullText = nls.global.node + ": " + Utils.nameDecorator(this.nodeName);
				var text = fullText;
				if (text.length > 40) {
					text = text.substring(0, 40) + "...";
				}
				primaryTitle += "<span class='primaryTitleNodeName' title='"+fullText+"'>" + text + "</span>";
			}*/
			return primaryTitle;
		},
		
		updateNodeName: function (newName) {
			if (newName === null || newName === undefined) {
				newName = "";
			}
			this.nodeName = newName;					
			this._setWindowTitle();
			//this.header.set('primaryTitle', this.getPrimaryTitle());			
		},
		
		_setWindowTitle: function() {
			var title = "";
			title += this.windowTitle + this.tabTitle + this.pageTitle;
			window.document.title = title;			
		},

		_initWidgets: function() {
			// summary:
			// 		Extend to define widgets used on this page
			
			console.debug(this.declaredClass + "._initWidgets()");

			var mainContent = new StackContainer({}, "mainContent");

			var actions = this._getPageActions();
			console.log("ACTIONS ARE:");
			console.dir(actions);
			for (var key in actions) {
				if (ism.user.hasPermission(actions[key])) {
					mainContent.addChild(new ContentPane({ id: key, "aria-label":actions[key].name , preload: false, loadingMessage: "", href: actions[key].uri }));
				}
			}

			// hide the loading message
			var results = query(".loadingMessage", "loading");
			if (results.length > 0) {
				domStyle.set(results[0], "display", "none");
			}
			
			// load the proper sub-page, based on URL parameter
			this.pageTitle = "";
			if (this.nav) {
				mainContent.selectChild(this.nav);
				this.pageTitle = " - " + this._getMenuLabel(this.name,this.nav);
				if (this.name === "license") {
					this.subPane = "";
					ism.user.setLicenseType(this.nav);
				}
				// record page visit, this.name is the tab, this.nav is the page				
				ism.user.recordVisit(this.name, this.nav, this.subPane);
			}  else if (this.name) {
				//  record page visit for home, license, first server
				ism.user.recordVisit(this.name);
			}
			
			this._setWindowTitle();			
		},
		
		_getMenuLabel: function(menu, item) {
		    if (nls.globalMenuBar[menu] && nls.globalMenuBar[menu][item]) {		    
		        return nls.globalMenuBar[menu][item];
		    }
		    if (Resources.pages[menu] && Resources.pages[menu].nav[item]) {
		        console.debug("_getMenuLabel("+menu+", "+item+"): " + Resources.pages[menu].nav[item].name);
		        return Resources.pages[menu].nav[item].name;
		    }
		    return undefined;
		},

		_initEvents: function() {
			// summary:
			// 		Extend to define connections, events, etc. for component interaction
			
			console.debug(this.declaredClass + "._initEvents()");

			var actions = this._getPageActions();
			for (var key in actions) {
				dojo.subscribe(actions[key].topic, lang.hitch(this, function(data) {
					this._switchPane(data);
				}));
			}

			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				this._handleServerStatus(data);
			}));

			// subscribe to error notifications
			dojo.subscribe(Resources.errorhandling.errorMessageTopic, lang.hitch(this, function(data) {
				// ignore errors if we already know the page is disabled
			    if (this.pageDisabled === undefined && this.lastStatusData) {
			        this._enableDisableBasedOnStatus(this.lastStatusData, true);
			    }
				if (!this.pageDisabled || data.sticky) {
					this.showErrorMessage(data.title, data.message, data.code, data.sticky);					
				}
			}));

			// subscribe to warning notifications
			dojo.subscribe(Resources.errorhandling.warningMessageTopic, lang.hitch(this, function(data) {
				var collapsed = data.collapsed === undefined ? true : data.collapsed;
				if (this.name.indexOf("license") < 0) {
					this.showWarningMessage(data.title, data.message, data.code, collapsed);
				}
			}));
			
			// subscribe to Information notifications
			dojo.subscribe(Resources.errorhandling.infoMessageTopic, lang.hitch(this, function(data) {
				this.showInfoMessage(data.title, data.message, data.code);
			}));

			dojo.subscribe("lastAccess", lang.hitch(this, function(data) {
        		var now = data.requestTime === undefined ? Date.now() : data.requestTime;
        		if (!this.lastAccess) {
        			this.lastAccess = ism.user.getLastAccess();
        		}
        		// If the last access was less than 60 seconds ago, do not add the overhead
        		// of another Liberty xhr request and don't bother to update the last access time.
        		if (now - this.lastAccess < 60000) {
        			return;
        		}
        		// If a Liberty request has been made, update the last access time and return.
				if (data.requestName === "xhrRequest") {
        			console.debug("Updating last access time at " + now/1000);
					this.lastAccess = now;
					ism.user.setLastAccess(now);
					return;
				}
				var _this = this;
				// Force an xhr request.  This will either tell Liberty that the httpSession is still
				// active or it will force a timeout if the httpSession invalidation timeout has
				// already been reached.
		        dojo.xhrGet({
	        	    headers: {
						"Content-Type": "application/json"
					},
				    url: Utils.getBaseUri() + "rest/config/webui/preferences/liberty/" + encodeURIComponent('httpSession_invalidationTimeout'),
				    handleAs: "json",
				    preventCache: true,
	                load: function(data) {
                		_this.lastAccess = Date.now();
            			console.debug("Updating last access time at " + _this.lastAccess/1000);
            			ism.user.setLastAccess(_this.lastAccess);
	                },
	                error: function(error,ioargs) {
						Utils.checkXhrError(ioargs);
						Utils.showPageError(nlsapp.appliance.webUISecurity.timeout.getInactivityError, error, ioargs);
					}
			    }); 			
			}));

			// In the header menu, we want to bind clicks on a navigation tab to the MenuTabController 
			// the showPagePopup function. If the menu controller has already been created we can 
			// configure it now, otherwise we use an aspect to configure it once the header has been
			// rendered. We assume the header is built first, so will have a consistent dijit ID.
			var menuController = dijit.byId("idx_layout_MenuTabController_0");
			if (menuController) {
				this._configureMenuHandling(menuController);
			} else {
				aspect.after(this.header, "_renderContent", lang.hitch(this,function() {
					this._configureMenuHandling(dijit.byId("idx_layout_MenuTabController_0"));
				}));
			}
			tracer.on("resolved", function(response, data) {
				data.t.handleResolved(response, data.args);
			});
			tracer.on("rejected", function(response, data) {
				data.t.handleRejected(response, data.args);
			});
		},

		_handleServerStatus: function(data) {
			var statusRC = data.RC;
			
			var haRole = data.harole;
			console.debug(this.declaredClass + "._handleServerStatus("+statusRC+", " +haRole+")");
			
			if (this.lastServerStatus != statusRC || this.lastHaRole != haRole) {			    
				this.lastServerStatus = statusRC;
				this.lastHaRole = haRole;
				this.lastStatusData = data;
				console.debug("Server status changed");
				// clear the certificate error?
				if (this.certErrorMessage && statusRC !== -1) {
				    this.certErrorMessage.destroy();
				    this.certErrorMessage = undefined;
				}
				this._enableDisableBasedOnStatus(data, true);
			}
			
			if ((statusRC === 9) && (data.Server.ErrorCode === 387) && (this.name !== Resources.pages.licenseAccept.name)) {
				this.redirect(Resources.pages.licenseAccept.uri);
			}
		},
		
		_enableDisableBasedOnStatus: function(data, showMessage) {
			var currentPage = undefined;
			var mainContent = dijit.byId("mainContent"); 
			if (mainContent && mainContent.selectedChildWidget) {
				currentPage = mainContent.selectedChildWidget.id; 
			}
			console.debug("currentPage="+currentPage);
			
			// update enablement of menu items according to status
			for (var page in Resources.pages) {
				var actions = this._getPageActions(page);
				for (var key in actions) {
					//console.debug("key="+key);
					//if (actions[key].availability) {
					var nowDisabled = Utils.isItemDisabled(actions[key], data);
					// if showMessage, then mark current page as disabled or enabled and show the page not available message
					if (showMessage && key == currentPage) {					
						if (nowDisabled.disabled && !this.pageDisabled) {
							console.debug("Current page is now disabled");
							this._closeMessages(true);
							var collapsed = nowDisabled.collapsed === undefined ? true : nowDisabled.collapsed;
							dojo.publish(Resources.errorhandling.warningMessageTopic, 
							        { title: nls.global.pageNotAvailable, message: nowDisabled.message, collapsed: collapsed });
						} else if (!nowDisabled.disabled && this.pageDisabled) {										
							console.debug("Current page is now enabled");
                            this._closeMessages(false);
						}
					    this.pageDisabled = nowDisabled.disabled;
					}
					// mark the menu item as disabled or enabled, if there is one
					var menuItem = registry.byId("navMenuItem_" + actions[key].topic);
					if (menuItem) {
						if (nowDisabled.disabled) {
							menuItem.set("disabled",true);
							menuItem.set("title", nls.global.pageNotAvailable);
							domClass.add(menuItem.domNode,"ismDisabledMenuItem");
						} else {
							menuItem.set("disabled",false);
							menuItem.set("title","");
							domClass.remove(menuItem.domNode,"ismDisabledMenuItem");
						}
					}
				}
			}			
						
		},
		
		_closeMessages: function(excludeSticky) {
            // close any displayed error and warning messages
            query(".idxSingleMessage","mainContent").forEach(function(node) {
                var message = dijit.byId(node.id);
                if (!excludeSticky || !message.sticky) {
                    message.destroy();
                } else {
                    console.debug("found a sticky message: " + message.get("title"));
                }      
            });
		},
		
		_configureMenuTabHandling: function(tab) {
			// summary:
			//		Binds clicks on a navigation tab to the newly exposed showPagePopup
            //      (showPagePopup was exposed in fix for IDX defect 14109).			
			// IDX defect 14109 - update suggested by IDX dev team
			on(tab.titleNode, "click", function() {
			    var mc = registry.byId("idx_layout_MenuTabController_0");
			    mc.showPagePopup(tab.page.id);
			    dojo.publish("lastAccess", {requestName: tab.page.id+".click"});
			});
		},
		
		_configureMenuHandling: function(menuController) {
			// summary:
			//		Configures the handling of sub-pages for the given menuController. It processes any existing
			//		sub-pages then uses an aspect to handle any sub-pages that might be added later.
			if (menuController) {
				var kids = menuController.getChildren();
				for (var i=0, len=kids.length; i < len; i++) {
					this._configureMenuTabHandling(kids[i]);
				}
				aspect.after(menuController, "addChild", lang.hitch(this,function(tab) {
					this._configureMenuTabHandling(tab);
				}),true);
			}
		},
		
		redirect: function(/*String*/uri) {
			// summary:
			// 		Redirect the browser to the specified uri
			// uri: String
			// 		The URI to redirect to
			Utils.reloading = true;
			window.location = uri;
		},

		_getPageActions: function(name) {
			// summary:
			// 		Return all actions for the current page
			if (!name) {
				name = this.name;
			}
			return Resources.pages[name].nav;
		},

		_switchPane: function(data) {
			// summary:
			// 		Switch sub-pages with animation.
			var newChildId = data.page;
			if (newChildId) {
				this.nav = newChildId;
			}
			console.debug(this.declaredClass + "._switchPane("+newChildId+")");			

			var controller = this;

			var mainContent = dijit.byId("mainContent");
			if (newChildId == mainContent.selectedChildWidget.id) {
				console.debug("switching to current page, nothing to do");
				return;
			}
			
			var outAnim = baseFx.fadeOut({ duration: 400, node: mainContent.selectedChildWidget.id });
			var _this=this;
			on(outAnim, "End", function() {
			    _this._closeMessages();
				mainContent.selectChild(newChildId);
				controller._updateSelected(data.urlParams);
			});
			var inAnim = baseFx.fadeIn({ duration: 400, node: mainContent.selectedChildWidget.id });
			on(inAnim, "End", function() {
				// publish message that page is shown
				dojo.publish(Resources.pages[controller.name].nav[newChildId].topic + ":onShow", { page: this.subpage });
			});
			fx.chain([
				outAnim, inAnim
			]).play();
		},

		_updateSelected: function(urlParams) {
			// summary:
			// 		Change the browser URL to provide a static link to the selected sub-page.
			console.debug(this.declaredClass + "._updateSelected()");
			var selected = dijit.byId("mainContent").selectedChildWidget.id;
			var params = "?nav=" + selected;
			if (urlParams) {
				params += "&";
				params += urlParams;
			}
			if (ism.user.server) {
				params += "&server=" +  encodeURIComponent(ism.user.server) + "&serverName=" + encodeURIComponent(ism.user.serverName);
			}
			if (window.history.pushState) {
				window.history.pushState("", "", window.location.pathname + params);
				// record page visit, this.name is tab, selected is page
				ism.user.recordVisit(this.name, this.nav);
			} else {
				// IE... we need to completely reload the page with the new URL
				window.location(window.location.pathname + params);		
			}
			// set the window title appropriately
			this.pageTitle = " - " + this._getMenuLabel(this.name,selected);
			this._setWindowTitle();
			
			// make sure the disabled state is correct
			if (this.lastStatusData) {
				var nowDisabled = Utils.isItemDisabled(Resources.pages[this.name].nav[selected], this.lastStatusData);
				if (nowDisabled.disabled) {
					console.debug("Current page is now disabled");
					this.pageDisabled = true;
					dojo.publish(Resources.errorhandling.warningMessageTopic, { title: nls.global.pageNotAvailable,
						message: nowDisabled.message });								
				} else {
					console.debug("Current page '" + Resources.pages[this.name].nav[selected].topic + "' is now enabled");
					this.pageDisabled = false;
				}
			}
		},

		closePage: function(key) {
			console.debug(this.declaredClass + ".closePage("+key+")");
		},				
		
		showErrorMessage: function(title, message, code, sticky) {
		    return this._showMessage({
				type: 'error', 
				title: title,
				description: message ? message : "", 
				messageId: code ? code : "",
				sticky: sticky,
				collapsed: !sticky});
		},
		
		showWarningMessage: function(title, message, code, collapsed) {
			return this._showMessage({
				type: 'warning', 
				title: title,
				description: message ? message : "", 
				messageId: code ? code : "",
				collapsed: collapsed });
		},
		
		showInfoMessage: function(title, message, code) {
			return this._showMessage({
				type: 'information', 
				title: title,
				description: message ? message : "", 
				messageId: code ? code : "",
				showId: code ? true : false });
		},
		
		_showMessage: function(options) {			
			if (!options) {
				options = {};
			}

			var attachPoint = "mainContent";
			if (this.nav) {
				attachPoint = this.nav;
			} else if (this.name) {
				attachPoint = this.name;
			}
			var messageDiv = domConstruct.create("div", null, attachPoint, "first");
			options.dateFormat = options.dateFormat ? options.dateFormat : {selector: 'time', timePattern: ism.user.localeInfo.timeFormatString};
			options.showRefresh = false;
			options.showAction = false;
			options.showDetailsLink	= false;	
			var singleMessage = new SingleMessage(options, messageDiv);
			singleMessage.sticky = options.sticky;
			// scroll to top of attachPoint
			var node = dom.byId(attachPoint);
			if (node) {
				node.scrollTop = 0;
			}
			return singleMessage;
		},
			
		_showAboutDialog: function() {
			var aboutDialogId = "aboutDialog";
			var dialog = manager.byId(aboutDialogId);
			if (!dialog) {
			    var aboutIconClass = "aboutIcon";
			    var dev_uri = lang.replace(nls.helpMenu.about.viewLicense, [nls.licenseType_Devlopers]);
			    var nonprod_uri = lang.replace(nls.helpMenu.about.viewLicense, [nls.licenseType_NonProd]);
			    var prod_uri = lang.replace(nls.helpMenu.about.viewLicense, [nls.licenseType_Prod]);
			    var contentStr = undefined;
			    if(this.relType == "BETA")
			    	contentStr = "<div style='width: 680px; height: 570px;'><div class='"+aboutIconClass+"' title='" + 
					nls.helpMenu.about.iconTitle +"'></div>" + "<p>Build: __buildLabel__</p>" +
					"<p><a href='"+ Resources.pages.license.uri +"' target='licensePage'>Show License Agreement</a></p></div>";
			    else
			    	contentStr = "<div style='width: 680px; height: 570px;'><div class='"+aboutIconClass+"' title='" + 
					nls.helpMenu.about.iconTitle +"'></div>" + "<p>Build: __buildLabel__</p>" +
					"<p><a href='"+ Resources.pages.license.uri +"?nav=Developers' target='licensePage'>" +
					dev_uri + "</a></p>" +
					"<p><a href='"+ Resources.pages.license.uri +"?nav=NonProduction' target='licensePage'>" +
					nonprod_uri + "</a></p>" +
					"<p><a href='"+ Resources.pages.license.uri +"?nav=Production' target='licensePage'>" +
					prod_uri + "</a></p></div>";
				dialog = new Dialog({
					id: aboutDialogId,
					title: nls.helpMenu.about.dialogTitle,
					style: "width: 700px; height: 730px; max-height:100%;",
					content: contentStr
				},aboutDialogId);
			}
			dialog.show();
		}
	});

	return PageController;
});
