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

import com.ibm.ima.admin.request.IsmConfigGetMessageHubRequest;
import com.ibm.ima.admin.request.IsmConfigSetMessageHubRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.MessageHub;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class MessageHubResource
 */
@Path("/config/messagehubs/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class MessageHubResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = MessageHubResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public MessageHubResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link MessageHub) objects from the ISM server.
     * 
     * @return A list of all {@link MessageHub} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getMessageHubs")
    public Response getMessageHubs(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getMessageHubs");

        List<MessageHub> result;

        IsmConfigGetMessageHubRequest request = new IsmConfigGetMessageHubRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<MessageHub>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<MessageHub>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<MessageHub>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getMessageHubs", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Retrieves a (@link MessageHub) with the specified name from the ISM server configuration.
     * 
     * @param messageHubName The name field of the (@link MessageHub) object to retrieve.
     * @return The {@link MessageHub} object having messageHubName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{messageHubName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getMessageHubByName")
    public Response getMessageHubByName(@PathParam("domainUid") String serverInstance, @PathParam("messageHubName") String messageHubName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getMessageHubByName", new Object[]{messageHubName});

        List<MessageHub> result = null;

        IsmConfigGetMessageHubRequest request = new IsmConfigGetMessageHubRequest(messageHubName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<MessageHub>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = messageHubName;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<MessageHub>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getMessageHubByName", result.toString());
        return rb.cacheControl(cache).build();
    }


    /**
     * Adds a (@link MessageHub) object to the ISM server configuration.
     * 
     * @param messageHub The (@link MessageHub) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addMessageHub")
    public Response addMessageHub(@PathParam("domainUid") String serverInstance, MessageHub messageHub) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addMessageHub", new Object[]{messageHub});
        ResponseBuilder response;

        // validate the request
        ValidationResult validation = messageHub.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetMessageHubRequest configRequest = new IsmConfigSetMessageHubRequest(getUserId());
        configRequest.setFieldsFromMessageHub(messageHub);
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
        URI location = getResourceUri(messageHub.getName());
        logger.trace(CLAS, "addMessageHub", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addMessageHub", result.toString());
        return result;
    }


    /**
     * Updates the (@link MessageHub) resource associated with messageHubName.
     * 
     * @param messageHubName The id field of the (@link MessageHub) object to retrieve.
     * @param messageHub The (@link MessageHub) used to update the resource associated with messageHubName.
     * @return The updated {@link MessageHub} object.
     */
    @PUT
    @Path("{messageHubName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "updateMessageHub")
    public Response updateMessageHub(@PathParam("domainUid") String serverInstance, @PathParam("messageHubName") String messageHubName, MessageHub messageHub) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateMessageHub", new Object[]{messageHub});
        MessageHub result = messageHub;

        // validate the request
        ValidationResult validation = messageHub.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetMessageHubRequest configRequest = new IsmConfigSetMessageHubRequest(getUserId());
        messageHub.setKeyName(messageHubName);
        configRequest.setFieldsFromMessageHub(messageHub);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = messageHubName;
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

        logger.traceExit(CLAS, "updateMessageHub", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the (@link MessageHub) resource associated with messageHubName.
     * 
     * @param messageHubName The id field of the (@link MessageHub) object to retrieve.
     */
    @DELETE
    @Path("{messageHubName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeMessageHub")
    public void removeMessageHub(@PathParam("domainUid") String serverInstance, @PathParam("messageHubName") String messageHubName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeMessageHub", new Object[] {messageHubName});

        IsmConfigSetMessageHubRequest configRequest = new IsmConfigSetMessageHubRequest(getUserId());
        configRequest.setName(messageHubName);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = messageHubName;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeMessageHub");
        return;
    }
}
