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
package com.ibm.ism.svt.tasks;

import com.ibm.automation.core.loggers.VisualReporter;
import com.ibm.automation.core.web.WebBrowser;
import com.ibm.ism.svt.application.SubscriberObjects;
import com.ibm.ism.svt.common.IsmSvtConstants;
import com.ibm.ism.svt.common.IsmSvtTest;

/*
 * Task to be driven off the ISM Sample Application web page:  ism_sample_sub.html 
 */
public class SubscriberTasks implements IsmSvtConstants  {

	/*
	 * verifyPageLoaded
	 */
	public static boolean verifyPageLoaded() {
		WebBrowser.waitForReady();
		VisualReporter.logInfo("Subscriber URL Loading...");
		return WebBrowser.isTextPresent("ISM WebSocket Sample");
	}
	
/*
 * subscribe() - Perform the Connection and Subscribe action of ISM Sample
 */
	public static boolean subscribe(boolean wss, String uid, String password, String ip, String port, String clientId, boolean cleanSession, String msgTopic, String subQoS ){
		// WSS  Check-box
		SubscriberObjects.getCheckBox_ismWSS().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		if (  wss ){
			SubscriberObjects.getCheckBox_ismWSS().check();					
		} else {
			SubscriberObjects.getCheckBox_ismWSS().uncheck();				
		}
		
		// userid
		SubscriberObjects.getTextEntry_ismUID().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextEntry_ismUID().setText( uid );
		
		// password
		SubscriberObjects.getTextEntry_ismPassword().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextEntry_ismPassword().setPassword( password );
		
		// ISM Server IP address
		SubscriberObjects.getTextEntry_ismIP().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextEntry_ismIP().setText( ip );

		// ISM Server IP Port
		SubscriberObjects.getTextEntry_ismPort().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextEntry_ismPort().setText( port );

		// MQTT Client Id
		SubscriberObjects.getTextEntry_ismClientId().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextEntry_ismClientId().setText( clientId );

		// Clean Session (Check-box)
		SubscriberObjects.getCheckBox_ismCleanSession().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		if ( cleanSession  ){
			SubscriberObjects.getCheckBox_ismCleanSession().check();				
		} else {
			SubscriberObjects.getCheckBox_ismCleanSession().uncheck();				
		}

		// msgTopic
		SubscriberObjects.getTextEntry_msgTopic().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextEntry_msgTopic().setText( msgTopic );

		// SubQoS  (Select Object)
		SubscriberObjects.getSelect_subQoS().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getSelect_subQoS().select( subQoS );

		// Push the Connect & Subscribe Button
		SubscriberObjects.getButton_subConnect().focus();
		IsmSvtTest.delay(SVT_DELAY);
		SubscriberObjects.getButton_subConnect().click();

	
		IsmSvtTest.waitForBrowserReady();

		/*
		 * validateConnection will check for a successful subscription
		 */
		
		return validateConnection( clientId );
		
	}
	
	/*
	 * unsubscribe() - Performs the Unsubscribe and Disconnect function of ISM Sample
	 */
	public static boolean unsubscribe(boolean wss, String uid, String password, String ip, String port, String clientId, boolean cleanSession, String msgTopic, String subQoS ){
		SubscriberObjects.getButton_subDisconnect().focus();
		SubscriberObjects.getButton_subDisconnect().click();
		
		/*
		 * validateUnsubscribe will check for a successful disconnect
		 */
		return validateUnsubscribe();
		
	}
	
	/*
	 * validateConnection()
	 */
	public static boolean validateConnection ( String clientId) {

		/*		
		Subscribed to topic: topicA.
		 */
		SubscriberObjects.getTextArea_messages().setFocus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		String theText = SubscriberObjects.getTextArea_messages().getText();
		
		//Does it match expected Text?
		if ( theText.contains( SVT_CONNECTION+clientId ) ) { 	
			return true;
		} else {
			return false;
		}
	}

	/*
	 * validateLostConnection()
	 */
	public static boolean validateLostConnection () {

		/*		
		Subscriber connection lost for client ${clientId}.
		 */
		SubscriberObjects.getTextArea_messages().focus();
		SubscriberObjects.getTextArea_messages().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		String theText = SubscriberObjects.getTextArea_messages().getText();
		
		//Does it match expected Text?
		if ( theText.contains( SVT_LOSTCONNECTION ) ) { 	
			return true;
		} else {
			return false;
		}
	}

	/*
	 * validateSubscribe()
	 */
	public static boolean validateMessagesReceived () {

		/*		
		Message 4 received on topic 'topicA': Sample Message
		Message msgCount received on topic 'msgTopic': msgText
		 */
		SubscriberObjects.getTextArea_messages().focus();
		SubscriberObjects.getTextArea_messages().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		String theText = SubscriberObjects.getTextArea_messages().getText();
		
		//Does it match expected Text?
		if ( theText.equals( SVT_SUBSCRIPTION ) ) { 	
			return true;
		} else {
			return false;
		}
	}

	/*
	 * validateUnsubscribe()
	 */
	public static boolean validateUnsubscribe () {

		/*		
		Subscriber disconnected.
		 */
		SubscriberObjects.getTextArea_messages().focus();
		SubscriberObjects.getTextArea_messages().click();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		String theText = SubscriberObjects.getTextArea_messages().getText();
		
		//Does it match expected Text?
		if ( theText.equals( SVT_DISCONNECT ) ) { 	
			return true;
		} else {
			return false;
		}
	}

	/*
	 * clearMessagesTextArea()
	 */
	public static boolean clearMessagesTextArea () {

		SubscriberObjects.getTextArea_messages().focus();
		IsmSvtTest.delay(STATIC_WAIT_TIME);
		SubscriberObjects.getTextArea_messages().clearText();
		
		return true;
	}
}
