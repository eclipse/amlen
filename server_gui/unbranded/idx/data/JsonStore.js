/*
 * Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
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
define([
	"dojo/_base/declare",
	"dojo/data/ItemFileWriteStore",
	"dojo/_base/lang",
	"dojo/_base/array"
], function(dDeclare,dItemFileWriteStore,dLang,dArray) {

/**
 * @name idx.data.JsonStore
 * @class Data store to deal with simple JavaScript object/array.
 *	This class also allows updating and retrieving array of JavaScript objects representing data items,
 *	using setData()/getData() methods. getItemData() method can be used to extract a plain JavaScript object from a specified data item.
 * @augments dojo.data.ItemFileWriteStore
 */
return dDeclare("idx.data.JsonStore", [dItemFileWriteStore], 
/**@lends idx.data.JsonStore#*/
{
	/**
   	 * Identifier for items.
   	 * @type String
   	 * @default ""
   	 */
	identifier: "",

	/**
   	 * Label attribute for items.
   	 * @type String
   	 * @default ""
   	 */
	label: "",

	/**
   	 * Property name to contain item array when an object is passed as data.
   	 * @type String
   	 * @default ""
   	 */
	items: "",

	/**
	 * Constructs with parameters.
	 * @param {Object} args Optionally specifies "identifier", "label" and "items".
	 */
	constructor: function(args){
		if(args){
			this.identifier = (args.identifier || this.identifier);
			this.label = (args.label || this.label);
			this.items = (args.items || this.items);
		}
	},

	/**
	 * Specifies a URL to load data.
	 * @param {String} url
	 */
	setUrl: function(url){
		this.url = url;
		this.forceLoad();
	},

	/**
	 * Sets new data.
	 * @param {Object|Array} data
	 */
	setData: function(data){
		this.data = data;
		this.forceLoad();
	},

	/**
	 * Retrieves the current data.
	 * @returns {Object|Array}
	 */
	getData: function(){
		var data = dArray.map(this._getItemsArray(), this.getItemData, this);
		if(this.items){
			var items = data;
			data = {};
			data[this.items] = items;
		}
		return data;
	},

	/**
	 * Retrieves the current data of the specified item.
	 * @param {Object} item
	 * @returns {Object}
	 */
	getItemData: function(item){
		var data = {};
		dArray.forEach(this.getAttributes(item), function(attribute){
			var values = item[attribute];
			if(!values || values.length === 0){
				return;
			}
			if(values.length === 1){
				var value = values[0];
				if(this.isItem(value)){
					value = this.getItemData(value);
				}
				data[attribute] = value;
			}else{
				data[attribute] = dArray.map(values, function(value){
					if(this.isItem(value)){
						value = this.getItemData(value);
					}
					return value;
				}, this);
			}
		}, this);
		return data;
	},

	/**
	 * Enforces loading or building items from "url" or "data" properties.
	 */
	forceLoad: function(){
		this._loadFinished = false;
	},

	/**
	 * Converts data to the format for the base class to build items.
	 * @param {Object} data
	 * @private
	 */
	_getItemsFromLoadedData: function(data){
		var items;
		if(this.items){
			items = data[this.items];
		}else{
			if(data && !dLang.isArray(data)){
				data = [data];
			}
			items = data;
		}
		arguments[0] = {items: items, identifier: this.identifier, label: this.label};
		return this.inherited(arguments);
	}
});

});
