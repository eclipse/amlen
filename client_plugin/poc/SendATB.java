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
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.InetSocketAddress;
import java.net.Socket;


public class SendATB {
    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    static String server = "10.10.0.5";
    static int    port   = 16109;
    static InetSocketAddress ipaddr;
    static OutputStream  ostream;
    static InputStream   istream;
    static Socket        sock;
    static byte []       readbuf;
    private static final String hexdigits = "0123456789abcdef";
    
    public static void main(String [] args) throws Exception {
        ipaddr = new InetSocketAddress(server, port);
        sock = new Socket();
        sock.setTcpNoDelay(true);
        sock.connect(ipaddr);

        ostream = sock.getOutputStream();
        istream = sock.getInputStream();
        readbuf = new byte[2048];
        
        int i;
        int out;
        if (args.length < 1) {
            System.out.println("packet file required");
            return;
        }
        byte [] fdata = null;
        try {
            File f = new File(args[0]);
            FileInputStream fis = new FileInputStream(f);
            fdata = new byte[(int)f.length()];
            fis.read(fdata);
            fis.close();
        } catch (Exception e) {
            e.printStackTrace(System.err);
        }

        byte [] data = new byte[(fdata.length/2)];
        out = 0;
        for (i=0; i<fdata.length; i+=2) {
            int hex1 = hexdigits.indexOf(fdata[i]);
            int hex2 = hexdigits.indexOf(fdata[i+1]);
            data[out] = (byte)((hex1<<4) + hex2);
            out++;
        }

        String cmd = "POST /atb HTTP/1.1\r\n" + 
                     "Host: " + server + "\r\n" +
                     "Content-type: application/octet-stream\r\n" +
                     "Content-length: " + out + "\r\n\r\n";
        byte [] cmdbytes = cmd.toString().getBytes("UTF-8");
        System.out.println("send ["+cmd+"]");
        ostream.write(cmdbytes, 0, cmdbytes.length);
        ostream.write(data, 0, data.length);

        int bread = istream.read(readbuf, 0, readbuf.length);
        if (bread >= 0)
            System.out.println("recv ["+new String(readbuf, 0, bread, "UTF-8")+"]");
    }
}
