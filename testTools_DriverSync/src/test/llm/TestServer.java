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

import test.llm.sync.SyncServer;

public final class TestServer {
	
	/**
	 * Local Test For Server
	 * 
	 * @param args
	 */
	public static void main(String[] args) {
		int port = 40000;
		String name = "unknown";
		String log = null;
		
		try {
			if (args.length < 1){
				usage();
			}
			port = Integer.parseInt(args[0]);
			
			for(int i = 1; i < args.length; i++){
				if(args[i].equals("-n")){					
					name = args[++i];
					continue;
				}
				if(args[i].equals("-f")){					
					log = args[++i];
					continue;
				}
				usage();
			}			
			SyncServer mine = new SyncServer(name, port);
			mine.run();	
		} catch (NumberFormatException e){
			System.err.println("Argument must be an integer");
			System.exit(-1);
		}catch (Throwable t) {
			t.printStackTrace();
		}		
	}
	
	private static void usage(){
		System.err.println("Usage: TestServer <port> -n [name] -f [logfile]");
	}
	
}
