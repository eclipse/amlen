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
package com.ibm.ism.test.testcases.bvt;

import com.ibm.ism.test.common.IsmTestData;
import com.ibm.ism.test.testcases.messaging.connectionpolicies.ImsCreateConnectionPoliciesTestData;
import com.ibm.ism.test.testcases.messaging.endpoints.ImsCreateEndpointsTestData;
import com.ibm.ism.test.testcases.messaging.messagehubs.ImsCreateMessageHubTestData;
import com.ibm.ism.test.testcases.messaging.messagingpolicies.ImsCreateMessagingPoliciesTestData;

public class ImsBVTTestData extends IsmTestData {
	
	private ImsCreateMessageHubTestData msghubTestData;
	private ImsCreateConnectionPoliciesTestData connpolTestData;
	private ImsCreateMessagingPoliciesTestData msgpolTestData;
	private ImsCreateEndpointsTestData endpointTestData;

	public ImsBVTTestData(String[] args) {
		super(args);
		msghubTestData = new ImsCreateMessageHubTestData(args);
		connpolTestData = new ImsCreateConnectionPoliciesTestData(args);
		msgpolTestData = new ImsCreateMessagingPoliciesTestData(args);
		endpointTestData = new ImsCreateEndpointsTestData(args);
	}
	
	public ImsCreateMessageHubTestData getMessageHubTestData() {
		return msghubTestData;
	}
	
	public ImsCreateConnectionPoliciesTestData getConnectionPolicyTestData() {
		return connpolTestData;
	}
	
	public ImsCreateMessagingPoliciesTestData getMessagingPolicyTestData() {
		return msgpolTestData;
	}
	
	public ImsCreateEndpointsTestData getEndpointTestData() {
		return endpointTestData;
	}
}
