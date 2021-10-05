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
/*-----------------------------------------------------------------
 * System Name : Internet Scale Messaging
 * File        : CompositeAction.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

/**
 * This class implements a composite action (i.e a set of actions that should be
 * executed sequentially) .
 * <p>
 * 
 */

class CompositeAction extends Action implements DataRepository {
	private final Map<String, Action> _actions = new HashMap<String, Action>();
	private long _startTime;
	private Map<String, LinkedList<Action>> _actionsPerThread = new HashMap<String, LinkedList<Action>>();
	private final Map<String, Object> _variables = new ConcurrentHashMap<String, Object>(
			1024);
	private final Map<String, Boolean> _results;
	private final HashSet<String> _activeThreads;

	CompositeAction(Element config, DataRepository dataRepository,
			TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_results = new HashMap<String, Boolean>();
		_activeThreads = new HashSet<String>();
		LinkedList<Element> actionsList = TestUtils
				.getActionsList(config, null);
		if (actionsList.isEmpty()) {
			throw new RuntimeException(
					"ERROR: Bad config file: CompositeAction is empty");
		}
		for (Element element : actionsList) {
			Action action = ActionFactory.createAction(element, this, trWriter);
			if (action != null) {
				addAction(action);
			}
		}
	}

	@Override
	protected boolean run() {
		_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPOSITE),
				"Start CompositeAction: {0}", _id);

		// Reset after previous run
		_activeThreads.clear();
		_results.clear();
		_variables.clear();
		storeVar(_id, this);//Store this action as a counter
		for (Action action : _actions.values()) {
			action.reset();
		}

		// Create and start action threads
		_startTime = System.currentTimeMillis();
		for (String tid : _actionsPerThread.keySet()) {
			Runnable rn = new CompositeActionRunable(tid);
			Thread thread = new Thread(rn, "TestThread." + getActionID() + '.'
					+ tid);
			_activeThreads.add(tid);
			thread.start();
		}

		// Wait for all threads to complete
		boolean result = true;
		synchronized (_activeThreads) {
			while (!_activeThreads.isEmpty()) {
				try {
					_activeThreads.wait();
				} catch (InterruptedException e) {
					_trWriter.writeException(e);
				}
			}
			for (Boolean rc : _results.values()) {
				result = (result && rc.booleanValue());
			}
		}
		_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPOSITE+1),
				"End CompositeAction: {0} RC = {1} ", _id, result);
		return result;
	}

	void cancel() {
		for (Action action : _actions.values()) {
			action.cancel();
		}
		super.cancel();
	}

	public void addAction(Action action) {
		if(getAction(action.getActionID()) != null)
			throw new RuntimeException("Duplicate action id: " + action.getActionID());
		_actions.put(action.getActionID(), action);
		String threadID = _id + '.' + action.getExecutionThreadId();
		LinkedList<Action> la = _actionsPerThread.get(threadID);
		if (la == null) {
			// This is the first action for the thread
			la = new LinkedList<Action>();
			_actionsPerThread.put(threadID, la);
		} else {
			Action previousAction = la.getLast();
			action.setPreviousAction(previousAction);
		}
		la.addLast(action);
	}

	public Action getAction(String id) {
		Action result = _actions.get(id);
		if ((result == null) && (_dataRepository != null)) {
			result = _dataRepository.getAction(id);
		}
		return result;
	}

	public long getCurrentTime() {
		return System.currentTimeMillis() - _startTime;
	}

	public Object getVar(String varName) {
		if (null == varName) return null;
		Object result = _variables.get(varName);
		if (result == null) {
			result = _dataRepository.getVar(varName);
		}
		return result;
	}

	public Object removeVar(String varName) {
		if (null == varName) return null;
		Object result = _variables.remove(varName);
		if (result == null) {
			result = _dataRepository.removeVar(varName);
		}
		return result;
	}

	public void storeVar(String varName, Object varValue) {
		if (varValue == null) {
			return;
		}
		_variables.put(varName, varValue);
	}

	private final class CompositeActionRunable implements Runnable {
		private final String _threadID;

		CompositeActionRunable(String tid) {
			_threadID = tid;
		}

		public void run() {
			boolean rc = true;
			try {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPOSITE+2),
						"Execution thread {0} started.", _threadID);
				// Copy the action list. We do this only to be able to print
				// actions that were not executed
				LinkedList<Action> actions = new LinkedList<Action>(
						_actionsPerThread.get(_threadID));
				while (!actions.isEmpty()) {
					Action action = actions.removeFirst();
					rc = action.execute();
					if (!rc) {
						_trWriter
								.writeTraceLine(
										LogLevel.LOGLEV_ERROR,
										"ISMTEST"+(Constants.COMPOSITE+3),
										"Thread {0}. Action {1} failed. Actions {2} will not be executed.",
										_threadID, action.getActionID(),
										actions.toString());
						cancel();
						break;
					}
				}

			} catch (Throwable t) {
				_trWriter.writeException(t);
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.COMPOSITE+4),
						"Execution thread {0} terminated. The reason is: {1}",
						_threadID, t);
				rc = false;
			}
			// Update results and active threads.
			Boolean result = new Boolean(rc);
			synchronized (_activeThreads) {
				_results.put(_threadID, result);
				_activeThreads.remove(_threadID);
				if (_activeThreads.isEmpty()) {
					_activeThreads.notifyAll();
				}
			}
			_trWriter
					.writeTraceLine(LogLevel.LOGLEV_DEBUG, "ISMTEST"+(Constants.COMPOSITE+5),
							"Execution thread {0} ended. Result {1}",
							_threadID, result);
		}
	}

}

