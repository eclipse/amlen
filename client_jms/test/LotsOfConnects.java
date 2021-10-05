/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.JMSException;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;


public class LotsOfConnects {
    static ConnectionFactory fact;
    static Connection conn;
    static Session    sess;
    static int    conncount = 3000;
    
    
    public static void parseArgs(String[] args) {
        if (args.length > 0) {
           try {
                conncount = Integer.valueOf(args[0]);
            } catch (Exception e) {
                System.out.println("The count is not valid");
                return;
            }    
        }
    }
    
    
    /*
     * Main method
     */
    public static void main(String[] args) {

        /*
         * Create connection factory and connection
         */
        try {
            fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("IMAServer");
            String port = System.getenv("IMAPort");
           
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties) fact).put("Server", server);
            ((ImaProperties) fact).put("Port", port);
            
            ((ImaProperties) fact).put("ExpireConnection", 2);
            for (int i=0; i<conncount; i++) {
            	((ImaProperties)fact).put("ClientID", "Lots_"+(i+1));
                conn = fact.createConnection();
                sess = conn.createSession(false, 0);
                if ((i % 10) == 0) {
                	System.out.println("Connection " + i);
                }
            }   
            try {Thread.sleep(20000);} catch (Exception e) {}
            System.out.println("exit");
            System.exit(0);
        } catch (JMSException jmse) {
            jmse.printStackTrace(System.out);
            System.exit(0);
        }
    }
        
}
