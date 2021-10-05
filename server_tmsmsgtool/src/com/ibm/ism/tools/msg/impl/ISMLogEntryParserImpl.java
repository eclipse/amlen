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
package com.ibm.ism.tools.msg.impl;

import com.ibm.ism.tools.msg.ISMLogEntryObject;
import com.ibm.ism.tools.msg.ISMLogEntryParser;
import com.ibm.ism.tools.msg.ISMLogFileParser;
import com.ibm.ism.tools.msg.ISMLogMsgObject;
import com.ibm.ism.tools.msg.ISMMsgTool;

public class ISMLogEntryParserImpl implements ISMLogEntryParser {

	

	public ISMLogEntryObject parseLLMLogLine(String traceLine,
			ISMLogEntryObject logEntry) throws RuntimeException {
		if (traceLine.indexOf("LOG(") >= 0) {
			logEntry = parseSimpleTraceInvoke(traceLine, logEntry);
		}else if(traceLine.startsWith("RC") && traceLine.indexOf("ISMRC_")>0  ){
			/*Parse the enum for the error code*/
			
			logEntry = parseErrorCodeString(traceLine, logEntry);
			
		}else if(traceLine.startsWith("DISPLAY_MSG") ){
			/*Parse the enum for the error code*/
			
			logEntry = parseISMCLIDisplayMsgString(traceLine, logEntry);
			
		}else if(traceLine.startsWith("GET_MSG") ){
			/*Parse the enum for the error code*/
			
			logEntry = parseISMCLIGetMsgString(traceLine, logEntry);
			
		}else if(traceLine.startsWith("ADD_HELPMSG") ){
			/*Parse the enum for the error code*/
			
			logEntry = parseISMCLIAddHelpMsgString(traceLine, logEntry);
			
		}
		return logEntry;
	}
	public ISMLogEntryObject parseErrorCodeString(String traceLine,
			ISMLogEntryObject logEntry) throws RuntimeException {
	//	System.out.println("parseErrorCodeString: " + traceLine);
		
		//String rx = "[{}],";
		//String[] items=  traceLine.split(rx);
		String[] items=  {traceLine};
		 for (int i = 0; i < items.length; i++) {
			 if((items[i].startsWith("{") || items[i].startsWith("RC")) && items[i].indexOf("ISMRC_")>0){
				 //System.out.println("Item: "+items[i]);
				 String sline = items[i].trim();
				 sline = sline.substring(2, sline.length());
				 sline = sline.substring(sline.indexOf("("), sline.length());
	        	 ISMLogMsgObject msgObject = new ISMLogMsgObject();
	        	 logEntry.addLogMsg(msgObject);
	        	 logEntry.setLogLevel("");
	        	 for (int count = 0; count <= 5; count++) {
	     			switch (count) {

	     			case 0:
	     				
	     				//Parse Error Code name
	     				String errorCodeName = sline.substring(1, sline.indexOf(","));
	     			
	     				logEntry.setErrorCodeName(errorCodeName.trim());
	     				sline = sline.substring(sline.indexOf(",") + 1);
	     					
	     				
	     				
	     				break;
	     			case 1:
	     				//Parse Error Code Value
	     				String errcodevalue = sline.substring(0, sline.indexOf(","));
	     				logEntry.setMsgKey(errcodevalue.trim());
	     				
	     				sline = sline.substring(sline.indexOf(",") + 1);
	     				break;
	     			case 2:
	     				//Parse Error Code Opts
	     				String errorcodeopts = sline.substring(0, sline.indexOf(","));
	     				sline = sline.substring(sline.indexOf(",") + 1);
	     				break;
	     				
	     			case 3:
	     				//Parse Error Code Message Text
	     				String msgText = sline.substring(0, sline.lastIndexOf(")"));
	     				msgText=msgText.trim();
	     				msgObject.setMsgText(ISMLogEntryParserUtil
	    						.extractTextBetweenQuotes(msgText));
	     				
	     				break;
	     			
	     			}

	     		}

			 }
		 }
		
		
		return logEntry;

	}
	

	public ISMLogEntryObject parseSimpleTraceInvoke(String traceLine,
			ISMLogEntryObject logEntry) throws RuntimeException {
		// System.out.println("SimpleTrace: " + traceLine);
		logEntry = parseLLMSampleTraceLogLine(traceLine, logEntry);
		// addAndValidateEntry(logEntry);
		return logEntry;

	}

	public ISMLogEntryObject parseLLMSampleTraceLogLine(String line,
			ISMLogEntryObject logEntry) throws RuntimeException {

		if (line == null)
			return null;

		String sline = line.substring(line.indexOf("(") + 1);
		ISMLogMsgObject msgObject = new ISMLogMsgObject();
		logEntry.addLogMsg(msgObject);
	//	System.out.println("logline="+sline);
		for (int count = 0; count <= 4; count++) {
			switch (count) {

			case 0:
				String logLevel = sline.substring(0, sline.indexOf(","));
				logEntry.setLogLevel(logLevel);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("LogLevel: "+ logLevel);
				break;
			case 1:
				String category = sline.substring(0, sline.indexOf(","));
				logEntry.setCategory(category);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("LogLevel: "+ logLevel);
				break;
			case 2:
				String msgKeyStr = sline.substring(0, sline.indexOf(","));
				msgKeyStr = msgKeyStr.trim();
				int msgKeyNum=0;
				try{
					msgKeyNum = Integer.parseInt(msgKeyStr)		;	
				}catch(Exception e){
					throw new RuntimeException(e);
				}
				logEntry.setMsgKey(msgKeyNum);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("MsgKey: "+ getMsgKeyNumber(msgKeyStr));

				break;
			case 3:
				String replacementTypes = sline
						.substring(0, sline.indexOf(","));
				sline = sline.substring(sline.indexOf(",") + 1);
				msgObject.setReplaceTypes(ISMLogEntryParserUtil
						.getTextBetweenQuote(replacementTypes));
				// System.out.println("ReplacementTypes: "+ replacementTypes);
				break;

			case 4:
				msgObject.setMsgText(ISMLogEntryParserUtil
						.extractTextBetweenQuotes(sline));

			}

		}

		return logEntry;
	}
	
	public ISMLogEntryObject parseISMCLIDisplayMsgString(String line,
			ISMLogEntryObject logEntry) throws RuntimeException {

		if (line == null)
			return null;

		String sline = line.substring(line.indexOf("(") + 1);
		ISMLogMsgObject msgObject = new ISMLogMsgObject();
		logEntry.addLogMsg(msgObject);
	//	System.out.println("logline="+sline);
		for (int count = 0; count <= 4; count++) {
			switch (count) {

			case 0:
				String logLevel = sline.substring(0, sline.indexOf(","));
				logEntry.setLogLevel(logLevel);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("LogLevel: "+ logLevel);
				break;
			
			case 1:
				String msgKeyStr = sline.substring(0, sline.indexOf(","));
				msgKeyStr = msgKeyStr.trim();
				int msgKeyNum=0;
				try{
					msgKeyNum = Integer.parseInt(msgKeyStr)		;	
				}catch(Exception e){
					throw new RuntimeException(e);
				}
				logEntry.setMsgKey(msgKeyNum);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("MsgKey: "+ getMsgKeyNumber(msgKeyStr));

				break;
			case 2:
				String replacementTypes = sline
						.substring(0, sline.indexOf(","));
				sline = sline.substring(sline.indexOf(",") + 1);
				msgObject.setReplaceTypes(ISMLogEntryParserUtil
						.getTextBetweenQuote(replacementTypes));
				// System.out.println("ReplacementTypes: "+ replacementTypes);
				break;

			case 3:
				msgObject.setMsgText(ISMLogEntryParserUtil
						.extractTextBetweenQuotes(sline));

			}

		}

		return logEntry;
	}

	public ISMLogEntryObject parseISMCLIGetMsgString(String line,
			ISMLogEntryObject logEntry) throws RuntimeException {

		if (line == null)
			return null;

		String sline = line.substring(line.indexOf("(") + 1);
		ISMLogMsgObject msgObject = new ISMLogMsgObject();
		logEntry.addLogMsg(msgObject);
		logEntry.setLogLevel("");
	//	System.out.println("logline="+sline);
		for (int count = 0; count <= 4; count++) {
			switch (count) {

			
			case 0:
				String msgKeyStr = sline.substring(0, sline.indexOf(","));
				
				msgKeyStr = msgKeyStr.trim();
				int msgKeyNum=0;
				try{
					
					msgKeyNum = Integer.parseInt(msgKeyStr)		;	
				}catch(Exception e){
					if(msgKeyStr.startsWith("ISMRC")){
						
						msgKeyNum = ISMMsgTool.util.getErrorCodeNumber(msgKeyStr);
					}
					if(msgKeyNum<0){
						throw new RuntimeException(e);
					}
				}
				logEntry.setMsgKey(msgKeyNum);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("MsgKey: "+ getMsgKeyNumber(msgKeyStr));

				break;
			case 1:
			case 2:
				sline = sline.substring(sline.indexOf(",") + 1);
				break;
			case 3:
				String replacementTypes = sline
						.substring(0, sline.indexOf(","));
				sline = sline.substring(sline.indexOf(",") + 1);
				msgObject.setReplaceTypes(ISMLogEntryParserUtil
						.getTextBetweenQuote(replacementTypes));
				// System.out.println("ReplacementTypes: "+ replacementTypes);
				break;

			case 4:
				msgObject.setMsgText(ISMLogEntryParserUtil
						.extractTextBetweenQuotes(sline));

			}

		}

		return logEntry;
	}
	
	public ISMLogEntryObject parseISMCLIAddHelpMsgString(String line,
			ISMLogEntryObject logEntry) throws RuntimeException {

		if (line == null)
			return null;
		
		String sline = line.substring(line.indexOf("(") + 1);
		
		logEntry.setLogLevel("");
		ISMLogMsgObject msgObject = new ISMLogMsgObject();
		msgObject.setAllowDup(true);
		//logEntry.addLogMsg(msgObject);
	//	System.out.println("logline="+sline);
		for (int count = 0; count <= 2; count++) {
			switch (count) {

			
			case 0:
				String msgKeyStr = sline.substring(0, sline.indexOf(","));
				msgKeyStr = msgKeyStr.trim();
				int msgKeyNum=0;
				try{
					msgKeyNum = Integer.parseInt(msgKeyStr)		;	
				}catch(Exception e){
					throw new RuntimeException(e);
				}
				
				ISMLogEntryObject tmplogEntry = ISMLogFileParser.HelpLogEntryMap.get(msgKeyStr);
				if(tmplogEntry!=null){
					logEntry = tmplogEntry;
					
				}else{
					ISMLogFileParser.HelpLogEntryMap.put(msgKeyStr, logEntry);
				}
				
				logEntry.setMsgKey(msgKeyNum);
				sline = sline.substring(sline.indexOf(",") + 1);
				// System.out.println("MsgKey: "+ getMsgKeyNumber(msgKeyStr));

				break;
			case 1:
				String msgText = ISMLogEntryParserUtil.extractTextBetweenQuotes(sline);
				
				msgObject.setMsgText(msgText);
				

			}

		}
		logEntry.addLogMsg(msgObject);
		return logEntry;
	}

}
