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

import com.ibm.ima.admin.request.IsmConfigGetHARoleRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.HARoleResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class HARoleResource
 */
@Path("/config/appliance/highavailability/role/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class HARoleResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = HARoleResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    public static final int ROLE_REQUEST_SUCCESSFUL = 0;
    public static final int ROLE_REQUEST_FAILED = 1001;

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public HARoleResource() {
        super();
    }

    /**
     * Retrieves the HA role from the ISM server configuration.
     * 
     * @return The role response
     */
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator,Role.User}, action = "getHARole")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getHARole(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getHARole");

        HARoleResponse response = new HARoleResponse(ROLE_REQUEST_FAILED, null);

        IsmConfigGetHARoleRequest request = new IsmConfigGetHARoleRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = null;
        resultObject = sendGetConfigRequest(request, new TypeReference<HARoleResponse>(){});

        if (resultObject instanceof String) {
            setRoleFromResultsString((String) resultObject, response);
        } else if (resultObject instanceof HARoleResponse) {
            response = (HARoleResponse)resultObject;
            response.setRC(ROLE_REQUEST_SUCCESSFUL);
            response.setHARole(response.getNewRole());
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getHARole", response);
        return rb.cacheControl(cache).build();
    }

    private void setRoleFromResultsString(String result, HARoleResponse response) {
        int roleStart = result.indexOf('=');
        if ((roleStart > 0) && (result.length() > roleStart + 1)) {
            response.setRC(ROLE_REQUEST_SUCCESSFUL);
            response.setHARole(result.substring(roleStart+1).trim());
        }
    }

}
