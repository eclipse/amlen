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
package com.ibm.ima.resources.security;

import java.io.File;
import java.net.URI;
import java.text.SimpleDateFormat;
import java.util.Date;

import javax.ws.rs.Consumes;
import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
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
import com.fasterxml.jackson.annotation.JsonProperty;
import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.admin.request.IsmConfigCertSyncRequest;
import com.ibm.ima.admin.request.IsmConfigGetHARoleRequest;
import com.ibm.ima.admin.request.IsmConfigRequest;
import com.ibm.ima.resources.AbstractIsmConfigResource;
import com.ibm.ima.resources.data.HARoleResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.ISMWebUIProperties;

/**
 * Servlet implementation class SSLKeyRepositoryResource
 */
@Path("/config/sslKeyRepository/{domainUid}")
public class SSLKeyRepositoryResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = SSLKeyRepositoryResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server (incl. mqcbridge)
    private final static String KEYREPOSITORY_DIRECTORY = "/certificates/MQC";

    private final static String KEY_EXTENSION = "kdb";
    private final static String PW_EXTENSION = "sth";

    private final static String SSL_KEY_FILENAME = "mqconnectivity." + KEY_EXTENSION;
    private final static String PW_STASH_FILENAME = "mqconnectivity." + PW_EXTENSION;

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    /**
     * Uploads a file to the temp directory.
     */
    @POST
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "uploadFiles")
    public Response uploadFiles(@PathParam("domainUid") String serverInstance, @Context UriInfo uriInfo, IMultipartBody mpMessage) {
        logger.traceEntry(CLAS, "uploadFiles", new Object[] {});

        BaseUploader uploader = new BaseUploader(currentRequest);
        String result = null;
        try {
//            // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//            // we need to check for Standby on our own
//            checkForStandbyMode(serverInstance);
            result = uploader.uploadFilesToTemp(uriInfo, mpMessage, null);
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
     * Adds the uploaded ssl key repository file to their applicable locations on disk
     * 
     * @param fileUpload The (@link SSLKeyRepository) object to add.
     * @return SSLKeyRepository
     */
    @POST
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "addSSLKeyFiles")
    public Response addSSLKeyFiles(@PathParam("domainUid") String serverInstance, SSLKeyRepository fileUpload) {
        logger.traceEntry(CLAS, "addSSLKeyFiles", new Object[] { fileUpload });

//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(serverInstance);
        
        if (fileUpload == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012", new Object[] { fileUpload });
        }

        if (fileUpload.getSslKeyFile() == null || fileUpload.getSslKeyFile().equals("") || 
                fileUpload.getPasswordFile() == null || fileUpload.getPasswordFile().equals("")) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5095", null);
        }
        
        if ((!fileUpload.getSslKeyFile().endsWith(KEY_EXTENSION))
                && (!fileUpload.getPasswordFile().endsWith(PW_EXTENSION))) {
            // Delete files if in store
            if (BaseUploader.checkFileExistsInTempDirectory(fileUpload.getSslKeyFile())) {
                BaseUploader.deleteFileFromDirectory(BaseUploader.TEMP_DIR, fileUpload.getSslKeyFile());
            }
            if (BaseUploader.checkFileExistsInTempDirectory(fileUpload.getPasswordFile())) {
                BaseUploader.deleteFileFromDirectory(BaseUploader.TEMP_DIR, fileUpload.getPasswordFile());
            }
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5084", null);
        }

        BaseUploader.moveFileFromTemp(fileUpload.getSslKeyFile(),
                                      ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                                      SSL_KEY_FILENAME, true);
        BaseUploader.moveFileFromTemp(fileUpload.getPasswordFile(),
                                      ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                                      PW_STASH_FILENAME, true);
        
        certSync(serverInstance);

        ResponseBuilder response = Response.ok(getSSLKeyRepositoryInfoObject());
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addSSLKeyFiles", result.toString());
        return result;
    }

    /**
     * Gets the information regarding the currently upload SSL Key Repository
     * 
     * @return Status JSON object containing the download URIs for the SSL Key Repository and the last modified date.
     */
    @GET
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "getSSLKeyInfo")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getSSLKeyInfo() {
        logger.traceEntry(CLAS, "getSSLKeyInfo", null);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(getSSLKeyRepositoryInfoObject());

        logger.traceExit(CLAS, "getSSLKeyInfo");
        return rb.cacheControl(cache).build();
    }

    private SSLKeyInfo getSSLKeyRepositoryInfoObject() {
        logger.traceEntry(CLAS, "getSSLKeyRepositoryInfoObject", null);

        long dateModified = 0;
        if (BaseUploader.checkFileExistsDirectory(
                       ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                       SSL_KEY_FILENAME)) {
            File keyFile = new File(
                       ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                       SSL_KEY_FILENAME);
            dateModified = keyFile.lastModified();
        }

        SSLKeyInfo sslKeyInfo =
                new SSLKeyInfo(dateModified, getResourceUri(SSL_KEY_FILENAME), getResourceUri(PW_STASH_FILENAME));

        logger.traceExit(CLAS, "getSSLKeyRepositoryInfoObject");

        return sslKeyInfo;
    }

    /**
     * Downloads the "mqconnectivity.kdb" file.
     */
    @GET
    @Path("mqconnectivity.kdb")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "getSSLKey")
    @Produces({ "application/octet-stream" })
    public Response getSSLKey() {
        logger.traceEntry(CLAS, "getSSLKey");

        Response resp = downloadFile(SSL_KEY_FILENAME);

        logger.traceExit(CLAS, "getSSLKey");

        return resp;
    }

    /**
     * Downloads the "mqconnectivity.sth" file.
     */
    @GET
    @Path("mqconnectivity.sth")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "getPasswordStash")
    @Produces({ "application/octet-stream" })
    public Response getPasswordStash() {
        logger.traceEntry(CLAS, "getPasswordStash");

        Response resp = downloadFile(PW_STASH_FILENAME);

        logger.traceExit(CLAS, "getPasswordStash");

        return resp;
    }

    /**
     * Prepares and downloads the specified file.
     * 
     * @param filename The file to download
     * @return The specified file ready for download.
     */
    private Response downloadFile(String filename) {
        logger.traceEntry(CLAS, "downloadFile");

        if (!BaseUploader.checkFileExistsDirectory(
                 ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                 SSL_KEY_FILENAME)) {
            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5083", new Object[] {
                    currentRequest.getUserPrincipal().getName(), filename });
        }

        File logFile = new File(
                 ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                 filename);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(logFile);

        logger.traceExit(CLAS, "downloadFile");

        return rb.cacheControl(cache).build();
    }

    /**
     * Deletes the SSL Key Repository files
     */
    @DELETE
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "deleteSSLKeyFiles")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response deleteSSLKeyFiles(@PathParam("domainUid") String serverInstance) {
        logger.traceEntry(CLAS, "deleteSSLKeyFiles", null);

//        // Since this method does not use AbstractIsmConfigResource.sendSetConfigRequest(),
//        // we need to check for Standby on our own
//        checkForStandbyMode(serverInstance);
        
        if (BaseUploader.checkFileExistsDirectory(
                 ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                 SSL_KEY_FILENAME)) {
            BaseUploader.deleteFileFromDirectory(
                 ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                 SSL_KEY_FILENAME);
            BaseUploader.deleteFileFromDirectory(
                 ISMWebUIProperties.instance().getServerInstallDirectory()+KEYREPOSITORY_DIRECTORY,
                 PW_STASH_FILENAME);
        }
        
        certSync(serverInstance);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(true);

        logger.traceExit(CLAS, "deleteSSLKeyFiles");
        return rb.cacheControl(cache).build();
    }

    // If harole is Primary, we need to tell the admin server to synch up the certificate
    private void certSync(String serverInstance) {
        String userId = getUserId();
        IsmConfigRequest request = new IsmConfigGetHARoleRequest(userId);
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<HARoleResponse>(){});

        if (resultObject instanceof HARoleResponse) {
            HARoleResponse harole = (HARoleResponse)resultObject;            
            if (harole.isPrimary()) {
            	request = new IsmConfigCertSyncRequest(userId);
            	request.setServerInstance(serverInstance);
                sendGetConfigRequest(request, null);
            }
        } 
    }
    
    public class SSLKeyInfo {
        @JsonProperty("dateUpdated")
        private String dateUpdated;

        @JsonProperty("sslKeyUri")
        private URI sslKeyUri;

        @JsonProperty("passwordUri")
        private URI passwordUri;

        public SSLKeyInfo(long dateUpdated, URI sslUri, URI pwUri) {
            if (dateUpdated > 0) {
                SimpleDateFormat df = new SimpleDateFormat("yyyy-MM-dd'T'HH:mmZ");
                Date date = new Date(dateUpdated);
                this.dateUpdated = df.format(date);
            } else {
                this.dateUpdated = "";
            }

            this.sslKeyUri = sslUri;
            this.passwordUri = pwUri;
        }

        /**
         * @return the date updated of the SSL Key file
         */
        @JsonProperty("dateUpdated")
        public String getDateUpdated() {
            return dateUpdated;
        }

        /**
         * @return the date updated of the SSL Key file
         */
        @JsonProperty("sslKeyUri")
        public URI getSslKeyUri() {
            return sslKeyUri;
        }

        /**
         * @return the date updated of the SSL Key file
         */
        @JsonProperty("passwordUri")
        public URI getPasswordUri() {
            return passwordUri;
        }
    }
}
