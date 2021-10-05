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
/*-----------------------------------------------------------------
 * System Name : Internet Scale Messaging
 * File        : WsConnectionAction.java
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.io.IOException;
import java.util.Iterator;
import java.util.List;
import java.util.Properties;
import java.util.Vector;

import org.eclipse.paho.mqttv5.common.packet.MqttProperties;
import org.eclipse.paho.mqttv5.common.packet.UserProperty;
import org.w3c.dom.Element;

import com.ibm.ism.mqtt.IsmMqttConnection;
import com.ibm.ism.mqtt.IsmMqttMessage;
import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class CreateConnectionAction extends ApiCallAction {
	/*
	 * private final String _structureID;
	 */
	protected final String	_connectionID;
	protected final String	_ip;
	protected final int		_port;
	protected final String	_failoverIp;
	protected final int		_failoverPort;
	protected final String	_path;
	private final String	_protocol;
	private final String	_connectionType;
	// The following are used for MQTT type connections
	private final int		_keepAlive;
	protected final String	_clientID;
	private final boolean	_cleanSession;
	private final String	_willTopic;
	private final String	_willMessage;
	private final int		_willQoS;
	private final boolean	_willRetain;
	private final boolean	_verbose;
	private final String	_user;
	private final String	_password;
	protected final String	_persistenceDirectory;
	protected final boolean	_SSL;
	protected final String	_SSLProps;
	protected final long	_failoverMaxMilli;
	protected final long    _failoverWaitBeforeReconnectMilli;
	protected final String  _keepWillMessage;
	protected final String	_servers;
	protected final int		_corruptFirst;
	protected final int		_corruptLast;
	protected final int		_corruptIncrement;
	protected final int		_messageDelayMS;
	protected final int		_msgNodelayCount;
	protected final String	_mqttVersion;
	protected final Boolean _sessionPresent;
	protected final int		_maxInflight;
	protected final int     _receiveMax;
	protected final long    _maxPacketSize;  /* uint32_t */
	          final int     _requestProblemInfo;
	          final boolean _requestResponseInfo;
	          final int     _topicAliasIn;
	          final int     _topicAliasOut;
	          final long    _sessionExpire;  /* uint32_t */
	          final String  _willContentType;
	          final long    _willDelay;    /* uint32_t */
	          final long    _willExpire;   /* uint32_t */
	          final List<MqttUserProp> _willProps;
	          final List<MqttUserProp> _connectUserProps;
	          final boolean _httpsHostnameVerificationEnabled;
          	  final String 	_useAssignedClientID;

	/**
	 * Constructor for the CreateConnection Action
	 * @param config   The xml element
	 * @param dataRepository The data repository
	 * @param trWriter The trace writer
	 */
	public CreateConnectionAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		
		super(config, dataRepository, trWriter);
		/*
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		*/
		
		_connectionID = _actionParams.getProperty("connection_id");
		if (_connectionID == null) {
			throw new RuntimeException("connection_id is not defined for "
					+ this.toString());
		}
		String connectionType = _actionParams.getPropertyOrEnv("connectionType");
		if (null == connectionType) {
			throw new RuntimeException("connectionType is not specified in ActionParameter or environment for"
					+ this.toString());
		}
		_connectionType = connectionType;
		// Valid type values are topic, queue, or default
		// The type parameter is ignored if _jndiName is specified
		_ip = _actionParams.getProperty("ip");
		if (_ip == null) {
			throw new RuntimeException("ip is not defined for "
				+ this.toString());
		}
		_port = Integer.parseInt(_apiParameters.getProperty("port", "16204"));
		if ((_port < 1 && _port > 65535)) {
			throw new RuntimeException("port value " + _port + "is not valid for"
				+ this.toString());
		}
		_failoverIp = _actionParams.getProperty("failoverIP");
		_failoverPort = Integer.parseInt(_apiParameters.getProperty("failoverPort", "0"));
		_path = _apiParameters.getProperty("path", null);
		_protocol = _apiParameters.getProperty("protocol", "monitoring.ism.ibm.com");
		_clientID = _apiParameters.getProperty("clientId");
		if ("WS".equals(_connectionType)) {
			if (_path == null) {
				throw new RuntimeException("path is not defined for "
					+ this.toString());
			}
			if (_protocol == null) {
				throw new RuntimeException("protocol is not defined for "
					+ this.toString());
			}
		}
		_keepAlive = Integer.parseInt(_apiParameters.getProperty("keepAlive", "35"));
		_cleanSession = Boolean.valueOf(_apiParameters.getProperty("cleanSession", "true"));
		_willTopic = _apiParameters.getProperty("willTopic");
		_willMessage = _apiParameters.getProperty("willMessage", "");
		_willDelay = Long.valueOf(_apiParameters.getProperty("willDelay", "-1"));
		_willContentType = _apiParameters.getProperty("willContentType", null);
	    _willExpire = Long.valueOf(_apiParameters.getProperty("willExpire", "-1")); 
	    _willProps = getUserProps("willprop");
	    _connectUserProps = getUserProps("userprop");
	    _sessionExpire = Long.valueOf(_apiParameters.getProperty("sessionExpire", "-1")); 
	    _topicAliasIn = Integer.valueOf(_apiParameters.getProperty("topicAliasIn", "0"));    
	    _topicAliasOut = Integer.valueOf(_apiParameters.getProperty("topicAliasOut", "0"));   
	    _maxPacketSize = Long.valueOf(_apiParameters.getProperty("maxPacketSize", "-1"));
	    _requestProblemInfo = Integer.valueOf(_apiParameters.getProperty("requestProblemInfo", "-1"));
	    _requestResponseInfo = Boolean.valueOf(_apiParameters.getProperty("requestResponseInfo", "false"));
		_keepWillMessage = _actionParams.getProperty("keepWillMesaage");
		_willQoS = Integer.valueOf(_apiParameters.getProperty("willQoS", "0"));
		_willRetain = Boolean.parseBoolean(_apiParameters.getProperty("willRETAIN", "false"));
		_verbose = Boolean.parseBoolean(_apiParameters.getProperty("verbose", "false"));
		_user = _apiParameters.getProperty("user");
		_password = _apiParameters.getProperty("password");
		if ("IMA_LTPA_AUTH".equals(_user) && null == _password) {
			throw new RuntimeException("When user is IMA_LTPA_AUTH, password cannot be null for "
					+ this.toString());
		}
		_corruptFirst = Integer.valueOf(_actionParams.getProperty("corruptFirst", "-1"));
		_corruptLast = Integer.valueOf(_actionParams.getProperty("corruptLast", "-1"));
		_corruptIncrement = Integer.valueOf(_actionParams.getProperty("corruptIncrement", "1"));
		_persistenceDirectory = _actionParams.getProperty("persistenceDirectory");
		_SSL = Boolean.parseBoolean(_actionParams.getProperty("SSL", "false"));
		_SSLProps = _apiParameters.getProperty("SSLProperties");
		_failoverMaxMilli = Long.parseLong(_actionParams.getProperty("failoverMaxMilli", "100000"));
		_failoverWaitBeforeReconnectMilli = Long.parseLong(_actionParams.getProperty("failoverWaitBeforeReconnectMilli", "1000"));
		_servers = _actionParams.getProperty("servers");
		_messageDelayMS = Integer.parseInt(_actionParams.getProperty("messageDelayMS", "0"));
		_msgNodelayCount = Integer.parseInt(_actionParams.getProperty("msgNodelayCount", "0"));
		_mqttVersion = _actionParams.getProperty("mqttVersion");
		String temp = _actionParams.getProperty("sessionPresent");
		if (null == temp) {
			_sessionPresent = null;
		} else {
			_sessionPresent = Boolean.valueOf(temp);
		}
		_maxInflight = Integer.parseInt(_actionParams.getProperty("maxInflight", "-1"));
	    _receiveMax  = Integer.parseInt(_actionParams.getProperty("receiveMax", "-1"));
	    _httpsHostnameVerificationEnabled = Boolean.parseBoolean(_apiParameters.getProperty("httpsHostnameVerificationEnabled", "false"));
	    _useAssignedClientID = _apiParameters.getProperty("useAssignedClientID");
	}

	/**
	 * Implement the connection action
	 * @see com.ibm.ism.ws.test.ApiCallAction#invokeApi()
	 * 
	 * The connection action varies significantly by which client is being used.  
	 * The paho clients split the connection object with the addition of an options
	 * object, but the ISM client uses a single object.
	 * 
	 * A number of the fields are only used when the MQTT version is 5 or greater.
	 */
	protected boolean invokeApi() throws IsmTestException {
		final MyConnection con;
		Properties sslProps = null;
		MyConnectionOptions opts = null;
        String [] servers = null;
		if (null != _SSLProps) {
			sslProps = (Properties)_dataRepository.getVar(_SSLProps);
			if (null == sslProps) {
				throw new IsmTestException("ISMTEST"+(Constants.CREATECONNECTION+2), "Unable to find properties object "+_SSLProps+" in store");
			}
		}
		String clientId = _clientID;
		

		
		Object storeVal = _dataRepository.getVar(_clientID);
		if (null != storeVal) {
			if (storeVal instanceof NumberString) {
				clientId = ((NumberString)storeVal).incrementAndGetName();
			} else if (storeVal instanceof String) {
				clientId = (String)storeVal;
			}
		}
		
		if(_useAssignedClientID != null){
			clientId = (String)_dataRepository.getVar(_useAssignedClientID);
		}
		
		int connType = -1;
		if ("WS".equals(_connectionType)) {
			connType = MyConnection.ISMWS;
		} else if ("WS-MQTT-bin".equals(_connectionType)) {
			connType = MyConnection.ISMMQTT;
		} else if ("WS-MQTT-WSbin".equals(_connectionType)) {
			connType = MyConnection.ISMWSMQTT;
		} else if ("MQ-MQTT".equals(_connectionType)) {
			connType = MyConnection.MQMQTT;
		} else if ("PAHO-MQTT".equals(_connectionType)) {
			connType = MyConnection.PAHO;
		} else if ("PAHO-MQTTv5".equals(_connectionType)) {
			connType = MyConnection.PAHOV5;
		} else if ("JSON-TCP".equals(_connectionType)) {
			connType = MyConnection.JSONTCP;
		} else if ("JSON-WS".equals(_connectionType)) {
			connType = MyConnection.JSONWS;
		} else {
			throw new IsmTestException("ISMTEST"+(Constants.CREATECONNECTION+1), "Unknown connectionType: "+_connectionType);
		}
		
		/*
		 * Determine the version
		 */
        int mqttver;
        switch(connType) {
        case MyConnection.ISMMQTT:
        case MyConnection.ISMWSMQTT:  mqttver = 4;   break;
        case MyConnection.PAHOV5:     mqttver = 5;   break;
        default:                      mqttver = 3;   break;
        }
        String mqttvers = _mqttVersion != null ? _mqttVersion : (String)_dataRepository.getVar("PahoVersion");  
        if (mqttvers != null) {
            try {
                mqttver = Integer.parseInt(mqttvers);
            } catch (Exception e) {};
        }
        if (connType == MyConnection.PAHO && mqttver == 5)
            connType = MyConnection.PAHOV5;
		
		//debug
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
				, "ISMTEST"+(Constants.CREATECONNECTION+15)
				, "Action {0}: connectionType: {1}."
				, _id, _connectionType);
		
		/*
		 * Create the options object
		 */
		int optType = connType;
		if (connType == MyConnection.ISMWSMQTT)
		    optType =  MyConnection.ISMMQTT;
		opts = new MyConnectionOptions(optType);
		
		opts.setMaxInflight(_maxInflight);
		opts.setUserName(_user);
		String password = (String) _dataRepository.getVar(_password);
		if (null == password) {
			password = _password;
		} else if ("IMA_LTPA_AUTH".equals(_user)) {
			if (-1 != _corruptFirst) {
				int last = _corruptLast;
				if (-1 == last) last=_corruptFirst;
				if (_corruptFirst >= password.length()) {
					throw new IsmTestException("ISMTEST"+(Constants.CREATECONNECTION+3),
							"Attempt to change character "+_corruptFirst+" of "
							+password.length()+" LTPA token");
				}
				if (last >= password.length()) last = password.length()-1;
				char[] charArray = password.toCharArray();
				for (int i=_corruptFirst; i<=last; i++) {
					charArray[i] = (char) (password.codePointAt(i)+_corruptIncrement);
				}
				password = new String(charArray);
			}
		}
		opts.setPassword(password);
		opts.setCleanSession(_cleanSession);
		opts.setKeepAliveInterval(_keepAlive);
		byte[] willMessage = null;
		if (_willTopic != null) {
			willMessage = _willMessage.getBytes();
			if ("RANDOM".equals(willMessage)) {
				willMessage = TestUtils.randomPayload(TestUtils.rand.nextInt(500), null);
			}
			if (null != _keepWillMessage) {
				_dataRepository.storeVar(_keepWillMessage, willMessage);
			}
			opts.setWill(_willTopic, willMessage, _willQoS, _willRetain);
			
		}
		if (null != sslProps) {
			opts.setSSLProperties(sslProps);
		}

		// If no clientId was specified, then generate one.
		if (null == clientId) 
		    clientId = MyConnection.generateClientId();
		
		int type = MyConnection.JSONWS;

		/*
		 * Create the connection object
		 */
		switch (connType) {
		case MyConnection.ISMWS:
			con = new MyConnection(connType, _path, _protocol, null, this, 0);
			setConnection(con);
			break;
			
		/*
		 * Support for client_mqtt test client	
		 */
		case MyConnection.ISMMQTT:
		case MyConnection.ISMWSMQTT:
			int encoding;
			if (MyConnection.ISMMQTT == connType) {
			    switch (mqttver) {
			    case 3: encoding = IsmMqttConnection.TCP3;  break;
			    default:
			    case 4: encoding = IsmMqttConnection.TCP4;  break;
			    case 5: encoding = IsmMqttConnection.TCP5;  break;
			    }
			} else {
                switch (mqttver) {
                case 3: encoding = IsmMqttConnection.WS3;  break;
                default:
                case 4: encoding = IsmMqttConnection.WS4;  break;
                case 5: encoding = IsmMqttConnection.WS5;  break;
                }
			}
			con = new MyConnection(MyConnection.ISMMQTT, _path, null, clientId, this, mqttver);
			IsmMqttConnection ismcon = (IsmMqttConnection)con.getConnectObject();
			ismcon.setClean(opts.isCleanSession());
			ismcon.setEncoding(encoding);
			setConnection(con);
			version = mqttver;
	        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.CREATECONNECTION+4),
	            "Create ISM client: version=" + mqttver + " ws=" + (connType!=MyConnection.ISMMQTT) +
	                 " clientID=" + clientId);
			
			/* Set the Will Message including properties */
			if (_willTopic != null) {
			    IsmMqttMessage willMsg = new IsmMqttMessage(_willTopic, opts.getWillMessage(), _willQoS, _willRetain);
			    willMsg.setContentType(_willContentType);
	            willMsg.setExpire(_willExpire);
	            if (_willProps != null) {
                    Iterator<MqttUserProp> it = _willProps.iterator();
                    while (it.hasNext()) {
                        MqttUserProp prp = it.next();
                        willMsg.addUserProperty(prp.getKey(), prp.getValue());
                    }
	            }
				ismcon.setWill(willMsg, _willDelay);
			}
			ismcon.setKeepAlive(_keepAlive);
			ismcon.setVerbose(_verbose);
			if (_user != null) {
				con.setUserInfo(opts.getUserName(), password);
			}
            if (_connectUserProps != null) {
                Iterator<MqttUserProp> it = _connectUserProps.iterator();
                while (it.hasNext()) {
                    MqttUserProp prp = it.next();
                    ismcon.addUserProperty(prp.getKey(), prp.getValue());
                }
            }
            if (_sessionExpire >= 0)
			    ismcon.setExpire(_sessionExpire);
			ismcon.setTopicAlias(_topicAliasIn, _topicAliasOut);
			if (_receiveMax > 0)
			    ismcon.setMaxReceive(_receiveMax);
			if (_requestProblemInfo >= 0)
			    ismcon.setRequestReason(_requestProblemInfo);
			ismcon.setRequestReplyInfo(_requestResponseInfo);
			
			/*
			 * Set up server lists
			 */
			int [] ports = null;
			if (_servers != null) {
	             servers = _servers.split(";");    
			} else if (_failoverIp != null) {
			    servers = new String[] {_ip, _failoverIp};
			    ports = new int [] {_port, _failoverPort}; 
			}
			if (servers != null) {
			    opts.setServerURIs(servers);
			    ismcon.setAddressList(servers);
			    if (ports != null)
		           ismcon.setPortList(ports);
			}
			break;
			
		case MyConnection.MQMQTT:
			con = new MyConnection(MyConnection.MQMQTT, _path, null, clientId, this, 0);
			setConnection(con);
			opts.setWill(con.getMqTopic(_willTopic), willMessage, _willQoS, _willRetain);
			break;
			
		case MyConnection.PAHO:   
	        opts.setMqttVersion(mqttver);
			con = new MyConnection(MyConnection.PAHO, _path, null, clientId, this, mqttver);
			setConnection(con);
            version = mqttver;

			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
					, "ISMTEST"+(Constants.CREATECONNECTION+4)
					, "Action {0}: Value of mqtt version is {1}", _id, version);
			/* If list of servers specified for HA rollover, then set into opts */
			if (null != _servers) {
				servers = _servers.split(";");
				String [] uris = new String[servers.length];
				for (int i=0; i<servers.length; i++) {
					uris[i] = (_SSL ? "ssl" : "tcp")+"://"+servers[i]+":"+_port;
				}
				opts.setServerURIs(uris);
			}
			break;
			
		/*
		 * Support for pahov5 
		 */
		case MyConnection.PAHOV5:
		    version = mqttver;
			con = new MyConnection(MyConnection.PAHOV5,	_path, null, clientId, this, 5);
			setConnection(con);
			
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.CREATECONNECTION+4),
					"Action {0}: Value of mqtt version is {1}", _id, version);
			/* If list of servers specified for HA rollover, then set into opts */
			if (null != _servers) {
				servers = _servers.split(";");
				String [] uris = new String[servers.length];
				for (int i=0; i<servers.length; i++) {
					uris[i] = (_SSL ? "ssl" : "tcp")+"://"+servers[i]+":"+_port;
				}
				opts.setServerURIs(uris);
			}
			org.eclipse.paho.mqttv5.client.MqttConnectionOptions v5opts = opts.getPahoOptsv5();
			v5opts.setTopicAliasMaximum(_topicAliasIn);   
			if (_sessionExpire >= 0){
				v5opts.setSessionExpiryInterval(_sessionExpire);
			}
			
	        if (_willTopic != null) {
	            MqttProperties mprops = new MqttProperties();
	            mprops.setContentType(_willContentType);
	            if (_willExpire >= 0)
	                mprops.setMessageExpiryInterval(_willExpire);
	            if (_willProps != null) {
	                Iterator<MqttUserProp> it = _willProps.iterator();
	                List<UserProperty> willprop = new Vector<UserProperty>(); 
                    while (it.hasNext()) {
	                    MqttUserProp prp = it.next();
	                    willprop.add(new UserProperty(prp.getKey(), prp.getValue()));
                    }  
                    mprops.setUserProperties(willprop);
	            }
	            if (_willDelay > 0)
                mprops.setWillDelayInterval((long)_willDelay);
	            org.eclipse.paho.mqttv5.common.MqttMessage willmsg = 
                   new org.eclipse.paho.mqttv5.common.MqttMessage(willMessage, _willQoS, _willRetain, mprops);
	            v5opts.setWillMessageProperties(mprops);
	            v5opts.setWill(_willTopic, willmsg);
	        }    
            if (_connectUserProps != null) {
                List<UserProperty> conprop = new Vector<UserProperty>(); 
                Iterator<MqttUserProp> it = _connectUserProps.iterator();
                while (it.hasNext()) {
                    MqttUserProp prp = it.next();
                    conprop.add(new UserProperty(prp.getKey(), prp.getValue()));
                }
                v5opts.setUserProperties(conprop);
            }
            if (_receiveMax > 0)
                v5opts.setReceiveMaximum(_receiveMax);
            if (_requestProblemInfo >= 0)
                v5opts.setRequestProblemInfo(_requestProblemInfo != 0);
            if (_requestResponseInfo)
                v5opts.setRequestResponseInfo(_requestResponseInfo);
            if (_maxPacketSize > 0)
                v5opts.setMaximumPacketSize((long)_maxPacketSize);
            v5opts.setHttpsHostnameVerificationEnabled(_httpsHostnameVerificationEnabled);
			break;	
			
		case MyConnection.JSONTCP:
			type = MyConnection.JSONTCP;
			/* fall thru */
		case MyConnection.JSONWS:
			con = new MyConnection(type, _path, null, clientId, this, 0);
			setConnection(con);
			con.setVerbose(_verbose);
			break;
			
		default:
			con = null;
			setConnection(null);
		}
		
		/*
		 * Make the connection
		 */
		try {
			boolean result = con.connect(opts);
			if (null != _sessionPresent) {
				if (result != _sessionPresent) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
							, "ISMTEST"+(Constants.CREATECONNECTION+4)
							, "Action {0}: Expected sessionPresent to be {1}, was {2}"
							, _id, _sessionPresent, result);
					return false;
				}
			}
			/* TODO: check other CONNACK return values */
			
		} catch (IOException ie) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR
					, "ISMTEST"+(Constants.CREATECONNECTION+2)
					, "Action {0}: Failed to create the Connection object ({1})."
					, _id, _connectionID);

			throw(new IsmTestException("ISMTEST"+(Constants.CREATECONNECTION+2)
					, ie.getLocalizedMessage(), ie));
		}
		_dataRepository.storeVar(_connectionID, con);
		con.setName(_connectionID);
	
		return true;
	}

}
