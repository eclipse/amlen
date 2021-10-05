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
/**
 * 
 */
package test.llm.sync;

import java.util.HashMap;
import java.util.Map;

/**
 * Managed Container for Conditions representing a 'namespace'.
 * Provides public accessors to get, set, and delete conditions or to reset itself
 */
public class Solution {
	/**
	 * map of the conditions
	 */
	protected Map<String, Condition> conditions;

	/**
	 * Create a Solution.
	 */
	public Solution() {
		conditions = new HashMap<String, Condition>();
	}

	/**
	 * Atomic removal from ConcurrentMap if existing should wait until free
	 * 
	 * @param name
	 *            of condition to be removed
	 */
	public void deleteCondition(String name) {
		synchronized (conditions) {
			_deleteCondition(name);
		}
	}

	/**
	 * internal, assumes 'conditions' already locked
	 * 
	 * @param name
	 *            Condition to be deleted
	 */
	private void _deleteCondition(String name) {
		Condition temp = conditions.get(name);
		if (temp != null) {
			synchronized (conditions.get(name)) {
				temp = conditions.remove(name);
				if (temp != null) {
					temp.notifyAll();
				}
			}
		}
	}

	/**
	 * Removes all conditions from a solution
	 */
	public void reset() {
		synchronized (conditions) {
			while (0 != conditions.keySet().size()) {
			    String temp = (String)conditions.keySet().toArray()[0];
				_deleteCondition(temp);
			}
		}
	}

	/**
	 * Insert the condition if it is absent
	 * 
	 * @param id
	 *            Name of condition
	 */
	private void putIfAbsent(String id) {
		synchronized (conditions) {
			if (conditions.get(id) == null) {
				Condition temp = new Condition(); // really not there insert it
				conditions.put(new String(id), temp);
			}
		}
	}

	/**
	 * 
	 * Get the named condition's state from map; inserting if need be.
	 * 
	 * @param id
	 *            Identifier of desired Condition
	 * @return desired Condition's state
	 */
	public long getConditionState(String id) {
		putIfAbsent(id);
		return conditions.get(id).state;
	}

	/**
	 * Set the state of a named condition; fails if non-existent
	 * 
	 * @param id
	 * @param value
	 * @return value that was set, -1 on error (in addition to exception)
	 */
	public long setConditionState(String id, long value) {
		try {
			putIfAbsent(id);
			synchronized (conditions.get(id)) {
				conditions.get(id).state = value;
				conditions.get(id).notifyAll();
			}
		} catch (NullPointerException e) {
			Logger.println("Warning: id( " + id + ") not found");
			return -1;
		}
		return value;
	}

	/**
	 * Wait for a Desired Condition to occur Waits for a condition state to
	 * reach desired value. Wait can be given a timeout in which case it will
	 * wait a maximum of that amount of time before 'timingOut' and failing.
	 * 
	 * @param condition
	 * @param state
	 * @param timeout
	 * @return 1 if condition was met
	 */
	public int waitForCond(String condition, long state, long timeout) {
		boolean timedOut = false;
		boolean found = false;
		int conditionMet = 0;
		long remainingTime = timeout;
		long timeBeforeWait = 0L;
		try {
			this.getConditionState(condition);
			synchronized (conditions.get(condition)) {
				Condition myCondition = conditions.get(condition);
				found = (myCondition.state == state);
				while (!timedOut && !found) {
					timeBeforeWait = System.currentTimeMillis();
					try {
						myCondition.wait(remainingTime);
						Logger.println("WAIT OVER for "+condition);
						myCondition = conditions.get(condition); // update
																	// condition
						found = (myCondition.state == state);
						if (!found && timeout != 0) { // if not found & not
							// indefinite wait
							remainingTime -= System.currentTimeMillis() - timeBeforeWait;
							if (remainingTime <= 0) {
								timedOut = true;
							}
						}
					} catch (InterruptedException e) {
						myCondition = conditions.get(condition); // update condition
						found = (myCondition.state == state);
						if (!found && timeout != 0) { // if not found and not indefinite wait
							remainingTime -= System.currentTimeMillis() - timeBeforeWait;
							if (remainingTime <= 0) {
								timedOut = true;
							}
						}
					} // catch
				} // while
				conditions.get(condition).notifyAll();
			} // synchrony
		} catch (NullPointerException t) {
			return -2;
		}

		if (found == true) {
			conditionMet = 1;
		}
		return conditionMet;
	}

}
