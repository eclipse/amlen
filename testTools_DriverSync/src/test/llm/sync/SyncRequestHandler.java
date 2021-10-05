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
package test.llm.sync;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.Socket;

public class SyncRequestHandler implements Runnable {
	private Socket			socket;
	private String			solution;	/* target solution name */
	private String			condition;	/* target condition name */
	private long				state;		/* target state */
	long					timeout;	/* target timeout */
	private Request			request;	/* parsed request */
	private BufferedReader	in;
	private PrintWriter		out;
	private SyncServer		myServer;	/* pointer to parent server */
	
	/**
	 * Creates a thread to do all tasks for the connection accepted
	 * 
	 * @param socket
	 *            Accepted Socket received from SyncServer
	 * @param srv
	 *            Reference to parent SyncServer
	 */
	public SyncRequestHandler(Socket socket, SyncServer srv) {
		this.socket = socket;
		myServer = srv;
		state = -1;
		Logger.println("Connection Received from: " + socket.getInetAddress().getHostAddress()
				+ " @ " + socket.getPort());
	}
	
	/*
	 * (non-Javadoc)
	 * 
	 * @see java.lang.Thread#run()
	 */
	synchronized public void run() throws RuntimeException {
		String outputLine;
		String inputLine;
		int len = 0;
		int bRead = 0;
		
		try {
			out = new PrintWriter(socket.getOutputStream(), true);
			in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
			char[] myBuf = new char[1024];
			
			while (true) {
				len = (int) in.read();  //read the length first
				
				if (len == -1) {
					break; // exit
				}
				
				bRead = 0;				
				while (bRead != len && len != -1) {
					bRead += in.read(myBuf);					
				}
				
				if (bRead == 0) {
					break; // exit
				}
				
				inputLine = new String(myBuf, 0, len);
				Logger.println("GOT: " + inputLine + " from " + Thread.currentThread().getName() + " " + socket.getInetAddress().getHostAddress()
						+ " @ " + socket.getPort());
				
				if (inputLine.equals("quit") || inputLine.equals("q")) {
					// received quit command
					break;
				}
				
				try {
					parseRequest(inputLine);
					outputLine = handleRequest();
					out.println(outputLine);
					Logger.println("SENT: " + outputLine +  " to " + Thread.currentThread().getName() + " " + socket.getInetAddress().getHostAddress()
							+ " @ " + socket.getPort());
				} catch (ParseException e) { // tells client req failed
					outputLine = "Error: " + e.getMessage() + e.getInput();
					Logger.println("Non Terminal Exception: \n" + outputLine);
					e.printStackTrace();
					out.println(outputLine);
				} catch (RuntimeException e) { // tells client its over
					outputLine = "Fatal: Closing Connection " + e.getMessage();
					Logger.println(outputLine);
					out.println(outputLine);
					e.printStackTrace();
				}
			}
		} catch (IOException e) {
			e.printStackTrace();
		} finally {
			try {
				in.close();
				out.close();
				socket.close();
				Logger.println("SyncThread Ending! Socket Closed: "
						+ socket.getInetAddress().getHostAddress() + " @ " + socket.getPort());
			} catch (IOException e) {
				e.printStackTrace();
			}
			Logger.println("End of Thread");
		}
	}
	
	/**
	 * parses request, solution, condition from input. if any not found error
	 * thrown also parses value and timeout as appropriate for create and wait.
	 * 
	 * defer to runtime exception for null result / empty input;
	 * 
	 * @param inputLine
	 *            String of input read from Socket. Should be of form
	 *            "REQUEST SOLUTION CONDITION [VALUE] [TIMEOUT]"
	 * @throws ParseException
	 */
	private void parseRequest(String inputLine) throws NumberFormatException, ParseException {
		int i;
		String[] result = inputLine.split("\\s");
		try {
			for (i = 0; i < 2; i++) {
				if (result[i] == null || result[i].length() == 0) {
					ParseException p = new ParseException("Parse Failure on input: ", inputLine);
					throw p;
				}
			}
						
			request = new Request(Integer.parseInt(result[0]));
			solution = new String(result[1]);			
			
			if (result.length >= 3 && result[2] != null && result[2].length() != 0) {
				condition = new String(result[2]);
			}
			if (result.length >= 4 && result[3] != null && result[3].length() != 0) {
				state = Long.parseLong(result[3]);
			}
			if (result.length >= 5 && result[4] != null && result[4].length() != 0) { // 4
				// only
				// valid if
				// 3 was
				timeout = Long.parseLong(result[4]);
			}
		} catch (NumberFormatException e) {
			ParseException p = new ParseException("Parse Failure on input line: ", inputLine);
			p.initCause(e);
			throw p;
		} catch (ArrayIndexOutOfBoundsException e) {
			ParseException p = new ParseException("Parse Failure on input line: ", inputLine);
			p.initCause(e);
			throw p;
		}
	}
	
	/**
	 * Handle execution of task received
	 * 
	 * @return output string suitable for output to client
	 */
	private String handleRequest() {
		String out;
		int temp;
		long ltemp;
		Solution mySolution = myServer.getSolution(solution);
		switch (request.getType()) {
		case Request.INIT:
			ltemp = mySolution.getConditionState(condition);
			out = "Init: " + condition + " = " + ltemp;
			break;
		case Request.SET:
			ltemp = mySolution.setConditionState(condition, state);
			out = "Set: " + condition + " = " + ltemp;
			break;
		case Request.WAIT:
			temp = mySolution.waitForCond(condition, state, timeout);
			out = "Waited: " + temp + " :: " + condition + " = " + state;
			break;
		case Request.DELETE:
			mySolution.deleteCondition(condition);
			out = "Deleted: " + condition;
			break;
		case Request.RESET:
			mySolution.reset();
			out = "Reset: " + solution;
			break;
		case Request.GET:
			out = "Get: " + mySolution.getConditionState(condition);
			break;
		default:
			out = "Error: Nothing Happened";
			break;
		}
		return out;
	}
}
