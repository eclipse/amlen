/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

import javax.servlet.ServletException;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.ibm.ima.ISMWebUIProperties;
import com.ibm.ima.resources.data.UserPasswordData;
import com.ibm.ima.resources.data.UserSecurityResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;

/**
 * Servlet implementation class LoginResource
 */
@Path("/")
@Produces(MediaType.APPLICATION_JSON)
public class LoginResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = LoginResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public LoginResource() {
        super();
    }

    /**
     * Provides a programmatic login.
     * 
     * @param credentials
     *            The credentials to use for login.
     * @return A {@link UserSecurityResponse} containing the user id and roles, plus necessary tokens
     */
    @POST
    @Path("login")
     public Response login(UserPasswordData credentials) {
        logger.traceEntry(CLAS, "login");
        
        if (credentials == null || credentials.getID() == null || credentials.getPassword() == null) {
            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5004", null);
        }
        
        if (currentRequest.getUserPrincipal() == null) {
            try {
                currentRequest.login(credentials.getID(), credentials.getPassword());
            } catch (ServletException e) {
                throw new IsmRuntimeException(Status.FORBIDDEN, e, "CWLNA5001", new Object[] {
                    ISMWebUIProperties.instance().getProductName(), credentials.getID()});
            }
        }
        
        UserSecurityResponse user = UserResource.getUserSecurity(currentRequest);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(user);
        rb.cookie(UserResource.getCookie(currentRequest));
        logger.traceExit(CLAS, "login", user);
        return rb.cacheControl(cache).build();
    }
    
    /**
     * Provides a programmatic logout.
     * 
     * @return OK
     */
    @POST
    @Path("logout")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "logout")    
     public Response logout() {
        logger.traceEntry(CLAS, "logout");
        
        try {
            currentRequest.logout();
        } catch (ServletException e) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "logout", "CWLNA5005", new Object[]{e.getMessage()}, e);
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok();
        logger.traceExit(CLAS, "logout");
        return rb.cacheControl(cache).build();
    }

}
