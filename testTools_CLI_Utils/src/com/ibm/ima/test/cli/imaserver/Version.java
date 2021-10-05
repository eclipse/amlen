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
package com.ibm.ima.test.cli.imaserver;


import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.ibm.ima.test.cli.util.SSHExec;
import com.jcraft.jsch.JSchException;

public enum Version {VER_100("100"),VER_110("110"),VER_120("120"),VER_130("130"),VER_200("200");
private String version=null;
private static String srv_version=null;
private static String raw_version=null;


private Version(String version){
	this.version=version;
}

public static Version fromString(String text) throws Exception {
	if (srv_version == null) throw new Exception("version not set. Must invoke setup() to retrive version from appliance");
    	if (text != null) {
    			for (Version release : Version.values()) {
    				if (text.equalsIgnoreCase(release.version)) {
    					return release;
        }
      }
    }
    return null;
}

public static Version getVersion() throws Exception {
	
	if (srv_version == null) throw new Exception("version not set. Must invoke setup() to retrive version from appliance");

	return fromString(srv_version);
 
}
public static boolean contains(String key) throws Exception {
	if (key !=null){
	 for (Version release : Version.values()) {
	        if (key.equalsIgnoreCase(release.version)) {
	          return true;
	        }
	      }
	}
	return false;
}
public static void setVersion(String ipaddress,String username,String password) throws Exception {
	
	SSHExec server;
	String versionRegEx = "(version is (\\d{1}\\.\\d{1}))";
	long timeout = 30000;
	
	
	boolean passwordless=false;
	
	//
	//if version has already been initialized then nothing todo. We should only instantiate once
	//
	if ((srv_version != null) && (raw_version !=null)) return;
	
	if (password ==null) passwordless=true;
	
	server = new SSHExec(ipaddress,username,password);
	
	try
	{
		if(passwordless)
		{
			server.connectPasswordless();
		}
		else
		{
			server.connect();
		}
		
		String result = server.exec(ImaCommandReference.SHOW_SERVER_COMMAND, timeout);
		// We only need 'show imaserver' output. Go ahead and disconnect
		server.disconnect();
		
		
		Pattern p = Pattern.compile(versionRegEx);
	    Matcher m = p.matcher(result);
	    m.find();
	    
	    raw_version = m.group();
		
	    if (raw_version.matches("version is 1.0"))
	    {
	    	srv_version="100";
	    } else if (raw_version.matches("version is 1.1"))
	    {
	    	srv_version="110";
	    } else if (raw_version.matches("version is 1.2"))
	    {	
	    	srv_version="120";
	    } else if (raw_version.matches("version is 1.3"))
	    {	
	    	srv_version="130";
	    } else if (raw_version.matches("version is 2.0"))
	    {
	    	srv_version="200";
		} else
		{ 
			//unsupported release throw exception
			throw new Exception ("Unsupported version:"+raw_version);
		}
	    
	    
	}
	catch(JSchException e)
	{
		throw new Exception("We were unable to connect to the server: Check that the ipaddress, username and password is correct" + e);
	}
	// invoke constructor
}
	
};

