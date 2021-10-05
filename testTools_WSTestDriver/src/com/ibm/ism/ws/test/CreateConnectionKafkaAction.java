/*
 * Copyright (c) 2019-2021 Contributors to the Eclipse Foundation
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

/*
 * CreateConnectionKafkaAction is the action class to connect to Kafka broker.
 * The following is an example example to use
 * 
 *      <Action name="ConnectEventStreams" type="CreateConnectionKafka">
            <ActionParameter name="connectionID">eventstreams</ActionParameter>
            <ApiParameter name="bootstrap_servers">kafka03.bluemix.net:9093,kafka04.bluemix.net:9093,kafka05.bluemix.net:9093</ApiParameter>
            <ApiParameter name="username">myusername</ApiParameter>
            <ApiParameter name="password"mypassword</ApiParameter>
            <ApiParameter name="servicename">kafka</ApiParameter>
            <ApiParameter name="trust_store_file">/var/es-cert.jks</ApiParameter>
        </Action>

 */
public class CreateConnectionKafkaAction extends ApiCallAction {
	
	
	private final String	_connectionID;

    private final String _bootstrap_servers;
    private final String _username;
    private final String _password;
    private final String _trust_store_file;
    private final String _servicename;

	/**
	 * Constructor for the CreateConnection Action
	 * @param config   The xml element
	 * @param dataRepository The data repository
	 * @param trWriter The trace writer
	 */
	public CreateConnectionKafkaAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_connectionID=_actionParams.getProperty("connectionID");

		_bootstrap_servers = _apiParameters.getProperty("bootstrap_servers");
		_username = _apiParameters.getProperty("username");
		_password = _apiParameters.getProperty("password");
		_servicename = _apiParameters.getProperty("servicename");
		_trust_store_file = _apiParameters.getProperty("trust_store_file");

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
		connectKafka = new MyConnectionKafka(_trWriter);
		connectKafka.createConnection(_bootstrap_servers, _servicename, _username, _password, _trust_store_file);
		_dataRepository.storeVar(_connectionID, connectKafka);
		return true;
	}
}

