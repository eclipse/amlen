// Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package svt.mqtt.mq;

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.List;
import java.io.BufferedReader;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.lang.NullPointerException;
//
//import static org.grep4j.core.Grep4j.grep;
//import static org.grep4j.core.Grep4j.constantExpression;
//import static org.grep4j.core.options.Option.*;
//import static org.grep4j.core.fluent.Dictionary.executing;
//import static org.grep4j.core.fluent.Dictionary.on;
//import static org.grep4j.core.fluent.Dictionary.options;
//import static org.grep4j.core.options.Option.countMatches;
//import static org.grep4j.core.options.Option.maxMatches;
//import org.grep4j.core.model.Profile;
//import org.grep4j.core.model.ProfileBuilder;
//import org.grep4j.core.result.GrepResult;
//import org.grep4j.core.result.GrepResults;

//import svt.common.Constants;


public class MqttIoTClient implements Runnable {

	List<String[]> lines = new ArrayList<String[]>();
	List<String> passthru_args = new ArrayList<String>();
	String[] NEW_ARGS=null;				// MqttSample args to pass thru
	int NEW_INDEX=0;

	int MQTT_ORG_INDEX=0;				// CLI -Z : Orgs# to start with
	int MQTT_ORG_INSTANCES=1;			// CLI -z : How many ORGs to start
	int MQTT_SUB_INSTANCES=1;			// CLI -y : How many SUBs in a Org to start
	int MQTT_PUB_INSTANCES=1;			// CLI -x : How many PUBs in a Org to start
	int MQTT_PUB_MSGS=100;				// CLI -n : How many messages to publish
	Boolean USE_DEFAULT_MSG=true;       // CLI -m : Message text to publish

	int MQTT_SUB_MSGS=MQTT_PUB_MSGS*MQTT_PUB_INSTANCES;			// RECOMPUTED after iotParseArgs()

	int APIKEY_ORG_MEMBERS=10;			// Number of SUB UIDs per ORG
	int DEVICE_ORG_MEMBERS=10;			// Number of PUB UIDs per ORG

	String  TARGET_ENV="wdc01-6";		//  ex. wdc01-6  SoftLayer Stand 
	//  String  MQTT_SERVER="tcp://198.11.242.190:1883";  //MSProxy in wdc01-6						
	String[][] APIKEYS=null;			// Subscriber Credentials
	String[][] DEVICES=null;			// Publisher Credentials

	static String MQTT_ACTION_SUB="subscribe";
	static String MQTT_ACTION_PUB="publish";

	String LOG_FINALE_PASS="PASSED";
	String LOG_FINALE_FAIL="FAILED";
	int LOG_ERROR_COUNT=0;
	Boolean DEBUG=true;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// Picks off the -S, -x, -y, -z, and -Z parameters, launches MqttSample with remaining params
		new MqttIoTClient(args).run();
	}

	/**
	 * @param args
	 */
	public MqttIoTClient(String[] args) {
		// Pick off the -S, -x, -y, -z, and -Z parameters, unknown to MqttSample
		iotParseArgs(args);
		// Build the Credential Arrays from /var/log/TARGET_ENV.*_master
		build_device_array();
		build_apikey_array();

	}



	private void iotParseArgs(String[] args) {
		passthru_args=new ArrayList<String>();

		for (int i = 0; i < args.length; i++) {
			// -S SoftLayer Stand to test with
			if ( "-S".equals(args[i]) && (i + 1 < args.length)) {
				passthru_args.add( "-s" );

				i++;
				TARGET_ENV = args[i];
				passthru_args.add( nslookup_proxy() );

			} else if ( "-x".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				MQTT_PUB_INSTANCES=Integer.parseInt(args[i]);

			} else if ( "-y".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				MQTT_SUB_INSTANCES=Integer.parseInt(args[i]);

			} else if ( "-z".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				MQTT_ORG_INSTANCES=Integer.parseInt(args[i]);

			} else if ( "-Z".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				MQTT_ORG_INDEX=Integer.parseInt(args[i]);

			} else  if ( "-n".equals(args[i])  && (i + 1 < args.length)) {
				i++;
				MQTT_PUB_MSGS=Integer.parseInt(args[i]);

				//  Args to Pass on to MqttSample, Some I also use...
			} else {
				if ( "-d".equals(args[i])  && (i + 1 < args.length)) {
					if ( args[i+1].equalsIgnoreCase("TRUE") ){
						DEBUG=true;
					} else{
						DEBUG=false;
					}
				}

				if ( "-m".equals(args[i]) ){
					USE_DEFAULT_MSG=false;
				}

				passthru_args.add( args[i] );
				if (i+1 < args.length){
					i++;
					passthru_args.add( args[i] );
				}
			}
		}  

		// RECOMPUTE MQTT_SUB_MSGS in case PUB parameters were updated, CLI -n.
		MQTT_SUB_MSGS=MQTT_PUB_MSGS * MQTT_PUB_INSTANCES;
		System.out.println(passthru_args.size());
	}

	/**
	 * @param args
	 */
	public void run() {
		//#  use "-x option" declare -i MQTT_PUB_INSTANCES
		//#  use "-y option" declare -i MQTT_SUB_INSTANCES
		//#  use "-z & -Z option'   MQTT_ORG_INSTANCES and start at MQTT_INSTANCE_INDEX

		ArrayList<Thread> subThreads = new ArrayList<Thread>();
		ArrayList<Thread> pubThreads = new ArrayList<Thread>();
		int ORGS_STARTED=0;
		int ORG_INDEX=0;
		int DEVICES_STARTED=0;
		int APIKEYS_STARTED=0;
		int ARRAY_INDEX=0;

		while ( ORGS_STARTED < MQTT_ORG_INSTANCES )   { 
			//# Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
			ORG_INDEX=( ORGS_STARTED + MQTT_ORG_INDEX )* DEVICE_ORG_MEMBERS;
			System.out.println("Start ORG["+ORGS_STARTED+"] of "+MQTT_ORG_INSTANCES+" ORGs starting at ORG "+MQTT_ORG_INDEX);

			//# Start the number of SUBs specifed by MQTT_SUB_INSTANCES
			while ( APIKEYS_STARTED < MQTT_SUB_INSTANCES ) {

				ARRAY_INDEX=ORG_INDEX + APIKEYS_STARTED; 
				System.out.println("start SUBSCRIBER "+APIKEYS_STARTED+" in ORG "+ORGS_STARTED+", APIKEY_CLIENTID["+ARRAY_INDEX+"]: "+get_clientID(MQTT_ACTION_SUB, ARRAY_INDEX)+" to receive "+MQTT_SUB_MSGS+" msgs. ");

				subThreads.add( launch_sub(MQTT_ACTION_SUB, ARRAY_INDEX) );

				APIKEYS_STARTED++;
			}  //done
			APIKEYS_STARTED=0;
//			for (Thread thread:subThreads) {
//				thread.start();
//			}
//
//			//# Verify the SUBs are connected and subscribed
//			verify_sub_ready();


			//# Start the number of PUBs specifed by MQTT_PUB_INSTANCES
			while ( DEVICES_STARTED < MQTT_PUB_INSTANCES) {

				ARRAY_INDEX=ORG_INDEX + DEVICES_STARTED; 
				System.out.println("start PUBLISHER "+DEVICES_STARTED+" in ORG "+ORGS_STARTED+", DEVICES_CLIENTID["+ARRAY_INDEX+"]: "+get_clientID(MQTT_ACTION_PUB, ARRAY_INDEX)+" to send "+MQTT_PUB_MSGS+" msgs. ");

				pubThreads.add( launch_pub(MQTT_ACTION_PUB, ARRAY_INDEX) );

				DEVICES_STARTED++;
			} //done
			DEVICES_STARTED=0;
			ORGS_STARTED++;

//			for ( Thread thread: pubThreads) {
//				thread.start();
//			}
		} //done

		ARRAY_INDEX--;

		System.out.println("------------------------------------------------------------------------");
		System.out.println("Waiting on "+MQTT_SUB_INSTANCES+" Subs and "+MQTT_PUB_INSTANCES+" Pubs launched in "+MQTT_ORG_INSTANCES+" Orgs starting at ORG #"+MQTT_ORG_INDEX+" to complete.");

		for (Thread thread:subThreads) {
			thread.start();
		}

		//# Verify the SUBs are connected and subscribed
		verify_sub_ready();

		for ( Thread thread: pubThreads) {
			thread.start();
		}

		//!!!	wait
		try {
			for (Thread thread:subThreads) {
				thread.join();
			}
			for (Thread thread:pubThreads) {
				thread.join();
			}
		} catch (InterruptedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}

		verify_pub_logs();
		verify_sub_logs();

		System.out.println("------------------------------------------------------------------------");

		if ( LOG_ERROR_COUNT == 0 ){
			System.out.println(LOG_FINALE_PASS+": Test case was successful! ");
		} else {
			System.out.println(LOG_FINALE_FAIL+": Test case failed with "+LOG_ERROR_COUNT+" error(s), see messages above! ");
		}

	}

	/**
	 * @param none
	 */
	private String nslookup_proxy(){
		String MQTT_SERVER=null;
		String host="";
		try {

			if ( TARGET_ENV == "LIVE" ) {
				host="messaging.internetofthings.ibmcloud.com";
			}else{
				host="messaging."+TARGET_ENV+".test.internetofthings.ibmcloud.com";
			}
			String address[] =InetAddress.getByName(host).toString().split("/");
			MQTT_SERVER="tcp://"+address[1]+":1883";
			System.out.println(TARGET_ENV+" DNS Address for MESSAGING is: "+ MQTT_SERVER);
		}catch (UnknownHostException e){
			e.printStackTrace();
		}
		return MQTT_SERVER;
	}


	/**
	 * @param none
	 */
	private void build_apikey_array(){
		/*
		 * 
		 */
		String TARGET_FILE="/var/log/"+TARGET_ENV+".apikeys_master";
		String line=null;
		String [] split_line=null;
		lines=new ArrayList<String[]>();

		try {
			BufferedReader br = new BufferedReader( new FileReader(TARGET_FILE) );
			while (( line = br.readLine() ) != null ){
				split_line=line.split("\\|");
				lines.add(split_line);
			}

			APIKEYS=lines.toArray(new String[0][0]);


			//			for(String[] oneline: lines){
			//				System.out.println(oneline[1]);
			//			}
			//			for(int row=0 ; row<lines.size() ; row++){
			//				System.out.println(lines.get(row)[1]);
			//			}

		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (NullPointerException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		lines=null;
	}


	/**
	 * @param none
	 */
	private void build_device_array(){
		/*
		 * 
		 */
		String TARGET_FILE="/var/log/"+TARGET_ENV+".devices_master";
		String line=null;
		String [] split_line=null;
		lines=new ArrayList<String[]>();		

		try {
			BufferedReader br = new BufferedReader( new FileReader(TARGET_FILE) );
			while (( line = br.readLine() ) != null ){
				split_line=line.split("\\|");
				lines.add(split_line);
			}

			DEVICES=lines.toArray(new String[0][0]);

		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (NullPointerException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		lines=null;	
	}



	/**
	 * @param action
	 * @param index
	 */
	private String get_clientID(String action, int index){
		// Subscribe
		if ( action.equalsIgnoreCase(MQTT_ACTION_SUB)){
			return (APIKEYS[index][0]);
			// Publish
		}else if (action.equalsIgnoreCase(MQTT_ACTION_PUB)) {
			return (DEVICES[index][0]);
			// UNKNOWN
		}else {
			String msg="ERROR:  The ACTION: "+action+" is unknown!";
			LOG_ERROR_COUNT++;
			System.err.println(msg);
			throw new java.lang.NullPointerException(msg);  //or should I throw NPE?
		}
	}

	/**
	 * @param action
	 * @param index
	 */
	private String get_topic(String action, int index){
		// Subscribe
		if ( action.equalsIgnoreCase(MQTT_ACTION_SUB)){
			return (APIKEYS[index][1]);
			// Publish
		}else if (action.equalsIgnoreCase(MQTT_ACTION_PUB)) {
			return (DEVICES[index][1]);
			// UNKNOWN
		}else {
			String msg="ERROR:  The ACTION: "+action+" is unknown!";
			System.err.println(msg);
			throw new java.lang.NullPointerException(msg);  //or should I throw NPE?
		}
	}

	/**
	 * @param action
	 * @param index
	 */
	private String get_userID(String action, int index){
		// Subscribe
		if ( action.equalsIgnoreCase(MQTT_ACTION_SUB)){
			return (APIKEYS[index][2]);
			// Publish
		}else if (action.equalsIgnoreCase(MQTT_ACTION_PUB)) {
			return (DEVICES[index][2]);
			// UNKNOWN
		}else {
			String msg="ERROR:  The ACTION: "+action+" is unknown!";
			System.err.println(msg);
			throw new java.lang.NullPointerException(msg);  //or should I throw NPE?
		}
	}

	/**
	 * @param action
	 * @param index
	 */
	private String get_password(String action, int index){
		// Subscribe
		if ( action.equalsIgnoreCase(MQTT_ACTION_SUB)){
			return (APIKEYS[index][3]);
			// Publish
		}else if (action.equalsIgnoreCase(MQTT_ACTION_PUB)) {
			return (DEVICES[index][3]);
			// UNKNOWN
		}else {
			String msg="ERROR:  The ACTION: "+action+" is unknown!";
			System.err.println(msg);
			throw new java.lang.NullPointerException(msg);  //or should I throw NPE?
		}
	}


	private Thread launch_sub(String action, int index){
		int i=0;
		String clientid=get_clientID(action, index); 
		String topic=get_topic(action, index); 
		String userid=get_userID(action, index); 
		String pswd=get_password(action, index); 

		System.out.println("SUB["+index+"] : CLIENTID="+clientid+" , TOPIC="+topic+" , UID="+userid+", PSWD="+pswd);
		String[] SUB_ARGS= new String[passthru_args.size() + 14];
		for ( i=0 ; i < passthru_args.size() ; i++ ){
			SUB_ARGS[i]= passthru_args.get(i);
		}

		SUB_ARGS[i]="-a";
		i++;
		SUB_ARGS[i]=MQTT_ACTION_SUB;
		i++;
		SUB_ARGS[i]="-i";
		i++;
		SUB_ARGS[i]=clientid;
		i++;
		SUB_ARGS[i]="-t";
		i++;
		SUB_ARGS[i]=topic;
		i++;
		SUB_ARGS[i]="-u";
		i++;
		SUB_ARGS[i]=userid;
		i++;
		SUB_ARGS[i]="-p";
		i++;
		SUB_ARGS[i]=pswd;
		i++;
		SUB_ARGS[i]="-n";
		i++;
		SUB_ARGS[i]=String.valueOf(MQTT_SUB_MSGS);
		i++;
		SUB_ARGS[i]="-o";
		i++;
		SUB_ARGS[i]=MQTT_ACTION_SUB+"."+clientid+".log";
		i++;

		return new Thread(new MqttSample(SUB_ARGS));

	}

	private Thread launch_pub(String action, int index){
		int i=0;
		String clientid=get_clientID(action, index); 
		String topic=get_topic(action, index); 
		String userid=get_userID(action, index); 
		String pswd=get_password(action, index); 
        String[] PUB_ARGS=null;
        
		System.out.println("PUB["+index+"] : CLIENTID="+clientid+" , TOPIC="+topic+" , UID="+userid+", PSWD="+pswd);
		if ( USE_DEFAULT_MSG ) {
			PUB_ARGS= new String[ passthru_args.size() + 16];
		} else {
			PUB_ARGS= new String[ passthru_args.size() + 14];
		}
		for ( i=0 ; i < passthru_args.size() ; i++ ){
			PUB_ARGS[i]= passthru_args.get(i);
		}

		PUB_ARGS[i]="-a";
		i++;
		PUB_ARGS[i]=MQTT_ACTION_PUB;
		i++;
		PUB_ARGS[i]="-i";
		i++;
		PUB_ARGS[i]=clientid;
		i++;
		PUB_ARGS[i]="-t";
		i++;
		PUB_ARGS[i]=topic;
		i++;
		PUB_ARGS[i]="-u";
		i++;
		PUB_ARGS[i]=userid;
		i++;
		PUB_ARGS[i]="-p";
		i++;
		PUB_ARGS[i]=pswd;
		i++;
		PUB_ARGS[i]="-n";
		i++;
		PUB_ARGS[i]=String.valueOf(MQTT_PUB_MSGS);
		i++;
		PUB_ARGS[i]="-o";
		i++;
		PUB_ARGS[i]=MQTT_ACTION_PUB+"."+clientid+".log";
		i++;
		if ( USE_DEFAULT_MSG ) {
			PUB_ARGS[i]="-m";
			i++;
			String MQTT_MSG="{\"d\":{\"clientId\":\""+clientid+"\", \"msgs\":"+MQTT_PUB_MSGS+"  }}";
			PUB_ARGS[i]=MQTT_MSG;
			i++;
		}

		return new Thread (	new MqttSample(PUB_ARGS) ); 

	}

	private void verify_sub_ready() {
		int ORGS_STARTED=0;
		int ORG_INDEX=0;
		int APIKEYS_STARTED=0;
		int ARRAY_INDEX=0;
		int LOG_CHECK_ITERATIONS=15;		// How many times to check for Verify Text before giving up
		String LOG_SUB_READY_1="subscribed to topic: ";

		while ( ORGS_STARTED < MQTT_ORG_INSTANCES )   { 
			//# Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
			ORG_INDEX=( ORGS_STARTED + MQTT_ORG_INDEX )* DEVICE_ORG_MEMBERS;

			//# Start the number of SUBs specifed by MQTT_SUB_INSTANCES
			while ( APIKEYS_STARTED < MQTT_SUB_INSTANCES ) {
				ARRAY_INDEX=ORG_INDEX + APIKEYS_STARTED; 

				// grep subscribe.clientid.log for READY TEXT
				String sub_logfile=MQTT_ACTION_SUB+"."+get_clientID(MQTT_ACTION_SUB, ARRAY_INDEX)+".log";

		        System.out.println("Verify SUBSCRIBER "+APIKEYS_STARTED+" in ORG "+ORGS_STARTED+", APIKEY_CLIENTID["+ARRAY_INDEX+"]: "+get_clientID(MQTT_ACTION_SUB, ARRAY_INDEX)+" has subscribed for "+MQTT_SUB_MSGS+" msgs.  ");

		          for ( int i=0 ; i <= LOG_CHECK_ITERATIONS ; i++ ) {
		        	String theGrepCmd="grep "+LOG_SUB_READY_1+" "+sub_logfile;
//grep		        	Profile localLogFile = ProfileBuilder.newBuilder()
//grep		        								.name(sub_logfile)
//grep		        			                    .filePath(sub_logfile)
//grep		        			                    .onLocalhost()
//grep		        			                    .build();
//		        	assertThat(executing(grep( constantExpression(LOG_SUB_READY_1), on(localLogFile))).totalLines(), maxMatches(1));
//		        	executing( grep( constantExpression(LOG_SUB_READY_1), on(localLogFile), maxMatches(1)));
//		        	GrepResults gr = grep( constantExpression(LOG_SUB_READY_1), on(localLogFile), maxMatches(1) );
//		        	for ( GrepResult oneResult : gr ) {
//		        		System.out.println(oneResult);
		        	}
		        	
//		                    RC=$?
//		                    if [[ $RC -ne 0 ]] ; then
//		                       sleep ${LOG_CHECK_SLEEP} 
//		                 
//		                    else 
//		                       LOG_SUB_READY="TRUE"
//		                       (( LOG_SUB_READY_COUNT++ ))
//		                       break
//		                    fi
//
//		          }
//
//		                    if [[ "${LOG_SUB_READY}" != "TRUE" ]] ; then
//		                       LOG_MSG="Unable to find 'log ready text': \"${LOG_SUB_READY_1}\" in file: ${MQTT_ACTION}.${APIKEY_CLIENTID[ ${ARRAY_INDEX}] }.log - may be slow consumer."
//		        #               log_error;
//		                       log_warning;
//		                    fi
//
//		                 LOG_SUB_READY="FALSE"
				
				
				
				APIKEYS_STARTED++;
			}  //done
			APIKEYS_STARTED=0;
			ORGS_STARTED++;

		} //done

		ARRAY_INDEX--;
		
	}

	private void verify_sub_logs() {
		int ORGS_STARTED=0;
		int ORG_INDEX=0;
		int APIKEYS_STARTED=0;
		int ARRAY_INDEX=0;

		while ( ORGS_STARTED < MQTT_ORG_INSTANCES )   { 
			//# Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
			ORG_INDEX=( ORGS_STARTED + MQTT_ORG_INDEX )* DEVICE_ORG_MEMBERS;

			//# Start the number of SUBs specifed by MQTT_SUB_INSTANCES
			while ( APIKEYS_STARTED < MQTT_SUB_INSTANCES ) {
				ARRAY_INDEX=ORG_INDEX + APIKEYS_STARTED; 

				// grep subscribe.clientid.log for READY TEXT
				String sub_logfile=MQTT_ACTION_SUB+"."+get_clientID(MQTT_ACTION_SUB, ARRAY_INDEX)+".log";
//		         THE_LOG="${MQTT_ACTION}.${APIKEY_CLIENTID[${ARRAY_INDEX}] }.log"
//		                 theGrep=`grep "${LOG_SUB_VERIFY_1}" ${THE_LOG} `
//		                 RC=$?
//		                 if [[ $RC -ne 0 ]] ; then
//		                    LOG_MSG="Unable to find 'sub verify 1 text': \"${LOG_SUB_VERIFY_1}\" in file: ${THE_LOG}. "
//		                    log_error;
//		                 fi
//		                 theGrep=`grep "${LOG_SUB_VERIFY_2}" ${THE_LOG} `
//		                 RC=$?
//		                 if [[ $RC -ne 0 ]] ; then
//		                    LOG_MSG="Unable to find 'sub verify 2 text': \"${LOG_SUB_VERIFY_2}\" in file: ${THE_LOG}. "
//		                    log_error;
//		                 fi

				APIKEYS_STARTED++;
			}  //done
			APIKEYS_STARTED=0;
			ORGS_STARTED++;

		} //done

		ARRAY_INDEX--;

//		   LOG_MSG="====== Subscriber Synopsis  ====================================="
//		   log_msg;
//		   let TOTAL_SUBS=${MQTT_ORG_INSTANCES}*${MQTT_SUB_INSTANCES}
//		   LOG_MSG="%==>  More stats:  Started ${TOTAL_SUBS} Subscribers."
//		   log_msg;
//		   TOTAL_MSG=`grep "${LOG_SUB_VERIFY_1}" ${MQTT_ACTION}.*.log |wc -l `
//		   LOG_MSG="%==>  ${TOTAL_MSG} Subscribers received the 'LOG_SUB_VERIFY_1' msg : \"${LOG_SUB_VERIFY_1}\"."
//		   log_msg;
//		   TOTAL_MSG=`grep "${LOG_SUB_VERIFY_2}" ${MQTT_ACTION}.*.log |wc -l `
//		   LOG_MSG="%==>  ${TOTAL_MSG} Subscribers received the 'LOG_SUB_VERIFY_2' msg : \"${LOG_SUB_VERIFY_2}\"."
//		   log_msg;
//		   LOG_MSG="==============================+================================="
//		   log_msg;
		
	}

	private void verify_pub_logs() {
		int ORGS_STARTED=0;
		int ORG_INDEX=0;
		int DEVICES_STARTED=0;
		int ARRAY_INDEX=0;

		while ( ORGS_STARTED < MQTT_ORG_INSTANCES )   { 
			//# Compute the INDEX into the DEVICES_* and APIKEYS_* arrays for this ORG
			ORG_INDEX=( ORGS_STARTED + MQTT_ORG_INDEX )* DEVICE_ORG_MEMBERS;

			//# Start the number of PUBs specifed by MQTT_PUB_INSTANCES
			while ( DEVICES_STARTED < MQTT_PUB_INSTANCES) {
				ARRAY_INDEX=ORG_INDEX + DEVICES_STARTED; 

				// grep publish.clientid.log for READY TEXT
				String pub_logfile=MQTT_ACTION_PUB+"."+get_clientID(MQTT_ACTION_PUB, ARRAY_INDEX)+".log";
//		         theGrep=`grep "${LOG_PUB_VERIFY_1}" ${THE_LOG} `
//		                 RC=$?
//		                 if [[ $RC -ne 0 ]] ; then
//		                    LOG_MSG="Unable to find 'pub verify 1 text': \"${LOG_PUB_VERIFY_1}\" in file: ${THE_LOG}. "
//		                    log_error;
//		                 fi
				

				DEVICES_STARTED++;
			} //done
			DEVICES_STARTED=0;
			ORGS_STARTED++;

		} //done

		ARRAY_INDEX--;

//		   LOG_MSG="====== Publisher Synopsis  ======================================"
//		   log_msg;
//		   let TOTAL_PUBS=${MQTT_ORG_INSTANCES}*${MQTT_PUB_INSTANCES}
//		   LOG_MSG="%==>  More stats:  Started ${TOTAL_PUBS} Publishers."
//		   log_msg;
//		   TOTAL_MSG=`grep "${LOG_PUB_VERIFY_1}" ${MQTT_ACTION}.*.log |wc -l `
//		   LOG_MSG="%==>  ${TOTAL_MSG} Publishers logged the 'LOG_PUB_VERIFY_1' msg : \"${LOG_PUB_VERIFY_1}\"."
//		   log_msg;
//		   LOG_MSG="================================================================"
//		   log_msg;
		
	}

}
