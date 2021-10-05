package com.ibm.ism.mqbridge.test;

import java.text.SimpleDateFormat;
import java.util.Date;
import com.ibm.mq.MQException;
import com.ibm.mq.MQQueueManager;

public abstract class MessagingTest {

	public MQQueueManager targetQueueManager;
	public static int numberOfMessages=10;
	public static boolean success = true;
	public String testName;
	public static String reason="";
	public static int numberOfPublishers;
	public static int numberOfSubscribers;
	public LoggingUtilities logger;
	static MessageUtilities messageUtilities;
//	public static boolean multipleSubscribers =false;
	public static int messagePersistence;
	public static String messageBody = MessageUtilities.duplicateMessage("1234567890-=[]'#,./\\!\"£$%^&*()_+{}:@~<>?|qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM`¬", 1);
	//public static String messageBody = MessageUtilities.duplicateMessage("111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", 54000);
	//public static String messageBody = MessageUtilities.duplicateMessage("222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222222", 1200000);	
	//public static String messageBody = "test�";
	String clientID = "";
	protected boolean mqClientConn = true;
	public static int waitInSeconds = 12;
	protected static int startingQualityOfService=0;
	protected static int currentQualityOfService;
	protected static int finalQualityOfService=2;
	public static long sleepTimeout = 10000;
	static boolean jmsSub = false;
	static boolean jmsPub = false;
	protected boolean[] canStartPublishing;
//	public static boolean startPublishing = false;
	/*
	 * mapping for MQ java persistence to MQTT qos
	 					subscription qos, 
	 mqpersistence \	0		1		2
	 0					0		0		0
	 1					0		1		2
	 value = expected qos
	 */
	public static int [][]expectedQos = {{0,0,0},{0,1,2}};

	public MessagingTest(String clientID) {
		this.clientID = clientID;
		logger = new LoggingUtilities();
		messageUtilities = new MessageUtilities(logger);
	}

	public static String checkMessageString(String messageText) {
		return messageText;		
	}

	protected void checkMessages() {
		String result = messageUtilities.checkMessages();
		logger.printString(result);
	}

	public String getClientID() {
		// TODO Auto-generated method stub
		return clientID;
	}

	public void setClientID(String clientID) {
		this.clientID = clientID;
	}

	public int getQualityOfService() {
		return currentQualityOfService;
	}

	public void parseInput(String [] inputArgs) {
		/*
		 * 1. Set up ConnectionDetails 
		 * 2. Set up Target Topic / Queue
		 * 3. Set up Source Topic / Queue
		 * 
		 */
		for (int x=0;x < inputArgs.length; x++) {
			if (inputArgs[x].equalsIgnoreCase("-mqPort")) {
				x++;
				ConnectionDetails.mqPort = Integer.valueOf(inputArgs[x]);
			}  else if (inputArgs[x].equalsIgnoreCase("-mqHostAddress")) {
				x++;
				ConnectionDetails.mqHostAddress = inputArgs[x];
			} else if (inputArgs[x].equalsIgnoreCase("-queueManager")) {
				x++;
				ConnectionDetails.queueManager = inputArgs[x];
			} else if (inputArgs[x].equalsIgnoreCase("-ismServerPort")) {
				x++;
				ConnectionDetails.ismServerPort = inputArgs[x];
			} else if (inputArgs[x].equalsIgnoreCase("-ismServerHost")) {
				x++;
				ConnectionDetails.ismServerHost = inputArgs[x];
			} else if (inputArgs[x].equalsIgnoreCase("-mqChannel")) {
				x++;
				ConnectionDetails.mqChannel = inputArgs[x];
			}  else if (inputArgs[x].equalsIgnoreCase("-numberOfMessages")) {
				x++;
				numberOfMessages = Integer.valueOf(inputArgs[x]);
			} 
		}
	}
	
	protected void basicMQSetup() {				
		try {			
			if (mqClientConn ) {
				ConnectionDetails.setUpMqClientConnectionDetails();	
			} 
			targetQueueManager = ConnectionDetails.getMqQueueManager();
			logger.printString("qm is " + targetQueueManager.getName());
		} catch (MQException e) {
			e.printStackTrace();
		}
	}	
	
	protected void mqTearDown() {
		try {
			targetQueueManager.disconnect();
		} catch (MQException e) {
			e.printStackTrace();
		}
	}
	
	public static String timeNow() {
		Date now = new Date(System.currentTimeMillis());
		SimpleDateFormat sdf = new SimpleDateFormat("yyyy-MM-dd HH:mm:ss.SSS");
		return sdf.format(now);
	}

	public void setNumberOfMessages(int numberOfMessages) {
		MessagingTest.numberOfMessages = numberOfMessages;
	}

	public static int getNumberOfMessages() {
		return numberOfMessages;
	}

	public void setQualityOfService(int qualityOfService) {
		MessagingTest.currentQualityOfService = qualityOfService;
	}

	public void setNumberOfPublishers(int numberOfPublishers) {
		MessagingTest.numberOfPublishers = numberOfPublishers;
	}

	public int getNumberOfPublishers() {
		return numberOfPublishers;
	}
	

	public synchronized void clearCanPublishArray() {
		canStartPublishing = new boolean [numberOfSubscribers];
		for (int x=0; x < numberOfSubscribers; x++) {
			canStartPublishing[x] = false;
		}
	}

	
	public synchronized void setTrueCanPublishArray() {
		canStartPublishing = new boolean [numberOfSubscribers];
		for (int x=0; x < numberOfSubscribers; x++) {
			canStartPublishing[x] = true;
		}
	}
	
	public synchronized boolean canStartPublishing() {
		for (int x=0; x < numberOfSubscribers; x++) {
			if (!canStartPublishing[x]){
				return false;
			}
		}
		return true;
	}
}
//	protected static void readMessageBodyFromFile() {
//
//		messageBody = "";	
//		// read messages from file		
//		BufferedReader in = null;
//		try {
//			in = new BufferedReader(new FileReader("/home/sherring/inputfile.txt"));
//		} catch (FileNotFoundException e1) {
//			e1.printStackTrace();
//		}
//		String line = null;
//		int lineNumber=0;
//		StringBuffer inputFile = new StringBuffer();
//		try {
//			line = in.readLine();
//			while (line != null) {
//				inputFile.append(line);
//				//	messageBody = messageBody + line;
//				line = in.readLine();	
//				lineNumber++;
//				logger.printString("on line " + lineNumber);
//			}
//			messageBody = inputFile.toString();
//		}catch (IOException e1) {
//			e1.printStackTrace();
//		}
//		try {
//			in.close();
//		} catch (IOException e1) {
//			e1.printStackTrace();
//		}
//		if (logger.isVerboseLogging()) {			
//			logger.printString("message body is " + messageBody);
//		}
//		logger.printString("file read");		
//	}
