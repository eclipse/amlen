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
		'dijit/_Widget',
		'dijit/_TemplatedMixin',
		'dijit/_WidgetsInTemplateMixin'
		], function(declare, _Widget, _TemplatedMixin, _WidgetsInTemplateMixin) {

	var _TemplatedContent = declare("ism.widgets._TemplatedContent", [_Widget, _TemplatedMixin, _WidgetsInTemplateMixin ]);
		// summary:
		//		A generic wrapper for a templated widget.
		// description:
		//		We use this as a convenience to allow an HTML snippet to be placed
		//		on a page without requiring the use of a ContentPane.
		
	return _TemplatedContent;
});
