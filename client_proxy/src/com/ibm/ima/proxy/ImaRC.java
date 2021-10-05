/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.proxy;

public enum ImaRC {
	
	IMARC_OK(0),
	IMARC_NotAuthorized(180),
	IMARC_NotAuthenticated(181),
	IMARC_CreateDeviceFailed(177),
	CUSTOM(-1);
	
	private int code;
	
	private ImaRC(int code){
		this.code = code;
	}
	
	/**
	 * Get RC as Int
	 * @return
	 */
	public int asInt(){
		return code;
	}

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
