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

import com.ibm.ima.admin.request.IsmConfigGetEndpointRequest;
import com.ibm.ima.admin.request.IsmConfigSetEndpointRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.Endpoint;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class EndpointResource
 */
@Path("/config/endpoints/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class EndpointResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = EndpointResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public EndpointResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link Endpoint) objects from the ISM server.
     * 
     * @return A list of all {@link Endpoint} objects.
     */
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getEndpoints")
    public Response getEndpoints(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getEndpoints");

        List<Endpoint> result = getEndpointObjects(serverInstance);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getEndpoints", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Retrieves a list of all (@link Endpoint) object names from the ISM server.
     * 
     * @return A list of all {@link Endpoint} object names.
     */
    @GET
    @Path("list/names")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator,Role.User}, action = "getEndpointNames")
    public Response getEndpointNames(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getEndpointNames");

        List<Endpoint> endpoints = getEndpointObjects(serverInstance);
        
        ArrayList<String> names = new ArrayList<String>(endpoints.size());
        for (Endpoint endpoint : endpoints) {
            names.add(endpoint.getName());
        }
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(names);

        logger.traceExit(CLAS, "getEndpointNames");
        return rb.cacheControl(cache).build();
    }
    
    /**
     * Adds a (@link Endpoint) object to the ISM server configuration.
     * 
     * @param endpoint The (@link Endpoint) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addEndpoint")
    public Response addEndpoint(@PathParam("domainUid") String serverInstance, Endpoint endpoint) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addEndpoint", new Object[]{endpoint});
        ResponseBuilder response;

        // validate the request
        endpoint.setServerInstance(serverInstance);
        ValidationResult validation = endpoint.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }


        IsmConfigSetEndpointRequest configRequest = new IsmConfigSetEndpointRequest(getUserId());
        configRequest.setFieldsFromEndpoint(endpoint);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                Status status = Status.INTERNAL_SERVER_ERROR;
                if (ismResponse.getRC().equals(ISMRC_EndpointMsgPolError) && ismResponse.getErrorString() != null) {
                    message = endpoint.getName();
                    status = Status.BAD_REQUEST;
                }
                throw new IsmRuntimeException(status, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        /* respond with the new resource location */
        URI location = getResourceUri(endpoint.getName());
        logger.trace(CLAS, "addEndpoint", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addEndpoint", result.toString());
        return result;
    }

    /**
     * Retrieves a (@link Endpoint) with the specified id from the ISM server configuration.
     * 
     * @param endpointName The id field of the (@link Endpoint) object to retrieve.
     * @return The {@link Endpoint} object having endpointName.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{endpointName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getEndpointById")
    public Response getEndpointById(@PathParam("domainUid") String serverInstance, @PathParam("endpointName") String endpointName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getEndpointById", new Object[]{endpointName});

        List<Endpoint> result = null;

        IsmConfigGetEndpointRequest request = new IsmConfigGetEndpointRequest(endpointName, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Endpoint>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = endpointName;
            }
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<Endpoint>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getEndpointById", result.toString());
        return rb.cacheControl(cache).build();
    }
    
    /**
     * Enables endpoints if they exist and are not enabled.
     * @param endpointNames comma separated list of names
     * @return list of names that were successfully changed
     */
    @PUT
    @Path("enable/{endpointNames}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "enableEndpoints")
    public Response enableEndpoints(@PathParam("domainUid") String serverInstance, @PathParam("endpointNames") String endpointNames) {
        logger.traceEntry(CLAS, "enableEndpoints", new Object[]{endpointNames});
        
        ArrayList<String> response = new ArrayList<String>();

        if (endpointNames != null) {
        	String[] endpoints = endpointNames.split(",");
        	for (String endpointName : endpoints) {
        		try {
        			IsmConfigGetEndpointRequest request = new IsmConfigGetEndpointRequest(endpointName, getUserId());
        			request.setServerInstance(serverInstance);
        			Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Endpoint>>(){});
        			if (resultObject instanceof String || resultObject instanceof IsmResponse) {
        				// get failed
        				logger.trace(CLAS, "enableEndpoints", resultObject.toString());
        				continue;
        			} 
    				@SuppressWarnings("unchecked")
					Endpoint endpoint = ((List<Endpoint>)resultObject).get(0);
    				// if the endpoint is enabled, go to next one
    				if (Boolean.getBoolean(endpoint.getEnabled())) {
    					continue;
    				}
    				// try to enable the endpoint
    				endpoint.setEnabled("True");
    				IsmConfigSetEndpointRequest configRequest = new IsmConfigSetEndpointRequest(getUserId());
    				endpoint.setKeyName(endpointName);
    				configRequest.setFieldsFromEndpoint(endpoint);
    				configRequest.setUpdate();
    		        configRequest.setServerInstance(serverInstance);
    				resultObject = sendSetConfigRequest(configRequest);
    				if (resultObject instanceof IsmResponse) {
    					IsmResponse ismResponse = (IsmResponse)resultObject;
    					if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
    						logger.trace(CLAS, "enableEndpoints", ismResponse.getErrorString());
    					} else {
    						// success
    						response.add(endpointName);
    					}
    				} else {
    					logger.trace(CLAS, "enableEndpoints", resultObject.toString());
    				}        					       			
        		} catch (Throwable t) {
        			logger.trace(CLAS, "enableEndpoints", t.getMessage());
        		}
        	}
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "enableEndpoints");
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates the (@link Endpoint) resource associated with endpointName.
     * 
     * @param endpointName The id field of the (@link Endpoint) object to retrieve.
     * @param endpoint The (@link Endpoint) used to update the resource associated with endpointName.
     * @return The updated {@link Endpoint} object.
     */
    @PUT
    @Path("{endpointName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "updateEndpoint")
    public Response updateEndpoint(@PathParam("domainUid") String serverInstance, @PathParam("endpointName") String endpointName, Endpoint endpoint) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addEndpoint", new Object[]{endpoint});
        Endpoint result = endpoint;

        // validate the request
        endpoint.setServerInstance(serverInstance);
        ValidationResult validation = endpoint.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetEndpointRequest configRequest = new IsmConfigSetEndpointRequest(getUserId());
        endpoint.setKeyName(endpointName);
        configRequest.setFieldsFromEndpoint(endpoint);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                Status status = Status.INTERNAL_SERVER_ERROR;
               if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND) || ismResponse.getRC().equals(ISMRC_EndpointMsgPolError) || ismResponse.getRC().equals(Utils.PENDING_DELETE)) {
                    message = endpointName;
                    status = Status.BAD_REQUEST;
                }                
               throw new IsmRuntimeException(status, Utils.getUtils()
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

        logger.traceExit(CLAS, "addEndpoint", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the (@link Endpoint) resource associated with endpointName.
     * 
     * @param endpointName The id field of the (@link Endpoint) object to retrieve.
     */
    @DELETE
    @Path("{endpointName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeEndpoint")
    public void removeEndpoint(@PathParam("domainUid") String serverInstance, @PathParam("endpointName") String endpointName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeEndpoint", new Object[] {endpointName});

        IsmConfigSetEndpointRequest configRequest = new IsmConfigSetEndpointRequest(getUserId());
        configRequest.setName(endpointName);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = endpointName;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeEndpoint");
        return;
    }
    
    @SuppressWarnings("unchecked")
    private List<Endpoint> getEndpointObjects(String serverInstance) {
        List<Endpoint> result;

        IsmConfigGetEndpointRequest request = new IsmConfigGetEndpointRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<Endpoint>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<Endpoint>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<Endpoint>)resultObject;
        }
        return result;
    }

}
