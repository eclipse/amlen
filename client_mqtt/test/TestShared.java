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


public class TestShared {
	int qos = 0;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// TODO Auto-generated method stub
		String  server;
		String  port;
		int     qos;
		boolean stopearly = false;
		boolean c1clean = false;
		boolean c2clean = false;
		boolean v5shared = true;
		int     delay = 1000;
		
		if (args.length > 0)
		    server = args[0];
		else 
		    server = "10.10.0.5";
		if (args.length > 1)
		    port = args[1];
		else
		    port = "16109";
		if (args.length > 2)
		    qos = Integer.valueOf(args[2]);
		else
		    qos = 0;
		
    	String [] sharedTopic = new String[3];
    	String shared = v5shared ? "$share" : "$SharedSubscription";
    	sharedTopic[0] = new String(shared + "/subSharedDurable1/RequestT");
    	sharedTopic[1] = new String(shared + "/subSharedDurable2/RequestT2");
    	sharedTopic[2] = new String(shared + "/subSharedDurable3/RequestT3");
    	int [] sharedqos = new int[3];
    	sharedqos[0] = qos;
    	sharedqos[1] = qos;
    	sharedqos[2] = qos;
    	

    	IsmMqttConnection connSharedDurable1 = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "sharedDurableClient1");
    	connSharedDurable1.setEncoding(IsmMqttConnection.BINARY);
    	connSharedDurable1.setVerbose(true);
    	connSharedDurable1.setClean(c1clean);
    	
    	IsmMqttConnection connSharedDurable2 = new IsmMqttConnection(server, Integer.parseInt(port), "/mqtt", "sharedDurableClient2");
    	connSharedDurable2.setEncoding(IsmMqttConnection.BINARY);
    	connSharedDurable2.setVerbose(true);
    	connSharedDurable2.setClean(c2clean);
    	try {
    	connSharedDurable1.connect();
    	connSharedDurable2.connect();
    	System.out.println("Subscribing...");

    	connSharedDurable1.subscribe(sharedTopic, sharedqos);
        connSharedDurable1.subscribe(sharedTopic, sharedqos);
    	System.out.println("Subscribed...");
    	// For manual test - check UI or CLI for subscription/consumer stats
    	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	
    	System.out.println("Subscribing again...");

    	connSharedDurable2.subscribe(sharedTopic, sharedqos);
    	System.out.println("Subscribed...");
    	// For manual test - check UI or CLI for subscription/consumer stats
    	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	} catch (Exception ex) {
    		System.err.println("error creating sharedDurableClient1 or creating subscrition $SharedSubscription/subSharedDurable1/RequestT");
    		ex.printStackTrace();
    	}
    	
    	System.out.println("Disconnecting...");
    	connSharedDurable1.disconnect();
    	// For manual test - check UI or CLI for subscription/consumer stats
    	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	System.out.println("Disconnecting...");
    	connSharedDurable2.disconnect();
    	
    	System.out.println("Disconnected...");
    	// For manual test - check UI or CLI for subscription/consumer stats
    	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	
    	connSharedDurable1.setEncoding(IsmMqttConnection.BINARY);
    	connSharedDurable1.setVerbose(true);
    	connSharedDurable1.setClean(stopearly|c1clean);
    	
    	connSharedDurable2.setEncoding(IsmMqttConnection.BINARY);
    	connSharedDurable2.setVerbose(true);
    	connSharedDurable2.setClean(stopearly||c2clean);
    	try {
    		System.out.println("Reconnecting");
        	connSharedDurable1.connect();
        	connSharedDurable2.connect();
        	System.out.println("Reconnected...");
        	
        	if (stopearly) {
        	    connSharedDurable2.disconnect();       
        	    connSharedDurable1.disconnect();
        	    System.exit(3);
        	}
        	
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
        	System.out.println("Resubscribing");
        	connSharedDurable1.subscribe(sharedTopic, sharedqos);       	
        	System.out.println("Resubscribed");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
        	
        	System.out.println("Resubscribing again");
        	connSharedDurable2.subscribe(sharedTopic, sharedqos);
        	System.out.println("Resubscribed");
        	// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	} catch (Exception ex) {
    		System.err.println("error creating sharedDurableClient1 or creating subscrition $SharedSubscription/subSharedDurable1/RequestT");
    		ex.printStackTrace();
    	}
    	
    	try {
        	System.out.println("Unsubscribing");
    		connSharedDurable1.unsubscribeMulti(sharedTopic);
    		System.out.println("Unsubscribed");
    		// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    		
    		System.out.println("Unsubscribing again");
    		connSharedDurable2.unsubscribeMulti(sharedTopic);
    		System.out.println("Unsubscribed");
    		// For manual test - check UI or CLI for subscription/consumer stats
        	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	} catch (Exception ex) {
    		System.err.println("Unsubscribe failed for $SharedSubscription/subSharedDurable1/RequestT");
    		ex.printStackTrace();
    	}
    	System.out.println("Disconnecting");
    	connSharedDurable1.disconnect();
    	// For manual test - check UI or CLI for subscription/consumer stats
    	try { Thread.sleep(delay); } catch (InterruptedException ex) { }
    	System.out.println("Disconnecting");
    	connSharedDurable2.disconnect();
    	
    	System.out.println("Disconnected...");
	}

}
