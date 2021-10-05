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

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.math.BigInteger;

import org.apache.commons.lang3.StringEscapeUtils;
import org.apache.commons.lang3.text.translate.CharSequenceTranslator;
import org.apache.commons.lang3.text.translate.NumericEntityEscaper;

import com.ibm.gvt.testdata.DataSource.TestData;

public class GvtHelper {

	/**
	 * @param args
	 * @throws UnsupportedEncodingException 
	 */
	public static void main(String[] args) throws UnsupportedEncodingException {
		String filename="gvt/gvtdata.xml";
		DataSource source=new DataSource(new File(filename));
		TestData data=source.getById(45);
		
		String testData=new String(data.getHexChars());
	
		CharSequenceTranslator escapeTranslator = StringEscapeUtils.ESCAPE_XML.with(NumericEntityEscaper.between(0x7f, Integer.MAX_VALUE));
		
		System.out.println("Java characters: " + StringEscapeUtils.escapeJava(testData));
		System.out.println("Number of characters : "  + testData.length());
		System.out.println("Number of UTF8 bytes : "  + testData.getBytes("UTF-8").length);
		System.out.println("Numeric escape : "  + escapeTranslator.translate(testData));
		System.out.println(testData);
		
		char[] c = testData.toCharArray();
		System.out.println("LEN is " + c.length);
		//for (int k=0; k<testData.length(); k++) {
			
			//System.out.println("CHAR: " + s + " LEN: " + s.length());
		//}
		
		String myStr = "";
		for (int k=0; k<1024; k++) {
			myStr = myStr + "C";
		}
		System.out.println(myStr);
	
		String bigTestData = null;
		for (int i=0; i<128; i++) {
			System.out.print(testData);
		}
	}
	
	public static String toHex(String arg) throws UnsupportedEncodingException {
	    return String.format("%040x", new BigInteger(arg.getBytes("UTF8")));
	}

}
