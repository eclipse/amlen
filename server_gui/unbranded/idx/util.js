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
 
define(["dojo/_base/lang",
        "idx/main",
        "dojo/_base/kernel",
        "dojo/has",
		"dojo/aspect",
        "dojo/_base/xhr",
        "dojo/_base/window",
        "dojo/_base/url",
        "dojo/date/stamp",
        "dojo/json",
        "dojo/string",
        "dojo/dom-class",
        "dojo/dom-style",
        "dojo/dom-attr",
        "dojo/dom-construct",
        "dojo/dom-geometry",
        "dojo/io-query",
        "dojo/query",
        "dojo/NodeList-dom",
		"dojo/Stateful",
        "dijit/registry",
        "dijit/form/_FormWidget",
        "dijit/_WidgetBase",
        "dojo/_base/sniff",
        "dijit/_base/manager"], // temporarily resolve dijit.byid() uncaught exception issue in aprser 
		function(dLang,				// dojo/_base/lang
				 iMain,				// idx
		         dKernel, 			// dojo/_base/kernel
		     	 dHas,				// dojo/has
				 dAspect,			// dojo/aspect
		     	 dXhr,				// dojo/_base/xhr
   		    	 dWindow,			// dojo/_base/window
   		     	 dURL,				// dojo/_base/url
   		     	 dDateStamp,		// dojo/date/stamp
	   		     dJson,				// dojo/json
	   		     dString,			// dojo/string
				 dDomClass,			// dojo/dom-class (for dDomClass.add)
				 dDomStyle,			// dojo/dom-style (for dDomStyle.getComputedStyle/set)
				 dDomAttr,			// dojo/dom-attr
				 dDomConstruct,		// dojo/dom-construct
				 dDomGeo,			// dojo/dom-geometry (for dDomGeo.getMarginBox)
				 dIOQuery,			// dojo/io-query
				 dQuery,			// dojo/query
				 dNodeList,			// dojo/NodeList-dom
				 dStateful,			// dojo/Stateful
				 dRegistry,			// dijit/registry
				 dFormWidget,		// dijit/form/_FormWidget
				 dWidget)			// dijit/_WidgetBase
{
    /**
 	 * @name idx.util
 	 * @namespace Provides Javascript utility methods in addition to those provided by Dojo.
 	 */
	var iUtil = dLang.getObject("util", true, iMain);
	
	/**
  	 * @public
  	 * @function
 	 * @name idx.util.getVersion
 	 * @description Returns the IDX toolkit version string from the "version.txt" file embedded in the toolkit.
 	 * @param {Boolean} full A boolean value indicating if the full version or partial version is desired.
 	 */
	iUtil.getVersion = function(full) {
		var params = {
			url: dKernel.moduleUrl("idx", "version.txt"),
			showProgress: false,
			handleAs: "json",
			load: function(response, ioArgs) {
				var msg = response.version;
				if(full) {
					msg += "-";
					msg += response.revision;
				}
				console.debug(msg);
			},
			error: function(response, ioArgs) {
				console.debug(response);
				return;
			}
		};
		dXhr.xhrGet(params);
	};
	
	/**
  	 * @public
  	 * @function
 	 * @name idx.util.getOffsetPosition
 	 * @description Returns the pixel offset from top-left for a given node relative to a "root" node.
 	 * @param {Node} node The node for which the offset is desired.
 	 * @param {Node} root The optional ancestor node of the first specified node.  If not specified
 	 *                    then the Window body tag is used.
 	 * @return {Object} The returned value has two fields: "l" and "t" representing "left offset" and 
 	 *                  "top offset", respectively.  Results are undefined if the "root" node is not an
 	 *                  ancestor of the first specified node.
 	 */
	iUtil.getOffsetPosition = function(node, root) {
		var body = dWindow.body();
		root = root || body;
		var n = node;
		
		var l = 0;
		var t = 0;
		
		while (n !== root) {
			// avoid infinite loop if root is not ancestor of node
			if (n === body) throw "idx.util.getOffsetPosition: specified root is not ancestor of specified node";
			
			// otherwise accumulate the offsets and move on to the parent
			l += n.offsetLeft;
			t += n.offsetTop;
			n = n.offsetParent;
		}
		return {l: l, t: t};
	};

	/**
  	 * @public
  	 * @function
 	 * @name idx.util.typeOfObject
 	 * @description Provides a type for the specified object based on the locally-scoped
 	 *              "val2type" function from "dojo/parser" module.
 	 * @param {Any} value The value for which the type is desired.
 	 * @return {String} Possible return values include: "string", "undefined", "number", "boolean", 
 	 *                  "function", "array", "date", "url" and "object".
 	 */
	iUtil.typeOfObject = function(/*Object*/ value){
		// summary:
		//		Returns name of type of given value.

		if(dLang.isString(value)){ return "string"; }
		if(typeof value == "undefined") { return "undefined"; }
		if(typeof value == "number"){ return "number"; }
		if(typeof value == "boolean"){ return "boolean"; }
		if(dLang.isFunction(value)){ return "function"; }
		if(dLang.isArray(value)){ return "array"; } // typeof [] == "object"
		if(value instanceof Date) { return "date"; } // assume timestamp
		if(value instanceof dURL){ return "url"; }
		return "object";
	};

	/**
  	 * @public
  	 * @function
 	 * @name idx.util.typeOfObject
 	 * @description Provides conversion of one object (or string) to another type based on
 	 *              the locally-scoped "str2obj" function from the "dojo/parser" module.
 	 * @param {String|Any} value The value to be converted.
 	 * @param {String} type The Type to convert the object to ("string", "number", "boolean", 
 	 *                      "function", "array", "date", or "url").  If not provided then 
 	 *                      string values are parsed as JSON and non-string value are returned as-is.
 	 * 
 	 * @return {Object} The converted value of the object according to the specified "type".
 	 */
	iUtil.parseObject = function(/*Object*/ value, /*String*/ type){
		var lastIndex = 0;
		// summary:
		//		Convert given string value to given type
		switch(type){
			case "regex": 
				value = "" + value;
				lastIndex = value.lastIndexOf('/');
				if ((value.length>2) && (value.charAt(0) == '/') && (lastIndex > 0)) {
					return new RegExp(value.substring(1,lastIndex), 
									  ((lastIndex==value.length-1)?undefined:value.substring(lastIndex+1)));
				} else {
					return new RegExp(value);
				}
				break;
			case "null":
				return null;
			case "undefined":
				return undefined;
			case "string":
				return "" + value;
			case "number":
				if (typeof value == "number") return value;
				return value.length ? Number(value) : NaN;
			case "boolean":
				// for checked/disabled value might be "" or "checked".  interpret as true.
				return (typeof value == "boolean") ? value : !(value.toLowerCase()=="false");
			case "function":
				if(dLang.isFunction(value)){
					// IE gives us a function, even when we say something like onClick="foo"
					// (in which case it gives us an invalid function "function(){ foo }"). 
					//  Therefore, convert to string
					value=value.toString();
					value=dLang.trim(value.substring(value.indexOf('{')+1, value.length-1));
				}
				try{
					if(value === "" || value.search(/[^\w\.]+/i) != -1){
						// The user has specified some text for a function like "return x+5"
						return new Function(value);
					}else{
						// The user has specified the name of a function like "myOnClick"
						// or a single word function "return"
						return dLang.getObject(value, false) || new Function(value);
					}
				}catch(e){ return new Function(); }
			case "array":
				if (dLang.isArray(value)) return value;
				return value ? value.split(/\s*,\s*/) : [];
			case "date":
				if (value instanceof Date) return value;
				switch(value){
					case "": return new Date("");	// the NaN of dates
					case "now": return new Date();	// current date
					default: {
						return dDateStamp.fromISOString(value);
					}
				}
			case "url":
				if(value instanceof dURL){ return value; }
				return dKernel.baseUrl + value;
			default:
				if (iUtil.typeOfObject(value) == "string") {
					return dJson.parse(value);
				} else {
					return value;
				}
		}
	};

	/**
  	 * @public
  	 * @function
 	 * @name idx.util.getCSSOptions
 	 * @description Creates a temporary div as a child of the optional parent (otherwise the body node),
 	 *              applies the specified CSS class to the div and extracts the query-string from the 
 	 *              background image of the created div to determine the CSS options.  The parameters in
 	 *              query string are converted to specific object types according to the optionally specified
 	 *              "guide" object which is checked for attributes with names in common with the provided 
 	 *              parameters (see idx.util.mixin).  This conversion is done in much the same way Dojo 1.6 
 	 *              parser converted string attribute values to the proper type for the created widget's attributes.
 	 * @param {String} className The CSS class name to use for getting the options.
 	 * @param {Node} parentNode The optional (but recommended) parent node parameter for the scope in which
 	 *                          to create the temporary node.
 	 * @param {Object} guide The optional guide to help in converting the CSS options to objects. 
 	 * 
 	 * @return {Object} The Object containing the CSS options with the attribute types matching the
 	 *                  specified guide where applicable.
 	 */
    iUtil.getCSSOptions = function(/*String*/  className,
                                   /*Node?*/   parentNode,
                                   /*Object?*/ guide,
                                   /*Object?*/ fallback) {
            var body = dWindow.body();
            if ((! parentNode) || (("canHaveHTML" in parentNode) && (! parentNode.canHaveHTML))) {
                parentNode = body;
            }
            
            // determine if the parent node is rooted in the body
            var rooted = false;
            var trav = parentNode;
            while (trav && (!rooted)) {
            	if (trav === body) rooted = true;
            	trav = trav.parentNode;
            }
            var root = null;
			if (!rooted) {
				trav = parentNode;
				var classes = [];
				while (trav) {
					classes.push(dDomAttr.get(trav, "class"));
					trav = trav.parentNode;
				}
				classes.reverse();
				var root = dDomConstruct.create("div",{style: "visibility: hidden; display: none;"},body,"last");
				var trav = root; 
				for (var index = 0; index < classes.length; index++) {
					var classAttr = classes[index];
					if (classAttr) classAttr = dString.trim(classAttr);
					if (classAttr && classAttr.length == 0) classAttr = null;
					var attrs = (classAttr) ? {"class": classAttr} : null;
					trav = dDomConstruct.create("div", attrs, trav, "last");
				}					
				parentNode = trav;
			}
			            
            var optionElem = dDomConstruct.create("div", null, parentNode);
        	dDomClass.add(optionElem, className);
        	var myStyle = dDomStyle.getComputedStyle(optionElem);
        	var bgImage = null;
        	if (myStyle) {
        		bgImage = "" + myStyle.backgroundImage;
        	}
        	dDomConstruct.destroy(optionElem);
            if (root) dDomConstruct.destroy(root);
            
            var noURL = ((! bgImage)
            			  || (bgImage.length < 5) 
            			  || (bgImage.toLowerCase().substring(0, 4) != "url(")
            			  || (bgImage.charAt(bgImage.length - 1) != ")"));
            var options = null;
            if (noURL && (bgImage == null || bgImage == "none") && fallback && (!dLang.isString(fallback))) {
            	options = fallback;
            } 
            
            if (! options) {
            	var cssOpts = null;
	            if (noURL && (bgImage == null || bgImage == "none") && fallback && dLang.isString(fallback)) {
            		cssOpts = fallback;
            		
            	} else if (!noURL) {
		            // remove the "url(" prefix and ")" suffix
    		        bgImage = bgImage.substring(4, bgImage.length - 1);
            
        			// check if our URL is quoted
	        	    if (bgImage.charAt(0) == "\"") {
    	        	    // if not properly quoted then we don't parse it
        	        	if (bgImage.length < 2) return null;
            	    	if (bgImage.charAt(bgImage.length - 1) != "\"") return null;
                
	                	// otherwise remove the quotes
    	            	bgImage = bgImage.substring(1, bgImage.length - 1);
            		}
            
	        		// find the query string
    	        	var queryIdx = bgImage.lastIndexOf("?");
            		var slashIdx = bgImage.lastIndexOf("/");
            		if (queryIdx < 0) return null;
            
        			// get just the query string from the URL
            		cssOpts = bgImage.substring(queryIdx + 1, bgImage.length);
            	}         
            	if (cssOpts == null) return null;
            	if (cssOpts.length == 0) return null;
            
        		// parse the query string and return the result
            	options = dIOQuery.queryToObject(cssOpts);
            }
            return (guide) ? iUtil.mixin({}, options, guide) : options;
        };
        
	/**
  	 * @public
  	 * @function
 	 * @name idx.util.mixinCSSDefaults
 	 * @description Obtains the "CSS Options" using idx.util.getCSSOptions via the specified CSS class name
 	 *              and optional parent node using the specified target object as the "guide" and mixing the
 	 *              CSS options directly into the target object via idx.util.mixin.  Any CSS options not 
 	 *              matching an attribute of the target object are ignored.
	 * 	 
 	 * @param {Object} target The target object to mix the CSS defaults into.
 	 * @param {String} className The CSS class name passed to idx.util.getCSSOptions.
 	 * @param {Node} parentNode The optional (but recommended) parent node passed to idx.util.getCSSOptions.
 	 * 
 	 * @return {Object} The Object containing the CSS options with the attribute types matching the
 	 *                  specified guide where applicable.
 	 */
    iUtil.mixinCSSDefaults = function(/*Object*/ target,
                                      /*String*/ className,
                                      /*Node?*/  parentNode) {
            if (!target) return null;
            var opts = iUtil.getCSSOptions(className, parentNode);

            if (!opts) return null;
            
            iUtil.mixin(target, opts);
            
            return opts;
        };

	/**
  	 * @public
  	 * @function
 	 * @name idx.util.mixin
 	 * @description Provides a restricted version of dojo.lang.mixin.  This function Mixes the attributes of the 
 	 *              specified "source" into the specified "target" but only if the specified target object (or
 	 *              optionally specified "guide" object) already has the attribute from the source object.  Further,
 	 *              the type of each attribute that is mixed in is interpretted to match the type of the same 
 	 *              attribute in the target (or rather the "guide" object if provided).  If no guide is specified, 
 	 *              then the target object is used as a guide.  If a guide is specified the the target object simply
 	 *              becomes the landing zone for the mixed-in attributes.
	 * 	 
 	 * @param {Object} target The target object to mix the attributes into.
 	 * @param {Object} source The source object to pull the attributes from.
 	 * @param {Object} guide The optional object whose attributes and types of those attributes will be used 
 	 *                       as a guide for converting the source attributes to the target attribute.
 	 * @return The specified target object is returned.
 	 */
     iUtil.mixin = function(/*Object*/  target,
                            /*Object*/  source,
                            /*Object?*/ guide) {
     	if (!target) return null; 	// cannot mixin to null
        if (!source) return target;	// if nothing to mixin then do nothing
        if (!guide) guide = target;	// if no guide is specified then use the target as the guide
        
        var src = { };
        // if we have the class info, then parse the fields of the options
        for (var field in source) {
	       	if (! (field in guide)) continue;
    	        var attrType = iUtil.typeOfObject(guide[field]);
                src[field] = iUtil.parseObject(source[field], attrType);
        }
            
         // mixin the options
         dLang.mixin(target, src);
         return target;
     };
        
    /**
  	 * @public
  	 * @function
 	 * @name idx.util.recursiveMixin
 	 * @description Recursively mixes in the second specified object into the first, optionally using
 	 *              the specifed options.  Recursion occurs when the attribute is contained in both
 	 *              the first and second object and is of type "object" in both cases.  Recursion can
 	 *              be made optional via the "options" parameter by specifying the name of a "controlField"
 	 *              and "controlValue".  In such cases the first object is checked for the presence of the
 	 *              "controlField" and if it exists and the value is equal to the specified "controlValue"
 	 *              then recusion occurs, otherwise it does not.  When recursion does not occur an an object
 	 *              value from the second object is copied it may optionally be cloned by setting "options.clone"
 	 *              to true.
     * @param {Object} first The object the attributes will be mixed into
     * @param {Object} second The object that holds the attributes to mixin
     * @param {Objet} options (includes "clone", "controlField" and "controlValue").  If "clone" is 
     *                        specified then attributes whose values are of type object in the second 
     *                        object are cloned before being set in the first object.  The "controlField"
     *                        and "controlValue" options are used to determine if an object in the first
     *                        object should be recursively mixed in.  If "controlField" is provided, but
     *                        not "controlValue" then "controlValue" is defaulted to true.
     */
     iUtil.recursiveMixin = function(first, second, options) {
        	var clone = null;
        	var controlField = null;
        	var controlValue = null;
        	if (options) {
        		clone = options.clone;
        		controlField = options.controlField;
        		if ("controlValue" in options) {
        			controlValue = options.controlValue;
        		} else {
        			controlValue = true;
        		}
        	}
        	
        	for (field in second) {
        		if (field in first) {
        			// get the field values
        			var firstValue = first[field];
        			var secondValue = second[field];
        			
        			// get the types for the values
        			var firstType = iUtil.typeOfObject(firstValue);
        			var secondType = iUtil.typeOfObject(secondValue);

        			// check if they are not the same type
        			if ((firstType == secondType) && (firstType == "object")
        				&& ((!controlField) || (firstValue[controlField] == controlValue))) {
        				// if both are objects then mix the second into the first
        				iUtil.recursiveMixin(firstValue, secondValue, options);
        				
        			} else {
        				// otherwise overwrite the first with the second
        				first[field] = (clone) ? dLang.clone(secondValue) : secondValue;
        			}
        		} else {
        			first[field] = (clone) ? dLang.clone(second[field]) : second[field];
        		}
        	}
        };
        
        
    /**
  	 * @public
  	 * @function
 	 * @name idx.util.nullify
 	 * @description Accepts a target object, an object that represents arguments passed
     *              to construct the target object (usually via mixin). and an
     *              and an array of property names for which the value should be null if
     *              not otherwise specified.  For each of the specified properties, if 
     *              the construction arguments does not specify a value for that property, 
     *              then the same property is set to null on the target object.  If a 
     *              property name is found not to exist in the target object then it is 
     *              ignored.
     *
     * @param {Object} target Usually the object being constructed.
     * 
     * @param {Object} ctorArgs The objects that would specify attributes on the target.
     * 
     * @param {Object} props The array of property names for properties to be set to null
     *                       if none of the objects in the argsArray specify them.
     */
    iUtil.nullify = function(target,ctorArgs,props) {
        var index = 0;
        for (index = 0; index < props.length; index++) {
            var prop = props[index];
            if (! (prop in target)) continue;
            if ((ctorArgs) && (prop in ctorArgs)) continue;
            target[prop] = null;
        }
    };
        
    /**
  	 * @private
  	 * @function
 	 * @name idx.util._getNodeStyle
     * @description Internal method to get the node's style as an object.  This method does not
     *              normalize the style fields so it will need to be extended to make it more
     *              robust for the general case.  idx.util only needs this for things like "position",
     *              "width" and "height" in order to reset values on a node after changing them.
     */
    iUtil._getNodeStyle = function(node) {
        	var nodeStyle = dDomAttr.get(node, "style");
        	if (!nodeStyle) return null;
        	var result = null;
       		if (iUtil.typeOfObject(nodeStyle) == "string") {
       			result = {};
       			var tokens = nodeStyle.split(";");
       			for (var index = 0; index < tokens.length; index++) {
       				var token = tokens[index];
       				var colonIndex = token.indexOf(":");
       				if (colonIndex < 0) continue;
       				var field = token.substring(0, colonIndex);
       				var value = "";
       				if (colonIndex < token.length - 1) {
       					value = token.substring(colonIndex+1);
       				}
       				result[field] = value;
       			}
     		} else {
     			result = nodeStyle;
     		}
       		return result;
        };

    /**
  	 * @private
  	 * @function
 	 * @name idx.util._getNodePosition
     * @description Internal method to get the node's specific position and detect when none is specifically
     *              assigned to the node.
     */
    iUtil._getNodePosition = function(node) {
        	var style = iUtil._getNodeStyle(node);
        	if (! style) return "";
        	if (! style.position) return "";
        	return style.position;
    };
        
    /**
  	 * @public
  	 * @function
 	 * @name idx.util.fitToWidth
     * @description Sizes a parent to fit the child node as if the child node's positioning was
     *              NOT absolute.  Absolutely positioned elements due not "reserve" space, so this
     *              method will temporarily position the element as "static", then determine the
     *              result size of the parent, set the parent's width explicitly, and then return the
     *              child to the default previously set positioning.  This is especially handy in that
     *              it allows the parent to define padding which will be respected.
     *
     * @param {Node} parent The parent node -- no checking is done to ensure this node is actually 
     *                      a parent of the specified child.
     * @param {Node} child The child node -- no checking is done to ensure this node is actually a
     *                     child of the specified parent.
     */
    iUtil.fitToWidth = function(/*Node*/ parent, /*Node*/ child) {
        	var pos = iUtil._getNodePosition(child);
            dDomStyle.set(parent, {width: "auto"});
            dDomStyle.set(child, {position: "static"});
            var dim = dDomGeo.getMarginBox(parent);
            dDomStyle.set(parent, {width: dim.w + "px"});
            dDomStyle.set(child, {position: pos});  
            return dim;
    };
        
    /**
  	 * @public
  	 * @function
 	 * @name idx.util.fitToHeight
 	 * @description Sizes a parent to fit the child node as if the child node's positioning was
 	 *              NOT absolute.  Absolutely positioned elements due not "reserve" space, so 
 	 *              this method will temporarily position the element as "static", then determine
 	 *              the result size of the parent, set the parent's height explicitly, and then 
 	 *              return the child to the default previously set positioning.  This is especially
 	 *              handy in that it allows the parent to define padding which will be respected.
     *
     * @param {Node} parent The parent node -- no checking is done to ensure this node is actually 
     *                      a parent of the specified child.
	 * @param {Node} child The child node -- no checking is done to ensure this node is actually a
	 *                     child of the specified parent.
 	 */
    iUtil.fitToHeight = function(/*Node*/ parent, /*Node*/ child) {
        	var pos = iUtil._getNodePosition(child);
            dDomStyle.set(parent, {height: "auto"});
            dDomStyle.set(child, {position: "static"});
            var dim = dDomGeo.getMarginBox(parent);
            dDomStyle.set(parent, {height: dim.h + "px"});
            dDomStyle.set(child, {position: pos});  
            return dim;
    };
       
    /**
  	 * @public
  	 * @function
 	 * @name idx.util.fitToSize
 	 * @description Sizes a parent to fit the child node as if the child node's positioning was 
 	 *              NOT absolute.  Absolutely positioned elements due not "reserve" space, so this
 	 *              method will temporarily position the element as "static", then determine the 
 	 *              result size of the parent, set the parent's size explicitly, and then return the
	 *              child to the default previously set positioning.  This is especially handy in 
	 *              that it allows the parent to define padding which will be respected.
	 * 
     * @param {Node} parent The parent node -- no checking is done to ensure this node is actually 
     *                      a parent of the specified child.
     *
     * @param {Node} child The child node -- no checking is done to ensure this node is actually a 
     *                     child of the specified parent.
     */
    iUtil.fitToSize = function(/*Node*/ parent, /*Node*/ child) {
        	var pos = iUtil._getNodePosition(child);
            dDomStyle.set(parent, {width: "auto", height: "auto"});
            dDomStyle.set(child, {position: "static"});
            var dim = dDomGeo.getMarginBox(parent);
            dDomStyle.set(parent, {width: dim.w + "px", height: dim.h + "px"});
            dDomStyle.set(child, {position: pos});
            return dim;
        };
     
     /**
      * @public
  	  * @function
 	  * @name idx.util.getStaticSize
 	  * @description Determines the  dimensions of the specified node if it were to use static positioning.
      *
      * @param {Node} node The node to work with.
      */  
     iUtil.getStaticSize = function(/*Node*/ node) {
        	var style = iUtil._getNodeStyle(node);
        	var pos = (style && style.position) ? style.position: "";
            var width  = (style && style.width) ? style.width : "";
            var height = (style && style.height) ? style.height : "";
            dDomStyle.set(node, {position: "static", width: "auto", height: "auto"});
            var dim = dDomGeo.getMarginBox(node);
            dDomStyle.set(node, {position: pos, width: width, height: height});
            return dim;
        };
        
     /**
      * @public
  	  * @function
 	  * @name idx.util.reposition
 	  * @description Determines the  dimensions of the specified node if it were to use static positioning.
      *
      * @param {Node} node The node to work with.
      * @param {String} position The CSS position to use.
      */  
     iUtil.reposition = function(/*Node*/ node, /*String*/ position) {
        	var oldpos = iUtil._getNodePosition(node);
            dDomStyle.set(node, {position: position});
            return oldpos;
     };

		        
     /**
      * @public
  	  * @function
 	  * @name idx.util.getParentWidget
 	  * @description Determines the widget that is the parent of the specified widget or node.  This 
 	  *              is determined by obtaining the parent node for the specified node or "widget.domNode"
 	  *              and then calling dijit.getEnclosingWidget().  This method may return null if no widget
 	  *              parent exists.
      *
      * @param {Node|Widget} child The node or dijit._WidgetBase child for which the parent is being requested.
      * @param {Type} widgetType The optional type of widget for the parent.  The method recursively looks for 
      *                          the first ancestor of this type until found or we run out of ancestors.
      *
      * @return {Widget} The parent or ancestor widget.
      */  
     iUtil.getParentWidget = function(/*Node|Widget*/ child,
     								  /*Type*/        widgetType) {
            // get the widget node
            var childNode = (child instanceof dWidget) ? child.domNode : child;
            
            // get the parent node of the DOM node
            var parentNode = childNode.parentNode;
            
            // check the parent node
            if (parentNode == null) return null;
            
            // get the widget for the node
            var parent = dRegistry.getEnclosingWidget(parentNode);
            
            // check if looking for a specific widget type
            while ((widgetType) && (parent) && (! (parent instanceof widgetType))) {
            	parentNode = parent.domNode.parentNode;
            	parent = null;
            	if (parentNode) {
            		parent = dRegistry.getEnclosingWidget(parentNode);
            	}
            } 
            
            // return the parent
            return parent;
    };

     /**
      * @public
  	  * @function
 	  * @name idx.util.getSiblingWidget
 	  * @description Determines the widget that is the first next or previous sibling of the 
 	  *              specified widget or node that is a widget (optionally of a specific type).  This
 	  *              is determined by obtaining the parent node for the specified node or "widget.domNode",
 	  *              finding the first next or previous sibling node that is the domNode for a widget, and
 	  *              if not moving on to the next.  This method may return null if no widget sibling exists.
      *
      * @param {Node|Widget} target The node or dijit._WidgetBase for which the sibling is being requested.
      * @param {Boolean} previous Optional parameter for indicating which sibling.  Specify true if the 
      *                           previous sibling is desired and false if the next sibling.  If omitted then
      *                           false is the default.
      * @param {Type} widgetType The optional type of widget for the sibling.  If specified, this method ignores
      *                          any sibling that is not of the specified type.
      *
      * @return {Widget} The sibling widget or null if no widget matching the criteria is found..
      */  
    iUtil.getSiblingWidget = function(/*Node|Widget*/ target, 
        	 						  /*Boolean*/     previous,
        							  /*Type*/        widgetType) {        	
            // get the widget node
            var widgetNode = (target instanceof dWidget) ? target.domNode : target;
            
            // get the parent node of the DOM node
            var parentNode = widgetNode.parentNode;
            
            // check the parent node
            if (parentNode == null) return null;
            
            // get the children of the parent
            var children = parentNode.childNodes;
            if (! children) return null;
            
            // find the index for the child
            var index = 0;
            for (index = 0; index < children.length; index++) {
            	if (children[index] == widgetNode) break;
            }
            
            if (index == children.length) return null;
            
            // work forward are backward from the index
            var step = (previous) ? -1: 1;
            var limit = (previous) ? -1 : children.length;
            var sibindex = 0;
            var sibling  = null;
            for (sibindex = (index + step); sibindex != limit; (sibindex += step)) {
            	var sibnode = children[sibindex];
            	
            	// get the widget for the node
                var sibwidget = dRegistry.getEnclosingWidget(sibnode);
                if (! sibwidget) continue;
                if (sibwidget.domNode == sibnode) {
                	if ((!widgetType) || (sibwidget instanceof widgetType)) {
                		sibling = sibwidget;
                		break;
                	}
                }
            }
            
            // return the sibling
            return sibling;
    };
        

     /**
      * @public
  	  * @function
 	  * @name idx.util.getChildWidget
 	  * @description Determines the widget that is the first or last child of the specified widget 
 	  *              or node that is a widget (optionally of a specified type).  This is determined
 	  *              by obtaining the widget for the specified parent, obtaining the children widgets
 	  *              and returning either the first or last that is optionally of a specified type.
      *              This method may return null if no widget child exists.
      *
      * @param {Node|Widget} parent The node or dijit._WidgetBase for which the child is being requested.
      * @param {Boolean} last Optional parameter for indicating which child.  Specify true if the 
      *                       last child is desired and false if the first child.  If omitted then
      *                       false is the default.
      * @param {Type} widgetType The optional type of widget for the child.  If specified, this method ignores
      *                          any child that is not of the specified type.
      *
      * @return {Widget} The child widget or null if no widget matching the criteria is found.
      */  
     iUtil.getChildWidget = function(/*Node|Widget*/ parent, 
        								   /*Boolean*/     last,
        								   /*Type*/        widgetType) {
            // get the widget node
        	if (! (parent instanceof dWidget)) {
            	var widget = dRegistry.getEnclosingWidget(parent);
            	if (widget) parent = widget;
            }

            var children = null;
            if (parent instanceof dWidget) {
            	children = parent.getChildren();
            } else {
                children = parent.childNodes;
            }
            
            // check the children
            if (! children) return null;
            if (children.length == 0) return null;

            // setup the looping variables
            var start = (last) ? (children.length - 1) : 0;
            var step  = (last) ? -1 : 1;
            var limit = (last) ? -1 : children.length;

            // work forward are backward from the index
            var childIndex = 0;
            var child      = null;
            for (childIndex = start; childIndex != limit; (childIndex += step)) {
            	var widget = children[childIndex];
            	if (! (widget instanceof dWidget)) {
                	// get the widget for the node
                    var node = widget;
                    widget = dRegistry.getEnclosingWidget(node);
                    if (! widget) continue;
                    if (widget.domNode != node) continue;
            	}
            	if ((!widgetType) || (widget instanceof widgetType)) {
            		child = widget;
            		break;
            	}
            }
            
            // return the child
            return child;
        };

     /**
      * @public
  	  * @function
 	  * @name idx.util.getFormWidget
 	  * @description Determines the widget (derived from dijit.form._FormWidget) that is a child of the 
 	  *              specified node or widget (usually a dijit.form.Form) that has the specified form 
 	  *              field name.  If the found form field is found to be an instance of dijit.form._FormWidget
 	  *              then it is returned, otherwise null is returned.
      *
      * @param {String} formFieldName The name of the form field widget that is being requested.
      * @param {Node|Widget} parent Optional root node or root widget under which to look for the form 
      *                             widgets. If not specified then the entire document body is searched.
      * @return The form field widget with the specified name or null if not found.
      */  
     iUtil.getFormWidget = function(/*String*/ 			formFieldName,
        							/*Node|Widget?*/	parent) {
        	// get the widget node
        	var rootNode = null;
        	if (!parent) {
        		parent = dWindow.body();
        	} else if (parent instanceof dWidget) {
        		rootNode = parent.domNode;
        	} else {
        		rootNode = form;
        	}
        			
        	var formWidget = null;
        	var nodeList = dQuery("[name='" + formFieldName + "']", rootNode);
        	for (var index = 0; (!formWidget) && (index < nodeList.length);index++) {
        		var node = nodeList[index];
        		var widget = dRegistry.getEnclosingWidget(node);
        		if (!widget) continue;
        		if (! (widget instanceof dFormWidget)) {
        			continue;
        		}
        		var name = widget.get("name");
        		if (name != formFieldName) {
        			continue;
        		}
        		formWidget = widget;
        	}
        	return formWidget;
   };
        
        
     /**
      * @public
  	  * @function
 	  * @name idx.util.isNodeOrElement
 	  * @description Attempts to determine if the specified object is a DOM node.  This is needed since IE does
 	  *              not recognize the "Node" type, and only IE-8 recognizes the "Element" type.
      *
      * @param {Object|Node|Element} obj The object to check.
      * @return The result is true if the specified object is a node or element, otherwise false.
      */  
     iUtil.isNodeOrElement = function(/*Object*/ obj) {
         return ((obj.parentNode) && (obj.childNodes) && (obj.attributes)) ? true : false;
     };

     /**
      * @public
  	  * @function
 	  * @name idx.util.debugObject
 	  * @description Generates a diagnostic string describing the specified object.
      *
      * @param {Object} obj The object to debug
      * @return The debug string for the object.
      */  
     iUtil.debugObject = function(/*Object*/ obj) {
        	return iUtil._debugObject(obj, "/", [ ]);
     };
        
     /**
      * @private
  	  * @function
 	  * @name idx.util._debugObject
 	  * @description Recursively generates a diagnostic string describing the specified object
 	  *              and tries to detect circular references.
      *
      * @param {Object} obj The object to debug
      * @param {String} path The current attribute path within the object.
      * @param {Object} seen The associative array of paths to boolean values to check if already seen in 
      *                      order to avoid circular recursion.
      * @return The debug string for the object.
      */  
     iUtil._debugObject = function(/*Object*/ obj, /*String*/ path, /*Array*/ seen) {
           if (obj === null) return "null";
           var objType = iUtil.typeOfObject(obj);
           switch (objType) {
           case 'object':
        	   for (var index = 0; index < seen.length; index++) {
        		   if (seen[index].obj == obj) {
        			   return "CIRCULAR_REFERENCE[ " + seen[index].path + " ]";
        		   }
        	   }
        	   seen.push({obj: obj, path: path});
        	   var result = "{ ";
        	   var prefix = "";
        	   for (field in obj) {
        	     result = (result + prefix + '"' + field + '": ' 
        	    		   + iUtil._debugObject(obj[field], 
        	    				   				   path + '/"' + field + '"', 
        	    				   				   seen));
        	     prefix = ", ";
        	   }
        	   result = result + " }";
        	   return result;
           case 'date':
        	   return "DATE[ " + dDateStamp.toISOString(obj) + " ]";
           default:
        	   return dJson.stringify(obj);
           }
        };
     
     /**
      * @public
  	  * @field
 	  * @name idx.util.isBrowser
 	  * 
 	  * @description Equivalent to "dojo.isBrowser" in Dojo 1.6 and dojo/has("host-browser") in Dojo 1.7+
 	  * @deprecated Use dojo/has("host-browser") instead.
 	  */
      iUtil.isBrowser = dHas("host-browser");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isIE
 	  * 
 	  * @description Equivalent to "dojo.isIE" in Dojo 1.6 and dojo/has("ie") in Dojo 1.7+
 	  * @deprecated Use dojo/has("ie") instead.
 	  */
      iUtil.isIE = dHas("ie");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isFF
 	  * 
 	  * @description Equivalent to "dojo.isFF" in Dojo 1.6 and dojo/has("ff") in Dojo 1.7+
 	  * @deprecated Use dojo/has("ff") instead.
 	  */
      iUtil.isFF = dHas("ff");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isSafari
 	  * 
 	  * @description Equivalent to "dojo.isSafari" in Dojo 1.6 and dojo/has("safari") in Dojo 1.7+
 	  * @deprecated Use dojo/has("safari") instead.
 	  */
      iUtil.isSafari = dHas("safari");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isChrome
 	  * 
 	  * @description Equivalent to "dojo.isChrome" in Dojo 1.6 and dojo/has("chrome") in Dojo 1.7+
 	  * @deprecated Use dojo/has("chrome") instead.
 	  */
      iUtil.isChrome = dHas("chrome");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isMozilla
 	  * 
 	  * @description Equivalent to "dojo.isMozilla" in Dojo 1.6 and dojo/has("mozilla") in Dojo 1.7+
 	  * @deprecated Use dojo/has("mozilla") instead.
 	  */
      iUtil.isMozilla = dHas("mozilla");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isOpera
 	  * 
 	  * @description Equivalent to "dojo.isOpera" in Dojo 1.6 and dojo/has("opera") in Dojo 1.7+
 	  * @deprecated Use dojo/has("opera") instead.
 	  */
      iUtil.isOpera = dHas("opera");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isKhtml
 	  * 
 	  * @description Equivalent to "dojo.isKhtml" in Dojo 1.6 and dojo/has("khtml") in Dojo 1.7+
 	  * @deprecated Use dojo/has("khtml") instead.
 	  */
      iUtil.isKhtml	= dHas("khtml");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isAIR
 	  * 
 	  * @description Equivalent to "dojo.isAIR" in Dojo 1.6 and dojo/has("air") in Dojo 1.7+
 	  * @deprecated Use dojo/has("air") instead.
 	  */
      iUtil.isAIR = dHas("air");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isQuirks
 	  * 
 	  * @description Equivalent to "dojo.isQuirks" in Dojo 1.6 and dojo/has("quirks") in Dojo 1.7+
 	  * @deprecated Use dojo/has("quirks") instead.
 	  */
      iUtil.isQuirks = dHas("quirks");
      
     /**
      * @public
  	  * @field
 	  * @name idx.util.isWebkit
 	  * 
 	  * @description Equivalent to "dojo.isWebkit" in Dojo 1.6 and dojo/has("webkit") in Dojo 1.7+
 	  * @deprecated Use dojo/has("webkit") instead.
 	  */
      iUtil.isWebKit = dHas("webkit");
        

        // idx.util.fromJson is copied from IBM Dojo Toolkit 1.7.x implementation of dojox.secure.fromJson
        // because that module lacks AMD support in Dojo 1.7 and early releases of Dojo 1.8.  The original 
        // implementation of dojox.secure.fromJson includes the following notice:
        //
        // Used with permission from Mike Samuel of Google (has CCLA), from the json-sans-eval project:
        // http://code.google.com/p/json-sans-eval/
        //	Mike Samuel <mikesamuel_at_gmail_dot_com>

     /**
      * @public
  	  * @function
 	  * @name idx.util.fromJson
 	  * @description Generates a diagnostic string describing the specified object.  Parses a string 
 	  *              of well-formed JSON text without exposing the application to cross-site request
 	  *              forgery attacks.  Use this in place of dojo.fromJson().  The code is copied from 
 	  *              IBM Dojo Toolkit 1.7.x implementation of dojox.secure.fromJson module which lacks 
 	  *              support for AMD as of Dojo 1.7 & Dojo 1.8 RC1.
      * 
      *              Parses a string of well-formed JSON text. If the input is not well-formed, then behavior
      *              is undefined, but it is deterministic and is guaranteed not to modify any object other 
      *              than its return value.
      *
	  * 			 This does not use `eval` so is less likely to have obscure security bugs than json2.js.
	  *              It is optimized for speed, so is much faster than json_parse.js.
	  *
	  *              This library should be used whenever security is a concern (when JSON may
	  *              come from an untrusted source), speed is a concern, and erroring on malformed
      *              JSON is *not* a concern.
	  *	
	  *              json2.js is very fast, but potentially insecure since it calls `eval` to
	  *              parse JSON data, so an attacker might be able to supply strange JS that
      *              looks like JSON, but that executes arbitrary javascript.
      * 
	  * @param {String} json JSON text per RFC 4627
	  * 
	  * @param {Function (this:Object, string, *)} optReviver Optional function that reworks JSON objects post-parse
	  *                                                       per Chapter 15.12 of EcmaScript3.1.  If supplied, the
	  *                                                       function is called with a string key, and a value.
	  * 													  The value is the property of 'this'.	The reviver should
	  *                                                       return the value to use in its place.	So if dates were
	  *                                                       serialized as {@code { "type": "Date", "time": 1234 }},
	  *                                                       then a reviver might look like 
	  *	  {@code 
	  *			function (key, value) {
      * 			if (value && typeof value === 'object' && 'Date' === value.type) {
	  *				return new Date(value.time);
	  *			} else {
	  * 				return value;
	  *			}
	  * 		}}.
	  * 		If the reviver returns {@code undefined} then the property named by key
	  *		will be deleted from its container.
	  *		{@code this} is bound to the object containing the specified property.
	  * 
	  * @return {Object|Array} representing the parsed object.
      */
     iUtil.fromJson = typeof JSON != "undefined" ? JSON.parse :
        	(function () {
        		var number
        			= '(?:-?\\b(?:0|[1-9][0-9]*)(?:\\.[0-9]+)?(?:[eE][+-]?[0-9]+)?\\b)';
        		var oneChar = '(?:[^\\0-\\x08\\x0a-\\x1f\"\\\\]'
        			+ '|\\\\(?:[\"/\\\\bfnrt]|u[0-9A-Fa-f]{4}))';
        		var string = '(?:\"' + oneChar + '*\")';

        		// Will match a value in a well-formed JSON file.
        		// If the input is not well-formed, may match strangely, but not in an unsafe
        		// way.
        		// Since this only matches value tokens, it does not match whitespace, colons,
        		// or commas.
        		var jsonToken = new RegExp(
        				'(?:false|true|null|[\\{\\}\\[\\]]'
        				+ '|' + number
        				+ '|' + string
        				+ ')', 'g');

        		// Matches escape sequences in a string literal
        		var escapeSequence = new RegExp('\\\\(?:([^u])|u(.{4}))', 'g');

        		// Decodes escape sequences in object literals
        		var escapes = {
        				'"': '"',
        				'/': '/',
        				'\\': '\\',
        				'b': '\b',
        				'f': '\f',
        				'n': '\n',
        				'r': '\r',
        				't': '\t'
        		};
        		function unescapeOne(_, ch, hex) {
        			return ch ? escapes[ch] : String.fromCharCode(parseInt(hex, 16));
        		}

        		// A non-falsy value that coerces to the empty string when used as a key.
        		var EMPTY_STRING = new String('');
        		var SLASH = '\\';

        		// Constructor to use based on an open token.
        		var firstTokenCtors = { '{': Object, '[': Array };

        		var hop = Object.hasOwnProperty;

        		return function (json, opt_reviver) {
        			// Split into tokens
        			var toks = json.match(jsonToken);
        			// Construct the object to return
        			var result;
        			var tok = toks[0];
        			var topLevelPrimitive = false;
        			if ('{' === tok) {
        				result = {};
        			} else if ('[' === tok) {
        				result = [];
        			} else {
        				// The RFC only allows arrays or objects at the top level, but the JSON.parse
        				// defined by the EcmaScript 5 draft does allow strings, booleans, numbers, and null
        				// at the top level.
        				result = [];
        				topLevelPrimitive = true;
        			}

        			// If undefined, the key in an object key/value record to use for the next
        			// value parsed.
        			var key;
        			// Loop over remaining tokens maintaining a stack of uncompleted objects and
        			// arrays.
        			var stack = [result];
        			for (var i = 1 - topLevelPrimitive, n = toks.length; i < n; ++i) {
        				tok = toks[i];

        			var cont;
        			switch (tok.charCodeAt(0)) {
        			default:	// sign or digit
        				cont = stack[0];
        				cont[key || cont.length] = +(tok);
        				key = void 0;
        				break;
        			case 0x22:	// '"'
        				tok = tok.substring(1, tok.length - 1);
        				if (tok.indexOf(SLASH) !== -1) {
        					tok = tok.replace(escapeSequence, unescapeOne);
        				}
        				cont = stack[0];
        				if (!key) {
        					if (cont instanceof Array) {
        						key = cont.length;
        					} else {
        						key = tok || EMPTY_STRING;	// Use as key for next value seen.
        						break;
        					}
        				}
        				cont[key] = tok;
        				key = void 0;
        				break;
        			case 0x5b:	// '['
        				cont = stack[0];
        				stack.unshift(cont[key || cont.length] = []);
        				key = void 0;
        				break;
        			case 0x5d:	// ']'
        				stack.shift();
        				break;
        			case 0x66:	// 'f'
        				cont = stack[0];
        				cont[key || cont.length] = false;
        				key = void 0;
        				break;
        			case 0x6e:	// 'n'
        				cont = stack[0];
        				cont[key || cont.length] = null;
        				key = void 0;
        				break;
        			case 0x74:	// 't'
        				cont = stack[0];
        				cont[key || cont.length] = true;
        				key = void 0;
        				break;
        			case 0x7b:	// '{'
        				cont = stack[0];
        				stack.unshift(cont[key || cont.length] = {});
        				key = void 0;
        				break;
        			case 0x7d:	// '}'
        				stack.shift();
        				break;
        			}
        		}
        		// Fail if we've got an uncompleted object.
        		if (topLevelPrimitive) {
        			if (stack.length !== 1) { throw new Error(); }
        			result = result[0];
        		} else {
        			if (stack.length) { throw new Error(); }
        		}

        		if (opt_reviver) {
        			// Based on walk as implemented in http://www.json.org/json2.js
        			var walk = function (holder, key) {
        				var value = holder[key];
        				if (value && typeof value === 'object') {
        					var toDelete = null;
        					for (var k in value) {
        						if (hop.call(value, k) && value !== holder) {
        							// Recurse to properties first.	This has the effect of causing
        							// the reviver to be called on the object graph depth-first.

        							// Since 'this' is bound to the holder of the property, the
        							// reviver can access sibling properties of k including ones
        							// that have not yet been revived.

        							// The value returned by the reviver is used in place of the
        							// current value of property k.
        							// If it returns undefined then the property is deleted.
        							var v = walk(value, k);
        							if (v !== void 0) {
        								value[k] = v;
        							} else {
        								// Deleting properties inside the loop has vaguely defined
        								// semantics in ES3 and ES3.1.
        								if (!toDelete) { toDelete = []; }
        								toDelete.push(k);
        							}
        						}
        					}
        					if (toDelete) {
        						for (var i = toDelete.length; --i >= 0;) {
        							delete value[toDelete[i]];
        						}
        					}
        				}
        				return opt_reviver.call(holder, key, value);
        			};
        			result = walk({ '': result }, '');
        		}

        		return result;
        		};
        	})();
        
     /**
      * @private
  	  * @function
 	  * @name idx.util._getFontMeasurements
 	  * @description Internal function for computing the measurements of on-screen fonts.
 	  */ 
     function _getFontMeasurements(measurements,fontSize){
    		// initialize heights to the specified fontMeasurements parameter
    		var heights = measurements;
    		
    		// if it does not yet exist then create it
    		if (!heights) {
    			heights = {
    				'1em': -1, '1ex': -1, '100%': -1, '12pt': -1, '16px': -1, 'xx-small': -1,
	    			'x-small': -1, 'small': 1, 'medium': -1, 'large': -1, 'x-large': -1,
    				'xx-large': -1
    			};
    		}
    		
    		// check if a specific font size was requested in addition to defaults
    		if (fontSize && (! (fontSize in heights))) {
    			heights[fontSize] = -1;
    		}
    		
    		var p;
    		if(dHas("ie")){
    			dWindow.doc.documentElement.style.fontSize="100%";
    		}
    		var div = dDomConstruct.create("div", {style: {
    				position: "absolute",
    				left: "0",
    				top: "-100000px",
    				width: "30px",
    				height: "1000em",
    				borderWidth: "0",
    				margin: "0",
    				padding: "0",
    				outline: "none",
    				lineHeight: "1",
    				overflow: "hidden"
    			}}, dWindow.body());
    		for(p in heights){
    			// check if the given font size was already calculated and skip it
    			if (heights[p] >= 0) {
    				continue;
    			}
    			
    			// set the font size
    			dDomStyle.set(div, "fontSize", p);
    			
    			// get the measurement
    			heights[p] = Math.round(div.offsetHeight * 12/16) * 16/12 / 1000;
    		}

    		dWindow.body().removeChild(div);
    		return heights; //object
    };
    
     /**
      * @private
      * @function
      * @name idx.util._toFontSize
      * @description Internal function to convert a node to a font size string.
      * @param fontSizeOrNode The font size as a string or a node.
      */
     function _toFontSize(fontSize) {
     		var fontSizeType = iUtil.typeOfObject(fontSize);
     		
     		if ((fontSizeType == "object")&&(iUtil.isNodeOrElement(fontSize))) {
    			var compStyle = dDomStyle.getComputedStyle(fontSize);
     			fontSize = compStyle["fontSize"];
     		} else if (fontSizeType != "string") {
     			fontSize = "" + fontSize;
     		}
     		return fontSize;
     };
     
     /**
      * @private
  	  * @function
 	  * @name idx.util._getCachedFontMeasurements
 	  * @description Internal function for computing and cacheing the measurements of on-screen fonts.
 	  */ 
	 var fontMeasurements = null;
     function _getCachedFontMeasurements(fontSize,recalculate){
     		var fontSizeType = iUtil.typeOfObject(fontSize);
     		
     		if(fontSizeType == "boolean"){ 
     			recalculate = fontSize;
     			fontSize = null;
     			
     		} else {
     			fontSize = _toFontSize(fontSize);
     		}
     		
     		// if recalculating then clear out old calculations
     		if (fontMeasurements && recalculate) {
     			if (fontSize) {
     				if (fontMeasurements[fontSize]) delete fontMeasurements[fontSize];
     			} else {
     				fontMeasurements = null;
     			}
     		}
     		
    		if(recalculate || !fontMeasurements || (fontSize && !fontMeasurements[fontSize])){
    			fontMeasurements = _getFontMeasurements(fontMeasurements, fontSize);
    		}
    		return fontMeasurements;
	};
   
   		 	
     /**
      * @public
  	  * @function
 	  * @name idx.util.normalizedLength
 	  * @description Converts non-percentage CSS widths from various units to pixels.
 	  * @param len The CSS length with optional units.
 	  * @param fontSizeOrNode The optional font size or node for which the font size is computed (defaults to "12pt").
 	  * @return The length/width in pixels.
 	  */ 
     iUtil.normalizedLength = function(len,fontSize) {
    		if(len.length === 0){ return 0; }
    		if(!fontSize) fontSize = "12pt";
    		else fontSize = _toFontSize(fontSize);
    		if(len.length > 2){
				// we don't want to use the fontSize parameter if the
				// specified length is not font-based units
				var units = len.slice(-2);
				var fontUnits = (units == "em" || units == "ex");
				if (! fontUnits) fontSize = "12pt";     		
    			var fm = _getCachedFontMeasurements(fontSize);
    			var px_in_pt = fm["12pt"] / 12;
    			var px_in_em = fm[fontSize];
    			var px_in_ex = px_in_em / 2;
    			var val = parseFloat(len);
    			switch(len.slice(-2)){
    				case "px": return val;
    				case "em": return val * px_in_em;
    				case "ex": return val * px_in_ex;
    				case "pt": return val * px_in_pt;
    				case "in": return val * 72 * px_in_pt;
    				case "pc": return val * 12 * px_in_pt;
    				case "mm": return val * g.mm_in_pt * px_in_pt;
    				case "cm": return val * g.cm_in_pt * px_in_pt;
    			}
    		}
    		return parseFloat(len);	// Number
    };
	/**
	 * @public
	 * @function
	 * @name idx.util.isPercentage
	 * @description Attempts to determine if the value is a percentage 
	 * @param value to be check 
	 * @return return true if value is a percentage
	 */
	iUtil.isPercentage = function(value){
		return  /^\d+%$/.test(value);
	};
	/**
	 * @public
	 * @function
	 * @name idx.util.includeValidCSSWidth
	 * @description Attempts to determine if the value includes a valid CSS width
	 * @param value to be check
	 * @return return ture if the value includes a valid CSS width
	 */
	iUtil.includeValidCSSWidth = function(value){
		return /width:\s*\d+(%|px|pt|in|pc|mm|cm)/.test(value)
	};
	iUtil.getValidCSSWidth = function(style){
		var styleType = iUtil.typeOfObject(style);
		if (styleType != "string") {
			if ("width" in style) {
				style = "width: " + style.width + ";";
			} else {
				style = "width: " + style + ";";
			}
		}
		return /width:\s*\d+(%|px|pt|in|pc|mm|cm|em|ex)/.test(style) ? 
			style.match(/width:\s*(\d+(%|px|pt|in|pc|mm|cm|em|ex))/)[1] : "";
	};

	/**
	 * Compares to widgets to see if they are identical or alternatively compares
	 * strings to strings or strings to widgets, treating the strings as the ID of
	 * a widget.  The widget for the specified ID need not exist yet in the registry
	 * for this function to compare since it relies on the "id" field of any specified
	 * widget to perform a string compare.
	 *
	 * @param {String|Widget} w1 The first widget or the ID for the first widget.
	 * @param {String|Widget| w2 The second widget or the ID for the second widget.
	 *
	 * @return {boolean} Returns true if equal, otherwise false.
	 */
	iUtil.widgetEquals = function(w1,w2) {
		if (w1 === w2) return true;
		if (!w1 && w2) return false;
		if (w1 && !w2) return false;
		if ((!dLang.isString(w1)) && ("id" in w1)) w1 = w1.id;
		if ((!dLang.isString(w2)) && ("id" in w2)) w2 = w2.id;
		return (w1 == w2);
	};
  
	/**
	 * Provides a way to watch an attribute on an object whether it be an instance of
	 * dojo/Stateful or dijit/_WidgetBase.  The "watch()" method does not work on 
	 * dijit/_WidgetBase instances that implement custom setters.
	 *
	 * @param {Stateful|Widget} obj The object that owns the attribute to watch.
	 * @param {String} attr The attribute name to watch.
	 *
	 * @return {Handle} Returns the handle to remove the watch or null if the attribute
	 *         cannot be watched on the specified object.
	 */
	iUtil.watch = function(obj, attr, func) {
		if (!obj) return null;
		if (!attr) return null;
		if  ((! ("watch" in obj)) || (!dLang.isFunction(obj.watch))) {
			// object is not an instance of dojo/Stateful
			return null;
		}
		attr = dString.trim(attr);
		if (attr.length == 0) return null;
		var uc = attr.charAt(0).toUpperCase();
		if (attr.length > 1) {
			uc = uc + attr.substring(1);
		}
		var funcName = "_set" + uc + "Attr";
		if (funcName in obj) {
			return dAspect.around(obj, funcName, function(origFunc) {
				return function(value) {
					var oldValue = obj.get(attr);
					origFunc.apply(obj, arguments);
					var newValue = obj.get(attr);
					if (oldValue != newValue) {
						func.call(undefined, attr, oldValue, newValue);
					}
				};
			});
		} else {
			return obj.watch(attr, func);
		}
	};
  	
    return iUtil;
});