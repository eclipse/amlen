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
package test.llm;

import java.io.BufferedReader;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.net.UnknownHostException;

import test.llm.sync.SyncClient;

/**
 * 
 */
public class TestClient extends SyncClient {
	protected BufferedReader stdIn;

	public TestClient(String server, String solution, int port)
			throws UnknownHostException, IOException {
		super(server, solution, port);
		this.connect();
	}

	public void start() throws IOException {
		String userInput;
		String inString;
		out = new PrintWriter(mySocket.getOutputStream(), true);
		in = new BufferedReader(
				new InputStreamReader(mySocket.getInputStream()));
		stdIn = new BufferedReader(new InputStreamReader(System.in));
		boolean err;
		System.out.println("Enter a command: (request condition [value] [timeout])");

		/* loop */
		while ((userInput = stdIn.readLine()) != null) {
			if (!userInput.equals("quit") && !userInput.equals("q")) {
				// if not equal to quit and not equal to q
				if (userInput.indexOf(" ") != -1) {
					userInput = userInput.substring(0, userInput.indexOf(" "))
							+ " " + solution
							+ userInput.substring(userInput.indexOf(" "));
				} else {
					userInput = userInput.concat(" " + solution);
				}
			}
			System.out.println("userInput: " + userInput);
			System.out.println("length: " + userInput.length());

			err = sendMessage(userInput);
			if (err) {
				System.out.println("Error found, closing");
				break;
			}
			if (userInput.equals("quit") || userInput.equals("q")) {
				// don't prompt for a new command
				break;
			}

			inString = getResponse();
			if (inString == null) {
				System.out.println("Stream Closed");
				break;
			}

			System.out.println(inString);
			System.out.println("Enter a command: " + "(request condition [value] [timeout], q to quit)");

		}
		/* end loop */

		out.close();
		in.close();
		stdIn.close();
		mySocket.close();
	}

	public void runCommand(String userInput) throws IOException {
		String inString;
		out = new PrintWriter(mySocket.getOutputStream(), true);
		in = new BufferedReader(
				new InputStreamReader(mySocket.getInputStream()));
		boolean err;

		if (userInput.indexOf(" ") != -1) {
			userInput = userInput.substring(0, userInput.indexOf(" "))
					+ " " + solution
					+ userInput.substring(userInput.indexOf(" "));
		} else {
			userInput = userInput.concat(" " + solution);
		}
			
		System.out.println("userInput: " + userInput);
		System.out.println("length: " + userInput.length());

		err = sendMessage(userInput);
		if (err) {
			System.out.println("Error found, closing");
		}

		inString = getResponse();
		if (inString == null) {
			System.out.println("Stream Closed");
		}
		
		System.out.println(inString);
		
		out.close();
		in.close();
		mySocket.close();
	}

	/**
	 * @param args
	 * @throws IOException
	 * @throws UnknownHostException
	 */
	public static void main(String[] args) {
		String name, log, solution, userInput;
		int port;

		name = "127.0.0.1";
		port = 0;
		solution = "unknown";
		userInput = "";

		try {
			if (args.length < 1) {
				usage();
				System.exit(-1);
			}
			port = Integer.parseInt(args[0]);

			for (int i = 1; i < args.length; i++) {
				if (args[i].equals("-n")) {
					name = args[++i];
					continue;
				}
				if (args[i].equals("-s")) {
					solution = args[++i];
					continue;
				}
				if (args[i].equals("-f")) {
					log = args[++i];
					continue;
				}
				if (args[i].equals("-u")) {
					userInput = args[++i];
					while (i < args.length-1) {
						if (args[i+1].startsWith("-")) {
							break;
						} else {
							userInput = userInput + " " + args[++i];
						}
					}
					continue;
				}
				usage();
				System.exit(-1);
			}
			TestClient test = new TestClient(name, solution, port);
			if (userInput.length() == 0 ){
				test.start();
			} else {
				test.runCommand(userInput);
			}
		} catch (NumberFormatException e) {
			System.err.println("Argument must be an integer");
			System.exit(-1);
		} catch (Throwable t) {
			t.printStackTrace();
		}
	}

	private static void usage() {
		System.err.println("Usage: TestClient <port> -n [name] -s [solution] -f [logfile] -u [userinput]");
		System.err.println("       -n   name of desired server (IP address)");
		System.err.println("       -s   defines component namespace, defaults to \"unknown\"");
		System.err.println("       -f   name of logfile");
		System.err.println("       -u   userInput (request condition [value] [timeout])");
		System.err.println("            request: 1=INIT, 2=SET, 3=WAIT, 4=RESET, 5=DELETE, 6=GET");
	}

	@Override
	public String buildRequest(String type, String condition, String value, String timeout) {
		return null;

	}

	@Override
	public String SendRequest(String req) throws IOException {
		return null;
	}

}
