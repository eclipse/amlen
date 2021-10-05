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

import com.ibm.ima.admin.request.IsmConfigGetTopicMonitorRequest;
import com.ibm.ima.admin.request.IsmConfigSetTopicMonitorRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.TopicMonitor;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class TopicMonitorResource
 */
@Path("/config/topicMonitors/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class TopicMonitorResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = TopicMonitorResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public TopicMonitorResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link TopicMonitor) objects from the ISM server.
     * 
     * @return A list of all {@link TopicMonitor} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator,Role.User}, action = "getTopicMonitors")
    public Response getTopicMonitors(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getTopicMonitors");

        List<TopicMonitor> result;

        IsmConfigGetTopicMonitorRequest request = new IsmConfigGetTopicMonitorRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<TopicMonitor>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<TopicMonitor>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<TopicMonitor>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getTopicMonitors", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Adds a (@link TopicMonitor) object to the ISM server configuration.
     * 
     * @param topicMonitor The (@link TopicMonitor) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addTopicMonitor")
    public Response addTopicMonitor(@PathParam("domainUid") String serverInstance, TopicMonitor topicMonitor) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addTopicMonitor", new Object[]{topicMonitor});
        ResponseBuilder response;

        // validate the request
        ValidationResult validation = topicMonitor.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetTopicMonitorRequest configRequest = new IsmConfigSetTopicMonitorRequest(getUserId());
        configRequest.setFieldsFromTopicMonitor(topicMonitor);
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
        URI location = getResourceUri(topicMonitor.getTopicString());
        logger.trace(CLAS, "addTopicMonitor", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addTopicMonitor", result.toString());
        return result;
    }

    /**
     * Retrieves a (@link TopicMonitor) with the specified id from the ISM server configuration.
     * 
     * @param topicString The id field of the (@link TopicMonitor) object to retrieve.
     * @return The {@link TopicMonitor} object having topicString.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{topicString}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getTopicMonitorById")
    public Response getTopicMonitorById(@PathParam("domainUid") String serverInstance, @PathParam("topicString") String topicString) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getTopicMonitorById", new Object[]{topicString});

        List<TopicMonitor> result = null;

        IsmConfigGetTopicMonitorRequest request = new IsmConfigGetTopicMonitorRequest(topicString, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<TopicMonitor>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = topicString;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<TopicMonitor>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getTopicMonitorById", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes a top-level topic monitor (empty topic string)
     */
    @DELETE
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeTopicMonitor")
    public void removeTopTopicMonitor(@PathParam("domainUid") String serverInstance, String topicString) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeTopTopicMonitor");

        IsmConfigSetTopicMonitorRequest configRequest = new IsmConfigSetTopicMonitorRequest(getUserId());
        configRequest.setTopicString("");
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = topicString;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeTopTopicMonitor");
        return;
    }

    /**
     * Deletes the (@link TopicMonitor) resource associated with topicString.
     * 
     * @param topicString The id field of the (@link TopicMonitor) object to retrieve.
     */
    @DELETE
    @Path("{topicString}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeTopicMonitor")
    public void removeTopicMonitor(@PathParam("domainUid") String serverInstance, @PathParam("topicString") String topicString) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeTopicMonitor", new Object[] {topicString});

        IsmConfigSetTopicMonitorRequest configRequest = new IsmConfigSetTopicMonitorRequest(getUserId());
        configRequest.setTopicString(topicString);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = topicString;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeTopicMonitor");
        return;
    }
}
