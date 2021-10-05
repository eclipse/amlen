/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.ibm.ima.admin.request.IsmConfigSetSubscriptionRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.MessagingResource;
import com.ibm.ima.resources.data.StatusResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class SubscriptionResource
 */
@Path("/config/subscriptions/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class SubscriptionResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = SubscriptionResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public SubscriptionResource() {
        super();
    }

    /**
     * Deletes the Subscription associated with clientId and subscription name.
     * 
     * @param subscriptionName      The subscription name
     * @param clientId              The clientId associated with the subscription name
     * @throws IsmRuntimeException  
     */
    @DELETE
    @Path("{clientId}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeSubscription")
    public Response removeSubscription(@PathParam("domainUid") String serverInstance, @QueryParam("subscriptionName") String subscriptionName, @PathParam("clientId") String clientId) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeSubscription", new Object[] {clientId,subscriptionName});

        StatusResponse response = requestDelete(serverInstance, subscriptionName, clientId);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "removeSubscription", response);
        return rb.cacheControl(cache).build();

    }

    public StatusResponse requestDelete(String serverInstance, String subscriptionName, String clientId) {
        IsmConfigSetSubscriptionRequest configRequest = new IsmConfigSetSubscriptionRequest(getUserId());
        configRequest.setClientId(clientId);
        configRequest.setSubscriptionName(subscriptionName);
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
            			message = subscriptionName;
            			status = Status.NOT_FOUND;
            		} else if (ismResponse.getRC().equals(IN_USE)) {
                        message = subscriptionName;
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
     * Deletes the list of Subscriptions passed in.
     * 
     * @param subscriptions      The list of subscriptions to delete
     * @return The list of subscriptions with the result of each request
     * @throws IsmRuntimeException
     */
    @PUT
    @Path("multipleDelete")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeSubscriptions")
    public Response removeSubscriptions(@PathParam("domainUid") String serverInstance, ArrayList<MessagingResource> subscriptions) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeSubscriptions");

        if (subscriptions == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5004", null);
        }
        
        int numSubs = subscriptions.size();
 
        Locale locale = getClientLocale();
        
        for (int i = 0; i < numSubs; i++) {
            MessagingResource sub = subscriptions.get(i);
            if (sub == null) {
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5004", null);                
            }
            try {                
                StatusResponse requestResponse = requestDelete(serverInstance, sub.getSubscriptionName(), sub.getClientId());
                sub.setRc(requestResponse.getStatus());                
            } catch (IsmRuntimeException ire) {
                sub.setRc(-1);
                sub.setErrorMessage(logger.getFormattedMessage(ire.getMessageID(), ire.getParams(), locale, true));
            }
        }
                
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(subscriptions);

        logger.traceExit(CLAS, "removeSubscriptions");
        return rb.cacheControl(cache).build();
    }

}
