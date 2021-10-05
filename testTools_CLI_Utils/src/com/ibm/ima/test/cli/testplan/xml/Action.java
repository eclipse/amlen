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

public class Action {

	
	private List<Action>  actionList = new ArrayList<Action>();
	private List<ActionChild> children = new ArrayList<ActionChild>();
	private List<String> comments = new ArrayList<String>();
	private LinkedHashMap<String, String> attrs = new LinkedHashMap<String, String>();

	public Action(String nameAttrValue, String typeAttrValue) {
		addAttr("name", nameAttrValue);
		addAttr("type", typeAttrValue);
		
	}
	
	public Element getElement(Document doc) throws ParserConfigurationException {
		
		if (doc == null) {
			DocumentBuilderFactory dbfac = DocumentBuilderFactory.newInstance();
			DocumentBuilder docBuilder = dbfac.newDocumentBuilder();
			doc = docBuilder.newDocument();
		}
		
		Element actionRoot = doc.createElement("Action");    
        
        Iterator<String> attrNames = attrs.keySet().iterator();
        while (attrNames.hasNext()) {
        	String attrName = attrNames.next();
        	String attrValue = attrs.get(attrName);
        	actionRoot.setAttribute(attrName, attrValue);
        }

        Iterator<Action> actions = actionList.iterator();
        while (actions.hasNext()) {
        	Action action = actions.next();
  
        	action.appendComments(actionRoot);
        	actionRoot.appendChild(action.getElement(doc));
        }
        
        Iterator<ActionChild> actionChildren = children.iterator();
        while (actionChildren.hasNext()) {
        	ActionChild child = actionChildren.next();
        	child.appendComments(actionRoot);
        	actionRoot.appendChild(child.getElement(doc));
        }
        
        return actionRoot;
	}
	
	public void appendComments(Element parentNode) throws ParserConfigurationException {
		
		Document doc = parentNode.getOwnerDocument();
		
		Iterator<String> commentIterator = comments.iterator();
		while(commentIterator.hasNext()) {
			String commentString = commentIterator.next();
			Comment commentNode = doc.createComment(commentString);
			parentNode.appendChild(commentNode);
			
		}
		
	}
	
	public void addNameAttr(String value) {
		attrs.put("name", value);
	}
	
	public void addTypeAttr(String value) {
		attrs.put("type", value);
	}
	
	public List<Action> getActionList() {
		return actionList;
	}

	public void setActionList(List<Action> actionList) {
		this.actionList = actionList;
	}

	public List<ActionChild> getChildren() {
		return children;
	}

	public void addActionChild(ActionChild child) {
		this.children.add(child);
	}
	
	public void addAction(Action action) {
		this.actionList.add(action);
	}

	public LinkedHashMap<String, String> getAttrs() {
		return attrs;
	}

	public void setAttrs(LinkedHashMap<String, String> attrs) {
		this.attrs = attrs;
	}

	public void addAttr(String attrName, String attrValue) {
		attrs.put(attrName, attrValue);
	}
	
	public void addComment(String comment) {
		comments.add(comment);
	}
	

}
