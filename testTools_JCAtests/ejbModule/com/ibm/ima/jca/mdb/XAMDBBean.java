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

import java.io.FileNotFoundException;
import java.util.Hashtable;
import java.util.Properties;

import javax.transaction.UserTransaction;
import javax.annotation.Resource;
import javax.ejb.CreateException;
import javax.ejb.EJBException;
import javax.ejb.MessageDriven;
import javax.ejb.MessageDrivenBean;
import javax.ejb.MessageDrivenContext;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.BytesMessage;
import javax.jms.TextMessage;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.ObjectMessage;
import javax.jms.Session;
import javax.naming.InitialContext;


import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.test.jca.AUtils;
import com.ibm.ima.jms.test.jca.TestProps;
import com.ibm.ima.jms.test.jca.TestProps.AppServer;
import com.ibm.ima.jms.test.jca.TestProps.FailureType;
import com.ibm.ima.jms.test.jca.TestProps.RollbackType;
import com.ibm.ima.jms.test.jca.TestProps.TransactionType;

/**
 * FVT Test MDB for the IBM MessageSight Resource Adapter.
 * 
 * This MDB expects to receive only javax.jms.ObjectMessage type messages.
 * These messages must contain a TestProps (see com.ibm.ima.jms.test.jca.TestProps
 * in testTools_JMSTestDriver) object containing the information to run a
 * particular test case.
 * 
 * If a message is arriving for the first time, it will either be passed along
 * to an EJB or processed directly from within the MDB.
 * 
 * If the message is passed to an EJB, it will be participating in some form
 * of transaction, which may be a local or global transaction.
 * If it is a global transaction, the EJB will involve either DB2 or ima.evilra.
 * If it is a local transaction, the reply will be sent from the EJB and no
 * other components will be involved.
 * 
 * If the message stays within the MDB, it may still participate in a local
 * transaction or no transaction.
 * 
 * In both the EJB and MDB case, the received message will be forwarded to a
 * reply destination specified on the message as well as in the TestProps object.
 * 
 * After either returning from the EJB, or forwarding the message from the MDB,
 * a javax.jms.TextMessage is sent containing the logs from the MDB/EJB to
 * /log/Topic.
 * 
 * If the message shows up with JMSRedelivered == TRUE, then the message is
 * simply forwarded from the MDB to the reply destination, and the log message
 * is sent.
 */
@MessageDriven
public class XAMDBBean implements MessageDrivenBean, MessageListener {

	private static final long serialVersionUID = 7566613751004766422L;

	@Resource
	protected MessageDrivenContext myEJBCtx;
	private ConnectionFactory cf;
	private TestProps test;

	/**
	 * onMessage receives a Message and makes sure it is an ObjectMessage containing a TestProps object.
	 * It then either calls invokeEJB() or processMessage() depending on TestProps.transactionType.
	 * Once complete, it sends the StringBuffer from the TestProps object containing log data
	 * to /log/Topic.
	 * 
	 * @param message The message received by the MDB.
	 */
	public void onMessage(Message message) {
		test = null;

		try {
			setTest(getTestPropsFromMessage(message));

			test.log("[XAMDBBean] TestProps #" + test.testNumber + " was created from MDB onMessage");

			/* Call invokeEJB if TransactionType is CMT*, BMT* or LOCAL_TRANS_EJB 
			 * and this message is arriving for the first time. */
			if ((test.transactionType != TransactionType.NONE && 
					test.transactionType != TransactionType.LOCAL_TRANS_MDB) &&
					message.getJMSRedelivered() == false) {
				test.log("[XAMDBBean] JMSRedelivered is false and TransactionType is not NONE. Calling invokeEJB()");
				invokeEJB();

				/* Otherwise, send our reply message from the MDB */
			} else if (message.getJMSReplyTo() != null) {
				test.log("[XAMDBBean] JMSRedelivered is true or not in a global transaction. Calling processMessage()");
				processMessage(message);
			}

			test.log("[XAMDBBean] End of XAMDBBean.onMessage()");

		} catch (JMSException je) {
			test.log(je.getMessage());
			je.printStackTrace();
			throw new RuntimeException("MDB onMessage() caught JMSException: " + je.getMessage(), je);
		} finally {
			if (test != null) {
				sendGatheredLogs();
			}
		}
	}

	/**
	 * processMessage:
	 * Take actions based on the contents of test.
	 * At a minimum, forward message to the JMSReplyTo destination.
	 * 
	 * @param message The message to be forwarded by the MDB.
	 * @throws RuntimeException processMessage will throw a RuntimeException wrapped around
	 *         a javax.jms.JMSException if getJMSRedelivered() fails.
	 * 
	 */
	private void processMessage(Message message) {
		try {
			/* ImaActivationSpec.enableRollback property provides LocalTransaction-like functionality
			 * for MDB's. This property requires the MDB to not be in a transaction.
			 * To test this, TransactionType == NONE and RollbackType == ROLLBACK.
			 * If enableRollback == True, the message will be redelivered. */
			if (test.transactionType == TransactionType.NONE &&
					test.rollbackType == RollbackType.ROLLBACK &&
					message.getJMSRedelivered() == false) {
				throw new RuntimeException("[XAMDBBean] enableRollback exception");
			}

			/* Crash the server if test.FailureType == IMA_CRASH and this is our first time
			 * seeing this message, and we are not in a transaction. */
			test.log("message.getJMSRedelivered: " + message.getJMSRedelivered());
			test.log("test.transactionType: " + test.transactionType);
			test.log("test.failureType: " + test.failureType);
			if (!message.getJMSRedelivered() &&
					test.transactionType == TransactionType.NONE &&
					test.failureType == FailureType.IMA_CRASH) {
				crashServer();
			}

			/* If the server was crashed above, this will fail the first time through
			 * as the server has been crashed. */
			sendReply(message);
		} catch (JMSException je) {
			test.log(je.getMessage());
			je.printStackTrace();
			throw new RuntimeException("MDB processMessage() caught JMSException: " + je.getMessage(), je);
		}
	}

	/**
	 * Extracts the TestProps object from the message.
	 * 
	 * @param message The javax.jms.ObjectMessage to retrieve the TestProps object from.
	 * @return The TestProps object.
	 * @throws RuntimeException getTestPropsFromMessage will throw a RuntimeException 
	 *         wrapped around a javax.jms.JMSException if om.getObject() fails.
	 */
	private TestProps getTestPropsFromMessage(Message message) {
		TestProps test;
		/* Make sure the message is a javax.jms.ObjectMessage */
		if (!(message instanceof ObjectMessage)) {
			if (message instanceof BytesMessage) {
				System.out.println("MDB Received BytesMessage");
				sendBadMessageReply("BytesMessage");
			} else if (message instanceof TextMessage) {
				System.out.println("MDB Received TextMessage");
				sendBadMessageReply("TextMessage");
			}
			throw new RuntimeException("[XAMDBBean] Received message is not of type ObjectMessage");
		}
		
		/* Setting IMAEnforceObjectMessageSecurity=false since getObject() method is blocked by default now 6-20-16 */
		System.setProperty("IMAEnforceObjectMessageSecurity","false");

		try {
			ObjectMessage om = (ObjectMessage) message;
			Object o = om.getObject();

			/* Make sure the ObjectMessage contains a TestProps object */
			if (!(o instanceof TestProps))
				throw new RuntimeException("[XAMDBBean] Object from message is not of type TestProps");

			test = (TestProps) o;
			return test;
		} catch (JMSException je) {
			System.out.println(je.getMessage());
			je.printStackTrace();
			throw new RuntimeException("MDB getTestPropsFromMessage() caught JMSException: " + je.getMessage(), je);
		}
	}

	/**
	 * Forward message to its JMSReplyTo destination using the connection factory set in test.
	 * 
	 * @param message The message to forward from the MDB.
	 * @throws RuntimeException sendReply will throw a RuntimeException wrapped around a 
	 *         javax.jms.JMSException if one is thrown.
	 * 
	 */
	private void sendReply(Message message) {
		/* Forward the message to the replyTo destination */
		Connection c = null;
		try {
			if (message.getJMSRedelivered() == true) {
				test.log("[XAMDBBean] JMSRedelivered == true");
			}

			/* Only perform UserTransaction calls if LOCAL_TRANS_MDB. */
			UserTransaction ut = null;
			if (test.transactionType == TransactionType.LOCAL_TRANS_MDB) {
				ut = beginLocalTrans(ut);
			}
			test.log("[XAMDBBean] JMSReplyTo == " + message.getJMSReplyTo().toString());

			cf = (ConnectionFactory) lookup(test.connectionFactory);
			Destination dest = message.getJMSReplyTo();
			c = cf.createConnection();
			Session s;

			s = c.createSession(true, Session.AUTO_ACKNOWLEDGE);

			MessageProducer p = s.createProducer(dest);
			p.send(message);

			test.log("[XAMDBBean] Forwarded message to:  " + message.getJMSReplyTo().toString());
			c.close();

			/* Only perform UserTransaction calls if LOCAL_TRANS_MDB. */
			if (test.transactionType == TransactionType.LOCAL_TRANS_MDB) {
				endLocalTrans(ut);
			}
		} catch (JMSException je) {
			test.log(je.getMessage());
			je.printStackTrace();
			// make sure connection is closed in the error case.
			try { 
				c.close();
			} catch (Exception e) {
				// do nothing
			}
			throw new RuntimeException("MDB sendReply() caught JMSException: " + je.getMessage(), je);
		}
	}

	/**
	 * Send text message containing TestProps logs back to the test driver for verification.
	 * 
	 * @throws RuntimeException sendGatheredLogs will throw a RuntimeException wrapped around a 
	 *         javax.jms.JMSException if one is thrown.
	 *         
	 */
	private void sendGatheredLogs() {
		Connection c = null;
		try {
			ConnectionFactory lcf = (ConnectionFactory) lookup(test.connectionFactory);
			Destination logdest = ImaJmsFactory.createTopic("/log/Topic/"+test.testNumber);
			c = lcf.createConnection();
			Session sess = c.createSession(false, Session.AUTO_ACKNOWLEDGE);
			MessageProducer prod = sess.createProducer(logdest);
			String logs = test.logs();
			System.out.println("sendGatheredLogs().logs: \n" + logs);
			Message m = sess.createTextMessage(logs);
			prod.send(m);
			c.close();
		} catch (JMSException je) {
			test.log(je.getMessage());
			je.printStackTrace();
			// Make sure connection gets closed
			try {
				c.close();
			} catch (Exception e) {
				// do nothing
			}
			throw new RuntimeException("MDB sendGatheredLogs() caught JMSException: " + je.getMessage(), je);
		}
	}

	private void sendBadMessageReply(String messageType) {
		Connection c = null;
		try {
			ConnectionFactory lcf = (ConnectionFactory) lookup("JMS_BASE_CF");
			Destination logdest = ImaJmsFactory.createTopic("/log/Topic/MQTTtoRA");
			c = lcf.createConnection();
			Session sess = c.createSession(false, Session.AUTO_ACKNOWLEDGE);
			MessageProducer prod = sess.createProducer(logdest);
			System.out.println("sendBadMessageReply(): \n" + messageType);
			Message m = sess.createTextMessage(messageType);
			prod.send(m);
			c.close();
		} catch (JMSException je) {
			je.printStackTrace();
			// Make sure connection gets closed
			try {
				c.close();
			} catch (Exception e) {
				// do nothing
			}
			throw new RuntimeException("MDB sendBadMessageReply() caught JMSException: " + je.getMessage(), je);
		}
	}

	/**
	 * Call the appropriate EJB depending on test.transactionType and test.ejbName.
	 */
	private void invokeEJB() {

		test.log("[XAMDBean] invokeEJB, test.transactionType: " + test.transactionType);

		/* Call out to an EJB */
		switch(test.transactionType) { 

		/* BMTUT means user transaction */
		case XA_BEAN_MANAGED_USER_TRANS:
			invokeBMTEJB();    
			break;

			/* Container manages the bean. May or may not be transacted depending on attribute */
		case XA_CONTAINER_MANAGED:
		case XA_CONTAINER_MANAGED_REQUIRED:
		case XA_CONTAINER_MANAGED_NOT_SUPPORTED:
			invokeCMTEJB();
			break;

			/* May be either container or bean-managed EJB that gets called */
		case LOCAL_TRANS_EJB:
			invokeLocalTXEJB();
			break;

		case NONE:
			break;
		default: /* test.TransactionType == null */
			test.log("test.TransactionType == null, you should consider setting it");
		}
	}

	/**
	 * Invoke the appropriate Bean-Managed Transaction EJB based on test.ejbName.
	 */
	private void invokeBMTEJB() {
		if ("XA_Stateful_Session_BeanTX_EJB".equals(test.ejbName)) {
			// Instantiate the EJB directly and call it from the MDB.
			XA_Session_BeanTX_EJB xaBean = new XA_Session_BeanTX_EJB();
			xaBean.setEJBContext(this.getMessageDrivenContext());
			test.log("[XAMDBBean] Calling XA_Stateful_Session_BeanTX.processTest() for BMTUT.");
			xaBean.processTest(test);

		} else if ("XA_Stateless_Session_BeanTX_EJB".equals(test.ejbName)) {
			// Instantiate the EJB directly and call it from the MDB.
			XA_Session_BeanTX_EJB xaBean = new XA_Session_BeanTX_EJB();
			xaBean.setEJBContext(this.getMessageDrivenContext());
			test.log("[XAMDBBean] Calling XA_Stateless_Session_BeanTX.processTest() for BMTUT.");
			xaBean.processTest(test);

		} else if ("XA_Singleton_Session_BeanTX_EJB".equals(test.ejbName)) {
			test.log("What to do here, since singleton is local, no interface");
		}
	}

	/**
	 * Invoke the appropriate Container-Managed Transaction EJB based on test.ejbName.
	 */
	private void invokeCMTEJB() {
		Object o = null;
		if ("XA_Stateful_Session_ContainerTX_EJB".equals(test.ejbName)) {
			if (test.appServer.equals(AppServer.WAS)) {
				o = lookup("XA_Stateful_Session_ContainerTX_EJB");
			} else if (test.appServer.equals(AppServer.WLS) || test.appServer.equals(AppServer.JBOSS)) {
				// weblogic and JBoss ejb remote interface jndi name
				o = lookup("java:module/XA_Stateful_Session_ContainerTX_EJB!com.ibm.ima.jca.mdb.XA_Session_ContainerTX_EJBRemote");
			} else {
				return;
			}
			XA_Session_ContainerTX_EJBRemote xaBeanRemote = (XA_Session_ContainerTX_EJBRemote) o;
			switch (test.transactionAttr) {
			case REQUIRED:
				test.log("[XAMDBBean] Calling XA_Stateful_Session_ContainerTX.sendMessage_REQUIRED() for CMT*.");
				test = xaBeanRemote.sendMessage_REQUIRED(test);
				break;
			case REQUIRES_NEW:
				test.log("[XAMDBBean] Calling XA_Stateful_Session_ContainerTX.sendMessage_REQUIRES_NEW() for CMT*.");
				test = xaBeanRemote.sendMessage_REQUIRES_NEW(test);
				break;
			case SUPPORTS:
				test.log("[XAMDBBean] Calling XA_Stateful_Session_ContainerTX.sendMessage_SUPPORTS() for CMT*.");
				test = xaBeanRemote.sendMessage_SUPPORTS(test);
				break;
			case NOT_SUPPORTED:
				test.log("[XAMDBBean] Calling XA_Stateful_Session_ContainerTX.sendMessage_NOT_SUPPORTED() for CMT*.");
				test = xaBeanRemote.sendMessage_NOT_SUPPORTED(test);
				break;
			case MANDATORY:
				test.log("[XAMDBBean] Calling XA_Stateful_Session_ContainerTX.sendMessage_MANDATORY() for CMT*.");
				test = xaBeanRemote.sendMessage_MANDATORY(test);
				break;
			case NEVER:
				test.log("[XAMDBBean] Calling XA_Stateful_Session_ContainerTX.sendMessage_NEVER() for CMT*.");
				test = xaBeanRemote.sendMessage_NEVER(test);
				break;
			default:
				throw new RuntimeException("Invalid TransactionAttribute value specified");
			}

		} else if ("XA_Stateless_Session_ContainerTX_EJB".equals(test.ejbName)) {
			if (test.appServer.equals(AppServer.WAS)) {
				o = lookup("XA_Stateless_Session_ContainerTX_EJB");
			} else if (test.appServer.equals(AppServer.WLS) || test.appServer.equals(AppServer.JBOSS)) {
				// weblogic and JBoss ejb remote interface jndi name
				o = lookup("java:module/XA_Stateless_Session_ContainerTX_EJB!com.ibm.ima.jca.mdb.XA_Session_ContainerTX_EJBRemote");
			} else {
				return;
			}
			XA_Session_ContainerTX_EJBRemote xaBeanRemote = (XA_Session_ContainerTX_EJBRemote) o;
			switch (test.transactionAttr) {
			case REQUIRED:
				test.log("[XAMDBBean] Calling XA_Stateless_Session_ContainerTX.sendMessage_REQUIRED() for CMT*.");
				test = xaBeanRemote.sendMessage_REQUIRED(test);
				break;
			case REQUIRES_NEW:
				test.log("[XAMDBBean] Calling XA_Stateless_Session_ContainerTX.sendMessage_REQUIRES_NEW() for CMT*.");
				test = xaBeanRemote.sendMessage_REQUIRES_NEW(test);
				break;
			case SUPPORTS:
				test.log("[XAMDBBean] Calling XA_Stateless_Session_ContainerTX.sendMessage_SUPPORTS() for CMT*.");
				test = xaBeanRemote.sendMessage_SUPPORTS(test);
				break;
			case NOT_SUPPORTED:
				test.log("[XAMDBBean] Calling XA_Stateless_Session_ContainerTX.sendMessage_NOT_SUPPORTED() for CMT*.");
				test = xaBeanRemote.sendMessage_NOT_SUPPORTED(test);
				break;
			case MANDATORY:
				test.log("[XAMDBBean] Calling XA_Stateless_Session_ContainerTX.sendMessage_MANDATORY() for CMT*.");
				test = xaBeanRemote.sendMessage_MANDATORY(test);
				break;
			case NEVER:
				test.log("[XAMDBBean] Calling XA_Stateless_Session_ContainerTX.sendMessage_NEVER() for CMT*.");
				test = xaBeanRemote.sendMessage_NEVER(test);
				break;
			default:
				throw new RuntimeException("Invalid TransactionAttribute value specified");
			}

		} else if ("XA_Singleton_Session_ContainerTX_EJB".equals(test.ejbName)) {
			test.log("what to do here, since singleton is local, no interface");
		} else {
			test.log("[XAMDBean] bad ejbName: " + test.ejbName);
		}

		if (test.rollbackType == TestProps.RollbackType.ROLLBACK
				|| test.rollbackType == TestProps.RollbackType.ROLLBACKMDBONLY) {
			myEJBCtx.setRollbackOnly();
			test.log("[XAMDBBean] Marking MDB for rollback.");
		} else if (test.rollbackType == TestProps.RollbackType.ROLLBACKEJBONLY) {
			if (myEJBCtx.getRollbackOnly()) {
				test.log("[XAMDBBean] Rollback was only set for the EJB. This will cause a TransactionRolledBackException.");
			} else {
				test.log("[XAMDBBean] The EJB should have set rollback, but it doesn't appear to be set in the MDB.");
			}
		}
	}

	/**
	 * If test.TransactionType == LOCAL_TRANS_EJB, then a local transaction
	 * may be performed from either a container or bean-managed transaction EJB.
	 * Call the appropriate bean based on test.ejbName.
	 */
	private void invokeLocalTXEJB() {
		if (test.ejbName.contains("ContainerTX")) {
			invokeCMTEJB();
		} else if (test.ejbName.contains("BeanTX")) {
			invokeBMTEJB();
		}
	}

	/**
	 * Crash primary server A1
	 * 
	 * @throws RuntimeException crashServer will throw a RuntimeException wrapped around a
	 *         FileNotFoundException if AUtils.getApplianceIPs() was unable to find the
	 *         env file containing the appliance IP Addresses.
	 * 
	 */
	private void crashServer() {
		try {
			String A1 = AUtils.getApplianceIPs()[0];
			test.log("about to stop-force MessageSight at: " + A1);
			AUtils.restartServer(A1);
			test.log("A1 (" + A1 + ") should be stopped now");
		} catch (FileNotFoundException fe) {
			test.log(fe.getMessage());
			fe.printStackTrace();
			throw new RuntimeException("MDB crashServer() caught FileNotFoundException: " + fe.getMessage(), fe);
		}
	}

	/**
	 * If test.transactionType == LOCAL_TRANS_MDB, then we are performing
	 * a local transaction on the reply message sent by this MDB.
	 * If it is bean-managed, then we need to call begin on the UserTransaction.
	 * 
	 * @param ut The UserTransaction to begin
	 * @return The started UserTransaction
	 * @throws RuntimeException beginLocalTrans will throw a RuntimeException wrapped around
	 *         either a NotSupportedException or SystemException thrown by ut.begin().
	 * 
	 */
	private UserTransaction beginLocalTrans(UserTransaction ut) {
		test.log("[XAMDBBean] Beginning local transaction in MDB");
		if (test.sendDestination.contains("BMTUT")) {
			try {
				ut = myEJBCtx.getUserTransaction();
				ut.begin();
			} catch (Exception e) {
				test.log(e.getMessage());
				e.printStackTrace();
				throw new RuntimeException("MDB beginLocalTrans() caught Exception: " + e.getMessage(), e);
			}
		}
		return ut;
	}

	/**
	 * If test.transactionType == LOCAL_TRANS_MDB, then we are performing
	 * a local transaction on the reply message sent by this MDB.
	 * If it is bean-managed, then we need to call rollback or commit on the UserTransaction.
	 * If it is container-managed, then we need to call setRollbackOnly if the test says to rollback.
	 * 
	 * @param ut The user transaction to commit or rollback.
	 * @throws RuntimeException endLocalTrans will throw a RuntimeException wrapped around 
	 *         following:
	 * 
	 * @throws SystemException - rollback(), commit()
	 * @throws SecurityException - rollback(), commit()
	 * @throws IllegalStateException - rollback(), commit()
	 * @throws RollbackException - commit()
	 * @throws HeuristicMixedException - commit()
	 * @throws HeuristicRollbackException - commit()
	 */
	private void endLocalTrans(UserTransaction ut) {
		if (test.sendDestination.contains("BMTUT")) {
			try {
				if (test.rollbackType == RollbackType.ROLLBACK) {
					test.log("[XAMDBBean] Rolling back local UserTransaction");
					ut.rollback();
				} else if (test.commitType != null) {
					test.log("[XAMDBBean] Committing local UserTransaction");
					ut.commit();
				}
			} catch (Exception e) {
				test.log(e.getMessage());
				e.printStackTrace();
				throw new RuntimeException("MDB endLocalTrans() caught Exception: " + e.getMessage(), e);
			}
		} else if (test.sendDestination.contains("CMT") && test.rollbackType == RollbackType.ROLLBACK) {
			myEJBCtx.setRollbackOnly();
		}
	}

	public TestProps getTest() {
		return test;
	}

	public void setTest(TestProps test) {
		this.test = test;
	}

	public void setMessageDrivenContext(javax.ejb.MessageDrivenContext mdc) {
		this.myEJBCtx = mdc;
	}

	private MessageDrivenContext getMessageDrivenContext() {
		return (MessageDrivenContext) myEJBCtx;
	}

	public Object lookup(String lookupString) {
		Object o = null;
		try {
			InitialContext ctx = new InitialContext(new Hashtable<String,String>());
			o = ctx.lookup(lookupString);
		}
		catch (Exception e) {
			e.printStackTrace();
		}
		return o;
	}

	public void ejbCreate() throws CreateException {   
	}

	@Override
	public void ejbRemove() throws EJBException {    
	}

}
