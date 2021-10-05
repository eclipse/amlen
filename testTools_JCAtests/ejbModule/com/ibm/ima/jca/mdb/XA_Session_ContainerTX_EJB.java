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
package com.ibm.ima.jca.mdb;

import java.util.Hashtable;
import java.util.Properties;

import javax.ejb.Local;
import javax.ejb.LocalBean;
import javax.ejb.Remote;
import javax.ejb.SessionContext;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.naming.InitialContext;
import javax.naming.NamingException;

import com.ibm.ima.jms.test.db2.DB2Tool;
import com.ibm.ima.jms.test.jca.TestProps;

/**
 * FVT Test Container-Managed EJB for the IBM MessageSight Resource Adapter.
 * 
 * This EJB is used to test container-managed transactions with the 6 different
 * transaction attributes that a container-managed transaction can have.
 * 
 * The MDB calling this EJB will either be container-managed with attribute 
 * NOT_SUPPORTED or container-managed with attribute REQUIRED.
 * 
 * If the MDB has attribute NOT_SUPPORTED and this EJB has attribute:
 *   REQUIRED:      This EJB creates a new container-managed transaction that does not
 *                  involve the MDB.
 *   REQUIRES_NEW:  This EJB creates a new container-managed transaction that does not
 *                  involve the MDB.
 *   MANDATORY:     ERROR - the MDB must have attribute REQUIRED in this case.
 *   NEVER:         This EJB does not perform within a transaction.
 *   SUPPORTS:      This EJB does not perform within a transaction.
 *   NOT_SUPPORTED: This EJB does not perform within a transaction.
 *   
 * If the MDB has attribute REQUIRED and this EJB has attribute:
 *   REQUIRED:      This EJB participates in the same transaction as the MDB.
 *   REQUIRES_NEW:  This EJB creates a new container-managed transaction that is
 *                  separate from the transaction of the MDB.
 *   MANDATORY:     This EJB participates in the same transaction as the MDB.
 *   NEVER:         ERROR - the MDB must have attribute NOT_SUPPORTED in this case.
 *   SUPPORTS:      This EJB participates in the same transaction as the MDB.
 *   NOT_SUPPORTED: The MDB's transaction is suspended and this EJB does not perform
 *                  within a transaction.
 *                  
 * Depending on the TestProps object, the EJB will involve DB2 or ima.evilra in the
 * transaction before sending the reply message.
 *
 */
@Local(XA_Session_ContainerTX_EJBLocal.class)
@Remote(XA_Session_ContainerTX_EJBRemote.class)
@LocalBean
public class XA_Session_ContainerTX_EJB implements XA_Session_ContainerTX_EJBRemote {

	private SessionContext sessCtx;
	private TestProps test;
	private DB2Tool db2;
	private EvilTool et;

	/**
	 * Send a reply message to the JMSReplyTo destination specified in the TestProps.
	 * Uses the connection factory specified in TestProps.
	 * 
	 * @throws RuntimeException wrapped around one of the following:
	 * 
	 * @throws NamingException context.lookup()
	 * @throws JMSException all JMS calls
	 */
	public void sendMessage() {
		ConnectionFactory cf = null;

		test.log("[XA_Session_ContainerTX_EJB] JMSReplyTo == " + test.replyDestination);

		Connection c = null;
		try {
			InitialContext context = new InitialContext(new Hashtable<String,String>());
			cf = (ConnectionFactory) context.lookup(test.connectionFactory);
			Destination dest = (Destination) context.lookup(test.replyDestination);
			c = cf.createConnection();
			Session s = c.createSession(false,Session.AUTO_ACKNOWLEDGE);
			MessageProducer p = s.createProducer(dest);

			/*Set this property false to get around block on getObject method*/
			System.setProperty("IMAEnforceObjectMessageSecurity","false");
			
			ObjectMessage message = s.createObjectMessage(test);
			message.setJMSReplyTo(dest);

			p.send(message);
			test.log("[XA_Session_ContainerTX_EJB] Forwarded message to:  " + message.getJMSReplyTo().toString());
			c.close();    
		} catch (NamingException ne) {
			test.log(ne.getMessage());
			ne.printStackTrace();
			throw new RuntimeException("EJB sendMessage() caught NamingException: " + ne.getMessage(), ne);
		} catch (JMSException je) {
			test.log(je.getMessage());
			je.printStackTrace();
			// Make sure connection gets closed
			try {
				c.close();
			} catch (Exception e) {
				// do nothing
			}
			throw new RuntimeException("EJB sendMessage() caught JMSException: " + je.getMessage(), je);
		}
	}

	/**
	 * Involve all resources specified within TestProps. This takes place within the
	 * container managed transaction if there is one.
	 * 
	 */
	private void doTX() {
		db2 = new DB2Tool(test);
		et = new EvilTool(test);
		if (test.transactionComponent2.equals(TestProps.TransactionComponent.DB2))
			queryDB();
		if (test.transactionComponent2.equals(TestProps.TransactionComponent.EVIL))
			doEvil();
		sendMessage();
		if (test.rollbackType == TestProps.RollbackType.ROLLBACK
				|| test.rollbackType == TestProps.RollbackType.ROLLBACKEJBONLY) {
			sessCtx.setRollbackOnly();
			test.log("[XA_Session_ContainerTX_EJB] Calling EJBContext.setRollbackOnly()");
		}
	}

	/**
	 * Entry method for EJB. If this method is called, then the EJB is using
	 * TransactionAttribute Required.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @return The TestProps object
	 */
	public TestProps sendMessage_REQUIRED(TestProps test) {
		setTest(test);

		test.log("[XA_Session_ContainerTX_EJB] REQUIRED");

		doTX();

		return test;
	}

	/**
	 * Entry method for EJB. If this method is called, then the EJB is using
	 * TransactionAttribute RequiresNew.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @return The TestProps object
	 */
	public TestProps sendMessage_REQUIRES_NEW(TestProps test) {
		setTest(test);

		test.log("[XA_Session_ContainerTX_EJB] REQUIRES_NEW");

		doTX();

		return test;
	}

	/**
	 * Entry method for EJB. If this method is called, then the EJB is using
	 * TransactionAttribute Mandatory.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @return The TestProps object
	 */
	public TestProps sendMessage_MANDATORY(TestProps test) {
		setTest(test);

		test.log("[XA_Session_ContainerTX_EJB] MANDATORY");

		doTX();

		return test;
	}

	/**
	 * Entry method for EJB. If this method is called, then the EJB is using
	 * TransactionAttribute Never.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @return The TestProps object
	 */
	public TestProps sendMessage_NEVER(TestProps test) {
		setTest(test);

		test.log("XA_Session_ContainerTX_EJB: NEVER");

		doTX();

		return test;
	}

	/**
	 * Entry method for EJB. If this method is called, then the EJB is using
	 * TransactionAttribute NotSupported.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @return The TestProps object
	 */
	public TestProps sendMessage_NOT_SUPPORTED(TestProps test) {
		setTest(test);

		test.log("[XA_Session_ContainerTX_EJB] NOT_SUPPORTED");   

		doTX();

		return test;

	}

	/**
	 * Entry method for EJB. If this method is called, then the EJB is using
	 * TransactionAttribute Supports.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @return The TestProps object
	 */
	public TestProps sendMessage_SUPPORTS(TestProps test) {
		setTest(test);

		test.log("[XA_Session_ContainerTX_EJB] SUPPORTS");

		doTX();

		return test;
	}

	public void queryDB() {
		test.log("[XA_Session_ContainerTX_EJB] queryDB()");
		if (!db2.queryDB())
			test.log("queryDB FAILED"); // this message is used by searchLogs
		else
			test.log("queryDB OK"); // this message is used by searchLogs
	}

	public void doEvil() {
		test.log("[XA_Session_ContainerTX_EJB] doEvil()");
		/* more things */
		if (!et.execute())
			test.log("doEvil FAILED"); // this message is used by searchLogs
		else
			test.log("doEvil OK"); // this message is used by searchLogs
	}

	public SessionContext getSessionContext() {
		return sessCtx;
	}

	public TestProps getTest() {
		return test;
	}

	public void setTest(TestProps test) {
		this.test = test;
	}
}
