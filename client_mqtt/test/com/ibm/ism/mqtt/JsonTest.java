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

package com.ibm.ism.mqtt;

import com.ibm.ism.mqtt.ImaJson;

import junit.framework.TestCase;

public class JsonTest extends TestCase {
    /*
     * Unit test method
     */
    static void jsonparse(ImaJson jo, String jstr, int count) {
    	int ents = jo.parse(jstr);
    	if (ents != count)
    		System.out.println("parse:" + jstr + " count=" + ents + " expected=" + count);
    	System.out.println("tostr: " + jo.toString());
    }
    
    /*
     * Entry point for unit test
     */
    public void testJsonParse() throws Exception {
    	ImaJson jo = new ImaJson();
    	jsonparse(jo, "{}", 1);
    	jsonparse(jo, "[0,1,2, 3 ,4]", 6);
    	jsonparse(jo, "[\"abc\"  ,\"def\", 0.31415E+1, true, false, null]", 7);
    	jsonparse(jo, "{\"abc\":3, \"x\":\"\\b\\r\\n\\tz\", \"obj\": { \"g\":3, \"h\":4.1, \"i\":\"5\" }\n }", 7);
    	jsonparse(jo, "{\"type\":\"PUBLISH\",\"topic\":\"fred\", \"payload\":[3,\"ghi\",{\"x\":3}]}", 8);
    	jsonparse(jo, "{\"type\":\"PUBLISH\",\"topic\":\"RequestT\",\"payload\":{\"id\":\"13981690\",\"name\":\"myName\",\"array\":[\"1\",\"2\"]},\"replyto\":\"ResponseT\",\"corrid\":\"<corrid>\"}", 11);
    }
}
