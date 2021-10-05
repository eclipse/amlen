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
define(["dojo/_base/declare",
        "dojo/_base/lang",
        'dojo/_base/array',
        "dojo/on",
        "dojo/dom-construct",
        "dojo/dom-class",
        "dojo/dom-style",
        'dojo/keys',
        "dojo/query",
        'dojo/store/Memory',
        'dijit/registry',
        "dijit/_Widget",
        "dijit/_TemplatedMixin",
        'idx/widget/HoverCard',
		'ism/IsmUtils',
        "dojo/i18n!ism/nls/messaging",
        "ism/widgets/AssociationControl"
        ], function(declare, lang, array, on, domConstruct, domClass, domStyle, keys, query,
        		Memory, registry, _Widget, _TemplatedMixin,
        		HoverCard, Utils, nls, AssociationControl){
	
	/**
	 * Extends AssociationControl to provide Message Hub specific hover cards for endpoints, messagingPolicies, and connectionPolicies
	 */
	var MessageHubAssociationControl = declare("ism.widgets.MessageHubAssociationControl",[ AssociationControl],{	

		associationType: null,  /* provide this as input */

		messageHubDetails: null,
		title: null,		
		hoverCardId: null,
		structure: [
					{	id: "Name", name: nls.messaging.messageHubs.hovercard.name, field: "Name", dataType: "string", width: "120px",
						decorator: function(cellData) {
							var value = Utils.nameDecorator(cellData);
							return "<span title='"+value+"'>"+value+"</span>";
						}
					},
					{	id: "Description", name: nls.messaging.messageHubs.hovercard.description, field: "Description", dataType: "string", width: "210px",
						decorator: function(cellData) {
							var value = Utils.textDecorator(cellData);
							return "<span title='"+value+"'>"+value+"</span>";
						}
					}					
				],
		gridId: null,
		associatedObjectStore: null,
		
		hoverCardWidth: 366, //px
		
		constructor: function(args) {
			dojo.safeMixin(this, args);		
			this.inherited(arguments);
		},
		
		postCreate: function() {						
			this.messageHubDetails = dijit.byId('ismMessageHubDetails');
			this.title = nls.messaging.messageHubs.hovercard[this.associationType];
			this.hoverCardId = "MessageHubHoverCard"; // use same ID to avoid getting multiple hover cards at once
			this.gridId = this.id + this.associationType + "Grid";			
		    
			this.watch('value', lang.hitch(this, function(property, oldvalue, newvalue) {
				this.qmcList = newvalue.list;
				var count = this.qmcList === "" ? 0 : this.qmcList.split(",").length;
				newvalue.sortWeight = count;
				var displayValue = "";
				if (count === 0 && !newvalue.zeroOK) {
					displayValue= 
					"<span style='color: red; vertical-align: middle; margin-left: 5px;'>0</span><div style='margin-right: 3px; float: left;'>" +
					"<img style='vertical-align: middle' src='css/images/msgWarning16.png' alt='"+ nls.messaging.messageHubs.hovercard.warning +"' /></div>";
					domStyle.set(this.hoverIcon, {display: 'none'});
				} else {
					var display = count === 0 ? 'none' : 'inline';
					var marginLeft = count === 0 ? "24" : "5";
					displayValue="<span style='vertical-align: middle; margin-left: "+marginLeft+"px;'>"+count+"</span>";
					domStyle.set(this.hoverIcon, {display: display});
				}				
				this.associationCount.innerHTML = displayValue;
				this.destName = newvalue.Name;
			}));

		},
				
		getAssociatedObjectStore: function() {
			if (!this.associatedObjectStore) {
			    this.associatedObjectStore = this.messageHubDetails.stores[this.associationType];
			}
			return this.associatedObjectStore;
		}
				
	});
		
	return MessageHubAssociationControl;
	
});
