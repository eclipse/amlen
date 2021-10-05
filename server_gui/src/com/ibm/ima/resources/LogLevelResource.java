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

import javax.ws.rs.GET;
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

import com.ibm.ima.admin.IsmClientType;
import com.ibm.ima.admin.request.IsmConfigGetLogLevelRequest;
import com.ibm.ima.admin.request.IsmConfigSetLogLevelRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.LogLevel;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;
import com.ibm.json.java.JSONArray;
import com.ibm.json.java.JSONObject;

/**
 * Servlet implementation class LogLevelResource
 */
@Path("/config/loglevel")
public class LogLevelResource extends AbstractIsmConfigResource {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = LogLevelResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    /**
     * Returns the log level for the appliance
     */
    @GET
    @Path("{logType}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getLogLevel")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getLogLevel(@PathParam("logType") String logType) {
        logger.traceEntry(CLAS, "getLogLevel", new Object[]{logType});
        return getLogLevelDomain(null, logType);
    }

    /**
     * Updates a (@link LogLevel) object in the ISM server configuration for the appliance
     */
    @PUT
    @Path("{logType}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "updateLogLevel")
    public Response updateLogLevel(@PathParam("logType") String logType, LogLevel logLevel) {
        logger.traceEntry(CLAS, "updateLogLevel", new Object[]{logType, logLevel});
        return updateLogLevelDomain(null, logType, logLevel);
    }
    
    /**
     * Returns the log level for the domain
     */
    @GET
    @Path("{domainUid}/{logType}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getLogLevel")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getLogLevelDomain(@PathParam("domainUid") String serverInstance, @PathParam("logType") String logType) {

        LogLevel result = new LogLevel();

        IsmConfigGetLogLevelRequest configRequest = new IsmConfigGetLogLevelRequest(getUserId(), logType);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(configRequest, null);

        if (resultObject instanceof String) {
            try {
                JSONArray array = JSONArray.parse(resultObject.toString());
                JSONObject obj = (JSONObject) array.get(0);
                result.setLogLevel((String) obj.get(logType));
            } catch (Exception e) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
            }
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = logType;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
       }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getLogLevel", result);
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates a (@link LogLevel) object in the ISM server configuration for the domain
     */
    @PUT
    @Path("{domainUid}/{logType}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "updateLogLevel")
    public Response updateLogLevelDomain(@PathParam("domainUid") String serverInstance, @PathParam("logType") String logType, LogLevel logLevel) {
        // validate the request
        logLevel.setLogType(logType);
        ValidationResult validation = logLevel.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetLogLevelRequest configRequest = new IsmConfigSetLogLevelRequest(getUserId(), logLevel);
        configRequest.setServerInstance(serverInstance);
        configRequest.setUpdateAllowedInStandby(true);
        configRequest.setServerInstance(serverInstance);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] {"[RC=" + ismResponse.getRC() + "] " + ismResponse.getErrorString()});
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        // if it's not the MQConnectivityLog, we need to send the request to
        // MQConnectivity, too
        if (!logLevel.isMQConnectivityLogType()) {
            // we don't care if it works or not, we just need to attempt a notify
            configRequest.setClientType(IsmClientType.ISM_ClientType_MQConnectivityConfiguration);
            try {
                resultObject = sendSetConfigRequest(configRequest);
                if (resultObject instanceof IsmResponse) {
                    IsmResponse ismResponse = (IsmResponse) resultObject;
                    if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                        logger.log(IsmLogger.LogLevel.LOG_WARNING, CLAS, "updateLogLevel", "CWLNA5096", new Object[] { logType });
                    } else {
                        logger.trace(CLAS, "updateLogLevel",
                                "MQConnectivity received new log level config: " + logLevel.toString());
                    }
                } else {
                    logger.log(IsmLogger.LogLevel.LOG_WARNING, CLAS, "updateLogLevel", "CWLNA5096", new Object[] { logType });
                }
            } catch (Throwable t) {
                logger.log(IsmLogger.LogLevel.LOG_WARNING, CLAS, "updateLogLevel", "CWLNA5096", new Object[] { logType });
            }
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok();

        Response result = rb.cacheControl(cache).build();

        logger.traceExit(CLAS, "updateLogLevel", result.toString());
        return result;
    }
}
