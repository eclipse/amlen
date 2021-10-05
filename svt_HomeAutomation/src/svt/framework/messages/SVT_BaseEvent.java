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
package svt.framework.messages;

public abstract class SVT_BaseEvent {
	public String requestTopic;
	public String replyTopic;
	public Integer qos;
	public String message;

	public String getMessageData() {
		return message;
	}

	public String getRequestTopic() {
		return requestTopic;
	}

	public String getReplyTopic() {
		return replyTopic;
	}

	public void setRequestTopic(String topicStr) {
		requestTopic = topicStr;
	}

	public void setReplyTopic(String topicStr) {
		replyTopic = topicStr;
	}
}
