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

import java.net.URI;
import java.util.ArrayList;
import java.util.List;

import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
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

import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.admin.request.IsmConfigGetMessagingPolicyRequest;
import com.ibm.ima.admin.request.IsmConfigSetMessagingPolicyRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.MessagingPolicy;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class MessagingPolicyResource
 */
@Path("/config/messagingPolicies/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class MessagingPolicyResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = MessagingPolicyResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public MessagingPolicyResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link MessagingPolicy) objects from the ISM server.
     * 
     * @return A list of all {@link MessagingPolicy} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getMessagingPolicies")
    public Response getMessagingPolicies(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getMessagingPolicies");
        List<MessagingPolicy> result = null;

        IsmConfigGetMessagingPolicyRequest request = new IsmConfigGetMessagingPolicyRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<MessagingPolicy>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<MessagingPolicy>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<MessagingPolicy>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getMessagingPolicies", (result == null) ? "NULL" : result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Adds a new (@link MessagingPolicy).
     * 
     * @param policy The (@link MessagingPolicy) to add.
     * @return A response containing the URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addMessagingPolicy")
    public Response addMessagingPolicy(@PathParam("domainUid") String serverInstance, MessagingPolicy policy) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addMessagingPolicy", new Object[] {policy});
        ResponseBuilder response = null;
        
        // even though this couldn't happen, FindBugs complains about it... so add a check
        if (policy == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);            
        }

        // validate the request
        ValidationResult validation = policy.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetMessagingPolicyRequest configRequest = new IsmConfigSetMessagingPolicyRequest(getUserId());
        configRequest.setFieldsFromMessagingPolicy(policy);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        /* respond with the new resource location */
        URI location = getResourceUri(policy.getName());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addMessagingPolicy", (result == null) ? "NULL" : result.toString());
        return result;
    }

    /**
     * Retrieves a specific (@link MessagingPolicy) from the ISM server configuration.
     * 
     * @param policyName The name field of the (@link MessagingPolicy) object to retrieve.
     * @return The {@link MessagingPolicy} object having policyName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{policyName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getMessagingPolicy")
    public Response getMessagingPolicy(@PathParam("domainUid") String serverInstance, @PathParam("policyName") String policyName) {
        logger.traceEntry(CLAS, "getMessagingPolicy", new Object[] {policyName});
        List<MessagingPolicy> result = null;

        IsmConfigGetMessagingPolicyRequest request = new IsmConfigGetMessagingPolicyRequest(policyName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<MessagingPolicy>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = policyName;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<MessagingPolicy>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getMessagingPolicy", (result == null) ? "NULL" : result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates the (@link MessagingPolicy) resource associated with policyName.
     * 
     * @param policyName The name field of the (@link MessagingPolicy) object to retrieve.
     * @param policy The (@link MessagingPolicy) used to update the resource associated with policyName.
     * @return The updated {@link MessagingPolicy} object.
     */
    @PUT
    @Path("{policyName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addMessagingPolicy")
    public Response updateMessagingPolicy(@PathParam("domainUid") String serverInstance, @PathParam("policyName") String policyName, MessagingPolicy policy) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateMessagingPolicy", new Object[] {policy});
        MessagingPolicy result = policy;

        // even though this couldn't happen, FindBugs complains about it... so add a check
        if (policy == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);            
        }
        
        // validate the request
        ValidationResult validation = policy.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetMessagingPolicyRequest configRequest = new IsmConfigSetMessagingPolicyRequest(getUserId());
        policy.keyName = policyName;
        configRequest.setFieldsFromMessagingPolicy(policy);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                Status status = Status.INTERNAL_SERVER_ERROR;
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = policyName;
                    status = Status.BAD_REQUEST;
                } else if (ismResponse.getRC().equals(ISMRC_EndpointMsgPolError) && ismResponse.getErrorString() != null) {
                    // look for the endpoint name in the message 
                    String[] parts = ismResponse.getErrorString().split("\"");
                    if (parts.length > 2) {
                        message = parts[1];
                        status = Status.BAD_REQUEST;
                    } 
                }
                throw new IsmRuntimeException(status, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "updateMessagingPolicy", (result == null) ? "NULL" : result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the (@link MessagingPolicy) resource associated with policyName.
     * 
     * @param policyName The name field of the (@link MessagingPolicy) object to retrieve.
     */
    @DELETE
    @Path("{policyName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeMessagingPolicy")
    public void removeMessagingPolicy(@PathParam("domainUid") String serverInstance, @PathParam("policyName") String policyName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeMessagingPolicy", new Object[] {policyName});

        IsmConfigSetMessagingPolicyRequest configRequest = new IsmConfigSetMessagingPolicyRequest(getUserId());
        configRequest.setName(policyName);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                Status statusRC = Status.INTERNAL_SERVER_ERROR;
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = policyName;
                    statusRC = Status.NOT_FOUND;
                } else if (ismResponse.getRC().equals(Utils.PENDING_DELETE)) {
                	statusRC = Status.CONFLICT;
                }
                throw new IsmRuntimeException(statusRC, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS, "removeMessagingPolicy");
        return;
    }
}
