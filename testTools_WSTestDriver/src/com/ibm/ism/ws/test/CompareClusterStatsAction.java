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
package com.ibm.ism.ws.test;

import java.io.IOException;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import org.apache.wink.client.ClientResponse;
import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TestUtils;
import com.ibm.ism.ws.test.TestUtils.Property;
import com.ibm.ism.ws.test.TrWriter.LogLevel;
import com.ibm.json.java.JSONArray;
import com.ibm.json.java.JSONObject;

/**
 * 
 * 
 * Properties:
 * 
 * 	structure_id - structure containing the output from monitor ClusterMembership RESTAPI
 * 	serveruid -   structure ID containing output from GetServerUID action
 * 	ObjectProperty name="Property" value="Value"
 * 
 *  If ObjectProperty name begins with "Q0:", property is in Q0 subobject
 *  If ObjectProperty name begins with "Q1:", property is in Q1 subobject
 *  
 * GetServerUID Action stores server uid into 'serveruidObject'
 * RestAPI Action stores output into 'clusterstats'
 * 
 * <Action name="CompareClusterStats" type="CompareClusterStats">
 * 	<ActionParameter name="structure_id">clusterstats</ActionParameter>
 * 	<ActionParameter name="serveruid">serveruidObject</ActionParameter>
 * 	<ObjectProperty name="ServerName" value="cluster2"/>
 * 	<ObjectProperty name="Unreliable:BufferedMessages" value="20"/>
 * 	<ObjectProperty name="Reliable:BufferedMessages" value="5"/>
 * </Action>
 * 
 * TODO: too many properties to make this work.........
 * 		response top level object may contain an array, an object map, or it may be
 * 		the object we want to actually verify the properties of.
 *
 * Validates as many properties as you like of a single element 
 * of the JSON Array or JSON Object map specifed by topLevelKey.
 * 
 * Error codes:
 * ISMTEST3410 - Object to compare not found
 * ISMTEST3411 - Unexpected JSON
 * ISMTEST3412 - Property not found
 * ISMTEST3413 - Property does not match expected value
 * ISMTEST3414 - Unexpected exception
 * ISMTEST3415 - Top level object does not exist
 * ISMTEST3416 - Top level object is empty
 * ISMTEST3417 - No matches found for property 
 *
 */
public class CompareClusterStatsAction extends ApiCallAction {
	private final String _structureID;
	private final String _serverUID;
	private final String _serverName;
	private final LinkedList<Property> _props = new LinkedList<Property>();
	
	public CompareClusterStatsAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		
		_structureID = _actionParams.getProperty("structureID");
		if (_structureID == null || _structureID.equals("")) {
			throw new RuntimeException("structureID is not defined for "
					+ this.toString());
		}
		
		_serverUID = _actionParams.getProperty("serverUID");
		
		_serverName = _actionParams.getProperty("serverName");
		
		if ((_serverName == null || _serverName.equals("")) 
				&& (_serverUID == null || _serverUID.equals(""))) {
			throw new RuntimeException("serverName or serverUID must be defined for "
					+ this.toString());
		}
		
		TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
            public void process(Element element) {
                String name = element.getAttribute("name");
                if (name == null) {
                    throw new RuntimeException(
                            "Name is not specified for ObjectProperty. "
                                    + TestUtils.xmlToString(element, true));
                }
                String value = element.getAttribute("value");
                if (value == null) {
                    throw new RuntimeException(
                            "Value is not specified for ObjectProperty " + name
                                    + " ."
                                    + TestUtils.xmlToString(element, true));
                }

                Property p = new Property(name,value,getConfigAttribute(element, "type", "STRING"));
                _props.addLast(p);
            }
        };
        TestUtils.walkXmlElements(config, "ObjectProperty", processor);
	}
	
	/**
	 * 
	 */
	protected boolean invokeApi() throws IsmTestException {
		ClientResponse response = (ClientResponse) _dataRepository.getVar(_structureID);
		String entity = response.getEntity(String.class);
		JSONObject json = null;
		int objectFound = 0;
		
		_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPARERESTAPI+1),
				"{0}: Response.entity: {1}", _id, entity);
		
		if (response.getEntity(String.class) == null) {
			throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+18), 
					"ClientResponse Entity is null");
		}
		
		try {
			json = JSONObject.parse(entity);
			
			if (json.containsKey("Cluster")) {
				Object topLevelObject = json.get("Cluster");
				if (topLevelObject instanceof JSONArray) {
					objectFound = handleJSONArray(objectFound, topLevelObject);
				} else {
					// Top Level Object isn't JSONObject or JSONArray... what?!
					throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+11),
							"Unexpected JSON");
				}
			} else {
				// Top Level Object missing
				throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+15),
						"Top Level Object does not exist - Cluster");
			}
		} catch (IOException e) {
			e.printStackTrace();
			throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+14),
					"Unexpected Exception", e);
		}
		
		if (objectFound == 0) {
			throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+10),
					"Object to compare was not found.");
		}
		
		return true;
	}

	/**
	 * Top Level Object that was specified is a JSON Array
	 * @param objectFound
	 * @param topLevelObject
	 * @return
	 * @throws IsmTestException
	 */
	private int handleJSONArray(int objectFound, Object topLevelObject)
			throws IsmTestException {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPARERESTAPI+2), 
				"{0}: topLevelObject is JSONArray: {1}", _id, ((JSONArray) topLevelObject).toString());
		
		JSONArray jsonArray = (JSONArray) topLevelObject;
		String serverUID = null;
		if (_serverUID != null && !_serverUID.equals("")) {
			String dataVar = (String) _dataRepository.getVar(_serverUID);
			if (dataVar != null && !dataVar.equals("")) {
				serverUID = dataVar;
			} else {
				try {
					serverUID = EnvVars.replace(_serverUID);
				} catch (Exception e) {
					// TODO Auto-generated catch block
					e.printStackTrace();
				}
			}
		}
		
		String serverName = null;
		if (_serverName != null && !_serverName.equals("")) {
			try {
				serverName = EnvVars.replace(_serverName);
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
		
		if (jsonArray.isEmpty()) {
			// Handle empty array
			throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+16),
					"Top Level Object is empty");
		}
		
		// Iterate over the JSONArray looking for the object we want to compare
		Iterator<?> arrayIter = jsonArray.iterator();
		while (arrayIter.hasNext()) {
			JSONObject subObject = (JSONObject) arrayIter.next();
			_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPARERESTAPI+3), 
					"{0}: subObject is JSONObject: {1}", _id, subObject.toString());
			
			if (serverUID != null) {
				if (subObject.get("ServerUID").equals(serverUID)) {
					objectFound = 1;
					validateProperties(subObject);
				} else {
					// Not the object we want to verify. Skip.
				}
			} else if (serverName != null) {
				if (subObject.get("ServerName").equals(serverName)) {
					objectFound = 1;
					validateProperties(subObject);
				} else {
					
				}
			}
		}
		return objectFound;
	}

	/**
	 * Validate the properties of the object specified by subObjectName
	 * @param mySubObject
	 * @throws IsmTestException
	 */
	private void validateProperties(JSONObject mySubObject)
			throws IsmTestException {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.COMPARERESTAPI+3), 
				"{0}: Validating properties of JSONObject: {1}", _id, mySubObject.toString());
		// Go through the list of properties we want to compare,
		// and compare them to the properties of the object.
		for (Property p : _props) {
			String propName = p.getName();
			String prefix = "";
			if (propName.length() > 9 && propName.substring(0,9).equals("Reliable:")) {
				prefix = "Reliable";
				validateForwarderChannelProperty(mySubObject, p, propName,
						prefix);
			} else if (propName.length() > 11 && propName.substring(0,11).equals("Unreliable:")) {
				prefix = "Unreliable";
				validateForwarderChannelProperty(mySubObject, p, propName,
						prefix);
			} else if (mySubObject.containsKey(propName)) {
				validateProperty(p, propName, propName, mySubObject);
			} else {
				// Object does not contain the property we want to verify
				throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+12), 
						"Object does not contain the property: "+propName);
			}
		}
	}

	private void validateForwarderChannelProperty(JSONObject mySubObject,
			Property p, String propName, String prefix) throws IsmTestException {
		String subPropName = propName.substring(prefix.length()+1);
		System.out.println(subPropName);
		JSONObject mySubChildObject = (JSONObject) mySubObject.get(prefix);
		
		validateProperty(p, propName, subPropName, mySubChildObject);
	}

	private void validateProperty(Property p, String propName, String subPropName,
			JSONObject mySubChildObject) throws IsmTestException {
		String propValue = (String) p.getValue(_dataRepository, _trWriter).toString();
		String mySubChildPropObject = (String) mySubChildObject.get(subPropName).toString();
		
		// If propValue is "REGEXP: ..."
		if (propValue.toLowerCase().contains("REGEXP: ".toLowerCase())) {
			String regexStr = propValue.substring(8);
			Pattern pattern = Pattern.compile(regexStr);
			Matcher matcher = pattern.matcher(mySubChildPropObject);
			boolean found = false;
			while(matcher.find()) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.COMPARERESTAPI+5),
						"{0}: Match found for property={1} regex={2} value={3}", _id, propName, regexStr, mySubChildPropObject);
				found = true;
			}
			if (!found) {
				throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+17),
						"No matches found for property="+propName+" regex="+regexStr+" value="+mySubChildPropObject);
			}
		} else if (mySubChildPropObject.equals(propValue)) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST"+(Constants.COMPARERESTAPI+4), 
					"{0}: Match found for property={1} expected={2} actual={3}", _id, propName, propValue, mySubChildPropObject);
		} else {
			// TODO: Handle mismatch
			throw new IsmTestException("ISMTEST"+(Constants.COMPARERESTAPI+13), 
					"No match found for property="+propName+" expected="+propValue+" actual="+mySubChildPropObject);
		}
	}
}
