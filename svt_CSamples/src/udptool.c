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
 * A sample UDP tool
 * USAGE: ./udptool -h 
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

#define MAX_BUF_SZ 256
#define TxRx "TxRx"
#define RxTx "RxTx"
#define Rx "Rx"
#define Tx "Tx"

#define SOCKET_TRANSMIT 0
#define SOCKET_RECEIVE  1

#define TxTport "5061"
#define TxRport "5062"
#define RxTport "5062"
#define RxRport "5061"

#define UNICAST "UNICAST"
#define MULTICAST "MULTICAST"

pthread_t INVALID_THREAD;
int g_conditionMet = 0;
pthread_cond_t g_cond  = PTHREAD_COND_INITIALIZER;
pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
char g_routing[MAX_BUF_SZ]="UNICAST";
char g_rcv_port[MAX_BUF_SZ]="";
char g_rcv_host[MAX_BUF_SZ]="127.0.0.1";			/* required if receiver port is specified */
char g_snd_port[MAX_BUF_SZ]="";
char g_mode[MAX_BUF_SZ]="";
char g_snd_host[MAX_BUF_SZ]="127.0.0.1";			/* required if sender port is specified */
char g_msg[MAX_BUF_SZ]="";
unsigned int g_throttle=100000;		/* throttle one message every 10th of second*/
unsigned int g_msg_number=1000;	    /* number of messages to send before exitting */
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
	if (g_trace_level>=0){			\
		fprintf(log2err, "WARNING: [%s] " _fmt_, __FUNCTION__, ##_args_); \
		if(errno) perror("ERRNO");	\
	} 

#define LOG_ERROR( _fmt_, _args_...)  \
	if (g_trace_level>=0){			\
		fprintf(log2err, "ERROR: [%s:%d] " _fmt_, __FUNCTION__,__LINE__, ##_args_); \
		if(errno) perror("ERRNO");	\
	} 

#define HALT_ERROR( _fmt_, _args_...)  \
	fprintf(log2err, "HALT: [%s:%d] " _fmt_, __FUNCTION__, __LINE__, ##_args_); \
	if(errno) { perror("ERRNO"); }	\
	exit(1);


void output_results(const char * msg){
	LOG_STDOUT( "%s: %d Rx %d Tx %d\n",msg,getpid(), g_rcvd_count, g_sent_count);	
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
    printf("        -c routing     : set the routing scheme  (arg required)\n");
    printf("           Valid routing schemes: \n");
	printf("           -------------------------------------------------\n");
    printf("             UNICAST   : configure tool for unicast routing   \n");
    printf("             MULTICAST : configure tool for multicast routing\n");
    printf("        -d number      : set the throttle useconds  (arg required)\n");
    printf("        -h             : print help message\n");
    printf("        -m mode        : set the mode of the tool   (arg required)\n");
    printf("           Valid modes : \n");
	printf("           -------------------------------------------------\n");
    printf("                TxRx   : configure tool as a transmitter/receiver\n");
    printf("                RxTx   : configure tool as a receiver/transmitter\n");
    printf("                Tx     : configure tool as a transmitter \n");
    printf("                Rx     : configure tool as a receiver\n");
    printf("        -n number      : set the number of messages(arg required)\n");
    printf("        -o logfile     : set the logfile for output (arg required)\n");
    printf("        -r port        : set the receive port num  (arg required)\n");
    printf("        -R host        : set the receive host      (arg required)\n");
    printf("        -s port        : set the sender port num   (arg required)\n");
    printf("        -S host        : set the sender host       (arg required)\n");
    printf("        -t seconds     : set the timeout seconds    (arg required)\n");
    printf("        -v             : set to verbose mode\n");
    printf("------------------------------------------------------------\n");
	printf("                                                            \n");
    printf("------------------------------------------------------------\n");
	printf("Notes:                                                      \n");
    printf("------------------------------------------------------------\n");
	printf("                                                            \n");
	printf("0. For verbose help with examples and more notes do -v -h   \n");
	if (g_trace_level>0){			\
	printf("1. Only one RxTx and one TxRx process pair may be started.  \n");
	printf("   for a single set of ports , but one could start multiple \n");
	printf("   pairs for different ports                                \n");
	printf("2. The RxTx process must be started first.                  \n");
	printf("                                                            \n");
    printf("------------------------------------------------------------\n");
	printf("Examples:                                                   \n");
    printf("------------------------------------------------------------\n");
	printf("                                                            \n");
	printf("                                                            \n");
	printf("Example 1: Basic Usage on one host (UNICAST)                \n");
    printf("------------------------------------------------------------\n");
	printf("On mar022:  Start two processes in this order.              \n");
	printf("                                                            \n");
	printf(" %s -m RxTx 												\n",name);
	printf("                                                            \n");
	printf(" %s -m TxRx 												\n",name);
	printf("                                                            \n");
	printf("                                                            \n");
	printf("Example 2: Basic Usage on one host 100000 messages (UNICAST)\n");
    printf("------------------------------------------------------------\n");
	printf("On mar022:  Start two processes in this order.              \n");
	printf("                                                            \n");
	printf(" %s -m RxTx -n 100000 										\n",name);
	printf("                                                            \n");
	printf(" %s -m TxRx -n 100000 										\n",name);
	printf("                                                            \n");
	printf("                                                            \n");
	printf("Example 3: Advanced usage on two different systems (UNICAST)\n");
    printf("------------------------------------------------------------\n");
	printf("On mar022:  Start the receive process first                 \n");
	printf("                                                            \n");
	printf(" %s -r 5555 -R mar022 -s 5556 -S mar061 -n 100000 -m RxTx   \n",name);
	printf("                                                            \n");
	printf("On mar061:  Start the transmit process second with hosts and ports inverted\n");
	printf("                                                            \n");
	printf(" %s -r 5556 -R mar061 -s 5555 -S mar022 -n 100000 -m TxRx   \n",name);
	printf("                                                            \n");
	printf("                                                            \n");
	printf("Example 4: Advanced usage starting multiple RxTx/TxRx pairs  \n");
    printf("------------------------------------------------------------\n");
	printf("On mar022:  Start the receive processes first               \n");
	printf("                                                            \n");
	printf(" %s -r 5555 -R mar022 -s 5556 -S mar061 -n 100000 -m RxTx   \n",name);
	printf(" %s -r 6555 -R mar022 -s 6556 -S mar061 -n 100000 -m RxTx	\n",name);
	printf(" %s -r 7555 -R mar022 -s 7556 -S mar061 -n 100000 -m RxTx	\n",name);
	printf("                                                            \n");
	printf("On mar061:  Start the transmit processes second with hosts and ports inverted\n");
	printf("                                                            \n");
	printf(" %s -r 5556 -R mar061 -s 5555 -S mar022 -n 100000 -m TxRx   \n",name);
	printf(" %s -r 6556 -R mar061 -s 6555 -S mar022 -n 100000 -m TxRx   \n",name);
	printf(" %s -r 7556 -R mar061 -s 7555 -S mar022 -n 100000 -m TxRx   \n",name);
	printf("                                                            \n");
	printf("                                                            \n");
	printf("Example 5: Advanced usage MULTICAST                          \n");
    printf("------------------------------------------------------------\n");
	printf("On mar022:  Start the receive processes first               \n");
	printf("                                                            \n");
	printf(" %s  -R 224.0.0.1 -n 100000000 -m Rx  -c MULTICAST -v  \n",name);
	printf(" %s  -R 224.0.0.1 -n 100000000 -m Rx  -c MULTICAST -v  \n",name);
	printf(" %s  -R 224.0.0.1 -n 100000000 -m Rx  -c MULTICAST -v  \n",name);
	printf("                                                            \n");
	printf("On mar061:  Start multicast transmit processes as desired   \n");
	printf(" %s  -S 224.0.0.1 -n 1  -m Tx -c MULTICAST -v          \n",name);
	printf("                                                            \n");
    printf("------------------------------------------------------------\n");
	}
	exit(0);
}


void validate_arguments(void) {
	/* check that the input arguments are valid */
	/* todo: implement*/
	return;
}

void parse_arguments(int argc , char **argv){
	int c,i;
	if (argc<=1) { 
		usage(argv[0]); 
	}
	while ((c=getopt(argc,argv,"c:d:hm:n:o:r:R:s:S:t:v")) != -1) {
		switch (c) {
			case 'c':
				strncpy(g_routing, optarg,MAX_BUF_SZ);
				break;
			case 'd':
				g_throttle=strtoul(optarg,0,10);
				break;

			case 'm':
				strncpy(g_mode, optarg, MAX_BUF_SZ);
				if(g_rcv_port[0] == '\0' ) {
					if(strncmp(g_mode, Tx, 2) == 0) {
						strncpy(g_rcv_port, TxRport,MAX_BUF_SZ);
					} else {
						strncpy(g_rcv_port, RxRport,MAX_BUF_SZ);
					}
				}
				if(g_snd_port[0] == '\0' ) {
					if(strncmp(g_mode, Tx, 2) == 0) {
						strncpy(g_snd_port, TxTport,MAX_BUF_SZ);
					} else {
						strncpy(g_snd_port, RxTport,MAX_BUF_SZ);
					}
				}
				break;
			case 'h':
				usage(argv[0]);
				break;
			case 'n':
				g_msg_number=strtoul(optarg,0,10);
				LOG_INFO( "g_msg_number is %d\n", g_msg_number);
				break;
			case 'o':
				log2out=fopen(optarg, "w+");
				if (!log2out) {
      				HALT_ERROR ("Unable to open output log file %s", optarg);
				}
				log2err=log2out;
				break;
			case 'r':
				strncpy(g_rcv_port,optarg,MAX_BUF_SZ);
				break;
			case 'R':
				strncpy(g_rcv_host,optarg,MAX_BUF_SZ);
				break;
			case 's':
				strncpy(g_snd_port,optarg,MAX_BUF_SZ);
				break;
			case 'S':
				strncpy(g_snd_host,optarg,MAX_BUF_SZ);
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


struct internal_vars { 
	int sockfd;
	char *port;
	char *host;
	char *msg;
	struct addrinfo hints;
	struct addrinfo *servinfo;
	struct addrinfo *bindinfo;
	struct sockaddr remote_addr;
	int ret_getaddrinfo;
	char buf[MAX_BUF_SZ];	
	socklen_t addr_sz;	
	pid_t thepid;
	pthread_t otherThread;
};


void open_socket(struct internal_vars *r, int flag){
	struct ip_mreq mreq;
	int optval=1;

    if ((r->ret_getaddrinfo= getaddrinfo(r->host, r->port,
				 &(r->hints), &(r->servinfo))) != 0) {
        HALT_ERROR("getaddrinfo:gai_strerror:%s", 
			gai_strerror(r->ret_getaddrinfo));
    }

    for(r->bindinfo = r->servinfo; r->bindinfo != NULL; 
					r->bindinfo =r->bindinfo->ai_next) {
        if ((r->sockfd = socket(r->bindinfo->ai_family,
			 	r->bindinfo->ai_socktype, r->bindinfo->ai_protocol)) == -1) {
            LOG_WARNING("socket rc == -1 :");
            continue;
        }

		if (flag == SOCKET_RECEIVE) {

			if (strcmp(g_routing, MULTICAST )==0 ){
			    /* allow multiple sockets to use the same PORT number */
    			if (setsockopt(r->sockfd,SOL_SOCKET,SO_REUSEADDR,&optval,sizeof(int)) < 0) {
       				HALT_ERROR("setsockopt failed.");
				}
       		}

        	if (bind(r->sockfd, r->bindinfo->ai_addr, 
						r->bindinfo->ai_addrlen) == -1) {
            	close(r->sockfd);
            	LOG_WARNING("bind rc == -1 :");
            	continue;
        	}

			if (strcmp(g_routing, MULTICAST )==0 ){
     			mreq.imr_multiaddr.s_addr=inet_addr(g_rcv_host);
     			mreq.imr_interface.s_addr=htonl(INADDR_ANY);
     			if (setsockopt(r->sockfd,IPPROTO_IP,IP_ADD_MEMBERSHIP,&mreq,sizeof(mreq)) < 0) {
       				HALT_ERROR("setsockopt failed.");
				}		
     		}
		}

        break;
    }

    if (r->bindinfo == NULL) {
     	HALT_ERROR("bind socket failure\n");
    }

	LOG_INFO("Successfully opened socket to %s %s\n", r->host , r->port);
}

void check_thread_rc(int rc,int line ,const char*file){
	if(rc!=0){
		HALT_ERROR("pthread_mutex_lock returned non-zero: rc : %d",rc);
	}
    LOG_INFO("rc %d for %d\n", rc, line);
}

void wait_for_full_transmit_and_receive(struct internal_vars *t){
	int count_sleeps=0;
	if(!((strcmp(g_mode, TxRx) == 0) ||
	   (strcmp(g_mode, RxTx) == 0))) {
		LOG_INFO("This is not a  TxRx / RxTx pair. So tool does not " \
				 "wait for completion.\n");
		return;
	}
    while((g_rcvd_count < g_msg_number) || (g_sent_count<g_msg_number)){
		LOG_INFO("waiting up to 10 seconds for g_rcvd_count:%d and " \
			"g_sent_count%d to be == g_msg_number %d\n", 
			g_rcvd_count , g_sent_count, g_msg_number);
		if(count_sleeps ==0 ){  
			LOG_WARNING("Status: Rx:%d Tx:%d Waiting up to 10 seconds for " \
				" operation to finish\n", g_rcvd_count, g_sent_count);
		}
		count_sleeps++;
        sleep(1);
		if(count_sleeps > 10 ){  /* arbitrary wait up to 10 seconds*/
			LOG_WARNING("Complete transmit and receive was not achieved.\n");
			if(t->otherThread!=INVALID_THREAD){
				LOG_WARNING("Issue Cancel to other thread.\n");
				pthread_cancel(t->otherThread);
			}
			LOG_WARNING("Continuing....\n");
			break;
		}
    }
}

void * do_transmit (void *v){
    struct internal_vars *t=(struct internal_vars *)v;
	int numbytes,rc;

    memset(&(t->hints), 0, sizeof (t->hints));
	t->hints.ai_family = AF_UNSPEC;
	t->hints.ai_socktype = SOCK_DGRAM;
	t->port=g_snd_port;
	t->host=g_snd_host;
	open_socket(t, SOCKET_TRANSMIT);
	t->thepid=getpid();
	
	rc = pthread_mutex_lock(&g_mutex);
	check_thread_rc(rc, __LINE__, __FILE__);

	while (!g_conditionMet) {
    	LOG_INFO("Thread blocked\n");
    	rc = pthread_cond_wait(&g_cond, &g_mutex);
		check_thread_rc(rc, __LINE__, __FILE__);
  	}

	while(g_sent_count<g_msg_number){

    	LOG_INFO("sendto...\n");

		sprintf(t->buf, "Send msg %d from %d", g_sent_count,t->thepid);

    	if ((numbytes = sendto(t->sockfd, t->buf, strlen(t->buf), 0,
           		t->bindinfo->ai_addr, t->bindinfo->ai_addrlen)) == -1) {
			HALT_ERROR("sendto == -1");
    	}

		g_sent_count++;
	
		LOG_INFO("sent packet %d with %d bytes to %s %s\n",g_sent_count,
			 	numbytes, g_snd_host, g_snd_port);
	}

	wait_for_full_transmit_and_receive(t);

	freeaddrinfo(t->servinfo);
    close(t->sockfd);

	rc = pthread_mutex_unlock(&g_mutex);
	check_thread_rc(rc, __LINE__, __FILE__);

	LOG_INFO("Completed operation %s r : %d s %d n %d\n", __FUNCTION__, g_rcvd_count, g_sent_count, g_msg_number);

	return NULL;
}

void * do_receive (void *v) {
	struct internal_vars *r=(struct internal_vars *)v;
	int numbytes,rc;

   	memset(&(r->hints), 0, sizeof (r->hints));
   	r->hints.ai_family = AF_UNSPEC; // set to AF_INET to force IPv4
   	r->hints.ai_socktype = SOCK_DGRAM;
   	//r->hints.ai_flags = AI_PASSIVE;
	r->port=g_rcv_port;
	r->host=g_rcv_host;
   	r->addr_sz = sizeof(r->remote_addr);
	open_socket(r, SOCKET_RECEIVE);

	while(g_rcvd_count<g_msg_number){
		LOG_INFO("waiting to receive packet... on host %s port %s\n", r->host, r->port);
	
    	if ((numbytes = recvfrom(r->sockfd, r->buf, MAX_BUF_SZ-1 , 0,
       		(struct sockaddr *)&(r->remote_addr), &(r->addr_sz))) == -1) {
			HALT_ERROR("receivefrom returned -1");
    	}
	
		g_rcvd_count++;
	
		if (g_rcvd_count == 1 && !g_conditionMet) {
			rc = pthread_mutex_lock(&g_mutex);
			check_thread_rc(rc, __LINE__, __FILE__);

  			g_conditionMet = 1;	/* allow transmits after the first successful receive */
			LOG_INFO("Wake up all waiting threads...\n");

  			rc = pthread_cond_broadcast(&g_cond);
  			check_thread_rc(rc,  __LINE__, __FILE__);

  			rc = pthread_mutex_unlock(&g_mutex);
  			check_thread_rc(rc,  __LINE__, __FILE__);
		}
	
   		if(g_trace_level){
			LOG_INFO("received packet %d from port %s\n",g_rcvd_count, g_rcv_port);
   			LOG_INFO("packet is %d bytes long\n", numbytes);
   			r->buf[numbytes] = '\0';
   			LOG_INFO("packet contains \"%s\"\n", r->buf);
		}
	}

	wait_for_full_transmit_and_receive(r);

	freeaddrinfo(r->servinfo);
    close(r->sockfd);

	LOG_INFO("Completed operation %s r : %d s %d n %d\n", __FUNCTION__, g_rcvd_count, g_sent_count, g_msg_number);

	return NULL;
}

void run_transmitter_receiver(char *mode){
	struct internal_vars t;
	struct internal_vars r;
	pthread_t thread1, thread2;
	int rc;
	if ( strcmp(mode, RxTx) == 0){
		g_conditionMet=0; /* allow transmits after first receive */
	} else if ( strcmp(mode, TxRx) == 0){
		g_conditionMet=1; /* allow transmits immediately */
	} else {
		HALT_ERROR("Invalid/unsupported mode: %s", mode);
	}
	r.otherThread=thread2;
	rc=pthread_create(&thread1, NULL, do_receive,(void*)&r);
	check_thread_rc(rc, __LINE__, __FILE__);
	t.otherThread=thread1;
	rc=pthread_create(&thread2, NULL, do_transmit,(void*)&t);
	check_thread_rc(rc, __LINE__, __FILE__);
	
	rc = pthread_join(thread1, NULL);
	rc = pthread_join(thread2, NULL);

	pthread_cond_destroy(&g_cond);	 
	pthread_mutex_destroy(&g_mutex);

	LOG_INFO("Completed operation %s\n", __FUNCTION__);
	output_results(__FUNCTION__);

}

int main(int argc, char **argv){
	struct internal_vars t;
	struct internal_vars r;
	log2out=stdout;
	log2err=stderr;
	signal(SIGALRM, handle);

	parse_arguments(argc, argv);
	
	if (g_timeout_seconds>0){
		alarm(g_timeout_seconds);
	}

	if (strcmp(g_mode, RxTx) == 0) {	/* note if not really necessary, but validates inputs */
		run_transmitter_receiver(g_mode); 
	} else if (strcmp(g_mode, TxRx) == 0) {
		run_transmitter_receiver(g_mode);
	} else if (strcmp(g_mode, Rx) == 0) {
		r.otherThread=INVALID_THREAD;
		do_receive(&r);
	} else if (strcmp(g_mode, Tx) == 0) {
		g_conditionMet=1; /* allow transmits immediately */
		t.otherThread=INVALID_THREAD;
		do_transmit(&t);
	} else {
		HALT_ERROR("Invalid mode %s",g_mode);
	}

    return 0;
}
