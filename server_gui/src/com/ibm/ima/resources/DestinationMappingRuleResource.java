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

import com.ibm.ima.admin.request.IsmConfigGetDestinationMappingRuleRequest;
import com.ibm.ima.admin.request.IsmConfigSetDestinationMappingRuleRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.VALIDATION_RESULT;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.DestinationMappingRule;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class DestinationMappingRuleResource
 */
@Path("/config/mqconnectivity/destinationMappingRules/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class DestinationMappingRuleResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = DestinationMappingRuleResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public DestinationMappingRuleResource() {
        super();
    }

    /**
     * Retrieves a list of all (@link {@link DestinationMappingRule}) objects from the MQ Connectivity server.
     * 
     * @return A list of all {@link DestinationMappingRule} objects.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getDestinationMappingRules")
    public Response getDestinationMappingRules(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getDestinationMappingRules");

        List<DestinationMappingRule> result;

        IsmConfigGetDestinationMappingRuleRequest request = new IsmConfigGetDestinationMappingRuleRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<DestinationMappingRule>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<DestinationMappingRule>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<DestinationMappingRule>)resultObject;
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getDestinationMappingRules", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Retrieves a (@link DestinationMappingRule) with the specified name from the ISM server configuration.
     * 
     * @param Name The name field of the (@link DestinationMappingRule) object to retrieve.
     * @return The {@link DestinationMappingRule} object having Name.
     */
    @GET
    @Path("{name}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getDestinationMappingRuleByName")
    public Response getDestinationMappingRuleByName(@PathParam("domainUid") String serverInstance, @PathParam("name") String name) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getDestinationMappingRuleByName", new Object[]{name});

        DestinationMappingRule rule = getDestinationMappingRule(serverInstance, name);
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(rule);

        logger.traceExit(CLAS, "getDestinationMappingRuleByName", rule.toString());
        return rb.cacheControl(cache).build();
    }
    
    @SuppressWarnings("unchecked")
	private DestinationMappingRule getDestinationMappingRule(String serverInstance, String name) {
        List<DestinationMappingRule> result = null;     
        
        DestinationMappingRule rule = new DestinationMappingRule();
        if (!rule.validateName(name, "Name").isOK()) {
        	throw new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5017", new Object[] { name });        	
        }
        
        IsmConfigGetDestinationMappingRuleRequest request = new IsmConfigGetDestinationMappingRuleRequest(name, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<DestinationMappingRule>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            String message = ismResponse.toString();
            Status status = Status.INTERNAL_SERVER_ERROR;
            if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                message = name;
                status = Status.NOT_FOUND;
            }
            throw new IsmRuntimeException(status, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
        } else {
            result = (List<DestinationMappingRule>)resultObject;
        }
        rule = null;
        if (result.size() > 0) {
        	rule = result.get(0);
        }  else {
        	throw new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5017", new Object[] { name });        	
        }
        
        return rule;
    }


    /**
     * Adds a (@link DestinationMappingRule) object to the ISM server configuration.
     * 
     * @param rule The (@link DestinationMappingRule) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "addDestinationMappingRule")
    public Response addDestinationMappingRule(@PathParam("domainUid") String serverInstance, DestinationMappingRule rule) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addDestinationMappingRule", new Object[]{rule});
        ResponseBuilder response;

        // validate the request
        ValidationResult validation = rule.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetDestinationMappingRuleRequest configRequest = new IsmConfigSetDestinationMappingRuleRequest(getUserId());
        configRequest.setFieldsFromDestinationMappingRule(rule);
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
        URI location = getResourceUri(rule.getName());
        logger.trace(CLAS, "addDestinationMappingRule", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addDestinationMappingRule", result.toString());
        return result;
    }


    /**
     * Updates the (@link DestinationMappingRule) resource associated with DestinationMappingRule.
     * 
     * @param name The id field of the (@link DestinationMappingRule) object to retrieve.
     * @param rule The (@link DestinationMappingRule) used to update the resource associated with qmConnName.
     * @return The updated {@link DestinationMappingRule} object.
     */
    @PUT
    @Path("{name}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "updateDestinationMappingRule")
    public Response updateDestinationMappingRule(@PathParam("domainUid") String serverInstance, @PathParam("name") String name, DestinationMappingRule rule) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "updateDestinationMappingRule", new Object[]{name});

        if (rule == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, VALIDATION_RESULT.VALUE_EMPTY.getMessageID(), new Object[] { "destinationMappingRule" });
        }
        
        DestinationMappingRule updatedRule = rule;
        DestinationMappingRule existingRule = getDestinationMappingRule(serverInstance, name);

        // Check for rule type - if not specified, then it's a 'special' request that will not have all the required parameters.
        // This case is for the "enable rule" and "disable rule" in the GUI
        if (rule.getRuleType() < 0) {
        	// just change enabled state
        	updatedRule = existingRule;
            updatedRule.setEnabled(rule.isEnabled() ? "True" : "False"); 
            changeEnabledOnly(serverInstance, name, rule.isEnabled());
        } else if (!rule.isEnabled() && existingRule.isEnabled()) {
        	// If we are disabling a rule along with other changes, disable it before making other changes
        	changeEnabledOnly(serverInstance, name, false);
        	existingRule.setEnabled("False");
        	sendUpdateRequest(serverInstance, name, updatedRule, existingRule);
        } else if (rule.isEnabled() && !existingRule.isEnabled()) {
        	// includes a change from disabled to enabled. Need to make that change last.
        	updatedRule.setEnabled("False");
        	sendUpdateRequest(serverInstance, name,updatedRule, existingRule);        	
        	updatedRule.setEnabled("True");
        	changeEnabledOnly(serverInstance, name, true);
        } else {        
        	sendUpdateRequest(serverInstance, name, updatedRule, existingRule);
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(updatedRule);

        logger.traceExit(CLAS, "updateDestinationMappingRule", updatedRule.toString());
        return rb.cacheControl(cache).build();
    }

    private void changeEnabledOnly(String serverInstance, String name, boolean enabled) {
    	logger.traceEntry(CLAS, "changeEnabledOnly", new Object[] {name, enabled});        	        	
        IsmConfigSetDestinationMappingRuleRequest configRequest = new IsmConfigSetDestinationMappingRuleRequest(getUserId());
        configRequest.setName(name);
        configRequest.setEnabled(enabled);
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
    	logger.traceExit(CLAS, "changeEnabledOnly");        	        			
	}

	/**
     * Sends an update request for the DestinationMappingRule based on updatedRule.
     * If a problem occurs, throws an IsmRuntimeException.
     * @param name			The name of the rule to update
     * @param updatedRule   The updated contents
	 * @param existingRule 
     */
	private void sendUpdateRequest(String serverInstance, String name, DestinationMappingRule updatedRule, DestinationMappingRule existingRule) {
    	logger.traceEntry(CLAS, "sendUpdateRequest", new Object[] {updatedRule.toString(), existingRule.toString()});        	        	

		// validate the request
        ValidationResult validation = updatedRule.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        IsmConfigSetDestinationMappingRuleRequest configRequest = new IsmConfigSetDestinationMappingRuleRequest(getUserId());
        updatedRule.setKeyName(name);
        configRequest.setFieldsFromDestinationMappingRule(updatedRule, existingRule);
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
    	logger.traceExit(CLAS, "sendUpdateRequest");        	        	
	}

    /**
     * Deletes the (@link DestinationMappingRule) resource associated with qmConnName.
     * 
     * @param name The id field of the (@link DestinationMappingRule) object to retrieve.
     */
    @DELETE
    @Path("{name}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "removeDestinationMappingRule")
    public void removeDestinationMappingRule(@PathParam("domainUid") String serverInstance, @PathParam("name") String name) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeDestinationMappingRule", new Object[] {name});

        IsmConfigSetDestinationMappingRuleRequest configRequest = new IsmConfigSetDestinationMappingRuleRequest(getUserId());
        configRequest.setName(name);
        configRequest.setDelete();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = name;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        logger.traceExit(CLAS,  "removeDestinationMappingRule");
        return;
    }
}
