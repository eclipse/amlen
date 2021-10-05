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
 * File        : Action.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.util.HashMap;
import java.util.Map;
import java.util.Properties;

import javax.naming.CompositeName;
import javax.naming.Context;
import javax.naming.InitialContext;

import org.w3c.dom.Element;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

/**
 * This is the base class for all test actions.
 * It is responsible for parsing of all common parameters.
 * <p>
 */
abstract class Action implements Counter {
	private static Context _ctx = null;
	static final String DEFAULT_THREAD_ID = "1";
	protected final String _id;
	private final Map<String, Action> _dependents;
	private final Map<String, DependencyData> _dependsOn;
	protected boolean _canceled;
	private final boolean _continueOnFailure;
	private final int _repeat;
	private final int _expected;
	private final int _atleast;
	private final long _repeatInterval;
	protected final Object _mutex;
	protected final DataRepository _dataRepository;
//	private final long _startTime;
	private final long _executionTime;
	private final String _threadId;
	private static String _jndiNamePrefix;
	protected final TestProperties _actionParams;
	protected final TrWriter _trWriter;

	private int				_counter = 0;
	
	static synchronized void jndiInit(Properties env, String namePrefix) throws Exception {
		if (_ctx != null)
			throw new IsmTestException("ISMTEST"+Constants.ACTION, "JNDI context is already initialized.");
		_jndiNamePrefix = namePrefix;
		_ctx = new InitialContext(env);
	}

	static synchronized Object jndiLookup(String name, TrWriter trWriter) throws IsmTestException{
		if(_ctx == null)
			throw new IsmTestException("ISMTEST"+(Constants.ACTION+1), "JNDI context is not initialized.");
		try {
			return _ctx.lookup(new CompositeName().add(name));
		} catch (Exception e) {
			trWriter.writeException(e);
			throw new IsmTestException("ISMTEST"+(Constants.ACTION+2), "JNDI lookup failed.", e);
		}
	}

	static synchronized void jndiBind(String name, Object adminobj, TrWriter trWriter) throws IsmTestException{
		if(_ctx == null)
			throw new IsmTestException("ISMTEST"+(Constants.ACTION+3), "JNDI context is not initialized.");
		try {
			_ctx.rebind(new CompositeName().add(name),adminobj);
			return;
		} catch (Exception e) {
			trWriter.writeException(e);
			throw new IsmTestException("ISMTEST"+(Constants.ACTION+4), "JNDI bind failed.", e);
		}
	}

	Action(Element config, DataRepository dataRepository, TrWriter trWriter) {
		_mutex = new Object();
		synchronized (_mutex) {
			_trWriter = trWriter;
			_id = config.getAttribute("name");
			_dependents = new HashMap<String, Action>();
			_dependsOn = new HashMap<String, DependencyData>();
			_canceled = false;
			_dataRepository = dataRepository;
			_actionParams = new TestProperties();
			int repeat;
			String configStr = getConfigAttribute(config, "repeat", "1");
			repeat = Integer.parseInt(configStr);
			configStr = getConfigAttribute(config, "continueOnFailure", "0");
			_continueOnFailure = configStr.equals("0") ? false : true;
			configStr = getConfigAttribute(config, "expected", "-1");
			_expected = Integer.parseInt(configStr);
			if (_expected > repeat - 1) {
				repeat = _expected + 1;
			}
			configStr = getConfigAttribute(config, "atleast", "-1");
			_atleast = Integer.parseInt(configStr);
			if (_expected != -1 && _atleast > repeat - 1) {
				repeat = _atleast + 1;
			}
			_repeat = repeat;
			_repeatInterval = Long.parseLong(getConfigAttribute(config, "repeat_interval", "0"));
			_threadId = getConfigAttribute(config, "thread", DEFAULT_THREAD_ID);
//			configStr = getConfigAttribute(config, "start", "0");
//			_startTime = Long.parseLong(configStr);
			configStr = getConfigAttribute(config, "execution_time", "0");
			_executionTime = Long.parseLong(configStr);
			parseDependencies(config);
			parseParameters(config);
		}
		_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"ISMTEST"+(Constants.ACTION+5),"Action {0} was created. Action parameters: {1}. Depends on: {2}",this.toString(),_actionParams, _dependsOn);
	}

	public final int getCount(){
		return _counter;
	}

	protected final int getCounterValue(String counterID){
		if(counterID.equalsIgnoreCase("this"))
			return _counter;
		Counter counter = (Counter) _dataRepository.getVar(counterID);
		if(counter == null)
			throw new RuntimeException("Counter " + counterID + " does not exist.");
		return counter.getCount();
	}
	
	private void parseDependencies(Element config) {
		final Action thisAction = this;
		TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {

			public void process(Element element) {
				String actionID = element.getAttribute("action_id");
				if(actionID.length() == 0){
					throw new RuntimeException("'action_id' is not specified for dependency");
				}
				Action action = _dataRepository.getAction(actionID);
				if(action == null){
					throw new RuntimeException("Action " + actionID + " is not defined");
				}
				action.addDependent(thisAction);
				String interval = getConfigAttribute(element, "interval", "10");
				long waitTime = Long.parseLong(interval);
				_dependsOn.put(actionID, new DependencyData(waitTime));
			}
		};
		TestUtils.walkXmlElements(config, "dependsOn", processor);
	}

	private void parseParameters(Element config) {
		TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
			public void process(Element element) {
				String paramName = element.getAttribute("name");
				String text = element.getTextContent();
				if (text != null) {
					_actionParams.setProperty(paramName, text);
				} else {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_WARN, "ISMTEST"+(Constants.ACTION+6),
							"Action {0}: Value for parameter {1} is not set.",
							_id, paramName);
				}
			}
		};
		TestUtils.walkXmlElements(config, "ActionParameter", processor);
	}

	void setPreviousAction(Action action) {
		synchronized (_mutex) {
			// Check whether dependency was defined explicitly and if not add
			// it.
			if (!_dependsOn.containsKey(action.getActionID())) {
				_dependsOn.put(action.getActionID(), new DependencyData(10));
				action.addDependent(this);
			}
		}
	}

	String getActionID() {
		return _id;
	}

	boolean isCanceled() {
		synchronized (_mutex) {
			return _canceled;
		}
	}
	
	boolean continueOnFailure() {
		return _continueOnFailure;
	}
	
	String getJndiNamePrefix (){
		return _jndiNamePrefix;
	}

	void addDependent(Action action) {
		synchronized (_mutex) {
			_dependents.put(action.getActionID(), action);
		}
	}

	void cancel() {
		synchronized (_mutex) {
			_canceled = true;
			_mutex.notifyAll();
		}
	}

	String getExecutionThreadId() {
		return _threadId;
	}

	void notifyDone(Action action) {
		synchronized (_mutex) {
			// Update the dependency data
			DependencyData dd = _dependsOn.get(action.getActionID());
			if (dd != null) {
				dd._completionTime = System.currentTimeMillis();
				_mutex.notifyAll();
			}
			_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"ISMTEST"+(Constants.ACTION+7),"Action {0} was notified of completion action {1}. Dependecies: {2}",this.toString(),action,_dependsOn.toString());
		}
	}

	boolean execute() {
		boolean rc = true;
		waitForDependencies();// Wait for dependencies to complete
		long nextStartTime = _dataRepository.getCurrentTime() /*+ _startTime*/;
		long endTime;
		int executed = 0;
		if(rc)
		{
			for (int i = 0; i < _repeat; i++) {
				waitForAlarm(nextStartTime);
				if(_canceled) {
					break;
				}
				_counter++;
				endTime = _dataRepository.getCurrentTime() + _executionTime;
				try{
					rc = run();
				} catch (Throwable e) {
						_trWriter.writeException(e);
						_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.ACTION+8), "Action {0} failed. The reason is: {1}", _id, e);
						rc = false;
				}
				if(!rc) {
					if (_expected == executed || (_atleast >= 0 && executed >= _atleast)) {
						rc = true;
						_canceled = false;
					}
					break;
				}
				waitForAlarm(endTime);
				nextStartTime = _dataRepository.getCurrentTime() + _repeatInterval;
				executed++;
				if (_repeat > 1) {
					_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST"+(Constants.ACTION+9), "Action {0} done iteration {1}", _id, executed);
				}
			}
		}
		if (_expected != -1) {
			if (executed != _expected) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.ACTION+10), "Action {0} failed. Expected {1} iterations but processed {2}.", _id, _expected, executed);
				rc = false;
			}
		} else if (_atleast != -1) {
			if (executed < _atleast) {
				_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.ACTION+10), "Action {0} failed. Expected at least {1} iterations but processed {2}.", _id, _atleast, executed);
				rc = false;
			}
		} else if (executed < _repeat && _repeat > 1) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST"+(Constants.ACTION+11), "Action {0} failed. Only completed {1} out of {2} iterations.", _id, executed, _repeat);
			rc = false;
		}
		synchronized (_mutex) {
			for (Action action : _dependents.values()) {
				if (((!rc) || (_canceled)) && (!_continueOnFailure)){
					action.cancel();
				}else{
					_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"ISMTEST"+(Constants.ACTION+12),"Action {0} is going to notify dependent action {1}.",this.toString(),action);
					action.notifyDone(this);
				}
			}
			if((!rc) || (_canceled)) {
				if(_canceled){
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO,"ISMTEST"+(Constants.ACTION+13),"Action {0} was canceled.",this.toString());
				}else{
					_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO,"ISMTEST"+(Constants.ACTION+14),"Action {0} failed.",this.toString());					
				}
				return false;
			}else{
				_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO,"ISMTEST"+(Constants.ACTION+15),"Action {0} completed successfully.",this.toString());
				return true;
			}
		}

	}

	public String toString() {
		String result = getName() + ':' + _id;
		return result;
	}

	protected abstract boolean run();

	private String getName() {
		String longClassName = this.getClass().getName();
		int i = longClassName.lastIndexOf('.');
		return longClassName.substring(i + 1);
	}

	private void waitForDependencies() {
		synchronized (_mutex) {
			do {
				long waitTime = 0;
				long tm = 0;
				if (_canceled) {
					return;
				}
				for (DependencyData dd : _dependsOn.values()) {
					tm = dd.getTimeToWait();
					if (tm > waitTime) {
						waitTime = tm;
					}
				}
				if (waitTime < 1) {
					return;
				}
				//
				if (waitTime < 10) {
					waitTime = 10;
				}

				try {
					String wt = "";
					if (waitTime < Long.MAX_VALUE) {
						wt = String.valueOf(waitTime) + "ms ";
					}
					_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"ISMTEST"+(Constants.ACTION+16),"Action {0} is going to wait {1}for dependencies {2}.",this.toString(),wt, _dependsOn);
					_mutex.wait(waitTime);
				} catch (InterruptedException e) {
					if (!_canceled) {
						_trWriter.writeException(e);
					} else {
						return;
					}
				}
			} while (true);
		}

	}

	private void waitForAlarm(long alarmTime) {
		while (true) {
			long waitTime = alarmTime - _dataRepository.getCurrentTime();
			if (waitTime < 1) {
				return;
			}
			_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE,"ISMTEST"+(Constants.ACTION+17),"Action {0} is going to wait {1}ms for start time.",this.toString(),waitTime);
			synchronized (_mutex) {
				if (_canceled) {
					return;
				}
				try {
					_mutex.wait(waitTime);
				} catch (InterruptedException e) {
					if (!_canceled) {
						_trWriter.writeException(e);
					} else {
						return;
					}
				}
			}
		}
	}

	protected void reset() {
		_canceled = false;
		_counter = 0;
		for (DependencyData dep : _dependsOn.values()) {
			dep._completionTime = 0;
		}
	}

	protected static final String getConfigAttribute(Element config, String attribName, String defaultValue){
		String result = config.getAttribute(attribName);
		if (result.length() == 0) {
			result = defaultValue;
		}
		return result;
	}

	private static final class DependencyData {
		private final long _waitTime;
		private long _completionTime;

		DependencyData(long interval) {
			_waitTime = interval;
			_completionTime = 0;
		}

		long getTimeToWait() {
			if (_completionTime == 0) {
				return Long.MAX_VALUE;
			}
			long result = _completionTime + _waitTime - System.currentTimeMillis();
			if (result < 0) {
				return 0;
			}
			return result;
		}

		public String toString() {
			return "(" + String.valueOf(_waitTime) + ", "
					+ String.valueOf(_completionTime) + ')';
		}
	}
}
