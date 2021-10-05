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

import java.util.ArrayList;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.List;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Comment;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Text;

public class ActionChild {
	
	private String elementName = null;
	private String elementText = null;
	private List<String> comments = new ArrayList<String>();
	private LinkedHashMap<String, String> attrs = new LinkedHashMap<String, String>();
	
	public ActionChild(String elementName, String elementText) {
		this.elementName = elementName;
		this.elementText = elementText;
	}
	
	public ActionChild(String elementName, String elementText, String attrName, String attrValue) {
		setElementName(elementName);
		setElementText(elementText);
		addAttr(attrName, attrValue);
	}

	public String getElementName() {
		return elementName;
	}

	public void setElementName(String elementName) {
		this.elementName = elementName;
	}

	public String getElementValue() {
		return elementText;
	}

	public void setElementText(String elementText) {
		this.elementText = elementText;
	}

	public LinkedHashMap<String, String> getAttrs() {
		return attrs;
	}

	public void addAttr(String attrName, String attrValue) {
		attrs.put(attrName, attrValue);
	}
	
	public Element getElement(Document doc) throws ParserConfigurationException {
		
		if (doc == null) {
			DocumentBuilderFactory dbfac = DocumentBuilderFactory.newInstance();
			DocumentBuilder docBuilder = dbfac.newDocumentBuilder();
			doc = docBuilder.newDocument();
		}
        
        Element actionChild = doc.createElement(elementName);
        
        Iterator<String> attrNames = attrs.keySet().iterator();
        while (attrNames.hasNext()) {
        	String attrName = attrNames.next();
        	String attrValue = attrs.get(attrName);
        	actionChild.setAttribute(attrName, attrValue);
        }
        
        if (elementText != null) {
            Text text = doc.createTextNode(elementText);
            actionChild.appendChild(text);
        }
		
        return actionChild;
		
	}
	
	public static ActionChild createActionParam(String nameAttrValue, String elementText) {
		ActionChild ismElement = new ActionChild("ActionParameter", elementText);
		ismElement.addAttr("name", nameAttrValue);
		return ismElement;
	}
	
	public static ActionChild createISMProperty(String nameAttrValue, String valueAttrValue, String typeAttrValue) {
		ActionChild ismElement = new ActionChild("ImaProperty", null);
		ismElement.addAttr("name", nameAttrValue);
		ismElement.addAttr("value", valueAttrValue);
		ismElement.addAttr("type", typeAttrValue);
		return ismElement;
	}
	
	public void addComment(String comment) {
		comments.add(comment);
	}
	
	public void appendComments(Element parentNode) throws ParserConfigurationException {
		
		Document doc = parentNode.getOwnerDocument();
		
		Iterator<String> commentIterator = comments.iterator();
		while(commentIterator.hasNext()) {
			String commentString = commentIterator.next();
			Comment commentNode = doc.createComment(commentString);
			System.out.println("ADDING COMMENT " + commentString);
			parentNode.appendChild(commentNode);
			
		}
		
	}


}
