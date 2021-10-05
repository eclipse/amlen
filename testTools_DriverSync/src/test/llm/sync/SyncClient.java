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
/**
 * 
 */
package test.llm.sync;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.net.Socket;
import java.net.UnknownHostException;

/**
 * This is a basic client that takes input from command line to demonstrate
 * functionality of the Sync Solution
 */
public abstract class SyncClient {
	public int port;
	protected Socket mySocket;
	public String serverName;
	public String solution;
	protected PrintWriter out;
	protected BufferedReader in;

	public SyncClient(String server, String solution, int port) {
		serverName = new String(server);
		this.solution = new String(solution);
		this.port = port;
		mySocket = new Socket();
	}

	/**
	 * Establishes Socket Connection
	 * 
	 * @throws UnknownHostException
	 * @throws IOException
	 */
	public void connect() throws UnknownHostException, IOException {
		// mySocket.setReuseAddress(true);
		// mySocket.bind(null);
		mySocket = new Socket(serverName, port);
		Logger.println("Connected to: "
				+ mySocket.getInetAddress().getHostAddress() + " @ "
				+ mySocket.getPort());
	}

	/**
	 * Builds a request from the given input
	 * 
	 * @param type
	 * @param condition
	 * @param value
	 * @param timeout
	 * @return a string representing the request, or empty string on failure
	 */
	public abstract String buildRequest(String type, String condition,
			String value, String timeout);

	/**
	 * Send a request to the Server, implementers will likely perform analysis
	 * as well
	 * 
	 * @param req
	 *            Request to be sent (generated from buildRequest)
	 * @return response string
	 * @throws IOException
	 */
	public abstract String SendRequest(String req) throws IOException;

	/**
	 * A routine available most likely for use in send request / handling result
	 * 
	 * @return response string from server
	 * @throws IOException
	 */
	protected String getResponse() throws IOException {
		return in.readLine();
	}

	/**
	 * A routine available to send a message to the server, to be used within
	 * SendRequest
	 * 
	 * @param msg
	 *            A fully validated message that is ready ready to be sent.
	 * @return returns true if an error occurred
	 */
	protected boolean sendMessage(String msg) {
		out.write(msg.length());
		out.print(msg);
		return out.checkError();
	}

}
