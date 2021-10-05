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

public class IncrementStoreIntegerAction extends ApiCallAction {
	private final 	String 	_structureID;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public IncrementStoreIntegerAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
	}

	protected boolean invokeApi() throws IsmTestException {
		
		Integer var = (Integer)_dataRepository.getVar(_structureID);
		if (null == var) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
					, "ISMTEST"+Constants.INCREMENTINTEGER
					, "Cannot find StoreInteger "+_structureID);
			throw new IsmTestException("ISMTEST"+Constants.INCREMENTINTEGER
					, "Cannot find StoreInteger "+_structureID);
		}
		_dataRepository.storeVar(_structureID, var+1);
		return true;
	}

}
