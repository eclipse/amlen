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
package com.ibm.ima.jms.test.jca;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Scanner;

import org.apache.wink.client.ClientConfig;
import org.apache.wink.client.ClientResponse;
import org.apache.wink.client.Resource;
import org.apache.wink.client.RestClient;

public final class AUtils {
	
	/**
	 * getApplianceIPs should always return the primary server IP as the first
	 * element in the array.
	 * @return
	 * @throws FileNotFoundException
	 */
	public static String[] getApplianceIPs() throws FileNotFoundException {
		/*
         * If java security is enabled, the properties file needs to be opened
         * in a privileged action
         */
        FileInputStream f = AccessController.doPrivileged(new PrivilegedAction<FileInputStream>() {
            public FileInputStream run() {
                try {
                    return new FileInputStream(Utils.WASPATH() + "/env.file");
                } catch (Exception e) {
                    return null;
                }
            }
        });
        if(f == null)
            throw new FileNotFoundException(Utils.WASPATH() + "/env.file does not exist");
	
        Scanner s = new Scanner(f);
        String a1 = s.nextLine();
        String a2 = s.nextLine();
        s.close();
        
        if (isPrimary(a1))
        	return new String[] {a1, a2};
        else        
        	return new String[] {a2, a1};
	}
	
	/**
	 * Get the status of the given hostname. If it is "Status = Running", return true
	 * that hostname is the primary server.
	 * @param hostname
	 * @return
	 */
	public static boolean isPrimary(String hostname) {
		ClientConfig clientConfig = new ClientConfig();
	
		// Define resource
		RestClient client = new RestClient(clientConfig);
		Resource resource = client.resource("http://"+hostname+":9089/ima/v1/service/status");
		ClientResponse response = null;
		response = resource.contentType("application/json").accept("*/*").get();

		String entity = response.getEntity(String.class);
		
		if (entity == null) {
			return false;
		}
		
		if (entity.contains("Running (production)")) {
			return true;
		} else {
			return false;
		}
	}
	
	public static void restartServer(String hostname) {
		ClientConfig clientConfig = new ClientConfig();
		
		// Define resource
		RestClient client = new RestClient(clientConfig);
		Resource resource = client.resource("http://"+hostname+":9089/ima/v1/service/restart");
		resource.contentType("application/json").accept("*/*").post("{\"Service\":\"Server\"}");
	}
	
	public static void stopServer(String hostname) {
		ClientConfig clientConfig = new ClientConfig();
		
		// Define resource
		RestClient client = new RestClient(clientConfig);
		Resource resource = client.resource("http://"+hostname+":9089/ima/v1/service/stop");
		resource.contentType("application/json").accept("*/*").post("{\"Service\":\"Server\"}");
	}
}
