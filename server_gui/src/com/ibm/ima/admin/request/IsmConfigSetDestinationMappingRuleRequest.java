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

package com.ibm.ima.admin.request;

import com.fasterxml.jackson.databind.annotation.JsonSerialize;
import com.fasterxml.jackson.databind.annotation.JsonSerialize.Inclusion;

import com.ibm.ima.resources.data.DestinationMappingRule;

public class IsmConfigSetDestinationMappingRuleRequest extends IsmMQConnectivityConfigRequest {

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


    @JsonSerialize
    private String Type;
    @JsonSerialize
    private String Item;
    @JsonSerialize
    private String Name;
    @JsonSerialize (include = Inclusion.NON_NULL)
    private String QueueManagerConnection = null;
    @JsonSerialize (include = Inclusion.NON_DEFAULT)
    private int RuleType = -1;
    @JsonSerialize (include = Inclusion.NON_NULL)
    private String Source = null;
    @JsonSerialize (include = Inclusion.NON_NULL)
    private String Destination = null;
    @JsonSerialize (include = Inclusion.NON_NULL)
    private String Enabled = null;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String MaxMessages = null;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String RetainedMessages = null;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Delete = null;
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String Update = null;
    
    @SuppressWarnings("unused")
    @JsonSerialize(include = Inclusion.NON_NULL)
    private String NewName;
    
    /**
     * Default constructor, required for using @JsonSerialize(include=Inclusion.NON_DEFAULT).
     */
    public IsmConfigSetDestinationMappingRuleRequest() {
        this.Action = "Set";
        this.Type = "Composite";
        this.Item = "DestinationMappingRule";    	
    }

    public IsmConfigSetDestinationMappingRuleRequest(String username) {
    	this();
        this.User = username;
    }

    public void setFieldsFromDestinationMappingRule(DestinationMappingRule rule) {
        if (rule.getKeyName() != null && !rule.getKeyName().equals(rule.getName())) {
            this.NewName = rule.getName();
            this.Name = rule.getKeyName();
        } else {
            this.Name = rule.getName();
        }
        this.Destination = rule.getDestination();
        this.MaxMessages = rule.getMaxMessages();
        this.Enabled = rule.isEnabled() ? "True" : "False";
        this.QueueManagerConnection = rule.getQueueManagerConnection();
        this.RuleType = rule.getRuleType();
        this.Source = rule.getSource();
        this.RetainedMessages = rule.getRetainedMessages();
    }

	/*
	 * Only set the fields from rule if they are different from existingRule
	 */
    public void setFieldsFromDestinationMappingRule(DestinationMappingRule rule, DestinationMappingRule existingRule) {
    	// Name must always be set
        if (rule.getKeyName() != null && !rule.getKeyName().equals(rule.getName())) {
            this.NewName = rule.getName();
            this.Name = rule.getKeyName();
        } else {
            this.Name = rule.getName();
        }
        this.Destination = getDiff(rule.getDestination(), existingRule.getDestination());
        this.MaxMessages = getDiff(rule.getMaxMessages(), existingRule.getMaxMessages());
        if (rule.isEnabled() != existingRule.isEnabled()) {
        	this.Enabled = rule.isEnabled() ? "True" : "False";
        }
        this.QueueManagerConnection = getDiff(rule.getQueueManagerConnection(), existingRule.getQueueManagerConnection());
        if (rule.getRuleType() != existingRule.getRuleType()) {
        	this.RuleType = rule.getRuleType();
        }
        this.Source = getDiff(rule.getSource(), existingRule.getSource());
        this.RetainedMessages = getDiff(rule.getRetainedMessages(), existingRule.getRetainedMessages());
	}

	/**
	 * If s1 != null and s1 != s2, return s1, otherwise return null.
	 * Used to limit the properties we send to only those that have changed.
	 * @param s1
	 * @param s2
	 * @return
	 */
    private String getDiff(String s1, String s2) {
		if (s1 != null && !s1.equals(s2)) {
			return s1;
		}
		return null;
	}

	public void setName(String name) {
        this.Name = name;
    }
    public void setDelete() {
        this.Delete = "true";
    }
    public void setUpdate() {
        this.Update = "true";
    }
    
	public void setEnabled(boolean enabled) {
		this.Enabled = enabled ? "True" : "False";		
	}


    @Override
    public String toString() {
        return "IsmConfigSetMQConnPropertiesRequest [Type=" + Type + ", Item=" + Item
                + ", Name=" + Name + ", Delete=" + Delete + ", Update=" + Update
                + ", Destination=" + Destination + ", MaxMessages=" + MaxMessages + ", Enabled=" + Enabled
                + ", QueueManagerConnection=" + QueueManagerConnection + ", RuleType=" + RuleType 
                + ", Source=" + Source + ", RetainedMessages=" + RetainedMessages + "]";
    }

}
