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

import java.text.ParseException;
import java.util.Date;

public class EndpointRecord extends MonitorRecord {
	public final String 	name;
	public final String		epInterface;
	public final boolean	enabled;
	public long				totalConnections;
	public long				activeConnections;
	public long				msgRead;
	public long				msgWrite;
	public long				bytesRead;
	public long				bytesWrite;
	public long				badConnections;
	public long				lostMessageCount;
	public Date				resetTime;
	
	EndpointRecord(ImaJson json) throws IsmTestException {
		super(json);

		// Parse the rest
		name = json.getString("Name");
		if (null == name) throw new IsmTestException("ISMTEST"+(Constants.ENDPOINTRECORD),
				"Name not in json data for Endpoint record");
		epInterface = json.getString("Interface");
		if (null == epInterface) throw new IsmTestException("ISMTEST"+(Constants.ENDPOINTRECORD+1),
			"Interface not in json data for Endpoint record");
		
		enabled = Boolean.valueOf(json.getString("Enabled"));

		try {
			totalConnections = Long.valueOf(json.getString("TotalConnections"));
			activeConnections = Long.valueOf(json.getString("ActiveConnections"));
			badConnections = Long.valueOf(json.getString("BadConnections"));
			msgRead = Long.valueOf(json.getString("MsgRead"));
			msgWrite = Long.valueOf(json.getString("MsgWrite"));
			bytesRead = Long.valueOf(json.getString("BytesRead"));
			bytesWrite = Long.valueOf(json.getString("BytesWrite"));
			lostMessageCount = Long.valueOf(json.getString("LostMessageCount"));
			String tStamp = json.getString("ResetTime");
			try {
				resetTime = TestUtils.parse8601(tStamp);
			} catch (ParseException pe) {
				throw new IsmTestException("ISMTEST"+(Constants.MONITORRECORD),
					"Parse Exception thrown trying to parse ResetTime '"+tStamp+"'", pe);
			}
		} catch (NumberFormatException nfe) {
			throw new IsmTestException("ISMTEST"+(Constants.ENDPOINTRECORD+2),
				"caught NumberFormatException processing '"+json.toString()+"'", nfe);
		}
		
		//
	}
}
