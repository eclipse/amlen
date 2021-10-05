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
package svt.framework.transports;

import svt.framework.messages.SVT_BaseEvent;
import svt.framework.scale.transports.SVT_Transport_Thread;
import svt.util.SVT_Params;


public abstract class SVT_Transport {
	SVT_Params parms;
	SVT_Transport_Thread myThread;
	String listenTopic; 
	

	public SVT_Transport(SVT_Params parms, SVT_Transport_Thread threadOwner) {
		this.parms = parms;
		this.myThread=threadOwner;
	}
	
	public abstract void setListenerTopic(String lstrTopic);
	public abstract boolean sendMessage(SVT_BaseEvent event);
	public abstract void addListenerTopic(String topic); 

}
