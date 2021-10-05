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
import java.io.InputStream;
import java.io.InputStreamReader;
import java.net.MalformedURLException;
import java.net.URL;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

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

import com.fasterxml.jackson.databind.ObjectMapper;

import com.ibm.ima.ISMWebUIProperties;
import com.ibm.ima.resources.data.LicenseAcceptResponse;
import com.ibm.ima.resources.data.LicenseResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class LicenseResource
 */
@Path("/config/license")
@Produces(MediaType.APPLICATION_JSON)
public class LicenseResource extends AbstractIsmConfigResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = LicenseResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
//    private static final String ACCEPT_LICENSE_COMMAND = "/opt/ibm/imaserver/bin/acceptLicense.py";

    public static final int STATUS_LICENSE_VALID = 0;
    public static final int STATUS_LICENSE_INVALID_INPUT = 1;
    public static final int STATUS_LICENSE_INVALID_LANUGAGE = 2;
    public static final int STATUS_LICENSE_IO_ERROR = 3;
    public static final int STATUS_LICENSE_NOT_ACCEPTED = 4;
    public static final int STATUS_LICENSE_ACCEPTED = 5;
	
    // this must match the encoding the license files are obtained in
    public static final String LICENSE_ENCODING = "UTF-8";

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public LicenseResource() {
        super();
    }

    /**
     * Returns the content of the license in the specified language. Will check
     * that the language tag is valid and that the specified license file
     * exists.
     * 
     * @param language
     *            The language of the license file to retrieve.
     * @return A {@link LicenseResponse} containing the contents of the license
     *         file.
     */
    @GET
    @Path("{type}/{language}")
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "getLicenseContent")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getLicenseContent(@PathParam("type") String type, @PathParam("language") String language) {
        logger.traceEntry(CLAS, "getLicenseContent");

        LicenseResponse response;

        if (validateLicenseLanguage(language)) {
            InputStream in = getLicenseStream(type, language);

            if (in != null) {
                try {
                    String newline = System.getProperty("line.separator");

                    StringBuffer licenseContents = new StringBuffer();
                    BufferedReader bis = new BufferedReader(new InputStreamReader(in, LICENSE_ENCODING));
                    String curLine;
                    while ((curLine = bis.readLine()) != null) {
                        licenseContents.append(curLine);
                        licenseContents.append(newline);
                    }
                    bis.close();

                    response = new LicenseResponse(STATUS_LICENSE_VALID);
                    response.setLicenseText(licenseContents.toString());
                } catch (Exception e) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "getLicenseContent", "CWLNA5061", e);
                    response = new LicenseResponse(STATUS_LICENSE_IO_ERROR);
                }
            } else {
                response = new LicenseResponse(STATUS_LICENSE_INVALID_LANUGAGE);
            }
        } else {
            //response = new LicenseResponse(STATUS_LICENSE_INVALID_INPUT);
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5033", null );
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(response);

        logger.traceExit(CLAS, "getLicenseContent", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Accepts the specified license. Will return a
     * {@link LicenseAcceptResponse} representing the current status of the
     * license.
     * 
     * @param language
     *            The language of the license to accept.
     * @return A {@link LicenseAcceptResponse} containing the license status
     *         object.
     */
    @POST
    @Path("{language}")
    @Permissions(defaultRoles = Role.SystemAdministrator, action = "acceptLicense")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response acceptLicense(@PathParam("language") String language) {
        logger.traceEntry(CLAS, "acceptLicense");

        // needed to remediate AppScan security issues
        if (!validateLicenseLanguage(language)) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5033", null );
        }

        LicenseAcceptResponse status = (LicenseAcceptResponse) isLicenseAccepted().getEntity();
//        if (status.getStatus() != STATUS_LICENSE_ACCEPTED) {
////            LicenseResponse response = (LicenseResponse) getLicenseContent(language).getEntity();
//            LicenseResponse response = (LicenseResponse) getLicenseContent("Developers",language).getEntity();
//
//            if (response.getStatus() == STATUS_LICENSE_VALID) {
//                status = new LicenseAcceptResponse(STATUS_LICENSE_ACCEPTED);
//                status.accept(language);
//                try {
//                    ProcessBuilder pb = new ProcessBuilder(ACCEPT_LICENSE_COMMAND);
//                    pb.start();
//                    // Don't wait for process, since restart could take some time
//                } catch (Exception e) {
//                    logger.log(LogLevel.LOG_ERR, CLAS, "acceptLicense", "CWLNA5061", e);
//                }                                
//            } else {
//                status = new LicenseAcceptResponse(response.getStatus());
//            }
//        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(status);


        logger.traceExit(CLAS, "acceptLicense", status);
        return rb.cacheControl(cache).build();
    }

    /**
     * Opens the accepted.json file and creates a {@link LicenseAcceptResponse}
     * representing the current status of the license. This object contains a
     * status code to whether the license has been accepted and, if it has, the
     * logging information associated with that acceptance (Language, User role,
     * etc).
     * 
     * @return A {@link LicenseAcceptResponse} containing the license status
     *         object.
     */
    @GET
    @Permissions(defaultRoles = { Role.SystemAdministrator, Role.MessagingAdministrator, Role.User }, action = "isLicenseAccepted")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response isLicenseAccepted() {
        logger.traceEntry(CLAS, "isLicenseAccepted");

        LicenseAcceptResponse licenseAccept = getLicenseAccepted();
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(licenseAccept);

        logger.traceExit(CLAS, "isLicenseAccepted", licenseAccept);
        return rb.cacheControl(cache).build();
    }

    /**
     * Determine if the license has been accepted and whether this is a virtual appliance
     * @return LicenseAcceptResponse
     */
    public static LicenseAcceptResponse getLicenseAccepted() {
        LicenseAcceptResponse licenseAccept = new LicenseAcceptResponse(STATUS_LICENSE_IO_ERROR);
        File acceptFile = getAcceptedFile(false);
        if (acceptFile != null) {
            try {
                licenseAccept = new ObjectMapper().readValue(acceptFile, LicenseAcceptResponse.class);
            } catch (IOException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "isLicenseAccepted", "CWLNA5061", e);
            }
        } else {
            licenseAccept.setStatus(STATUS_LICENSE_NOT_ACCEPTED);
        }
        
        licenseAccept.setVirtual(Utils.getUtils().isVirtualAppliance());
        return licenseAccept;
    }

    /**
     * Creates a handle to the accepted json file. Will check whether the file
     * exists and if it does not and the createFile flag has been set, it will
     * create a dummy json file with a default value of the 'Not Accepted'
     * status code.
     * 
     * @param createFile
     *            Flag to indicate whether to create the accepted.json file
     * @return The File handle to the accepted.json file.
     */
    private static File getAcceptedFile(boolean createFile) {
        logger.traceEntry(CLAS, "getAcceptedFile");

        String acceptPath = ISMWebUIProperties.instance().getDataDir() + "/accepted.json";

        File acceptFile = new File(acceptPath);

        if (!acceptFile.exists()) {
            if (createFile) {
                try {
                    acceptFile.createNewFile();
                    acceptFile.setReadable(true, false);
                    LicenseAcceptResponse licenseAccept = new LicenseAcceptResponse(STATUS_LICENSE_NOT_ACCEPTED);
                    ObjectMapper mapper = new ObjectMapper();
                    try {
                        mapper.writeValue(acceptFile, licenseAccept);
                    } catch (IOException e) {
                        logger.log(LogLevel.LOG_ERR, CLAS, "getAcceptedFile", "CWLNA5061", e);
                        acceptFile = null;
                    }

                } catch (IOException e) {
                    logger.log(LogLevel.LOG_ERR, CLAS, "getAcceptedFile", "CWLNA5061", e);
                    acceptFile = null;
                }
            } else {
                acceptFile = null;
            }
        }

        logger.traceExit(CLAS, "getAcceptedFile", acceptFile);
        return acceptFile;
    }

    /**
     * Creates a handle to the specified license file. This method assume that
     * the language tag is valid and will only perform validation on whether the
     * resulting handle points to a file that exists.
     * 
     * @param language
     *            The language tag to open the license for.
     * @return The resulting handle to the specified langauge file.
     */
    private InputStream getLicenseStream(String type, String language) {
        logger.traceEntry(CLAS, "getLicenseStream");

        InputStream in = null;
        URL licensePath = getLicensePath(type, "LA_" + language);

        if (licensePath != null) {
            try {
                in = licensePath.openStream();
            } catch (IOException e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "getLicenseStream", "CWLNA5061", e);
            }
        }

        logger.traceExit(CLAS, "getLicenseStream", in);
        return in;
    }

    /**
     * Validates whether the input language string conforms to the language
     * pattern. Namely two single characters.
     * 
     * @param language
     *            The string to validate.
     * @return <code>true</code> if the string only contains two characters,
     *         <code>false</code> otherwise.
     */
    private boolean validateLicenseLanguage(String language) {
        logger.traceEntry(CLAS, "validateLicenseLanguage");

        boolean result = false;
        Pattern languagePattern = Pattern.compile("^..$|^.._..$", Pattern.CASE_INSENSITIVE | Pattern.DOTALL);
        Matcher languageMatcher = languagePattern.matcher(language);
        if (languageMatcher.find()) {
            result = true;
        }

        logger.traceExit(CLAS, "validateLicenseLanguage", result);
        return result;
    }

    /**
     * Gets the absolute path on the system to license file specified on the server.
     * The method checks to see if the appliance is a virtual appliance or not, and
     * uses the appropriate path. This method does not check that the resource exists.
     * 
     * @param relativePath
     *            The path to the requested resource relative to the appropriate
     *            Licenses directory.
     * @return The absolute path to the resource.
     */
    private URL getLicensePath(String type, String langPath) {
    	String licensePath = null;
    	if (type.equals("Production") || type.equals("NonProduction")) {
    		licensePath = "Licenses/Production/"; 
    	} else {
    		licensePath = "Licenses/Developer/"; 
    	}

    	return getPathOnServer(licensePath+langPath);
    }
    
	/**
     * Gets the absolute path on the system to file specified on the server,
     * relative to the root of the application on the server. This method does
     * not check that the resource exists.
     * 
     * @param relativePath
     *            The path to the requested resource relative to the root of the
     *            application WebContent.
     * @return The absolute path to the resource.
     */
    private URL getPathOnServer(String relativePath) {
        logger.traceEntry(CLAS, "getPathOnServer");

        URL url = null;

        try {
            url = currentRequest.getServletContext().getResource("/" + relativePath);
        } catch (MalformedURLException e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getPathOnServer", "CWLNA5061");
        }

        logger.traceExit(CLAS, "getPathOnServer", url);
        return url;
    }

    /**
     * Gets a specific pdf file
     */
    @GET
    @Path("pdf/{pdf}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getPdf")
    @Produces({"application/pdf"})
    public Response getPdf(@PathParam("pdf") String filename) {
        logger.traceEntry(CLAS, "getPdf");

        if (filename == null || !filename.endsWith(".pdf")) {
            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5001", new Object[] {currentRequest.getUserPrincipal().getName(), filename});
        }

        InputStream in = null;
        URL licensePath = getLicensePath("",filename);

        if (licensePath != null) {
            try {
                in = licensePath.openStream();
            } catch (IOException e) {
                throw new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5091", new Object[] {filename});
            }
        } else {
            throw new IsmRuntimeException(Status.NOT_FOUND, "CWLNA5091", new Object[] {filename});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(in);

        logger.traceExit(CLAS, "getPdf");

        return rb.cacheControl(cache).build();
    }

}
