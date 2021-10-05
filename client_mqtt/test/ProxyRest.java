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

import java.util.Iterator;
import java.util.Set;
import java.util.jar.Attributes;

import com.ibm.ism.ws.IsmHTTPConnection;
import com.ibm.ism.ws.IsmHTTPRequest;
import com.ibm.ism.ws.IsmHTTPResponse;


public class  ProxyRest {

    public static void main(String[] args) throws Exception {
        
        IsmHTTPConnection hcon = new IsmHTTPConnection("kwb5", 1883, 0);
        hcon.setVerbose(4, false);
        hcon.connect();
        
        hcon.setContext("test 1");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "GET", "/", null);
            hreq.setVerbose(1);
            IsmHTTPResponse msg = hreq.request();
            if (msg.getStatus() == 200)
                System.out.println("about found");
            System.out.println(msg.getHeader("date"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        hcon.setContext("test 2");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "POST", "/admin/set/KafkaMeteringTopic/now/is/the/time", null);
            hreq.setUser("user", "password");
            IsmHTTPResponse msg = hreq.request();
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        hcon.setContext("test 3");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "GET", "/admin/config/set/KafkaMeteringTopic", null);
            hreq.setUser("user", "password");
            IsmHTTPResponse msg = hreq.request();
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        hcon.setContext("test 4");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "POST", "/admin/config/endpoint/mqtt2", null);
            hreq.setUser("user", "password");
            hreq.setContent("{\"Enabled\":false}");
            IsmHTTPResponse msg = hreq.request();
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        hcon.setContext("test 5");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "GET", "/admin/config/endpoint/mqtt2", null);
            hreq.setUser("user", "password");
            hreq.setContent("{\"Enabled\":false}");
            IsmHTTPResponse msg = hreq.request();
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        hcon.setContext("test 6");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "GET", "/admin/endpoint/mqtt", null);
            hreq.setUser("user", "password");
            IsmHTTPResponse msg = hreq.request();
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        hcon.setContext("test 7");
        try {
            IsmHTTPRequest hreq = new IsmHTTPRequest(hcon, "GET", "/admin/time", null);
            hreq.setUser("user", "password");
            hreq.setHeader("user-agent",  "IsmHTTPConnection");
            hreq.setHeader("x-409",  "Cleaner");
            hreq.setContent("{\"Enabled\":false}");
            IsmHTTPResponse msg = hreq.request();
            Attributes hdrs = msg.getHeaders();
            Set<Object> map = hdrs.keySet();
            Iterator it = map.iterator();
            while (it.hasNext()) {
                String hdr = ""+it.next();
                System.out.println(hdr + ": " + hdrs.getValue(hdr));
            }
            
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
    }
    

}
