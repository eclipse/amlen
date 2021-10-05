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

package com.ibm.ima.resources.data;

import javax.ws.rs.core.Response.Status;

import com.fasterxml.jackson.annotation.JsonAnySetter;
import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.resources.exceptions.IsmRuntimeException;
import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;
import com.ibm.json.java.JSONObject;

/**
 * Represents an endpoint history metric
 *  
 *
 */
public class EndpointSummaryMetric {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = EndpointSummaryMetric.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();

    private String endpoint = null;
    private String type = null;
    private String duration = null;
    private String interval = null;
    private String lastUpdateTimestamp = null;  // comes in as a string representing timestamp in nanoseconds
    private String data = null;
    private long peak = -1;
    private long average = -1;
    private long total = -1;
    
    private String RC = "0";
    private String ErrorString = "";
    
	public EndpointSummaryMetric() {

	}
	
    /*
     * Handle any property we don't know about.
     */
    @JsonAnySetter
    public void handleUnknown(String key, Object value) {
        logger.trace(CLAS, "handleUnknown", "Unknown property " + key + " with value " + value + " received");
    }

	/**
	 * @return the endpoint
	 */
    @JsonProperty("Endpoint")
	public String getEndpoint() {
		return endpoint;
	}

	/**
	 * @param endpoint the endpoint to set
	 */
    @JsonProperty("Endpoint")
	public void setEndpoint(String endpoint) {
		this.endpoint = endpoint;
	}

	/**
	 * @return the type
	 */
    @JsonProperty("Type")
	public String getType() {
		return type;
	}

	/**
	 * @param type the type to set
	 */
    @JsonProperty("Type")
	public void setType(String type) {
		this.type = type;
	}

	/**
	 * @return the duration
	 */
    @JsonProperty("Duration")
	public String getDuration() {
		return duration;
	}

	/**
	 * @param duration the duration to set
	 */
    @JsonProperty("Duration")
	public void setDuration(String duration) {
		this.duration = duration;
	}

	/**
	 * @return the interval
	 */
    @JsonProperty("Interval")
	public String getInterval() {
		return interval;
	}

	/**
	 * @param interval the interval to set
	 */
    @JsonProperty("Interval")
	public void setInterval(String interval) {
		this.interval = interval;
	}

	/**
     * @return the lastUpdateTimestamp
     */
    @JsonProperty("LastUpdateTimestamp")
    public long getLastUpdateTimestamp() {
        long millis = System.currentTimeMillis();
        if (lastUpdateTimestamp == null) {
            logger.trace(CLAS, "getLastUpdateTimestamp", "LastUpdateTimestamp is null");            
        } else {
            try {
                long nanos = Long.parseLong(lastUpdateTimestamp);                
                millis = nanos / (1000000);
            } catch (NumberFormatException nfe) {
                logger.trace(CLAS, "getLastUpdateTimestamp", "Failed to parse LastUpdateTimestamp: " + lastUpdateTimestamp);
            }
        }
        
        return millis;
    }

    /**
     * @param lastUpdateTimestamp the lastUpdateTimestamp to set
     */
    @JsonProperty("LastUpdateTimestamp")
    public void setLastUpdateTimestamp(String lastUpdateTimestamp) {
        this.lastUpdateTimestamp = lastUpdateTimestamp;
    }

    /**
     * @return the currentTime
     */
    @JsonProperty("CurrentSystemTimestamp")
    public long getCurrentTime() {
        return System.currentTimeMillis();
    }


    /**
	 * @return the data
	 */
    @JsonProperty("Data")
	public String getData() {
		return data;
	}

	/**
	 * @param data the data to set
	 */
    @JsonProperty("Data")
	public void setData(String data) {
		this.data = data;
	}

	/**
	 * @return the peak
	 */
    @JsonProperty("Peak")
	public long getPeak() {
		return peak;
	}

	/**
	 * @param peak the peak to set
	 */
    @JsonProperty("Peak")
	public void setPeak(long peak) {
		this.peak = peak;
	}

	/**
	 * @return the average
	 */
    @JsonProperty("Average")
	public long getAverage() {
		return average;
	}

	/**
	 * @param average the average to set
	 */
    @JsonProperty("Average")
	public void setAverage(long average) {
		this.average = average;
	}

	/**
	 * @return the total
	 */
    @JsonProperty("Total")
	public long getTotal() {
		return total;
	}

	/**
	 * @param total the total to set
	 */
    @JsonProperty("Total")
	public void setTotal(long total) {
		this.total = total;
	}

    @JsonProperty("RC")
    public String getRC() {
        return RC;
    }
    
    @JsonProperty("RC")
    public void setRC(String rC) {
        RC = rC;
    }
    
    @JsonProperty("ErrorString")
    public String getErrorString() {
        return ErrorString;
    }
    
    @JsonProperty("ErrorString")
    public void setErrorString(String errorString) {
        ErrorString = errorString;
    }
    
    @JsonIgnore
    public String getErrorMessage() {
        return "[RC=" + RC + "] " + ErrorString;
    }

    
	@JsonIgnore
    /**
     * Calculates some stats depending on the type
     * 
     * ActiveConnections: Calculates total, average, and peak
     * Connections, BadConnections, LostMsgs:  Calculates total over the period
     */
	public JSONObject summarize() {
    	if (type == null || type.isEmpty() || data == null || data.isEmpty()) {
    		return null;
    	}
		JSONObject summary = new JSONObject();
    	long[] dataPoints = getDataAsLong();
		int numPoints = dataPoints.length;
    	if (type.equals("ActiveConnections")) {
    		// Average is total / num points; Peak is largest point
    		peak = 0;
    		total = 0;
    		for (int i=0; i<numPoints; i++) {
    			total += dataPoints[i];
    			peak = Math.max(peak, dataPoints[i]);
    		}
    		average = total / numPoints;    
    		summary.put("Peak", peak);
    		summary.put("Average", average);
    	} else if (type.equals("Connections") || type.equals("BadConnections") || type.equals("LostMsgs")) {
    		// cumulative items, just determine the total
    		if (numPoints > 1) {
    			total = dataPoints[0] - dataPoints[numPoints-1];  // total over the requested period    			
    		} else {
    			total = 0;
    		}
    		summary.put("Total", total);
    		summary.put("SinceReset", dataPoints[0]);
    	}
		return summary;
	}

	protected long[] getDataAsLong() {
	    if (data == null || data.isEmpty()) {
	        return new long[0];
	    }
		String[] sData = data.split(",");
		long[] intData = new long[sData.length];
		for (int i=0; i < sData.length; i++) {
		    try {
		        intData[i] = Long.parseLong(sData[i]);
		    } catch (NumberFormatException nfe) {
		        logger.log(LogLevel.LOG_ERR, CLAS, "getDataAsLong", "CWLNA5119", new Object[] {sData[i], nfe.getLocalizedMessage()}, nfe);
		        throw new IsmRuntimeException(Status.INTERNAL_SERVER_ERROR, nfe, "CWLNA5119", new Object[] {sData[i], nfe.getLocalizedMessage()});
		    }
		}
		return intData;
	}

	/**
	 * Merges the summary data with this one and then summarizes.
	 * Intended to merge Read and Write data into message throughput
	 * @param summary object to merge
	 */
	public JSONObject mergeAndSummarize(EndpointSummaryMetric summaryMetric) {
		JSONObject summary = new JSONObject();
		int iInterval = Integer.parseInt(interval);
    	long[] dataPoints = getDataAsLong();
		summary.put(getType()+"SinceReset", dataPoints[0]);
    	long[] mergePoints = summaryMetric.getDataAsLong();
		summary.put(summaryMetric.getType()+"SinceReset", mergePoints[0]);    	
		int numPoints = Math.min(dataPoints.length, mergePoints.length);
		peak = 0;
		total = dataPoints[0];
		dataPoints[0] += mergePoints[0];
		for (int i=1; i<numPoints; i++) {
			dataPoints[i] += mergePoints[i];
			peak = Math.max(peak, (dataPoints[i-1] - dataPoints[i]));  // peak delta
		}
		peak = peak / iInterval;
		total = dataPoints[0] - dataPoints[numPoints-1];
		average = total / numPoints / iInterval;
		summary.put("Peak", peak);
		summary.put("Total", total);
		summary.put("Average", average);
		summary.put("SinceReset", dataPoints[0]);
		return summary;
	}

    @Override
    public String toString() {        
        return "EndpointSummaryMetric [endpoint=" + endpoint + ", type=" + type + ", duration=" + duration +
                    ", interval=" + interval + ", lastUpdateTimestamp=" + lastUpdateTimestamp +"]";
    }

	
    
}
