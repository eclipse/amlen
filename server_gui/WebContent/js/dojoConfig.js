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

var dojoConfig = {
	async: true,
	isDebug: true,
	locale: 'en',
	versionLabel: "",
	
	baseUrl: ".",
	
	infocenterURL: "http://ibm.biz/iotms_v50_docs", // replaced in production builds
	
	packages : [ {
		name : "dojo",
		location : "./js/dojotoolkit/dojo"
	}, {
		name : "dijit",
		location : "./js/dojotoolkit/dijit"
	}, {
		name : "dojox",
		location : "./js/dojotoolkit/dojox"
	}, {
		name : "gridx",
		location : "./js/dojotoolkit/gridx"
	}, {
		name : "idx",
		location : "./js/idx"
	}, {
		name : "d3",
		location : "./js/d3" 
	}, {
		name : "ism",
		location : "./js/ism"
	} ]
};

