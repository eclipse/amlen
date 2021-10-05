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
package svt.server;

import javax.jms.Connection;
import javax.jms.Message;
import javax.jms.TextMessage;

public class server {
    public String action = null;
    public Connection connection = null;
    public String clientId = null;
    public String userName = null;
    public char[] password = null;
    public int throttleWaitMSec = 0;
    public int count = 1;
    public String payload = "Sample Message";
    public boolean persistence = false;
    public boolean isDurable = false;
    public boolean deleteDurableSubscription = false;
    public String scheme = null;
    public String port = null;
    public String server = null;
    public String defaultTopicName = "/JMSSample";
    public String destName = null;
    public String destType = null;
    public boolean verbose = false;
    public boolean disableACKS = false;
    public long timeout = 0;

    /**
     * @param args
     */
    public static void main(String[] args) {
        if (args.length != 5) {
            System.out.println("args.length is" + args.length);
            System.out.println("usage:  " + "server <server> <port> <servertopic> <clienttopic> <body>");
            System.exit(0);
        } else {
            String server = args[0];
            String port = args[1];
            String serverTopic = args[2];
            String clientTopic = args[3];
            String body = args[4];

            System.out.println("MessageSight server is " + server);
            System.out.println("MessageSight port is " + port);

            server myserver = new server();
            if (myserver != null) {
                myserver.run(server, port, serverTopic, clientTopic, body);
            }
        }
    }

    void run(String server, String port, String serverTopic, String clientTopic, String body) {
        String user = null;
        String topic = null;

        try {
            Sender sender = new Sender("sv_s*");
            Receiver receiver = new Receiver("sv_r*");

            sender.connect(server, port);
            receiver.connect(server, port, "topic", serverTopic);

          Message m = receiver.doReceiveMessage();
          while (true) {
              System.out.printf("Received '%s' on topic '%s'\n", ((TextMessage)m).getText(), serverTopic);
              Thread.currentThread().sleep(10000);
              System.out.printf("Sending '%s' on topic '%s'\n", body, clientTopic);
                  sender.doSendString("topic", "${ClientID}", body);
                  m = receiver.doReceiveMessage();
          }

          //            user = receiver.doReceiveUser();
//            while (true) {
//                if (user != null) {
//                    topic = String.format(clientTopic, user);
//                    // Thread.sleep(300);
//                    sender.doSendString("topic", String.format(clientTopic, user), body);
//                }
//                user = receiver.doReceiveUser();
//            }

            // receiver.close();
            // sender.close();
        } catch (Throwable ex) {
            ex.printStackTrace();
        }
    }
}
