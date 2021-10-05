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
package com.ibm.mqst.mqxr.scada;
public final class Headers
{
    public final static byte RESERVED = 0x00;
    public final static byte CONNECT = 0x01;
    public final static byte CONNACK = 0x02;
    public final static byte PUBLISH = 0x03;
    public final static byte PUBACK = 0x04;
    public final static byte PUBREC = 0x05;
    public final static byte PUBREL = 0x06;
    public final static byte PUBCOMP = 0x07;
    public final static byte SUBSCRIBE = 0x08;
    public final static byte SUBACK = 0x09;
    public final static byte UNSUBSCRIBE = 0x0A;
    public final static byte UNSUBACK = 0x0B;
    public final static byte PINGREQ = 0x0C;
    public final static byte PINGRESP = 0x0D;
    public final static byte DISCONNECT = 0x0E;
    public final static byte AUTH = 0x0F;

}
