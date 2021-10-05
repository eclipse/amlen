// Copyright (c) 2009-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
package com.ibm.ism.tools.msg;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Hashtable;
import java.util.List;
import java.util.StringTokenizer;

public class ISMFilterManager {

	private static final String FILE_FILTER_NAME="skipfiles.txt";
	private static final String MSGID_FILTER_NAME="tmsskipmsgids.txt";
	private static final String MSGID_RESERVE_NAME = "msgidsreserved.txt";
	private static List<String> skipFileList = null;
	private static List<String> skipMsgIDList = null;
	private static Hashtable<String, List<String>> reservedIDTable = null;

	static {
		reservedIDTable = new Hashtable<String, List<String>>();
		List<String> tempMsgIDList = new ArrayList<String>();

		// Add TRANSPORT
		for (int i = Constants.TRANSPORT_MSG_KEY_MIN; i <= Constants.TRANSPORT_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("TRANSPORT", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add PROTOCOL
		for (int i = Constants.PROTOCOL_MSG_KEY_MIN; i <= Constants.PROTOCOL_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("PROTOCOL", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add STORE
		for (int i = Constants.STORE_MSG_KEY_MIN; i <= Constants.STORE_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("STORE", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add ADMIN
		for (int i = Constants.ADMIN_MSG_KEY_MIN; i <= Constants.ADMIN_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("ADMIN", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add GUI
		for (int i = Constants.GUI_MSG_KEY_MIN; i <= Constants.GUI_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("GUI", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add MONITORING
		for (int i = Constants.MONITORING_MSG_KEY_MIN; i <= Constants.MONITORING_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("MONITORING", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add MQCONNECTIVITY
		for (int i = Constants.MQCONNECTIVITY_MSG_KEY_MIN; i <= Constants.MQCONNECTIVITY_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("MQCONNECTIVITY", tempMsgIDList);
		}
		tempMsgIDList.clear();

		// Add UTILS
		for (int i = Constants.UTILS_MSG_KEY_MIN; i <= Constants.UTILS_MSG_KEY_MAX; i++) {
			tempMsgIDList.add(Integer.toString(i));
			reservedIDTable.put("UTILS", tempMsgIDList);
		}
		tempMsgIDList.clear();
	}

	public static List<String> getSkippedFilesList(){

		if(skipFileList!=null){
			return skipFileList;
		}

		skipFileList = new ArrayList<String>();

		File fileFilter = new File(FILE_FILTER_NAME);
		if(!fileFilter.exists()){
			return skipFileList;
		}

		try{

			BufferedReader in = new BufferedReader(new FileReader(fileFilter));

			String inline=null;
			while ((inline = in.readLine()) != null) {
				if(inline.startsWith("#")){
					continue;
				}
				inline=inline.trim();
				skipFileList.add(inline);

			}

		}catch(IOException ioexception){
			ioexception.printStackTrace();
		}

		return skipFileList;

	}
	/**
	 * Get the list of message ID that needed to be skipped.
	 * @return list of msg IDs
	 */
	public static List<String> getSkippedMsgIDList(){

		if(skipMsgIDList!=null){
			return skipMsgIDList;
		}

		skipMsgIDList = new ArrayList<String>();

		File msgIDFilter = new File(MSGID_FILTER_NAME);
		if(!msgIDFilter.exists()){
			return skipMsgIDList;
		}

		try{

			BufferedReader in = new BufferedReader(new FileReader(msgIDFilter));

			String inline=null;
			while ((inline = in.readLine()) != null) {
				if(inline.startsWith("#")){
					continue;
				}
				inline=inline.trim();
				skipMsgIDList.add(inline);

			}

		}catch(IOException ioexception){
			ioexception.printStackTrace();
		}

		return skipMsgIDList;

	}

	public static List<String> getComponentReservedIDs(ISMLogEntryObject.LOG_ENTRY_COMPONENT_TYPE compType){
		if(reservedIDTable==null){
			//Initialize the table
			getComponentReservedIDsFromFile();

		}
		if(compType==null || reservedIDTable==null)return null;

		return reservedIDTable.get(compType.name());

	}

	private static void getComponentReservedIDsFromFile(){
		File reserveFile = new File(MSGID_RESERVE_NAME);
		if(!reserveFile.exists()){
			return;
		}

		try{

			BufferedReader in = new BufferedReader(new FileReader(reserveFile));

			String inline=null;
			while ((inline = in.readLine()) != null) {
				if(inline.startsWith("#")){
					continue;
				}
				//Parse the component name
				//Get the startID
				//Get the endID;
				//Put each number in the range in the list.
				if(inline.indexOf("=")>0){
					StringTokenizer tokens = new StringTokenizer(inline,"=");
					String compName = tokens.nextToken();
					ISMLogEntryObject.LOG_ENTRY_COMPONENT_TYPE lineCompType =ISMLogEntryObject.getComponentType(compName);

					String msgIDRange = null;
					String startIDStr = null;
					String endIDStr = null;
					if(tokens.hasMoreTokens()){
						msgIDRange = tokens.nextToken();
					}
					//Check if there is an end ID
					if(msgIDRange.indexOf(":")>0){
						StringTokenizer rangeTokens = new StringTokenizer(msgIDRange, ":");
						startIDStr = rangeTokens.nextToken();
						if(rangeTokens.hasMoreTokens()){
							endIDStr = rangeTokens.nextToken();
						}
					}else{
						startIDStr = msgIDRange;
					}

					//If there is start and end ID. Parse and put them in the list
					if(startIDStr!=null){
						startIDStr = startIDStr.trim();
						//Convert startID string to number.
						List<String> tempMsgIDList = new ArrayList<String>();
						int startIDNum = -1;
						try{
							startIDNum = Integer.parseInt(startIDStr);
						}catch(Exception exception){
							System.err.println("The line contained invalid reserve start or end ID: "+ inline);
							continue; //Continue with next line
						}

						if(endIDStr!=null){
							//Only put start in the list
							endIDStr = endIDStr.trim();
							int endIDNum = -1;
							try{
								endIDNum = Integer.parseInt(endIDStr);
							}catch(Exception exception){
								System.err.println("The line contained invalid reserve start or end ID: "+ inline);
								continue; //Continue with next line
							}
							if(endIDNum > startIDNum){

								for(int tmpID=startIDNum; tmpID<=endIDNum; tmpID++){
									tempMsgIDList.add("" + tmpID);
								}

							}
						}else{
							if(startIDNum>0)tempMsgIDList.add(startIDNum+"");
						}
						//Merge with list in the hash table
						if(tempMsgIDList.size()>0){
							if(reservedIDTable==null){
								reservedIDTable = new Hashtable<String, List<String>>();
							}
							List<String>origReserveList = reservedIDTable.get(lineCompType.name());
							if(origReserveList!=null){
								//Merge
								tempMsgIDList.removeAll(origReserveList);
								origReserveList.addAll(tempMsgIDList);


							}else{
								reservedIDTable.put(lineCompType.name(), tempMsgIDList);
							}
						}
					}
				}

			}

		}catch(IOException ioexception){
			ioexception.printStackTrace();
		}




	}

}
