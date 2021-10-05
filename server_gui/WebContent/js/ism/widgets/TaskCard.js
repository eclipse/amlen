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
		'dojo/_base/connect',
		'dojo/dom-construct',
		'dojo/fx',
		'dojo/fx/Toggler',
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin',
		'dijit/layout/ContentPane',
		'ism/config/Resources',
		'dojo/text!ism/widgets/templates/TaskCard.html',
		'dojo/i18n!ism/nls/strings'
		], function(declare, lang, connect, domConstruct, fx, Toggler, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin, ContentPane, Resources, template, nls) {

	var TaskCard = declare("ism.widgets.TaskCard", [ContentPane, _TemplatedMixin, _WidgetsInTemplateMixin], {
		// summary:
		// 		A closeable information "card", used to display tasks on the Welcome page.
		//
		// description: 
		// 		The template for this widget has the following components:
		// 			- An icon
		// 			- A heading (bold text)
		// 			- A description
		// 			- A set of links to sub-pages
		// 			- A close button
		//
		// 		Widgets extending this class MUST provide a "type" parameter in the constructor,
		// 		which pulls the content for heading, description, links, id, and icon from the Resources file.
		// 		The Resource definition for a task card looks like this:
		//
		// 		<object definition>
		
		// heading for task card
		heading: null,
		// description for task card
		description: null,
		// links for task card
		links: [{uri: "", label: ""}],
		// id of task card
		id: null,
		// css definition of task card
		cssClass: null,
		
		nls: nls,

		templateString: template,

		constructor: function(args) {
			// summary:
			// 		Constructor.  Overrides content fields based on "type" param.
			
			this.inherited(arguments);
			dojo.safeMixin(this, args);
			if (this.type !== "") {
				this.heading = Resources.tasks[this.type].heading;
				this.description = Resources.tasks[this.type].description;
				this.links = Resources.tasks[this.type].links;
				this.cssClass = Resources.tasks[this.type].cssClass;
				this.id = "id_" + Resources.tasks[this.type].id;
			} 
		},

		postCreate: function() {
			// summary:
			// 		Performs post-template-generation function.
			//
			// description:
			// 		After content is generated from template, this method modifies the link text
			// 		based on the Resource definition and sets up a publish method to notify subscribers
			// 		that this task card is to be closed.
			
			console.debug(this.declaredClass + ".startup()");
			this.inherited(arguments);

			var linksText = "<span>";
			var sep = "";
			for (var prop in this.links) {
				var link = this.links[prop];
				var linkText = link.label;
				if (link.uri !== "") {
				    var uri = link.uri;
                    if (ism.user.server) {
                        var server = encodeURIComponent(ism.user.server);
                        uri += (uri.indexOf("?") < 0) ? "?server="+server : "&server="+server; 
                        uri += "&serverName=" + encodeURIComponent(ism.user.serverName);                            
                    }
				    
					linkText = "<a class='linkText' href='" + uri + "'>" + link.label + "</a>";
					linkText += "<span class='noLinkText' title='" + nls.global.pageNotAvailable +
						"'>" + link.label + "</span>";
				}
				linksText += sep + linkText;
				sep = " | ";
			}
			linksText += "</span>";
			domConstruct.place(linksText, this.linksNode);
			this.closeButton.onclick = lang.hitch(this, function(evt) {
				var topic = Resources.tasks[this.type].id + "-close";
				var msg = { id: Resources.tasks[this.type].id };
				dojo.publish(topic, msg);
			});
		}
	});

	return TaskCard;
});
