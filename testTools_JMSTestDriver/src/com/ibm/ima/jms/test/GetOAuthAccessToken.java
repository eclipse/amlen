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

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLEncoder;

import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.HttpsURLConnection;
import javax.net.ssl.SSLSession;

import org.w3c.dom.Element;

import com.ibm.ima.jms.test.TrWriter.LogLevel;
import com.ibm.json.java.JSONObject;

/**
 * Retrieve an OAuth 2 AccessToken from the MessageSightOAuth OAuth provider
 * that we have configured in WAS.
 * The provider in WAS will have OAuth clients defined.
 * clientId and clientSecret are used to specify one of the OAuth clients.
 * username and password are used to specify a WAS user.
 * 
 * Ex. I have OAuth client "imaclient" and make the token request as WAS user
 * "admin".
 * 
 * Creating the HttpsUrlConnection also requires having the WAS certificate
 * in our truststore...
 * 
 * TODO: There are other forms of OAuth authentication than "password".
 * 
 *
 */
public class GetOAuthAccessToken extends Action{
	private final String charset = "UTF-8";
	private String accessToken;
	private String _uri;
	private String _clientId;
	private String _clientSecret;
	private String _username;
	private String _password;
	//	private static String scope = "";
	//	private static String state = "";
	private String tokenEndPoint;

	static {
		HttpsURLConnection.setDefaultHostnameVerifier(new HostnameVerifier()
		{
			public boolean verify(String hostname, SSLSession session)
			{
				// I don't care that our automation WAS servers have self-signed certificates.
				// Just let me in please :(
				return true;
			}
		});
	}

	public GetOAuthAccessToken(Element config, DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_uri = _actionParams.getProperty("WAS_URI");
		_clientId = _actionParams.getProperty("clientId", "imaclient");
		_clientSecret = _actionParams.getProperty("clientSecret", "password");
		_username = _actionParams.getProperty("username", "admin");
		_password = _actionParams.getProperty("password", "admin");
	}

	@Override
	protected boolean run() {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "Request OAuth 2 AccessToken from WAS.");

		try {
			// Get just the IP
			_uri = EnvVars.replace(_uri);
			tokenEndPoint = "https://" + _uri + "/oauth2/endpoint/MessageSightOAuth/token";
			String grant_type = "password";//"client_credentials";
			//String query = String.format("grant_type=%s&scope=%s&client_id=%s&client_secret=%s",
			String query = String.format("grant_type=%s&client_id=%s&client_secret=%s&username=%s&password=%s",
					URLEncoder.encode(grant_type, charset),
					//URLEncoder.encode(scope, charset),
					URLEncoder.encode(_clientId, charset),
					URLEncoder.encode(_clientSecret, charset),
					URLEncoder.encode(_username, charset),
					URLEncoder.encode(_password, charset));
			//send Resource Request using (accessToken);
			sendRequestForAccessToken(query);
			if (accessToken != null)	{
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "Retrieved Access Token: " + accessToken);
			} else {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Failed to get Access Token");
				return false;
			}
		} catch (Exception exc) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD0000", "Exception thrown in GetOAuthAccessToken: " 
					+ exc.getMessage());
			exc.printStackTrace();
			return false;
		}
		_dataRepository.storeVar("OAUTH_ACCESS_TOKEN", accessToken);
		return true;
	}

	private void sendRequestForAccessToken(String query) throws IOException {
		URL url = new URL(tokenEndPoint);
		HttpsURLConnection con = (HttpsURLConnection)url.openConnection();
		con.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset="  + charset);
		con.setDoOutput(true);
		con.setRequestMethod("POST");
		OutputStream output = null;
		try {
			output = con.getOutputStream();
			output.write(query.getBytes(charset));
			output.flush();
		} finally {
			if (output != null) try {
				output.close();
			} catch (IOException logOrIgnore) {}
		}
		con.connect();
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "response message is = " + con.getResponseMessage());
		// read the output from the server
		BufferedReader reader = null;
		StringBuilder stringBuilder;
		reader = new BufferedReader(new InputStreamReader(con.getInputStream()));
		stringBuilder = new StringBuilder();
		String line = null;
		try {
			while ((line = reader.readLine()) != null) {
				stringBuilder.append(line + "\n");
			}
		} finally {
			if (reader != null) try {
				reader.close();
			} catch (IOException logOrIgnore) {}
		}
		String tokenResponse = stringBuilder.toString();
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000", "response is = " + tokenResponse);
		JSONObject json = JSONObject.parse(tokenResponse);
		if (json.containsKey("access_token")) {
			/* Pass the entire JSON response as password instead of just access_token */
			//accessToken = (String)json.get("access_token");
			accessToken = tokenResponse;
		}
	}
}
