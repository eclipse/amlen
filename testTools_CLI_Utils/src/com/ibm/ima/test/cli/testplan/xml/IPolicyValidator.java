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
package com.ibm.ima.test.cli.testplan.xml;



public interface IPolicyValidator {
	
	public String getXmlName();
	
	public String getDriverName();
	
	
	public  void setClientId(String clientId);

	public  void setUser(String user);
	


	public  void setPassword(String password);



	public  void setTopic(String topic);
	
	public String getRootElementName();
	
	public Action getRootAction();
	

}
