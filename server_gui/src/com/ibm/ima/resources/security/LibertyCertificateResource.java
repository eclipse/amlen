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

import javax.ws.rs.Consumes;
import javax.ws.rs.POST;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.Context;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;

import com.ibm.websphere.jaxrs20.multipart.IMultipartBody;

import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class LibertyCertificateResource
 */
@Path("/config/webui/libertyCertificate")
public class LibertyCertificateResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = LibertyCertificateResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    public final static int LIBERTY_CERTIFICATE_RESOURCE_SUCCESS = 0;
    public final static int LIBERTY_CERTIFICATE_RESOURCE_FAILED = 1;

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    @POST
    @Consumes(MediaType.MULTIPART_FORM_DATA)
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "uploadFiles")
    public Response uploadFiles(@Context UriInfo uriInfo, IMultipartBody mpMessage) {
        logger.traceEntry(CLAS, "uploadFiles", new Object[] {});

        CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);

        String result = null;
        try {
            result = uploader.uploadFiles(uriInfo, mpMessage, null, null, null);
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
     * Adds the uploaded certificate to the keystore folder and replaces the keystore properties in Liberty config
     * 
     * @param certificateProfile The (@link CertificateProfile) object to add.
     * @return CertificateProfileResponse
     */
    @POST
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = { Role.SystemAdministrator }, action = "addLibertyCertificate")
    public Response addLibertyCertificate(LibertyCertificate libertyCertificate) {

        logger.traceEntry(CLAS, "addLibertyCertificate", new Object[] { libertyCertificate });

        if (libertyCertificate == null) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5012", new Object[] { libertyCertificate });
        }

        // we need to make sure the uploaded certificates are supported
        CertificatesFileUploader uploader = new CertificatesFileUploader(currentRequest);
        uploader.processLibertyCertificate(libertyCertificate);

        libertyCertificate.setExpirationDate(Utils.getUtils().libertyPropertyAccessRequest(Utils.GET_LIBERTY_PROPERTY,
                Utils.KEYSTORE_EXP, null));

        ResponseBuilder response = Response.ok(libertyCertificate);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "addLibertyCertificate", result.toString());
        return result;

    }
}
