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

import com.fasterxml.jackson.core.type.TypeReference;

import com.ibm.ima.ISMWebUIProperties;
import com.ibm.ima.admin.request.IsmConfigGetEnableDiskPersistenceRequest;
import com.ibm.ima.admin.request.IsmConfigGetVersionRequest;
import com.ibm.ima.admin.request.IsmConfigMQStatusRequest;
import com.ibm.ima.admin.request.IsmConfigRequest;
import com.ibm.ima.admin.request.IsmConfigSetEnableDiskPersistenceRequest;
import com.ibm.ima.admin.request.IsmConfigSetRunModeRequest;
import com.ibm.ima.admin.request.IsmConfigStartStopRequest;
import com.ibm.ima.admin.request.IsmConfigStatusRequest;
import com.ibm.ima.admin.response.IsmResponse;
import com.ibm.ima.resources.data.AbstractIsmConfigObject.ValidationResult;
import com.ibm.ima.resources.data.ApplianceResponse;
import com.ibm.ima.resources.data.EnableDiskPersistence;
import com.ibm.ima.resources.data.MQConnectivityResponse;
import com.ibm.ima.resources.data.RunMode;
import com.ibm.ima.resources.data.ServerStatusResponse;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;
import com.ibm.json.java.JSONArray;
import com.ibm.json.java.JSONObject;

/**
 * Servlet implementation class ApplianceResource
 */
@Path("/config/appliance")  // TODO support serverInstance for some APIs
@Produces(MediaType.APPLICATION_JSON)
public class ApplianceResource extends AbstractIsmConfigResource{

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    public static final int SERVER_STOPPED = 99;
    public static final int UNKNOWN = -1;
    public static final int APPLIANCE_REQUEST_FAILED = 1001;
    public static final int APPLIANCE_REQUEST_SUCCESSFUL = 0;

    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    private final static String CLAS = ApplianceResource.class.getCanonicalName();

    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server
    private final static String FORCE_STOP_COMMAND = "/bin/forcestopserver.sh";
    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server
    private final static String START_SERVER_COMMAND = "/bin/startserver.sh";
    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server (incl. mqcbridge)
    private final static String START_MQCONNECTIVITY_COMMAND = "/bin/startmqservice.sh";
    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server (incl. mqcbridge)
    private final static String STOP_MQCONNECTIVITY_COMMAND = "/bin/forcestopmqc.sh";

    //Relative to com.ibm.ima.serverBinDirectory (only works if installed with server
    private final static String CLEAN_STORE_CHECK_COMMAND = "/bin/cleanstorelockfilecheck.sh";

    private final static String VERSION = ISMWebUIProperties.instance().getVersion();

    private final static String IMASERVER_PROCESS = "ismserver";
    private final static String MQCONNECTIVITY_PROCESS = "mqconnectivity";

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public ApplianceResource() {
        logger.traceExit(CLAS, "ApplianceResource");
    }

    /**
     * The strings coming back from a request for ismserver status
     */
    public enum StatusResults {
        Initializing,
        Running,
        Stopping,
        Initialized,
        TransportStarted,
        ProtocolStarted,
        StoreStarted,
        EngineStarted,
        MessagingStarted,
        Maintenance,
        Standby,
        StoreStarting,
        CleanStoreInProgress
    }

    /**
     * Get the information returned by show version
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the firmware version
     * 
     */
    @GET
    @Path("version/{domainUid}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getVersion")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getVersion(@PathParam("domainUid") String serverInstance) {
        logger.traceEntry(CLAS, "getVersion");
        ApplianceResponse response = new ApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, VERSION);
        IsmConfigGetVersionRequest request = new IsmConfigGetVersionRequest(getUserId());
        request.setServerInstance(serverInstance);
        
        Object resultObject = sendGetConfigRequest(request, new TypeReference<String>(){});
        if (resultObject instanceof String) {
            response.setApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, resultObject.toString());
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        }    

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getVersion", response);
        return rb.cacheControl(cache).build();
    }

    
    /**
     * Get the firmware version
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the firmware version
     * 
     */
    @GET
    @Path("firmware")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator}, action = "getFirmwareVersion")
    @Produces({ MediaType.APPLICATION_JSON })
    public Response getFirmwareVersion() {
        logger.traceEntry(CLAS, "getFirmwareVersion");
        ApplianceResponse response = new ApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, VERSION);

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getFirmwareVersion", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Get ISM process current status
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the requested ISM process current status
     */
    @GET
    @Path("status/ismserver")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getISMServerStatus")
    public Response getISMServerStatus() throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getISMServerStatus");
        
        ServerStatusResponse response = _getServerStatus(2);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getISMServerStatus", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Get ISM process current status
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the requested ISM process current status
     */
    @GET
    @Path("status/ismserver/{domainUid}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getISMServerStatusDomain")
    public Response getISMServerStatusDomain(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "getISMServerStatus");        
        
        ServerStatusResponse response = null;
        IsmConfigStatusRequest request = new IsmConfigStatusRequest(IMASERVER_PROCESS, getUserId());
        request.setServerInstance(serverInstance);
        
        // if it's the local host, call the more robust method
        if (request.isLocalServer()) {
        	return getISMServerStatus();
        }
        
        Object resultObject = sendGetConfigRequest(request, new TypeReference<ServerStatusResponse>(){});
        if (resultObject instanceof ServerStatusResponse) {
        	response = (ServerStatusResponse)resultObject;
        	response.setHARole(response.getNewRole());
        } else if (resultObject instanceof IsmResponse) {
        	IsmResponse ismResponse = (IsmResponse)resultObject;
        	throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
        			.getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        } else if (resultObject instanceof String) {
        	throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        }

        if (response == null) {
        	response = new ServerStatusResponse();
        	response.setRC(UNKNOWN);
        	response.setServerState(""+response.getRC());
        }
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getISMServerStatus", response);
        return rb.cacheControl(cache).build();
    }
    
    private ServerStatusResponse _getServerStatus(int retries) {
        ServerStatusResponse response = new ServerStatusResponse();
        IsmConfigStatusRequest request = new IsmConfigStatusRequest(IMASERVER_PROCESS, getUserId());
        Object resultObject = null;
        try {
            resultObject = sendGetConfigRequest(request, new TypeReference<ServerStatusResponse>(){});
        } catch (IsmRuntimeException ire) {
        	response.setRC(UNKNOWN);
        	response.setServerState(""+response.getRC());
            response.setResponse(ire.getMessage());
            try {
                String[] output = new String[2];
                int rc = -1;
                ProcessBuilder pb = new ProcessBuilder(
                                      ISMWebUIProperties.instance().getServerInstallDirectory()+CLEAN_STORE_CHECK_COMMAND);
                Process process = pb.start();
                rc = Utils.getUtils().getOutput(process, output);
                if (logger.isTraceEnabled()) {
                	logger.trace(CLAS, "getISMServerStatus", 
                                      ISMWebUIProperties.instance().getServerInstallDirectory()+CLEAN_STORE_CHECK_COMMAND 
                                      + " rc=" + rc + "; out="+output[0]+"; err="+output[1]);
                }
                switch (rc) {
                case 0: // server is running??? Try getting status again if retries > 0
                    if (retries > 0) {
                        retries--;
                        logger.trace(CLAS, "getISMServerStatus", "retry with retries = " + retries + " since" + 
                                          ISMWebUIProperties.instance().getServerInstallDirectory()+CLEAN_STORE_CHECK_COMMAND
                                           + " returns 0");
                        return _getServerStatus(retries);
                    }
                    // leave it unknown
                    break;
                case 1: // CLEAN_STORE is happening
                	response.setRC(StatusResults.CleanStoreInProgress.ordinal());
                	response.setResponse(StatusResults.CleanStoreInProgress.name());
                	break;
                case 2: // server is really stopped
                    response.setRC(SERVER_STOPPED);
                    break;                	
                case 3: // STORE_STARTING
                    response.setRC(StatusResults.StoreStarting.ordinal());
                    response.setResponse(StatusResults.StoreStarting.name());
                    break;
                default: // all others mean we really don't know the state
                	break;
                }
            	response.setServerState(""+response.getRC());
            } catch (Exception e) {
                logger.log(LogLevel.LOG_ERR, CLAS, "getISMServerStatus", "CWLNA5063", e);
            }           
        }
        if (resultObject instanceof ServerStatusResponse) {
            response = (ServerStatusResponse)resultObject;
            response.setHARole(response.getNewRole());
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        } else if (resultObject instanceof String) {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] { resultObject });
        }
        return response;
    }

    /**
     * Check MQConnectivity service status
     * 
     * @return A {@link ApplianceResponse} containing the status of the MQ Connectivity Service
     */
    @GET
    @Path("status/mqconnectivity")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getMQConnectivityStatus")
    public Response getMQConnectivityStatus() throws IsmRuntimeException {
    	
    	logger.traceEntry(CLAS, "getMQConnectivityStatus");
    	
        MQConnectivityResponse response = new MQConnectivityResponse();
        IsmConfigRequest request = new IsmConfigMQStatusRequest(MQCONNECTIVITY_PROCESS, getUserId());
        
        try {
        	// if we can talk to the service it is running
            sendGetConfigRequest(request, new TypeReference<ServerStatusResponse>(){});
           	response.setStatus(MQConnectivityResponse.STARTED);
        } catch (IsmRuntimeException e) {
           	response.setStatus(MQConnectivityResponse.STOPPED);
        }
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        logger.traceExit(CLAS, "getMQConnectivityStatus", response);
        return rb.cacheControl(cache).build();
    	
    }
    
    /**
     * Stop (potentially) remote Amlen server
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the requested ISM process current status
     */
    @PUT
    @Path("imaserver/{domainUid}/stop")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "stopImaServer")     
    public Response stopImaServer(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
    	ApplianceResponse response = new ApplianceResponse(APPLIANCE_REQUEST_FAILED, null);
        IsmConfigRequest request = new IsmConfigStartStopRequest("stop", IMASERVER_PROCESS, getUserId());
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<String>(){});
        if (resultObject instanceof String) {
            response.setApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, resultObject.toString());
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        return rb.cacheControl(cache).build();        
    }

    /**
     * Restart (potentially) remote Amlen server
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the requested ISM process current status
     */
    @PUT
    @Path("imaserver/{domainUid}/restart")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "restartImaServer")     
    public Response restartImaServer(@PathParam("domainUid") String serverInstance) throws IsmRuntimeException {
    	ApplianceResponse response = new ApplianceResponse(APPLIANCE_REQUEST_FAILED, null);
    	IsmConfigStartStopRequest request = new IsmConfigStartStopRequest("stop", IMASERVER_PROCESS, getUserId());
        request.setRestart(true);
        request.setServerInstance(serverInstance);
        Object resultObject = sendGetConfigRequest(request, new TypeReference<String>(){});
        if (resultObject instanceof String) {
            response.setApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, resultObject.toString());
        } else if (resultObject instanceof IsmResponse) {
            IsmResponse ismResponse = (IsmResponse)resultObject;
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                    .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
        }
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(response);

        return rb.cacheControl(cache).build();        
    }


    /**
     * Start or stop ISM process
     * 
     * @param processName The name field of ISM process
     * @return A {@link ApplianceResponse} containing operation rc and the requested ISM process current status
     */
    @PUT
    @Path("{action}/{processName}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "startStopProcess")
    public Response startStopProcess(@PathParam("action") String action, @PathParam("processName") String processName) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "startStopProcess", new Object[]{action,processName});

        String action1 = action.toLowerCase();

        if (!action1.equals("start") && !action1.equals("stop") && !action1.equals("restart")) {
            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5001", new Object[] {userId, action});
        }

        if (!hasPermission(processName)) { 
            throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5001", new Object[] {userId, processName});
        }

        if (action1.equals("start")) {
        	if (processName.equals(IMASERVER_PROCESS)) {
        		return runCommandWrapper(
                                ISMWebUIProperties.instance().getServerInstallDirectory()+START_SERVER_COMMAND,
                                action, null);
        	} else {
        		return runCommandWrapper(
                                ISMWebUIProperties.instance().getServerInstallDirectory()+START_MQCONNECTIVITY_COMMAND,
                                action, null);
        	}
        } 

        ApplianceResponse response = new ApplianceResponse(APPLIANCE_REQUEST_FAILED, null);
        
        if (action1.equals("restart")) {
            // restart is for the webui only right now
            if (processName.equalsIgnoreCase("webui")) {
               // wait a couple of seconds then restart. The reason we wait is so that
               // the UI can load the login page before the server goes down
               Utils.restartApplication(2000);
               response.setApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, "");
            } else {
                throw new IsmRuntimeException(Status.FORBIDDEN, "CWLNA5001", new Object[] {userId, processName});                
            }
        } else if (action1.equals("stop") && processName.equals(MQCONNECTIVITY_PROCESS)) {
        	
        	// stop of mqconnectivity requested....
        	return runCommandWrapper(
                        ISMWebUIProperties.instance().getServerInstallDirectory()+STOP_MQCONNECTIVITY_COMMAND,
                        action, null);
        	
        } else {
            IsmConfigRequest request = new IsmConfigStartStopRequest(action1, processName, getUserId());
            Object resultObject = sendGetConfigRequest(request, new TypeReference<String>(){});
            if (resultObject instanceof String) {
                response.setApplianceResponse(APPLIANCE_REQUEST_SUCCESSFUL, resultObject.toString());
            } else if (resultObject instanceof IsmResponse) {
                IsmResponse ismResponse = (IsmResponse)resultObject;
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(ismResponse.getRC()), new Object[] { ismResponse.toString() });
            }
        }

        ResponseBuilder rb =  Response.ok(response);
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);

        logger.traceExit(CLAS, "startStopProcess", response);
        return rb.cacheControl(cache).build();
    }

    /**
     * Force the stop of the ISM Server
     * 
     * @return A {@link ApplianceResponse} containing operation rc and the result of running the command
     */
    @PUT
    @Path("forceStop")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "forceStopIsmServer")
    public Response forceStopIsmServer() throws IsmRuntimeException {
        logger.traceEntry(CLAS, "forceStopIsmServer");

        return runCommandWrapper(
                  ISMWebUIProperties.instance().getServerInstallDirectory()+FORCE_STOP_COMMAND,
                  "forceStop", null);
    }

    /**
     * Right now, the user only has permission to start or stop ismserver
     * @param processName
     * @return true if the user has permission to start or stop this process
     */
    private boolean hasPermission(String processName) {
        if (processName != null && (processName.equalsIgnoreCase("ismserver") || processName.equalsIgnoreCase("webui") || processName.equalsIgnoreCase("mqconnectivity"))) {
            return true;
        }
        return false;
    }
    
    /**
     * Request that the runmode of the server be changed. The request will throw
     * an IsmRuntimeException if the requested mode is invalid or a mode change
     * is currently pending. 
     *  
     * @param mode  The the runmode that the server should be set to
     * @return      A Response when the runmode was changed
     * @throws IsmRuntimeException  If an error occured while changing the runmode
     */
    @PUT
    @Path("runMode/{domainUid}")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "setRunMode")
    public Response setRunMode(@PathParam("domainUid") String serverInstance, RunMode mode) throws IsmRuntimeException {
        logger.traceEntry(CLAS, "setRunMode", new Object[]{mode});

        // validate the request
        ValidationResult validation = mode.validate();
        if (!validation.isOK()) {
            throw new IsmRuntimeException(Status.BAD_REQUEST, validation.getErrorMessageID(), validation.getParams());
        }

        // request to change mode should return IsmResponse
        IsmConfigSetRunModeRequest configRequest = new IsmConfigSetRunModeRequest(getUserId(), mode);
        configRequest.setServerInstance(serverInstance);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
        	IsmResponse ismResponse = (IsmResponse) resultObject;
        	// make sure we have a return code of 0
        	if (!ismResponse.getRC().equals("0")) {
        		throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, 
        					Utils.getUtils().getIsmErrorMessage(ismResponse.getRC()), 
        					new Object[] {"[RC=" + ismResponse.getRC() + "] " + ismResponse.getErrorString()});
        	}
        } else {
        	// unknown error
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5111", new Object[] {mode.getModeString()});
        }

        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok();

        Response result = rb.cacheControl(cache).build();

        logger.traceExit(CLAS, "setRunMode", result.toString());
        return result;
    }
    

    @GET
    @Path("enableDiskPersistence")
    @Produces(MediaType.APPLICATION_JSON)
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "getEnableDiskPersistence")
    public Response getEnableDiskPersistence() {
        logger.traceEntry(CLAS, "getEnableDiskPersistence");

        EnableDiskPersistence persistence = new EnableDiskPersistence();

        IsmConfigGetEnableDiskPersistenceRequest configRequest = new IsmConfigGetEnableDiskPersistenceRequest(getUserId());
        String resultString = (String)sendGetConfigRequest(configRequest, null);
        logger.log(LogLevel.LOG_INFO, CLAS, "getEnableDiskPersistence", "resultString : " + resultString);

        try {
            JSONArray array = JSONArray.parse(resultString);
            JSONObject obj = (JSONObject) array.get(0);
            persistence.setEnableDiskPersistence(Boolean.parseBoolean((String) obj.get("EnableDiskPersistence")));
        } catch (Exception e) {
            logger.log(LogLevel.LOG_ERR, CLAS, "getEnableDiskPersistence", "CWLNA5003", e);
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, e, "CWLNA5003", null);
        }


        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(persistence);

        logger.traceExit(CLAS, "getEnableDiskPersistence", persistence);
        return rb.cacheControl(cache).build();
    }


    @PUT
    @Path("enableDiskPersistence")
    @Permissions(defaultRoles = {Role.SystemAdministrator}, action = "updateEnableDiskPersistence")
    public Response updateEnableDiskPersistence(EnableDiskPersistence persistence) {
        logger.traceEntry(CLAS, "updateEnableDiskPersistence", new Object[]{persistence});
        ResponseBuilder response;

        IsmConfigSetEnableDiskPersistenceRequest configRequest = new IsmConfigSetEnableDiskPersistenceRequest(getUserId(), persistence);
        Object resultObject = sendSetConfigRequest(configRequest);
        if (resultObject instanceof IsmResponse) {
            IsmResponse r = (IsmResponse)resultObject;
            if (r.getRC() != null && !r.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {"[RC=" + r.getRC() + "] " + r.getErrorString()});
            }
        } else {
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5005", new Object[] {resultObject});
        }

        response = Response.ok();
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        Response result = response.cacheControl(cache).build();

        logger.traceExit(CLAS, "updateEnableDiskPersistence", result.toString());
        return result;
    }

}
