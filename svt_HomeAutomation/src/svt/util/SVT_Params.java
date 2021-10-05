/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package svt.util;


public class SVT_Params {
	public String mode;
	public String ismserver;
	public String ismserver2;
	public Integer ismport; 
	public String appId;
	public Long minutes;
	public Long hours;
	public Long messages;
	public SVTStats stats;
	public long threadId;
	public boolean order;
	public Integer qos;
	public boolean listener;
	public boolean ssl;
	public String userName;
	public String password;
	public boolean cleanSession;
	public Integer clientType;

	public SVT_Params(String mode, String server,
	        String server2, Integer port, String appid, Long minutes,
            Long hours , Long messages , SVTStats stats, long threadId, boolean order ,
            Integer qos , boolean listener , boolean ssl , String name,
            String password, Boolean cleanSession,Integer clientType) {
		this.mode= mode; ;
		this.ismserver = server;
		this.ismserver2 = server2;
		this.ismport = port; 
		this.appId = appid;
		this.minutes=minutes;
		this.cleanSession = cleanSession;
		this.hours=hours;
		this.messages= messages;
		this.stats = stats;
		this.threadId= threadId;
		this.order= order;
		this.qos=qos;
		this.listener=listener;
		this.ssl=ssl;
		this.userName=name;
		this.password=password;
		this.clientType=clientType;
	}
	
}
