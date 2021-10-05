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
		'dojo/_base/array',
		'dojo/_base/connect',
		'dojo/aspect',
		'dojo/cookie',
		'ism/config/Resources',
		'ism/widgets/License',
		'ism/IsmUtils',
		'dojo/_base/lang',		
		'dojo/i18n!ism/nls/strings',
		'dojo/i18n!ism/nls/appliance',
		'ism/widgets/Preferences',
		'ism/widgets/AdminEndpoint'
		], function(declare, array, connect, aspect, cookie, Resources, License, Utils, lang, nls, nlsa, Preferences, AdminEndpoint) {

	var User = declare("ism.widgets.User", null, {
		// summary:
		// 		A representation of an ISM User
		// description: 
		// 		This widget will manage a user's interaction with the admin UI, including
		// 		permission checks for page access, displaying messages related to the user, etc.

		STATUS_LICENSE_ACCEPTED: 5,
		
		displayName: "defaultUser",
		displayImage: null,		
		roles: [],
		
		Resources: Resources,

		_needsLogin: true,

		_acceptedLicense: undefined,
		
		_virtual: undefined,

		_cci: false,
		
		_docker: false,  // are we running in a docker container?
		
		_homeTasksVisible: true,
		
		_hasAuth: true,
		
		_prefs: null,
		
		_prefsData: {},
		
		columnWidthDefaults: {},

		defaultAdminServerAddress: undefined,
        
        defaultAdminServerPort: undefined,
        
        server: undefined,
        
		licenseType: undefined,
        
        serverName: undefined,
        clusterName: undefined,
//        clusterStatus: undefined,
        lastAccess: undefined,
        sessionTimeout: undefined,
        checkSessionTimeout: undefined,
        
        adminEndpoint: undefined,
		
		localeInfo: { 
			localeString: "en", 
			dateTimeFormatString:"MMMM d, yyyy h:mm:ss a", 
			dateFormatString:"MMMM d, yyyy",
			timeFormatString: "h:mm:ss a"
		},
		
		constructor: function(args) {
			// summary:
			// 		Constructor.
			
			dojo.safeMixin(this, args);
			
			var auth = this._getUserAuth();
			
			if (auth === undefined) {
				// Redirect to login page if no auth record found. (User is using cache when not logged in.)
				console.debug("User has no auth record.");
				this._hasAuth = false; 
			}
			else {
				this.setLastAccess(Date.now());
				this.initSessionTimeout();
				this.displayName = auth.username.replace("\\\\","");
				if (this.displayName.length > 33) {
					this.displayName = this.displayName.substr(0, 30) + "...";
				}
				
				this.roles = auth.userRoles;
				
				if (!this._prefs) {
					this._prefs = new Preferences({userId: auth.username, blocking: true});
				}
				
				this._prefsData = this._prefs.getPreferences();
				if (!this._prefsData.global) {
					this._prefsData.global = {};
				}
				
				if (!this._prefsData.user) {
					this._prefsData.user = {};
				} else if (this._prefsData.user.homeTasksHidden) {
					console.debug("Home page tasks are hidden");
					this._homeTasksVisible = false;				
				}	
				
				var servers = this.getServerList();				
				if (servers.length === 0) {
				    var name,address,port,uid;
				    if (this.serverName && this.serverName.length > 0) {
				        name = this.serverName;
				    } 

				    var colon = this.server ? this.server.indexOf(":") : -1;
				    if (colon > 0) {
				        uid = this.server;
				        address = this.server.substring(0, colon);
				        port = this.server.substring(colon+1);
				    } else {
				        address = this.defaultAdminServerAddress ? this.defaultAdminServerAddress : undefined;
				        port = this.defaultAdminServerPort ? this.defaultAdminServerPort : undefined;                        
				        uid = address.indexOf(":") < 0 ? address : "[" + address +"]";
				        uid += ":" + port;
				    }
				    if (address && port && uid) { 
				        name = name ? name : uid;
				        servers.push({uid: uid, name: name, adminServerAddress: address, adminServerPort: port});
				        this.server = uid;
				        this.serverName = name;
				        this.setServerList(servers);
				    }
				} else if (!this.server) {
				    // if server not defined, use the last one visited
				    var recent = this.getRecentServerUids();
				    if (recent.length > 0) {
				        this.server = recent[0];
				        var lastServer = this.getServer(this.server);
				        if (lastServer && lastServer.name) {
				            this.serverName = lastServer.name;
				        } else {
				            this.serverName = this.server;
				        }
				    }
				}
				this.updateRecentServerUids(this.server);
				
				if (this.server && !this.clusterName) {
					var srv = this.getServer(this.server);
					this.clusterName = srv.clusterName;
				}
							
				console.debug(this.declaredClass+" setting _acceptedLicense to "+this._acceptedLicense);
			}
		},
		
		initSessionTimeout: function() {
			var _this = this;
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
                		_this.setSessionTimeout(timeout);
            	        _this.checkTimeout();
                	}
                },
                error: function(error,ioargs) {
					Utils.checkXhrError(ioargs);
					Utils.showPageError(nlsa.appliance.webUISecurity.timeout.getInactivityError, error, ioargs);
				}
			});
		},
		
		setSessionTimeout: function(newvalue) {
			if (newvalue === undefined) {
				return;
			}
			this.sessionTimeout = (newvalue * 1000) + 60000;
	        if (this.checkSessionTimeout === undefined) {
		        this.checkTimeout();
	        }
		},
		
		getSessionTimeout: function() {
			return this.sessionTimeout;
		},
		
		checkTimeout: function() {
			var _this = this;
			// Check every two minutes to see whether we've reached the session timeout
			this.checkSessionTimeout = setTimeout(function() {
				if(Date.now() - _this.getLastAccess() > _this.getSessionTimeout()) {
					_this.initSessionTimeout();
				} else {
					_this.checkTimeout();
				}
			}, 120000);
		},

		getLastAccess: function() {
			return this.lastAccess;
		},
		
		setLastAccess: function(lastAccess) {
			this.lastAccess = lastAccess;
		},

		getHeaderMessage: function() {
			// summary:
			// 		Get message to display in header.
			return lang.replace(nls.global.greeting,[this.displayName]);
		},

		logout: function() {
			// summary:
			// 		log out and redirect to login page
			console.log(this.declaredClass + ".logout()");
			Utils.reloading = true;
			dojo.xhrPost({
				url:"ibm_security_logout",
				load: function(){
					window.location = Resources.pages.login.uri;
				},
				error: function() {
					var uniqueUrl = Resources.pages.login.uri+"?ts="+(new Date()).getTime();
					window.location = uniqueUrl;
				}
			});
		},

		hasPermission: function(item) {
			// summary:
			// 		Determines user permissions for a generic item having a .roles property
			// returns:
			// 		Boolean
			
            var has = false;
			dojo.some(this.roles, function(userRole) {
				if (item.roles == "all" || dojo.indexOf(item.roles, userRole) != -1) {
					has = true;
					return false;
				}
				return true;
			});
			
			return has;
		},
		
		hasAuth: function() {
			console.debug("NoAuth");
			return this._hasAuth;
		},

		isLicenseAccepted: function() {
			console.debug(this.declaredClass + ".isLicenseAccepted("+this._acceptedLicense+")");
			return this._acceptedLicense;
		},
		
		isVirtual: function() {
			return (this._virtual === true);
		},
		
		isCCI: function() {
			return (this._cci === true);
		},

        getDomainUid: function() {
            return this.server ? encodeURIComponent(this.server) : 0;
        },
        
        // Get the AdminEndpoint for the current server
        getAdminEndpoint: function() {
           if (!this.adminEndpoint) {
               var uid = this.server ? this.server : "127.0.0.1:9089";
               this.adminEndpoint = new AdminEndpoint({uid: uid});
           }
           return this.adminEndpoint;            
        },
        
        getServerByName: function(/*String*/ name) {
            var servers = this.getServerList();
            var j = 0;
            var result = [];
            for (var i in servers) {
                if (servers[i].name == name) {
                    result[j] = servers[i];
                    j++;
                }
            }
            return result;
        },
        
        updateServerName: function(/*String*/ uid, /*String*/ oldName, /*String*/newName) {
        	var updated = false;
            var servers = this.getServerList();
            for (var i in servers) {
                if (servers[i].uid === uid) {
                    servers[i].name = newName;
                    servers[i].displayName = newName;
                    if(servers[i].name.indexOf(servers[i].uid) < 0) {
                    	servers[i].displayName += "(" + servers[i].uid + ")";
                    }
                    updated = true;
                    break;
                }
            }
            if (updated) {
            	this.setServerList(servers);
            	Utils.updateStatus();
            }
        },
        
        updateClusterName: function(/*String*/ uid, /*String*/ oldName, /*String*/newName) {
        	var updated = false;
        	this.clusterName = newName;
            var servers = this.getServerList();
            for (var i in servers) {
                if (servers[i].uid === uid) {
                    servers[i].clusterName = newName;
                    updated = true;
                    break;
                }
            }
            if (updated) {
            	this.setServerList(servers);
            	Utils.updateStatus();
            }
        },
        
        getServer: function(/*String*/ uid) {
            var servers = this.getServerList();
            for (var i in servers) {
                if (servers[i].uid == uid) {
                    return servers[i];
                }
            }
            // If we didn't find one, return what we can...
            var server = {uid: uid};
            var adminAddress, adminPort;
            var index = uid.lastIndexOf(":");
            if (index > 0 && index < uid.length-1) {
                if (uid.indexOf("[") === 0 && uid.indexOf("]") > 0 )  {
                    adminAddress = uid.substring(1, uid.indexOf("]"));
                } else {
                    adminAddress = uid.substring(0, index);
                }
                adminPort = uid.substring(index+1);
                server.adminServerAddress = adminAddress;
                server.adminServerPort = adminPort;
            }
            return server;
        },
		
        getNewServer: function(/*String*/ uid, /*String*/ addr, /*String*/ port) {
        	var newserver = undefined;
            var server = this.getServer(uid);
            if (addr.toLowerCase() === "all") {
                addr = server.adminServerAddress;
            }
            if (addr+":"+port !== uid) {
            	newserver = {};
            	for (var prop in server) {
            		newserver[prop] = server[prop];
            	}
            	newserver["uid"] = addr+":"+port;
            	newserver["adminServerAddress"] = addr;
            	newserver["adminServerPort"] = port;
            }
            return newserver;
        },
        
        /*
         * Persists the status for whether first server has been configured.
         */
		setFirstServerConfigured: function() {
            var recentservers = this.getRecentServerUids();
            this.removeUidFromRecentServers(recentservers[0]);
			this._prefsData.user.firstServerConfigured = true;
            this.updateRecentServerUids(this._prefsData.user.servers[0].uid);                       
		},
		
		/*
		 * Returns status of whether first server has been configured.
		 */
		isFirstServerConfigured: function() {
            if (this._prefsData.user.firstServerConfigured === undefined) {
                this._prefsData.user.firstServerConfigured = false;
            }
            return this._prefsData.user.firstServerConfigured;
		},
		
        /**
         * Gets the list of servers this user is managing
         * List is like [ {uid: uid, name: name, adminServerAddress: ipAddress, adminServerPort: port}, ... ]
         * @returns {Array}
         */
         getServerList: function() {
            if (!this._prefsData.user.servers) {
                this._prefsData.user.servers = [];
            }
            return this._prefsData.user.servers;
        },
          
        /**
         * Persists the server list.
         */
        setServerList: function(/*Array*/ servers, /*Boolean*/isFirstServer) {
            this._prefsData.user.servers = servers;
            if (!isFirstServer) {
                this._prefs.persistPreferences(this._prefsData); 
            } else {
            	this.setFirstServerConfigured();
            }
            // clear the AdminEndpoint so that it gets freshly instantiated
            this.adminEndpoint = undefined;
        },
		
        /**
         * Gets the list of recent server uids. The first item
         * in the list is the most recent.
         * @returns {Array}
         */
        getRecentServerUids: function() {
            if (!this._prefsData.user.recentServerUids) {
                this._prefsData.user.recentServerUids = [];
            }
            return this._prefsData.user.recentServerUids;            
        },
        
        /**
         * Removes a uid from the recent servers list. Does not persist
         * the changes. Call setServerList() or updateRecentServers() to persist.
         * @param uid
         */
        removeUidFromRecentServers: function(/*String*/ uid) {
            var recentServers = this.getRecentServerUids();
            var uidIndex = recentServers.indexOf(uid);
            if (uidIndex > -1) {
                recentServers.splice(uidIndex, 1);
            }         
        },
        
        /**
         * Sets the specified uid as the most recent and prunes
         * the list so that the uid only exists once in the list
         * and the list doesn't grow beyond 20 servers.
         * @param uid
         */
        updateRecentServerUids: function(/*String*/ uid) { 
            if (!uid || uid === "") {
                return;
            }
            var recentServers = this.getRecentServerUids();
            var uidIndex = recentServers.indexOf(uid);
            if (uidIndex === 0) {
                return; // nothing to do, it's already the first one
            }
            // if it's elsewhere in the array, remove it
            if (uidIndex > 0) {
                recentServers.splice(uidIndex, 1);
            }
            // now add it as the first element in the array
            length = recentServers.unshift(uid);
            // don't let the array get too long
            while (length > 20) {
                recentServers.splice(20, length - 20);
            }
           
            this._prefsData.user.recentServerUids = recentServers;
            this._prefs.persistPreferences(this._prefsData);          
        },
        
		setLicenseAccept: function(message, onFinish, onError) {
			console.debug(this.declaredClass + ".setLicenseAccept(true)");
			
			message.Accept = true;
			var user = this;
			var promise = Utils.acceptLicense(message);
			promise.then(function() {
				user._acceptedLicense = true;
				if (onFinish) {
					onFinish();
				}
			}, function(error) {
				if(onError) {
					onError(error);
				}
			});
		},
		
		setLicenseType: function(type) {
			this.licenseType = type;
		},
		
		getLicenseType: function() {
			return this.licenseType;
		},
		
		setLicenseAccepted: function(accepted) {
			this._acceptedLicense = accepted;
		},
		
		getClusterName: function() {
			return this.clusterName;
		},
		
		getHomeTasksVisible: function() {
			return this._homeTasksVisible;
		},
		
		setHomeTasksVisible: function(visible) {
			console.debug(this.declaredClass + ".setHomeTasksVisible("+visible+")");
			this._homeTasks = visible;
			var hidden = !visible;
			this._prefsData.user.homeTasksHidden = hidden;
			if (visible) {
				// restore the tasks, too
				var tasks = [];
				for (var type in Resources.tasks) {
					tasks.push(Resources.tasks[type].id);
				}
				this.setHomeTasksOpen(tasks, "open");
			} else {			
				// only set if not done in setHomeTasksOpen
				this._prefs.persistPreferences(this._prefsData, true);
			}
		},
		
		getHomeTaskOpen: function(/*String*/ taskId) {
			if (this._prefsData.user) {
				var task = this._prefsData.user[taskId]; 
				if (task == "closed") {
					return false;
				}
			}
			return true;
		},

		setHomeTaskOpen: function(/*String*/ taskId, /*boolean*/ isOpen) {
			console.debug(this.declaredClass + ".setHomeTaskOpen("+isOpen+")");
			var state = "closed";
			if (isOpen) {
				state = "open";
			}
			this._prefsData.user[taskId] = state;
			
			this._prefs.persistPreferences(this._prefsData);			
		},

		// sets all home tasks to open
		setHomeTasksOpen: function(/*String Array*/ taskIds, /*boolean*/ isOpen) {
			console.debug(this.declaredClass + ".setHomeTaskOpen("+isOpen+")");
			var state = "closed";
			if (isOpen) {
				state = "open";
			}

			for (var i=0, len=taskIds.length; i < len; i++) {
				var taskId = taskIds[i];
				this._prefsData.user[taskId] = state;
			}
			
			this._prefs.persistPreferences(this._prefsData, true);			
		},
		
		getSectionOpen: function(/*String*/ triggerId, /*boolean*/ defaultState) {
			if (defaultState == undefined) {
				defaultState = true;
			}
			if (this._prefsData.user.toggles) {
				var state = this._prefsData.user.toggles[triggerId];
				if (state == undefined) {
					return defaultState;
				}
				return state != "closed";
			} else {
				this._prefsData.user.toggles = {};
			}
			return defaultState;			
		},
		
		setSectionOpen: function(/*String*/ triggerId, /*boolean*/ isOpen) {
			var state = "closed";
			if (isOpen) {
				state = "open";
			}
			if (!this._prefsData.user.toggles) {
				this._prefsData.user.toggles = {};
			}

			this._prefsData.user.toggles[triggerId] = state;
			this._prefs.persistPreferences(this._prefsData);			
		},

		getStatSelected: function(/*String*/ statId, /*boolean*/ defaultState) {
			if (defaultState == undefined) {
				defaultState = true;
			}
			if (this._prefsData.user.stats) {
				var state = this._prefsData.user.stats[statId];
				if (state == undefined) {
					return defaultState;
				}
				return state;
			} else {
				this._prefsData.user.stats = {};
			}
			return defaultState;			
		},
		
		setStatSelected: function(/*String*/statId, /*boolean*/ selected) {
			var state = selected;
			if (!this._prefsData.user.stats) {
				this._prefsData.user.stats = {};
			}
			this._prefsData.user.stats[statId] = state;
			this._prefs.persistPreferences(this._prefsData);			
		},

		/**
		 * Get the preferred width for a column
		 * @param gridId  The id of the grid the column is in
		 * @param columnId The column id
		 * @param defaultWidth The default width for the column. Defaults to "100px".
		 * @returns The preferred width if set or the default width if not
		 */
		getColumnWidth: function(/*String*/ gridId, /*String*/ columnId, /*String*/ defaultWidth) {
			if (defaultWidth == undefined) {
				defaultWidth = "100px";
			}
			if (this._prefsData.user.grids) {
				var grid = this._prefsData.user.grids[gridId];
				if (!grid || !grid[columnId] || grid[columnId].width === undefined) {
					return defaultWidth;
				}
				return grid[columnId].width;
			} else {
				this._prefsData.user.grids = {};
			}
			return defaultWidth;			
		},
		
		/**
		 * Persist the width for the specified grid and column
		 * @param gridId  The id of the grid the column is in
		 * @param columnId The column id
		 * @param width The width to persist, or null to clear the setting
		 */
		setColumnWidth: function(/*String*/ gridId, /*String*/ columnId, /*String*/ width) {
			var changed = false;
			if (!this._prefsData.user.grids) {
				this._prefsData.user.grids = {};
			}
			if (!this._prefsData.user.grids[gridId]) {
				this._prefsData.user.grids[gridId] = {};
			}
			if (!this._prefsData.user.grids[gridId][columnId]) {
				this._prefsData.user.grids[gridId][columnId] = {};
			}				
			if (width === undefined || width === null && this._prefsData.user.grids[gridId][columnId].width) {
				delete this._prefsData.user.grids[gridId][columnId].width;
				changed = true;
			} else if (this._prefsData.user.grids[gridId][columnId].width !== width){
				this._prefsData.user.grids[gridId][columnId].width = width;
				changed = true;
			}
			if (changed) {
				console.debug("column width changed for column " + columnId);
				this._prefs.persistPreferences(this._prefsData);
			}
		},

		/**
		 * Get the preference for whether to show a column
		 * @param gridId  The id of the grid the column is in
		 * @param columnId The column id
		 * @param defaultshown true if it should be shown by default, false otherwise.
		 * @returns The preference if set or the default otherwise
		 */
		getColumnShown: function(/*String*/ gridId, /*String*/ columnId, /*boolean*/ defaultShown) {
			if (this._prefsData.user.grids) {
				var grid = this._prefsData.user.grids[gridId];
				if (!grid || !grid[columnId] || grid[columnId].shown === undefined) {
					return defaultShown;
				}
				return grid[columnId].shown;
			} else {
				this._prefsData.user.grids = {};
			}
			return defaultShown;			
		},
		
		/**
		 * Persist the shown attribute for the specified grid and column
		 * @param gridId  The id of the grid the column is in
		 * @param columnId The column id
		 * @param shown The state to persist, or null to clear the setting
		 */
		setColumnShown: function(/*String*/ gridId, /*String*/ columnId, /*boolean*/ shown) {
			if (!this._prefsData.user.grids) {
				this._prefsData.user.grids = {};
			}
			if (!this._prefsData.user.grids[gridId]) {
				this._prefsData.user.grids[gridId] = {};
			}
			if (!this._prefsData.user.grids[gridId][columnId]) {
				this._prefsData.user.grids[gridId][columnId] = {};
			}				
			if (shown === undefined || shown === null) {
				delete this._prefsData.user.grids[gridId][columnId].shown;
			} else {
				this._prefsData.user.grids[gridId][columnId].shown = shown;
			}
			this._prefs.persistPreferences(this._prefsData);			
		},
		
		getHiddenColumns: function(/*String*/ gridId) {
			var hiddenColumns = [];
			if (this._prefsData.user.grids && this._prefsData.user.grids[gridId]) {
				for (var column in this._prefsData.user.grids[gridId]) {
					if (this._prefsData.user.grids[gridId][column].shown === false) {
						hiddenColumns.push(column);
					}
				}
			}
			return hiddenColumns;
		},
		
		showAllColumns: function(/*String*/ gridId) {
			if (this._prefsData.user.grids && this._prefsData.user.grids[gridId]) {
				for (var column in this._prefsData.user.grids[gridId]) {
					this._prefsData.user.grids[gridId][column].shown = true; 
				}
				this._prefs.persistPreferences(this._prefsData);
			}			
		},
		
		getShowConfirmation: function(/*String*/ confirmationId) {
			if (this._prefsData.user.showConfirmation) {
				var show = this._prefsData.user.showConfirmation[confirmationId];
				if (show === undefined) {
					return true;
				}
				return show;
			} else {
				this._prefsData.user.showConfirmation = {};
			}
			return true;			
		},
		
		setShowConfirmation: function(/*String*/ confirmationId, /*boolean*/ show) {
			if (!this._prefsData.user.showConfirmation) {
				this._prefsData.user.showConfirmation = {};
			} else if (this._prefsData.user.showConfirmation[confirmationId] !== undefined && 
					this._prefsData.user.showConfirmation[confirmationId] == show) {
				console.debug("User.setShowConfirmation called, but already set.");
				return;
			}

			this._prefsData.user.showConfirmation[confirmationId] = show;
			this._prefs.persistPreferences(this._prefsData);			
		},

		/**
		 * increment the count for the visited page.
		 * @param page The main page (equates to a Tab, First Steps, or License)
		 * @param pane The pane being visited or null if none (equates to an item on a tab's menu)
		 * @param subPane If included, this is assumed to be a specific instance
		 *        of a pane, such as the details for a specific message hub
		 */
		recordVisit: function(page, pane, subPane) {
			if (!page) {
				return;
			}
			if (!this._prefsData.user.pages) {
				this._prefsData.user.pages = {};
			}
			var pages = this._prefsData.user.pages;
			var count = 0;
			
			if (!pages[page]) {
				pages[page] = {};				
			}
			if (pane && !pages[page][pane]) {
				pages[page][pane] = {};
			} 			
			if (subPane) {
				if (!pages[page][pane][subPane]) {			
					pages[page][pane][subPane] = {visits: 1};
				} else {
					count = pages[page][pane][subPane].visits ? pages[page][pane][subPane].visits + 1 : 1;
					pages[page][pane][subPane].visits = count;
				}
			} else if (pane) {
				count = pages[page][pane].visits ? pages[page][pane].visits + 1 : 1;
				pages[page][pane].visits = count;
			} else {
				count = pages[page].visits ? pages[page].visits + 1 : 1;
				pages[page].visits = count;				
			}

			this._prefs.persistPreferences(this._prefsData);			
		},
		
		_getUserAuth: function() {
			console.debug(this.declaredClass + ".getUserAuth()");
			
			var userAuth;
			var t = this;
			dojo.xhrGet({
			    url: Utils.getBaseUri() + "rest/config/webui/preferences/authAndPrefs",
			    handleAs: "json",
			    preventCache: true,
			    sync: true, // need to wait for an answer
			    load: function(data){
			    	console.dir(data);
			    	userAuth = {userRoles: data.userRoles, username: data.username};
			    	if (data.preferences !== undefined) {
			    		t._prefs = new Preferences({userId: data.username, preferences: data.preferences});
			    	}
			    	if (data.virtual !== undefined) {
			    		t._virtual = data.virtual;
			    	}
			    	if (data.cci !== undefined) {
			    		t._cci = data.cci;
			    	}
			    	if (data.docker !== undefined) {
			    	    t._docker = data.docker;
			    	}
		    	    t.defaultAdminServerAddress = data.defaultAdminServerAddress;
                    t.defaultAdminServerPort = data.defaultAdminServerPort;
			    	if (data.localeInfo !== undefined) {
			    		t.localeInfo = data.localeInfo;
			    	}
			    }
			  });
			return userAuth;
		},
		
		getSSOToken: function(sync) {
		    console.debug(this.declaredClass + ".getSSOToken()");
		    dojo.xhrGet({
		        url: Utils.getBaseUri() + "rest/config/userregistry/sso",
		        handleAs: "json",
		        preventCache: true,
		        sync: sync // need to wait for an answer
		    });
		    return;
		},
		
		clearSSOToken: function() {
		    console.debug(this.declaredClass + ".clearSSOToken()");
		    cookie("imaSSO", null, {expires: -1});
		    return;
		}


	});

	return User;
});

