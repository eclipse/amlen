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

public class client {
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
        if (args.length != 2) {
            System.out.println("args.length is" + args.length);
            System.out.println("usage:  " + "client server port");
            System.exit(0);
        } else {
            String server = args[0];
            String port = args[1];

            System.out.println("MessageSight server is" + server);
            System.out.println("MessageSight port is" + port);

            client myclient = new client();
            if (myclient != null) {
                myclient.run(server, port);
            }
        }
    }

    void run(String server, String port) {
        String user = "USER1";
      String clientTopic = "Joe}";
//        String clientTopic = "/USER/USER1/APP/GetAvailableCars";
//      String serverTopic = "/APP/GetAvailableCars/USER/%s";
        String serverTopic = "**";
        String request = "{" + "\"CarType\":\"ALL\"," + "\"Date\":\"20131004\"," + "\"searchRadius\":\"10\","
                        + "\"latitude\":\"30\"," + "\"longitude\":\"-95\"}";

        try {
            Sender sender = new Sender("**");
            Receiver receiver = new Receiver("Joe");

            sender.connect(server, port);
            receiver.connect(server, port, "topic", clientTopic);

            System.out.println("client request: " + request);
            sender.doSendString("topic", String.format(serverTopic, user), request);
            String body = receiver.doReceiveBody();
            System.out.println("server response: " + body);

            receiver.close();
            sender.close();
        } catch (Throwable ex) {
            ex.printStackTrace();
        }

//        String car = "WKRP567";
//        String client2Topic = "/USER/USER1/APP/ReserveCar/CAR/" + car;
//        String server2Topic = "/APP/ReserveCar/USER/%s/CAR/%s";
//
//        try {
//            Sender sender2 = new Sender("cl_s*");
//            Receiver receiver2 = new Receiver("cl_r*");
//
//            sender2.connect(server, port);
//            receiver2.connect(server, port, "topic", client2Topic);
//
//            String topic2 = String.format(server2Topic, user, car);
//            System.out.println("server topic: " + topic2);
//            sender2.doSendString("topic", topic2, "");
//            String body2 = receiver2.doReceiveBody();
//            System.out.println("server response: " + body2);
//
//            receiver2.close();
//            sender2.close();
//        } catch (Throwable ex) {
//            ex.printStackTrace();
//        }

    }
}
