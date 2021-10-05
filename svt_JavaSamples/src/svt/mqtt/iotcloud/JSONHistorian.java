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
package svt.mqtt.iotcloud;

import java.io.UnsupportedEncodingException;
import java.net.InetAddress;
import java.net.URLEncoder;
import java.net.UnknownHostException;

import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.Hashtable;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.LinkedList;
import java.util.List;
import java.util.Map;

import org.glassfish.jersey.SslConfigurator;
import org.glassfish.jersey.client.authentication.HttpAuthenticationFeature;

import static org.glassfish.jersey.client.authentication.HttpAuthenticationFeature.HTTP_AUTHENTICATION_BASIC_PASSWORD;
import static org.glassfish.jersey.client.authentication.HttpAuthenticationFeature.HTTP_AUTHENTICATION_BASIC_USERNAME;

import org.json.simple.JSONArray;
//import org.json.simple.JSONObject;
import org.json.simple.JSONValue;
import org.json.simple.parser.ContainerFactory;
import org.json.simple.parser.JSONParser;
import org.json.simple.parser.ParseException;

//import com.sun.research.ws.wadl.Request;

import javax.net.ssl.SSLContext;
import javax.ws.rs.client.Client;
import javax.ws.rs.client.ClientBuilder;
//import javax.ws.rs.client.Invocation;
import javax.ws.rs.client.WebTarget;
import javax.ws.rs.core.Cookie;
//import javax.ws.rs.core.MediaType;
//import javax.ws.rs.core.NewCookie;
//import javax.ws.rs.core.MediaType;
import javax.ws.rs.core.Response;

public class JSONHistorian implements Runnable {

	// PARAMETERs that can be RESET BY INPUT Options
	double MSG_COUNT = 1000; 	// CLI: -n
	String TARGET_ENV = "LIVE"; // CLI: -S ex. wdc01-6 or LON02-5, LIVE
								// SoftLayer Stand
	String FILTER = null; 		// CLI: -F
	String ORG = null; 			// CLI: -O
	String USER = null; 		// CLI: -u
	String PSWD = null; 		// CLI: -p
	
	double EXPECT_FIRST_MSG = 0;   // Validation of returned rows, needs to be flexible with filters...

	// Default: LIVE sub01 in ORG u1ha7
	String LIVE_ORG  = "u1ha7"; // CLI: -O
	String LIVE_USER = "a:u1ha7:fhytdv3rjq";
	String LIVE_PSWD = "CMvMXpfp)Ein0r)9sy";
	// Default: lon02-5 sub for ORG 2o2idk
	String LON02_1_ORG  = "2o2idk"; // CLI: -O
	String LON02_1_USER = "a-2o2idk-oxhtnymubo";
	String LON02_1_PSWD = "-XCOerTX&hJO?7W747";
	// Terence Q's LIVE ORG and USER
	String tq_ORG  = "sejty";
	String tq_User = "a:sejty:phdlciestz";
	String tq_PSWD = "u1w9iOGeN@k9ye@gyn";

	String ORGANIZATION_URI = null;
	String HISTORIAN_URI = null;
	String REST_URI = null;
	String REST_RESOURCE = null;

	int PUB_STARTED = 0;
	int SUB_STARTED = 0;
	int ORG_STARTED = 0;
	int ORG_INDEX = 0;

	// For JERSERY Rest API usage
	Client client = null;
	WebTarget restWebTarget = null;
	WebTarget orgsWebTarget = null;
	WebTarget histWebTarget = null;
	// String keyStoreFile="cacerts.pkcs12";
	// String keyStorePswd="k1ngf1sh";
	// String trustStoreFile="cacerts.jks";
	// String trustStorePswd="k1ngf1sh";
	String trustStoreFile = "iot.jks";
	final String lon_02_1_trustStoreFile = "scripts/ssl/lon02-1.iot.jks";
	final String LIVE_trustStoreFile = "scripts/ssl/PRODUCTION.iot.jks";
	final String trustStorePswd = "password";

	Hashtable<String, LinkedList<Long>> publishersIndexTable = null;
	LinkedList<Long> pubIdxLinkedList = new LinkedList<Long>();
	Hashtable<String, LinkedList<Long>> publishersTimestampTable = null;
	LinkedList<Long> pubTSLinkedList = new LinkedList<Long>();
	Long messageIndex = 0L;
	boolean drillDown = true; // Passingn CLI -F param on input will set this to false.
	Hashtable<String, String[]> drillDownDevice = null;
	String[] deviceStats = new String[7];
	boolean  firstDrillOrgs = true;
	Hashtable<String, String[]> drillOrgs = new Hashtable< String, String[]>();
	String[] orgsStats = new String[3];
	final int IDX_EVT_TYPE    = 0;
	final int IDX_DEVICE_TYPE = 1;
	final int IDX_DEVICE_ID   = 2;
	final int IDX_START_TS    = 3;
	final int IDX_END_TS     = 4;
	final int IDX_TS_MSGS     = 5;
	final int IDX_TOTAL_MSGS  = 6;
	final String DEVICE_TYPE = "device_type";
	final String DEVICE_ID   = "device_id";
	final String EVT_TYPE    = "evt_type";
	final String TIMESTAMP   = "timestamp";
	final String EVT         = "evt";

	final String CHUNK = "chunked"; 		// REST API Header: Transfer-Encoding value
	final String IOT_CURSOR = "cursorId"; 	// REST API Header: Chunked Cursor position name
	final String IOT_COOKIE = "iotHistorianSessionId"; // REST API Header:  Cookie Name

	// Drill Down
	// Must do QUERY with NO Filter first to rebuild the drillDownDevice data for subsequent queries
	// In NoFilter QUERY must set to REBUILD_DrillDown=true to get the data rebuilt
	boolean   rebuild_DrillDownDevice = true;
	boolean   isSummarizeQuery = false;

	final int DD_FIRST_FILTER = 0; 			// First Filter in FOR LOOPs
	final int DD_ORG_FILTER_EVTTYPE = 0; 	// $ORG?evt_type=mqttbench
	final int DD_ORG_FILTER_TS = 1; 		// $ORG?evt_type=mqttbench,start=tsStart,end=tsStop
	final int DD_TYPE = 2; 					// $ORG/$TYPE
	final int DD_TYPE_FILTER_TOP = 3; 		// $ORG/$TYPE?top=50
	final int DD_TYPE_FILTER_TS = 4; 		// $ORG/$TYPE?evt_type=mqttbench,start=tsStart,end=tsStop
	final int DD_TYPE_FILTER_SUM_COUNT = 5; // $ORG/$TYPE?evt_type=mqttbench,summarize={},summarize_type[avg,count,min,mix,sum,range,stddev,variance]
	final int DD_TYPE_FILTER_TS_SUM_RANGE = 6; 	// $ORG/$TYPE?evt_type=mqttbench,start=tsStart,end=tsStop,summarize={},summarize_type[range]
	final int DD_TYPE_FILTER_TS_SUM_SUM = 7; 	// $ORG/$TYPE?evt_type=mqttbench,start=tsStart,end=tsStop,summarize={},summarize_type[sum]
	final int DD_TYPE_FILTER_SUM_RANGE = 8; // $ORG/$TYPE?evt_type=mqttbench,summarize={},summarize_type[range]
	final int DD_TYPE_FILTER_SUM_MIN = 9; 	// $ORG/$TYPE?evt_type=mqttbench,summarize={},summarize_type[min]
	final int DD_TYPE_FILTER_SUM_MAX = 10; 	// $ORG/$TYPE?evt_type=mqttbench,summarize={},summarize_type[max]
	final int DD_TYPE_FILTER_SUM_STDEV = 11; // $ORG/$TYPE?evt_type=mqttbench,summarize={},summarize_type[stdev]
	final int DD_TYPE_FILTER_SUM_VARY = 12; // $ORG/$TYPE?evt_type=mqttbench,summarize={},summarize_type[variance]
	final int DD_ID = 13; 					// $ORG/$TYPE/$ID
	final int DD_ID_FILTER_TOP = 14; 		// $ORG/$TYPE/$ID?top=10
	final int DD_ID_FILTER_TS = 15; 		// $ORG/$TYPE/$ID?evt_type=mqttbench,start=tsStart,end=tsStop
	final int DD_ID_FILTER_TS_START = 16; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,start=tsStart
	final int DD_ID_FILTER_TS_END = 17;   	// $ORG/$TYPE/$ID?evt_type=mqttbench,end=tsStop
	final int DD_ID_FILTER_SUM_AVG = 18; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,summarize={},summarize_type[avg,count,min,max,sum,range,stddev,variance]
	final int DD_ID_FILTER_SUM_RANGE = 19; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,summarize={},summarize_type[range]
	final int DD_ID_FILTER_SUM_MIN = 20; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,summarize={},summarize_type[min]
	final int DD_ID_FILTER_SUM_MAX = 21; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,summarize={},summarize_type[max]
	final int DD_ID_FILTER_SUM_SUM = 22; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,summarize={},summarize_type[sum]
	final int DD_ID_FILTER_TS_SUM_COUNT = 23; 	// $ORG/$TYPE/$ID?evt_type=mqttbench,start=tsStart,end=tsStop,summarize={},summarize_type[count]
	final int DD_ID_FILTER_TOP_VARIANCE = 24; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[variance]
	final int DD_ID_FILTER_TOP_STDEV = 25; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[stdev]
	final int DD_ID_FILTER_TOP_AVG = 26; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[avg]
	final int DD_ID_FILTER_TOP_COUNT = 27; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[count]
	final int DD_ID_FILTER_TOP_MIN = 28; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[min]
	final int DD_ID_FILTER_TOP_MAX = 29; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[max]
	final int DD_ID_FILTER_TOP_RANGE = 30; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[range]
	final int DD_ID_FILTER_TOP_SUM = 31; 	// $ORG/$TYPE/$ID?top=5&summarize={}&summarize_type[sum]
	final int DD_LAST_FILTER = 31; 			// Last Filter in FOR LOOPs

	final String EVT_TYPE_VALUE = "mqttbench";
	final String FILTER_EVT_TYPE = "evt_type";
	final String FILTER_TS_START = "start";
	final String FILTER_TS_END = "end";
	final String FILTER_TOP = "top";
	final String FILTER_SUM = "summarize";
	final String FILTER_SUM_TYPE = "summarize_type";
	final String SUM_TYPE_AVG = "avg";
	final String SUM_TYPE_COUNT = "count";
	final String SUM_TYPE_MIN = "min";
	final String SUM_TYPE_MAX = "max";
	final String SUM_TYPE_SUM = "sum";
	final String SUM_TYPE_RANGE = "range";
	final String SUM_TYPE_STDEV = "stdev";
	final String SUM_TYPE_VARY = "variance";

	static final String LOG_FINALE_PASS = "PASSED";
	static final String LOG_FINALE_FAIL = "FAILED";
	static int LOG_ERROR_COUNT = 0;

	boolean DEBUG = false;

	// boolean DEBUG=true;

	/**
	 * @param args
	 */
	public static void main(String[] args) {
		// Picks off input args, configuration and runs
		new JSONHistorian(args).run();

		if (LOG_ERROR_COUNT == 0) {
			System.out.println(LOG_FINALE_PASS
					+ ":  Test completed successfully. ");
		} else {
			System.out.println(LOG_FINALE_FAIL + ":  Test completed with "
					+ LOG_ERROR_COUNT + " errors.");
		}

	}

	/**
	 * @param args
	 */
	public JSONHistorian(String[] args) {
		// Pick off input args if there are any needed?
		iotParseArgs(args);

	}

	private int iotParseArgs(String[] args) {
		if (DEBUG)
			System.out.println("ENTER:  iotparseArgs() with " + args.length
					+ " parameters");

		// What Stand are we running against, or running against the LIVE
		// production?
		for (int i = 0; i < args.length; i++) {

			// -S SoftLayer Stand to test with
			if ("-S".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				TARGET_ENV = args[i];

				// Organization to query
			} else if ("-O".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				ORG = args[i];

				// FILTER to query
			} else if ("-F".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				FILTER = args[i];
				drillDown = false;

				// Publishers not needed?
			} else if ("x".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				PUB_STARTED = Integer.parseInt(args[i]);

				// Subscribers not needed?
			} else if ("-y".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				SUB_STARTED = Integer.parseInt(args[i]);

				// Number of Orgs to start not needed?
			} else if ("-z".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				ORG_STARTED = Integer.parseInt(args[i]);

				// Orgs Index Offset not needed?
			} else if ("-Z".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				ORG_INDEX = Integer.parseInt(args[i]);

				// Number of Messages sent
			} else if ("-n".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				MSG_COUNT = Double.parseDouble(args[i]);

				// APIKEY UserID to perform the query
			} else if ("-u".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				USER = args[i];

				// Password of APIKEY UserID
			} else if ("-p".equals(args[i]) && (i + 1 < args.length)) {
				i++;
				PSWD = args[i];

				// Debug logging turned on
			} else if ("-D".equalsIgnoreCase(args[i])) {
				DEBUG = true;

				// HELP requested
			} else if ("-H".equalsIgnoreCase(args[i])) {
				show_syntax();
				System.exit(99);

			} else {
				System.out.println("Error passing INPUT PARAMETER: " + args[i]
						+ "\n");
				LOG_ERROR_COUNT++;
				show_syntax();

			}
		}

		REST_URI = nslookup_historian(TARGET_ENV);
		HISTORIAN_URI = REST_URI + "/historian";
		ORGANIZATION_URI = REST_URI + "/organizations";

		if (!(TARGET_ENV.equals(""))) {
			if (TARGET_ENV.equalsIgnoreCase("LIVE")) {
				trustStoreFile = LIVE_trustStoreFile;
			} else if (TARGET_ENV.equalsIgnoreCase("LON02-1")) {
				trustStoreFile = lon_02_1_trustStoreFile;
			} else if (TARGET_ENV.equalsIgnoreCase("LON02-5")) {
				trustStoreFile = lon_02_1_trustStoreFile;
			} // If TARGET_ENV is anything else, most likely will fail using the
				// CERT build SSLContext

			if (USER == null ) {
				if (TARGET_ENV.equalsIgnoreCase("LIVE")) {
					USER = LIVE_USER;
				} else if (TARGET_ENV.equalsIgnoreCase("LON02-5")) {
					USER = LON02_1_USER;
				} else {
					System.out.println("ERROR:  Syntax check:  -O [ORG] is a required parameter.");
					System.out.println("ERROR:  No USER was passed.  Unable to default without ORG.");
					LOG_ERROR_COUNT++;
				}
			}

			if (PSWD == null ) {
				if (TARGET_ENV.equalsIgnoreCase("LIVE")) {
					PSWD = LIVE_PSWD;
				} else if (TARGET_ENV.equalsIgnoreCase("LON02-5")) {
					PSWD = LON02_1_PSWD;
				} else {
					System.out.println("ERROR:  Syntax check:  -O [ORG] is a required parameter.");
					System.out.println("ERROR:  No PSWD was passed.  Unable to default without ORG.");
					LOG_ERROR_COUNT++;
				}
			}

		} else {
			System.out.println("ERROR:  Syntax check:  -S [STAND|LIVE] is a required parameter.");
			LOG_ERROR_COUNT++;
		}

		if ( ORG== null  ){
			System.out.println("ERROR:  Syntax check:  -O [ORG] is a required parameter.");
			LOG_ERROR_COUNT++;			
		}
		return LOG_ERROR_COUNT;
	}

	private void show_syntax() {
		// HELP
		System.out.println(" Syntax Help:   ");
		System.out.println(" ");
		System.out.println(" -S [STAND_NAME or LIVE] : (REQUIRED) LIVE is Production Env., STAND ex. LON02-5.  default="
						+ TARGET_ENV);
		System.out.println(" -O [ORG]                : (REQUIRED) Organization to query.  default="
						+ ORG);
		System.out.println(" ");
		System.out.println(" -F [FILTER]             : Complex Filter to query.  default="
						+ FILTER);
		System.out.println(" -u [USER]               : APIKEY UserId to perform the Query. default="
						+ USER);
		System.out.println(" -p [PSWD]               : APIKEY UserId to perform the Query.  default="
						+ PSWD);
		System.out.println(" -n [MSG_COUNT]          : Number of Messages Published. default="
						+ MSG_COUNT);
		System.out.println(" ");
		System.out.println(" -x,y,z,Z ?NOT NEEDED?, not implemented!");
		System.out.println(" -x                      : Number of DEVICE Publishers  NOT NEEDED?");
		System.out.println(" -y                      : Number of APIKEY Subscribers");
		System.out.println(" -z                      : Number of Orgs to query");
		System.out.println(" -Z                      : Org Index Offset to start");
		System.out.println(" -                       : ");
		System.out.println(" -D                      : Debug verbose output.  default="
						+ DEBUG);
		System.out.println(" -H                      : Help for syntax");
	}

	/**
	 * @param String
	 *            theTarget - the SoftLayer Stand name (LON02-5) or LIVE for
	 *            real Production Env.
	 * @return String theURI - String of the URI Stem to the Historian REST API
	 */
	private String nslookup_historian(String theTarget) {
		if (DEBUG) {
			System.out.println("ENTER:  nslookup_historian() with " + theTarget);
		}
		String theURI = null;
		String theURI_HOSTNAME = null;
		String host = null;

		try {
			if (theTarget.equalsIgnoreCase("LIVE")) {
				host = "internetofthings.ibmcloud.com";
			} else {
				host = theTarget + ".test.internetofthings.ibmcloud.com";
			}

			String address[] = InetAddress.getByName(host).toString()
					.split("/");
			theURI = "https://" + address[1] + "/api/v0001";
			theURI_HOSTNAME = "https://" + host + "/api/v0001";
			System.out.println(theTarget
					+ " DNS IP and base URL Address for HISTORIAN is: "
					+ theURI_HOSTNAME);
			if (DEBUG)
				System.out.println(theTarget
						+ " DNS IP Address for HISTORIAN is: " + theURI
						+ " for debug info only, not used directly.");

		} catch (UnknownHostException e) {
			e.printStackTrace();
		}
		return theURI_HOSTNAME;
	}

	@Override
	public void run() {
		String theDeviceType = null;
		String theDeviceId   = null;
		String predicate     = FILTER;

		if (DEBUG) {
			System.out.println("ENTER:  run() ");
		}

		String myURL = HISTORIAN_URI + "/" + ORG;
		rebuild_DrillDownDevice = true;
		
		
		System.out.println( "computeSUM(2,6)=[20], got:"+ computeSUM("2", "6") );
		System.out.println( "computeAVG(2,6)=[4], got:"+ computeAVG("2", "6") );
		System.out.println( "computeRANGE(2,6)=[5], got:"+ computeRANGE("2", "6") );
		System.out.println( "computeSIZE(2,6)=[5], got:"+ computeSIZE("2", "6") );
		System.out.println( "computeSTDEV(2,6)=[1.414], got:"+ computeSTDEV("2", "6") );
		System.out.println( "computeVariance(2,6)=[2.0], got:"+ computeVARIANCE("2", "6") );

		System.out.println( "computeSUM(0,999)=[499500], got:"+ computeSUM("0", "999") );
		System.out.println( "computeAVG(0,999)=[499], got:"+ computeAVG("0", "999") );
		System.out.println( "computeRANGE(0,999)=[1000], got:"+ computeRANGE("0", "999") );
		System.out.println( "computeSIZE(0,999)=[1000], got:"+ computeSIZE("0", "999") );
		System.out.println( "computeSTDEV(0,999)=[288.6], got:"+ computeSTDEV("0", "999") );
		System.out.println( "computeVariance(0,999)=[83333.0], got:"+ computeVARIANCE("0", "999") );

		System.out.println( "computeSUM(250,500)=[94125], got:"+ computeSUM("250", "500") );
		System.out.println( "computeAVG(250,500)=[375], got:"+ computeAVG("250", "500") );
		System.out.println( "computeRANGE(250,500)=[251], got:"+ computeRANGE("250", "500") );
		System.out.println( "computeSIZE(250,500)=[251], got:"+ computeSIZE("250", "500") );
		System.out.println( "computeSTDEV(250,500)=[72.456], got:"+ computeSTDEV("250", "500") );
		System.out.println( "computeVariance(250,500)=[5250.0], got:"+ computeVARIANCE("250", "500") );
	
		System.out.println( "computeSUM(995,999)=[4985], got:"+ computeSUM("995", "999") );
		System.out.println( "computeAVG(995,999)=[997], got:"+ computeAVG("995", "999") );
		System.out.println( "computeRANGE(995,999)=[5], got:"+ computeRANGE("995", "999") );
		System.out.println( "computeSIZE(995,999)=[5], got:"+ computeSIZE("995", "999") );
		System.out.println( "computeSTDEV(995,999)=[1.414], got:"+ computeSTDEV("995", "999") );
		System.out.println( "computeVariance(995,999)=[2.0], got:"+ computeVARIANCE("995", "999") );

		// Create the Jersey Client Objects to query REST APIs
		SSLContext sslContext = null;

		SslConfigurator sslConfig = SslConfigurator.newInstance()
				.trustStoreFile(trustStoreFile)
				.trustStorePassword(trustStorePswd)
				.keyStoreFile(trustStoreFile).keyPassword(trustStorePswd);

		sslContext = sslConfig.createSSLContext();

		// The SSL Client
		client = ClientBuilder.newBuilder().sslContext(sslContext).build();

		// If a FILTER was provided, ONLY run that query and not do the drill down
		if (!(drillDown)) {
			String[] filterArray = null;
			// Does the FILTER has elements to right of the ?
			if ( FILTER.contains("?") ) {
				filterArray = FILTER.split("[\u003F]");
				predicate = filterArray[0];
				// Url is the static Historian Stem + the elements to the left of the ?
				myURL = myURL + predicate;
				histWebTarget =   client.target(myURL);
				
				if ( filterArray[1].contains("&") ) {
					String[] filterParams = filterArray[1].split("&");
					for ( String item : filterParams ){
						String[] itemBits = item.split("=");
						histWebTarget = buildWebTargetQueryParams( histWebTarget, itemBits[0], itemBits[1] );
					}
				} else {
						String[] itemBits = filterArray[1].split("=");
						histWebTarget = buildWebTargetQueryParams( histWebTarget, itemBits[0], itemBits[1] );
				}
			// Only URI elements, no data to right of the '?' as a Complex Filter	
			} else {
				myURL = myURL + FILTER;
				histWebTarget =   client.target(myURL);
			}
			// Set default values to be passed to processRequest()
			String[] predicateParts = predicate.split("/");
			if ( predicateParts.length >= 2 ) theDeviceType = predicateParts[1];
			if ( predicateParts.length >= 3 ) theDeviceId   = predicateParts[2];
		} else {
			// This is a DRILL DOWN request
			if ( FILTER != null ) {
				myURL = myURL + FILTER;
			}
			histWebTarget =   client.target(myURL);
			
		}

		
		HttpAuthenticationFeature feature = HttpAuthenticationFeature
				.basicBuilder().build();

		client.register(feature);

		int rc = processRequest(histWebTarget, theDeviceType, theDeviceId);
		rebuild_DrillDownDevice = false;

		if (drillDown) {
//			for (int i = DD_FIRST_FILTER; i <= DD_FIRST_FILTER; i++) {
			for (int i = DD_FIRST_FILTER; i <= DD_LAST_FILTER; i++) {
				int rcDrillDown = drillDownQueries(i);

				if (rcDrillDown != 0) {
					System.out.println("   %==>   Something when very wrong in the drillDownQueries(#"+ i +"). rc = "
									+ rcDrillDown);
				}
			}
		}

	}

	private int processRequest(WebTarget myURL, String drillType, String drillID) {
		// restWebTarget = client.target(REST_URI +"/historian/"+
		// REST_RESOURCE);
		// orgsWebTarget = restWebTarget.path("/organizations/" + ORG);
		int chunkCount=0;
		System.out.println("Enter: processRequest() @request.get() for "
				+ histWebTarget );

		int rcGetStatus = 0;
		int rc = 0;
		int rc_final = 0;
		String jsonString = null;
		publishersIndexTable = new Hashtable<String, LinkedList<Long>>();
		publishersTimestampTable = new Hashtable<String, LinkedList<Long>>();
		if ( rebuild_DrillDownDevice ) {
			drillDownDevice = null;
			drillDownDevice = new Hashtable<String, String[]>();
		}

		Response response = client.target(myURL.getUri()).request()
				.property(HTTP_AUTHENTICATION_BASIC_USERNAME, USER)
				.property(HTTP_AUTHENTICATION_BASIC_PASSWORD, PSWD).get();

		rcGetStatus = response.getStatus();
		System.out.println("getStatus:  " + rcGetStatus);

		System.out.println("getStatusInfo:  " + response.getStatusInfo());

		System.out.println("getDate:  " + response.getDate());
		System.out.println("getLocation:  " + response.getLocation());
		System.out.println("getLength:  " + response.getLength());
		System.out.println("getHeaders:  " + response.getHeaders().toString());

		// Closes the connection
		// JCD? Might need to relook at this, can I do readentity and then
		// process chunks ?

		jsonString = response.readEntity(String.class);
		System.out.println("readEntity:  " + jsonString);
		// was request.get() successful?
		if (rcGetStatus == 200 || rcGetStatus == 201) {
			// Need iotHistoriamSessioID cookie and cursorId to iterate through
			// the chunks (multiple response pages)
			// tq Map<String, NewCookie> cookies = null;
			// tq NewCookie iotCookie = null;
			Cookie iotCookie = null;
			// int cursor = 0;

			String chunked = response.getHeaderString("Transfer-Encoding");
			String cursorId = response.getHeaderString(IOT_CURSOR);
			// if (chunked.equalsIgnoreCase( CHUNK )){
			// If cursorId is NULL, then will not need to iterate through the
			// response 'chunks' to get all the data
			if (cursorId != null) {
				// returns null !? Terence has a different way...
				iotCookie = response.getCookies().get(IOT_COOKIE);

				// Parse the response into the Linked List for verification

				rc = parseResponse(jsonString, drillType, drillID, chunkCount++);
				rc_final += rc;

				if (rc != 0) {
					System.out.println("ERROR:  parseResponse() encountered an error, Do I/Can I continue?  rc="
							+ rc);
				}

				// chunks While ( chunks.hsaMore() ) {
				while (cursorId != null) {
					// Add Session Cookie and CursorId to the request
					// Obj
//					System.out.println("@request.get() for " + myURL + FILTER
//							+ " and cursorId=" + cursorId);
					System.out.println("@request.get() for " + histWebTarget
							+ " and cursorId=" + cursorId);

					response = client
							.target(myURL.getUri())
							.request()
							.cookie(iotCookie)
							.header(IOT_CURSOR, cursorId)
							.property(HTTP_AUTHENTICATION_BASIC_USERNAME,USER)
							.property(HTTP_AUTHENTICATION_BASIC_PASSWORD,PSWD).get();

					rcGetStatus = response.getStatus();
					System.out.println("getStatus:  " + rcGetStatus);

					System.out.println("getStatusInfo:  "
							+ response.getStatusInfo());

					System.out.println("getDate:  " + response.getDate());
					System.out.println("getLocation:  "+ response.getLocation());
					System.out.println("getLength:  "+ response.getLength());
					System.out.println("getHeaders:  "+ response.getHeaders().toString());

					// Closes the connection

					jsonString = response.readEntity(String.class);
					System.out.println("readEntity:  " + jsonString);

					cursorId = response.getHeaderString(IOT_CURSOR);
					chunked = response.getHeaderString("Transfer-Encoding");

					// Parse the response into the Linked List for verification

					rc = parseResponse(jsonString, drillType, drillID, chunkCount++);
					rc_final += rc;

					if (rc != 0) {
						System.out.println("ERROR:  parseResponse() encountered an error, Do I/Can I continue?  rc="
								+ rc);
					}
				} // while more chunks exist to be processed.
			} else {
				// response was not chunked, process the JSON Data into the
				// linked lists. Len of 2 is empty response "[]"
				if (jsonString.length() > 2) {
					rc = parseResponse(jsonString, drillType, drillID, chunkCount++);
					rc_final += rc;
				} else {
					rc++;
					LOG_ERROR_COUNT++;
					rc_final += rc;
					System.out.println("ERROR:  REST Response was '"
									+ chunked
									+ "', yet cursorID was NULL (first&last Data chunk) YET NO JSON Data was returned.  Unexpected!  TERMINATING:"
									+ LOG_FINALE_FAIL);
					System.exit(99);
				}

			}
			// When finished with all the CHUNKs, Check the linkedLists for
			// completeness in the publishersIndexTable

			rc = validateResults();
			rc_final += rc;

		} else {
			System.out.println("ERROR:  the URL: " + myURL + " failed with RC="
					+ rcGetStatus + " Reason: " + response.getStatusInfo());
			LOG_ERROR_COUNT++;
			rc++;
			rc_final += rc;
		}
		
		publishersIndexTable = null;
		publishersTimestampTable = null;
				
		return rc_final;

	}

	private int drillDownQueries(int pattern) {
		// Take the generic ORG Query and test Filter Iterations, 
		// then drill down to TYPE and ID Filter Iterations
		int rc = 0;
		int rc_final = 0;
		String myUrl = HISTORIAN_URI + "/" + ORG;
		String myBaseUrl = myUrl;
		// Iterate through the FILTER Patterns for each drillDownDevice
		switch (pattern) {

		case (DD_ORG_FILTER_EVTTYPE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				myUrl = myBaseUrl ;
				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

			}
			break;
		}
		case (DD_ORG_FILTER_TS): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_FIRST_MSG = EXPECT_FIRST_MSG;
				EXPECT_FIRST_MSG = Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4;
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2) - (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4);


				myUrl = myBaseUrl ;
				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				EXPECT_FIRST_MSG = ORIGINAL_FIRST_MSG;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		case (DD_TYPE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
// ?NO				rebuild_DrillDownDevice=true;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];
				histWebTarget = null;
				histWebTarget = client.target(myUrl);

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

			}
// NO?			rebuild_DrillDownDevice=false;
			break;
		}
		case (DD_TYPE_FILTER_TOP): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_FIRST_MSG = EXPECT_FIRST_MSG;
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = 50;
				EXPECT_FIRST_MSG = Double.parseDouble( deviceStats[IDX_TOTAL_MSGS] ) - MSG_COUNT;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];
				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;
				
				MSG_COUNT = ORIGINAL_MSG_COUNT;
				EXPECT_FIRST_MSG = ORIGINAL_FIRST_MSG;

			}
			break;
		}
		case (DD_TYPE_FILTER_TS): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);				

				double ORIGINAL_FIRST_MSG = EXPECT_FIRST_MSG;
				EXPECT_FIRST_MSG = Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4;
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2) - (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4);

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				EXPECT_FIRST_MSG = ORIGINAL_FIRST_MSG;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_TYPE_FILTER_SUM_COUNT): {
			double ORIGINAL_MSG_COUNT = MSG_COUNT;
			MSG_COUNT = 0;
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				MSG_COUNT+= new Integer( deviceStats[IDX_TOTAL_MSGS] );
			}	
				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_COUNT  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			break;
		}

		case (DD_TYPE_FILTER_TS_SUM_RANGE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2) - (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4);

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}


//		case (DD_TYPE_FILTER_TS_START): {
//			Enumeration<String> enumKey = drillDownDevice.keys();
//			while (enumKey.hasMoreElements()) {
//				String key = enumKey.nextElement();
//				String[] deviceStats = drillDownDevice.get(key);
//
//				int ORIGINAL_MSG_COUNT = MSG_COUNT;
//				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) ) - (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4);
//
//				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];
//
//				histWebTarget = null;
//				histWebTarget = client.target(myUrl);
//				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
//				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
//////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
//////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
//////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
//////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );
//
//				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
//				rc_final += rc;
//
//				MSG_COUNT = ORIGINAL_MSG_COUNT;
//			}
//			break;
//		}
//
//
//		case (DD_TYPE_FILTER_TS_END): {
//			Enumeration<String> enumKey = drillDownDevice.keys();
//			while (enumKey.hasMoreElements()) {
//				String key = enumKey.nextElement();
//				String[] deviceStats = drillDownDevice.get(key);
//
//				int ORIGINAL_MSG_COUNT = MSG_COUNT;
//				MSG_COUNT = Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2;
//
//				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];
//
//				histWebTarget = null;
//				histWebTarget = client.target(myUrl);
//				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
//////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
//				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
//////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
//////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
//////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );
//
//				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
//				rc_final += rc;
//
//				MSG_COUNT = ORIGINAL_MSG_COUNT;
//			}
//			break;
//		}
//

		case (DD_TYPE_FILTER_TS_SUM_SUM): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = 0;
				for (int i = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4) ; i < (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2) ; i++) {
					MSG_COUNT += i;
				}
				MSG_COUNT = computeSUM("1", "5");
				MSG_COUNT = computeSUM("250", "500");
				
				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_SUM  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}

		case (DD_TYPE_FILTER_SUM_RANGE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS])) - 1;
				

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}

		case (DD_TYPE_FILTER_SUM_MIN): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = 0;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_MIN  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}

		case (DD_TYPE_FILTER_SUM_MAX): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Double.parseDouble(deviceStats[IDX_TOTAL_MSGS])) - 1;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_MAX  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}

		case (DD_TYPE_FILTER_SUM_STDEV): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				int lastIndexValue = Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) - 1;
				MSG_COUNT = computeSTDEV( "0", Integer.toString(lastIndexValue) );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_STDEV  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}

		case (DD_TYPE_FILTER_SUM_VARY): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				int lastIndexValue = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS])) - 1;
				MSG_COUNT = computeVARIANCE("0", Integer.toString(lastIndexValue) );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_VARY  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				MSG_COUNT = ORIGINAL_MSG_COUNT;
				isSummarizeQuery = false;
			}
			break;
		}

		case (DD_ID): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
// NO! ?				rebuild_DrillDownDevice = true;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

			}
//			rebuild_DrillDownDevice = false;
			break;
		}
		
		case (DD_ID_FILTER_TOP): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_FIRST_MSG = EXPECT_FIRST_MSG;
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				int adjustment = 5;
				EXPECT_FIRST_MSG = adjustment;
				MSG_COUNT = Integer.parseInt( deviceStats[ IDX_TOTAL_MSGS ] ) - adjustment;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, MSG_COUNT );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				MSG_COUNT = ORIGINAL_MSG_COUNT;
				EXPECT_FIRST_MSG = ORIGINAL_FIRST_MSG;
			}
			break;
		}
		
		case (DD_ID_FILTER_TS): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_FIRST_MSG = EXPECT_FIRST_MSG;
				EXPECT_FIRST_MSG = Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4;
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2) - (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4);

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				EXPECT_FIRST_MSG = ORIGINAL_FIRST_MSG;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TS_START): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_FIRST_MSG = EXPECT_FIRST_MSG;
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) ) - (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 4);
				EXPECT_FIRST_MSG = Double.parseDouble("250");

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				MSG_COUNT = ORIGINAL_MSG_COUNT;
				EXPECT_FIRST_MSG = ORIGINAL_FIRST_MSG;
			}
			break;
		}
		
		case (DD_ID_FILTER_TS_END): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) / 2;
				
				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_SUM_AVG): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				// substract one from TOTAL msgs since INDEX starts with 0, not 1
				int lastIndexValue = (Integer.parseInt(deviceStats[IDX_TOTAL_MSGS])-1);
				MSG_COUNT =  computeAVG("0", Integer.toString(lastIndexValue) ) ;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_AVG  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_SUM_RANGE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				// substract one from TOTAL msgs since INDEX starts with 0, not 1
				MSG_COUNT =  (( Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) ) -1 ) ;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_SUM_MIN): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				// substract one from TOTAL msgs since INDEX starts with 0, not 1
				MSG_COUNT =  0;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_MIN  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_SUM_MAX): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				// substract one from TOTAL msgs since INDEX starts with 0, not 1
				MSG_COUNT =  (( Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) ) -1 )  ;

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_MAX  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_SUM_SUM): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);

				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = 0;
				for ( int i = 0 ; i < Integer.parseInt(deviceStats[IDX_TOTAL_MSGS]) ; i++ ) {
					MSG_COUNT += i;  
				}

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_SUM  );	
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TS_SUM_COUNT): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				int iFirstNum = Integer.parseInt(deviceStats[ IDX_TOTAL_MSGS])/4;
				int iLastNum = Integer.parseInt(deviceStats[ IDX_TOTAL_MSGS])/2;
				MSG_COUNT = computeSIZE( Integer.toString(iFirstNum), Integer.toString(iLastNum) );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TOP, MSG_COUNT );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_COUNT );	
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
//DD_ID_FILTER_TOP_VARIANCE=24  added for defect 75546
		
		case (DD_ID_FILTER_TOP_VARIANCE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = computeVARIANCE("995", "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_VARY  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
//DD_ID_FILTER_TOP_STDEV    Added for defect 75652
		
		case (DD_ID_FILTER_TOP_STDEV): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = computeSTDEV("995", "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_STDEV  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
//DD_ID_FILTER_TOP_AVG, COUNT, MIN, MAX, RANGE, SUM  Added for defect 75660
		
		case (DD_ID_FILTER_TOP_AVG): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = computeAVG("995", "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_AVG  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TOP_COUNT): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = computeSIZE("995", "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_COUNT  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TOP_MIN): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = Double.parseDouble( "995" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_MIN  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TOP_MAX): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = Double.parseDouble( "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_MAX  );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TOP_RANGE): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = computeRANGE("995", "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_RANGE );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		case (DD_ID_FILTER_TOP_SUM): {
			Enumeration<String> enumKey = drillDownDevice.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				String[] deviceStats = drillDownDevice.get(key);
				
				double ORIGINAL_MSG_COUNT = MSG_COUNT;
				MSG_COUNT = computeSUM("995", "999" );

				myUrl = myBaseUrl + "/" + deviceStats[IDX_DEVICE_TYPE] + "/"
						+ deviceStats[IDX_DEVICE_ID];

				histWebTarget = null;
				histWebTarget = client.target(myUrl);
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_EVT_TYPE, deviceStats[IDX_EVT_TYPE] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_START, deviceStats[IDX_START_TS] );
////				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_TS_END,  deviceStats[IDX_END_TS]  );
				histWebTarget = histWebTarget.queryParam( FILTER_TOP, 5 );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM, "{Index}" );
				histWebTarget = buildWebTargetQueryParams( histWebTarget, FILTER_SUM_TYPE, SUM_TYPE_SUM );
				isSummarizeQuery = true;

				rc = processRequest(histWebTarget, deviceStats[IDX_DEVICE_TYPE], deviceStats[IDX_DEVICE_ID] );
				rc_final += rc;

				isSummarizeQuery = false;
				MSG_COUNT = ORIGINAL_MSG_COUNT;
			}
			break;
		}
		
		default: {
			System.out.println("ERROR:  The Drill Down Query pattern #, "
					+ pattern + ", " + "is not known. There are only "
					+ DD_LAST_FILTER + " known filters.");
			// rc++; rc_final += rc; LOG_ERROR_COUNT++; 
			// probably NOT, this is a programming error

		}
		}

		return rc_final;
	}

	private WebTarget buildWebTargetQueryParams(WebTarget theWebTarget,
			String key, String value) {
		// TODO Auto-generated method stub
		String encodeValue = null;
		try {
			encodeValue = URLEncoder.encode( value, "ISO-8859-1" );
		} catch (UnsupportedEncodingException e) {
			
			e.printStackTrace();
			System.out.println("Unable to encode "+ value +" in 'ISO-8859-1");
		}
		theWebTarget = theWebTarget.queryParam( key, encodeValue );

		
		return theWebTarget;
	}

	private int validateResults() {
		System.out.println("%==> ");
		System.out.println("%==> Starting validateResults() - where the Good Stuff Happens.");
		int rc = 0;
		int rc_final = 0;

		if ( isSummarizeQuery ){
			// Queries with Summarize Filter are actually verified as they are parsed.
			System.out.println("%==> SUMMARY FILTERed Queries are validated in parseResponse(), check rc above. " +
					"Current cumulative rc: "+ LOG_ERROR_COUNT);
		} else {
			// For EACH Publisher in publisherIndexTable, sort the LinkedList and
			// verify all msgs were found.
			Enumeration<String> enumKey = publishersIndexTable.keys();
			while (enumKey.hasMoreElements()) {
				String key = enumKey.nextElement();
				LinkedList<Long> pubResultList = publishersIndexTable.get(key);


				Collections.sort(pubResultList);

				// walk the linked list
				Long currentValue = pubResultList.getFirst();
				Long real_count = 1L; // Number of messages actually received, not
				// just the index of the rec'd msg
				Long next = 0L;
				if (currentValue != EXPECT_FIRST_MSG) {
					System.out.println("ERROR:  The Publisher "
							+ key
							+ " did not start at message # "+ EXPECT_FIRST_MSG +", instead started at msg # "
							+ currentValue);
					rc++;
					rc_final += rc;
					LOG_ERROR_COUNT++;
				} else {
					System.out.println("The Publisher " + key
							+ " correctly started at message # "+ EXPECT_FIRST_MSG);
				}

				for (int i = 1; i < pubResultList.size(); i++) {
					next = pubResultList.get(i);
					real_count++;
					if (currentValue.equals(next - 1)) {
						currentValue = next;
					} else {
						if ((i + 1) < pubResultList.size()) {
							i++;
							Long resume = pubResultList.get(i);
							real_count++;
							if (currentValue.equals(next)) {
								System.out.println("ERROR:  Publisher: " + key
										+ " Missed Message # " + (currentValue + 1)
										+ " and found duplicate msg # " + next
										+ " instead. Resume validation at msg # "
										+ resume);
							} else {
								System.out.println("ERROR:  Publisher: " + key
										+ " Missed Message # " + (currentValue + 1)
										+ " and found msg # " + next
										+ " instead. Resume validation at msg # "
										+ resume);
							}
							currentValue = resume;
						} else {
							System.out.println("ERROR:  Publisher: " + key
									+ " Missed Message # " + (currentValue + 1)
									+ " and found msg # " + next
									+ " instead. Reached the EOF.");

						}
						rc++;
						rc_final += rc;
						LOG_ERROR_COUNT++;
					}
				} // for pubResultList.size()
				currentValue++; // to account for Index 0 being first msg, not 1
				if (MSG_COUNT == real_count) {
					System.out.println("Publisher " + key + " received "
							+ real_count + " messsage and expected " + MSG_COUNT
							+ " messages");
				} else {
					System.out.println("ERROR:  Publisher " + key
							+ " did not receive " + MSG_COUNT
							+ " expected messages, instead received " + real_count
							+ " msgs.");
					rc++;
					rc_final += rc;
					LOG_ERROR_COUNT++;
				}
			}
			System.out.println("%==>  Exit: validateResults() with "+ LOG_ERROR_COUNT +" errors found so far, This pass ended with rc="+rc);
			System.out.println("%==> ");

		}
		return rc;
	}

	private int parseResponse( String jsonString, String drillType, String drillID , int chunkCount) {
		// DEBUG=false;
		if ( jsonString.startsWith("{\"err\":") ) {
			// Sometimes you get:  {"err":"Connection not established","errmsg":"Connection not established (SQL Code: -79730, SQL State: IX000)","ok":false}
			System.out.println("Unexpected Error returned: "+ jsonString);
			return 1;
		} else {
			// Other times you get expected data:  [{"device_id":"5","device_type":"
			Object object = JSONValue.parse(jsonString);
			JSONArray jArray = (JSONArray) object;
			Boolean theRetain = null;
			Long theIndex = null;
			Long tsMsgCount = null;
			int rc = 0;
			int rc_final = 0;
			String theClient = null;
			String theTopic = null;
			String theQoS = null;
			String thePayload = null;
			String theEventType = null;
			String theDeviceType = drillType;
			String theDeviceId = drillID;
			String theTimestamp = null;


			System.out.println("jArray.length=" + jArray.size()
					+ " messages in chunk "+ chunkCount );
			for (int i = 0; i < jArray.size(); i++) {

				String jsonText = jArray.get(i).toString();
				if (DEBUG)
					System.out.println("jArray[" + i + "]=" + jsonText);

				// https://code.google.com/p/json-simple/wiki/DecodingExamples
				JSONParser parser = new JSONParser();
				ContainerFactory containerFactory = new ContainerFactory() {
					public List creatArrayContainer() {
						return new LinkedList();
					}

					public Map createObjectContainer() {
						return new LinkedHashMap();
					}

				};

				try {
					Map json = (Map) parser.parse(jsonText, containerFactory);
					Iterator iter = json.entrySet().iterator();
					if (DEBUG)
						System.out.println("==iterate result==");
					while (iter.hasNext()) {
						Map.Entry entry = (Map.Entry) iter.next();
						String key = (String) entry.getKey();
						// SPLIT at the Commas - not without flaws, does not handle
						// element with multiple values i.e. evt:{.., payload={}, ..
						// }
						String[] elements = entry.getValue().toString().split(",");

						if (DEBUG)
							System.out.println(key + "[" + i + "] => "
									+ entry.getValue());

						// 'entry' has the complete message with the iot added JSON
						// pairs and original msg payload as 'evt' keyword's value.
						// Processing:
						// {"device_id":"d:u1ha7:mqttPub:pub02","device_type":"mqttPub","evt_type":"mqttbench","timestamp":{"$date":1411532867000},"evt"...
						// Save the 'device_*' and 'evt_type' values for drillDown
						// when arrive at the evt element, you will pick it apart
						// for Index:
						// "evt":{"Pub":"d:u1ha7:mqttPub:pub02","QoS":0,"Retain":false,"Index":999,"payload":"Sensor0:u1ha7,Sensor1:pub02,Sensor2:20,Sensor3:IncompleteInnerJSON"}},
						// note the quotes on the values of "evt" will get stripped
						// along the way somewhere...grr

						if (key.equalsIgnoreCase(DEVICE_TYPE)) {
							theDeviceType = elements[0];

						} else if (key.equals(DEVICE_ID)) {
							theDeviceId = elements[0];

						} else if (key.equals(EVT_TYPE)) {
							theEventType = elements[0];

						} else if (key.equals(TIMESTAMP)) {
							String[] theTS = elements[0].split("=");
							theTimestamp = theTS[1];
							theTimestamp = theTimestamp.replace("}", "");

							// if 'evt', need to get the publisher clientId and the
							// Index to track ordering and completeness (msg loss)
							// unfortunately the original payload had the String
							// Quotes eaten and can not re-MAP that element and
							// iterate the same way as before.
						} else if (key.equals(EVT)) {

							// Process Each element in the elementsList
							for (String s : elements) {
								if (DEBUG)
									System.out.println("Processing: " + s);
								String[] split_element = s.split("=");
								if (split_element[0].startsWith("{")) {
									split_element[0] = split_element[0].replace(
											"{", "");
								}
								split_element[0] = split_element[0].trim();

								if (split_element[0].equalsIgnoreCase("Pub")) {
									theClient = split_element[1];
								} else if (split_element[0]
										.equalsIgnoreCase("Topic")) {
									theTopic = split_element[1];
								} else if (split_element[0].equalsIgnoreCase("QoS")) {
									theQoS = split_element[1];
								} else if (split_element[0]
										.equalsIgnoreCase("Retain")) {
									theRetain = new Boolean(split_element[1]);
								} else if (split_element[0]
										.equalsIgnoreCase("Index")) {
									theIndex = new Long(split_element[1]);
								} else if (split_element[0]
										.equalsIgnoreCase("payload")) {
									thePayload = split_element[1];
								} else {
									if (DEBUG)
										System.out.println("Unknown KEYWORD "
												+ split_element[0]
														+ " in Parsed MSG: "
														+ entry.getValue());

								} // end of parse on elements[0]

							} // for elementList iterElementList

						} // parse evt element

					} // while there are more elements in this row to process

					// Finished parsing the row, Check for REQUIRED evt pieces to
					// build the pubLinkedList of MsgIndex's and drillDownDevice
					// Hashtable
					if ( theClient != null && theIndex != null && theTopic != null ) {

						/*
						 * DO I NEED TO WORRY ABOUT DUPLICATE MSGs...? Should they
						 * be logged in HISTORIAn, is it an error?
						 */

						// Put the [ClientId | Topic] key into the PubIndexTable
						// Hashtable if not there already and add/update the linked
						// list of Msg Indexes
						String theKey = theClient + "|" + theTopic;
						pubIdxLinkedList = publishersIndexTable.get(theKey);
						if (pubIdxLinkedList == null) {
							pubIdxLinkedList = new LinkedList<Long>();
							publishersIndexTable.put(theKey, pubIdxLinkedList);
						}
						pubIdxLinkedList.add(theIndex);

						// Put the [clientId | Topic] key into the PubTimestampTable
						// if not there already and add/update the linked list of
						// timestamps
						pubTSLinkedList = publishersTimestampTable.get(theKey);
						if (pubTSLinkedList == null) {
							pubTSLinkedList = new LinkedList<Long>();
							publishersTimestampTable.put(theKey, pubTSLinkedList);
						}
						pubTSLinkedList.add(new Long(theTimestamp));

						if ( rebuild_DrillDownDevice ) {
							// Put the [ClientId | Topic] key into the DrillDownDevice
							// Hashtable if not there already and add/update the linked
							// list
							deviceStats = drillDownDevice.get(theKey);
							if (deviceStats == null) {
								// Build what you can for the DrillDownDevice, will be
								// completed in validateResult()
								deviceStats = new String[7];
								if (theDeviceId == null ) {
									theDeviceId = drillID;
								}
								deviceStats[IDX_DEVICE_ID] = new String(theDeviceId);

								if ( theDeviceType == null ){
									theDeviceType = drillType;
								}
								deviceStats[IDX_DEVICE_TYPE] = new String(theDeviceType);
								deviceStats[IDX_EVT_TYPE] = new String(theEventType);
								deviceStats[IDX_TOTAL_MSGS] = "1";
								drillDownDevice.put(theKey, deviceStats);
							} else {
								Long msgCount = new Long(
										deviceStats[IDX_TOTAL_MSGS]);
								msgCount++;
								deviceStats[IDX_TOTAL_MSGS] = msgCount.toString();
								drillDownDevice.put(theKey, deviceStats);
							}
						}
						if (DEBUG)
							System.out.println(
									"Put ClientId in HashTable(s) and the Index into the Linked List.");

					} else {
						// THIS Could Be a FILTER Response, check that before giving up....
						String theURL = histWebTarget.getUri().toString();
						
						if ( theURL.contains(SUM_TYPE_AVG) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_AVG +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_AVG +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}
							
						} else if ( theURL.contains(SUM_TYPE_COUNT) ) {
							String[] parts = jsonText.split(":");
							int int_MSG_COUNT = (int)Math.round(MSG_COUNT);
							if ( parts[1].startsWith( String.valueOf(int_MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_COUNT +" and expected result was found: "+ int_MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_COUNT +" and expected result was NOT found: "+ int_MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}
						
						} else if ( theURL.contains(SUM_TYPE_MAX) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_MAX +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==> ERROR:  The Query is: "+ SUM_TYPE_MAX +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}
						
						} else if ( theURL.contains(SUM_TYPE_MIN) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_MIN +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_MIN +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}
						
						} else if ( theURL.contains(SUM_TYPE_RANGE) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_RANGE +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_RANGE +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}

						} else if ( theURL.contains(SUM_TYPE_STDEV) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_STDEV +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_STDEV +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}

						} else if ( theURL.contains(SUM_TYPE_VARY) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_VARY +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_VARY +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}
						// HINT:  Keep 'sum' last so it does not get confused with other keywords containing the chars 'SUM' 	
						} else if ( theURL.contains(SUM_TYPE_SUM) ) {
							String[] parts = jsonText.split(":");
							if ( parts[1].startsWith( String.valueOf(MSG_COUNT) )) {
								System.out.println("%==>  The Query is: "+ SUM_TYPE_SUM +" and expected result was found: "+ MSG_COUNT);
							} else {
								System.out.println("%==>  ERROR:  The Query is: "+ SUM_TYPE_SUM +" and expected result was NOT found: "+ MSG_COUNT 
										+".  Instead this was JSON payload:  "+ jsonText);
								rc++;
								rc_final += rc;
								LOG_ERROR_COUNT++;
							}
						
						} else {
							//  REALLY CAN'T handle it...
							System.out.println(
									"ERROR: CAN NOT DO CHECKING without the CLIENTID, Topic And INDEX information"
											+ " in the JSON payload: " + jsonText);
							rc++;
							rc_final += rc;
							LOG_ERROR_COUNT++;
						}
					} // parse evt element's pieces

					if (DEBUG)
						System.out.println("==toJSONString()==");
					if (DEBUG)
						System.out.println(JSONValue.toJSONString(json));
				} catch (ParseException pe) {
					System.out.println(pe);
					rc++;
					rc_final += rc;
					LOG_ERROR_COUNT++;
				}
			} // For as long as there are more rows to process.

			if ( rebuild_DrillDownDevice ) {
				// Compute the FINAL deviceStats TimeStamp values for each entry in the
				// drillDownDevice hashtable now that all rows are processed
				Long half_time = 0L;
				Long qtr_time = 0L;
				// For EACH Publisher in drillDownDevice, find the HALF and QTR
				// timestamp and update the timestamp values in deviceStats
				Enumeration<String> enumKey = drillDownDevice.keys();
				while (enumKey.hasMoreElements()) {
					String key = enumKey.nextElement();
					deviceStats = new String[7];
					deviceStats = drillDownDevice.get(key);

					LinkedList<Long> tsLinkedList = publishersTimestampTable.get(key);
					if ( tsLinkedList != null ) {
						Collections.sort(tsLinkedList);
						List<Long> arrayTS = new ArrayList<Long>(tsLinkedList);
						half_time = (new Long( tsLinkedList.size() )) / 2;
						qtr_time  = (new Long( tsLinkedList.size() )) / 4;
						int int_half_time = half_time.intValue();
						int int_qtr_time = qtr_time.intValue();

						deviceStats[IDX_START_TS] = arrayTS.get(int_qtr_time).toString();
						deviceStats[IDX_END_TS] = arrayTS.get(int_half_time).toString();
						deviceStats[IDX_TS_MSGS] = String.valueOf(int_half_time
								- int_qtr_time);
						if (deviceStats[IDX_TS_MSGS].equals("0")) {
							deviceStats[IDX_TS_MSGS] = "1";
						}
						drillDownDevice.put(key, deviceStats);
					}
				}
			}
			System.out.println("Exit: parseResponse completed with rc="+ rc );
			return rc;
		}
	}

	private int computeSUM ( String firstNum, String lastNum ){
		int theSum = 0;
		int firstInt = Integer.parseInt(firstNum);
		int lastInt = Integer.parseInt(lastNum);
		for ( int i = firstInt ; i <= lastInt ; i++){
			theSum += i;
		}
		return theSum;
	}

	private int computeRANGE ( String firstNum, String lastNum ){
		int firstInt = Integer.parseInt(firstNum);
		int lastInt = Integer.parseInt(lastNum);
		int theRANGE = lastInt - firstInt + 1;
		return theRANGE;
	}

	private int computeSIZE ( String firstNum, String lastNum ){
		int theSIZE = 0;
		int firstInt = Integer.parseInt(firstNum);
		int lastInt = Integer.parseInt(lastNum);
		for ( int i = firstInt ; i <= lastInt ; i++) {
			theSIZE += 1;
		}
		return theSIZE;
	}

	private double computeAVG ( String firstNum, String lastNum ){
		double theAvg = 0L;
		theAvg = (double)computeSUM(firstNum, lastNum) / (double)computeSIZE(firstNum, lastNum);
		return theAvg;
	}

	private double computeVARIANCE ( String firstNum, String lastNum ){
		double theVariance = 0L;
		int firstInt = Integer.parseInt(firstNum);
		int lastInt = Integer.parseInt(lastNum);

		double theAvg = computeAVG(firstNum, lastNum);
		double theTotal = 0L;
		for ( int i = firstInt ; i <= lastInt ; i++){
			theTotal += (theAvg - i)*(theAvg - i);
		}
		
		theVariance = theTotal/computeSIZE(firstNum, lastNum);
		return theVariance;
	}

	private double computeSTDEV ( String firstNum, String lastNum ){
		double theSTDEV = Math.sqrt( computeVARIANCE(firstNum, lastNum));
		return theSTDEV;
	}
}
