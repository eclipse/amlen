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

package com.ibm.ima.jms.test;

import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.Collection;
import java.util.HashMap;
import java.util.concurrent.ConcurrentHashMap;
import java.util.LinkedList;
import java.util.Map;
import java.util.Properties;

import java.util.jar.Attributes;
import java.util.jar.JarFile;
import java.util.jar.Manifest;

import javax.jms.Connection;
import javax.jms.JMSException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import test.llm.sync.SyncClient;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public final class JMSTestDriver implements DataRepository {

    private final Map<String, Action> _actions = new HashMap<String, Action>();
    private long _startTime;
    private static boolean _overallResult = true;
    private Map<String, LinkedList<Action>> _actionsPerThread = new HashMap<String, LinkedList<Action>>();
    private final Map<String, Object> _variables = new ConcurrentHashMap<String, Object>(1024);
    private final TrWriter _trWriter;
    private final String _xmlFileName;
    private final static DocumentBuilderFactory factory;
    private final static DocumentBuilder builder;
    private static String _version_string;
    private static String _actionDelimiter;
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
    JMSTestDriver(Document config, String validActions, LogLevel logLevel,
            String logFile) throws Exception {
        _trWriter = new TrWriter(logFile, logLevel);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMDR9000",
                _version_string, (Object) null);
        if(logFile != null)
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMDR9000",
                "LOGFILE: " + logFile, (Object) null);
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
        
        _xmlFileName = config.getDocumentURI();
        Element root = config.getDocumentElement();
        if (root == null) {
            throw new JmsTestException("JMST0002", "Test configuration file "
                    + _xmlFileName + " has no root element");
        }
        parseTestCase(root, validActions);
    }

    private final static class RMTestRunnable implements Runnable {
        final LinkedList<Action> _la;
        Thread thread = null;
        boolean result = true;

        RMTestRunnable(LinkedList<Action> la) {
            _la = la;
        }

        public void run() {
            for (Action action : _la) {
                if(result){
                    boolean rc = action.execute();
                    if(!action.continueOnFailure()) {
                        result = rc;
                        /*
                         * Attempt to close any open connections in dataRepository
                         */
                        if (!rc) {
                            Collection<Object> objs = action._dataRepository.values();
                            for (Object o : objs) {
                                if (o instanceof Connection) {
                                    Connection c = (Connection) o;
                                    try {
                                        c.close();
                                    } catch (JMSException e) {
                                        e.printStackTrace();
                                    }
                                }
                            }
                        }
                    } else
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
        LinkedList<RMTestRunnable> threads = new LinkedList<RMTestRunnable>();
        _startTime = System.currentTimeMillis();
        for (String tid : _actionsPerThread.keySet()) {
            RMTestRunnable rn = new RMTestRunnable(_actionsPerThread.get(tid));
            rn.thread = new Thread(rn, "TestThread." + tid);
            rn.thread.start();
            threads.addLast(rn);
        }
        for (RMTestRunnable rn : threads) {
            try {
                rn.thread.join();
            } catch (InterruptedException e) {
                _trWriter.writeException(e);
            }
        }
        
        if (_overallResult) {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD5000",
                    "Test ({0}) Success!", _xmlFileName);
        } else {
            _trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR, "RMTD3000",
                    "Test ({0}) Failure!", _xmlFileName);
        }
//        _trWriter.close();
    }

    /**
     * Usage Statement
     */
    private static void usage() {
        System.err
                .println("Usage: JMSTestDriver <TestConfigFile.xml> [options]");
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
            File myjar = new File(JMSTestDriver.class.getProtectionDomain().getCodeSource().getLocation().getPath());
            try {
                atts = new JarFile(myjar).getManifest().getMainAttributes();
            } catch (Exception ex){
                File myman = new File (JMSTestDriver.class.getProtectionDomain().getCodeSource().getLocation().getPath().toString()+"META-INF/MANIFEST.MF");
                InputStream in = new FileInputStream(myman);
                Manifest man = new Manifest(in);
                atts = man.getMainAttributes();
            }
            version = atts.getValue("Implementation-Version");
            build = atts.getValue("Implementation-Build");
            _version_string = "JMSTestDriver: "+version+":"+build;
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
            
            //(06-17-16) Setting IMAEnforceObjectMessageSecurity=false always, since getObject method is now blocked by default
            System.setProperty("IMAEnforceObjectMessageSecurity", "false");
            
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
                usage();
            }

            JMSTestDriver rmtd = new JMSTestDriver(document, testcase,
                    logLevel, logFile);
            rmtd.runTest();
        } catch (Throwable t) {
            t.printStackTrace();
        }
        return;
    }

    private void parseJndiParameters(Element config) throws Exception {
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
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9999", "Before jndiInit");
        Action.jndiInit(jndiConfig,namePrefix);
        _trWriter.writeTraceLine(LogLevel.LOGLEV_INFO, "RMTD9999", "After jndiInit");
    }
    
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
                throw new JmsTestException("JMST0001", "The included file "
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
        nl = config.getElementsByTagName("JndiClient");
        if (nl.getLength() > 0) {
            parseJndiParameters((Element) nl.item(0));
        }
        LinkedList<Element> actionsList = TestUtils.getActionsList(config,
                names,_actionDelimiter);
        if (actionsList.isEmpty()) {
            System.err.println("WARNING: No actions defined for the test");
            System.exit(1);
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
        _trWriter.writeTraceLine(LogLevel.LOGLEV_XTRACE, "RMDR9000",
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
            JmsSyncClient.init(address, solution, myPort, _trWriter);

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
        return _variables.get(varName);
    }

    public Object removeVar(String varName) {
        return _variables.remove(varName);
    }

    public void storeVar(String varName, Object varValue) {
        _variables.put(varName, varValue);
    }
    
    public Collection<Object> values() {
        return _variables.values();
    }

}
