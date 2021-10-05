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
package com.ibm.ism.ws.test;

import java.util.HashMap;
import java.util.Map;
import java.util.Random;

import org.w3c.dom.Element;

public class CreateRandomStringAction extends ApiCallAction {
	private final 	String 					_structureID;
	private final	int						_length;
	private final	String					_validCharacters;
	private final	Map<String, Integer>	_charMax;


	/**
	 * @param config
	 * @param dataRepository
	 * @param trWriter
	 */
	public CreateRandomStringAction(Element config,
			DataRepository dataRepository, TrWriter trWriter) {
		super(config, dataRepository, trWriter);
		_structureID = _actionParams.getProperty("structure_id");
		if (_structureID == null) {
			throw new RuntimeException("structure_id is not defined for "
					+ this.toString());
		}
		String temp = _actionParams.getProperty("length");
		if (null == temp) {
			throw new RuntimeException("length is not defined for "
					+this.toString());
		}
		_length = Integer.parseInt(temp);
		
		temp = _actionParams.getProperty("validCharacters");
		if (null == temp) {
			_validCharacters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890`~!@$%^&*()[]{};':\",./<>?=_-";
		} else {
			_validCharacters = temp;
		}
		temp = _actionParams.getProperty("characterMax");
		if (null != temp) {
			_charMax = new HashMap<String, Integer>();
			String[] pieces = temp.split(";");
			for (int i = 0; i< pieces.length; i++) {
				String key = pieces[i].substring(0, 1);
				_charMax.put(key, new Integer(pieces[i].substring(1)));
			}
		} else {
			_charMax = null;
		}
	
	}

	private Map<String, Integer> charCounts;
	
	private boolean lessThanMax(char c) {
		if (null == _charMax) {
			return true;
		}
		char[] tempChars = new char[1];
		tempChars[0] = c;
		String tempString = new String(tempChars);
		Integer maxCount = _charMax.get(tempString);
		if (null == maxCount) {
			return true;
		}
		if (maxCount > 0) {
			Integer currCount = charCounts.get(tempString);
			if (null == currCount) {
				currCount = new Integer(1);
				charCounts.put(tempString, currCount);
				return true;
			}
			if (currCount < maxCount) {
				currCount++;
				charCounts.put(tempString, currCount);
				return true;
			}
		}
		return false;
	}
	
	/*private int countOccurrances(String string, char thischar) {
		int count = 0;
		int position = 0;
		int nextPosition;
		while (-1 != (nextPosition = string.indexOf(thischar, position))) {
			position = nextPosition + 1;
			count++;
		}
		return count;
	}*/
	
	protected boolean invokeApi() throws IsmTestException {
		if (null != _charMax) {
			charCounts = new HashMap<String, Integer>();
		}
		String string;
		char[] chars = new char[_length];
		int charopts = _validCharacters.length();
		Random rand = new Random();
		for (int i=0; i<_length; i++) {
			char newChar;
			int j;
			do {
				j = rand.nextInt(charopts);
				newChar = _validCharacters.charAt(j);
				// Don't allow '$' as first character
				if (0 == i) {
					while ('$' == newChar) {
						j = rand.nextInt(charopts);
						newChar = _validCharacters.charAt(j);
					}
				}
			} while (!lessThanMax(newChar));
			chars[i] = newChar;
		}
		string = new String(chars);
		//int count = countOccurrances(string, '/');
		//System.out.println("Found "+count+" occurances of '/'");
		_dataRepository.storeVar(_structureID, string);
		return true;
	}

}
