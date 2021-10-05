/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
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
/*

set CLASSPATH=.;C:\Program Files (x86)\IBM\WebSphere MQ\java\lib\com.ibm.mq.jar;C:\Program Files (x86)\IBM\WebSphere MQ\java\lib\com.ibm.mqjms.jar
javac MQJMSPub.java
java MQJMSPub

*/

import java.util.Enumeration;
import javax.jms.*;
import com.ibm.mq.*;
import com.ibm.mq.jms.*;

public class MQJMSPub
{
  public static void main(String[] args)
  {
    try
    {
      System.out.println("Usage MQJMSPub <type> <persistent> <messages> <setreplyqueue>");

      // What kind of message should we create ?
      Integer mtype = 0;

      if (args.length > 0)
      {
        mtype = Integer.parseInt(args[0]);
      }

      // Should it be persistent ?
      Integer delmode = DeliveryMode.NON_PERSISTENT;
      boolean disableAck = false;

      if (args.length > 1)
      {
        switch (Integer.parseInt(args[1]))
        {
        case 0:
            /* DisableAck */
            disableAck = true;
            break;
        case 1:
            break;
        case 2:
        default:
            delmode = DeliveryMode.PERSISTENT;
            break;
        }
      }

      // How many messages ? 
      Integer messages = 1;

      if (args.length > 2)
      {
        messages = Integer.parseInt(args[2]);
        System.out.println("Putting " + messages + " message(s)");
      }

      // Should we pass a reply queue ?
      Integer replyqueue = 0;

      if (args.length > 3)
      {
        replyqueue = Integer.parseInt(args[3]);
      }

      MQConnectionFactory connectionFactory = new MQConnectionFactory();

      connectionFactory.setQueueManager("XP1");
      connectionFactory.setTransportType(JMSC.MQJMS_TP_CLIENT_MQ_TCPIP);
      connectionFactory.setChannel("SYSTEM.IMA.SVRCONN");
      connectionFactory.setHostName("127.0.0.1");

      Connection connection = connectionFactory.createConnection();
      System.out.println("connectionFactory.createConnection");

      Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
      System.out.println("connection.createSession");

      Topic topic = session.createTopic("MQTOPIC");
      System.out.println("session.createTopic");

      MessageProducer producer = session.createProducer(topic);
      System.out.println("session.createProducer");

      producer.setDeliveryMode(delmode);

      Message message;

      if (mtype == 1)
      {
        message = session.createBytesMessage();
        System.out.println("session.createBytesMessage");
        ((BytesMessage) message).writeUTF("MyBytesMessage");

        message.setJMSType("MyJMSType");
      }
      else if (mtype == 2)
      {
        message = session.createStreamMessage();
        System.out.println("session.createStreamMessage");
        ((StreamMessage) message).writeBoolean(true);
        ((StreamMessage) message).writeByte((byte) 1);
        ((StreamMessage) message).writeBytes("ABC".getBytes());
        ((StreamMessage) message).writeChar('c');
        ((StreamMessage) message).writeDouble((double) 2.2);
        ((StreamMessage) message).writeFloat((float) 3.3);
        ((StreamMessage) message).writeInt((int) 4);
        ((StreamMessage) message).writeLong((long) 5);
        ((StreamMessage) message).writeShort((short) 6);
        ((StreamMessage) message).writeString("<T>est<S>tring");
      }
      else if (mtype == 3)
      {
        message = session.createMapMessage();
        System.out.println("session.createMapMessage");
        ((MapMessage) message).setBoolean("MyBoolean", true);
        ((MapMessage) message).setByte("MyByte", (byte) 1);
        ((MapMessage) message).setBytes("MyBytes", "ABC".getBytes());
        ((MapMessage) message).setChar("MyChar", 'c');
        ((MapMessage) message).setDouble("MyDouble", (double) 2.2);
        ((MapMessage) message).setFloat("MyFloat", (float) 3.3);
        ((MapMessage) message).setInt("MyInt", (int) 4);
        ((MapMessage) message).setLong("MyLong", (long) 5);
        ((MapMessage) message).setShort("MyShort", (short) 6);
        ((MapMessage) message).setString("MyString", "<T>est<S>tring");
      }
      else if (mtype == 4)
      {
        message = session.createObjectMessage();
        System.out.println("session.createObjectMessage");

        ((ObjectMessage) message).setObject("MyObjectMessage");
      }
      else if (mtype == 5)
      {
        message = session.createMessage();
        System.out.println("session.createMessage");

        message.setJMSType("MyJMSType");
      }
      else
      {
        message = session.createTextMessage("MyTextMessage");
        System.out.println("session.createTextMessage");

        message.setJMSType("MyJMSType");
      }

      // Set some JMS properties
      message.setJMSCorrelationID("MyCorrelationID");
      message.setJMSDeliveryMode(DeliveryMode.NON_PERSISTENT);
      message.setJMSExpiration(27);
      message.setJMSPriority(9);

      if (replyqueue == 1)
      {
        Queue response = session.createQueue("MyReplyToQueue");
        message.setJMSReplyTo(response);
      }
      else
      {
        Topic response = session.createTopic("MyReplyToTopic");
        message.setJMSReplyTo(response);
      }

      // Set some JMSX properties
      message.setStringProperty("JMSXGroupID", "MyGroupId");
      message.setIntProperty("JMSXGroupSeq", 1);

      // Set some user properties
      message.setBooleanProperty("MyBoolean", true);
      message.setByteProperty("MyByte", (byte) 1);
      message.setDoubleProperty("MyDouble", (double) 2.2);
      message.setFloatProperty("MyFloat", (float) 3.3);
      message.setIntProperty("MyInt", (int) 4);
      message.setLongProperty("MyLong", (long) 5);
      message.setShortProperty("MyShort", (short) 6);
      message.setStringProperty("MyString", "TestString");

      for (int i = 0; i < messages; i++)
      {
          producer.send(message);
      }
      System.out.println("producer.send delmode(" + delmode + ") " + messages + " message(s)");

      connection.close();
      System.out.println("connection.close");
    }
    catch(Exception e)
    {
      e.printStackTrace();
    }
  }
}

