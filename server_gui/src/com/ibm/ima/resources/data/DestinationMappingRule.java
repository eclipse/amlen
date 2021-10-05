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

import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

import com.fasterxml.jackson.annotation.JsonIgnore;
import com.fasterxml.jackson.annotation.JsonProperty;

import com.ibm.ima.util.IsmLogger;
import com.ibm.ima.util.IsmLogger.LogLevel;

/**
 * The REST representation of an MQ Connectivity Destination Mapping Rule
 *
 */
public class DestinationMappingRule extends AbstractIsmConfigObject {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    private final static String CLAS = DestinationMappingRule.class.getCanonicalName();
    private final static IsmLogger logger = IsmLogger.getGuiLogger();


    private final static String[] RQD_STRINGS = {"QueueManagerConnection","Source","Destination","MaxMessages"};

    public final static int RULE_TYPE_1_ISMTOPIC_2_MQQUEUE = 1;
    public final static int RULE_TYPE_2_ISMTOPIC_2_MQTOPIC = 2;
    public final static int RULE_TYPE_3_MQQUEUE_2_ISMTOPIC = 3;
    public final static int RULE_TYPE_4_MQTOPIC_2_ISMTOPIC = 4;
    public final static int RULE_TYPE_5_ISMTOPICSUBTREE_2_MQQUEUE = 5;
    public final static int RULE_TYPE_6_ISMTOPICSUBTREE_2_MQTOPIC = 6;
    public final static int RULE_TYPE_7_ISMTOPICSUBTREE_2_MQTOPICSUBTREE = 7;
    public final static int RULE_TYPE_8_MQTOPICSUBTREE_2_ISMTOPIC = 8;
    public final static int RULE_TYPE_9_MQTOPICSUBTREE_2_ISMTOPICSUBTREE = 9;
    public final static int RULE_TYPE_10_ISMQUEUE_2_MQUEUE = 10;
    public final static int RULE_TYPE_11_ISMQUEUE_2_MQTOPIC = 11;
    public final static int RULE_TYPE_12_MQQUEUE_2_ISMQUEUE = 12;
    public final static int RULE_TYPE_13_MQTOPIC_2_ISMQUEUE = 13;
    public final static int RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE = 14;
    
    public final static int QUEUE_NAME_MAX_LENGTH = 48;
    public final static Pattern QUEUE_NAME_VALIDATOR = Pattern.compile("[A-Za-z0-9\\._/%]*");
    public final static int MQTOPIC_MAX_LENGTH = 10240;
    public final static int TOPIC_MAX_LENGTH = 65535;
    public final static int QUEUE_MAX_LENGTH = 65535;
    
    public enum RetainedMessagesValue {
    	None,All;
    	
        /**
         * @return the default retained messages value as a string
         */
        public static String getDefaultRetainedMessagesValue() {
            return None.name();
        }
    }

    private static final int MIN_MESSAGE_COUNT_VALUE = 1;
    private static final int MAX_MESSAGE_COUNT_VALUE = 20000000;

    private String Name;
    private String QueueManagerConnection;
    private int RuleType = -1;
    private String Source;
    private String Destination;
    private String MaxMessages = "5000";
    private boolean Enabled = true;
    private String RetainedMessages = RetainedMessagesValue.getDefaultRetainedMessagesValue();

    @JsonIgnore
    private String keyName;

    public DestinationMappingRule() {

    }

    /**
     * @return the name
     */
    @JsonProperty("Name")
    public String getName() {
        return Name;
    }

    /**
     * @param name the name to set
     */
    @JsonProperty("Name")
    public void setName(String name) {
        Name = name;
    }


    /**
     * @return the queueManagerConnection
     */
    @JsonProperty("QueueManagerConnection")
    public String getQueueManagerConnection() {
        return QueueManagerConnection;
    }



    /**
     * @param queueManagerConnection the queueManagerConnection to set
     */
    @JsonProperty("QueueManagerConnection")
    public void setQueueManagerConnection(String queueManagerConnection) {
        QueueManagerConnection = queueManagerConnection;
    }



    /**
     * @return the ruleType
     */
    @JsonProperty("RuleType")
    public int getRuleType() {
        return RuleType;
    }



    /**
     * @param ruleType the ruleType to set
     */
    @JsonProperty("RuleType")
    public void setRuleType(int ruleType) {
        RuleType = ruleType;
    }



    /**
     * @return the source
     */
    @JsonProperty("Source")
    public String getSource() {
        return Source;
    }



    /**
     * @param source the source to set
     */
    @JsonProperty("Source")
    public void setSource(String source) {
        Source = source;
    }



    /**
     * @return the destination
     */
    @JsonProperty("Destination")
    public String getDestination() {
        return Destination;
    }



    /**
     * @param destination the destination to set
     */
    @JsonProperty("Destination")
    public void setDestination(String destination) {
        Destination = destination;
    }



    /**
     * @return the enabled
     */
    @JsonProperty("Enabled")
    public boolean isEnabled() {
        return Enabled;
    }


    @JsonProperty("Enabled")
    public void setEnabled(String enabledString) {
        this.Enabled = Boolean.parseBoolean(enabledString);
    }


    /**
     * @return the keyName
     */
    @JsonIgnore
    public String getKeyName() {
        return keyName;
    }


    /**
     * @param keyName the keyName to set
     */
    @JsonIgnore
    public void setKeyName(String keyName) {
        this.keyName = keyName;
    }

    @JsonProperty("MaxMessages")
    public String getMaxMessages() {
        return MaxMessages;
    }
    @JsonProperty("MaxMessages")
    public void setMaxMessages(String MaxMessages) {
        this.MaxMessages = MaxMessages;
    }
    
    @JsonProperty("RetainedMessages")
	public String getRetainedMessages() {
		return RetainedMessages;
	}

    @JsonProperty("RetainedMessages")
	public void setRetainedMessages(String RetainedMessages) {
		this.RetainedMessages = RetainedMessages;
	}
    
    /* (non-Javadoc)
     * @see com.ibm.ima.resources.data.AbstractIsmConfigObject#validate()
     */
    @Override
    public ValidationResult validate() {
        // check the name
        ValidationResult result = validateName(this.Name, "Name");
        if (result.isOK()) {
            // make sure required fields have a non-empty value
            if (checkForEmptyRequireds(new String[] {QueueManagerConnection, Source, Destination, MaxMessages},
            		RQD_STRINGS, result)) {
                return result;
            }
            if (!isRuleOK(RuleType, Source, Destination, result)) {
                return result;
            }
            
            // validate Max Message count
            int value = 0;
            try {
                value = Integer.parseInt(MaxMessages);
            } catch (NumberFormatException e) {
                result.setResult(VALIDATION_RESULT.NOT_AN_INTEGER);
                result.setParams(new Object[] {"MaxMessages"});
                return result;
            }
            if (value < MIN_MESSAGE_COUNT_VALUE || value > MAX_MESSAGE_COUNT_VALUE) {
                result.setResult(VALIDATION_RESULT.VALUE_OUT_OF_RANGE);
                result.setParams(new Object[] {"MaxMessages", MIN_MESSAGE_COUNT_VALUE, MAX_MESSAGE_COUNT_VALUE});
                return result;
            }
            
            // validate RetainedMessages value and ensure it is consistent with RuleType and queueManagerConnection 
            if (RetainedMessages != null && trimUtil(RetainedMessages).length() > 0) {
            	try {
            		RetainedMessagesValue rmValue = RetainedMessagesValue.valueOf(RetainedMessages);
            		if (rmValue != RetainedMessagesValue.None) {
            			// rules with a queue as the destination are not allowed (1, 5, 10, 12, 13, 14)
            			switch (RuleType) {
            			case RULE_TYPE_1_ISMTOPIC_2_MQQUEUE:
            			case RULE_TYPE_5_ISMTOPICSUBTREE_2_MQQUEUE:
            			case RULE_TYPE_10_ISMQUEUE_2_MQUEUE:
            			case RULE_TYPE_12_MQQUEUE_2_ISMQUEUE:
            			case RULE_TYPE_13_MQTOPIC_2_ISMQUEUE:
            			case RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE:
            				result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
            				result.setErrorMessageID("CWLNA5106");
            				result.setParams(new String[] {RetainedMessages});
            				return result;
            			default:
            				break;            					
            			}
            			// exactly one QueueManagerConnection is required
            			String[] connections = QueueManagerConnection.split(",");
            			if (connections.length > 1) {
            				result.setResult(VALIDATION_RESULT.UNEXPECTED_ERROR);
            				result.setErrorMessageID("CWLNA5107");
            				result.setParams(new String[] {RetainedMessages});
            				return result;            				
            			}
            		}
            	} catch (Exception e) {
            		result.setResult(VALIDATION_RESULT.INVALID_ENUM);
            		result.setParams(new Object[] {RetainedMessages, Arrays.toString(RetainedMessagesValue.values())});
            		return result;
            	}
            }

        }

        return result;
    }
    
    @Override
	public String toString() {
		return "DestinationMappingRule [Name=" + Name + ", QueueManagerConnection=" + QueueManagerConnection +
				", RuleType=" + RuleType + ", Source=" + Source + ", Destination=" + Destination +
				", MaxMessages=" + MaxMessages + ", Enabled=" + Enabled + ", RetainedMessages=" + RetainedMessages +"]";
	}

    private boolean isRuleOK(int ruleType, String source, String destination, ValidationResult result) {
        switch (ruleType) {
        case (RULE_TYPE_1_ISMTOPIC_2_MQQUEUE):
            if (!isTopicOK(source, "Source", result) || !isMqQueueOK(destination, "Destination", result)) {
                return false;
            }
        break;
        case (RULE_TYPE_2_ISMTOPIC_2_MQTOPIC):
            if (!isTopicOK(source, "Source", result) || !isMqTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;
        case (RULE_TYPE_3_MQQUEUE_2_ISMTOPIC):
            if (!isMqQueueOK(source, "Source", result) || !isTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_4_MQTOPIC_2_ISMTOPIC):
            if (!isMqTopicOK(source, "Source", result) || !isTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_5_ISMTOPICSUBTREE_2_MQQUEUE):
            if (!isTopicOK(source, "Source", result) || !isMqQueueOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_6_ISMTOPICSUBTREE_2_MQTOPIC):
            if (!isTopicOK(source, "Source", result) || !isMqTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_7_ISMTOPICSUBTREE_2_MQTOPICSUBTREE):
            if (!isTopicOK(source, "Source", result) || !isMqTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_8_MQTOPICSUBTREE_2_ISMTOPIC):
            if (!isMqTopicOK(source, "Source", result) || !isTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_9_MQTOPICSUBTREE_2_ISMTOPICSUBTREE):
            if (!isMqTopicOK(source, "Source", result) || !isTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_10_ISMQUEUE_2_MQUEUE):
            if (!isQueueOK(source, "Source", result) || !isMqQueueOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_11_ISMQUEUE_2_MQTOPIC):
            if (!isQueueOK(source, "Source", result) || !isMqTopicOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_12_MQQUEUE_2_ISMQUEUE):
            if (!isMqQueueOK(source, "Source", result) || !isQueueOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_13_MQTOPIC_2_ISMQUEUE):
            if (!isMqTopicOK(source, "Source", result) || !isQueueOK(destination, "Destination", result)) {
                return false;
            }
        break;

        case (RULE_TYPE_14_MQTOPICSUBTREE_2_ISMQUEUE):
            if (!isMqTopicOK(source, "Source", result) || !isQueueOK(destination, "Destination", result)) {
                return false;
            }
        break;
        
        default:
            logger.log(LogLevel.LOG_WARNING, CLAS, "checkRule", "CWLNA5022", new Object[] {ruleType});  //
        }
        return true;
    }


    /**
     * Rules for an MQQueue name: Up to 48 single byte characters. Upper and lower
     * case A-Z, 0-9, period (.), underscore (_)
     * All variables assumed to be verified non-null and not empty before calling
     * If the MQQueue name is OK, returns true
     */
    private boolean isMqQueueOK(String name, String propertyName, ValidationResult result) {

        if (getByteCount(name) > QUEUE_NAME_MAX_LENGTH) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG_CHARS);
            result.setParams(new Object[] {propertyName, QUEUE_NAME_MAX_LENGTH});
            return false;
        }
        Matcher m = QUEUE_NAME_VALIDATOR.matcher(name);
        if (!m.matches()) {
            result.setResult(VALIDATION_RESULT.INVALID_CHARS);
            result.setErrorMessageID("CWLNA5023");
            result.setParams(new Object[] {name});
            return false;
        }
        return true;
    }

    /**
     * All variables assumed to be verified non-null and not empty before calling
     * If the topic string is OK, returns true
     */
    private boolean isMqTopicOK(String topic, String propertyName, ValidationResult result) {
        if (getByteCount(topic) > MQTOPIC_MAX_LENGTH) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            result.setParams(new Object[] {propertyName, MQTOPIC_MAX_LENGTH});
            return false;
        }
        
        // Make sure the topic is not reserved for system use
        if (topic.startsWith("$SYS")) {
            result.setResult(VALIDATION_RESULT.INVALID_SYS_TOPIC);
            result.setParams(new Object[] {propertyName});
            return false;
        }

        return checkWildcards(topic, propertyName, result);

    }
    
    /**
     * All variables assumed to be verified non-null and not empty before calling
     * If the topic string is OK, returns true
     */
    private boolean isTopicOK(String topic, String propertyName, ValidationResult result) {
        if (getByteCount(topic) > TOPIC_MAX_LENGTH) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            result.setParams(new Object[] {propertyName, TOPIC_MAX_LENGTH});
            return false;
        }

        // Make sure the topic is not reserved for system use
        if (topic.startsWith("$SYS")) {
            result.setResult(VALIDATION_RESULT.INVALID_SYS_TOPIC);
            result.setParams(new Object[] {propertyName});
            return false;
        }
        
        return checkWildcards(topic, propertyName, result);

    }

    private boolean checkWildcards(String topic, String propertyName, ValidationResult result) {
        
        if (topic.contains("/#/") || topic.endsWith("/#") || topic.startsWith("#/") || topic.equals("#")) {
            result.setResult(VALIDATION_RESULT.INVALID_WILDCARD);
            result.setParams(new Object[] {propertyName, "#"});
            return false;
        }

        if (topic.contains("/+/") || topic.endsWith("/+") || topic.startsWith("+/") || topic.equals("+")) {
            result.setResult(VALIDATION_RESULT.INVALID_WILDCARD);
            result.setParams(new Object[] {propertyName, "+"});
            return false;
        }
        
        return true;
    }
    
    /**
     * All variables assumed to be verified non-null and not empty before calling
     * If the queue name is OK, returns true
     */
    private boolean isQueueOK(String queue, String propertyName, ValidationResult result) {
    	
    	if (getByteCount(queue) > QUEUE_MAX_LENGTH) {
            result.setResult(VALIDATION_RESULT.VALUE_TOO_LONG);
            result.setParams(new Object[] {propertyName, QUEUE_MAX_LENGTH});
            return false;
        }
    	

    	ValidationResult queueValidation = validateName(queue, propertyName);
    	
    	if (!queueValidation.isOK()) {
            result.setResult(queueValidation.getResult());
            result.setErrorMessageID(queueValidation.getErrorMessageID());
            result.setParams(queueValidation.getParams());
            return false;
    	}

        return true;
    }

    /**
     * If there are empty requireds, returns true
     */
    private boolean checkForEmptyRequireds(String[] strings, String[] rqdStrings, ValidationResult result) {

        int num = strings.length;
        for (int i = 0; i < num; i++) {
            if (strings[i] == null || trimUtil(strings[i]).length() == 0) {
                String objectName = "";
                if (rqdStrings.length > i) {
                    objectName = rqdStrings[i];
                }
                result.setResult(VALIDATION_RESULT.VALUE_EMPTY);
                result.setParams(new Object[] {objectName});
                return true;
            }
        }
        return false;
    }

    /**
     * All variables assumed to be verified non-null and not empty before calling
     */
    private int getByteCount(String value) {
        int length = value.length();
        try {
            byte[] bytes = value.getBytes("UTF8");
            length = bytes.length;
        } catch (UnsupportedEncodingException e) {
            logger.log(LogLevel.LOG_WARNING, CLAS, "getByteCount", "CWLNA5000", e);
        }
        return length;
    }

}



