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
 * Synchronization Server for LLM Driver Usage
 */
package test.llm.sync;

import java.io.IOException;
import java.net.ServerSocket;
import java.util.Scanner;
import java.util.concurrent.ConcurrentHashMap;
import java.util.concurrent.ConcurrentMap;

/**
 * SyncServer is a multi-threaded socket based synchronization agent. It
 * provides a means for distributed IPC on system namespaces or 'solutions.'
 * 
 * 
 */
public class SyncServer implements Runnable {
	protected String							name;
	protected ServerSocket						srvSocket;
	protected Thread							uThread;
	protected boolean							listening;
	protected ConcurrentMap<String, Solution>	solutions;
	
	/**
	 * General Use Constructor
	 * 
	 * @param name
	 *            (the address of the server)
	 * @param port
	 *            The Socket to which clients connect.
	 * @throws IOException
	 */
	public SyncServer(String name, int port) {
		// solutions = new HashMap<String, Solution>();
		solutions = new ConcurrentHashMap<String, Solution>();
		this.name = new String(name);
		try {
			srvSocket = new ServerSocket(port);
			listening = true;
		} catch (IOException e) {
			Logger.println("Failed first attempt at socket");
			Logger.println(e.getMessage());
			e.printStackTrace();
			try {
				Thread.sleep(1000);
			} catch (InterruptedException ie) {}
			try {  // Try one more time
				Logger.println("Try second attempt at socket");
				srvSocket = new ServerSocket(port);
				listening = true;
			} catch (IOException ioe) {
				Logger.println("Could not listen on port: " + port);
				Logger.println(e.getMessage());
				e.printStackTrace();
				
                showPortUsage(port);
				
				System.exit(-1);
			}
		}
		Logger.println("Server Created");
	}
	
	/**
	 * @port The port number
	 */
	private void showPortUsage(int port) {

        Logger.println("os.name: " + System.getProperty("os.name"));

        if(System.getProperty("os.name").contains("indows")) {
            Logger.println("not running netstat/ps/ss/lsof on Windows");
            return;
        }
               
        try_lsof("" + port);
        
        try_ss("" + port);

        try_netstat("" + port);
	}
	
	private void try_netstat(String port) {
		Logger.println(">> Attempting netstat, port: " + port);
		String result = execOutput("netstat", "-tulpn");
		for(String line : result.split("\n")) {
			if(line.contains(port)) {
				Logger.println("\t" + line);
				String pid = parsePidFromNetstat(line);
				ps_ef(pid);
			}
		}
	}
    
    private void try_lsof(String port) {
    	Logger.println(">> Attempting lsof, port: " + port);
    	String result = execOutput("lsof", "-i", "tcp:" + port);
    	if(result != null) {
    		for(String line : result.split("\n"))
    			if(line.contains(port)) {
    				Logger.println("\t" + line);
    				String pid = parsePidFromLsof(line);
    				ps_ef(pid);
    			}
    	} else
    		Logger.println("lsof failed");
    }
    
    private void try_ss(String port) {
    	Logger.println(">> Attempting ss, port: " + port);
    	String result = execOutput("ss", "-lnp");
    	if(result != null) {
	    	for(String line : result.split("\n"))
	    		if(line.contains(port)) {
	    			Logger.println("\t" + line);
	    			String pid = parsePidFromSS(line);
	    			ps_ef(pid);
	    		}
    	} else
    		Logger.println("ss failed");
    }
    
 
    // example `netstat -tulpn` output
    // tcp 0 0 0.0.0.0:55000 0.0.0.0:* LISTEN 32157/python
    
    private String parsePidFromNetstat(String line) {
        String[] toks = line.split("\\s+");
        if(toks.length == 0)
            return null;
        if(toks[toks.length-1].contains("/")) {
            toks = toks[toks.length-1].split("/");
            if(toks.length == 0)
                return null;
            String pid = toks[0];
            return pid;
        }
        return null;
    }
       
    // example `lsof -i :port` output
    // python 32157 root 3u IPv4 21453505 0t0  TCP *:55000 (LISTEN)
    
    private String parsePidFromLsof(String line) {
    	String[] toks = line.split("\\s+");
    	if(toks.length < 2)
    		return null;
    	return toks[1];
    }
    
    // example `ss -lnp` output
    // LISTEN 0 5 *:55000 *:* users:(("python",32157,3))

    private String parsePidFromSS(String line) {
    	String[] toks = line.split(",");
    	if(toks.length < 2)
    		return null;
    	return toks[1];
    }

    /**
     * @pid The process id for which to search for after running `ps -ef`
     */
    private void ps_ef(String pid) {
    	if(pid == null) {
    		Logger.println("\tno pid to run ps -ef");
    		return;
    	}
    	
    	String result = execOutput("ps", "-ef");
    	for(String line : result.split("\n"))
    		if(line.contains(pid))
    			Logger.println("\t" + line);
    }
    
    /**
     * 
     * @param command The command you wish to run, where each token in the command is an argument
     * @return The combined stdout and stderr output (in that order) from running command. 
     */
    private String execOutput(String... command) {
    	Runtime r = Runtime.getRuntime();
    	Process p = null;
    	String output = null;
    	try {
    		p = r.exec(command);
    		p.waitFor();
    		
    	} catch(Exception e) {
    		Logger.println(e.getMessage());
    		e.printStackTrace();
    	}
    	
    	if(p != null) {
    		Scanner stdout_buf = new Scanner(p.getInputStream());
    		Scanner stderr_buf = new Scanner(p.getErrorStream());
    		StringBuilder sb = new StringBuilder();
    		while(stdout_buf.hasNext())
    			sb.append(stdout_buf.nextLine()).append('\n');
    		while(stderr_buf.hasNext())
    			sb.append(stderr_buf.nextLine()).append('\n');
    		output = sb.toString();
    	}
    	
    	return output;
    }


	/**
	 * Need to insert extra layer in thread for timing out the socket.
	 */
	public void run() {
		Logger.println("Server Started");
		while (listening) {
			try {
				uThread = new Thread(new SyncRequestHandler(srvSocket.accept(), this));
				uThread.start();
			} catch (IOException e) {
				e.printStackTrace();
				System.exit(-1);
			}
		}
		try {
			srvSocket.close();
			Logger.println("Server Stopped!");
		} catch (IOException e) {
			Logger.println("Could not close.");
			System.exit(-1);
		}
	}
	
	/**
	 * Stop the server
	 */
	public void stop() {
		listening = false;
	}
	
	/**
	 * Obtain the name of the server
	 * 
	 * @return Name of Server
	 */
	public String getName() {
		return this.name;
	}
	
	/**
	 * Obtain the listen port of this server
	 * 
	 * @return localListenPort
	 */
	public int getPort() {
		return srvSocket.getLocalPort();
	}
	
	/**
	 * Get the named Solution stored on this server. It will be created if
	 * non-existent
	 * 
	 * @param solID
	 *            Name of the target solution
	 * @return named Solution
	 */
	public Solution getSolution(String solID) {
		synchronized (solutions) {
			Solution input = new Solution();
			Solution output;
			output = solutions.putIfAbsent(new String(solID), input);
			if (output == null) {
				return input;
			} else {
				return output;
			}
		}
	}
}
