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
import com.ibm.ism.mqtt.IsmMqttConnection;

public class CertIoT {

    public static void main(String [] args) throws Exception {
        IsmMqttConnection conn = new IsmMqttConnection("kwb5", 8883, "/mqtt", "d:x:typ:device");
        conn.setEncoding(IsmMqttConnection.TCP4);
        conn.setSecure(3);
        conn.setKeystore("keystore_good", null);
        conn.setHost("x.kwb5");
        conn.setUser("kwb", "password");
        conn.connect();
        System.out.println("after connect");
        Thread.sleep(2000);
        conn.disconnect();
        System.out.println("after disconnect");
    }
}
