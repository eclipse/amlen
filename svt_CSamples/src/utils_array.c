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
 * @file utils_array.c
 *
 * The purpose of this file is to implment helper and utility
 * functions that are necessary to implement the sample.
 * 
 */

#include <mqttsample_array.h>

/*
 * A Signal handling function
 * 
 * @param sig   The signal number that the handler is being called for.
 */
void do_signal_handle(int sig){

    if (sig==SIGUSR1){
        // the following is not POSIX compliant, for debug use only.
        // it is possible that unexpected results may occurr.
        //do_info_display();
        av.info=1;
        av.trace_level++;
        av.appstate=STATE_CONTINUE;
    } else if (sig==SIGUSR2){
        // didn't work - looks like the variable is only read during intialization setenv("MQTT_C_CLIENT_TRACE", "ON", 1);
        av.appstate=STATE_CONTINUE;
    } else if (sig==SIGINT){
        av.halt++;
        if (av.halt>=2){
            HALT_ERROR("Exit due to SIGINT. av.halt=%d\n", av.halt);
        }
    } else {
        av.halt++;
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
        s_tm.tm_mon+1,
        s_tm.tm_mday,
        s_tm.tm_hour,
        s_tm.tm_min,
        s_tm.tm_sec,
        usec);
    return str;
}


char * get_next_ltpa_token (clientVariables *tv){
    static FILE *fp=NULL;
    char line[1024]="";
    char *ptr=NULL;
    
    if (fp == NULL) {
       if ( av.userLtpaFile[0]!='\0' ) {
            fp=fopen(av.userLtpaFile, "r");
            if (!fp){
                perror("ERROR: Could not open file for reading\n");
                HALT_ERROR ("Invalid input for av.userLtpaFile: %s\n", av.userLtpaFile);
            }
       } 
    }
    
    if (fp) {
        if (fgets(line,1024,fp) != NULL){
            if ( line[strlen(line)-1] == '\n' ) {
                line[strlen(line)-1] = '\0' ; // remove trailing newline, should not be any other spaces
            }
            ptr=strstr(line, " "); // go to first space, and then ..
            if (ptr==NULL){
                HALT_ERROR ("Formatting error on line \"%s\" read from input file av.userLtpaFile\n", line);
            }
            ptr++;                  // one char after should be ltpa token
        } else {
            HALT_ERROR ("Internal code bug fgets returned NULL, this could also mean too many users were requested from input file\n");
        }
    } else { 
        HALT_ERROR ("Internal code bug fp is NULL\n");
    }        
    
    tv->ltpaToken=(char*)malloc(strlen(ptr)+1);;
    strcpy(tv->ltpaToken, ptr);
    return tv->ltpaToken;    
}


/*
 * Required setup needed to display SVT metric information,
 * Must be called early when the program first starts.
 */
void do_info_display_setup(clientVariables *tv) {
    rateControl *r=NULL;
    if (tv) {
        r=&tv->rate_s;
        if (av.tv->rate_s.appStartTime==0){
            av.tv->rate_s.appStartTime=getHRTimer();
        }
        r->appStartTime=av.tv->rate_s.appStartTime;
        /* client start time is recorded after first message is sent*/
    } //else { 
        //r=&av.rate_s;
        //r->appStartTime=getHRTimer();
        //r->startTime=r->appStartTime;
    // }   
}

/*
 * Display SVT metric information
 */
void do_info_display(clientVariables *tv, int flag, int multiplier) {
    char str1[256];
    char str2[256];
    char str3[256];
    char str4[256];

    rateControl *r=&tv->rate_s;

    if(r->firstMsgTime == 0){
        r->firstMsgTime=getHRTimer();
    }
    r->currTime=getHRTimer();
    r->elapsed = (double)(r->lastMsgTime - r->firstMsgTime);
    //if (r->elapsed <= 0.0){
     //   r->elapsed = 0.000001;
    //}

    if(flag == MYBASIC){
        LOG_STDOUT("SVT_MQTT_C_STATUS,%f,%llu,%s,(actuals for rate,msgs,lastmsgtime)\n" ,
            (double)r->nmsgs/((double)(r->elapsed)/MICROS_PER_SECOND),
            r->nmsgs,
            getTimeString(r->lastMsgTime,str3));
    } else if(flag == MYDETAILED){
        LOG_STDOUT("SVT_MQTT_C_INFO_START\n" \
            "expected_rate: %d (msgs/sec) Note: if == 0 means no rate was specified\n"
            "actual_rate: %f (msgs/sec) Note: rate calculated from first_msg_time to last_msg_time\n" \
            "actual_msgs: %llu ( %.1f %% of actual)\n" \
            "expected_msgs: %llu\n" \
            "app_start_time: %s\n" \
            "first_msg_time: %s\n" \
            "last_msg_sent_or_recv_time: %s\n" \
            "current_time: %s\n" \
            "messaging_elapsed_time: %f (sec) Note: from first_msg_time to last_msg_time\n" \
            "total_elapsed_time: %f (sec) Note: from app_start_time to current_time\n" \
            "pid: %d\n" \
            "appstate: %d\n"\
            "warncount: %llu\n" \
            "errcount: %llu\n" \
            "SVT_MQTT_C_INFO_END\n",
            r->desiredMsgsPerSec,
            (double)(r->nmsgs)/((double)(r->elapsed)/MICROS_PER_SECOND),
            r->nmsgs, 
            ((double)(r->nmsgs)/((double)av.msg_number*multiplier))*100.0,
            av.msg_number*multiplier,
            getTimeString(r->appStartTime,str1),
            getTimeString(r->firstMsgTime,str2),
            getTimeString(r->lastMsgTime,str3),
            getTimeString(r->currTime,str4),
            (double)((double)(r->lastMsgTime - r->firstMsgTime)/MICROS_PER_SECOND),
            (double)((double)(r->currTime - r->appStartTime)/MICROS_PER_SECOND),
            getpid(), av.appstate, av.warning_count, av.error_count);
    } else {
        HALT_ERROR ("Internal code bug: Invalid input flag: %d\n", flag);
    }

}

void do_rate_control(clientVariables *tv, int curIndex){
    do_rate_control_v2(tv,curIndex);
}

/*
 * The purpose of this function is to calculate the amount of
 * delay needed and to delay the program execution by that amount
 * to achieve a constant rate of messages. An informational message
 * is logged when rate control is achieved and/or lost.
 * 
 * The reason for _v2 is to use the application wide av.tv->rate_s for
 * actual rate control which means that rate control can be applied to 
 * arrays of clients as well.
 * 
 * @param rateControl   The structure holding all the values needed
 *                      to perform rate control
 * @returns             When the rate control has been achieved
 */
void do_rate_control_v2(clientVariables *tv, int curIndex){
    rateControl *r=tv->rate;
    int doSleep=0;
    //double curRate=0;
    unsigned long long curTime=getHRTimer();

    if(curIndex==(av.tv_array[0]->total_clients-1)){
        //LOG_INFO("doSleep set to 1 for completed cycle %d\n", curIndex);
        doSleep=1; // possible to sleep after all clients have had a chance to send/recv
    }


    if (av.desiredMsgsPerSec!=0 && r==NULL){
        LOG_STDOUT("There has been a dynamic update to the rate control. Previously there was none.\n");
        tv->rate=&tv->rate_s;
        tv->rate->desiredMsgsPerSec=av.desiredMsgsPerSec;
    }

    if (r==NULL || av.desiredMsgsPerSec==0 ) {
        /* user does not want rate control, but we still save 
         * some info so that rate information can be later displaed */
        if(tv->rate_s.firstMsgTime == 0){
            tv->rate_s.firstMsgTime=curTime;
        }
        if(av.tv->rate->firstMsgTime== 0){
            av.tv->rate->firstMsgTime=tv->rate_s.firstMsgTime;
        }
        tv->rate_s.lastMsgTime=curTime;
        av.tv->rate->lastMsgTime=tv->rate_s.lastMsgTime;
        tv->rate_s.nmsgs++;
        av.tv->rate_s.nmsgs++;
        if (av.desiredMsgBurst > 0) {
            // ------------------------------------------
            // Even if doSleep is not set , if a message burst has completed,
            // as specified by the -x burst=xx flag, then a sleep will occurr.
            // regardless of other settings , as long as a rate < 1.0 was provided.
            // ------------------------------------------
            if ((av.tv->rate_s.nmsgs%av.desiredMsgBurst) == 0){
                LOG_INFO ("Current message rate is %f\n", (double)(av.tv->rate_s.nmsgs-av.lastStatMsgs)/((double)(curTime-av.lastStatTime)/MICROS_PER_SECOND));
                do_safe_mqtt_client_sleep(tv, av.desiredWaitTime) ;
            }
        } else if (av.desiredWaitTime > 0 && (doSleep == 1)){
            LOG_INFO (".a");
            do_safe_mqtt_client_sleep(tv, av.desiredWaitTime) ;
            LOG_INFO (".b");
        }
        //LOG_INFO("r is NULL, incremented av.tv->rate_s.nmsgs to %llu\n", av.tv->rate_s.nmsgs);
        return;
    } else if (r->firstMsgTime == 0){
        r->firstMsgTime=curTime;
        r->currentfirstMsgTime=curTime;
        if(av.tv->rate->firstMsgTime== 0){
            av.tv->rate->firstMsgTime=tv->rate_s.firstMsgTime;
        }
        av.tv->rate_s.currentfirstMsgTime=curTime;
        av.tv->rate_s.currentnmsgs=0;
        av.tv->rate_s.currentlastMsgTime=0;
    } 

    r=&(av.tv->rate_s);

    if (r->desiredMsgsPerSec!=av.desiredMsgsPerSec){
        LOG_STDOUT("It seems the rate has changed. Changing from old rate %d to new rate %d\n", 
            r->desiredMsgsPerSec, av.desiredMsgsPerSec );
        r->desiredMsgsPerSec=av.desiredMsgsPerSec;
        LOG_STDOUT("now r->desiredMsgsPerSec is %d\n", r->desiredMsgsPerSec);
        // TODO: not sure what happens if r->desiredMsgsPerSec=0 here. 

    }

    /* Increment message counters.  This counter represents the number of messages sent */
    r->nmsgs++;
    r->currentnmsgs++;
    tv->rate_s.nmsgs++;

    r->lastMsgTime = curTime;
    r->currentlastMsgTime = curTime;
    tv->rate->lastMsgTime=tv->rate_s.lastMsgTime;

    if ((r->lastSampleTime<=0) ){ 
        r->lastSampleTime=curTime;
        r->lastSampleMsgs=r->nmsgs;
       // LOG_STDOUT("A %lld init lastSampleMsgs= %lld from r->nmsgs %lld\n", r->lastSampleTime, r->lastSampleMsgs, r->nmsgs);
        
    } 
    //if ((r->lastSampleTime<=0) ||
    //   ((r->lastSampleTime-curTime)>MICROS_PER_SECOND)) {
    if ((curTime-r->lastSampleTime)>MICROS_PER_SECOND) {
        r->lastSampleTime=curTime;
        r->lastSampleMsgs=r->nmsgs;
        //LOG_STDOUT("B %lld init lastSampleMsgs= %lld from r->nmsgs %lld\n", r->lastSampleTime, r->lastSampleMsgs, r->nmsgs);
        
    } 
    if ((r->nmsgs-r->lastSampleMsgs) >=  (r->desiredMsgsPerSec-1)) { 
        r->sleepInterval = (MICROS_PER_SECOND - (curTime-r->lastSampleTime));
        if (r->sleepInterval <0 ){
             HALT_ERROR ("Internal code bug with rate calculation :%f\n", r->sleepInterval )
        }
        if (r->sleepInterval >MICROS_PER_SECOND ){
             HALT_ERROR ("Internal code bug with rate calculation :%f\n", r->sleepInterval )
        }
        LOG_INFO("sleeping usleep(r->sleepInterval); %f, r->desiredMsgsPerSec:%d\n", r->sleepInterval, r->desiredMsgsPerSec);
        r->state=1; // sleeping
        r->nsleeps++;
        usleep(r->sleepInterval); 
        return;
    } else {
        //LOG_STDOUT("r->nmsgs-r->lastSampleMsgs %lld is <  r->desiredMsgsPerSec %d\n", (r->nmsgs-r->lastSampleMsgs) ,  r->desiredMsgsPerSec);
        
    }

#if 0
    /* Message Send Rate Control is done here.  Sleep in the message send loop according to the
    projected time of the next message submission */
    r->lastMsgTime = curTime;
    r->currentlastMsgTime = curTime;
    tv->rate->lastMsgTime=tv->rate_s.lastMsgTime;
    r->elapsed = (double)(r->currentlastMsgTime - r->currentfirstMsgTime);
    r->projected = ((double) r->currentnmsgs / (double) r->desiredMsgsPerSec) * MICROS_PER_SECOND;
    if (r->elapsed < r->projected) {
        r->state=1; // sleeping
        r->nsleeps++;
        r->sleepInterval = r->projected - r->elapsed;
        if (doSleep){
            perfSleep(0,(int) r->sleepInterval);       /* sleep for 0 seconds + sleepInterval microseconds */
            LOG_INFO("slept %d\n", (int) r->sleepInterval);
        }
    } else {
        r->state=0; // not sleeping
    }
#endif
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
int do_handle_mqtt_client_errors(clientVariables *tv,
        int mqtt_rc , unsigned int ln,
        const char * func, const char * msg) {
    int rc=0;
//    long long lastWarnTime=0;
    int try_reconnect=0;
    //int check_log=0;
            

    if (mqtt_rc== 911 ){  // this isnt really an mqtt rc, but to mean it means i need to reconnect.
        try_reconnect=1;
    } else if (mqtt_rc== -1 || mqtt_rc== -2 || mqtt_rc== -3 ){
             //(errno == 115 ||  errno == 11)) { // Operation now in progress  
            // (errno == 104) seen while killing server while publishing qos2
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
    } else if (mqtt_rc==-22) { // For more info on -22 see defect 24734 
        rc=0; // retry
    } else if (mqtt_rc==-103) { // What is this? 3.8.13  testing 20899
        LOG_INFO("Received rc 103: what is this ? " );
        rc=0; // retry
    } else if (mqtt_rc==1) { //1: Connection refused: Unacceptable protocol version
        LOG_ERROR("Received rc 1: Connection refused: Unacceptable protocol version" );
        rc=1; // it seems like it makes sense to stop processing for this error
    } else if (mqtt_rc==2) {  
        LOG_ERROR("Received rc 2: Connection refused: Identifier rejected " );
        rc=1; // it seems like it makes sense to stop processing for this error
    } else if (mqtt_rc==3) {  
        LOG_INFO("Received rc 3: Connection refused: Server unavailable");
        try_reconnect=1;
        rc=0;
    } else if (mqtt_rc==4) {  
        LOG_ERROR("Received rc 4: Bad user name or password " );
        rc=1; // it seems like it makes sense to stop processing for this error
    } else if (mqtt_rc==5) {  
        LOG_ERROR("Received rc 5: Not authorized " );
        rc=1; // it seems like it makes sense to stop processing for this error
    } else if (mqtt_rc==3 && errno == 115) { // This can happen with socket errors
        // ha bringup see this when connecting to backup /standby appliance 
        rc=0;
        
    } else if (mqtt_rc==-1 && errno == 0) { // This can happen with socket errors
         // recently started happening after implementing SSL
         //    SSLSocket error error:FFFFFFFFFFFFFFFF:lib(255):func(4095):reason(4095)(5) in SSL_connect for socket 4
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
    } else if (av.noError==1) { // try not to have any errors.
        rc=0;
    } else {
        // fail operation, this is not a handled error.
        rc= __LINE__;
    }

    if( tv->clientstate == STATE_DISCONNECTED){
        try_reconnect=1;
    }

    if (av.warningWait >0 ){
        do_safe_mqtt_client_sleep(tv, av.warningWait);
    }

    if (rc==0) {
//        lastWarnTime=tv->rate_s.warningTime;
        tv->rate_s.warningTime=getHRTimer();
        /*only print out a warning if have not printed out warning in last 1 seconds*/
        //if(tv->rate_s.warningTime-lastWarnTime > 1){
            CLIENT_LOG_WARNING(tv,"Caller @line:[%d %s] "
                "should retry operation: %s mqtt rc:%d\n",
                ln,func,msg, mqtt_rc);
        //} else {
            //tv->warning_count++;
        //}
        if(try_reconnect==1){
            if( tv->clientstate != STATE_CONNECTING &&
                tv->clientstate != STATE_DISCONNECTING){
                // don't try to reconnect while already connecting
                do_reconnect(tv); 
                if (tv->clientstate == STATE_FAILED_CONNECTION){
                    return __LINE__;
                } 
            }
        }
        return rc; // caller should retry operation if rc is still zero.
    } else {
        CLIENT_LOG_WARNING(tv,"Caller @line:[%d %s] "
                "should halt operation: %s mqtt rc:%d\n",
                ln,func,msg, mqtt_rc);
        return rc; // caller should halt operation 
    }
}



/*
 * Print help menu/usage instructions
 * 
 * @param name  The name of the program that the help menu is printed for.
 */
void do_usage(char *name){
    printf("Usage: %s [-o <logfile>] [-v] -a <publish|publish_wait|subscribe|receive_wait> -s <uri> \n"\
            "         [-h] [-m <message>] [-n <number>] [-q 0|1|2] [-S <key>=<value>] \n" \
            "         [-c <true|false>] [-i <client id>]\n" \
            "         [-u <user>] [-p <password>]\n" \
            "         [-t <topic>] [-r <directory>]\n" \
            "         [-w <rate>] [-z <clients>]\n", name);
    printf("    Parameters: (** denotes mandatory arguments ) \n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -a action      : **set the action (publish|publish_wait|subscribe|receive_wait)\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -c true|false  : set cleansession to true or false\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -h             : print help message\n"); 
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -i string      : set clientId to string (max 23 chars)\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -m message     : set the msg. default: " \
           "\"%s\"\n",av.defaultMsg);
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -n number      : set the number of messages\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -o logfile     : set the logfile for output \n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -p password    : set the password\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -q 0,1,2       : set the Quality of Service (0,1,or 2)\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -r directory   : enable persistence directory\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -s uri         : **set the uri <tcp://ip:port>\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -S <key>=<val> : Setup SSL. Set a valid key = <val>.\n");
    printf("         valid keys are: keyStore|trustStore|privateKey|privateKeyPassword|\n");
    printf("                       : enabledCipherSuites|enableServerCertAuth\n");
if(av.trace_level>0){
    printf("              keyStore : The file in PEM format containing the public certificate chain of the client. \n" \
           "                       : It may also include the client's private key.\n");
    printf("            trustStore : The file in PEM format containing the public digital certificates trusted by the client.\n");
    printf("            privateKey : If not included in the sslKeyStore, this setting points to the file in\n " \
           "                       : PEM format containing the client's private key.\n");
    printf("    privateKeyPassword : The password to load the client's privateKey if encrypted. \n");
    printf("   enabledCipherSuites : The list of cipher suites that the client will present to the server during the SSL handshake. For a \n" \
           "                       : full explanation of the cipher list format, please see the OpenSSL on-line documentation:\n" \
           "                       : http://www.openssl.org/docs/apps/ciphers.html#CIPHER_LIST_FORMAT\n" \
           "                       : If this setting is ommitted, its default value will be ALL, that is, all the cipher suites -excluding\n" \
           "                       : those offering no encryption- will be considered.\n" \
           "                       : This setting can be used to set an SSL anonymous connection (aNULL string value, for instance).\n" );
    printf("  enableServerCertAuth : 1/0 option to enable/disable verification of the server certificate \n");
}
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -t topic       : set the topic name. Default =  MQTTV3Sample/C\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -u user        : set the user\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -v             : set to verbose mode\n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -w number      : set the constant rate messages / sec > 0.0. Note setting == 0 means max speed \n");
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -x <key>=<val> : Set a key, value property. \n");
    printf("         valid keys are: scaleTest|orderMsg|verifyStillActive|sendOutOfOrder|86WaitForCompletion\n");
    printf("                       : noDisconnect|noError|topicChangeTest|msgFile|testCriteriaPctMsg ...\n");
if(av.trace_level>0){
    printf("  statusUpdateInterval : set = >1 seconds that you want to see status updates:   \n" \
           "                       :    note: Format of status updates is a line that looks like the following:\n" \
           "                       :    2013-00-15 19:34:22.796917 SVT_MQTT_C_STATUS, actual msg rate,msgs since last interval,total msgs,lastmsgtime\n" \
           "                 burst : optional, if set in conjunction with a -w V , where value is < 1.0, a burst of messages \n" \
           "                       : will be sent by doing a modulo with this number and the number of messages sent before the\n" \
           "                       : desired wait time is slept on.\n" \
           "       checkConnection : Set = X of what number of receiveTimeoutExpired count to check if the client is connected \n" \
           "     dynamicClientCert : Set = <path> to cert files and keys . Must also specify dccIncrementer\n" \
           "        dccIncrementer : Set >= 0 for the starting number where CNxxxxxx will be calculated from\n" \
           "           connectRate : Set >= 0.0 seconds to throttle the speed that connections are made\n" \
           "dynamicReceiveTimeoutv2: Set to cause the receiveTimeout to dynamically increase based on \n" \
           "                       : how much processor time the program is getting currently. \n" \
           " dynamicReceiveTimeout : Set to cause the receiveTimeout to dynamically increase based on the number \n" \
           "                       : of times that a timeout has occured (increases 10 ms for each timeout)\n" \
           "                       : This will only occurr after the first successful receive has transpired\n" \
           "                       : TODO: Currently it only goes up, it would be good if it went up and down\n" \
           "   incrPubRateInterval : DEPRECATED\n" \
           "      incrPubRateCount : DEPRECATED\n" \
           "           incrPubRate : DEPRECATED\n" \
           "       incrPubFeedback : DEPRECATED\n" \
           "      injectDisconnect : Set = x (where x >=0 to give a injectDisconnect in injectDisconnectX chance of a random disconnect ocurring after receive\n" \
           "     injectDisconnectX : Set >=1 to give a injectDisconnect in injectDisconnectX chance of error injection occurring\n" \
           "   injectDisconnectMax : Set >=1 to give max number of injectDisconnect error injections \n" \
           "                 haURI : optional, set the high availability uri <tcp://ip:port> or <ssl://ip:port> \n" \
           "          retryConnect : set >=0, the number of times to retry the connect operation. Default is 10.\n" \
           "      sharedMemoryType : set = 0 (default) to enable shm write based on msg counters. \n" \
           "                       : set = 1 (default) to enable shm write based on msg order numbers (required orderMsg must also be set). \n" \
           "    sharedSubscription : set = X. Where X is a string. When set the string $SharedSubscription/X/ will be prepended to the user supplied topic\n" \
           "        sharedSubMulti : set N >=1 . Must use in conjunction with sharedSubscription. When specfified, \n" \
           "                       : This number of unique (1-N) sharedSubscriptions $SharedSubscription/X.N will be used . \n" \
           "  sharedMemoryCoupling : set = 1 or 0(default) to enable or disable publisher/subscriber coupling. If enabled the publisher will block until it syncs up with the subscriber. If disabled, the publisher will continue to publish after each 1 msec timeout. \n" \
           "       sharedMemoryKey : set =<string>, if you want the current process to use a shared memory region to synchronize publishers and subscribers\n" \
           "                       : (use in conjunction with the sharedMemoryVal option) \n" \
           "                       : IMPORTANT: Each unique publisher must have its own unique sharedMemoryKey, subscribers can \n" \
           "                       : IMPORTANT: share the same key but each publisher must have its own unique key to prevent collisions when writing..\n" \
           "       sharedMemoryVal : set >=1 (default), to specify how out of sync the publisher and subscriber can be. \n" \
           "                       : For example: if you set it to 10 that means that at any given time the publisher will \n " \
           "                       : not publish more than 10 messages than the subscriber has already received.\n " \
           "                       : This option can be used to push publisher/subscriber pairs to their maximum achievable combined throughput\n" \
           "       useCNasClientID : set == 1 if you want the CommonName used as the clientId (must also use dccIncrementer for accurate setting\n" \
           "  useWrongCNasClientID : set >= 1 if you want the CommonName used as the clientId to be wrong, i.e. incremented by this value.\n" \
           "             scaleTest : set = 1, If you want the client to go into \"scaleTest\" mode.\n" \
           "                       : 1. clientId will be used as the topic, and will be appended to the default topic, or user specified topic.\n" \
           "                       :    e.g if user input is -t /APP/1/CAR/ , then the final topic will be APP/1/CAR/<clientId>.\n" \
           "                       :    e.g if user does not input topic, then the final topic will be /MQTTV3Sample/C<clientId>.\n" \
           "                       : 2. clientId must contain hostname_pid format\n" \
           "                       :    e.g pClientId = prefix + host + _ + pid + _ + tid \n" \
           "                       : 3. every minute, client will publish to topic /svt/clientId with format: \n" \
           "                       :     clientId:num_topics:count:delta_ms:rate_sec:failedCount\n" \
           "                       :     num_topics: required - set to number of clients/topics\n" \
           "                       :     rate_sec: cummulative rate messages per second sent\n" \
           "                       :     Note: all other formatting are optional (may be set to 0 ) \n" \
           "                       : Note:This mode is intended to be used with the svt monitoring application using the -M parameter.  \n");
    printf("                       : eg: java svt.mqtt.mq.MqttSample -a subscribe -t \"#\"  -s tcp://${ismserver}:16102 -n 100000000 -v -M\n");
    printf("    topicMultiTestIncr : set >= 0 , if you want the counter for topicMultiTestto start at a certain offset\n");
    printf("        topicMultiTest : set = x, If you want the publisher client to publish to topic av.topic/0 - av.topic/x-1 for each publish pass\n");
    printf("                       : Note:This mode takes precedent over topicChangeTest if specified together\n");
    printf("                       : Note:This mode operates differently if the user also specifies prependCN2Topic . In that case it will publish to topic /CNxxxxx/av.topic - /CNyyyyyy/av.topic , where xxx and yyy are calculated from av.dccIncrementer . Thus it is important to also specify dccIncrementer for proper behavior. See Example 8 for publish/subscribe example\n");
    printf("       prependCN2Topic : set = 1, if for Client Certificate only, you want the CommonName from client cert prepended to supplied topic (make sure to have already specified -t parameter.)  \n");
    printf("                       : Note: /xxxxxxx/ will be prependeed to the topic where xxxxxx is the common name from the client cert.  \n");
    printf("                       : Note: For subscribe this operation is enabled only when also using the dccIncrementer.  The logic being that if you are not using dccIncrementer, then you should just specify the topic directly\n");
    printf("                       : Note: For publish this operation is enabled only when also using the topicMultiTest.\n");

    printf("       topicChangeTest : set = 1, If you want the client to send (publish) messages on a different numbered topic \n");
    printf("                       : which is appended to the user input topic for each message published. (does not apply to subscribe\n"); 
    printf("                       : Note:This mode can be overriden by topicMultiTest if specified together at same time.\n");
    printf("     sendOutOfOrderMsg : set = 1, If you want the client to send (publish) messages > 10 out of order\n");
    printf("               noError : set = 1, If you want the client to try to the best of its ability to not have any errors.\n");
    printf("     keepAliveInterval : set = >= 2 If you want to customize the keepAliveInterval Seconds. From MQTT Client Doc:\n");
    printf("                       : The \"keep alive\" interval, measured in seconds, defines the maximum time that should pass without communication between the client and the server. The client will ensure that at least one message travels across the network within each keep alive period. In the absence of a data-related message during the time period, the client sends a very small MQTT \"ping\" message, which the server will acknowledge. The keep alive interval enables the client to detect when the server is no longer available without having to wait for the long TCP/IP timeout. Also for this client the minimum allowed keep alive interval is 2 due to the needs of function do_safe_mqtt_client_sleep using a 1 second interval as a \"safe\" time.\n");
    printf("                       : IBM Messaging Appliance Note: in imaserver-connection.log, you may see conections closed with Reason=\"The connection has timed out\" when the keep alive timeout is too low for the current workload - recommendation try a larger timeout.\n");
    printf("          noDisconnect : set = 1, If you want the client to not issue a disconnect (even when reconnecting)\n");
    printf("             sleepLoop : set = X >=0  if you want to do a sleep(X) during recv/send loops\n");
    printf("            usleepLoop : set = X >=0  if you want to do a usleep(X) during recv/send loops\n");
    printf("    msgTimeoutAfterSub : set = >0 for uSeconds to wait for at least one message after subscribing before disconnecting\n");
    printf("                       : WARNING: this is not designed for an array of clients, just use on a single client\n");
    printf("                       : WARNING: (do not use -z parm for this one)\n");
    printf("        connectTimeout : set = >0 for seconds for connectTimeout\n");
    printf("                       : This can be helpful for when doing Qos=2 sessions with clean session set to false and you want the \n");
    printf("                       : state of the session to not get disconnected. eg: 1. subscribe a clean=false client with no disconnect\n");
    printf("                       : 2. publish 20 qos2 messages, 3. subscribe a clean=false client with no disconnect expecting 10 messages,\n");
    printf("                       : 4. repeat step 3.\n");
    printf("              reliable : set = 0 (default) / 1 to set the reliable flag to true/false. From MQTT Client doc: \n" );
    printf("                       : This is a boolean value that controls how many messages can be in-flight simultaneously. Setting reliable to true means that a published message must be completed (acknowledgements received) before another can be sent. Attempts to publish additional messages receive an MQTTCLIENT_MAX_MESSAGES_INFLIGHT return code. Setting this flag to false allows up to 10 messages to be in-flight. This can increase overall throughput in some circumstances.\n");
    printf("   86WaitForCompletion : set = 1, If you do not want the publisher to do waitForCompletion on qos 1,2 messages\n");
    printf("               msgFile : set = filename, if you want the client to use the content of a file as the message\n");
    printf("         reconnectWait : set = X, Where X=seconds to wait before attempting reconnections (Default 0).\n" );
    printf("      waitAfterReceive : set = 1, If you want the subscriber client to stop and wait for signal 10 after receiving all messages\n");
    printf("             verifyMsg : set = 1, If you want the subscriber client to verify the received message contents match what was passed in with -m\n");
    printf("              orderMsg : set = 1, If you want the client to go into \"orderMsg\" mode.\n" \
           "                       : 1. On publish the original message payload will be prefixed with additional formatting:\n" \
           "                       :     ORDER:<clientId>:<ordering number>:<original message payload>\n" \
           "                       : 2. On subscribe,(NOT IMPLEMENTED YET) the received message will be expected to have the above formatting, \n" \
           "                       :    and the order will be verified, producing an error if order is not sustained\n");
    printf("                       : set = 2, If you want the client to go into \"orderMsg\" mode, without actually failing on ordering problems. You might want to do this if you want to see the ordering information, but not actually fail on it. Or it can also be used to test other criteria for example testCriteriaOrderMin or testCriteriaOrderMax\n" );
    printf("                       : set = 3... if... more info ... for discard messaging tests\n");
    printf("                       : set = 4 if you want ORDER inforation appended to message body. For use with mdbmessages.jar\n");
    printf("         orderMsgStart : set >= 1, along with orderMsg>0 to start ordering msgs at an offset other than 0.\n" );
    printf("    testCriteriaPctMsg : Set to the percent messages that must be sent or recieved to acheive success\n" );
    printf(" testCriteriaVerifyMsg : Set to number >= 1 . If non-verified messages are found >= to that number the test fails.\n");
    printf("  testCriteriaOrderMsg : Set to number >= 1 . If out of order messages are found >= to that number the test fails.\n");
    printf("  testCriteriaOrderMin : Set to number >= 0 . If any ordered messages are found < that number the test fails.\n");
    printf("  testCriteriaOrderMax : Set to number >= 0 . If any ordered messages are found > that number the test fails.\n");
    printf("  testCriteriaMsgCount : Set to number >= 0 To perform the following test below. \n");
    printf("                       : For Qos==2 If exactly that number of messages was not sent/recieved the test fails.\n");
    printf("                       : For Qos==1 If at least that many messages were not sent/received the test fails.\n");
    printf("                       : For Qos==0 the test is not performed.\n");
    printf("          retainedFlag : Set to 0 (do not retain message - default) or 1 (retain message) \n");
    printf("             topicFile : set = filename, if you want the client to use the content of a file as the input topics. See Example 10 for more info.\n");
    printf("    retainedMsgCounted : Set to 1 (default 0) to count any retained message received as part of the expected -n count of messages \n");
    printf("    userReceiveTimeout : Set to >= 0, to specifiy the receveTimeout in mSec used by calls to MQTTClient_receive, default is 100\n");
    printf(" cleanupOnConnectFails : Set = 1 if you want the client disconnected and destroyed on all connection failures \n");
    printf("  cleanupBeforeConnect : Set = 1 if you want the client cleaned up (destroyed) before each connection\n"); 
    printf("                       : Added for RTC 43580 - after getting to 85 percent utilization my clients seem unable to publish\n");
    printf("                       : even after the percent utilization goes below 85. It seems that destroying client and then trying again\n");
    printf("                       : allows further messages to be published. Before adding this some of these traces were observed:\n");
    printf("                       : eg: \"PUBREL of unknown ID\", which seemed to be part of the issue. \n");
    printf("                        : [ 2013-11-01 10:39:30.687223 ]  logger transport.c:506: 2013-11-01T10:39:30.687-05:00 Connection   timer0     CWLNA1117 I: Create mqtt connecti\n                        : [ 2013-11-01 10:39:30.687226 ]  timer0 frame.c:188: MQTT send 20 CONACK connect=346182: len=2 0000 [..]                     \n                        : [ 2013-11-01 10:39:30.687233 ]  tcpdelivery engine.c:1352: >>> ism_engine_startMessageDelivery (hSession 0x7f6ad00bc2a0, options 0)\n                        : [ 2013-11-01 10:39:30.687235 ]  tcpdelivery engine.c:1451: >>> getNextConsumerToEnable (hSession 0x7f6ad00bc2a0)        \n                        : [ 2013-11-01 10:39:30.687235 ]  tcpdelivery engine.c:1515: <<< getNextConsumerToEnable rc=0, hconsumer=(nil)\n                        : [ 2013-11-01 10:39:30.687236 ]  tcpdelivery engine.c:1441: <<< ism_engine_startMessageDelivery rc=0\n                        : [ 2013-11-01 10:39:30.687450 ]  tcpiop.2 mqtt.c:630: MQTT receive 68 PUBREL connect=346182: len=2  []                                          \n                        : [ 2013-11-01 10:39:30.687459 ]  tcpiop.2 mqtt.c:710: PUBREL of unknown ID: connect=346182 client=p01558503516d1208mar473 id=36\n                        : [ 2013-11-01 10:39:30.687461 ]  tcpiop.2 frame.c:188: MQTT send 70 PUBCOMP connect=346182: len=2 0024 [.$]    \n                        : [ 2013-11-01 10:39:30.687596 ]  tcpiop.2 tcp.c:544: closeConnectionNotify: connect=346182(0x1776fb0) reason=Connection closed by client\n                        : [ 2013-11-01 10:39:30.687602 ]  tcpiop.2 mqtt.c:3033: ism_mqtt_closing: connect=346182 client=p01558503516d1208mar473 rc=0 clean=0 reason=Conne\n ");

    printf("         skipSubscribe : Set = 1 if you want to skip the subscribe (e.g. -shared subs do not need to subscribe on reconnect \n");
    printf("    subscribeOnConnect : Set = 1 if you want client to subscribe any time it connects (RTC 28720)- only valid if action set to subscribe\n");
    printf("  subscribeOnReconnect : Set = 1 if you want client to subscribe any time it reconnects (RTC 28720)- only valid if action set to subscribe\n");
    printf(" trueSleepAfterConnect : Set >=0 if you want client to sleep without any mqtt yields immediately after connect\n");
    printf(" publishDelayOnConnect : Set >=0 seconds that you want the publisher to delay after connect (RTC 28720)- before it starts to publish\n");
    printf("          throttleFile : Set = <filename> . If the file exists, and it contains a valid integer >=0 , then that msgrate per second will be dynamically used, for example you can echo a new number into this file to dynamically change the message rate. \n");
    printf("        stopClientFile : <NOT IMPLEMENTED - use kill -15 instead.  Set = <filename> . If the file exists, the client will stop execution w/ success. \n");
    printf("       userIncrementer : Set >=0 to specify that the user name will be formed with av.user + [this value] * tv->array_id (RTC 25629)\n" );
    printf("  unsubscribeAfterConn : Set = 1 if you want the client to unsubscribe after doing a connect - gorrilla testing \n");
    printf("  unsubscribeAfterRecv : Set = 1 if you want the client to unsubscribe after doing a receive - gorrilla testing \n");
    printf("    subscribeAfterRecv : Set = 1 if you want the client to subscribe after doing a receive - gorrilla testing \n");
    printf("           unsubscribe : Set = 1 if you want the client to unsubscribe before disconnecting \n");
    printf("          userLtpaFile : Set <filename> to specify that user ltpa keys should be read from input file with column format\n" );
    printf("                       :  <username1> <ltpakey1> \n");
    printf("                       :  <username2> <ltpakey2> \n");
    printf("                       :  <username3> <ltpakey3> \n");
    printf("                       :       ........        \n");
    printf("                       : When using this option the user MUST also specify \"-u IMA_AUTH_LTPA\" as an option   \n");
    printf("     verifyPubComplete : Set to filename if you are using verifyStillActive, and you also want the client to \n" \
           "                       : check for the existence of this file, before exitting the program when verfiyStillActive\n"\
           "                       : timeout expires. Normally a test program would create this file when publishers have\n"\
           "                       : finished all expected publishing\n");
    printf("     verifyStillActive : If you want the client to verify that it is still active set this value to a\n" \
           "                       : number of seconds greater than zero. Then if at any time the number of messages \n" \
           "                       : received or sent does not change for this many seconds, then the program will \n" \
           "                       : exit gracefully as long as at least one message has flowed through. \n" );
    printf("           warningWait : set >= 0 to specify number of seconds to wait after a warning (default 1 second).\n" );
    printf("   warningWaitDynamic  : set to 1 to specify a dyanmic warning wait of 1 second after connect fails, which is zeroed out after connect success (default disabled).\n" );
    printf(" waitForCompletionMode : set to 1 to process wait for completions in bulk (batch mode).\n" );
    printf("             willTopic : set to string to enable last will and testament\n" );
    printf("           willMessage : set to string to set message for last will and testament (default \"a will message\"\n" );
    printf("          willRetained : set to 0 or 1 to specify retained flag for last will and testament (default 0)\n" );
    printf("               willQos : set to 0 - 2 to specify Qos for last will and testament (default 0)\n" );
}
    printf("         --------------:---------------------------------------------------------------\n");
    printf("        -z number      : set the number of array clients\n");
    printf("         --------------:---------------------------------------------------------------\n");

if(av.trace_level>1){
    printf("---------------------------------------------------------------------------------------\n");
    printf(" Detailed Examples                                                                     \n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 1: Procedure to publish Qos 2 messages and pull ethernet cable while publishing.\n");
    printf("In this example the appliance ip and port  is 10.10.1.87:16102 \n");
    printf("    1. %s -s 10.10.1.87:16102 -a subscribe  -q 2 -n 0 -v -x orderMsg=1 -z 1 -r man -c false -t \"/MARC/1\"  -x reconnectWait=10\n" ,name);      
    printf("    2. record the id for teh subscriber: echo subscriber id is s05215514d21c9damar022  \n");
    printf("    3. %s -s 10.10.1.87:16102 -a publish  -q 2 -n 300 -v -x orderMsg=1 -z 1 -r man -c false -t \"/MARC/1\" -x reconnectWait=10\n" ,name);   
    printf("    4. pull eth cord while publishing for 10.10.1.87, and plug back in after 60 seconds \n");
    printf("    5. %s -s 10.10.1.87:16102 -a subscribe  -q 2 -n 400 -v -x orderMsg=1 -z 1 -r man -c false -t \"/MARC/1\"  -x reconnectWait=10 -i s05215514d21c9damar022\n" ,name);
    printf("    6. Verify all 300 messages were received by subscriber exactly one time.\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 2: Procedure to subscribe for 500 qos 2 messages on an HA Pair configuration, specifying to wait 10 seconds in between each reconnect with up to 100 reconnects. The client should automatically figure out which appliance is Primary and connect to that, and will switch over to the new appliance during a failover event and should continue to receive messages seemlessly.\n");
    printf("    1. %s -s 10.10.1.87:16102 -x haURI=10.10.1.99:16102 -a subscribe -q 2 -n 500 -v -x reconnectWait=10 -x retryConnect=100 \n" ,name);      
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 3: Procedure to do SVT HA Regression Test: RTC 20854 . substitue 213 with a unique number. The quantity of publishers or subscribers can be increased to do a fan in/out pattern  testing.\n");
    printf("1. Start HA test cycles:        . ./ha_test.sh ; do_run_all_tests; \n");
    printf("2. Start HA aware subscriber:   ./mqttsample_array -s 10.10.1.39:16102 -x haURI=10.10.1.40:16102 -a subscribe -q 2 -n 1000000 -v -x reconnectWait=10 -x retryConnect=100  -x connectTimeout=10 -o log.s -i MARC_213_S -c false -r s213 \n");
    printf("3. Start 10 HA aware publishers:for v in {1..10} ; do ./mqttsample_array -s 10.10.1.39:16102 -x haURI=10.10.1.40:16102 -a publish -q 2 -n 100000 -v -x reconnectWait=10 -x retryConnect=100  -x connectTimeout=10 -o log.p.$v -i MARC_213_P_$v -c false -r s213\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 4: Procedure to do SVT NVDIMM testing w/ messaging,  w/ BIGFILE - 262,144 KB size messages -  This test procedure is more about data quantity exercising NVDIMMS vs. rate of messages.\n");
    printf("1. Configure Appliance endpoints to accept messages as large as you plan to send in this case we will send 262,144 KB messages which is the limit. (Note: Below, BIGFILE is a 262,144 KB size text file. You may need to create your own text file. DO NOT CREATE A BINARY FILE - currently only text files are supported by the client application).\n");
    printf("2. Configure Appliance and restart imaserver w/ nvdimms enabled. Key settings: Store.NVRAMOffset = 0x1080000000, Store.MemoryType = 1 .\n");
    printf("3. Start subscriber to start a durable subscription clean session false, with message order verification and message file verification:\n ./mqttsample_array -s 10.10.1.39:16102  -a subscribe -q 2 -n 100  -v -x reconnectWait=10 -x retryConnect=100  -x connectTimeout=10 -i MARC_221_S -c false  -t MARC221  -x userReceiveTimeout=30000 -x verifyMsg=1 -x msgFile=./BIGFILE  -x orderMsg=1  -x keepAliveInterval=60\n");
    printf("4. Wait for \"Begin receiving messages\" to print on the subcriber \n");
    printf("5. Start publisher with clean session false, message ordering: ./mqttsample_array -s 10.10.1.39:16102 -a publish -q 2 -n 100 -v -x reconnectWait=10 -x retryConnect=100  -x connectTimeout=10  -i MARC_221_P -c false -t MARC221  -x orderMsg=1  -x msgFile=./BIGFILE  -x keepAliveInterval=60\n");
    printf("6. At this point , inject an error such as device restart, imaserver stop, imaserver stop force, power off appliance. \n");
    printf("7. When server comes back up, publisher/subscriber should continue where it left off and all messages should be received. Disregard warnings printed out while the clients try to reestablish communication to the appliance, unless they are labeled as ERRORS, or if you do not receive all expected messages.\n");
    printf("8. Repeat steps 3-7 in various combinations, for example, publish all messages, then inject error, then receive all messages. \n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 5: LDAP and send to different users using the userIncrementer feature RTC 25629\n");
    printf(" \n");
    printf("Starting the LDAP Server on mar080 , (should not be needed but fyi) \n ");
    printf("\n");
    printf("openldap : (do not use for mar080)\n");
    printf("\n");
    printf("service slapd status/start/stop/restart  (OpenLDAP)\n");
    printf("\n");
    printf("ITDS: Use this for mar080\n");
    printf("/opt/ibm/ldap/V6.3/sbin/ibmdiradm\n");
    printf("/opt/ibm/ldap/V6.3/bin/ibmdirctl -D \"cn=root\" -w ima4test start\n");
    printf("netstat -n |grep 389\n");
    printf("\n");
    printf("Mar080 uses  ITDS\n");
    printf("\n");
    printf("1a. Configure Appliance for LDAP: imaserver update  LDAP Enabled=True \"URL=ldap://10.10.10.10:5389\" \"BaseDN=ou=SVT,O=IBM,C=US\" \"BindDN=cn=root\" \"BindPassword=ima4test\" \"UserSuffix=ou=users,ou=SVT,O=IBM,C=US\" \"GroupSuffix=ou=groups,ou=SVT,O=IBM,C=US\" \"UserIdMap=uid\" \"GroupIdMap=cn\" \"GroupMemberIdMap=member\" \"EnableCache=True\" \"CacheTimeout=10\" \"GroupCacheTimeout=300\" \"Timeout=30\" \"MaxConnections=100\" \"IgnoreCase=False\" \"NestedGroupSearch=False\" \n");
    printf (" or 1b.  new beefy ldap server\n");
    printf("imaserver update  LDAP Enabled=True \"URL=ldap://10.10.0.32:389\" \"BaseDN=ou=SVT,O=IBM,C=US\" \"BindDN=cn=root,O=IBM,C=US\" \"BindPassword=ima4test\" \"UserSuffix=ou=users,ou=SVT,O=IBM,C=US\" \"GroupSuffix=ou=groups,ou=SVT,O=IBM,C=US\" \"UserIdMap=uid\" \"GroupIdMap=cn\" \"GroupMemberIdMap=member\" \"EnableCache=True\" \"CacheTimeout=10\" \"GroupCacheTimeout=300\" \"Timeout=30\" \"MaxConnections=100\" \"IgnoreCase=False\" \"NestedGroupSearch=False\"\n");
    
    printf("\n");
    printf (" or 1c. ... confirmed workong on 7.29.13, note : 10.10.0.32 does NOT work at this time..... \n");
    printf("\n");
    printf("imaserver update LDAP Enabled=True \"URL=ldap://10.10.10.10:389\" \"BaseDN=ou=SVT,o=IBM,c=US\" \"BindDN=cn=root,o=IBM,c=US\"    \"BindPassword=ima4test\" \"UserSuffix=ou=users,ou=SVT,o=IBM,c=US\" \"GroupSuffix=ou=groups,ou=SVT,o=IBM,c=US\" \"UserIdMap=uid\" \"GroupIdMap=cn\" \"GroupMemberIdMap=member\" \"EnableCache=True\" \"CacheTimeout=10\" \"GroupCacheTimeout=300\" \"Timeout=30\" \"MaxConnections=100\" \"IgnoreCase=False\" \"NestedGroupSearch=false\" \n");
    printf("\n");
    printf (" or 1d. ... new ldap server from Mike Tran on 2.20.14 : mar345 ... Note: Mike found that the BaseDN and BindDN needed to be fixed \n");
    printf(" imaserver update LDAP Enabled=True \"URL=ldap://10.10.1.45:389\" \"BaseDN=o=IBM,c=US\" \"BindDN=cn=root,o=IBM,c=US\" \"BindPassword=ima4test\" \"UserSuffix=ou=users,ou=SVT,O=IBM,C=US\" \"GroupSuffix=ou=groups,ou=SVT,O=IBM,C=US\" \"UserIdMap=uid\" \"GroupIdMap=cn\" \"GroupMemberIdMap=member\" \"EnableCache=True\" \"CacheTimeout=10\" \"GroupCacheTimeout=300\" \"Timeout=30\" \"MaxConnections=100\" \"IgnoreCase=False\" \"NestedGroupSearch=False\" \n");
    printf("\n");
    printf("2. Set Messaging Policy and Connection Policy Group Id if desired: svtUsersInternet\n");
    printf("3. Use the userincrementer feature to send to different users (same password - note different passwords not supported yet\n");
    printf(" e.g. ./mqttsample_array -s 10.10.1.39:16102 -a publish -u u -x userIncrementer=0 -p imasvtest  -z 3 -v \n");
    printf("  running the above command uses users:   u0000000 u0000001 u0000002 , all with password imasvtest\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 6: Using LTPA authentication directly RTC 37212\n");
    printf("\n");
    printf("mqttsample_array  -S trustStore=ssl/trustStore.pem -s ssl://10.10.1.20:17776 -a publish -S keyStore=ssl/imaclient-crt.pem   -S privateKey=ssl/imaclient-key.pem  -u IMA_LTPA_AUTH -p j98jP6lM/iqJYYzaKyu7z0Zp+OsJ3NG1j+dMfElmHMK+QFG7xeBlV0cGmo4uhYqBax3USk0anTtn8sY2FGcghCdF42u26gEp884xKCiBH+kmzvzadqJ96kaDYQZHlbTUefYMTpuOOAo+H8DA6IFXwqheIBic242jogQqmwZ81U5shJ76xUmZnQWcVg4bYs7VAHmgCcut0cvvfIaLAlQew3KKRnlHDawBhdatPL2RtDAeiV3vb37d5h0rqiXC8vT9ttnp2jPbj4hXfuM6IHlx6YCoDD9WF4IW8T8kOznkyyWiKO07CkGZcHmg7Ah4mGf+FnDvcoDLn5ZDqMXxsUxRFgD5M7MjcISAi0/7BHgnENtaUYennPXT1QoUGAu+gUPVxoDuB6qZdN9amXV/mJKU6yRCTFB32saKr+qVdBWcmMeXZjEBPIdRoZhU5kfOzWkQEDbUo32kx/I4ztrqAcuOtAhT+/RUnj9rAkk1jODsY5VzoEoFTpq9qIKYkTaXDWol/m+XyIOIZI4j/cP1uclR/5B2l6alxz3mRa0xrJOrHTexd1YUNddBaTj27E13HN8mU8NAVGFObyLlUzodrmzVh4m0Gij+WZKJKyqkgm/Wmr9beouz5TCsYzpxtekMZmXwbo5MXUM2r+lJ3adyY/O39/17CSFKoIm7qWO8w3Qkf3viT6Nb/7UNSm/jjomA/gEeX\n");
    printf("\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 6b: Using LTPA authentication by reading in LTPA tokens from a flat file RTC 37212\n");
    printf("\n");
    printf("mqttsample_array  -S trustStore=ssl/trustStore.pem -s ssl://10.10.1.20:17776 -a subscribe  -S keyStore=ssl/imaclient-crt.pem   -S privateKey=ssl/imaclient-key.pem  -u IMA_LTPA_AUTH   -v -z 2 -x userLtpaFile=./db.flat.u0000000 \n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 7: Using dynamicClientCert and dccIncrementer to use many different client certs RTC 39682\n");
    printf("\n");
    printf("This example uses 10 different certificate files and keys CN0000010.pem - CN0000019.pem  \n");
    printf("having started at dccIncrementer value of 10 for 10 different clients (-z 10)\n");
    printf("\n");
    printf("Note: the value of dynamicClientCert= must point to a root certificate path containing directories\n");
    printf("stored in the format of 0, 1000, 2000, 3000, .... 999000 each containing 1000 certificates.  \n");
    printf("\n");
    printf(" mqttsample_array   -S trustStore=trustStore.pem  -s ssl://10.10.10.10:17774  -a publish  -x dccIncrementer=10 -z 10 -v -x dynamicClientCert=.  \n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 8a: CommonName - ClientCertificate - Sign at Foundry - Using dccIncrementer and prependCN2Topic together to dynamically subscribe and publish to different topics /CN0100000/man - /CN0100001/man \n");
    printf("\n");
    printf("Note: this is important when the Messaging policy Destination specifies /$CommonName/ \n");
    printf("Note: This example shows equal number of publishers as clients - arch pattern name::: \n");
    printf("\n");
    printf("This example uses 2 different certificate files and keys CN0100000.pem - CN0100001.pem  \n");
    printf("\n");
    printf("/niagara/test/xlinux/bin64/mqttsample_array   -S trustStore=/niagara/test/svt_cmqtt/ssl/trustStore.pem -s ssl://10.10.10.10:17777  -x dccIncrementer=100000 -n 10  -x dynamicClientCert=/svt.data/certificates/ -a subscribe -t /man -x prependCN2Topic=1 -v  -x statusUpdateInterval=1 -z 2\n");
    printf("\n");
    printf("/niagara/test/svt_cmqtt>  /niagara/test/xlinux/bin64/mqttsample_array   -S trustStore=/niagara/test/svt_cmqtt/ssl/trustStore.pem -s ssl://10.10.10.10:17777  -x dccIncrementer=100000  -x dynamicClientCert=/svt.data/certificates/ -a publish -t /man -x prependCN2Topic=1 -v   -n 10 -z 2\n");
    printf("\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 8b: CommonName - ClientCertificate - Sign at Foundry - Doing a fanout using topicMultiTest, dccIncrementer and prependCN2Topic together to dynamically subscribe and publish to different topics /CN0100000/man - /CN0100001/man \n");
    printf("\n");
    printf("Note: This example shows 1 publishers and 100 subscribers as clients - arch pattern name:::  fan out\n");
    printf("\n");
    printf("/niagara/test/xlinux/bin64/mqttsample_array   -S trustStore=/niagara/test/svt_cmqtt/ssl/trustStore.pem -s ssl://10.10.10.10:17777  -x dccIncrementer=100000 -n 10  -x dynamicClientCert=/svt.data/certificates/ -a subscribe -t /man -x prependCN2Topic=1 -v   -x statusUpdateInterval=1 -z 100\n");
    printf("\n");
    printf("/niagara/test/xlinux/bin64/mqttsample_array -s 10.10.10.10:16102  -x dccIncrementer=100000  -a publish -t /man -x prependCN2Topic=1 -v   -x topicMultiTest=100  -n 1000\n");
    printf("\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 8c: CommonName - ClientCertificate - Sign at Foundry - fan out:  dccIncrementer, dynamicClientCert, prependCN2Topic, useCNasClientID, topicMultiTest \n");
    printf("\n");
    printf("Note: This example builds upon what was introducted by example 8b, but now uses and endpoint configured to allow the publisher to publish on any topic, and\n");
    printf("in addition it shows how to useCNasClientID is used to make the client ID become the commonname followed by (appended with) what was passed in for -i\n");
    printf("\n");
    printf("As in example 8b, this example shows a single publisher publishing to 100 different subscribers each on their specific CommonName topic.\n");
    printf("\n");
    printf("/niagara/test/xlinux/bin64/mqttsample_array   -S trustStore=/niagara/test/svt_cmqtt/ssl/trustStore.pem -s ssl://10.10.1.20:17777  -x dccIncrementer=100000 -n 10  -x dynamicClientCert=/svt.data/certificates/ -a subscribe -t \"/yo\" -x prependCN2Topic=1 -v -z 100  -x statusUpdateInterval=100 -z 1 -x useCNasClientID=1 -i s\n");
    printf("\n");
    printf("niagara/test/xlinux/bin64/mqttsample_array   -S trustStore=/niagara/test/svt_cmqtt/ssl/trustStore.pem -s ssl://10.10.10.10:17777  -x dccIncrementer=100000 -x dynamicClientCert=/svt.data/certificates/ -a publish -t \"/yo\" -x prependCN2Topic=1 -v -n 1000 -z 1 -x useCNasClientID=1 -i p -x topicMultiTest=100\n");
    printf("\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 9: Receiving up to 20 M retained messages. (note : special rebuild of libmqttcs.so library required to support MQTT_C_CLIENT_COMMAND_TIMEOUT env variable \n");
    printf("\n");
    printf("MQTT_C_CLIENT_COMMAND_TIMEOUT=600 mqttsample_array -s 10.10.10.10:16102 -x retainedFlag=1  -a subscribe -t \"#\"  -x retainedMsgCounted=1 -n 200000000 -x statusUpdateInterval=1 -v \n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 10: Shared subscription. Also don't forget to update the messaging policy\n");
    printf("mqttsample_array -s 10.10.10.10:17772 -a subscribe -t \"\\$SharedSubscription/SVTcmqttSharedSubscription//hi\" -n 100000000 -x statusUpdateInterval=1\n");
    printf("mqttsample_array -s 10.10.10.10:17772 -a subscribe -t \"\\$SharedSubscription/SVTcmqttSharedSubscription//hi\" -n 100000000 -x statusUpdateInterval=1\n");
    printf("mqttsample_array -s 10.10.10.10:17772 -a subscribe -t \"\\$SharedSubscription/SVTcmqttSharedSubscription//hi\" -n 100000000 -x statusUpdateInterval=1\n");
    printf("mqttsample_array -s 10.10.10.10:17772 -a publish -t \"/hi\" -n 100000000 -i PMARC -x statusUpdateInterval=1\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("\n");
    printf("---------------------------------------------------------------------------------------\n");
    printf("Example 10: using topicFile to clear 681 retained topics\n");
    printf("\n");
    printf("Below TOPICLIST is a 681 line file with 681 different topics\n");
    printf("Each topic is read in and one NULLMSG is published to each different topic\n");
    printf("thereby clearing the retained messages off of the server\n");
    printf("\n");
    printf("mqttsample_array-s 10.10.10.10:17772 -a publish -x msgFile=NULLMSG -x topicFile=TOPICLIST -x retainedFlag=1 -v -n 681\n");
    printf("---------------------------------------------------------------------------------------\n");


}


#if 0
---------------------------
Update: 3.22.13 - Client Certificates.
---------------------------

Documentation: How To get Client Certificates working with the IMA and the MQTT C client

1. IMA setup - Server Side setup for Client Certificates
1a. Create Certificate profile w/ imaserver-crt.pem and imasever-key.pem
1b. Create security profile and check Client Certificate Authentication
1c. Upload Trusted Certificates to Security Profile : imaCA-crt.pem , rootCA-crt.pem
1d. Configure Message Hub for your Secure Endpoint with Client Certificates Make sure that your Secure Endpoint with Client Certificates uses the Security Profile configured in steps 1b, 1c

2. MQTT C client setup
2a. Develop Client application to use the SSL . Note: there is no documentation in the Dec 14th client Pack about how to do this.  -> Opened SVT: IMA: No ssl documentation in client pack. (29362)
2b. Important -Make sure to link to the secure version of the library libmqttv3cs.so . Note there is no documentation in the client pack about why/ when you would use the various libraries delivered. Opened defect: SVT: IMA : no documentation about why/when various libraries delivered in client pack should be used. (29363)
2c Create trustStore.pem file Using files configured on IMA in step 1c
Keys to success:
Create client side trustStore.pem file using the same files configured on IMA in step 1c

rm -rf trustStore.pem ;
for x in  /niagara/test/common/rootCA-crt.pem  /niagara/test/common/imaCA-crt.pem  ; do
     openssl x509 -in $x -text >> trustStore.pem ;
done

2d Run your client application to do Client Certificate authenticated messaging
Keys to success:
Specify your trustStore created in step 2c, your keyStore and privateKey parameters
Specify ssl:// as the URI parameter supplied to MQTTClient_create API calls


 ./mqttsample_array -S trustStore=trustStore.pem  -s ssl://10.10.10.10:16113 -a publish  -S keyStore=imaclient-crt.pem  -S privateKey=imaclient-key.pem  -v

#endif 

    exit(0);
}


/* 
    flag can be used 0,1
        0 - do not adjust burst rate.
        1 - automatically adjust burst rate 
*/
int do_set_msg_rate(char *input, int flag){
    double f;
    int j;
    if (strstr(input, ".")) {
        f=atof(input);
        if (f == 0.0){
            LOG_INFO("Setting rate to default - max speed\n");
            av.desiredMsgsPerSec=0; // default
            av.desiredWaitTime=0;
            if (flag == 1){
                av.desiredMsgBurst=-1; 
            }
        } else if(f<0.0){
            LOG_WARNING ("Invalid msg per second (<0.0) specified: %f.\n", f);
            return 1;
        } else {
            av.desiredWaitTime=(int)round(1.0/f);
            av.desiredMsgsPerSec=0; // default
            LOG_INFO("Setting av.desiredWaitTime to %d\n", av.desiredWaitTime);
            if (flag == 1){
                av.desiredMsgBurst=1; 
            }
        }
    } else {
        j=strtoul(input, 0,10);
        if (j == 0){
            LOG_INFO("Setting rate to default - max speed\n");
            av.desiredMsgsPerSec=0; // default
            av.desiredWaitTime=0;
            if (flag == 1){
                av.desiredMsgBurst=-1; // actually this is a no-op for > 0, but just in case.
            }
        } else if(j<1){
            LOG_WARNING("Invalid msg per second (<1) specified: %d.\n", j);
            return 1;
        } else {
            LOG_INFO("Setting av.desiredMsgsPerSec to %d\n", j);
            av.desiredMsgsPerSec=j;
            if (flag == 1){
                av.desiredMsgBurst=j; // actually this is a no-op for > 0, but just in case.
            }
        }
    }
    return 0; // successful set.
}

/*
 * Parse user inputs  
 *
 * @param argc  The argument count
 * @param argv  The argument vector
 */
void do_parse_arguments(int argc , char **argv){
    int c,j,i,rc,k;
    unsigned long long jj;
    double f;
    struct stat st;
    FILE *fp;

    while ((c=getopt(argc,argv,"a:c:hi:m:n:o:p:q:r:s:S:t:u:vw:x:z:")) != -1) {
        //LOG_INFO("Process Parameter %c.\n", optopt);
        switch (c) {
            case 'a':
                strncpy(av.action, optarg, MAX_BUF_SZ);
                LOG_INFO( "av.action is %s\n", av.action);
                if (av.topic[0] == '\0' ){
                    // Set the default topic according to the specified action
                    if (strcmp(av.action, PUBLISH) == 0 ||
                        strcmp(av.action, PUBLISH_WAIT) == 0 ){
                        strcpy(av.topic, "/MQTTV3Sample/C");
                    } else {
                        strcpy(av.topic, "/MQTTV3Sample/C");
                    }
                }
                if(av.msg==NULL) {
                     av.msg=av.defaultMsg;
                }
                break;
            case 'c':
                if (strcasecmp(optarg, "true")==0){
                    av.cleansession=1;
                } else if (strcasecmp(optarg, "false")==0){
                    av.cleansession=0;
                } else {
                    HALT_ERROR ("Invalid -c flag argument: %s\n", optarg); 
                }
                break;
            case 'h':
                do_usage(argv[0]);
                break;
            case 'i':
                memset(av.clientId, 0, MAX_BUF_SZ);
                strncpy(av.clientId, optarg, CLIENTID_MAX_LEN);
                break;
            case 'm':
                av.msg_buf_len=strlen(optarg);
                av.msg_buf=(char*)malloc(strlen(optarg)+1);
                strcpy(av.msg_buf,optarg);
                av.msg=av.msg_buf;
                break;
            case 'n':
                av.msg_number=strtoul(optarg,0,10);
                LOG_INFO( "av.msg_number is %llu\n", av.msg_number);
                break;
            case 'o':
                av.log2out=fopen(optarg, "w+");
                if (!av.log2out) {
                      HALT_ERROR ("Unable to open output log file (1st) %s\n",optarg);
                }
                av.log2err=av.log2out;
                LOG_STDOUT_SYNC("CMDLINE: ");
                for(k=0;k<argc;k++){
                    LOG_STDOUT_NO_TIMESTAMP("%s ", argv[k]);
                }
                LOG_STDOUT_NO_TIMESTAMP("\n");
                fclose(av.log2out);
                av.log2out=fopen(optarg, "a+");
                if (!av.log2out) {
                      HALT_ERROR ("Unable to open output log file (2nd) %s\n",optarg);
                }
                if ( do_check_file_exists(optarg) != 1 ){
                    // This code shouldnt execute....
                    fprintf(stderr, "Extremely wierd. %s does not exist but should. Wait for it: ... zzz", optarg );
                    LOG_INFO("Extremely wierd. %s does not exist but should. Wait for it: ... zzz", optarg );
                    while ( do_check_file_exists(optarg) != 1) {
                        fprintf(stderr, "z" ); // This code shouldnt execute....
                        LOG_INFO("z");
                        usleep(1000);
                    }
                }
                break;
            case 'p':
                strncpy(av.password, optarg, MAX_BUF_SZ);
                LOG_INFO("Using password %s\n", av.password);
                break;
            case 'q':
                av.qos=strtoul(optarg,0,10);
                if (av.qos>2 || av.qos<0){ 
                    HALT_ERROR ("Invalid Qos %d\n", av.qos); 
                }
                break;
            case 'r':
                strncpy(av.persistence_context_buf, optarg, MAX_BUF_SZ);
                av.persistence_context=av.persistence_context_buf;
                av.persistence_type=MQTTCLIENT_PERSISTENCE_DEFAULT;
                break;
            case 's':
                strncpy(av.uri, optarg, MAX_BUF_SZ);
                break;
            case 'S':
                LOG_INFO("OPTARG for -S is %s\n", optarg);
                if(strncmp(       optarg, "keyStore=", 9) == 0){
                    j=strlen(optarg+9)+1;
                    av.keyStore=(char*)malloc(j);
                    strncpy(av.keyStore, optarg+9, j);
                } else if(strncmp(optarg, "trustStore=", 11) == 0){
                    j=strlen(optarg+11)+1;
                    av.trustStore=(char*)malloc(j);
                    LOG_INFO("Allocating %d bytes \n",j);
                    strncpy(av.trustStore, optarg+11,j );
                    LOG_INFO("av.trustStore is %s \n",av.trustStore);
                } else if(strncmp(optarg, "privateKey=", 11) == 0){
                    j=strlen(optarg+11)+1;
                    av.privateKey=(char*)malloc(strlen(optarg+11)+1);
                    strncpy(av.privateKey, optarg+11, j);
                } else if(strncmp(optarg, "privateKeyPassword=", 19) == 0){
                    j=strlen(optarg+19)+1;
                    av.privateKeyPassword=(char*)malloc(j);
                    strncpy(av.privateKeyPassword, optarg+19, j);
                } else if(strncmp(optarg, "enabledCipherSuites=", 20) == 0){
                    j=strlen(optarg+20)+1;
                    av.enabledCipherSuites=(char*)malloc(j);
                    strncpy(av.enabledCipherSuites, optarg+20, j);
                } else if(strncmp(optarg, "enableServerCertAuth=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j < 0 || j>1){
                        HALT_ERROR ("Invalid enableServerCertAuth= value specified: %d\n", j);
                    }
                    av.enableServerCertAuth=j;
                } else {
                    HALT_ERROR ("Invalid switch specifed for -S : %s\n", optarg);
                }
                break;
            case 't':
                strncpy(av.topic, optarg, MAX_BUF_SZ);
                break;
            case 'u':
                strncpy(av.user, optarg, MAX_BUF_SZ);
                break;
            case 'v':
                av.trace_level=av.trace_level+1;
                break;
            case 'w':
                if ( do_set_msg_rate(optarg,0)!=0){
                    HALT_ERROR ("Invalid input for -w flag %s.\n", optarg);
                }
                break;
            case 'x':
                LOG_INFO("OPTARG for -x is %s\n", optarg);
                if(strncmp(       optarg, "scaleTest=", 10) == 0){
                    j=strtoul(optarg+10,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid scaleTest= value specified: %d\n", j);
                    }
                    av.scaleTest=j;
                } else if(strncmp(       optarg, "injectDisconnect=", 17) == 0){
                    j=strtoul(optarg+17,0,10);
                    if (j < 0 ){
                        HALT_ERROR ("Invalid av.injectDisconnect= value specified: %d\n", j);
                    }
                    av.injectDisconnect=j;
                } else if(strncmp(       optarg, "injectDisconnectX=", 18) == 0){
                    j=strtoul(optarg+18,0,10);
                    if (j < 0 ){
                        HALT_ERROR ("Invalid av.injectDisconnectX= value specified: %d\n", j);
                    }
                    av.injectDisconnectX=j;

                } else if(strncmp(       optarg, "willQos=", 8) == 0){
                    j=strtoul(optarg+8,0,10);
                    if (j < 0 || j > 2){
                        HALT_ERROR ("Invalid av.willQos= value specified: %d\n", j);
                    }
                    av.willQos=j;
                } else if(strncmp(       optarg, "willRetained=", 13) == 0){
                    j=strtoul(optarg+13,0,10);
                    if (j < 0 || j > 2){
                        HALT_ERROR ("Invalid av.willRetained= value specified: %d\n", j);
                    }
                    av.willRetained=j;
                    LOG_INFO("set willRetained to %d\n", av.willRetained);
                } else if(strncmp(       optarg, "willTopic=", 10) == 0){
                    av.willTopic=(char*)malloc(strlen(optarg+10) +1 );
                    memset(av.willTopic, '\0', (strlen(optarg+10) +1 ));
                    strcpy(av.willTopic ,optarg+10);
                    LOG_INFO("set willTopic to %s\n", av.willTopic);
                } else if(strncmp(       optarg, "willMessage=", 12) == 0){
                    av.willMessage=(char*)malloc(strlen(optarg+12) +1 );
                    memset(av.willMessage, '\0', (strlen(optarg+12) +1 ));
                    strcpy(av.willMessage ,optarg+12);
                    LOG_INFO("set willMessage to %s\n", av.willMessage);
                } else if(strncmp(       optarg, "injectDisconnectMax=", 20) == 0){
                    j=strtoul(optarg+20,0,10);
                    if (j < 0 ){
                        HALT_ERROR ("Invalid av.injectDisconnectMax= value specified: %d\n", j);
                    }
                    av.injectDisconnectMax=j;
                } else if(strncmp(       optarg, "useWrongCNasClientID=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j >= 0 ){
                        HALT_ERROR ("Invalid av.useWrongCNasClientID= value specified: %d\n", j);
                    }
                    av.useWrongCNasClientID=j;
                } else if(strncmp(       optarg, "useCNasClientID=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid av.useCNasClientID= value specified: %d\n", j);
                    }
                    av.useCNasClientID=j;
                } else if(strncmp(       optarg, "connectRate=", 12) == 0){
                    f=atof(optarg+12);
                    if (f < 0.0){
                        HALT_ERROR ("Invalid connectRate= value specified: %f\n", f);
                    }
                    av.connectRate=f;
                } else if(strncmp(       optarg, "burst=", 6) == 0){
                    j=strtoul(optarg+6,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid burst= value specified: %d\n", j);
                    }
                    av.desiredMsgBurst=j;
                } else if(strncmp(       optarg, "retainedFlag=", 13) == 0){
                    j=strtoul(optarg+13,0,10);
                    if (j != 0 && j !=1){
                        HALT_ERROR ("Invalid retainedFlag= value specified: %d\n", j);
                    }
                    av.retained_flag=j;
                } else if(strncmp(       optarg, "retainedMsgCounted=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j != 0 && j !=1){
                        HALT_ERROR ("Invalid retainedMsgCounted= value specified: %d\n", j);
                    }
                    av.retainedMsgCounted=j;
                } else if(strncmp(       optarg, "haURI=", 6) == 0){
                    av.haConnect=1; // start with a reconnect to uri. , then toggle to 2 to try av.haURI , repeat etc..
                    strncpy(av.haURI, optarg+6, MAX_BUF_SZ);
                    LOG_INFO("User has set up HighAvailability av.haURI to %s\n", av.haURI);
                } else if(strncmp(       optarg, "statusUpdateInterval=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j <= 0){
                        HALT_ERROR ("Invalid statusUpdateInterval= value specified: %d\n", j);
                    }
                    av.lastStatTimeIntervalUSec=j*MICROS_PER_SECOND;
                } else if(strncmp(optarg, "reconnectWait=", 14) == 0){
                    j=strtoul(optarg+14,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid reconnectWait= value specified: %d\n", j);
                    }
                    av.reconnectWait=j; 
                } else if(strncmp(optarg, "subscribeAfterRecv=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j < 0 || j>1){
                        HALT_ERROR ("Invalid subscribeAfterRecv= value specified: %d\n", j);
                    }
                    av.subscribeAfterRecv=j;
                } else if(strncmp(optarg, "unsubscribeAfterConn=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j < 0 || j>1){
                        HALT_ERROR ("Invalid unsubscribeAfterConn= value specified: %d\n", j);
                    }
                    av.unsubscribeAfterConn=j;
                } else if(strncmp(optarg, "unsubscribeAfterRecv=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j < 0 || j>1){
                        HALT_ERROR ("Invalid unsubscribeAfterRecv= value specified: %d\n", j);
                    }
                    av.unsubscribeAfterRecv=j;
                } else if(strncmp(optarg, "unsubscribe=", 12) == 0){
                    j=strtoul(optarg+12,0,10);
                    if (j < 0 || j>1){
                        HALT_ERROR ("Invalid unsubscribe= value specified: %d\n", j);
                    }
                    av.unsubscribe=j;

                } else if(strncmp(optarg, "userLtpaFile=", 13) == 0){
                    strncpy(av.userLtpaFile, optarg+13, MAX_BUF_SZ);
                    LOG_INFO("User has set up av.userLtpaFile to %s\n", av.userLtpaFile );
                } else if(strncmp(optarg, "userIncrementer=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid userIncrementer= value specified: %d\n", j);
                    }
                    av.userIncrementer=j;
                } else if(strncmp(optarg, "msgTimeoutAfterSub=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid msgTimeoutAfterSub= value specified: %d\n", j);
                    }
                    av.msgTimeoutAfterSub=j; 
                } else if(strncmp(optarg, "connectTimeout=", 15) == 0){
                    j=strtoul(optarg+15,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid connectTimeout= value specified: %d\n", j);
                    }
                    av.connectTimeout=j; 
                } else if(strncmp(optarg, "noError=", 8) == 0){
                    j=strtoul(optarg+8,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid noError= value specified: %d\n", j);
                    }
                    av.noError=j; 


                } else if(strncmp(       optarg, "sharedSubMulti=", 15) == 0){
                    j=strtoul(optarg+15,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid sharedSubMulti= specified: %d\n", j);
                    }
                    av.sharedSubMulti=j; 

                } else if(strncmp(       optarg, "sharedSubscription=", 19) == 0){
                    //strncpy(av.sharedSubscription, optarg+19, MAX_BUF_SZ);
                    sprintf(av.sharedSubscription, "$SharedSubscription/%s", optarg+19);
                    LOG_INFO("User has set up  av.sharedSubscription to %s\n", av.sharedSubscription);
                } else if(strncmp(       optarg, "sharedMemoryCoupling=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j !=0 && j!= 1){
                        HALT_ERROR ("Invalid sharedMemoryCoupling= value specified: %d\n", j);
                    }
                    av.sharedMemoryCoupling=j;
                } else if(strncmp(       optarg, "sharedMemoryType=", 17) == 0){
                    j=strtoul(optarg+17,0,10);
                    if (j !=0 && j!= 1){
                        HALT_ERROR ("Invalid sharedMemoryType= value specified: %d\n", j);
                    }
                    av.sharedMemoryType=j;
                } else if(strncmp(       optarg, "sharedMemoryKey=", 16) == 0){
                    strncpy(av.sharedMemoryKey, optarg+16, MAX_BUF_SZ);
                    do_shm_setup();
                    LOG_INFO("User has set up av.sharedMemoryKey  to %s\n", av.sharedMemoryKey);
                } else if(strncmp(       optarg, "sharedMemoryVal=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j < 1){
                        HALT_ERROR ("Invalid sharedMemoryVal= value specified: %d\n", j);
                    }
                    av.sharedMemoryVal=j; 
                } else if(strncmp(optarg, "noDisconnect=", 13) == 0){
                    j=strtoul(optarg+13,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid noDisconnect= value specified: %d\n", j);
                    }
                    av.noDisconnect=j; 
                } else if(strncmp(optarg, "86WaitForCompletion=", 20) == 0){
                    j=strtoul(optarg+20,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid 86WaitForCompletion= value specified: %d\n", j);
                    }
                    av.eightySixWaitForCompletion=j; 
                    
                } else if(strncmp(       optarg, "retainedFlag=", 13) == 0){
                    j=strtoul(optarg+13,0,10);
                    if (j != 0 && j !=1){
                        HALT_ERROR ("Invalid retainedFlag= value specified: %d\n", j);
                    }
                    av.retained_flag=j;
                //} else if(strncmp(       optarg, "retainedMsgCounted=", 19) == 0){
                    //j=strtoul(optarg+19,0,10);
                    //if (j != 0 && j !=1){
                        //HALT_ERROR ("Invalid retainedMsgCounted= value specified: %d\n", j);
                    //}
                    //av.retainedMsgCounted=j;
                } else if(strncmp(       optarg, "haURI=", 6) == 0){
                    av.haConnect=1; // start with a reconnect to uri. , then toggle to 2 to try av.haURI , repeat etc..
                    strncpy(av.haURI, optarg+6, MAX_BUF_SZ);
                    LOG_INFO("User has set up HighAvailability av.haURI to %s\n", av.haURI);
                } else if(strncmp(       optarg, "statusUpdateInterval=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j <= 0){
                        HALT_ERROR ("Invalid statusUpdateInterval= value specified: %d\n", j);
                    }
                    av.lastStatTimeIntervalUSec=j*MICROS_PER_SECOND;
                } else if(strncmp(optarg, "skipSubscribe=", 14) == 0){
                    j=strtoul(optarg+14,0,10);
                    if (j < 0 || j > 1){
                        HALT_ERROR ("Invalid skipSubscribe= value specified: %d\n", j);
                    }
                    av.skipSubscribe=j; 
                } else if(strncmp(optarg, "checkConnection=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid checkConnection= value specified: %d\n", j);
                    }
                    av.checkConnection=j; 
                } else if(strncmp(optarg, "waitAfterReceive=", 17) == 0){
                    j=strtoul(optarg+17,0,10);
                    if (j < 0 || j > 1){
                        HALT_ERROR ("Invalid waitAfterReceive= value specified: %d\n", j);
                    }
                    av.waitAfterReceive=j; 
                } else if(strncmp(optarg, "reconnectWait=", 14) == 0){
                    j=strtoul(optarg+14,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid reconnectWait= value specified: %d\n", j);
                    }
                    av.reconnectWait=j; 
                } else if(strncmp(optarg, "userIncrementer=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid userIncrementer= value specified: %d\n", j);
                    }
                    av.userIncrementer=j;
                } else if(strncmp(optarg, "connectTimeout=", 15) == 0){
                    j=strtoul(optarg+15,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid connectTimeout= value specified: %d\n", j);
                    }
                    av.connectTimeout=j; 
                } else if(strncmp(optarg, "noError=", 8) == 0){
                    j=strtoul(optarg+8,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid noError= value specified: %d\n", j);
                    }
                    av.noError=j; 
                } else if(strncmp(       optarg, "dynamicClientCert=", 18) == 0){
                    strncpy(av.dynamicClientCert, optarg+18, MAX_BUF_SZ);
                    LOG_INFO("User has set up av.dynamicClientCert= to %s\n", av.dynamicClientCert);

                } else if(strncmp(optarg, "dccIncrementer=", 15) == 0){
                    j=strtoul(optarg+15,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid dccIncrementer= value specified: %d\n", j);
                    }
                    av.dccIncrementer=j; 

                } else if(strncmp(optarg, "noDisconnect=", 13) == 0){
                    j=strtoul(optarg+13,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid noDisconnect= value specified: %d\n", j);
                    }
                    av.noDisconnect=j; 
                } else if(strncmp(optarg, "86WaitForCompletion=", 20) == 0){
                    j=strtoul(optarg+20,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid 86WaitForCompletion= value specified: %d\n", j);
                    }
                    av.eightySixWaitForCompletion=j; 
                } else if(strncmp(optarg, "testCriteriaOrderMin=", 21) == 0){
                    jj=strtoull(optarg+21,0,10);
                    if (jj < 0 ){
                        HALT_ERROR ("Invalid testCriteriaOrderMin= value specified: %llu, must be >= 0\n", jj);
                    }
                    av.testCriteriaOrderMin_enabled=1;
                    av.testCriteriaOrderMinVal=jj;
                    av.testCriteriaOrderMin=0;
                } else if(strncmp(optarg, "testCriteriaOrderMax=", 21) == 0){
                    jj=strtoull(optarg+21,0,10);
                    if (jj < 0 ){
                        HALT_ERROR ("Invalid testCriteriaOrderMax= value specified: %llu, must be >= 0\n", jj);
                    }
                    av.testCriteriaOrderMax_enabled=1;
                    av.testCriteriaOrderMaxVal=jj;
                    av.testCriteriaOrderMax=0;
                } else if(strncmp(optarg, "orderMsgStart=", 14) == 0){
                    jj=strtoull(optarg+14,0,10);
                    if (jj < 1 ){
                        HALT_ERROR ("Invalid orderMsgStart= value specified: %llu, must be >= 1\n", jj);
                    }
                    av.orderMsgStart=jj;
                } else if(strncmp(optarg, "testCriteriaOrderMsg=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j < 0 ){
                        HALT_ERROR ("Invalid testCriteriaOrderMsg= value specified: %d, must be >= 0\n", j);
                    }
                    av.testCriteriaOrderMsg_enabled=1;
                    av.testCriteriaOrderMsg=0;
                    LOG_INFO("User has set av.testCriteriaOrderMsg_enabled to %d. \n" \
                             "There must be < than that many out of order messages to pass this criteria\n",
                             av.testCriteriaOrderMsg_enabled);
                } else if(strncmp(optarg, "testCriteriaVerifyMsg=", 22) == 0){
                    j=strtoul(optarg+22,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid testCriteriaVerifyMsg= value specified: %d, must be >= 0\n", j);
                    }
                    av.testCriteriaVerifyMsg_enabled=j;
                    av.testCriteriaVerifyMsg=0;
                    LOG_INFO("User has set av.testCriteriaVerifyMsg_enabled to %d. \n" \
                             "There must be < than that many out of order messages to pass this criteria\n",
                             av.testCriteriaVerifyMsg_enabled);
                } else if(strncmp(optarg, "testCriteriaMsgCount=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j < 0 ){
                        HALT_ERROR ("Invalid testCriteriaMsgCount= value specified: %d, must be >=0\n", j);
                    }
                    av.testCriteriaMsgCount_enabled=1;
                    av.testCriteriaMsgCount=j;
                    LOG_INFO("User has set av.testCriteriaMsgCount to %d\n", av.testCriteriaMsgCount);

                } else if(strncmp(optarg, "testCriteriaPctMsg=", 16) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j < 0 || j>100){
                        HALT_ERROR ("Invalid testCriteriaPctMsg= value specified: %d, must be 0-100\n", j);
                    }
                    av.testCriteriaPercentMessage_enabled=1;
                    av.testCriteriaPercentMessage=j;
                    LOG_INFO("User has set av.testCriteriaPctMessage to %d\n", av.testCriteriaPercentMessage);
                } else if(strncmp(optarg, "badTopicChangeTest=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid badTopicChangeTest= value specified: %d\n", j);
                    }
                    av.badTopicChangeTest=j;
                    LOG_INFO("User has set av.badTopicChangeTest to %d\n", av.badTopicChangeTest);
                } else if(strncmp(optarg, "topicMultiTestIncr=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid topicMultiTestIncr= value specified: %d\n", j);
                    }
                    av.topicMultiTestIncr=j;
                    LOG_INFO("User has set av.topicMultiTestIncr to %llu\n", av.topicMultiTestIncr);
                } else if(strncmp(optarg, "topicMultiTest=", 15) == 0){
                    j=strtoul(optarg+15,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid topicMultiTest= value specified: %d\n", j);
                    }
                    av.topicMultiTest=j;
                    av.topicMultiTestCounter=0; // count from topic/0  to topic/j-1
                    LOG_INFO("User has set av.topicMultiTest to %d\n", av.topicMultiTest);
                } else if(strncmp(optarg, "topicChangeTest=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid topicChangeTest= value specified: %d\n", j);
                    }
                    av.topicChangeTest=j;
                    LOG_INFO("User has set av.topicChangeTest to %d\n", av.topicChangeTest);
                } else if(strncmp(optarg, "prependCN2Topic=", 16) == 0){
                    j=strtoul(optarg+16,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid prependCN2Topic= value specified: %d\n", j);
                    }
                    av.prependCN2Topic=j;
                    LOG_INFO("User has set av.prependCN2Topic= to %d\n", av.prependCN2Topic);
                } else if(strncmp(optarg, "userReceiveTimeout=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid userReceiveTimeout= value specified: %d\n", j);
                    }
                    av.userReceiveTimeout=j;
                } else if(strncmp(optarg, "verifyMsg=", 10) == 0){
                    j=strtoul(optarg+10,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid verifyMsg= value specified: %d\n", j);
                    }
                    av.verifyMsg=j;
                    if ( av.testCriteriaVerifyMsg_enabled <= 0) {
                        av.testCriteriaVerifyMsg_enabled=1;
                        av.testCriteriaVerifyMsg=0;
                    }
                } else if(strncmp(optarg, "usleepLoop=", 11) == 0){
                    j=strtoul(optarg+11,0,10);
                    if (j <0 ){
                        HALT_ERROR ("Invalid usleepLoop= value specified: %d\n", j);
                    }
                    av.usleepLoop=j;
                } else if(strncmp(optarg, "sleepLoop=", 10) == 0){
                    j=strtoul(optarg+10,0,10);
                    if (j <0 ){
                        HALT_ERROR ("Invalid sleepLoop= value specified: %d\n", j);
                    }
                    av.sleepLoop=j;
                } else if(strncmp(optarg, "reliable=", 9) == 0){
                    j=strtoul(optarg+9,0,10);
                    if (j != 0 && j != 1){
                        HALT_ERROR ("Invalid reliable= value specified: %d\n", j);
                    }
                    av.reliable=j;
                } else if(strncmp(optarg, "keepAliveInterval=", 18) == 0){
                    j=strtoul(optarg+18,0,10);
                    if (j < 2){
                        HALT_ERROR ("Invalid keepAliveInterval= value specified: %d\n", j);
                    }
                    av.keepAliveInterval=j;
                } else if(strncmp(optarg, "cleanupBeforeConnect=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j != 0 && j != 1 ){
                        HALT_ERROR ("Invalid cleanupBeforeConnect= value specified: %d\n", j);
                    }
                    av.destroyClientBeforeConnect=j;
                } else if(strncmp(optarg, "cleanupOnConnectFails=", 22) == 0){
                    j=strtoul(optarg+22,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid cleanupOnConnectFails= value specified: %d\n", j);
                    }
                    av.destroyClientOnAllConnectFails=j;
                } else if(strncmp(optarg, "dynamicReceiveTimeoutv2=", 24) == 0){
                    j=strtoul(optarg+24,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid dynamicReceiveTimeoutv2= value specified: %d\n", j);
                    }
                    av.dynamicReceiveTimeoutv2=j;
                } else if(strncmp(optarg, "dynamicReceiveTimeout=", 22) == 0){
                    j=strtoul(optarg+22,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid dynamicReceiveTimeout= value specified: %d\n", j);
                    }
                    av.dynamicReceiveTimeout=j;
                } else if(strncmp(optarg, "orderMsg=", 9) == 0){
                    j=strtoul(optarg+9,0,10);
                    if (j < 0 || j>4){
                        HALT_ERROR ("Invalid orderMsg= value specified: %d\n", j);
                    }
                    av.orderMsg=j;
                    if (av.testCriteriaOrderMsg_enabled<=0) {
                        av.testCriteriaOrderMsg_enabled=1;
                        av.testCriteriaOrderMsg=0;
                    }
                } else if(strncmp(optarg, "trueSleepAfterConnect=", 22) == 0){
                    j=strtoul(optarg+22,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid trueSleepAfterConnect= value specified: %d\n", j);
                    }
                    av.trueSleepAfterConnect=j;
                } else if(strncmp(optarg, "retryConnect=", 13) == 0){
                    j=strtoul(optarg+13,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid retryConnect= value specified: %d\n", j);
                    }
                    av.retry_connect=j;
                } else if(strncmp(optarg, "sendOutOfOrderMsg=", 18) == 0){
                    j=strtoul(optarg+18,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid sendOutOfOrderMsg= value specified: %d\n", j);
                    }
                    av.sendOutOfOrderMsg=j;
                    LOG_INFO("av.sendOutOfOrderMsg specified: %d\n", av.sendOutOfOrderMsg);

                } else if(strncmp(optarg, "publishDelayOnConnect=", 22) == 0){
                    j=strtoul(optarg+22,0,10);
                    if (j < 0 ){
                        HALT_ERROR ("Invalid publishDelayOnConnect= value specified: %d\n", j);
                    }
                    av.publishDelayOnConnect=j;
                    LOG_INFO("av.publishDelayOnConnect specified: %d\n", av.publishDelayOnConnect );
                
                } else if (strncmp(optarg, "stopClientFile=", 15) == 0){
                    strncpy(av.stopClientFile, optarg+15, MAX_BUF_SZ);
                    LOG_INFO("av.stopClientFile specified: %s\n", av.stopClientFile);
                } else if (strncmp(optarg, "throttleFile=", 13) == 0){
                    strncpy(av.throttleFile, optarg+13, MAX_BUF_SZ);
                    LOG_INFO("av.throttleFile specified: %s\n", av.throttleFile);
                } else if(strncmp(optarg, "waitForCompletionMode=",22 )  == 0){
                    j=strtoul(optarg+22,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid waitForCompletionMode= value specified: %d\n", j);
                    }
                    av.waitForCompletionMode=j;
                    LOG_INFO("av.waitForCompletionMode specified: %d\n", av.waitForCompletionMode);
                } else if(strncmp(optarg, "subscribeOnReconnect=", 21) == 0){
                    j=strtoul(optarg+21,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid subscribeOnReconnect= value specified: %d\n", j);
                    }
                    av.subscribeOnReconnect=j;
                    LOG_INFO("av.subscribeOnReconnect specified: %d\n", av.subscribeOnReconnect );
                } else if(strncmp(optarg, "subscribeOnConnect=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j !=1 && j != 0){
                        HALT_ERROR ("Invalid subscribeOnConnect= value specified: %d\n", j);
                    }
                    av.subscribeOnConnect=j;
                    LOG_INFO("av.subscribeOnConnect specified: %d\n", av.subscribeOnConnect );
                } else if(strncmp(optarg, "warningWaitDynamic=", 19) == 0){
                    j=strtoul(optarg+19,0,10);
                    if (j != 0 && j != 1){
                        HALT_ERROR ("Invalid warningWaitDynamic= value specified: %d\n", j);
                    }
                    av.warningWaitDynamic=j;
                    LOG_INFO("av.warningWaitDynamic specified: %d\n", av.warningWaitDynamic );
                } else if(strncmp(optarg, "warningWait=", 12) == 0){
                    j=strtoul(optarg+12,0,10);
                    if (j < 0){
                        HALT_ERROR ("Invalid warningWait= value specified: %d\n", j);
                    }
                    av.warningWait=j;
                    LOG_INFO("av.warningWait specified: %d\n", av.warningWait );
                } else if(strncmp(optarg, "topicFile=", 10) == 0){
                    LOG_INFO("User wants to use the contents of the input file: %s as the topic. Loading file.\n", optarg+10);
                    do_load_file(optarg+10, &av.topic_buf, &av.topic_buf_len  );

                } else if(strncmp(optarg, "msgFile=", 8) == 0){
                    LOG_INFO("User wants to use the contents of the input file: %s as the message. Loading file.\n", optarg+8);
                    stat(optarg+8,&st);
                    av.msg_buf_len=st.st_size;
                    LOG_INFO("av.msg_buf_len set to %d\n", av.msg_buf_len);
                    av.msg_buf=(char*)malloc(st.st_size+1);
                    av.msg=av.msg_buf;
                    memset(av.msg, '\0', st.st_size+1);
                    fp=fopen(optarg+8, "r");
                    if(fp!=NULL){
                        rc=fread(av.msg_buf, 1, st.st_size, fp);
                        if (rc!= st.st_size){
                            HALT_ERROR ("Could not read %d bytes from input msgFile, instead only read %d\n",(int)st.st_size, rc);
                        }
                    } else {
                        HALT_ERROR ("Problem reading input file from msgFile, %s\n",optarg+8);
                    }
                    fclose (fp);
                    LOG_INFO("Done Loading file %s. Closed file\n", optarg+8);
                } else if(strncmp(optarg, "verifyPubComplete=", 18) == 0){
                    av.verifyPubComplete=(char*)malloc(strlen(optarg+18)+1);
                    strcpy(av.verifyPubComplete, optarg+18);
                    LOG_INFO("verifyPubComplete file specified: %s\n", av.verifyPubComplete);
                } else if(strncmp(optarg, "verifyStillActive=", 18) == 0){
                    av.verifyStillActive=strtoull(optarg+18,0,10)*MICROS_PER_SECOND;
                    if (av.verifyStillActive < 0){
                         HALT_ERROR ("Invalid verifyStillActive setting <1 specified: %llu\n", av.verifyStillActive);
                    }
                    LOG_INFO("verifyStillActive specified: %llu\n", av.verifyStillActive);
                } else {
                    HALT_ERROR ("Invalid switch specifed for -x : %s\n", optarg);
                }
                break;
            case 'z':
                av.num_clients=strtoul(optarg,0,10);
                if(av.num_clients<1 || av.num_clients>32768){
                    HALT_ERROR ("Invalid num_clients specified %s.\n",
                       optarg); 
                }
                //LOG_INFO("Setting num_clients to %d from %c %s\n", av.num_clients,optopt, optarg);
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
      
    if (av.uri[0]=='\0' ){
         HALT_ERROR ("Missing required parameter: -s\n");
    }

    if ( av.action[0]=='\0' ){
         HALT_ERROR ("Missing required parameter: -a\n");
    }
}

void do_safe_mqtt_client_usleep(clientVariables *tv, long long useconds) {

    long long  currTime=getHRTimer();
    long long endTime=currTime+useconds;
    
    do{
        do_status_update(tv,0);
        MQTTClient_yield();
        currTime=getHRTimer();
        if (currTime>endTime){
            break;
        }else if ((endTime-currTime)>MICROS_PER_SECOND){
            sleep(1);
        } else {
            usleep(endTime-currTime);
        }
    } while(1);

}

int do_check_file_exists(char *filename){
    if (access(filename, F_OK) != 0){ // test if file exists
        return 0;   // file does not exist
    } else {
        return 1;  // file exists
    }
}

void do_throttle_check(void){
    struct stat st;
    static long long lastcheck=0;
    long long  currTime=getHRTimer();
    FILE *fp;
    char buffer[32];
    char *fn=av.throttleFile;
    int rc;
    
    if (fn[0]!='\0' && ((currTime-lastcheck)>MICROS_PER_SECOND)){
        lastcheck=currTime; // we will only check for a new dynamic throttle rate at most once per second.
        if (do_check_file_exists(fn)== 1){
            stat(fn,&st);
            if (difftime(av.throttleFileLastModTime, st.st_mtime)!=0.0){
            LOG_STDOUT("C av.throttleFile: %s \n", fn);
                // the fn has changed, need to reinitialize throttle value.
                av.throttleFileLastModTime=st.st_mtime;
                if (st.st_size <32 ){
                    LOG_STDOUT("av.throttleFile: %s dynamic change detected reading in new value\n", fn);
                    fp=fopen(fn, "r");
                    if(fp!=NULL){
                        rc=fscanf (fp, "%s", buffer); 
                        if (rc!=1){
                            LOG_WARNING ("Problem reading input file :%s, rc=%d\n",fn,(int)rc);
                        }else {
                            LOG_STDOUT("av.throttleFile:  Adjusting rate for subscriber to: %s\n", buffer);
                            if ( do_set_msg_rate(buffer, 1)!=0){
                                LOG_WARNING("av.throttleFile: (b) invalid contents in this file. ignoring input.\n"); 
                            }
                        }
                    } else {
                        HALT_ERROR ("Problem reading input file :%s\n",fn);
                    }
                    fclose(fp);
                } else {
                    LOG_WARNING("av.throttleFile:(a) invalid contents in this file. ignoring input.\n"); 
                }
            }
        }
    }
}

void do_safe_mqtt_client_sleep(clientVariables *tv, int seconds) {
    CLIENT_LOG_INFO(tv, "Client will sleep for %d seconds\n", seconds); 
    do_safe_mqtt_client_usleep(tv, seconds*MICROS_PER_SECOND);
    CLIENT_LOG_INFO(tv, "Client will sleep for %d seconds\n", seconds); 
}

void do_status_update(clientVariables *tv, int force){
    static int verifyFailed=0;
    long long  currTime=getHRTimer();
    char pubTopic[]="/svt/";
    char pubMsg[256];
    char str1[256];
    int rc=0;
    rateControl *r=&av.tv->rate_s;
    MQTTClient_message mqttMessage = MQTTClient_message_initializer;

    do {
        if(av.verifyStillActive!=0){
            if( av.tv->rate_s.nmsgs > av.lastActiveCount ){
                av.lastActiveCount=av.tv->rate_s.nmsgs;
                av.lastActiveTime=currTime;
                if (verifyFailed!=0) { verifyFailed=0; }
            } else if (((currTime-av.lastActiveTime) > av.verifyStillActive ) && 
                        (av.lastActiveCount > 0)){
                if (verifyFailed==0){
                    LOG_INFO("(only prints once)Verification failed for " \
                             "verifyStillActive setting expired : %llu.\n ",
                            av.verifyStillActive);
                }
                if (av.verifyPubComplete != NULL && av.verifyPubComplete[0]!='\0') {
                    if (access(av.verifyPubComplete, F_OK) != 0){ // test if file exists
                        if (verifyFailed==0){
                            LOG_INFO("(only prints once)av.verifyPubComplete " \
                              "file %s does not exist. Will not terminate program yet.\n",
                            av.verifyPubComplete);
                        }
                        verifyFailed=1;
                        break; 
                    } else {
                        LOG_INFO("av.verifyPubComplete file %s exists. " \
                                "This means all publishers completed.\n",
                            av.verifyPubComplete);
                    }     
                }
                LOG_WARNING("Verification failed for verifyStillActive setting " \
                    "expired : %llu. halting program execution due to inactivity\n",
                    av.verifyStillActive);
                av.halt++;
            } else {
                if (verifyFailed!=0) { verifyFailed=0; }
            }
        }
    } while(0);

    if (((currTime-av.lastStatTime)> av.lastStatTimeIntervalUSec) || force >0 ){
        //do_info_display(av.tv,MYBASIC,av.num_clients); - displays a cumulative update
        LOG_STDOUT("SVT_MQTT_C_STATUS,%f,%llu,%llu,%s,%llu,%llu,%llu,%f,%f,%f," \
                   "(actual msg rate,msgs since last interval,total msgs,lastmsgtime," \
                   " connection attempts,success,fail,connection time min,max,total)\n" ,
            (double)(av.tv->rate_s.nmsgs-av.lastStatMsgs)/
                ((double)(currTime-av.lastStatTime)/MICROS_PER_SECOND),
            av.tv->rate_s.nmsgs-av.lastStatMsgs,
            r->nmsgs,
            getTimeString(av.tv->rate_s.lastMsgTime,str1),
            av.connectionAttempts,
            av.connectionCounter,
            av.connectionFailure,
            av.connectionMinTime/MICROS_PER_SECOND,
            av.connectionMaxTime/MICROS_PER_SECOND,
            av.connectionSumTime/MICROS_PER_SECOND
        );
        av.lastStatMsgs=av.tv->rate_s.nmsgs;
        av.lastStatTime=currTime;
    }

    if(av.scaleTest!=0){
        // Every 60 seconds (hardcoded) below a status message is published to topic /svt/
        if (av.lastScaleTestAlertTime == 0 ) {
            av.lastScaleTestAlertTime=currTime;
        } else if ((currTime-av.lastScaleTestAlertTime ) > 60*MICROS_PER_SECOND ){
            av.lastScaleTestAlertTime=currTime;
            sprintf(pubMsg, "%s:%d:%llu:0:%f:%llu:", tv->clientId, av.num_clients,r->nmsgs,
                ((double)r->nmsgs/(((double)(r->lastMsgTime - 
                    r->firstMsgTime))/MICROS_PER_SECOND)),av.error_count);
            mqttMessage.payload = pubMsg;
            mqttMessage.payloadlen = strlen(pubMsg);
            mqttMessage.qos = 0;
            //mqttMessage.retained = av.retained_flag;
            mqttMessage.retained = 0;
            rc = wrap_MQTTClient_publishMessage(tv->mqtt_client, (char*)pubTopic,
                &mqttMessage, &tv->mqtt_token);
            LOG_INFO("Return code not checked: rc=%d for status message " \
                        "published to topic \"%s\" %d , message \"%s\" \n",
                rc, pubTopic, mqttMessage.qos , pubMsg  );
        }
    
    }   

}

int get_rand_chance(int chance_0_to_x, int x){
    static int init=0;
    static time_t t_result; 
    int r;
    if (init==0){
        time(&t_result);
        srand((unsigned int)t_result+getpid());
        init=1; 
    }
        
    r=rand(); 
    if ((r%x)<=chance_0_to_x){
        return 1;  // true - the chance succeeds
    } else { 
        return 0; // false -the chance did not succeed
    }
    //printf("rand: %d  %100= %d\n", r, r%100);
    
}


int wrap_MQTTClient_receive(MQTTClient h, char** tn, int* tl,
         MQTTClient_message **m, unsigned long to){
    int rc;
    MQTTCLIENT_NULL_CHECK
    rc=MQTTClient_receive(h,tn,tl,m,to);
    //MQTTCLIENT_ERROR_INJECT_DISCONNECT : not good here because prevents counters and stuff from updating
    return rc;
}

int wrap_MQTTClient_unsubscribe(MQTTClient h, char* t){
    MQTTCLIENT_NULL_CHECK
    return MQTTClient_unsubscribe(h, t);
}

int wrap_MQTTClient_subscribe(MQTTClient h, char* t, int q){
    MQTTCLIENT_NULL_CHECK
    return MQTTClient_subscribe(h, t, q);
}
void wrap_MQTTClient_freeMessage(MQTTClient_message **m){
    MQTTClient_freeMessage(m);
}
void wrap_MQTTClient_free(void *v){
    MQTTClient_free(v);
}
int wrap_MQTTClient_create (MQTTClient *h, char *s, char *c, int p, void *pc){
    return MQTTClient_create (h,s,c,p,pc);
}
int wrap_MQTTClient_connect (MQTTClient h, MQTTClient_connectOptions *o){
    return MQTTClient_connect (h, o);
}
int wrap_MQTTClient_disconnect (MQTTClient h, int t){
    return MQTTClient_disconnect (h, t);
}
int wrap_MQTTClient_isConnected (MQTTClient h){
    return MQTTClient_isConnected (h);
}
void wrap_MQTTClient_destroy (MQTTClient *h){
    MQTTClient_destroy (h);
}
int  wrap_MQTTClient_publishMessage (MQTTClient h, char *t, 
        MQTTClient_message *m, MQTTClient_deliveryToken *d) {
    MQTTCLIENT_NULL_CHECK
    return MQTTClient_publishMessage (h, t, m, d);
}                       
int  wrap_MQTTClient_waitForCompletion (MQTTClient h,
         MQTTClient_deliveryToken d, unsigned long t) {
    MQTTCLIENT_NULL_CHECK
    return MQTTClient_waitForCompletion (h, d,t);
}       


void do_set_client_topic(clientVariables *tv, const char * pub_or_sub){
    static char tmpbuf[MAX_BUF_SZ]=""; 
    char ssprepend[256]="";
   

    if(tv->topic[0]== '\0' || av.topicMultiTest != 0 || av.topic_buf != NULL ) {
        if( av.sharedSubMulti != 0 ) {
            if (av.sharedSubMultiCounter>=av.sharedSubMulti){
                av.sharedSubMultiCounter=0;
            }
            sprintf(ssprepend, "%s_%llu/",av.sharedSubscription, av.sharedSubMultiCounter);
            av.sharedSubMultiCounter++;
        }else if ( av.sharedSubscription[0] != '\0' ) {
            sprintf(ssprepend, "%s/",av.sharedSubscription);
        }
        if(av.topic_buf != NULL){ //read topics in from a file
            do_circular_read_line_from_buf(av.topic_buf,&av.topic_buf_ptr, &av.topic_buf_plen);
            memset(tmpbuf, '\0', MAX_BUF_SZ);
            strncpy(tmpbuf, av.topic_buf_ptr, av.topic_buf_plen);
            sprintf(tv->topic, "%s",tmpbuf);
            LOG_INFO("Topic read from a file: %s, %d\n", tv->topic, av.topic_buf_plen);
        } else if(av.topicMultiTest != 0){ // av.topicMultiTest takes precedent over av.topicChangeTest
                    CLIENT_LOG_INFO(tv,"K: %lld\n",av.topicMultiTestCounter);

            if (av.topicMultiTestCounter>=av.topicMultiTest){
                av.topicMultiTestCounter=0;
            }
            if( av.prependCN2Topic != 0){ // functions differently if using prependCN2Topic
                sprintf(tv->topic, "%s/CN%07d%s%s",
                    ssprepend,
                    (int)(av.dccIncrementer + av.topicMultiTestCounter+av.topicMultiTestIncr),
                    tv->topicUSPrepend,av.topic);
                av.topicMultiTestCounter++;
            } else {
                sprintf(tv->topic, "%s%s%s%s/%llu", ssprepend, 
                tv->topicCNPrepend, tv->topicUSPrepend,av.topic, av.topicMultiTestCounter+av.topicMultiTestIncr);
                av.topicMultiTestCounter++;
            }
        } else if(av.topicChangeTest != 0){
                  CLIENT_LOG_INFO(tv,"A\n");
            if(tv->topic[0]== '\0' ){
                sprintf(tv->topic, "%s%s%s%s/%d",  ssprepend, tv->topicCNPrepend,
                     tv->topicUSPrepend,av.topic, tv->array_id);
            }
        } else if (pub_or_sub[0]=='p' ) {
                    CLIENT_LOG_INFO(tv,"B\n");
            if(av.scaleTest != 0){
                if(tv->topic[0]== '\0' ){
                    sprintf(tv->topic, "%s%s%s%s%s", ssprepend,
                     tv->topicCNPrepend, tv->topicUSPrepend,av.topic, tv->clientId);
                }    
            }else {
                if(tv->topic[0]== '\0' ){
                    sprintf(tv->topic, "%s%s%s%s", ssprepend,
                         tv->topicCNPrepend, tv->topicUSPrepend, av.topic);
                }
            }
    
        } else if (pub_or_sub[0]=='s' ) {
                    CLIENT_LOG_INFO(tv,"C\n");
            if(av.scaleTest != 0){
                HALT_ERROR("scaleTest not supported yet for " \
                            "subscribe action, only publish action\n");
            }else {
                if(tv->topic[0]== '\0' ){
                    sprintf(tv->topic, "%s%s%s%s", ssprepend, 
                         tv->topicCNPrepend, tv->topicUSPrepend,av.topic);
                }
            }
    
        } else {
            HALT_ERROR("Internal code bug with %s\n", pub_or_sub);
        }
    } // else it was already set and is not a dynamic topic
    if(tv->topic[0] == '\0' ){
        //#HALT_ERROR("Internal code bug topic setting logic error\n");
            LOG_WARNING("topic is empty string\n");
    }
}

int do_shm_cleanup(void){
    int rc=0, rc2=0;
    if (av.shmfd != 0){ 
        rc2= close(av.shmfd);
        if (rc2!=0){
            //LOG_WARNING("Non-zero rc from close: %d\n", rc2);
        }
        av.shmfd=0;
    }        
    if (av.shm_ptr != NULL){ 
        rc= shm_unlink(av.sharedMemoryKey);
        if (rc!=0){
            //LOG_WARNING("Non-zero rc from shm_unlink: %d\n", rc);
        }
        av.shm_ptr=NULL;
    } 
    av.sharedMemoryInit=0;
    return rc+rc2;
}

int do_shm_setup(void){
    av.shmfd = shm_open(av.sharedMemoryKey, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (av.shmfd == -1) {
        /* Handle error */;
        HALT_ERROR("Could not perform shm_open \n");
    }

    if (ftruncate(av.shmfd, sizeof(struct shm_region)) == -1){
        /* Handle error */;
        HALT_ERROR("Could not perform ftruncate \n");
    }

    /* Map shared memory object */
    av.shm_ptr = mmap(NULL, sizeof(struct shm_region),
       PROT_READ | PROT_WRITE, MAP_SHARED, av.shmfd, 0);
    if (av.shm_ptr == MAP_FAILED){
        /* Handle error */;
        HALT_ERROR("Could not mmap av.shm_ptr \n");
    }
    if (av.shm_ptr->initialized!= 0xFABEBABE ){
        if (strcmp(av.action, SUBSCRIBE) == 0){
            sem_init(&(av.shm_ptr->semaphore), 1, 1);
            //sem_post(&(av.shm_ptr->semaphore)); /* unlock the semaphore*/
        }
        av.shm_ptr->initialized = 0xFABEBABE ;
    }

    av.sharedMemoryInit=1;
    return 0;
}

void do_shm_read(struct shm_rw_region *v, int area){
    if (av.sharedMemoryInit==0){ do_shm_setup(); }
    if (  av.shm_ptr ) {
        if (area==0){
            memcpy(v, &av.shm_ptr->subrw, sizeof(struct shm_rw_region));
        }else if (area==1){
            memcpy(v, &av.shm_ptr->pubrw, sizeof(struct shm_rw_region));
        } else {
            HALT_ERROR("shm: invalid area: %d\n",area);
        }
    } else {
        return ;
    }
    return ;
}
void do_shm_write(struct shm_rw_region *v, int area){
    int rc;
    if (av.sharedMemoryInit==0){ do_shm_setup(); }
    if (  av.shm_ptr ) {
        rc=sem_wait(&(av.shm_ptr->semaphore)); /* lock the semaphore*/
        if (rc!=0){ LOG_WARNING("%d rc from rc=sem_wait\n", rc);}
        if (area==0){
            memcpy(&av.shm_ptr->subrw, v, sizeof(struct shm_rw_region));
            //LOG_INFO(" av.shm_ptr->subrw.count = %llu \n", av.shm_ptr->subrw.count);
            //LOG_INFO(" av.shm_ptr->subrw.rate = %llu \n", av.shm_ptr->subrw.rate);
            //LOG_INFO(" v->count = %llu \n", v->count);
            //LOG_INFO(" v->rate = %llu \n", v->rate);
        } else if (area==1){
            memcpy(&av.shm_ptr->pubrw, v, sizeof(struct shm_rw_region));
        } else {
            HALT_ERROR("shm: invalid area: %d\n",area);
        }
        rc=sem_post(&(av.shm_ptr->semaphore)); /* unlock the semaphore*/
        if (rc!=0){ LOG_WARNING("%d rc from rc=sem_post\n", rc);}
    } else {
        return ;
    }
    return ;
}

void do_load_file(char *inputfile, char **buf, int *len  ){
    FILE *fp;
    struct stat st;
    int rc;
    
    LOG_INFO("User wants to use the contents of the input file: " \
             "%s as the message. Loading file.\n", inputfile);
    stat(inputfile,&st);
    *len=st.st_size;
    LOG_INFO("len set to %d\n",*len );
    *buf=(char*)malloc(st.st_size+1);
    memset(*buf, '\0', st.st_size+1);
    fp=fopen(inputfile, "r");
    if(fp!=NULL){
        rc=fread(*buf, 1, st.st_size, fp);
        if (rc!= st.st_size){
            HALT_ERROR ("Could not read %d bytes from input file:%s, " \
                "instead only read %d\n",(int)st.st_size, inputfile, rc);
        }
    } else {
        HALT_ERROR ("Problem reading input file %s\n",inputfile);
    }
    fclose (fp);
    LOG_INFO("Done Loading file %s. Closed file\n", inputfile);
}

// do_circular_read_line_from_buf read lines from buffer one line 
// at a time and starts over and repeats if hit end of buffer.
void do_circular_read_line_from_buf(char *buf, char **buf_ptr, int *out_ptr_len){
     char *newp;
     if (*buf_ptr==NULL){ /* initialize to the front of the line*/
        *buf_ptr=buf;
     } else { /* move forward to the next line*/
        newp=strstr(*buf_ptr, "\n");
        if (newp) {
            newp++;
            if (*newp != '\0' ) {
                *buf_ptr=newp;
                //printf("A)set buf_ptr to \"%s\"\n", newp);
            } else {
                //printf("A2)reset the process\n");
                // reset the process
                *buf_ptr=buf;
            }
        } else {
            //printf("A)reset the process\n");
            // reset the process
            *buf_ptr=buf;
        }

     }
            //printf("hola"  ); fflush(stdout);

     newp=strstr(*buf_ptr, "\n");

     if (newp) {
        *out_ptr_len=newp-*buf_ptr;
        //printf("B1)out_ptr_len=%d = %d - %d\n", *out_ptr_len, newp, *buf_ptr );
     } else {
            //printf("hi buf_ptr is %s\n",*buf_ptr  ); fflush(stdout);
            //printf("hi **buf_ptr is %c\n",**buf_ptr  ); fflush(stdout);
        if ( **buf_ptr != '\0' ) {
            //printf("B2)out_ptr_len=%d = %d - (%d )\n", *out_ptr_len, strlen(buf), *buf_ptr-buf  ); fflush(stdout);
            *out_ptr_len=strlen(buf)-(*buf_ptr-buf);
        } else {
            // reset the process
            //printf("B3)reset the process\n");
            *buf_ptr=buf;
        }
     }

}

