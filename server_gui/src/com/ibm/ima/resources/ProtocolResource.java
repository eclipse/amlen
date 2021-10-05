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

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;

import javax.ws.rs.GET;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.admin.request.IsmConfigGetProtocolRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.Protocol;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class ProtocolResource
 */
@Path("/config/protocols/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class ProtocolResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = ProtocolResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public ProtocolResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link Protocol) objects from the ISM server.
     * 
     * @return A list of all {@link Protocol} objects.
     */
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getProtocols")
    public Response getProtocols(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getProtocols");

        List<Protocol> result = getProtocolList(serverInstance);
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getProtocols", result.toString());
        return rb.cacheControl(cache).build();
    }
    
    /**
     * Obtain a List of protocols from the admin component.
     * 
     * @return  A List of protocols and their capabilities.
     * @throws IsmRuntimeException
     */
    @SuppressWarnings("unchecked")
    public List<Protocol> getProtocolList(String serverInstance) throws IsmRuntimeException {
    	
        logger.traceEntry(CLAS, "getProtocolList");

        List<Protocol> result;

        IsmConfigGetProtocolRequest request = new IsmConfigGetProtocolRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Protocol>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<Protocol>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<Protocol>)resultObject;
        }
        
        logger.traceExit(CLAS, "getProtocolList", result.toString());
        return result;
    }
    
    /**
     * Obtain a map of all known protocols
     * 
     * @return  A map of all known protocols
     */
    public HashMap<String, Protocol> getProtocolMap(String serverInstance) {
    	
    	// get the list of protocols from the admin component
		List<Protocol> protocolList = getProtocolList(serverInstance);
		HashMap<String, Protocol> protocolMap = new HashMap<String, Protocol>();
		
		// populate the map - make sure key is lowercase
		for (Protocol p: protocolList) {
			protocolMap.put(p.getName().toLowerCase(), p);
		}
	
		return protocolMap;
		
    }


    /**
     * Retrieves a (@link Protocol) with the specified name from the ISM server configuration.
     * 
     * @param protocolName The name field of the (@link Protocol) object to retrieve.
     * @return The {@link Protocol} object having protocolName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{protocolName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getProtocolByName")
    public Response getProtocolByName(@PathParam("domainUid") String serverInstance, @PathParam("protocolName") String protocolName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getProtocolByName", new Object[]{protocolName});

        List<Protocol> result = null;

        IsmConfigGetProtocolRequest request = new IsmConfigGetProtocolRequest(protocolName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Protocol>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
        	Status status = Status.INTERNAL_SERVER_ERROR;
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
            	status = Status.NOT_FOUND;
                message = protocolName;
            }
            throw new IsmRuntimeException(status, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<Protocol>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getProtocolByName", result.toString());
        return rb.cacheControl(cache).build();
    }

}
