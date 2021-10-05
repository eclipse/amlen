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
package com.ibm.mqst.mqxr.scada;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;

public class Client
{
    private static int CLIENT_ID_COUNTER = 0;
    
    private Socket socket;
    private DataOutputStream output;
    private DataInputStream input;
    final private String 	clientId;
    final private String  	remoteAddress; 
    final private String  	localAddress; 
    final private int		remotePort;
    final private int		localPort;
    public Client(String host, int port) throws IOException
    {
        clientId = "Client" + CLIENT_ID_COUNTER++;
        socket = new Socket(host, port);
        localPort = socket.getLocalPort();
        localAddress = String.valueOf(socket.getLocalAddress());
        remotePort = socket.getPort();
        remoteAddress = String.valueOf(socket.getInetAddress());
        socket.setSoTimeout(SCADAutils.timeout);
        output = new DataOutputStream(socket.getOutputStream());
        input = new DataInputStream(socket.getInputStream());
    }
    
    public DataInputStream getInputStream()
    {
        return input;
    }
    
    public DataOutputStream getOutputStream()
    {
        return output;
    }

    public String getClientId()
    {
        return clientId;
    }
    
    public boolean isSocketClosed() {
    	if(socket != null)
    		return socket.isClosed();
    	else
    		return true;
    }

    public boolean isSocketConnected() {
    	if(socket != null)
    		return socket.isConnected();
    	else
    		return false;
    }
    
    protected void finalize() throws Throwable
    {
    	if(socket != null) {
    		input = null;
    		output = null;
    		socket.close();
    		System.out.println("Connection from " + localAddress + ':' +localPort +
    				" to " + remoteAddress + ':' + remotePort + " was closed." );
    		socket = null;
    	} else {
    		System.out.println("!!! Closed connection from " + localAddress + ':' +localPort +
    				" to " + remoteAddress + ':' + remotePort + " was closed." );
    	}
    }
}
