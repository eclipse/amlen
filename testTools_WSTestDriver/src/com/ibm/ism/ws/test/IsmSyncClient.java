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
 * File        : IsmSyncClient.java
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.UnknownHostException;

import test.llm.sync.Request;
import test.llm.sync.SyncClient;


/**
 * Concrete subclass of {@link SyncClient} Integrated client for test driver
 * 
 * 
 */
public class IsmSyncClient extends SyncClient {
	private static IsmSyncClient _syncClient = null;
	static final Request INIT_REQUEST;
	static final Request SET_REQUEST;
	static final Request WAIT_REQUEST;
	static final Request RESET_REQUEST;
	static final Request DELETE_REQUEST;
	static final Request GET_REQUEST;
	static {
		try {
			INIT_REQUEST = new Request(1);
			SET_REQUEST = new Request(2);
			WAIT_REQUEST = new Request(3);
			RESET_REQUEST = new Request(4);
			DELETE_REQUEST = new Request(5);
			GET_REQUEST = new Request(6);
		} catch (Throwable t) {
			t.printStackTrace();
			throw new RuntimeException(t);
		}
	}

	/**
	 * @param server
	 * @param solution
	 * @param port
	 */
	private IsmSyncClient(String server, String solution, int port) {
		super(server, solution, port);
	}

	static synchronized void init(String server, String solution, int port,
			TrWriter trWriter) throws IsmTestException {
		if (_syncClient != null)
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT),
					"IsmSyncClient is already initialized.");
		_syncClient = new IsmSyncClient(server, solution, port);
		try {
			long endTime = System.currentTimeMillis() + 5000;
			boolean connected = false;
			Exception lastException = null;
			do {
				try {
					_syncClient.connect();
					connected = true;
					break;
				} catch (UnknownHostException u) {
					throw u;
				} catch (IOException e) {
					lastException = e;
					try {
						Thread.sleep(500);
					} catch (Throwable t) {
						trWriter.writeException(t);
					}
				}
			} while (System.currentTimeMillis() < endTime);
			if (!connected)
				throw lastException;
			_syncClient.out = new PrintWriter(
					_syncClient.mySocket.getOutputStream(), true);
			_syncClient.in = new BufferedReader(new InputStreamReader(
					_syncClient.mySocket.getInputStream()));
		} catch (Exception e) {
			trWriter.writeException(e);
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+1),
					"IsmSyncClient failed to connect to SyncServer: " + server
							+ ':' + port, e);
		}
	}

	static synchronized String invokeSyncRequest(Request reqType,
			String condition, int value, int timeout, TrWriter trWriter)
			throws IsmTestException {
		String result = null;
		StringBuffer request = new StringBuffer(256);
		request.append(reqType.getType()).append(' ')
				.append(_syncClient.solution);
		if (timeout < 0)
			timeout = 0;
		if (((condition == null) || (condition.length() == 0))
				&& (reqType.getType() != Request.RESET)) {
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+2),
					"SyncRequest is not valid. Condition is required for \""
							+ reqType + "\" request.");
		}
		switch (reqType.getType()) {
		case Request.RESET:
			break;
		case Request.WAIT:
			request.append(' ').append(condition).append(' ').append(value)
					.append(' ').append(timeout);
			break;
		case Request.SET:
			request.append(' ').append(condition).append(' ').append(value);
			break;
		case Request.INIT:
		case Request.DELETE:
		case Request.GET:
			request.append(' ').append(condition);
			break;
		}

		if (_syncClient.sendMessage(request.toString()))
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+3),
					"Send of SyncRequest failed.");
		try {
			result = _syncClient.getResponse();
		} catch (Exception e) {
			trWriter.writeException(e);
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+4),
					"Failed to get SyncServer response.");
		}
		if (result == null)
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+5),
					"Connection to SyncServer was closed.");
		return analyzeResponse(result);
	}

	@Override
	public String buildRequest(String type, String condition, String value,
			String timeout) {
		return null;
	}

	@Override
	public String SendRequest(String req) throws IOException {
		return null;
	}

	/**
	 * Inspects the response string from the server to determine if desired
	 * outcome occurred
	 * 
	 * For test driver throws an exception of response contains either an error
	 * or indication that a wait timed out
	 * 
	 * @param response
	 *            The string to be validated, as received from the server
	 */

	private static String analyzeResponse(String response)
			throws IsmTestException {
		if (response.contains("Error")) {
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+6),
					"An error was returned by the SyncServer");
		}
		if (response.contains("Waited: 0")) {
			throw new IsmTestException("ISMTEST"+(Constants.ISMSYNCCLIENT+7),
					"SyncRequest timed out.");
		}
		return response;
	}

}
