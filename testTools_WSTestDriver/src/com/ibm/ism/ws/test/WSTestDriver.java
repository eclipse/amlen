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
 * File        : JMSTestDriver.java
 * Author      :  
 *-----------------------------------------------------------------
 */

package com.ibm.ism.ws.test;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Map;

import java.util.concurrent.ConcurrentHashMap;
import java.util.jar.Attributes;
import java.util.jar.JarFile;
import java.util.jar.Manifest;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import test.llm.sync.SyncClient;

import com.ibm.ism.ws.test.TrWriter.LogLevel;

public final class WSTestDriver implements DataRepository {

	private final Map<String, Action> _actions = new HashMap<String, Action>();
	private long _startTime;
	private static boolean _overallResult = true;
	private Map<String, LinkedList<Action>> _actionsPerThread = new HashMap<String, LinkedList<Action>>();
	private final Map<String, Object> _variables = new ConcurrentHashMap<String, Object>(
			1024);
	private final TrWriter _trWriter;
	private final String _xmlFileName;
	private final static DocumentBuilderFactory factory;
	private final static DocumentBuilder builder;
	private static String _version_string;
	private static String _actionDelimiter;
	public static String pahoVersion = null;
	static {
		try {
			factory = DocumentBuilderFactory.newInstance();
			builder = factory.newDocumentBuilder();
		} catch (Throwable e) {
			e.printStackTrace();
			throw new RuntimeException(e);
		}
	}

	/**
	 * Test Driver class
	 * 
	 * @param config
	 *            XML document
	 * @param testcase
	 *            name of testcase
	 * @param logLevel
	 * @param logFile
	 */
	WSTestDriver(Document config, String validActions, LogLevel logLevel,
			String logFile) throws Exception {
		_trWriter = new TrWriter(logFile, logLevel);
		_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST0000",
				_version_string, (Object) null);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
                "HOSTNAME: " + java.net.InetAddress.getLocalHost().getHostName());
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
                "OPERATING SYSTEM: " + System.getProperty("os.name"));
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
                "JAVA VERSION: " + System.getProperty("java.version"));
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
                "JAVA VENDOR: " + System.getProperty("java.vendor"));
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
                "JAVA VM_NAME: " + System.getProperty("java.vm.name"));
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
        		"LOGFILE: "+logFile);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD0000",
        		"TESTACTIONS: "+validActions);
        System.out.println("LOGFILE: "+logFile);
        System.out.println("HOSTNAME: " + java.net.InetAddress.getLocalHost().getHostName());
        System.out.println("OPERATING SYSTEM: " + System.getProperty("os.name"));
        System.out.println("JAVA VERSION: " + System.getProperty("java.version"));
        System.out.println("JAVA VENDOR: " + System.getProperty("java.vendor"));
        System.out.println("JAVA VM_NAME: " + System.getProperty("java.vm.name"));
        System.out.println("TESTACTIONS: "+validActions);
		_xmlFileName = config.getDocumentURI();
		Element root = config.getDocumentElement();
		if (root == null) {
			throw new IsmTestException("ISMTEST0002", "Test configuration file "
					+ _xmlFileName + " has no root element");
		}
		parseTestCase(root, validActions);
	}

	private final static class IsmTestRunnable implements Runnable {
		final LinkedList<Action> _la;
		Thread thread = null;
		boolean result = true;

		IsmTestRunnable(LinkedList<Action> la) {
			_la = la;
		}

		public void run() {
			for (Action action : _la) {
				if(result){
					boolean rc = action.execute();
					if(!action.continueOnFailure())
						result = rc;
					else
						result = true;
					if(_overallResult)
						_overallResult = rc;
				}else{
					action.cancel();
				}
			}
		}
	}

	/**
	 * 
	 */
	private void runTest() {
		//boolean rc = true;
		LinkedList<IsmTestRunnable> threads = new LinkedList<IsmTestRunnable>();
		_startTime = System.currentTimeMillis();
		for (String tid : _actionsPerThread.keySet()) {
			IsmTestRunnable rn = new IsmTestRunnable(_actionsPerThread.get(tid));
			rn.thread = new Thread(rn, "TestThread." + tid);
			rn.thread.start();
			threads.addLast(rn);
		}
		for (IsmTestRunnable rn : threads) {
			try {
				rn.thread.join();
				//rc &= rn.result;
			} catch (InterruptedException e) {
				_trWriter.writeException(e);
			}
		}
		
		if (_overallResult) {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "ISMTEST0003",
					"Test ({0}) Success!", _xmlFileName);
		} else {
			_trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "ISMTEST0004",
					"Test ({0}) Failure!", _xmlFileName);
		}
//		_trWriter.close();
	}

	/**
	 * Usage Statement
	 */
	private static void usage() {
		System.err
				.println("Usage: WSTestDriver <TestConfigFile.xml> [options]");
		System.err.println("Options:");
		System.err.println("\t -n: driver name");
		System.err.println("\t -l: log level");
		System.err.println("\t -f: log file");
		System.exit(1);
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		String version = null;
		String build = null;
		Attributes atts = null;
		try {
			File myjar = new File(WSTestDriver.class.getProtectionDomain().getCodeSource().getLocation().getPath());
			try {
				JarFile jar = new JarFile(myjar);
				atts = jar.getManifest().getMainAttributes();
				jar.close();
			} catch (Exception ex){
				File myman = new File (WSTestDriver.class.getProtectionDomain().getCodeSource().getLocation().getPath().toString()+"META-INF/MANIFEST.MF");
				InputStream in = new FileInputStream(myman);
				Manifest man = new Manifest(in);
				atts = man.getMainAttributes();
			}
			version = atts.getValue("Implementation-Version");
			build = atts.getValue("Implementation-Build");
			_version_string = "WSTestDriver: "+version+":"+build;
			System.out.println(_version_string);
			
			if (args.length < 1) {
				usage();
			}
			String testcase = null;
			LogLevel logLevel = LogLevel.LOGLEV_EVENT;
			String logFile = null;
			Document document = builder.parse(new File(args[0]));
			parseIncludes(document);
			parseRootElement(document);
			/*
			 * LinkedList<Element> l =
			 * TestUtils.getActionsList(document.getDocumentElement(),
			 * "node1,node2"); for (Element element : l) {
			 * System.err.println("------------------------");
			 * System.err.println(TestUtils.xmlToString(element, true)); } l =
			 * TestUtils.getActionsList(document.getDocumentElement(), null);
			 * for (Element element : l) {
			 * System.err.println("------------------------");
			 * System.err.println(TestUtils.xmlToString(element, true)); }
			 */
			pahoVersion = System.getenv("PAHO_VERSION");

			for (int i = 1; i < args.length; i++) {
				if ((i + 1) == args.length) {
					usage();
				}
				if (args[i].equals("-n")) {
					testcase = args[++i];
					continue;
				}
				if (args[i].equals("-f")) {
					logFile = args[++i];
					continue;
				}
				if (args[i].equals("-l")) {
					logLevel = LogLevel.valueOf(Integer.parseInt(args[++i]));
					continue;
				}
				if (args[i].equals("-v")) {
					pahoVersion = args[++i];
					continue;
				} 
				usage();
			}

			WSTestDriver rmtd = new WSTestDriver(document, testcase,
					logLevel, logFile);
			rmtd.runTest();
		} catch (Throwable t) {
			t.printStackTrace();
		}
		System.exit(0);
		return;
	}

/*	private void parseJndiParameters(Element config) throws Exception {
		final Properties jndiConfig = new Properties();
		String namePrefix = null;
		NodeList nl = config.getElementsByTagName("NamePrefix");
		if (nl.getLength() > 0) {
			namePrefix = nl.item(0).getTextContent();
		}
		TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
			public void process(Element element) {
				String paramName = element.getAttribute("name");
				String text = element.getTextContent();
				if (text != null) {
					jndiConfig.setProperty(paramName, text);
				} else {
					throw new RuntimeException(
							"Value for JNDI configuration parameter "
									+ paramName + " is not set.");
				}
			}
		};
		TestUtils.walkXmlElements(config, "JndiParameter", processor);
		Action.jndiInit(jndiConfig,namePrefix);
	}
*/
	
	private static void parseRootElement(Document doc) throws Exception {
		Element rootEl = doc.getDocumentElement();
		String delim = rootEl.getAttribute("actionDelim");
		_actionDelimiter = (delim.equals("") ? "," : delim);
		
	}

	private static void parseIncludes(Document doc) throws Exception {
		LinkedList<Node> nodesToRemove = new LinkedList<Node>();
		LinkedList<Node> nodesToAdd = new LinkedList<Node>();
		NodeList nl = doc.getElementsByTagName("include");
		for (int i = 0; i < nl.getLength(); i++) {
			Element origElement = (Element) nl.item(i);
			String fileName = origElement.getTextContent();
			Document document = builder.parse(new File(fileName));
			Element n = document.getDocumentElement();
			if (n == null)
				throw new IsmTestException("ISMTEST0005", "The included file "
						+ fileName + " is empty.");
			Node replaceElement = doc.importNode(n, true);
			nodesToRemove.addLast(origElement);
			nodesToAdd.addLast(replaceElement);
		}
		while (!nodesToRemove.isEmpty()) {
			Node origElement = nodesToRemove.removeFirst();
			Node replaceElement = nodesToAdd.removeFirst();
			Node parent = origElement.getParentNode();
			parent.replaceChild(replaceElement, origElement);
		}
	}

	/**
	 * parse TestCase to build driver model
	 * 
	 * @param config
	 *            NonNull input
	 * @param testcase
	 * @throws Exception
	 */
	private void parseTestCase(Element config, String names) throws Exception {

		NodeList nl = config.getElementsByTagName("SyncClient");
		if (nl.getLength() > 0) {
			parseSyncClient(nl.item(0));
		}
		LinkedList<Element> actionsList = TestUtils.getActionsList(config,
				names,_actionDelimiter);
		if (actionsList.isEmpty()) {
			System.err.println("WARNING: No actions defined for the test");
			System.exit(1);
		}
		if (null != pahoVersion) {
			this.storeVar("PahoVersion", pahoVersion);
		} else {
			this.storeVar("PahoVersion", "4");
		}
		for (Element element : actionsList) {
			Action action = ActionFactory
					.createAction(element, this, _trWriter);
			addAction(action);
		}
	}

	/**
	 * Parse the SyncClient XML
	 * 
	 * @param node
	 *            NON NULL input
	 * @return
	 */
	private void parseSyncClient(Node node) {
		Node child = node;
		String address = "", solution = "";
		int myPort = 0;
		_trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "ISMTEST0006",
				"Enter: begin parsing SyncClient", (Object) null);
		try {
			child = node.getFirstChild();
			child = getFirstElement(child);
			if (!child.getNodeName().equals("server")) {
				throw new RuntimeException("Parsing Error, no 'server' element");
			}

			child = getFirstElement(child.getFirstChild());
			if (!child.getNodeName().equals("address")) {
				throw new RuntimeException(
						"Parsing Error, no 'address' element");
			}
			address = child.getTextContent();

			child = getFirstElement(child.getNextSibling());
			if (!child.getNodeName().equals("port")) {
				throw new RuntimeException("Parsing Error, no 'port' element");
			}
			myPort = Integer.parseInt(child.getTextContent());

			child = getFirstElement(child.getParentNode().getNextSibling());
			if (!child.getNodeName().equals("solution")) {
				throw new RuntimeException(
						"Parsing Error, no 'solution' element");
			}
			solution = child.getTextContent();
			IsmSyncClient.init(address, solution, myPort, _trWriter);

		} catch (RuntimeException e) {
			e.printStackTrace();
			System.err.println("Null SyncServer returned, exiting");
			System.exit(-1);
		} catch (Throwable e) {
			e.printStackTrace();
			System.err.println("Connection failed:\n" + e.getMessage());
			System.exit(-1);
		}
	}

	/**
	 * @param node
	 * @return
	 */
	private Node getFirstElement(Node node) {
		while (node != null && node.getNodeType() != Node.ELEMENT_NODE) {
			node = node.getNextSibling();
		}
		return node;
	}

	public void addAction(Action action) {
		if(getAction(action.getActionID()) != null)
			throw new RuntimeException("Duplicate action id: " + action.getActionID());
		_actions.put(action.getActionID(), action);
		LinkedList<Action> la = _actionsPerThread.get(action
				.getExecutionThreadId());
		if (la == null) {
			la = new LinkedList<Action>();
			_actionsPerThread.put(action.getExecutionThreadId(), la);
		} else {
			Action previousAction = la.getLast();
			action.setPreviousAction(previousAction);
		}
		la.addLast(action);
	}

	public Action getAction(String id) {
		return _actions.get(id);
	}

	public SyncClient getClient() {
		return null;
	}

	public long getCurrentTime() {
		return System.currentTimeMillis() - _startTime;
	}

	public Object getVar(String varName) {
		if (null == varName) return null;
		return _variables.get(varName);
	}

	public Object removeVar(String varName) {
		if (null == varName) return null;
		return _variables.remove(varName);
	}

	public void storeVar(String varName, Object varValue) {
		_variables.put(varName, varValue);
	}

}
