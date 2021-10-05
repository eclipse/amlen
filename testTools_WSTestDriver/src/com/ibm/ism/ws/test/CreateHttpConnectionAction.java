/*
 * Copyright (c) 2017-2021 Contributors to the Eclipse Foundation
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

import java.util.Properties;

import org.w3c.dom.Element;

import com.ibm.ism.ws.IsmHTTPConnection;
import com.ibm.ism.ws.test.TrWriter.LogLevel;

/*
 * Create an HTTP Connection
 * This can be used to make HTTP requests
 */
public class CreateHttpConnectionAction extends ApiCallAction {
    protected final String  connectionID;
    protected final String  ip;
    protected final int     port;
    protected final boolean useTLS;
    protected final String  tlsProps;
                    String  tlslevel = "none";
                    String  sniname = null;
                    boolean isConnected = false;
    
    IsmHTTPConnection connect;


    /**
     * Constructor for the Create HTTP connection action 
     * @param config   XML config element
     * @param dataRepository  The data repository
     * @param trWriter The trace writer
     */
    public CreateHttpConnectionAction(Element config, DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        connectionID = _actionParams.getProperty("connection_id");
        if (connectionID == null) {
            throw new RuntimeException("connection_id is not defined for " + this);
        }
        ip = _apiParameters.getProperty("ip");
        if (ip == null) {
            throw new RuntimeException("ip is not defined for " + this);
        }
        port = Integer.parseInt(_apiParameters.getProperty("port", "0"));
        if ((port < 1 || port > 65535)) {
            throw new RuntimeException("port value " + port + "is not valid for" + this);
        }
        useTLS = Boolean.parseBoolean(_apiParameters.getProperty("port", "false"));
        tlsProps = _apiParameters.getProperty("TLSProperties");  /* created using CreaeSSLProperties */
    }
    
    public IsmHTTPConnection getConnect() {
        return connect;
    }
    
    public boolean isConnected() {
        if (connect == null)
            return isConnected;
        else
            return connect.isConnected();
    }
    
    /**
     * @see com.ibm.ism.ws.test.ApiCallAction#invokeApi()
     */
    protected boolean invokeApi() throws IsmTestException {
        connect = new IsmHTTPConnection(ip, port, 0);
        Properties props = null;
        if (tlsProps != null) {
            props = (Properties)_dataRepository.getVar(tlsProps);
            if (props == null) {
                throw new IsmTestException("ISMTEST"+(Constants.CREATECONNECTION+2), "Unable to find properties object " + tlsProps + " in store");
            }
            String truststore = props.getProperty("com.ibm.ssl.trustStore");
            String tspassword = props.getProperty("com.ibm.ssl.trustStorePassword");
            connect.setTruststore(truststore, tspassword);
            
            String keystore = props.getProperty("com.ibm.ssl.keyStore");
            String kspassword = props.getProperty("com.ibm.ssl.keyStorePassword");
            connect.setKeystore(keystore, kspassword);
            
            sniname = props.getProperty("com.ibm.ssl.servername");
            if (sniname != null) 
                connect.setHost(sniname);
            
            int secure = 0;
            if (useTLS) {
                tlslevel = props.getProperty("com.ibm.ssl.protocol");
                if (tlslevel != null) {
                    if (tlslevel.equals("TLSv1.2")) {
                        secure = 3;
                    } else if (tlslevel.equals("TLSv1.1")) {
                        secure = 2;
                    } else if (tlslevel.equals("TLSv1")) {
                        secure = 1;
                    } else {
                        tlslevel = "none";
                    }
                }
            }
        }
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST3601", "Create " + this);
        try {
            connect.connect();
            isConnected = true;
        } catch (Exception e) {
            throw new IsmTestException("ISMTEST3610", "Unable to connect: " + this + ": " + e);
        }
        _dataRepository.storeVar(connectionID, this);
        return true;
    }

    /**
     * Format the name of the object
     * @see com.ibm.ism.ws.test.Action#toString()
     */
    public String toString() {
        String ret = "HTTPConnection name=" + connectionID + " ip=" + ip + " port=" + port + " tls=" + tlslevel;
        if (sniname != null)
            ret += " host=" + sniname;
        return ret;
    }
}
