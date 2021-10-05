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
package com.ibm.ima.jca.servlet;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.security.AccessController;
import java.security.PrivilegedAction;
import java.util.Hashtable;
import java.util.Map;
import java.util.Properties;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.ConnectionMetaData;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.jms.Destination;
import javax.jms.Topic;
import javax.jms.Queue;
import javax.servlet.ServletException;
import javax.servlet.ServletOutputStream;
import javax.servlet.annotation.WebServlet;
import javax.servlet.http.HttpServlet;
import javax.servlet.http.HttpServletRequest;
import javax.servlet.http.HttpServletResponse;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.test.db2.DB2Tool;
import com.ibm.ima.jms.test.jca.TestProps;
import com.ibm.ima.jms.test.jca.TestProps.AppServer;
import com.ibm.ima.jms.test.jca.TestProps.DestinationType;
import com.ibm.ima.jms.test.jca.Utils;
import com.ibm.ima.ra.ImaResourceAdapterMetaData;

/**
 * Servlet implementation class TestServlet
 */
@WebServlet("/JCAServlet")
public class JCAServlet extends HttpServlet {
    private static final long serialVersionUID = 1L;
    
    private ConnectionFactory cf;
    private Destination sendDest;
    private Destination replyDest;
    
    String destName = null;   
    Map<Integer, TestProps> testCases;
    
    /**
     * @throws FileNotFoundException 
     * @see HttpServlet#HttpServlet()
     */
    public JCAServlet() throws FileNotFoundException {
        super();
       
        
        /*
         * If java security is enabled, the properties file needs to be opened
         * in a privileged action
         */
        FileInputStream f = AccessController.doPrivileged(new PrivilegedAction<FileInputStream>() {
            public FileInputStream run() {
                try {
                    return new FileInputStream(Utils.WASPATH() + "/tests.properties");
                } catch (Exception e) {
                    return null;
                }
            }
        });
        if(f == null)
            throw new FileNotFoundException(Utils.WASPATH() + "/tests.properties does not exist");
        testCases = TestProps.readCases(f);
    }
    
    
    private void runTest(TestProps test, ServletOutputStream o) throws IOException {
    	o.println("Running Test: " + test.testNumber);
    	o.println("Test properties: " + test.toString());
    	
    	String[] appliances = ServletUtils.getApplianceIPs();
    	
    	o.println("a1_ipv4_1: " + appliances[0]);
    	o.println("a2_ipv4_1: " + appliances[1]);
    	
    	if (test.transactionType.equals(TestProps.TransactionType.DB2CHECK)) {
        	/* Simple DB2 Connectivity Check (non XA) */
        	DB2Tool db2 = new DB2Tool(test);
        	o.println("DB2 CHECK " + (db2.queryDB() ? "OK" : "FAILED"));
        	o.println(test.logs());
        	return; 
      	}
 
    	
    	try {
    	    lookupJNDI(test);
    	    
            Connection connection = cf.createConnection();
            
            
            if (test.appServer != null && !test.appServer.equals(TestProps.AppServer.JBOSS)) {
	            ConnectionMetaData data = connection.getMetaData();
	            ImaResourceAdapterMetaData raMetaData = new ImaResourceAdapterMetaData();
	            o.println("Resource Adapter Name: " + raMetaData.getAdapterName());
	            o.println("Resource Adapter Version: " + raMetaData.getAdapterVersion());
	            o.println("Ima JMS Client Version: " + ImaJmsObject.getClientVerstion());
	            o.println("JMS Version: " + data.getJMSVersion());
	            o.println("JMS Provider Name: " + data.getJMSProviderName());
	            o.println("Provider Version: " + data.getProviderVersion());
            } else {
            	String jboss_is_great = "[JBOSS does not support access to RA from servlet]";
            	o.println("Resource Adapter Name: " + jboss_is_great);
	            o.println("Resource Adapter Version: " + jboss_is_great);
	            o.println("Ima JMS Client Version: " + jboss_is_great);
	            o.println("JMS Version: " + jboss_is_great);
	            o.println("JMS Provider Name: " + jboss_is_great);
	            o.println("Provider Version: " + jboss_is_great);
            }
            
            Session session = connection.createSession(false, test.ackMode.value());
            MessageProducer producer = session.createProducer(sendDest);
            producer.setDeliveryMode(test.deliveryMode.value());
            
            /*Set this property false to get around block on getObject method*/
            System.setProperty("IMAEnforceObjectMessageSecurity","false");
            
            ObjectMessage om = session.createObjectMessage(test);
            om.setJMSReplyTo(replyDest);
            
            if (test.selector != null) {
            	om.setStringProperty("TestProperty", test.selector);
            	o.println("[JCAServlet] set message selector to: " + test.selector);
            }
            
            for (int i=0; i<test.numMessages; i++) {
                producer.send(om);
                if (test.destinationType == DestinationType.TOPIC) {
                    o.println("[JCAServlet] Sent message to Destination: "+((Topic)sendDest).getTopicName());
                } else {
                    o.println("[JCAServlet] Sent message to Destination: "+((Queue)sendDest).getQueueName());
                }
            }           
            session.close();
            connection.close();         
        } catch (Exception e) {
            e.printStackTrace();        
        }
    }

    private void lookupJNDI(TestProps test) throws NamingException, JMSException {
        Hashtable<String, String> env = new Hashtable<String, String>();
        InitialContext context = new InitialContext(env);
        /* Servlet will always use its own connection factory. */
        String jndiPrefix = test.appServer == AppServer.JBOSS ? "java:/" : "";
        cf = (ConnectionFactory) context.lookup(jndiPrefix + "JMS_SERVLET_CF");
        sendDest = (Destination) context.lookup(jndiPrefix + test.sendDestination);
        replyDest = (Destination) context.lookup(jndiPrefix + test.replyDestination);
        if (test.destinationType == DestinationType.TOPIC) {
            destName = ((Topic) sendDest).getTopicName();
        } else {
            destName = ((Queue) sendDest).getQueueName();
        }
    }
    
    
    /**
     * @see HttpServlet#doGet(HttpServletRequest request, HttpServletResponse response)
     */
    @Override
    protected void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException, IOException {
    	ServletOutputStream o = response.getOutputStream();
    	o.println("[JCAServlet] Processing GET request ... ");   	    	        
        
    	int testNum = ServletUtils.parseURLParam(request.getQueryString());
    	if(testNum < 0)
    		throw new RuntimeException("Test number not provided, url params: " + request.getQueryString());
    	
    	TestProps test = testCases.get(testNum);
    	if(test == null)
    		throw new RuntimeException("No test found for number: " + testNum);
    	runTest(test, o);
    	o.println(test.logs()); // for searchLogs
    }

}
