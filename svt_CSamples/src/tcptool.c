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
/* 
 * A sample tcp tool
 * USAGE: ./tcptool -h 
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>

#define _MULTI_THREADED
#include <pthread.h>

#define MAX_BUF_SZ 1024
#define TxRx "TxRx"
#define RxTx "RxTx"

char g_port[MAX_BUF_SZ]="5061";
char g_mode[MAX_BUF_SZ]="";
char g_snd_host[MAX_BUF_SZ]="127.0.0.1";			
char g_msg[MAX_BUF_SZ]="";
unsigned int g_throttle=100000;		/* throttle one message every 10th of second*/
unsigned int g_msg_number=10;	    /* number of messages to send before exitting */
unsigned int g_sent_count=0;	
unsigned int g_rcvd_count=0;	
unsigned int g_trace_level=0;
unsigned int g_timeout_seconds=0;
char g_status[MAX_BUF_SZ]="";
FILE *log2out;
FILE *log2err;

#define LOG_STDOUT( _fmt_, _args_...)  \
	fprintf(log2out, "LOG : [%s] " _fmt_, __FUNCTION__, ##_args_); 

#define LOG_STDERR( _fmt_, _args_...)  \
	fprintf(log2err, "LOG _ [%s] " _fmt_, __FUNCTION__, ##_args_); 

#define LOG_INFO( _fmt_, _args_...)  \
	if (g_trace_level>0){			\
		 fprintf(log2err, "INFO: [%s] " _fmt_, __FUNCTION__, ##_args_); \
	} 

#define LOG_WARNING( _fmt_, _args_...)  \
	if (g_trace_level>0){			\
		fprintf(log2err, "WARNING: [%s] " _fmt_, __FUNCTION__, ##_args_); \
		if(errno) perror("ERRNO");	\
	} 

#define LOG_ERROR( _fmt_, _args_...)  \
	if (g_trace_level>0){			\
		fprintf(log2err, "ERROR: [%s:%d] " _fmt_, __FUNCTION__,__LINE__, ##_args_); \
		if(errno) perror("ERRNO");	\
	} 

#define HALT_ERROR( _fmt_, _args_...)  \
	fprintf(log2err, "HALT: [%s:%d] " _fmt_, __FUNCTION__, __LINE__, ##_args_); \
	if(errno) { perror("ERRNO"); }	\
	exit(1);


void output_results_thread(int r, int s) {
	LOG_STDOUT( "%d [threadid:%u] Rx %d Tx %d\n",getpid(),(unsigned int)pthread_self(), r, s);	
}

void output_results(const char * msg){
	if(msg) { LOG_STDOUT("%s:",msg); }
	LOG_STDOUT( "%d Rx %d Tx %d\n",getpid(), g_rcvd_count,g_sent_count);	
}

void handle(int sig){
	if (sig == SIGALRM) {
		/* Note that  printf is NOT in the POSIX.1-2003 list of 
		   safe functions for inside a signal handler includes write, but 
		   does not include printf */
		LOG_STDOUT("Alarm hit, printing results\n");
		output_results("alarm");
		exit(0);
	}
}

void usage(char *name){
	printf("\n");
	printf("------------------------------------------------------------\n");
	printf("Usage: %s [options] \n", name);
	printf("    Options: \n");
	printf("        -h             : print help message\n");
	printf("        -l logfile     : set the logfile for output (arg required)\n");
	printf("        -m mode        : set the mode of the tool   (arg required)\n");
	printf("           Valid modes : \n");
	printf("           -------------------------------------------------\n");
	printf("                TxRx   : configure tool as a transmitter/receiver\n");
	printf("                RxTx   : configure tool as a receiver/transmitter\n");
	printf("        -n number      : set the number of messages(arg required)\n");
	printf("        -p port        : set the port num  (arg required)\n");
	printf("        -S host        : set the send (to) host       (arg required)\n");
	printf("        -t seconds     : set the timeout seconds    (arg required)\n");
	printf("        -v             : set to verbose mode\n");
	printf("------------------------------------------------------------\n");
	printf("Notes:                                                      \n");
	printf("------------------------------------------------------------\n");
	printf("                                                            \n");
	printf("0. For verbose help with examples and more notes do -v -h   \n");
	printf("                                                            \n");
	if (g_trace_level>0){			\
	printf("------------------------------------------------------------\n");
	printf("Examples:                                                   \n");
	printf("------------------------------------------------------------\n");
	printf("                                                            \n");
	printf("Examples 1:Start senders and reciever on diff host / target \n");
	printf("------------------------------------------------------------\n");
	printf("                                                            \n");
	printf("Start a RxTx on mar022:                                     \n");
	printf("                                                            \n");
	printf(" %s -m RxTx ; #defaults to local host for receiver\n ",name);
	printf("                                                            \n");
	printf("Start 10 TxRx on mar061 to each send 10000 messages:        \n");
	printf("                                                            \n");
	printf(" let x=0; while(($x<10)); do " \
 		   "%s -m TxRx -S 10.10.10.10 -n 10000 & let x=$x+1; done\n", name);
	printf("                                                            \n");
	printf("------------------------------------------------------------\n");
	}
	exit(0);
}


void validate_arguments(void) {
	/* check that the input arguments are valid */
	return;
}

void parse_arguments(int argc , char **argv){
	int c,i;
	if (argc<=1) { 
		usage(argv[0]); 
	}
	while ((c=getopt(argc,argv,"hl:m:n:p:S:t:v")) != -1) {
		switch (c) {
			case 'l':
				log2out=fopen(optarg, "w+");
				if (!log2out) {
	  				HALT_ERROR ("Unable to open output log file %s", optarg);
				}
				log2err=log2out;
				break;
			case 'm':
				strncpy(g_mode, optarg, MAX_BUF_SZ);
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'n':
				g_msg_number=strtoul(optarg,0,10);
				LOG_INFO( "g_msg_number is %d\n", g_msg_number);
				break;
			case 'p':
				strncpy(g_port, optarg, MAX_BUF_SZ);
				break;
			case 'S':
				strcpy(g_snd_host, optarg);
				break;
			case 't':
				g_timeout_seconds=strtoul(optarg,0,10);
				break;
			case 'v':
				g_trace_level=10;
				break;
			default:
			 	LOG_ERROR("Incorrect switch.");
				usage(argv[0]);
		}
	}

	for (i=optind; i< argc; i++) {
	  	HALT_ERROR ("Unexpected non-option argument number %d \"%s\".\n"\
				"Check help menu with -h\n", i, argv[i]);
	}

}

void check_thread_rc(int rc,int line ,const char*file){
	if(rc!=0){
		HALT_ERROR("pthread_mutex_lock returned non-zero: rc : %d",rc);
	}
	LOG_INFO("rc %d for %d\n", rc, line);
}

struct internal_vars {
	int clientFd;
	int threadId;
	pthread_t *thread1;
	pthread_attr_t *attr;
	
};

void * do_rxtx(void*v){
	int rcvd_count=0;
	int sent_count=0,nBytes;
	char buf[MAX_BUF_SZ];
	struct internal_vars *t=(struct internal_vars *)v;

  	while (1) {	
		LOG_INFO("Waiting for input.\n");
   		bzero(buf, MAX_BUF_SZ);
   		nBytes = read(t->clientFd, buf, MAX_BUF_SZ);
   		if (nBytes < 0) {
   			HALT_ERROR("ERROR: receiving data from client fd:%d errno: %d\n", t->clientFd, errno);
   		} else if (nBytes == 0) {
   			LOG_INFO("END: done receiving data from client: %d %d\n", nBytes,errno);
			break;
   		} else {
   			LOG_INFO("Received: %d bytes: Buffer: %s\n", nBytes, buf);
			rcvd_count++;
   		} 
	
   		LOG_INFO("Responding\n");
		sprintf(buf, "Respond to threadId %d clientFd %d, message data \"%s\" with sent message %d", 
			 t->threadId, t->clientFd, buf, sent_count);
	   	nBytes = write(t->clientFd, buf, strlen(buf));
   		if (nBytes < 0) {
   			HALT_ERROR("ssending data to client %d\n",nBytes);
   		}
		sent_count++;
	}

   	LOG_INFO("Thread %d Results: ", t->threadId);
	output_results_thread(rcvd_count, sent_count);
   	LOG_INFO("Closing clientFd\n");
   	close(t->clientFd);
	free(t->thread1);
	free(t->attr);
	free(v);
	return NULL;
}

void run_thread(int clientFd, int threadId) {
	pthread_t *thread1 = (pthread_t *)malloc(sizeof(pthread_t));
	struct internal_vars *x= (struct internal_vars *)malloc(sizeof(struct internal_vars));
	int rc;
	pthread_attr_t *attr=(pthread_attr_t*)malloc(sizeof(pthread_attr_t));

	x->clientFd=clientFd;
	x->threadId=threadId;
	x->thread1=thread1;
	x->attr=attr;
	pthread_attr_init(attr);
	rc=pthread_attr_setdetachstate(attr, PTHREAD_CREATE_DETACHED);
	check_thread_rc(rc, __LINE__, __FILE__);
	LOG_INFO("Start new thread with clientFd: %d, threadId, %d\n", clientFd, threadId);
	fflush(stdout);
	fflush(stderr);
   	rc=pthread_create(thread1, attr, do_rxtx,(void*)x); /* let thread free its resources*/
	check_thread_rc(rc, __LINE__, __FILE__);
}

#define BACKLOG 5
void start_server(void){
	int serverPort = strtoul(g_port,0,10);
	int serverFd;
	int optval;
	int threadId=1;
	struct sockaddr_in serverAddr;
	
	int clientFd;
	struct sockaddr_in clientAddr;
	unsigned int clientLen = sizeof(clientAddr);
	
	serverFd = socket(AF_INET, SOCK_STREAM, 0);
	if (serverFd < 0) { HALT_ERROR("ERROR: socket call failed: %d\n", errno); }

	optval = 1;
	setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  	bzero((char *) &serverAddr, sizeof(serverAddr));
  	serverAddr.sin_family = AF_INET;
  	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons((unsigned short)serverPort);

	if (bind(serverFd, (struct sockaddr *) &serverAddr, sizeof(serverAddr)) < 0) {
	    close(serverFd);
	    HALT_ERROR("ERROR: bind failed: %d\n", errno);
	}


	if (listen(serverFd, BACKLOG) < 0) {
	    close(serverFd);
	    HALT_ERROR("ERROR: listen failed: %d\n", errno);
	}

	while(1) {
		LOG_INFO("listening on port %s\n", g_port);
		clientFd = accept(serverFd, (struct sockaddr *) &clientAddr, &clientLen);
		if (clientFd < 0) {
	    	close(serverFd);
			HALT_ERROR("ERROR: accept failed: %d\n", errno);
		} else {
			LOG_INFO("accepted connection fd %d\n",  clientFd);
		}
		LOG_INFO("lannch new thread to handle clientFd:%d and threadId:%d\n",clientFd,threadId);
		run_thread(clientFd,threadId);
		threadId++;
			
	}
}


void start_client(void){
	int clientFd,len;
	struct sockaddr_in serverAddr;
	struct hostent *server;
	char buffer[MAX_BUF_SZ]="";
	char *buff = buffer;
	int defaultPort = strtoul(g_port,0,10);
	int nBytes;

	clientFd = socket(AF_INET, SOCK_STREAM, 0);
	if ( clientFd < 0 ) {
	    printf("ERROR: socket call failed: %d\n", errno);fflush(stdout);
	    exit(1);
	}

	bzero((char *)&serverAddr, sizeof(serverAddr));
	server = gethostbyname(g_snd_host);
	if ( server == NULL ) {
	    close(clientFd);
	    HALT_ERROR("ERROR: gethostbyname call failed: %d\n", errno);
	}

	serverAddr.sin_family = AF_INET;
	memcpy((char *) &(serverAddr.sin_addr.s_addr), (char *)(server->h_addr), server->h_length);
	serverAddr.sin_port = htons(defaultPort);

	if (connect(clientFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
	    close(clientFd);
	    HALT_ERROR("ERROR: connect call failed: %d\n", errno);
	}
	
	LOG_INFO("connected on port %s\n", g_port);

	while(g_sent_count<g_msg_number) {
		sprintf(buff, "Send msg %d\n", g_sent_count);
		len=strlen(buff);	
		nBytes = send(clientFd, buff, len,0);
		if ( nBytes != len){
	   	close(clientFd);
	   		HALT_ERROR("ERROR: write failed: only %d of %d sent, errno %d\n", nBytes, len,errno); 
		}

  		LOG_INFO("Sent buffer %d (of %d ) :%s\n", nBytes, (int)strlen(buff), buff);
		g_sent_count++;

  		LOG_INFO("waiting to receive response\n");
	
		if( ( nBytes = recv(clientFd,buffer,MAX_BUF_SZ-1 ,0)) == -1) {
			LOG_WARNING("receive failed");
		} else { 
   			LOG_INFO("Received buffer %s (%d bytes)\n", buffer,(int) strlen(buffer));
			g_rcvd_count++;
		}
	}

	LOG_INFO("exitting\n");
	close(clientFd);
}



int main(int argc, char **argv){
	log2out=stdout;
	log2err=stderr;
	signal(SIGALRM, handle);

	parse_arguments(argc, argv);
	
	if (g_timeout_seconds>0){
		alarm(g_timeout_seconds);
	}

	if (strcmp(g_mode, RxTx) == 0) {	/* note if not really necessary, but validates inputs */
		start_server();
	} else if (strcmp(g_mode, TxRx) == 0) {
		start_client();
	} else {
		HALT_ERROR("Invalid mode %s",g_mode);
	}

	output_results(NULL);
	return 0;
}
