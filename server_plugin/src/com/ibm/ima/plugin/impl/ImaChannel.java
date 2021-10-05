/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.plugin.impl;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;
import java.nio.ByteBuffer;

/*
 * A channel is a connection from the server to the plug-in processor.  
 * There is normally one per IOP thread and others as required.
 * The channel runs in non-blocking mode and this class represents a thread.
 */
public class ImaChannel extends Thread {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    Socket sock;
    InputStream in;
    OutputStream out;
    int  id = 0;
    boolean      closed = false;
    
    /*
     * Construct the thread
     */
    ImaChannel(Socket sock) throws IOException {
        this.sock = sock;
        in = sock.getInputStream();
        out = sock.getOutputStream();
        setDaemon(true);
    }

    private void close() {
        if (!closed) {
            closed = true;
/*        
            Iterator<Entry<Integer, ImaConnectionImpl>> it = ImaConnectionImpl.connects.entrySet().iterator();
            while (it.hasNext()) {
                Entry<Integer, ImaConnectionImpl> entry = it.next();
                ImaConnectionImpl con = entry.getValue();
                if ((con.channel == this) && (con.listener != null)) {
                    // TODO: Use correct rc
                    con.listener.onClose(0, "Connection to IMA server is closed.");
                    it.remove();
                }
            }
            try {
                sock.close();
            } catch (IOException e) {
                ImaTrace.trace(4, "Error closing channel.");
                ImaTrace.traceException(4, e);
            }
*/
            /* Restart java process if communication with IMA server is broken */
            ImaPluginMain.terminate(0, "Connection to IMA server is closed.");
        }
    }
    
    /*
     * Set the channel ID
     */
    void setID(int id) {
        this.id = id;
    }
    
    /*
     * Run the processing thread
     */
    public void run() {
        int pos;
        int readlen;
        int datalen;
        Exception e = null;
        byte[] result = new byte[32 * 1024];

        while (!closed && (e == null)) {
            try {
                readlen = 0;
                int read_bytes = 0;
                while (readlen < 6){
                    read_bytes = in.read(result, readlen, 6-readlen);
                    if (read_bytes < 0) {
                        ImaTrace.trace(4, "Connection closed by client: " + id);
                        break;
                    }
                    readlen += read_bytes;
                }
                if (read_bytes < 0)
                    break;
            } catch (IOException ioe) {
                e = ioe;
                break;
            }
            datalen = getIntBig(result, 2);
            if (result[1] > 15 || datalen > 0xffffff) {
                e = new IOException("The connection data not valid: " + id);  
                break;
            }

            if ((datalen + 6) >= result.length) {
                byte ba[] = new byte[datalen + 1000];
                System.arraycopy(result, 0, ba, 0, 6);
                result = ba;
            }
            datalen += 6;
            pos = 6;
            for (;;) {
                try {
                    readlen = in.read(result, pos, datalen-pos);
                    if (readlen < 0) {
                        ImaTrace.trace(4, "Connection closed by client: " + id);
                        break;  
                    } else {
                        pos += readlen;
                        if (pos == datalen) {
                            ImaPluginAction act = new ImaPluginAction(this, result, datalen);
                            // System.out.println("process action: sock=" + sock);
                            act.process();
                            break;
                        }
                    }    
                } catch (IOException ioe) {
                    e = ioe;
                    break;
                }
            }
        } 
        if (!closed) {
            String err = "Channel closed: id=" + id;
            if (e != null)
                err += ' ' + e.toString();
            ImaTrace.trace(4, err);
            close();
        }
    }
    
    
    /*
     * Write a byte array 
     */
    void send(ByteBuffer bb) {
        byte [] x = bb.array();
        try {
            out.write(x, 0, bb.limit());
        } catch (IOException e) {
            ImaTrace.trace(4, "Error sending data to server.");
            ImaTrace.traceException(4, e);
            close();
        }
    }

    /*
     * Get a big-endian int
     */
    static int getIntBig(byte[] buff, int offset) {
        return ((buff[offset]&0xff)<<24)   |
               ((buff[offset+1]&0xff)<<16) |
               ((buff[offset+2]&0xff)<<8)  |
                (buff[offset+3]&0xff);      
    }

    
    /*
     * @see java.lang.Thread#toString()
     */
    public String toString() {
        return "ImaChannel id=" + id + " sock=" + sock;
    }
}
