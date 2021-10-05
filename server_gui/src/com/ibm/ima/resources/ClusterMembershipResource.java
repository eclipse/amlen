/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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

import com.ibm.ima.admin.request.IsmConfigGetClusterMembershipRequest;
import com.ibm.ima.admin.request.IsmConfigGetServerProperty;
import com.ibm.ima.admin.request.IsmConfigSetClusterMembershipRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.ClusterMembership;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.json.java.JSONArray;
import com.ibm.json.java.JSONObject;

/**
 * Servlet implementation class ClusterMembershipResource
 */
@Path("/config/cluster/clusterMembership/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class ClusterMembershipResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = ClusterMembershipResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();


    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public ClusterMembershipResource() {
        super();
    }

    /**
     * Retrieves a (@link Cluster) from the ISM server configuration.
     * 
     * @return The {@link Cluster} object retrieved from the server.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getClusterMembership")
    public Response getClusterMembership(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getClusterMembership");

        List<ClusterMembership> result = null;

        IsmConfigGetClusterMembershipRequest request = new IsmConfigGetClusterMembershipRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<ClusterMembership>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throwException(ismResponse.getRC(), ismResponse.getErrorString());
        } else {
            result = (List<ClusterMembership>)resultObject;
        }

        if (!result.isEmpty()) {
        	try {
        		// Get the ServerUID to display to the user
        		IsmConfigGetServerProperty propertyRequest = new IsmConfigGetServerProperty(getUserId(), "ServerUID");
        		String resultString = (String)sendGetConfigRequest(propertyRequest, null);
        		logger.trace(CLAS, "getClusterMembership", "ServerUID resultString : " + resultString);
        		JSONArray array = JSONArray.parse(resultString);
        		JSONObject obj = (JSONObject) array.get(0);
        		result.get(0).setServerUID((String) obj.get("ServerUID"));
        	} catch (Exception e) {
        		logger.trace(CLAS, "getClusterMembership", "Error trying to parse the ServerUID: " + e.getMessage());
        	}  
        }
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getClusterMembership", result);
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates the (@link ClusterMembership) resource associated with ClusterMembership.
     * 
     * @param clusterMembership The (@link ClusterMembership) used to update the HA resource.
     * @return The ClusterMembership used for updating the HA resource.
     */
    @PUT
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "updateClusterMembership")
    public Response updateClusterMembership(@PathParam("domainUid") String serverInstance, ClusterMembership clusterMembership) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateClusterMembership", new Object[]{clusterMembership});

        ClusterMembership result = clusterMembership;
        // validate the request
        ValidationResult validation = clusterMembership.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        } else {
            logger.trace(CLAS, "updateClusterMembership", "Validation is successful: " + validation.toString());
        }

        IsmConfigSetClusterMembershipRequest configRequest = new IsmConfigSetClusterMembershipRequest(getUserId());
        configRequest.setFieldsFromClusterMembership(clusterMembership);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                throwException(ismResponse.getRC(), ismResponse.getErrorString());
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "updateClusterMembership", result);
        return rb.cacheControl(cache).build();
    }
}
