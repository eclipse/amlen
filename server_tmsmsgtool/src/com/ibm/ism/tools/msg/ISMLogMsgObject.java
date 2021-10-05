// Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.tools.msg;


/**
 * The Log Message Object contains the message replace types, message text,
 * and message data variable
 * 
 *
 */
public class ISMLogMsgObject {

	private String replaceTypes;
	private String msgText;
	private String replaceData;
	private boolean allowDup=false;

	public boolean isAllowDup() {
		return allowDup;
	}

	public void setAllowDup(boolean allowDup) {
		this.allowDup = allowDup;
	}

	/**
	 * Get message replacement data types
	 * @return String of types
	 */
	public String getReplaceTypes() {
		return replaceTypes;
	}

	/**
	 * Set the message replacement data types
	 * @param replaceTypes string 
	 */
	public void setReplaceTypes(String replaceTypes) {
		this.replaceTypes = replaceTypes;
	}

	/**
	 * Get message text
	 * @return message text string
	 */
	public String getMsgText() {
		return msgText;
	}

	/**
	 * Set message text
	 * @param msgText string
	 */
	public void setMsgText(String msgText) {
		this.msgText = msgText;
	}

	/**
	 * Get the replacement data string
	 * @return replacement data string
	 */
	public String getReplaceData() {
		return replaceData;
	}

	/**
	 * Set replacement data
	 * @param replaceData string
	 */
	public void setReplaceData(String replaceData) {
		this.replaceData = replaceData;
	}
	
	public boolean equals(Object obj){
		if(obj == null){
			return false;
		}
	
		ISMLogMsgObject msgObj = (ISMLogMsgObject)obj;
		
		if(msgObj.isAllowDup()){
			return false;
		}
		
		if(msgText==null || msgObj.msgText==null)
			return false;
		if(this.msgText.equals(msgObj.msgText)){
			return true;
		}else{
			return false;
		}
		
   }
	

}
