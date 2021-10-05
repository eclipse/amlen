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
package com.ibm.ism.ws.test;


import java.util.Arrays;
import java.util.Properties;

import javax.net.SocketFactory;

import com.ibm.ism.ws.test.TrWriter.LogLevel;
import com.ibm.micro.client.mqttv3.MqttConnectOptions;
import com.ibm.micro.client.mqttv3.MqttTopic;

public class MyConnectionOptions {
	private MqttConnectOptions mqConnectOpts = null;
	private org.eclipse.paho.client.mqttv3.MqttConnectOptions pahoConnectOpts = null;
	private org.eclipse.paho.mqttv5.client.MqttConnectionOptions pahoConnectOptsv5 = null;
	private ConnectOptions mine;
	private int mytype = 0;

	public static final int ISMWS	= MyConnection.ISMWS;
	public static final int ISMMQTT	= MyConnection.ISMMQTT;
	public static final int MQMQTT	= MyConnection.MQMQTT;
	public static final int PAHO	= MyConnection.PAHO;
	public static final int JSONWS	= MyConnection.JSONWS;
	public static final int JSONTCP	= MyConnection.JSONTCP;
	public static final int PAHOV5  = MyConnection.PAHOV5;
	
	MyConnectionOptions(int connectType)  {
		mytype = connectType;
		switch (mytype) {
		case MQMQTT:
			mqConnectOpts = new MqttConnectOptions();
			break;
		case PAHO:
			pahoConnectOpts = new org.eclipse.paho.client.mqttv3.MqttConnectOptions();
			break;
		case PAHOV5:
			pahoConnectOptsv5 = new org.eclipse.paho.mqttv5.client.MqttConnectionOptions();
			break;
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			mine = new ConnectOptions();
		default:	
		}
	}
	
	public void setCleanSession(boolean cleanSession) {
		switch (mytype) {
		case MQMQTT:
			mqConnectOpts.setCleanSession(cleanSession);
			break;
		case PAHO:
			pahoConnectOpts.setCleanSession(cleanSession);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setCleanStart(cleanSession);
			break;
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			mine.setCleanSession(cleanSession);
			break;
		}
	}
	
	public boolean isCleanSession() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.isCleanSession();
		case PAHO:
			return pahoConnectOpts.isCleanSession();
		case PAHOV5:
			return pahoConnectOptsv5.isCleanStart();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.isCleanSession();
		default:
			return false;
		}
	}
	
	public void setKeepAliveInterval(int keepAliveInterval) {
		switch (mytype) {
		case MQMQTT:
			mqConnectOpts.setKeepAliveInterval(keepAliveInterval);
			break;
		case PAHO:
			pahoConnectOpts.setKeepAliveInterval(keepAliveInterval);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setKeepAliveInterval(keepAliveInterval);
			break;
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			mine.setKeepAlive(keepAliveInterval);
			break;
		}
	}
	
	public int getKeepAliveInterval() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.getKeepAliveInterval();
		case PAHO:
			return pahoConnectOpts.getKeepAliveInterval();
		case PAHOV5:
			return pahoConnectOptsv5.getKeepAliveInterval();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.getKeepAlive();
		default:
			return -1;
		}
	}
	
	public void setMaxInflight(int max) {
		if (max<0) return;
		switch (mytype) {
		case PAHO:
			pahoConnectOpts.setMaxInflight(max);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setReceiveMaximum(max);
			break;
		}
	}
	
	public int getMaxInflight() {
		switch (mytype) {
		case PAHO:
			return pahoConnectOpts.getMaxInflight();
		case PAHOV5:
			Integer pahoReceiveMax = pahoConnectOptsv5.getReceiveMaximum();
			if (pahoReceiveMax == null) {
				return 65535;
			} else {
				return pahoReceiveMax;
			}
		}
		return -1;
	}
	
	public void setReceiveMaximum(int max) {
		if (max<0) return;
		switch (mytype) {
		case PAHO:
			pahoConnectOpts.setMaxInflight(max);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setReceiveMaximum(max);
			break;
		}
	}
	
	public int getReceiveMaximum(){
		switch(mytype) {
		case PAHO:
			return pahoConnectOpts.getMaxInflight();
		case PAHOV5:
			return pahoConnectOptsv5.getReceiveMaximum();
		}
		return -1;
	}
	
	public void setWill(Object topic, byte[] payload, int qos, boolean retained) {
		switch (mytype) {
		case MQMQTT:
			if (topic instanceof MqttTopic) {
				mqConnectOpts.setWill((MqttTopic)topic, payload, qos, retained);
			}
			break;
		case PAHO:
			pahoConnectOpts.setWill((String)topic, payload, qos, retained);
			break;
		case PAHOV5:
		    break;
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			mine.setWillMsg(payload);
			mine.setWillTopic((String)topic);
			mine.setWillQoS(qos);
			mine.setWillRetain(retained);
		}
	}
	
	public String getWillTopic() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.getWillDestination().getName();
		case PAHO:
			return pahoConnectOpts.getWillDestination();
		case PAHOV5:
			return pahoConnectOptsv5.getWillDestination();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.getWillTopic();
		default:
			return null;
		}
	}
	
	public byte[] getWillMessage() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.getWillMessage().getPayload();
		case PAHO:
			return pahoConnectOpts.getWillMessage().getPayload();
		case PAHOV5:
			return pahoConnectOptsv5.getWillMessage().getPayload();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.getWillMsg();
		default:
			return null;
		}
	}
	
	public int getWillQoS() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.getWillMessage().getQos();
		case PAHO:
			return pahoConnectOpts.getWillMessage().getQos();
		case PAHOV5:
			return pahoConnectOptsv5.getWillMessage().getQos();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.getWillQoS();
		default:
			return -1;
		}
	}
	
	public boolean isWillRetained() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.getWillMessage().isRetained();
		case PAHO:
			return pahoConnectOpts.getWillMessage().isRetained();
		case PAHOV5:
			return pahoConnectOptsv5.getWillMessage().isRetained();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.isWillRetain();
		default:
			return false;
		}
	}
	
	public void setUserName(String userName) {
		if (null == userName) return;
		switch (mytype) {
		case MQMQTT:
			mqConnectOpts.setUserName(userName);
			break;
		case PAHO:
			pahoConnectOpts.setUserName(userName);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setUserName(userName);
			break;
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			mine.setUserName(userName);
		}
	}
	
	public String getUserName() {
		switch (mytype) {
		case MQMQTT:
			return mqConnectOpts.getUserName();
		case PAHO:
			return pahoConnectOpts.getUserName();
		case PAHOV5:
			return pahoConnectOptsv5.getUserName();
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.getUserName();
		default:
			return null;
		}
	}
	
	public void setPassword(String password) {
		if (null == password) return;
		switch (mytype) {
		case MQMQTT:
			mqConnectOpts.setPassword(password.toCharArray());
			break;
		case PAHO:
			pahoConnectOpts.setPassword(password.toCharArray());
			break;
		case PAHOV5:
			String passwordString = password;
			pahoConnectOptsv5.setPassword(passwordString.getBytes());
			break;
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			mine.setPassword(password);
		}
	}
	
	public String getPassword() {
		switch (mytype) {
		case MQMQTT:
			if (null == mqConnectOpts.getPassword()) {
				return null;
			} else {
				return new String(mqConnectOpts.getPassword());
			}
		case PAHO:
			if (null == pahoConnectOpts.getPassword()) {
				return null;
			} else {
				return new String(pahoConnectOpts.getPassword());
			}
		case PAHOV5:
			if (null == pahoConnectOptsv5.getPassword()) {
				return null;
			} else {
				return new String(pahoConnectOptsv5.getPassword());
			}
		case JSONWS:
		case JSONTCP:
		case ISMWS:
		case ISMMQTT:
			return mine.getPassword();
		default:
			return null;
		}
	}
	
	
	public void setSSLProperties(Properties props) {
		switch (mytype) {
		case MQMQTT:
			mqConnectOpts.setSSLProperties(props);
			break;
		case PAHO:
			pahoConnectOpts.setSSLProperties(props);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setSSLProperties(props);
			break;
		case ISMMQTT:
			mine.setSSLProperties(props);
			break;
		}
	}
	
	public void setServerURIs(String[] uris) {
		switch (mytype) {
		case ISMMQTT:
		    mine.setServers(uris);
		    break;
		case PAHO:
			pahoConnectOpts.setServerURIs(uris);
			break;
		case PAHOV5:
			pahoConnectOptsv5.setServerURIs(uris);
			break;
		}
	}
	
	public String[] getServerURIs() {
		switch (mytype) {
		case ISMMQTT:
		    return mine.getServers();
		case PAHO:
			return pahoConnectOpts.getServerURIs();
		case PAHOV5:
			return pahoConnectOptsv5.getServerURIs();
		}
		return null;
	}
	
	public void setSocketFactory(SocketFactory factory) {
		switch (mytype) {
		case PAHO:
			pahoConnectOpts.setSocketFactory(factory);
			break;
		case PAHOV5:
			pahoConnectOpts.setSocketFactory(factory);
			break;
		}
	}
	
	public void setMqttVersion(int version) throws IsmTestException {
		switch (mytype) {
		case PAHO:
			if (3 == version) {
				pahoConnectOpts.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1);
			} else if (4 == version) {
				pahoConnectOpts.setMqttVersion(org.eclipse.paho.client.mqttv3.MqttConnectOptions.MQTT_VERSION_3_1_1);
			} else {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+19),
				"Attempt to set invalid MQTT version: "+version);
			}
			break;
		case PAHOV5:
            /* For now only 5 works and is already set */
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+18),
					"setMqttVersion not supported for this client.");
		}
	}
	
	public int getMqttVersion() throws IsmTestException {
		switch (mytype) {
		case PAHO:
			return pahoConnectOpts.getMqttVersion();
		case PAHOV5:
			return pahoConnectOptsv5.getMqttVersion();
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+18),
					"getMqttVersion not supported for this client.");
		}
	}

	MqttConnectOptions getMqOpts() {
		return mqConnectOpts;
	}
	
	org.eclipse.paho.client.mqttv3.MqttConnectOptions getPahoOpts() {
		return pahoConnectOpts;
	}
	
	org.eclipse.paho.mqttv5.client.MqttConnectionOptions getPahoOptsv5() {
		return pahoConnectOptsv5;
	}
	
	ConnectOptions getIsmOpts() {
		return mine;
	}
	
	class ConnectOptions {
		private String	userName = null;
		private String	password = null;
		private byte[]	willMsg = null;
		private String	willTopic = null;
		private int		willQoS;
		private boolean	willRetain;
		private boolean	cleanSession = false;
		private int		keepAlive = 0;
		private String	sniServername = null;
		private int		tlsprotocol = 0;
		private String [] serverList;
		
		public String getUserName() {
			return userName;
		}
		public void setUserName(String userName) {
			this.userName = userName;
		}
		public String getPassword() {
			return password;
		}
		public void setPassword(String password) {
			this.password = password;
		}
		public byte[] getWillMsg() {
			return willMsg;
		}
		public void setWillMsg(byte[] willMsg) {
			this.willMsg = willMsg;
		}
		public String getWillTopic() {
			return willTopic;
		}
		public void setWillTopic(String willTopic) {
			this.willTopic = willTopic;
		}
		public int getWillQoS() {
			return willQoS;
		}
		public void setWillQoS(int willQoS) {
			this.willQoS = willQoS;
		}
		public boolean isWillRetain() {
			return willRetain;
		}
		public void setWillRetain(boolean willRetain) {
			this.willRetain = willRetain;
		}
		public void setCleanSession(boolean cleanSession) {
			this.cleanSession = cleanSession;
		}
		public boolean isCleanSession() {
			return cleanSession;
		}
		public void setKeepAlive(int keepAlive) {
			this.keepAlive = keepAlive;
		}
		public int getKeepAlive() {
			return keepAlive;
		}
		public String [] getServers() {
		    return serverList;
		}
		public void setServers(String [] servers) {
		    this.serverList = servers;
		}
		public void setSSLProperties(Properties props) {
			String truststore = props.getProperty("com.ibm.ssl.trustStore");
			if (truststore != null) {
				System.setProperty("MqttTrustStore", truststore);
			}
			String tspassword = props.getProperty("com.ibm.ssl.trustStorePassword");
			if (tspassword != null) {
				System.setProperty("MqttTrustStorePW", tspassword);
			}
			String keystore = props.getProperty("com.ibm.ssl.keyStore");
			if (keystore != null) {
				System.setProperty("MqttKeyStore", keystore);
			}
			String kspassword = props.getProperty("com.ibm.ssl.keyStorePassword");
			if (kspassword != null) {
				System.setProperty("MqttKeyStorePW", kspassword);
			}
			this.sniServername = props.getProperty("com.ibm.ssl.servername");
			String tlsprot = props.getProperty("com.ibm.ssl.protocol");
			if (tlsprot != null) {
			    if (tlsprot.equals("TLSv1.2")) {
				   this.tlsprotocol = 3;
			    } else if (tlsprot.equals("TLSv1.1")) {
				    this.tlsprotocol = 2;
			    } else if (tlsprot.equals("TLSv1")) {
				    this.tlsprotocol = 1;
			    }
			}
		}
		public String getSniServername() {
			return this.sniServername;
		}
		public int getTlsProtocol() {
			return this.tlsprotocol;
		}
	}
}
