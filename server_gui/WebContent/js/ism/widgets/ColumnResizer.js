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

define([
	'dojo/_base/declare',
	'dojo/_base/lang',
	'dojo/query',
	'gridx/modules/ColumnResizer'
], function(declare, lang, query, ColumnResizer){

	/**
	 * @name ism.widgets.ColumnResizer
	 * @class ISM extension of gridx version 
	 * @augments grid.modules.ColumnResizer
	 */
	return declare("ism.widgets.ColumnResizer", [ColumnResizer],
	{							
		onResize: function(/* colId, newWidth, oldWidth */){
			this.inherited(arguments);
			this.ismOnResize(this.grid.id, arguments[0], arguments[1], arguments[2]);
		},
		
		ismOnResize: function(gridId, colId, newWidth, oldWidth) {			
			console.log("ism.widgets.ColumnResizer.ismOnResize gridId=" + gridId +", colId=" + colId + ", newWidth=" + newWidth);
		}
		
	});
});
