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
package svt.util;

import javax.jms.Destination;
import javax.jms.Message;
import javax.jms.Queue;
import javax.jms.TextMessage;
import javax.jms.Topic;

import svt.util.SVTLog.TTYPE;

public class SVTJMSMessage implements SVTMessage {
    TTYPE TYPE = TTYPE.ISMJMS;
    Message jmsmessage;

    public SVTJMSMessage(Message message) {
        this.jmsmessage = message;
    }

    public String getTopicName() {
        String destName = null;

        try {
            Destination dest = jmsmessage.getJMSDestination();
            if (dest instanceof Topic) {
                destName = ((Topic) dest).getTopicName();
            } else if (dest instanceof Queue) {
                destName = ((Queue) dest).getQueueName();
            }
        } catch (Throwable e) {
            SVTLog.logex(TYPE, "getJMSDestination|getTopicName|getQueueName", e);
        }

        return destName;
    }

    public String[] getTopicTokens() {
        String[] token;
        String delims = "[/]+";

        token = getTopicName().split(delims);

        return token;
    }

    public String getMessageText() {
        String text = "";

        try {
            if (jmsmessage instanceof TextMessage) {
                text = ((TextMessage) jmsmessage).getText();
            }
            if (jmsmessage instanceof javax.jms.BytesMessage) {
                int length = (int) ((javax.jms.BytesMessage) jmsmessage).getBodyLength();
                byte[] bytes = new byte[length];
                ((javax.jms.BytesMessage) jmsmessage).readBytes(bytes, length);
                text = new String(bytes, "UTF8");
            } else {
                SVTLog.log(TYPE, jmsmessage.getJMSType());
                SVTLog.log(TYPE, jmsmessage.toString());
                throw new Exception("Unexpected message received.");
            }
        } catch (Throwable e) {
            SVTLog.logex(TYPE, "getText|getBodyLength|readBytes|...", e);
        }

        return text;
    }

}
