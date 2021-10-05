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
package com.ibm.ima.jms.test.jca;

import java.io.InputStream;
import java.io.Serializable;
import java.util.HashMap;
import java.util.Map;
import java.util.Scanner;

import com.ibm.ima.jms.test.jca.BadTestPropsException;

public class TestProps implements Serializable {
	
    private static final long serialVersionUID = (long) 0xDEADBEEF;
    private StringBuilder logbuf;

    public enum AckMode { 
		AUTO_ACK, CLIENT_ACK, DISABLE_ACK;
		private static AckMode parse(String ackMode) {
			if(ackMode.equals("CLIENT_ACK"))
				return CLIENT_ACK;
			else if(ackMode.equals("DISABLE_ACK"))
				return DISABLE_ACK;
			else if(ackMode.equals("AUTO_ACK"))
				return AUTO_ACK;
			else
				throw new BadTestPropsException("Invalid AckMode specified: " + ackMode);
		}
		
		public int value() {
			switch(this) {
			case AUTO_ACK: return 1;
			case CLIENT_ACK: return 2;
			case DISABLE_ACK: return 3;
			default: throw new BadTestPropsException("Impossible Enumeration");
			}
		}
	}
    
    public enum AppServer {
    	WAS, WLS, JBOSS;
    	private static AppServer parse(String appServer) {
    		if (appServer.equals("WAS")) {
    			return WAS;
    		} else if (appServer.equals("WLS")) {
    			return WLS;
    		} else if (appServer.equals("JBOSS")) {
    			return JBOSS;
    		} else {
    			throw new BadTestPropsException("Invalid AppServer specified: " + appServer);
    		}
    	}
    }
    
    public enum DescriptorType {
    	XML, ANNOTATED;
    	private static DescriptorType parse(String descriptorType) {
    		if (descriptorType.equals("XML"))
    			return XML;
    		else if (descriptorType.equals("ANNOTATED"))
    			return ANNOTATED;
    		else
    			throw new BadTestPropsException("Invalid DescriptorType specified: " + descriptorType);
    	}
    }
	
	public enum DestinationType {
		TOPIC, QUEUE;
		private static DestinationType parse(String destType) {
			if(destType.equals("TOPIC"))
				return TOPIC;
			else if(destType.equals("QUEUE"))
				return QUEUE;
			else
				throw new BadTestPropsException("Invalid DestinationType specified: " + destType);
		}
	}
	
	public enum DeliveryMode {
		PERSISTENT, NON_PERSISTENT;
		private static DeliveryMode parse(String deliveryMode) {
			if(deliveryMode.equals("PERSISTENT"))
				return PERSISTENT;
			else if(deliveryMode.equals("NON_PERSISTENT"))
				return NON_PERSISTENT;
			else
				throw new BadTestPropsException("Invalid DeliveryMode specified: " + deliveryMode);
		}
		
		public int value() {
			switch(this) {
			case PERSISTENT: return 2;
			case NON_PERSISTENT: return 1;
			default: throw new BadTestPropsException("Impossible Enumeration");
			}
		}
	}
	
	public enum SecurityMode {
		ENABLED, DISABLED;
		private static SecurityMode parse(String securityMode) {
			if(securityMode.equals("ENABLED"))
				return ENABLED;
			else if(securityMode.equals("DISABLED"))
				return DISABLED;
			else
				throw new BadTestPropsException("Invalid SecuirtyMode specified: " + securityMode);
		}
	}
	
	public enum TransactionAttribute {  REQUIRED, 
										REQUIRES_NEW,
										SUPPORTS,
										NOT_SUPPORTED,
										MANDATORY,
										NEVER;
	    
    	private static TransactionAttribute parse(String tranAttr) {
    	    if(tranAttr.equals("REQUIRED"))
    	        return REQUIRED;
    	    else if(tranAttr.equals("REQUIRES_NEW"))
    	        return REQUIRES_NEW;
    	    else if(tranAttr.equals("SUPPORTS"))
    	        return SUPPORTS;
    	    else if(tranAttr.equals("NOT_SUPPORTED"))
                return NOT_SUPPORTED;
    	    else if(tranAttr.equals("MANDATORY"))
    	        return MANDATORY;
    	    else if(tranAttr.equals("NEVER"))
    	        return NEVER;
    	    else
    	        throw new BadTestPropsException("Invalid TransactionAttribute specified: " + tranAttr);
    	}
	}
	
	public enum TransactionType { NONE, DB2CHECK,
										LOCAL_TRANS_MDB,
										LOCAL_TRANS_EJB,
										XA_BEAN_MANAGED_USER_TRANS,
										XA_CONTAINER_MANAGED,
										XA_CONTAINER_MANAGED_NOT_SUPPORTED,
										XA_CONTAINER_MANAGED_REQUIRED,
										XA_UNSPECIFIED_TRANS_CONTEXT;
		private static TransactionType parse(String transType) {
			if(transType.equals("NONE"))
				return NONE;
			else if(transType.equals("LOCAL_TRANS_MDB"))
				return LOCAL_TRANS_MDB;
			else if(transType.equals("LOCAL_TRANS_EJB"))
			    return LOCAL_TRANS_EJB;
			else if(transType.equals("DB2CHECK"))
				return DB2CHECK;
			else if(transType.equals("XA_BEAN_MANAGED_USER_TRANS"))
				return XA_BEAN_MANAGED_USER_TRANS;
			else if(transType.equals("XA_CONTAINER_MANAGED"))
				return XA_CONTAINER_MANAGED;
			else if(transType.equals("XA_CONTAINER_MANAGED_NOT_SUPPORTED"))
				return XA_CONTAINER_MANAGED_NOT_SUPPORTED;
			else if(transType.equals("XA_CONTAINER_MANAGED_REQUIRED"))
				return XA_CONTAINER_MANAGED_REQUIRED;
			else if(transType.equals("XA_UNSPECIFIED_TRANS_CONTEXT"))
				return XA_UNSPECIFIED_TRANS_CONTEXT;
			else
				throw new BadTestPropsException("Invalid TransactionType specified: " + transType);
		}
	}
	
	public enum TransactionComponent {
		NONE, IMA, DB2, EVIL;
		private static TransactionComponent parse(String transComp) {
			if(transComp.equals("NONE"))
				return NONE;
			else if(transComp.equals("IMA"))
				return IMA;
			else if(transComp.equals("DB2"))
				return DB2;
			else if(transComp.equals("EVIL"))
				return EVIL;
			else
				throw new BadTestPropsException("Invalid TransactionComponent specified: " + transComp);
		}
	}
	
	public enum CommitType {
		ONE_PHASE, TWO_PHASE;
		private static CommitType parse(String commitType) {
			if(commitType.equals("ONE_PHASE"))
				return ONE_PHASE;
			else if(commitType.equals("TWO_PHASE"))
				return TWO_PHASE;
			else
				throw new BadTestPropsException("Invalid CommitType specified: " + commitType);
		}
	}
	
	public enum RollbackType {
		NONE, ROLLBACK, ROLLBACKEJBONLY, ROLLBACKMDBONLY;
		private static RollbackType parse(String rollbackType) {
			if(rollbackType.equals("NONE"))
				return NONE;
			else if(rollbackType.equals("ROLLBACK"))
				return ROLLBACK;
			else if(rollbackType.equals("ROLLBACKEJBONLY"))
				return ROLLBACKEJBONLY;
			else if(rollbackType.equals("ROLLBACKMDBONLY"))
			    return ROLLBACKMDBONLY;
			else
				throw new BadTestPropsException("Invalid RollbackType specified: " + rollbackType);
		}
	}

	public enum RollbackComponent { 
		BOTH, COMPONENT1, COMPONENT2;
		private static RollbackComponent parse(String rollbackComp) {
			if(rollbackComp.equals("COMPONENT1"))
				return COMPONENT1;
			else if(rollbackComp.equals("COMPONENT2"))
				return COMPONENT2;
			else if(rollbackComp.equals("BOTH"))
				return BOTH;
			else
				throw new BadTestPropsException("Invalid RollbackComponent specified: " + rollbackComp);
		}
	}
	
	public enum FailureType {
		NONE,
		EVIL_FAIL_PREPARE,
		EVIL_FAIL_COMMIT,
		EVIL_FAIL_ROLLBACK,
		EVIL_FAIL_START,
		EVIL_FAIL_END,
		EVIL_FAIL_FORGET,
		EVIL_FAIL_RECOVER,
		EVIL_FAIL_GET_TRANSACTION_TIMEOUT,
		EVIL_FAIL_SET_TRANSACTION_TIMEOUT,
		WAS_NETWORK_DISCONNECT,
		IMA_NETWORK_DISCONNECT,
		DB2_NETWORK_DISCONNECT,
		WAS_CRASH,
		IMA_CRASH,
		DB2_CRASH, 
		HEURISTIC_COMMIT,
		HEURISTIC_PREPARE;
		/* so many more... */
		private static FailureType parse(String failureType) {
			if(failureType.equals("NONE"))
				return NONE;
			else if(failureType.equals("EVIL_FAIL_PREPARE"))
				return EVIL_FAIL_PREPARE;
			else if(failureType.equals("EVIL_FAIL_COMMIT"))
				return EVIL_FAIL_COMMIT;
			else if(failureType.equals("EVIL_FAIL_ROLLBACK"))
				return EVIL_FAIL_ROLLBACK;
			else if(failureType.equals("EVIL_FAIL_START"))
				return EVIL_FAIL_START;
			else if(failureType.equals("EVIL_FAIL_END"))
				return EVIL_FAIL_END;
			else if(failureType.equals("EVIL_FAIL_FORGET"))
				return EVIL_FAIL_FORGET;
			else if(failureType.equals("EVIL_FAIL_RECOVER"))
				return EVIL_FAIL_RECOVER;
			else if(failureType.equals("EVIL_FAIL_GET_TRANSACTION_TIMEOUT"))
				return EVIL_FAIL_GET_TRANSACTION_TIMEOUT;
			else if(failureType.equals("EVIL_FAIL_SET_TRANSACTION_TIMEOUT"))
				return EVIL_FAIL_SET_TRANSACTION_TIMEOUT;
			else if(failureType.equals("WAS_NETWORK_DISCONNECT"))
				return WAS_NETWORK_DISCONNECT;
			else if(failureType.equals("IMA_NETWORK_DISCONNECT"))
				return IMA_NETWORK_DISCONNECT;
			else if(failureType.equals("DB2_NETWORK_DISCONNECT"))
				return DB2_NETWORK_DISCONNECT;
			else if(failureType.equals("WAS_CRASH"))
				return WAS_CRASH;
			else if(failureType.equals("IMA_CRASH"))
				return IMA_CRASH;
			else if(failureType.equals("DB2_CRASH"))
				return DB2_CRASH;
			else if(failureType.equals("HEURISTIC_PREPARE"))
				return HEURISTIC_PREPARE;
			else if(failureType.equals("HEURISTIC_COMMIT"))
				return HEURISTIC_COMMIT;
			else
				throw new BadTestPropsException("Invalid FailureType specified: " + failureType);
		}
	}
	
	public int testNumber;
	public String hostName;
	public String connectionFactory;
	public String sendDestination;
	public String replyDestination;
	public String selector;
	public DestinationType destinationType;
	public AppServer appServer;
	public DescriptorType descriptorType;
	public int numMessages;
	public SecurityMode securityMode;
	public AckMode ackMode;
	public DeliveryMode deliveryMode;
	public String ejbName;
	public String db2Name;
	public TransactionAttribute transactionAttr;
	public TransactionType transactionType;
	public TransactionComponent transactionComponent1;
	public TransactionComponent transactionComponent2;
	public CommitType commitType;
	public RollbackType rollbackType;
	public RollbackComponent rollbackComponent;
	public FailureType failureType;
	
	private TestProps(String testNum) {
		testNumber = Integer.parseInt(testNum);
		logbuf = new StringBuilder();
	}
	
	public String toString() {
		StringBuilder sb = new StringBuilder();
		sb.append("[[TestNumber: ").append(testNumber);
		sb.append(", HostName: ").append(hostName);
		sb.append(", ConnectionFactory: ").append(connectionFactory);
		sb.append(", SendDestination: ").append(sendDestination);
		sb.append(", ReplyDestination: ").append(replyDestination);
		sb.append(", AppServer: ").append(appServer);
		sb.append(", Selector: " + selector);
		sb.append(", DestinationType: ").append(destinationType);
		sb.append(", SecurityMode: ").append(securityMode);
		sb.append(", NumMessages: ").append(numMessages);
		sb.append(", AckMode: ").append(ackMode);
		sb.append(", DeliveryMode: ").append(deliveryMode);
		sb.append(", EJBName: ").append(ejbName);
		sb.append(", DB2Name: ").append(db2Name);
		sb.append(", DescriptorType: " ).append(descriptorType);
		sb.append(", TransactionAttribute: ").append(transactionAttr);
		sb.append(", TransactionType: ").append(transactionType);
		sb.append(", TransactionComponent1: ").append(transactionComponent1);
		sb.append(", TransactionComponent2: ").append(transactionComponent2);
		sb.append(", CommitType: ").append(commitType);
		sb.append(", RollbackType: ").append(rollbackType);
		sb.append(", RollbackComponent: ").append(rollbackComponent);
		sb.append(", FailureType: ").append(failureType);
		sb.append("]]");
		return sb.toString();
	}
	
	public static Map<Integer, TestProps> readCases(InputStream is) {
		Map<Integer, TestProps> cases = new HashMap<Integer, TestProps>();
		
		Scanner s = new Scanner(is);
		
		TestProps tp = null;
		
		while(s.hasNextLine()) {
			String line = s.nextLine().trim();
			if(line.length() == 0 || line.startsWith("#"))
				continue;
			String[] toks = line.split("\\s+");
			String key = toks[0];
			String value = toks[1];
			
			if(key.equals("TestNumber")) {
				if(tp != null)
					addTest(cases, tp);
				tp = new TestProps(value);
			}
			else if(key.equals("HostName"))
				tp.hostName = value;
			else if(key.equals("ConnectionFactory")) 
			    tp.connectionFactory = value;
			else if(key.equals("SendDestination")) 
			    tp.sendDestination = value;
			else if(key.equals("ReplyDestination")) 
			    tp.replyDestination = value;
			else if(key.equals("DestinationType"))
				tp.destinationType = DestinationType.parse(value);
			else if(key.equals("AppServer"))
				tp.appServer = AppServer.parse(value);
			else if(key.equals("DescriptorType"))
				tp.descriptorType = DescriptorType.parse(value);
			else if(key.equals("Selector"))
				tp.selector = value;				
			else if(key.equals("Security"))
				tp.securityMode = SecurityMode.parse(value);
			else if(key.equals("AckMode"))
				tp.ackMode = AckMode.parse(value);
			else if(key.equals("DeliveryMode"))
				tp.deliveryMode = DeliveryMode.parse(value);
			else if(key.equals("NumMessages"))
				tp.numMessages = Integer.parseInt(value);
			else if(key.equals("EJBName"))
			    tp.ejbName = value;
			else if(key.equals("DB2Name"))
				tp.db2Name = value;
			else if(key.equals("TransactionAttribute"))
			    tp.transactionAttr = TransactionAttribute.parse(value);
			else if(key.equals("TransactionType"))
				tp.transactionType = TransactionType.parse(value);
			else if(key.equals("TransactionComponentOne"))
				tp.transactionComponent1 = TransactionComponent.parse(value);
			else if(key.equals("TransactionComponentTwo"))
				tp.transactionComponent2 = TransactionComponent.parse(value);
			else if(key.equals("CommitType"))
				tp.commitType = CommitType.parse(value);
			else if(key.equals("RollbackType"))
				tp.rollbackType = RollbackType.parse(value);
			else if(key.equals("RollbackComponent"))
				tp.rollbackComponent = RollbackComponent.parse(value);
			else if(key.equals("FailureType"))
				tp.failureType = FailureType.parse(value);
			else {
				s.close();
				throw new BadTestPropsException("Invalid TestProps property specified: " + key);
			}
		}
		
		if(tp != null)
			addTest(cases, tp);
		
		s.close();
		return cases;
	}
	
	private static void addTest(Map<Integer, TestProps> cases, TestProps test) {
		if(cases.containsKey(test.testNumber))
			throw new BadTestPropsException("Duplicate TestNumber found: " + test.testNumber);
		cases.put(test.testNumber, test);
	}
	
	public void logf(String fmt, Object... args) {
		log(String.format(fmt, args));
	}
	
	public void log(String msg) {
		logbuf.append(msg).append("\n");
		System.out.println(msg);
	}
	
	public String logs() {
		return logbuf.toString();
	}
}
