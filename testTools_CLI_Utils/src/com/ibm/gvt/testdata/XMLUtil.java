/*
 * Copyright (c) 2004-2021 Contributors to the Eclipse Foundation and others
 * Major Portions from org.unicode.cldr.util.LDMLUtilities
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
package com.ibm.gvt.testdata;

import org.w3c.dom.*;

public class XMLUtil {

//    /**
//     * Decide if the node is text, and so must be handled specially 
//     * @param n
//     * @return
//     */
//    private static boolean isTextNode(Node n) {
//      if (n == null)
//        return false;
//      short nodeType = n.getNodeType();
//      return nodeType == Node.CDATA_SECTION_NODE || nodeType == Node.TEXT_NODE;
//    }   
    public static Node getAttributeNode(Node sNode, String attribName){
        NamedNodeMap attrs = sNode.getAttributes();
        if(attrs!=null){
           return attrs.getNamedItem(attribName);
        }
        return null;
    }
    /**
     * Utility method to fetch the attribute value from the given 
     * element node
     * @param sNode
     * @param attribName
     * @return
     */
    public static String getAttributeValue(Node sNode, String attribName){
        String value=null;
        NamedNodeMap attrs = sNode.getAttributes();
        if(attrs!=null){
            Node attr = attrs.getNamedItem(attribName);
            if(attr!=null){
                value = attr.getNodeValue();
            }
        }
        return value;
    }

    /**
     * Utility method to fetch the value of the element node
     * @param node
     * @return
     */
    public static String getNodeValue(Node node){
        for(Node child=node.getFirstChild(); child!=null; child=child.getNextSibling() ){
            if(child.getNodeType()==Node.TEXT_NODE){
                return child.getNodeValue();
            }
        }
        return null;
    }

    public static Node findChild(Node node, String chName){
        for(Node child=node.getFirstChild(); child!=null; child=child.getNextSibling() ){
            if(child.getNodeType()==Node.ELEMENT_NODE && child.getNodeName().equals(chName)){
                return child;
            }
        }
        return null;
    }
    
}
