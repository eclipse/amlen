/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

import java.util.HashMap;
import java.util.Map;

public class TestConfig implements ImaAuthenticator {
    
    ImaProxyListener proxy;
    
    public TestConfig(ImaProxyListener listener) {
        System.out.println("\nTestConfig: init " + listener);
        proxy = listener;
    }
    
    void printSA(String [] s) {
        int  i;
        for (i=0; i<s.length; i++) {
            if (i != 0)
                System.out.print(", ");
            System.out.print(s[i]);
        }
        System.out.println();
    }
    
    public void run() {
        String  json;
        ImaJson parseobj = new ImaJson();
        ImaProxyResult ret; 
        Map   map;
        
        try {
            System.out.println("Tenant");
            map = proxy.getTenant("");
            System.out.println(""+map);
            ret = proxy.getJSON("", "tenant=");
            System.out.println(ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }    
         
        try {
            System.out.println("Server");
            map = proxy.getServer("IoTServer");
            System.out.println(""+map);
            ret = proxy.getJSON("", "server=IoTServer");
            System.out.println(ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
        } 
          
        try {
            System.out.println("Endpoint");
            map = proxy.getEndpoint("mqtt");
            System.out.println(""+map);
            ret = proxy.getJSON("", "endpoint=mqtts");
            System.out.println(ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
        } 
         
        try {
            System.out.println("User");
            map = proxy.getUser("kwb", "");
            System.out.println(""+map);
            ret = proxy.getJSON("", "user=,kwb");
            System.out.println(ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
        } 
      
        try {
            System.out.println("getJSON list");
            ret = proxy.getJSON("", "list,endpoint=*qt*");
            System.out.println(ret.getStringValue());
            
            System.out.println("getTenantList");
            String [] out = proxy.getTenantList("*");
            printSA(out);
            
            System.out.println("getEndpointList");
            out = proxy.getEndpointList("*");
            printSA(out);
            
            System.out.println("getServerList");
            out = proxy.getServerList("*");
            printSA(out);
            
            System.out.println("getUserList");
            out = proxy.getUserList("*", "");
            printSA(out);
        } catch (Exception e) {
            e.printStackTrace(System.out);
        } 
          
        try {
            System.out.println("getConfigItem TraceLevel=" + proxy.getConfigItem("TraceLevel"));
            System.out.println("getConfigItem FileLimit=" + proxy.getConfigItem("FileLimit"));
            System.out.println("getConfigItem Fred=" + proxy.getConfigItem("Fred"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            System.out.println("setConfigItem TraceLevel");
            proxy.setConfigItem("TraceLevel", "6,tcp=9,monitoring=3");
            System.out.println("getConfigItem TraceLevel=" + proxy.getConfigItem("TraceLevel"));
            proxy.setConfigItem("Fred", 7);  /* Setting unknown is igored */
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            System.out.println("setEndpoint");
            HashMap<String,Object> emap = new HashMap<String,Object>();
            emap.put("Ciphers", "Best");
            emap.put("Port", 8888);
            emap.put("UseClientCipher", true);
            proxy.setEndpoint("mqtts", emap);
            map = proxy.getEndpoint("mqtts");
            System.out.println(""+map);
            
            proxy.setEndpoint("sam", emap);
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            System.out.println("postTenant");
            ImaProxyResult rc = proxy.postJSON(null, null,  
               "{\"Tenant\":{\"bill\":{\"User\":{\"iotcloud481\":{\"Password\":\"a\"}},\"RequireCertificate\":false,\"Enabled\":true,\"AllowSysTopic\":true,\"MaxQoS\":2,\"AllowDurable\":true,\"ChangeTopic\":\"IoT1\",\"CheckUser\":true,\"AllowShared\":false,\"RequireSecure\":false," + 
               "\"Server\":\"IoTServer\",\"AllowAnonymous\":false,\"Port\":1883,\"CheckTopic\":\"None\",\"MaxConnections\":1000000}}}");
            System.out.println("rc=" + rc);
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            System.out.println("setEndpoint json");
            proxy.postJSON(null, "endpoint=mqtts", "{\"Port\":9999,\"Ciphers\":\"Medium\"}");
            map = proxy.getEndpoint("mqtts");
            System.out.println(""+map);
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            proxy.log("MYLOG1", "This is a msg. repl1={0} repl2={1}", "value1", 2);
            System.out.println("logged");
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            HashMap<String,Object>  value = new HashMap<String,Object>();
            String[] addresses;
            addresses = new String[] {"14.13.12.1","14.13.13.3"};
            String[] backupaddresses;
            backupaddresses = new String[] {"11.12.11.1","11.12.11.3"};
            value.put("Address", addresses);
            value.put("Backup", backupaddresses);
            value.put("Port", 16102);
            value.put("MqttProxyProtocol", false);
            proxy.setServer("Server5", value);
            map = proxy.getServer("Server5");
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        try {
            proxy.deleteUser("kwb", "", false);
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            System.out.println(proxy.getEndpointStats("*"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        try {
            System.out.println(proxy.getServerStats("*"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        try {
            System.out.println(proxy.getServerStats(null));
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        
        try {
            String acls = "@myacls\n+dtype/dev1\n+dtype/dev2\n";
            proxy.setACL(acls);
        } catch (Exception e) {
            e.printStackTrace(System.out);
        }
        

       try {
           String actmon =     
    "{\"ActivityMonitoring\":{\"Address\":[\"mongos-0.outlast.mgmt.mongo.test.internetofthings.ibmcloud.com:27017\"," +
    "\"mongos-1.outlast.mgmt.mongo.test.internetofthings.ibmcloud.com:27017\",\"mongos-2.outlast.mgmt.mongo.test.internetofthings.ibmcloud.com:27017\"]," +
    "\"Type\":\"MongoDB\",\"UseTLS\":true,\"TrustStore\":\"/opt/ibm/msproxy/truststore/client/ca_public_cert.pem\"," +
    "\"User\":\"fra02_fra02_2_cs_activity\",\"Password\":\"passwordvity\",\"Name\":\"fra02-2-cs-activity\"," +
    "\"MemoryLimitPct\":2}}";
           System.out.println("set ActivityMonitoring: " + actmon);
           proxy.postJSON("", "", actmon);
       } catch (Exception e) {
           e.printStackTrace(System.out);
       }
        
        
      // System.out.println("Before terminate");
      //  proxy.terminate(20);
      //  System.out.println("After terminate");
        proxy.setAuthenticator(this);
        
    }

    /*
     * @see com.ibm.ima.proxy.ImaAuthenticator#authenticate(com.ibm.ima.proxy.ImaAuthConnection)
     */
    public void authenticate(ImaAuthConnection connect) {
        String clientID = connect.getClientID();
        char   clientClass = (clientID.length()>1 && clientID.charAt(1)==':') ? clientID.charAt(0) : ' ';
        String orgID = "";
        if (clientID.length()>2 && clientID.charAt(1)==':') {
            int endpos = clientID.indexOf(':', 2);
            if (endpos > 0)
                orgID = clientID.substring(2, endpos);
        }
        System.out.println("clientClass=" + clientClass + " Org=" + orgID);
        if ("password".equals(connect.getPassword())) {
            connect.authenticate(true);
            System.out.println("Passed authenticate: " + connect);
                    
        } else {
            connect.authenticate(false);
            System.out.println("Failed authenticate: clientID=" + connect);
        }
    }

	@Override
	public void authorize(ImaAuthConnection connect) {
		// TODO Auto-generated method stub
		
	}

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
