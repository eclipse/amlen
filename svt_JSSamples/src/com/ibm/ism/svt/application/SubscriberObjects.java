// Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package com.ibm.ism.svt.application;

//import com.ibm.automation.core.web.widgets.selenium.*;
import com.ibm.automation.core.web.widgets.WebButton;
import com.ibm.automation.core.web.widgets.WebCheckBox;
import com.ibm.automation.core.web.widgets.WebComboBox;
import com.ibm.automation.core.web.widgets.WebTextField;
//(Button?)import com.ibm.automation.core.web.widgets.selenium.SeleniumTestObject;
import com.ibm.automation.core.web.widgets.selenium.SeleniumTextTestObject;;

public class SubscriberObjects {

	//  Secure Socket Layer -- CheckBox, Selenium WebCheckBox --
	public static WebCheckBox getCheckBox_ismWSS() {
		return new WebCheckBox("//input[@id='wss']");
	};
	// ISM User ID in Connection Options
	public static SeleniumTextTestObject getTextEntry_ismUID() {
		return new SeleniumTextTestObject("//input[@id='ismUID']");
	};
	// ISM User Password in Connection Options  -- WebTextField allows setPassword() --
	public static WebTextField getTextEntry_ismPassword() {
		return new WebTextField("//input[@id='ismPassword']");
	};
	// ISM Server IP address
	public static SeleniumTextTestObject getTextEntry_ismIP() {
		return new SeleniumTextTestObject("//input[@id='ismIP']");
	};
	// ISM Server port
	public static SeleniumTextTestObject getTextEntry_ismPort() {
		return new SeleniumTextTestObject("//input[@id='ismPort']");
	};
	//  MQTT Client Id
	public static SeleniumTextTestObject getTextEntry_ismClientId() {
		return new SeleniumTextTestObject("//input[@id='ismClientId']");
	};
	// MQTT Clean Session on restart  -- CheckBox, Selenium WebCheckBox --
	public static WebCheckBox getCheckBox_ismCleanSession() {
		return new WebCheckBox("//input[@id='ismCleanSession']");
	};
	// Message Topic Name
	public static SeleniumTextTestObject getTextEntry_msgTopic() {
		return new SeleniumTextTestObject("//input[@id='msgTopic']");
	};
	//  Subscribe Quality of Service -- HTML Select, Selenium WebComboBox --  
	public static WebComboBox getSelect_subQoS() {
		return new WebComboBox("//select[@id='subQoS']");
	};
	//  Connect and Publish Button  -- HTML Button, Selenium WebButton --
	public static WebButton getButton_subConnect() {
		return new WebButton("//button[@id='subConnect']");
	};
	//  Unsubscribe and Disconnect Button  -- HTML Button, Selenium WebButton --
	public static WebButton getButton_subDisconnect() {
		return new WebButton("//button[@id='subDisconnect']");
	};
	//  Messages text area  -- HTML TextArea, Selenium WebTextField --
	public static WebTextField getTextArea_messages() {
		return new WebTextField("//textarea[@id='messages']");
	};

}

