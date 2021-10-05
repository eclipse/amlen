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

import java.io.BufferedReader;
import java.io.File;
import java.io.IOException;
import java.io.InputStreamReader;
import java.text.SimpleDateFormat;
import java.util.ArrayList;
import java.util.Date;
import java.util.Hashtable;
import java.util.List;
import java.util.Random;

import javax.naming.CompositeName;
import javax.naming.Context;
import javax.naming.Name;
import javax.naming.NameAlreadyBoundException;
import javax.naming.NameClassPair;
import javax.naming.NamingEnumeration;
import javax.naming.NamingException;
import javax.naming.directory.Attribute;
import javax.naming.directory.Attributes;
import javax.naming.directory.BasicAttribute;
import javax.naming.directory.BasicAttributes;
import javax.naming.directory.DirContext;
import javax.naming.directory.InitialDirContext;
import javax.naming.directory.ModificationItem;
import javax.naming.directory.SearchResult;
import javax.naming.ldap.InitialLdapContext;
import javax.naming.ldap.LdapContext;
import javax.naming.ldap.Rdn;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpSession;
import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.PathParam;
import javax.ws.rs.Produces;
import javax.ws.rs.QueryParam;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.NewCookie;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.ibm.icu.util.Calendar;
import com.ibm.ima.ISMLDAPProperties;
import com.ibm.ima.plugin.util.ImaSsoCrypt;
import com.ibm.ima.resources.data.LDAPConnection;
import com.ibm.ima.resources.data.UserGroupData;
import com.ibm.ima.resources.data.UserGroupListResponse;
import com.ibm.ima.resources.data.UserPasswordData;
import com.ibm.ima.resources.data.UserSecurityResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;

/**
 * Servlet implementation class UserResource
 */
@Path("/config/userregistry")
public class UserResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public static final String XSRF_TOKEN = "xsrfToken";
    public static final String SSO_TOKEN = "imaSSO";
    private final static int MAX_ID_LENGTH = 100;
    private final static int MAX_MESSAGING_ID_LENGTH = 100;
    private final static int MAX_DESC_LENGTH = 256;

    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    private final static String CLAS = UserResource.class.getCanonicalName();

    private final static String WEBUIGROUP_SYSTEM_ADMIN = "SystemAdministrators";
    private final static String[] roles = { "SystemAdministrator", "MessagingAdministrator", "User" };
    private final static String CONTEXT_WEBUI_PEOPLE = ISMLDAPProperties.getContextForWebUiUsers();
    private final static String CONTEXT_WEBUI_GROUP = ISMLDAPProperties.getContextForWebUiGroups();
    private final static String CONTEXT_MESSAGING_PEOPLE = ISMLDAPProperties.getContextForMessagingUsers();
    private final static String CONTEXT_MESSAGING_GROUP = ISMLDAPProperties.getContextForMessagingGroups();
    
    private final static String SLAPPASSWDCOMMAND = "/usr/sbin/slappasswd";
    private final static String PWDHASHCOMMAND = "/usr/bin/pwdhash";

    // Assure property imakey value is initialized as soon as the server is started
    static {
    	ISMLDAPProperties.getAuthenticationPassword();
    }

    @javax.ws.rs.core.Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public UserResource() {
        logger.traceExit(CLAS, "UserResource");
    }
    
    /**
     * Get the xsrfToken to use with the currentRequest
     * @param session the session
     * @return the token
     */
    public static String getXsrfToken(HttpSession session) {
        if (session == null) {
            return null;
        }
        return "xsrf:"+session.getId().hashCode();
    }

    /**
     * Returns an object containing the details of the user that is making the request taken directly from the
     * HttpServletRequest object. The username and roles are extracted and returned as a JSON object.
     * 
     * @return A {@link UserSecurityResponse} containing the details of the current user.
     */
    @GET
    @Path("auth")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "getWebUIUserSecurity")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getWebUIUserSecurity() {
        logger.traceEntry(CLAS, "getUserSecurity");

        UserSecurityResponse response = getUserSecurity();

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(response);
        rb.cookie(getCookie());
        logger.traceExit(CLAS, "getUserSecurity", response);
        return rb.cacheControl(cache).build();
    }

    public static NewCookie getCookie(HttpServletRequest request) {
        if (request == null) {
            return null;
        }
        return new NewCookie(UserResource.XSRF_TOKEN, getXsrfToken(request.getSession()), "/", request.getServerName(), "", -1, true);
    }

    private NewCookie getCookie() {        
        return getCookie(currentRequest);
    }

    @GET
    @Path("sso")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "getSSOCookie")
    public Response getSSOCookie(@QueryParam("clear") boolean clear) {
    	
    	String cookieString = null;
    	if (clear) {
    		cookieString = UserResource.SSO_TOKEN+"=deleted;Domain=" + 
    				currentRequest.getServerName() + ";Path=/;Expires=Thu, 01-Jan-1970 00:00:01 GMT";
    	} else {	    	
	    	String ssoToken = getSSOToken();
	    	if (ssoToken != null) {
	    		cookieString = UserResource.SSO_TOKEN+"=" + ssoToken +";Domain=" + currentRequest.getServerName() + ";Path=/;secure;";
	    	}
    	}
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok();
        if (cookieString != null) {
        	rb.header("Set-Cookie", cookieString);
        }
        logger.traceExit(CLAS, "getSSOCookie");
        return rb.cacheControl(cache).build();
    }
    

    private String getSSOToken() {
    	        	
    	String token = null;
    	String username = currentRequest.getUserPrincipal().getName();

        DirContext ctx = connectToOnBoardLDAP();

        NamingEnumeration<SearchResult> results = searchLDAP(ctx, CONTEXT_WEBUI_PEOPLE, "cn", username);
        try {
            if (results.hasMore()) {
                SearchResult result = (SearchResult) results.next();
                Attributes userAttrs = result.getAttributes();

                // Get password
                Attribute passwordAttr = userAttrs.get("userPassword");
                String password = new String((byte[]) passwordAttr.get());
                
                // build token
                token = username + ":" + password;
                // Add an expiration time in YYYY-MM-DDThh:mmZ (now + 2 hours)
                try {
                    SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mmZ");
                    Calendar calendar = Calendar.getInstance();
                    calendar.setTime(new Date());
                    calendar.add(Calendar.HOUR_OF_DAY, 2);
                    token = df.format(calendar.getTime()) + "!" + token;
                    token = ImaSsoCrypt.encrypt(token);
                } catch (Exception e) {
                	token = null;  // if we cannot build it properly then don't use it!
                	logger.trace(CLAS, "getSSOToken", e.getMessage());
                }
            }
        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getSSOToken", "CWLNA5060");
        }

		return token;
	}

	/**
     * @return
     */
    public static UserSecurityResponse getUserSecurity(HttpServletRequest request) {
        if (request == null || request.getUserPrincipal() == null) {
            return null;
        }
        String username = request.getUserPrincipal().getName();
        List<String> userRoles = new ArrayList<String>();

        for (String role : roles) {
            if (request.isUserInRole(role)) {
                userRoles.add(role);
            }
        }

        // Generate the jsessionid cookie so we can use a session inactivity timeout...
        // This is always called for a page load, so adding it here. When Liberty
        // supports JAX-RS 2.0, move this to a filter
        HttpSession session = request.getSession();
        
        logger.trace(CLAS, "getUserSecurity", "user: " + username + "; sessionId = " + session.getId()); 

        return new UserSecurityResponse(username, userRoles.toArray(new String[0]));
    }
    
    /**
     * @return
     */
    private UserSecurityResponse getUserSecurity() {
        return getUserSecurity(currentRequest);
    }


    /**
     * Returns an object containing the details of all users within the webui subcontext.
     * 
     * @return A {@link UserGroupListResponse} containing the details of all webui users.
     */
    @GET
    @Path("webui/users")
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "getWebUIUsers")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getWebUIUsers() {
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(listPath(CONTEXT_WEBUI_PEOPLE, CONTEXT_WEBUI_GROUP, false));
        return rb.cacheControl(cache).build();
    }

    /**
     * Returns an object containing the details of all groups within the webui subcontext.
     * 
     * @return A {@link UserGroupListResponse} containing the details of all webui groups.
     */
    @GET
    @Path("webui/groups")
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "getWebUIGroups")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getWebUIGroups() {
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(listPath(CONTEXT_WEBUI_GROUP, CONTEXT_WEBUI_GROUP, true));
        return rb.cacheControl(cache).build();
    }

    /**
     * Returns an object containing the details of all users within the messaging subcontext.
     * 
     * @return A {@link UserGroupListResponse} containing the details of all messaging users.
     */
    @GET
    @Path("messaging/users")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "getMessagingUsers")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getMessagingUsers() {
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(listPath(CONTEXT_MESSAGING_PEOPLE, CONTEXT_MESSAGING_GROUP, false));
        return rb.cacheControl(cache).build();
    }

    /**
     * Returns an object containing the details of all groups within the messaging subcontext.
     * 
     * @return A {@link UserGroupListResponse} containing the details of all messaging groups.
     */
    @GET
    @Path("messaging/groups")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "getMessagingGroups")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getMessagingGroups() {
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(listPath(CONTEXT_MESSAGING_GROUP, CONTEXT_MESSAGING_GROUP, true));
        return rb.cacheControl(cache).build();
    }

    /**
     * Creates a user in the webui subcontext using the details in the provided JSON object.
     * 
     * @param user The JSON object containing user details to create.
     * @return Status JSON object containing the return code from the create action.
     */
    @POST
    @Path("webui/users")
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "createWebUIUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response createWebUIUser(UserGroupData user) {
        return createEntryOnLDAP(user, CONTEXT_WEBUI_PEOPLE, CONTEXT_WEBUI_GROUP, false, MAX_ID_LENGTH);
    }

    /**
     * Creates a user in the messaging subcontext using the details in the provided JSON object.
     * 
     * @param user The JSON object containing user details to create.
     * @return Status JSON object containing the return code from the create action.
     */
    @POST
    @Path("messaging/users")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "createMessagingUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response createMessagingUser(UserGroupData user) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "createMessagingUser", "CWLNA5070");
            throw authFailed;
        }

        return createEntryOnLDAP(user, CONTEXT_MESSAGING_PEOPLE, CONTEXT_MESSAGING_GROUP, false, MAX_MESSAGING_ID_LENGTH);
    }

    /**
     * Creates a group in the messaging subcontext using the details in the provided JSON object.
     * 
     * @param user The JSON object containing group details to create.
     * @return Status JSON object containing the return code from the create action.
     */
    @POST
    @Path("messaging/groups")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "createMessagingGroup")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response createMessagingGroup(UserGroupData user) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "createMessagingGroup", "CWLNA5070");
            throw authFailed;
        }

        return createEntryOnLDAP(user, CONTEXT_MESSAGING_GROUP, CONTEXT_MESSAGING_GROUP, true, MAX_MESSAGING_ID_LENGTH);
    }

    /**
     * Modifies a user in the webui subcontext using the details in the provided JSON object. Whilst the UserGroupData
     * object is used to modify the user, it cannot be used to reset the users password and that value is ignored.
     * 
     * @param userName The user to modify
     * @param user The JSON object containing new user details.
     * @return Status JSON object containing the return code from the modify action.
     */
    @PUT
    @Path("webui/users/{user}")
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "modifyWebUIUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response modifyWebUIUser(@PathParam("user") String userName, UserGroupData user) {
        return modifyEntryOnLDAP(userName, user, CONTEXT_WEBUI_PEOPLE, CONTEXT_WEBUI_GROUP, false, MAX_ID_LENGTH);
    }

    /**
     * Modifies a user in the messaging subcontext using the details in the provided JSON object. Whilst the
     * UserGroupData object is used to modify the user, it cannot be used to reset the users password and that value is
     * ignored.
     * 
     * @param userName The user to modify
     * @param user The JSON object containing new user details.
     * @return Status JSON object containing the return code from the modify action.
     */
    @PUT
    @Path("messaging/users/{user}")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "modifyMessagingUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response modifyMessagingUser(@PathParam("user") String userName, UserGroupData user) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "modifyMessagingUser", "CWLNA5070");
            throw authFailed;
        }

        return modifyEntryOnLDAP(userName, user, CONTEXT_MESSAGING_PEOPLE, CONTEXT_MESSAGING_GROUP, false, MAX_MESSAGING_ID_LENGTH);
    }

    /**
     * Modifies a group in the messaging subcontext using the details in the provided JSON object.
     * 
     * @param groupName The group to modify
     * @param grp The JSON object containing new group details.
     * @return Status JSON object containing the return code from the modify action.
     */
    @PUT
    @Path("messaging/groups/{group}")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "modifyMessagingGroup")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response modifyMessagingGroup(@PathParam("group") String groupName, UserGroupData grp) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "modifyMessagingGroup", "CWLNA5070");
            throw authFailed;
        }

        return modifyEntryOnLDAP(groupName, grp, CONTEXT_MESSAGING_GROUP, CONTEXT_MESSAGING_GROUP, true, MAX_MESSAGING_ID_LENGTH);
    }

    /**
     * Deletes a user in the webui subcontext
     * 
     * @param userName The user to delete.
     * @return Status JSON object containing the return code from the delete action.
     */
    @DELETE
    @Path("webui/users/{user}")
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "deleteWebUIUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response deleteWebUIUser(@PathParam("user") String userName) {
        return deleteEntryOnLDAP(userName, CONTEXT_WEBUI_PEOPLE, CONTEXT_WEBUI_GROUP);
    }

    /**
     * Deletes a user in the messaging subcontext
     * 
     * @param userName The user to delete.
     * @return Status JSON object containing the return code from the delete action.
     */
    @DELETE
    @Path("messaging/users/{user}")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "deleteMessagingUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response deleteMessagingUser(@PathParam("user") String userName) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "deleteMessagingUser", "CWLNA5070");
            throw authFailed;
        }

        return deleteEntryOnLDAP(userName, CONTEXT_MESSAGING_PEOPLE, CONTEXT_MESSAGING_GROUP);
    }

    /**
     * Deletes a group in the messaging subcontext
     * 
     * @param groupName The group to delete.
     * @return Status JSON object containing the return code from the delete action.
     */
    @DELETE
    @Path("messaging/groups/{group}")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "deleteMessagingGroup")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response deleteMessagingGroup(@PathParam("group") String groupName) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "deleteMessagingGroup", "CWLNA5070");
            throw authFailed;
        }

        return deleteEntryOnLDAP(groupName, CONTEXT_MESSAGING_GROUP, CONTEXT_MESSAGING_GROUP);
    }

    /**
     * Resets a webui users password to the provided value. Despite using the UserPasswordData object this does not
     * require the user's old password as anyone calling this REST interface must be an administrator.
     * 
     * @param user JSON object containing the webui user id and the new password to set.
     * @return Status JSON object containing the return code from the reset password action.
     */
    @POST
    @Path("webui/password")
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "resetWebUIUserPassword")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response resetWebUIUserPassword(UserPasswordData user) {
        return resetUserPwOnLDAP(user, CONTEXT_WEBUI_PEOPLE);
    }

    /**
     * Resets a messaging users password to the provided value. Despite using the UserPasswordData object this does not
     * require the user's old password as anyone calling this REST interface must be an administrator.
     * 
     * @param user JSON object containing the messaging user id and the new password to set.
     * @return Status JSON object containing the return code from the reset password action.
     */
    @POST
    @Path("messaging/password")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "resetMessagingUserPassword")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response resetMessagingUserPassword(UserPasswordData user) {
        if (isExternalLDAPEnabled()) {
            IsmRuntimeException authFailed = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5070", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "resetMessagingUserPassword", "CWLNA5070");
            throw authFailed;
        }

        return resetUserPwOnLDAP(user, CONTEXT_MESSAGING_PEOPLE);
    }

    /**
     * Changes a webui users password to the provided value. This can be called by any logged in user and therefore they
     * must provide their old password in the {@link UserPasswordData} object. This request will fail if the user
     * attempts to change an id other their own even if the password is correct.
     * 
     * @param user JSON object containing the webui user id, current password and the new password to set.
     * @return Status JSON object containing the return code from the reset password action.
     */
    @PUT
    @Path("webui/password")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "changeUserPassword")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response changeUserPassword(UserPasswordData userPw) {
        logger.traceEntry(CLAS, "changeUserPassword");
        
//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(null);

        String username = currentRequest.getUserPrincipal().getName();

        if (username.equals(userPw.getID())) {
            if (userPw.getOldPassword() == null || userPw.getOldPassword().equals("")) {
                IsmRuntimeException notValidException =
                        new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012", new Object[] { "oldpassword" });
                logger.log(LogLevel.LOG_ERR, CLAS, "changeUserPassword", "CWLNA5012", new Object[] { "oldpassword" });
                throw notValidException;
            }

            if (userPw.getPassword() == null || userPw.getPassword().equals("")) {
                IsmRuntimeException notValidException =
                        new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012", new Object[] { "password" });
                logger.log(LogLevel.LOG_ERR, CLAS, "changeUserPassword", "CWLNA5012", new Object[] { "password" });
                throw notValidException;
            }

            DirContext ctx = connectToOnBoardLDAP();

            NamingEnumeration<SearchResult> results = searchLDAP(ctx, CONTEXT_WEBUI_PEOPLE, "cn", userPw.getID());
            try {
                if (results.hasMore()) {
//                    SearchResult result = (SearchResult) results.next();
//                    Attributes userAttrs = result.getAttributes();

                    // Verify old password
                    if(connectWithOldPw(userPw.getID(),userPw.getOldPassword())) {
                        resetUserPwOnLDAP(userPw, CONTEXT_WEBUI_PEOPLE);
                    } else {
                        IsmRuntimeException authFailed =
                              new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5032", new Object[] { userPw.getID() });
                        logger.log(LogLevel.LOG_ERR, CLAS, "changeUserPassword", "CWLNA5032", new Object[] { userPw.getID() });
                        throw authFailed;
                    }
                }
            } catch (NamingException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "changeUserPassword", "CWLNA5060");
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", null);
            } 
        } else {
            IsmRuntimeException authFailed =
                    new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5031", new Object[] { userPw.getID() });
            logger.log(LogLevel.LOG_ERR, CLAS, "changeUserPassword", "CWLNA5031", new Object[] { userPw.getID() });
            throw authFailed;
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(true);

        logger.traceExit(CLAS, "changeUserPassword");
        return rb.cacheControl(cache).build();
    }

    /**
     * Sets a users password on the LDAP server. This is context agnostic and the location on the LDAP much be specified
     * when calling this method.
     * 
     * @param user The {@link UserPasswordData} object containing the username and password
     * @param contextPath The LDAP subcontext which this user belongs to.
     * @return Status JSON object containing the return code from the action.
     */
    private Response resetUserPwOnLDAP(UserPasswordData user, String contextPath) {
        logger.traceEntry(CLAS, "resetUserPwOnLDAP");
        
//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(null);

        try {
            DirContext ctx = connectToOnBoardLDAP();

            if (user.getPassword() == null || user.getPassword().equals("")) {
                IsmRuntimeException notValidException =
                        new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012", new Object[] { "password" });
                logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5012", new Object[] { "password" });
                throw notValidException;
            }

            if (!isIdValid(user.getPassword())) {
                IsmRuntimeException notValidException =
                        new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5059", null);
                logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5059");
                throw notValidException;
            }

            NamingEnumeration<SearchResult> results = searchLDAP(ctx, contextPath, "cn", user.getID());
            if (results.hasMore()) {

                ctx.modifyAttributes("cn=" + escapeSpecialCharacters(user.getID(), true) + "," + contextPath,
                        new ModificationItem[] { new ModificationItem(DirContext.REPLACE_ATTRIBUTE, new BasicAttribute(
                                "userPassword", getPassH(user.getPassword()))) });
            } else {
                IsmRuntimeException notFoundException = new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5031", null);
                logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5031");
                throw notFoundException;
            }

        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5060");
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", null);
        } catch (IOException ioe) {
            logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5060", new Object[] { "IOException on password" });
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", null);
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(true);

        logger.traceExit(CLAS, "resetUserPwOnLDAP");
        return rb.cacheControl(cache).build();
    }

    /**
     * Create an entry on the LDAP server. This is context agnostic and the location on the LDAP much be specified when
     * calling this method.
     * 
     * @param entry The object containing the information for the entry.
     * @param contextPath The LDAP subcontext which this entry will belong to.
     * @param groupContext The LDAP subcontext which the groups associated with the entry can be part off.
     * @param isGroup Flag indicating whether the entry being created is a group.
     * @param maxIdLength 
     * @return Status JSON object containing the return code from the action.
     */
    private Response createEntryOnLDAP(UserGroupData entry, String contextPath, String groupContext, boolean isGroup, int maxIdLength) {
        logger.traceEntry(CLAS, "createEntryOnLDAP");
        
//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(null);
        
        DirContext ctx = connectToOnBoardLDAP();

        if (entry.getID() == null || entry.getID().equals("")) {
            IsmRuntimeException notValidException =
                    new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012", new Object[] { "id" });
            logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5012", new Object[] { "id" });
            throw notValidException;
        }

        try {
            if (isIdValid(entry.getID())) { 
                if (entry.getID().length() > maxIdLength) {
                    IsmRuntimeException tooLargeException =
                            new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5072", null);
                    logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5072");
                    throw tooLargeException;
                }
                String id = escapeSpecialCharacters(entry.getID(), true);
                NamingEnumeration<SearchResult> results = searchLDAP(ctx, contextPath, "cn", id);

                if (results.hasMore()) {
                    IsmRuntimeException alreadyExistsEx =
                            new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5030", null);
                    logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5030");
                    throw alreadyExistsEx;
                } else {

                    Attributes attrs = new BasicAttributes(true);
                    Attribute objClass = new BasicAttribute("objectclass");

                    String desc = entry.getDescription();

                    if (isGroup) {
                        objClass.add("groupOfNames");
                        attrs.put("member", "");
                    } else {
                        if (entry.getPassword() == null || entry.getPassword().equals("")) {
                            IsmRuntimeException notValidException =
                                    new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012",
                                            new Object[] { "password" });
                            logger.log(LogLevel.LOG_ERR, CLAS, "resetUserPwOnLDAP", "CWLNA5012", new Object[] { "password" });
                            throw notValidException;
                        }

                        objClass.add("inetOrgPerson");
                        attrs.put("uid", id);
                        attrs.put("sn", id);

                        if (!isIdValid(entry.getPassword())) {
                            IsmRuntimeException notValidException =
                                    new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5059", null);
                            logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5059");
                            throw notValidException;
                        }
                        
                        attrs.put("userPassword", getPassH(entry.getPassword()));

                    }
                    if (!desc.equals("")) {
                        attrs.put("description", desc);

                        if (desc.length() > MAX_DESC_LENGTH) {
                            IsmRuntimeException tooLargeException =
                                    new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5085", null);
                            logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5085");
                            throw tooLargeException;
                        }
                    }

                    //Process employeeNumber (if applicable)
                    if (contextPath.equals(CONTEXT_WEBUI_PEOPLE)) {
                        attrs.put("employeeNumber", generateEmployeeNum(entry.getID()));
                    }

                    attrs.put(objClass);

                    // Prepare full LDAP paths for new entry, one to create and one to add entry to groups
                    String entryPath = "cn=" + id + "," + contextPath;
                    String entryPathForGroups =
                            "cn=" + escapeSpecialCharacters(entry.getID(), false) + "," + contextPath;

                    CompositeName name = new CompositeName(entryPath);

                    // Create the entry on LDAP
                    ctx.createSubcontext(name, attrs);

                    // Add the entry to the member section of registered groups
                    boolean addItself = false;
                    ArrayList<String> errorGroups = null;
                    for (String groupId : entry.getGroups()) {
                        NamingEnumeration<SearchResult> groupResults = searchLDAP(ctx, groupContext, "cn", groupId);
                        if (groupResults.hasMore()) {
                            if (!(isGroup && groupId.equals(entry.getID()))) {
                                String fullGroupId =
                                        "cn=" + escapeSpecialCharacters(groupId, true) + "," + groupContext;
                                logger.trace(CLAS, "createEntryOnLDAP", "adding member " + entryPathForGroups
                                        + " to group " + groupId);
                                try {
                                    ctx.modifyAttributes(fullGroupId,
                                            new ModificationItem[] { new ModificationItem(DirContext.ADD_ATTRIBUTE,
                                                    new BasicAttribute("member", entryPathForGroups)) });
                                } catch (NamingException e) {
                                    if (errorGroups == null) {
                                        errorGroups = new ArrayList<String>();
                                    }
                                    errorGroups.add(groupId);
                                }
                            } else {
                                addItself = true;
                            }
                        } else {
                            // the user id does get created, it just isn't added to all groups.
                            // notFoundException = new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5031", null);
                            if (errorGroups == null) {
                                errorGroups = new ArrayList<String>();
                            }
                            errorGroups.add(groupId);
                        }
                    }

                    /*
                     * if (notFoundException != null) { logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP",
                     * "CWLNA5031", notFoundException); throw notFoundException; }
                     */

                    if (addItself) {
                        IsmRuntimeException addItselfException =
                                new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5052", null);
                        logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5052");
                        throw addItselfException;
                    }

                    if (errorGroups != null) {
                        ArrayList<String> groups = new ArrayList<String>();
                        StringBuilder sb = new StringBuilder();
                        for (String group : entry.getGroups()) {
                            if (!errorGroups.contains(group)) {
                                groups.add(group);
                            } else {
                                sb.append(group + ", ");
                            }
                        }
                        String excludedGroups = sb.toString();
                        int lastComma = excludedGroups.lastIndexOf(", ");
                        if (lastComma > 0) {
                            excludedGroups = excludedGroups.substring(0, lastComma);
                        }
                        entry.setGroups(groups.toArray(new String[groups.size()]));
                        entry.setCode("CWLNA5056");
                        entry.setMessage(logger.getFormattedMessage("CWLNA5056", new Object[] { entry.getID(),
                                excludedGroups }, clientLocale, false));
                        logger.log(LogLevel.LOG_WARNING, CLAS, "createEntryOnLDAP", "CWLNA5056",
                                new Object[] { entry.getID(), errorGroups.toString() });
                    }

                }

            } else {
                IsmRuntimeException notValidException =
                        new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5057", null);
                logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5057");
                throw notValidException;
            }
        } catch (NameAlreadyBoundException  be) {
                logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5030");
                throw new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5030", null);        
        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5060", new Object[] { e.toString() });
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", new Object[] { e.toString() });
        } catch (IOException ioe) {
            logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5060", new Object[] { "IOException on password" });
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", new Object[] { "IOException on password" });
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(entry);

        logger.traceExit(CLAS, "createEntryOnLDAP");
        return rb.cacheControl(cache).build();
    }

    private String getPassH(String inpass) throws IOException {
        String[] slappasswdcommand = {SLAPPASSWDCOMMAND, "-s", inpass, "-n"};
        String[] pwdhashcommand = {PWDHASHCOMMAND, "-s", "SHA512", inpass};

        File sfile = new File(SLAPPASSWDCOMMAND);
        File pfile = new File(PWDHASHCOMMAND);

        Process process = null;

        Runtime runtime = Runtime.getRuntime();

        if (sfile.exists() && sfile.canExecute()) {
            process = runtime.exec(slappasswdcommand);
        } else if(pfile.exists() && pfile.canExecute()) {
            process = runtime.exec(pwdhashcommand);
        } else {
            // if neither command exist try returning passwd in clear text
            // this only happens over localhost
            return null;
        }

        if (process == null) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getPassH", "CWLNA5060", new Object[] { "IOException no command available for hashing the password" });
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", new Object[] { "IOException no command available for hashing the password" });
        } else {
            BufferedReader stdin = new BufferedReader(new InputStreamReader(process.getInputStream()));
            String line = null;
            if ((line = stdin.readLine()) == null) {
                logger.log(LogLevel.LOG_ERR, CLAS, "getPassH", "CWLNA5060", new Object[] { "IOException no output after hashing the password" });
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", new Object[] { "IOException no output after hashing the password" });
            } else {
                // the pwdhash command does not remove newlines
                line = line.replaceAll("(?:\\n|\\r)", "");
                return line.trim();
            }
        }
    }

    /**
     * Modify an entry on the LDAP server. This is context agnostic and the location on the LDAP much be specified when
     * calling this method. The name of the entry in the entry parameter is the new name of the object.
     *
     * @param lookupName The name of the entry being renamed.
     * @param entry The object containing the information for the entry.
     * @param contextPath The LDAP subcontext which this entry will belong to.
     * @param groupContext The LDAP subcontext which the groups associated with the entry can be part off.
     * @param isGroup Flag indicating whether the entry being created is a group.
     * @param maxIdLength 
     * @return Status JSON object containing the return code from the action.
     */
    private Response modifyEntryOnLDAP(String lookupName, UserGroupData entry, String contextPath, String groupContext,
            boolean isGroup, int maxIdLength) {
        logger.traceEntry(CLAS, "modifyEntryOnLDAP");

//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(null);

        try {
            DirContext ctx = connectToOnBoardLDAP();

            NamingEnumeration<SearchResult> results = searchLDAP(ctx, contextPath, "cn", lookupName);

            if (results.hasMore()) {

                if (entry.getID() != null) {
                    if (entry.getID().length() > maxIdLength) {
                        IsmRuntimeException tooLargeException =
                                new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5072", null);
                        logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5072");
                        throw tooLargeException;
                    }
                } 

                boolean isWebUIAdminUser = contextPath.equals(CONTEXT_WEBUI_PEOPLE) && lookupName.equals("admin");
                // Update the existing entry
                String rawId = lookupName;
                String id = entry.getID();
                String oldEntryPath = "cn=" + escapeSpecialCharacters(lookupName, true) + "," + contextPath;
                String oldSearchPath = "cn=" + escapeSpecialCharacters(lookupName, false) + "," + contextPath;
                String newEntryPath = null;
                String modEntryPath = null;
                boolean nameChange = false;
                if (entry.getID() != null) {
                    newEntryPath = "cn=" + escapeSpecialCharacters(entry.getID(), true) + "," + contextPath;
                    modEntryPath = "cn=" + escapeSpecialCharacters(entry.getID(), false) + "," + contextPath;
                    nameChange = !oldEntryPath.equals(newEntryPath);
                } else {
                    newEntryPath = oldEntryPath;
                    modEntryPath = "cn=" + escapeSpecialCharacters(lookupName, false) + "," + contextPath;
                }

                if (nameChange && isWebUIAdminUser) {
                    IsmRuntimeException invalidRequestEx = new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5033", null);
                    logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5033");
                    throw invalidRequestEx;
                }

                if (nameChange) {
                    if (!isIdValid(entry.getID())) {
                        IsmRuntimeException notValidException =
                                new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5057", null);
                        logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5057");
                        throw notValidException;
                    }

                    rawId = entry.getID();
                    id = escapeSpecialCharacters(entry.getID(), true);
                    newEntryPath = "cn=" + id + "," + contextPath;
                    NamingEnumeration<SearchResult> checkEntryExists = searchLDAP(ctx, contextPath, "cn", id);

                    if (checkEntryExists.hasMore()) {
                        IsmRuntimeException entryExistsEx =
                                new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5034", null);
                        logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5034");
                        throw entryExistsEx;
                    }

                    ctx.rename(oldEntryPath, newEntryPath);
                }

                ArrayList<ModificationItem> modifications = new ArrayList<ModificationItem>();

                if (!isGroup && nameChange) {
                    modifications.add(new ModificationItem(DirContext.REPLACE_ATTRIBUTE, new BasicAttribute("sn", id)));
                    modifications
                    .add(new ModificationItem(DirContext.REPLACE_ATTRIBUTE, new BasicAttribute("uid", id)));
                }

                if (entry.getDescription() != null) {
                    if (entry.getDescription().equals("")) {
                        modifications.add(new ModificationItem(DirContext.REMOVE_ATTRIBUTE, new BasicAttribute(
                                "description")));
                    } else {
                        if (entry.getDescription().length() > MAX_DESC_LENGTH) {
                            IsmRuntimeException tooLargeException =
                                    new IsmRuntimeException(Status.NOT_ACCEPTABLE, "CWLNA5085", null);
                            logger.log(LogLevel.LOG_ERR, CLAS, "createEntryOnLDAP", "CWLNA5085");
                            throw tooLargeException;
                        }

                        modifications.add(new ModificationItem(DirContext.REPLACE_ATTRIBUTE, new BasicAttribute(
                                "description", entry.getDescription())));
                    }
                }

                // Lookup group entries and modify if applicable
                if (nameChange || (entry.getGroups() != null)) {
                    ArrayList<String> groupPathsToRemoveEntry = new ArrayList<String>();
                    ArrayList<String> groupPathsToAddEntry = new ArrayList<String>();

                    NamingEnumeration<SearchResult> foundInGroups = searchLDAP(ctx, groupContext, "member", oldSearchPath);

                    // Find all entries the old entry name is in
                    while (foundInGroups.hasMore()) {
                        SearchResult groupResult = (SearchResult) foundInGroups.next();
                        Attributes groupAttrs = groupResult.getAttributes();

                        String groupName = (String) groupAttrs.get("cn").get();
                        boolean inSystemAdminAndIsAdmin = isWebUIAdminUser && groupName.equals(WEBUIGROUP_SYSTEM_ADMIN);
                        if (!inSystemAdminAndIsAdmin) {
                            groupPathsToRemoveEntry.add(groupName);
                        }
                    }

                    String[] groupNames;
                    if (entry.getGroups() != null) {
                        groupNames = entry.getGroups();
                    } else {
                        groupNames = groupPathsToRemoveEntry.toArray(new String[0]);
                    }

                    for (String groupId : groupNames) {
                        boolean inSystemAdminAndIsAdmin = isWebUIAdminUser && groupId.equals(WEBUIGROUP_SYSTEM_ADMIN);
                        if (inSystemAdminAndIsAdmin || (!nameChange && groupPathsToRemoveEntry.contains(groupId))) {
                            groupPathsToRemoveEntry.remove(groupId);
                        } else {
                            groupPathsToAddEntry.add(groupId);
                        }
                    }
                    
                    // The two lists should now be correct so lets process them
                    for (String groupId : groupPathsToRemoveEntry) {
                        String fullGroupId = "cn=" + escapeSpecialCharacters(groupId, false) + "," + groupContext;

                        BasicAttribute memberAttr = new BasicAttribute("member", oldSearchPath);
                        BasicAttributes memberAttrs = new BasicAttributes();
                        memberAttrs.put(memberAttr);
                        logger.trace(CLAS, "modifyEntryOnLDAP", "removing attribute " + memberAttrs + " from group "
                                + groupId);
                        ctx.modifyAttributes(fullGroupId, DirContext.REMOVE_ATTRIBUTE, memberAttrs);
                    }

                    boolean addItself = false;
                    ArrayList<String> errorGroups = null;
                    /* BEAM suppression:  Loop can be executed at most once */
                    for (String groupId : groupPathsToAddEntry) {
                        if (!(isGroup && groupId.equals(rawId))) {
                            String fullGroupId = "cn=" + escapeSpecialCharacters(groupId, true) + "," + groupContext;
                            logger.trace(CLAS, "modifyEntryOnLDAP", "adding member " + modEntryPath + " to group "
                                    + groupId);
                            try {
                                ctx.modifyAttributes(fullGroupId, new ModificationItem[] { new ModificationItem(
                                        DirContext.ADD_ATTRIBUTE, new BasicAttribute("member", modEntryPath)) });

                                // Process employeeNumber (if applicable)
                                if ((contextPath.equals(CONTEXT_WEBUI_PEOPLE)) && (groupId.equals(WEBUIGROUP_SYSTEM_ADMIN))) {
                                    modifications.add(new ModificationItem(DirContext.REPLACE_ATTRIBUTE, new BasicAttribute(
                                            "employeeNumber", generateEmployeeNum(entry.getID()))));
                                }
                            } catch (NamingException e) {
                                if (errorGroups == null) {
                                    errorGroups = new ArrayList<String>();
                                }
                                errorGroups.add(groupId);
                            }
                        } else {
                            addItself = true;
                        }
                    }

                    if (addItself) {
                        IsmRuntimeException addItselfException =
                                new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5052", null);
                        logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5052");
                        throw addItselfException;
                    }

                    if (errorGroups != null) {
                        ArrayList<String> groups = new ArrayList<String>();
                        StringBuilder sb = new StringBuilder();
                        for (String group : entry.getGroups()) {
                            if (!errorGroups.contains(group)) {
                                groups.add(group);
                            } else {
                                sb.append(group + ", ");
                            }
                        }
                        String excludedGroups = sb.toString();
                        int lastComma = excludedGroups.lastIndexOf(", ");
                        if (lastComma > 0) {
                            excludedGroups = excludedGroups.substring(0, lastComma);
                        }
                        String itemId = entry.getID() != null ? entry.getID() : lookupName;
                        entry.setGroups(groups.toArray(new String[groups.size()]));
                        entry.setCode("CWLNA5056");
                        entry.setMessage(logger.getFormattedMessage("CWLNA5056",
                                new Object[] { itemId, excludedGroups }, clientLocale, false));
                        logger.log(LogLevel.LOG_WARNING, CLAS, "createEntryOnLDAP", "CWLNA5056", new Object[] { itemId,
                                errorGroups.toString() });
                    }

                }

                ctx.modifyAttributes(newEntryPath, modifications.toArray(new ModificationItem[0]));
            } else {
                IsmRuntimeException notFoundException = new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5031", null);
                logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5031");
                throw notFoundException;
            }
        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5060", new Object[] { e.toString() });
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5060", new Object[] { e.toString() });
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(entry);

        logger.traceExit(CLAS, "modifyEntryOnLDAP");
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes an entry on the LDAP server. This is context agnostic and the location on the LDAP much be specified when
     * calling this method.
     * 
     * @param lookupName The name of the entry being renamed.
     * @param contextPath The LDAP subcontext which this entry will belong to.
     * @param groupContext The LDAP subcontext which the groups associated with the entry can be part off.
     * @return Status JSON object containing the return code from the action.
     */
    private Response deleteEntryOnLDAP(String entryName, String contextPath, String groupContext) {
        logger.traceEntry(CLAS, "deleteEntryOnLDAP");
        
//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(null);
        
        if (!(groupContext.equals(CONTEXT_WEBUI_GROUP) && entryName.equals(getUserId()))) {
            try {
                DirContext ctx = connectToOnBoardLDAP();

                NamingEnumeration<SearchResult> results = searchLDAP(ctx, contextPath, "cn", entryName);
                if (results.hasMore()) {
                    String entryPath = "cn=" + escapeSpecialCharacters(entryName, true) + "," + contextPath;
                    String deletePath = "cn=" + escapeSpecialCharacters(entryName, false) + "," + contextPath;
                    ctx.destroySubcontext(entryPath);

                    NamingEnumeration<SearchResult> foundInGroups = searchLDAP(ctx, groupContext, "member", deletePath);
                    ArrayList<String> groupPathsToRemoveEntry = new ArrayList<String>();

                    // Find all entries the old entry name is in
                    while (foundInGroups.hasMore()) {
                        SearchResult groupResult = (SearchResult) foundInGroups.next();
                        Attributes groupAttrs = groupResult.getAttributes();

                        groupPathsToRemoveEntry.add((String) groupAttrs.get("cn").get());
                    }

                    for (String groupId : groupPathsToRemoveEntry) {
                        String fullGroupId = "cn=" + escapeSpecialCharacters(groupId, true) + "," + groupContext;

                        BasicAttribute memberAttr = new BasicAttribute("member", deletePath);
                        BasicAttributes memberAttrs = new BasicAttributes();
                        memberAttrs.put(memberAttr);

                        ctx.modifyAttributes(fullGroupId, DirContext.REMOVE_ATTRIBUTE, memberAttrs);
                    }
                } else {
                    IsmRuntimeException notFoundException =
                            new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5031", null);
                    logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5031");
                    throw notFoundException;
                }
            } catch (NamingException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "deleteEntryOnLDAP", "CWLNA5060", new Object[] { e.toString() });
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5060", new Object[] { e.toString() });
            }
        } else {
            IsmRuntimeException invalidRequestEx = new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5033", null);
            logger.log(LogLevel.LOG_ERR, CLAS, "modifyEntryOnLDAP", "CWLNA5033");
            throw invalidRequestEx;
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(true);

        logger.traceExit(CLAS, "deleteEntryOnLDAP");
        return rb.cacheControl(cache).build();
    }

    /**
     * Lists all entries on the LDAP server. This is context agnostic and the location on the LDAP much be specified
     * when calling this method.
     * 
     * @param contextPath The LDAP subcontext which this entry will belong to.
     * @param groupContext The LDAP subcontext which the groups associated with the entry can be part off.
     * @param isGroup Flag indicating whether the entry being created is a group.
     * @return Status JSON object containing the return code from the action.
     */
    private UserGroupListResponse listPath(String contextPath, String groupContext, boolean isGroup) {
        logger.traceEntry(CLAS, "listPath");
        UserGroupListResponse ulr = null;

        List<UserGroupData> users = new ArrayList<UserGroupData>();

        DirContext ctx = connectToOnBoardLDAP();
        try {
            NamingEnumeration<NameClassPair> list = ctx.list(contextPath);

            while (list.hasMore()) {
                NameClassPair nc = (NameClassPair) list.next();

                String name = nc.getName();
                UserGroupData userInfo = getUserGroupData(ctx, groupContext, name, contextPath, isGroup);
                if (userInfo != null) {
                    users.add(userInfo);
                }

            }

            ulr = new UserGroupListResponse(0, users.toArray(new UserGroupData[0]));
        } catch (Exception ex) {
            logger.log(LogLevel.LOG_ERR, CLAS, "listPath", "CWLNA5060");
        }

        logger.traceExit(CLAS, "listPath");
        return ulr;
    }

    /**
     * Retrieves an entry from LDAP and puts it into a @link {@link UserGroupData} object. This is context agnostic and
     * the location on the LDAP much be specified when calling this method.
     * 
     * @param Diretory context handle of the server to use.
     * @param groupContext The LDAP subcontext which the groups associated with the entry can be part off.
     * @param fullLookupName Full path to the entry including subcontext
     * @param isGroup Flag indicating whether the entry being created is a group.
     * @return UserGroupData Object populated with details of the entry.
     */
    private UserGroupData getUserGroupData(DirContext ctx, String groupContext, String lookupName, String contextPath,
            boolean isGroup) {
        logger.traceEntry(CLAS, "getUserGroupData");
        UserGroupData user = null;
        try {

            String name = lookupName;
            logger.trace(CLAS, "getUserGroupData", "lookupName " + lookupName);
            boolean useComposite = false;
            if (name.startsWith("\"cn=") && name.endsWith("\"")) {
                name = name.substring(1, name.length() - 1);
                name = name.replace("\\\\\"","\\\"");
                useComposite = true;
            }
            
            String fullLookupName = name + "," + contextPath;
            
            Attributes userAttrs = null;
            if (useComposite) {
                logger.trace(CLAS, "getUserGroupData", "Using CompositeName fullLookupName: " + fullLookupName);
            	Name n = new CompositeName().add(fullLookupName);
            	userAttrs = ctx.getAttributes(n);
            } else  {
                logger.trace(CLAS, "getUserGroupData", "Using String fullLookupName: " + fullLookupName);
            	  userAttrs = ctx.getAttributes(fullLookupName);
            }

            String id = null;
            id = (String) userAttrs.get("cn").get();
            String desc = "";
            if (userAttrs.get("description") != null) {
                desc = (String) userAttrs.get("description").get();
            }

            String nameNonEscape = "cn=" + escapeWithCharCodes(id) + "," + contextPath;

            String[] groups = getGroupsForName(ctx, groupContext, nameNonEscape);

            user = new UserGroupData(id, desc, groups);
            if (userAttrs.get("employeeNumber") != null) {
                user.setEmployeeNum((String) userAttrs.get("employeeNumber").get());
            }

        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getUserGroupData", "CWLNA5060");
        }
        logger.traceExit(CLAS, "getUserGroupData");
        return user;
    }

    /**
     * Retrieves a list of objects that the entry belongs too. This is context agnostic and the location on the LDAP
     * much be specified when calling this method.
     * 
     * @param ctx Directory context handle of the server to use.
     * @param groupContext The LDAP subcontext which the groups associated with the entry can be part off.
     * @param fullLookupName Full path to the entry including subcontext
     * @return String[] List of groups the entry belongs too.
     */
    private String[] getGroupsForName(DirContext ctx, String groupContext, String fullLookupName) {
        logger.traceEntry(CLAS, "getGroupsForName");
        ArrayList<String> userGroups = new ArrayList<String>();

        try {
            NamingEnumeration<SearchResult> results = searchLDAP(ctx, groupContext, "member", fullLookupName);

            while (results.hasMore()) {
                SearchResult sr = (SearchResult) results.next();
                Attributes groupAttrs = sr.getAttributes();
                String groupId = (String) groupAttrs.get("cn").get();
                userGroups.add(groupId);
            }
        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getGroupsForName", "CWLNA5060");
        }
        logger.traceExit(CLAS, "getGroupsForName");
        return userGroups.toArray(new String[0]);
    }

    private String generateEmployeeNum(String id) {
        UserGroupListResponse users = listPath(CONTEXT_WEBUI_PEOPLE, CONTEXT_WEBUI_GROUP, false);

        Random r = new Random(System.currentTimeMillis());
        String newEmployeeNum = "";

        boolean unique = false;
        while (!unique) {
        	// using 0-32767 is consistent with CLI. If we get too big of a String CLI cannot get
        	// an authentication token...
            newEmployeeNum = "" + r.nextInt(32767);
            unique = true;
            for(int i = 0; i < users.getData().length; i++) {
                if (users.getData()[i].getID().equals(id)) {
                    continue;
                }
                boolean isSysAdmin = false;
                for(int grp = 0; grp < users.getData()[i].getGroups().length; grp++) {
                    if (users.getData()[i].getGroups()[grp].equals(WEBUIGROUP_SYSTEM_ADMIN)) {
                        isSysAdmin = true;
                        break;
                    }
                }
                if (isSysAdmin) {
                    if (users.getData()[i].getEmployeeNum().equals(newEmployeeNum)) {
                        unique = false;
                        break;
                    }
                }
            }
        }

        return newEmployeeNum;
    }

    /**
     * Searchs the LDAP server for entries based on the specified parameters.
     * 
     * @param ctx Directory context handle of the server to use.
     * @param searchTree The context to search
     * @param searchAttributeName The attribute name that the search will be performed on
     * @param searchAttributeValue The attribute value that the search will be performed on.
     * @return NamingEnumeration Enumeration of the entries found by the search. It can be empty.
     */
    private NamingEnumeration<SearchResult> searchLDAP(DirContext ctx, String searchTree, String searchAttributeName,
            String searchAttributeValue) {
        logger.traceEntry(CLAS, "searchLDAP");
        Attributes matchAttrs = new BasicAttributes(true);
        Attribute searchAttr = new BasicAttribute(searchAttributeName, searchAttributeValue);
        matchAttrs.put(searchAttr);

        NamingEnumeration<SearchResult> answer = null;
        try {
            answer = ctx.search(searchTree, matchAttrs);
        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "searchLDAP", "CWLNA5060");
        }
        logger.traceExit(CLAS, "searchLDAP");
        return answer;
    }

    /**
     * Connects to the on-the-box LDAP server and returns a handle to the root context.
     * 
     * @return Directory context handle of the server.
     */
    private DirContext connectToOnBoardLDAP() {
        logger.traceEntry(CLAS, "connectToOnBoardLDAP");
        DirContext ctx = null;
        Hashtable<String, String> env = new Hashtable<String, String>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
        env.put(Context.PROVIDER_URL, ISMLDAPProperties.getURL());
        env.put(Context.SECURITY_AUTHENTICATION, ISMLDAPProperties.getAuthenticationType());
        env.put(Context.SECURITY_PRINCIPAL, ISMLDAPProperties.getAuthenticationPrincipalID());
        env.put(Context.SECURITY_CREDENTIALS, ISMLDAPProperties.getAuthenticationPassword());
        try {
            ctx = new InitialDirContext(env);
        } catch (NamingException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "connectToOnBoardLDAP", "CWLNA5060");
        }
        logger.traceExit(CLAS, "connectToOnBoardLDAP");
        return ctx;
    }

    private boolean connectWithOldPw(String id, String key) {
        logger.traceEntry(CLAS, "connectWithOldPw");
        LdapContext ctx = null;
        Hashtable<String, String> env = new Hashtable<String, String>();
        env.put(Context.INITIAL_CONTEXT_FACTORY, "com.sun.jndi.ldap.LdapCtxFactory");
        env.put(Context.PROVIDER_URL, ISMLDAPProperties.getURL());
        env.put(Context.SECURITY_AUTHENTICATION, ISMLDAPProperties.getAuthenticationType());
        env.put(Context.SECURITY_PRINCIPAL, "cn="+escapeSpecialCharacters(id, true)+",ou=people,ou=webui,dc=ism.ibm,dc=com");
        env.put(Context.SECURITY_CREDENTIALS, key);
        try {
            ctx = new InitialLdapContext(env, null);
            ctx.close();
        } catch (/*Naming*/Exception e) {
        	e.printStackTrace();
            logger.log(LogLevel.LOG_ERR, CLAS, "connectWithOldPw", "CWLNA5060");
            ctx = null;
            return false;
        }
        ctx = null;
        logger.traceExit(CLAS, "connectWithOldPw");
        return true;
    }

    private String escapeSpecialCharacters(String input, boolean isJNDIString) {
        String output = Rdn.escapeValue(input);

        if (isJNDIString) {
            // The above would be fine, but JNDI doesn't like slashes or quotes and they must be escaped again!
            String[] specialCharsRegex = { "\\", "\"" };

            for (String regex : specialCharsRegex) {
                output = output.replace(regex, "\\" + regex);
            }

            output = output.replace("/", "\\/");
        }
        return output;
    }

    private String escapeWithCharCodes(String input) {
        String output = input;
        char[] chars = { ',', '+', '"', '\\', '<', '>', ';', '#' };
        boolean performEscape = false;
        for (char c : chars) {
            if (output.contains("" + c)) {
                performEscape = true;
                break;
            }
        }

        if (performEscape) {
            output = "";
            for (int i = 0; i < input.length(); i++) {
                char foundChar = '0';
                for (char c : chars) {
                    if (input.charAt(i) == c) {
                        foundChar = c;
                        break;
                    }
                }
                if (foundChar != '0') {
                    output += "\\" + Integer.toHexString(foundChar);
                } else {
                    output += input.charAt(i);
                }
            }
        }
        return output;
    }

    private boolean isIdValid(String id) {
        if ((id.charAt(0) == ' ') || (id.charAt(id.length() - 1) == ' ')) {
            return false;
        }
        return true;
    }

    private boolean isExternalLDAPEnabled() {
        LDAPResource ldapResource = new LDAPResource();
        List<LDAPConnection> connections = ldapResource.getLDAPConnections(null);  // TODO if we support editing messaging users in docker, need to fix this
        if (connections != null && connections.size() > 0) {
            return connections.get(0).getEnabled();
        }
        return false;
    }
}
