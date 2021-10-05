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

/**
 * @name idx.containers
 * @class Extension module for ensuring "_idxStyleChildren" from "idx.widgets" is called
 *        whenever "addChild" or "removeChild" method is called on a container.  This is
 *        automatically included with "idx.ext" module.
 */
define(["dojo/_base/lang","idx/main","dijit/_Container","dijit/_WidgetBase","idx/widgets"],
	function(dLang,iMain,dContainer) {
	var iContainers = dLang.getObject("containers", true, iMain);
	
	// get the combo button prototype
	var baseProto  = dContainer.prototype;
    
	// 
	// Get the base functions so we can call them from our overrides
	//
	var baseAddChild  = baseProto.addChild;
	var baseRemoveChild = baseProto.removeChild;
	
    baseProto.addChild = function(child,index) {
    	if (baseAddChild) baseAddChild.call(this, child, index);
    	if (this._started) {
    		this._idxStyleChildren();
    	}
    };
    
    baseProto.removeChild = function(child) {
    	if (baseRemoveChild) baseRemoveChild.call(this, child);
    	if (typeof child == "number") {
    		child = this.getChildren()[child];
    	}
    	if (this._started) {
    		this._idxStyleChildren();
    	}
    };    
    
    return iContainers;
});