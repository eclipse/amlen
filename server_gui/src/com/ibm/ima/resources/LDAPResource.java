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

import com.ibm.ima.admin.request.IsmConfigGetLDAPConnRequest;
import com.ibm.ima.admin.request.IsmConfigSetLDAPConnRequest;
import com.ibm.ima.admin.request.IsmConfigTestLDAPConnRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.ApplianceResponse;
import com.ibm.ima.resources.data.LDAPConnection;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.resources.security.BaseUploader;
import com.ibm.ima.resources.security.CertificatesFileUploader;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;
import com.ibm.ima.ISMWebUIProperties;

@Path("/config/ldap/connections/{domainUid}")
@Produces(MediaType.APPLICATION_JSON)
public class LDAPResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = LicenseResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

	//Relative to com.ibm.ima.serverInstallDirectory (only works if stored alongside server)
	public final static String LDAP_KEYSTORE = "/certificates/LDAP";
	public final static String CERT_NAME = "ldap.pem";
	private final static String CERT_TEST_DIR = "/tmp/userfiles";

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public LDAPResource() {
        super();
    }

    @GET
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "getLDAPConnectionResponse")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getLDAPConnectionResponse(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getLDAPConnectionResponse");
        List<LDAPConnection> result = getLDAPConnections(serverInstance);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(result);

        logger.traceExit(CLAS, "getLDAPConnectionResponse", result.toString());
        return rb.cacheControl(cache).build();
    }

    @SuppressWarnings("unchecked")
    public List<LDAPConnection> getLDAPConnections(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        List<LDAPConnection> result;

        IsmConfigGetLDAPConnRequest request = new IsmConfigGetLDAPConnRequest(getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<List<LDAPConnection>>() {});
        if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse) resultObject;
            if (ismResponse.getRC().equals(Utils.NOT_FOUND_RC)) { // "not found"
                result = new ArrayList<LDAPConnection>();
            } else {
                logger.trace("", "", ismResponse.getRC());
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils().getIsmErrorMessage(
                        ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        } else {
            result = (List<LDAPConnection>) resultObject;
        }

        return result;
    }

    /**
     * Adds a (@link LDAPConnection) object to the ISM server configuration.
     * 
     * @param LDAPConnection The (@link LDAPConnection) object to add.
     * @return The URI of the added resource.
     */
    @POST
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "addLDAPConnection")
    public Response addLDAPConnection(@PathParam("domainUid") String serverInstance, LDAPConnection ldapConnection) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "addLDAPConnection", new Object[] { ldapConnection });
        ResponseBuilder response;

        List<LDAPConnection> connections = getLDAPConnections(serverInstance);
        if (connections != null && connections.size() > 1) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5071", null);
        }

        IsmConfigSetLDAPConnRequest configRequest = new IsmConfigSetLDAPConnRequest(getUserId());
        configRequest.setFieldsFromLDAPConnection(ldapConnection);
        configRequest.setServerInstance(serverInstance);

        boolean deleteCert = false;
        if ((ldapConnection.getCertificate() != null) && (!ldapConnection.getCertificate().equals(""))) {
            CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
            uploader.processCertificateWithNoKey(ldapConnection.getCertificate(), LDAPResource.CERT_NAME, 
                                                 ISMWebUIProperties.instance().getServerInstallDirectory()+LDAPResource.LDAP_KEYSTORE, 
                                                 false);
            deleteCert = true;
        }

        // validate the request
        ValidationResult validation = ldapConnection.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }
        
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse) resultObject;
            if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils().getIsmErrorMessage(
                        ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }

            if (deleteCert) {
                BaseUploader.deleteFileFromDirectory(BaseUploader.TEMP_DIR, ldapConnection.getCertificate());
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        }

        /* respond with the new resource location */
        URI location = getResourceUri(ldapConnection.getName());
        logger.trace(CLAS, "addLDAPConnection", "location: " + location.toString());
        response = Response.created(location);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addLDAPConnection", result.toString());
        return result;
    }

//    /**
//     * Updates the existing LDAP connection.
//     * 
//     * @param ldapConnection The (@link LDAPConnection) used to update the resource.
//     * @return The updated {@link LDAPConnection} object.
//     */
//    @PUT
//    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "updateQueueManagerConnection")
//    public Response updateLDAPConnection(@PathParam("domainUid") String serverInstance, LDAPConnection ldapConnection) throws IsmRuntimeException {
//    	logger.traceEntry(CLAS, "updateLDAPConnection", new Object[] { ldapConnection });
//
//    	// validate the request
//    	ValidationResult validation = ldapConnection.validate();
//    	if (!validation.isOK()) {
//    		throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
//    	}
//
//    	ApplianceResponse result = new ApplianceResponse(0, "");
//    	// if the configuration is enabled test it first.... unless we are running in Docker
//    	if (ldapConnection.getEnabled() && !Utils.getUtils().isDockerContainer()) {
//    		result = testConnection(serverInstance, ldapConnection);
//    	}
//
//
//    	if (result.getRc() == 0) {
//
//    		IsmConfigSetLDAPConnRequest configRequest = new IsmConfigSetLDAPConnRequest(getUserId());
//    		configRequest.setFieldsFromLDAPConnection(ldapConnection);
//    		configRequest.setUpdate();
//            configRequest.setServerInstance(serverInstance);
//
//    		boolean attemptUpload = true;
//    		List<LDAPConnection> conns = getLDAPConnections(serverInstance);
//    		if (!conns.isEmpty()) {
//    			LDAPConnection oldConnInfo = conns.get(0);
//    			if ((oldConnInfo.getCertificate() != null) && !(oldConnInfo.getCertificate().equals(""))) {
//    				if ((oldConnInfo.getCertificate().equals(ldapConnection.getCertificate()))
//    						&& !CertificatesFileUploader
//    						.checkCertificateExistsInTempDirectory(oldConnInfo.getCertificate())) {
//    					attemptUpload = false;
//    				}
//    			}
//    			if ((oldConnInfo.getCertificate() == null) || (oldConnInfo.getCertificate().equals(""))) {
//    				if ((ldapConnection.getCertificate() != null) && !(ldapConnection.getCertificate().equals(""))
//    						&& BaseUploader.checkFileExistsDirectory(ISMWebUIProperties.instance().getServerInstallDirectory()+LDAP_KEYSTORE, CERT_NAME)) {
//    					attemptUpload = false;
//    				}
//    			}
//    		}
//
//    		boolean deleteCert = false;
//    		if (attemptUpload) {
//    			if ((ldapConnection.getCertificate() != null) && (!ldapConnection.getCertificate().equals(""))) {
//    				CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
//    				uploader.processCertificateWithNoKey(ldapConnection.getCertificate(), LDAPResource.CERT_NAME,
//                                                                        ISMWebUIProperties.instance().getServerInstallDirectory()+ LDAPResource.LDAP_KEYSTORE, false);
//    				deleteCert = true;
//    			}
//    		}
//
//
//    		Object resultObject = sendSetConfigRequest(configRequest);
//
//    		if (resultObject instanceof IsmResponse) {
//    			IsmResponse ismResponse = (IsmResponse) resultObject;
//    			if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
//    				throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils().getIsmErrorMessage(
//    						ismResponse.getRC()), new Object[] { ismResponse.toString() });
//    			}
//
//    			if (deleteCert) {
//    				BaseUploader.deleteFileFromDirectory(BaseUploader.TEMP_DIR, ldapConnection.getCertificate());
//    			}
//    		} else {
//    			throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
//    		}
//
//    	}
//
//    	CacheControl cache = new CacheControl();
//    	cache.setMaxAge(0);
//    	cache.setNoCache(true);
//    	cache.setNoStore(true);
//    	ResponseBuilder rb = Response.ok(result);
//
//    	logger.traceExit(CLAS, "updateLDAPConnection", result.toString());
//    	return rb.cacheControl(cache).build();
//    }

    /**
     * Deletes the existing LDAP connection.
     */
    @DELETE
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator }, action = "removeLDAPConnection")
    public void removeLDAPConnection(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "removeLDAPConnection", null);

        List<LDAPConnection> conns = getLDAPConnections(serverInstance);
        if (!conns.isEmpty()) {
            IsmConfigSetLDAPConnRequest configRequest = new IsmConfigSetLDAPConnRequest(getUserId());
            configRequest.setDelete();
            configRequest.setServerInstance(serverInstance);
            Object resultObject = sendSetConfigRequest(configRequest);

            if (resultObject instanceof IsmResponse) {
                IsmResponse ismResponse = (IsmResponse) resultObject;
                if (ismResponse.getRC() != null && !ismResponse.getRC().equals("0")) {
                    String message = ismResponse.toString();
                    if (ismResponse.getRC().equals(REFERENCE_NOT_FOUND)) {
                        message = "ldapconfig";
                    }
                    throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                            .getIsmErrorMessage(ismResponse.getRC()), new Object[] { message });
                }
            } else {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(REFERENCE_NOT_FOUND), new Object[] { "ldapconfig" });
        }
        
        try {
            BaseUploader.deleteFileFromDirectory(ISMWebUIProperties.instance().getServerInstallDirectory()+LDAP_KEYSTORE,
                                                 CERT_NAME);
        } catch (Exception e) {
            // this does not warrant user attention, just log it in trace
            logger.trace(CLAS, "removeLDAPConnection", e.getMessage());
        }

        logger.traceExit(CLAS, "removeLDAPConnection");
        return;
    }
    
}
