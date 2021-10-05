/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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
 * @file utils.c
 *
 * The purpose of this file is to implment helper and utility
 * functions that are necessary to implement the sample.
 * 
 */

#include <mqttsample.h>

/*
 * A Signal handling function
 * 
 * @param sig   The signal number that the handler is being called for.
 */
void do_signal_handle(int sig){

    if (sig==SIGUSR1){
        // the following is not POSIX compliant, for debug use only.
        // it is possible that unexpected results may occurr.
        do_info_display(MYDETAILED);
    } else if (sig==SIGINT){
        g_halt++;
        if (g_halt>3){
            do_info_display(MYDETAILED);
            HALT_ERROR("Exit due to SIGINT");
        }
    } else {
        g_halt++;
    }
}

/*
 * Call to sleep with high resolution
 *
 */
void perfSleep(int seconds, int microseconds){
    struct timespec ts, tr;

    while(microseconds >= MICROS_PER_SECOND){
        seconds++ ;
        microseconds -= MICROS_PER_SECOND ;
    }
    ts.tv_sec  = seconds;
    ts.tv_nsec = microseconds * NANO_PER_MICRO;
    while(nanosleep(&ts, &tr) == -1){
        if(errno != EINTR)
            break ;
        ts.tv_sec  = tr.tv_sec ;
        ts.tv_nsec = tr.tv_nsec ;
    }
}

/*
 * Call to get the value of the High Resolution Timer.
 * 
 * @returns             The High resolution timer value
 */
unsigned long long getHRTimer(void){
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (unsigned long long) (((double)tv.tv_sec * MICROS_PER_SECOND) + tv.tv_usec);
}

/*
 * Call to get a string representation for the input hr timer
 * 
 * @param hr            The input high res timer to print string for        
 * @param str           The input storage for the string output, must be
 *                      at least 26 chars wide to hold output value.
 * @returns             The High resolution timer value string
 */
char * getTimeString(unsigned long long hr, char *str) {
    time_t sec=hr/1000000;
    unsigned long usec=hr%1000000;
    struct tm s_tm;
    //fprintf(stdout, "sec is %lu usec is %lu hr is %llu\n", sec, usec,hr);
       //fflush(stdout);
    localtime_r(&sec, &s_tm);
    sprintf(str, "%04d-%02d-%02d %02d:%02d:%02d.%06lu", 
        s_tm.tm_year+1900, 
        s_tm.tm_mon,
        s_tm.tm_mday,
        s_tm.tm_hour,
        s_tm.tm_min,
        s_tm.tm_sec,
        usec);
    return str;
}




/*
 * Required setup needed to display SVT metric information,
 * Must be called early when the program first starts.
 */
void do_info_display_setup(void) {
    rateControl *r=g_rate;
    if(!r){ 
        /* user has not specified a rate to send/receive messages*/
       r=&g_rate_s; 
    }
    r->appStartTime=getHRTimer();
}

/*
 * Display SVT metric information
 */
void do_info_display(int flag) {
    char str1[256];
    char str2[256];
    char str3[256];
    char str4[256];
    rateControl *r=&g_rate_s;

    if(r->startTime == 0){
        r->startTime=getHRTimer();
    }
    r->currTime=getHRTimer();
    r->elapsed = (double)(r->currTime - r->startTime);
    if ((r->nmsgs != g_subscribe_count) && g_subscribe_count>0){
        r->nmsgs=g_subscribe_count;
    }else if ((r->nmsgs != g_publish_count) && g_publish_count>0){
        r->nmsgs=g_publish_count;
    }

    if(flag == MYBASIC){
        LOG_STDOUT("SVT_MQTT_C_STATUS,%f,%llu,%s,(actuals for rate,msgs,lastmsgtime)\n" ,
            (double)r->nmsgs/((double)(r->currTime-r->startTime)/MICROS_PER_SECOND),
            r->nmsgs,
            getTimeString(r->lastMsgTime,str3));
    } else if(flag == MYDETAILED){
        LOG_STDOUT("SVT_MQTT_C_INFO_START\n" \
            "actual_rate: %f (msgs/sec)\n" \
            "expected_rate: %d (msgs/sec) Note: if == 0 means no rate was specified\n" \
            "actual_msgs: %llu\n" \
            "expected_msgs: %d\n" \
            "app_start_time: %s\n" \
            "first_msg_time: %s\n" \
            "last_msg_sent_or_recv_time: %s\n" \
            "current_time: %s\n" \
            "elapsed_time: %f (sec)\n" \
            "pid: %d\n" \
            "appstate: %d\n"\
            "warncount: %d\n" \
            "errcount: %d\n" \
            "SVT_MQTT_C_INFO_END\n",
            (double)r->nmsgs/((double)(r->currTime-r->startTime)/MICROS_PER_SECOND),
            r->desiredMsgsPerSec, r->nmsgs,g_msg_number,
            getTimeString(r->appStartTime,str1),
            getTimeString(r->startTime,str2),
            getTimeString(r->lastMsgTime,str3),
            getTimeString(r->currTime,str4),
            (double)(r->elapsed /MICROS_PER_SECOND),
            getpid(), g_appstate, g_warning_count, g_error_count);
    } else {
        HALT_ERROR ("Internal code bug: Invalid input flag: %d\n", flag);
    }
}

void do_status_update(void){
    long long  currTime=getHRTimer();
    if ((currTime-g_lastStatTime)> g_lastStatTimeIntervalUSec){
        do_info_display(MYBASIC);
        g_lastStatTime=currTime;
    }
}

/*
 * The purpose of this function is to calculate the amount of
 * delay needed and to delay the program execution by that amount
 * to achieve a constant rate of messages. An informational message
 * is logged when rate control is achieved and/or lost.
 * 
 * @param rateControl   The structure holding all the values needed
 *                      to perform rate control
 * @returns             When the rate control has been achieved
 */
void do_rate_control(rateControl *r){
    
    if (r==NULL) {
        /* user does not want rate control, but we still save 
         * some info so that rate information can be later displaed */
        if(g_rate_s.startTime == 0){
            g_rate_s.startTime=getHRTimer();
        }
        g_rate_s.lastMsgTime = getHRTimer();
        return;
    } else if (r->startTime == 0){
        r->startTime= getHRTimer();
    }

    /* Increment message counter.  This counter represents the number of messages sent */
    r->nmsgs++;
    /* Message Send Rate Control is done here.  Sleep in the message send loop according to the
    projected time of the next message submission */
    r->lastMsgTime = getHRTimer();
    r->elapsed = (double)(r->lastMsgTime - r->startTime);
    r->projected = ((double) r->nmsgs / (double) r->desiredMsgsPerSec) * MICROS_PER_SECOND;
    if (r->elapsed < r->projected) {
        r->state=1; // sleeping
        r->nsleeps++;
        r->sleepInterval = r->projected - r->elapsed;
        perfSleep(0,(int) r->sleepInterval);       /* sleep for 0 seconds + sleepInterval microseconds */
    } else {
        r->state=0; // not sleeping
    }
        
}

/*
 * The error handling function. This function decides if a retry is
 * warranted or not for a failing mqtt client library operation.
 * 
 * @param mqtt_rc return code from mqtt client library
 * @param ln      line of code from caller
 * @param func    function of caller
 * @param msg     message from caller to be logged 
 * @returns       0 if the caller should retry the mqtt operation
 *                non-zero if the caller should halt.
 */
int do_handle_mqtt_client_errors(int mqtt_rc , unsigned int ln,
        const char * func, const char * msg) {
       int rc=0;
    long long lastWarnTime=0;
    int try_reconnect=0;
    //int check_log=0;

    if ((mqtt_rc== -1 || mqtt_rc== -2 || mqtt_rc== -3) &&
             (errno == 115 ||  errno == 11)) { // Operation now in progress  
        // This error happens:
        //  a) When the ismserver is not available
        //  b) When the ismserver is under very high load 
        // Retry it up to N times, because if it is happening while the ismserver
        // is not available we do not want to retry indefinitely.
        // Alternatively, a check for if the ismserver is pingable might be 
        // another way to verify which case the error is occurring for.
        if (mqtt_rc == -3){ // means that client was disconnected
            try_reconnect=1;
        }
        rc=0;
    } else if ((mqtt_rc== -1 || mqtt_rc== 99) && errno == 99) { // Cannot assign requested address
        // This recently started happening under high load with arrays of connections... retry?
        rc=0;
    } else if ((mqtt_rc== -1 || mqtt_rc== -2 || mqtt_rc== -3) && errno == 4) { // Interrupted system call
        // This error may happen if SIGUSR1 was sent to get debug/status info
        // retry it.
        //LOG_WARNING("It appears that debug is being performed, ignore error\n");
        rc=0;
    } else if (errno == 107) { // endpoint not connected
        // try to reconnect, with a zero return code a retry will occurr.
        try_reconnect=1;
        rc=0;
    } else {
        // fail operation, this is not a handled error.
        rc= __LINE__;
    }

    if (rc==0) {
        lastWarnTime=g_rate_s.warningTime;
        g_rate_s.warningTime=getHRTimer();
        /*only print out a warning if have not printed out warning in last 1 seconds*/
        if(g_rate_s.warningTime-lastWarnTime > 1){
            LOG_WARNING("Caller @line:[%d %s] "
                "should retry operation: %s mqtt rc:%d\n",
                ln,func,msg, mqtt_rc);
        } else {
            g_warning_count++;
        }
        if(try_reconnect==1){
            if( g_appstate != STATE_CONNECTING){
                // don't try to reconnect while already connecting
                rc=do_reconnect();
            }
        }
        sleep(0);
        return rc; // caller should retry operation if rc is still zero.
    } else {
        return rc; // caller should halt operation 
    }
 
}



/*
 * Print help menu/usage instructions
 * 
 * @param name  The name of the program that the help menu is printed for.
 */
void do_usage(char *name){
    printf("Usage: %s [-o <logfile>] -a <publish|subscribe> -s <uri> \n"\
            "         [-h] [-m <message>] [-n <number>] [-q 0|1|2] [-S <key>=<value]\n" \
            "         [-c <true|false>] [-i <client id>]\n" \
            "         [-u <user>] [-p <password>]\n" \
            "         [-t <topic>] [-r <directory>] [-v]\n", name);
    printf("    Parameters: (** denotes mandatory arguments ) \n");
    printf("        -a action      : **set the action (publish|subscribe)\n");
    printf("        -c true|false  : set cleansession to true or false\n");
    printf("        -h             : print help message\n"); 
    printf("        -i string      : set clientId to string (max 23 chars)\n");
    printf("        -m message     : set the msg. default: " \
           "\"%s\"\n",defaultMsg);
    printf("        -n number      : set the number of messages\n");
    printf("        -o logfile     : set the logfile for output \n");
    printf("        -p password    : set the password\n");
    printf("        -q 0,1,2       : set the Quality of Service (0,1,or 2)\n");
    printf("        -r directory   : enable persistence directory\n");
    printf("        -s uri         : **set the uri <tcp://ip:port>\n");
    printf("        -S <key>=<val> : Setup SSL. Set a valid key = <val>.\n");
    printf("         valid keys are: keyStore|trustStore|privateKey|privateKeyPassword|\n");
    printf("                       : enabledCipherSuites|enableServerCertAuth\n");
if(g_trace_level>0){
    printf("           keyStore    :  \n                       : ");
    printf(" The file in PEM format containing the public certificate chain of the client. " \
           "It may also include the client's private key.\n");
    printf("          trustStore   : \n                       : ");
    printf("The file in PEM format containing the public digital certificates trusted by the client.\n");
    printf("          privateKey   : \n                       : ");
    printf("If not included in the sslKeyStore, this setting points to the file in " \
           "PEM format containing the client's private key.\n");
    printf("    privateKeyPassword : \n                       : ");
    printf(" The password to load the client's privateKey if encrypted. \n");
    printf("  enabledCipherSuites  : \n                       : ");
    printf("The list of cipher suites that the client will present to the server during the SSL handshake. For a \n" \
    "                       : full explanation of the cipher list format, please see the OpenSSL on-line documentation:\n" \
    "                       : http://www.openssl.org/docs/apps/ciphers.html#CIPHER_LIST_FORMAT\n" \
    "                       : If this setting is ommitted, its default value will be ALL, that is, all the cipher suites -excluding\n" \
    "                       : those offering no encryption- will be considered.\n" \
    "                       : This setting can be used to set an SSL anonymous connection (aNULL string value, for instance).\n" );
    printf("  enableServerCertAuth : \n                       : ");
    printf(" 1/0 option to enable/disable verification of the server certificate \n");
}
    printf("        -t topic       : set the topic name. Default =  " \
           "MQTTV3Sample/C\n");
    printf("        -u user        : set the user\n");
    printf("        -v             : set to verbose mode\n");
    printf("        -w number      : set the constant rate messages / sec\n");
    exit(0);
}

/*
 * Parse user inputs  
 *
 * @param argc  The argument count
 * @param argv  The argument vector
 */
void do_parse_arguments(int argc , char **argv){
    int c,i,j;

    while ((c=getopt(argc,argv,"a:c:hi:m:n:o:p:q:r:s:S:t:u:vw:")) != -1) {
        switch (c) {
            case 'a':
                strncpy(g_action, optarg, MAX_BUF_SZ);
                LOG_INFO( "g_action is %s\n", g_action);
                if (g_topic[0] == '\0' ){
                    // Set the default topic according to the specified action
                    if (strcmp(g_action, PUBLISH) == 0){
                        strcpy(g_topic, "/MQTTV3Sample/C");
                        if(g_msg==NULL) {
                           g_msg=defaultMsg;
                        }
                    } else {
                        strcpy(g_topic, "/MQTTV3Sample/C");
                    }
                }
                break;
            case 'c':
                if (strcasecmp(optarg, "true")==0){
                    g_cleansession=1;
                } else if (strcasecmp(optarg, "false")==0){
                    g_cleansession=0;
                } else {
                    HALT_ERROR ("Invalid -c flag argument: %s\n", optarg); 
                }
                break;
            case 'h':
                do_usage(argv[0]);
                break;
            case 'i':
                memset(g_clientId, 0, MAX_BUF_SZ);
                strncpy(g_clientId, optarg, CLIENTID_MAX_LEN);
                break;
            case 'm':
                g_msg_buf=(char*)malloc(strlen(optarg)+1);
                strcpy(g_msg_buf,optarg);
                g_msg=g_msg_buf;
                break;
            case 'n':
                g_msg_number=strtoul(optarg,0,10);
                LOG_INFO( "g_msg_number is %d\n", g_msg_number);
                break;
            case 'o':
                g_log2out=fopen(optarg, "w+");
                if (!g_log2out) {
                      HALT_ERROR ("Unable to open output log file %s\n",optarg);
                }
                g_log2err=g_log2out;
                break;
            case 'p':
                strncpy(g_password, optarg, MAX_BUF_SZ);
                break;
            case 'q':
                g_qos=strtoul(optarg,0,10);
                if (g_qos>2 || g_qos<0){ 
                    HALT_ERROR ("Invalid Qos %d\n", g_qos); 
                }
                break;
            case 'r':
                strncpy(g_persistence_context_buf, optarg, MAX_BUF_SZ);
                g_persistence_context=g_persistence_context_buf;
                g_persistence_type=MQTTCLIENT_PERSISTENCE_DEFAULT;
                g_retained_flag=1;
                break;
            case 's':
                strncpy(g_uri, optarg, MAX_BUF_SZ);
                break;
            case 'S':
                LOG_INFO("OPTARG for -S is %s\n", optarg);
                if(strncmp(       optarg, "keyStore=", 9) == 0){
                    j=strlen(optarg+9)+1;
                    g_keyStore=(char*)malloc(j);
                    strncpy(g_keyStore, optarg+9, j);
                } else if(strncmp(optarg, "trustStore=", 11) == 0){
                    j=strlen(optarg+11)+1;
                    g_trustStore=(char*)malloc(j);
                    LOG_INFO("Allocating %d bytes \n",j);
                    strncpy(g_trustStore, optarg+11,j );
                    LOG_INFO("g_trustStore is %s \n",g_trustStore);
                } else if(strncmp(optarg, "privateKey=", 11) == 0){
                    j=strlen(optarg+11)+1;
                    g_privateKey=(char*)malloc(strlen(optarg+11)+1);
                    strncpy(g_privateKey, optarg+11, j);
                } else if(strncmp(optarg, "privateKeyPassword=", 19) == 0){
                    j=strlen(optarg+19)+1;
                    g_privateKeyPassword=(char*)malloc(j);
                    strncpy(g_privateKeyPassword, optarg+19, j);
                } else if(strncmp(optarg, "enabledCipherSuites=", 20) == 0){
                    j=strlen(optarg+20)+1;
                    g_enabledCipherSuites=(char*)malloc(j);
                    strncpy(g_enabledCipherSuites, optarg+20, j);
                } else if(strncmp(optarg, "enableServerCertAuth=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j < 0 || j>1){
                        HALT_ERROR ("Invalid enableServerCertAuth= value specified: %d\n", j);
                    }
                    g_enableServerCertAuth=j;
                }
                break;
            case 't':
                strncpy(g_topic, optarg, MAX_BUF_SZ);
                break;
            case 'u':
                strncpy(g_user, optarg, MAX_BUF_SZ);
                break;
            case 'v':
                g_trace_level=10;
                break;
            case 'w':
                j=strtoul(optarg,0,10);
                if (j == 0){
                    LOG_INFO("rate specified as zero - this means max speed. no change made\n");      
                } else if(j<1){
                    HALT_ERROR ("Invalid msg per second (<1) specified: %d.\n", j);
                } else {
                    g_rate=&g_rate_s;
                    g_rate->desiredMsgsPerSec=strtoul(optarg,0,10);
                }
                break;
            default:
                LOG_ERROR("Invalid Parameter %c. Check help menu with -h\n",
                        optopt);
                exit(1);    
        }
    }

    for (i=optind; i< argc; i++) {
          HALT_ERROR ("Unexpected non-option argument number %d \"%s\".\n"\
                "Check help menu with -h\n", i, argv[i]);
    }
      
    if (g_uri[0]=='\0' ){
         HALT_ERROR ("Missing required parameter: -s\n");
    }

    if ( g_action[0]=='\0' ){
         HALT_ERROR ("Missing required parameter: -a\n");
    }
}
