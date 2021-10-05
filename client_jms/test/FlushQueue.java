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
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.Queue;
import javax.jms.Session;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;


public class FlushQueue {
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	long wait = 50;
        String queuename = "MY_QUEUE";
        
        /*
         * Create connection factory and connection
         */
        try {
            ConnectionFactory fact = ImaJmsFactory.createConnectionFactory();
            String server = System.getenv("ImaServer");
            String port   = System.getenv("ImaPort");
            if (server == null)
                server = "127.0.0.1";
            if (port == null)
                port = "16102";
            ((ImaProperties)fact).put("Server", server);
            ((ImaProperties)fact).put("Port", port);
            Connection connect = fact.createConnection();
            Session session = connect.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            Queue queue = session.createQueue(queuename);
            MessageConsumer consumer = session.createConsumer(queue);
            connect.start();
            for (;;) {
            	Message msg = consumer.receive(wait);
            	if (msg == null)
            		break;
            	System.out.println(ImaJmsObject.toString(msg, "*b"));
            }
            connect.close();
        } catch (Exception ex) {
        	ex.printStackTrace(System.err);
        }
    }    
}
