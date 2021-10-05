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
javac MQJMSGet.java
java MQJMSGet

*/

import java.util.Enumeration;
import java.util.Arrays;
import javax.jms.*;
import com.ibm.mq.*;
import com.ibm.mq.jms.*;

public class MQJMSGet
{
  public static void main(String[] args)
  {
    try
    {
      MQConnectionFactory connectionFactory = new MQConnectionFactory();

      connectionFactory.setQueueManager("XP1");
      connectionFactory.setTransportType(JMSC.MQJMS_TP_CLIENT_MQ_TCPIP);
      connectionFactory.setChannel("SYSTEM.IMA.SVRCONN");
      connectionFactory.setHostName("127.0.0.1");

      Connection connection = connectionFactory.createConnection();
      System.out.println("connectionFactory.createConnection");

      Session session = connection.createSession(false, Session.AUTO_ACKNOWLEDGE);
      System.out.println("connection.createSession");

      Queue queue = session.createQueue("MQQUEUE");
      System.out.println("session.createQueue");

      MessageConsumer consumer = session.createConsumer(queue);
      System.out.println("session.createConsumer");

      connection.start();
      System.out.println("connection.start");

      Message message = consumer.receive(30000);

      while (message != null)
      {
        System.out.println("getJMSCorrelationID '" + message.getJMSCorrelationID() + "'");
        System.out.println("getJMSDeliveryMode '" + message.getJMSDeliveryMode() + "'");
        System.out.println("getJMSDestination '" + message.getJMSDestination() + "'");
        System.out.println("getJMSExpiration '" + message.getJMSExpiration() + "'");
        System.out.println("getJMSMessageID '" + message.getJMSMessageID() + "'");
        System.out.println("getJMSPriority '" + message.getJMSPriority() + "'");
        System.out.println("getJMSReplyTo '" + message.getJMSReplyTo() + "'");
        System.out.println("getJMSType '" + message.getJMSType() + "'");

        Enumeration e = message.getPropertyNames();

        while (e.hasMoreElements())
        {
          String name = (String) e.nextElement();
          Object prop = message.getObjectProperty(name);
          Class type = prop.getClass();
          System.out.println("name '" + name + "' value '" + prop.toString() + "' type '" + type.getName() + "'");
        }

        if (message instanceof TextMessage)
        {
          System.out.println("TextMessage '" + ((TextMessage) message).getText() + "'");
        }
        else if (message instanceof BytesMessage)
        {
          System.out.println("BytesMessage '" + ((BytesMessage) message).readUTF() + "'");
        }
        else if (message instanceof StreamMessage)
        {
          System.out.println("StreamMessage");

          try
          {
            while (true)
            {
              Object obj = ((StreamMessage) message).readObject();

              if (obj instanceof byte[])
              {
                System.out.println("  '" + Arrays.toString((byte []) obj) + "'");
              }
              else
              {
                System.out.println("  '" + obj.toString() + "'");
              }
            }
          }
          catch (MessageEOFException eof) {}
        }
        else if (message instanceof MapMessage)
        {
          System.out.println("MapMessage");

          Enumeration mapnames = ((MapMessage) message).getMapNames();

          while (mapnames.hasMoreElements())
          {
            String mapname = (String) mapnames.nextElement();

            Object obj = ((MapMessage) message).getObject(mapname);

            if (obj instanceof byte[])
            {
              System.out.println("  name '" + mapname + "' '" + Arrays.toString((byte []) obj) + "'");
            }
            else
            {
              System.out.println("  name '" + mapname + "' '" + obj.toString() + "'");
            }
          }
        }
        else if (message instanceof ObjectMessage)
        {
          System.out.println("ObjectMessage");
          Object obj = ((ObjectMessage) message).getObject();
          System.out.println("  '" + obj.toString() + "'");
        }
        else if (message instanceof Message)
        {
          System.out.println("Message");
        }

        message = consumer.receive(30000);
      }

      connection.close();
      System.out.println("connection.close");
    }
    catch(Exception e)
    {
      e.printStackTrace();
    }
  }
}

