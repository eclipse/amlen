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
package com.ibm.ima.mqcon.enabledediting;

import com.ibm.ima.test.cli.config.ImaConfig;
import com.ibm.ima.test.cli.config.ImaMQConnectivity;
import com.ibm.ima.test.cli.imaserver.ImaServerStatus;
import com.ibm.ima.test.cli.mq.MQConfig;

public class EnabledEditConfig extends JmsToJms {
	String prefixName;
	String hubName;
	String cPolicyName;
	String mPolicyName;
	String mPolicyNameQueue;
	String epName;
	
	String mqNameForRules = mqhostname + "(" + mqport + ")";
	
	ImaMQConnectivity mqConn;
	ImaServerStatus status;
	ImaConfig config;
	MQConfig mqConfig;
	
	public EnabledEditConfig(String IMAHostName, int IMAPort, String MQHostName, int MQPort, 
			String MQQUeueManagerName, String IMADestinationName, String MQDestinationName,
			String prefix){
		imahostname = IMAHostName;
		imaport = IMAPort;
		
		mqhostname = MQHostName;
		mqport = MQPort;
		mqqueuemanager = MQQUeueManagerName;
		
		imadestinationName = IMADestinationName;
		mqDestinationName = MQDestinationName;
		
		prefixName = prefix;
		hubName = prefixName + "_HUB";
		cPolicyName = prefixName + "_CP";
		mPolicyName = prefixName + "_MP";
		mPolicyNameQueue = mPolicyName + "_Q";
		//mPolicyName = prefixName + "_MP_MQTT";
		epName = prefixName + "_EP";
		
		mqNameForRules = mqhostname + "(" + mqport + ")";
	}
	
	public void setup(){
		//MAIN SETUP (Common for all tests?)
		try {
			System.out.println("Setting up...");
			

			status = new ImaServerStatus(imahostname, "admin", "admin");
			if(!status.isConnected()){
				System.out.println("Connecting 'ImaServerStatus' session");
				status.connectToServer();
				if(status.isConnected()){
					System.out.println("Connected 'ImaServerStatus' session");
				}
			}
			
			config = new ImaConfig(imahostname, "admin", "admin");
			if(!config.isConnected()){
				System.out.println("Connecting 'ImaConfig' session");
				config.connectToServer();
				if(config.isConnected()){
					System.out.println("Connected 'ImaConfig' session");
				}
			}
			
			mqConn = new ImaMQConnectivity(imahostname, "admin", "admin");
			if(!mqConn.isConnected()){
				System.out.println("Connecting 'ImaMQConnectivity' session");
				mqConn.connectToServer();
				if(mqConn.isConnected()){
					System.out.println("Connected 'ImaMQConnectivity' session");
				}
			}
			
			mqConfig = new MQConfig(mqhostname, "mquser", "yell0f1n", null);
			if(!mqConfig.isConnected()){
				System.out.println("Connecting 'MQConfig' session");
				mqConfig.connectToServer();
				if(mqConfig.isConnected()){
					System.out.println("Connected 'MQConfig' session");
				}
			}
			
			String desc = "Used for testing editing 'Destination Mapping Rules' that are enabled";
			
			if(!config.messageHubExists(hubName)){
				System.out.println("Creating message hub");
				config.createHub(hubName, desc);
				if(config.messageHubExists(hubName)){
					System.out.println("Message hub created");
				}
			}
			
			if(!config.connectionPolicyExists(cPolicyName)){
				System.out.println("Creating connection policy");
				config.createConnectionPolicy(cPolicyName, desc, ImaConfig.TYPE.JMS);
				config.updateConnectionPolicy(cPolicyName, "Protocol", "MQTT,JMS");
				if(config.connectionPolicyExists(cPolicyName)){
					System.out.println("Connection policy created");
				}
			}
			
			if(!config.messagingPolicyExists(mPolicyName)){
				System.out.println("Creating messaging policy");
				config.createMessagingPolicy(mPolicyName, desc, "*", ImaConfig.TYPE.Topic, ImaConfig.TYPE.JMS);
				config.updateMessagingPolicy(mPolicyName, "Protocol", "MQTT,JMS");
				if(config.messagingPolicyExists(mPolicyName)){
					System.out.println("Messaging policy created");
				}
			}
			
			if(!config.messagingPolicyExists(mPolicyNameQueue)){
				System.out.println("Creating messaging policy for Queues");
				config.createMessagingPolicy(mPolicyNameQueue, desc, "*", ImaConfig.TYPE.Queue, ImaConfig.TYPE.JMS);
				if(config.messagingPolicyExists(mPolicyNameQueue)){
					System.out.println("Messaging policy created");
				}
			}
			
			if(!config.endpointExists(epName)){
				System.out.println("Creating endpoint");
				config.createEndpoint(epName, desc, Integer.toString(imaport), ImaConfig.TYPE.JMS, cPolicyName, mPolicyName, hubName);
				config.updateEndpoint(epName, "Protocol", "MQTT,JMS");
				config.updateEndpoint(epName, "MessagingPolicies", mPolicyName+","+mPolicyNameQueue);
				if(config.endpointExists(epName)){
					System.out.println("Endpoint created");
				}
			}
			
			if(!mqConn.queueManagerConnectionExists(prefixName)){
				System.out.println("Creating queue manager connection");
				mqConn.createQueueManagerConnection(prefixName, mqqueuemanager, mqNameForRules, "SYSTEM.ADMIN.SVRCONN");			
				if(mqConn.queueManagerConnectionExists(prefixName)){
					System.out.println("Queue manager created");
				}
			}
		
		} catch (Exception e) {
			System.out.println("Test failed during setup");
			e.printStackTrace();
		}
		//End main setup
	}
	
	public void teardown(){
		//Tear down
		try {
			System.out.println("Tearing down...");
			
			if(config.endpointExists(epName)){
				System.out.println("Deleting endpoint");
				config.deleteEndpoint(epName);
				if(!config.endpointExists(epName)){
					System.out.println("Endpoint deleted");
				}
			}
			
			if(config.messagingPolicyExists(mPolicyName)){
				System.out.println("Deleting messaging policy");
				config.deleteMessagingPolicy(mPolicyName);
				if(!config.messagingPolicyExists(mPolicyName)){
					System.out.println("Messaging policy deleted");
				}
			}
			
			if(config.messagingPolicyExists(mPolicyNameQueue)){
				System.out.println("Deleting messaging policy");
				config.deleteMessagingPolicy(mPolicyNameQueue);
				if(!config.messagingPolicyExists(mPolicyNameQueue)){
					System.out.println("Messaging policy deleted");
				}
			}
			
			if(config.connectionPolicyExists(cPolicyName)){
				System.out.println("Deleting connection policy");
				config.deleteConnectionPolicy(cPolicyName);
				if(!config.connectionPolicyExists(cPolicyName)){
					System.out.println("Connection policy deleted");
				}
			}
			
			if(config.messageHubExists(hubName)){
				System.out.println("Deleting message hub policy");
				config.deleteMessageHub(hubName);
				if(!config.messageHubExists(hubName)){
					System.out.println("Message hub deleted");
				}
			}
			
			if(mqConn.queueManagerConnectionExists(prefixName)){
				System.out.println("Deleting queue manager connection");
				mqConn.deleteQueueManagerConnection(prefixName);
				if(!mqConn.queueManagerConnectionExists(prefixName)){
					System.out.println("Queue manager connection deleted");
				}
			}
			
			if(status.isConnected()){
				System.out.println("Disconnecting 'ImaServerStatus' session");
				status.disconnectServer();
				if(!status.isConnected()){
					System.out.println("'ImaServerStatus' session disconnected");
				}
			}
			
			if(config.isConnected()){
				System.out.println("Disconnecting 'ImaConfig' session");
				config.disconnectServer();
				if(!config.isConnected()){
					System.out.println("'ImaConfig' session disconnected");
				}
			}
			
			if(mqConn.isConnected()){
				System.out.println("Disconnecting 'ImaMQConnectivity' session");
				mqConn.disconnectServer();
				if(!mqConn.isConnected()){
					System.out.println("'ImaMQConnectivity' session disconnected");
				}
			}
			
			if(mqConfig.isConnected()){
				System.out.println("Disconnecting 'MQConfig' session");
				mqConfig.disconnectServer();
				if(!mqConfig.isConnected()){
					System.out.println("'MQConfig' session disconnected");
				}
			}
			
			System.out.println("Teardown complete.");
		} catch (Exception e) {
			// TODO Auto-generated catch block
			System.out.println("Test failed during teardown");
			e.printStackTrace();
		}
	}
}
