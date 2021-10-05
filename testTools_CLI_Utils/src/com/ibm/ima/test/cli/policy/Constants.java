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

package com.ibm.ima.test.cli.policy;

import java.util.ArrayList;
import java.util.List;

public class Constants {
	
	public enum PROTOCAL {
		
		JMS("JMS"),
		MQTT("MQTT"),
		JMS_MQTT("JMS,MQTT");
	
	  private String value;
	  private List<String> protocolStrings = new ArrayList<String>();

	  PROTOCAL(String value) {
	    this.value = value;
	    protocolStrings.add(value);
	  }
	  
	  PROTOCAL(String value1, String value2) {
		    this.value = value1 + "," + value2;
		    protocolStrings.add(value1);
		    protocolStrings.add(value2);
	  }

	  public String getText() {
	    return this.value;
	  }
	  
	  public List<String> getProtocolStrings() {
		  return this.protocolStrings;
	  }
	  
	  public static PROTOCAL fromString(String value) {
		    if (value != null) {
		      for (PROTOCAL b : PROTOCAL.values()) {
		        if (value.equalsIgnoreCase(b.value)) {
		          return b;
		        }
		      }
		      if (value.equals("JMS_MQTT")) {
		    	  return JMS_MQTT;
		      }
		    }
		    return null;
		  }


	}
	
	public enum CLIENTS {
		
		M1("${M1_IPv4_1}"),
		M2("${M2_IPv4_1}"),
		M1_M2("${M1_IPv4_1},${M2_IPv4_1}");
	
	  private String value;

	  CLIENTS(String value) {
	    this.value = value;
	  }

	  public String getText() {
	    return this.value;
	  }
	  
	  public static CLIENTS fromString(String value) {
		    if (value != null) {

		      if(value.equals("M1")) {
		    	  return M1;
		      } else if (value.equals("M2")) {
		    	  return M2;
		      } else if (value.equals("M1_M2")) {
		    	  return M1_M2;
		      } 
		    
		  }
		    return null;
	}

	}
	
	public enum EPINTERFACE {
		
		ALL("all"),
		A1("${A1_IPv4_INTERNAL_1}");
	
	  private String value;

	  EPINTERFACE(String value) {
	    this.value = value;
	  }

	  public String getText() {
	    return this.value;
	  }
	  
	  public static EPINTERFACE fromString(String value) {
		    if (value != null) {
		      for (EPINTERFACE b : EPINTERFACE.values()) {
		        if (value.equalsIgnoreCase(b.value)) {
		          return b;
		        }
		      }
		      
		      if (value.equals("A1")) {
		    	  return A1;
		      }
		    
		  }
		   return null;
	}

	}
	
	
	public enum MESSAGING_ACTION {
		
		PUB("Publish"),
		SUB("Subscribe"),
		PUB_SUB("Publish,Subscribe"),
		REC("Receive"),
		SEND("Send"),
		BRW("Browse"),
		REC_SEND("Receive,Send"),
		REC_BRW("Receive,Browse"),
		REC_SEND_BRW("Receive,Send,Browse"),
		SEND_BRW("Send,Browse");
	
	  private String value;

	  MESSAGING_ACTION(String value) {
	    this.value = value;
	  }

	  public String getText() {
	    return this.value;
	  }

	}
	
	public enum DESTINATION_TYPE {
		
		TOPIC("Topic"),
		QUEUE("Queue");
	
	  private String value;

		DESTINATION_TYPE(String value) {
	    this.value = value;
	  }

	  public String getText() {
	    return this.value;
	  }

	}
	
	
	public enum TESTCASE_TYPE {
		
		VALID_SEPARATION("validSeparation"),
		VALID_NO_SEPARATION("validNoSeparation"),
		NEGATIVE_SEPARATION("negativeSeparation"),
		NEGATIVE_NO_SEPARATION("negativeNoSeparation"),
		GVT_TOPIC("gvtTopic");
	
	  private String value;

	  TESTCASE_TYPE(String value) {
	    this.value = value;
	  }

	  public String getText() {
	    return this.value;
	  }

	}
	
	


}
