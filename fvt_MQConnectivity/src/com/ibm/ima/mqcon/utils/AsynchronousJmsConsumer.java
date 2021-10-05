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
package com.ibm.ima.mqcon.utils;

import javax.jms.Message;

import com.ibm.ima.mqcon.msgconversion.JmsToJms;

public abstract class AsynchronousJmsConsumer extends JmsToJms {

	public abstract void callback(Message msg);
}
