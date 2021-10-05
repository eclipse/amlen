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
package com.ibm.ima.proxy;

import java.util.Map;

public class TestJavaConfig implements ImaAuthenticator {
    
    ImaProxyListener proxy;
    
    public TestJavaConfig(ImaProxyListener listener) {
        System.out.println("\nTestJavaConfig: init " + listener);
        proxy = listener;
        proxy.log("MYLOG001","\nTestJavaConfig: init {0}", listener);
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
            proxy.log("MYLOG002","Tenant");
            map = proxy.getTenant("");
            System.out.println(""+map);
            proxy.log("MYLOG003","{0}", map);
            ret = proxy.getJSON("", "tenant=");
            System.out.println(ret.getStringValue());
            proxy.log("MYLOG004","{0}",ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG5ERR", "0", e.getStackTrace().toString());
        }    
         
//        try {
//            System.out.println("Server");
//            proxy.log("MYLOG006", "Server");
//            map = proxy.getServer("IoTServer");
//            System.out.println(""+map);
//            proxy.log("MYLOG007","{0}",map);
//            ret = proxy.getJSON("", "server=IoTServer");
//            System.out.println(ret.getStringValue());
//            proxy.log("MYLOG008","{0}",ret.getStringValue());
//            parseobj.parse(ret.getStringValue());
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//            proxy.log("MYLOG9ERR", "{0}", e.getStackTrace().toString());
//        } 
          
        try {
            System.out.println("Endpoint");
            proxy.log("MYLOG010","Endpoint");
            map = proxy.getEndpoint("mqtt");
            System.out.println(""+map);
            proxy.log("MYLOG011","{0}",map);
            ret = proxy.getJSON("", "endpoint=mqtts");
            System.out.println(ret.getStringValue());
            proxy.log("MYLOG012","{0}",ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG13ERR", "0", e.getStackTrace().toString());
        } 
         
        try {
            System.out.println("User");
            proxy.log("MYLOG014", "User");
            map = proxy.getUser("kwb", "");
            System.out.println(""+map);
            proxy.log("MYLOG015","{0}",map);
            ret = proxy.getJSON("", "user=,kwb");
            System.out.println(ret.getStringValue());
            proxy.log("MYLOG016","{0}",ret.getStringValue());
            parseobj.parse(ret.getStringValue());
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG17ERR", "{0}", e.getStackTrace().toString());
        } 
      
        try {
            System.out.println("getJSON list");
            proxy.log("MYLOG108","getJSON list");
            ret = proxy.getJSON("", "list,endpoint=*qt*");
            System.out.println(ret.getStringValue());
            proxy.log("MYLOG019","{0}", ret.getStringValue());
            
            System.out.println("getTenantList");
            proxy.log("MYLOG020","getTenantList");
            String [] out = proxy.getTenantList("*");
            printSA(out);
            
            System.out.println("getEndpointList");
            proxy.log("MYLOG21","getEndpointList");
            out = proxy.getEndpointList("*");
            printSA(out);
            
            System.out.println("getServerList");
            proxy.log("MYLOG022","getServerList");
            out = proxy.getServerList("*");
            printSA(out);
            
            System.out.println("getUserList");
            proxy.log("MYLOG023","getUserList");
            out = proxy.getUserList("*", "");
            printSA(out);
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG24ERR", "{0}", e.getStackTrace().toString());
        } 
          
        try {
            System.out.println("getConfigItem TraceLevel=" + proxy.getConfigItem("TraceLevel"));
            proxy.log("MYLOG025","getConfigItem TraceLevel={0}", proxy.getConfigItem("TraceLevel"));
//            System.out.println("getConfigItem FileLimit=" + proxy.getConfigItem("FileLimit"));
//            proxy.log("MYLOG026","getConfigItem FileLimit={0}", proxy.getConfigItem("FileLimit"));
////            System.out.println("getConfigItem Fred=" + proxy.getConfigItem("Fred"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG27ERR", "{0}", e.getStackTrace().toString());
        }
        
//        try {
//            System.out.println("setConfigItem TraceLevel");
//            proxy.setConfigItem("TraceLevel", "6,tcp=9,monitoring=3");
//            System.out.println("getConfigItem TraceLevel=" + proxy.getConfigItem("TraceLevel"));
//            proxy.log("MYLOG028","getConfigItem TraceLevel={0}", proxy.getConfigItem("TraceLevel"));
////            proxy.setConfigItem("Fred", 7);  /* Setting unknown is igored */
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//            proxy.log("MYLOG29ERR", "{0}", e.getStackTrace().toString());
//        }
        
//        try {
//            System.out.println("setEndpoint");
//            HashMap<String,Object> emap = new HashMap<String,Object>();
//            emap.put("Ciphers", "Best");
//            emap.put("Port", 8888);
//            emap.put("UseClientCipher", true);
//            proxy.setEndpoint("mqtts", emap);
//            map = proxy.getEndpoint("mqtts");
//            System.out.println(""+map);
//            
//            proxy.setEndpoint("sam", emap);
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//        }
//        
//        try {
//            System.out.println("postTenant");
//            ImaProxyResult rc = proxy.postJSON(null, null,  
//               "{\"Tenant\":{\"bill\":{\"User\":{\"iotcloud481\":{\"Password\":\"a\"}},\"RequireCertificate\":false,\"Enabled\":true,\"AllowSysTopic\":true,\"MaxQoS\":2,\"AllowDurable\":true,\"ChangeTopic\":\"IoT1\",\"CheckUser\":true,\"AllowShared\":false,\"RequireSecure\":false," + 
//               "\"Server\":\"IoTServer\",\"AllowAnonymous\":false,\"Port\":1883,\"CheckTopic\":\"None\",\"MaxConnections\":1000000}}}");
//            System.out.println("rc=" + rc);
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//        }
//        
//        try {
//            System.out.println("setEndpoint json");
//            proxy.postJSON(null, "endpoint=mqtts", "{\"Port\":9999,\"Ciphers\":\"Medium\"}");
//            map = proxy.getEndpoint("mqtts");
//            System.out.println(""+map);
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//        }
        
        try {
            proxy.log("MYLOG030", "This is a msg. repl1={0} repl2={1}", "value1", 2);
            System.out.println("logged");
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG31ERR", "{0}", e.getStackTrace().toString());
        }
        
//        try {
//            HashMap<String,Object>  value = new HashMap<String,Object>();
//            String[] addresses;
//            addresses = new String[] {"14.13.12.1","14.13.13.3"};
//            String[] backupaddresses;
//            backupaddresses = new String[] {"11.12.11.1","11.12.11.3"};
//            value.put("Address", addresses);
//            value.put("Backup", backupaddresses);
//            value.put("Port", 16102);
//            value.put("MqttProxyProtocol", false);
//            proxy.setServer("Server5", value);
//            map = proxy.getServer("Server5");
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//        }
//        try {
//            proxy.deleteUser("kwb", "", false);
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//        }
        
        try {
            System.out.println(proxy.getEndpointStats("*"));
            proxy.log("MYLOG032","Endpoint stats(*): {0}", proxy.getEndpointStats("*"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG33ERR", "{0}", e.getStackTrace().toString());
        }
        try {
            System.out.println(proxy.getServerStats("*"));
            proxy.log("MYLOG034","Server stats(*): {0}",proxy.getServerStats("*"));
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG35ERR", "{0}", e.getStackTrace().toString());
        }
        try {
            System.out.println(proxy.getServerStats(null));
            proxy.log("MYLOG036","Endpoint stats(null): {0}", proxy.getEndpointStats(null));
        } catch (Exception e) {
            e.printStackTrace(System.out);
            proxy.log("MYLOG37ERR", "{0}", e.getStackTrace().toString());
        }
        
//        try {
//            String acls = "@myacls\n+dtype/dev1\n+dtype/dev2\n";
//            proxy.setACL(acls);
//        } catch (Exception e) {
//            e.printStackTrace(System.out);
//        }
        
      // System.out.println("Before terminate");
      //  proxy.terminate(20);
      //  System.out.println("After terminate");
        proxy.setAuthenticator(this);
        
    }

    /*
     * @see com.ibm.ima.proxy.ImaAuthenticator#authenticate(com.ibm.ima.proxy.ImaAuthConnection)
     */
    public void authenticate(ImaAuthConnection connect) {
    	boolean authenticated = false;
		System.out.println("In " + this.getClass().getName() + ".authenticate()");
		proxy.log("MYLOG37a", "IN {0}.authenticate()", this.getClass().getName());
		//showConnectionProperties(connect);
        String clientID = connect.getClientID();
        char   clientClass = (clientID.length()>1 && clientID.charAt(1)==':') ? clientID.charAt(0) : ' ';
        String orgID = "";
        if (clientID.length()>2 && clientID.charAt(1)==':') {
            int endpos = clientID.indexOf(':', 2);
            if (endpos > 0)
                orgID = clientID.substring(2, endpos);
        }
        System.out.println("clientID=" + clientID + " clientClass=" + String.valueOf(clientClass) + " Org=" + orgID);
        proxy.log("MYLOG38","clientID={0} clientClass={1} Org={2}", clientID, String.valueOf(clientClass), orgID);
        if ("password".equals(connect.getPassword())) {
            authenticated = true;
            System.out.println("Password authentication succeeded: " + connect);
            proxy.log("MYLOG39","Password authentication succeeded: {0}", connect.toString());
            if ("exactMatch".equals(connect.getUserID())) {
        		System.out.println("In " + this.getClass().getName() + ".authenticate() userid=exactMatch");
        		proxy.log("MYLOG41", "IN {0}.authenticate() userid=exactMatch", this.getClass().getName());
            	if (connect.isExactNameMatch()) {
            		System.out.println("In " + this.getClass().getName() + ".authenticate() isExactNameMatc=true");
            		proxy.log("MYLOG41a", "IN {0}.authenticate() isExactNameMatch=true", this.getClass().getName());

                    System.out.println("Passed authenticate: " + connect + "CN or SAN exactly matches client ID");
                    proxy.log("MYLOG41b","Passed authenticate: {0} - CN or SAN exactly matches client ID", connect.toString());
            	} else {
            		authenticated = false;
            		System.out.println("In " + this.getClass().getName() + ".authenticate() isExactNameMatch=false");
            		proxy.log("MYLOG41c", "IN {0}.authenticate() isExactNameMatch=false", this.getClass().getName());

                    System.out.println("Failed authenticate: clientID=" + connect.getClientID() + " - " + connect + " - expected exact match but found only a partial match");
                    proxy.log("MYLOG41d","Failed authenticate: clientID={0} - {1} - expected exact match but found only a partial match", connect.getClientID(), connect.toString());
            	}
            } else if ("match".equals(connect.getUserID())) {
        		System.out.println("In " + this.getClass().getName() + ".authenticate() userid=match");
        		proxy.log("MYLOG42", "IN {0}.authenticate() userid=match", this.getClass().getName());
            	if (connect.isNameMatch()) {
            		System.out.println("In " + this.getClass().getName() + ".authenticate() isNameMatch=true");
            		proxy.log("MYLOG42a", "IN {0}.authenticate() isNameMatch=true", this.getClass().getName());
            	} else {
            		authenticated = false;
            		System.out.println("In " + this.getClass().getName() + ".authenticate() isNameMatch=false");
            		proxy.log("MYLOG42b", "IN {0}.authenticate() isNameMatch=false", this.getClass().getName());
                    System.out.println("Failed authenticate: clientID=" + connect.getClientID() + " - " + connect + " - expected at least a partial match but found no match");	
                    proxy.log("MYLOG42c","Failed authenticate: clientID={0} - {1} - expected at least a partial match but found no match", connect.getClientID(), connect.toString());
            	}
            }
        } else {
            System.out.println("Failed password authentication: clientID=" + connect.getClientID() + " - " + connect);
            proxy.log("MYLOG40","Failed password authentication: clientID={0} - {1}", connect.getClientID(), connect.toString());
        }
        connect.authenticate(authenticated);
    }

	@Override
	public void authorize(ImaAuthConnection connect) {
		// TODO Auto-generated method stub
		System.out.println("In " + this.getClass().getName() + ".authorize()");
		proxy.log("MYLOG40a", "IN {0}.authorize()", this.getClass().getName());
	}
	
	private void showConnectionProperties(ImaAuthConnection connect) {
		System.out.println("AAAEnabled: "+connect.getAaaEnabled());
		System.out.println("Action: " +connect.getAction());
		System.out.println("AuthToken:" +connect.getAuthToken());
		System.out.println("AuthTokenLength:" +connect.getAuthTokenLenghth());
		System.out.println("CertificateIssuer:" +connect.getCertificateIssuer());
		System.out.println("CertificateName:" +connect.getCertificateName());
		System.out.println("ClientAddress:" +connect.getClientAddress());
		System.out.println("ClientID:" +connect.getClientID());
		System.out.println("CRLState:" +connect.getCrlState());
		System.out.println("DeviceID:" +connect.getDeviceId());
		System.out.println("Password:" +connect.getPassword());
		System.out.println("ProtocolUserID:" +connect.getProtocolUserID());
		System.out.println("SecGuardianEnabled:" +connect.getSecGuardianEnabled());
		System.out.println("UserID:" +connect.getUserID());
		
		proxy.log("MYLOG101","AAAEnabled: "+connect.getAaaEnabled());
		proxy.log("MYLOG102","Action: " +connect.getAction());
		proxy.log("MYLOG103","AuthToken:" +connect.getAuthToken());
		proxy.log("MYLOG104","AuthTokenLength:" +connect.getAuthTokenLenghth());
		proxy.log("MYLOG105","CertificateIssuer:" +connect.getCertificateIssuer());
		proxy.log("MYLOG106","CertificateName:" +connect.getCertificateName());
		proxy.log("MYLOG107","ClientAddress:" +connect.getClientAddress());
		proxy.log("MYLOG108","ClientID:" +connect.getClientID());
		proxy.log("MYLOG109","CRLState:" +connect.getCrlState());
		proxy.log("MYLOG110","DeviceID:" +connect.getDeviceId());
		proxy.log("MYLOG111","Password:" +connect.getPassword());
		proxy.log("MYLOG112","ProtocolUserID:" +connect.getProtocolUserID());
		proxy.log("MYLOG113","SecGuardianEnabled:" +connect.getSecGuardianEnabled());
		proxy.log("MYLOG114","UserID:" +connect.getUserID());
	}
}
