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

import javax.ejb.EJBContext;
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
import javax.transaction.UserTransaction;

import com.ibm.ima.jms.test.db2.DB2Tool;
import com.ibm.ima.jms.test.jca.TestProps;
import com.ibm.ima.jms.test.jca.TestProps.RollbackType;
import com.ibm.ima.jms.test.jca.TestProps.TransactionType;

/**
 * FVT Test Bean-Managed Transaction EJB for the IBM MessageSight Resource Adapter.
 * 
 * This EJB is used for testing UserTransactions with the RA. If we create a
 * UserTransaction in this bean, then the entire transaction scope takes place
 * here. The message received by the MDB onMessage is not included in the
 * transaction. The log message sent by the MDB is also not a part in the scope
 * of this transaction.
 * 
 * Depending on the TestProps object, the EJB will involve DB2 or ima.evilra in the
 * transaction before sending the reply message.
 *
 */
@Local(XA_Session_BeanTX_EJBLocal.class)
@Remote(XA_Session_BeanTX_EJBRemote.class)
@LocalBean
public class XA_Session_BeanTX_EJB implements XA_Session_BeanTX_EJBRemote {

	private SessionContext sessCtx;
	private EJBContext myEJBCtx;
	private TestProps test;
	private DB2Tool db2;
	private EvilTool et;

	/**
	 * Entry method for Bean Managed EJB. UserTransaction begins and ends within 
	 * this function. Calls to perform DB2 or EvilRA related work happen within 
	 * the bounds of the UserTransaction if one exists.
	 * 
	 * @param test The TestProps object to get test properties from
	 * @throws RuntimeException - wrapped around one of the following:
	 * 
	 * @throws NotSupportedException - begin()
	 * @throws SystemException - begin(), rollback(), commit()
	 * @throws SecurityException - rollback(), commit()
	 * @throws IllegalStateException - rollback(), commit()
	 * @throws RollbackException - commit()
	 * @throws HeuristicMixedException - commit()
	 * @throws HeuristicRollbackException - commit()
	 */
	public void processTest(TestProps test) {
		setTest(test);
		db2 = new DB2Tool(test);
		et = new EvilTool(test);

		if (myEJBCtx != null) {

			UserTransaction ut = myEJBCtx.getUserTransaction();

			// conditionally begin transaction
			if (test.transactionType == TransactionType.XA_BEAN_MANAGED_USER_TRANS ||
					test.transactionType == TransactionType.LOCAL_TRANS_EJB) {
				try {
					ut.begin();
					test.log("[XA_Session_BeanTX_EJB] USER_TRANSACTION has begun");
				} catch (Exception e) {
					test.log(e.getMessage());
					e.printStackTrace();
					throw new RuntimeException("EJB UserTransaction.begin() caught exception: " + e.getMessage(), e);
				}
			} else {
				test.log("[XA_Session_BeanTX_EJB] Not running in a transaction");
			}

			// Use DB2 as the second component of our global transaction
			if (test.transactionComponent2.equals(TestProps.TransactionComponent.DB2)) {
				queryDB();
			}

			// Use ima.evilra as the second component of our global transaction
			if (test.transactionComponent2.equals(TestProps.TransactionComponent.EVIL)) {
				doEvil();
			}

			// Forward the message to the reply destination
			sendMessage();

			if (test.transactionType == TransactionType.XA_BEAN_MANAGED_USER_TRANS ||
					test.transactionType == TransactionType.LOCAL_TRANS_EJB) {
				// conditionally mark for rollback
				if (test.rollbackType == RollbackType.ROLLBACK) {
					try {
						ut.rollback();
						test.log("[XA_Session_BeanTX_EJB] USER_TRANSACTION has rolled back");
					} catch (Exception e) {
						test.log(e.getMessage());
						e.printStackTrace();
						throw new RuntimeException("EJB UserTransaction.rollback() caught exception: " + e.getMessage(), e);
					}
				} 

				// conditionally commit transaction
				if (test.commitType != null) {
					try {
						ut.commit();
						test.log("[XA_Session_BeanTX_EJB] USER_TRANSACTION has committed");
					} catch (Exception e) {
						test.log(e.getMessage());
						e.printStackTrace();
						throw new RuntimeException("EJB UserTransaction.commit() caught exception: " + e.getMessage(), e);
					}
				}
			}
		}

		return;
	}

	/**
	 * Send a reply message to the JMSReplyTo destination specified in the TestProps.
	 * Uses the connection factory specified in TestProps.
	 * 
	 * @throws RuntimeException wrapped around one of the following:
	 * 
	 * @throws NamingException context.lookup()
	 * @throws JMSException all JMS calls
	 */
	private void sendMessage() {
		ConnectionFactory cf = null;

		test.log("[XA_Session_BeanTX_EJB] JMSReplyTo == " + test.replyDestination);

		Connection c = null;
		try {
			InitialContext context = new InitialContext(new Hashtable<String,String>());
			cf = (ConnectionFactory) context.lookup(test.connectionFactory);
			Destination dest = (Destination) context.lookup(test.replyDestination);
			c = cf.createConnection();
			Session s = c.createSession(false,Session.AUTO_ACKNOWLEDGE);

			/*Set this property=false to get around block on getObject method*/
			System.setProperty("IMAEnforceObjectMessageSecurity","false");
			
			ObjectMessage message = s.createObjectMessage(test);
			message.setJMSReplyTo(dest);

			MessageProducer p = s.createProducer(dest);
			p.send(message);
			test.log("[XA_Session_BeanTX_EJB] Forwarded message to:  " + message.getJMSReplyTo().toString());
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

	public void queryDB() {
		test.log("[XA_Session_BeanTX_EJB] queryDB()");
		if (!db2.queryDB())
			test.log("queryDB FAILED");
		else
			test.log("queryDB OK");
	}

	public void doEvil() {
		test.log("[XA_Session_BeanTX_EJB] doEvil()");
		if (!et.execute())
			test.log("doEvil FAILED");
		else
			test.log("doEvil OK");
	}

	public SessionContext getSessionContext() {
		return sessCtx;
	}

	public void setEJBContext(EJBContext ctx) {
		myEJBCtx = ctx;
	}

	public TestProps getTest() {
		return test;
	}

	public void setTest(TestProps test) {
		this.test = test;
	}

}
