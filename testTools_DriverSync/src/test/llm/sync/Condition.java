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
package test.llm.sync;

/**
 * Simple Base for Conditions.
 * 
 * Conditions are named by an ID and have a state. Thread-safe through synchrony
 * on set/get. Typically managed by a Solution.. Exists mainly for extension
 * purposes
 * 
 * 
 */
public class Condition {
	/**
	 * Current Condition state
	 */
	public long	state;
	
	/**
	 * Creates a Condition with in the default state (-1).
	 */
	public Condition() {
		this.state = -1;
	}
	
	/**
	 * Creates a Condition with a given state
	 * 
	 * @param initialVal
	 *            Integer value assigned to state.
	 */
	public Condition(int initialVal) {
		this.state = initialVal;
	}
	
}
