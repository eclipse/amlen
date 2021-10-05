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
package com.ibm.ism.mqbridge.test;
import java.util.Properties;
import java.util.UUID;
public final class Example {
  public static final String          TCPAddress = 
    System.getProperty("TCPAddress", "tcp://localhost:16102");
  public static final String          SSLAddress = 
    System.getProperty("SSLAddress", "ssl://localhost:8883");
  public static final String            username =
    System.getProperty("username", System.getProperty("user.name"));
  public static final char []           password = 
    System.getProperty("password", "Password").toCharArray();
  public static String                  clientId = 
  String.format("%-23.23s", username  + "_" + 
    System.getProperty("clientId", 
    (UUID.randomUUID().toString())).trim()).replace('-', '_');
  public static final String         topicString = 
    System.getProperty("topicString", "News/Finance");
  public static final String         publication = 
    System.getProperty("publication", "Hello World " + 
  String.format("%tc", System.currentTimeMillis()));
  public static final int         quiesceTimeout = 
    Integer.parseInt(System.getProperty("timeout", "10000"));
  public static final int           sleepTimeout = 
    Integer.parseInt(System.getProperty("timeout", "30000"));
  public static final boolean       cleanSession = 
    Boolean.parseBoolean(System.getProperty("cleanSession", "false"));
  public static final int                    QoS = 
    Integer.parseInt(System.getProperty("QoS", "1"));
  public static final boolean           retained = 
    Boolean.parseBoolean(System.getProperty("retained", "false"));
  public static final Properties getSSLSettings() {
    final Properties properties = new Properties();
    properties.setProperty("com.ibm.ssl.keyStore", 
        "C:\\IBM\\MQ\\Data\\ClientKeyStore.jks");
    properties.setProperty("com.ibm.ssl.keyStoreType", 
        "JKS");
    properties.setProperty("com.ibm.ssl.keyStorePassword", 
        "password");
    properties.setProperty("com.ibm.ssl.trustStore", 
        "C:\\IBM\\MQ\\Data\\ClientTrustStore.jks");
    properties.setProperty("com.ibm.ssl.trustStoreType", 
        "JKS");
    properties.setProperty("com.ibm.ssl.trustStorePassword", 
        "password");
    return properties;
  }
}
