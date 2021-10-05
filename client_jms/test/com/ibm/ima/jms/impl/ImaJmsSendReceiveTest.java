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

package com.ibm.ima.jms.impl;

import java.io.Serializable;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.MapMessage;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Queue;
import javax.jms.QueueConnection;
import javax.jms.QueueReceiver;
import javax.jms.QueueSender;
import javax.jms.StreamMessage;
import javax.jms.TextMessage;
import javax.jms.Topic;
import javax.jms.TopicPublisher;
import javax.jms.TopicSubscriber;

import junit.framework.TestCase;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.impl.ImaConnection;
import com.ibm.ima.jms.impl.ImaConstants;
import com.ibm.ima.jms.impl.ImaMessage;
import com.ibm.ima.jms.impl.ImaPropertiesImpl;
import com.ibm.ima.jms.impl.ImaSession;

public class ImaJmsSendReceiveTest extends TestCase implements ExceptionListener {
    static int msgsize = 120;
    static int logLevel = 6;
    static boolean showLog = false;
    static boolean verbose = false;
    
    /*
     * Class objects
     */
    static ConnectionFactory fact;
    static Connection conn;
    static Topic topic;
    static ImaSession session;
    static MessageConsumer cons = null;
    static MessageProducer output = null;
    static final String topic1 = "OneTopic";
    static byte [] bmsg = new byte[msgsize];
    static String rcvmsg;
	static String xmlmsg = new String("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n" +
			"<!-- \n"                                                     +
			" This file specifies the IP address of the IMA-CM server.\n" +
			" Several test cases use this file as the include file.   \n" +
			" Required Action:\n"                                         +
			" 1. Replace CMIP with the IP address of the IMA-CM server.\n"+
			"-->\n"														  +
			"<param type=\"structure_map\">ip_address=CMIP</param>");
	
    /*
     * Event handler
     * @see javax.jms.ExceptionListener#onException(javax.jms.JMSException)
     */
    public void onException(JMSException jmse) {
            if (showLog)
                System.out.println(""+jmse);
    }

    /*
     * Setup
     */
    public void testJMSSendReceiveSetup() throws Exception {
    	ImaProperties props;
        
        if (verbose)
            System.out.println("*TEST: testJMSSendReceiveSetup");
        try {
            /* 
             * Set up the JMS connection
             */
            fact = ImaJmsFactory.createTopicConnectionFactory();
            assertEquals("topic", ((ImaProperties)fact).get("objectTYPE"));
            props = (ImaProperties)fact;
            conn = new ImaConnection(true);
            ((ImaConnection)conn).props.putAll(((ImaPropertiesImpl)props).props);
            ((ImaConnection)conn).client = ImaTestClient.createTestClient(conn);
            assertEquals("topic", ((ImaProperties)conn).get("objectTYPE"));
            conn.setExceptionListener(this);
            session = (ImaSession) conn.createSession(false, 0);
            assertEquals(null, ((ImaProperties)conn).get("userid"));
            assertEquals(null, ((ImaProperties)session).get("userid"));
            
            /*
             * Create some input destinations
             */
            topic = ImaJmsFactory.createTopic(topic1);
            cons = session.createConsumer(topic);
            output = session.createPublisher(topic);
            
            if (verbose) {
                System.out.println(ImaJmsObject.toString(fact, "*"));
                System.out.println(ImaJmsObject.toString(conn, "*"));
                System.out.println(ImaJmsObject.toString(session, "*"));
                System.out.println(ImaJmsObject.toString(topic, "*"));
                System.out.println(ImaJmsObject.toString(cons, "*"));
                System.out.println(ImaJmsObject.toString(output, "*"));
            }    
            conn.start(); 
            if (System.getenv("IMAServer") == null)
                ((ImaMessageConsumer)cons).isStopped = false;
        }catch (JMSException je) {
        	je.printStackTrace(System.err);
        }
        assertEquals(false, session.isClosed.get());
    }
    
    /*
     * Test text Message
     */
    public void testTextMessageReceive() throws Exception {
	    final String [] textmsg = {
	        "Hello, World",
	    	"&lt&tdIMAJMS is a project for this release\t",
	    	xmlmsg,
	    	"@123%$3",
		    "   123  5&*\" ",
		    " ",
		    "",
		    "\u00a5\u01fc\u0391\u03a9\u8002\ud801\udd02",
 		    null,
	    };
	    int [] textmsgsize = { 12, 43, 293, 7, 13, 1, 0, 15, -1};

	    if (verbose)
	        System.out.println("*TEST: testTextMessageReceive ");
	    
        /*
         * Send messages
         */
	    int size = textmsg.length;
	    try {
	        TextMessage jmsg = null;
	    	for (int i = 0; i < size; i++) {    
	        	jmsg = session.createTextMessage(textmsg[i]);
	        	output.send((Message)jmsg);
	       
    		    Message rmsg = cons.receive(500);
	        	if (verbose) {
	                System.out.println(ImaJmsObject.toString(rmsg, "*b"));
	        	}
	        	assertEquals(textmsgsize[i], ImaJmsObject.bodySize(rmsg));
                assertEquals(textmsg[i], ((TextMessage)rmsg).getText());
	    	}   	
	        	
        	jmsg.clearBody();
        	output.send((Message)jmsg);
            Message rmsg = cons.receive(500);
            assertEquals(null, ((TextMessage)rmsg).getText());
        	
        	jmsg = session.createTextMessage();
            output.send((Message)jmsg);
            rmsg = cons.receive(500);
            assertEquals(null, ((TextMessage)rmsg).getText());

            jmsg = session.createTextMessage("fred");
            jmsg.setText(null);
            output.send((Message)jmsg);
            rmsg = cons.receive(500);
            assertEquals(null, ((TextMessage)rmsg).getText());
	    } catch (Exception ie) {
        	ie.printStackTrace(System.err);
        	assertTrue(false);
        }
    }
     
    /*
     * Test BytesMessage
     * A BytesMessage is an undifferentiated stream of bytes that can
     * be read in various formats. To get correct value, need to 
     * be read in the same order in which they were
     * written.
     */
    public void testBytesMessageReceive() throws Exception {
	    int i;
	    Message rmsg;
	    BytesMessage rbytemsg;
	    
	    if (verbose)
	        System.out.println("*TEST: testByteMessageReceive");

        for (i=0; i<bmsg.length; i++) {
            bmsg[i] = (byte)i;
        }
      
        try {   
        	/* Test boolean */
        	BytesMessage bytesmsg = session.createBytesMessage();
        	bytesmsg.writeBoolean(false);
        	bytesmsg.setFloatProperty(getName(), (float) 12.000);
        	output.send((Message)bytesmsg);

            rmsg = cons.receive(500);
    	    rbytemsg = (BytesMessage) rmsg;
    	    boolean br = rbytemsg.readBoolean();
    	    assertEquals(false, br);
    	    assertEquals((float) 12.000, rmsg.getFloatProperty(getName()));
    	    if (verbose)
    	        System.out.println(ImaJmsObject.toString(rmsg, "*b"));
	        bytesmsg.clearBody();
	        bytesmsg.clearProperties();
	        
	        /* Test double */
	        bytesmsg.writeDouble(123.456789e22);
	        bytesmsg.setBooleanProperty("IMATest", true);
	        output.send((Message)bytesmsg);
	        
 	        rmsg = cons.receive(500);
 	        rbytemsg = (BytesMessage) rmsg;
  	        double dr = rbytemsg.readDouble();
  	        assertEquals((double)123.456789e22, dr);
  	        assertEquals(true, rmsg.getBooleanProperty("IMATest"));
    	    if (verbose)
    	        System.out.println(ImaJmsObject.toString(rmsg, "*b"));
	        bytesmsg.clearBody();
	        bytesmsg.clearProperties();
	        
	        /* Test int, char and byte array*/
	        
	        byte[] rbs = new byte[256];
	        bytesmsg.writeInt(77889900);
	        bytesmsg.writeChar('x');
	        bytesmsg.writeBytes(bmsg);
	        bytesmsg.setJMSCorrelationID("ByteMessageTest_ID");
	        output.send((Message)bytesmsg);
	        
    	    rmsg = cons.receive(500);
    	    rbytemsg = (BytesMessage) rmsg;
    	    if (verbose)
    	        System.out.println(ImaJmsObject.toString(rmsg, "*b"));
    	    int ir = rbytemsg.readInt();
    	    char cr = rbytemsg.readChar();
    	    int rc = rbytemsg.readBytes(rbs);
    	
    	    assertEquals((int)77889900, ir);
    	    assertEquals((char)'x', cr);
    	    assertEquals(bmsg.length, rc);
            assertEquals(0, rmsg.getIntProperty("JMS_IBM_Retain"));
            assertEquals(true, rmsg.propertyExists("JMS_IBM_ACK_SQN"));
    	        
    	    for (i = 0; i< rc; i++) {
    	     	assertEquals(bmsg[i], rbs[i]);
    	    }
    	    String rbpd = rbytemsg.getJMSCorrelationID();
    	    assertEquals("ByteMessageTest_ID", rbpd);

	        bytesmsg.clearBody();
	        bytesmsg.clearProperties();
		       
	        
	        /* Test hex, float and bytes array property using obj property*/
	        bytesmsg.writeInt(0x7f800000);
	        long lg = System.currentTimeMillis();
	        bytesmsg.writeLong(lg);
	        bytesmsg.writeFloat((float) 0.00000000000000000000034);
	        bytesmsg.writeByte((byte) 9);
	        bytesmsg.setShortProperty("$Using_to_send4property", (short) 1);
	        output.send((Message)bytesmsg);
	        float fr;
	        byte  br0;
	        
	        rmsg = cons.receive(500);
	        rbytemsg = (BytesMessage) rmsg;
    	    if (verbose)
    	        System.out.println(ImaJmsObject.toString(rmsg, "*b"));
	    
	        //byte br0 = rbytemsg.readByte();
	        int   hr  = rbytemsg.readInt();
	        long  lr  = rbytemsg.readLong();
	        fr   = rbytemsg.readFloat();
	        br0  = rbytemsg.readByte();
	       
	     
	        @SuppressWarnings("unused")
            short sb = rbytemsg.getShortProperty("$Using_to_send4property");
	        assertEquals((float)0.00000000000000000000034, fr);
	        assertEquals((byte) 9, br0);
	        assertEquals(0x7f800000, hr);
	        assertEquals(lg, lr);
	        assertEquals((float)0.00000000000000000000034, fr);
	        
	        bytesmsg.clearBody();
	        bytesmsg.clearProperties();
	        
	        
	        /* 
	         * Test hex, float and bytes array property using obj property
	         */
	        bytesmsg.writeInt(0x7f800000);
	        bytesmsg.writeFloat((float) 0.00000000000000000000034);
	        bytesmsg.writeByte((byte) 9);
	        bytesmsg.setJMSType("Using_ID34345");
	        bytesmsg.setJMSCorrelationID("$Bytes@thisLocation");
	        output.send((Message)bytesmsg);
	        
	        rmsg = cons.receive(500);
	        rbytemsg = (BytesMessage) rmsg;
	        int ri = rbytemsg.readInt();
	        fr = rbytemsg.readFloat();
            byte br1 = rbytemsg.readByte();
	   
	        String rid = rbytemsg.getJMSType();
	        @SuppressWarnings("unused")
            String tid = rbytemsg.getJMSCorrelationID();
	        assertEquals(0x7f800000, ri);
	        assertEquals((float)0.00000000000000000000034, fr);
	        assertEquals((byte) 9, br1);
	        assertEquals("Using_ID34345", rid);
        	rbytemsg.clearBody();
        	rbytemsg.clearProperties();
	        
	        bytesmsg.clearBody();
	        output.send(bytesmsg);
            rmsg = cons.receive(500);
            assertEquals(0, ((BytesMessage)rmsg).getBodyLength());
            assertEquals(0, ImaJmsObject.bodySize(rmsg));

        	bytesmsg = session.createBytesMessage();
        	output.send(bytesmsg);
      	    rmsg = cons.receive(500);
     	    assertEquals(0, ((BytesMessage)rmsg).getBodyLength());
        } catch (Exception e) {
            e.printStackTrace();
         	assertTrue(false);
        } 	
    }

    /*
     * Test StreamMessage
     * A StreamMessage contains values of various types.
     * They must be read in the same order in which they were
     * written.
     */
    public void testStreamMessageReceive() throws Exception {
	    int i;
	    Message rmsg;
	    StreamMessage rstreammsg = null;
	    
	    if (verbose)
	        System.out.println("*TEST: testStreamMessageReceove");

        for (i=0; i<bmsg.length; i++) {
            bmsg[i] = (byte)(i+3);
        }
        try {   
        	StreamMessage streammsg = session.createStreamMessage();
        	streammsg.writeBoolean(true);
        	streammsg.writeByte((byte)3);
        	streammsg.writeBytes(bmsg);
        	streammsg.writeChar('t');
        	streammsg.writeDouble(0.12300000000000000000000000000004567);
        	streammsg.writeFloat((float)-9234551.0000000002345);
        	streammsg.writeInt(34);
        	streammsg.writeInt(0x18abef3);
        	streammsg.writeLong((long)567893134);
        	streammsg.writeString(xmlmsg);
        	streammsg.setBooleanProperty("SteamBoolean", false);
        	streammsg.setByteProperty("$amount_share", (byte)7);   
        	streammsg.setDoubleProperty("IBM_stock_price_in_$", 160.75);
        	streammsg.setIntProperty("StreamID", 30557);
        	streammsg.setJMSCorrelationID("StreamCorrelationID_4567");
        	streammsg.setJMSType("IMA_JMS_$_@_0987");
        	streammsg.setIntProperty("JMS_IBM_Retain", 1);
            assertEquals(1, streammsg.getIntProperty("JMS_IBM_Retain"));
        	
            output.send((Message)streammsg);
	        
	        rmsg = cons.receive(500);
	        
	        if (verbose)
                System.out.println(ImaJmsObject.toString(rmsg, "*b"));
	        
	        rstreammsg  = (StreamMessage) rmsg;
	        boolean b   = rstreammsg.readBoolean();
	        byte    br  = rstreammsg.readByte();
	        byte[]  brs = new byte[256];
	        int rc      = rstreammsg.readBytes(brs);
	        char    cr  = rstreammsg.readChar();
	        double  dr  = rstreammsg.readDouble();
	        float   fr  = rstreammsg.readFloat();
	        int     ir0 = rstreammsg.readInt();
	        int     ir1 = rstreammsg.readInt();
	        long    lr  = rstreammsg.readLong();
	        String  sr  = rstreammsg.readString();
	        boolean bpr = rstreammsg.getBooleanProperty("SteamBoolean");
	        double  dpr = rstreammsg.getDoubleProperty("IBM_stock_price_in_$");
	        byte    bypr= rstreammsg.getByteProperty("$amount_share");
	        int     ipr = rstreammsg.getIntProperty("StreamID");
	        String  spr0= rstreammsg.getJMSCorrelationID();
	        String  spr1= rstreammsg.getJMSType();
	        
	        assertEquals(true, b);
	        assertEquals((byte)3, br);
	        assertEquals(bmsg.length, rc);
	        assertEquals(1, rmsg.getIntProperty("JMS_IBM_Retain"));
	        
	        for (i = 0; i< rc; i++) {
	        	assertEquals(bmsg[i], brs[i]);
	        }
	        assertEquals('t', cr);
	        assertEquals((double) 0.12300000000000000000000000000004567, dr);
	        assertEquals((float)-9234551.0000000002345, fr);
	        assertEquals(34, ir0);
	        assertEquals(0x18abef3, ir1);
	        assertEquals((long)567893134, lr);
	        assertEquals(false, bpr);
	        assertEquals(160.75, dpr);
	        assertEquals((byte)7, bypr);
	        assertEquals(30557, ipr);
	        assertEquals(xmlmsg, sr);
	        assertEquals("StreamCorrelationID_4567", spr0);
	        assertEquals("IMA_JMS_$_@_0987", spr1);
            
        	rstreammsg.clearBody();
        	rstreammsg.clearProperties();
        	
        	output.send((Message)streammsg);
            rstreammsg = (StreamMessage)(cons.receive(500));
        } catch (JMSException je) {
        	je.printStackTrace(System.err);
         	assertTrue(false);
        }
    }
    
    /*
     * Test MapMessage
     * A MapMessage contains a series of name/value pairs.
     * The name is a string; the value can be of various types.
     * The receiving program can read any or all of the values,
     * in any order.
     */
    
    public void testMapMessageReceive() throws Exception {  
	    int i;
	    Message rmsg;
	    MapMessage rmapmsg = null;

	    if (verbose)
	        System.out.println("*TEST: testMapMessageReceive");

        for (i=0; i<bmsg.length; i++) {
            bmsg[i] = (byte)(i+3);
        }
        try {   
        	MapMessage mapmsg = session.createMapMessage();
        	mapmsg.setDouble("set_1st_double", 0.9999999999999999999900000000004567);
        	mapmsg.setBoolean("This is Map Message Test", true);
        	mapmsg.setByte("Set#bytes%123", (byte)3);
        	mapmsg.setBytes("$it is a bytes array", bmsg);
        	mapmsg.setChar("<symbol>IBM</symbol>", 't');
        	mapmsg.setDouble("set&lt*double", 0.12300000000000000000000000000004567);
        	mapmsg.setFloat("float=int/1000000", (float)-9234551.0000000002345);
        	mapmsg.setBoolean("This is 2nd Map Message Boolean Test", false);
        	mapmsg.setInt("Integer@mapmsg", 34);
        	mapmsg.setInt("Hex inside mapmsg", 0x18abef3);
        	mapmsg.setString("String name=[yourtest-0987%er]", xmlmsg);
        	mapmsg.setLong("123*456=", (long)567893134);
        	mapmsg.setBooleanProperty("MapBooleanValue", false);
        	mapmsg.setByteProperty("$amount_share", (byte)7);   
        	mapmsg.setDoubleProperty("IBM_stock_price_in_$", 160.75);
        	mapmsg.setIntProperty("MapStreamID", 30557);
        	mapmsg.setJMSCorrelationID("MapCorrelationID_4567");
        	mapmsg.setJMSType("<type=IMA_JMS_$_@_0987/>");
        	
            output.send((Message)mapmsg);
            
	        rmsg = cons.receive(500);
	        rmapmsg  = (MapMessage) rmsg;
	        
	        String  sr   = rmapmsg.getString("String name=[yourtest-0987%er]");
	        boolean b1   = rmapmsg.getBoolean("This is 2nd Map Message Boolean Test");
	        boolean b2   = rmapmsg.getBoolean("This is Map Message Test");
	        byte[]  brs  = rmapmsg.getBytes("$it is a bytes array");
	        byte    br   = rmapmsg.getByte("Set#bytes%123");
	        
	        float   fr   = rmapmsg.getFloat("float=int/1000000");
	        long    lr   = rmapmsg.getLong("123*456=");
	        char    cr   = rmapmsg.getChar("<symbol>IBM</symbol>");
	        
	        double  dr1  = rmapmsg.getDouble("set&lt*double");
	        double  dr2  = rmapmsg.getDouble("set_1st_double");
	        
	        int     ir0  = rmapmsg.getInt("Integer@mapmsg");
	        int     ir1  = rmapmsg.getInt("Hex inside mapmsg");
	        
	        byte    bypr= rmapmsg.getByteProperty("$amount_share");
	        int     ipr = rmapmsg.getIntProperty("MapStreamID");
	        String  spr0= rmapmsg.getJMSCorrelationID();
	        String  spr1= rmapmsg.getJMSType();
	        
	        boolean bpr = rmapmsg.getBooleanProperty("MapBooleanValue");
	        double  dpr = rmapmsg.getDoubleProperty("IBM_stock_price_in_$");
	        
	        if (verbose) {
                System.out.println(ImaJmsObject.toString(rmapmsg, "*b"));
	        }

	        assertEquals(xmlmsg, sr);
	        assertEquals(false, b1);
	        assertEquals(true, b2);
	        assertEquals((byte)3, br);
	        assertEquals(bmsg.length, brs.length);	        
	        for (i = 0; i< bmsg.length; i++) {
	        	assertEquals(bmsg[i], brs[i]);

	        }
	        assertEquals('t', cr);
	        assertEquals((double) 0.12300000000000000000000000000004567, dr1);
	        assertEquals((double) 0.9999999999999999999900000000004567, dr2);
	        assertEquals((float)-9234551.0000000002345, fr);
	        assertEquals(34, ir0);
	        assertEquals(0x18abef3, ir1);
	        assertEquals((long)567893134, lr);
	        
	        assertEquals(false, bpr);
	        assertEquals(160.75, dpr);
	        assertEquals((byte)7, bypr);
	        assertEquals(30557, ipr);
	        assertEquals("MapCorrelationID_4567", spr0);
	        assertEquals("<type=IMA_JMS_$_@_0987/>", spr1);
        
	        rmapmsg.clearBody();
	        rmapmsg.clearProperties();	        
        }catch (JMSException je) {
        	je.printStackTrace(System.err);
         	assertTrue(false);
        }
    }
    

    /*
     * Test ObjectMessage
     * An ObjectMessage can contain any Java object.  The test
     * reads the object casts it to the appropriate type.
     */
    public void testObjectMessageReceive() throws Exception {
	    int i;
	    Message rmsg;
	    ObjectMessage robjmsg = null;
	    
	    if (verbose)
            System.out.println("*TEST: testObjectMessageReceive");

        for (i=0; i<bmsg.length; i++) {
            bmsg[i] = (byte)(i+3);
        }
               
        try {   
        	ObjectMessage objmsg = session.createObjectMessage();
        	/*String*/
        	objmsg.setObject(xmlmsg);	
            output.send((Message)objmsg);
	  
            rmsg = cons.receive(500);
	        robjmsg  = (ObjectMessage) rmsg;
	        try {
		        String strobj = (String)robjmsg.getObject();
	        } catch (JMSException ex) {
	            assertTrue(ex instanceof ImaJmsSecurityException);
	        }
// Defect 136634 - Saving commented out code in case we eventually find a better way
// to manage the object deserialization security risk.  Currently, we're runnning JUnit
// with default setting for IMAEnforceObjectMessageSecurity so getObject() always throws
// an exception.  If we're able to restore the commented out code here, then the try/catch
// block preceding this comment can be deleted.
//	        assertEquals(xmlmsg, (String)robjmsg.getObject());
//	        
//	        robjmsg.clearBody();
//	        
//	        robjmsg.clearProperties();
//	        
//	        /*byte[] */
//	        objmsg.setObject(bmsg);
//	        output.send((Message)objmsg);
//		        
//	        rmsg = cons.receive(500);
//	        robjmsg  = (ObjectMessage) rmsg;
//	        byte[] rbs = (byte[])robjmsg.getObject();
//	        for (i = 0; i< bmsg.length; i++) {
//	        	assertEquals(bmsg[i], rbs[i]);
//	        }
//	        robjmsg.clearBody();
//	        robjmsg.clearProperties();
//	        
//	        /*Date*/
//	        Date today = new Date();
//	        objmsg.setObject(today);
//	        output.send((Message)objmsg);
//	        
//            rmsg = cons.receive(500);
//            robjmsg  = (ObjectMessage) rmsg;
//            assertEquals(today, (Date)robjmsg.getObject());
//	        
//	        
//	        robjmsg.clearBody();
//	        robjmsg.clearProperties();
//	        
//	        /*jmsTeamMember*/
//	        jmsTeamMember[] jteam = new jmsTeamMember[3];       
//	        jteam[0] = new jmsTeamMember("Ken", 55, "#000*01a-207-370-6580");
//	        jteam[1] = new jmsTeamMember("Caroline", 28, "#0A01B-512-098-6432");
//	        jteam[2] = new jmsTeamMember("Li", 44, "#0A01B-512-286-7249");
//	        
//	        objmsg.setObject(jteam);
//	        output.send((Message)objmsg);
//		     
//	        rmsg = cons.receive(500);
//	        robjmsg  = (ObjectMessage) rmsg;
//	        jmsTeamMember[] rjm = (jmsTeamMember[])robjmsg.getObject();
//	        assertEquals(jteam.length, rjm.length);
//	        for (i = 0; i<jteam.length; i++) {
//	        	assertEquals(jteam[i].getName(), rjm[i].getName());
//	        	assertEquals(jteam[i].getAge(), rjm[i].getAge());
//	        	assertEquals(jteam[i].getID(), rjm[i].getID());
//	        }
//	        
//	        /* Send object from cleared message */
//	        objmsg.clearBody();
//	        output.send((Message)objmsg);
//             rmsg = cons.receive(500);
//             robjmsg  = (ObjectMessage) rmsg;
//             Object objr = robjmsg.getObject();
//             assertEquals(null, objr);
//            
//            objmsg = session.createObjectMessage(jteam);
//            objmsg.setObject(null);
//            output.send((Message)objmsg);
//             rmsg = cons.receive(500);
//             robjmsg  = (ObjectMessage) rmsg;
//             objr = robjmsg.getObject();
//             assertEquals(null, objr);
//	        
//	        /* Send a null message */
//	        objmsg = session.createObjectMessage(null);
//            output.send((Message)objmsg);
//             rmsg = cons.receive(500);
//             robjmsg  = (ObjectMessage) rmsg;
//             objr = robjmsg.getObject();
//             assertEquals(null, objr);
//
//            /* Send a null message */
//            objmsg = session.createObjectMessage();
//            output.send((Message)objmsg);
//             rmsg = cons.receive(500);
//             robjmsg  = (ObjectMessage) rmsg;
//             objr = robjmsg.getObject();
//             assertEquals(null, objr);
            
        }catch (JMSException je) {
        	je.printStackTrace(System.err);
         	assertTrue(false);
        }
    }
    
    /*
     * Test Message
     */
    public void testMessageReceive() throws Exception {
	    Message rmsg;

	   if (verbose)
	       System.out.println("*TEST: testMessageReceive");
       try {   
        	Message msg = session.createMessage();
        	msg.setBooleanProperty("MapBooleanValue", false);
        	msg.setByteProperty("$amount_share", (byte)7);   
        	msg.setDoubleProperty("IBM_stock_price_in_$", 160.75);
        	msg.setIntProperty("MapStreamID", 30557);
        	msg.setJMSCorrelationID("MapCorrelationID_4567");
        	msg.setJMSType("<type=IMA_JMS_$_@_0987/>");
            output.send((Message)msg);
            
            rmsg = cons.receive(500);
            assertEquals(DeliveryMode.PERSISTENT, rmsg.getJMSDeliveryMode());
            assertEquals(Message.DEFAULT_PRIORITY, rmsg.getJMSPriority());
	        byte    bypr= rmsg.getByteProperty("$amount_share");
	        int     ipr = rmsg.getIntProperty("MapStreamID");
	        String  spr0= rmsg.getJMSCorrelationID();
	        String  spr1= rmsg.getJMSType();
	        
	        boolean bpr = rmsg.getBooleanProperty("MapBooleanValue");
	        double  dpr = rmsg.getDoubleProperty("IBM_stock_price_in_$");

	        assertEquals(false, bpr);
	        assertEquals(160.75, dpr);
	        assertEquals((byte)7, bypr);
	        assertEquals(30557, ipr);
	        assertEquals("MapCorrelationID_4567", spr0);
	        assertEquals("<type=IMA_JMS_$_@_0987/>", spr1);
	        
	        msg.clearBody();
	        msg.clearProperties();	        
       }catch (JMSException je) {
         	je.printStackTrace(System.err);
         	assertTrue(false);
       }	    
    }
    
    /*
     * Test Message - Multisession & Multitopic
     */
    public void testMessageReceiveMultiSessMultiTopic() throws Exception {
        Message rmsg, rmsg2;

       if (verbose)
           System.out.println("*TEST: testMessageReceive");
       try {   
            Message msg = session.createMessage();
            msg.setBooleanProperty("MapBooleanValue", false);
            msg.setByteProperty("$amount_share", (byte)7);   
            msg.setDoubleProperty("IBM_stock_price_in_$", 160.75);
            msg.setIntProperty("MapStreamID", 30557);
            msg.setJMSCorrelationID("MapCorrelationID_4567");
            msg.setJMSType("<type=IMA_JMS_$_@_0987/>");
            
            ImaSession session2 = (ImaSession)conn.createSession(false, 0);
            Topic topic2 = ImaJmsFactory.createTopic("buy-1.txtnjs");
            MessageProducer output2 = session2.createProducer(topic2);
            MessageConsumer cons2 = session2.createConsumer(topic2);
            if (System.getenv("IMAServer") == null)
                ((ImaMessageConsumer)cons2).isStopped = false;
            Message msg2 = session2.createMessage();
            msg2.setBooleanProperty("MapBooleanValue", true);
            msg2.setByteProperty("$amount_share", (byte)2);   
            msg2.setDoubleProperty("IBM_stock_price_in_$", 260.75);
            msg2.setIntProperty("MapStreamID", 20557);
            msg2.setJMSCorrelationID("MapCorrelationID_24567");
            msg2.setJMSType("<type=IMA2_JMS_$_@_0987/>");
            
            output.send((Message)msg);
            
            rmsg = cons.receive(500);
            assertEquals(DeliveryMode.PERSISTENT, rmsg.getJMSDeliveryMode());
            assertEquals(Message.DEFAULT_PRIORITY, rmsg.getJMSPriority());
            byte    bypr= rmsg.getByteProperty("$amount_share");
            int     ipr = rmsg.getIntProperty("MapStreamID");
            String  spr0= rmsg.getJMSCorrelationID();
            String  spr1= rmsg.getJMSType();
            
            boolean bpr = rmsg.getBooleanProperty("MapBooleanValue");
            double  dpr = rmsg.getDoubleProperty("IBM_stock_price_in_$");

            assertEquals(false, bpr);
            assertEquals(160.75, dpr);
            assertEquals((byte)7, bypr);
            assertEquals(30557, ipr);
            assertEquals("MapCorrelationID_4567", spr0);
            assertEquals("<type=IMA_JMS_$_@_0987/>", spr1);
            
            msg.clearBody();
            msg.clearProperties();      

            output2.send((Message)msg2);
            
            rmsg2 = cons2.receive(500);
            assertEquals(DeliveryMode.PERSISTENT, rmsg2.getJMSDeliveryMode());
            assertEquals(Message.DEFAULT_PRIORITY, rmsg2.getJMSPriority());
            bypr= rmsg2.getByteProperty("$amount_share");
            ipr = rmsg2.getIntProperty("MapStreamID");
            spr0= rmsg2.getJMSCorrelationID();
            spr1= rmsg2.getJMSType();
            
            bpr = rmsg2.getBooleanProperty("MapBooleanValue");
            dpr = rmsg2.getDoubleProperty("IBM_stock_price_in_$");

            assertEquals(true, bpr);
            assertEquals(260.75, dpr);
            assertEquals((byte)2, bypr);
            assertEquals(20557, ipr);
            assertEquals("MapCorrelationID_24567", spr0);
            assertEquals("<type=IMA2_JMS_$_@_0987/>", spr1);
            
            msg.clearBody();
            msg.clearProperties(); 
       }catch (JMSException je) {
            je.printStackTrace(System.err);
            assertTrue(false);
       }        
    }

    
    /*
     * Test Message all send params
     */
    public void testMessageSendParams() throws Exception {
        Message rmsg;

       if (verbose)
           System.out.println("*TEST: testMessageSendParams");
       try {  
            Message msg = session.createMessage();
            output.send((Message)msg, DeliveryMode.NON_PERSISTENT, Message.DEFAULT_PRIORITY - 1, 0L);
            
            rmsg = cons.receive(500);
            assertNotNull(rmsg);
            if (verbose)
                System.out.println(((ImaMessage)rmsg).toString(ImaConstants.TS_All|ImaConstants.TS_Body));
            assertEquals(DeliveryMode.NON_PERSISTENT, rmsg.getJMSDeliveryMode());
            assertEquals(Message.DEFAULT_PRIORITY - 1, rmsg.getJMSPriority());
            
            msg.clearBody();
            msg.clearProperties();          
       }catch (JMSException je) {
            je.printStackTrace(System.err);
            assertTrue(false);
       } 
    }
    
    /*
     * Test Durable1
     */
    public void testDurable1() throws Exception {
        Message rmsg;
        Topic dtopic1 = session.createTopic("dtopic1");
        Topic dtopic2 = session.createTopic("dtopic2");
        TopicPublisher tpub = session.createPublisher(dtopic1);
        TopicPublisher tpub2 = session.createPublisher(dtopic2);
        TopicSubscriber tsub = session.createDurableSubscriber(dtopic1, "durableTopicsubscriber");
        if (System.getenv("IMAServer") == null)
            ((ImaMessageConsumer)tsub).isStopped = false;

       if (verbose)
           System.out.println("*TEST: testDurable1");
       try {  
            Message msg = session.createMessage();
            tpub.send((Message)msg);
            
            rmsg = tsub.receive(500);
            assertNotNull(rmsg);
            
            msg.clearBody();
            msg.clearProperties();
            
            tsub.close();
            tsub = session.createDurableSubscriber(dtopic2, "durableTopicsubscriber");
            if (System.getenv("IMAServer") == null)
                ((ImaMessageConsumer)tsub).isStopped = false;
            msg = session.createMessage();
            tpub2.send((Message)msg);
            
            rmsg = tsub.receive(500);
            assertNotNull(rmsg);
            
            msg.clearBody();
            msg.clearProperties();
       }catch (JMSException je) {
            je.printStackTrace(System.err);
            assertTrue(false);
       } 
    }
    
    /*
     * Test queue with message selector
     */
    public void testQSelector() throws Exception {
        Message rmsg;
        
        QueueConnection qConn = (QueueConnection)new ImaConnection(true);
        ImaProperties props = (ImaProperties)qConn;
        ((ImaConnection)props).putInternal("ClientID", "q_client");
        ((ImaConnection)qConn).clientid = "q_client";
        ((ImaConnection)qConn).client = ImaTestClient.createTestClient(qConn);       
        ImaSession qSess = (ImaSession)qConn.createSession(false, 0);
        Queue q = ImaJmsFactory.createQueue("MY_QUEUE");
        QueueReceiver qCons = qSess.createReceiver(q,"q = true");
        if (System.getenv("IMAServer") == null)
            ((ImaMessageConsumer)qCons).isStopped = false;
        QueueSender qProd = qSess.createSender(q);
        qConn.start();
        
       if (verbose)
           System.out.println("*TEST: testQSelector");
       try {  
            Message msg = qSess.createMessage();
            msg.setBooleanProperty("q", true);
            
            qProd.send((Message)msg, DeliveryMode.NON_PERSISTENT, Message.DEFAULT_PRIORITY - 1, 0L);
            
            rmsg = qCons.receive(500);
            assertNotNull(rmsg);
            assertEquals(DeliveryMode.NON_PERSISTENT, rmsg.getJMSDeliveryMode());
            assertEquals(Message.DEFAULT_PRIORITY - 1, rmsg.getJMSPriority());
            
            msg.clearBody();
            msg.clearProperties();          
       }catch (JMSException je) {
            je.printStackTrace(System.err);
            assertTrue(false);
       } 
    }
    
    public void testJMSSendReceiveSessionClose() {
        if (verbose)
            System.out.println("*TEST: testJMSSentReceiveSessionClose");
    	
    	try {
	        conn.close();
    	} catch (JMSException je) {
    		je.printStackTrace(System.err);
    	}
    }
    
    /*
     * Fix getName for use when running as an application.
     * @see junit.framework.TestCase#getName()
     */
    public String getName() {
        String ret = super.getName();
        if (ret == null)
            ret = "IMAJmsSendReceiveTest";
        return ret;
    }
    
    /*
     * Main test 
     */
    public static void main(String args[]) {      
        try {
            verbose = true;
        	new ImaJmsSendReceiveTest().testJMSSendReceiveSetup();
            new ImaJmsSendReceiveTest().testTextMessageReceive();
            new ImaJmsSendReceiveTest().testBytesMessageReceive();
            new ImaJmsSendReceiveTest().testObjectMessageReceive();
            new ImaJmsSendReceiveTest().testStreamMessageReceive();
            new ImaJmsSendReceiveTest().testMapMessageReceive();
            new ImaJmsSendReceiveTest().testMessageReceive();
            new ImaJmsSendReceiveTest().testMessageReceiveMultiSessMultiTopic();
            new ImaJmsSendReceiveTest().testMessageSendParams();
            new ImaJmsSendReceiveTest().testDurable1();
            new ImaJmsSendReceiveTest().testQSelector();
            new ImaJmsSendReceiveTest().testJMSSendReceiveSessionClose();
        } catch (Exception e){
            e.printStackTrace(System.err);
        }                 
    } 
}


final class jmsTeamMember implements Serializable {
	private static final long serialVersionUID = 1L;
	private String name;
	private int age;
	private String IBM_ID;
	public jmsTeamMember(String name, int age, String id) {
		this.name = name;
		this.age = age;
		this.IBM_ID = id;
	}
	public int getAge() {
		return age;
	}
	public void setAge(int age) {
		this.age = age;
	}
	public String getName() {
		return name;
	}
	public void setName(String name) {
		this.name = name;
	}
	public String getID() {
		return IBM_ID;
	}
	public void setID(String id) {
		this.IBM_ID = id;
	}
}
