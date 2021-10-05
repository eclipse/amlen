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

import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.w3c.dom.Text;

public class ISMLogEntryParserUtil {
	
	public static boolean isLogLine(String inline){
		boolean isLogLine=false;
	
		if (!inline.startsWith("LOGCAT") && inline.startsWith("LOG")) {
			isLogLine = true;
		}else if(inline.startsWith("{") && inline.indexOf("ISMRC_")>0  ){
			isLogLine = true;
		}else if(inline.startsWith("ISMRC_") && inline.indexOf("=")>0 ){
			isLogLine = true;
		}else if(inline.startsWith("RC") && inline.indexOf("ISMRC_")>0 ){
			isLogLine = true;
		}else if(inline.startsWith("DISPLAY_MSG") ){
			isLogLine = true;
		}else if(inline.startsWith("GET_MSG") ){
			isLogLine = true;
		}else if(inline.startsWith("ADD_HELPMSG") ){
			isLogLine = true;
		}
		return isLogLine;
		
	}
	public static String getTextBetweenQuote(String inputText) {
		Pattern pattern = Pattern.compile("(\")(.*)(\")");
		Matcher matcher = pattern.matcher(inputText);
		String retVal = null;
		if (matcher.find()) {
			retVal = matcher.group(2);
		}

		return retVal;
	}
	
	public static String extractTextBetweenQuotes(String inputString) {
		StringBuffer tempBuf = new StringBuffer();
		// System.out.println("Input String: "+inputString);
		boolean inLiteral = false;
		if (inputString != null) {
			String tString = inputString.trim();
			char[] stringArr = tString.toCharArray();
			for (int count = 0; count < stringArr.length; count++) {

				char ac = stringArr[count];
				if (ac == '\"') {
					if (count > 0 && stringArr[count - 1] == '\\') {
						// Check if the escape char
						tempBuf.append(ac);
					} else if (!inLiteral) {
						// it is the beginning of the string
						inLiteral = true;
						continue;
					} else {
						inLiteral = false;
						break;
					}
				} else {
					tempBuf.append(ac);
				}
			}
		}
		
		//return tempBuf.toString().replaceAll("\\\\\"", "'");
		return tempBuf.toString();
	}
	
	
	public static String extractTextBetweenBracket(String inputString) {
		StringBuffer tempBuf = new StringBuffer();
		// System.out.println("Input String: "+inputString);
		boolean inLiteral = false;
		if (inputString != null) {
			char[] stringArr = inputString.toCharArray();
			for (int count = 0; count < stringArr.length; count++) {

				char ac = stringArr[count];
				if (ac == '\"') {
					if (count > 0 && stringArr[count - 1] == '\\') {
						// Check if the escape char
						tempBuf.append(ac);
					} else if (!inLiteral) {
						// it is the beginning of the string
						inLiteral = true;
						continue;
					} else {
						inLiteral = false;
						break;
					}
				} else {
					tempBuf.append(ac);
				}
			}
		}

		return tempBuf.toString().replaceAll("\\\\\"", "'");
		
	}
	
	private static void removeAllChildNodes(Node node) {
	    NodeList childNodes = node.getChildNodes();
	    int length = childNodes.getLength();
	    for (int i = 0; i < length; i++) {
	        Node childNode = childNodes.item(i);
	        if(childNode instanceof Element) {
	            if(childNode.hasChildNodes()) {
	                removeAllChildNodes(childNode);                
	            }        
	            node.removeChild(childNode);  
	        }
	    }
	}
	public static void parseRawStringIntoTMSXMLNode(Document doc, Node pnode, String rawStr){
		/*parse each character. If see " then add 2 child node into the primary node*/
		if(rawStr!=null){
			/*Remove all child nodes first*/
			removeAllChildNodes(pnode);
			pnode.setTextContent(null);
			int rawStrLen = rawStr.length();
			StringBuffer buffer = new StringBuffer();
			Text txtNode =null;
			Node nuNode=null;
			for(int index = 0; index < rawStrLen; index++){
				char strChar = rawStr.charAt(index);
				char nextStrChar = ((index+1)<rawStrLen)?rawStr.charAt(index+1):0;
				if(strChar=='\\' && (nextStrChar=='\"' ||nextStrChar=='n')){
					txtNode = doc.createTextNode(buffer.toString());
					pnode.appendChild(txtNode);
					
					switch(nextStrChar){
					case '\"':
						nuNode = doc.createElement("q");
						pnode.appendChild(nuNode);
						index++; //Skip the escaped character (")
						break;
					case 'n':
						nuNode = doc.createElement("br");
						pnode.appendChild(nuNode);
						index++; //Skip the escaped character (")
						break;
					default:
						
					};
					
					buffer.delete(0, buffer.length());
					
				}else{
					/*Add to buffer*/
					buffer.append(strChar);
				}
			}
			/*Set the rest of the buffer*/
			if(buffer.length()>0){
				txtNode = doc.createTextNode(buffer.toString());
				pnode.appendChild(txtNode);
				buffer.delete(0, buffer.length());
				
			}
			
		}
		
	
	}




}
