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
package com.ibm.ima.test.cli.monitor;

public class DestMappingRuleStatResult {

	private String ruleName = null;
	private String queueManagerConnection = null;
	private int commitCount = 0;
	private int rollbackCount = 0;
	private int committedMessageCount = 0;
	private int persistentCount = 0;
	private int nonpersistentCount = 0;
	private int status = 0;
	private String expandedMessage = null;
	
	
	public DestMappingRuleStatResult(String ruleName, String queueManagerConnection, int commitCount,
			int rollbackCount, int committedMessageCount, int persistentCount, int nonpersistentCount,
			int status, String expandedMessage)
	{
		this.ruleName = ruleName;
		this.queueManagerConnection = queueManagerConnection;
		this.commitCount = commitCount;
		this.rollbackCount = rollbackCount;
		this.committedMessageCount = committedMessageCount;
		this.persistentCount = persistentCount;
		this.nonpersistentCount = nonpersistentCount;
		this.status = status;
		this.expandedMessage = expandedMessage;
		
	}
	
	public String getRuleName() {
		return ruleName;
	}
	public String getQueueManagerConnection() {
		return queueManagerConnection;
	}
	public int getCommitCount() {
		return commitCount;
	}
	public int getRollbackCount() {
		return rollbackCount;
	}
	public int getCommittedMessageCount() {
		return committedMessageCount;
	}
	public int getPersistentCount() {
		return persistentCount;
	}
	public int getNonpersistentCount() {
		return nonpersistentCount;
	}
	public int getStatus() {
		return status;
	}
	public String getExpandedMessage() {
		return expandedMessage;
	}
	
	

	public String toString()
	{
		StringBuffer buf = new StringBuffer();
		buf.append("RuleName=" + ruleName + "\n");
		buf.append("QueueManagerConnection=" + queueManagerConnection + "\n");
		buf.append("CommitCount=" + commitCount + "\n");
		buf.append("RollbackCount=" + rollbackCount + "\n");
		buf.append("CommittedMessageCount=" + committedMessageCount + "\n");
		buf.append("PersistentCount=" + persistentCount + "\n");
		buf.append("NonpersistentCount=" + nonpersistentCount + "\n");
		buf.append("Status=" + status + "\n");
		buf.append("ExpandedMessage=" + expandedMessage + "\n");
		
		return buf.toString();
	}
	

}
