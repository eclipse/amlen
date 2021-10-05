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

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class SetStoreIntegerAction extends ApiCallAction {
	private final 	String 	_structureID;
	private final	int	_value;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public SetStoreIntegerAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		_value = Integer.valueOf(_actionParams.getProperty("value", "0"));
	}

	protected boolean invokeApi() throws IsmTestException {
		
		Object storeValue = _dataRepository.getVar(_structureID);
		if (null == storeValue) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
					, "ISMTEST"+Constants.SETSTOREINTEGER
					, "Cannot find variable "+_structureID);
			throw new IsmTestException("ISMTEST"+Constants.SETSTOREINTEGER
					, "Cannot find variable "+_structureID);
		} else if (storeValue instanceof Integer) {
			Integer intValue = (Integer)storeValue;
			intValue = _value;
			_dataRepository.storeVar(_structureID, intValue);
		} else if (storeValue instanceof NumberString) {
			((NumberString)storeValue).setNumber(_value);
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
					, "ISMTEST"+Constants.SETSTOREINTEGER
					, "Variable is not a stored Integer or a NumberString "+_structureID);
			throw new IsmTestException("ISMTEST"+Constants.SETSTOREINTEGER
					, "Variable is not a stored Integer or a NumberString "+_structureID);
		}
		return true;
	}

}
