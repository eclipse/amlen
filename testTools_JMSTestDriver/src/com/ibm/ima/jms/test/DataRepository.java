/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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
/*-----------------------------------------------------------------
 * System Name : MQ Low Latency Messaging
 * File        : DataRepository.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import java.util.Collection;

/**
 * This is an interface for test data repository.
 * <p>
 * 
 */

public interface DataRepository {
    /**
     * Get action with the specified action id
     * 
     * @param id
     *            - action id
     * @return action with the specified id
     */
    Action getAction(String id);

    /**
     * Get current time
     * 
     * @return current time
     */
    long getCurrentTime();

    /**
     * Store action into the repository
     * 
     * @param action
     */
    void addAction(Action action);

    /**
     * Get variable by name
     * 
     * @param varName
     * @return variable
     */
    Object getVar(String varName);

    /**
     * Store variable into the repository
     * 
     * @param varName
     * @param varValue
     */
    void storeVar(String varName, Object varValue);

    /**
     * Remove variable from the repository
     * 
     * @param varName
     * @return removed variable
     */
    Object removeVar(String varName);

    /**
     * Ret collection of values contained in DataRepository
     * 
     * @return
     */
    Collection<Object> values();
}
