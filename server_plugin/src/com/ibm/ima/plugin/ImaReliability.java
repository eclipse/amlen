/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.plugin;

/**
 * Define the reliability or quality of service of the message.
 * <p>
 * This describes the reliability in the face of client or network failure.
 * To maintain this reliability for qos1 and qos2 the message should 
 * also be persistent but reliability and persistence may be set independently.
 */
public enum ImaReliability {
    /** The message might not be delivered and is not re-delivered (at most once) */
    qos0,
    
    /** The message is re-delivered in case of an error or connection restart (at least once) */
    qos1,
    
    /** The message is reliably delivered without duplicates (exactly once) */ 
    qos2;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";

}
