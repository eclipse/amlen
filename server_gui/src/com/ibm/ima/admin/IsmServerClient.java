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

package com.ibm.ima.admin;

import java.io.IOException;
import java.util.HashMap;
import java.util.Locale;
import java.util.Set;

import javax.ws.rs.core.Response.Status;

import com.fasterxml.jackson.databind.ObjectMapper;

import com.ibm.ima.admin.request.IsmBaseRequest;
import com.ibm.ima.admin.request.IsmConfigRequest;
import com.ibm.ima.admin.request.IsmMonitoringRequest;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger;
//import com.ibm.ima.util.IsmLogger.LogLevel;


public class IsmServerClient {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = IsmServerClient.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    private final static long TIMEOUT_MILLIS = 30 * 1000; // 30 seconds

    private String ip = "127.0.0.1";
    private String path = "/";
//    private IsmWSConnection wsClient = null;
    private IsmClientType type;


    @Deprecated 
    public IsmServerClient(IsmClientType type, Locale locale) {
        this.type = type;        
        int port = type.getPort();
        
        /*
         * Get Server IP and Port from Environment variable
         * NOTE: This code is temporarily added to continue prototype
         *       work for WebUI docker image
         */
        String portstr = System.getenv("IMA_SERVER_PORT");
        if (portstr != null) {
            port = Integer.parseInt(portstr);
        }
        String ipstr = System.getenv("IMA_SERVER_HOST");
        if (ipstr != null) {
            ip = ipstr;
        }

//        wsClient = new IsmWSConnection(ip, port, path, type.getProtocol(), locale);
//
//        wsClient.setVerbose(logger.isTraceEnabled());
    }

    /**
     * Version that supports managing multiple remote servers
     * @param request
     * @param locale
     */
    public IsmServerClient(IsmBaseRequest request, Locale locale) {

        this.type = request.getClientType();        
        int port = request.getPort();
        ip = request.getServerIPAddress();
        
        if (logger.isTraceEnabled()) {
        	logger.trace(CLAS, "ctor", "Creating client with type " + type +", server ip address " + ip + ", and port " + port);
        }
        
//        wsClient = new IsmWSConnection(ip, port, path, type.getProtocol(), locale);
//
//        wsClient.setVerbose(logger.isTraceEnabled());
    }

    /*
     * Send configuration message to ISM server
     */
    public String sendConfigMessage(IsmConfigRequest configRequest) {
        String result = "";

//        // Connect to ISM server
//        try {
//            wsClient.connect();
//        } catch (IOException e) {
//            // Get reason code from exception and add to return result
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5035", new Object[] {type.getServerName(), e.getMessage()});
//        }

        // Send set configuration request
        try {
            ObjectMapper mapper = new ObjectMapper();
            logger.trace(CLAS, "sendConfigMessage", "configRequest: " + configRequest.toString());
            String json = mapper.writeValueAsString(configRequest);
//            if (wsClient.verbose && configRequest.hasMask()) {
//                wsClient.send(json, configRequest.applyMask(json));
//            } else {
//                wsClient.send(json);
//            }
        } catch (IOException e) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5036", new Object[] {type.getServerName(), e.getMessage()});
        }

//        // Get results back from ISM server
//        try {
//            IsmWSMessage rcvMessage = wsClient.receive(TIMEOUT_MILLIS);
//            result = rcvMessage.getPayload();
//        } catch (IOException e) {
//            Object[] args = new Object[] {type.getServerName(), e.getMessage()};
//            logger.log(LogLevel.LOG_WARNING, CLAS, "sendConfigMessage", "CWLNA5037", args);
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5037", args);
//        } catch (InterruptedException ie) {
//            logger.log(LogLevel.LOG_WARNING, CLAS, "sendConfigMessage", "CWLNA5118");
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5118", null);
//        }

        // Disconnect from ISM server
        finally {            
//            try {
//                logger.trace(CLAS, "sendConfigMessage", "Disconnecting...");
//                wsClient.disconnect();
//            } catch (IOException e1) {
//                logger.log(LogLevel.LOG_WARNING, CLAS, "sendConfigMessage", "CWLNA5000", e1);
//            }
        }
        return result;
    }

    /*
     * Send Monitoring message to ISM server
     */
    public String sendMonitoringMessage(IsmMonitoringRequest monitoringRequest) {
        String result = "";

//        // Connect to ISM server
//        try {
//            wsClient.connect();
//        } catch (IOException e) {
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5035", new Object[] {type.getServerName(), e.getMessage()});
//        }
//
//        // Send set configuration request
//        try {
//            ObjectMapper mapper = new ObjectMapper();
//            String json = mapper.writeValueAsString(monitoringRequest);
//            wsClient.send(json);
//        } catch (IOException e) {
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5036", new Object[] {type.getServerName(), e.getMessage()});
//        }

//        // Get results back from ISM server
//        try {
//            IsmWSMessage rcvMessage = wsClient.receive(TIMEOUT_MILLIS);
//            result = rcvMessage.getPayload();
//        } catch (IOException e) {
//            Object[] args = new Object[] {type.getServerName(), e.getMessage()};
//            logger.log(LogLevel.LOG_WARNING, CLAS, "sendMonitoringMessage", "CWLNA5037", args);            
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5037", args);
//        } catch (InterruptedException ie) {
//            logger.log(LogLevel.LOG_WARNING, CLAS, "sendMonitoringMessage", "CWLNA5118");            
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5118", null);
//        }
//        finally {
//	        // Disconnect from ISM server
//	        try {
//                logger.trace(CLAS, "sendMonitoringMessage", "Disconnecting... ");	            
//	            wsClient.disconnect();
//	        } catch (IOException e1) {
//	            // Log exception
//	            logger.log(LogLevel.LOG_WARNING, CLAS, "sendMonitoringMessage", "CWLNA5000", e1);
//	        }
//        }
        return result;
    }
    
    /*
     * Send multiple Monitoring messages to ISM server
     */
    public HashMap<String, String> sendMonitoringMessages(HashMap<String, IsmMonitoringRequest> monitoringRequests) {
    	if (monitoringRequests == null) {
    		return null;
    	}
    	
//        // Connect to ISM server
//        try {
//            wsClient.connect();
//        } catch (IOException e) {
//            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5035", new Object[] {type.getServerName(), e.getMessage()});
//        } 

    	HashMap<String, String> results = new HashMap<String, String>();
        try {
        	ObjectMapper mapper = new ObjectMapper();
        	Set<String> keys = monitoringRequests.keySet();
        	for (String key : keys) {
//        		// Send set configuration request
//        		try {
//        			String json = mapper.writeValueAsString(monitoringRequests.get(key));
//        			logger.trace(CLAS, "sendMonitoringMessages", "sending " + json + " for key " + key);
//        			wsClient.send(json);
//        		} catch (IOException e) {
//        			throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5036", new Object[] {type.getServerName(), e.getMessage()});
//        		}

//        		// Get results back from ISM server
//        		try {
//        			IsmWSMessage rcvMessage = wsClient.receive(TIMEOUT_MILLIS);
//        			String result = rcvMessage.getPayload();
//        			logger.trace(CLAS, "sendMonitoringMessages", "received " + result);
//        			results.put(key, result);
//                } catch (IOException e) {
//                    Object[] args = new Object[] {type.getServerName(), e.getMessage()};
//                    logger.log(LogLevel.LOG_WARNING, CLAS, "sendMonitoringMessages", "CWLNA5037", args);            
//                    throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5037", args);
//                } catch (InterruptedException ie) {
//                    logger.log(LogLevel.LOG_WARNING, CLAS, "sendMonitoringMessages", "CWLNA5118");            
//                    throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5118", null);
//                }
//        		wsClient.reset();
        	} 

        	// Disconnect from ISM server
        } finally {
//        	try {
//                logger.trace(CLAS, "sendMonitoringMessages", "Disconnecting... ");             
//        		wsClient.disconnect();
//        	} catch (IOException e1) {
//        		logger.log(LogLevel.LOG_WARNING, CLAS, "sendMonitoringMessages", "CWLNA5000", e1);
//        	}
        }

        return results;
    }

}
