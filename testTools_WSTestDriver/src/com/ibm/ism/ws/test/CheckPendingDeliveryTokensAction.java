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

public class CheckPendingDeliveryTokensAction extends ApiCallAction {
	private final 	String 					_structureID;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CheckPendingDeliveryTokensAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("connection_id");
	}

	protected boolean invokeApi() throws IsmTestException {
		MyConnection con = (MyConnection)_dataRepository.getVar(_structureID);
		if (null == con) {
			throw new IsmTestException("ISMTEST"+Constants.CHECKMESSAGESRECEIVED,
					"Connection "+_structureID+" is not found");
		}
		int count = con.checkPendingDeliveryTokens();
		
		return 0 == count;
	}

}
