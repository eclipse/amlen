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

package com.ibm.gvt.testdata;

import java.io.File;
import java.io.UnsupportedEncodingException;
import java.net.URI;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

public class DataSource implements Iterable<DataSource.TestData> {
	 
	private static final String TESTDATA = "testdata";
	private static final String TESTDATA_ID = "id";
	private static final String TESTDATA_STATUS = "status";
	private static final String TESTDATA_GVTGUIDE = "GVTGuide";
	private static final String GVTGUIDE_TYPE = "TestArea";
	private static final String TESTDATA_STRING = "String";
	private static final String TESTDATA_HEX = "Hex";
	private static final String STRING_CONTENT_TYPE = "content-type";
	private static final String STRING_CONTENT_TYPE_DEFAULT = "text/plain";
	private static final String HEX_encoding = "encoding";
	private static final String TESTDATA_IMAGE = "image";
	private static final String TESTDATA_IMAGE_URL = "url";
	

	public enum DataType { STRING, HEX };
	public enum BinaryType { UTF_16BE, OTHER };

	Document doc = null;
	
	private static final boolean DEBUG=false;

	public static class TestData implements Comparable<TestData> {

		private int id;
		private String status;
		private String guide;
		private String guidetype;
		private String imageLink;
		
		private DataType dataType;
		
		private String stringValue;
		private String stringType;
		private BinaryType hexType;
		private String hexString = null;
		char[]  hexData;
		String hexencoding;
		char[] hexChars = null; // if not 'other', may be code units

		TestData(Node prod) {
			this.id = Integer.parseInt(XMLUtil.getAttributeValue(prod, TESTDATA_ID));
			this.status = XMLUtil.getAttributeValue(prod, TESTDATA_STATUS);
			
			Node gvtguide = XMLUtil.findChild(prod, TESTDATA_GVTGUIDE);
			this.guidetype = XMLUtil.getAttributeValue(gvtguide, GVTGUIDE_TYPE);
			this.guide = XMLUtil.getNodeValue(gvtguide);
			
			Node image = XMLUtil.findChild(prod, TESTDATA_IMAGE);
			if(image != null) {
				imageLink = XMLUtil.getAttributeValue(image, TESTDATA_IMAGE_URL);
			}
			
			Node string = XMLUtil.findChild(prod, TESTDATA_STRING);
			Node hex = XMLUtil.findChild(prod, TESTDATA_HEX);
			
			if(string != null) {
				dataType = DataType.STRING;
				stringValue = XMLUtil.getNodeValue(string);
				stringType = XMLUtil.getAttributeValue(string, STRING_CONTENT_TYPE);
				if(stringType == null) {
					stringType = STRING_CONTENT_TYPE_DEFAULT;
				}
			} else if(hex != null) {
				dataType = DataType.HEX;
				stringValue = XMLUtil.getNodeValue(hex);
				stringType = XMLUtil.getAttributeValue(hex, STRING_CONTENT_TYPE);
				if(stringType == null) {
					stringType = STRING_CONTENT_TYPE_DEFAULT;
				}
				hexencoding = XMLUtil.getAttributeValue(hex, HEX_encoding);
				if(hexencoding.equals("utf-16be")) {
					hexType = BinaryType.UTF_16BE;
				} else {
					hexType = BinaryType.OTHER;
				}
				
				// decode hex string
				hexData = new char[stringValue.length()/2];
				for(int i=0;i<hexData.length;i++) {
					String ss = stringValue.substring((i*2),(i*2)+2);
					hexData[i] =  (char)Short.parseShort(ss, 16);
					if(DEBUG) System.out.println("#"+i+" = " + hexData[i]+ " ["+ss+"]");
				}

				if(hexencoding.equals("utf-16be")) {
					hexType = BinaryType.UTF_16BE;

					byte b[] = new byte[hexData.length];
					for(int i=0;i<hexData.length;i++) {
						b[i] = (byte)hexData[i];
					}
					try {
						hexString = new String(b,"utf-16be"); // TODO: use icu conv, not java
						hexChars = hexString.toCharArray();
					} catch (UnsupportedEncodingException e) {
						//stringValue = "("+e.toString()+")";
					}
				} else {
					hexType = BinaryType.OTHER;
					hexChars = null; // only available as code units
					hexString = null;
				}			
			}
		}
		
		public byte[] getByteArray() {
			String encoded=getHexCode();
		    if ((encoded.length() % 2) != 0)
		        throw new IllegalArgumentException("Input string must contain an even number of characters");

		    final byte result[] = new byte[encoded.length()/2];
		    final char enc[] = encoded.toCharArray();
		    for (int i = 0; i < enc.length; i += 2) {
		        StringBuilder curr = new StringBuilder(2);
		        curr.append(enc[i]).append(enc[i + 1]);
		        result[i/2] = (byte) Integer.parseInt(curr.toString(), 16);
		    }
		    return result;
		}
		
		
		public String getHexCode() {
			return stringValue;

	}
		
		public String getRawString(){
			return hexString;
		}
		
		
		public String getGuide() {
			return guide;
		}
		public int getId() {
			return id;
		}
		/**
		 * Return whether an item is hex or string.
		 * @return
		 */
		public DataType getDataType() {
			return dataType;
		}
		public BinaryType getHexType() {
			if(dataType == DataType.HEX) {
				return hexType;
			} else {
				throw new UnsupportedOperationException("Can't get the hex type of a "+dataType+" object.");
			}
		}
		/**
		 * Get the String value of a 
		 * @return
		 */
		public String getStringValue() {
			if(dataType == DataType.STRING) {
				return stringValue;
			} else {
				//throw new UnsupportedOperationException("Can't get the string value of a "+dataType+" object.");
				return null;
			}
		}
		
		public String getHexString() {
			if(dataType == DataType.HEX) {
				return hexString;
			} else {
				//throw new UnsupportedOperationException("Can't get the hex string of a "+dataType+" object.");
			return null;
			}
		}


		/**
		 * Get hex data from a hex object
		 * @return
		 */
		public char[] getHexData() {
			if(dataType == DataType.HEX) {
				return hexData;
			} else {
				//throw new UnsupportedOperationException("Can't get hex data from " + dataType);
				return null;
			}
		}
		public String getEncoding() {
				return hexencoding;
		}
		public char[] getHexChars() {
			if(dataType == DataType.HEX) {
				if(hexType != BinaryType.OTHER) {
					return hexChars;
				} else {
					//throw new UnsupportedOperationException("Can't get hex chars from  codepoint " + hexencoding);
					return null;
				}
			} else {
				return null;
				//throw new UnsupportedOperationException("Can't get hex chars from " + dataType);
			}
		}


		@Override
		public int compareTo(TestData arg0) {
			return arg0.id-id;
		}
		
		@Override
		public
		boolean equals(Object arg0) {
			if(arg0==this) return true;
			if(arg0 instanceof TestData) {
				return (compareTo((TestData)arg0)==0);
			} else {
				return false;
			}
		}
		
		@Override
		public int hashCode() {
			return id;
		}
		
		public String getImageLink() {
			return imageLink;
		}


		/**
		 * @return the status
		 */
		public String getStatus() {
			return status;
		}


		/**
		 * @return the guidetype
		 */
		public String getGuidetype() {
			return guidetype;
		}

	}
	
	private URI baseUri;
	
	public DataSource(File baseDir) {
		baseUri = baseDir.toURI();
	}
	public DataSource(URI baseUri) {
		this.baseUri = baseUri;
	}
	
	public Set<TestData> getByGuide(String guide) {
		return getGuideMap().get(guide);
	}
	
	public TestData getById(int id) {
		return getIdMap().get(id);
	}

	private Map<String,Set<TestData>> guideMap = null;
	private Map<Integer,TestData> idMap = null;
	
	
	private Map<Integer, TestData> getIdMap() {
		if(idMap == null) {
			getGuideMap();
		}
		return idMap;
	}

	private Map<String,Set<TestData>> getGuideMap() {
		if(guideMap == null) {			
			Map<String,Set<TestData>> map = new HashMap<String,Set<TestData>>();
			Map<Integer,TestData> newid = new HashMap<Integer,TestData>();
			Document d = getDocument();
			
	        NodeList prods = d.getElementsByTagName(TESTDATA);
	        for(int i=0;i<prods.getLength();i++) {
	        	Node prod = prods.item(i);
	        	
	        	TestData td = new TestData(prod);
	        	newid.put(td.getId(), td);
	        	Set<TestData> sets = map.get(td.getGuide());
	        	if(sets==null) {
	        		sets = new HashSet<TestData>();
	        		map.put(td.getGuide(), sets);
	        	}
	        	sets.add(td);
	        	if(DEBUG) System.err.println("Adding: " + td.guide+"/#"+td.getId());
	        }
	        if(DEBUG) System.err.println("Parsed: " + map.size());
			guideMap = map;
			idMap = newid;
		}
		return guideMap;
	}
	
	public Set<String> getGuideList() {
		return getGuideMap().keySet();
	}
	
	public Collection<TestData> getList() {
		return getIdMap().values();
	}
	
	private Document getDocument() {
		if(doc == null) {
			try {
				DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
				DocumentBuilder docBuilder = factory.newDocumentBuilder();
				
				if(DEBUG) System.err.println("Parsing: " + baseUri);
				doc = docBuilder.parse(baseUri.toString());
			} catch (Exception e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
				throw new InternalError("Could not parse " + baseUri + " - " + e.toString());
			}
		}
		return doc;
	}
	@Override
	public Iterator<TestData> iterator() {
		return getList().iterator();
	}
}
