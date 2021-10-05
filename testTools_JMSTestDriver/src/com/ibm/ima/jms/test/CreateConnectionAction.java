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
package com.ibm.ima.jms.test;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.ConnectionMetaData;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;

import org.w3c.dom.Element;

import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.test.TrWriter.LogLevel;

public class CreateConnectionAction extends ApiCallAction {
    private final String _factoryID;
    private final String _user;
    private final String _passwd;
    private final String _structureID;
    private final String _exceptionListener;
    private final String _clientID;
    private final String _verifyID;

    /**
     * @param config
     * @param dataRepository
     * @param trWriter
     */
    public CreateConnectionAction(Element config,
            DataRepository dataRepository, TrWriter trWriter) {
        super(config, dataRepository, trWriter);
        _structureID = _actionParams.getProperty("structure_id");
        if (_structureID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _factoryID = _actionParams.getProperty("factory_id");
        if (_factoryID == null) {
            throw new RuntimeException("structure_id is not defined for "
                    + this.toString());
        }
        _user = _apiParameters.getProperty("user");
        _passwd = _apiParameters.getProperty("passwd");
        _clientID = _apiParameters.getProperty("ClientID");
        if (_actionParams.getProperty("verifyID") != null) {
        	_verifyID = _actionParams.getProperty("verifyID");
        } else {
        	_verifyID = _apiParameters.getProperty("verifyID");
        }
        _exceptionListener = _apiParameters.getProperty("exceptionListener");
    }

    protected boolean invokeApi() throws JMSException {
        final Connection con;
        final ConnectionFactory factory = (ConnectionFactory) _dataRepository
                .getVar(_factoryID);
        if (factory == null) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,    "RMTD2041",    
                    "Action {0}: Failed to locate the ConnectionFactory object ({1}).",
                    _id, _factoryID);
            return false;
        }

        if ("IMA_OAUTH_ACCESS_TOKEN".equals(_user)) {
        	String accessToken = (String) _dataRepository.getVar("OAUTH_ACCESS_TOKEN");
        	_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9000", "Connecting with Access Token: " + accessToken);
        	con = factory.createConnection(_user, accessToken);
        } else {
        	con = factory.createConnection(_user, _passwd);
        }
        if(_exceptionListener != null){
            ExceptionListener l = (ExceptionListener) _dataRepository.getVar(_exceptionListener);
            if (l == null) {
                _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD2041",
                        "Action {0}: Failed to locate the ExceptionListener object ({1}).",
                        _id,_exceptionListener);
                return false;
            }
            con.setExceptionListener(l);
        }
        
        _dataRepository.storeVar(_structureID, con);
        ConnectionMetaData metadata = con.getMetaData();
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9000", "Ims Jms Client Version: {0}", ImaJmsObject.getClientVerstion());
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9000", "JMS Version: {0}", metadata.getJMSVersion());
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9000", "JMS Provider Name: {0}", metadata.getJMSProviderName());
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9000", "Provider Version: {0}", metadata.getProviderVersion());
        
        if(_clientID != null){
            con.setClientID(_clientID);
        }
        
        if(_verifyID != null){
            if(!_verifyID.equals(con.getClientID())){
                _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD2073", "Action {0}: Expected ClientID: {1} || Actual ClientID: {2}.", this.toString(), _verifyID, con.getClientID());
                con.close();
                return false;
            }
        }
        
        return true;
    }

}
