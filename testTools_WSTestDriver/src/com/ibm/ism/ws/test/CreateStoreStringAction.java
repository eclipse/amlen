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
package com.ibm.ism.ws.test;

import org.w3c.dom.Element;

public class CreateStoreStringAction extends ApiCallAction {
	private final 	String 	_structureID;
	private final	String	_value;

	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CreateStoreStringAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		_value = _actionParams.getProperty("value");
		if (_value == null) {
			throw new RuntimeException("value is not defined for "
					+ this.toString());
		}
	}

	protected boolean invokeApi() throws IsmTestException {
		_dataRepository.storeVar(_structureID, _value);
		return true;
	}

}
