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

package com.ibm.ima.resources.data;

import com.fasterxml.jackson.annotation.JsonIgnoreProperties;
import com.fasterxml.jackson.annotation.JsonProperty;

/*
 * The REST representation of a server transport TopicMonitor object
 */
@JsonIgnoreProperties
public class TopicMonitor extends AbstractIsmConfigObject implements Comparable<TopicMonitor> {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private String topicString;

    public TopicMonitor() {
    }

    @JsonProperty("TopicString")
    public String getTopicString() {
        return topicString;
    }
    @JsonProperty("TopicString")
    public void setTopicString(String topicString) {
        this.topicString = topicString;
    }

    @Override
    public int compareTo(TopicMonitor l) {
        return this.getTopicString().compareTo(l.getTopicString());
    }

    @Override
	public String toString() {
		return "TopicMonitor [topicString=" + topicString
				+  "]";
	}

	@Override
	public ValidationResult validate() {
		
		ValidationResult result = new ValidationResult();
		if (this.topicString == null || this.topicString.length() == 0) {
			result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
		} else if (this.topicString.length() == 1 && !this.topicString.equals("#")) {
			result.setResult(VALIDATION_RESULT.INVALID_TOPIC_MONITOR);
		} else if (this.topicString.length() > 3 && this.topicString.startsWith("$SYS")) {
			result.setResult(VALIDATION_RESULT.INVALID_TOPIC_MONITOR);
		} else if (this.topicString.length() > 1 && !this.topicString.endsWith("/#")) {
			result.setResult(VALIDATION_RESULT.INVALID_TOPIC_MONITOR);
		} else {
			result.setResult(VALIDATION_RESULT.OK);
		}
		
		return result;
	}
}
