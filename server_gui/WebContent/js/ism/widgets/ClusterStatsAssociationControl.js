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
		'dojo/number',
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
        "dojo/i18n!ism/nls/clusterStatistics",
        "ism/widgets/AssociationControl"
        ], function(declare, lang, array, number, on, domConstruct, domClass, domStyle, keys, query,
        		Memory, registry, _Widget, _TemplatedMixin,
        		HoverCard, Utils, nls, AssociationControl){
	
	/**
	 * Extends AssociationControl to provide Cluster status specific hover cards
	 */
	var ClusterStatsAssociationControl = declare("ism.widgets.ClusterStatsAssociationControl",[ AssociationControl],{	

		associationType: null,  /* provide this as input */

		clusterStatsDetails: null,
		title: null,		
		hoverCardId: null,
		structure: [],
		gridId: null,
		associatedObjectStore: null,
		
//		hoverCardWidth: 366, //px
		hoverCardWidth: null, //px
		colSize: {Name: "100px",
	          BufMsg: "100px", 
	          BufBytes: "100px", 
	          BufMsgHWM: "100px", 
	          DscMsg: "100px"},
		
		constructor: function(args) {
			dojo.safeMixin(this, args);		
			this.inherited(arguments);
			this._initStructure(this.associationType);
		},
		
		postCreate: function() {						
			this.clusterStatsDetails = dijit.byId('ismClusterStatsGrid');
			this.title = nls[this.associationType+"AssocType"];
			this.hoverCardId = "ClusterStatsHoverCard"; // use same ID to avoid getting multiple hover cards at once
			this.gridId = this.id + this.associationType + "Grid";			
		    
			this.watch('value', lang.hitch(this, function(property, oldvalue, newvalue) {
				this.qmcList = newvalue.list;
				var count = 0;
				if(this.associationType === "Buffered")
					count = number.parse(newvalue.list[0].BufferedMsgs) + number.parse(newvalue.list[1].BufferedMsgs);
				var displayValue = "";
				newvalue.sortWeight = 2;
				var marginLeft = count === 0 ? "24" : "5";
				displayValue="<span style='vertical-align: middle; margin-left: "+marginLeft+"px;'>"+count+"</span>";
				domStyle.set(this.hoverIcon, {display: 'inline'});
				this.associationCount.innerHTML = displayValue;
				this.destName = newvalue.Name;
			}));

		},
				
		getAssociatedObjectStore: function() {
			if (!this.associatedObjectStore) {
				var clusterStats = registry.byId("clusterstats");
		    	if (clusterStats && clusterStats.channelStore) {
		    		this.associatedObjectStore = clusterStats.channelStore;
		    	}
			}
			return this.associatedObjectStore;
		},
		
		getGridData: function() {
			if (!this.qmcList) {
				return null;
			}
			var associatedObjectStore = this.getAssociatedObjectStore();
			if (!associatedObjectStore) {
				return null;
			}
	    	return array.map(this.qmcList, lang.hitch(this, function(channel) {
    		    var item = associatedObjectStore.query({ Name: channel.Name })[0];
//	    		var item = null;
//	    		switch(this.associationType) {
//	    		case "Buffered":
//	    		    item = associatedObjectStore.query({ Buffered: channel.Name })[0];
//	    		    break;
//	    		default:
//	    			break;
//	    		}
	    		item.id = escape(item.Name);
//	    		item.status = {};
//	    		if (this.ruleStatus) {
//	    			item.status.rc = this.ruleStatus[item.Name];
//	    		}
//	    		if (this.ruleMessage && this.ruleMessage[item.Name]) {
//	    			item.status.msg = this.ruleMessage[item.Name];
//	    		}
	    		return item;
	    	}));
		},
		
//		getGridData: function() {
//			if (!this.qmcList) {
//				return null;
//			}
//			return this.qmcList;
//		},
		
		_initStructure: function(type) {
			switch (type) {
			case "Buffered":
				this.hoverCardWidth = 450;
				this.structure = 
					[
                     { id: "Name", name: nls.channelTitle, tooltip:nls.channelTooltip, field: "Name", dataType: "string", width: this.colSize.Name,
                         decorator: Utils.nameDecorator },
				     { id: "BufferedMsgs", name: nls.bufferedMsgsLabel, tooltip:nls.bufferedMsgsTooltip, field: "BufferedMsgs", dataType: "number", width: this.colSize.BufMsg,
					     decorator: Utils.integerDecorator },
				     { id: "BufferedBytes", name: nls.bufferedBytesLabel, tooltip:nls.bufferedBytesTooltip, field: "BufferedBytes", dataType: "number", width: this.colSize.BufBytes, 
					     decorator: Utils.integerDecorator },
				     { id: "BufferedMsgsHWM", name: nls.bufferedHWMLabel, tooltip:nls.bufferedHWMTooltip, field: "BufferedMsgsHWM", dataType: "number", width: this.colSize.BufMsgHWM, 
					     decorator: Utils.integerDecorator }
					];

				break;
			case "Discarded":
				this.hoverCardWidth = 210;
				this.structure = 
					[
                     { id: "Name", name: nls.channelTitle, tooltip:nls.channelTooltip, field: "Name", dataType: "string", width: this.colSize.Name,
                         decorator: Utils.nameDecorator },
				     { id: "DiscardedMsgs", name: nls.discardedMsgsLabel, tooltip:nls.discardedMsgsTooltip, field: "DiscardedMsgs", dataType: "number",  width: this.colSize.DscMsg,
					     decorator: Utils.integerDecorator }
					];
				break;
			default:
				break;
			}
		}
				
	});
		
	return ClusterStatsAssociationControl;
	
});
