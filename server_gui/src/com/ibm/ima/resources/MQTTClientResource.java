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

package com.ibm.ima.resources;

import java.util.ArrayList;
import java.util.Locale;

import javax.ws.rs.DELETE;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.ibm.ima.admin.request.IsmConfigSetMQTTClientRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.MessagingResource;
import com.ibm.ima.resources.data.StatusResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class MQTTClientResource
 */
@Path("/config/mqttClients/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class MQTTClientResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = MQTTClientResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public MQTTClientResource() {
        super();
    }

    /**
     * Deletes the MQTT client associated with clientId.
     * 
     * @param clientId The client ID of the MQTT client to delete
     * @return 
     */
    @DELETE
    @Path("{clientId}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeMQTTClient")
    public Response removeMQTTClient(@PathParam("domainUid") String serverInstance, @PathParam("clientId") String clientId) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeMQTTClient", new Object[] {clientId});
        
        StatusResponse response = requestDelete(serverInstance, clientId);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS,  "removeMQTTClient");
        return rb.cacheControl(cache).build();
    }

    private StatusResponse requestDelete(String serverInstance, String clientId) {
        IsmConfigSetMQTTClientRequest configRequest = new IsmConfigSetMQTTClientRequest(getUserId());
        configRequest.setName(clientId);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        StatusResponse response = null;

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null) {
                if (ismResponse.getRC().equals("0")) {
                    response = new StatusResponse(StatusResponse.RESPONSE_OK);
                } else if (ismResponse.getRC().equals(Utils.ASYNC_COMPLETION)) {
                    response = new StatusResponse(StatusResponse.ASYNC_COMPLETION);
                } else {
                    String message = ismResponse.toString();
                    Status status = Status.INTERNAL_SERVER_ERROR;
                    if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                        message = clientId;
                        status = Status.NOT_FOUND;
                    } else if (ismResponse.getRC().equals(IN_USE) || ismResponse.getRC().equals(CLIENT_IN_USE)) {
                        message = clientId;
                        status = Status.FORBIDDEN;                      
                    }
                    throw new IsmRuntimeException(status, Utils.getUtils()
                            .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
                }               
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }
        
        return response;
    }
    
    /**
     * Deletes the list of clients passed in.
     * 
     * @param clients      The list of clients to delete
     * @return The list of clients with the result of each request
     * @throws IsmRuntimeException
     */
    @PUT
    @Path("multipleDelete")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeClients")
    public Response removeClients(@PathParam("domainUid") String serverInstance, ArrayList<MessagingResource> clients) throws IsmRuntimeException {

        if (clients == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5004", null);
        }
        
        int numSubs = clients.size();
 
        Locale locale = getClientLocale();
        
        for (int i = 0; i < numSubs; i++) {
            MessagingResource client = clients.get(i);
            if (client == null) {
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5004", null);                
            }
            try {                
                StatusResponse requestResponse = requestDelete(serverInstance, client.getClientId());
                client.setRc(requestResponse.getStatus());                
            } catch (IsmRuntimeException ire) {
                client.setRc(-1);
                client.setErrorMessage(logger.getFormattedMessage(ire.getMessageID(), ire.getParams(), locale, true));
            }
        }
                
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(clients);

        logger.traceExit(CLAS, "removeClients");
        return rb.cacheControl(cache).build();
    }

}
