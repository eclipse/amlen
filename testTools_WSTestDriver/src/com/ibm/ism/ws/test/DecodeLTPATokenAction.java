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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.io.InputStream;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.Properties;
import java.util.StringTokenizer;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public class DecodeLTPATokenAction extends ApiCallAction {
	static {
	    HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier() {
			
			public boolean verify(String hostname, SSLSession session) {
				// TODO Auto-generated method stub
				return true;
			}
		});
	        
	}

	private final	String	_LTPAToken;
	private final	String _LTPAKeyFile;
	private final	String	_password;

	public DecodeLTPATokenAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_password = _actionParams.getProperty("password");
		if (_password == null) {
			throw new RuntimeException("password is not defined for "
					+ this.toString());
		}
		_LTPAToken = _actionParams.getProperty("LTPAToken");
		if (_LTPAToken == null) {
			throw new RuntimeException("LTPAToken is not defined for "
					+ this.toString());
		}
		_LTPAKeyFile = _actionParams.getProperty("LTPAKeyFile");
		if (_LTPAKeyFile == null) {
			throw new RuntimeException("LTPAKeyFile is not defined for "
					+ this.toString());
		}

	}
	
	protected boolean invokeApi() throws IsmTestException {
		String LTPAToken = (String)_dataRepository.getVar(_LTPAToken);
		File file = new File(_LTPAKeyFile);
		InputStream stream = null;
		try {
			stream = new FileInputStream(file);
		} catch (FileNotFoundException e) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.DECODELTPATOKEN+3)
					, "FileNotFoundException trying to open file '"+_LTPAKeyFile+"'");
			throw new IsmTestException("ISMTEST"+(Constants.DECODELTPATOKEN+3)
					, "FileNotFoundException trying to open file '"+_LTPAKeyFile+"'", e);
		}
		Properties keyFileProps = new Properties();
		try {
			keyFileProps.load(stream);
		} catch (IOException e) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.DECODELTPATOKEN+4)
					, "IOException trying to load Properties from file '"+_LTPAKeyFile+"'");
			throw new IsmTestException("ISMTEST"+(Constants.DECODELTPATOKEN+4)
					, "IOException trying to load Properties from file '"+_LTPAKeyFile+"'", e);
		}
		String ltpa3DESKey = keyFileProps.getProperty("com.ibm.websphere.ltpa.3DESKey");

		try {
			byte[] secretKey = LtpaUtils.getSecretKey(ltpa3DESKey, _password);
			String ltpaPlaintext = new String(LtpaUtils.decryptLtpaToken(LTPAToken, secretKey));
			logTokenData(ltpaPlaintext);
		} catch (Exception e) {
			throw new IsmTestException("ISMTEST"+(Constants.DECODELTPATOKEN)
					, e.getMessage(), e);
		}

		return true;
	}
	
    public void logTokenData(String token)
    {
        System.out.println("\n");
        StringTokenizer st = new StringTokenizer(token, "%");
        String userInfo = st.nextToken();
        String expires = st.nextToken();
//        String signature = st.nextToken();
        
        Date d = new Date(Long.parseLong(expires));
        SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd-HH:mm:ss z");
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
            	, "ISMTEST"+(Constants.DECODELTPATOKEN+1)
            	, "Token is for: " + userInfo);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
            	, "ISMTEST"+(Constants.DECODELTPATOKEN+1)
            	, "Token expires at: " + sdf.format(d)+" from value "+expires);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO
            	, "ISMTEST"+(Constants.DECODELTPATOKEN+1)
            	, "Full token string : " + token);
    }
}
