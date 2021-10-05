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

import java.io.BufferedReader;
//import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.io.RandomAccessFile;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.ByteBuffer;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Set;
import java.util.StringTokenizer;
import java.util.Timer;
import java.util.TimerTask;
import java.util.concurrent.Executors;
import java.util.concurrent.ScheduledExecutorService;
import java.util.concurrent.ScheduledFuture;
import java.util.concurrent.TimeUnit;
import java.util.concurrent.atomic.AtomicLong;

import javax.jms.BytesMessage;
import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.ExceptionListener;
import javax.jms.JMSException;
import javax.jms.Message;
import javax.jms.MessageConsumer;
import javax.jms.MessageListener;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.Topic;
import javax.net.SocketFactory;

import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaJmsObject;
import com.ibm.ima.jms.ImaProperties;
import com.ibm.ima.jms.ImaSubscription;

import java.util.concurrent.locks.Lock;
import java.util.concurrent.locks.ReentrantLock;

/*
 * This benchmark test will measure message throughput and RTT latency. 
 */
public class JMSBenchTest implements Runnable, Cloneable{
	/* TODO: May need to add order checking. Should be done in FVT and SVT tests at least. */
	
	public Object clone()
	{
		try 
		{
			return super.clone();
		} catch ( CloneNotSupportedException e)
		{
			return null;
		}
	}
	
	/* 
	 * This tool supports several workloads see syntaxhelp() method documentation for more details on command line syntax
	 */
	
	
	/**************************************** Inner classes ***************************************/
	/* Each producer or consumer builds a list a destinations (i.e. DestTuples).  This list 
	 * is constructed after parsing the command line arguments to determine the number of 
	 * connections, sessions, and queues/topics per producer or consumer thread */
	class DestTuple <E>{
		public String serverIP;				/* the destination IP address (i.e. the IMAServer address).  Round Robin assignment from IMAServer env variable */
		public String serverPort;			/* the destination port number (i.e. the IMAServer port).  Round Robin assignment from IMAPort env variable. 
		 									 * Both IMAServer and IMAPort are space separated lists and the length of the lists must be the same. */
		public int threadIdx;				/* the id instance variable is used to identify the producer or consumer application thread for this
		 								     * JVM.  This field only used for calculating the name of the queue or topic */
		public Connection conn;				/* JMS connection object used by this destination */
		public int connIdx;					/* Used to identify the connection ID for this thread */
		public Session sess;				/* JMS session object used by this destination */
		public int thrSessId;				/* The id used by this application to identify a JMS session per application thread */
		public int sessIdx;					/* Used to identify the session ID, this is unique per connection */
		public String SessionId;            /* JMS unique Thread-Connection-Session Id */
		public E dest;						/* JMS object, can either be a JMS MessageProducer or JMS MessageConsumer object */
		public Destination anonDest;        
		public String destQTname;			/* This is the name of the JMS queue or topic.  The name is a function of threadIdx, connIdx, sessIdx, and qtps. 
		 									 * These value are derived from the command line parameters (-rx and -tx). Specifically: snum, nthrd, cpt, spc, 
		 									 * and qtps. When nthrd, cpt, spc, or qtps are negative the above indices are not used to compute the queue or topic 
		 									 * name.  See buildDestinationList() for how the queue or topic name is constructed.  The queue or topic name is the 
		 									 * sum of products of these indices. */
		public BytesMessage msg;			/* The JMS message object used to send messages.  Currently, only BytesMessage type is supported. */
		public int msgIdx;					/* used to loop through the list of messages in the message range */
		public long count;					/* count per MessageConsumer and MessageProducer */
		public long sessAckcount;           /* count of ack msg per desttuple */
		public long lastACKMsg;				/* the last ACKed msg sequence number for this destination */
		public boolean isDone;				/* Used to indicate when a MessageProducer has sent all expected number of messages on this destination*/
		
		public String toString() {
			if ((dest!= null) && (dest instanceof MessageConsumer))
				return "GET=" + count + "->cons"+threadIdx+".conn"+connIdx+".sess"+sessIdx+"."+destQTname;
			if ((dest!= null) && (dest instanceof MessageProducer))
				return "PUT=" + count + "<-prod"+threadIdx+".conn"+connIdx+".sess"+sessIdx+"."+destQTname;
			
			return "";
		}
		
		public boolean isConsumer() {
			return ((dest!= null) && (dest instanceof MessageConsumer));
		}
		
		public boolean isProducer() {
			return ((dest!= null) && (dest instanceof MessageProducer));
		}
		
		public Latency latencyData;
	}
	
	/* Latency data class for RTT and internal latency measurements.  For accurate avg and std 
	 * dev measurements adjust the histogram units (-u command line parameter) such that the latency 
	 * being measured fits into the histogram.  For example, if measuring if max expected latency is 
	 * 30ms (see MAXLATENCY) use -u 1e-6 .*/
	class Latency {
		public int[] histogram;							/* the histogram data */
		public int minLatency = Integer.MAX_VALUE;		/* contained within the histogram */
		public int maxLatency = 0;						/* not necessarily contained within the histogram */
		public long maxLatencySqn = 0;					/* sample which is the max latency */
		public int bigLatency = 0;						/* number of samples larger than can fit in the histogram */
	}
	
	/* The list of destination tuples associated with a queue or topic */
	class QTMap {
		public boolean isComplete = false;
		public ArrayList<DestTuple<?>> tuples;
	}

	static class GraphiteSender implements Runnable {

		class GraphiteMetric {
			String name;
			long timestamp;
			String value;

			GraphiteMetric(String name, long timestamp, String value) {
				this.name = name;
				this.timestamp = timestamp;
				this.value = value;
			}
		}
		
		String CHARSET = "UTF-8";
		private List<GraphiteMetric> metrics;
		private int port;
		private String graphiteServer;
		private String prefix;
		private PrintStream latencyOut;
		private long lastTime = 0;
		
		public GraphiteSender(String host, int port, String prefix) {
			metrics = new ArrayList<GraphiteMetric>();
			this.graphiteServer = host;
			this.port = port;
			this.prefix = prefix;
			
			try {
				latencyOut = new PrintStream(new FileOutputStream(JMSBenchTest.csvLatencyFile));
			} catch (FileNotFoundException e) {
				System.err.println("Graphite sender failed to open latency csv file for writing");
			}
		}
		
		public void createGraphiteMetric(long timestamp, String metricName, String metricValue) {
			StringBuilder metricBuilder = new StringBuilder(50);
			metricBuilder.append(prefix);
			metricBuilder.append(".");
			metricBuilder.append(GraphiteMetricRootBase);
			metricBuilder.append(".");
			metricBuilder.append(metricName);
			metrics.add(new GraphiteMetric(metricBuilder.toString(), timestamp, metricValue));
		}
		
		public void writeAndSendMetrics() {
			if (metrics.size() > 0) {
				try {
					Socket sock = new Socket();
					sock.connect(new InetSocketAddress(graphiteServer, port), 20000);
					sock.setSoTimeout(20000);
					PrintStream ps = new PrintStream(sock.getOutputStream(), false, CHARSET);
					for (GraphiteMetric metric : metrics) {
						ps.printf("%s %s %d\n", metric.name, metric.value, Long.valueOf(metric.timestamp));
						if (ps.checkError()) {
							break;
						}
					}
					ps.close();
					sock.close();
				} catch (Exception e) {
					System.err.println("Failed to send metrics to " + graphiteServer);
				}

			}
		}

		@Override
		public void run() {
			long currTime = (long) (System.currentTimeMillis() * 1e-3);
			getStats(null, true, 0, this, currTime);
			Utils.calculateLatency(consumerThreads, JMSBenchTest.CHKTIMERTT, latencyOut, false, true, this, currTime, UNITS);
			writeAndSendMetrics();
			
			// Reset consumer latency statistics every 180 seconds
			if((currTime - lastTime) > 180) {
				for (JMSBenchTest jbt : consumerThreads) {
					int numDests = jbt.consTuples.size();
					for (int destCt = 0 ; destCt < numDests ; destCt++){
						JMSBenchTest.DestTuple<MessageConsumer> destTuple = jbt.consTuples.get(destCt);
						JMSBenchTest.Latency destLatData = destTuple.latencyData;
						Arrays.fill(destLatData.histogram, 0);
					}
				}
			}
				
			lastTime = currTime;
		}
	}
	
	/***************************************** Constants ******************************************/
	static final int    MAXLATENCY            = 30000;    /* default units are 1e-6 seconds (i.e. microseconds).  Histogram can hold up to 10 millisecond latencies.*/
	static final int    MAXGLOBALRATE         = 10000000;
	static final double MICROS_PER_SECOND     = 1000000.0;
	static final double MICROS_PER_MILLI      = 1000.0;
	static final double NANO_PER_MICRO        = 1000.0;
	static final double MILLI_PER_SECOND      = 1000.0;
	static final double SECONDS_PER_MINUTE    = 60.0;
	
	/* Actions */
	static final int BLOCKRECV     = 0;
	static final int BLOCKRECVTO   = 1;
	static final int NONBLOCKRECV  = 2;
	static final int MSGLISTENER   = 3;
	static final int SEND  		   = 4;
	static final String JBTVERSION = "0.6";

	/* Latency measurements */
	static final int CHKTIMERTT	   = 0x1;		/* measure round trip latency per destination */
	static final int PRINTRTTMM    = 0x2;		/* print the min/max destinations for avg RTT latency */
	static final int CHKTIMECOMMIT = 0x4;		/* latency of commit call */
	static final int CHKTIMERECV   = 0x8;		/* latency of blocking receive call */
	static final int CHKTIMESEND   = 0x10;      /* latency of send call */
	static final int CHKTIMECONN   = 0x20;      /* latency of creating a JMS connection */
	static final int CHKTIMESESS   = 0x40;      /* latency of creating a JMS session */

	/**************************************** Static variables ************************************/
	static int  minMsgLen        = 21;          /* minimum message length. includes 1 byte (flag), 8 bytes (timestamp), 8 bytes (seq num) + 4 bytes (payload length) */
												/* 
												 *  JMSBenchTest application framing
												 * 
												 *  0                   1                   2                   3
												 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
												 * +---------------+-----------------------------------------------+
												 * |      TSF(1)   |                 Timestamp (8)                 |
												 * +---------------+                                               |
												 * |                                                               |
												 * |               +-----------------------------------------------+
												 * |               |             App msg sqn number (8)            |
												 * +---------------+                                               |
												 * |                                                               |
												 * |               +-----------------------------------------------+
												 * |               |             Payload length (4)                :
												 * +---------------------------------------------------------------+
												 * :               |             Payload ('X'...)                  :
												 * +---------------------------------------------------------------+
												 * 
												 * -Header (TSF + Timestamp + AppMsgSqn + Payload length) = 21 bytes-------------------------
												 * TSF                - (1 byte) Flag to indicate that a timestamp is present in the message 
												 *                    - 0 = no timestamp; 1 = timestamp
												 * Timestamp          - (8 byte) timestamp using TSC clock of producer thread
												 * App msg sqn number - (8 byte) message sequence number
												 * Payload length     - (4 byte) payload length 
												 * 
												 * -Payload----------------------------------------------------------------------------------
												 * Payload            - (-s <size> - Header) bytes of char 'X'
												 */
	static int  minMsg           = 128; 		/* min range of message size (bytes) -s <min>-<max> */
	static int  maxMsg           = 128; 		/* max range of message size (bytes) -s <min>-<max> */
	static int  globalMsgRate    = 1000;   		/* global message rate (msgs/sec) for the process (i.e. sum of all producers) */
	static int  testDuration     = 0;			/* duration in seconds of the test */
	static int  perDestMsgCount  = 0;			/* perDestMsgCount is the number of messages sent to every destination of 
	 											 * every producer. message count (-c) and test duration (-d) are mutually exclusive,
	 											 * test duration takes precedence over msg count */
	static int  snapInterval     = 0;			/* interval between stats, default = 0 (i.e. no interval)*/
	static String msgType        = null;        /* message type: topic, queue, shared subscription */
	static int  beginStats       = 0;           /* how long to wait before resetting clock to restart statistics */
	static String csvLogFile     = null;        /* filename of the csv log file. */
	static String csvLatencyFile = null;        /* filename of the csv latency log file. */
	static int  sampleRate       = 1000;		/* sampling rate latency measurements per second (i.e. number of times to read the clock per second) */
	static boolean chkTime       = false;  	    /* If enabled will measure latency of client calls .  Enabled with -T on the command line */

	static int latencyMask       = 0;			/* Mask of latency measurements (see CHK* constants in the constants section above) */
	static double UNITS		     = 1e6;		    /* Units for RTT latency statistics */
	static int delaySend		 = 0;			/* Add a delay on producer threads to allow all producer threads to complete creating connections */
	static int waitReady         = 0;           /* Delay sending message on producer threads to allow all producer threads to complete creating connections */
	static int roundRobinSendSub = 0;           /* Do round robin in sending subscription. */
	static int resetTStampXMsgs  = 0;           /* Number of messages to receive before starting statistics on receiver (this is for thruput) */
	static int num_partition     = 0;           /* Number of partitions of topic names. */
	static int appSimLoop		 = 0;			/* A busy loop used to simulate an application processing a message */
	static int [] connBalance;                  /* Array of connection balance */
	static int total_conn        = 0;           /* Total number of connections */
	static int num_connbal       = 0;           /* # of tokens in CONNBAL */
	static int idx_connbal       = 0;            
	public static int rateControllerCPU         = -1;     /* CPU on which rate controller thread runs, default is -1 (i.e. no affinity) */
	static volatile boolean rateControllerReady = false;  /* Main thread needs to wait for RateController thread */
	static volatile boolean stopRateController  = false;  /* stop the RateController thread */
	static RateController globalRateController;           /* This rate controller object is used by the rate controller thread */
	static Object rateControllerMutex;
	private static final Lock connLock = new ReentrantLock(); /* Lock for creating connections */ 
	static int rateControl                      = 0; 	  /* determines what form of rate control to use by producer threads in JMSBenchTest
												 	       *   rateControl == -1 ; no rate control
												 	       *   rateControl == 0 (default) ; individual producer threads control their own rate
												 	       *   rateControl == 1 ; a single rate control thread used to control the rate of all producer threads 
												 	       *                      but use the master rate controller instead */
	static int ackIntrvl = 100;						  /* when consumer ACK mode is client application ACK mode (i.e. Session.CLIENT_ACKNOWLEDGE) units are msgs/ACK 
	 												   * ACK interval is scoped to a session.  Default is 100 msgs/ACK */
	static boolean isPubSub = false;			      /* Determines whether to create queues or topics.  The default is to create queues.  This can be enabled with 
	 											       * the '-mt t' command line option.  JMSBenchTest does not support mixed queue and pubsub consumers and producers 
	 											       * in the same JVM.*/
	static int [] simulHist;                          /* Delay histogram for the simulate application loop */
	static int asSleepNano      = 0;                  /* Sleep time in simulate application (nanosec) */
	static int asSleepMilli     = 0;                  /* Sleep time in simulate application (millisec) */
	static boolean isSharedSub = false;          
	static int RateChanged_RCThread    = 0;
	static String password         = null;
	static String username         = null;
	static int useSecureConnection = 0;
	static String ConnBal = null;
	static boolean Anonymousdest = false;
    static ConnectionFactory fact;		              /* Global connection factory */
	
	static ArrayList<JMSBenchTest>  consumerThreads;  /* consumer threads */
	static ArrayList<JMSBenchTest>  producerThreads;  /* producer threads */
	
	static Timer consMsgChk = null;                   /* Periodic task that wakes up and checks aggregate send 
	 											       * receive counts per queue when -c option is used. Checks if all sent messages 
	 											       * have been received. */
    static TimerTask consMsgChkTask;
    static HashMap<String,QTMap> qtMap;               /* map of producers and consumers to queues/topics per JVM */
    
    static ArrayList<String> serverIPs;				  /* For multi-NIC server testing */
    static ArrayList<String> serverPorts;             /* For multi-port server testing */
    
	static String consumerStatusFile = "ConsumersReady.txt";
	static String producerStatusFile = "ProducersReady.txt";
	
	static int clientMsgCache = 1000;
	
	static String disableACK = "0";				      /* DisableACK env variable is stored in this static variable. Possible values are 1 (true) or 0 (false) */
	
	static String pipeCommands = "0" ;                /* determine using stdin or pipe */
	static int imaServerSets = 1;					  /* the number of segments to split the IMAServer & IMAPort lists into */
	static int indexOfServerSet = 0;                  /* Used to index into IMAServer & IMAPort lists with env IMAServerSets */
	static int currConnectionsInSetCtr = 0;           /* Global connection counter used for aggregrate number of connections. */

	
	static String GraphiteIP = "";
	static int GraphitePort = 2003;
	static int MetricInterval = 5;
	static String GraphiteMetricRoot = "testX";
	final static String GraphiteMetricRootBase = "jmsbench";
	static GraphiteSender graphiteSender;
	static ScheduledExecutorService scheduler;
	static ScheduledFuture<?> timer;
	
	/**************************************** JNI functions ***************************************/
	static {
		System.loadLibrary("perfutil");
	}
	public static native void setAffinity(int cpuId);
	public static native double readTSC();
	
	/**************************************** Instance variables **********************************/
	int  id;							/* id of consumer or producer thread. ids are reused between producers and consumers */
	int  threadIndex;					/* this is used to construct queue/topic names. It indicates the thread that created the queue or topic */
	String qtName="t";					/* the base name of the queue or topic created by the producer or consumer
	 									 * in buildDestinations the queue or topic name will be appended to with ".X" 
	 									 * where X ranges from 0 to qtps */
	String pName="";                    /* Partition name */
	String tName="";                    /* Topic name */
	int qtps=1;							/* number of queues or topics created per session */
    int spc=1;							/* number of sessions per connection */
    int cpt=1;							/* number of connections per thread */
    int nThrds=1;						/* number of cloned JMSBenchTest instances/threads */
	int action;							/* see Actions constants above */
	long perThreadMsgCount;				/* the expected number of messages to be sent per producer thread. When using pubsub this is also the expected 
	 									 * number of messages to be received on each destination */
	int  cpu=-1;						/* The CPU that this thread will be bound to. default is -1 (i.e. no affinity) */
	int  qnStart=0;                     /* Number to start naming queues at. */
	volatile boolean recvdone=false;	/* Consumers check this variable, main thread sets it when using -d 
	 									 * Consumers set this variable when using -c and pub/sub 
	 									 * TaskTimer thread sets this variable when using -c and queueing */
    volatile boolean senddone=false;	/* Producers check this variable, main thread sets it when using -d
		 								 * Producers set this variable when using -c and pub/sub */
    boolean clientReady=false;          /* Consumers/Producers flag to indicate ready to receive/send. */
    double tokenIn=0;					/* Putting tokens in the bucket allows the producer to go faster */
    double tokenOut=0;					/* Taking tokens out of the bucket slows the producer down */

    ArrayList<DestTuple<MessageProducer> > prodTuples = null;     /* list of destinations per producer */
    ArrayList<DestTuple<MessageConsumer> > consTuples = null;     /* list of destinations per consumer */
    
    ConnectionFactory privateFactory;		        /* connection factory per JMSBenchTest */
    
    int numSessions = 0;  	                        /* number of sessions per producer or consumer thread */

    boolean durable = false;                        /* used for durable subscriptions. */
    /* Send configuration params */
    int deliveryMode = DeliveryMode.PERSISTENT;		/* default delivery mode is persistent */
    int sendCommit = 1;								/* msgs sent per commit, a value of 0 means session is not transacted. 
     												 * the commit rate is scoped to a session */
    RateController perPubRateController;			/* each producer creates its own rate controller */
    
    /* Recv configuration params */
    int ackMode = Session.DUPS_OK_ACKNOWLEDGE;		/* default ACK mode (ACK rate configured) */
    int recvCommit = 1;								/* msgs received per commit, a value of 0 means session is not transacted.  
     												 * the commit rate is scoped to a session */
    
	/* Statistics */
    long prevtime=0;
    
	/* Send Statistics */
	long  sendcount;            					/* per producer thread send count */
    //AtomicLong  asendcount = new AtomicLong(0);	/* per producer thread send count for multi-threaded access */
	long  lastsendcount;
	long  firstsendcount;
	int   sendrate;
	ArrayList<Integer> snapSendRate = null;
	
	/* Recv Statistics */
	//long  recvcount;	         					/* per consumer thread receive count */
	AtomicLong  arecvcount = null;                  /* per consumer thread receive count for multi-threaded access */
	AtomicLong  aAckcount = null;                   /* per consumer thread acknowledgment count */

	long  lastrecvcount;
	long  firstrecvcount;
	int   recvrate;
	ArrayList<Integer> snapRecvRate = null;
	
	/* Latency measurements */
	Latency commitHist;						/* latency histogram of commit call (producer or consumer) CHKTIMECOMMIT */
	Latency recvHist; 						/* latency histogram of receive call on a consumer thread CHKTIMERECV */
	Latency sendHist; 						/* latency histogram of send call on a producer thread CHKTIMESEND */
	Latency connHist; 						/* latency histogram of createConnection call on a producer or consumer thread CHKTIMECONN */
	Latency sessHist; 						/* latency histogram of createSession call on a producer or consumer thread CHKTIMESESS */
	
	/**************************************** Class methods ***************************************/
	/* Builds the list of destinations per producer or consumer application thread (i.e. instance(clone) of JMSBenchTest)
	 * and returns the number of unique queue or topic names.  The return value is used to check when queues are drained 
	 * when using message queueing (only used with -c command line parameter. It is important when using the -c option  
	 * that there is no "under-subscription" by consumers).  The JMS objects are not created in this method, they are 
	 * created in the buildDestinations method. 
	 */
	public int buildDestinationList() throws JMSException {
		int threadSessionId=0;
		int queuePfx = 0;
		int startVal = this.qnStart;
		int totalqt = 0;
		int totalclnt = Math.abs(spc)* Math.abs(cpt) * Math.abs(nThrds);
		int connectionsInSet = 0;
		int indexIncrementor = 0;
		
		// Take the partition name and topic name
		if (num_partition > 0) {
			StringTokenizer qt = new StringTokenizer(qtName, "/");
			pName = qt.nextToken();
			tName = qt.nextToken();
		}
	    
		if (roundRobinSendSub == 1) { // Use -rrs
			if (num_partition == 0){
				totalqt = Math.abs(qtps);
				if (totalqt <= totalclnt){
					qtps = 1;
				}	
				if (qtName.contains("#") == true || qtName.contains("+") == true){
					qtps = 1;
				}
			}
			else {
				totalqt = Math.abs(qtps) * num_partition;
				if (totalqt <= totalclnt){
					qtps = 1;
				}	
				if (qtName.contains("#") == true || qtName.contains("+") == true){
					if (num_partition <= totalclnt)
						qtps =1;
				}
			}
		}
		else {
			if (num_partition == 0){
				if (qtName.contains("#") == true || qtName.contains("+") == true){
					qtps = 1;
				}
			}
			else {
				totalqt = Math.abs(qtps) * Math.abs(spc)* Math.abs(cpt)* Math.abs(nThrds);
			}
		}
		
		/* If the user specified IMAServerSets then need to handle this situation by determining how
		 * to split the Server IP Addresses & Ports according to the total number of connections,  
		 * number of sets (IMAServerSets) and number of IP Addresses (IMAServer).  */
		if (imaServerSets > 1) { 
			int totalNumConnections = Math.abs(cpt) * Math.abs(nThrds);
			
			/* TODO - the total number of connections must be a multiple of IMAServerSets */
			if (roundRobinSendSub == 0) {
				connectionsInSet = totalNumConnections  / imaServerSets;
				indexIncrementor = serverIPs.size() / imaServerSets;
			} else {
				imaServerSets = 1;
    	    	System.err.println("ignoring environment IMAServerSets since -rrs option was specified.");
			}
		}
		
		/* Create connections */
		for(int i=0 ; i < Math.abs(cpt) ; i++){
			int index = 0;
			String serverIP = "";
			String serverPort = "";

			if (imaServerSets > 1) {
				index = indexOfServerSet + (currConnectionsInSetCtr++ % indexIncrementor);

				if (currConnectionsInSetCtr == connectionsInSet) {
					indexOfServerSet += indexIncrementor;
					currConnectionsInSetCtr = 0;
				}
			} else {
				index = (i + id) % serverIPs.size();
			}
			
			serverIP = serverIPs.get(index);
			serverPort = serverPorts.get(index);
			
			/* Create session(s) per connection */
			for(int j=0 ; j<Math.abs(spc) ; j++, threadSessionId++){
				boolean isProd = action==SEND; /* is a producer */
				
				if ( roundRobinSendSub == 1 && totalqt > totalclnt ){
					qtps = totalqt/totalclnt + (((threadIndex*Math.abs(cpt)+i)*Math.abs(spc)+j)<(totalqt%totalclnt)?1:0);
				}
				/* Create queues/topics per session */
				for(int k=0 ; k<Math.abs(qtps); k++){
					String name;
					if (roundRobinSendSub == 0) {
						/* Update the prefix part of the queue name */
						//queuePfx = startVal + (nThrds<0?1:threadIndex * (cpt<0?1:cpt) * (spc<0?1:spc) * (qtps<0?1:qtps)) + ((cpt<0?1:i) * (spc<0?1:spc) * (qtps<0?1:qtps)) + ((spc<0?1:j) * (qtps<0?1:qtps)) + (qtps<0?1:k);
						queuePfx = startVal + 
						           (nThrds<0 ? 0:(threadIndex * (cpt<0?1:cpt) * (spc<0?1:spc) * (qtps<0?1:qtps))) +  
						           (cpt<0 ? 0:(i * (spc<0?1:spc) * (qtps<0?1:qtps))) +
						           (spc<0 ? 0:(j * (qtps<0?1:qtps))) + 
						           (qtps<0 ? 0:k);                        
						if ( num_partition == 0 ) {
							if (qtName.contains("#") == true || qtName.contains("+") == true){
								name = String.format("%s",qtName);
							}
							else
								name = String.format("%s.%08d", qtName, queuePfx);
						}
						else {
							int pIndex = (queuePfx/(totalqt < num_partition?1:totalqt/num_partition) < num_partition? queuePfx/(totalqt < num_partition?1:totalqt/num_partition): num_partition-1);
							qtName = "/"+pName + pIndex +"/" + tName;
							if (qtName.contains("#") == true || qtName.contains("+") == true){
								name = String.format("%s",qtName);
							}
							else {
								queuePfx = queuePfx - pIndex * (totalqt/num_partition);
								name = String.format("%s.%08d", qtName, queuePfx);
							}
						}
					}
					else {
						queuePfx = startVal + (threadIndex * Math.abs(cpt) + i) * Math.abs(spc) + j;
						if (num_partition == 0 ){
							if (qtName.contains("#") == true || qtName.contains("+") == true){
								name = String.format("%s",qtName);
							}
							else{
								queuePfx = ( queuePfx + k* totalclnt) % totalqt ; 
								name = String.format("%s.%08d", qtName, queuePfx);
							}
						}
						else {
							int pIndex = ((queuePfx + k*totalclnt) % totalqt )% num_partition;
							qtName = "/"+pName + pIndex +"/" + tName;
							if (qtName.contains("#") == true || qtName.contains("+") == true){
								name = String.format("%s",qtName);
							}
							else {
								queuePfx = ((queuePfx + k*totalclnt) % totalqt) / num_partition;
								name = String.format("%s.%08d", qtName, queuePfx);
							}
						}
					}
					
					DestTuple<?> tuple;
					if(isProd)
						tuple = new DestTuple<MessageProducer>();
					else
						tuple = new DestTuple<MessageConsumer>();
					
					tuple.threadIdx = id;
					tuple.connIdx = i;
					tuple.serverIP = serverIP;
					tuple.serverPort = serverPort;
					tuple.thrSessId = threadSessionId;
					tuple.sessIdx = j;
					tuple.SessionId = "cons"+id+".conn"+i+".sess"+threadSessionId;
					tuple.destQTname = name;
					tuple.sessAckcount = (long)0;
					tuple.isDone = false;

					if(isProd)
						prodTuples.add((DestTuple<MessageProducer>) tuple);		/* ignore javac warning */
					else {
					    if((latencyMask & CHKTIMERTT) == CHKTIMERTT) {
					    	tuple.latencyData = new Latency();
							tuple.latencyData.histogram = new int[MAXLATENCY];
					    }
						consTuples.add((DestTuple<MessageConsumer>) tuple);		/* ignore javac warning */
					}
					
					if(qtMap.containsKey(name)){
						QTMap map = qtMap.get(name);
						map.tuples.add(tuple);
						qtMap.put(name,map);
					} else {
						QTMap map = new QTMap();
						map.tuples = new ArrayList<DestTuple<?>>();
						map.tuples.add(tuple);
						qtMap.put(name, map);
					}
				} /* queue/topic loop */
			} /* session loop */
		} /* connection loop */

		numSessions=Math.abs(cpt*spc);
		
		return qtMap.keySet().size();
	}
	
	/* Creates the JMS objects from the list of destinations configured for each instance of JMSBenchTest.  
	 * This list is constructed in buildDestinationList.  This method is called on each producer/consumer thread */
	public void buildDestinations() throws JMSException {
		int i=-1, j=-1;	/* assumes for loops (i.e. connIdx(i) and sessIdx(j)) in buildDestinationList start at 0 */
		Connection conn = null;
		Connection ghost = null;
		Session sess = null;
		MessageProducer prod = null;
		BytesMessage msg = null;
		
		privateFactory = cloneConnectionFactory(fact);
		int clntMsgCache = ((ImaProperties)fact).getInt("ClientMessageCache",0);
		
		for (DestTuple<?> tuple : action==SEND ? prodTuples : consTuples) {
			
			if(i!=tuple.connIdx){
				j=-1;           /* Reset j since a new connection. */
				i=tuple.connIdx;
				if (tuple.serverIP != null)
	    	    	((ImaProperties)privateFactory).put("Server", tuple.serverIP);
	    	    if (tuple.serverPort != null)
	    	    	((ImaProperties)privateFactory).put("Port", tuple.serverPort);
	    	    
	    	    connLock.lock();
	    	    try{
	    	    	if (num_connbal > 0){
		    	    	if (connBalance[idx_connbal] > 0){
		    	    		connBalance[idx_connbal]--;
		    	    	}
		    	    	idx_connbal = (idx_connbal + 1 ) % num_connbal;
	    	    	}
	    	    	
	    	        if((latencyMask & CHKTIMECONN) == CHKTIMECONN){
			    	    double t0,t1,delta;
			    	   	t0 = readTSC();
			    	    	
			    	   	if ((username != null) && (password != null))
			    	   		conn = privateFactory.createConnection(username, password);
			    	   	else
				   	    	conn = privateFactory.createConnection();
	
			   	    	t1 = readTSC();
			   	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > connHist.maxLatency) {
							connHist.maxLatency = (int) delta;
							connHist.maxLatencySqn = (tuple.threadIdx * tuple.connIdx) + tuple.connIdx; 
			    	   	}
			    	   	if (delta < connHist.minLatency)
			    	   		connHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							connHist.histogram[(int) delta]++;
						else
							connHist.bigLatency++;
   		    	    } else {
			   	    	if ((username != null && username.length() > 0) && (password != null && password.length() > 0))
			   	    		conn = privateFactory.createConnection(username, password);
			   	    	else
			   	    		conn = privateFactory.createConnection();
			   	    }
	    	        if (num_connbal > 0){
		    	        int ghostconn = 0;
		    	    	while (connBalance[idx_connbal] == 0 && ghostconn < num_connbal) {
			    	        ghost = privateFactory.createConnection();
			    	        idx_connbal = (idx_connbal + 1 ) % num_connbal;
			    	        ghostconn ++;
			    	    }
	    	        }
	    	    }
	    	    finally{
	    	    	connLock.unlock();
	    	    }
			
	    	    /* Create the Client ID for this connection. */
	    	    String host = "localhost";
	    	    try{
	    	    	host = InetAddress.getLocalHost().getHostName();
	    	    } catch (Exception e) {
	    	    	System.err.println("Unable to retrieve the local host name, using \"localhost\"");
	    	    	e.printStackTrace();
				}
			
				if (action == SEND)
					conn.setClientID(host + "_p" + tuple.threadIdx + "_conn" + tuple.connIdx);
				else {
					if (!isSharedSub)
						conn.setClientID(host + "_c" + tuple.threadIdx + "_conn" + tuple.connIdx);
				}
			} /* if(i!=tuple.connIdx) */
			
			conn.setExceptionListener(new ExceptionListener() {
				public void onException(JMSException e) {
					e.printStackTrace();
				}
			});
			
			tuple.conn = conn;
			
			if(j!=tuple.sessIdx){
				j=tuple.sessIdx;
				switch (action) {
				case SEND:
					if((latencyMask & CHKTIMESESS) == CHKTIMESESS){
		    	    	double t0,t1,delta;
		    	    	t0 = readTSC();
		    	    	sess = conn.createSession(sendCommit != 0, ackMode);
		    	    	if (Anonymousdest){
                             prod = sess.createProducer(null);
                             prod.setDeliveryMode(deliveryMode);
                        }
		    	    	t1 = readTSC();
		    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > sessHist.maxLatency) {
							sessHist.maxLatency = (int) delta;
							sessHist.maxLatencySqn = (tuple.threadIdx * tuple.connIdx * tuple.sessIdx) + (tuple.connIdx * tuple.sessIdx) + tuple.sessIdx; 
		    	    	}
		    	    	if (delta < sessHist.minLatency)
		    	    		sessHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							sessHist.histogram[(int) delta]++;
						else
							sessHist.bigLatency++;
		    	    } else{
		    	    	sess = conn.createSession(sendCommit != 0, ackMode);
		    	    	if (Anonymousdest){
                            prod = sess.createProducer(null);
                            prod.setDeliveryMode(deliveryMode);
                       }
		    	    }
					
					msg = sess.createBytesMessage();
					break;
				case BLOCKRECV:
				case BLOCKRECVTO:
				case NONBLOCKRECV:
				case MSGLISTENER:
					if((latencyMask & CHKTIMESESS) == CHKTIMESESS){
		    	    	double t0,t1,delta;
		    	    	t0 = readTSC();
		    	    	sess = conn.createSession(recvCommit != 0, ackMode);
		    	    	t1 = readTSC();
		    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > sessHist.maxLatency) {
							sessHist.maxLatency = (int) delta;
							sessHist.maxLatencySqn = (tuple.threadIdx * tuple.connIdx * tuple.sessIdx) + (tuple.connIdx * tuple.sessIdx) + tuple.sessIdx; 
		    	    	}
		    	    	if (delta < sessHist.minLatency)
		    	    		sessHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							sessHist.histogram[(int) delta]++;
						else
							sessHist.bigLatency++;
		    	    } else
		    	    	sess = conn.createSession(recvCommit != 0, ackMode);
					break;
				}
			}
			
			tuple.sess = sess;
			if (Anonymousdest)
				((DestTuple<MessageProducer>)tuple).dest = prod;
			
			Destination d = null;
			if (isPubSub)
				d = sess.createTopic(tuple.destQTname);
			else
				d = sess.createQueue(tuple.destQTname);
			
			((ImaProperties)d).put("ClientMessageCache", clntMsgCache);
			
			if(action==SEND){ /* This destination is configured as a MessageProducer */
				if (Anonymousdest) {
					((DestTuple<MessageProducer>)tuple).anonDest = d;
				}
				else{
					prod = sess.createProducer(d);
					prod.setDeliveryMode(deliveryMode);
					((DestTuple<MessageProducer>)tuple).dest = prod;
				}
				tuple.msg = msg;
			} else {          /* This destination is configured as a MessageConsumer */
				MessageConsumer cons;
				if (isSharedSub){
					String SubName = tuple.destQTname + "_ss";
					if (durable)
						cons = ((ImaSubscription)sess).createSharedDurableConsumer((Topic)d, SubName);
					else
						cons = ((ImaSubscription)sess).createSharedConsumer((Topic)d, SubName);
				}
				else if (durable) {
					cons = sess.createDurableSubscriber((Topic)d, conn.getClientID() + "_" + ((Topic)d).getTopicName());
				} else {
					cons = sess.createConsumer(d);
				}
				
				((DestTuple<MessageConsumer>)tuple).dest = cons;
			}
			
			/* For all destinations, EXCEPT for those associated with a message listener, start the connection.  For destinations 
			 * associated with a message listener the connection must be started after setting the message listeners */
			if(action!=MSGLISTENER)
				conn.start();     
		}
	}
	
	/* Setup the per thread histograms based on the value of the -T command line parameter.  The per destination 
	 * histograms are setup in buildDestinationList() */
	public void setupPerThreadHistograms(){
		if((latencyMask & CHKTIMECOMMIT) == CHKTIMECOMMIT){
			commitHist = new Latency();
			commitHist.histogram = new int[MAXLATENCY];
		}
		if((latencyMask & CHKTIMESEND) == CHKTIMESEND){
			sendHist = new Latency();
			sendHist.histogram = new int[MAXLATENCY];
		}
		if((latencyMask & CHKTIMERECV) == CHKTIMERECV){
			recvHist  = new Latency();
			recvHist.histogram = new int[MAXLATENCY];
		}
		if((latencyMask & CHKTIMECONN) == CHKTIMECONN){
			connHist  = new Latency();
			connHist.histogram = new int[MAXLATENCY];
		}
		if((latencyMask & CHKTIMESESS) == CHKTIMESESS){
			sessHist = new Latency();
			sessHist.histogram = new int[MAXLATENCY];
		}
		if (appSimLoop != 0) {
			simulHist = new int [MAXLATENCY+1];
		}
	}
	
    /*
     * Thread entry point for each instance of the JMSBenchTest class
     */
    public void run() {
    	try {
    		/* Setup histograms for latency measurements */
    		setupPerThreadHistograms();
    		
    		/* Create JMS objects (connections, sessions, queues/topics, and MessageProducers/MessageConsumers */
    		buildDestinations();
    		
    		/* IBM Messaging Appliance JMS client creates a child thread that inherits CPU affinity from the parent (i.e. the application thread).  This is why 
        	 * we set the CPU affinity of the parent after the connection has been created */
        	if(cpu>=0)
        		setAffinity(cpu);
    		
    		/* doBlockingReceive, doBlockingTOReceive, doNonBlockingReceive, doListen are VERY similar
    		 * be sure if you update one of them to update all of them */
    		switch (action) {
    		case BLOCKRECV:
    		case BLOCKRECVTO:
    		case NONBLOCKRECV:
    			doReceive();			
    			break;
    		case MSGLISTENER:
    			doListen();
    			break;
    		case SEND:
    			doSend();
    			break;
    		}
    	} catch (Exception e) {
    		e.printStackTrace(System.err);
    		recvdone = true;   /* may not need this */
    		senddone = true;   /* may not need this */
    		System.exit(1);
    	}
    }
    
    /*
     * Send messages 
     */
    public void doSend() throws JMSException {
    	synchronized (rateControllerMutex) {
    		if (rateControl == 1 && globalRateController == null) {
    			globalRateController = new RateController(producerThreads); // Use a single rate controller thread to control rate
    			rateControllerMutex.notify();
    			new Thread(globalRateController, "RateController").start();
    		} else if (rateControl == 0) {
    			perPubRateController = new RateController(producerThreads);
    			perPubRateController.initRateController(globalMsgRate
    					/ producerThreads.size());
    		}
    	} 
    	int [] commitcount = new int[numSessions];   /* commit rate is per session */
    	int destcount = 0;
    	int numDests = prodTuples.size();
    	int sampleRatePerThread = 0;
    	int messageRatePerThread = 0;

    	/* Create list of messages to send based on binary progression from minMsg to maxMsg */
    	ArrayList<byte[]> msglist = new ArrayList<byte[]>();
    	int size = minMsg;
    	ByteBuffer buff;
    	do {
    		buff = ByteBuffer.allocate(size);

    		/* Clear the header bytes (minMsgLen). */
    		for (int i=0 ; i<minMsgLen ; i++) {
    			buff.put((byte)0);
    		}

    		/* Set the payload length field */
    		buff.putInt(1+8+8, size);

    		/* Fill the payload with 'X' */ 
    		for (int i=minMsgLen; i<size ; i++) {
    			buff.put(i,(byte)'X');  /* 0x58585858... */
    		}

    		msglist.add(buff.array());

    		size<<=1;	

    	} while(size<=maxMsg);

    	int numMsgs = msglist.size();

    	/* Set up the variables if running with latency. */
    	Utils sampler = new Utils();
    	if ((latencyMask & CHKTIMERTT) == CHKTIMERTT)	{
    		sampleRatePerThread = (int)((double)sampleRate / (double)producerThreads.size());

    		if (rateControl == -1)
    			messageRatePerThread = (int)((double)MAXGLOBALRATE /(double)producerThreads.size());
    		else
    			messageRatePerThread = (int)((double)globalMsgRate/(double)producerThreads.size());

    		sampler.initMessageSampler(sampleRatePerThread, messageRatePerThread);
    	}

    	if(delaySend > 0){
    		try {
    			Thread.sleep(delaySend * 1000);  // delaySend (-ds) is in units of seconds
    		} catch (InterruptedException e) {
    			System.err.println("Took an interrupt exception on producer " + id + " during delay time before sending messages.");
    			e.printStackTrace();
    		}  
    	} else if (waitReady > 0) {
    		try {
    			clientReady = true;
    			while (waitReady > 0) {
    				Thread.sleep(0, 1000);
    			}
    		} catch (InterruptedException e) {
    			System.err.println("Took an interrupt exception on producer " + id + " while waiting for all producers to be ready.");
    			e.printStackTrace();
    		}
    	} else if (testDuration >= 0)
    		clientReady = true;

		long startTime = System.currentTimeMillis();
		
		if(perDestMsgCount!=0 && testDuration==0){  /* -c <messages> */
			for (sendcount=0; sendcount<perThreadMsgCount; sendcount++) {  /* increment the per producer thread counter */
			//for (asendcount.set(0); asendcount.get() < perThreadMsgCount; asendcount.getAndIncrement()) {
				DestTuple<MessageProducer> destTuple = prodTuples.get(destcount);
				int threadSessionId = destTuple.thrSessId;
				BytesMessage bmsg = destTuple.msg;
				MessageProducer prod = destTuple.dest;
				Destination anonDest = destTuple.anonDest;
				
				if (++destTuple.msgIdx >= numMsgs)
				  destTuple.msgIdx = 0;
				  
				byte[] msgbytes = msglist.get(destTuple.msgIdx);
				
				if (sendcount == 0) {
					long tstart=System.currentTimeMillis();
					System.err.println(tstart);
				}
				
				/* If measuring RTT latency then need to put readTSC() in the message. */ 
				if ((latencyMask & CHKTIMERTT) == CHKTIMERTT) {
					try
					{
						if (sampler.selectForSampling())
						{
							msgbytes[0] = 1;
							sampler.writeTimestamp(msgbytes, Double.doubleToLongBits(readTSC()));
						}
						else
							msgbytes[0] = 0;
					} catch (Exception e) {
						System.err.println("failed to set sampling flag and/or timestamp in the outgoing message");
						System.err.println(e.getMessage());
						System.exit(88);
					}
				}
				
				if (perThreadMsgCount < 1000){
					try
					{
						msgbytes[0] = 1;
						sampler.writeTimestamp(msgbytes, Double.doubleToLongBits(readTSC()));
					} catch (Exception e) {
						System.err.println("failed to set sampling flag and/or timestamp in the outgoing message");
						System.err.println(e.getMessage());
						System.exit(88);
					}
				}
				
				bmsg.clearBody(); 
				bmsg.writeBytes(msgbytes);
				if((latencyMask & CHKTIMESEND) == CHKTIMESEND){
	    	    	double t0,t1,delta;
	    	    	t0 = readTSC();
	    	    	if (Anonymousdest)
	    	    		prod.send(anonDest, bmsg);						/* asynchronous call */
	    	    	else
	    	    		prod.send(bmsg);
	    	    	t1 = readTSC();
	    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
	    	    	if (delta > sendHist.maxLatency) {
						sendHist.maxLatency = (int) delta;
						sendHist.maxLatencySqn = sendcount;
	    	    	}
	    	    	if (delta < sendHist.minLatency)
	    	    		sendHist.minLatency = (int) delta;
					if (delta < MAXLATENCY)
						sendHist.histogram[(int) delta]++;
					else
						sendHist.bigLatency++;
	    	    } else{
	    	    	if (Anonymousdest)
	    	    		prod.send(anonDest, bmsg);						/* asynchronous call */
	    	    	else
	    	    		prod.send(bmsg);
	    	    }
				
				destTuple.count++;						/* increment the per destination counter */
				
				if (sendCommit > 0 && ++commitcount[threadSessionId] >= sendCommit) {  /* currently do not support mixed sessions (txn and non-txn in a single producer thread) */
				    Session sess = destTuple.sess;
				    if((latencyMask & CHKTIMECOMMIT) == CHKTIMECOMMIT){
		    	    	double t0,t1,delta;
		    	    	t0 = readTSC();
		    	    	sess.commit();
		    	    	t1 = readTSC();
		    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > commitHist.maxLatency) {
		    	    		commitHist.maxLatency = (int) delta;
		    	    		commitHist.maxLatencySqn = sendcount;
		    	    	}
		    	    	if (delta < commitHist.minLatency)
		    	    		commitHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							commitHist.histogram[(int) delta]++;
						else
							commitHist.bigLatency++;
		    	    } else
		    	    	sess.commit();
					commitcount[threadSessionId] = 0;
				}
				
				if(++destcount >= numDests){
					destcount=0;
				}
			}
			
			long endTime = System.currentTimeMillis();
			
			if((endTime - startTime) < 1000 ) {
				System.err.println("Prod" + id + ": the test did not run long enough (" + (endTime - startTime)*1e-3 + " seconds) to provide meaningful throughput results.");
			} else {
				sendrate = (int) (sendcount/((endTime-startTime)*1e-3));
				//sendrate = (int) (asendcount.get()/((endTime-startTime)*1e-3));
				System.err.println("Prod" + id + ": completed at " + new Date().toString() + " avg send rate: " +
						sendrate);
			}
			senddone=true;
		} else {	/* -d <seconds> */ 
			while(!senddone){ /* main thread has the stop watch */
				DestTuple<MessageProducer> destTuple = prodTuples.get(destcount);
				int threadSessionId = destTuple.thrSessId;
				BytesMessage bmsg = destTuple.msg;
				MessageProducer prod = destTuple.dest;
				Destination anonDest = destTuple.anonDest;
				
				if (++destTuple.msgIdx >= numMsgs)
					  destTuple.msgIdx = 0;
					  
				byte[] msgbytes = msglist.get(destTuple.msgIdx);
				
				/* If measuring RTT latency then need to put readTSC() in 
				 * the message. */ 
				if ((latencyMask & CHKTIMERTT) == CHKTIMERTT) {
					try
					{
						if (sampler.selectForSampling())
						{
							msgbytes[0] = 1;
							sampler.writeTimestamp(msgbytes, Double.doubleToLongBits(readTSC()));
						}
						else
							msgbytes[0] = 0;
					} catch (Exception e) {
						System.err.println("failed to set sampling flag and/or timestamp in the outgoing message");
						System.err.println(e.getMessage());
						System.exit(88);
					}
				}
								
			    bmsg.clearBody();
				bmsg.writeBytes(msgbytes);
				if((latencyMask & CHKTIMESEND) == CHKTIMESEND){
	    	    	double t0,t1,delta;
	    	    	t0 = readTSC();
	    	    	if (Anonymousdest)
	    	    		prod.send(anonDest, bmsg);						/* asynchronous call */
	    	    	else
	    	    		prod.send(bmsg);
	    	    	t1 = readTSC();
	    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
	    	    	if (delta > sendHist.maxLatency) {
						sendHist.maxLatency = (int) delta;
						sendHist.maxLatencySqn = sendcount;
	    	    	}
	    	    	if (delta < sendHist.minLatency)
	    	    		sendHist.minLatency = (int) delta;
					if (delta < MAXLATENCY)
						sendHist.histogram[(int) delta]++;
					else
						sendHist.bigLatency++;
	    	    } else{
	    	    	if (Anonymousdest)
	    	    		prod.send(anonDest, bmsg);						/* asynchronous call */
	    	    	else
	    	    		prod.send(bmsg);
	    	    }
	    	    	
				if (sendCommit > 0 && ++commitcount[threadSessionId] >= sendCommit) {
					Session sess = destTuple.sess;
					if((latencyMask & CHKTIMECOMMIT) == CHKTIMECOMMIT){
		    	    	double t0,t1,delta;
		    	    	t0 = readTSC();
		    	    	sess.commit();
		    	    	t1 = readTSC();
		    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > commitHist.maxLatency) {
		    	    		commitHist.maxLatency = (int) delta;
		    	    		commitHist.maxLatencySqn = sendcount;
		    	    	}
		    	    	if (delta < commitHist.minLatency)
		    	    		commitHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							commitHist.histogram[(int) delta]++;
						else
							commitHist.bigLatency++;
		    	    } else
		    	    	sess.commit();

					commitcount[threadSessionId] = 0;
				}
				
				destTuple.count++;		/* per destination send counter */
				sendcount++;			/* per producer thread send counter */
				//asendcount.getAndIncrement();			/* per producer thread send counter */
				
				if(rateControl >= 0) {  // use rate control
					if(rateControl == 0){ // Individual producer rate control
						perPubRateController.controlRate();
					} else {
						/* The rate control thread will increment the tokenIn counter in batches for 
						 * each publishing thread.  This publishing thread will increment the tokenOut counter 
						 * by one per transmitted message. Batch size is a function of the goal message rate 
						 * and the scheduling rate of the rate control thread.  When tockenIn is less than or 
						 * equal to tokenOut the token bucket is "full" and the publishing threads have to yield 
						 * the CPU (i.e. slow down) until the bucket has been "emptied" by the publishing
						 * thread.  The  bucket is empty when tokenIn is incremented such that it is 
						 * greater than tokenOut. */
						while(tokenIn <= tokenOut && !senddone) {
							Thread.yield();
						}

						/* Fill the token bucket by one token per transmitted message */
						tokenOut++;
					}
				}
				
				if(++destcount >= numDests){
					destcount=0;
				}
			}
		}
		
		/* Iterate through all session and commit pending messages 
		 * Iterate through all connections and close them */
		for (DestTuple<MessageProducer> prodTuple : prodTuples) {
			if(sendCommit != 0)
				prodTuple.sess.commit();
		}
		
		/* must commit all session associated with a connection before closing the connection */
		for (DestTuple<MessageProducer> prodTuple : prodTuples) {
			prodTuple.conn.close();
		}
    }
    
    /*
     * Receive based on action: blocking receive, receive with timeout, or non-blocking receive
     * Synchronous transacted receive messages if recvCommit is greater than 0
     * Synchronous non-transacted if recvCommit is 0
     */
    public void doReceive() throws JMSException {
    	byte [] b = new byte[maxMsg + 1];   /* +1 used below when verifying that message length (readBytes) is equal to payload length in the msg */
    	ByteBuffer bb = ByteBuffer.wrap(b);
    	int [] commitcount = new int[numSessions];
    	int [] ackcount = new int[numSessions];
    	int destcount = 0;
    	int numDests = consTuples.size();
    	int latency;
    	long startTime=0;
    	double recvTimestamp;
    	double sendTimestamp = 0;
    	boolean firstmsg = true;
    	boolean _disableACK = false;
		if (disableACK != null && disableACK.length() > 0)
			_disableACK = Integer.parseInt(disableACK) != 0;
    	
   		Utils receiver = new Utils();

    	if (perDestMsgCount!=0 && testDuration==0){ /* -c <messages> */
    		for (arecvcount.set(0); arecvcount.get() < perThreadMsgCount && !recvdone; arecvcount.getAndIncrement()) {
				DestTuple<MessageConsumer> destTuple = consTuples.get(destcount);
				int threadSessionId = destTuple.thrSessId;
				MessageConsumer cons = destTuple.dest;
				Message msg = null;
				String sessId = "cons" + destTuple.threadIdx + ".conn" + destTuple.connIdx + ".sess" + threadSessionId;
				switch (action) {
				case BLOCKRECV:
					if((latencyMask & CHKTIMERECV) == CHKTIMERECV){
						double t0,t1,delta;
						t0 = readTSC();
						msg = cons.receive();  			//blocking recv call (most common workload - can change if this is no longer the case)
						if (msg != null) {
							t1 = readTSC();
							delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
							if (delta > recvHist.maxLatency) {
								recvHist.maxLatency = (int) delta;
								recvHist.maxLatencySqn = arecvcount.get();
							}
							if (delta < recvHist.minLatency)
								recvHist.minLatency = (int) delta;
							if (delta < MAXLATENCY)
								recvHist.histogram[(int) delta]++;
							else
								recvHist.bigLatency++;
						} else {
							System.err.println("Did not receive message: " + destTuple.count + " after blocking receive call on dest=" + destTuple.toString());
							throw new JMSException("No message received from MessageConsumer blocking receive call");
						}
		    	    } else {
		    	    	msg = cons.receive();  			//blocking recv call (most common workload - can change if this is no longer the case)
		    	    	if (msg == null) {
		    	    		System.err.println("Did not receive message: " + destTuple.count + " after blocking receive call on dest=" + destTuple.toString());
		    	    		throw new JMSException("No message received from MessageConsumer blocking receive call");
		    	    	}
		    	    }
					break;
				case NONBLOCKRECV:
					msg = cons.receiveNoWait();  	//non-blocking recv call
					if (msg == null) {
	    				destTuple.count--;			/* did not receive a message */
	    				arecvcount.getAndDecrement();
	    				continue;
	    			}
					break;
				case BLOCKRECVTO:
					msg = cons.receive(1000); 		//blocking recv call with 1 second timeout
					if (msg == null) {
						destTuple.count--;
						arecvcount.getAndDecrement();
	    				System.err.println("Timeout of more than 1 second in receive message: " + destTuple.count + " on dest=" + destTuple.toString());
	    				continue;
	    			}
					break;
				}

    			/* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
    			BytesMessage bmsg = (BytesMessage)msg;
    			int bytesRead = bmsg.readBytes(b);
    			int payloadLength =  bb.getInt(1+8+8); /* see JMSBenchTest framing diagram above (get payload length) */
    			if ( payloadLength == bytesRead) {
    			//if (bmsg.getBodyLength() == msgLen && b[0] == (byte)i) {  check order later
    				if(firstmsg){
    					startTime=System.currentTimeMillis();
    					firstmsg=false;
    				}
    			} else {
    				throw new JMSException("Message received is not the correct message (not expected msg length): " + destTuple.count + 
    						               " on dest=" + destTuple.toString() + ". Actual=" + bytesRead + " Expected=" + payloadLength);
    			}
    			
    			//double time0, time1, del;
    			if (appSimLoop > 0) {
	    			//time0 = readTSC();
	    			/* Simulate some amount of application processing for "real-world" testing */
					for (int i = 0; i < appSimLoop; i++) {
						try {
							MessageDigest algorithm = MessageDigest.getInstance("MD5");
							algorithm.reset();
							algorithm.update(b);
							byte messageDigest[] = algorithm.digest();
						} catch (NoSuchAlgorithmException e) {
							e.printStackTrace();
						}
					}
					//time1 = readTSC();
	    	    	//del = (time1 - time0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
	    	    	//if ( del < MAXLATENCY)
	    	    	//	simulHist[(int) del]++;
	    	    	//else
	    	    	//	simulHist[MAXLATENCY]++;
    			}
    			else if (appSimLoop < 0) {
    				//time0 = readTSC();
    				try {
    					Thread.sleep(asSleepMilli, asSleepNano);
    				} catch (InterruptedException e) {
    					System.err.println("Took an interrupt exception in application simulate during delay time.");
    					e.printStackTrace();
    				}  
    				//time1 = readTSC();
	    	    	//del = (time1 - time0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
	    	    	//if ( del < MAXLATENCY)
	    	    	//	simulHist[(int) del]++;
	    	    	//else
	    	    	//	simulHist[MAXLATENCY]++;
    			}
    	    	
    			if (recvCommit > 0 && ++commitcount[threadSessionId] >= recvCommit) {
    				Session sess = destTuple.sess;
    				if((latencyMask & CHKTIMECOMMIT) == CHKTIMECOMMIT){
		    	    	double t0,t1,delta;
		    	    	t0 = readTSC();
		    	    	sess.commit();
		    	    	t1 = readTSC();
		    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > commitHist.maxLatency) {
		    	    		commitHist.maxLatency = (int) delta;
		    	    		commitHist.maxLatencySqn = sendcount;
		    	    	}
		    	    	if (delta < commitHist.minLatency)
		    	    		commitHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							commitHist.histogram[(int) delta]++;
						else
							commitHist.bigLatency++;
		    	    } else
		    	    	sess.commit();
    				
    				commitcount[threadSessionId] = 0;
    			} else {
    				if (ackMode == Session.CLIENT_ACKNOWLEDGE && !_disableACK) {
    				  if (++ackcount[threadSessionId] >= ackIntrvl) {  
    					  //String msgid = msg.getJMSMessageID();
    					  //Long l = msg.getLongProperty(ImaMessage.ACK_SQN_PROP_NAME);
    					  Long l = msg.getLongProperty("JMS_IBM_ACK_SQN");
    					  msg.acknowledge();
    					  aAckcount.getAndIncrement();
    					  destTuple.sessAckcount++;
    					  //TODO store last ACK msg id from msg object and put in lastACKMsg
    					  ackcount[threadSessionId]=0;
    					  if(l != null){
    						  destTuple.lastACKMsg = l.longValue();
    					  } else 
    						  destTuple.lastACKMsg = 0;
    				  }
    				}
    			}
    			
    			if(++destcount >= numDests){
					destcount=0;
				}		
    		}
    		
    		long endTime = System.currentTimeMillis();
    		if((endTime - startTime) < 1000 ) {
    			System.err.println("Cons" + id + ": the test did not run long enough (" + (endTime - startTime)*1e-3 + " seconds) to provide meaningful throughput results.");
    		} else {
    			recvrate = (int) ((arecvcount.get())/((endTime-startTime)*1e-3));
    			System.err.println("Cons" + id + ": completed at " + new Date().toString() + " avg recv rate: " +
    					recvrate);
    		}
    		recvdone=true;
    		
    	} else { /* -d <seconds> */
    		clientReady = true;
    		
    		while(!recvdone){ /* main thread has the stop watch */
    			DestTuple<MessageConsumer> destTuple = consTuples.get(destcount);
    			int threadSessionId = destTuple.thrSessId;
				MessageConsumer cons = destTuple.dest;
				Message msg = null;
				String sessId = "cons" + destTuple.threadIdx + ".conn" + destTuple.connIdx + ".sess" + threadSessionId;
		    	final Latency rttLatencyPerDest = destTuple.latencyData;
		    	
				switch (action) {
				case BLOCKRECV:
					if((latencyMask & CHKTIMERECV) == CHKTIMERECV){
						double t0,t1,delta;
						t0 = readTSC();
						msg = cons.receive();  			//blocking recv call (most common workload - can change if this is no longer the case)
						if (msg != null) {
							t1 = readTSC();
							delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
							if (delta > recvHist.maxLatency) {
								recvHist.maxLatency = (int) delta;
								recvHist.maxLatencySqn = arecvcount.get();
							}
							if (delta < recvHist.minLatency)
								recvHist.minLatency = (int) delta;
							if (delta < MAXLATENCY)
								recvHist.histogram[(int) delta]++;
							else
								recvHist.bigLatency++;
						} else {
							System.err.println("Did not receive message: " + destTuple.count + " after blocking receive call on dest=" + destTuple.toString());
							throw new JMSException("No message received from MessageConsumer blocking receive call");
						}
		    	    } else {
		    	    	msg = cons.receive();  			//blocking recv call (most common workload - can change if this is no longer the case)
		    	    	if (msg == null) {
		    	    		System.err.println("Did not receive message: " + destTuple.count + " after blocking receive call on dest=" + destTuple.toString());
		    	    		throw new JMSException("No message received from MessageConsumer blocking receive call");
		    	    	}
		    	    }
					break;
				case NONBLOCKRECV:
					msg = cons.receiveNoWait();  	//non-blocking recv call
					if (msg == null) {
						destTuple.count--;			/* did not receive a message */
	    				continue;
	    			}
					break;
				case BLOCKRECVTO:
					msg = cons.receive(1000); 		//blocking recv call with 1 second timeout
					if (msg == null) {
						destTuple.count--;
	    				System.err.println("Timeout of more than 1 second in receive message: " + destTuple.count + " on dest=" + destTuple.toString());
	    				continue;
	    			}
					break;
				}

				/* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
    			BytesMessage bmsg = (BytesMessage)msg;
    			int bytesRead = bmsg.readBytes(b);
    			int payloadLength =  bb.getInt(1+8+8); /* see JMSBenchTest framing diagram above (get payload length) */
    			if ( payloadLength == bytesRead) {
    			//if (bmsg.getBodyLength() == msgLen && b[0] == (byte)i) {  check order later
    				if(firstmsg){
    					startTime=System.currentTimeMillis();
    					firstmsg=false;
    				}
    			} else {
    				throw new JMSException("Message received is not the correct message (not expected msg length): " + destTuple.count + 
    						               " on dest=" + destTuple.toString() + ". Actual=" + bytesRead + " Expected=" + payloadLength);
    			}
				
    			/* Read the message and see if the 1st bytes is 1, which indicates a sampled
    			 * message.  Read the current time to be used in determining the latency of
    			 * the message and ultimately put in the histogram. */
				if ((latencyMask & CHKTIMERTT) == CHKTIMERTT) {
					if (b[0] == 1) {
						recvTimestamp = readTSC();
						
						try {
							sendTimestamp = Double.longBitsToDouble(receiver.readTimestamp(b));
						}
						catch (Exception e) {
							System.err.println("Unable to obtain the timestamp from the message.");
							System.exit(8);
						}
						
						/* Calculate the round trip latency in microseconds.  Latency is
						 * found by taking the current timestamp and subtracting the 
						 * timestamp stored in the message, then multiply by UNITS to  
						 * convert nanoseconds to UNITS. */
						latency = (int)((recvTimestamp - sendTimestamp) * UNITS); 
						
						if (latency > rttLatencyPerDest.maxLatency) {
							rttLatencyPerDest.maxLatency = latency;
							rttLatencyPerDest.maxLatencySqn = destTuple.count; 
						}
						if (latency < rttLatencyPerDest.minLatency)
							rttLatencyPerDest.minLatency = latency;
						
						if (latency < MAXLATENCY)
							rttLatencyPerDest.histogram[latency]++;
						else
							rttLatencyPerDest.bigLatency++;
					}
				}
				
				//double time0, time1, del;
				/* Simulate some amount of application processing for "real-world" testing */
				if (appSimLoop > 0) {
	    			//time0 = readTSC();
	    			for (int i = 0; i < appSimLoop; i++) {
						try {
							MessageDigest algorithm = MessageDigest.getInstance("MD5");
							algorithm.reset();
							algorithm.update(b);
							byte messageDigest[] = algorithm.digest();
							messageDigest = null;
							algorithm = null;
						} catch (NoSuchAlgorithmException e) {
							e.printStackTrace();
						}
					}
					//time1 = readTSC();
	    	    	//del = (time1 - time0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
	    	    	//if ( del < MAXLATENCY)
	    	    	//	simulHist[(int) del]++;
	    	    	//else
	    	    	//	simulHist[MAXLATENCY]++;
    			}
				else if (appSimLoop < 0) {
					//time0 = readTSC();
    				try {
    					Thread.sleep(asSleepMilli, asSleepNano);
    				} catch (InterruptedException e) {
    					System.err.println("Took an interrupt exception in application simulate during delay time.");
    					e.printStackTrace();
    				}  
    				//time1 = readTSC();
	    	    	//del = (time1 - time0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
	    	    	//if ( del < MAXLATENCY)
	    	    	//	simulHist[(int) del]++;
	    	    	//else
	    	    	//	simulHist[MAXLATENCY]++;
    			}
				
    			if (recvCommit > 0 && ++commitcount[threadSessionId] >= recvCommit) {
    				Session sess = destTuple.sess;
    				if((latencyMask & CHKTIMECOMMIT) == CHKTIMECOMMIT){
		    	    	double t0,t1,delta;
		    	    	t0 = readTSC();
		    	    	sess.commit();
		    	    	t1 = readTSC();
		    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
		    	    	if (delta > commitHist.maxLatency) {
		    	    		commitHist.maxLatency = (int) delta;
		    	    		commitHist.maxLatencySqn = sendcount;
		    	    	}
		    	    	if (delta < commitHist.minLatency)
		    	    		commitHist.minLatency = (int) delta;
						if (delta < MAXLATENCY)
							commitHist.histogram[(int) delta]++;
						else
							commitHist.bigLatency++;
		    	    } else
		    	    	sess.commit();
    				
    				commitcount[threadSessionId] = 0;
    			} else {
    				if (ackMode == Session.CLIENT_ACKNOWLEDGE && !_disableACK) {
    					if (++ackcount[threadSessionId] >= ackIntrvl) {  
    						//String msgid = msg.getJMSMessageID();
    						//Long l = msg.getLongProperty(ImaMessage.ACK_SQN_PROP_NAME);
    						Long l = msg.getLongProperty("JMS_IBM_ACK_SQN");
    						msg.acknowledge();
    						aAckcount.getAndIncrement();
    						destTuple.sessAckcount++;
    						//TODO store last ACK msg id from msg object and put in lastACKMsg
    						ackcount[threadSessionId]=0;
    						if(l != null){
    							destTuple.lastACKMsg = l.longValue();
    						} else 
    							destTuple.lastACKMsg = 0;	      					
    					}
      				}
      			}
    			
    		    destTuple.count++;   /* per destination receive counter */
    			arecvcount.getAndIncrement();         /* per consumer thread receive counter */
    			
    			if(++destcount >= numDests){
					destcount=0;
				}
    		} /* while(!recvdone) */
    	} /* if (perDestMsgCount!=0 && testDuration==0) */

    	/* Iterate through all session and commit pending messages 
		 * Iterate through all connections and close them */
		for (DestTuple<MessageConsumer> consTuple : consTuples) {
			if(recvCommit != 0)
				consTuple.sess.commit();
		}
		
		/* must commit all sessions associated with a connection before closing the connection */
		for (DestTuple<MessageConsumer> consTuple : consTuples) {
			consTuple.conn.close();
		}
		
		/* Obtain the latency. */
    }
        
    /*
     * doListen()
     * 
     * Message Listener - Asynchronous non-transacted receive messages
     */
    public void doListen() throws JMSException {
    	int numDests = consTuples.size();
    	//final LinkedList<Message> syncUnACKedMsgs = new LinkedList<Message>();
    	final int [] ackcount    = new int[numSessions];
    	final int [] commitcount = new int[numSessions];
    	clientReady= true;
    	
    	boolean bDisableACK = false;
		if (disableACK != null && disableACK.length() > 0)
			bDisableACK = Integer.parseInt(disableACK) != 0;
		final boolean disable_ack = bDisableACK;
    	
    	for(int destcount=0; destcount<numDests; destcount++){
    		final DestTuple<MessageConsumer> destTuple = consTuples.get(destcount);
    		final int threadSessId = destTuple.thrSessId;
    		MessageConsumer cons = destTuple.dest;
    		final Latency rttLatencyPerDest = destTuple.latencyData;
    		final String sessId = "cons" + destTuple.threadIdx + ".conn" + destTuple.connIdx + ".sess" + threadSessId;    				
    		
    		cons.setMessageListener(new MessageListener() {
    			boolean firstmsg = true;
    			boolean _disableACK = disable_ack;
    			long startTime = 0;
    			DestTuple<MessageConsumer> _destTuple = destTuple;
    			int _threadSessionId = threadSessId; 
    			Latency destLatencyData = rttLatencyPerDest;
    			Utils receiver = new Utils();

    			public void onMessage(Message msg) {
    				/* Validate the message, this will throw ClassCastException if it is not a BytesMessage */
    				BytesMessage bmsg = (BytesMessage)msg;
    				int latency = 0;
   				
    				try {
    					long bodyLen = bmsg.getBodyLength();
    					byte marked = bmsg.readByte();
    					double sendTimestamp = bmsg.readDouble();
    					bmsg.readLong();
    	    			int payloadLength =  bmsg.readInt();//bb.getInt(1+8+8); /* see JMSBenchTest framing diagram above (get payload length) */
    	    			double sendTimeProperty = 0;
    	    			if (msg.propertyExists("$TS")) {
    	    				sendTimeProperty = msg.getDoubleProperty("$TS");  
    	    			}
    	    			
						if ((latencyMask & CHKTIMERTT) == CHKTIMERTT) {
							if (sendTimeProperty != 0) {
								double recvTimestamp = System.currentTimeMillis() * 1e-3; // convert millis to seconds
								latency = (int)((recvTimestamp - sendTimeProperty) * UNITS);
							} /*else if (marked == 1) {
								double recvTimestamp = readTSC();

								 Calculate the round trip latency in microseconds.  Latency is
								 * found by taking current timestamp and subtracting the 
								 * timestamp in the message, then multiply by UNITS to convert
								 * nanoseconds to UNITS. 
								latency = (int)((recvTimestamp - sendTimestamp) * UNITS); 
							} */

							if (latency >= 0) {
								if (latency > destLatencyData.maxLatency) {
									destLatencyData.maxLatency = latency;
									destLatencyData.maxLatencySqn = _destTuple.count; 
								}
								
								if (latency < rttLatencyPerDest.minLatency)
									rttLatencyPerDest.minLatency = latency;
	
								if (latency < MAXLATENCY)
									destLatencyData.histogram[latency]++;
								else
									destLatencyData.bigLatency++;
							}
						}

						if(firstmsg){
							startTime=System.currentTimeMillis();
							firstmsg=false;
						}
					
						//double time0, time1, del;
						/* Simulate some amount of application processing for "real-world" testing */
						if (appSimLoop > 0) {
							byte [] b = new byte[(int)bodyLen + 1];
			    			//time0 = readTSC();
			    			for (int i = 0; i < appSimLoop; i++) {
			    				try {
									MessageDigest algorithm = MessageDigest.getInstance("MD5");
									algorithm.reset();
									algorithm.update(b);
									byte messageDigest[] = algorithm.digest();
								} catch (NoSuchAlgorithmException e) {
									e.printStackTrace();
								}
							}
							//time1 = readTSC();
			    	    	//del = (time1 - time0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
			    	    	//if ( del < MAXLATENCY)
			    	    	//	simulHist[(int) del]++;
			    	    	//else
			    	    	//	simulHist[MAXLATENCY]++;
		    			}
						else if (appSimLoop < 0) {
							//time0 = readTSC();
		    				try {
		    					Thread.sleep(asSleepMilli, asSleepNano);
		    				} catch (InterruptedException e) {
		    					System.err.println("Took an interrupt exception in application simulate during delay time.");
		    					e.printStackTrace();
		    				}  
		    				//time1 = readTSC();
			    	    	//del = (time1 - time0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
			    	    	//if ( del < MAXLATENCY)
			    	    	//	simulHist[(int) del]++;
			    	    	//else
			    	    	//	simulHist[MAXLATENCY]++;
		    			}

						if (recvCommit > 0 && ++commitcount[_threadSessionId] >= recvCommit) {
							Session sess = destTuple.sess;
							if((latencyMask & CHKTIMECOMMIT) == CHKTIMECOMMIT){
				    	    	double t0,t1,delta;
				    	    	t0 = readTSC();
				    	    	sess.commit();
				    	    	t1 = readTSC();
				    	    	delta = (t1 - t0) * UNITS;	// internal latency measurements are assumed to be in units of UNITS
				    	    	if (delta > commitHist.maxLatency) {
				    	    		commitHist.maxLatency = (int) delta;
				    	    		commitHist.maxLatencySqn = sendcount;
				    	    	}
				    	    	if (delta < commitHist.minLatency)
				    	    		commitHist.minLatency = (int) delta;
								if (delta < MAXLATENCY)
									commitHist.histogram[(int) delta]++;
								else
									commitHist.bigLatency++;
				    	    } else
				    	    	sess.commit();
							commitcount[_threadSessionId] = 0;
						} else {
							if(ackMode == Session.CLIENT_ACKNOWLEDGE && !_disableACK){
								if (++ackcount[_threadSessionId] >= ackIntrvl) {
									/*synchronized (syncUnACKedMsgs) {
        							syncUnACKedMsgs.addLast(msg);
        							if(syncUnACKedMsgs.size() == 1)
        								syncUnACKedMsgs.notify();
								}*/
									try {
										//String msgid = msg.getJMSMessageID();
										//Long l = msg.getLongProperty(ImaMessage.ACK_SQN_PROP_NAME);
										Long l = msg.getLongProperty("JMS_IBM_ACK_SQN");
										msg.acknowledge();
										aAckcount.getAndIncrement();
										destTuple.sessAckcount++;
										if(l != null){
											destTuple.lastACKMsg = l.longValue();
										} else 
											destTuple.lastACKMsg = 0;
										
									} catch (JMSException e) {
										System.err.println("Cons" + id + ": failed to ACK message " + _destTuple.count + " on dest " + _destTuple.toString());
										e.printStackTrace();
										System.exit(1);
									}
									ackcount[_threadSessionId]=0;
								}
							}
						}
						
						_destTuple.count++;	
						arecvcount.getAndIncrement();

    				}
    				catch (JMSException jmse) {
    					if(!recvdone)
    						throw new RuntimeException(jmse);
    					else
    						return;
    				}
    				
    				if (isPubSub && (perDestMsgCount > 0) && (arecvcount.get() == perThreadMsgCount)) {
    					long endTime = System.currentTimeMillis();
    					if((endTime - startTime) < 1000 ) {
    						System.err.println("Cons" + id + ": the test did not run long enough (" + (endTime - startTime)*1e-3 + " seconds) to provide meaningful throughput results.");
    					} else {
    						recvrate = (int) (arecvcount.get()/((endTime-startTime)*1e-3));
    						System.err.println("Cons" + id + ": completed at " + new Date().toString() + " avg recv rate: " +
    								recvrate);
    					}
    					recvdone=true;
    				}

    				if (isPubSub && (perDestMsgCount > 0) && (_destTuple.count > perDestMsgCount))  /* take this out eventually when we have verified code is working */
    					System.err.println("Cons" + id + " oops: destCount=" + _destTuple.count + " perDestMsgCount=" + perDestMsgCount + " consTuple size: " +consTuples.size());
    			}
    		});
    	}

    	/* Consumers do not get any messages if the connections are started before the message listeners are set*/
    	for (DestTuple<MessageConsumer> consTuple : consTuples) {
			consTuple.conn.start();
		}
        
   /* 	if(ackMode==Session.CLIENT_ACKNOWLEDGE){
    		while(!recvdone){
    			Message m = null;
    			synchronized (syncUnACKedMsgs) {
    				try {    				
    					while(syncUnACKedMsgs.isEmpty()){
    						if(recvdone)
    							break;
    						syncUnACKedMsgs.wait(1000);
    					}
    					m = syncUnACKedMsgs.removeFirst();
    				} catch (Exception e) {
    					 Check to see if done running and if so just continue. 
    					if (!recvdone)
    						e.printStackTrace();
    					continue;
    				}
    			}
    			
    			if(m!=null){
    				m.acknowledge();
    				m=null;
    			}
    		}
    	} else {*/
    		while(!recvdone){
    			try {
					Thread.sleep(1000);
				} catch (InterruptedException e) {
					System.err.println("Cons" + id + " was interrupted in doListen() while waiting for test to complete");
					e.printStackTrace();
				}
    		}
    	//}
    	
    	/* Iterate through all connections and close them */
		for (DestTuple<MessageConsumer> consTuple : consTuples) {
			consTuple.conn.close();
		}
    }
    
    /**************************************** static methods **************************************/  
    static final ConnectionFactory cloneConnectionFactory(ConnectionFactory factory) throws JMSException{
    	ConnectionFactory result = ImaJmsFactory.createConnectionFactory();
    	ImaProperties props = ImaJmsObject.getProperties(factory);
    	Object obj = result;
    	ImaProperties p1 = ((ImaProperties)obj);
    	Set<String> pset = props.propertySet();
    	for (String name : pset) {
			p1.put(name, props.get(name));
		}
    	return result;
    }
    
    /*
     * printRemaining
     * Print by queue or topic, the list of producers and consumers that have pending messages to send or 
     * receive on the queue
     */
    public static void printRemaining(String destname){
    	if(destname == null || destname.equalsIgnoreCase("*")) {
    		for (String qt : qtMap.keySet()) {
    			QTMap map = qtMap.get(qt);
    			if(!map.isComplete){
    				StringBuffer buff = new StringBuffer();
    				long put=0;
    				long get=0;
    				for (DestTuple<?> tuple : map.tuples) {
    					if(tuple.isConsumer())
							get+=tuple.count;
						else if(tuple.isProducer())
							put+=tuple.count;
						else
							continue;
						
						buff.append("\t");
						buff.append(tuple.toString());
						buff.append("\n");	
    				}

    				System.err.println(qt + " PUT=" + put + " GET=" + get);
    				System.err.println(buff);
    			}
    		}
    	} else {
    		QTMap map = qtMap.get(destname);
    		if (map == null){
    			System.err.println("The queue or topic with name " + destname + " is not a valid destination in this JVM");
    			return;
    		}
			if(!map.isComplete){
				StringBuffer buff = new StringBuffer();
				long put=0;
				long get=0;
				for (DestTuple<?> tuple : map.tuples) {
					if(!tuple.isDone){
						if(tuple.isConsumer())
							get+=tuple.count;
						else if(tuple.isProducer())
							put+=tuple.count;
						else
							continue;
						
						buff.append("\t");
						buff.append(tuple.toString());
						buff.append("\n");		
					}
				}

				System.err.println(destname + " PUT=" + put + " GET=" + get);
				System.err.println(buff);
			}
    	}
    }
    
    /*
     * printQTNames
     * Print the list of topic and queue names
     */
    public static void printQTNames(){
    	StringBuffer buff = new StringBuffer();
    	int i = 0;
    	for (String qt : qtMap.keySet()) {
    		if (i++ == 10) {
    			buff.append('\n');
    			i = 1;
    		} 
    		int numProd=0, numCons=0;
    		QTMap qtm = qtMap.get(qt);
    		for (DestTuple<?> tuple : qtm.tuples) {
			  if(tuple.isConsumer())
				  numCons++;
			  if(tuple.isProducer())
				  numProd++;
			}
    		buff.append(qt + "(" + numProd + "P:" + numCons + "C) ");
    	}
    	System.err.println(buff);
    }
    
    /*
     * resetStats
     * Take the current counters and store them for rate calculations.
     */
    public static void resetStats(){
    	/* Consumers */
    	for (JMSBenchTest jbt : consumerThreads) {
    		jbt.firstrecvcount = jbt.arecvcount.get();
    	}
    	
    	/* Producers */
    	for (JMSBenchTest jbt : producerThreads) {
   			//jbt.firstsendcount = jbt.asendcount.get();
    		jbt.firstsendcount = jbt.sendcount;
   		}
    }
    
    /*
     * getStats
     * Get the statistics for the consumer and producer.
     * 
     * getStats( ) is used in several different ways:
     * 
     *                                                                       Input Parms
     *     Description                                                       out       csv  
     *     -------------------------------------------------------------   ----------  ---  
     *     1. Provide statistics to the console                            System.err   0
     *     2. Provide accumulative statistics to a csv file                csvStream    1     
     *     3. Provide accumulative and snapshot statistics to a csv file   null         1 
     */
    public static void getStats(PrintStream out, boolean csv, long startTime, JMSBenchTest.GraphiteSender graphiteSender, long metricTime){
    	int i=1;
    	int aggrxrate=0;	/* aggregate rx rate (all consumers) */
    	int aggtxrate=0;	/* aggregate tx rate (all producers) */
    	
    	if(!csv)
    		out.println();
    	
    	/* Consumers */
    	for (JMSBenchTest jbt : consumerThreads) {
    		long currtime = System.currentTimeMillis();
    		int rate;
    		long rcvcnt = jbt.arecvcount.get();
    		long ackcnt = jbt.aAckcount.get();
    		int delay25, delay50, delay75;
    		
    		if(startTime == 0){
    			rate = (int) ((rcvcnt - jbt.lastrecvcount) / ((currtime - jbt.prevtime)* 1e-3));
    			
    			jbt.prevtime=currtime;
    			jbt.lastrecvcount = rcvcnt;
    		} else {
    			rate = (int) ((rcvcnt - jbt.firstrecvcount) / ((currtime - startTime)* 1e-3));
    		}
    		
    		if(csv){
    			if(out == null){
    				jbt.snapRecvRate.add(rate);
    			} else {
    				if(snapInterval == 0){
    					out.println("Cons" + i++ + "," + Math.abs(jbt.cpt) + "," + Math.abs(jbt.spc) + "," +  Math.abs(jbt.qtps) + "," + jbt.qtName + "," + 
    					            minMsg + "-" + maxMsg + "," + consumerThreads.get(0).recvCommit + "," + rate + ",0");
    				} else {
    					out.print("Cons" + i++ + "," + Math.abs(jbt.cpt) + "," + Math.abs(jbt.spc) + "," +  Math.abs(jbt.qtps) + "," + jbt.qtName + "," +
    							  minMsg + "-" + maxMsg + "," + consumerThreads.get(0).recvCommit + "," + rate);
    					
    					for(int y=0 ; y < jbt.snapRecvRate.size() ; y++ ){
    						if(y==0)
    							out.print("," + jbt.snapRecvRate.get(y));
    						else
    							out.print("_" + jbt.snapRecvRate.get(y));
    					}
    					
    					out.println();
    				}
    			}
    		} else {
    			if (appSimLoop != 0) {
    				delay25 = Utils.getLatencyPercentile(jbt.simulHist, 0.25);
    	    		delay50 = Utils.getLatencyPercentile(jbt.simulHist, 0.50);
    	    		delay75 = Utils.getLatencyPercentile(jbt.simulHist, 0.75);
    				out.println("Cons" + i++ + ": msg rate: " + rate + " msgs/sec " + "   last: " + rcvcnt + "  lastAck: " + ackcnt + " Delay: " + delay25 + " " + delay50 + " " + delay75);
    			}
    				else
    				out.println("Cons" + i++ + ": msg rate: " + rate + " msgs/sec " + "   last: " + rcvcnt + "  lastAck: " + ackcnt);
    		}
    		
    		aggrxrate+=rate;
    	}
    	
    	if(!csv){
    		out.println("Total RX" + ": msg rate: " + aggrxrate + " msgs/sec");
		}
    	
    	i=1;    /* reset i for producers */
    	
    	/* Producers */
    	for (JMSBenchTest jbt : producerThreads) {
    		long currtime = System.currentTimeMillis();
    		int rate;
    		//long sndcnt = jbt.asendcount.get();
    		long sndcnt = jbt.sendcount;
    		
    		if (startTime == 0){
    			rate = (int) ((sndcnt - jbt.lastsendcount) / ((currtime - jbt.prevtime)* 1e-3));

    			jbt.prevtime=currtime;
    			jbt.lastsendcount = sndcnt;
    		} else {
    			rate = (int) ((sndcnt - jbt.firstsendcount) / ((currtime - startTime)* 1e-3));
    		}
    		
    		if (csv){
    			if (out == null){
    				jbt.snapSendRate.add(rate);
    			} else {
    				if (snapInterval == 0){
    					out.println("Prod" + i++ + "," + Math.abs(jbt.cpt) + "," + Math.abs(jbt.spc) + "," +  Math.abs(jbt.qtps) + "," + jbt.qtName + "," +
    							    minMsg + "-" + maxMsg + "," + producerThreads.get(0).sendCommit + "," + rate + ",0");
    				} else {
    					out.print("Prod" + i++ + "," + Math.abs(jbt.cpt) + "," + Math.abs(jbt.spc) + "," +  Math.abs(jbt.qtps) + "," + jbt.qtName + "," + 
    							  minMsg + "-" + maxMsg + "," + producerThreads.get(0).sendCommit + "," + rate);

    					for (int y=0 ; y < jbt.snapSendRate.size() ; y++ ){
    						if (y==0)
    							out.print("," + jbt.snapSendRate.get(y));
    						else
    							out.print("_" + jbt.snapSendRate.get(y));
    					}
    					
    					out.println();
    				}
    			}
    		} else {
    			out.println("Prod" + i++ + ": msg rate: " + rate + " msgs/sec " + "   last: " + sndcnt);
    		}
    		
    		aggtxrate+=rate;
    	}

    	if(!csv){
    		out.println("Total TX" + ": msg rate: " + aggtxrate + " msgs/sec");
    	} else {
    		/* for now assume send commit rate and receive commit rate are the same for all producers and consumers */
    		if(out != null){
    			if (producerThreads.size() == 0)
        			out.println(minMsg + "-" + maxMsg + ",0," + aggtxrate + "," + aggrxrate);
    			else
    				out.println(minMsg + "-" + maxMsg + "," + producerThreads.get(0).sendCommit + "," + aggtxrate + "," + aggrxrate);
    		}
		}
    	
    	if (graphiteSender != null) {
    		graphiteSender.createGraphiteMetric(metricTime, "TXRATE", String.valueOf(aggtxrate));
    		graphiteSender.createGraphiteMetric(metricTime, "RXRATE", String.valueOf(aggrxrate));
    	}
    }

    
    /*
     * Command loop
     */
    public static void doCommands() {
    	String command;
    	String FIFONAME = "/tmp/jmsbench-np";
    	String testPipe;
        
    	testPipe = System.getenv("PipeCommands");
    	if ((testPipe != null) && !(testPipe.isEmpty()))
    		pipeCommands = testPipe;

    	if (!pipeCommands.equalsIgnoreCase("0")) { /* Read from pipe */
			try {
				Runtime.getRuntime().exec("/usr/bin/mkfifo " + FIFONAME);
			} catch (IOException e) {
				e.printStackTrace();
				System.exit(1);
			}
		}
		
    	BufferedReader stdin = new BufferedReader(new InputStreamReader(System.in));
    	while(true) {
    		try {
    			System.err.print(DateFormat.getTimeInstance(DateFormat.MEDIUM).format(new Date()) + " bench> ");
    			
    			if(! pipeCommands.equalsIgnoreCase("0")){ /* Read from pipe */			
    				RandomAccessFile raf = new RandomAccessFile(FIFONAME, "r");
    				command = raf.readLine();
    				raf.close();
    				if (command == null) continue;
    			} else { /* read from stdin */
    				command = stdin.readLine();
    			}
    			
    			if(command.equalsIgnoreCase("quit") || command.equalsIgnoreCase("q")){
    				return;
    			}
    			if(command.equalsIgnoreCase("help") || command.equalsIgnoreCase("h") || command.equalsIgnoreCase("?")){
    				System.err.println();
    				/* update below when changing this */ 
    				System.err.println("possible commands: ");
    				System.err.println("\t\t help(h)" +
    								   "\n\t\t quit(q)" +
    								   "\n\t\t stat" +
    								   "\n\t\t rate=<new rate>" +
    								   "\n\t\t rc=<new recvcommit rate>" +
    								   "\n\t\t sc=<new sendcommit rate>" + 
    								   "\n\t\t l" + 
    								   "\n\t\t n" +
    								   "\n\t\t al" + 
    								   "\n\t\t qt=<qtname> or *" +
    								   "\n\t\t sess=<session id>");
    				continue;
    			}
    			if(command.equalsIgnoreCase("stat") || command.equalsIgnoreCase("")){
    				getStats(System.err, false, 0, null, 0);
    				continue;
    			}
    			if(command.equalsIgnoreCase("l") || command.equalsIgnoreCase("al")){
    				PrintStream out = new PrintStream(System.err);
    				ArrayList<JMSBenchTest> jbtList = consumerThreads;
    				jbtList.addAll(producerThreads);
    				/* Get latency for each type of latency (-T <mask>) by going through all producers AND consumers */
    				int mask = 0x1;
    				boolean printRTTMM = (latencyMask & PRINTRTTMM) == PRINTRTTMM;
    				boolean aggregateOnly = command.equalsIgnoreCase("al");
    				for (int i=0 ; i < Byte.SIZE ; mask<<=1, i++){
    					if((mask & latencyMask) == mask && (mask & latencyMask) != PRINTRTTMM){
    						Utils.calculateLatency(jbtList,mask,out,printRTTMM,aggregateOnly, null, 0, 0.0);
    					}
    				}
    				continue;
    			}
    			if(command.startsWith("rc=")){
    				int newrecvcommit=0;
    				StringTokenizer t = new StringTokenizer(command,"=");
    				t.nextToken(); 
    				newrecvcommit = Integer.valueOf(t.nextToken());
    				for (JMSBenchTest jbt : consumerThreads) {
    					jbt.recvCommit=newrecvcommit; 
    				}
    				continue;
    			}
    			if(command.startsWith("sc=")){
    				int newsendcommit=0;
    				StringTokenizer t = new StringTokenizer(command,"=");
    				t.nextToken(); 
    				newsendcommit = Integer.valueOf(t.nextToken());
    				for (JMSBenchTest jbt : producerThreads) {
    					jbt.sendCommit=newsendcommit; 
    				}
    				continue;
    			}
    			if(command.startsWith("rate=") || command.equalsIgnoreCase("r")){
    				int newrate=0;
    				StringTokenizer t = new StringTokenizer(command,"=");
    				t.nextToken(); /* skip left hand side of = */
    				newrate = Integer.valueOf(t.nextToken());
    				
    				if (producerThreads.size() == 0) {
    					System.err.println("rate command is only supported with producers.");
    				} else {
    					globalMsgRate = newrate;
    					int newPerPublisherRate = newrate / producerThreads.size();
    					if (rateControl == 1){
    						//globalRateController.initRateController(RateController.batchRate);
    						RateChanged_RCThread = 1;
    					} else if (rateControl == 0) {
    						for (JMSBenchTest jbt : producerThreads) {
    							jbt.perPubRateController.initRateController(newPerPublisherRate);
    						}
    					}
    				}
    				continue;
    			}
    			if(command.startsWith("qt=")){
    				if(command.equalsIgnoreCase("qt=*"))
    					printRemaining("*");
    				else {
    					StringTokenizer t = new StringTokenizer(command,"=");
        				t.nextToken(); /* skip left hand side of = */
        				printRemaining(t.nextToken());
    				}	
    				continue;
    			}
    			if(command.startsWith("n")){
    				printQTNames();
    				continue;
    			}
    			if(command.startsWith("sess=")){
    				StringTokenizer t = new StringTokenizer(command,"=");
    				long ackmsgcount= (long) 0;
    				boolean found = false;
    				t.nextToken();
    				String sessid = t.nextToken(); 
    				long lastACKMsg = 0;
    				for (JMSBenchTest jbt : consumerThreads) {
    					for (DestTuple<MessageConsumer> consTuple : jbt.consTuples) {
		    				if (sessid.equals(consTuple.SessionId)){
		    					found = true;
		    					ackmsgcount += consTuple.sessAckcount;
		    					if (consTuple.lastACKMsg > lastACKMsg)
		    						lastACKMsg = consTuple.lastACKMsg;
		    				}
    					}
    					if(found)
    						break;
    				}
    				if (found){
    					//System.err.println("Ack msg count: " + ackmsgcount + "   Last ack msg id: " + lastAckMsg);
    					System.err.println("Ack msg count: " + ackmsgcount);
    					System.err.println("Last ACKed msg (max for all destinations on this session): " + lastACKMsg);
    				}else{
    					System.err.println("Invalid session ID!");
    				}
    				continue;
    			}
    			System.err.println("unrecognized command");
    			System.err.println("possible commands: help quit stat rate=<new rate> recvcommit=<new recvcommit rate> sendcommit=<new sendcommit rate> sess=<sess id>");
    		} catch (Exception e) {
    			e.printStackTrace();
    			System.err.println("unrecognized command");
    			System.err.println("possible commands: help quit stat rate=<new rate> recvcommit=<new recvcommit rate> sendcommit=<new sendcommit rate> sess=<sess id>");
    		}
    	}
    }

    /*
     * Parse arguments from command line
     */
    public static int parseArgs(String [] args) {
    	int rc = 0;
    	boolean rateSet = false;
 
    	for (int i = 0; i < args.length; i++) {
    		if (args[i].equalsIgnoreCase("--help")) {
    	    	syntaxhelp("JMSBenchTest (v " + JBTVERSION);
    			rc=1;
    			break;
    		}
    		if (args[i].equalsIgnoreCase("-b")) {
    			try { 
        			beginStats = Integer.valueOf(args[++i]);
        		} catch (Exception e) {
        			syntaxhelp("Begin statistics is not valid (units in seconds)");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-c")) {
    			try { 
        			perDestMsgCount = Integer.valueOf(args[++i]);
        		} catch (Exception e) {
        			syntaxhelp("Message count is not valid (units in messages)");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-u")) {
    			try { 
        			UNITS = 1/Double.parseDouble(args[++i]);
        		} catch (Exception e) {
        			syntaxhelp("Units for latency measurements (-u) is not a valid floating point number");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-d")) {
    			try { 
        			testDuration = Integer.valueOf(args[++i]);
        			if(testDuration == 1)
        				throw new Exception();
        		} catch (Exception e) {
        			syntaxhelp("Test duration is not valid (units in seconds). Should be more than 1 second.");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-m")) {
    			try { 
        			rateControllerCPU = Integer.valueOf(args[++i]);
        		} catch (Exception e) {
        			syntaxhelp("Master rate controller cpu is not a valid CPU");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-r")) {
    			try { 
    				rateSet = true;
        			globalMsgRate = Integer.valueOf(args[++i]);
        		} catch (Exception e) {
        			syntaxhelp("Message rate is not valid (units in messages/second)");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-rr")) {
    			try { 
        			resetTStampXMsgs = Integer.valueOf(args[++i]);
        		} catch (Exception e) {
        			syntaxhelp("Reset reciever timestamp is not valid (units in messages).");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equalsIgnoreCase("-s")) {
    			try { 
        			StringTokenizer tok = new StringTokenizer(args[++i],"-");
        			minMsg = Integer.valueOf(tok.nextToken());
        			maxMsg = Integer.valueOf(tok.nextToken());
        			if (minMsg > maxMsg){
        				System.err.println("(w) The <max> message length must be at least <min> bytes in length.  Modifying <max> length to: " + minMsg);
        				maxMsg=minMsg;
        			}
        		} catch (Exception e) {
        			syntaxhelp("Message length is not valid.  Format of -s parameter is <min>-<max> where <min> and <max> are positive integer values (units in bytes)");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equals("-ds")) {
    			try {
    				delaySend=Integer.valueOf(args[++i]);
    				if (delaySend < 0)
    					throw new Exception();
    			} catch (Exception e) {
        			syntaxhelp("A valid value for the delaySend parameter (-ds) was not specified. Cannot be less than 0.");
        			rc=1;
        			break;
        		}
        		continue;
			}
    		if (args[i].equals("-as")) {
    			appSimLoop=Integer.valueOf(args[++i]);
    			if (appSimLoop < 0) {
    				asSleepMilli=Math.abs(appSimLoop)/1000;
    				System.err.println("sleep milli: "+asSleepMilli);
    				asSleepNano=Math.abs(appSimLoop)%1000*1000;
    				System.err.println("sleep nano: "+asSleepNano);
    			}
        		continue;
			}
    		if (args[i].equals("-t")) {
    			try { 
        			rateControl = Integer.valueOf(args[++i]);
        			if(rateControl < -1 || rateControl > 1)
        				throw new Exception();
        		} catch (Exception e) {
        			syntaxhelp("A valid value for the rateControl parameter (-t) was not specified");
        			rc=1;
        			break;
        		}			
        		continue;
			}
    		if (args[i].equals("-T")) {		
    			try {
    				latencyMask=Integer.decode(args[++i]).intValue();
    				
    			} catch (NumberFormatException e) {
    				syntaxhelp("A valid value for the latencyMask parameter (-T) was not specified. Must be a hexadecimal number");
    				rc=1;
    				break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-tx")) {
    			try { 
    				StringTokenizer prods = new StringTokenizer(args[++i], ",");
    				int j=0;
    				while(prods.hasMoreTokens()){
    					int tidx=0;
    					String pub = prods.nextToken();
    					StringTokenizer tx = new StringTokenizer(pub, ":");
    					JMSBenchTest jbt = new JMSBenchTest();
    					jbt.id = j++;															/* producer id */
    					jbt.threadIndex = tidx++;												/* used in the queue/topic name to indicate which thread created the queue/topic */
    					jbt.cpu = Integer.valueOf(tx.nextToken());								/* cpu affinity */
    					jbt.sendCommit = Integer.valueOf(tx.nextToken());   					/* msgs/commit */
    					jbt.deliveryMode = Integer.valueOf(tx.nextToken())==1 ? DeliveryMode.PERSISTENT : DeliveryMode.NON_PERSISTENT;
    					jbt.action = SEND;			      										/* no rate controller is the default, 
    																							 * set -t to enable rate controller */
    					jbt.qtName = tx.nextToken();											/* get the base name of the queue or topic */
    					jbt.qtps = Integer.valueOf(tx.nextToken());								/* get the number of queues or topics per session */
    					jbt.spc = Integer.valueOf(tx.nextToken());								/* get the number of sessions per connection */
    					jbt.cpt = Integer.valueOf(tx.nextToken());								/* get the number of connections per thread */

    					if (tx.hasMoreTokens())
    						jbt.nThrds = Integer.valueOf(tx.nextToken());                           /* get the number of threads to create. */
    					
    					if (tx.hasMoreTokens())
    						jbt.qnStart = Integer.valueOf(tx.nextToken());                      /* get the starting number for queue name */

    					jbt.ackMode = Session.DUPS_OK_ACKNOWLEDGE;								/* for now producers use DUPS_OK_ACKNOWLEDGE */
    					
						/* Need to create the non primitives for the class. */
						createJBTClassFields(jbt);
						
						/* Add to the producerThreads arraylist. */
   						producerThreads.add(jbt);
   						
   						/* If there were multiple threads requested then need to clone the
   						 * current JMSBenchTest class and then update. */
    					for (int k=1; k < Math.abs(jbt.nThrds) ; k++){
   							JMSBenchTest jbtClone = (JMSBenchTest)jbt.clone();

   							/* Need to create the non primitives for the clone. */
   							createJBTClassFields(jbtClone);
   							
   							/* Update the id since this is a new thread. */
   							jbtClone.id = j++;
   							jbtClone.threadIndex = tidx++;
   							
   							/* Add to the producerThreads arraylist. */
   							producerThreads.add(jbtClone);
    					}
    					total_conn = jbt.nThrds * jbt.cpt;
    				}
    			} catch (Exception e) {
    				syntaxhelp("-tx not valid format.  Valid format is -tx <txparams1>,<txparams2>,...");
    				rc=1;
        			break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-rx")) {
    			try { 
    				StringTokenizer subs = new StringTokenizer(args[++i], ",");
    				int j=0;
    				while(subs.hasMoreTokens()){
    					int tidx=0;
    					String sub = subs.nextToken();
    					StringTokenizer rx = new StringTokenizer(sub, ":");
    					JMSBenchTest jbt = new JMSBenchTest();
    					jbt.id = j++;															 /* consumer id */
    					jbt.threadIndex = tidx++;												 /* used in the queue/topic name to indicate which thread created the queue/topic */
    					jbt.cpu = Integer.valueOf(rx.nextToken());								 /* cpu affinity */
    					jbt.recvCommit = Integer.valueOf(rx.nextToken());						 /* msgs/commit */
    					jbt.action = Integer.valueOf(rx.nextToken()); 
    					jbt.ackMode = Integer.valueOf(rx.nextToken());							 /* get the ACK mode for the consumer */
    					jbt.durable = Integer.valueOf(rx.nextToken()) == 1 ? true : false;
    					jbt.qtName = rx.nextToken();											 /* get the name of the queue or topic */
    					jbt.qtps = Integer.valueOf(rx.nextToken());							 	 /* get the number of queues or topics per session */
    					jbt.spc = Integer.valueOf(rx.nextToken());							 	 /* get the number of sessions per connection */
    					jbt.cpt = Integer.valueOf(rx.nextToken());							 	 /* get the number of connections per thread */
    					
    					if (rx.hasMoreTokens())
    						jbt.nThrds = Integer.valueOf(rx.nextToken());                            /* get the number of threads to create. */
    					
    					if (rx.hasMoreTokens())
    						jbt.qnStart = Integer.valueOf(rx.nextToken());                       /* get the starting number for queue name */
    					
						/* Need to create the non primitives for the class. */
						createJBTClassFields(jbt);

						/* Add to the consumerThreads arraylist. */
						consumerThreads.add(jbt);
						
   						/* If there were multiple threads requested then need to clone the
   						 * current JMSBenchTest class and then update. */
    					for (int k=1; k < Math.abs(jbt.nThrds) ; k++){
   							JMSBenchTest jbtClone = (JMSBenchTest)jbt.clone();
   							
   							/* Need to create the non primitives for the clone. */
   							createJBTClassFields(jbtClone);
   							
   							/* Update the id since this is a new thread. */
   							jbtClone.id = j++;
   							jbtClone.threadIndex = tidx++;
   							
   							/* Add to the consumerThreads arraylist. */
   							consumerThreads.add(jbtClone);
    					}
    					total_conn = jbt.nThrds * jbt.cpt;
    				}
    			} catch (Exception e) {
    				syntaxhelp("-rx not valid format.  Valid format is -rx <rxparams1>,<rxparams2>,...");
    				rc=1;
    				break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-csv")) {
    			try {
    				csvLogFile = args[++i].toString();
    			} catch (Exception e) {
    				syntaxhelp("-csv specified but filename provided.");
    				rc=1;
        			break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-lcsv")) {
    			try {
    				csvLatencyFile = args[++i].toString();
    			} catch (Exception e) {
    				syntaxhelp("-lcsv specified but filename provided.");
    				rc=1;
        			break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-snap")) {
    			try {
    				snapInterval = Integer.valueOf(args[++i]);
    			} catch (Exception e) {
        			syntaxhelp("Snapshot interval is not valid (units in seconds)");
    				rc=1;
        			break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-mt")) {
    			try {
    				msgType = args[++i].toString();
    				if (msgType.equals("t") || msgType.equals("ss")){
    					isPubSub = true;
    					if (msgType.equals("ss"))
    						isSharedSub = true;
    				}
    			} catch (Exception e) {
        			syntaxhelp("Message type is not valid");
    				rc=1;
        			break;
    			}
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-wr")) {
    			waitReady = 1;
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-rrs")) {
    			roundRobinSendSub = 1;
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-anon")) {
    			Anonymousdest = true;
    			continue;
    		}
    		if (args[i].equalsIgnoreCase("-np")) {
    			try {
    				num_partition = Integer.valueOf(args[++i]);
    			} catch (Exception e) {
        			syntaxhelp("Number of partition is not valid");
    				rc=1;
        			break;
    			}
    			continue;
    		}
    		else
    		{
    			if (args[i].length() > 0)
    			{
    				System.err.println();
    				syntaxhelp("unknown command:  " + args[i].toString());
    				rc=1;
    				break;
    			}

    			continue;
    		}
    	}

    	do {
    		if( ((consumerThreads.isEmpty() && producerThreads.isEmpty()) || args.length==0) && rc != 1){
    			syntaxhelp("At least one producer (-tx) or one consumer (-rx) must be provided");
    			rc=1;
    			break;
    		}
    		
    		if ((producerThreads.isEmpty() == false) && (waitReady > 0)) {
    			if (producerThreads.size() > 1)
    				waitReady = producerThreads.size();
    			else {
    				System.err.println("(i) -wr option is not useful with a single producer thread.  Disabling!");
    				waitReady = 0;
    			}
    		}
    		
    		if (isSharedSub) {
    			if (producerThreads.isEmpty() == false){
    				syntaxhelp("Shared subscriptions (-mt ss) are only allowed on consumers.");
    				isSharedSub = false;
    				rc=1;
					break;
				}
    		}
    		
    		if (!isPubSub) {
				for (JMSBenchTest jbt : consumerThreads) {
					if (jbt.durable) {
						syntaxhelp("Durable subscriptions are only allowed on topics.  Must use command line parameter: -mt t or -mt ss");
						rc = 1;
						break;
					}
				}
				
				if (rc == 1)
					break;
    		}

    		/* If the csv log file wasn't specified then set it to the default: jmsbench.csv */
    		if (csvLogFile == null)
    			csvLogFile = "jmsbench.csv";
    		
    		/* If the csv latency file wasn't specified then set it to the default: latency.csv */
    		if (csvLatencyFile == null)
    			csvLatencyFile = "latency.csv";

        	/* Check to make sure the minMsg and maxMsg is greater than the minMsgLen. */
    		if (minMsg < minMsgLen) {
    			System.err.println("(w) The message length must be at least: " + minMsgLen + " (bytes).  Modifying length to: " + minMsgLen);
    			minMsg = minMsgLen;
    			if (minMsg > maxMsg){
    				System.err.println("(w) The <max> message length must be at least <min> bytes in length.  Modifying <max> length to: " + minMsg);
    				maxMsg=minMsg;
    			}
    		}
    		
    		/* -t -1 and -r <rate> are conflicting */
    		if( rateControl == -1 && rateSet){
    			syntaxhelp("The -t -1 and -r <rate> options are conflicting parameters.  Make command line parameters are consistent.");
    			rc=1;
    			break;
    		}
    		
    		/* RTT latency measurements are based on a sampling rate and message rate. When using the -1 value for rateControl (-t) no message rate is 
    		 * specified.  RTT latency measurements therefore cannot be taken when using -t -1. */
    		if (rateControl == -1 && ((latencyMask & CHKTIMERTT) == CHKTIMERTT) ){
    			System.err.println("(w) When using -t -1 and -T 0x1, the expected number of latency samples will not be achieved. Should consider running with -t 1 and -r <rate>.");
    		}
    		
    	} while(false);
    	
    	return rc;
    }

    /*
     * Create the class fields that are in the JMSBenchTest class.
     */
    public static void createJBTClassFields(JMSBenchTest jbt) {
		jbt.prodTuples = new ArrayList<DestTuple<MessageProducer> >();     /* list of destinations per producer */
		jbt.consTuples = new ArrayList<DestTuple<MessageConsumer> >();     /* list of destinations per consumer */
		jbt.snapSendRate = new ArrayList<Integer>();
		jbt.arecvcount = new AtomicLong(0);
		jbt.aAckcount = new AtomicLong(0);
		jbt.snapRecvRate = new ArrayList<Integer>();
    }

    /*
     * This benchmark test will measure message throughput.
     * 
     * This test takes the following parameters
     * 
     * 
     * -tx <producer>+   = comma separated list of producers. each producer token is 
     *                     a colon separated list of the following params:
     *      
     *                     <CPU affinity>:<# of msgs/commit>:<isPersistent>:<qtname>:<qtps>:<spc>:<cpt>:<nthrd>:<snum>
     *      
     *                     e.g. -tx 1:1:1:p1:1:1:1:1:1,2:1:1:p2:1:1:1:1:1 will create one producer thread bound to 
     *                     CPU 1 and one producer thread bound to CPU 2, each will create 
     *                     1 queue on 1 session on 1 connection and both will commit after every publish, and messages
     *                     will be persisted.
     *                     
     *                     NOTE: A cpu affinity of -1 means no CPU affinity.
     * -rx <consumer>+   = comma separated list of consumers. each consumer token is
     *                     a colon separated list of the following params:
     *      
     *                     <CPU affinity>:<# of msgs/commit>:<recieveMode>:<ackMode>:<qtname>:<qtps>:<spc>:<cpt>:<nthrd>:<snum>
     *      
     *                     receiveMode:
     *      		          0 = blocking receive                           (application thread)
     *      		          1 = blocking receive with timeout of 1 second  (application thread)
     *      		          2 = non-blocking receive                       (application thread)
     *      		          3 = message listener                           (ism client thread)
     *                     ackMode: 
     *      		          0 = transacted (ACK on commit)
     *      		          1 = auto_ack (ACK after return of receive message call)
     *      		          2 = client_ack (ACK on application call to message.acknowledge()
     *      	              3 = dups_ok_ack (ACK at configured rate)
     *      
     *                     e.g. -rx 1:1:1:0:p1:1:1:1:1:1,2:1:1:0:p2:1:1:1:1:1 will create one consumer thread bound to 
     *                     CPU 1 and one consumer thread bound to CPU 2, each will create 1 
     *                     queue on 1 session on 1 connection and both will commit after every recv and both will receive 
     *                     synchronously.
     *
     * The following environment variables are used by this test:
     * 
     * IMAServer 			- a space separated list of IP addresses of the server (default = 127.0.0.1). For example, export IMAServer="10.10.10.1 10.10.11.1".
     *                        When combined with the IMAPort list the two lists are combined as follows
     *                        export IMAServer="ip1 ip2 ip3"
     *                        export IMAPort="port1 port2 port3"
     *                        
     *                        Dest1=ip1:port1 Dest2=ip2:port2 Dest3=ip3:port3
     *                        
     *                        The length of the IMAServer and IMAPort lists must be the same and destination are assigned in
     *                        round robin fashion when creating client connnections.
     *                
     * IMAPort   			- the port number of the server (default = 16102)
     * AckInterval 			- interval in milliseconds between sending ACKs to the IMA server
     * IMATransport 		- the transport used by the IBM Messaging Appliance JMS client (TCP, ...)
     * SampleRate           - if not specifying a specific rate then set to: 10000  
     *                        Note:  This is required if running at message rates > 10M msgs/sec.
     *                         
     * The default can also be modified in the section below.
     */
    public static void syntaxhelp(String msg)  {
    	if (msg != null)
            System.err.println(msg);
    	
    	System.err.println();
    	System.err.println("JMSBenchTest [-d <seconds> | -c <msgcount> [-snap <seconds>]] [-ds <seconds>] [-t <mode>] [-m <cpu>] [-T <mode>] [-mt <mode>] ");
    	System.err.println("             [-as <loops>] [-csv <filename>] [-lcsv <filename>] [-b <seconds>] [-wr] [-rr <nummsgs>] -s <min>-<max> -r <msgs/sec>");
    	System.err.println("             -tx txparams1,txparams2,... -rx rxparams1,rxparams2,...");
    	System.err.println();
    	System.err.println("   -d <second>       ; test duration. A value 0 means run forever, duration and message count (-c) are mutually");
    	System.err.println("                     ; exclusive, -d takes precedence over -c");
    	System.err.println("   -c <num_messages> ; send num_messages to every destination of every producer WITHOUT rate control");
    	System.err.println("                     ; NOTE: THE -c OPTION CAN ONLY BE USED WHEN RUNNING ALL CONSUMERS AND PRODUCERS IN A SINGLE JVM");
    	System.err.println("   -snap <seconds>   ; print statistical snapshots at interval of <seconds>");
    	System.err.println("   -b <seconds>      ; begin collecting statistics after x seconds");
    	System.err.println("   -ds <seconds>     ; option to add a delay per producer thread before sending messages");
    	System.err.println("   -wr               ; Option to add a delay per producer thread before sending messages");
    	System.err.println("   -rr <num_messages>; Option to reset the statistics after x messages received by consumers.");
    	System.err.println("   -t <mode>         ; mode: -1, no rate control ");
    	System.err.println("                     ; mode:  0 (default), individual producers control rate ");
    	System.err.println("                     ; mode:  1, single rate controller thread");
    	System.err.println("   -m <CPU>          ; set CPU affinity for master rate controller thread to <CPU>. Ignored when <mode> is < 1 for -t param");
    	System.err.println("   -T <mask>         ; enable latency measurements. ");
    	System.err.println("                     ; 0x1: enable RTT latency measurements");
    	System.err.println("                     ; 0x2: print min/max average RTT latency destinations");
    	System.err.println("                     ; 0x4: enable latency measurement of the commit call");
    	System.err.println("                     ; 0x8: enable latency measurement of the recv call (consumers only)");
    	System.err.println("                     ; 0x10: enable latency measurement of the send call (producers only)");
    	System.err.println("                     ; 0x20: enable latency measurement of the create JMS connection call");
    	System.err.println("                     ; 0x40: enable latency measurement of the create JMS session call");
    	System.err.println("   -mt <mode>        ; Client subscription mode. ");
    	System.err.println("                     ; t: JMS pub/sub (topic)");
    	System.err.println("                     ; q: JMS queue");
    	System.err.println("                     ; ss: JMS shared subscription (consumer only)");
    	System.err.println("   -as <loops>		 ; Loop in message processing to simulate application message processing of a message ");
    	System.err.println("                     ; Does MD5 digest of message for <loops> iterations");
    	System.err.println("                     ; Positive value means # of iterations");
    	System.err.println("                     ; Negative value means # of microsecond sleep");
    	System.err.println("   -s <min>-<max>    ; the range of messages size to be used in this test.  The units for min and max are bytes/message.");
    	System.err.println("                     ; when <min>=<max> then messages of only one size are created (default <min>=<max>=128)");
    	System.err.println("                     ; the range of messages is used to generate a binary progression of message size from <min> to <max>");
    	System.err.println("                     ; these messages will be sent in round robin order per destination");
    	System.err.println("   -r <msg/sec>      ; global message rate (msgs/sec) across all producer threads. If the test is configured ");
    	System.err.println("                     ; for 3 producers and -r 3000 is provided, then each producer will send 1000 msgs/sec ");
    	System.err.println("   -rrs              ; use round robin in subscriptions");
        System.err.println("   -np <partitions>  ; set # of partitions for multi-level topic names ");
    	System.err.println("   txparams = <cpu>:<msgs/commit>:<isPersistent>:<qtname>:[-]<qtps>:[-]<spc>:[-]<cpt>:[-]<nthrd>:<snum>");
    	System.err.println("   rxparams = <cpu>:<msgs/commit>:<receiveMode>:<ackMode>:<durable>:<qtname>:[-]<qtps>:[-]<spc>:[-]<cpt>:[-]<nthrd>:<snum>");
    	System.err.println("     Tokens: ");
    	System.err.println("       receiveMode: 0=BLOCKRECV, 1=BLOCKRECVTO, 2=NONBLOCKRECV, 3=MSGLISTENER");
    	System.err.println("       ackMode: 0=transacted, 1=auto_ack, 2=client_ack, 3=dups_ok_ack");
    	System.err.println("       isPersistent: 0=non-persistent messages 1=persistent messages");
    	System.err.println("       msgs/commit: 0=non-transacted session, >0 transacted session");
    	System.err.println("       durable: 0=false, 1=true (durable topic subscriber)");
    	System.err.println("       qtname: base queue or topic name used by the producer or consumer");
    	System.err.println("       qtps: number of queues or topics per session");
    	System.err.println("       spc: number of sessions per connection");
    	System.err.println("       cpt: number of connections per producer or consumer thread");
    	System.err.println("       cpu affinity of -1 means no affinity is set.");
    	System.err.println("       nthrd: number of threads to create with same characteristics as current one.");
    	System.err.println("       snum: starting number to name queues at.");
    	System.err.println("       negative values for nthrd, cpt, spc, or qtps denote that this will not be a component of the queue/topic name");
    	System.err.println("   -u <units> ; units of time for storing latency measurements (e.g. 1e-6 to store latency in units of microseconds) ");
    	System.err.println("   -csv <filename> write the data to file with name <filename> in csv format (default: jmsbench.csv");
    	System.err.println("   -lcsv <filename> write the latency data to file with name <filename> in csv format (default: latency.csv");
    	System.err.println();
    	System.err.println("The location of the IMA server can be specified using two environment variables:");
    	System.err.println("   IMAServer     = The space separated list of server IP addresses (default = 127.0.0.1).");
    	System.err.println("   IMAPort       = The space separated list of server port numbers (default = 16102).");
    	System.err.println("   AckInterval   = The interval at which session ACKs are sent to the server (default = 100).");
    	System.err.println("   SampleRate    = Set this value to 10000 if running at message rates > 10M msgs/sec.");
    	System.err.println("See JMSBenchTest.java for latest env variables.");
    }
    
    
    /*
     * Main method 
     */
    public static void main(String [] args) {
    	int i;
    	int rc=0;
    	consumerThreads = new ArrayList<JMSBenchTest>();
    	producerThreads = new ArrayList<JMSBenchTest>();
    	ArrayList<Thread> rxthreads = new ArrayList<Thread>();
    	ArrayList<Thread> txthreads = new ArrayList<Thread>();
    	qtMap = new HashMap<String,QTMap>();
    	rateControllerMutex = new Object();
    	serverIPs = new ArrayList<String>();
    	serverPorts = new ArrayList<String>();
    	
    	String ip = System.getenv("GraphiteIP");
    	String gport = System.getenv("GraphitePort");
    	String root = System.getenv("GraphiteMetricRoot");
    	if (ip != null)
    		GraphiteIP = ip;
    	if (gport != null) {
	    	try {
				GraphitePort =       Integer.parseInt(gport);
			} catch (NumberFormatException e1) {
				System.err.println("Environment variable: GraphitePort is not an integer, using default port 2003");
			}
    	}
    	if (root != null) {
    		GraphiteMetricRoot = root; 
    	}
    	
    	String homeDir      = System.getenv("HOME");
	    if (homeDir == null)
	    	homeDir = "/tmp" ;
	    consumerStatusFile  = homeDir + "/" + consumerStatusFile ;
	    producerStatusFile  = homeDir + "/" + producerStatusFile ;
	    
        /* Remove the ConsumersReady.txt and ProducersReady.txt files if it exists. */
        Utils.removeFile(JMSBenchTest.consumerStatusFile);
        Utils.removeFile(JMSBenchTest.producerStatusFile);
    	
    	if(parseArgs(args)!=0)
    		System.exit(1);
    	
    	/*
    	 * Create connection factory
    	 */
    	try {
        	fact = ImaJmsFactory.createConnectionFactory();
            
    	    String server       = System.getenv("IMAServer");
    	    String port         = System.getenv("IMAPort");
    	    String imaserverset = System.getenv("IMAServerSets");
    	    String ackInterval  = System.getenv("AckInterval");
    	    String clntMsgCache = System.getenv("ClientMessageCache");
    	    String asyncSend    = System.getenv("AsyncSend");
    	    
    	    if ((ackInterval == null) || (ackInterval.length() == 0)){
    	      ackInterval = "100";	
    	    }
   	    	ackIntrvl            = Integer.parseInt(ackInterval);
    	    disableACK           = System.getenv("DisableACK");             /* DisableACK env variable is stored in this static variable. Possible values are 1 (true) or 0 (false) */
    	    
    	    if ( (imaserverset != null) && (imaserverset.length() != 0)){
    	    	imaServerSets        = Integer.parseInt(imaserverset);
    	    }
    	    
    	    ((ImaProperties)fact).put("DisableACK",false);
    	    if ((disableACK != null) && (disableACK.length() != 0)) {    	    		
    	    	((ImaProperties)fact).put("DisableACK", (Integer.parseInt(disableACK) != 0));  // DisableACK possible values are 1 (true) or 0 (false) 
    	    }
    	    
    	    String transport     = System.getenv("IMATransport");
    	    String recvBufferSize    = System.getenv("RecvBufferSize");     // size of buffer to read when reading from network
    	    String recvSockBuffer    = System.getenv("RecvSockBuffer");
    	    String sendSockBuffer    = System.getenv("SendSockBuffer");
    	    String latSampleRate = System.getenv("SampleRate");  /* Sample Rate for latency. */
    	    String useSecureConn = System.getenv("UseSecureConn");
    	    String SSLClientMeth = System.getenv("SSLClientMeth");
    	    String CipherSuites = System.getenv("SSLCipher");
            username = System.getenv("Username");
            password = System.getenv("Password");
            ConnBal = System.getenv("LOADBAL");
            
            if (ConnBal != null ){
		        StringTokenizer bal_conn = new StringTokenizer(ConnBal, ":");
		        num_connbal = bal_conn.countTokens();
		        if (num_connbal > 0){
		            connBalance = new int [num_connbal];
		            int idx, val=0;
					for (idx = 0; idx < num_connbal ; idx++) {
						connBalance[idx] = total_conn * Integer.valueOf(bal_conn.nextToken()) / 100;
						val += connBalance[idx];
					}
					while (total_conn - val > 0){
						idx = idx % num_connbal;
						connBalance[idx] = 1;
						val += 1;
						idx ++;
					}
	            }
            }
    	    
    	    if ((server != null) && (server.length()!= 0)){
    	    	StringTokenizer tok = new StringTokenizer(server);
    	    	while(tok.hasMoreTokens()){
					serverIPs.add(tok.nextToken());
    	    	}
    	    } else {
    	    	serverIPs.add("127.0.0.1");
    	    }
    	    if ((port != null) && (port.length() != 0)){
    	    	StringTokenizer tok = new StringTokenizer(port);
    	    	while(tok.hasMoreTokens()){
					serverPorts.add(tok.nextToken());
    	    	}
    	    } else {
    	    	serverPorts.add("16102");
    	    }
    	    
    	    if (serverPorts.size() != serverIPs.size()){
    	    	System.err.println("The IMAServer env variable list must be the same length as the IMAPort env variable list.");
    	    	rc=1;
    	    	throw new Exception();
    	    }

    	    ((ImaProperties)fact).put("AsyncTransactionSend",true);
    	    if ((asyncSend != null) && (asyncSend.length() != 0))
    	    	((ImaProperties)fact).put("AsyncTransactionSend",Integer.parseInt(asyncSend) != 0);
    	    
    	    if ((useSecureConn != null) && (useSecureConn.length() != 0))
    	    	useSecureConnection = Integer.valueOf(useSecureConn);
    	    
    	    if (CipherSuites != null)
    	    	((ImaProperties)fact).put("CipherSuites",CipherSuites);
    	    
   	    	((ImaProperties)fact).put("AckInterval", ackInterval);
   	    	/* Default Client Message Cache for JMSBenchTest is 1000 messages */
   	    	((ImaProperties)fact).put("ClientMessageCache", 1000);
   	    	if ((clntMsgCache != null) && (clntMsgCache.length() != 0))
   	    		((ImaProperties)fact).put("ClientMessageCache", clntMsgCache);
    	    if ((recvBufferSize != null) && (recvBufferSize.length() != 0))
    	    	((ImaProperties)fact).put("RecvBufferSize", recvBufferSize);
    	    if ((recvSockBuffer != null) && (recvSockBuffer.length() != 0))
    	    	((ImaProperties)fact).put("RecvSockBuffer", recvSockBuffer);
    	    if ((sendSockBuffer != null) && (sendSockBuffer.length() != 0))
    	    	((ImaProperties)fact).put("SendSockBuffer", sendSockBuffer);
    	    if ((transport != null) && (transport.length() != 0))
    	    	((ImaProperties)fact).put("Protocol", transport);
    	    
    	    ((ImaProperties)fact).put("DisableMessageTimestamp", true);
            ((ImaProperties)fact).put("DisableMessageID", true);
    	    //((ImaProperties)fact).put("DisableMessageID", false);
            
            if ((latSampleRate != null) && (latSampleRate.length() != 0 ))
            	sampleRate = Integer.valueOf(latSampleRate);
        	            
            if (useSecureConnection > 0){
            	((ImaProperties)fact).put("Protocol", "tcps");
            	if (SSLClientMeth.equals("TLSv12"))
            		((ImaProperties)fact).put("SecurityProtocols", "TLSv1.2");
            	else if (SSLClientMeth.equals("TLSv1"))
            		((ImaProperties)fact).put("SecurityProtocols", "TLSv1");
            	else if (SSLClientMeth.equals("TLSv11"))
            		((ImaProperties)fact).put("SecurityProtocols", "TLSv1.1");
            	else if (SSLClientMeth.equals("SSLv3"))
            		((ImaProperties)fact).put("SecurityProtocols", "SSLv3");
            }
    	} catch (Exception e) {
    		e.printStackTrace(System.err);
    		rc=1;
    		System.exit(rc);
    	}
    	
    	/* Build destination lists for consumers and producers and check for "under-subscription" */
    	do{
    		int rxQSet=0, txQSet=0;
    		try {
    			/* Build consumer destinations */
    			for (JMSBenchTest jbt : consumerThreads) {
    				rxQSet = jbt.buildDestinationList();
    			}

    			/* Build producer destinations */
    			for (JMSBenchTest jbt : producerThreads) {
    				txQSet = jbt.buildDestinationList();
    			}
    		} catch (JMSException e) {
    			System.err.println("(e) Unable to build the requested message consumers and producers. Exiting...");
    			e.printStackTrace();
    			rc=1;
    			break;
    		}

    		if(perDestMsgCount>0){
    			if(isPubSub){
    					for (JMSBenchTest jbt : consumerThreads) {
    						jbt.perThreadMsgCount = perDestMsgCount * jbt.consTuples.size();
    					}
    			} else {
    				for (JMSBenchTest jbt : consumerThreads) {
						jbt.perThreadMsgCount = Long.MAX_VALUE;     /* when using message queues make sure that we never exit the for loop because 
						 					 						 * the first condition evaluates to false (see TimerTask below for exiting for loop
						 					 						 * in message queueing case; i.e. second condition).
						 					 						 * doReceive... for loop when using -c command line option */
    				}
    			}
    			for (JMSBenchTest jbt : producerThreads) {
    				jbt.perThreadMsgCount = perDestMsgCount * jbt.prodTuples.size();
    			}

    			if(!isPubSub){ /* make sure all queues have at least one subscriber to when using the -c command line option. 
    			                * TODO may want to add a warning in all other cases if user configures an under-subscribing scenario */
    				if(rxQSet < txQSet){
    					System.err.println("(e) Consumers are under-subscribed.  When running with the -c option AND using message queues " +
    					                   " the consumers must subscribe to all queues that producers are sending to.");
    					rc=1;
    					break;
    				}

    				/* when using queues and -c option need to periodically check when 
    				 * aggregate receive count equals expected send count (i.e. totalTxCount) */
    				consMsgChk = new Timer("ConsMsgChecker", true);
    				consMsgChkTask = new TimerTask() {

    					public void run() {
    						for (String qt : qtMap.keySet()) { // iterate through each queue in qMap
    							QTMap map = qtMap.get(qt);
    							if(!map.isComplete){
    								long expectedTxQCount = 0;
    								long actualRxQCount = 0;
    								long actualTxQCount = 0;
    								for (DestTuple<?> tuple : map.tuples) {  // iterate through all MessageProducers and MessageConsumers associated with the queue
    									if(!tuple.isConsumer()){
    										expectedTxQCount+=perDestMsgCount; // total TX count for the queue is the sum of the product MessageProducers and msgs/MessageProducer
    										actualTxQCount+=tuple.count;	
    										if(!tuple.isDone && tuple.count == perDestMsgCount)
    											tuple.isDone = true;  // mark this MessageProducer as done
    									} else {
    										actualRxQCount+=tuple.count;  // add up RX counts on the queue
    									}	
    								}

    								if (actualRxQCount == expectedTxQCount){  // if RX counts equals expected the mark all MessageConsumers associated with the queue as done
    									for (DestTuple<?> tuple : map.tuples) {
    										if(tuple.isConsumer() && !tuple.isDone)
    											tuple.isDone = true;
    									}
    									map.isComplete = true;
    								}	
    							}
							}
    						
							for (JMSBenchTest jbt : consumerThreads) {
								boolean rxdone = true;
								if(!jbt.recvdone){
									for (DestTuple<MessageConsumer> tuple : jbt.consTuples) {
										if(!tuple.isDone){
											rxdone=false;
											break;
										}
									}
									jbt.recvdone = rxdone;  // if all MessageConsumers associated with a Consumer thread are done then mark the Consumer thread as done
								}
							}
    					}
    				};
    				consMsgChk.schedule(consMsgChkTask, 1000, 2000); /* schedule in 1 second and run every 2 seconds */
    			}
    		}
    	} while(false);
    	
    	if(rc!=0){
    		System.exit(rc);
    	}
    	
    	/*
    	 * Start the receiving threads 
    	 */
    	i=1;
    	for (JMSBenchTest jbt : consumerThreads) {
   			Thread t = new Thread(jbt,"Cons"+i++);
   			rxthreads.add(t);
   			t.start();
		}
    	
    	/* Give consumers a chance to initialize before sending messages */
    	try {
			Thread.sleep(1000);
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
    	
    	/*
    	 * Start the sending threads.  The sending threads will use the data provided by the
    	 * buildDestinationsList and create the connections and sessions.  
    	 */
		i=1;
    	for (JMSBenchTest jbt : producerThreads) {
   			Thread t = new Thread(jbt,"Prod"+i++);
			txthreads.add(t);
			t.start();
		}
    	
    	/* If the delay send option was used then must cause the main thread to 
    	 * sleep as well since the sending threads will not be sleeping the 
    	 * delay send amount of time.  Hence, do not want to initialize the
    	 * statistics until the delay send time. */
   		if(delaySend > 0){
   			try {
				Thread.sleep(delaySend * 1000);  // delaySend (-ds) is in units of seconds
			} catch (InterruptedException e) {
				System.err.println("Took an interrupt exception in main during delay time before sending messages.");
				e.printStackTrace();
			}  
   		}  else if (waitReady > 1) {
   			try {
   				int prodReady = 0;
   				while (prodReady < waitReady) {
   					prodReady = 0;
   					
   					for (JMSBenchTest jbt : producerThreads) {
   						if (jbt.clientReady)
   							prodReady++;
   					}
   					
   					if (prodReady < waitReady)
   						Thread.sleep(0, 1000);
   				}
   				
   				waitReady = 0;
   			} catch (InterruptedException e) {
				System.err.println("Took an interrupt exception in main while waiting for the producer(s) to be ready.");
				e.printStackTrace();
   			}
   		}
   		
    	/* Initialize statistics */
		for (JMSBenchTest jbt : consumerThreads) {
			jbt.prevtime = System.currentTimeMillis();
			jbt.lastrecvcount = jbt.arecvcount.get();
			jbt.firstrecvcount = 0;
		}
		for (JMSBenchTest jbt : producerThreads) {
			jbt.prevtime = System.currentTimeMillis();
			jbt.lastsendcount = jbt.sendcount;
			//jbt.lastsendcount = jbt.asendcount.get();
			jbt.firstsendcount = 0;
		}
    	
    	/* When specifying that the test duration should be specified based on messages received 
    	 * then the main thread should wait for the consumers to receive everything. Otherwise
    	 * the main thread will tell the producers and consumer when to stop */
    	if(perDestMsgCount!=0 && testDuration==0) {  /* using the -c option */
    		try {
				while(true){
					/* Check all consumers and producers if they are done*/
					int notdone = consumerThreads.size() + producerThreads.size();
					for (JMSBenchTest jbt : consumerThreads) {
						if(jbt.recvdone)
							notdone--;
					}
					for (JMSBenchTest jbt : producerThreads) {
						if(jbt.senddone)
							notdone--;
					}	
					if (notdone == 0)
						break;
					
					if (snapInterval!=0) {
						Thread.sleep(snapInterval * 1000); /* convert seconds to milliseconds */
						getStats(System.err, false, 0, null, 0);
					} else {
						Thread.sleep(2000);
					}
				}	
				
			} catch (InterruptedException e) {
				e.printStackTrace();
			}
    	}
    	else if (testDuration == 0) { /* using -d 0 ... Run forever */
    		/* create ReadyFile after connections finish */
    		if ((consumerThreads.isEmpty() == false) && producerThreads.isEmpty()) {
    			try {
        			int consReady = 0;
       				while (consReady < consumerThreads.size()) {
       					consReady = 0;
       					for (JMSBenchTest jbt : consumerThreads) {
       						if (jbt.clientReady)
       							consReady++;
       					}
       					//System.err.println("Consumers are not ready.");
       					if (consReady < consumerThreads.size())
       						Thread.sleep(1000,0);
       				}
       			
       				try {
       					PrintStream ConsReadyFile = new PrintStream(new FileOutputStream(JMSBenchTest.consumerStatusFile));
       					ConsReadyFile.println("All the consumers have started for JMSBenchTest.");
        			} catch (FileNotFoundException e) {
        				System.err.println("Unable to open the file: " + JMSBenchTest.consumerStatusFile + "  Exiting! ");
        				System.exit(89);
        			}
       			} catch (InterruptedException e) {
    				System.err.println("Took an interrupt exception in main while waiting for the consumer(s) to be ready.");
    				e.printStackTrace();
    			}
    		} else if ((producerThreads.isEmpty() == false) && consumerThreads.isEmpty()) {
    			try {
        			int prodReady = 0;
       				while (prodReady < producerThreads.size()) {
       					prodReady = 0;
       					for (JMSBenchTest jbt : producerThreads) {
       						if (jbt.clientReady)
       							prodReady++;
       					}
       					
       					if (prodReady < producerThreads.size())
       						Thread.sleep(0, 1000);
       				}
       				
       				try {
       					PrintStream ProdReadyFile = new PrintStream(new FileOutputStream(JMSBenchTest.producerStatusFile));    			
       					ProdReadyFile.println("All the producers have started for JMSBenchTest.");
        			} catch (FileNotFoundException e) {
        				System.err.println("Unable to open the file: " + JMSBenchTest.producerStatusFile + "  Exiting! ");
        				System.exit(89);
        			}
    			} catch (InterruptedException e) {
    				System.err.println("Took an interrupt exception in main while waiting for the producer(s) to be ready.");
    				e.printStackTrace();
    			}
    		}
    		
    		if(GraphiteIP.length() != 0) {
	    		graphiteSender = new GraphiteSender(GraphiteIP, GraphitePort, GraphiteMetricRoot);
	    		scheduler = Executors.newScheduledThreadPool(1);
	    		if(snapInterval == 0)
	    			snapInterval = 5;
	    		timer = scheduler.scheduleAtFixedRate(graphiteSender, snapInterval, snapInterval, TimeUnit.SECONDS);
    		}
    		doCommands();
    		long currTime = (long) (System.currentTimeMillis() * 1e-3);
    		getStats(System.err, true, 0, graphiteSender, currTime);		
    	}
    	else { /* using -d > 0 ... Main thread tell producers and consumers when to stop */
			/* If this is just a consumer or producer process then need to determine if all the connections
			 * have occurred.  If so then create the file to indicate that either the consumers or producers
			 * are ready to go. */
    		if ((consumerThreads.isEmpty() == false) && producerThreads.isEmpty()) {
    			try {
        			int consReady = 0;
       				while (consReady < consumerThreads.size()) {
       					consReady = 0;
       					for (JMSBenchTest jbt : consumerThreads) {
       						if (jbt.clientReady)
       							consReady++;
       					}
       					
       					//System.err.println("Consumers are not ready.");
       					if (consReady < consumerThreads.size())
       						Thread.sleep(1000,0);
       				}
       			
       				try {
       					PrintStream ConsReadyFile = new PrintStream(new FileOutputStream(JMSBenchTest.consumerStatusFile));
       					ConsReadyFile.println("All the consumers have started for JMSBenchTest.");
       					if (resetTStampXMsgs > 0) {
       			   			try {
       			   				int numMsgsRecv = 0;
       			   				
       			   				while (numMsgsRecv < resetTStampXMsgs) {
       			   					numMsgsRecv = 0;
       			   					
       			   					for (JMSBenchTest jbt : consumerThreads) {
       			   						numMsgsRecv += jbt.arecvcount.get();
       			   					}

       			   					if (numMsgsRecv < resetTStampXMsgs)
       			   	   					Thread.sleep(0, 1000);
       			   				}
       			   			} catch (InterruptedException e) {
       							System.err.println("Took an interrupt exception in main while waiting for " + resetTStampXMsgs +
       									           " received message(s).");
       							e.printStackTrace();
       						}  
       			   		}
        			} catch (FileNotFoundException e) {
        				System.err.println("Unable to open the file: " + JMSBenchTest.consumerStatusFile + "  Exiting! ");
        				System.exit(89);
        			}
       			} catch (InterruptedException e) {
    				System.err.println("Took an interrupt exception in main while waiting for the consumer(s) to be ready.");
    				e.printStackTrace();
    			}
    		} else if ((producerThreads.isEmpty() == false) && consumerThreads.isEmpty()) {
    			try {
        			int prodReady = 0;
       				while (prodReady < producerThreads.size()) {
       					prodReady = 0;
       					for (JMSBenchTest jbt : producerThreads) {
       						if (jbt.clientReady)
       							prodReady++;
       					}
       					
       					if (prodReady < producerThreads.size())
       						Thread.sleep(0, 1000);
       				}
       				
       				try {
       					PrintStream ProdReadyFile = new PrintStream(new FileOutputStream(JMSBenchTest.producerStatusFile));    			
       					ProdReadyFile.println("All the producers have started for JMSBenchTest.");
        			} catch (FileNotFoundException e) {
        				System.err.println("Unable to open the file: " + JMSBenchTest.producerStatusFile + "  Exiting! ");
        				System.exit(89);
        			}
    			} catch (InterruptedException e) {
    				System.err.println("Took an interrupt exception in main while waiting for the producer(s) to be ready.");
    				e.printStackTrace();
    			}
    		}
    		
    		if ((latencyMask & CHKTIMERTT) == CHKTIMERTT)
    			System.err.println("Performing latency testing....");
    		
    		try {
    			PrintStream csvstream = new PrintStream(new FileOutputStream(csvLogFile));
    			long testStartTime = System.currentTimeMillis();
    			
    			/* If going to start statistics after x seconds then need to sleep and
    			 * reset the testStartTime.
    			 */
				if(beginStats>0)
				{
					Thread.sleep(beginStats * 1000); /* convert seconds to milliseconds */
	    			System.err.println("Resetting start time.");
	    			testDuration -= beginStats;
	    			resetStats();
	    			testStartTime = System.currentTimeMillis();
				}
				
    			if(snapInterval!=0)
    			{
    				int timeleft = testDuration;
    				
    				while(timeleft>0){
    					Thread.sleep(snapInterval * 1000); /* convert seconds to milliseconds */
    					getStats(null, true, 0, null, 0);
    					timeleft-=snapInterval;
    				}
    			} else {
    				Thread.sleep(testDuration * 1000); /* convert seconds to milliseconds */
    			}

    			/* Log results in csv file format */ 
    			getStats(csvstream, true, testStartTime, null, 0);
    			System.err.println();
    			System.err.println("Completed writing the stats to csv file.");

    		} catch (Exception e) {
    			e.printStackTrace();
    		} 
    	}
    	
    	/* Shutdown consumers */
    	for (JMSBenchTest jbt : consumerThreads) {
			jbt.recvdone=true;  /* shutdown consumers that are using -d or -c */
    	}
    	
		/* Shutdown producers */
		for (JMSBenchTest jbt : producerThreads) {
			jbt.senddone=true;  /* shutdown producers that are using -d */
			jbt.perThreadMsgCount = 0;   /* shutdown producers that are using -c */
		}

		/* Get latency for each type of latency (-T <mask>) by going through each consumers histogram */
		int mask = 0x1;
		try
		{
			PrintStream out = new PrintStream(new FileOutputStream(JMSBenchTest.csvLatencyFile));
			boolean printRTTMM =(latencyMask & PRINTRTTMM) == PRINTRTTMM; 
			for (i=0 ; i < Byte.SIZE ; mask<<=1, i++){
				if((mask & latencyMask) == 1){
					Utils.calculateLatency(consumerThreads, mask, out, printRTTMM, false, null, 0, 0.0);
				}
			}
		}
		catch (FileNotFoundException e)
		{
			System.err.println("Unable to open the file: " + JMSBenchTest.csvLatencyFile + "  Exiting! ");
			System.exit(89);
		}
   	
		/* Going to just exit at this point due to issue with queues being backed
		 * up.  If running at 1M msgs/sec and only handing 500K then there is 
		 * a substantial amount of time needed to empty the queues.  For automation
		 * purposes this is unacceptable and need to exit.
		 */
		try {
			/* Join consumer threads */
			for (Thread thread : rxthreads) {
				thread.join(1000);
			}
			/* Join producer threads */
			for (Thread thread : txthreads) {
				thread.join(1000);
			}
		} catch (InterruptedException e) {
			e.printStackTrace();
		}
		
		if (testDuration >= 0) {
			if ((consumerThreads.isEmpty() == false) && producerThreads.isEmpty())
				Utils.removeFile(JMSBenchTest.consumerStatusFile);
			else if ((producerThreads.isEmpty() == false) && consumerThreads.isEmpty())
				Utils.removeFile(JMSBenchTest.producerStatusFile);
		}

		System.exit(rc);
    }
   
}
