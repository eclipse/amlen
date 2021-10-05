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

import com.ibm.ima.admin.request.IsmConfigGetConnectionPolicyRequest;
import com.ibm.ima.admin.request.IsmConfigSetConnectionPolicyRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.ConnectionPolicy;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class ConnectionPolicyResource
 */
@Path("/config/connectionPolicies/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class ConnectionPolicyResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = ConnectionPolicyResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public ConnectionPolicyResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link ConnectionPolicy) objects from the ISM server.
     * 
     * @return A list of all {@link ConnectionPolicy} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getConnectionPolicies")
    public Response getConnectionPolicies(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getConnectionPolicies");
        List<ConnectionPolicy> result = null;

        IsmConfigGetConnectionPolicyRequest request = new IsmConfigGetConnectionPolicyRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<ConnectionPolicy>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<ConnectionPolicy>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<ConnectionPolicy>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getConnectionPolicies", (result == null) ? "NULL" : result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Adds a new (@link ConnectionPolicy).
     * 
     * @param policy The (@link ConnectionPolicy) to add.
     * @return A response containing the URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addConnectionPolicy")
    public Response addConnectionPolicy(@PathParam("domainUid") String serverInstance, ConnectionPolicy policy) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addConnectionPolicy", new Object[] {policy});
        ResponseBuilder response = null;

        // validate the request
        policy.setServerInstance(serverInstance);
        ValidationResult validation = policy.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetConnectionPolicyRequest configRequest = new IsmConfigSetConnectionPolicyRequest(getUserId());
        configRequest.setFieldsFromConnectionPolicy(policy);
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

        logger.traceExit(CLAS, "addConnectionPolicy", (result == null) ? "NULL" : result.toString());
        return result;
    }

    /**
     * Retrieves a specific (@link ConnectionPolicy) from the ISM server configuration.
     * 
     * @param policyName The name field of the (@link ConnectionPolicy) object to retrieve.
     * @return The {@link ConnectionPolicy} object having policyName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{policyName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getConnectionPolicy")
    public Response getConnectionPolicy(@PathParam("domainUid") String serverInstance, @PathParam("policyName") String policyName) {
        logger.traceEntry(CLAS, "getConnectionPolicy", new Object[] {policyName});
        List<ConnectionPolicy> result = null;

        IsmConfigGetConnectionPolicyRequest request = new IsmConfigGetConnectionPolicyRequest(policyName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<ConnectionPolicy>>(){});
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
            result = (List<ConnectionPolicy>)resultObject;
        }
        ConnectionPolicy response = result != null && !result.isEmpty() ? result.get(0) : null;
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getConnectionPolicy", (response == null) ? "NULL" : response.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates the (@link ConnectionPolicy) resource associated with policyName.
     * 
     * @param policyName The name field of the (@link ConnectionPolicy) object to retrieve.
     * @param policy The (@link ConnectionPolicy) used to update the resource associated with policyName.
     * @return The updated {@link ConnectionPolicy} object.
     */
    @PUT
    @Path("{policyName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addConnectionPolicy")
    public Response updateConnectionPolicy(@PathParam("domainUid") String serverInstance, @PathParam("policyName") String policyName, ConnectionPolicy policy) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateConnectionPolicy", new Object[] {policy});
        ConnectionPolicy result = policy;

        // even though this couldn't happen, FindBugs complains about it... so add a check
        if (policy == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);            
        }

        // validate the request
        policy.setServerInstance(serverInstance);
        ValidationResult validation = policy.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }


        IsmConfigSetConnectionPolicyRequest configRequest = new IsmConfigSetConnectionPolicyRequest(getUserId());
        policy.keyName = policyName;
        configRequest.setFieldsFromConnectionPolicy(policy);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = policyName;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(policy);

        logger.traceExit(CLAS, "updateConnectionPolicy", (result == null) ? "NULL" : result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the (@link ConnectionPolicy) resource associated with policyName.
     * 
     * @param policyName The name field of the (@link ConnectionPolicy) object to retrieve.
     */
    @DELETE
    @Path("{policyName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeConnectionPolicy")
    public void removeConnectionPolicy(@PathParam("domainUid") String serverInstance, @PathParam("policyName") String policyName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeConnectionPolicy", new Object[] {policyName});

        IsmConfigSetConnectionPolicyRequest configRequest = new IsmConfigSetConnectionPolicyRequest(getUserId());
        configRequest.setName(policyName);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = policyName;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS, "removeConnectionPolicy");
        return;
    }
}
