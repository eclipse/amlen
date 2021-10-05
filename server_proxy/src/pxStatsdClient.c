/*
 * Copyright (c) 2016-2021 Contributors to the Eclipse Foundation
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

#define TRACE_COMP System

#include <ismutil.h>
#ifndef HAS_BRIDGE
#include <statsd-client.h>
#include <string.h>
#include <iotrest.h>
#include <pxmqtt.h>
#include <pxtcp.h>
#include <auth.h>
#include <alloca.h>
#include <tenant.h>
#define NO_KAFKA_POBJ
#include <pxkafka.h>
#ifndef NO_PXACT
#include <pxactivity.h>
#endif

#define MAX_BUF_SIZE 64*1024
static char     updateBuffer[MAX_BUF_SIZE];
static uint8_t  lastStatIndex = 0;

static px_auth_stats_t  authStats[2];
static px_http_stats_t  httpStats[2];
static px_mqtt_stats_t  mqttStats[2];
static px_tcp_stats_t   tcpStats[2];
static px_msgsize_stats_t   mqttMsgSizeStats[2];
static px_msgsize_stats_t   httpMsgSizeStats[2];
static ism_MemoryStatistics_t memoryStats[2];;
static px_server_stats_t * serverStats = NULL;
static int serverStatsAllocated = 0;
static px_mux_stats_t * muxStats = NULL;
static int muxStatsAllocated = 0;

extern int g_useKafkaIMMessaging;
static px_kafka_messaging_stat_t   kafkaIMMessagingStats[2];
static px_kafka_messaging_stat_t   mhubMessagingStats[2];

#ifndef NO_PXACT
static uint8_t  lastPXActStatIndex = 0;  //PXACT
static double  lastPXActUpdateTime = 0.0;
#endif

static uint8_t  lastIMMessagingStatIndex = 0;
static double  lastIMMessagingUpdateTime = 0.0;

extern int g_mhubEnabled;
static uint8_t  lastMHUBMessagingStatIndex = 0;
//static double  lastMHUBMessagingUpdateTime = 0.0;
extern int ism_mhub_getMessagingStats(px_kafka_messaging_stat_t * stats) ;


extern int ism_proxy_getHTTPMsgSizeStats(px_msgsize_stats_t * stats) ;
int ism_proxy_getMQTTMsgSizeStats(px_msgsize_stats_t* stats) ;
extern int ism_proxy_getKafkaConnectionStats(px_kafka_messaging_stat_t * stats, const char * kafkaTopic , int * pCount);
int IMMessagingConnectionStatsAllocated =0;
px_kafka_messaging_stat_t * immessaging_conn_stats = NULL;
extern int g_kafkaIMConnDetailStatsEnabled ;
extern uint64_t g_MaxProduceLatencyMillis;

static double           lastUpdateTime = 0.0;


static statsd_link      *pStatsdLink = NULL;

static inline void encodeIP(const char *src, char * dst) {
    while(*src) {
        *dst = (*src == '.') ? '_' : *src;
        src++;
        dst++;
    }
    *dst = '\0';
}

/*Update MUX Stats*/
static void updateMUXStats(void) {
	int count = muxStatsAllocated;
	uint64_t min = 0;
	uint64_t max = 0;
	uint64_t avg = 0;
	uint64_t totalVC = 0;
	uint64_t value=0;

	char prepBuff[1024];
	char counterName[1024];
	int j;
	uint64_t totalPhysicalConnections = 0;
	uint64_t vcConnectionsTotal=0;
	uint64_t physicalConnectionsTotal=0;


	while(ism_proxy_getMuxStats(muxStats, &count)) {
		muxStatsAllocated = count + 32;
		void * statptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_mux_stats,2),muxStats, muxStatsAllocated*sizeof(px_mux_stats_t));
		if(!statptr) {
			ism_common_free(ism_memory_proxy_mux_stats,muxStats);
			muxStats=NULL;
			muxStatsAllocated = 0;
			return;
		}
		muxStats = statptr;
		count = muxStatsAllocated;
	}
	if(count) {
		char sendBuffer[MAX_BUF_SIZE];
		int i;
		sendBuffer[0] = '\0';
		min = muxStats[0].virtualConnectionsTotal;
		for(i = 0; i < count; i++) {

			vcConnectionsTotal = muxStats[i].virtualConnectionsTotal;
			snprintf(counterName, sizeof(counterName),"mux.iop%d.%s", i, "virtualConnectionsTotal");
			value = (muxStats[i].virtualConnectionsTotal);
			statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
			strcat(sendBuffer,prepBuff);

			physicalConnectionsTotal = muxStats[i].physicalConnectionsTotal;
			snprintf(counterName, sizeof(counterName),"mux.iop%d.%s", i, "physicalConnectionsTotal");
			value = (muxStats[i].physicalConnectionsTotal);
			statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
			strcat(sendBuffer,prepBuff);

			j = strlen(sendBuffer) - 1;
			if(j > 16384) {
				sendBuffer[j] = '\0';
				statsd_send(pStatsdLink, sendBuffer);
				sendBuffer[0] = '\0';
			}

			totalVC += vcConnectionsTotal;
			totalPhysicalConnections+=physicalConnectionsTotal;
			if(vcConnectionsTotal>max){
				max = vcConnectionsTotal;
			}
			if(vcConnectionsTotal<min){
				min = vcConnectionsTotal;
			}

			TRACE(6, "Proxy MUX stats collected: iop=%d virtualConnectionsTotal=%llu physicalConnectionsTotal=%llu\n",
					i, (ULL)muxStats[i].virtualConnectionsTotal, (ULL)muxStats[i].physicalConnectionsTotal);


		}


		avg = totalVC/count;

		snprintf(counterName, sizeof(counterName),"mux.virtualConnectionsMinimum");
		value = min;
		statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"mux.virtualConnectionsMaximum");
		value = max;
		statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"mux.virtualConnectionsAvergage");
		value = avg;
		statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"mux.physicalConnectionsTotal");
		value = totalPhysicalConnections;
		statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"mux.TotalVirtualConnections");
		value = totalVC;
		statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);


		if(sendBuffer[0]){
			i = strlen(sendBuffer) - 1;
			sendBuffer[i] = '\0';
			statsd_send(pStatsdLink, sendBuffer);
		}

	}

}

static void updateServersStats(void) {
    int count = serverStatsAllocated;
    while(ism_proxy_getServersStats(serverStats, &count)) {
        serverStatsAllocated = count + 32;
        void * statptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_stats,1),serverStats, serverStatsAllocated*sizeof(px_server_stats_t));
        if(!statptr) {
        		ism_common_free(ism_memory_proxy_stats,serverStats);
        		serverStats=NULL;
            serverStatsAllocated = 0;
            return;
        }
        	serverStats=statptr;
        count = serverStatsAllocated;
    }
    if(count) {
        char sendBuffer[MAX_BUF_SIZE];
        int i;
        sendBuffer[0] = '\0';
        for(i = 0; i < count; i++) {
            char prepBuff[1024];
            char counterName[1024];
            char ip[512];
            int j;
            uint64_t value;
            snprintf(counterName, sizeof(counterName),"Server.%s.%s", serverStats[i].name, "monitorConnectionStatus");
            value =  ((serverStats[i].connectionState & 0xf0) >> 4);
            statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
            strcat(sendBuffer,prepBuff);
            value =  (serverStats[i].connectionState & 0x0f);
            snprintf(counterName, sizeof(counterName),"Server.%s.%s", serverStats[i].name, "mqttConnectionStatus");
            if(value != 0x0f) {
                statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
            } else {
                statsd_prepare(pStatsdLink, counterName, -1, "g", 1.0, prepBuff, sizeof(prepBuff),1);
            }
            strcat(sendBuffer,prepBuff);
            snprintf(counterName, sizeof(counterName),"Server.%s.%s", serverStats[i].name, "usePrimary");
            value = (serverStats[i].usePrimary);
            statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
            strcat(sendBuffer,prepBuff);
            snprintf(counterName, sizeof(counterName),"Server.%s.%s", serverStats[i].name, "pendingHTTPRequests");
            value = (serverStats[i].pendingHTTPRequests);
            statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
            strcat(sendBuffer,prepBuff);
            for(j = 0; j < serverStats[i].primaryCount; j++) {
                if (serverStats[i].primaryIPs[j]) {
                    encodeIP(serverStats[i].primaryIPs[j], ip);
                    snprintf(counterName, sizeof(counterName),"Server.%s.primary.%s_%d.useCount", serverStats[i].name,
                            ip, serverStats[i].port);
                    value = serverStats[i].primaryUseCount[j];
                    statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
                    strcat(sendBuffer,prepBuff);
                }
            }
            for(j = 0; j < serverStats[i].backupCount; j++) {
                if (serverStats[i].backupIPs[j]) {
                    encodeIP(serverStats[i].backupIPs[j], ip);
                    snprintf(counterName, sizeof(counterName),"Server.%s.backup.%s_%d.useCount", serverStats[i].name,
                            ip, serverStats[i].port);
                    value = serverStats[i].backupUseCount[j];
                    statsd_prepare(pStatsdLink, counterName, value, "g", 1.0, prepBuff, sizeof(prepBuff),1);
                    strcat(sendBuffer,prepBuff);
                }
            }
            j = strlen(sendBuffer) - 1;
            if(j > 16384) {
                sendBuffer[j] = '\0';
                statsd_send(pStatsdLink, sendBuffer);
                sendBuffer[0] = '\0';
            }
        }
        if(sendBuffer[0]){
            i = strlen(sendBuffer) - 1;
            sendBuffer[i] = '\0';
            statsd_send(pStatsdLink, sendBuffer);
        }
    }
}

static void updateIMMessagingConnectionStats(double sampleRate) {
    int count = 0;
    uint64_t topicTotalSent=0;
    uint64_t topicTotalReSent=0;
    uint64_t topicTotalReceived=0;
    uint64_t topicTotalPending=0;
    uint64_t topicTotalBytesReceived=0;
    uint64_t topicTotalBytesSent=0;
    uint64_t topicTotalBytesReSent=0;
    uint64_t topicTotalProduceBatchCount=0;
    uint64_t topicTotalBatchProduceAckReceivedCount=0;
    uint64_t topicTotalBatchProduceAckErrorCount=0;
    uint64_t topicTotalBatchReProduceCount=0;


    char counterName[1024];
    char prepBuff[1024];
    char * topic = NULL;

    	if(!g_kafkaIMConnDetailStatsEnabled){
    		TRACE(9, "Kafka Connection Stats Disabled\n");
    		return;
    	}


    while(ism_proxy_getKafkaConnectionStats(immessaging_conn_stats, NULL, &count)) {
    		IMMessagingConnectionStatsAllocated = count + 32;
    		void * statptr = ism_common_realloc(ISM_MEM_PROBE(ism_memory_proxy_stats,2),immessaging_conn_stats, IMMessagingConnectionStatsAllocated*sizeof(px_kafka_messaging_stat_t));
        if(!statptr) {
        		ism_common_free(ism_memory_proxy_stats,immessaging_conn_stats);
        		immessaging_conn_stats=NULL;
        		IMMessagingConnectionStatsAllocated = 0;
            return;
        }
        immessaging_conn_stats = statptr;
        count = IMMessagingConnectionStatsAllocated;
    }

    if(count) {
        char sendBuffer[MAX_BUF_SIZE];
        int i;
        sendBuffer[0] = '\0';
        for(i = 0; i < count; i++) {

            int j;
            uint64_t value;
            topic = immessaging_conn_stats[i].topic;

            snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s",topic,
					immessaging_conn_stats[i].partid, "connected");
		   value =  immessaging_conn_stats[i].connected;
		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

            //immessaging_conn_stats[i].kakfaC2PMsgsTotalReceived
            snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
            			immessaging_conn_stats[i].partid, "C2PMsgsTotalReceived");

            value =  immessaging_conn_stats[i].kakfaC2PMsgsTotalReceived;
            topicTotalReceived+=value;

            statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
            strcat(sendBuffer,prepBuff);

            snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "C2PMsgsTotalSent");

		   value =  immessaging_conn_stats[i].kakfaC2PMsgsTotalSent;
		   topicTotalSent+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
		  					immessaging_conn_stats[i].partid, "C2PMsgsTotalReSent");

		   value =  immessaging_conn_stats[i].kakfaC2PMsgsTotalReSent;
		   topicTotalReSent+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);


		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "C2PMsgsTotalPending");

		   value =  immessaging_conn_stats[i].kakfaTotalPendingMsgsCount;
		   topicTotalPending+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "C2PBytesTotalReceived");

		   value =  immessaging_conn_stats[i].kakfaC2PBytesTotalReceived;
		   topicTotalBytesReceived+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "C2PBytesTotalSent");

		   value =  immessaging_conn_stats[i].kakfaC2PBytesTotalSent;
		   topicTotalBytesSent+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "C2PBytesTotalReSent");

		   value =  immessaging_conn_stats[i].kakfaC2PBytesTotalReSent;
		   topicTotalBytesReSent+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "BatchProduceCount");

		   value =  immessaging_conn_stats[i].kakfaTotalBatchProduceCount;
		   topicTotalProduceBatchCount+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "BatchReProduceCount");

		   value =  immessaging_conn_stats[i].kakfaTotalBatchReProduceCount;
		   topicTotalBatchReProduceCount+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "BatchProduceAckReceivedCount");

		   value =  immessaging_conn_stats[i].kakfaTotalBatchProduceAckReceivedCount;
		   topicTotalBatchProduceAckReceivedCount+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
					immessaging_conn_stats[i].partid, "BatchProduceAckErrorCount");

		   value =  immessaging_conn_stats[i].kakfaTotalBatchProduceAckErrorCount;
		   topicTotalBatchProduceAckErrorCount+=value;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);

		   snprintf(counterName, sizeof(counterName),"kafka.%s.part%d.%s", topic,
		   					immessaging_conn_stats[i].partid, "KafkaProduceLatencyMS");
		   value =  immessaging_conn_stats[i].kafkaProduceLatencyMS;

		   statsd_prepare(pStatsdLink, counterName, value, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		   strcat(sendBuffer,prepBuff);



            j = strlen(sendBuffer) - 1;
            if(j > 16384) {
                sendBuffer[j] = '\0';
                statsd_send(pStatsdLink, sendBuffer);
                sendBuffer[0] = '\0';
            }
        }

        snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PMsgsTotalReceived");
        statsd_prepare(pStatsdLink, counterName, topicTotalReceived, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
        strcat(sendBuffer,prepBuff);

        snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PMsgsTotalSent");
		statsd_prepare(pStatsdLink, counterName, topicTotalSent, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PMsgsTotalReSent");
		statsd_prepare(pStatsdLink, counterName, topicTotalReSent, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);


		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PMsgsTotalPending");
		statsd_prepare(pStatsdLink, counterName, topicTotalPending, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PBytesTotalReceived");
		statsd_prepare(pStatsdLink, counterName, topicTotalBytesReceived, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PBytesTotalSent");
		statsd_prepare(pStatsdLink, counterName, topicTotalBytesSent, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "C2PBytesTotalReSent");
		statsd_prepare(pStatsdLink, counterName, topicTotalBytesReSent, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "BatchProduceCount");
		statsd_prepare(pStatsdLink, counterName, topicTotalProduceBatchCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "BatchReProduceCount");
		statsd_prepare(pStatsdLink, counterName, topicTotalBatchReProduceCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "BatchProduceAckReceivedCount");
		statsd_prepare(pStatsdLink, counterName, topicTotalBatchProduceAckReceivedCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		snprintf(counterName, sizeof(counterName),"kafka.%s.%s", topic, "BatchProduceAckErrorCount");
		statsd_prepare(pStatsdLink, counterName, topicTotalBatchProduceAckErrorCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

        if(sendBuffer[0]){
            i = strlen(sendBuffer) - 1;
            sendBuffer[i] = '\0';
            statsd_send(pStatsdLink, sendBuffer);
        }
    }
}

//IM Messaging Stats
static void updateIMMessagingStats(void) {
    char prepBuff[2048];
    char sendBuffer[MAX_BUF_SIZE];


    /*If Kafka IM Messaging is Enabled. Send its stats also*/
    	if(g_useKafkaIMMessaging){
		double currTime = ism_common_readTSC();
		double sampleRate = currTime - lastIMMessagingUpdateTime;
		uint8_t currStatIndex = !lastIMMessagingStatIndex;
		sendBuffer[0] = '\0';
		ism_proxy_getKafkaIMMessagingStats(&kafkaIMMessagingStats[currStatIndex]);
		uint64_t kakfaC2PMsgsTotalReceived = kafkaIMMessagingStats[currStatIndex].kakfaC2PMsgsTotalReceived - kafkaIMMessagingStats[lastStatIndex].kakfaC2PMsgsTotalReceived;
		uint64_t kakfaC2PBytesTotalReceived = kafkaIMMessagingStats[currStatIndex].kakfaC2PBytesTotalReceived - kafkaIMMessagingStats[lastStatIndex].kakfaC2PBytesTotalReceived;
		uint64_t kakfaC2PMsgsTotalSent = kafkaIMMessagingStats[currStatIndex].kakfaC2PMsgsTotalSent - kafkaIMMessagingStats[lastStatIndex].kakfaC2PMsgsTotalSent;
		uint64_t kakfaC2PMsgsTotalReSent = kafkaIMMessagingStats[currStatIndex].kakfaC2PMsgsTotalReSent - kafkaIMMessagingStats[lastStatIndex].kakfaC2PMsgsTotalReSent;
		uint64_t kakfaC2PBytesTotalSent = kafkaIMMessagingStats[currStatIndex].kakfaC2PBytesTotalSent - kafkaIMMessagingStats[lastStatIndex].kakfaC2PBytesTotalSent;
		uint64_t kakfaC2PBytesTotalReSent = kafkaIMMessagingStats[currStatIndex].kakfaC2PBytesTotalReSent - kafkaIMMessagingStats[lastStatIndex].kakfaC2PBytesTotalReSent;

		 TRACE(6, "Proxy Kafka IM Messaging Stats collected: kakfaC2PMsgsTotalReceived=%llu kakfaC2PBytesTotalReceived=%llu kakfaC2PMsgsTotalSent=%llu kakfaC2PMsgsTotalReSent=%llu kakfaC2PBytesTotalSent=%llu kakfaC2PBytesTotalSent=%llu kakfaTotalPendingMsgsCount=%llu\n",
						   (ULL)kakfaC2PMsgsTotalReceived, (ULL)kakfaC2PBytesTotalReceived, (ULL)kakfaC2PMsgsTotalSent, (ULL)kakfaC2PMsgsTotalReSent, (ULL)kakfaC2PBytesTotalSent, (ULL)kakfaC2PBytesTotalReSent,
						   (ULL)kafkaIMMessagingStats[currStatIndex].kakfaTotalPendingMsgsCount);

		statsd_prepare(pStatsdLink, "kafka.", kakfaC2PMsgsTotalReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.C2PBytesTotalReceived", kakfaC2PBytesTotalReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.C2PMsgsTotalSent", kakfaC2PMsgsTotalSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.C2PMsgsTotalReSent", kakfaC2PMsgsTotalReSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.C2PBytesTotalSent", kakfaC2PBytesTotalSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.C2PBytesTotalReSent", kakfaC2PBytesTotalReSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.TotalPendingMsgsCount",  kafkaIMMessagingStats[currStatIndex].kakfaTotalPendingMsgsCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.TotalBatchProduceAckReceivedCount",  kafkaIMMessagingStats[currStatIndex].kakfaTotalBatchProduceAckReceivedCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.BatchProduceAckErrorCount",  kafkaIMMessagingStats[currStatIndex].kakfaTotalBatchProduceAckErrorCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.BatchProduceCount",  kafkaIMMessagingStats[currStatIndex].kakfaTotalBatchProduceCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "kafka.BatchReProduceCount",  kafkaIMMessagingStats[currStatIndex].kakfaTotalBatchReProduceCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		statsd_send(pStatsdLink, sendBuffer);
		lastIMMessagingUpdateTime = currTime;
		lastIMMessagingStatIndex = currStatIndex;

		updateIMMessagingConnectionStats(sampleRate);
    	}else{

    	    	TRACE(9, "Kafka Connection Stats Disabled\n");

    	}
}

//IM Messaging Stats
static void updateMHUBMessagingStats(double sampleRate) {
    char prepBuff[2048];
    char sendBuffer[MAX_BUF_SIZE];


    /*If Kafka IM Messaging is Enabled. Send its stats also*/
    	if(g_mhubEnabled){
		//double currTime = ism_common_readTSC();
		//double sampleRate = currTime - lastMHUBMessagingUpdateTime;
		uint8_t currStatIndex = !lastMHUBMessagingStatIndex;
		sendBuffer[0] = '\0';
		ism_mhub_getMessagingStats(&mhubMessagingStats[currStatIndex]);
		uint64_t kakfaC2PMsgsTotalReceived = mhubMessagingStats[currStatIndex].kakfaC2PMsgsTotalReceived - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaC2PMsgsTotalReceived;
		uint64_t kafkaC2PMsgsTotalFiltered = mhubMessagingStats[currStatIndex].kafkaC2PMsgsTotalFiltered - mhubMessagingStats[lastMHUBMessagingStatIndex].kafkaC2PMsgsTotalFiltered;
		uint64_t kakfaC2PBytesTotalReceived = mhubMessagingStats[currStatIndex].kakfaC2PBytesTotalReceived - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaC2PBytesTotalReceived;
		uint64_t kakfaC2PMsgsTotalSent = mhubMessagingStats[currStatIndex].kakfaC2PMsgsTotalSent - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaC2PMsgsTotalSent;
		uint64_t kakfaC2PMsgsTotalReSent = mhubMessagingStats[currStatIndex].kakfaC2PMsgsTotalReSent - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaC2PMsgsTotalReSent;
		uint64_t kakfaC2PBytesTotalSent = mhubMessagingStats[currStatIndex].kakfaC2PBytesTotalSent - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaC2PBytesTotalSent;
		uint64_t kakfaC2PBytesTotalReSent = mhubMessagingStats[currStatIndex].kakfaC2PBytesTotalReSent - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaC2PBytesTotalReSent;
		uint64_t kafkaMsgInBatchWaitTimeMS = mhubMessagingStats[currStatIndex].kafkaMsgInBatchWaitTimeMS ;
		uint64_t kafkaBatchProduceCount = mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceCount - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaTotalBatchProduceCount;
		uint64_t kafkaBatchReProduceCount = mhubMessagingStats[currStatIndex].kakfaTotalBatchReProduceCount - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaTotalBatchReProduceCount;
		uint64_t kafkaBatchProduceAckErrorCount = mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceAckErrorCount - mhubMessagingStats[lastMHUBMessagingStatIndex].kakfaTotalBatchProduceAckErrorCount;
		uint64_t kafkaPartitionChangedCount = mhubMessagingStats[currStatIndex].kafkaPartitionChangedCount - mhubMessagingStats[lastMHUBMessagingStatIndex].kafkaPartitionChangedCount;
		uint64_t kafkaPartitionMsgsTransferredCount = mhubMessagingStats[currStatIndex].kafkaPartitionMsgsTransferredCount - mhubMessagingStats[lastMHUBMessagingStatIndex].kafkaPartitionMsgsTransferredCount;

		//Calculate Average of Throttle Time
		uint64_t avgMsgInBatchWaitTimeMillis=0;
		if(kafkaBatchProduceCount){
			avgMsgInBatchWaitTimeMillis = kafkaMsgInBatchWaitTimeMS / kafkaBatchProduceCount;
		}
		uint64_t avgProduceLatencyMS =0;
		uint64_t avgThrottleTimeMS =0;
		if(mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceAckReceivedCount>0){
				avgProduceLatencyMS = (uint64_t) (ceil( mhubMessagingStats[currStatIndex].kafkaTotalProduceLatencyMS /  mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceAckReceivedCount));
				avgThrottleTimeMS=(uint64_t) (ceil( mhubMessagingStats[currStatIndex].kafkaProduceTotalThrottleTimeMS /  mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceAckReceivedCount));
		}

		uint64_t avgMsgsInProduceBatch =0;
		if(kafkaBatchProduceCount > 0){
			avgMsgsInProduceBatch = (uint64_t) (ceil( kakfaC2PMsgsTotalSent /  kafkaBatchProduceCount));
		}

		uint64_t avgBytesInProduceBatch =0;
		if(kafkaBatchProduceCount > 0 && kakfaC2PBytesTotalSent>0){
			avgBytesInProduceBatch = (uint64_t) (ceil( kakfaC2PBytesTotalSent /  kafkaBatchProduceCount));
		}


		TRACE(6, "Proxy MHUB Messaging Stats collected: kakfaC2PMsgsTotalReceived=%llu kakfaC2PBytesTotalReceived=%llu kakfaC2PMsgsTotalSent=%llu kakfaC2PMsgsTotalReSent=%llu kakfaC2PBytesTotalSent=%llu "
				"kakfaC2PBytesTotalReSent=%llu kakfaTotalPendingMsgsCount=%llu BatchProduceLatencyMS=%llu BatchProduceMaxLatency=%llu avgThrottleTimeMS=%llu MsgInBatchWaitTimeMS=%llu currBatchProduceCount=%llu "
				"AvgMsgInBatchWaitTimeMS=%llu MaxMsgInBatchWaitTimeMS=%llu avgMsgsInProduceBatch=%llu avgBytesInProduceBatch=%llu BatchProduceAckReceivedCount=%llu PartitionChangedCount=%llu PartitionMsgsTransferredCount=%llu"
				"kafkaC2PMsgsTotalFiltered=%llu\n",
						   (ULL)kakfaC2PMsgsTotalReceived, (ULL)kakfaC2PBytesTotalReceived, (ULL)kakfaC2PMsgsTotalSent, (ULL)kakfaC2PMsgsTotalReSent, (ULL)kakfaC2PBytesTotalSent, (ULL)kakfaC2PBytesTotalReSent,
						   (ULL)kafkaIMMessagingStats[currStatIndex].kakfaTotalPendingMsgsCount, (ULL) mhubMessagingStats[currStatIndex].kafkaProduceLatencyMS,(ULL) mhubMessagingStats[currStatIndex].kafkaProduceMaxLatencyMS,
						   (ULL) avgThrottleTimeMS, (ULL)kafkaMsgInBatchWaitTimeMS, (ULL)kafkaBatchProduceCount,  (ULL)avgMsgInBatchWaitTimeMillis, (ULL) mhubMessagingStats[currStatIndex].kafkaMsgInBatchMaxWaitTimeMS,
						   (ULL) avgMsgsInProduceBatch,(ULL) avgBytesInProduceBatch, (ULL) mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceAckReceivedCount,
						   (ULL) kafkaPartitionChangedCount, (ULL)kafkaPartitionMsgsTransferredCount, (ULL)kafkaC2PMsgsTotalFiltered);

		statsd_prepare(pStatsdLink, "mhub.C2PMsgsTotalReceived", kakfaC2PMsgsTotalReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.C2PMsgsTotalFiltered", kafkaC2PMsgsTotalFiltered, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.C2PBytesTotalReceived", kakfaC2PBytesTotalReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.C2PMsgsTotalSent", kakfaC2PMsgsTotalSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.C2PMsgsTotalReSent", kakfaC2PMsgsTotalReSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.C2PBytesTotalSent", kakfaC2PBytesTotalSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.C2PBytesTotalReSent", kakfaC2PBytesTotalReSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceCount",  kafkaBatchProduceCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceAckErrorCount",  kafkaBatchProduceAckErrorCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceAckReceivedCount",  mhubMessagingStats[currStatIndex].kakfaTotalBatchProduceAckReceivedCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchReProduceCount",  kafkaBatchReProduceCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.PartitionChangedCount",  kafkaPartitionChangedCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.PartitionMsgsTransferredCount",  kafkaPartitionMsgsTransferredCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);

		//Send in as Gauge
		statsd_prepare(pStatsdLink, "mhub.TotalPendingMsgsCount",  mhubMessagingStats[currStatIndex].kakfaTotalPendingMsgsCount, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.ProduceAvgThrottleTimeMS",  avgThrottleTimeMS,  "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.MsgInBatchAvgWaitTimeMS",  avgMsgInBatchWaitTimeMillis, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceAvgLatencyMS",  avgProduceLatencyMS, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceAvgMsgs",  avgMsgsInProduceBatch, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceAvgBytes",  avgBytesInProduceBatch, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.BatchProduceMaxLatencyMS",  mhubMessagingStats[currStatIndex].kafkaProduceMaxLatencyMS, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);
		statsd_prepare(pStatsdLink, "mhub.MsgInBatchMaxWaitTimeMS",  mhubMessagingStats[currStatIndex].kafkaMsgInBatchMaxWaitTimeMS, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
		strcat(sendBuffer,prepBuff);


		statsd_send(pStatsdLink, sendBuffer);
		//lastMHUBMessagingUpdateTime = currTime;
		lastMHUBMessagingStatIndex = currStatIndex;


    	}else{

    	    	TRACE(9, "MMHUB Stats Disabled\n");

    	}
}

//PXACT
static void updatePXActStats(void) {
#ifndef NO_PXACT
    char prepBuff[2048];
    char sendBuffer[MAX_BUF_SIZE];
    double currTime = ism_common_readTSC();
    double sampleRate = currTime - lastPXActUpdateTime;
    uint8_t currPXActStatIndex = !lastPXActStatIndex;

    ismPXACT_Stats_t currStats;
    ism_pxactivity_Stats_get(&currStats);

    TRACE(6, "Proxy client-activity-connectivity stats collected: state=%ld "
            "num_db_read=%ld num_db_insert=%ld num_db_update=%ld "
            "num_db_bulk_update=%ld num_db_bulk=%ld num_db_delete=%ld "
            "num_db_error=%ld avg_db_read_latency_ms=%ld "
    		"avg_db_update_latency_ms=%ld avg_db_bulk_update_latency_ms=%ld "
            "num_activity=%ld num_connectivity=%ld "
            "num_clients=%ld num_new_clients=%ld num_evict_clients=%ld memory_bytes=%ld "
            "avg_conflation_delay_ms=%ld avg_conflation_factor=%ld\n",
            currStats.activity_tracking_state,
            currStats.num_db_read, currStats.num_db_insert, currStats.num_db_update,
            currStats.num_db_bulk_update, currStats.num_db_bulk, currStats.num_db_delete,
            currStats.num_db_error, currStats.avg_db_read_latency_ms,
			currStats.avg_db_update_latency_ms, currStats.avg_db_bulk_update_latency_ms,
            currStats.num_activity, currStats.num_connectivity,
            currStats.num_clients, currStats.num_new_clients, currStats.num_evict_clients, currStats.memory_bytes,
            currStats.avg_conflation_delay_ms, currStats.avg_conflation_factor);

    sendBuffer[0] = '\0';

    statsd_prepare(pStatsdLink, "pxact.state", currStats.activity_tracking_state, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);

    statsd_prepare(pStatsdLink, "pxact.num_db_read", currStats.num_db_read, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_db_insert", currStats.num_db_insert, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_db_update", currStats.num_db_update, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_db_bulk_update", currStats.num_db_bulk_update, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_db_bulk", currStats.num_db_bulk, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_db_delete", currStats.num_db_delete, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_db_error", currStats.num_db_error, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.avg_db_read_latency_ms", currStats.avg_db_read_latency_ms, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.avg_db_update_latency_ms", currStats.avg_db_update_latency_ms, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.avg_db_bulk_update_latency_ms", currStats.avg_db_bulk_update_latency_ms, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);

    statsd_prepare(pStatsdLink, "pxact.num_activity", currStats.num_activity, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_connectivity", currStats.num_connectivity, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_clients", currStats.num_clients, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_new_clients", currStats.num_new_clients, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.num_evict_clients", currStats.num_evict_clients, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.memory_bytes", currStats.memory_bytes, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);

    statsd_prepare(pStatsdLink, "pxact.avg_conflation_delay_ms", currStats.avg_conflation_delay_ms, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "pxact.avg_conflation_factor", currStats.avg_conflation_factor, "g", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(sendBuffer,prepBuff);

    statsd_send(pStatsdLink, sendBuffer);
    lastPXActUpdateTime = currTime;
    lastPXActStatIndex = currPXActStatIndex;
#endif
}

static void updateStats(void) {
    char prepBuff[3096];
    double currTime = ism_common_readTSC();
    double sampleRate = currTime - lastUpdateTime;
    uint8_t currStatIndex = !lastStatIndex;

    ism_proxy_getAuthStats(&authStats[currStatIndex]);
    ism_proxy_getMQTTStats(&mqttStats[currStatIndex]);
    ism_proxy_getHTTPStats(&httpStats[currStatIndex]);
    ism_proxy_getTCPStats(&tcpStats[currStatIndex]);
    ism_proxy_getMQTTMsgSizeStats(&mqttMsgSizeStats[currStatIndex]);
    ism_proxy_getHTTPMsgSizeStats(&httpMsgSizeStats[currStatIndex]);

#ifdef COMMON_MALLOC_WRAPPER
    ism_common_getMemoryStatistics(&memoryStats[currStatIndex]);
#endif

    uint64_t tcpC2PDataReceived = tcpStats[currStatIndex].tcpC2PDataReceived - tcpStats[lastStatIndex].tcpC2PDataReceived;
    uint64_t tcpP2SDataReceived = tcpStats[currStatIndex].tcpP2SDataReceived - tcpStats[lastStatIndex].tcpP2SDataReceived;
    uint64_t tcpC2PDataSent = tcpStats[currStatIndex].tcpC2PDataSent - tcpStats[lastStatIndex].tcpC2PDataSent;
    uint64_t tcpP2SDataSent = tcpStats[currStatIndex].tcpP2SDataSent - tcpStats[lastStatIndex].tcpP2SDataSent;
    uint64_t incomingConnectionsTotal = tcpStats[currStatIndex].incomingConnectionsTotal - tcpStats[lastStatIndex].incomingConnectionsTotal;
    uint64_t outgoingConnectionsTotal = tcpStats[currStatIndex].outgoingConnectionsTotal - tcpStats[lastStatIndex].outgoingConnectionsTotal;
    uint64_t incomingTLSv1_1 = tcpStats[currStatIndex].incomingTLSv1_1 - tcpStats[lastStatIndex].incomingTLSv1_1;
    uint64_t incomingTLSv1_2 = tcpStats[currStatIndex].incomingTLSv1_2 - tcpStats[lastStatIndex].incomingTLSv1_2;
    uint64_t incomingTLSv1_3 = tcpStats[currStatIndex].incomingTLSv1_3 - tcpStats[lastStatIndex].incomingTLSv1_3;
    uint64_t mqttConnectionsTotal = mqttStats[currStatIndex].mqttConnectionsTotal - mqttStats[lastStatIndex].mqttConnectionsTotal;
    uint64_t mqttC2PMsgsSent = mqttStats[currStatIndex].mqttC2PMsgsSent - mqttStats[lastStatIndex].mqttC2PMsgsSent;
    uint64_t mqttP2SMsgsSent = mqttStats[currStatIndex].mqttP2SMsgsSent - mqttStats[lastStatIndex].mqttP2SMsgsSent;
    uint64_t mqttC2PMsgsReceived0 = mqttStats[currStatIndex].mqttC2PMsgsReceived[0] - mqttStats[lastStatIndex].mqttC2PMsgsReceived[0];
    uint64_t mqttC2PMsgsReceived1 = mqttStats[currStatIndex].mqttC2PMsgsReceived[1] - mqttStats[lastStatIndex].mqttC2PMsgsReceived[1];
    uint64_t mqttC2PMsgsReceived2 = mqttStats[currStatIndex].mqttC2PMsgsReceived[2] - mqttStats[lastStatIndex].mqttC2PMsgsReceived[2];
    uint64_t mqttP2SMsgsReceived0 = mqttStats[currStatIndex].mqttP2SMsgsReceived[0] - mqttStats[lastStatIndex].mqttP2SMsgsReceived[0];
    uint64_t mqttP2SMsgsReceived1 = mqttStats[currStatIndex].mqttP2SMsgsReceived[1] - mqttStats[lastStatIndex].mqttP2SMsgsReceived[1];
    uint64_t mqttP2SMsgsReceived2 = mqttStats[currStatIndex].mqttP2SMsgsReceived[2] - mqttStats[lastStatIndex].mqttP2SMsgsReceived[2];
    uint64_t httpConnectionsTotal = httpStats[currStatIndex].httpConnectionsTotal - httpStats[lastStatIndex].httpConnectionsTotal;
    uint64_t httpC2PMsgsReceived = httpStats[currStatIndex].httpC2PMsgsReceived - httpStats[lastStatIndex].httpC2PMsgsReceived;
    uint64_t httpP2SMsgsSent = httpStats[currStatIndex].httpP2SMsgsSent - httpStats[lastStatIndex].httpP2SMsgsSent;
    uint64_t httpS2PMsgsReceived = httpStats[currStatIndex].httpS2PMsgsReceived - httpStats[lastStatIndex].httpS2PMsgsReceived;
    uint64_t httpP2CMsgsSent = httpStats[currStatIndex].httpP2CMsgsSent - httpStats[lastStatIndex].httpP2CMsgsSent;

    /*MsgSize Stats*/
    uint64_t mqttC2P512BMsgReceived = mqttMsgSizeStats[currStatIndex].C2P512BMsgReceived - mqttMsgSizeStats[lastStatIndex].C2P512BMsgReceived;
    uint64_t mqttC2P1KBMsgReceived = mqttMsgSizeStats[currStatIndex].C2P1KBMsgReceived- mqttMsgSizeStats[lastStatIndex].C2P1KBMsgReceived;
    uint64_t mqttC2P4KBMsgReceived = mqttMsgSizeStats[currStatIndex].C2P4KBMsgReceived - mqttMsgSizeStats[lastStatIndex].C2P4KBMsgReceived;
    uint64_t mqttC2P16KBMsgReceived = mqttMsgSizeStats[currStatIndex].C2P16KBMsgReceived - mqttMsgSizeStats[lastStatIndex].C2P16KBMsgReceived;
    uint64_t mqttC2P64KBMsgReceived = mqttMsgSizeStats[currStatIndex].C2P64KBMsgReceived - mqttMsgSizeStats[lastStatIndex].C2P64KBMsgReceived;
    uint64_t mqttC2PLargeMsgReceived = mqttMsgSizeStats[currStatIndex].C2PLargeMsgReceived - mqttMsgSizeStats[lastStatIndex].C2PLargeMsgReceived;

    uint64_t mqttP2S512BMsgReceived = mqttMsgSizeStats[currStatIndex].P2S512BMsgReceived - mqttMsgSizeStats[lastStatIndex].P2S512BMsgReceived;
	uint64_t mqttP2S1KBMsgReceived = mqttMsgSizeStats[currStatIndex].P2S1KBMsgReceived- mqttMsgSizeStats[lastStatIndex].P2S1KBMsgReceived;
	uint64_t mqttP2S4KBMsgReceived = mqttMsgSizeStats[currStatIndex].P2S4KBMsgReceived - mqttMsgSizeStats[lastStatIndex].P2S4KBMsgReceived;
	uint64_t mqttP2S16KBMsgReceived = mqttMsgSizeStats[currStatIndex].P2S16KBMsgReceived - mqttMsgSizeStats[lastStatIndex].P2S16KBMsgReceived;
	uint64_t mqttP2S64KBMsgReceived = mqttMsgSizeStats[currStatIndex].P2S64KBMsgReceived - mqttMsgSizeStats[lastStatIndex].P2S64KBMsgReceived;
	uint64_t mqttP2SLargeMsgReceived = mqttMsgSizeStats[currStatIndex].P2SLargeMsgReceived - mqttMsgSizeStats[lastStatIndex].P2SLargeMsgReceived;


    uint64_t httpC2P512BMsgReceived = httpMsgSizeStats[currStatIndex].C2P512BMsgReceived - httpMsgSizeStats[lastStatIndex].C2P512BMsgReceived;
	uint64_t httpC2P1KBMsgReceived = httpMsgSizeStats[currStatIndex].C2P1KBMsgReceived- httpMsgSizeStats[lastStatIndex].C2P1KBMsgReceived;
	uint64_t httpC2P4KBMsgReceived = httpMsgSizeStats[currStatIndex].C2P4KBMsgReceived - httpMsgSizeStats[lastStatIndex].C2P4KBMsgReceived;
	uint64_t httpC2P16KBMsgReceived = httpMsgSizeStats[currStatIndex].C2P16KBMsgReceived - httpMsgSizeStats[lastStatIndex].C2P16KBMsgReceived;
	uint64_t httpC2P64KBMsgReceived = httpMsgSizeStats[currStatIndex].C2P64KBMsgReceived - httpMsgSizeStats[lastStatIndex].C2P64KBMsgReceived;
	uint64_t httpC2PLargeMsgReceived = httpMsgSizeStats[currStatIndex].C2PLargeMsgReceived - httpMsgSizeStats[lastStatIndex].C2PLargeMsgReceived;

	uint64_t httpP2S512BMsgReceived = httpMsgSizeStats[currStatIndex].P2S512BMsgReceived - httpMsgSizeStats[lastStatIndex].P2S512BMsgReceived;
	uint64_t httpP2S1KBMsgReceived = httpMsgSizeStats[currStatIndex].P2S1KBMsgReceived- httpMsgSizeStats[lastStatIndex].P2S1KBMsgReceived;
	uint64_t httpP2S4KBMsgReceived = httpMsgSizeStats[currStatIndex].P2S4KBMsgReceived - httpMsgSizeStats[lastStatIndex].P2S4KBMsgReceived;
	uint64_t httpP2S16KBMsgReceived = httpMsgSizeStats[currStatIndex].P2S16KBMsgReceived - httpMsgSizeStats[lastStatIndex].P2S16KBMsgReceived;
	uint64_t httpP2S64KBMsgReceived = httpMsgSizeStats[currStatIndex].P2S64KBMsgReceived - httpMsgSizeStats[lastStatIndex].P2S64KBMsgReceived;
	uint64_t httpP2SLargeMsgReceived = httpMsgSizeStats[currStatIndex].P2SLargeMsgReceived - httpMsgSizeStats[lastStatIndex].P2SLargeMsgReceived;


	uint64_t   maxAuthenticationResponseTime = 0;
    uint64_t   avgAuthenticationResponseTime = 0;
    if(authStats[currStatIndex].authenticationRequestsCount) {
        maxAuthenticationResponseTime = (uint64_t)(1e3 * authStats[currStatIndex].maxAuthenticationResponseTime +0.5);//In milliseconds
        avgAuthenticationResponseTime = (uint64_t)((1e3 * authStats[currStatIndex].authenticationResponseTime)/authStats[currStatIndex].authenticationRequestsCount +0.5);//In milliseconds
    }
    TRACE(6, "Proxy TCP stats collected: tcpC2PDataReceived=%llu tcpP2SDataReceived=%llu tcpC2PDataSent=%llu tcpP2SDataSent%llu incomingConnectionsTotal=%llu outgoingConnectionsTotal=%llu "
            "incomingConnectionsCounter=%d outgoingConnectionsCounter=%d pendingOutgoingConnectionsCounter=%d "
            "incomingTLSv1_1=%llu incomingTLSv1_2=%llu incomingTLSv1_3=%llu\n",
            (ULL)tcpC2PDataReceived, (ULL)tcpP2SDataReceived, (ULL)tcpC2PDataSent, (ULL)tcpP2SDataSent, (ULL)incomingConnectionsTotal, (ULL)outgoingConnectionsTotal,
            tcpStats[currStatIndex].incomingConnectionsCounter, tcpStats[currStatIndex].outgoingConnectionsCounter, tcpStats[currStatIndex].pendingOutgoingConnectionsCounter,
            (ULL)incomingTLSv1_1, (ULL)incomingTLSv1_2, (ULL)incomingTLSv1_1);

    TRACE(6, "Proxy MQTT stats collected: mqttC2PMsgsSent=%llu mqttP2SMsgsSent=%llu mqttC2PMsgsReceived0=%llu mqttC2PMsgsReceived1=%llu mqttC2PMsgsReceived2=%llu "
            "mqttP2SMsgsReceived0=%llu mqttP2SMsgsReceived1=%llu mqttP2SMsgsReceived2=%llu mqttConnectionsTotal=%llu mqttPendingAuthenticationRequests=%d mqttPendingAuthorizationRequests=%d "
            "mqttConnections=%u mqttConnectedDev=%u mqttConnectedApp=%u mqttConnectedGWs=%u mqttVersion3=%u mqttVersion4=%u mqttVersion5=%u mqttUseWebsocket=%u\n",
            (ULL)mqttC2PMsgsSent, (ULL)mqttP2SMsgsSent, (ULL)mqttC2PMsgsReceived0, (ULL)mqttC2PMsgsReceived1,
            (ULL)mqttC2PMsgsReceived2,(ULL)mqttP2SMsgsReceived0, (ULL)mqttP2SMsgsReceived1, (ULL)mqttP2SMsgsReceived2, (ULL)mqttConnectionsTotal, mqttStats[currStatIndex].mqttPendingAuthenticationRequests,
            mqttStats[currStatIndex].mqttPendingAuthorizationRequests, mqttStats[currStatIndex].mqttConnections, mqttStats[currStatIndex].mqttConnectedDev, mqttStats[currStatIndex].mqttConnectedApp,
            mqttStats[currStatIndex].mqttConnectedGWs, mqttStats[currStatIndex].mqttVersion3_1, mqttStats[currStatIndex].mqttVersion3_1_1, mqttStats[currStatIndex].mqttVersion5_0,
            mqttStats[currStatIndex].mqttUseWebsocket);

    TRACE(6, "Proxy HTTP stats collected: httpS2PMsgsReceived=%llu, httpP2CMsgsSent=%llu, httpC2PMsgsReceived=%llu httpP2SMsgsSent=%llu httpConnectionsTotal=%llu httpConnections=%d httpConnectedDev=%d httpConnectedApp=%d httpConnectedGW=%d httpPendingAuthenticationRequests=%d httpPendingAuthorizationRequests=%d httpPendingGetCommandRequests=%d\n",
    		(ULL)httpS2PMsgsReceived, (ULL)httpP2CMsgsSent, (ULL)httpC2PMsgsReceived, (ULL)httpP2SMsgsSent, (ULL)httpConnectionsTotal, httpStats[currStatIndex].httpConnections, httpStats[currStatIndex].httpConnectedDev, httpStats[currStatIndex].httpConnectedApp, httpStats[currStatIndex].httpConnectedGWs,
            httpStats[currStatIndex].httpPendingAuthenticationRequests, httpStats[currStatIndex].httpPendingAuthorizationRequests,
			httpStats[currStatIndex].httpPendingGetCommandRequests);

    TRACE(6, "Proxy Auth stats collected: maxAuthenticationResponseTime=%llu avgAuthenticationResponseTime=%llu authenticationRequestsCount=%llu\n",
            (ULL)maxAuthenticationResponseTime, (ULL)avgAuthenticationResponseTime, (ULL)authStats[currStatIndex].authenticationRequestsCount);

    TRACE(6, "Proxy MQTT MessageSize Stats collected: mqttC2P512BMsgReceived=%llu mqttC2P1KBMsgReceived=%llu mqttC2P4KBMsgReceived=%llu "
    		 "mqttC2P16KBMsgReceived=%llu mqttC2P64KBMsgReceived=%llu mqttC2PLargeMsgReceived=%llu "
    		 "mqttP2S512BMsgReceived=%llu mqttP2S1KBMsgReceived=%llu mqttP2S4KBMsgReceived=%llu "
    		 "mqttP2S16KBMsgReceived=%llu mqttC2P64KBMsgReceived=%llu mqttP2SLargeMsgReceived=%llu\n",
               (ULL)mqttC2P512BMsgReceived, (ULL)mqttC2P1KBMsgReceived, (ULL)mqttC2P4KBMsgReceived,
			   (ULL)mqttC2P16KBMsgReceived, (ULL)mqttC2P64KBMsgReceived, (ULL)mqttC2PLargeMsgReceived,
			   (ULL)mqttP2S512BMsgReceived, (ULL)mqttP2S1KBMsgReceived, (ULL)mqttP2S4KBMsgReceived,
			   (ULL)mqttP2S16KBMsgReceived, (ULL)mqttP2S64KBMsgReceived, (ULL)mqttP2SLargeMsgReceived);

    TRACE(6, "Proxy HTTP MessageSize Stats collected: httpC2P512BMsgReceived=%llu httpC2P1KBMsgReceived=%llu httpC2P4KBMsgReceived=%llu "
        		 "httpC2P16KBMsgReceived=%llu httpC2P64KBMsgReceived=%llu httpC2PLargeMsgReceived=%llu "
        		 "httpP2S512BMsgReceived=%llu httpP2S1KBMsgReceived=%llu httpP2S4KBMsgReceived=%llu "
        		 "httpP2S512BMsgReceived=%llu httpP2S1KBMsgReceived=%llu httpP2S4KBMsgReceived=%llu\n",
                   (ULL)httpC2P512BMsgReceived, (ULL)httpC2P1KBMsgReceived, (ULL)httpC2P4KBMsgReceived,
    			   (ULL)httpC2P16KBMsgReceived, (ULL)httpC2P64KBMsgReceived, (ULL)httpC2PLargeMsgReceived,
				   (ULL)httpP2S512BMsgReceived, (ULL)httpP2S1KBMsgReceived, (ULL)httpP2S4KBMsgReceived,
				   (ULL)httpP2S16KBMsgReceived, (ULL)httpP2S64KBMsgReceived, (ULL)httpP2SLargeMsgReceived);

    updateBuffer[0] = '\0';

    //TCP
    statsd_prepare(pStatsdLink, "tcp.C2PDataReceived", tcpC2PDataReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.P2SDataReceived", tcpP2SDataReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.C2PDataSent", tcpC2PDataSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.P2SDataSent", tcpP2SDataSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.incomingConnectionsTotal", incomingConnectionsTotal, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.incomingTLSv1_1", incomingTLSv1_1, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.incomingTLSv1_2", incomingTLSv1_2, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.incomingTLSv1_3", incomingTLSv1_3, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.outgoingConnectionsTotal", outgoingConnectionsTotal, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.incomingConnectionsCounter", tcpStats[currStatIndex].incomingConnectionsCounter, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.outgoingConnectionsCounter", tcpStats[currStatIndex].outgoingConnectionsCounter, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "tcp.pendingOutgoingConnectionsCounter", tcpStats[currStatIndex].pendingOutgoingConnectionsCounter, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);

    //MQTT
    statsd_prepare(pStatsdLink, "mqtt.ConnectionsTotal", mqttConnectionsTotal, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2PMsgsSent", mqttC2PMsgsSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.P2SMsgsSent", mqttP2SMsgsSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2PMsgsReceived0", mqttC2PMsgsReceived0, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2PMsgsReceived1", mqttC2PMsgsReceived1, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2PMsgsReceived2", mqttC2PMsgsReceived2, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.P2SMsgsReceived0", mqttP2SMsgsReceived0, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.P2SMsgsReceived1", mqttP2SMsgsReceived1, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.P2SMsgsReceived2", mqttP2SMsgsReceived2, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.pendingAuthenticationRequests", mqttStats[currStatIndex].mqttPendingAuthenticationRequests, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.pendingAuthorizationRequests", mqttStats[currStatIndex].mqttPendingAuthorizationRequests, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.connections", mqttStats[currStatIndex].mqttConnections, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.connectedDev", mqttStats[currStatIndex].mqttConnectedDev, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.connectedApp", mqttStats[currStatIndex].mqttConnectedApp, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.connectedGWs", mqttStats[currStatIndex].mqttConnectedGWs, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.version3_1", mqttStats[currStatIndex].mqttVersion3_1, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.version3_1_1", mqttStats[currStatIndex].mqttVersion3_1_1, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.version5_0", mqttStats[currStatIndex].mqttVersion5_0, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.useWebsocket", mqttStats[currStatIndex].mqttUseWebsocket, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    /*MQTT Msg Size Stats*/
    statsd_prepare(pStatsdLink, "mqtt.C2P512BMsgReceived", mqttC2P512BMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2P1KBMsgReceived", mqttC2P1KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2P4KBMsgReceived", mqttC2P4KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2P16KBMsgReceived", mqttC2P16KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2P64KBMsgReceived", mqttC2P64KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "mqtt.C2PLargeMsgReceived", mqttC2PLargeMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);

    statsd_prepare(pStatsdLink, "mqtt.P2S512BMsgReceived", mqttP2S512BMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
   strcat(updateBuffer,prepBuff);
   statsd_prepare(pStatsdLink, "mqtt.P2S1KBMsgReceived", mqttP2S1KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
   strcat(updateBuffer,prepBuff);
   statsd_prepare(pStatsdLink, "mqtt.P2S4KBMsgReceived", mqttP2S4KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
   strcat(updateBuffer,prepBuff);
   statsd_prepare(pStatsdLink, "mqtt.P2S16KBMsgReceived", mqttP2S16KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
   strcat(updateBuffer,prepBuff);
   statsd_prepare(pStatsdLink, "mqtt.P2S64KBMsgReceived", mqttP2S64KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
   strcat(updateBuffer,prepBuff);
   statsd_prepare(pStatsdLink, "mqtt.P2SLargeMsgReceived", mqttP2SLargeMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
   strcat(updateBuffer,prepBuff);

    //HTTP
    statsd_prepare(pStatsdLink, "http.connectionsTotal", httpConnectionsTotal, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.S2PMsgsReceived", httpS2PMsgsReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.P2CMsgsSent", httpP2CMsgsSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.C2PMsgsReceived", httpC2PMsgsReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.P2SMsgsSent", httpP2SMsgsSent, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.connections", httpStats[currStatIndex].httpConnections, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.connectedDev", httpStats[currStatIndex].httpConnectedDev, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.connectedApp", httpStats[currStatIndex].httpConnectedApp, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.connectedGW", httpStats[currStatIndex].httpConnectedGWs, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.pendingAuthenticationRequests", httpStats[currStatIndex].httpPendingAuthenticationRequests, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.pendingAuthorizationRequests", httpStats[currStatIndex].httpPendingAuthorizationRequests, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.pendingGetCommandRequests", httpStats[currStatIndex].httpPendingGetCommandRequests, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    /*HTTP MSg size stats*/
    statsd_prepare(pStatsdLink, "http.C2P512BMsgReceived", httpC2P512BMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.C2P1KBMsgReceived", httpC2P1KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "http.C2P4KBMsgReceived", httpC2P4KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.C2P16KBMsgReceived", httpC2P16KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.C2P64KBMsgReceived", httpC2P64KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.C2PLargeMsgReceived", httpC2PLargeMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);

	statsd_prepare(pStatsdLink, "http.P2S512BMsgReceived", httpP2S512BMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.P2S1KBMsgReceived", httpP2S1KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.P2S4KBMsgReceived", httpP2S4KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.P2S16KBMsgReceived", httpP2S16KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.P2S64KBMsgReceived", httpP2S64KBMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);
	statsd_prepare(pStatsdLink, "http.P2SLargeMsgReceived", httpP2SLargeMsgReceived, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
	strcat(updateBuffer,prepBuff);

	//AUTH
    statsd_prepare(pStatsdLink, "authentication.requestsCount", authStats[currStatIndex].authenticationRequestsCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "authentication.throttleCount", authStats[currStatIndex].authenticationThrottleCount, "c", sampleRate, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "authentication.maxResponseTimeMs", maxAuthenticationResponseTime, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);
    statsd_prepare(pStatsdLink, "authentication.avgResponseTimeMs", avgAuthenticationResponseTime, "g", 1.0, prepBuff, sizeof(prepBuff),1);
    strcat(updateBuffer,prepBuff);

    //memory
    // Note this doesn't have to be a nested for loop, statsd doesn't care which order the stats are added
    // but this follows the same pattern as producing the json (which does care)
#ifdef COMMON_MALLOC_WRAPPER
    uint32_t groupId = 0;
    for (groupId = 0; groupId < ism_common_mem_numgroups; groupId++) {
        char total[128 + 7] = "memory.";
        strncat(total, ism_common_getMemoryGroupName(groupId), 64);
        strcat(total, ".Total");
        statsd_prepare(pStatsdLink, total,
                memoryStats[currStatIndex].groups[groupId], "g", 1.0, prepBuff,
                sizeof(prepBuff), 1);
        strcat(updateBuffer, prepBuff);

        uint32_t typeId = 0;
        for (typeId = 0; typeId < ism_common_mem_numtypes; typeId++) {
            if (ism_common_getMemoryGroupFromType(typeId) == groupId) {
                char name[128 + 8] = "memory.";
                strncat(name, ism_common_getMemoryGroupName(groupId), 64);
                strcat(name, ".");
                strncat(name, ism_common_getMemoryTypeName(typeId), 64);
                statsd_prepare(pStatsdLink, name,
                        memoryStats[currStatIndex].types[typeId], "g", 1.0,
                        prepBuff, sizeof(prepBuff), 1);
                strcat(updateBuffer, prepBuff);
            }
        }
    }

    statsd_prepare(pStatsdLink, "memory.FFDCs", ism_common_get_ffdc_count(),
            "g", 1.0, prepBuff, sizeof(prepBuff), 0);
    strcat(updateBuffer, prepBuff);
#endif

    statsd_send(pStatsdLink, updateBuffer);

    updateMHUBMessagingStats(sampleRate); //PUBLISH mhub stats

    lastUpdateTime = currTime;
    lastStatIndex = currStatIndex;
}



static int updateStatsTimer(ism_timer_t key, ism_time_t timestamp, void * userdata) {
    if(lastUpdateTime == 0.0) {
        lastUpdateTime = ism_common_readTSC();
#ifndef NO_PXACT
        lastPXActUpdateTime = lastUpdateTime; //PXACT
#endif
        return 1;
    }
    updateStats();
    updateServersStats();
    updateMUXStats();
    updatePXActStats(); //PXACT
    updateIMMessagingStats();

    return 1;
}

#endif

XAPI int ism_proxy_statsd_init(void) {
#ifndef HAS_BRIDGE
    int   statsdPort = 8125;
    const char * statsdHost = getenv("STATSD_PORT_8125_UDP_ADDR");
    char * instanceID = getenv("INSTANCE_ID");
    char * namespace;
    int statsUpdateInterval = ism_common_getIntConfig("statsUpdateInterval", 1);
    if(statsdHost == NULL) {
//        LOG(WARN, Server, 961, "", "statsd host is not defined");
        TRACE(4,"STATSD_PORT_8125_UDP_ADDR environment variable is not set.\n");
        return ISMRC_OK;
    }
    if(instanceID == NULL) {
        char hostname[1024];
        if(gethostname(hostname, sizeof(hostname))) {
            instanceID = "UNKNOWN";
        } else {
            instanceID = alloca(strlen(hostname) + 16);
            sprintf(instanceID, "%s",hostname);
        }
        TRACE(4,"INSTANCE_ID environment variable is not set. Using %s\n", instanceID);
    }
    namespace = alloca(strlen(instanceID) + 64);
    sprintf(namespace, "msproxy.%s.UsageStatistics", instanceID);
    pStatsdLink = statsd_init_with_namespace(statsdHost, statsdPort, namespace);
    if(pStatsdLink) {

    } else {
        TRACE(4,"Failed to create a statsd link to %s%d (ns=%s)\n", statsdHost, statsdPort, instanceID);
        return ISMRC_OK;
    }

    memset(tcpStats,0,sizeof(px_tcp_stats_t)*2);
    memset(mqttStats,0,sizeof(px_mqtt_stats_t)*2);
    memset(httpStats,0,sizeof(px_http_stats_t)*2);
    memset(authStats,0,sizeof(px_auth_stats_t)*2);
    ism_common_setTimerRate(ISM_TIMER_LOW, (ism_attime_t) updateStatsTimer, NULL, 30, statsUpdateInterval, TS_SECONDS);
#endif
    return ISMRC_OK;
}


