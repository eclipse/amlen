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
package com.ibm.ima.test.stat;

import java.net.InetAddress;
import java.net.UnknownHostException;




public class ExecutionStatus {
	
	private int exitCode = 0;
	private String clientId = null;
	
	private CLIENT_TYPE clientType = null;
    private STATUS_TYPE statusType = null;
	
	private long startTime = 0;
	private long endtime = 0;
	
	private long msgsProduced = 0;
	private long goodSends = 0;
	private long badSends = 0;
	
	private long goodConns = 0;
	
	private long msgsConsumed = 0;
	
	private String summaryOutFile = null;
	
	private static String ipAddr = getIpAsString();
	
	public static final String PRODUCER_HEADER ="host,clientType,clientId,exitCode,msgsProduced,goodSends,badSends,runTime\n";
	public static final String CONSUMER_HEADER ="host,clientType,clientId,exitCode,msgsConsumed,runTime\n";
	public static final String CONN_STORM_HEADER ="host,clientType,clientId,exitCode,goodConns,runTime\n";

	
	public enum STATUS_TYPE {

		EXIT_STATUS("EXIT"),
		INITIALIZED_STATUS("INITIALIZED");

		private String value;


		STATUS_TYPE(String value) {
			this.value = value;
		}


		public String getText() {
			return this.value;
		}


		public static STATUS_TYPE fromString(String value) {
			if (value != null) {
				for (STATUS_TYPE b : STATUS_TYPE.values()) {
					if (value.equalsIgnoreCase(b.value)) {
						return b;
					}
				}
			}
			return null;
		}
	}
	
	public enum CLIENT_TYPE {

		JMS_CONSUMER("JMS Consumer"),
		JMS_PRODUCER("JMS Producer"),
		MQTT_CONSUMER("MQTT Consumer"),
		MQTT_PRODUCER("MQTT Producer"),
		MQTT_CONN_STORM("MQTT Connection Storm");

		private String value;


		CLIENT_TYPE(String value) {
			this.value = value;
		}


		public String getText() {
			return this.value;
		}


		public static CLIENT_TYPE fromString(String value) {
			if (value != null) {
				for (CLIENT_TYPE b : CLIENT_TYPE.values()) {
					if (value.equalsIgnoreCase(b.value)) {
						return b;
					}
				}
			}
			return null;
		}
	}
	
	public ExecutionStatus(String clientId, CLIENT_TYPE clientType, STATUS_TYPE statusType) {
		this.clientId = clientId;
		this.clientType = clientType;
		this.statusType = statusType;
	}
	

	public int getExitCode() {
		return exitCode;
	}

	public void setExitCode(int exitCode) {
		this.exitCode = exitCode;
	}

	public void setClientId(String clientId) {
		this.clientId = clientId;
	}

	public String getClientId() {
		return clientId;
	}

	public CLIENT_TYPE getClientType() {
		return clientType;
	}


	public void setClientType(CLIENT_TYPE clientType) {
		this.clientType = clientType;
	}


	public STATUS_TYPE getStatusType() {
		return statusType;
	}


	public void setStatusType(STATUS_TYPE statusType) {
		this.statusType = statusType;
	}


	public long getStartTime() {
		return startTime;
	}


	public void setStartTime(long startTime) {
		this.startTime = startTime;
	}


	public long getEndtime() {
		return endtime;
	}


	public void setEndtime(long endtime) {
		this.endtime = endtime;
	}

	

	public long getMsgsProduced() {
		return msgsProduced;
	}


	public void setMsgsProduced(long msgsProduced) {
		this.msgsProduced = msgsProduced;
	}


	public long getGoodSends() {
		return goodSends;
	}


	public void setGoodSends(long goodSends) {
		this.goodSends = goodSends;
	}


	public long getBadSends() {
		return badSends;
	}


	public void setBadSends(long badSends) {
		this.badSends = badSends;
	}


	public long getMsgsConsumed() {
		return msgsConsumed;
	}


	public void setMsgsConsumed(long msgsConsumed) {
		this.msgsConsumed = msgsConsumed;
	}
	
	


	public long getGoodConns() {
		return goodConns;
	}


	public void setGoodConns(long goodConns) {
		this.goodConns = goodConns;
	}


	public void producerExit(int exitCode, long startTime, long endTime, long msgsProduced, long goodSends, long badSends) {
		this.exitCode = exitCode;
		this.startTime = startTime;
		this.endtime = endTime;
		this.msgsProduced = msgsProduced;
		this.goodSends = goodSends;
		this.badSends = badSends;
	}
	
	public void consumerExit(int exitCode, long startTime, long endTime, long msgsConsumed) {
		this.exitCode = exitCode;
		this.startTime = startTime;
		this.endtime = endTime;
		this.msgsConsumed = msgsConsumed;
	}
	
	public void connStormExit(int exitCode, long startTime, long endTime, int goodConns) {
		this.exitCode = exitCode;
		this.startTime = startTime;
		this.endtime = endTime;
		this.goodConns = goodConns;
	}
	
	public String getProducerExitMsg() {
		
		
		long runtTime = this.endtime - this.startTime;
		if (runtTime > 0) {
			runtTime = runtTime /1000;
		}
		String msg = ipAddr + "," + this.clientType.getText() + "," + this.clientId + "," + this.exitCode + "," +
	                 this.msgsProduced + "," + this.goodSends + "," + this.badSends + "," + runtTime + "\n";
		
		return msg;
	}
	
	public String getConsumerExitMsg() {
		
		long runtTime = this.endtime - this.startTime;
		if (runtTime > 0) {
			runtTime = runtTime /1000;
		}

		String msg = ipAddr + "," + this.clientType.getText() + "," + this.clientId + "," + this.exitCode + "," +
	                 this.msgsConsumed + "," + runtTime + "\n";
		
		return msg;
	}
	
	public String getConnStormExitMsg() {
		
		long runtTime = this.endtime - this.startTime;
		if (runtTime > 0) {
			runtTime = runtTime /1000;
		}

		String msg = ipAddr + "," + this.clientType.getText() + "," + this.clientId + "," + this.exitCode + "," +
	                 this.goodConns + "," + runtTime + "\n";
		
		return msg;
	}
	
	
	
	
	private static String getIpAsString() {

		String hostname = "unknown"; //str = new StringBuffer();
		try {
			/*byte[] ipAddress = InetAddress.getLocalHost().getAddress();
			for(int i=0; i<ipAddress.length; i++) {
				if(i > 0) str.append('.');
				str.append(ipAddress[i] & 0xFF);				
			}*/
			
			hostname = InetAddress.getLocalHost().getHostName();
		} catch (UnknownHostException e) {
			
		}
		

		return hostname;
	}
	

}
