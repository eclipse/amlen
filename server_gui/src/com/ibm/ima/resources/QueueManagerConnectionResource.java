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
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.admin.request.IsmConfigGetQueueManagerConnRequest;
import com.ibm.ima.admin.request.IsmConfigSetQueueManagerConnRequest;
import com.ibm.ima.admin.request.IsmConfigTestQueueManagerConnRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.QueueManagerConnection;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class QueueManagerConnectionResource
 */
@Path("/config/mqconnectivity/queueManagerConnections/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class QueueManagerConnectionResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = QueueManagerConnectionResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public QueueManagerConnectionResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link QueueManagerConnection) objects from the ISM server.
     * 
     * @return A list of all {@link QueueManagerConnection} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getQueueManagerConnections")
    public Response getQueueManagerConnections(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getQueueManagerConnections");

        List<QueueManagerConnection> result;

        IsmConfigGetQueueManagerConnRequest request = new IsmConfigGetQueueManagerConnRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<QueueManagerConnection>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<QueueManagerConnection>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<QueueManagerConnection>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getQueueManagerConnections", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Retrieves a (@link QueueManagerConnection) with the specified name from the ISM server configuration.
     * 
     * @param qmConnName The name field of the (@link QueueManagerConnection) object to retrieve.
     * @return The {@link QueueManagerConnection} object having qmConnName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{qmConnName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getQueueManagerConnectionByName")
    public Response getQueueManagerConnectionByName(@PathParam("domainUid") String serverInstance, @PathParam("qmConnName") String qmConnName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getQueueManagerConnectionByName", new Object[]{qmConnName});

        List<QueueManagerConnection> result = null;

        IsmConfigGetQueueManagerConnRequest request = new IsmConfigGetQueueManagerConnRequest(qmConnName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<QueueManagerConnection>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = qmConnName;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<QueueManagerConnection>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getQueueManagerConnectionByName", result.toString());
        return rb.cacheControl(cache).build();
    }


    /**
     * Adds a (@link QueueManagerConnection) object to the ISM server configuration.
     * 
     * @param QueueManagerConnection The (@link QueueManagerConnection) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addQueueManagerConnection")
    public Response addQueueManagerConnection(@PathParam("domainUid") String serverInstance, QueueManagerConnection queueManagerConnection) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addQueueManagerConnection", new Object[]{queueManagerConnection});
        ResponseBuilder response;

        // validate the request
        ValidationResult validation = queueManagerConnection.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetQueueManagerConnRequest configRequest = new IsmConfigSetQueueManagerConnRequest(getUserId());
        configRequest.setFieldsFromQueueManagerConnection(queueManagerConnection);
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
        URI location = getResourceUri(queueManagerConnection.getName());
        logger.trace(CLAS, "addQueueManagerConnection", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addQueueManagerConnection", result.toString());
        return result;
    }


    /**
     * Updates the (@link QueueManagerConnection) resource associated with QueueManagerConnection.
     * 
     * @param qmConnName The id field of the (@link QueueManagerConnection) object to retrieve.
     * @param queueManagerConnection The (@link QueueManagerConnection) used to update the resource associated with qmConnName.
     * @return The updated {@link QueueManagerConnection} object.
     */
    @PUT
    @Path("{qmConnName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "updateQueueManagerConnection")
    public Response updateQueueManagerConnection(@PathParam("domainUid") String serverInstance, @PathParam("qmConnName") String qmConnName, QueueManagerConnection queueManagerConnection) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateQueueManagerConnection", new Object[]{qmConnName});
        QueueManagerConnection result = queueManagerConnection;

        // validate the request
        ValidationResult validation = queueManagerConnection.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetQueueManagerConnRequest configRequest = new IsmConfigSetQueueManagerConnRequest(getUserId());
        queueManagerConnection.setKeyName(qmConnName);
        configRequest.setFieldsFromQueueManagerConnection(queueManagerConnection);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = qmConnName;
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
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "updateQueueManagerConnection", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the (@link QueueManagerConnection) resource associated with qmConnName.
     * 
     * @param qmConnName The id field of the (@link QueueManagerConnection) object to retrieve.
     */
    @DELETE
    @Path("{qmConnName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeQueueManagerConnection")
    public void removeQueueManagerConnection(@PathParam("domainUid") String serverInstance, @PathParam("qmConnName") String qmConnName, @QueryParam("Force") boolean force) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeQueueManagerConnection", new Object[] {qmConnName});

        IsmConfigSetQueueManagerConnRequest configRequest = new IsmConfigSetQueueManagerConnRequest(getUserId());
        configRequest.setName(qmConnName);
        configRequest.setDelete();
        if (force) {
            configRequest.setForce();
        }
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = qmConnName;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeQueueManagerConnection");
        return;
    }
    
    /**
     * Test a (@link QueueManagerConnection) object.
     * 
     * @param QueueManagerConnection The (@link QueueManagerConnection) object to test.
     * @return The URI of the added resource.
     */
    @POST  // make it a post since a name is not required
    @Path("actions/testConnection")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "testQueueManagerConnection")
    public Response testQueueManagerConnection(@PathParam("domainUid") String serverInstance, QueueManagerConnection queueManagerConnection) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "testQueueManagerConnection", new Object[]{queueManagerConnection});
        ResponseBuilder response;

        // validate the request
        queueManagerConnection.setNameRequired(false);
        ValidationResult validation = queueManagerConnection.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigTestQueueManagerConnRequest configRequest = new IsmConfigTestQueueManagerConnRequest(getUserId());
        configRequest.setFieldsFromQueueManagerConnection(queueManagerConnection);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        // if we didn't get a valid resultObject with a non-null RC, throw an exception
        if (!(resultObject instanceof IsmResponse && ((IsmResponse)resultObject).getRC() != null)) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        response = Response.ok(resultObject);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "testQueueManagerConnection", result.toString());
        return result;
    }

}
