package com.ibm.mqst.mqxr.scada;
import java.util.Vector;

/*
 * Copyright (c) 2002-2021 Contributors to the Eclipse Foundation
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
 * @version 1.0
 */
/*
 * This class represents a list of topics and requested QoS levels for
 * subscriptions
 */

public class SubList
{

    Vector topics;

    Vector reqQos;

    // Constructor - adds in 1 topic/QoS pair
    public SubList(String topic, byte qos)
    {

        topics = new Vector();
        reqQos = new Vector();
        addSub(topic, qos);

    }

    // Add in another Topic/QoS pair
    public void addSub(String topic, byte qos)
    {

        byte[] topicUTF = SCADAutils.StringToUTF(topic);

        topics.add(topicUTF);
        reqQos.add(new Byte(qos));

    }

    // Dumps all information to byte array for use in SUBSCRIBE msgs
    public byte[] toByteArray()
    {

        int size = 0;

        for (int i = 0; i < topics.size(); i++)
        {
            byte[] tmp = (byte[]) topics.elementAt(i);
            size += tmp.length;
            size++; // For associated QoS
        }

        byte[] result = new byte[size];

        int offset = 0;

        for (int i = 0; i < topics.size(); i++)
        {

            byte[] tmp = (byte[]) topics.elementAt(i);
            byte qosTmp = ((Byte) reqQos.elementAt(i)).byteValue();

            for (int j = 0; j < tmp.length; j++)
            {
                result[offset] = tmp[j];
                offset++;
            }
            result[offset] = qosTmp;
            offset++;
        }

        return result;
    }

    // Dumps all QoS levels into byte array
    public byte[] qosArray()
    {

        byte[] result = new byte[reqQos.size()];

        for (int i = 0; i < reqQos.size(); i++)
        {
            result[i] = ((Byte) reqQos.elementAt(i)).byteValue();
        }

        return result;

    }

}
