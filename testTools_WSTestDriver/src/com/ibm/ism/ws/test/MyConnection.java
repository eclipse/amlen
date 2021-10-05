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

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;
import java.util.Vector;

import com.ibm.ism.jsonmsg.JsonConnection;
import com.ibm.ism.jsonmsg.JsonListener;
import com.ibm.ism.jsonmsg.JsonMessage;
import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttListenerAsync;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.mqtt.IsmMqttResult;
import com.ibm.ism.mqtt.IsmMqttUserProperty;
import com.ibm.ism.mqtt.MqttRC;
import com.ibm.ism.ws.IsmWSConnection;
import com.ibm.ism.ws.IsmWSMessage;
import com.ibm.ism.ws.test.TrWriter.LogLevel;
import com.ibm.micro.client.mqttv3.MqttCallback;
import com.ibm.micro.client.mqttv3.MqttClient;
import com.ibm.micro.client.mqttv3.MqttClientPersistence;
import com.ibm.micro.client.mqttv3.MqttDefaultFilePersistence;
import com.ibm.micro.client.mqttv3.MqttDeliveryToken;
import com.ibm.micro.client.mqttv3.MqttException;
import com.ibm.micro.client.mqttv3.MqttMessage;
import com.ibm.micro.client.mqttv3.MqttTopic;

import org.eclipse.paho.client.mqttv3.IMqttToken;
import org.eclipse.paho.client.mqttv3.MqttAsyncClient;
import org.eclipse.paho.client.mqttv3.util.Debug;
import org.eclipse.paho.mqttv5.common.packet.MqttProperties;
import org.eclipse.paho.mqttv5.common.packet.UserProperty;

/**
 *
 */
public class MyConnection implements Counter{

	public static final int ISMWS		= 1;
	public static final int ISMMQTT		= 2;
	public static final int ISMWSMQTT	= 3;
	public static final int MQMQTT		= 4;
	public static final int PAHO		= 5;
	public static final int JSONWS		= 6;
	public static final int JSONTCP		= 7;
	public static final int PAHOV5      = 8;
	
    public  static final int BINARY   = IsmMqttConnection.BINARY;     /* version 3 */
    public  static final int BINARYWS = IsmMqttConnection.BINARYWS;   /* version 3 websockets */ 

    
	private List<MyMessage> messages = Collections.synchronizedList(new LinkedList<MyMessage>());

	private int connType = 0;
	private int messagesReceived = 0;
	private Map<String, Integer> messagesPerTopic = new HashMap<String, Integer>();
//	Object connection = null;
	IsmWSConnection wsConn = null;
	IsmMqttConnection wsMqttConn = null;
	JsonConnection jsonConn = null;
	MqttClient mqttConn= null;
	volatile MqttAsyncClient pahoConn = null;
	volatile org.eclipse.paho.mqttv5.client.MqttAsyncClient pahoConnv5 = null;
	private TrWriter _trWriter = null;
	private boolean connectionLost = false;
	private boolean reconnected = false;
	private Throwable cause;
	private String    reason;
	private int       reasonCode;
	private CreateConnectionAction myConnAct;
	private String clientID;
	private String currentIP = null;
	private MyConnectionOptions myOpts = null;
	private long[] monitorCounts = new long[7];
	private long totalMonitorRecords = 0;
	private boolean flag = false;
	private String topic = null;
	private int noDelayCount = 0;
	private Map<String, JsonTopic> subscriptions = new HashMap<String, JsonTopic>();
	private int lastSubscribe = 0; 
	private int id = 0;
	int inFlightMax = 0;
	private String name = null;
	int mqttVersion = 3;
	private boolean sessionPresent = false;
	Map<Integer, String> topicAliasMap;
	private org.eclipse.paho.mqttv5.client.IMqttToken connectionToken = null;

	public MyConnection(int connType, String path
			, String protocol, String clientID
			, CreateConnectionAction connAct, int mversion) throws IsmTestException {
		myConnAct = connAct;
		this.clientID = clientID; 
		this.connType = connType;
		String myip = myConnAct._ip;
		if (mversion > 0)
		    mqttVersion = mversion;
		if (-1 != myip.indexOf(":")) {
			myip = "["+myConnAct._ip+"]";
		}
		currentIP = myConnAct._ip;
		_trWriter = myConnAct._trWriter;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_STATUS, "ISMTEST","Creating MyConnection");
		switch (connType) {
		case ISMWS:
			wsConn = new IsmWSConnection(myConnAct._ip, myConnAct._port, path, protocol);
			wsConn.setVersion(IsmWSConnection.WebSockets13);
			break;
			
		case ISMMQTT:
//			System.out.println("creating ism mqtt connection");
			_trWriter.writeTraceLine(LogLevel.LOGLEV_STATUS, "ISMTEST","creating ism mqtt connection");
			wsMqttConn = new IsmMqttConnection(myConnAct._ip, myConnAct._port, path, clientID);
			wsMqttConn.setAsyncListener(new TestDriverIsmListener(this));
			break;
			
		case MQMQTT:
			try {
				if (null != myConnAct._persistenceDirectory) {
					MqttClientPersistence persist = (MqttClientPersistence) new MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
					mqttConn = new MqttClient((myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._port
							, clientID, persist);					
				} else {
					mqttConn = new MqttClient((myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._port, clientID);
				}
				mqttConn.setCallback(new TestDriverMqttCallback());
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION), 
						e.getMessage()+" Attempting to create MQ MqttClient", e);
			}
			break;
			
		case PAHO:
			try {
				if (null != myConnAct._persistenceDirectory) {
					org.eclipse.paho.client.mqttv3.MqttClientPersistence persist = 
							(org.eclipse.paho.client.mqttv3.MqttClientPersistence)
							new org.eclipse.paho.client.mqttv3.persist.MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
					pahoConn = new MqttAsyncClient((myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._port
							, clientID, persist);					
				} else {
					pahoConn = new MqttAsyncClient((myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._port, clientID);
				}
				pahoConn.setCallback(new TestDriverPahoCallback(this));
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION), 
						e.getMessage()+" Attempting to create Paho MqttClient", e);
			}
			break;
			
		case PAHOV5:
			try {
				if (null != myConnAct._persistenceDirectory) {
					org.eclipse.paho.mqttv5.client.MqttClientPersistence persist = 
							(org.eclipse.paho.mqttv5.client.MqttClientPersistence)
							new org.eclipse.paho.mqttv5.client.persist.MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
					pahoConnv5 = new org.eclipse.paho.mqttv5.client.MqttAsyncClient((myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._port
							, clientID, persist);					
				} else {
					pahoConnv5 = new org.eclipse.paho.mqttv5.client.MqttAsyncClient((myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._port, clientID, null);
				}
				pahoConnv5.setCallback(new TestDriverPahov5Callback(this));
			} catch (org.eclipse.paho.mqttv5.common.MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION), 
						e.getMessage()+" Attempting to create Paho v5 MqttClient", e);
			}
			topicAliasMap = new HashMap<Integer, String>();
			break;
			
		case JSONWS:
		case JSONTCP:
			jsonConn = new JsonConnection(myip, myConnAct._port, path, clientID);
			jsonConn.setEncoding((connType == JSONWS ? JsonConnection.JSON_WS : JsonConnection.JSON_TCP));
			jsonConn.setListener(new TestDriverJsonCallback());
			break;
			
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+1),
					"Unknown connection type: "+connType);
		}
	}
	
	public Object getConnectObject() {
	    switch (connType) {
	    case ISMWS:
	        return wsConn;
	    case ISMMQTT:
	        return wsMqttConn;
	    case MQMQTT:
	        return mqttConn;
	    case PAHO:
	        return pahoConn;
	    case PAHOV5:
	        return pahoConnv5;
	    case JSONWS:
	    case JSONTCP:
	        return jsonConn;
	    }
	    return null;
	}
	
	/*
	 * Common implementation of sleep without error handling
	 */
	public static void sleep(int millis) {
        try {
            Thread.sleep(millis, 0);
        } catch (InterruptedException e) {
             Thread.currentThread().interrupt();
        }
	}

	public int getConnectionType() {
		return connType;
	}
	
	public boolean isConnected() {
		switch (connType) {
		case ISMWS:
			return wsConn.isConnected();
		case ISMMQTT:
			return wsMqttConn.isConnected();
		case MQMQTT:
			return mqttConn.isConnected();
		case PAHO:
			return pahoConn.isConnected();
		case PAHOV5:
			return pahoConnv5.isConnected();
		case JSONWS:
		case JSONTCP:
			return jsonConn.isConnected();
		}
		return false;
	}
	
	/*
	 * Return the reason code of the connection.
	 * This is generally 0 if isConnected() is true
	 */
	public int getReasonCode() {
	    return reasonCode;
	}
	
	/*
	 * Return the reason string of the connection.
	 * This is generally null if the isConnected() is true
	 */
	public String getReason() {
	    return reason;
	}
	
	/*
	 * Return session present (v5)
	 */
	public boolean getSessionPresent() {
		return sessionPresent;
	}
	
	/*
	 * Returns connection token (v5)
	 */
	public org.eclipse.paho.mqttv5.client.IMqttToken getConnToken() {
		switch (connType) {
		case ISMWS:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50)
					, "getConnToken is not supported for this connection type");
			return null;
		case ISMMQTT:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50)
					, "getConnToken is not supported for this connection type");
			return null;
		case MQMQTT:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50)
					, "getConnToken is not supported for this connection type");
			return null;
		case PAHO:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50)
					, "getConnToken is not supported for this connection type");
			return null;
		case PAHOV5:
			return connectionToken;
		case JSONWS:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50)
					, "getConnToken is not supported for this connection type");
			return null;
		case JSONTCP:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50)
					, "getConnToken is not supported for this connection type");
			return null;
		}
		return null;
		
	}
	
//	public int inQueueMessages() {
//		return messages.size();
//	}
	public MyConnectionOptions getMyOpts() {
		return myOpts;
	}

	
	public MyMessage myReceive() throws IOException {
		MyMessage result = null;
		switch (connType) {
		case ISMWS:
			return new MyMessage(wsMqttConn.receive());
		case ISMMQTT:
		case MQMQTT:
		case PAHO:
		case PAHOV5:
		case JSONWS:
		case JSONTCP:
			while (result == null) {
				try {
					if (messages.size() > 0) {
						result = (MyMessage)messages.remove(0);
						return result;
					}
				} catch (Exception e) {
				    break;
				}
				if (!isConnected())
			        break;
			    try {
			        Thread.sleep(10, 0);
			    } catch (InterruptedException e) {
                     Thread.currentThread().interrupt();
			    }
			}
		}
		return result;
	}
	
	public MyMessage myReceive(long millis) throws IOException, IsmTestException {
		MyMessage result = null;
		switch (connType) {
		case ISMWS:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
					"WebSocket connections do not support timed receive");
		case ISMMQTT:
		case PAHO:
		case PAHOV5:
		case MQMQTT:
		case JSONWS:
		case JSONTCP:
		    long lEndTime = System.currentTimeMillis() + millis;
		    boolean restarted = false;
			while (null == result && (System.currentTimeMillis() < lEndTime)) {
				try {
					if (messages.size() !=  0) {
						result = (MyMessage)messages.remove(0);
						return result;
					}
				} catch (Exception e) {}
				if (isConnected()) {
				    try {
				        Thread.sleep(10, 0);
				    } catch (InterruptedException e) {
		                Thread.currentThread().interrupt();
				    }    
		        }
				if (!restarted && this.connectionLost) {
					if ((null != myOpts && null != myOpts.getServerURIs())
							|| (null != myConnAct._failoverIp && null != myConnAct._clientID)) {
						this.waitForReconnection(myConnAct._failoverMaxMilli);
						lEndTime = System.currentTimeMillis() + millis;
						restarted = true;
					} else {
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+32),
								"Cannot receive messages on a disconnected Connection");
					}
				}
			}
		}
		return result;
	}
	
	private synchronized int nextID() {
		if (++id > 32767) id = 1;
		return id;
	}
	
	public void send(MyMessage message, String topic, Boolean retain, String QoS, boolean waitForAck, long millis) throws IsmTestException {
		send(message, topic, retain, QoS, waitForAck, millis, -1);
	}
	
	
	
	/*
	 * Publish a message
	 * The topic, reetain, and QoS fields can be specified here.  If they are null the suttings 
	 * in the message are used.
	 */
	public void send(MyMessage message, String topic, Boolean retain, String QoS,  boolean waitForAck, long millis, int expectedrc) throws IsmTestException  {
		switch (connType) {
		case ISMWS:
			try {
				wsConn.send((IsmWSMessage)message.getMessage());
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			break;
			
		case ISMMQTT:
			try {
				if (QoS != null) 
					message.setQos(Integer.valueOf(QoS));
				if (topic != null)
				    message.setTopic(topic);
				if (retain != null)
				    message.setRetained(retain);
				IsmMqttMessage smsg = (IsmMqttMessage)message.getMessage();
				IsmMqttResult res = wsMqttConn.publish(smsg, false);
				if (waitForAck) {
	                long endTime = System.currentTimeMillis() + millis;
	                while (isConnected() && !res.isComplete() && System.currentTimeMillis() < endTime) {
	                    sleep(10);
	                }
	                if (!res.isComplete()) {
	                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
	                            "Timed out waiting for ACK complete on send message");
	                } else {
	                    int rc = res.getReasonCode();
	                    if (rc != 0) {
	                        if (res.getReason() != null)
	                            res.setReason(MqttRC.toString(rc));
	                        if (rc == 255) 
	                            throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8), reason);
	                        else
	                            throw new IsmTestException("ISMTEST"+(Constants.MQTTRC + rc),
	                                res.getReason() + " (" + rc + ")");
	                    }    
	                }
				}
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			break;
			
		case JSONWS:
		case JSONTCP:
			{
				JsonMessage msg = (JsonMessage)message.getMessage(true);
				if (topic != null)
				    msg.setTopic(topic);
				if (retain != null)
				    msg.setRetain(retain);
				if (null != QoS) {
					msg.setQos(Integer.valueOf(QoS));
				}
				try {
					if (msg.getQoS() > 0) {
						jsonConn.publish(msg, nextID());
					} else {
						jsonConn.publish(msg);
					}
				} catch (IOException e) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
							e.getMessage()+" thrown attempting to send message", e);
				}
			}
			break;
			
		case MQMQTT:
			MqttTopic mqTopic = mqttConn.getTopic(topic);
			MqttDeliveryToken mqToken;
			try {
				// Send a copy of the message so we can change it immediately
				MqttMessage msg = (MqttMessage)message.getMessage(message.isIncludeQoS());
				MqttMessage msg2 = new MqttMessage(msg.getPayload());
				if (null == QoS) {
					msg2.setQos(msg.getQos());
				} else {
					msg2.setQos(Integer.valueOf(QoS));
				}
				if (retain != null)
				    msg2.setRetained(retain);
				mqToken = mqTopic.publish(msg2);
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			if (waitForAck) {
			    long lEndTime = System.currentTimeMillis() + millis;
				while (!mqToken.isComplete() && (System.currentTimeMillis() < lEndTime)) {
				    sleep(10);
					if (!isConnected()) 
					    break;
				}
				if (!mqToken.isComplete()) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
							"Timed out wiating for ACK complete on send message");
				}
			}
			break;
			
		case PAHO:
			org.eclipse.paho.client.mqttv3.IMqttDeliveryToken pahoToken = null;
			org.eclipse.paho.client.mqttv3.MqttMessage msg = (org.eclipse.paho.client.mqttv3.MqttMessage)message.getMessage(message.isIncludeQoS());
			org.eclipse.paho.client.mqttv3.MqttMessage msg2 = new org.eclipse.paho.client.mqttv3.MqttMessage(msg.getPayload());
			if (null == QoS) {
				msg2.setQos(msg.getQos());
				QoS = ""+msg.getQos();
			} else {
				msg2.setQos(Integer.valueOf(QoS));
			}
			msg2.setRetained(retain == null ? msg.isRetained() : retain);
			try {
				// Send a copy of the message so we can change it immediately
				synchronized (this) {
					int tokens;
					do {
						tokens = pahoConn.getPendingDeliveryTokens().length;
						if (tokens > inFlightMax) {
							inFlightMax = tokens;
							_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.MYCONNECTION+33)
									, "Max in flight has hit "+tokens);
						}
					} while (tokens > (myOpts.getMaxInflight() - 2));
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
							, "ISMTEST"+(Constants.MYCONNECTION+17)
							, "Publishing message at QoS="+QoS+" on topic '"+topic+"'");
					pahoToken = pahoConn.publish(topic, msg2);
				}
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
						, "ISMTEST"+(Constants.MYCONNECTION+19)
						, "this.connectionLost="+this.connectionLost);
				if (this.connectionLost && null != myConnAct._failoverIp
						&& null != myConnAct._clientID) {
					waitForReconnection(myConnAct._failoverMaxMilli);
					try {
						synchronized(this) {
							pahoToken = pahoConn.publish(topic,msg2);
						}
					} catch (org.eclipse.paho.client.mqttv3.MqttException e1) {
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
								e1.getMessage()+" thrown attempting to send message", e1);
					}
				} else throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			if (waitForAck) {
			    long lEndTime = System.currentTimeMillis() + millis;
				while (!pahoToken.isComplete() && (System.currentTimeMillis() < lEndTime)) {
				    sleep(10);
					if (!isConnected()) 
					    break;
				}
				if (!pahoToken.isComplete()) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
							"Timed out waiting for ACK complete on send message");
				}
			}
			break;
			
		case PAHOV5:
    		{
                org.eclipse.paho.mqttv5.client.IMqttToken v5Token = null;
                /*  Make a copy of the message so we can change it immediately */
                org.eclipse.paho.mqttv5.common.MqttMessage v5msg = 
                        (org.eclipse.paho.mqttv5.common.MqttMessage)message.getMessage(message.isIncludeQoS(), true);
                if (null == QoS) {
                    QoS = ""+v5msg.getQos();
                } else {
                    v5msg.setQos(Integer.valueOf(QoS));
                }
                if (retain != null)
                    v5msg.setRetained(retain);
                try {
                    synchronized (this) {
                        int tokens;
                        do {
                            tokens = pahoConnv5.getPendingTokens().length;
                            if (tokens > inFlightMax) {
                                inFlightMax = tokens;
                                //_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.MYCONNECTION+33)
                                //        , "Max in flight has hit "+tokens);
                            }
//                            Thread.sleep(10);
                        } while (tokens > (myOpts.getMaxInflight() - 2));
                        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                                , "ISMTEST"+(Constants.MYCONNECTION+17)
                                , "Publishing message at QoS="+QoS+" on topic '"+topic+"'");
                        v5Token = pahoConnv5.publish(topic, v5msg);
                    }
                } catch (Exception e) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                            , "ISMTEST"+(Constants.MYCONNECTION+19)
                            , "this.connectionLost="+this.connectionLost);
                    if (this.connectionLost && null != myConnAct._failoverIp
                            && null != myConnAct._clientID) {
                        waitForReconnection(myConnAct._failoverMaxMilli);
                        try {
                            synchronized(this) {
                                v5Token = pahoConnv5.publish(topic, v5msg);
                            }
                        } catch (Exception e1) {
                            throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
                                    e1.getMessage()+" thrown attempting to send message", e1);
                        }
                    } else throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
                            e.getMessage()+" thrown attempting to send message", e);
                }
                if (waitForAck) {
                    long lEndTime = System.currentTimeMillis() + millis;
                    while (!v5Token.isComplete() && (System.currentTimeMillis() < lEndTime)) {
                        sleep(10);
                        if (!isConnected()) 
                            break;
                    }
                    if (!v5Token.isComplete()) {
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
                                "Timed out waiting for ACK complete on send message");
                    } else {
                    	if (expectedrc != -1){
                    		boolean rcmatch = false;
                    		int[] rclist = v5Token.getReasonCodes();
                        	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                                    , "ISMTEST"+(Constants.MYCONNECTION+40)
                                    , "publish rc: "+ Arrays.toString(rclist));
                        	if(rclist != null) {
                        		for(int i=0; i < rclist.length; i++) {
                            		if (rclist[i] == expectedrc) {
                            			rcmatch = true;
                            			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                                                , "ISMTEST"+(Constants.MYCONNECTION+41)
                                                , "RC match found: expected: " + expectedrc + ", received: " + rclist[i]);
                            		}
                            	}
                        	} else {
                        		_trWriter.writeTraceLine(LogLevel.LOGLEV_WARN
                                        , "ISMTEST"+(Constants.MYCONNECTION+41)
                                        , "No return codes on ACKs found.");
                        	}
                        	
                        	if (rcmatch == false){
                        		throw new IsmTestException("ISMTEST" + (Constants.MYCONNECTION+42),
                        				"Expected RC not found, expected " + expectedrc + ", received " + Arrays.toString(rclist));
                        	}
                    	}
                    	
                    }
                }
                break;
    		}
		}
	}
		
	/*
	 * Send a message with no overrides
	 */
	public void send(MyMessage message, boolean waitForAck, long millis) throws IsmTestException  {
		switch (connType) {
		case ISMWS:
			try {
				wsConn.send((IsmWSMessage)message.getMessage());
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			break;
			
		case ISMMQTT:
			try {
				IsmMqttResult res = wsMqttConn.publish((IsmMqttMessage)message.getMessage(), false);
				if (waitForAck) {
                    long endTime = System.currentTimeMillis() + millis;
                    while (isConnected() && !res.isComplete() && System.currentTimeMillis() < endTime) {
                        sleep(10);
                    }
                    if (!res.isComplete()) {
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
                                "Timed out waiting for ACK complete on send message");
                    } else {
                        int rc = res.getReasonCode();
                        if (rc != 0) {
                            if (res.getReason() != null)
                                res.setReason(MqttRC.toString(rc));
                            throw new IsmTestException("ISMTEST"+(Constants.MQTTRC + rc),
                               res.getReason() + " (" + rc + ")");
                        }    
                    }
                }
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			break;
			
		case JSONWS:
		case JSONTCP:
			try {
				jsonConn.publish(message.getTopic(), message.getPayload());
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			break;
			
		case MQMQTT:
			MqttTopic mqTopic = message.getMqttTopic();
			if (null == mqTopic) {
				mqTopic = mqttConn.getTopic(message.getTopic());
			}
			MqttDeliveryToken mqToken;
			try {
				// Send a copy of the message so we can change it immediately
				MqttMessage msg = (MqttMessage)message.getMessage(message.isIncludeQoS());
				MqttMessage msg2 = new MqttMessage(msg.getPayload());
				msg2.setQos(msg.getQos());
				msg2.setRetained(msg.isRetained());
				mqToken = mqTopic.publish(msg2);
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			if (waitForAck) {
			    long lEndTime = System.currentTimeMillis() + millis;
				while (!mqToken.isComplete() && (System.currentTimeMillis() < lEndTime)) {
				    sleep(10);
					if (!isConnected()) 
					    break;
				}
				if (!mqToken.isComplete()) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
							"Timed out wiating for ACK complete on send message");
				}
			}
			break;
			
		case PAHO:
			String pahoTopic = message.getTopic();
			org.eclipse.paho.client.mqttv3.IMqttDeliveryToken pahoToken = null;
			org.eclipse.paho.client.mqttv3.MqttMessage msg = (org.eclipse.paho.client.mqttv3.MqttMessage)message.getMessage(message.isIncludeQoS());
			org.eclipse.paho.client.mqttv3.MqttMessage msg2 = new org.eclipse.paho.client.mqttv3.MqttMessage(msg.getPayload());
			msg2.setQos(msg.getQos());
			msg2.setRetained(msg.isRetained());
			try {
				// Send a copy of the message so we can change it immediately
				synchronized (this) {
					int tokens;
					do {
						tokens = pahoConn.getPendingDeliveryTokens().length;
					} while (tokens > 8);
					pahoToken = pahoConn.publish(pahoTopic,msg2);
				}
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				if (this.connectionLost && null != myConnAct._failoverIp
						&& null != myConnAct._clientID) {
					waitForReconnection(myConnAct._failoverMaxMilli);
					try {
						pahoToken = pahoConn.publish(pahoTopic,msg2);
					} catch (org.eclipse.paho.client.mqttv3.MqttException e1) {
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
								e1.getMessage()+" thrown attempting to send message", e1);
					}
				} else throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						e.getMessage()+" thrown attempting to send message", e);
			}
			if (waitForAck) {
			    long lEndTime = System.currentTimeMillis() + millis;
				while (!pahoToken.isComplete() && (System.currentTimeMillis() < lEndTime)) {
				    sleep(10);
					if (!isConnected()) 
					    break;
				}
				if (!pahoToken.isComplete()) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
							"Timed out wiating for ACK complete on send message");
				}
			}
			break;
			
		case PAHOV5:
        {
            org.eclipse.paho.mqttv5.client.IMqttToken v5Token = null;
            org.eclipse.paho.mqttv5.common.MqttMessage v5msg = 
                    (org.eclipse.paho.mqttv5.common.MqttMessage)message.getMessage(message.isIncludeQoS(), true);
            try {
                // Send a copy of the message so we can change it immediately
                synchronized (this) {
                    int tokens;
                    do {
                        tokens = pahoConnv5.getPendingTokens().length;
                        if (tokens > inFlightMax) {
                            inFlightMax = tokens;
                            //_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.MYCONNECTION+33)
                            //        , "Max in flight has hit "+tokens);
                        }
                        Thread.sleep(10);
                    } while (tokens > (myOpts.getMaxInflight() - 2));
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                            , "ISMTEST"+(Constants.MYCONNECTION+17)
                            , "Publishing message at QoS=" + v5msg.getQos() + " on topic '"+topic+"'");
                    v5Token = pahoConnv5.publish(topic, v5msg, null, null);
                }
            } catch (Exception e) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                        , "ISMTEST"+(Constants.MYCONNECTION+19)
                        , "this.connectionLost="+this.connectionLost);
                if (this.connectionLost && null != myConnAct._failoverIp
                        && null != myConnAct._clientID) {
                    waitForReconnection(myConnAct._failoverMaxMilli);
                    try {
                        synchronized(this) {
                            v5Token = pahoConnv5.publish(topic, v5msg, null, null);
                        }
                    } catch (Exception e1) {
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
                                e1.getMessage()+" thrown attempting to send message", e1);
                    }
                } else throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
                        e.getMessage()+" thrown attempting to send message", e);
            }
            if (waitForAck) {
                long lEndTime = System.currentTimeMillis() + millis;
                while (!v5Token.isComplete() && (System.currentTimeMillis() < lEndTime)) {
                    sleep(10);
                    if (!isConnected()) 
                        break;
                }
                if (!v5Token.isComplete()) {
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+9), 
                            "Timed out waiting for ACK complete on send message");
                } else {
                	int[] rclist = v5Token.getReasonCodes();
                	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                            , "ISMTEST"+(Constants.MYCONNECTION+19)
                            , "publish rc: "+ rclist);
                }
            }
            break;
        }
		}
	}

	
	public void setVerbose(boolean verbose) {
		switch (connType) {
		case ISMMQTT:
			wsMqttConn.setVerbose(verbose);
			break;
		case JSONWS:
		case JSONTCP:
			jsonConn.setVerbose(verbose);
			break;
		}
	}
	
	public void setUserInfo(String user, String password) {
		switch (connType) {
		case ISMMQTT:
			wsMqttConn.setUser(user, password);
			break;
		case JSONWS:
		case JSONTCP:
			
		}
	}
	
	public MqttTopic getMqTopic(String topic) {
		if (MQMQTT == connType) {
			mqttConn.getTopic(topic);
		}
		return null;
	}
	
//	public org.eclipse.paho.client.mqttv3.MqttTopic getPahoTopic(String topic) {
//		if (PAHO == connType) {
//			return ((org.eclipse.paho.client.mqttv3.MqttAsyncClient)connection).getTopic(topic);
//		}
//		return null;
//	}
	
	public int getCount() {
		return messagesReceived;
	}
	
	public int getMessagesReceived() {
		return messagesReceived;
	}
	
	public int getMessagesReceived(String topic) {
		Integer count = messagesPerTopic.get(topic);
		if (null == count) return 0;
		return count;
	}
	
	public Map<String, Integer> getMessagesPerTopic() {
		return messagesPerTopic;
	}
	
	public int countPendingDeliveryTokens() {
		switch (connType) {
		case MQMQTT:
			return mqttConn.getPendingDeliveryTokens().length;
		case PAHO:
			return pahoConn.getPendingDeliveryTokens().length;
		case PAHOV5:
			return pahoConnv5.getPendingTokens().length;
		}
		return 0;
	}
	
	public int checkPendingDeliveryTokens() {
		int counter = 0;
		switch (connType) {
		case MQMQTT:
			MqttDeliveryToken mqTokens[] = mqttConn.getPendingDeliveryTokens();
			counter = mqTokens.length;
			if (counter > 0) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
						, "ISMTEST"+(Constants.MYCONNECTION+11)
						, "There are "+counter+" incomplete delivery tokens.");
				for (int i=0; i<counter; i++) {
					try {
						MqttMessage msg = mqTokens[i].getMessage();
						String msgText = new String(msg.getPayload());
						String duplicate = msg.isDuplicate() ? "duplicate " : "";
						_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
								, "ISMTEST"+(Constants.MYCONNECTION+10)
								, "Incomplete delivery token for "+duplicate+"message "
								+msgText);
					} catch (Exception e) {}
				}
			}
			break;
		case PAHO:
			org.eclipse.paho.client.mqttv3.IMqttDeliveryToken pahoTokens[] 
					= pahoConn.getPendingDeliveryTokens();
			counter = pahoTokens.length;
			if (counter > 0) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
						, "ISMTEST"+(Constants.MYCONNECTION+11)
						, "There are "+counter+" incomplete delivery tokens.");
				for (int i=0; i<counter; i++) {
					try {
						org.eclipse.paho.client.mqttv3.MqttMessage msg = pahoTokens[i].getMessage();
						String duplicate = msg.isDuplicate() ? "duplicate " : "";
						String msgTopics[] = pahoTokens[i].getTopics();
						String msgText = new String(msg.getPayload(), StandardCharsets.UTF_8);
						_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
								, "ISMTEST"+(Constants.MYCONNECTION+34)
								, "Incomplete delivery token for "+duplicate+"message "
								+msgText);
						for (int j=0; j<msgTopics.length; j++) {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG
								, "ISMTEST"+(Constants.MYCONNECTION+35)
								, "  on topic["+i+"]:'"+msgTopics[i]+"'.");
						}
					} catch (Exception e) {}
				}
			}
			break;
		case PAHOV5:
	        org.eclipse.paho.mqttv5.client.IMqttToken v5Tokens[] =
	            pahoConnv5.getPendingTokens();
	        counter = v5Tokens.length;
			break;
		}
		return counter;
	}
   
	
	/**
	 * increment the total message count and (if not null) the count for topic
	 * @param topic - null or topic string
	 * @return		- new total message count
	 */
	public int incrementMessages(String topic) {
		Integer count = messagesPerTopic.get(topic);
		if (null != topic) {
			if (null == count) {
				messagesPerTopic.put(topic, new Integer(1));
			} else {
				count += 1;
				messagesPerTopic.put(topic, count);
			}
		}
		return messagesReceived++;
	}
	
	public boolean connect(MyConnectionOptions opts) throws IOException, IsmTestException {
		boolean result = false;
		myOpts = opts;
		switch (connType) {
		case ISMWS:
			wsConn.connect();
			break;
		case ISMMQTT:
			try {
				if (myConnAct._SSL) {
//					System.out.println("setting ism mqtt connection to secure");
					_trWriter.writeTraceLine(LogLevel.LOGLEV_STATUS, "ISMTEST","setting ism mqtt connection to secure");
					if (null != myOpts) {
						_trWriter.writeTraceLine(LogLevel.LOGLEV_STATUS, "ISMTEST","setting ism mqtt connection to secure: "+myOpts.getIsmOpts().getTlsProtocol());
					    wsMqttConn.setSecure(myOpts.getIsmOpts().getTlsProtocol());
					    String servername = myOpts.getIsmOpts().getSniServername();
					    if (servername != null && servername.length() > 0) {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_STATUS, "ISMTEST","setting ism mqtt connection SNI servername to " + servername);
					    	wsMqttConn.setHost(servername);
					    } else {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_STATUS, "ISMTEST","NOT setting ism mqtt connection SNI servername");
					    }
					}
				}
				mqttVersion = wsMqttConn.getVersion();
				IsmMqttResult res = wsMqttConn.connect(true, false);
				int rc = res.getReasonCode();
				if (rc != 0) {
				    int vers = wsMqttConn.getVersion();
				    String reason = res.getReason();
				    int rc5 = vers >= 5 ? rc : MqttRC.connackToMqttRC(rc);
				    if (reason == null) 
				        reason = MqttRC.toString(rc5);
				    reason = "RC=" + rc + ' ' + reason + " attempting to connect"; 
				    /* Compatible RC from versions 3 and 4 */
                    if (rc5 == MqttRC.UnknownVersion) 
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+21), reason);
                    if (rc5 == MqttRC.IdentifierNotValid)
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+22), reason);
                    if (rc5 == MqttRC.ServerUnavailable) 
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+23), reason);
                    if (rc5 == MqttRC.BadUserPassword) 
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+24), reason); 
                    if (rc5 == MqttRC.NotAuthorized) 
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+25), reason);
                    /* All other RCs */
                    if (vers >= 5 && rc > 0 && rc < 255)
                        throw new IsmTestException("ISMTEST"+(Constants.MQTTRC+rc5), reason);
                    else
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+2),
                            "errow attempting to connect to " +wsMqttConn.getAddress() + 
                                " rc=" + rc + " reason=" + reason);
				}  
                result = wsMqttConn.isSessionPresent();
			} catch (IOException e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
						"ISMTEST"+(Constants.MYCONNECTION+8),
						"Exception trying to connect");
				throw e;
			}
			break;
		case JSONWS:
		case JSONTCP:
			jsonConn.setUser(myOpts.getUserName(), myOpts.getPassword());
			jsonConn.connect();
			break;
		case MQMQTT:
			try {
				mqttConn.connect(opts.getMqOpts());
				_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT,
						"ISMTEST"+(Constants.MYCONNECTION+16), 
						"Connected to "+mqttConn.getServerURI());
			} catch (MqttException e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
						"ISMTEST"+(Constants.MYCONNECTION+8),
						"Exception trying to connect to: "+mqttConn.getServerURI());
				Throwable t = TestUtils.getRootCause(e);
				if (TestUtils.checkForCause(e, "javax.net.ssl.SSLHandshakeException")) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
							t.getMessage()+" thrown attempting to connect", e);
				}
				if ("Bad user name or password (4)".equals(e.toString())) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+13),
						e.getMessage()+" thrown attempting to connect", e);
				}
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+2),
						e.getMessage()+" thrown attempting to connect to "+mqttConn.getServerURI(), e);
			}
			break;
		case PAHO:
			try {
				/*if (null != myConnAct._HAipList) {
					String args[] = new String[myConnAct._HAipList.length];
					for (int i=0; i<myConnAct._HAipList.length; i++) {
						args[i] = (myConnAct._SSL ? "ssl" : "tcp")+"://"+myConnAct._HAipList[i]+":"+myConnAct._port;
					}
					myOpts.getPahoOpts().setServerURIs(args);
				}*/
			    mqttVersion = opts.getMqttVersion();
				IMqttToken connToken = pahoConn.connect(opts.getPahoOpts());
				try {
					connToken.waitForCompletion();
					org.eclipse.paho.client.mqttv3.MqttException e = connToken.getException();
					if (null != e) {
						throw e;
					}
					result = connToken.getSessionPresent();
					// System.out.println("connect: SessionPresent="+result);
				} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
							"ISMTEST"+(Constants.MYCONNECTION+8),
							"Exception trying to connect to: "+pahoConn.getServerURI()+" "+e.getMessage()+" ClientID: "+clientID);
					Throwable t = TestUtils.getRootCause(e);
					if (TestUtils.checkForCause(e, "javax.net.ssl.SSLHandshakeException")) {
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
								t.getMessage()+" thrown attempting to connect", e);
					}
					int reasonCode = e.getReasonCode();
					if (1 == reasonCode) {
						// Invalid protocol version
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+21),
							e.getMessage()+" thrown attempting to connect", e);
					} else if (2 == reasonCode) {
						// Invalid client ID
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+22),
							e.getMessage()+" thrown attempting to connect", e);
					} else if (3 == reasonCode) {
						// Broker unavailable
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+23),
							e.getMessage()+" thrown attempting to connect", e);
					} else if (4 == reasonCode) {
						// Bad user name or password
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+24),
							e.getMessage()+" thrown attempting to connect", e);
					} else if (5 == reasonCode) {
						// Not authorized to connect
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+25),
							e.getMessage()+" thrown attempting to connect", e);
					}
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+2),
							e.getMessage()+" thrown attempting to connect to "+pahoConn.getServerURI(), e);
				}
				_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT,
						"ISMTEST"+(Constants.MYCONNECTION+16), 
						"Connected to "+pahoConn.getServerURI());
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
						"ISMTEST"+(Constants.MYCONNECTION+8),
						"Exception trying to connect to: "+pahoConn.getServerURI());
				Throwable t = TestUtils.getRootCause(e);
				if (TestUtils.checkForCause(e, "javax.net.ssl.SSLHandshakeException")) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						t.getClass().getName()+" thrown attempting to connect to "+pahoConn.getServerURI()
						, e);
				}
				if ("Bad user name or password (4)".equals(e.toString())) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+13),
						"Bad user name or password (4)"+" thrown attempting to connect to "+pahoConn.getServerURI()
						, e);
				}
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+2),
						t.getMessage()+" thrown attempting to connect to "+pahoConn.getServerURI(), e);
			}
			break;
		case PAHOV5:
			try {
				/*if (null != myConnAct._HAipList) {
					String args[] = new String[myConnAct._HAipList.length];
					for (int i=0; i<myConnAct._HAipList.length; i++) {
						args[i] = (myConnAct._SSL ? "ssl" : "tcp")+"://"+myConnAct._HAipList[i]+":"+myConnAct._port;
					}
					myOpts.getPahoOpts().setServerURIs(args);
				}*/
				org.eclipse.paho.mqttv5.client.IMqttToken connToken = pahoConnv5.connect(opts.getPahoOptsv5());
				try {
					connToken.waitForCompletion();
					org.eclipse.paho.mqttv5.common.MqttException e = connToken.getException();
					if (null != e) {
						throw e;
					}
					result = connToken.getSessionPresent();
					sessionPresent = connToken.getSessionPresent();
					connectionToken = connToken;
					// System.out.println("connect: SessionPresent="+result);
				} catch (org.eclipse.paho.mqttv5.common.MqttException e) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
							"ISMTEST"+(Constants.MYCONNECTION+8),
							"Exception trying to connect to: "+pahoConnv5.getServerURI()+" "+e.getMessage()+" ClientID: "+clientID);
					Throwable t = TestUtils.getRootCause(e);
					if (TestUtils.checkForCause(e, "javax.net.ssl.SSLHandshakeException")) {
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
								t.getMessage()+" thrown attempting to connect", e);
					}
					int reasonCode = e.getReasonCode();
		            String reason = null; 
		            if (connToken != null && connToken instanceof org.eclipse.paho.mqttv5.client.MqttToken) {
		                /* TODO wait for fix from pahov5 */
		                // MqttProperties mprops = ((org.eclipse.paho.mqttv5.client.MqttToken)connToken).getMessageProperties(); 
		                // if (mprops != null)
		                //    reason = mprops.getReasonString();
		            }
		            if (reason == null)
		                reason = e.getMessage();
		            else
		                reason += "(0x" + Integer.toHexString(reasonCode).toUpperCase() + ")";

					if (MqttRC.UnknownVersion == reasonCode || 1 == reasonCode) {
						// Invalid protocol version
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+21),
							reason + " thrown attempting to connect", e);
					} else if (MqttRC.IdentifierNotValid == reasonCode) {
						// Invalid client ID
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+22),
							reason + " thrown attempting to connect", e);
					} else if (MqttRC.ServerUnavailable == reasonCode) {
						// Broker unavailable
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+23),
					        reason + " thrown attempting to connect", e);
					} else if (MqttRC.BadUserPassword == reasonCode) {
						// Bad user name or password
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+24),
							reason + " thrown attempting to connect", e);
					} else if (MqttRC.NotAuthorized == reasonCode) {
						// Not authorized to connect
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+25),
							reason + " thrown attempting to connect", e);
					}
					if (reasonCode < 254) {
					    throw new IsmTestException("ISMTEST"+(Constants.MQTTRC + reasonCode),
							reason + " thrown attempting to connect to "+pahoConnv5.getServerURI(), e);
					} else {
					    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+2),
                            e.getMessage()+" thrown attempting to connect to "+pahoConnv5.getServerURI(), e);
					}
				}
				_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT,
						"ISMTEST"+(Constants.MYCONNECTION+16), 
						"Connected to "+pahoConnv5.getServerURI());
			} catch (org.eclipse.paho.mqttv5.common.MqttException e) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,
						"ISMTEST"+(Constants.MYCONNECTION+8),
						"Exception trying to connect to: "+pahoConnv5.getServerURI());
				Throwable t = TestUtils.getRootCause(e);
				if (TestUtils.checkForCause(e, "javax.net.ssl.SSLHandshakeException")) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+8),
						t.getClass().getName()+" thrown attempting to connect to "+pahoConnv5.getServerURI()
						, e);
				}
				if ("Bad user name or password (4)".equals(e.toString())) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+13),
						"Bad user name or password (4)"+" thrown attempting to connect to "+pahoConnv5.getServerURI()
						, e);
				}
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+52),
						t.getMessage()+" thrown attempting to connect to "+pahoConnv5.getServerURI(), e);
			}
		}
		return result;
	}
	
	private String clientName(int clientType) {
		String result = "Unknown";
		switch (clientType) {
		case ISMWS:
			return "ISM Web Sockets client";
		case ISMMQTT:
			return "ISM MQTT client";
		case ISMWSMQTT:
			return "ISM MQTT Web Sockets client";
		case MQMQTT: 
			return "MQ MQTT client (Paho predecessor)";
		case PAHO:
			return "Paho MQTT client";
		case PAHOV5:
			return "Paho MQTTv5 client";
		case JSONWS:
			return "JSON Web Sockets client";
		case JSONTCP:
			return "JSON TCP/IP client";
		}
		return result;
	}
	
	private class JsonTopic {
		String name;
		String topic;
		boolean durable;
	}
	
	private JsonTopic getJsonTopic(String topic, boolean durable) {
		JsonTopic jsTopic = subscriptions.get(topic);
		if (null == jsTopic) {
			jsTopic = new JsonTopic();
			jsTopic.durable = durable;
			jsTopic.name = (null == clientID ? "subscribe" : clientID+".")+ ++lastSubscribe;
			jsTopic.topic = topic;
			subscriptions.put(topic, jsTopic);
		}
		return jsTopic;
	}
	
	public int subscribe(String topic, int QoS, boolean durable) throws IsmTestException {
        _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.SUBSCRIBE+10), "Subscribing to topic: " + topic + " at QoS: "+ QoS);
		switch (connType) {
		case JSONWS:
		case JSONTCP:
			JsonTopic jsTopic = getJsonTopic(topic, durable);
			int ID = 0;//nextID();
			try {
				int rc = jsonConn.subscribe(jsTopic.name, false, topic, durable, 0, true, QoS, ID);
				if (0 != rc) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+03)
						, "Subscribe failed with rc="+rc);
				}
				return rc;
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+26)
					, "durable is not supported for client "+clientName(connType));
		}
	}
	
	/* 
	 * Older form subscribe without subid and properties
	 */
	public int[] subscribe(String topic, int QoS) throws IsmTestException {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.SUBSCRIBE+10),
        		"Subscribing to topic: " + topic + " at QoS: "+ QoS);
	    return subscribe(topic, QoS, null, null);
	}
	
	/*
	 * Full MQTTv5 single topic subscribe
	 */
	public int[] subscribe(String topic, int subopt, Integer subid, List<MqttUserProp> props) throws IsmTestException {
		int[] grantedQoS = null;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.SUBSCRIBE+10),
        		"Subscribing to topic: " + topic);
		switch (connType) {
		case ISMMQTT:
			try {
				IsmMqttResult res;
	            List<IsmMqttUserProperty> ismprop = toIsmProps(props);
	            res = wsMqttConn.subscribe(new String[]{topic}, new int[]{subopt}, subid == null ? 0 : subid, true, ismprop);	            	
                byte[] rcs = res.getReasonCodes();
                if (rcs != null) {
                    grantedQoS = new int[rcs.length];
                    for (int i=0; i<rcs.length; i++) {
                        grantedQoS[i] = rcs[i]&0xff;
                    }    
                }
			} catch (Exception e) {
				String emsg = e.getMessage();
				if (emsg != null && emsg.length() > 0) {
					/* Work around: For SNI failure tests, we use a call to subscribe
					 * to check whether the connect failed and caused a disconnect.
					 * For these tests, we want to make sure we see 2599 instead of 2503.
					 */
					if (emsg.contains("Connection closed")) {
						throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+99),
								e.getMessage()+" thrown attempting to subscribe", e);
					}
				}
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		case JSONWS:
		case JSONTCP:
			try {
				JsonTopic jsTopic = getJsonTopic(topic, false);
				int rc = jsonConn.subscribe(jsTopic.name, topic, false, subopt, nextID());
				if (0 != rc) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+03)
						, "Subscribe failed with rc="+rc);
				}
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		case MQMQTT:
			try {
				mqttConn.subscribe(topic, subopt);
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		case PAHO:
			try {
				IMqttToken subToken = pahoConn.subscribe(topic, subopt);
				subToken.waitForCompletion();
				org.eclipse.paho.client.mqttv3.MqttException e = subToken.getException();
				if (null != e) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
							e.getMessage()+" thrown attempting to subscribe", e);
				}
				grantedQoS = subToken.getGrantedQos();
				// System.out.println("subscribe: grantedQoS="+grantedQoS[0]);
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		case PAHOV5:
	        org.eclipse.paho.mqttv5.common.MqttSubscription [] subs = 
	                 new  org.eclipse.paho.mqttv5.common.MqttSubscription[1];
            subs[0] = new  org.eclipse.paho.mqttv5.common.MqttSubscription(topic, subopt&3);
            if ((subopt&4) != 0)
                subs[0].setNoLocal(true);
            if ((subopt&8) != 0)
                subs[0].setRetainAsPublished(true);
            if ((subopt&0x30) != 0)
                subs[0].setRetainHandling((subopt&0x30)>>4);
	        
            MqttProperties mprops2 = new MqttProperties();
            if (subid != null ) {
            	mprops2.setSubscriptionIdentifier(subid);
            }
            
			org.eclipse.paho.mqttv5.client.IMqttToken subToken = null;
			try {	
	            subToken = pahoConnv5.subscribe(subs, null, null, mprops2);
				subToken.waitForCompletion();
				grantedQoS = subToken.getReasonCodes();
			} catch (org.eclipse.paho.mqttv5.common.MqttException me) {
			    int rc = me.getReasonCode();
			    String reason = null; 
			    if (subToken != null && subToken instanceof org.eclipse.paho.mqttv5.client.MqttToken) {
                   	try{
			    		MqttProperties mprops = ((org.eclipse.paho.mqttv5.client.MqttToken)subToken).getResponseProperties();
	                    if (mprops != null)
	                        reason = mprops.getReasonString();
			    	} catch (NullPointerException e){
			    		_trWriter.writeTraceLine(LogLevel.LOGLEV_WARN, "ISMTEST"+(Constants.MYCONNECTION+3), "WARNING: Subscribe failed, getResponseProperties null");
			    	}			    	
			    }
			    if (reason == null)
			        reason = me.getMessage();
			    else
			        reason += "(0x" + Integer.toHexString(rc).toUpperCase() + ")";
			    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3), reason);
			} catch (Exception e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		}
		return grantedQoS;
	}
	
	/*
	 * Convert a set of test driver user properties to a list of ISM client properties
	 */
	public static List<IsmMqttUserProperty> toIsmProps(List<MqttUserProp> props) {
	    if (props == null)
	        return null;
	    List<IsmMqttUserProperty> ret = new Vector<IsmMqttUserProperty>();
        Iterator<MqttUserProp> it = props.iterator();
        while (it.hasNext()) {
            MqttUserProp prp = it.next();
            ret.add(new IsmMqttUserProperty(prp.getKey(), prp.getValue()));
        }
	    return ret;
	}
	
	/*
	 * Convert a set of test drever user properteis to a list of pahov5 user properties
	 */
	public static List<UserProperty> toPahoProps(List<MqttUserProp> props) {
	       if (props == null || props.size() == 0)
	            return null;
	        List<UserProperty> ret = new Vector<UserProperty>();
	        Iterator<MqttUserProp> it = props.iterator();
	        while (it.hasNext()) {
	            MqttUserProp prp = it.next();
	            ret.add(new UserProperty(prp.getKey(), prp.getValue()));
	        }
	        return ret;    
	}
	
	/*
	 * Older form of multi topic subscube without subid and properties 
	 */
	public int[] subscribe(String[] topic, int[] subopt) throws IsmTestException {
	    return subscribe(topic, subopt, null, null);
	}
	
	public int[] subscribe(String[] topics, int[] subopts, Integer subid, List<MqttUserProp> props) throws IsmTestException {
	    int  i;
		int[] grantedQoS = null;
		if (subopts == null)
		    subopts = new int[topics.length];
		switch (connType) {
		case ISMMQTT:
		    try {
		        List<IsmMqttUserProperty> ismprop = toIsmProps(props);
		        IsmMqttResult res = wsMqttConn.subscribe(topics, subopts, subid == null ? 0 : subid, true, ismprop);
		        byte[] rcs = res.getReasonCodes();
		        if (rcs != null) {
    		        grantedQoS = new int[rcs.length];
    		        for (i=0; i<rcs.length; i++) {
    		            grantedQoS[i] = rcs[i]&0xff;
    		        }    
		        }
            } catch (Exception e) {
			    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+3),
						e.getMessage()+"thrown attempting to subscribe", e);
			}
			break;
		case JSONWS:
		case JSONTCP:
			for (i=0; i<topics.length; i++) {
				int localQoS = 0;
				if (null != subopts && i<subopts.length) {
					localQoS = subopts[i];
				}
				subscribe(topics[i], localQoS);
			}
			break;
		case MQMQTT:
			try {
			    mqttConn.subscribe(topics, subopts);
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+5),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		case PAHO:
			try {            
				IMqttToken subToken = pahoConn.subscribe(topics, subopts);
				subToken.waitForCompletion();
				org.eclipse.paho.client.mqttv3.MqttException e = subToken.getException();
				if (null != e) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+5),
							e.getMessage()+" thrown attempting to subscribe", e);
				}
				grantedQoS = subToken.getGrantedQos();
				// System.out.println("subscribe: grantedQoS="+grantedQoS);
				// String qoses = " "+grantedQoS[0];
				// for (int i = 1; i<grantedQoS.length; i++) {
				//     qoses = qoses + ", " + grantedQoS[i];
				// }
				// System.out.println("subscribe: grantedQoS="+qoses);
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+5),
						e.getMessage()+" thrown attempting to subscribe", e);
			}
			break;
		case PAHOV5:
	        try {
	            List<UserProperty> pahoprop = toPahoProps(props);
                org.eclipse.paho.mqttv5.common.MqttSubscription [] subs = 
                    new  org.eclipse.paho.mqttv5.common.MqttSubscription[topics.length];
                for (i=0; i<topics.length; i++) {
                    subs[i] = new  org.eclipse.paho.mqttv5.common.MqttSubscription(topics[i], subopts[i]&3);
                    if ((subopts[i]&4) != 0)
                        subs[i].setNoLocal(true);
                    if ((subopts[i]&8) != 0)
                        subs[i].setRetainAsPublished(true);
                    if ((subopts[i]&0x30) != 0)
                        subs[i].setRetainHandling((subopts[i]&0x30)>>4);
                }    
                MqttProperties mprops = new MqttProperties();
//                if (subid != 0) {
//                    Vector<Integer> vi = new Vector<Integer>(1);
//                    vi.add(subid);
//                    mprops.setSubscriptionIdentifier(vi);
//                }    
                if (subid != null) {
                	mprops.setSubscriptionIdentifier(subid);
                }
                
                if (pahoprop != null) {
                    mprops.setUserProperties(pahoprop);
                }
                
                org.eclipse.paho.mqttv5.client.IMqttToken subToken = pahoConnv5.subscribe(subs, null, null, mprops);
                subToken.waitForCompletion();
                Exception e = subToken.getException();
                if (e != null) {
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+5),
                            e.getMessage()+" thrown attempting to subscribe", e);
                }
                grantedQoS = subToken.getGrantedQos();
            } catch (Exception e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+5),
                        e.getMessage()+" thrown attempting to subscribe", e);
            }
		}
		return grantedQoS;
	}
	
	public void subscribe(String[] topic, int[] QoS, boolean durable) throws IsmTestException {
		switch (connType) {
		case JSONWS:
		case JSONTCP:
			for (int i=0; i<topic.length; i++) {
				int localQoS = 0;
				if (null != QoS && i<QoS.length) {
					localQoS = QoS[i];
				}
				int rc = subscribe(topic[i], localQoS, durable);
				if (0 != rc) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+03)
						, "Subscribe failed with rc="+rc);
				}
			}
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+26)
					, "durable is not supported for client "+clientName(connType));
		}
	}
	
	public int [] unsubscribe(String topic) throws IsmTestException {
	    return unsubscribe(topic, null);
	}
	
	public int [] unsubscribe(String topic, List<MqttUserProp> props) throws IsmTestException {
	    int [] ret = null;
		switch (connType) {
		case ISMMQTT:
            try {
                List<IsmMqttUserProperty> ismprop = toIsmProps(props);
                IsmMqttResult res = wsMqttConn.unsubscribe(new String[] {topic}, true, ismprop);
                if (mqttVersion >= 5) {
                    ret = new int[] {res.getReasonCode()};
                }
                if (res.getThrowable() != null) {
                    Throwable e = res.getThrowable();
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                            e.getMessage()+" trying to unsubscribe.", e);    
                }
            } catch (IOException e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                        e.getMessage()+" trying to unsubscribe.", e);
            }
            break;
            
		case JSONWS:
		case JSONTCP:
			JsonTopic jsTopic = subscriptions.get(topic);
			if (null != jsTopic) {
				subscriptions.remove(topic);
				try {
					jsonConn.closeSubscription(jsTopic.name);
					if (jsTopic.durable) jsonConn.destroySubscription(jsTopic.name);
				} catch (IOException e) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
							e.getMessage()+" trying to unsubscribe.", e);
				}
			}
			break;
		case MQMQTT:
			try {
				mqttConn.unsubscribe(topic);
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
						e.getMessage()+" trying to unsubscribe.", e);
			}
			break;
		case PAHO:
			try {
				IMqttToken unsubToken = pahoConn.unsubscribe(topic);
				unsubToken.waitForCompletion();
				org.eclipse.paho.client.mqttv3.MqttException e = unsubToken.getException();
				if (null != e) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
							e.getMessage()+" trying to unsubscribe.", e);
				}
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
						e.getMessage()+" trying to unsubscribe.", e);
			}
			break;
		case PAHOV5:
            try {
                org.eclipse.paho.mqttv5.client.IMqttToken unsubToken = pahoConnv5.unsubscribe(topic);
                unsubToken.waitForCompletion();
                Exception e = unsubToken.getException();
                if (e != null) {
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                            e.getMessage()+" trying to unsubscribe.", e);
                }
                ret = unsubToken.getReasonCodes();
            } catch (Exception e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                        e.getMessage()+" trying to unsubscribe.", e);
            }
            break;
		case ISMWS:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
					"unsubscribe not supported for "+getConnType());
		}
		return ret;
	}
	
	/*
	 * Unsubscribe without properties
	 */
	public int [] unsubscribe(String[] topics) throws IsmTestException {
	    return unsubscribe(topics, null);
	}
	
	/*
	 * Unsubscribe with properties
	 */
	public int [] unsubscribe(String[] topics, List<MqttUserProp> props) throws IsmTestException {
	    int [] ret = null;
		switch (connType) {
		case ISMMQTT:
			try {
	            List<IsmMqttUserProperty> ismprop = toIsmProps(props);
				IsmMqttResult res = wsMqttConn.unsubscribe(topics, true, ismprop);
				if (mqttVersion >= 5) {
                    byte[] rcs = res.getReasonCodes();
                    if (rcs != null) {
                        ret = new int[rcs.length];
                        for (int i=0; i<rcs.length; i++) {
                            ret[i] = rcs[i]&0xff;
                        }    
                    }
				}
                if (res.getThrowable() != null) {
                    Throwable e = res.getThrowable();
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                            e.getMessage()+" trying to unsubscribe.", e);    
                }
			} catch (IOException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
						e.getMessage()+" trying to unsubscribe.", e);
			}
			break;
		case JSONWS:
		case JSONTCP:
			for (int i=0; i<topics.length; i++) {
				unsubscribe(topics[i]);
			}
			break;
		case MQMQTT:
			try {
				mqttConn.unsubscribe(topics);
			} catch (MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
						e.getMessage()+" trying to unsubscribe.", e);
			}
			break;
		case PAHO:
			try {
				IMqttToken unsubToken = pahoConn.unsubscribe(topics);
				unsubToken.waitForCompletion();
				org.eclipse.paho.client.mqttv3.MqttException e = unsubToken.getException();
				if (null != e) {
					throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
							e.getMessage()+" trying to unsubscribe.", e);
				}
			} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
						e.getMessage()+" trying to unsubscribe.", e);
			}
			break;
		case PAHOV5:
            try {
                org.eclipse.paho.mqttv5.client.IMqttToken unsubToken = pahoConnv5.unsubscribe(topics);
                unsubToken.waitForCompletion();
                Exception e = unsubToken.getException();
                if (e != null) {
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                            e.getMessage()+" trying to unsubscribe.", e);
                }
                ret = unsubToken.getReasonCodes();
            } catch (Exception e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
                        e.getMessage()+" trying to unsubscribe.", e);
            }
            break;
		case ISMWS:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+6),
					"unsubscribe not supported for "+getConnType());
		}
		return ret;
	}
	
	/*
	 * Legacy disconnect with no RC or reason
	 */
	public void disconnect() throws IOException, IsmTestException {
	    disconnect(0, null, 0, null);
	}
	
	/*
	 * Disconnect with rc, reason, and props
	 */
	public void disconnect(int rc, String reason, long sessionExpire, List<MqttUserProp> props) throws IOException, IsmTestException {
	    int tokens = countPendingDeliveryTokens();
        if (tokens > 0) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
                    , "ISMTEST"+(Constants.MYCONNECTION+12)
                    , "Trying to close connection with "+tokens+" delivery tokens incomplete");
        }
        if (rc != 0) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.MYCONNECTION+40),
                "Close connection: rc={0} reason={1}", rc, reason);
        }    
        
        if (isConnected()) {
            this.reasonCode = rc;
            this.reason = reason;
        }
        switch (connType) {
        case MQMQTT:
            try {
                mqttConn.disconnect();
            } catch (MqttException e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+7),
                        e.getMessage()+" trying to disconnect.", e);
            }
            break;
        case JSONWS:
        case JSONTCP:
            jsonConn.closeConnection();
            jsonConn.terminate();
            break;
        case PAHO:
            try {
                IMqttToken unsubToken = pahoConn.disconnect();
                unsubToken.waitForCompletion();
                org.eclipse.paho.client.mqttv3.MqttException e = unsubToken.getException();
                if (null != e) {
                    throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+7),
                            e.getMessage()+" trying to disconnect.", e);
                }
            } catch (org.eclipse.paho.client.mqttv3.MqttException e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+7),
                        e.getMessage()+" trying to disconnect.", e);
            }
            break;
        case PAHOV5:
            try {
//                if (pahoConnv5.isConnected()) {
                    List<UserProperty> pahoprop = toPahoProps(props);
                    MqttProperties mprops = new MqttProperties();
                    if (pahoprop != null && pahoprop.size() > 0)
                        mprops.setUserProperties(pahoprop);
                    if (reason != null);
                        mprops.setReasonString(reason);
                    if (sessionExpire >= 0)
                        mprops.setSessionExpiryInterval(sessionExpire);
                    org.eclipse.paho.mqttv5.client.IMqttToken disToken = pahoConnv5.disconnect(
                            1000, null, null, rc, mprops);
                    disToken.waitForCompletion();
                    /* TODO: map pahov5 error handling */
                    org.eclipse.paho.mqttv5.common.MqttException e = disToken.getException();
                    if (null != e) {
                        throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+7),
                                e.getMessage()+" trying to disconnect.", e);
                    }
//                }
            } catch (org.eclipse.paho.mqttv5.common.MqttException e) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+7),
                        e.getMessage()+" trying to disconnect.", e);
            }
            break;
        case ISMMQTT:
            List<IsmMqttUserProperty> ismprop = toIsmProps(props);
            if (!wsMqttConn.isConnected()) {
                throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+7),
                        "Trying to disconnect when already disconnected.");
            }
            wsMqttConn.disconnect(rc, reason, sessionExpire, ismprop);
            break;
        case ISMWS:
            wsConn.disconnect();
        }
    }
	
	
	public void getRetainedMessage(String topic, Integer ID) throws IsmTestException {
		switch (connType) {
		case JSONWS:
		case JSONTCP:
			try {
				System.out.println("getRetainedMessage: calling jsonConn.getRetained");
				jsonConn.getRetained(topic, ID);
			} catch (IOException ioe) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+28)
						, "IOException thrown attempting to getRetainedMessage"
						, ioe); 
			}
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+28)
					, "getRetainedMessage not supported for connection of type "
					+getConnType()); 
		}
	}
	
	public void deleteRetainedMessage(String topic, Integer ID) throws IsmTestException {
		switch (connType) {
		case JSONWS:
		case JSONTCP:
			try {
				System.out.println("deleteRetainedMessage: calling jsonConn.deleteRetained");
				jsonConn.deleteRetained(topic, ID);
			} catch (IOException ioe) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+28)
						, "IOException thrown attempting to deleteRetained"
						, ioe); 
			}
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+28)
					, "deleteRetainedMessage not supported for connection of type "
					+getConnType()); 
		}
	}
	
/*	public Map<String,Object> getProperties() throws IsmTestException {
		Map<String,Object> result = null;
		switch (connType) {
		case JSONWS:
		case JSONTCP:
			try {
				System.out.println("getProperties: calling jsonConn.getProperties");
				result = jsonConn.getProperties(nextID());
			} catch (IOException ioe) {
				throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+30)
						, "IOException thrown attempting to getProperties"
						, ioe); 
			}
			break;
		default:
			throw new IsmTestException("ISMTEST"+(Constants.MYCONNECTION+31)
					, "getProperties not supported for connection of type "
					+getConnType()); 
		}
		return result;
	}
*/
	public boolean isConnectionLost() {
		return connectionLost;
	}
	
	public boolean waitForReconnection(long maxMillis) {
		if ((null != myOpts && null != myOpts.getServerURIs())
				|| (null != myConnAct._failoverIp && null != myConnAct._clientID)) {
			long startTime = System.currentTimeMillis();
			long endTime = startTime + maxMillis;
			while (System.currentTimeMillis() < endTime) {
				if (reconnected) 
				    return true;
				sleep(10);
			}
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
					, "ISMTEST"+(Constants.MYCONNECTION+14)
					, "Trying to WaitForReconnection without failover server.");
		}
		
		return false;
	}
	
	private String getConnType() {
		switch (connType) {
		case MQMQTT:
			return "MQ client";
		case PAHO:
			return "PAHO client";
		case PAHOV5:
			return "PAHO MQTTv5 client";
		case ISMMQTT:
			return "IsmClient MQTT connection";
		case ISMWS:
			return "IsmClient WebSocket connection";
		case JSONWS:
		case JSONTCP:
			return "Ima JSON client";
		default:
			return "Unknown message type " + connType;
		}
	}
	
	static public String generateClientId() {
		return MqttClient.generateClientId();
	}
	
	public MyConnectionOptions getNewOpts() {
		return new MyConnectionOptions(connType);
	}
	
	public Debug getDebug() {
		switch (connType) {
		case PAHO:
			return pahoConn.getDebug();
		case PAHOV5:
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+50), "Not supported yet ");
			return null;
		case MQMQTT:
			return mqttConn.getDebug();
		default:
			return null;
		}
	}
	  
    public int getVersion() {
        return mqttVersion;
    }
	
	public void countMonitorRecords(MonitorRecord mon) {
		monitorCounts[mon.getMessageTypeInt()]++;
		totalMonitorRecords++;
	}
	
	public long getMonitorRecordCount(MonitorRecord.ObjectType type) {
		return monitorCounts[MonitorRecord.getMessageTypeInt(type)];
	}
	
	public long getMonitorRecordCount() {
		return totalMonitorRecords;
	}
	
	public void setFlag(String topic) {
		this.flag = true;
		this.topic = topic;
	}
	
	public boolean getFlag() {
		return flag;
	}
	
	public void clearFlag() {
		this.flag = false;
		this.topic = null;
	}

	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}
	
	/*
	 * Async listener class for ISMMQTT connections
	 */
    private class TestDriverIsmListener implements IsmMqttListenerAsync {
        MyConnection connect;
        
        TestDriverIsmListener(MyConnection connect) {
            this.connect = connect;
        }

        /*
         * On message callback
         * @see com.ibm.ism.mqtt.IsmMqttListenerAsync#onMessage(com.ibm.ism.mqtt.IsmMqttMessage)
         */
        public int onMessage(IsmMqttMessage msg) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.MYCONNECTION+17), 
                    "Receiving message topic:"+msg.getTopic()+" QoS="+msg.getQoS()+" RETAIN="+msg.getRetain()+" Message:"+msg.getPayload());
            messages.add(new MyMessage(msg));
            if (null != topic) {
                if (topic.equals(msg.getTopic())) 
                    flag = false;
            }
            if (myConnAct._messageDelayMS > 0) {
                if (myConnAct._msgNodelayCount < ++noDelayCount) {
                    noDelayCount = 0;
                    MyConnection.sleep(myConnAct._messageDelayMS);
                }
            }
            return 0;
        }
        
        public void onCompletion(IsmMqttResult result) {
            
        }

        /*
         * Disconnect callback
         * @see com.ibm.ism.mqtt.IsmMqttListenerAsync#onDisconnect(com.ibm.ism.mqtt.IsmMqttResult)
         */
        public void onDisconnect(IsmMqttResult result) {
            connectionLost = true;
            Throwable e = result.getThrowable();
            connect.cause = e;
            int rc = result.getReasonCode();
            connect.reasonCode = rc;
            connect.reason = null;
            if (null != e) { 
                _trWriter.writeException(e);
                _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14),
                    "onDisconnect Client " + clientID + " was disconnected with rc="+rc, e);
            } else {
                String reason = result.getReason();
                if (reason == null)
                    reason = MqttRC.toString(rc);
                connect.reason = reason;
                _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT,  "ISMTEST"+(Constants.MQTTRC+rc),
                    "onDisconnect Client " + clientID + " disconnected: rc=" + rc + " reason=" + reason);
            }    
        }
        
    }
	
	private class TestDriverJsonCallback implements JsonListener {

		public void onMessage(JsonMessage msg) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.MYCONNECTION+17), 
					"Receiving message topic:"+msg.getTopic()+" QoS="+msg.getQoS()+" RETAIN="+msg.getRetain()+" Message:"+msg.getPayload());
			MyJsonMsg mymsg = new MyJsonMsg(msg);
			messages.add(new MyMessage(mymsg));
			if (null != topic) {
				if (topic.equals(msg.getTopic())) 
				    flag = false;
			}
			
		}

		public void onDisconnect(int rc, Throwable e) {
			connectionLost = true;
			if (null != e) 
			    e.printStackTrace();
			_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
					, "Client was disconnected with rc="+rc, e);
		}
		
	}

	private class TestDriverMqttCallback implements MqttCallback {

		public void connectionLost(Throwable arg0) {
			connectionLost = true;
			reconnected = false;
			if (null == topic) {
				flag = false;
			}
			if (null != myConnAct._failoverIp && null != myConnAct._clientID) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
						, "Attempting to connect to standby server at "+myConnAct._failoverIp+":"+myConnAct._failoverPort);
				/* Try failover server */
				String myip = myConnAct._failoverIp;
				if (-1 != myip.indexOf(":")) {
					myip = "["+myConnAct._failoverIp+"]";
				}
				if (myip.equals(currentIP)) return;
				String serverURI = (myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._failoverPort;

				long startTime = System.currentTimeMillis();
				long endTime = startTime + myConnAct._failoverMaxMilli;
				MyConnection.sleep((int)myConnAct._failoverWaitBeforeReconnectMilli);
				do {
					try {
						if (null != myConnAct._persistenceDirectory) {
							MqttClientPersistence persist = (MqttClientPersistence) new MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
							mqttConn = new MqttClient(serverURI
									, myConnAct._clientID, persist);					
						} else {
							mqttConn = new MqttClient(serverURI, myConnAct._clientID);
						}
						mqttConn.setCallback(new TestDriverMqttCallback());
						mqttConn.connect(myOpts.getMqOpts());
						_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
								, "Connect to standby server at "+myConnAct._failoverIp+" succeeded");
						connectionLost = false;
						reconnected = true;
						return;
					} catch (MqttException e) {
					    MyConnection.sleep(10);
					}
				} while (System.currentTimeMillis() < endTime);
			}

		}

		public void deliveryComplete(MqttDeliveryToken arg0) {
		}

		public void messageArrived(MqttTopic arg0, MqttMessage arg1)
				throws Exception {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.MYCONNECTION+17), 
					"Receiving message topic:"+arg0.getName()+" QoS="+arg1.getQos()+" RETAIN="+arg1.isRetained()+" contents: "+TestUtils.bytesToHex(arg1.getPayload()));
			messages.add(new MyMessage(arg0, arg1));
			if (null != topic) {
				if (topic.equals(arg0)) 
				    flag = false;
			}
		}

	}

	private class TestDriverPahoCallback implements org.eclipse.paho.client.mqttv3.MqttCallback {
		MyConnection owner;

		public TestDriverPahoCallback(MyConnection owner) {
			super();
			this.owner = owner;
		}
		
		public void connectionLost(Throwable arg0) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
					, "Connection lost on "+myConnAct._ip+":"+myConnAct._port);
			_trWriter.writeException(arg0);
			connectionLost = true;
			reconnected = false;
			if (null == topic) {
				flag = false;
			}
			if (null != myOpts.getPahoOpts() && null != myOpts.getPahoOpts().getServerURIs()) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
						, "Attempting to connect to standby HA server set on connection "+myConnAct._connectionID);

				long startTime = System.currentTimeMillis();
				long endTime = startTime + myConnAct._failoverMaxMilli;
				MyConnection.sleep((int)myConnAct._failoverWaitBeforeReconnectMilli);

				_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.MYCONNECTION+14)
                        , "Completed initial wait");
				synchronized (this) {
					do {
						try {
							/*if (null != myConnAct._persistenceDirectory) {
								org.eclipse.paho.client.mqttv3.MqttClientPersistence persist = 
									(org.eclipse.paho.client.mqttv3.MqttClientPersistence) 
									new org.eclipse.paho.client.mqttv3.persist.MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
								pahoConn = new org.eclipse.paho.client.mqttv3.MqttAsyncClient(""
										, myConnAct._clientID, persist);					
							} else {
								pahoConn = new org.eclipse.paho.client.mqttv3.MqttAsyncClient("", myConnAct._clientID);
							}
							pahoConn.setCallback(new TestDriverPahoCallback());*/
							IMqttToken connToken = pahoConn.connect(myOpts.getPahoOpts());
							connToken.waitForCompletion();
							org.eclipse.paho.client.mqttv3.MqttException e = connToken.getException();
							if (null != e) {
								throw e;
							}
							_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
									, "Connect to standby server at succeeded on connection "+myConnAct._connectionID);
							reconnected = true;
							connectionLost = false;
							return;
						} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
							_trWriter.writeException(e);
							MyConnection.sleep(10);
						}
					} while (System.currentTimeMillis() < endTime);
				}
			} else if (null != myConnAct._failoverIp && null != myConnAct._clientID) {
				synchronized (this) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
							, "Attempting to connect to standby server at "+myConnAct._failoverIp
							  +" on connection "+myConnAct._connectionID);
					/* Try failover server */
					String myip = myConnAct._failoverIp;
					if (-1 != myip.indexOf(":")) {
						myip = "["+myConnAct._failoverIp+"]";
					}
					if (myip.equals(currentIP)) return;
					String serverURI = (myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._failoverPort;

					long startTime = System.currentTimeMillis();
					long endTime = startTime + myConnAct._failoverMaxMilli;
					MyConnection.sleep((int)myConnAct._failoverWaitBeforeReconnectMilli);
					do {
						try {
							if (null != myConnAct._persistenceDirectory) {
								org.eclipse.paho.client.mqttv3.MqttClientPersistence persist = 
									(org.eclipse.paho.client.mqttv3.MqttClientPersistence) 
									new org.eclipse.paho.client.mqttv3.persist.MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
								pahoConn = new org.eclipse.paho.client.mqttv3.MqttAsyncClient(serverURI
										, myConnAct._clientID, persist);					
							} else {
								pahoConn = new org.eclipse.paho.client.mqttv3.MqttAsyncClient(serverURI, myConnAct._clientID);
							}
							pahoConn.setCallback(new TestDriverPahoCallback(owner));
							IMqttToken connToken = pahoConn.connect(myOpts.getPahoOpts());
							connToken.waitForCompletion();
							org.eclipse.paho.client.mqttv3.MqttException e = connToken.getException();
							if (null != e) {
								throw e;
							}
							_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
									, "Connect to standby server at "
									+ pahoConn.getServerURI()+" succeeded on connection "+myConnAct._connectionID);
							reconnected = true;
							connectionLost = false;
							return;
						} catch (org.eclipse.paho.client.mqttv3.MqttException e) {
						    MyConnection.sleep(10);
						}
					} while (System.currentTimeMillis() < endTime);
				}
			}

		}

		public void deliveryComplete(org.eclipse.paho.client.mqttv3.IMqttDeliveryToken arg0) {
		}
		
		public void messageArrived(String arg0, org.eclipse.paho.client.mqttv3.MqttMessage arg1)
				throws Exception {
			String message;
			message = new String(arg1.getPayload(), StandardCharsets.UTF_8);
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.MYCONNECTION+17), 
					"Receiving message connection: "+owner.getName()+" topic:"+arg0+" QoS="+arg1.getQos()+" RETAIN="+arg1.isRetained()+" contents: "+message);
			messages.add(new MyMessage(arg0, arg1));
			if (null != topic) {
				if (arg0.startsWith(topic)) flag = false;
			}
			if (0 < myConnAct._messageDelayMS) {
				if (myConnAct._msgNodelayCount < ++noDelayCount) {
					noDelayCount = 0;
					MyConnection.sleep(myConnAct._messageDelayMS);
				}
			}
		}

	}
	
	/*
	 * Implement the callbacks for PAHOV5
	 */
	private class TestDriverPahov5Callback implements org.eclipse.paho.mqttv5.client.MqttCallback {
		MyConnection owner;

		public TestDriverPahov5Callback(MyConnection owner) {
			super();
			this.owner = owner;
		}
		
        /**
	     * This method is called when the server gracefully disconnects from the client
         * by sending a disconnect packet, or when the TCP connection is lost due to a
         * network issue or if the client encounters an error.
         * 
         * @param disconnectResponse
         *            a {@link MqttDisconnectResponse} containing relevant properties
         *            related to the cause of the disconnection.
         */
		public void disconnected(org.eclipse.paho.mqttv5.client.MqttDisconnectResponse response){
			_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+17)
					, "Disconnected: " + response.toString());
			if (response.getException() != null)
		        _trWriter.writeException(response.getException());
			
			reasonCode = response.getReturnCode();
			reason = response.getReasonString();
			connectionLost = true;
            reconnected = false;
            if (null == topic) {
                flag = false;
            }
            if (null != myOpts.getPahoOptsv5() && null != myOpts.getPahoOptsv5().getServerURIs()) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
                        , "Attempting to connect to standby HA server set on connection "+myConnAct._connectionID);

                long startTime = System.currentTimeMillis();
                long endTime = startTime + myConnAct._failoverMaxMilli;
                try {
                    Thread.sleep(myConnAct._failoverWaitBeforeReconnectMilli);
                } catch (InterruptedException e) {}

                _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.MYCONNECTION+14)
                        , "Completed initial wait");
                synchronized (this) {
                    do {
                        try {
							_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST" + (Constants.MYCONNECTION + 20),
									"Setting cleanStart=false for reconnection");
							myOpts.setCleanSession(false);
                            org.eclipse.paho.mqttv5.client.IMqttToken connToken = pahoConnv5.connect(myOpts.getPahoOptsv5());
                            connToken.waitForCompletion();
                            org.eclipse.paho.mqttv5.common.MqttException e = connToken.getException();
                            if (null != e) {
                                throw e;
                            }
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
                                    , "Connect to standby server at succeeded on connection "+myConnAct._connectionID);
                            reconnected = true;
                            connectionLost = false;
                            return;
                        } catch (org.eclipse.paho.mqttv5.common.MqttException e) {
                            _trWriter.writeException(e);
                            try {
                                Thread.sleep(10);
                            } catch (InterruptedException ie) {}
                        }
                    } while (System.currentTimeMillis() < endTime);
                }
            } else if (null != myConnAct._failoverIp && null != myConnAct._clientID) {
                synchronized (this) {
                    _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
                            , "Attempting to connect to standby server at "+myConnAct._failoverIp
                              +" on connection "+myConnAct._connectionID);
                    /* Try failover server */
                    String myip = myConnAct._failoverIp;
                    if (-1 != myip.indexOf(":")) {
                        myip = "["+myConnAct._failoverIp+"]";
                    }
                    if (myip.equals(currentIP)) 
                        return;
                    String serverURI = (myConnAct._SSL ? "ssl" : "tcp")+"://"+myip+":"+myConnAct._failoverPort;

                    long startTime = System.currentTimeMillis();
                    long endTime = startTime + myConnAct._failoverMaxMilli;
                    try {
                        Thread.sleep(myConnAct._failoverWaitBeforeReconnectMilli);
                    } catch (InterruptedException e) {}
                    do {
                        try {
                            if (null != myConnAct._persistenceDirectory) {
                                org.eclipse.paho.mqttv5.client.MqttClientPersistence persist = 
                                    (org.eclipse.paho.mqttv5.client.MqttClientPersistence) 
                                    new org.eclipse.paho.mqttv5.client.persist.MqttDefaultFilePersistence(myConnAct._persistenceDirectory);
                                pahoConnv5 = new org.eclipse.paho.mqttv5.client.MqttAsyncClient(serverURI
                                        , myConnAct._clientID, persist);                    
                            } else {
                                pahoConnv5 = new org.eclipse.paho.mqttv5.client.MqttAsyncClient(serverURI, myConnAct._clientID);
                            }
							pahoConnv5.setCallback(new TestDriverPahov5Callback(owner));
							_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST" + (Constants.MYCONNECTION + 20),
									"Setting cleanStart=false for reconnection");
							myOpts.setCleanSession(false);
                            org.eclipse.paho.mqttv5.client.IMqttToken connToken = pahoConnv5.connect(myOpts.getPahoOptsv5());
                            connToken.waitForCompletion();
                            org.eclipse.paho.mqttv5.common.MqttException e = connToken.getException();
                            if (null != e) {
                                throw e;
                            }
                            _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+14)
                                    , "Connect to standby server at "
                                    + pahoConnv5.getServerURI()+" succeeded on connection "+myConnAct._connectionID);
                            reconnected = true;
                            connectionLost = false;
                            return;
                        } catch (org.eclipse.paho.mqttv5.common.MqttException e) {
                            try {
                                Thread.sleep(10);
                            } catch (InterruptedException ie) {}
                        }
                    } while (System.currentTimeMillis() < endTime);
                }
            }
			
		}
		
	    /**
         * This method is called when an exception is thrown within the MQTT client.
         * The reasons for this may vary, from malformed packets, to protocol errors
         * or even bugs within the MQTT client itself. This callback surfaces those
         * errors to the application so that it may decide how best to deal with them.
         * 
         * For example, The MQTT server may have sent a publish message with an invalid
         * topic alias, the MQTTv5 specification suggests that the client should disconnect
         * from the broker with the appropriate return code, however this is completely up to
         * the application itself. 
         * @param exception - The exception thrown causing the error.
         */
		public void mqttErrorOccurred(org.eclipse.paho.mqttv5.common.MqttException exception){

	          _trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+68),
	              "MQTT error occurred:" + exception);
		}
		
		/**
         * Called when delivery for a message has been completed, and all
         * acknowledgments have been received. For QoS 0 messages it is called once the
         * message has been handed to the network for delivery. For QoS 1 it is called
         * when PUBACK is received and for QoS 2 when PUBCOMP is received. The token
         * will be the same token as that returned when the message was published.
         *
         * @param token
         *            the delivery token associated with the message.
         */
		public void deliveryComplete(org.eclipse.paho.mqttv5.client.IMqttToken token) {

			int[] rclist = token.getReasonCodes();
			_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+67)
					, "Delivery complete: Message ID: " + token.getMessageId() + ", rc: " + Arrays.toString(rclist));

		}
		
		public void messageArrived(String topic, org.eclipse.paho.mqttv5.common.MqttMessage v5msg) throws Exception {   
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.MYCONNECTION+17), 
					"Receiving message connection: "+owner.getName()+" topic:"+topic + " message=" + v5msg);
			messages.add(new MyMessage(topic, v5msg));
			if (null != topic) {
				if (topic.startsWith(topic)) flag = false;
			}
			if (0 < myConnAct._messageDelayMS) {
				if (myConnAct._msgNodelayCount < ++noDelayCount) {
					noDelayCount = 0;
					Thread.sleep(myConnAct._messageDelayMS);
				}
			}
		}
		
		public void connectComplete(boolean reconnect, String uri) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_EVENT, "ISMTEST"+(Constants.MYCONNECTION+29)
					, "Connect complete: " + uri);
		}


        public void authPacketArrived(int arg0, MqttProperties arg1) {
        }

	}
	
}
