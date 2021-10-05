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

import com.ibm.ima.admin.request.IsmConfigGetQueueRequest;
import com.ibm.ima.admin.request.IsmConfigSetQueueRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.Queue;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class QueueAdministrationResource
 */
@Path("/config/queues/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class QueueAdministrationResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = QueueAdministrationResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public QueueAdministrationResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link Queue) objects from the ISM server.
     * 
     * @return A list of all {@link Queue} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getQueues")
    public Response getQueues(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getQueues");

        List<Queue> result;

        IsmConfigGetQueueRequest request = new IsmConfigGetQueueRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Queue>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<Queue>();
            } else {
                logger.trace(CLAS, "getQueues", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<Queue>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getQueues", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Retrieves a (@link Queue) with the specified name from the ISM server configuration.
     * 
     * @param queueName The name field of the (@link Queue) object to retrieve.
     * @return The {@link Queue} object having queueName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{queueName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getQueueByName")
    public Response getQueueByName(@PathParam("domainUid") String serverInstance, @PathParam("queueName") String queueName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getQueueByName", new Object[]{queueName});

        List<Queue> result = null;

        IsmConfigGetQueueRequest request = new IsmConfigGetQueueRequest(queueName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Queue>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {         
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = queueName;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<Queue>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getQueueByName", result.toString());
        return rb.cacheControl(cache).build();
    }


    /**
     * Adds a (@link Queue) object to the ISM server configuration.
     * 
     * @param Queue The (@link Queue) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addQueue")
    public Response addQueue(@PathParam("domainUid") String serverInstance, Queue queue) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addQueue", new Object[]{queue});
        ResponseBuilder response;

        // validate the request
        //TODO: check the correct validation is used
        ValidationResult validation = queue.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetQueueRequest configRequest = new IsmConfigSetQueueRequest(getUserId());
        configRequest.setFieldsFromQueue(queue);
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
        URI location = getResourceUri(queue.getName());
        logger.trace(CLAS, "addQueue", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addQueue", result.toString());
        return result;
    }


    /**
     * Updates the (@link Queue) resource associated with Queue.
     * 
     * @param queueName The id field of the (@link Queue) object to retrieve.
     * @param queue The (@link Queue) used to update the resource associated with queueName.
     * @return The updated {@link Queue} object.
     */
    @PUT
    @Path("{queueName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "updateQueue")
    public Response updateQueue(@PathParam("domainUid") String serverInstance, @PathParam("queueName") String queueName, Queue queue) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateQueue", new Object[]{queueName});
        Queue result = queue;

        // validate the request
        //TODO: check the validation
        ValidationResult validation = queue.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetQueueRequest configRequest = new IsmConfigSetQueueRequest(getUserId());
        queue.setKeyName(queueName);
        configRequest.setFieldsFromQueue(queue);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = queueName;
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

        logger.traceExit(CLAS, "updateQueue", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the (@link Queue) resource associated with queueName.
     * 
     * @param queueName The id field of the (@link Queue) object to retrieve.
     */
    @DELETE
    @Path("{queueName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeQueue")
    public void removeQueue(@PathParam("domainUid") String serverInstance, @PathParam("queueName") String queueName, @QueryParam("DiscardMessages") boolean discard) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeQueue", new Object[] {queueName});

        IsmConfigSetQueueRequest configRequest = new IsmConfigSetQueueRequest(getUserId());
        configRequest.setName(queueName);
        configRequest.setDelete();
        if (discard) {
            configRequest.setDiscardMessages();
        }
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = queueName;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeQueue");
        return;
    }
}
