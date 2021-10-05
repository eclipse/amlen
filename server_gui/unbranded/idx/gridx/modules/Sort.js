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
	'dojo/_base/kernel',
	'gridx/modules/Sort'
], function(kernel, Sort){
	kernel.deprecated('Sort is moved from idx/gridx/modules/Sort to gridx/modules/Sort.', 'Use the new path instead.', 'IDX 1.5');

	return Sort;
});

