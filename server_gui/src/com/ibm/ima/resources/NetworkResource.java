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

package com.ibm.ima.resources;

import java.net.InetAddress;
import java.net.UnknownHostException;

import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.data.NetworkInterface;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.json.java.JSONObject;

/**
 * Servlet implementation class NetworkResource.
 * 
 * This class is intended for us only when view only information is needed and is needed to support CCI.
 */
@Path("/config/appliance/network-interfaces")
@Produces(MediaType.APPLICATION_JSON)
public class NetworkResource extends AbstractResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    private final static String CLAS = NetworkResource.class.getCanonicalName();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public NetworkResource() {
        logger.traceExit(CLAS, "NetworkResource");
    }
    
    /**
     * Determine the rest address from the provided request
     * @param request the incoming request
     * @return the address or null if could not be determined
     */
    public static String getRestAddress(HttpServletRequest request) {
        String address = null;

        if (request != null) {
            String requestServerName = request.getServerName();
            if (requestServerName != null) {
                try {
                    address = InetAddress.getByName(requestServerName).getHostAddress();
                } catch (UnknownHostException e) {
                    logger.trace(CLAS, "getRestAddress", "Could not get IP address of hostname from current request server name " + requestServerName);
                }
            } else {
                logger.trace(CLAS, "getRestAddress", "Could not get server name from current request " + request.toString());
            }
        } else {
            logger.trace(CLAS, "getRestAddress", "request is null");
        }

        return address;
    }
    
    // TODO make better in future...
    private NetworkInterfaceResponse getNetworkInterfaces(boolean haInterfaces) {
		NetworkInterfaceResponse response = new NetworkInterfaceResponse();
        String address = getRestAddress(currentRequest);
        if (address != null) {
            response.setRestAddress(address);
        }
        
        return response;
    }

    /**
     * Returns a list of all available ethernet resource object values.
     * 
     * @return A {@link NetworkResponse} containing operation rc and a list for all ethernet resource values.
     */
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getNetworkResources")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getNetworkResources() {
        logger.traceEntry(CLAS, "getNetworkResources");
        NetworkInterfaceResponse response = getNetworkInterfaces(false);        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getNetworkResources", response);
        return rb.cacheControl(cache).build();
    }

    private class NetworkInterfaceResponse {
    	private int rc = 0;
        private String restAddress = "";
        private JSONObject interfaces;  // an object where the key is an interface name and the value is an NetworkInterface object
        
        /*
         * Handle any property we don't know about.
         */
        @JsonAnySetter
        public void handleUnknown(String key, Object value) {
            logger.trace(CLAS, "handleUnknown", "Unknown property " + key + " with value " + value + " received");
        }
        
        @JsonProperty("RC")
        public int getRC() {
            return rc;
        }
        
        @JsonProperty("interfaces")
        public JSONObject [] getInterfaces() {
        	JSONObject[] obj = new JSONObject[1];
        	logger.trace(CLAS, "getInterfaces", "restAddress: " + restAddress);
        	if (restAddress != null) {
        		obj[0] = new JSONObject();
        		obj[0].put("ipaddress", restAddress);
        		return obj;
        	}
            return null;
        }

        /**
         * Set the IP address the REST request came in on
         * @param restAddress the restAddress to set
         */
        public void setRestAddress(String restAddress) {
            this.restAddress = restAddress;
        }
        
        /*
         *  Add an interface to the interfaces object
         */
        @JsonIgnore
        public void addInterface(String interfaceName, NetworkInterface networkInterface) {
            if (interfaceName == null || networkInterface == null) {
                return;
            }
            if (interfaces == null) {
                interfaces = new JSONObject();
            }
            
            interfaces.put(interfaceName,networkInterface.asJsonObject());
        }

        /*
         *  Add an interface to the interfaces object
         */
        @JsonIgnore
        public void addInterface(String interfaceName, JSONObject networkInterface) {
            if (interfaceName == null || networkInterface == null) {
                return;
            }
            if (interfaces == null) {
                interfaces = new JSONObject();
            }
            
            interfaces.put(interfaceName,networkInterface);
        }
    }

}

