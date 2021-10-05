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

package com.ibm.ima.test.cli.policy;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.FileReader;
import java.io.IOException;
import java.io.OutputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;
import java.util.zip.ZipOutputStream;

import org.supercsv.io.CsvBeanReader;
import org.supercsv.io.ICsvBeanReader;
import org.supercsv.prefs.CsvPreference;

import com.ibm.gvt.testdata.DataSource;
import com.ibm.gvt.testdata.DataSource.TestData;
import com.ibm.ima.test.cli.policy.Constants.TESTCASE_TYPE;
import com.ibm.ima.test.cli.policy.csv.PolicyTestBean;
import com.ibm.ima.test.cli.testplan.xml.IPolicyValidator;
import com.ibm.ima.test.cli.testplan.xml.ValidationXML;

public class PolicyTestSuite {

	private static String SEP =  "\n";
	
	private String zipBaseDir = null;
	private String testSuiteId = null;
	private String testSuiteName = null;
	private String testPlanFilename = null;
	private TESTCASE_TYPE type = null;
	private boolean doUpdates = false;
	
	public PolicyTestSuite(String testSuiteId, String testSuiteName, String testPlanFilename, TESTCASE_TYPE type) {
		
		this.zipBaseDir = "policy_tests/" + testSuiteId + "/";
		this.testSuiteId = testSuiteId;
		this.testPlanFilename = testPlanFilename;
		this.testSuiteName = testSuiteName;
		this.type = type;
		
	}

	public void createTestSuite(int portStart, String zipOutFilename) {

		FileOutputStream fos = null;
		ZipOutputStream zos = null;
		List<PolicyTestCase> testCaseList = new ArrayList<PolicyTestCase>();

		// Use SuperCSV BeanReader to process CSV
		ICsvBeanReader inFile = null;
		try {

			fos = new FileOutputStream(zipOutFilename);
			zos = new ZipOutputStream(fos);

			inFile = new CsvBeanReader(new FileReader(testPlanFilename), CsvPreference.EXCEL_PREFERENCE);

			PolicyTestBean policyBean = null;
			int port = portStart;

			while( (policyBean = inFile.read(PolicyTestBean.class, PolicyTestBean.header, PolicyTestBean.policyTestProcessor)) != null) {

				String testCaseIdentifier = "" + port;
				policyBean.setTestCaseIdentifier(testCaseIdentifier);

				PolicyTestCase testCase = new PolicyTestCase(policyBean, testSuiteName, testCaseIdentifier, port, type);
				testCase.setDoUpdates(doUpdates);
				testCase.buildTestCase();
				testCaseList.add(testCase);

				port++;

			}

			writeTestCaseFiles(testCaseList, zos);
			writeScenarioFile(testCaseList, zos);

		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				// tidy up a bit...
				inFile.close();
			} catch (Exception e) {
			}
			try {
				zos.close();
			} catch (Exception e2) {
			}

			try {
				fos.close();
			} catch (Exception e2) {
			}
		}

	}
	
	public void createGVTTestSuite(int portStart, String protocol, String zipOutFilename) {

		FileOutputStream fos = null;
		ZipOutputStream zos = null;
		List<PolicyTestCase> testCaseList = new ArrayList<PolicyTestCase>();

		String filename="gvt/gvtdata.xml";
		DataSource source=new DataSource(new File(filename));
		TestData gvtData = null;
		try {

			fos = new FileOutputStream(zipOutFilename);
			zos = new ZipOutputStream(fos);

			

			PolicyTestBean policyBean = null;
			int port = portStart;

			for (int i=1; i<=45; i++) {
				
				if (i==1 || i==10 || i==30 || i==35) {
					port++;
					continue;
				}
				
				//Skipping these tests for MQTT 3.1.1
				if (protocol.equals("MQTT") && (i==32 || i==39)) {
					port++;
					continue;
				}
				gvtData = source.getById(i);

				String testCaseIdentifier = "ID_" + i;
				policyBean = new PolicyTestBean();
				String gvtString = new String(gvtData.getHexChars());
				policyBean.setGvtString(gvtString);
				policyBean.setCpProtocol(protocol);
				policyBean.setEpInterface("A1");
				policyBean.setMp1Protocol(protocol);
				policyBean.setCpClientId(gvtString + "*");
				policyBean.setMp1ClientId(gvtString + "*");
			
				
				
				policyBean.setTestCaseIdentifier(testCaseIdentifier);

				PolicyTestCase testCase = new PolicyTestCase(policyBean, testSuiteName, testCaseIdentifier, port, type);
				testCase.setDoUpdates(doUpdates);
				testCase.buildTestCase();
				testCaseList.add(testCase);

				port++;

			}

			writeTestCaseFiles(testCaseList, zos);
			writeScenarioFile(testCaseList, zos);

		} catch (Exception e) {
			e.printStackTrace();
		} finally {
			try {
				zos.close();
			} catch (Exception e2) {
			}

			try {
				fos.close();
			} catch (Exception e2) {
			}
		}

	}
	
	private void writeScenarioFile(List<PolicyTestCase> testCaseList, ZipOutputStream zos) throws IOException {
		
		ZipEntry entry = new ZipEntry(zipBaseDir + testSuiteId + "_runScenarios.sh");
		zos.putNextEntry(entry);
		initScenarioScript(zos);
		for (PolicyTestCase testCase : testCaseList) {
			testCase.writeTestScript(zipBaseDir, zos);
		}
		
	}
	
	
	private void writeTestCaseFiles(List<PolicyTestCase> testCaseList, ZipOutputStream zos) throws IOException {
		
	
		
		for (PolicyTestCase testCase : testCaseList) {
			String testCaseIdentifier = testCase.getTestCaseIdentifier();
			String testCaseZipDir = zipBaseDir + testCaseIdentifier + "/";
			
			ZipEntry entry = new ZipEntry(testCaseZipDir + testSuiteName + "_" + testCaseIdentifier + ".cli");

			zos.putNextEntry(entry);

			testCase.writeCommands(zos);
			
			List<IPolicyValidator> xmlFiles = testCase.getXmlFiles();
			for (IPolicyValidator xmlFile : xmlFiles) {
				String xmlFileName = xmlFile.getXmlName();
				entry = new ZipEntry(testCaseZipDir + xmlFileName);
				zos.putNextEntry(entry);
				// Specify testCaseIdentifier as the unique namespace used by the sync driver
				ValidationXML.setTestName(testCaseIdentifier);
				ValidationXML.writeXml(xmlFile.getRootElementName(), xmlFile.getRootAction(), zos);
			}
			
			entry = new ZipEntry(testCaseZipDir + testSuiteName + "_" + testCaseIdentifier + "_runScenarios.sh");
			zos.putNextEntry(entry);
			initScenarioScript(zos);
			testCase.writeTestScript(zipBaseDir, zos);
			
		}
		
	}

	private void initScenarioScript(OutputStream out) throws IOException {
		StringBuffer buf = new StringBuffer();

		buf.append("#!/bin/bash" + SEP);
		buf.append("scenario_set_name=\"" + testSuiteName + "_Tests\"" + SEP);
		buf.append("typeset -i n=0" + SEP);
		buf.append(SEP);
		out.write(buf.toString().getBytes());

	}
	
	

	public void setDoUpdates(boolean doUpdates) {
		this.doUpdates = doUpdates;
	}

	public static void main(String[] args) {
		
		ArrayList<String> testCaseZips = new ArrayList<String>();
		
		testCaseZips.add("policyTestPlans/topic/manual/ptws_topic_manual.zip");
		testCaseZips.add("policyTestPlans/topic/automation/ptws_topic_auto.zip");
		testCaseZips.add("policyTestPlans/topic/bvt/ptws_topic_bvt.zip");
		testCaseZips.add("policyTestPlans/topic/manual/ptns_topic_manual.zip");
		testCaseZips.add("policyTestPlans/topic/automation/ptns_topic_auto.zip");
		testCaseZips.add("policyTestPlans/topic/automation/gvt_topic_jms_auto.zip");
		testCaseZips.add("policyTestPlans/topic/automation/gvt_topic_mqtt_auto.zip");

		PolicyTestSuite suite = new PolicyTestSuite("ptws_topic_manual","cli_PTWS_MANUAL", "policyTestPlans/topic/manual/ptws_topic_manual.csv", TESTCASE_TYPE.VALID_SEPARATION);
		suite.createTestSuite(11000, "policyTestPlans/topic/manual/ptws_topic_manual.zip");
		
		suite = new PolicyTestSuite("ptws_topic_auto","cli_PTWS_AUTO", "policyTestPlans/topic/automation/ptws_topic_auto.csv", TESTCASE_TYPE.VALID_SEPARATION);
		suite.setDoUpdates(true);
		suite.createTestSuite(12000, "policyTestPlans/topic/automation/ptws_topic_auto.zip");
		
		suite = new PolicyTestSuite("ptws_topic_bvt","cli_PTWS_BVT", "policyTestPlans/topic/bvt/ptws_topic_bvt.csv", TESTCASE_TYPE.VALID_SEPARATION);
		suite.setDoUpdates(true);
		suite.createTestSuite(13000, "policyTestPlans/topic/bvt/ptws_topic_bvt.zip");
		
		suite = new PolicyTestSuite("ptns_topic_manual","cli_PTNS_MANUAL", "policyTestPlans/topic/manual/ptns_topic_manual.csv", TESTCASE_TYPE.VALID_NO_SEPARATION);
		suite.createTestSuite(14000, "policyTestPlans/topic/manual/ptns_topic_manual.zip");
		
		suite = new PolicyTestSuite("ptns_topic_auto","cli_PTNS_AUTO", "policyTestPlans/topic/automation/ptns_topic_auto.csv", TESTCASE_TYPE.VALID_NO_SEPARATION);
		suite.setDoUpdates(true);
		suite.createTestSuite(15000, "policyTestPlans/topic/automation/ptns_topic_auto.zip");
		
		suite = new PolicyTestSuite("gvt_topic_jms_auto","cli_GVT_JMS_AUTO", null, TESTCASE_TYPE.GVT_TOPIC);
		suite.setDoUpdates(true);
		suite.createGVTTestSuite(16001, "JMS", "policyTestPlans/topic/automation/gvt_topic_jms_auto.zip");
		
		suite = new PolicyTestSuite("gvt_topic_mqtt_auto","cli_GVT_MQTT_AUTO", null, TESTCASE_TYPE.GVT_TOPIC);
		suite.setDoUpdates(true);
		suite.createGVTTestSuite(17001, "MQTT", "policyTestPlans/topic/automation/gvt_topic_mqtt_auto.zip");
		
	try {
		
		FileOutputStream fsOut = new FileOutputStream("policyTestPlans/topic/policyTests.zip");
		ZipOutputStream zos = new ZipOutputStream(fsOut);
		for (String testCaseZipFile : testCaseZips) {
		
		
		
		FileInputStream fsIn = new FileInputStream(testCaseZipFile);
		ZipInputStream zis = new ZipInputStream(fsIn);
		ZipEntry zE;
		while((zE=zis.getNextEntry())!=null){
			zos.putNextEntry(zE);
			
			byte[] data = new byte[2048];
			int b = -1;
			while ((b = zis.read(data)) != -1)
			{
			   zos.write(data, 0, b);
			}
			zos.closeEntry();
		    zis.closeEntry();
		  }


		zis.close();
		}
		
		zos.close();
		
	} catch (Exception e) {
		e.printStackTrace();
	}
	
		/*ZipEntry zE;
		while((zE=zis.getNextEntry())!=null){
		    System.out.println(ze.getName());
		    zis.closeEntry();
		  }

		zis.close();*/
		//suite = new PolicyTestSuite("ptns1","NOSEPARATION_POLICY_VALIDATION", "testplans/policyTestsNoSeparation1.csv", TESTCASE_TYPE.VALID_NO_SEPARATION);
		//suite.createTestSuite(11974, "testplans/policyTestsNoSeparation1.zip");




	}

}
