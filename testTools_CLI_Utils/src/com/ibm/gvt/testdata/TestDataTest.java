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
package com.ibm.gvt.testdata;

import java.net.URI;
import java.net.URISyntaxException;

public class TestDataTest {
	public static void main(String[] args) throws URISyntaxException {
//		if(args.length!=1) {
//			throw new IllegalArgumentException("Usage: " + TestDataTest.class.getName()+" file:///URI/to/testdata.xml");
//			// from eclipse: ${workspace_loc:/gvt_dtd/xml/testdata.xml}
//		}
		String filename="gvtdata.xml";
		DataSource ds = new DataSource(new URI(filename));
		
		System.out.println("Data source for " + filename);
		
		for (DataSource.TestData td : ds.getList()) {
			System.out.println("ID: " + td.getId()+", guide:"+td.getGuide());
			System.out.println("Hex Code:"+td.getHexCode());
			System.out.println("Raw String:"+td.getRawString());
			System.out.println("Encoding: " + td.getEncoding());
			byte[] dataInBytes=td.getByteArray();
			System.out.println();
		}
	}
}
