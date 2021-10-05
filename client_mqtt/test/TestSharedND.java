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
import com.ibm.ism.mqtt.IsmMqttConnection;


public class TestSharedND {
	int qos = 0;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		String server = args[0];
		String port = args[1];
		int qos = Integer.valueOf(args[2]);
		
    	String [] sharedndTopic = new String[3];
    	sharedndTopic[0] = new String("$SharedSubscription/subSharedND1/RequestT");
    	sharedndTopic[1] = new String("$SharedSubscription/subSharedND2/RequestT2");
    	sharedndTopic[2] = new String("$SharedSubscription/subSharedND3/RequestT3");
    	int [] sharedndqos = new int[3];
    	sharedndqos[0] = qos;
    	sharedndqos[1] = qos;
    	sharedndqos[2] = qos;
		
    	IsmMqttConnection connSharedND1 = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "sharedNDClient1");
    	connSharedND1.setEncoding(IsmMqttConnection.BINARY);
    	connSharedND1.setVerbose(true);
    	IsmMqttConnection connSharedND2 = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "sharedNDClient2");
    	connSharedND2.setEncoding(IsmMqttConnection.BINARY);
    	connSharedND2.setVerbose(true);
    	try {
        	connSharedND1.connect();
        	connSharedND2.connect();
        	
        	System.out.println("Subscribing...");
        	connSharedND1.subscribe(sharedndTopic, sharedndqos);
        	System.out.println("Subscribed...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
        	
        	System.out.println("Subscribing again...");        	
        	connSharedND2.subscribe(sharedndTopic, sharedndqos);
        	System.out.println("Subscribed...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
    	} catch (Exception ex) {
    		System.err.println("error creating connSharedND1 or creating subscrition $SharedSubscription/subSharedND1/RequestT");
    		ex.printStackTrace();
    	}
    	
    	try { Thread.sleep(5000); } catch (InterruptedException ex) { }
    	
    	System.out.println("Disconnecting...");
    	connSharedND1.disconnect();
    	connSharedND2.disconnect();    	
    	System.out.println("Disconnected...");
    	// For manual test - check UI or CLI for subscription/consumer stats
    	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
    	
    	connSharedND1.setEncoding(IsmMqttConnection.BINARY);
    	connSharedND1.setVerbose(true);
    	
    	connSharedND2.setEncoding(IsmMqttConnection.BINARY);
    	connSharedND2.setVerbose(true);
    	try {
        	connSharedND1.connect();
        	connSharedND2.connect();
        	System.out.println("Reconnected...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
        	
        	System.out.println("Subscribing...");
        	connSharedND1.subscribe(sharedndTopic, sharedndqos);
        	System.out.println("Subscribed...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
        	
        	System.out.println("Subscribing again...");
        	connSharedND2.subscribe(sharedndTopic, sharedndqos);
        	System.out.println("Subscribed...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
    	} catch (Exception ex) {
    		System.err.println("error creating connSharedND1 or creating subscrition $SharedSubscription/subSharedND1/RequestT");
    		ex.printStackTrace();
    	}
    	
    	try {
    		System.out.println("Unsubscribing...");
        	connSharedND1.unsubscribeMulti(sharedndTopic);      
        	System.out.println("Unsubscribed...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
        	
        	System.out.println("Unsubscribing again...");
        	connSharedND2.unsubscribeMulti(sharedndTopic);
        	System.out.println("Unsubscribed...");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(10000); } catch (InterruptedException ex) { }
    	} catch (Exception ex) {
    		System.err.println("Unsubscribe failed for $SharedSubscription/subSharedND1/RequestT");
    		ex.printStackTrace();
    	}
    	System.out.println("Disconnecting...");
    	connSharedND1.disconnect();
    	connSharedND2.disconnect();   	
    	System.out.println("Disconnected...");
	}

}
