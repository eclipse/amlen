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

package com.ibm.ima.test.stat.validate;

import org.supercsv.cellprocessor.ift.CellProcessor;



public class QueueStatBean implements Comparable<QueueStatBean> {

	private String queueName = ""; /* Keeping for backward compatibility */
	private String name = "";
	private String producers = "";
	private String consumers = "";
	private String bufferedMsgs = "";
	private String bufferedMsgsHWM = "";
	private String bufferedPercent = "";
	private String maxMessages = "";
	private String producedMsgs = "";
	private String consumedMsgs = "";
	private String rejectedMsgs = "";
	private String bufferedHWMPercent = "";
	private String expiredMsgs = "";


/*	public static final String[] header = new String[] { 
		"Name", // Previously QueueName
		"Producers",
		"Consumers",
		"BufferedMsgs",
		"BufferedMsgsHWM",
		"BufferedPercent",
		"MaxMessages",
		"ProducedMsgs",
		"ConsumedMsgs",
		"RejectedMsgs"
		"BufferedHWMPercent"
		"ExpiredMsgs"
	};*/

	public static final CellProcessor[] queueStatProcessor = new CellProcessor[] {
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null,
		null
	};
	
	public boolean equals(Object expected) {
		
		if (expected == null) {
			System.out.println("Entire stat entry line was empty.");
			return false;
		}
		
		
		if ( this == expected ) return true;
		
		 if ( !(expected instanceof QueueStatBean) ) return false;
		
		 QueueStatBean expectedBean = (QueueStatBean) expected;

		
		boolean areEqual = true;
		
		if (queueName.equals(expectedBean.getQueueName()) == false) {
			System.out.println("QueueName not equal. Expected <" +expectedBean.getQueueName() + "> " + "received <" + queueName +">");
			areEqual = false;
		}
		
		if (name.equals(expectedBean.getName()) == false) {
			System.out.println("Name not equal. Expected <" +expectedBean.getName() + "> " + "received <" + name +">");
			areEqual = false;
		}
		
		if (producers.equals(expectedBean.getProducers()) == false) {
			System.out.println("Producers not equal. Expected <" +expectedBean.getProducers() + "> " + "received <" + producers +">");
			areEqual = false;
		}
		
		if (consumers.equals(expectedBean.getConsumers()) == false) {
			System.out.println("Consumers not equal. Expected <" +expectedBean.getConsumers() + "> " + "received <" + consumers +">");
			areEqual = false;
		}
		
		if (bufferedMsgs.equals(expectedBean.getBufferedMsgs()) == false) {
			System.out.println("BufferedMsgs not equal. Expected <" +expectedBean.getBufferedMsgs() + "> " + "received <" + bufferedMsgs +">");
			areEqual = false;
		}
		
		if (bufferedMsgsHWM.equals(expectedBean.getBufferedMsgsHWM()) == false) {
			System.out.println("BufferedMsgsHWM not equal. Expected <" +expectedBean.getBufferedMsgsHWM() + "> " + "received <" + bufferedMsgsHWM +">");
			areEqual = false;
		}
		
		if (bufferedPercent.equals(expectedBean.getBufferedPercent()) == false) {
			System.out.println("BufferedPercent not equal. Expected <" +expectedBean.getBufferedPercent() + "> " + "received <" + bufferedPercent +">");
			areEqual = false;
		}
		
		if (maxMessages.equals(expectedBean.getMaxMessages()) == false) {
			System.out.println("MaxMessages not equal. Expected <" +expectedBean.getMaxMessages() + "> " + "received <" + maxMessages +">");
			areEqual = false;
		}
		
		if (producedMsgs.equals(expectedBean.getProducedMsgs()) == false) {
			System.out.println("ProducedMsgs not equal. Expected <" +expectedBean.getProducedMsgs() + "> " + "received <" + producedMsgs +">");
			areEqual = false;
		}
		
		if (consumedMsgs.equals(expectedBean.getConsumedMsgs()) == false) {
			System.out.println("ConsumedMsgs not equal. Expected <" +expectedBean.getConsumedMsgs() + "> " + "received <" + consumedMsgs +">");
			areEqual = false;
		}
		
		if (rejectedMsgs.equals(expectedBean.getRejectedMsgs()) == false) {
			System.out.println("RejectedMsgs not equal. Expected <" +expectedBean.getRejectedMsgs() + "> " + "received <" + rejectedMsgs +">");
			areEqual = false;
		}
		
		if (bufferedHWMPercent.equals(expectedBean.getBufferedHWMPercent()) == false) {
			System.out.println("BufferedHWMPercent not equal. Expected <" +expectedBean.getBufferedHWMPercent() + "> " + "received <" + bufferedHWMPercent +">");
			areEqual = false;
		}
		
		if (expiredMsgs.equals(expectedBean.getExpiredMsgs()) == false) {
			System.out.println("ExpiredMsgs not equal. Expected <" +expectedBean.getExpiredMsgs() + "> " + "received <" + expiredMsgs +">");
			areEqual = false;
		}
		
		return areEqual;
		
	}


	public String getQueueName() {
		return queueName;
	}

	public void setQueueName(String queueName) {
		this.queueName = queueName;
	}
	
	public String getName() {
		return name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getProducers() {
		return producers;
	}

	public void setProducers(String producers) {
		this.producers = producers;
	}

	public String getConsumers() {
		return consumers;
	}

	public void setConsumers(String consumers) {
		this.consumers = consumers;
	}

	public String getBufferedMsgs() {
		return bufferedMsgs;
	}

	public void setBufferedMsgs(String bufferedMsgs) {
		this.bufferedMsgs = bufferedMsgs;
	}

	public String getBufferedMsgsHWM() {
		return bufferedMsgsHWM;
	}

	public void setBufferedMsgsHWM(String bufferedMsgsHWM) {
		this.bufferedMsgsHWM = bufferedMsgsHWM;
	}

	public String getBufferedPercent() {
		return bufferedPercent;
	}

	public void setBufferedPercent(String bufferedPercent) {
		this.bufferedPercent = bufferedPercent;
	}

	public String getMaxMessages() {
		return maxMessages;
	}

	public void setMaxMessages(String maxMessages) {
		this.maxMessages = maxMessages;
	}

	public String getProducedMsgs() {
		return producedMsgs;
	}

	public void setProducedMsgs(String producedMsgs) {
		this.producedMsgs = producedMsgs;
	}

	public String getConsumedMsgs() {
		return consumedMsgs;
	}

	public void setConsumedMsgs(String consumedMsgs) {
		this.consumedMsgs = consumedMsgs;
	}

	public String getRejectedMsgs() {
		return rejectedMsgs;
	}

	public void setRejectedMsgs(String rejectedMsgs) {
		this.rejectedMsgs = rejectedMsgs;
	}

	public String getBufferedHWMPercent() {
		return bufferedHWMPercent;
	}

	public void setBufferedHWMPercent(String bufferedHWMPercent) {
		this.bufferedHWMPercent = bufferedHWMPercent;
	}

	public String getExpiredMsgs() {
		return expiredMsgs;
	}

	public void setExpiredMsgs(String expiredMsgs) {
		this.expiredMsgs = expiredMsgs;
	}
	
	@Override
	public int compareTo(QueueStatBean o) {
		return 0;
	}

}
