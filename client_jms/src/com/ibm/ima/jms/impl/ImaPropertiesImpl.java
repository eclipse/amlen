/*
 * Copyright (c) 2013-2021 Contributors to the Eclipse Foundation
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
package com.ibm.ima.jms.impl;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.Serializable;
import java.nio.ByteBuffer;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.Map;
import java.util.Set;

import javax.jms.JMSException;
import javax.naming.BinaryRefAddr;
import javax.naming.RefAddr;
import javax.naming.StringRefAddr;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaProperties;

/*
 * Implement the ImaProperties interface for the IBM MessageSight JMS client.
 * This is used to store properties for the administered objects.
 * 
 */
public class ImaPropertiesImpl implements ImaProperties, Serializable {

	private static final long	serialVersionUID	= -1078593546312641820L;

    public static final String COPYRIGHT = "\n\nCopyright (c) 2014-2021 Contributors to the Eclipse Foundation\n" +
        "See the NOTICE file(s) distributed with this work for additional\n" +
        "information regarding copyright ownership.\n\n" +
        "This program and the accompanying materials are made available under the\n" +
        "terms of the Eclipse Public License 2.0 which is available at\n" +
        "http://www.eclipse.org/legal/epl-2.0\n\n" +
        "SPDX-License-Identifier: EPL-2.0\n\n";


	HashMap<String, Object>	  props;
	HashMap<String, String>	  defaultAdvR;
	HashMap<String, String>	  defaultAdvT;
	int	                      proptype;

	/*
	 * This constructor is used for a factory object
	 */
	ImaPropertiesImpl(int proptype) {
		props = new HashMap<String, Object>();
		this.proptype = proptype;
	}

	/*
	 * This constructor is used when an object is created from a factory
	 */
	ImaPropertiesImpl(ImaProperties imaProps) {
		if (!(imaProps instanceof ImaPropertiesImpl))
			/*
			 * No message code required.  Only errors in internal methods will
			 * cause this.
			 */
			throw new IllegalArgumentException("Input must be of type ImaProperties");
		props = ((ImaPropertiesImpl) imaProps).props;
	}

	/*
	 * Allow the name to be in any case
	 */
	String fixName(String name) {
		String goodname = ImaJms.getCasemap(proptype).get(name.toLowerCase());
		return goodname != null ? goodname : name;
	}

	/*
	 * Clear all properties.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#clear()
	 */
	public void clear() throws JMSException {
		props.clear();
	}

	/*
	 * Check if a property exists.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#exists(java.lang.String)
	 */
	public boolean exists(String name) {
		if (props.containsKey(name))
			return true;
		return props.containsKey(fixName(name));
	}

	/*
	 * Get a property by name.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#get(java.lang.String)
	 */
	public Object get(String name) {
		Object obj = props.get(name);
		if (obj != null)
			return obj;
		return props.get(fixName(name));
	}

	/*
	 * Get the property value as an integer. If the property does not exist, or
	 * cannot be converted to integer, return the default value.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#getInt(java.lang.String, int)
	 */
	public int getInt(String name, int defaultValue) {
		int ret = defaultValue;
		Object obj = get(name);
		if (obj != null) {
			if (obj instanceof Boolean) {
				ret = ((Boolean) obj) ? 1 : 0;
			} else if (obj instanceof Number) {
				ret = ((Number) obj).intValue();
			} else {
				try {
					ret = Integer.parseInt("" + obj);
				} catch (Exception e) {
					/* If this is an enumeration, return the enumerated value */
					HashMap<String, String> validProps = ImaJms
					        .getValidProps(proptype);
					String validator = validProps.get(fixName(name));
					if (validator != null) {
						if (validator.length() > 2
						        && validator.charAt(0) == 'E') {
							ImaEnumeration enumer = null;
							try {
								enumer = (ImaEnumeration) ImaJms.class
								        .getField(validator.substring(2)).get(
								                null);
								ret = enumer.getValue("" + obj);
								if (ret == ImaEnumeration.UNKNOWN)
									ret = defaultValue;
							} catch (Exception ex) {
							}
						} else if (validator.length() > 0
						        && validator.charAt(0) == 'B') {
							ret = ImaJms.Boolean.getValue("" + obj);
							if (ret == ImaEnumeration.UNKNOWN)
								ret = defaultValue;
						} else if (validator.length() > 0
						        && validator.charAt(0) == 'K') {
							try {
								ret = getUnitValue("" + obj);
							} catch (Exception nex) {
							}
						}
					}
				}
			}
		}
		return ret;
	}

	/*
	 * Return the property value as a boolean
	 */
	public boolean getBoolean(String name, boolean deflt) {
		int val = getInt(name, deflt ? 1 : 0);
		return val != 0;
	}

	/*
	 * Return the property value as a string.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#getString(java.lang.String)
	 */
	@SuppressWarnings("deprecation")
	public String getString(String name) {
		String ret = null;
		Object obj = get(name);
		if (obj != null) {
			if (obj instanceof byte[]) {
				try {
					ByteBuffer bb = ByteBuffer.wrap((byte[]) obj);
					ret = ImaUtils.fromUTF8(bb, bb.limit(), true);
				} catch (Exception e) {
					ret = "\uffff" + new String((byte[]) obj, 0);
				}
			} else {
				ret = "" + obj;
			}
		}
		return ret;
	}

	/*
	 * Return the set of property names. This is not linked to the original.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#propertySet()
	 */
	public Set<String> propertySet() {
		HashSet<String> set = new HashSet<String>();
		set.addAll((Set<String>) props.keySet());
		return set;
	}

	/*
	 * Put a property
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#put(java.lang.String,
	 * java.lang.Object)
	 */
	public Object put(String name, Object value) throws JMSException {
		if (value != null && !(value instanceof Serializable)) {
			ImaJmsExceptionImpl jex = new ImaJmsExceptionImpl("CWLNC0301",
			        "A call to the ImaProperties put() method failed because the value for property {0} is not serializable.", name);
			ImaTrace.init();
			ImaTrace.traceException(2, jex);
			throw jex;
		}
		return props.put(fixName(name), value);
	}

	/*
	 * Put a string property
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#put(java.lang.String,
	 * java.lang.String)
	 */
	public String put(String name, String value) {
		return (String) props.put(fixName(name), value);
	}
	
	/*
     * Put a boolean property
     */
    public void put (String name, boolean value) {
        props.put(fixName(name), value);
    }

	/*
	 * Package private method to allow any value to be modified. This overcomes
	 * the null and the serializable check.
	 */
	void putInternal(String name, Object value) {
		props.put(name, value);
	}

	/*
	 * Put a set of properties.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#putAll(java.util.Map)
	 */
	public void putAll(Map<String, Object> map) throws JMSException {
		Iterator<String> it = map.keySet().iterator();
		while (it.hasNext()) {
			String key = it.next();
			put(key, map.get(key));
		}
	}

	/*
	 * Remove a property.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#remove(java.lang.String)
	 */
	public Object remove(String name) {
		return props.remove(fixName(name));
	}

	/*
	 * Return the number of properties.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#size()
	 */
	public int size() {
		return props.size();
	}

	/*
	 * Validate all of the properties.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#validate()
	 */
	public ImaJmsException[] validate(boolean nowarn) {
		HashSet<ImaJmsException> v = null;
		Iterator<String> it = props.keySet().iterator();
		while (it.hasNext()) {
			String name = it.next();
			try {
				validate(name, nowarn);
			} catch (JMSException jmsex) {
				if (v == null)
					v = new HashSet<ImaJmsException>();
				if (!(jmsex instanceof ImaJmsException))
					jmsex = new ImaJmsExceptionImpl("CWLNC0313", jmsex,
					        "ImaProperties validation failed for property {0} due to an unexpected exception.",
					        name);
				v.add((ImaJmsException) jmsex);
			}
		}
		return v == null ? null : v.toArray(new ImaJmsException[0]);
	}

	/*
	 * Validate a single property
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#validate(java.lang.String)
	 */
	public void validate(String name, boolean nowarn) throws JMSException {
		validate(name, props.get(name), nowarn);
	}

	/*
	 * Validate a single property
	 * 
	 * @see
	 * com.ibm.ima.jms.ImaProperties#validate(java.lang.String,java.lang.Object)
	 */
	public void validate(String name, Object value, boolean nowarn)
	        throws JMSException {
		HashMap<String, String> validProps = ImaJms.getValidProps(proptype);
		String validator = validProps.get(fixName(name));
		ImaJmsPropertyException jmsex = null;

		if (validator == null || validator.length() < 1) {
			ImaJmsPropertyException jex = new ImaJmsPropertyException(
			        "CWLNC0303",
			        "ImaProperties validation failed for property {0} because the validator string is null or is an empty string.", name);
			ImaTrace.init();
			ImaTrace.traceException(2, jex);
			throw jex;
		}

		/* Ignore the two lead bytes for extended properties */
		if (validator.charAt(0) == 'X' && validator.length() > 2)
			validator = validator.substring(2);

		switch (validator.charAt(0)) {

		/* Boolean */
		case 'B':
			jmsex = validateBoolean(name, value);
			break;

		/* String */
		case 'S':
			char stringtype = validator.length() > 1 ? validator.charAt(1)
			        : ' ';
			switch (stringtype) {
			/* Destination */

			/* Name */
			case 'N':
				if (!ImaMessage.isValidName("" + value))
					jmsex = new ImaJmsPropertyException("CWLNC0302",
					        "ImaProperties validation failed for the property {0} because the value {1} is not a valid name.",
					        name, value);
				break;

			/* SQL */
			case 'Q':
				try {
					new ImaMessageSelector("" + value);
				} catch (JMSException jmse) {
					jmsex = new ImaJmsPropertyException("CWLNC0315", jmse,
					        "ImaProperties validation failed for the property {0} because {1} is not a valid selector."
					                + name, value);
				}
				break;
			}
			break;

		/* byte array */
		case '[': /* No byte array validation for now */
			break;

		/* Integer or unsigned integer */
		case 'I':
		case 'K':
		case 'U':
			if (validator.length() > 2)
				jmsex = validateInt(name, validator, value);
			break;

		/* Enumeration */
		case 'E':
			if (validator.length() > 2 && validator.charAt(1) == ':')
				jmsex = validateEnumeration(name, validator.substring(2), value);
			break;

		/* case XX or XT. no validation */
		case 'X':
			break;

		/* case Z: system property */
		case 'Z':
			/*
			 * Give a warning if a system property is set. This is not really an
			 * error and the system properties are unlikely to be confused with
			 * other properties, but the value may be modified by the system
			 * within the derived objects.
			 */
			if (!nowarn)
				jmsex = new ImaJmsPropertyException("CWLNC0314",
				        "ImaProperties validation reported a warning for property {0} because it is a system property.", name);
		}
		if (jmsex != null)
			throw jmsex;
	}

	/*
	 * Validate an enumeration.
	 */
	public ImaJmsPropertyException validateEnumeration(String name,
	        String enumeration, Object value) {
		ImaEnumeration enumer = null;
		ImaJmsPropertyException jmsex;
		boolean valid;
		try {
			enumer = (ImaEnumeration) ImaJms.class.getField(enumeration).get(
			        null);
		} catch (Exception e) {
		}
		if (enumer == null) {
			jmsex = new ImaJmsPropertyException(
			        "CWLNC0304",
			        "The property {0} is of an enumeration {1} that is not known.",
			        name, enumeration);
			return jmsex;
		}
		if (value == null)
			return new ImaJmsPropertyException("CWLNC0305",
			        "ImaProperties validation failed for property {0} because the property value was not set.", name);
		if (value instanceof Number) {
			valid = enumer.getName(((Number) value).intValue()) != null;
		} else {
			valid = enumer.getValue("" + value) != ImaEnumeration.UNKNOWN;
		}
		if (valid)
			return null;
		jmsex = new ImaJmsPropertyException("CWLNC0306",
		        "A call to createConnection() or to the ImaProperties validate() method failed because the property {0} was set to {2}.  The value must be one of: {1}.",
		        name, enumer.getNameString(), value);
		return jmsex;
	}

	/*
	 * Add a valid property.
	 * 
	 * @see com.ibm.ima.jms.ImaProperties#addValidProperty(java.lang.String,
	 * java.lang.String)
	 */
	public void addValidProperty(String name, String validator) {
		if (validator == null || validator.length() < 1) {
			ImaIllegalArgumentException iex = new ImaIllegalArgumentException("CWLNC0316",
			        "A call to the ImaProperties addValidProperty() method failed because the validator string is null or is an empty string.");
			ImaTrace.init();
			ImaTrace.traceException(2, iex);
			throw iex;
		}
		int len = validator.length();
		switch (validator.charAt(0)) {
		case 'I':
		case 'U':
			if (len < 2 || validator.charAt(1) != ':')
				validator = validator.substring(0, 1);
			break;
		case 'B':
			validator = validator.substring(0, 1);
			break;
		case 'S':
			if (len < 3 || validator.charAt(1) != ':')
				validator = validator.substring(0, 1);
			else
				validator = validator.substring(0, 1)
				        + validator.substring(2, 3);
			break;
		case 'X': /* Not supported for external API consumers */
		case 'E': /* Not supported for external API consumers */
		case 'K': /* Not supported for external API consumers */
		default:
			ImaIllegalArgumentException iex = new ImaIllegalArgumentException(
			        "CWLNC0317", "A call to the ImaProperties addValidProperty() method failed because the validator {0} is not valid.", validator);
			ImaTrace.init();
			ImaTrace.traceException(2, iex);
			throw iex;
		}
		ImaJms.addValidProperty(proptype, name, validator);
	}

	/*
	 * Validate a boolean value
	 */
	public ImaJmsPropertyException validateBoolean(String name, Object value) {
		ImaJmsPropertyException jmsex;
		if (value == null) {
			jmsex = new ImaJmsPropertyException("CWLNC0305",
			        "ImaProperties validation failed for property {0} because the property value was not set.", name);
			return jmsex;
		}
		if (value instanceof Boolean || value instanceof Integer)
			return null;
		if (value instanceof String) {
			return ImaJms.Boolean.getValue((String) value) != ImaEnumeration.UNKNOWN ? null
			        : new ImaJmsPropertyException(
			                "CWLNC0307",
			                "ImaProperties validation failed for Boolean property {0} because the value {1} is not a valid Boolean string.",
			                name, value);

		}
		jmsex = new ImaJmsPropertyException(
		        "CWLNC0308",
		        "ImaProperties validation failed for Boolean property {0} because {1} is not a valid Boolean value.",
		        name, value);
		return jmsex;
	}

	/*
	 * Get a value with a K or M suffix
	 */
	int getUnitValue(String s) {
		int val = 0;
		for (int i = 0; i < s.length(); i++) {
			char ch = s.charAt(i);
			if (ch >= '0' && ch <= '9') {
				val = val * 10 + (ch - '0');
			} else {
				if (i + 1 == s.length()) {
					if (ch == 'k' || ch == 'K') {
						val *= 1024;
					} else if (ch == 'm' || ch == 'M') {
						val *= (1024 * 1024);
					} else {
						/* 
						 * This error cannot be reached via the public API (K validator is 
						 * not supported externally) so no message catalog error is required for it.
						 */
						NumberFormatException nex = new NumberFormatException(
						        "The string '" + s + "' is not a valid value. It must have a K (Kilobytes) or M (Megabytes) suffix.");
						ImaTrace.init();
						ImaTrace.traceException(2, nex);
						throw nex;
					}
				} else {
					/* 
					 * This error cannot be reached via the public API (K validator is 
					 * not supported externally) so no message catalog error is required for it.
					 */
					NumberFormatException nex = new NumberFormatException(
					        "The string '" + s + "' is not a valid value. It must have a K (Kilobytes) or M (Megabytes) suffix.");
					ImaTrace.init();
					ImaTrace.traceException(2, nex);
					throw nex;
				}
			}
		}
		return val;
	}

	/*
	 * Validate an integer value
	 */
	public ImaJmsPropertyException validateInt(String name, String validator,
	        Object value) {
		ImaJmsPropertyException jmsex;
		int val = 0;
		if (value == null) {
			jmsex = new ImaJmsPropertyException("CWLNC0305",
			        "ImaProperties validation failed for property {0} because the property value was not set.", name);
			return jmsex;
		}

		/*
		 * Determine the value of the property
		 */
		if (value instanceof Number) {
			val = ((Number) value).intValue();
		} else if (value instanceof String) {
			try {
				if (validator.charAt(0) == 'K') {
					val = getUnitValue((String) value);
				} else {
					val = Integer.valueOf((String) value);
				}
			} catch (Exception e) {
				jmsex = new ImaJmsPropertyException(
				        "CWLNC0309",
				        "ImaProperties validation failed for integer property {0} because {1} is not an integer.",
				        name, value);
				return jmsex;
			}
		}
		/*
		 * If a validator is specified, parse it and run the validator
		 */
		if (validator != null) {
			if (val < 0 && validator.charAt(0) == 'U') {
				jmsex = new ImaJmsPropertyException(
				        "CWLNC0310",
				        "ImaProperties validation failed for unsigned numeric property {0} because {1} is a negative value and is not valid.",
				        name, val);
				return jmsex;
			}
			if (validator.length() > 1 && validator.charAt(1) == ':') {
				int minval = 0;
				int maxval = Integer.MAX_VALUE;
				int len = validator.length();
				int pos = 2;
				boolean neg = false;
				if (pos < len && validator.charAt(pos) == '-') {
					neg = true;
					pos++;
				}
				while (pos < len) {
					char ch = validator.charAt(pos++);
					if (ch >= '0' && ch <= '9') {
						minval = minval * 10 + (ch - '0');
					} else {
						if (ch == ':') {
							maxval = 0;
							break;
						}
					}
				}
				while (pos < len) {
					char ch = validator.charAt(pos++);
					if (ch >= '0' && ch <= '9') {
						maxval = maxval * 10 + (ch - '0');
					}
				}
				if (neg)
					minval = -minval;
				if (val < minval) {
					jmsex = new ImaJmsPropertyException(
					        "CWLNC0311",
					        "ImaProperties validation failed for numeric property {0} because the value {1} is less than the minimum value {2}.",
					        name, val, minval);
					return jmsex;
				}
				if (val > maxval) {
					jmsex = new ImaJmsPropertyException(
					        "CWLNC0312",
					        "ImaProperties validation failed for numeric property {0} because the value {1} is greater than the maximum value {2}.",
					        name, val, maxval);
					return jmsex;
				}
			}
		}
		return null;
	}

	/*
	 * Set a default advanced receive config item.
	 * 
	 * These items do not need to be in the list of valid properties, but should
	 * always be of the correct case if they are since the case independent
	 * processing is not done.
	 */
	public void setAdvancedReceive(String key, String val) {
		if (defaultAdvR == null)
			defaultAdvR = new HashMap<String, String>();
		defaultAdvR.put(key, val);
	}

	/*
	 * Set a default advanced transmit config item.
	 * 
	 * These items do not need to be in the list of valid properties, but should
	 * always be of the correct case if they are since the case independent
	 * processing is not done.
	 */
	public void setAdvancedTransmit(String key, String val) {
		if (defaultAdvT == null)
			defaultAdvT = new HashMap<String, String>();
		defaultAdvT.put(key, val);
	}

	/*
	 * Construct a RefAddr from a property.
	 * 
	 * The object Boolean, Integer, Long, Float, and Double are represented as
	 * string by appending a type byte in front of the value. Strings are
	 * represented unchanged unless they contain a colon in the second byte in
	 * which case "S:" is appended in front of the value. All other serializable
	 * objects are serialized, and any other object is referenced by name.
	 */
	public RefAddr getRefAddr(String name) {
		RefAddr ret = null;
		Object obj = get(name);
		if (obj != null) {
			if (obj instanceof String) {
				String s = (String) obj;
				if (s.length() > 1 && s.charAt(1) == ':')
					s = "S:" + s;
				ret = new StringRefAddr(name, s);
			} else if (obj instanceof Integer) {
				ret = new StringRefAddr(name, "I:" + obj);
			} else if (obj instanceof Boolean) {
				ret = new StringRefAddr(name, "B:" + obj);
			} else if (obj instanceof Long) {
				ret = new StringRefAddr(name, "L:" + obj);
			} else if (obj instanceof Double) {
				ret = new StringRefAddr(name, "D:" + obj);
			} else if (obj instanceof Float) {
				ret = new StringRefAddr(name, "F:" + obj);
			} else if (obj instanceof Serializable) {
				ByteArrayOutputStream baos = new ByteArrayOutputStream();
				try {
					ObjectOutputStream oos = new ObjectOutputStream(baos);
					oos.writeObject(obj);
					ret = new BinaryRefAddr(name, baos.toByteArray());
				} catch (Exception e) {
				}
			}

			/*
			 * If not a known object and not serializable, use its string form
			 */
			if (ret == null) {
				String s = "" + obj;
				if (s.length() > 1 && s.charAt(1) == ':')
					s = "S:" + s;
				ret = new StringRefAddr(name, s);
			}
		}
		return ret;
	}

	/*
	 * Set a property from a RefAddr. Any problem in converting a value will
	 * result in an exception.
	 */
	public void putRefAddr(RefAddr ref) throws Exception {
		if (ref == null)
			return;

		/*
		 * Convert from a string to common objects
		 */
		Object obj = ref.getContent();
		if (obj instanceof String) {
			String s = (String) obj;
			if (s.length() > 1 && s.charAt(1) == ':') {
				char kind = s.charAt(0);
				s = s.substring(2);
				switch (kind) {
				case 'B':
					obj = Boolean.valueOf(s);
					break;
				case 'D':
					obj = new Double(s);
					break;
				case 'F':
					obj = new Float(s);
					break;
				case 'I':
					obj = new Integer(s);
					break;
				case 'L':
					obj = new Long(s);
					break;
				case 'S':
					obj = (Object) s;
					break;
				}
			}
		}

		/*
		 * All other objects are store serialized
		 */
		else {
			ByteArrayInputStream bais = new ByteArrayInputStream((byte[]) obj);
			ObjectInputStream ois = new ObjectInputStream(bais);
			obj = ois.readObject();
		}

		/*
		 * Put the object into the properties
		 */
		if (obj != null)
			put(ref.getType(), obj);
	}

	/*
	 * @see java.lang.Object#toString()
	 */
	public String toString() {
		return props.toString();
	}

	public String getClassName() {
		return "ImaProperties";
	}

	public String getDetails() {
		return null;
	}

	public String getLinks() {
		return null;
	}

	public String toString(int opt) {
		return toString(opt, this);
	}

	/*
     * 
     */
	public String toString(int opt, ImaPropertiesImpl props) {
		StringBuffer sb = new StringBuffer();
		boolean started = false;

		if (props == null) {
			return "";
		}

		if ((opt & ImaConstants.TS_Class) != 0) {
			sb.append(props.getClassName());
			started = true;
		}
		if ((opt & ImaConstants.TS_HashCode) != 0) {
			sb.append('@').append(Integer.toHexString(hashCode()));
			started = true;
		}

		if ((opt & (ImaConstants.TS_Info | ImaConstants.TS_Properties)) != 0) {
			if (started)
				sb.append(' ');
			started = true;
			sb.append("properties=").append(props.props);
		}

		if ((opt & ImaConstants.TS_Details) != 0) {
			String details = props.getDetails();
			if (details != null) {
				if (started)
					sb.append("\n  ");
				started = true;
				sb.append(details);
			}
		}
		if ((opt & ImaConstants.TS_Links) != 0) {
			String links = props.getLinks();
			if (links != null) {
				if (started)
					sb.append("\n  ");
				started = true;
				sb.append(links);
			}

		}
		return sb.toString();
	}
}
