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
 * File        : TestUtils.java
 *-----------------------------------------------------------------
 */

package com.ibm.ima.jms.test;

import java.io.File;
import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.Properties;
import java.util.StringTokenizer;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import com.ibm.ima.jms.test.TrWriter.LogLevel;

public final class TestUtils {
    private static final char[] H = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
    private final static XPath _xpath = XPathFactory.newInstance().newXPath();
    private final static Transformer xmlTransformer;
    static {
        try {
            xmlTransformer = TransformerFactory.newInstance().newTransformer();
            xmlTransformer.setOutputProperty(OutputKeys.INDENT, "yes");
        } catch (Throwable e) {
            e.printStackTrace();
            throw new RuntimeException(e);
        }
    }

    private TestUtils() {

    }

    public static String arrayToString(int nParams, Object... params) {
        StringBuffer sb = new StringBuffer(128);
        sb.append('{');
        if ((params != null) && (nParams > 0)) {
            sb.append('\"').append(String.valueOf(params[0])).append('\"');
            for (int i = 1; i < nParams; i++) {
                sb.append(", ").append('\"').append(String.valueOf(params[i]))
                        .append('\"');
            }
        }
        sb.append('}');
        return sb.toString();
    }

    public static int[] parseFragLens(String maxFragLens) {
        ArrayList<Integer> tokens = new ArrayList<Integer>();

        StringTokenizer strtok = new StringTokenizer(maxFragLens, "#");
        while (strtok.hasMoreTokens()) {
            String token = strtok.nextToken();
            if (token.length() == 0) {
                continue;
            }
            tokens.add(Integer.parseInt(token));
        }
        Integer wrap[] = tokens.toArray(new Integer[tokens.size()]);
        int frags[] = new int[tokens.size()];
        for (int i = 0; i < tokens.size(); i++) {
            frags[i] = wrap[i].intValue();
        }
        return frags;

    }

    public static Properties parseConfigProperties(String configProperties,
            String delim) {
        Properties result = new Properties();
        StringTokenizer strtok = new StringTokenizer(configProperties, delim);
        while (strtok.hasMoreTokens()) {
            String token = strtok.nextToken();
            token = token.trim();
            if (token.length() == 0) {
                continue;
            }
            int i = token.indexOf('=');
            if (i == -1) {
                throw new RuntimeException("Invalid configuration parameters: "
                        + configProperties);
            }
            String key = token.substring(0, i);
            String value = token.substring(i + 1);
            result.setProperty(key, value);
        }
        return result;
    }

    public static Properties parseConfigProperties(String configProperties) {
        return parseConfigProperties(configProperties, ";");
    }

    public static Properties parseVerifyConfigProperties(String configProperties) {
        Properties result = new Properties();
        char operatorSign = ' ';
        int i = -1;
        StringTokenizer strtok = new StringTokenizer(configProperties, ";");
        while (strtok.hasMoreTokens()) {
            String token = strtok.nextToken();
            token = token.trim();
            if (token.length() == 0) {
                continue;
            }
            i = token.indexOf('=');
            if (i != -1) {
                operatorSign = '=';
            } else {
                i = token.indexOf('<');
                if (i != -1) {
                    operatorSign = '<';
                } else {
                    i = token.indexOf('>');
                    if (i != -1) {
                        operatorSign = '>';
                    }
                }
            }

            if (i == -1) {
                throw new RuntimeException("Invalid configuration parameters: "
                        + configProperties);
            }
            String key = token.substring(0, i);
            String value = operatorSign + ";" + token.substring(i + 1);
            result.setProperty(key, value);
        }
        return result;
    }

    public static byte[] parseBitmap(String bitmap, int length) {
        boolean trim = false;
        if (length == -1) {
            trim = true;
            length = 8;
        }
        byte[] result = new byte[length];
        final int radix;
        if (bitmap.startsWith("0x")) {
            bitmap = bitmap.substring(2);
            radix = 16;
        } else {
            radix = 10;
        }
        long value = Long.parseLong(bitmap, radix);
        for (int i = 1; i <= length; i++) {
            result[length - i] = (byte) (value & 0xff);
            value = value >>> 8;
        }
        if (trim) {
            int i;
            for (i = 0; i < result.length; i++) {
                if (result[i] != 0) {
                    break;
                }
            }
            byte[] tmp = new byte[result.length - i];
            for (int j = 0; i < result.length; i++, j++) {
                tmp[j] = result[i];
            }
            result = tmp;
        }
        return result;
    }

    public static Properties parseAncillaryParams(String ancillaryParams) {
        Properties result = new Properties();
        StringTokenizer strtok = new StringTokenizer(ancillaryParams, ",");
        while (strtok.hasMoreTokens()) {
            String token = strtok.nextToken();
            if (token.length() == 0) {
                continue;
            }
            int i = token.indexOf('=');
            if (i == -1) {
                throw new RuntimeException("Invalid ancillary parameters: "
                        + ancillaryParams);
            }
            String key = token.substring(0, i);
            String value = token.substring(i + 1);
            result.setProperty(key, value);
        }
        return result;
    }

    public static int int2byteArray(int num, byte[] buff, int offset) {
        int n = num;
        int index = offset;
        buff[index++] = (byte) (n & 0xff);
        n = n >>> 8;
        buff[index++] = (byte) (n & 0xff);
        n = n >>> 8;
        buff[index++] = (byte) (n & 0xff);
        n = n >>> 8;
        buff[index++] = (byte) (n & 0xff);
        return index;
    }

    public static int byteArray2int(byte[] ba, int offset) {
        int index = offset;
        int n;
        int j1 = 0xff & ba[index++];
        int j2 = 0xff & ba[index++];
        int j3 = 0xff & ba[index++];
        int j4 = 0xff & ba[index++];
        j2 = j2 << 8;
        j3 = j3 << 16;
        j4 = j4 << 24;
        n = j1 | j2 | j3 | j4;
        return n;
    }

    /**
     * Utility function to expand 'includes' within the XML files
     * 
     * @param config
     *            XML Element to parse for includes
     * @param trWriter
     *            LogWriter object
     */
    public static void parseIncludes(Element config, TrWriter trWriter) {
        LinkedList<Node> nodesToRemove = new LinkedList<Node>();
        LinkedList<Node> nodesToAdd = new LinkedList<Node>();
        Document owner = config.getOwnerDocument();
        for (Node n = config.getFirstChild(); n != null; n = n.getNextSibling()) {
            if (n.getNodeType() != Node.ELEMENT_NODE) {
                continue;
            }
            if (!("include".equals(n.getNodeName()))) {
                continue;
            }
            nodesToRemove.addLast(n);
            Element node = (Element) n;
            String location = node.getAttribute("location");
            if (location.length() == 0) {
                throw new RuntimeException(
                        "Invalid location for include file. " + node.toString());
            }
            try {
                DocumentBuilderFactory factory = DocumentBuilderFactory
                        .newInstance();
                DocumentBuilder builder = factory.newDocumentBuilder();
                Document document = builder.parse(new File(location));
                for (Node chld = document.getFirstChild(); chld != null; chld = chld
                        .getNextSibling()) {
                    if (chld.getNodeType() != Node.ELEMENT_NODE) {
                        continue;
                    }
                    nodesToAdd.addLast(owner.importNode(chld, true));
                }
            } catch (Throwable t) {
                trWriter.writeException(t);
                throw new RuntimeException("Failed to parse include file.", t);
            }
            for (Node node1 : nodesToAdd) {
                config.insertBefore(node1, n.getNextSibling());
                n = node1;
            }
            for (Node node1 : nodesToRemove) {
                config.removeChild(node1);
            }
            nodesToAdd.clear();
            nodesToRemove.clear();
        }
    }

    public static byte[] longToByteArray(long value) {
        byte[] arrByte = new byte[8];
        for (int i = 0; i < 8; i++) {
            arrByte[0 + i] = (byte) (value & 0xFF);
            value >>>= 8;
        }

        return arrByte;
    }

    public static long byteArrayToLong(byte[] arr) {
        long l = 0;
        for (int i = 7; i >= 0; i--) {
            l <<= 8;
            l |= (arr[0 + i] & 0xFF);
        }
        return l;

    }
    
    public static byte[] stringToByteArray(String s){
        StringTokenizer st = new StringTokenizer(s,",");
        byte[] result = new byte[st.countTokens()];
        int i = 0;
        while(st.hasMoreTokens()){
            result[i++] = Byte.valueOf(st.nextToken());
        }
        return result;
    }
    
    static final LinkedList<Element> getActionsList(Element config, String names) {
        return getActionsList(config, names, ",");
    }

    static final LinkedList<Element> getActionsList(Element config, String names, String delim) {
        final LinkedList<Element> result = new LinkedList<Element>();
        final HashSet<String> validNames;
        if ((names != null) && (names.length() > 0)) {
            StringTokenizer st = new StringTokenizer(names, delim);
            validNames = new HashSet<String>();
            while (st.hasMoreTokens()) {
                validNames.add(st.nextToken());
            }
        } else {
            validNames = null;
        }
        TestUtils.XmlElementProcessor processor = new TestUtils.XmlElementProcessor() {
            public void process(Element element) {
                String actionName = element.getAttribute("name");
                if ((validNames == null) || (validNames.contains(actionName))) {
                    result.addLast(element);
                }
            }
        };
        TestUtils.walkXmlElements(config, "Action", processor);
        return result;
    }

    static String xmlToString(Node node, boolean noXmlDeclaration) {
        StreamResult result = new StreamResult(new StringWriter());
        try {
            String oldProp = null;
            if (noXmlDeclaration) {
                oldProp = xmlTransformer
                        .getOutputProperty(OutputKeys.OMIT_XML_DECLARATION);
                xmlTransformer.setOutputProperty(
                        OutputKeys.OMIT_XML_DECLARATION, "yes");
            }
            xmlTransformer.transform(new DOMSource(node), result);
            if (oldProp != null)
                xmlTransformer.setOutputProperty(
                        OutputKeys.OMIT_XML_DECLARATION, oldProp);
        } catch (Throwable t) {
            t.printStackTrace();
            return null;
        }
        return result.getWriter().toString();
    }
    //Stream ID is an opaque data and should be treated as byte array
    static void sid2str(long sid, StringBuffer buff)
    {
        for(int i = 0; i < 8; i++){
            byte b = (byte) (0xff & sid);
            byte b1 = (byte)( (b>>>4)& 15); 
            char c = H[b1];
            buff.append(c);
            c = H[b&15];
            buff.append(c);
            sid >>>= 8;
        }
    }

    static final void walkXmlElements(Element config, String name,
            XmlElementProcessor processor) {
        try {
            XPathExpression expr = _xpath.compile("./" + name);
            NodeList nl = (NodeList) expr.evaluate(config,
                    XPathConstants.NODESET);
            for (int i = 0; i < nl.getLength(); i++) {
                Node n = nl.item(i);
                if (n.getNodeType() != Node.ELEMENT_NODE) {
                    continue;
                }
                processor.process((Element) n);
            }
        } catch (Throwable t) {
            t.printStackTrace();
            throw new RuntimeException(t);
        }
    }

    interface XmlElementProcessor {
        void process(Element element);
    }

    static final class Property {
        final PropertyType type;
        final String name;
        final String value;
        String validator = null;
        enum PropertyType {
            INT, SHORT, LONG, DOUBLE, FLOAT, STRING, BYTE, BYTE_ARRAY,OBJECT, BOOLEAN
        }

        Property(String n, String v,String t) {
            type = PropertyType.valueOf(t);
            name = n;
            value = v;
        }

        String getName() {
            return name;
        }
        PropertyType getType(){
            return type;
        }
        
        String getValidator() {
            return validator;
        }
        
        void setValidator(String _validator) {
            this.validator = _validator;
        }

        Object getValue(DataRepository dataRepository, TrWriter trWriter) {
            switch (type) {
            case BYTE:
                return Byte.valueOf(value);
            case BYTE_ARRAY:
                return stringToByteArray(value);
            case BOOLEAN:
                return Boolean.valueOf(value);
            case SHORT:
                return Short.valueOf(value);
            case INT:
                return Integer.valueOf(value);
            case LONG:
                return Long.valueOf(value);
            case FLOAT:
                return Float.valueOf(value);
            case DOUBLE:
                return Double.valueOf(value);
            case OBJECT: {
                Object obj = dataRepository.getVar(value);
                if (obj == null) {
                    trWriter.writeTraceLine(LogLevel.LOGLEV_ERROR,    "RMTD2041",    
                            "Failed to locate the required object ({0}) in data repository. ", value);
                }
                return obj;
            }
            case STRING:
                return value;
            }
            throw new RuntimeException("Unknown property type: " + type);
        }

        public String toString() {
            StringBuffer sb = new StringBuffer("[");
            sb.append("type=").append(type).append(", ").append("name=")
                    .append(name).append(", ").append("value=").append(value)
                    .append(']');
            return sb.toString();
        }
    }

}
