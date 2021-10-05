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

import java.util.List;

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

import com.ibm.ima.admin.request.IsmConfigGetHAPairRequest;
import com.ibm.ima.admin.request.IsmConfigSetHAPairRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.HAObject;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class HAPairResource
 */
@Path("/config/appliance/highavailability/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class HAPairResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = HAPairResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    public final static int MIN_HEARTBEAT_TIMEOUT = 1;
    public final static int MIN_DISCOVERY_TIMEOUT = 10;
    public final static int DEFAULT_HEARTBEAT_TIMEOUT = 10;
    public final static int DEFAULT_DISCOVERY_TIMEOUT = 600;
    public final static int MAX_TIMEOUT = 2147483647;
    public final static String DEFAULT_STARTUP_MODE = "AutoDetect";

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public HAPairResource() {
        super();
    }

    /**
     * Retrieves a (@link HAObject) from the ISM server configuration.
     * 
     * @return The {@link HAObject} object retrieved from the server.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getHAObject")
    public Response getHAObject(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getHAObject");

        List<HAObject> result;

        IsmConfigGetHAPairRequest request = new IsmConfigGetHAPairRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<HAObject>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        } else {
            result = (List<HAObject>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getHAObject", result);
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates the (@link HAObject) resource associated with HAObject.
     * 
     * @param haObject The (@link HAObject) used to update the HA resource.
     * @return The HAObject used for updating the HA resource.
     */
    @PUT
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "updateHAPairObject")
    public Response updateHAPairObject(@PathParam("domainUid") String serverInstance, HAObject haObject) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateHAPairObject", new Object[]{haObject});

        HAObject result = haObject;
        // validate the request
        ValidationResult validation = haObject.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        } else {
            logger.trace(CLAS, "updateHAPairObject", "Validation is successful: " + validation.toString());
        }

        IsmConfigSetHAPairRequest configRequest = new IsmConfigSetHAPairRequest(getUserId());
        configRequest.setFieldsFromHAObject(haObject);
        configRequest.setUpdate();
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

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "updateHAPairObject", result);
        return rb.cacheControl(cache).build();
    }

    /**
     * Adds a (@link HAObject) object to the ISM server configuration.
     * 
     * @param haObject The (@link HAObject) to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "createHAPairObject")
    public Response createHAPairObject(@PathParam("domainUid") String serverInstance, HAObject haObject) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "createHAPairObject", new Object[]{haObject});

        HAObject result = haObject;
        // validate the request
        ValidationResult validation = haObject.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetHAPairRequest configRequest = new IsmConfigSetHAPairRequest(getUserId());
        configRequest.setFieldsFromHAObject(haObject);
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

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "createHAPairObject", result);
        return rb.cacheControl(cache).build();
    }


}
