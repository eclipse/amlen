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

import java.text.DateFormat;
import java.text.SimpleDateFormat;
import java.util.Locale;

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

import com.ibm.ima.resources.data.UserSecurityAndPrefsResponse;
import com.ibm.ima.resources.data.UserSecurityResponse;
import com.ibm.ima.resources.data.utils.PreferencesManager;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.node.ObjectNode;
import com.fasterxml.jackson.databind.ObjectMapper;

/**
 * Servlet implementation class PreferencesResource
 */
@Path("/config/webui/preferences")
@Produces(MediaType.APPLICATION_JSON)
public class PreferencesResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String PROPERTY_KEY = "value";
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    private final static String CLAS = PreferencesResource.class.getCanonicalName();
    
    private final static String DEFAULT_HTTP_PORT = "default_http_port";
    private final static String DEFAULT_HTTPS_PORT = "default_https_port";
    
    private final static String LIBERTY_DEFAULT_TRACE_SPEC = "*=audit=enabled";
    private final static String TRACE_SPEC_PROPERTY = "logging_traceSpecification";
    
    private final static int LOW_END = 9000;
    private final static int HIGH_END = 9099;
    private final static int LIBERTY_HTTPS = 9087;
    
    @Context
    javax.servlet.http.HttpServletRequest currentRequest;


    /**
     * Returns the global preferences.
     */
    @GET
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator,Role.User}, action = "getPreferences")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getPreferences() {
        logger.traceEntry(CLAS, "getPreferences");

        JsonNode response = PreferencesManager.getInstance().getPreferences(null);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getPreferences", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Returns an object containing the details of the user that is making the request taken directly from the
     * HttpServletRequest object. The username, roles, preferences, and licenseAccept are extracted and returned as a JSON object.
     * 
     * @return A JSON Object containing the details of the current user.
     */
    @GET
    @Path("authAndPrefs")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "getWebUIUserSecurityAndPreferences")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getWebUIUserSecurityAndPreferences() {
        logger.traceEntry(CLAS, "getUserSecurity");

        UserSecurityResponse userSecurity = UserResource.getUserSecurity(currentRequest);
        UserSecurityAndPrefsResponse response = new UserSecurityAndPrefsResponse(userSecurity.getUsername(), userSecurity.getUserRoles());
        if (userSecurity.getUserRoles().length > 0) {
            response.setLicenseStatus(LicenseResource.getLicenseAccepted().getStatus());
            response.setPreferences(PreferencesManager.getInstance().getPreferences(userSecurity.getUsername()));
        }

        response.setLocaleInfo(getLocaleInfo());

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(response);
        rb.cookie(UserResource.getCookie(currentRequest));
        logger.traceExit(CLAS, "getUserSecurity", response);
        return rb.cacheControl(cache).build();
    }

    private JsonNode getLocaleInfo() {

        Locale l = null;
        String localeString = "en";
        String dateTimeFormatString = "yyyy-MM-dd'T'HH:mm";
        String dateFormatString = "yyyy-MM-dd";
        String timeFormatString = "HH:mm:ss";

        try {
            l = currentRequest.getLocale();

            localeString = l.getLanguage();
            String country = l.getCountry();
            if (country != null && country.length() > 0) {
                localeString += "-" + country.toLowerCase();
            }

            Object formatter = DateFormat.getDateTimeInstance(DateFormat.SHORT, DateFormat.MEDIUM, l);
            if (formatter instanceof SimpleDateFormat) {
                dateTimeFormatString = ((SimpleDateFormat) formatter).toPattern();
            }

            formatter = DateFormat.getDateInstance(DateFormat.SHORT, l);
            if (formatter instanceof SimpleDateFormat) {
                dateFormatString = ((SimpleDateFormat) formatter).toPattern();
            }

            formatter = DateFormat.getTimeInstance(DateFormat.MEDIUM, l);
            if (formatter instanceof SimpleDateFormat) {
                timeFormatString = ((SimpleDateFormat) formatter).toPattern();
            }
        } catch (Throwable t) {
            logger.log(LogLevel.LOG_NOTICE, CLAS, "getLocaleInfo", "CWLNA5097", new Object[] { l }, t);
        }

        ObjectMapper objectMapper = new ObjectMapper();
        ObjectNode localeInfo = objectMapper.createObjectNode();
        localeInfo.put("localeString", localeString);
        localeInfo.put("dateTimeFormatString", dateTimeFormatString);
        localeInfo.put("dateFormatString", dateFormatString);
        localeInfo.put("timeFormatString", timeFormatString);

        return localeInfo;
    }


    /**
     * Returns the preferences for the requested user, including global preferences
     */
    @GET
    @Path("{userId}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator,Role.User}, action = "getPreferencesForUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getPreferencesForUser(@PathParam("userId") String userId) {
        logger.traceEntry(CLAS, "getPreferencesForUser");

        JsonNode response = PreferencesManager.getInstance().getPreferences(userId);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getPreferencesForUser", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Updates the preferences for the specified user, including any global preferences provided
     */
    @PUT
    @Path("{userId}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator,Role.User}, action = "updatePreferencesForUser")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response updatePreferencesForUser(@PathParam("userId") String userId, JsonNode preferences) {
        logger.traceEntry(CLAS, "updatePreferencesForUser");

        PreferencesManager.getInstance().setPreferences(userId, preferences);
        JsonNode response = PreferencesManager.getInstance().getPreferences(userId);
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "updatePreferencesForUser", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Returns one of the liberty profile properties
     */
    @GET
    @Path("liberty/{property}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getLibertyProfileProperty")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getLibertyProfileProperty(@PathParam("property") String property){
        logger.traceEntry(CLAS, "getLibertyProfileProperty");
        ObjectMapper objectMapper = new ObjectMapper();
        ObjectNode response = objectMapper.createObjectNode();

        try {
            // checking a few times in case the file is busy
            int retry = 0;
            String value = null;
            while (retry < 5) {
                value = Utils.getUtils().getLibertyProperty(property);
                if (value.equals(Utils.PROPERTY_NOT_FOUND)) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "getLibertyProfileProperty", "CWLNA5069", new Object[] {property});
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5069", new Object[] {property});
                } else if (value.equals(Utils.PROPRETIES_NOT_AVAILABLE)) {
                    value = null;
                    retry++;
                    Thread.sleep(500);
                } else {
                    if (property.equals(TRACE_SPEC_PROPERTY) && LIBERTY_DEFAULT_TRACE_SPEC.equals(value)) {
                        // return the empty string
                        value = "";
                    }
                    response.put(property, value);
                    break;
                }
            }
            if (value == null) {
                logger.log(LogLevel.LOG_ERR, CLAS, "getLibertyProfileProperty", "CWLNA5068");
                logger.trace(CLAS, "getLibertyProfileProperty", "Failed to get liberty property " + property);
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5068", null);
            }
        } catch (IsmRuntimeException isme) {
            throw isme;
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getLibertyProfileProperty", "CWLNA5000", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5000", new Object[] {property});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getLibertyProfileProperty", response);
        return rb.cacheControl(cache).build();
    }


    /**
     * Updates one of the Liberty profile properties
     */
    @PUT
    @Path("liberty/{property}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getEthernetResource")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response setLibertyProfileProperty(@PathParam("property") String property, JsonNode data){
        logger.traceEntry(CLAS, "setLibertyProfileProperty");
        
        // even though this couldn't happen, FindBugs complains about it... so add a check
        if (data == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);            
        }
        
        String value = data.get(PROPERTY_KEY).textValue();

        ObjectMapper objectMapper = new ObjectMapper();
        ObjectNode response = objectMapper.createObjectNode();
        
        if ((data != null)&&(value != null)) {
            try {
                // check if property to change is the port number
                if (property.equals(DEFAULT_HTTP_PORT)||property.equals(DEFAULT_HTTPS_PORT)) {
                    if (!portWithinRange(value)) {
                        logger.log(LogLevel.LOG_ERR, CLAS, "setLibertyProfileProperty", "CWLNA5086");
                        throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5086", new Object[] {value});
                    }
                } else if (property.equals(TRACE_SPEC_PROPERTY) && (value == null || value.trim().length() == 0)) {
                    // restore the default traceSpecification
                    value = LIBERTY_DEFAULT_TRACE_SPEC;
                }
                // checking a few times in case the file is busy
                int retry = 0;
                while (retry < 5) {
                    String result = Utils.getUtils().setLibertyProperty(property, value);
                    if (result.equals(Utils.PROPERTY_NOT_FOUND)) {
                        logger.log(LogLevel.LOG_ERR, CLAS, "setLibertyProfileProperty", "CWLNA5069", new Object[] {property});
                        throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5069", new Object[] {property});
                    } else if (result.equals(Utils.PROPRETIES_NOT_AVAILABLE)) {
                        retry++;
                        Thread.sleep(500);
                    } else if (result.equals(Utils.SUCCESS)) {
                        response.put(property,value);
                        break;
                    }
                }
                if (retry == 5) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "setLibertyProfileProperty", "CWLNA5068");
                    throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5068", new Object[] {property});
                }
            } catch (IsmRuntimeException isme) {
                throw isme;
            } catch (Exception e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "setLibertyProfileProperty", "CWLNA5000", e);
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5000", new Object[] {property});
            }
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "setLibertyProfileProperty", response);
        return rb.cacheControl(cache).build();
    }
    
    /**
     * Helper method to check for valid port number
     */
    private boolean portWithinRange(String value) {
        int port = Integer.parseInt(value);
        if ((port != 0)&&((port < LOW_END)||(port > HIGH_END)||(port == LIBERTY_HTTPS))) {
            return true;
        }
        return false;
    }
}
