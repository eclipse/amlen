package com.ibm.mqst.mqxr.scada;
import java.util.Vector;
/*
 * Copyright (c) 2002-2021 Contributors to the Eclipse Foundation
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
/**
 * @version 	1.0
 */
/* This class represents a list of topics used in UNSUBSCRIBE 
 * messages */

public class TopicList {
	
	Vector topics;
	
	// Constructor - build topic list
	public TopicList(String topic) {
		
		topics = new Vector();
		addTopic(topic);
	}
	
	// Add a topic to list
	public void addTopic(String topic) {
		
		byte[] topicUTF = SCADAutils.StringToUTF(topic);
		
		for (int i = 0; i < topicUTF.length; i++) {
			topics.addElement(new Byte(topicUTF[i]));
			
		}
	}
	
	// Dump Vector to byte array for use in UNSUBSCRIBE msgs
	public byte[] toByteArray() {
		
		byte[] result = new byte[topics.size()];
		
		for (int i = 0; i < result.length; i++) {
				result[i] = ((Byte)(topics.elementAt(i))).byteValue();
		}
		
		return result;
	}
		

}
