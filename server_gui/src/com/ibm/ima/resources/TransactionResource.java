/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

import java.util.Arrays;
import java.util.List;

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

import com.ibm.ima.admin.request.IsmConfigProcessTransactionRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.StatusResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class TransactionResource
 */
@Path("/config/transactions/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class TransactionResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = TransactionResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    private static List<String> validActionList =  Arrays.asList("commit", "rollback", "forget");

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public TransactionResource() {
        super();
    }

    /**
     * Perform either a commit, rollback or forget on a transaction associated with the XID.
     * 
     * @param action           The action to perform - commit, rollback or forget
     * @param xId              The transaction id.
     * @throws IsmRuntimeException  
     */
    @PUT
    @Path("{action}/{XID}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "processTransaction")
    public Response processTransaction(@PathParam("domainUid") String serverInstance, @PathParam("action") String action, @PathParam("XID") String xId) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "processTransaction", new Object[] {action, xId});

        if (!validActionList.contains(action.toLowerCase())) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5127", null);
        }
        
        IsmConfigProcessTransactionRequest configRequest = new IsmConfigProcessTransactionRequest(getUserId(),action );
        configRequest.setXID(xId);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        StatusResponse response = null;

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null) {
            	if (ismResponse.getRC().equals("0")) {
            		response = new StatusResponse(StatusResponse.RESPONSE_OK);
            	} else if (ismResponse.getRC().equals(ISMRC_REQUEST_REJECTED)) {
              		throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5126", new Object[] { action, xId });
            	} else {
            		String message = ismResponse.toString();
            		if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
            			message = xId;
            		}
            		throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
            				.getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            	}            	
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "processTransaction", response);
        return rb.cacheControl(cache).build();

    }
}
