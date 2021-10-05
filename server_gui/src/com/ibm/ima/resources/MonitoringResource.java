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

import java.util.Arrays;
import java.util.HashMap;

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

import com.ibm.ima.admin.IsmServerClient;
import com.ibm.ima.admin.request.IsmMonitoringRequest;
import com.ibm.ima.resources.data.EndpointSummary;
import com.ibm.ima.resources.data.EndpointSummaryMetric;
import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.security.Permissions;
import com.ibm.ima.security.Permissions.Role;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.ima.util.Utils;

/**
 * Servlet implementation class MonitoringResource
 */
@Path("/monitoring")
@Produces(MediaType.APPLICATION_JSON)
public class MonitoringResource extends AbstractResource {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = MonitoringResource.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();
    
    private static final String HISTORY = "History";
    private static final String CURRENT = "Current";   

    @Context
    javax.servlet.http.HttpServletRequest currentRequest;

    public enum Action {
    	Subscription (new String[] {"Durable", "Nondurable", "All"}, 
    	        new String[]{"PublishedMsgsHighest", "PublishedMsgsLowest", "BufferedMsgsHighest", "BufferedMsgsLowest", 
    	            "BufferedPercentHighest", "BufferedPercentLowest", "RejectedMsgsHighest", "RejectedMsgsLowest", 
    	            "BufferedHWMPercentHighest", "BufferedHWMPercentLowest", "ExpiredMsgsHighest","ExpiredMsgsLowest",
    	            "DiscardedMsgsHighest","DiscardedMsgsLowest", "PublishedMsgs", "-PublishedMsgs", "BufferedMsgs",
                    "-BufferedMsgs", "BufferedPercent", "-BufferedPercent", "RejectedMsgs", "-RejectedMsgs", 
    	            "BufferedHWMPercent", "-BufferedHWMPercent", "ExpiredMsgs","-ExpiredMsgs", "DiscardedMsgs","-DiscardedMsgs"}, false),
        Queue (null, new String[] {"BufferedMsgsHighest", "BufferedMsgsLowest", "BufferedPercentHighest", "BufferedPercentLowest", 
                "ProducedMsgsHighest", "ProducedMsgsLowest", "ConsumedMsgsHighest", "ConsumedMsgsLowest", "ConsumersHighest", 
                "ConsumersLowest", "ProducersHighest", "ProducersLowest", "BufferedHWMPercentHighest", "BufferedHWMPercentLowest",
                "ExpiredMsgsHighest","ExpiredMsgsLowest"}, false ),
        Topic (null, new String[] {"PublishedMsgsHighest", "PublishedMsgsLowest", "SubscriptionsHighest", "SubscriptionsLowest", "RejectedMsgsHighest", "RejectedMsgsLowest", "FailedPublishesHighest", "FailedPublishesLowest"}, false),
        Transaction (null, new String[] {"Unsorted", "LastStateChangeTime"}, false),
        MQTTClient (null, new String[] {"LastConnectedTimeOldest"}, false),
        Connection (null, new String[] {"NewestConnection", "OldestConnection", "HighestThroughputMsgs", "LowestThroughputMsgs", "HighestThroughputKB", "LowestThroughputKB"}, false),
        Endpoint (new String[] {CURRENT, HISTORY}, new String[]{"ActiveConnections", "Connections", "BadConnections", "LostMsgs", "ReadMsgs", "ReadBytes", "WriteMsgs", "WriteBytes", "Summary", "WarnMsgs"}, false),
        DestinationMappingRule (null, new String[] {"CommitCount", "RollbackCount", "PersistentCount", "NonpersistentCount", "CommittedMessageCount", "Status", "ExpandedMessage"}, false),
        Memory (new String[] {CURRENT, HISTORY}, new String[]{"MemoryTotalBytes","MemoryFreeBytes","MemoryFreePercent","ServerVirtualMemoryBytes","ServerResidentSetBytes","MessagePayloads","PublishSubscribe","Destinations","CurrentActivity", "ClientStates"}, true),
        Store (new String[] {CURRENT, HISTORY}, new String[]{"DiskUsedPercent","DiskFreeBytes","MemoryTotalBytes", "MemoryFreeBytes", "Pool1TotalBytes", "Pool1UsedBytes",
        		"Pool1RecordsLimitBytes","Pool1RecordsUsedBytes","Pool2TotalBytes","Pool2UsedBytes","ClientStatesBytes","QueuesBytes",
        		"TopicsBytes","SubscriptionsBytes","TransactionsBytes","MQConnectivityBytes", "IncomingMessageAcksBytes", "Pool1RecordSize",
        		"MemoryUsedPercent","Pool1UsedPercent","Pool2UsedPercent"}, true);    	
    	private final String[] subTypes;
    	private final String[] statTypes;
    	private boolean allowsMultipleStatTypes = false;
    	
		private Action(final String[] subTypes, final String[] statTypes, boolean allowsMultipleStatTypes) {
			this.subTypes = subTypes;
			this.statTypes = statTypes;
			this.allowsMultipleStatTypes = allowsMultipleStatTypes;
		}
    	
		public void validate(IsmMonitoringRequest request) {
		    if (logger.isTraceEnabled()) {
		        logger.traceEntry("Action", "validate", new Object[] {request.toString()});
		    }		        
			verify(this.subTypes, request.getSubType());
			String requestedStatType = request.getStatType();
			if (this.allowsMultipleStatTypes && requestedStatType != null && requestedStatType.contains(",")) {
			    logger.trace("Action", "validate", "checking multiple statTypes " + requestedStatType);
			    String[] statTypes = requestedStatType.split(",");
			    for (String statType : statTypes) {
	                logger.trace("Action", "validate", "verifying statType " + statType);
			        verify(this.statTypes, statType);
			    }
			} else {
                logger.trace("Action", "validate", "verifying a single statType of " + requestedStatType);
			    verify(this.statTypes, requestedStatType);
			}
		}

		private void verify(String[] values, String value) {
		    if (logger.isTraceEnabled()) {
		        logger.traceEntry("Action", "verify", new Object[] {Arrays.toString(values), value});
		    }
			if (values != null && value != null && !value.isEmpty()) {
				for (String s : values) {
					if (s.equals(value)) {
						return;
					}
				}
				logger.trace("Action", "verify", "Found invalid value: " + value);
				throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);
			}
		}
    }

    
     public MonitoringResource() {
        super();
        logger.traceExit(CLAS, "MonitoringResource");
    }

    /**
     * @param request The monitoring request to send to ISM server.
     * @return The result of the monitoring request.
     */
    @POST
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getMonitoringData")
    public Response getMonitoringData(IsmMonitoringRequest request) {
        logger.traceEntry(CLAS, "getMonitoringData", new Object[]{request});
        return getMonitoringDataDomain(null, request);
    }
    
    /**
     * @param request The monitoring request to send to ISM server.
     * @return The result of the monitoring request.
     */
    @POST
    @Path("{domainUid}")
    @Permissions(defaultRoles = {Role.SystemAdministrator,Role.MessagingAdministrator, Role.User}, action = "getMonitoringData")
    public Response getMonitoringDataDomain(@PathParam("domainUid") String serverInstance, IsmMonitoringRequest request) {

        checkRequest(request);
        request.setUser(getUserId());
        request.setLocale(getClientLocale().toString());
        request.setServerInstance(serverInstance);

        IsmServerClient client = new IsmServerClient(request, getClientLocale());
        
        String resultString = "";
        logger.trace(CLAS, "getMonitoringData", "Request: " + request.toString());
        if (request.getStatType() != null && request.getStatType().equals("Summary")) {
        	// special case request
        	return getSummary(request, client);
        } 
        resultString = client.sendMonitoringMessage(request);        
        logger.trace(CLAS, "getMonitoringData", "Response: " + resultString);
        if (resultString == null) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "getMonitoringData", "CWLNA5003");
            throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5003", null);            
        }
        Object result = resultString;
        if (!resultString.startsWith("{") && !resultString.startsWith("[")) {
            if (resultString.equals("No connection data is found.")) {
                result = "{ \"RC\":\"113\", \"ErrorString\":\"No connection data is found.\" }";
            } else {
                logger.log(LogLevel.LOG_WARNING, CLAS, "getMonitoringData", "CWLNA5003");
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5003", null);
            }
        } else if (Action.Endpoint.name().equals(request.getAction()) && HISTORY.equals(request.getSubType())) {
        	// Endpoint history has a timestamp in nanos. We need to convert that to milliseconds for the browser...
            EndpointSummaryMetric parsedResult = parseEndpointHistory(resultString);
            if (parsedResult != null) {
            	result = parsedResult;
            }            
        }
        
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(result);
        
        logger.traceExit(CLAS, "getMonitoringData");
        return rb.cacheControl(cache).build();
    }
    
    private EndpointSummaryMetric parseEndpointHistory(String result) {
        logger.traceEntry(CLAS, "parseEndpointHistory", new Object[] {result});
        ObjectMapper mapper = new ObjectMapper();
        EndpointSummaryMetric history = null;
        try {
            history = mapper.readValue(result, EndpointSummaryMetric.class);
            if (!history.getRC().equals("0")) {
                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
                        .getIsmErrorMessage(history.getRC()), new Object[] { history.getErrorMessage() });
            }
        } catch (IsmRuntimeException ire) {        	
        	logger.trace(CLAS, "parseEndpointHistory", "Invalid JSON returned: " + result);
        } catch (Exception e) {
        	logger.trace(CLAS, "parseEndpointHistory", "Invalid JSON returned: " + result);
        }
        logger.traceExit(CLAS, "parseEndpointHistory", history);
        return history;      
    }

	private Response getSummary(IsmMonitoringRequest request, IsmServerClient client) {
		HashMap<String, IsmMonitoringRequest> monitoringRequests = new HashMap<String, IsmMonitoringRequest>();
		String[] statTypes = EndpointSummary.ENDPOINT_SUMMARY_STATS;
		for (String statType : statTypes) {
			monitoringRequests.put(statType, getSummaryRequest(request, statType));
		}
		HashMap<String, String> results = client.sendMonitoringMessages(monitoringRequests);        
		ObjectMapper mapper = new ObjectMapper();
		EndpointSummary rollup = new EndpointSummary();
		EndpointSummaryMetric readWriteSummary = null;
		rollup.setType("Summary");		
		for (String statType : statTypes) {
			String result = results.get(statType);
			if (result == null || result.isEmpty()) {
				continue;
			}
			try {
				EndpointSummaryMetric summary = mapper.readValue(result, EndpointSummaryMetric.class);
				if (!summary.getRC().equals("0")) {
	                throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, Utils.getUtils()
	                        .getIsmErrorMessage(summary.getRC()), new Object[] { summary.getErrorMessage() });
				}
				if (statType.equals("ReadMsgs") || statType.equals("WriteMsgs")) {
					// first one in gets to be "Messages"
					if (readWriteSummary == null) {
						readWriteSummary = summary;
						rollup.setDuration(summary.getDuration());
						rollup.setInterval(summary.getInterval());
						rollup.setEndpoint(summary.getEndpoint());
					} else {
						// second one in gets merged
						rollup.addMetric("Messages", readWriteSummary.mergeAndSummarize(summary));
					}
				} else {
					rollup.addMetric(statType,summary.summarize());
				}
			} catch (IsmRuntimeException ire) {
				logger.log(LogLevel.LOG_WARNING, CLAS, "getSummary", "CWLNA5003", ire);
				throw ire;
			} catch (Exception e) {
				logger.log(LogLevel.LOG_WARNING, CLAS, "getSummary", "CWLNA5003", e);
				throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, "CWLNA5003", null);
			}
		}
		
		// try to consolidate read and write messages in to one
		
		
        CacheControl cache = new CacheControl();
        cache.setMaxAge(0);
        cache.setNoCache(true);
        cache.setNoStore(true);
        ResponseBuilder rb =  Response.ok(rollup);

        return rb.cacheControl(cache).build();
	}

	private IsmMonitoringRequest getSummaryRequest(IsmMonitoringRequest request, String statType) {
		IsmMonitoringRequest summaryRequest = new IsmMonitoringRequest();
		summaryRequest.setAction(request.getAction());
		summaryRequest.setLocale(getClientLocale().toString());
		summaryRequest.setDuration(request.getDuration());
		summaryRequest.setSubType(request.getSubType());
		summaryRequest.setUser(request.getUser());
		summaryRequest.setStatType(statType);
		return summaryRequest;
	}

	private void checkRequest(IsmMonitoringRequest request) {
		if (request == null || request.getAction() == null) {
			throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);
		}
		
		try {
			Action action = Action.valueOf(request.getAction());
			action.validate(request);
		} catch (IsmRuntimeException ire) {
			throw ire;
		} catch (Exception e) {
			throw new IsmRuntimeException(Status.BAD_REQUEST, "CWLNA5003", null);
		}
		
	}

}
