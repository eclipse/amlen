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

import java.io.File;
import java.io.IOException;
import java.net.URI;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

import javax.ws.rs.Consumes;
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
import javax.ws.rs.core.UriInfo;

import com.ibm.websphere.jaxrs20.multipart.IMultipartBody;
import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.admin.request.IsmConfigDeleteSecurityProfileRequest;
import com.ibm.ima.admin.request.IsmConfigGetSecurityProfileRequest;
import com.ibm.ima.admin.request.IsmConfigSetSecurityProfileRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.AbstractIsmConfigResource;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.VALIDATION_RESULT;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;
import com.ibm.ima.ISMWebUIProperties;

/**
 * Servlet implementation class SecurityResource
 */
@Path("/config/securityProfiles/{domainUid}")
public class SecurityProfilesResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = SecurityProfilesResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    //Relative to com.ibm.ima.serverInstallDirectory (only works when installed alongsidethe server 
    private final static String TRUSTSTORE_DIR = "/certificates/truststore/";
    
    //Relative to com.ibm.ima.serverDataDirectory (only works when installed alongsidethe server 
    private final static String USERFILES_DIR = "/userfiles";

    public final static int SECURITY_PROFILE_RESOURCE_SUCCESS = 0;
    public final static int SECURITY_PROFILE_RESOURCE_FAILED = 1;


    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public SecurityProfilesResource() {
        super();
    }

    /**
     * Retrieves all SecurityProfiles on the ISM Server.
     * 
     * @return A list of all SecurityProfiles.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator, Role.MessagingAdministrator}, action = "getSecurityProfiles")
    public Response getSecurityProfiles(@PathParam("domainUid") String serverInstance) {
        logger.traceEntry(CLAS, "getSecurityProfiles");

        List<SecurityProfile> result;

        IsmConfigGetSecurityProfileRequest configRequest = new IsmConfigGetSecurityProfileRequest(getUserId());
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(configRequest, new TypeReference<List<SecurityProfile>>(){});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) {  // "not found"
                result = new ArrayList<SecurityProfile>();
            } else {
                logger.trace(CLAS, "getSecurityProfiles", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<SecurityProfile>)resultObject;
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);

        logger.traceExit(CLAS, "getSecurityProfiles", result.toString());
        return rb.cacheControl(cache).build();

    }

    /**
     * Retrieves the named SecurityProfile from the ISM Server.
     * 
     * @return A response including the named SecurityProfile if it could be retrieved.
     */
    @GET
    @Path("{name}")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator, Role.MessagingAdministrator}, action = "getSecurityProfile")
    public Response getSecurityProfile(@PathParam("domainUid") String serverInstance, @PathParam("name") String name) {
        logger.traceEntry(CLAS, "getSecurityProfile", new Object[]{name});

        List<SecurityProfile> result = _getSecurityProfile(serverInstance, name);
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result.get(0));

        logger.traceExit(CLAS, "getSecurityProfile", result.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * @param name
     * @param result
     * @return
     */
    @SuppressWarnings("unchecked")
    private List<SecurityProfile> _getSecurityProfile(String serverInstance, String name) {
        // is it a valid name?
        List<SecurityProfile> result = null;
        SecurityProfile profile = new SecurityProfile();
        ValidationResult validationResult = profile.validateName(name, "Name");
        if (!validationResult.isOK()) {
            throw new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5017", new Object[] {name});
        } else {
            // yes, try to get it
            IsmConfigGetSecurityProfileRequest configRequest = new IsmConfigGetSecurityProfileRequest(getUserId(),name);
            configRequest.setServerInstance(serverInstance);
            Object resultObject = sendGetConfigRequest(configRequest, new TypeReference<List<SecurityProfile>>(){});
            if (resultObject instanceof String) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
            } else if (resultObject instanceof IsmResponse) {
                IsmResponse ismResponse = (IsmResponse)resultObject;
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = name;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            } else {
                result = (List<SecurityProfile>)resultObject;
            }
        }
        return result;
    }

    /**
     * Adds a (@link SecurityProfile) object to the ISM server configuration.
     * 
     * @param securityProfile The (@link SecurityProfile) object to add.
     * @return SecurityProfileResponse
     */
    @POST
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "addSecurityProfile")
    public Response addSecurityProfile(@PathParam("domainUid") String serverInstance, SecurityProfile securityProfile) {
        logger.traceEntry(CLAS, "addSecurityProfile", new Object[]{securityProfile});

        ValidationResult vr = null;

        if (securityProfile == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, VALIDATION_RESULT.VALUE_EMPTY.getMessageID(), new Object[]{"certificateProfile"});
        }
        if (!(vr = securityProfile.validate()).isOK()) {
            // validation failed
            throw new IsmRuntimeException(Status.BAD_REQUEST, vr.getResult().getMessageID(), vr.getParams());
        }

        // validated, so try to create it...
        IsmConfigSetSecurityProfileRequest configRequest = new IsmConfigSetSecurityProfileRequest(getUserId(),securityProfile);
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
        URI location = getResourceUri(securityProfile.getName());
        logger.trace(CLAS, "addCertificateProfile", "location: " + location.toString());
        ResponseBuilder response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addSecurityProfile", result.toString());
        return result;
    }

    /**
     * Updates a (@link SecurityProfile) object in the ISM server configuration.
     * 
     * @param securityProfile The (@link SecurityProfile) object to update.
     * @return SecurityProfileResponse
     */
    @PUT
    @Path("{name}")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "updateSecurityProfile")
    public Response updateSecurityProfile(@PathParam("domainUid") String serverInstance, @PathParam("name") String name, SecurityProfile securityProfile) {
        logger.traceEntry(CLAS, "updateSecurityProfile", new Object[]{securityProfile});

        SecurityProfile result = securityProfile;
        ValidationResult vr = null;

        if (securityProfile == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, VALIDATION_RESULT.VALUE_EMPTY.getMessageID(), new Object[]{"SecurityProfile"});
        } else if (!(vr = securityProfile.validate()).isOK()) {
            // validation failed
            throw new IsmRuntimeException(Status.BAD_REQUEST, vr.getResult().getMessageID(), vr.getParams());
        } else {

            // validated, so try to create it...
            if (!name.equals(securityProfile.getName())) {
                securityProfile.setKeyName(name);
            }
            _updateSecurityProfile(serverInstance, securityProfile);
        }

        ResponseBuilder rb =  Response.ok(result);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);

        logger.traceExit(CLAS, "getSecurityProfile", result.toString());
        return rb.cacheControl(cache).build();
    }
    
    private void _updateSecurityProfile(String serverInstance, SecurityProfile securityProfile) {
        IsmConfigSetSecurityProfileRequest configRequest = new IsmConfigSetSecurityProfileRequest(getUserId(), securityProfile);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = securityProfile.getName();
                } else if (ismResponse.getRC().equals(Utils.OAUTH_NOT_FOUND)) {
                    message = securityProfile.getoAuthProfile();
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }
    }

    /**
     * Deletes a (@link SecurityProfile) object in the ISM server configuration.
     * 
     * @param securityProfile The (@link SecurityProfile) object to delete.
     * @return SecurityProfileResponse
     */
    @DELETE
    @Path("{name}")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "deleteSecurityProfile")
    public void deleteSecurityProfile(@PathParam("domainUid") String serverInstance, @PathParam("name") String name) {
        logger.traceEntry(CLAS, "deleteSecurityProfile", new Object[]{name});

        IsmConfigDeleteSecurityProfileRequest configRequest = new IsmConfigDeleteSecurityProfileRequest(getUserId(),name);
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
    }


    /**
     * Gets list of certificates from a security profile store.
     * 
     * @param securityProfile The security profile to delete the certificate from.
     */
    @GET
    @Path("{securityProfile}/truststore")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "listCertificates")
    public Response listCertificates(@PathParam("domainUid") String serverInstance, @PathParam("securityProfile") String securityProfile) {

        logger.traceEntry(CLAS, "listCertificates", new Object[]{securityProfile});
        
        // make sure the profile exists
        List<SecurityProfile> profiles = _getSecurityProfile(serverInstance, securityProfile);
        
        ArrayList<String> certificates = getTrustedCertificates(profiles.get(0).getName());
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(certificates);

        return rb.cacheControl(cache).build();
    }

    private static ArrayList<String> getTrustedCertificates(String profileName) {
        ArrayList<String> certificates = null;
        File trustStoreDir = Utils.safeFileCreate(ISMWebUIProperties.instance().getServerInstallDirectory()+TRUSTSTORE_DIR, profileName, true);
        if (trustStoreDir != null && trustStoreDir.exists()) {
            try {
                certificates = CertificatesFileUploader.getCertificates(trustStoreDir.getCanonicalPath());
            } catch (IOException e) {
                logger.log(LogLevel.LOG_WARNING, CLAS, "listCertificates", "CWLNA5005", e);
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5005", new Object[] {e.getMessage()});
            }
        }
        if (certificates == null) {
            certificates = new ArrayList<String>();
        }
        return certificates;
    }

    /*
     * Helper method to determine if the names security profile has any trusted certificates
     * in it's truststore.
     * @param security profile name
     * @return true if the truststore exists, can be read, and is non-empty, false otherwise
     */
    public static boolean hasTrustedCertificates(String profileName) {
        try {
            return !getTrustedCertificates(profileName).isEmpty();
        } catch (IsmRuntimeException ire) {
            return false;
        }
    }

    /**
     * Adds or updates a certificate from a security profile store.
     * 
     * @param securityProfile The security profile to upload the certificate to.
     */
    @POST
    @Path("{securityProfile}/truststore")
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "uploadCertificates")
    public Response uploadCertificates(@PathParam("domainUid") String serverInstance, @PathParam("securityProfile") String securityProfile,
            @Context UriInfo uriInfo, IMultipartBody mpMessage) {
        logger.traceEntry(CLAS, "uploadCertificates", new Object[]{securityProfile});

        CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
        String result = null;
        
        try {
//            // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//            // we need to check for Standby on our own
//            checkForStandbyMode(serverInstance);
            
            // make sure the profile exists
            List<SecurityProfile> profiles = _getSecurityProfile(serverInstance, securityProfile);

            // handle the file upload to userfiles dir
            ArrayList<String> files = new ArrayList<String>();
            result = uploader.uploadFiles(uriInfo, mpMessage, getClientLocale(), ISMWebUIProperties.instance().getServerDataDirectory()+USERFILES_DIR, files);

            // verify and copy to the truststore
            StringBuilder errors = new StringBuilder();
            Locale locale = getClientLocale();
            Object[] params = new Object[3];
            params[1] = securityProfile;
            String truststoreDir = ISMWebUIProperties.instance().getServerInstallDirectory()+TRUSTSTORE_DIR+securityProfile;
            
            updateUseClientCertificates(serverInstance, profiles.get(0));

            if (errors.length() > 0) {
                // at least one error occurred
                throw  new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5088", new Object[] {errors.toString()});
            }
        } catch (IsmRuntimeException ire) {
            // IE 9 doesn't tolerate exceptions during file upload...
            if (uploader.isIE9()) {
                return uploader.buildTextAreaExceptionResponse(ire);
            }
            throw ire;
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(result);

        return rb.cacheControl(cache).build();
    }


    /**
     * Update the UseClientCertificate of the specified security profile. If any certificates exist
     * in the truststore, set to true, otherwise set to false
     * @param securityProfile
     */
    private boolean updateUseClientCertificates(String serverInstance, SecurityProfile securityProfile) {
        logger.traceEntry(CLAS, "updateUseClientCertificate", new Object[]{securityProfile});
        String useClientCertificates = "True";
      
        ArrayList<String> certificates = null;
        File trustStoreDir = Utils.safeFileCreate(
                                    ISMWebUIProperties.instance().getServerInstallDirectory()+TRUSTSTORE_DIR,
                                    securityProfile.getName(), true);

        if (trustStoreDir != null && trustStoreDir.exists()) {
            try {
                certificates = CertificatesFileUploader.getCertificates(trustStoreDir.getCanonicalPath());
            } catch (IOException e) {
                logger.log(LogLevel.LOG_WARNING, CLAS, "listCertificates", "CWLNA5011", e);
            }
        }

        if (certificates == null || certificates.isEmpty()) {
            useClientCertificates = "False";
        }
        securityProfile.setClientCertificateUsed(useClientCertificates);
        _updateSecurityProfile(serverInstance, securityProfile);
        return useClientCertificates.equals("True");
    }
}
