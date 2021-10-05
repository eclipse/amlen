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

import java.util.ArrayList;
import java.util.List;

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

import com.ibm.ima.admin.request.IsmConfigDeleteCertificateProfileRequest;
import com.ibm.ima.admin.request.IsmConfigGetCertificateProfileRequest;
import com.ibm.ima.admin.request.IsmConfigSetCertificateProfileRequest;
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

/**
 * Servlet implementation class CertificateResource
 */
@Path("/config/certificateProfiles/{domainUid}")
public class CertificateProfilesResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = CertificateProfilesResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    public final static int CERTIFICATE_PROFILE_RESOURCE_SUCCESS = 0;
    public final static int CERTIFICATE_PROFILE_RESOURCE_FAILED = 1;

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public CertificateProfilesResource() {
        super();
    }

    /**
     * Retrieves all CertificateProfiles on the ISM Server.
     * 
     * @return A list of all CertificateProfiles.
     */
    @GET
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "getCertificateProfiles")
    public Response getCertificateProfiles(@PathParam("domainUid") String serverInstance) {
        logger.traceEntry(CLAS, "getCertificateProfiles");

        List<CertificateProfile> response = listCertificateProfiles(serverInstance);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(response);

        logger.traceExit(CLAS, "getCertificateProfiles", response.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Retrieves the named CertificateProfile from the ISM Server.
     * 
     * @return A response including the named CertificateProfile if it could be retrieved.
     */
    @SuppressWarnings("unchecked")
    @GET
    @Path("{name}")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "getCertificateProfile")
    public Response getCertificateProfile(@PathParam("domainUid") String serverInstance, @PathParam("name") String name) {
        logger.traceEntry(CLAS, "getCertificateProfile", new Object[] { name });

        List<CertificateProfile> result;

        // is it a valid name?
        CertificateProfile profile = new CertificateProfile();
        ValidationResult validationResult = profile.validateName(name, "Name");
        if (!validationResult.isOK()) {
            throw new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5017", new Object[] { name });
        } else {
            // yes, try to get it
            IsmConfigGetCertificateProfileRequest configRequest = new IsmConfigGetCertificateProfileRequest(getUserId(), name);
            configRequest.setServerInstance(serverInstance);
            Object resultObject = sendGetConfigRequest(configRequest, new TypeReference<List<CertificateProfile>>() {});
            if (resultObject instanceof String) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
            } else if (resultObject instanceof IsmResponse) {
                IsmResponse ismResponse = (IsmResponse) resultObject;
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = name;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            } else {
                result = (List<CertificateProfile>) resultObject;
                if (!result.isEmpty()) {
                    profile = result.get(0);
                    if (profile != null && profile.getExpirationDate() == null) {
                        CertificatesFileUploader.setCertificateExpiry(profile);
                    }
                }
            }
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(profile);

        logger.traceExit(CLAS, "getCertificateProfile", profile);
        return rb.cacheControl(cache).build();
    }

    @POST
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "uploadFiles")
    public Response uploadFiles(@PathParam("domainUid") String serverInstance, @Context UriInfo uriInfo, IMultipartBody mpMessage) {
        // System.out.println("uploadFile in CertificateProfilesResource");
    	logger.traceEntry(CLAS, "uploadFiles");

        CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
        String result = null;
        try {
//            // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//            // we need to check for Standby on our own
//            checkForStandbyMode(serverInstance);
            result = uploader.uploadFiles(uriInfo, mpMessage, getClientLocale(), null, null);
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

    	logger.traceExit(CLAS, "uploadFiles");
    	
        return rb.cacheControl(cache).build();
    }

    /**
     * Adds a (@link CertificateProfile) object to the ISM server configuration.
     * 
     * @param certificateProfile The (@link CertificateProfile) object to add.
     * @return CertificateProfileResponse
     */
    @POST
    @Consumes(MediaType.APPLICATION_JSON)
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "addCertificateProfile")
    public Response addCertificateProfile(@PathParam("domainUid") String serverInstance, CertificateProfileRequest certificateProfileRequest) {

        logger.traceEntry(CLAS, "addCertificateProfile", new Object[] { certificateProfileRequest });        
        
        ValidationResult vr = null;

        if (certificateProfileRequest == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, VALIDATION_RESULT.VALUE_EMPTY.getMessageID(), new Object[] { "certificateProfile" });
        }

        CertificateProfile certificateProfile = new CertificateProfile(certificateProfileRequest.getName(), certificateProfileRequest.getCertificate(), certificateProfileRequest.getKey());

        List<CertificateProfile> existingCertificates = listCertificateProfiles(serverInstance);
        String cert = certificateProfileRequest.getCertificate();
        String key = certificateProfileRequest.getKey();
        for (CertificateProfile certificate : existingCertificates) {
            if (certificate.getCertFilename().equals(cert) || certificate.getCertFilename().equals(key)) {
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5066", new Object[] {cert, certificate.getName()});
            }
            if (certificate.getKeyFilename().equals(cert) || certificate.getKeyFilename().equals(key)) {
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5066", new Object[] {key, certificate.getName()});
            }
        }

        if (!(vr = certificateProfile.validate()).isOK()) {
            // validation failed
            throw new IsmRuntimeException(Status.BAD_REQUEST, vr.getErrorMessageID(), vr.getParams());
        }

        CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
        uploader.processCertificatesAndPasswords(certificateProfile, certificateProfileRequest.getCertificatePassword(), certificateProfileRequest.getKeyPassword(), true);

        // validated, so try to create it...
        IsmConfigSetCertificateProfileRequest configRequest = new IsmConfigSetCertificateProfileRequest(getUserId(), certificateProfile);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse) resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils().getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder response = Response.ok(certificateProfile);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addCertificateProfile", result.toString());
        return result;

    }

    /**
     * Updates a (@link CertificateProfile) object in the ISM server configuration.
     * 
     * @param certificateProfile The (@link CertificateProfile) object to update.
     * @return CertificateProfileResponse
     */
    @PUT
    @Path("{name}")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "updateCertificateProfile")
    public Response updateCertificateProfile(@PathParam("domainUid") String serverInstance, @PathParam("name") String name, CertificateProfileRequest certificateProfileRequest) {
        logger.traceEntry(CLAS, "updateCertificateProfile", new Object[] { certificateProfileRequest });

        ValidationResult vr = null;

        if (certificateProfileRequest == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, VALIDATION_RESULT.VALUE_EMPTY.getMessageID(), new Object[] { "certificateProfile" });
        }

        CertificateProfile certificateProfile = new CertificateProfile(certificateProfileRequest.getName(), certificateProfileRequest.getCertificate(), certificateProfileRequest.getKey());
        if (!name.equals(certificateProfile.getName())) {
            certificateProfile.setKeyName(name);
        }

        String cert = certificateProfileRequest.getCertificate();
        String key = certificateProfileRequest.getKey();

        List<CertificateProfile> existingCertificates = listCertificateProfiles(serverInstance);
        for (CertificateProfile certificate : existingCertificates) {
            if (certificate.getName().equals(name)) continue;

            if (certificate.getCertFilename().equals(cert) || certificate.getCertFilename().equals(key)) {
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5066", new Object[] {cert, certificate.getName()});
            }
            if (certificate.getKeyFilename().equals(cert) || certificate.getKeyFilename().equals(key)) {
                throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5066", new Object[] {key, certificate.getName()});
            }

        }

        if (!(vr = certificateProfile.validate()).isOK()) {
            // validation failed
            throw new IsmRuntimeException(Status.BAD_REQUEST, vr.getErrorMessageID(), vr.getParams());
        }

        CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
        uploader.processCertificatesAndPasswords(certificateProfile, certificateProfileRequest.getCertificatePassword(), certificateProfileRequest.getKeyPassword(), false);

        // validated, so try to create it...
        IsmConfigSetCertificateProfileRequest configRequest = new IsmConfigSetCertificateProfileRequest(getUserId(), certificateProfile);
        configRequest.setUpdate();
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse) resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = name;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(certificateProfile);

        logger.traceExit(CLAS, "updateCertificateProfile", certificateProfile.toString());
        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes a (@link CertificateProfile) object in the ISM server configuration.
     * 
     * @param certificateProfile The (@link CertificateProfile) object to delete.
     * @return CertificateProfileResponse
     */
    @DELETE
    @Path("{name}")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "deleteCertificateProfile")
    public void deleteCertificateProfile(@PathParam("domainUid") String serverInstance, @PathParam("name") String name) {
        logger.traceEntry(CLAS, "deleteCertificateProfile", new Object[] { name });

        IsmConfigDeleteCertificateProfileRequest configRequest = new IsmConfigDeleteCertificateProfileRequest(getUserId(), name);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);

        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse) resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                String message = ismResponse.toString();
                if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                    message = name;
                }
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        }
    }

    @SuppressWarnings("unchecked")
    private List<CertificateProfile> listCertificateProfiles(String serverInstance) {
        List<CertificateProfile> certs = new ArrayList<CertificateProfile>();

        // CertificateProfileResponse response = new CertificateProfileResponse(CERTIFICATE_PROFILE_RESOURCE_SUCCESS, result);

        IsmConfigGetCertificateProfileRequest configRequest = new IsmConfigGetCertificateProfileRequest(getUserId());
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(configRequest, new TypeReference<List<CertificateProfile>>() {});        
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse) resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) { // "not found"
                certs = new ArrayList<CertificateProfile>();
            } else {
                logger.trace(CLAS, "getCertificateProfiles", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils().getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            certs = (List<CertificateProfile>) resultObject;
            for (CertificateProfile profile : certs) {
                if (profile != null && profile.getExpirationDate() == null) {
                    try {
                        CertificatesFileUploader.setCertificateExpiry(profile);
                    } catch (Exception e) {
                        logger.log(LogLevel.LOG_WARNING, CLAS, "getCertificates", "CWLNA5005", new Object[] { e.getMessage() }, e);
                    }
                }
            }
        }

        return certs;
    }
}
