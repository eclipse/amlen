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
		'dojo/_base/query',
		'dojo/_base/lang',
		'dojo/_base/array',
		'dojo/_base/json',
		'dojo/_base/fx',
		'dojo/dom-class',
		'dojo/dom-style',
		'dojo/dom-construct',
		'dojo/dom-attr',
		'dojo/fx',
		'dojo/fx/easing',
		'dijit/form/Button',
		'dijit/registry',
		'idx/form/TextBox',
		'dijit/form/SimpleTextarea',
		'idx/data/JsonStore',
		'ism/config/Resources',
		'ism/widgets/ToggleContainer',
		'ism/widgets/TaskCard',
		'dojo/i18n!ism/nls/home',
		'ism/IsmUtils'
		], function(declare, query, lang, array, json, baseFx, domClass, domStyle, domConstruct, domAttr, fx, easing, Button, registry,
				TextBox, SimpleTextarea, JsonStore, Resources, ToggleContainer, TaskCard, nls, Utils) {

	var Tasks = declare("ism.controller.content.Tasks", [ToggleContainer], {
	
		startsOpen: true,
		lastServerStatus: null,
		lastHaRole: "UNKNOWN",
		_data: null,
		domainUid: 0,
		
		constructor: function(args) {
			dojo.safeMixin(this, args);
			this.title = nls.home.taskContainer.title;
			this.triggerId = "recommendedTasks";
			this.startsOpen = ism.user.getSectionOpen(this.triggerId);
			console.debug("Tasks section should start open: " + this.startsOpen);
			
			// subscribe to server status changes
			dojo.subscribe(Resources.serverStatus.adminEndpointStatusChangedTopic, lang.hitch(this, function(data) {
				this._handleServerStatus(data);
			}));
			

		},

		postCreate: function() {
			console.debug(this.declaredClass + ".postCreate()");
			this.inherited(arguments);
            this.domainUid = ism.user.getDomainUid();
			if (ism.user.roles == Resources.roles.observer || ism.user.getHomeTasksVisible() === false) {
				this.set("style", "display: none");
				domAttr.set(this.domNode, "hidden", true);
				return;
			}
			
            // kick off a get of the status
            var promise =  Utils.getStatus();        
            
			this.toggleExtra.innerHTML += "<span class='taskCountWrap'>&nbsp;&nbsp;<span class='taskCount' id='taskCount' aria-live='polite'></span></span>";
			
			this.toggleNode.innerHTML = "";
			// put hide and restore links at top
			var nodeText = [
				"<span class='tagline'>" + nls.home.taskContainer.tagline + "</span><br />",
			    "<div id='tasksLinks' class='tasksLinks' aria-live='polite'>",
				"<span style='display: none; opacity: 0;' hidden='true' id='restoreTasks'><a class='linkText' " + 
					"href='javascript://' id='restoreTasksLink' title='" + nls.home.taskContainer.links.restoreTitle + 
					"'>" + 	nls.home.taskContainer.links.restore +"</a></span>",
				"<span style='display: none; opacity: 0;' hidden='true' id='hideTasks'>|&nbsp;&nbsp;<a class='linkText' " +
					"href='javascript://' id='hideTasksLink' title='" + nls.home.taskContainer.links.hideTitle + 
					"'>"+ nls.home.taskContainer.links.hide +"</a></span></div>",
				"<div class='tasksContent' id='tasksContent'></div>"
			].join("\n");

			domConstruct.place(nodeText, this.toggleNode);
			console.debug(this.toggleNode);
			console.debug(dojo.byId("tasksContent"));

	        var _this = this;
			dojo.query("#restoreTasksLink").connect("click", function(evt) {
				_this._restoreTasks({ animate: true });
			});

			dojo.query("#hideTasksLink").connect("click", function(evt) {
				_this._hideTasks({ animate: true });
			});

			this._loadTasks();
            this._enableDisableLinks({RC: -1, harole: "UNKNOWN"});
			this._updateTaskCount();
			
			// when the status result is back, handle it
			promise.then(
			    function(data) {
			        Utils.supplementStatus(data);
			        _this._handleServerStatus(data);
			    }, 
			    function(error){ 
			        _this._handleServerStatus({RC: -1, harole: "UNKNOWN"});
			});				

		},
				
		_enableDisableLinks: function (data) {
			// First enable all Tasks
			query(".linkText","tasksContent").forEach(function(node) {
				domStyle.set(node,"display","inline-block");
			});
			query(".noLinkText","tasksContent").forEach(function(node) {
				domStyle.set(node,"display","none");
			});
			
			// extract the nav key
			var _getUriKey = function(uri) {
				var key = uri;
				var idx = uri.indexOf("?nav=");
				if (idx > 0) {
					key = uri.substring(idx+5);
				}
				// remove other params
				idx = key.indexOf("&");
                if (idx > 0) {
                    key = key.substring(0, idx);
                }
				return key;
			};

			
			// Next disable any Tasks that are not applicable to the current status
			query(".linkText","tasksContent").forEach(function(node) {
				var href = _getUriKey(node.href);					
				var task = registry.getEnclosingWidget(node);
				if (task && task.links) {
					for (var prop in task.links) {
						var link = task.links[prop];
						if (_getUriKey(link.uri) == href) {
							if (Utils.isItemDisabled(link, data).disabled) {
								// hide the link and show the non-link replacement (which is the next node)
								domStyle.set(node,"display","none");
								var nextNode = node.nextSibling;
								domStyle.set(nextNode,"display","inline-block");
							}
						}
					}						
				}					
			});
		},

		_handleServerStatus: function(data) {
			this._data = data;			
			var statusRC = data.RC;			
			var haRole = data.harole;
			console.debug(this.declaredClass + "._handleServerStatus("+statusRC+", " +haRole+")");
						
			if (this.lastServerStatus != statusRC || this.lastHaRole != haRole) {
				this.lastServerStatus = statusRC;
				this.lastHaRole = haRole;
				console.debug("Server status changed");
				this._enableDisableLinks(data);
			}
		},				
		
		_loadTasks: function(args) {
			args = lang.mixin({}, args);
			var style = "";
			if (args.animate) {
				style = "display: none; opacity: 0";
			}
			var animations = [];
			var tasksAdded = [];
			for (var type in Resources.tasks) {
				if (ism.user.hasPermission(Resources.tasks[type])) {
					var taskRefName = Resources.tasks[type].id;
					var subsetOf = Resources.tasks[type].subsetOf;
					if (subsetOf && dojo.indexOf(tasksAdded, subsetOf) >= 0 ) {
						continue;  // task that supersets this has already been added
					}
					tasksAdded.push(taskRefName);
					if (ism.user.getHomeTaskOpen(taskRefName) === true) {
						if (!dojo.byId(taskRefName)) {
							domConstruct.place("<div style='"+style+"' id='"+taskRefName+"'></div>", "tasksContent");
							var task = new TaskCard({ type: type });
							domConstruct.place(task.domNode, taskRefName);
							this.subscribe(taskRefName + "-close", lang.hitch(this, function(msg) {
								console.debug(msg);
								this._closeTask(msg.id);
							}));
						}
						if (domStyle.get(taskRefName, "display") == "none") {
							if (args.animate) {
								animations.push(
									fx.combine([
										fx.wipeIn({ node: taskRefName, duration: 500, easing: easing.expoOut }),
										baseFx.fadeIn({ node: taskRefName, duration: 500 })
									]));
							}
						}
					}
				}				
			}
			if (animations.length > 0) {
				fx.chain(animations).play();
			}
		},

		_restoreTasks: function(args) {
			var tasks = [];
			for (var type in Resources.tasks) {
				tasks.push(Resources.tasks[type].id);
			}
			ism.user.setHomeTasksOpen(tasks, "open");
			this._loadTasks(args);
			this._updateTaskCount();
			if (this._data) {
				this._enableDisableLinks(this._data);
			} 
		},
		
		_hideTasks: function(args) {
			ism.user.setHomeTasksVisible(false);
			this.set("style", "display: none");
			return;			
		},

		_closeTask: function(domId) {
			fx.combine([
				fx.wipeOut({ 
					node: domId, 
					duration: 1000, 
					easing: easing.expoOut
				}),
				baseFx.fadeOut({
					node: domId, 
					duration: 1200, 
					easing: easing.expoOut
				})
			]).play();
			ism.user.setHomeTaskOpen(domId, false);
			this._updateTaskCount();
		},

		_updateTaskCount: function() {
			var count = 0;
			var total = 0;
			for (var type in Resources.tasks) {
				if (ism.user.hasPermission(Resources.tasks[type])) {					
					if ( ism.user.getHomeTaskOpen(Resources.tasks[type].id) === true) {
						count++;
					}
					total++;
				}
			}
			dojo.byId("taskCount").innerHTML = lang.replace(nls.home.taskContainer.taskCount,[count]);
			if (count == total) {
				if (domStyle.get("restoreTasks", "opacity") == "1") {
					baseFx.fadeOut({ node: "restoreTasks", duration: 500 }).play();	
					domAttr.set("restoreTasks", "hidden", true);
					domStyle.set("restoreTasks","display","none");
				}
				if (domStyle.get("hideTasks", "opacity") == "1") {
					baseFx.fadeOut({ node: "hideTasks", duration: 500 }).play();
					domAttr.set("hideTasks", "hidden", true);
					domStyle.set("hideTasks","display","none");
				}
			} else {  // Some tasks are closed
				// Make sure the Restore Tasks link is showing
				if (domStyle.get("restoreTasks", "opacity") == "0") {
					baseFx.fadeIn({ node: "restoreTasks", duration: 500 }).play();
					domAttr.set("restoreTasks", "hidden", false);
					domStyle.set("restoreTasks","display","inline");					
				}
				// If there are no tasks open, show the Hide Tasks link
				if (count === 0) {
					if (domStyle.get("hideTasks", "opacity") == "0") {
						baseFx.fadeIn({ node: "hideTasks", duration: 500 }).play();
						domAttr.set("hideTasks", "hidden", false);	
						domStyle.set("hideTasks","display","inline");						
					} 
				// Otherwise, make sure it's hidden
				} else {
					if (domStyle.get("hideTasks", "opacity") == "1") {
						baseFx.fadeOut({ node: "hideTasks", duration: 500 }).play();
						domAttr.set("hideTasks", "hidden", true);
						domStyle.set("hideTasks","display","none");						
					} 					
				}
			}
		}
	});

	return Tasks;
});
