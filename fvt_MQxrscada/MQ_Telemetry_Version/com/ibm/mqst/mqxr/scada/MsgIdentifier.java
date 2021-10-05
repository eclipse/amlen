package com.ibm.mqst.mqxr.scada;
/*
 * Copyright (c) 2002-2021 Contributors to the Eclipse Foundation
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
 * @version 	1.0
 */

/*
 * This class represents hte 16-bit unsigned integer used as
 * a message identifier for SCADA msgs using QoS 1 or 2.
 * 
 * This class may be extended in the future to track which IDs
 * have been used and free those that are no longer needed etc.
 * 
 */
 
public class MsgIdentifier {
	
	byte[] msgId;
	
	// Constructor - tries to create with int supplied - checks
	// to see if is in bounds
	public MsgIdentifier(int id) throws ScadaException {
		
		msgId = new byte[2];
		
		// Need to check that value supplied is in bounds
		if(id > 65535 || id < 0 ) {
			// then the id is not valid (0 is reserved) - throw exception
			msgId = null;
			ScadaException myE = new ScadaException("Attempt to construct msg ID with value out of range - value supplied is " + id);
			throw(myE);
		}
		else {
			msgId = SCADAutils.shortToBytes((short)id);
		}
			
	}
	
	// Constructor - takes pre-formed byte array
	public MsgIdentifier(byte[] id) {
		
		msgId = id;
	}

}
