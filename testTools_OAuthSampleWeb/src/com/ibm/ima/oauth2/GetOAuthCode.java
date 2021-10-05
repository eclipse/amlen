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
package com.ibm.ima.oauth2;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.OutputStream;
import java.net.URL;
import java.net.URLEncoder;
import java.util.Enumeration;

import javax.net.ssl.HttpsURLConnection;
import javax.servlet.ServletException;

import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;

import com.ibm.json.java.JSONObject;

/**
 * Servlet implementation class GetOAuthCode
 * 
 * Make sure to add the admin user in security role mapping
 */
@WebServlet("/GetOAuthCode")
public class GetOAuthCode extends HttpServlet {
	private static final long serialVersionUID = 1L;


	private static final String ACCESS_TOKEN = "ACCESS_TOKEN";
	private static final String ACCESS_TOKEN_JSON = "ACCESS_TOKEN_JSON";
	private static final String INSTANCE_URL = "INSTANCE_URL";
	private static final String CHARSET = "UTF-8";

	private String clientId = "imaclient";
	private String clientSecret = "password";
	private String grant_type = "authorization_code";
	private String authURI = "/oauth2/endpoint/MessageSightOAuth/authorize?response_type=code&client_id=" + clientId;
	private String tokenURI = "/oauth2/endpoint/MessageSightOAuth/token";
	
	/**
	 * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		/* Print some debug info */
		System.out.println("getRequestURL() - " + request.getRequestURL());
		System.out.println("getRequestURI() - " + request.getRequestURI());
		System.out.println("getContextPath() - " + request.getContextPath());
		System.out.println("getQueryString() - " + request.getQueryString());
		Enumeration<String> params = request.getParameterNames();
		while (params.hasMoreElements()) {
			System.out.println("Parameter: " + params.nextElement());
		}
		Enumeration<String> attrs = request.getAttributeNames();
		while (attrs.hasMoreElements()) {
			System.out.println("Attribute: " + attrs.nextElement());
		}
		
		/* Start doing some actual work */
		String uri = request.getScheme() + "://" +
	             request.getServerName() +
	             ":" +                   
	             request.getServerPort();
		
		String accessToken = (String) request.getSession().getAttribute(ACCESS_TOKEN);

		/* If we already have an access token, we should be all set */
		if (accessToken == null) {
			String instanceUrl = null;
			String code = request.getParameter("code");

			/* We haven't received an authorization code yet */
			if (code == null) {
				/* So we need to send the user to authorize */
				response.sendRedirect(uri + authURI);//authUrl);
				return;
			} else {
				/* The user is authorized. Request an access token */
				System.out.println("Authorization successful");

				String query = String.format("code=%s&grant_type=%s&client_id=%s&client_secret=%s",
						URLEncoder.encode(code, CHARSET),
						URLEncoder.encode(grant_type, CHARSET),
						URLEncoder.encode(clientId, CHARSET),
						URLEncoder.encode(clientSecret, CHARSET));

				try {
					URL url = new URL(uri + tokenURI);//tokenUrl);
					HttpsURLConnection con = (HttpsURLConnection)url.openConnection();
					con.setRequestProperty("Content-Type", "application/x-www-form-urlencoded;charset="  + CHARSET);
					con.setDoOutput(true);
					con.setRequestMethod("POST");
					OutputStream output = null;
					try {
						output = con.getOutputStream();
						output.write(query.getBytes(CHARSET));
						output.flush();
					} finally {
						if (output != null) try {
							output.close();
						} catch (IOException logOrIgnore) {}
					}
					con.connect();
					/* Read the output from the server */
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
					JSONObject json = JSONObject.parse(tokenResponse);
					if (json.containsKey("access_token")) {
						/* Retrieve the access_token from the JSON */
						accessToken = (String)json.get("access_token");
						// Set a session attribute so that other servlets can get the access
						// token
						System.out.println("Access Token: " + tokenResponse);
						request.getSession().setAttribute(ACCESS_TOKEN, accessToken);
						request.getSession().setAttribute(ACCESS_TOKEN_JSON, tokenResponse);
					}
				} catch (IOException e) {
					e.printStackTrace();
				}
			}

			// We also get the instance URL from the OAuth response, so set it
			// in the session too
			request.getSession().setAttribute(INSTANCE_URL, instanceUrl);
		}

		if (accessToken != null) {
			/* Just to end the testing.. go to a page telling us we passed~ */
			response.sendRedirect(request.getContextPath() + "/Home.jsp?access_token=" + accessToken);
		} else {
			response.sendRedirect(request.getContextPath() + "/Error.jsp");
		}
	}

	/**
	 * @see HttpServlet#doPost(HttpServletRequest request, HttpServletResponse response)
	 */
	protected void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
		doGet(request, response);
	}

}
