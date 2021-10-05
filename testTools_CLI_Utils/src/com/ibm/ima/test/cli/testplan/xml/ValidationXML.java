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

import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.StringWriter;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

public  class ValidationXML   {
	
	private static String testName = "jms_mqtt_001mqtt";		
	
	public static void setTestName(String name) {
		testName = name;
	}

	public static String getXml(String rootElementName, Action rootAction) throws Exception {
		
		
		DocumentBuilderFactory dbfac = DocumentBuilderFactory.newInstance();
        DocumentBuilder docBuilder = dbfac.newDocumentBuilder();
        Document doc = docBuilder.newDocument();
	

        Element root = doc.createElement(rootElementName);
        root.setAttribute("name", testName);
        
        //SysClient sysClient = new SysClient("../common/JMS_syncip.xml", "../common/JMS_syncport.xml", "tmrefcount");
        SysClient sysClient = new SysClient("../common/JMS_syncip.xml", "../common/JMS_syncport.xml", testName);
        Element sysClientElement = sysClient.getElement(doc);
        root.appendChild(sysClientElement);


        Element validationElement = rootAction.getElement(doc);
          
          root.appendChild(validationElement);

        
	     TransformerFactory transfac = TransformerFactory.newInstance();
         Transformer trans = transfac.newTransformer();
         trans.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "no");
         trans.setOutputProperty(OutputKeys.INDENT, "yes");
         trans.setOutputProperty("{http://xml.apache.org/xslt}indent-amount", "3");

         //create string from xml tree
         StringWriter sw = new StringWriter();
         StreamResult result = new StreamResult(sw);
 
         DOMSource source = new DOMSource(root);
         trans.transform(source, result);
         String xmlString = sw.toString();

         //print xml
        // System.out.println("Here's the xml:\n\n" + xmlString);
        
		
		return xmlString;
	}
	
	public static void writeXml(String rootElementName, Action rootAction, OutputStream zOut) {
		
 		try {
 			OutputStreamWriter out = new OutputStreamWriter(zOut,"utf-8");
 			String xmlString = getXml(rootElementName, rootAction);
 			String xmlUTFString = null;
 			byte[] utf8Bytes = xmlString.getBytes("UTF8");
 			xmlUTFString = new String(utf8Bytes, "UTF8");
 			out.write(xmlUTFString);
 			out.flush();
 		} catch (Exception e) {
 			e.printStackTrace();
 		}
		
	}
	



}
