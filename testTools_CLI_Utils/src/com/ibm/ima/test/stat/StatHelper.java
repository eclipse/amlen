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
package com.ibm.ima.test.stat;

import java.io.FileInputStream;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;

import javax.net.ssl.SSLContext;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;

import com.ibm.ima.test.stat.jms.JMSConsumer;
import com.ibm.ima.test.stat.jms.JMSProducer;
import com.ibm.ima.test.stat.mqtt.PahoConnectionStorm;
import com.ibm.ima.test.stat.mqtt.PahoConsumer;
import com.ibm.ima.test.stat.mqtt.PahoProducer;


/**
 * JMSStatHelper is a super simple JMS client driver that may be used to
 * produce and consume data from a queue or queues. Producer and consumer
 * configurations are obtain via properties file. Any number of producer
 * and consumer threads may be specified and created.
 *
 */
public class StatHelper {
	
	 static SSLContext sslContext;

	// the properties file containing consumer and producer configurations
	private static String configPropsFile = null;
	
	// verbose output during execution
	private static boolean verbose = false;
	
	// debug output during execution
	private static boolean debug = false;
	
	private static String debugOut = "stdout";
	
	private static String hostname = null;
	
	private static String keyFileRoot = null;
	
	private static boolean sendSummaryToFile = false;


	/**
	 * Given a List of Config instances (producer configurations) create the 
	 * applicable producer thread instances. The threads will be created but
	 * not started by this method. The threads will be added to the List
	 * provided.
	 * 
	 * @param producerConfigList  The List of Config instances - producers
	 * @param producerList        The List that producer threads will
	 *                            added to
	 */
	private static void addProducerThreads(List<Config> producerConfigList, List<Runnable> producerList, ExecutionMonitor monitor) {

		for (Config produrcerConfig : producerConfigList) {
			int numClients = produrcerConfig.getThreadCount();
			for (int i = 1; i<= numClients; i++) {
				String clientId = produrcerConfig.getClientIdPrefix() + i;
				monitor.addProducerClient(clientId);
				Runnable producer = null;
				if (Config.CLIENT_TYPE.MQTT.equals(produrcerConfig.getClientType())) {
					producer = new PahoProducer(produrcerConfig, clientId, monitor, verbose);
				} else if (Config.CLIENT_TYPE.JMS.equals(produrcerConfig.getClientType())) {
					producer = new JMSProducer(produrcerConfig, clientId, monitor, verbose);
				} else if (Config.CLIENT_TYPE.MQTT_CONN_STORM.equals(produrcerConfig.getClientType())) {
					producer = new PahoConnectionStorm(produrcerConfig, clientId, monitor, verbose);
				}
				producerList.add(producer);
			}
		}

	}

	
	/**
	 * Given a List of Config instances (consumer configurations) create the 
	 * applicable consumer thread instances. The threads will be created but
	 * not started by this method. The threads will be added to the List
	 * provided.
	 * 
	 * @param consumerConfigList  The List of Config instances - consumers
	 * @param consumerList        The List that consumer threads will
	 *                            added to
	 */
	private static void addConsumerThreads(List<Config> consumerConfigList, List<Runnable> consumerList, ExecutionMonitor monitor) {

		for (Config consumerConfig : consumerConfigList) {
			int numClients = consumerConfig.getThreadCount();
			for (int i = 1; i<= numClients; i++) {
				String clientId = consumerConfig.getClientIdPrefix() + i;
				monitor.addConsumerClient(clientId);
				Runnable consumer = null;
				if (Config.CLIENT_TYPE.MQTT.equals(consumerConfig.getClientType())) {
					consumer = new PahoConsumer(consumerConfig, clientId, monitor, verbose);
				} else {
					consumer = new JMSConsumer(consumerConfig, clientId, i, monitor, verbose);
				}
				consumerList.add(consumer);
			}
		}

	}


	/**
	 * Parse to command line provided to the JMSStatHelper main method.
	 * 
	 * @param args  Command line arguments provided
	 */
	private static void processCommandLine(String[] args) {

		// config property file...
		OptionBuilder.withArgName( "hostname" );
		OptionBuilder.hasArg();
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("optional address that will override any host specified in config properties" );
		Option hostOption = OptionBuilder.create("host");

		OptionBuilder.withArgName( "keyfileroot" );
		OptionBuilder.hasArg();
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("optional root directory to prepend to keyfile path" );
		Option keyFileRootOption = OptionBuilder.create("keyfileroot");
		
		OptionBuilder.withArgName( "file" );
		OptionBuilder.hasArg();
		OptionBuilder.isRequired(true);
		OptionBuilder.withDescription("Properties file specifying producer and consumer configurations." );
		Option configData = OptionBuilder.create("configdata");

		// verbose output....
		OptionBuilder.withArgName("verbose");
		OptionBuilder.hasArg(false);
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("Enable verbose output - optional." );
		Option verboseOutput = OptionBuilder.create("verbose");
		
		// verbose output....
		OptionBuilder.withArgName("output location - default stdout");
		OptionBuilder.hasOptionalArgs(1);
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("Enable debug output - optional." );
		Option debugOutput = OptionBuilder.create("debug");
		
		// verbose output....
		OptionBuilder.withArgName("savesummary");
		OptionBuilder.hasArg(false);
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("Save summary data to files" );
		Option savesummaryOption = OptionBuilder.create("savesummary");

		// execute options
		Options executeOptions = new Options();
		executeOptions.addOption(configData);
		executeOptions.addOption(hostOption);
		executeOptions.addOption(keyFileRootOption);
		executeOptions.addOption(verboseOutput);
		executeOptions.addOption(debugOutput);
		executeOptions.addOption(savesummaryOption);
		

		// create a help option
		Option help = new Option( "help", "print this message" );
		Options helpOption = new Options();
		helpOption.addOption(help);

		// create an all options used for printing usage
		Options allOptions = new Options();
		allOptions.addOption(help);
		allOptions.addOption(configData);
		allOptions.addOption(hostOption);
		allOptions.addOption(keyFileRootOption);
		allOptions.addOption(verboseOutput);
		allOptions.addOption(debugOutput);
		allOptions.addOption(savesummaryOption);

		// make sure there is at leas one option...
		if (args == null || args.length == 0) {
			
			HelpFormatter formatter = new HelpFormatter();
			formatter.printHelp( "JMSStathelper", allOptions );
			System.exit(1);
		
		} else {

			// parse the command line
			CommandLineParser parser = new GnuParser();

			try {

				// parse the command line arguments
				CommandLine line = parser.parse( helpOption, args, true );
				// has the help argument been passed?
				if( line.hasOption( "help" ) ) {

					// just need some help
					HelpFormatter formatter = new HelpFormatter();
					formatter.printHelp( "JMSStathelper", allOptions );
					System.exit(0);

				} else {

					// everything looks good - set the appropriate values...
					line = parser.parse( executeOptions, args );
					configPropsFile = line.getOptionValue("configdata");
					hostname = line.getOptionValue("host");
					keyFileRoot = line.getOptionValue("keyfileroot");

					if (configPropsFile == null) {
						// should not get here....
						ParseException pe = new ParseException("No configuration properties file specified.");
						throw pe;
					}

					verbose = line.hasOption("verbose"); 
					Config.setVerboseOutput(verbose);
					
					sendSummaryToFile = line.hasOption("savesummary");
					
					debug = line.hasOption("debug");
					if (debug) {
						String debugOutputLocation = line.getOptionValue("debug");
						if (debugOutputLocation != null) {
							debugOut = debugOutputLocation;
						}
					}

				}
				
			} catch( ParseException exp ) {

				// oops, something went wrong
				System.err.println(exp.getMessage());
				HelpFormatter formatter = new HelpFormatter();
				formatter.printHelp( "JMSStathelper", allOptions );
				System.exit(1);

			}

		}
		
	}


	public static void main(String[] args) {
		

		// first parse the command line...
		processCommandLine(args);
		
		if (debug) {
			System.setProperty("IMATraceLevel", "9");
	        System.setProperty("IMATraceFile", debugOut);
		}

		// configuration input stream
		FileInputStream fis = null;
		
		// configuration properties
		Properties statTestProps = new Properties();
		
		// List of producer configurations
		List<Config> producerConfigList = null;
		
		// List of consumer configurations 
		List<Config> consumerConfigList = null;
		
		// List of Producer threads
		List<Runnable> producerList = new ArrayList<Runnable>();
		
		// List of Consumer threads
		List<Runnable> consumerList = new ArrayList<Runnable>();

		try {
			
			// load the configuration properties file
			fis = new FileInputStream(configPropsFile);
			statTestProps.load(fis);
			
			Config.initSecurity(statTestProps, keyFileRoot);
			// get consumer and producer configurations
			producerConfigList = Config.getProducerConfigList(statTestProps, hostname);
			consumerConfigList = Config.getConsumerConfigList(statTestProps, hostname);
			
		} catch (FileNotFoundException fne) {
			System.out.println("Could not open configuration properties file <" + configPropsFile + ">" );
			fne.printStackTrace();
			System.exit(1);
		} catch (IOException ioe) {
			System.out.println("Could not correctly process the propertie in the file <" + configPropsFile + ">" );
			ioe.printStackTrace();
			System.exit(1);
		} catch (Exception e) {
			System.out.println("An unrecoverable error occurred.");
			e.printStackTrace();
			System.exit(1);
		} finally {
			try {
				fis.close();
			} catch (Throwable t) {

			}
		}

		// create monitor object....
		ExecutionMonitor monitor = new ExecutionMonitor(sendSummaryToFile);
		
		// create the threads....
		addProducerThreads(producerConfigList, producerList, monitor);
		addConsumerThreads(consumerConfigList, consumerList, monitor);

		// create monitor thread
		Thread monitorThread = new Thread(monitor);
		monitorThread.start();
		monitor.setProducerList(producerList);
		
		for (Runnable consumer : consumerList) {
			Thread t = new Thread(consumer);
			t.start();
		}
		




	}

}
