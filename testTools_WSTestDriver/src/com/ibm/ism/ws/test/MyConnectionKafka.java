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

// TODO We will consider more about non-security or non-authentication for other non-EventStreams testing 

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


import java.util.Properties;
import java.util.Arrays;
import java.util.UUID;

import javax.net.ssl.TrustManager;
import java.security.cert.X509Certificate;
import javax.net.ssl.X509TrustManager;
import javax.net.ssl.SSLContext;
import javax.net.ssl.HttpsURLConnection;

import com.ibm.ism.ws.test.TrWriter.LogLevel;
import org.apache.kafka.clients.consumer.KafkaConsumer;
import org.apache.kafka.clients.CommonClientConfigs;
import org.apache.kafka.clients.consumer.ConsumerConfig;
import org.apache.kafka.common.serialization.StringDeserializer;
import org.apache.kafka.clients.consumer.ConsumerRecords;
import org.apache.kafka.clients.consumer.ConsumerRecord;
import org.apache.kafka.common.KafkaException;
import org.apache.kafka.common.config.SaslConfigs;
import org.apache.kafka.common.config.SslConfigs;




import com.ibm.ism.jsonmsg.JsonConnection;
import com.ibm.ism.jsonmsg.JsonListener;
import com.ibm.ism.jsonmsg.JsonMessage;

/*
 * MyConnectionKafka is an agent class for a Kafka connection
 */
public class MyConnectionKafka implements Counter{

	public MyConnectionKafka(TrWriter trWriter) {
		_trWriter = trWriter;
	}

	public int getCount() {
		return -1;
	}

    private KafkaConsumer<String, String> kafkaConsumer;
    private TrWriter _trWriter = null;


    public void createConnection(String bootstrap_servers, String servicename, String username, String password, String trust_store_file){

        Properties properties = new Properties();
        properties.put(CommonClientConfigs.BOOTSTRAP_SERVERS_CONFIG, bootstrap_servers);
        properties.put(CommonClientConfigs.SECURITY_PROTOCOL_CONFIG, "SASL_SSL");
        properties.put(ConsumerConfig.KEY_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class);
        properties.put(ConsumerConfig.VALUE_DESERIALIZER_CLASS_CONFIG, StringDeserializer.class);
        properties.put(ConsumerConfig.GROUP_ID_CONFIG, UUID.randomUUID().toString());
        properties.put(ConsumerConfig.AUTO_OFFSET_RESET_CONFIG, "latest");
        properties.put(SslConfigs.SSL_PROTOCOL_CONFIG, "TLSv1.2");
        if (trust_store_file != null && !trust_store_file.isEmpty()) properties.put(SslConfigs.SSL_TRUSTSTORE_LOCATION_CONFIG, trust_store_file);
        properties.put(SaslConfigs.SASL_MECHANISM, "PLAIN");
        String sn = ((servicename == null) || servicename.isEmpty())?"":"serviceName=\"" + servicename + "\" ";
        String saslJaasConfig = "org.apache.kafka.common.security.plain.PlainLoginModule required " + sn + "username=\"" + username + "\" password=\"" + password + "\";";
        properties.put(SaslConfigs.SASL_JAAS_CONFIG, saslJaasConfig);

        try {
            kafkaConsumer = new KafkaConsumer<String, String>(properties);
        } catch (KafkaException kafkaError) {
        	_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.MYCONNECTION+9)
        			, "createConnection exception: " + kafkaError.toString());
        }
        

    }
    public void subscribe(String topic){
        kafkaConsumer.subscribe(Arrays.asList(topic));
    }
    public ConsumerRecord<String, String> receiveMessage(int poll_duration){
        ConsumerRecords<String, String> records = kafkaConsumer.poll(poll_duration);

        if (records != null && records.iterator().hasNext()){
            ConsumerRecord<String, String> record = records.iterator().next();
            if (record != null){
                return record;
            }
            else {
                return null;
            }
        }
        else { 
            return null;
        }
    }
    public void closeConnection(){
        kafkaConsumer.close();
    }


	
}
