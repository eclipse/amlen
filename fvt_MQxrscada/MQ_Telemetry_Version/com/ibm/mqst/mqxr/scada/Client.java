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
import java.net.Socket;

public class Client
{
    private static int CLIENT_ID_COUNTER = 0;
    
    private Socket socket;
    private DataOutputStream output;
    private DataInputStream input;
    private String clientId;
    
    public Client(String host, int port) throws IOException
    {
        clientId = "Client" + CLIENT_ID_COUNTER++;
        socket = new Socket(host, port);
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
    
    protected void finalize() throws Throwable
    {
        socket.close();
    }
}
