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
package com.ibm.ima.jms.test;

public interface JmsMessageUtils {
    enum JmsMessageType {
        SIMPLE, MAP, OBJECT, STREAM, TEXT, BYTES
    }
    enum JmsActionType {
        GET, SET
    }
    enum JmsPropertyType    {
        CorrelationID,DeliveryMode,Destination,Expiration,MessageID,Priority,
        Redelivered,ReplyTo,Timestamp,MessageType,Common;
    }
    enum JmsValueType {
        Boolean, Byte, ByteArray, Short, Float, Integer, Long, Double, String, Object, Counter, UTF8
    }

}
