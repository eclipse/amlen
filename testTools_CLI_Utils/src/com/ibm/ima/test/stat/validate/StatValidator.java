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
package com.ibm.ima.test.stat.validate;

import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

import org.apache.commons.cli.CommandLine;
import org.apache.commons.cli.CommandLineParser;
import org.apache.commons.cli.GnuParser;
import org.apache.commons.cli.HelpFormatter;
import org.apache.commons.cli.Option;
import org.apache.commons.cli.OptionBuilder;
import org.apache.commons.cli.Options;
import org.apache.commons.cli.ParseException;
import org.supercsv.cellprocessor.ift.CellProcessor;
import org.supercsv.io.CsvBeanReader;
import org.supercsv.io.ICsvBeanReader;
import org.supercsv.prefs.CsvPreference;


public class StatValidator {


	public boolean verbose = false;
	public boolean sortData = false;

	public String expectedCSVFile = null;
	public String obtainedCSVFile = null;
	public CSV_DATA_FORMAT csvFormatRequested = null;
	
    private enum CSV_DATA_FORMAT {

		TOPIC("TOPIC"),
		QUEUE("QUEUE"),
		SUBSCRIPTION("SUBSCRIPTION"),
		MQTTCLIENT("MQTTCLIENT"),
		CONNECTION("CONNECTION");

		private String value;


		CSV_DATA_FORMAT(String value) {
			this.value = value;
		}


		public String getText() {
			return this.value;
		}


		public static CSV_DATA_FORMAT fromString(String value) {
			if (value != null) {
				for (CSV_DATA_FORMAT b :CSV_DATA_FORMAT.values()) {
					if (value.equalsIgnoreCase(b.value)) {
						return b;
					}
				}
			}
			return null;
		}
	}

	/**
	 * Parse to command line provided to the JMSStatHelper main method.
	 * 
	 * @param args  Command line arguments provided
	 */
	private void processCommandLine(String[] args) {

		// config property file...
		OptionBuilder.withArgName( "file" );
		OptionBuilder.hasArg();
		OptionBuilder.isRequired(true);
		OptionBuilder.withDescription("A CSV file with expected values." );
		Option expectedCSV = OptionBuilder.create("expected");

		// config property file...
		OptionBuilder.withArgName( "file" );
		OptionBuilder.hasArg();
		OptionBuilder.isRequired(true);
		OptionBuilder.withDescription("A CSV file with obtained values." );
		Option obtainedCSV = OptionBuilder.create("obtained");
		
		// config property file...
		OptionBuilder.withArgName( "topic|queue|subscription" );
		OptionBuilder.hasArg();
		OptionBuilder.isRequired(true);
		OptionBuilder.withDescription("What stat type is represented in the csv file." );
		Option csvFormat = OptionBuilder.create("csvformat");

		// verbose output....
		OptionBuilder.withArgName("verbose");
		OptionBuilder.hasArg(false);
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("Enable verbose output - optional." );
		Option verboseOutput = OptionBuilder.create("verbose");
		
		// sort CSV data before compare....
		OptionBuilder.withArgName("sortcsv");
		OptionBuilder.hasArg(false);
		OptionBuilder.isRequired(false);
		OptionBuilder.withDescription("Sort CSV data before compare - optional." );
		Option sortCSV = OptionBuilder.create("sortcsv");

		// execute options
		Options executeOptions = new Options();
		executeOptions.addOption(expectedCSV);
		executeOptions.addOption(obtainedCSV);
		executeOptions.addOption(verboseOutput);
		executeOptions.addOption(sortCSV);
		executeOptions.addOption(csvFormat);


		// create a help option
		Option help = new Option( "help", "print this message" );
		Options helpOption = new Options();
		helpOption.addOption(help);

		// create an all options used for printing usage
		Options allOptions = new Options();
		allOptions.addOption(help);
		allOptions.addOption(expectedCSV);
		allOptions.addOption(obtainedCSV);
		allOptions.addOption(verboseOutput);
		allOptions.addOption(sortCSV);
		allOptions.addOption(csvFormat);

		// make sure there is at least one option...
		if (args == null || args.length == 0) {

			HelpFormatter formatter = new HelpFormatter();
			formatter.printHelp( "QueueStatValidator", allOptions );
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
					formatter.printHelp( "QueueStatValidator", allOptions );
					System.exit(0);

				} else {

					// everything looks good - set the appropriate values...
					line = parser.parse( executeOptions, args );
					expectedCSVFile = line.getOptionValue("expected");
					obtainedCSVFile = line.getOptionValue("obtained");

					if (expectedCSVFile == null) {
						// should not get here....
						ParseException pe = new ParseException("No expected values CSV file specified.");
						throw pe;
					}

					if (obtainedCSVFile == null) {
						// should not get here....
						ParseException pe = new ParseException("No obtained values CSV file specified.");
						throw pe;
					}

					verbose = line.hasOption("verbose"); 
					sortData = line.hasOption("sortcsv"); 
					
					String csvFormatString = line.getOptionValue("csvformat");
					csvFormatRequested = CSV_DATA_FORMAT.fromString(csvFormatString);
					if (csvFormatRequested == null) {
						ParseException pe = new ParseException("csv format must be topic|queue|subscription");
						throw pe;
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
	
	private CellProcessor[] getCellProcessorArray() {
		
		CellProcessor[] processorArray = null;
		
		if (CSV_DATA_FORMAT.QUEUE.equals(csvFormatRequested)) {
			processorArray = QueueStatBean.queueStatProcessor;
		} else if (CSV_DATA_FORMAT.TOPIC.equals(csvFormatRequested)) {
			processorArray = TopicStatBean.topicStatProcessor;
		} else if (CSV_DATA_FORMAT.SUBSCRIPTION.equals(csvFormatRequested)) {
			processorArray = SubscriptionStatBean.subscriptionStatProcessor;
		} else if (CSV_DATA_FORMAT.MQTTCLIENT.equals(csvFormatRequested)) {
			processorArray = MQTTClientStatBean.mqttclientStatProcessor;
		} else if (CSV_DATA_FORMAT.CONNECTION.equals(csvFormatRequested)) {
			processorArray = ConnectionStatBean.connectionStatProcessor;
		}
		
		return processorArray;
		
	}

	@SuppressWarnings("rawtypes")
	private Class getBeanClass() {
		
		Class beanClass = null;
		
		if (CSV_DATA_FORMAT.QUEUE.equals(csvFormatRequested)) {
			beanClass = QueueStatBean.class;
		} else 	if (CSV_DATA_FORMAT.TOPIC.equals(csvFormatRequested)) {
			beanClass = TopicStatBean.class;
		} else 	if (CSV_DATA_FORMAT.SUBSCRIPTION.equals(csvFormatRequested)) {
			beanClass = SubscriptionStatBean.class;
		} else 	if (CSV_DATA_FORMAT.MQTTCLIENT.equals(csvFormatRequested)) {
			beanClass = MQTTClientStatBean.class;
		} else 	if (CSV_DATA_FORMAT.CONNECTION.equals(csvFormatRequested)) {
			beanClass = ConnectionStatBean.class;
		}
		
		return beanClass;
		
	}
	
	@SuppressWarnings({ "rawtypes", "unchecked" })
	private List<Object> parseCSVFile(String csvFileName) throws IOException  {

		List<Object> expectedValueList = new ArrayList<Object>();
		
		CellProcessor[] processorArray = getCellProcessorArray();
		Class beanClass = getBeanClass();

		// Use SuperCSV BeanReader to process CSV
		ICsvBeanReader inFile = null;
		try {

			inFile = new CsvBeanReader(new FileReader(csvFileName), CsvPreference.EXCEL_PREFERENCE);
			final String[] header = inFile.getCSVHeader(true);
			Object statBean = null;


			while( (statBean = inFile.read(beanClass, header, processorArray)) != null) {

				expectedValueList.add(statBean);

			}
		
		} finally {
			try {
				// tidy up a bit...
				inFile.close();
			} catch (Exception e) {
			}

		}

		return expectedValueList;

	}

	@SuppressWarnings({ "rawtypes", "unchecked" })
	private boolean compareCSVFiles(List expectedList, List obtainedList) {

		boolean areEqual = true;

		if (expectedList.size() != obtainedList.size()) {
			System.out.println("Epected CSV and obtained CSV are of different size.");
			System.out.println("Epected CSV size : " + expectedList.size());
			System.out.println("Obtained CSV size : " + obtainedList.size());
			areEqual = false;
		}

		if (sortData) {
			Collections.sort(expectedList);
			Collections.sort(obtainedList);
		}
		
		if (areEqual) {
			for (int i=0; i<expectedList.size(); i++) {

				System.out.println("Comparing data row <" + (i+1) + "> of expected and obtained CSV data.");

				Object expectedBean = expectedList.get(i);
				Object obtainedBean = obtainedList.get(i);

				if (obtainedBean.equals(expectedBean) == false) {
					areEqual = false;
					break;
				}
			}
		}

		if (areEqual) {
			System.out.println("SUCCESS! Expected and obtained CSV are equal.");
		} else {
			System.out.println("ERROR! Expected and obtained CSV are not equal!");
		}

		return areEqual;
	}
	
	
	private boolean validate() {
		
		boolean areEqual = false;
		
		List<Object> expectedList = null;
		List<Object> obtainedList = null;
		
		try {
			expectedList = parseCSVFile(expectedCSVFile);
			obtainedList = parseCSVFile(obtainedCSVFile);
			if (expectedList != null && obtainedList != null) {
				areEqual = compareCSVFiles(expectedList, obtainedList);
			} 
		} catch (Exception e) {
			System.out.println("Could not parse the obtained CSV data!");
			e.printStackTrace();
		}
		

		return areEqual;

		
	}

	/**
	 * @param args
	 */
	public static void main(String[] args) {

		int exitCode = 1;
		StatValidator validator = new StatValidator();
		validator.processCommandLine(args);
		boolean areEqual = validator.validate();
		
		if (areEqual) {
			exitCode = 0;
		}
		
		System.exit(exitCode);
		
	}
}
