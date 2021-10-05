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

package com.ibm.ima.resources.security;

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

import com.ibm.ima.admin.request.IsmConfigGetFIPSRequest;
import com.ibm.ima.admin.request.IsmConfigSetFIPSRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.AbstractIsmConfigResource;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.json.java.JSONArray;
import com.ibm.json.java.JSONObject;

/**
 * Servlet implementation class FIPSResource
 */
@Path("/config/appliance/fips/{domainUid}")
public class FIPSResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = FIPSResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    public final static int FIPS_RESOURCE_SUCCESS = 0;
    public final static int FIPS_RESOURCE_FAILED = 1;


    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public FIPSResource() {
        super();
    }

    /**
     * Retrieves all SecurityProfiles on the ISM Server.
     * 
     * @return A list of all SecurityProfiles.
     */
    @GET
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getFIPS")
    public Response getFIPS(@PathParam("domainUid") String serverInstance) {  
        logger.traceEntry(CLAS, "getFIPS");

        Fips fips = new Fips();

        IsmConfigGetFIPSRequest configRequest = new IsmConfigGetFIPSRequest(getUserId());
        configRequest.setServerInstance(serverInstance);
        String resultString = (String)sendGetConfigRequest(configRequest, null);

        try {
            JSONArray array = JSONArray.parse(resultString);
            JSONObject obj = (JSONObject) array.get(0);
            fips.setFIPS(Boolean.parseBoolean((String) obj.get("FIPS")));
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getFIPS", "CWLNA5003", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5003", null);
        }


        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(fips);

        logger.traceExit(CLAS, "getFIPS", fips);
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates a (@link SecurityProfile) object in the ISM server configuration.
     * 
     * @param securityProfile The (@link SecurityProfile) object to update.
     * @return SecurityProfileResponse
     */
    @PUT
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "updateFIPS")
    public Response updateFIPS(@PathParam("domainUid") String serverInstance, Fips fips) {
        logger.traceEntry(CLAS, "updateFIPS", new Object[]{fips});
        ResponseBuilder response;

        IsmConfigSetFIPSRequest configRequest = new IsmConfigSetFIPSRequest(getUserId(), fips);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse r = (IsmResponse)resultObject;
            if (r.getRC() != null && !r.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {"[RC=" + r.getRC() + "] " + r.getErrorString()});
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        response = Response.ok();
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "updateFIPS", result.toString());
        return result;
    }

}
