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
define(['dojo/_base/lang', 'dojo/_base/array'], function(lang, arrayUtil){	
	
	/**
	 * Enhances SimpleQueryEngine sorting to sort fields as follows:
	 * -- If the field contains a string, do case insensitive sort
	 * -- If the field contains an object with a sortWeight property,
	 *    uses it to sort
	 */
	return function(query, options) {	
		
		// create our matching query function
		/*jsl:ignore*/
		switch(typeof query){
			default:
				throw new Error("Can not query with a " + typeof query);
			case "object": 
			case "undefined":
				var queryObject = query;
				query = function(object){
					for(var key in queryObject){
						var required = queryObject[key];
						if(required && required.test){
							// an object can provide a test method, which makes it work with regex
							if(!required.test(object[key], object)){
								return false;
							}
						}else if(required != object[key]){
							return false;
						}
					}
					return true;
				};
				break;
			case "string":
				// named query
				if(!this[query]){
					throw new Error("No filter function " + query + " was found in store");
				}
				query = this[query];
				// fall through
			case "function":
				// fall through
		}
		/*jsl:end*/
					
		function execute(array){
			// execute the whole query, first we filter
			var results = arrayUtil.filter(array, query);
			// next we sort
			var sortSet = options && options.sort;
			if(sortSet){
				results.sort(typeof sortSet == "function" ? sortSet : function(a, b){
					for(var sort, i=0; sort = sortSet[i]; i++){
						var aValue = a[sort.attribute];
						var bValue = b[sort.attribute];
						if (typeof aValue == "string") {
							aValue = aValue.toLowerCase();
						} else if (aValue && aValue.sortWeight !== undefined) {
							aValue = aValue.sortWeight;
						}
						if (typeof bValue == "string") {
							bValue = bValue.toLowerCase();
						} else if (bValue && bValue.sortWeight !== undefined) {
							bValue = bValue.sortWeight;
						}
						if (aValue != bValue){
							return !!sort.descending == (aValue === null || aValue === undefined || aValue > bValue) ? -1 : 1;
						}
					}
					return 0;
				});
			}
			// now we paginate
			if(options && (options.start || options.count)){
				var total = results.length;
				results = results.slice(options.start || 0, (options.start || 0) + (options.count || Infinity));
				results.total = total;
			}
			return results;
		}
		execute.matches = query;
		return execute;
		
	};
	
});
