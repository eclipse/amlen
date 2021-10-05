/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
	"idx/main",
	"dojo/_base/lang",
	"dojo/_base/window",
	"dojo/dom-construct",
	"dojo/dom-geometry"
], function (iMain,
			 dLang,
			 dWindow,
			 dDomConstruct,
			 dDomGeo) {
	
/**
 * @name idx.html
 * @namespace Provides convenient functions to handle HTML and DOM.
 */
	var iHTML = dLang.getObject("html", true, iMain);

/**
 * @public
 * @function
 * @name idx.html.getOffsetPosition
 * @description Obtains position of a node from the specified root node.
 * @param {Element} node
 * @param {Element} root If omitted, body is used.
 * @returns {Object} position with "l" and "t" properties
 */
	iMain.getOffsetPosition = iHTML.getOffsetPosition = function(node, root) {
		root = root || dWindow.body();
		var n = node;
		
		var l = 0;
		var t = 0;
		
		while (n != root) {
			l += n.offsetLeft;
			t += n.offsetTop;
			n = n.offsetParent;
		}
		return {l: l, t: t};
	};
	
/**
 * @public
 * @function
 * @name idx.html.containsNode
 * @description Tests whether a node contains another node as its descendant.
 * @param {Element} parent Possible ancestor
 * @param {Element} node Possible descendant
 * @returns {Boolean}
 */
	iMain.containsNode = iHTML.containsNode = function(parent, node) {
		var n = node;
		while (n && n != dWindow.body()) {
			if (n == parent) {
				return true;
			}
			if (n.parentNode) {
				n = n.parentNode;	
			} else {
				break;
			}
		}
		return false;
	};
	
/**
 * @public
 * @function
 * @name idx.html.containsCursor
 * @description Tests whether a node contains the mouse cursor of an event.
 * @param {Element} node
 * @param {Event} evt
 * @returns {Boolean}
 */
	iMain.containsCursor = iHTML.containsCursor = function(node, evt) {
		var pos = dDomGeo.position(node);
		var l = pos.x;
		var t = pos.y;
		var r = l + pos.w;
		var b = t + pos.h;
		var cx = evt.clientX;
		var cy = evt.clientY;
		
		var contained = cx > l && cx < r && cy > t && cy < b;
		return contained;
	};
	
/**
 * @public
 * @function
 * @name idx.html.setTextNode
 * @description Creates single child of text node with the specified string.
 * @param {Element} node
 * @param {String} text
 */
	iMain.setTextNode = iHTML.setTextNode = function(/*node*/ node, /*string*/ text){
		if(!node){
			return;
		}
		dDomConstruct.place(dWindow.doc.createTextNode(text), node, "only");
	};
	
/**
 * @public
 * @function
 * @name idx.html.escapeHTML
 * @description Converts a string escaping HTML special characters (<>&").
 * @param {String} s
 * @returns {String}
 */
	iMain.escapeHTML = iHTML.escapeHTML = function(/*string*/ s){
		if(!dLang.isString(s)){
			return s;
		}
		return s.replace(/&/g,"&amp;").replace(/</g,"&lt;").replace(/>/g,"&gt;").replace(/"/g,"&quot;");
	};
	
/**
 * @public
 * @function
 * @name idx.html.unescapeHTML
 * @description Converts a string with restoring character entities for HTML special characters (<>&").
 * @param {String} s
 * @returns {String}
 */
	iMain.unescapeHTML = iHTML.unescapeHTML = function(/*string*/s){
		if(!dLang.isString(s)){
			return s;
		}
		return s.replace(/&lt;/g,"<").replace(/&gt;/g,">").replace(/&quot;/g,"\"").replace(/&amp;/g,"&");
	};

	return iHTML;

});
