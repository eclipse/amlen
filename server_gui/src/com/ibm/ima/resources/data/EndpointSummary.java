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

import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.json.java.JSONObject;

/**
 * Represents a summarization of endpoint history data for the efficiency
 * and convenience of the dashboard.
 *  
 *
 */
public class EndpointSummary {
    @SuppressWarnings("unused")

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	private final static String CLAS = EndpointSummary.class.getCanonicalName();

    public final static String[] ENDPOINT_SUMMARY_STATS = {"ActiveConnections", "Connections", "BadConnections", "LostMsgs", "ReadMsgs", "WriteMsgs"};
    
    private String endpoint = null;
    private String type = null;
    private String duration = null;
    private String interval = null;
    private JSONObject metrics = null;
    
	public EndpointSummary() {
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

	public JSONObject getMetrics() {
		return metrics;
	}

	public void setMetrics(JSONObject metrics) {
		this.metrics = metrics;
	}

	public void addMetric(String name, JSONObject metric) {
		if (name == null || metric == null) {
			return;
		}
		if (metrics == null) {
			metrics = new JSONObject();
		}
		metrics.put(name, metric);		
	}

}
