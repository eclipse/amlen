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

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Text;

public class SysClient {
	
	private String include = null;
	private String portInclude = null;
	private String solution = null;
	
	
	public SysClient(String include, String port, String solution) {
		setInclude(include);
		setPortInclude(port);
		setSolution(solution);
	}
	
	public Element getElement (Document doc) throws ParserConfigurationException {
		
		if (doc == null) {
		  DocumentBuilderFactory dbfac = DocumentBuilderFactory.newInstance();
          DocumentBuilder docBuilder = dbfac.newDocumentBuilder();
          doc = docBuilder.newDocument();
		}

          Element syncClient = doc.createElement("SyncClient");
          doc.appendChild(syncClient);
          
          Element server = doc.createElement("server");
          syncClient.appendChild(server);
          
          Element include = doc.createElement("include");
          Text includeText = doc.createTextNode(getInclude());
          include.appendChild(includeText);
          server.appendChild(include);
          
          Element port = doc.createElement("include");
          Text portText = doc.createTextNode(getPortInclude());
          port.appendChild(portText);
          server.appendChild(port);

          Element solution = doc.createElement("solution");
          Text solutionText = doc.createTextNode(getSolution());
          solution.appendChild(solutionText);
          syncClient.appendChild(solution);
        
          return syncClient;
          
	}


	public String getInclude() {
		return include;
	}


	public void setInclude(String include) {
		this.include = include;
	}


	public String getPortInclude() {
		return portInclude;
	}


	public void setPortInclude(String port) {
		this.portInclude = port;
	}


	public String getSolution() {
		return solution;
	}


	public void setSolution(String solution) {
		this.solution = solution;
	}

}
