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

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.HashMap;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.ResourceBundle;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import javax.servlet.http.HttpServletRequest;
import javax.ws.rs.core.CacheControl;
import javax.ws.rs.core.MultivaluedMap;
import javax.ws.rs.core.Response;
import javax.ws.rs.core.Response.ResponseBuilder;
import javax.ws.rs.core.Response.Status;

import com.ibm.websphere.jaxrs20.multipart.IAttachment;
import com.fasterxml.jackson.databind.ObjectMapper;

import com.ibm.ima.resources.data.ExceptionResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.SecurityContext;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

/**
 * Base class for File resources.
 * 
 * 
 */
public abstract class AbstractFileUploader {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = AbstractFileUploader.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    // Semicolon between 0 or more whitespace chars
    private static final Pattern SEMICOLON_PATTERN = Pattern.compile("\\s*+;\\s*+");

    // All non-whitespace chars between 0 or more whitespace chars
    private static final Pattern NON_WHITESPACE_PATTERN = Pattern.compile("\\s*+(\\S*)\\s*+");

    // All non-quote chars between exactly one double-quote char
    private static final Pattern BETWEEN_QUOTES_PATTERN = Pattern.compile("\"([^\"]*)\"");

    // Client type
    enum CLIENT_TYPE {
        HTML5,
        HTML,
        FLASH
    };

    protected CLIENT_TYPE clientType = CLIENT_TYPE.HTML;
    private HttpServletRequest currentRequest;


    public AbstractFileUploader(HttpServletRequest currentRequest) {
        this.currentRequest = currentRequest;
    }


    /**
     * Attempts to upload a file from the specified part. If a file is found, stores
     * that file in the targetDir directory using the filename (or name) found in the
     * Content-Disposition header for the part. The File created is returned.
     * @param part  The part to upload the file from
     * @param targetDir The directory to save the file to
     * @return The File created, or null if no file found to upload
     */
    public File uploadFileFromPart(IAttachment part, String targetDir) throws IOException {
        logger.traceEntry(CLAS, "uploadFileFromPart", new Object[]{part, targetDir});

        File file = null;

        MultivaluedMap<String, String> headers = part.getHeaders();


        if (headers.containsKey("Content-Disposition")) {
            Map<String,String> contentDispositionHeaderMap = parseContentDispositionHeader(headers.get("Content-Disposition").get(0));
            clientType = guessClientType(contentDispositionHeaderMap, currentRequest);
            String filename = getFilename(contentDispositionHeaderMap);
                // write the file
                file = Utils.safeFileCreate(targetDir, filename, true);
                if (file != null) {
                    writeToFile(part.getDataHandler().getInputStream(), file);
                    // make sure the file is not empty
                    if (file.length() == 0) {
                        file.delete();
                        throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5024", null); 
                    }
                }
        }

        logger.traceExit(CLAS, "uploadFileFromPart", file);

        return file;
    }

    /**
     * @return the clientType
     */
    protected CLIENT_TYPE getClientType() {
        return clientType;
    }


    /**
     * @param clientType the clientType to set
     */
    protected void setClientType(CLIENT_TYPE clientType) {
        this.clientType = clientType;
    }


    /**
     * Write an InputStream to a local file
     * 
     * @param stream contains the file
     * @param file the file to write it to
     * @throws IOException
     */
    protected static final void writeToFile(InputStream stream, File file) throws IOException {
        logger.traceEntry(CLAS, "writeToFile");

        BufferedInputStream in = null;
        BufferedOutputStream out = null;

        try {
            in = new BufferedInputStream(stream);
            out = new BufferedOutputStream(new FileOutputStream(file, false));

            byte[] buffer = new byte[256];
            int n = 0;
            while ((n = in.read(buffer)) > 0) {
                out.write(buffer, 0,  n);
            }
            out.flush();

        } finally {
            if (in != null) {
                try {
                    in.close();
                } catch (IOException e) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "writeToFile", "CWLNA5007", new Object[] {in}, e);
                }
            }
            if (out != null) {
                try {
                    out.close();
                } catch (IOException e) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "writeToFile", "CWLNA5007", new Object[] {out}, e);
                }
            }
        }
        logger.traceExit(CLAS, "writeToFile");
        return;
    }

    /**
     * Formats the response based on the clientType.
     * @param files The array to use in the format of the response. An array of JSONObjects,
     *                  each with a "uri" and "name" for a single file that was uploaded.
     * @return The response
     */
    protected String formatResponse(List<String> files) {
        logger.traceEntry(CLAS, "formatResponse", new Object[]{files});
        String result;

        if (files == null || files.isEmpty()) {
            logger.log(LogLevel.LOG_ERR, CLAS, "formatResponse", "CWLNA5024");
            IsmRuntimeException ire = new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5024", null); 
            if (isIE9()) {
                return buildTextAreaExceptionString(ire);
            } else {
                throw ire;
            }
        }

        CLIENT_TYPE clientType = getClientType();

        StringBuilder sb = new StringBuilder();

        if (clientType == CLIENT_TYPE.HTML) {
            sb.append("<textarea>");
        }

        sb.append("{\"files\":[");
        for (String file : files) {
            sb.append("\"");
            sb.append(file);
            sb.append("\",");
        }
        sb.setLength(sb.length() - 1);
        sb.append("]}");

        if (clientType == CLIENT_TYPE.HTML) {
            sb.append("</textarea>");
        }


        result = sb.toString();
        return result;
    }

    /**
     * Parses a string containing key/value pairs to extract the Content-Disposition header,
     * name, and filename, if they are found.
     * 
     * @param contentDispositionHeader key/value pairs contained in a String
     * @return a Map of the key/value pairs
     */
    protected static final Map<String, String> parseContentDispositionHeader(String contentDispositionHeader) {

        logger.traceEntry(CLAS, "parseContentDispositionHeader", new Object[] { contentDispositionHeader });

        String[] all = SEMICOLON_PATTERN.split(contentDispositionHeader);
        int i = 0;
        Matcher m = (NON_WHITESPACE_PATTERN).matcher(all[i]);
        String headerType = "";
        if (m.find()) {
            headerType = m.group(1);
        }

        Map<String, String> results = new HashMap<String, String>();

        results.put("Content-Disposition", headerType);

        for (i = 1; i < all.length; i++) {
            if (all[i].startsWith("name")) {
                m = (BETWEEN_QUOTES_PATTERN).matcher(all[i]);
                if (m.find()) {
                    results.put("name", m.group(1));
                }
            } else if (all[i].startsWith("filename")) {
                m = (BETWEEN_QUOTES_PATTERN).matcher(all[i]);
                if (m.find()) {
                    results.put("filename", m.group(1));
                }
            }
        }

        logger.traceExit(CLAS, "parseContentDispositionHeader", results);
        return results;
    }

    /**
     * Try to guess the client type. Defaults to CLIENT_TYPE.HTML.
     * 
     * @param contentDispositionHeaderMap The parsed parameters in the Content-Disposition request header.
     * @return the CLIENT_TYPE implied by the Content-Disposition request header.
     * 
     */
    protected final static CLIENT_TYPE guessClientType(Map<String, String> contentDispositionHeaderMap, HttpServletRequest request) {
        logger.traceEntry(CLAS, "guessClientType", new Object[] {contentDispositionHeaderMap});

        CLIENT_TYPE clientType = CLIENT_TYPE.HTML;
        String name = contentDispositionHeaderMap.get("name");
        logger.trace(CLAS, "guessCLientType", "name is " + name);

        if (name == null) {
            return clientType;
        }

        if (name.equals("uploadedfilesFlash")) {
            clientType = CLIENT_TYPE.FLASH;
        } else if (name.endsWith("[]")) {
            clientType = CLIENT_TYPE.HTML5;
        } else {
            // make sure the browser is really IE.  Dojo 1.8 is not sending HTML5 clients with [] at the moment.
            String userAgent = request.getHeader("user-agent");
            if (userAgent != null && userAgent.indexOf("MSIE 9") < 0) {
                clientType = CLIENT_TYPE.HTML5;
            }
        }

        logger.traceExit(CLAS, "guessClientType", clientType);

        return clientType;
    }


    /**
     * Get the filename of the upload file from the Content-Disposition Header. If
     * there is no filename, returns null.  If there is an empty filename, returns name.
     * Returns just the filename and no portion of the path.
     * @param contentDispositionHeaderMap
     * @return filename or null if none found
     */
    protected final static String getFilename(Map<String, String> contentDispositionHeaderMap) {
        logger.traceEntry(CLAS, "getFilename", new Object[] {contentDispositionHeaderMap});

        String filename = contentDispositionHeaderMap.get("filename");

        if (filename != null && filename.trim().length() > 0) {
            filename = filename.replace('\\', '/');
            filename = filename.contains("/") ? filename.substring(filename.lastIndexOf("/") + 1, filename.length()) : filename;
        }

        logger.traceExit(CLAS, "getFilename", filename);

        return filename;
    }
    
    public boolean isIE9() {
        String userAgent = currentRequest.getHeader("user-agent");
        logger.trace(CLAS, "isIE9", "userAgent: " + userAgent);
        if (userAgent != null && userAgent.indexOf("MSIE 9") >= 0) {
            return true;
        }
        return false;
    }

    /**
     * If clientType is HTML, it requires errors wrapped in <textarea>. Build a response
     * for based on the specified exception wrapped in a textarea.
     * @param t exception to wrap
     * @return response
     */
    public Response buildTextAreaExceptionResponse(Throwable t) {
        String result = buildTextAreaExceptionString(t);
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb = Response.ok(result);

        return rb.cacheControl(cache).build();
    }


    private String buildTextAreaExceptionString(Throwable t) {
    	Locale locale = getClientLocaleFromContext();
    	ResourceBundle bundle = ResourceBundle.getBundle("com.ibm.ima.msgcatalog.IsmUIStringsResourceBundle", locale);
        String result = "<textarea>" + bundle.getString("ERROR_OCCURRED") + "</textarea>";
        if (t != null) {
            StringBuilder sb = new StringBuilder("<textarea>");
            try {
                ExceptionResponse er = new ExceptionResponse(t, locale);
                ObjectMapper mapper = new ObjectMapper();
                sb.append(mapper.writeValueAsString(er));
            } catch (Exception e) {
                logger.trace(CLAS, "buildTextAreaExceptionResponse", e.getMessage());
            }           
            sb.append("</textarea>");
            result = sb.toString();
        }
        return result;
    }
    
    /**
     * Obtain the locale requested by the client. This method
     * retrieves the locale directly from the SecurityContext
     * instance for the given thread. 
     *  
     * @return The locale that best matches that requested
     *         by the client.
     */
    protected Locale getClientLocaleFromContext() {
    	Locale locale = null;
        SecurityContext context = SecurityContext.getContext();
        if (context != null) {
            locale = SecurityContext.getContext().getLocale();
        } else {
        	locale = Locale.ENGLISH;
        }
        return locale;
    }


}
