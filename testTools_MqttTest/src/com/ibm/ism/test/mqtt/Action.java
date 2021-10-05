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
package com.ibm.ism.test.mqtt;

import java.util.Map;

public class Action {
	
	private Map<String,String> _config;
	private Map<String,String> _parameters;	
	
	public Action(Map<String,String> config, Map<String,String> parameters) {
		_config=config;
		_parameters=parameters;
	}
	
	public void setConfig(Map<String,String> config) {
		_config=config;
	}
	
	public void setParameters(Map<String,String> parameters) {
		_parameters=parameters;
	}
	
	public Map<String,String> getConfig() {
		return _config;
	}
	
	public Map<String,String> getParameters() {
		return _parameters;
	}	
	
	public String toString() {
		return "Config: " + _config.toString() + " Parameters: " + _parameters.toString();
	}

}
