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

package com.ibm.ima;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Properties;

import javax.servlet.http.HttpServletRequestWrapper;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpSession;
import javax.ws.rs.Consumes;
import javax.ws.rs.DELETE;
import javax.ws.rs.GET;
import javax.ws.rs.POST;
import javax.ws.rs.PUT;
import javax.ws.rs.Path;
import javax.ws.rs.Produces;
import javax.ws.rs.core.Application;
import javax.ws.rs.core.Cookie;
import javax.ws.rs.core.HttpHeaders;
import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response.Status;
import javax.ws.rs.core.UriInfo;
import javax.ws.rs.core.MultivaluedMap;

import javax.ws.rs.container.ContainerRequestContext;
import javax.ws.rs.container.ContainerRequestFilter;
import javax.ws.rs.container.ResourceInfo;
import javax.ws.rs.ext.Provider;
import javax.annotation.Priority;
import javax.ws.rs.core.Context;


import com.ibm.ima.resources.UserResource;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.security.SecurityContext;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;

//Json debug imports
import java.io.IOException;
import java.io.InputStream;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;

/**
 * Factory for ISM-specific Filters to pre-process requests before they reach the resource methods.
 */
@Provider
public class ImaRequestFilter implements ContainerRequestFilter {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    // @CLASS-COPYRIGHT@
    
    private final static String CLAS = ImaRequestFilter.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    @Context
    private ResourceInfo resinfo;

    @Context
    private HttpHeaders headers;

    @Context
    private HttpServletRequest servletRequest;

    public ImaRequestFilter() {
        logger.traceEntry(CLAS, "handleRequest", new Object[] { "ImaRequestFilter startin' up" });
    }

    //Use this filter as admin instead if trying to simplify debugging and take
    //the filter out of the list of suspects
    /*@Override
    public void filter(ContainerRequestContext requestContext) {
        logger.trace(CLAS, "filter", "targetURI: " + requestContext.getUriInfo().getPath());

        String resourceClass = resinfo.getResourceClass().getName();
        String methodName    = resinfo.getResourceMethod().getName();
        logger.trace(CLAS, "filter", "targetClass: " + resourceClass);
        logger.trace(CLAS, "filter", "targetMethod: " + methodName);

        traceIncomingJsonPayload(requestContext);

    }*/

    public void filter(ContainerRequestContext requestContext) {
        logger.trace(CLAS, "filter", "targetURI: " + requestContext.getUriInfo().getPath());

        String resourceClass = resinfo.getResourceClass().getName();
        String methodName    = resinfo.getResourceMethod().getName();
        logger.trace(CLAS, "filter", "targetClass: " + resourceClass + " Method: " + methodName);

        traceIncomingJsonPayload(requestContext);
                try {
                    // targetMethod is the method (for example, UserResource.getName)
                    // that this incoming request will be directed to.
                    logger.traceEntry(CLAS, "handleRequest", new Object[] { requestContext });
                    
                   if (requestContext.getMethod().equals("POST") && requestContext.getUriInfo().getPath().equals("login")) {
                       // allow login
                       return;
                   }

                   Method targetMethod = resinfo.getResourceMethod();
                   Class<? extends Annotation> httpMethod = getHTTPMethodFromContext(requestContext);

                   MediaType mediaType = headers.getMediaType();
                   if (mediaType == null) {
                       mediaType = MediaType.APPLICATION_JSON_TYPE;
                   }
                   
                    if (!verifySession(requestContext, mediaType)) {
                        denyAccess(requestContext);
                        return;
                    }
                                        
                    if (httpMethod != null) {
                        /* The below commented code was from JAX-RS1.0 days. I don't /think/ necessary with JAX-RS2.0
                        String requestPath = "/" + context.getAttribute(UriInfo.class).getPath(false);
                        String[] requestPathParts = requestPath.split("/");

                        // find the resource method matching the incoming path
                        for (Class<?> clazz : context.getAttribute(Application.class).getClasses()) {
                            Method fuzzyMatch = null; // match that has a variable in the path should not take priority over an exact match
                            int numVariablesInFuzzyMatch = Integer.MAX_VALUE;
                            for (Method method : clazz.getMethods()) {
                                if (!method.isAnnotationPresent(httpMethod)) {
                                    continue;
                                }
                                

                                if (method.getAnnotation(Path.class) != null) {
                                    String methodPath = clazz.getAnnotation(Path.class).value();
                                    if (!methodPath.endsWith("/")) {
                                        methodPath += "/"; 
                                    }
                                    methodPath += method.getAnnotation(Path.class).value();
                                   if (pathMatches(requestPath,methodPath,method,mediaType)) {
                                       targetMethod = method;
                                       break;
                                   }
                                   if (methodPath.contains("{")) {
                                       int count = methodPath.length() - methodPath.replace("{","").length();
                                       methodPath = adjustMethodPath(methodPath, requestPathParts);
                                       if (pathMatches(requestPath,methodPath,method,mediaType) && count < numVariablesInFuzzyMatch) {
                                           // we want the fuzzy match with the least number of variables
                                           fuzzyMatch = method;
                                       }
                                   }
                                } else if (clazz.getAnnotation(Path.class) != null) {
                                    // We need to see if the method supports a REST interface otherwise we'll pick up generic Object methods
                                    if (method.isAnnotationPresent(GET.class) || method.isAnnotationPresent(POST.class) || method.isAnnotationPresent(PUT.class) || method.isAnnotationPresent(DELETE.class)) {
                                        String methodPath = clazz.getAnnotation(Path.class).value();
                                        if (pathMatches(requestPath,methodPath,method,mediaType)) {
                                           targetMethod = method;
                                           break;
                                       }                                        
                                       if (methodPath.contains("{")) {
                                           int count = methodPath.length() - methodPath.replace("{","").length();
                                           methodPath = adjustMethodPath(methodPath, requestPathParts);
                                           if (pathMatches(requestPath,methodPath,method,mediaType) && count < numVariablesInFuzzyMatch) {
                                               // we want the fuzzy match with the least number of variables
                                               fuzzyMatch = method;
                                           }
                                       }
                                    }
                                }
                            }
                            if (targetMethod != null) {
                                break;
                            } else if (fuzzyMatch != null) {
                                targetMethod = fuzzyMatch;
                                break;
                            }
                        }*/
                       logger.trace(CLAS, "handleRequest", "targetMethod: " + targetMethod);

                        if (targetMethod != null) {
                            Permissions permissions = targetMethod.getAnnotation(Permissions.class);

                            if (permissions == null || permissions.defaultRoles().length == 0) {
                                denyAccess(requestContext);
                                return;
                            }
                            for (Role role : permissions.defaultRoles()) {
                                if (requestContext.getSecurityContext().isUserInRole(role.name())) {
                                    /*
                                     * This user is allowed access; our work here is done.
                                     * Be sure to obtain the locale for the authorized user
                                     */
                                    List<Locale> acceptableLocales = getLocalesFromContext(requestContext);
                                    SecurityContext.setContext(requestContext.getSecurityContext().getUserPrincipal(), acceptableLocales);
                                    logger.trace(CLAS, "handleRequest", "context set to " + SecurityContext.getContext().getUser());
                                    logger.trace(CLAS, "handleRequest", "user locale set to " + SecurityContext.getContext().getLocale().toString());
                                    
                                    checkForXMLHttpRequest(requestContext, targetMethod);
                                    
                                    //We are happy - return without denyaccess to allow the user in
                                    return;
                                }
                            }

                            denyAccess(requestContext);
                            return;
                        }
                    }
                    logger.log(LogLevel.LOG_ERR, CLAS, "handleRequest", "httpMethod: " + httpMethod + ", targetMethod: " + targetMethod);
                    denyAccess(requestContext);
                    return;
                } catch (IsmRuntimeException ire) {
                    throw ire;
                } catch (Throwable t) {
                    logger.log(LogLevel.LOG_WARNING, CLAS, "handleRequest", "CWLNA5000", t);                    
                    throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, t, "CWLNA5000", null);
                } finally {
                    logger.traceExit(CLAS, "handleRequest");
                }
            }

            private void traceIncomingJsonPayload(ContainerRequestContext requestContext) {
                if (requestContext.getMediaType().toString().contains("application/json")) {
                    try {
                        ByteArrayOutputStream out = new ByteArrayOutputStream();
                        InputStream in = requestContext.getEntityStream();
                        String jsonresult = null;
        
                        byte[] buffer = new byte[4096];
                        for (int len = in.read(buffer); len != -1; len = in.read(buffer)) {
                            out.write(buffer, 0, len);
                        }
        
                        byte[] requestEntity = out.toByteArray();
        
                        if (requestEntity.length == 0) {
                            jsonresult = "";
                        } else {
                            jsonresult = new String(requestEntity, "UTF-8");
                        }
                        requestContext.setEntityStream(new ByteArrayInputStream(requestEntity));
                        logger.trace(CLAS, "filter", "incoming JSON: " + jsonresult);
        
                    } catch (IOException ex) {
                        logger.trace(CLAS, "filter", "Failed to trace incoming JSON: " + ex.toString());
                    }
                } else {
                    logger.trace(CLAS, "filter", 
                           "Incoming content type not JSON (it was:  " + requestContext.getMediaType().toString()+")");
                }
            }

            /**
             * This helper substitutes any variables in the incoming methodPath with the corresponding
             * value from requestPathParts to help match an incoming requestPath with @Path annotations
             * in a method or class
             * @param methodPath  The method Path to modify
             * @param requestPathParts  The results of split("/") on the request path
             * @return modified methodPath with variables substituted from the request
             */
           private String adjustMethodPath(String methodPath, String[] requestPathParts) {
               logger.traceEntry(CLAS, "adjustMethodPath", new Object[] {methodPath, Arrays.toString(requestPathParts)});
               if (methodPath == null || !methodPath.contains("{")) {
                   return methodPath;
               }
               // substitute in actual variables
               String[] methodPathParts = methodPath.split("/");
               StringBuilder realPath = new StringBuilder();
               for (int i = 0; i < methodPathParts.length; i++) {
                   if (methodPathParts[i].isEmpty()) {
                       continue;
                   }
                   realPath.append("/");
                   if (methodPathParts[i].startsWith("{") && i < requestPathParts.length) {
                       realPath.append(requestPathParts[i]);
                   } else {
                       realPath.append(methodPathParts[i]);                                                    
                   }
               }
               methodPath = realPath.toString();
               logger.traceExit(CLAS, "adjustMethodPath", methodPath);
               return methodPath;
           }
           
           private boolean pathMatches(String requestPath, String methodPath, Method method, MediaType mediaType) {
               return (requestPath.equals(methodPath) || requestPath.equals(methodPath + "/")) && 
                       mediaTypeOK(method, mediaType);
           }
           
           private boolean mediaTypeOK (Method method, MediaType mediaType) {
               logger.traceEntry(CLAS, "doesMediaTypeMatch", new Object[] {method, mediaType});
               boolean ok = true;
               Consumes consumes = method.getAnnotation(Consumes.class);               
               if (consumes != null) {
                   String[] values = consumes.value();
                   if (values != null && values.length > 0) {
                       ok = false;                                        
                       for (String value : values) {
                           MediaType mt = MediaType.valueOf(value);
                           if (mediaType.isCompatible(mt)) {
                               ok = true;
                               break;
                           }
                       }
                   }
               }
               logger.traceExit(CLAS, "doesMediaTypeMatch", ok);
               return ok;                
           }
            
            /**
             * The headers will be used to first look for a cookie named
             * imslocale. If that cookie is present the value will be read
             * and a new Locale instance will be created and placed at the
             * front of the preferred locale list. Then the list of 
             * acceptable languages will be appended to the end.
             *  
             * @param context The MessageContext to obtain the HttpHeaders
             * @return  A List of preferred locale instances
             */
            private List<Locale> getLocalesFromContext(ContainerRequestContext requestContext) {
                
                List<Locale> acceptableLocales = new ArrayList<Locale>();            
    
                // if the headers are not null get locale info
                if (headers != null) {
                    Map<String, Cookie> cookieMap = requestContext.getCookies();

                    if (cookieMap != null) {
                        // look for the imslocale cookie
                        Cookie imaLocaleCookie = cookieMap.get("ima_locale");
                        if (imaLocaleCookie != null) {
                            String cookieLocaleValue = imaLocaleCookie.getValue();
                            if (cookieLocaleValue != null && cookieLocaleValue.length() > 0) {
                                // the imslocale cookie was found - create a Locale instance
                                String[] localeLangCntry = cookieLocaleValue.split("_");
                                if (localeLangCntry.length > 1) {
                                    acceptableLocales.add(new Locale(localeLangCntry[0], localeLangCntry[1]));
                                } else {
                                    acceptableLocales.add(new Locale(localeLangCntry[0]));
                                }
                            }
                        }
                        
                    }
                    // now append List of acceptable languages...
                    acceptableLocales.addAll(headers.getAcceptableLanguages());
                }
                
                return acceptableLocales;
                
            }
            
           private boolean verifySession(ContainerRequestContext requestContext, MediaType mediaType) {           
               if (requestContext == null || mediaType == null) {
                   return false;
               }                
               String path = requestContext.getUriInfo().getPath();
               if (path == null) {
                   return false;
               }

               Map<String, Cookie> cookies = requestContext.getCookies();
               Cookie xsrfCookie = cookies.get(UserResource.XSRF_TOKEN);
               HttpSession session = servletRequest.getSession(false);

               List<String> xRequestedWithHeader = headers.getRequestHeader("X-Requested-With");
               if (xRequestedWithHeader == null || !xRequestedWithHeader.contains("XMLHttpRequest")) {
                   // Request not via our normal xhr request mechanism... Block it except for special cases.
                   // First, ensure the xsrfCookie and session cookie exist and are consistent with each other since
                   // we do not have our custom header
                   if (session == null || xsrfCookie == null || !UserResource.getXsrfToken(session).equals(xsrfCookie.getValue())) {
                       return false;
                   }

                   // Downloading logs, MIBs, MessagingTester, license files, or mqconnectivity files are OK
                   if (requestContext.getMethod().equals("GET") && (path.startsWith("config/logs") || (path.startsWith("files/mibs/")) ||
                           path.equals("downloads/MessagingTester.zip") || path.startsWith("config/license/pdf") ||
                           (path.startsWith("config/sslKeyRepository/") && 
                           (path.endsWith("mqconnectivity.kdb") || path.endsWith("mqconnectivity.sth"))))) {
                       // ok to get the file
                       return true;
                   } 
                   logger.trace(CLAS, "verifySession", "Not via our usual xhr mechanism");
                   
                  // Uploading certificates and keys are OK since they just go to tmp dir until the config request is made
                  if (requestContext.getMethod().equals("POST") && mediaType.isCompatible(MediaType.MULTIPART_FORM_DATA_TYPE) &&
                           (path.startsWith("config/certificateProfiles")) ||
                           path.startsWith("config/sslKeyRepository") || path.startsWith("config/ltpaProfiles") || 
                           path.startsWith("config/oAuthProfiles") || path.equals("config/webui/libertyCertificate") ||
                           (path.startsWith("config/securityProfiles/") && path.endsWith("/truststore"))) {
                       return true;
                   }
                   return false;
               }
               
               List<String> xsrfHeader = headers.getRequestHeader("X-Session-Verify");                 
               if (xsrfHeader == null || xsrfHeader.isEmpty()) {
                   if (requestContext.getMethod().equals("GET") && 
                           (path.equals("config/webui/preferences/authAndPrefs") || path.equals("config/userregistry/auth"))) {
                       // need to allow this GET because cookie might not be set yet
                       return true;  
                   }                     
                   // unauthorized request!                    
                   logger.trace(CLAS, "verifySession", "No X-Session-Verify header. Method is " + requestContext.getMethod() +"; path is " + requestContext.getUriInfo().getPath());
                  return false;
               }
               
               if (session == null) {
                  // unauthorized request!                    
                  logger.trace(CLAS, "verifySession", "No session. Method is " + requestContext.getMethod() +"; path is " + requestContext.getUriInfo().getPath());
                  return false;                    
               }
               
               logger.trace(CLAS, "verifySession", xsrfHeader.toString());

               String xsrfHeaderString = xsrfHeader.get(0).trim();
               
               if (   xsrfHeaderString.length() >= 2 
                   && xsrfHeaderString.charAt(0) == '"' 
                   && xsrfHeaderString.charAt(xsrfHeaderString.length() - 1) == '"')
               {
                   xsrfHeaderString = xsrfHeaderString.substring(1, xsrfHeaderString.length() - 1);
               }
               
               if (!UserResource.getXsrfToken(session).equals(xsrfHeaderString)) {
                   if (requestContext.getMethod().equals("GET") && path.equals("config/webui/preferences/authAndPrefs")) {
                       // need to allow this GET because cookie might not be set yet
                       return true;  
                   }                     
                   // unauthorized request!                    
                   logger.trace(CLAS, "verifySession", "token we got: " + xsrfHeaderString +", token we were looking for: " + UserResource.getXsrfToken(session));
                   return false;
               }

               return true;
           }

           /**
            * For serviceability, we want to know if a REST request that should only
            * come in via xhr is coming in another way...
            * @param context  the message context of the request
            * @param targetMethod the identified target method
            */
            private void checkForXMLHttpRequest(ContainerRequestContext context, Method targetMethod) {
               try {
                   if (targetMethod.isAnnotationPresent(Produces.class) && !targetMethod.getAnnotation(Produces.class).value()[0].equals(MediaType.APPLICATION_JSON)) {
                       // don't worry about the ones that don't return json
                       if (logger.isTraceEnabled()) {
                           String produces = targetMethod.isAnnotationPresent(Produces.class) ? targetMethod.getAnnotation(Produces.class).value()[0] : "none specified";
                           logger.trace(CLAS, "checkForXMLHttpRequest", "Skipping " + targetMethod.getName() + ", @Produces: " + produces);
                       }
                       return;
                   }

                   List<String> xRequestedWith = headers.getRequestHeader("X-Requested-With");
                   if ((xRequestedWith == null || !xRequestedWith.contains("XMLHttpRequest"))) {
                       // Say what!?!  We expect to be called via Ajax, so log this oddity
                       // in case it helps debug an issue...
                       List<String> referers = headers.getRequestHeader("Referer");
                       String referer = "NA";
                       if (referers != null && referers.size() > 0) {
                           referer = referers.get(0);
                       }
                       List<String> userAgents = headers.getRequestHeader(HttpHeaders.USER_AGENT);
                       String userAgent = "NA";
                       if (userAgents != null && userAgents.size() > 0) {
                           userAgent = userAgents.get(0);
                       }
                       logger.log(LogLevel.LOG_INFO, CLAS, "checkForXMLHttpRequest", "CWLNA5105", new Object[] {targetMethod.getName(), referer, userAgent});
                   }
               } catch (Throwable t) {
                   logger.log(LogLevel.LOG_ERR, CLAS, "checkForXMLHttpRequest", "CWLNA5005", new Object[] {t.getMessage()}, t);
               }
           }

    /**
     * Retrieve the Annotation class that represents the type of REST request being
     * made (GET, POST, PUT, DELETE).
     * 
     * @param context The context of the HTTP Request
     * @return The Annotation class used by Http Method or null if the method is not
     *         one of the four types.
     */
    private Class<? extends Annotation> getHTTPMethodFromContext(ContainerRequestContext context) {
        Class<? extends Annotation> clazz = null;

        if (context.getMethod().equals("GET")) {
            clazz = GET.class;
        } else if (context.getMethod().equals("POST")) {
            clazz = POST.class;
        } else if (context.getMethod().equals("PUT")) {
            clazz = PUT.class;
        } else if (context.getMethod().equals("DELETE")) {
            clazz = DELETE.class;
        }
        return clazz;
    }

    /**
     * Modify the request context so that a permission-denied response will be sent
     * to the client. After calling this method, the handleRequest method should
     * immediately return.
    */
    private void denyAccess(ContainerRequestContext requestContext) {
        String userName = "";
        String requestPath = "";
        try {
            userName = requestContext.getSecurityContext().getUserPrincipal().getName();
            requestPath = "/" +requestContext.getUriInfo().getPath();
        } catch (Throwable t) {
            // do nothing
        }

        Object[] args = new Object[] { userName, requestPath };
        logger.log(LogLevel.LOG_WARNING, CLAS, "denyAccess", "CWLNA5001", args);
        throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5001", args);
    }
}
