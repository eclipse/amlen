/*
 * Copyright (c) 2018-2021 Contributors to the Eclipse Foundation
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

#ifndef IOTMHUB_H
#define IOTMHUB_H

#include <ismutil.h>
#include <pxkafka.h>
#include <sasl_scram.h>

#ifdef __cplusplus
extern "C" {
#endif


struct mhub_topic_t ;
/*
 * Define a single MessageHub partition
 *
 * Most of the state is kept in the transport object, but that only
 * exists which a connection is active or connecting.
 */
typedef struct mhub_part_t {
    uint8_t    valid;       /* The metadata is valid */
    uint8_t    open;        /* The connection state */
    uint8_t    flags[1];
    uint8_t    needreproduce;
    int32_t    leader;      /* The leader for this partition (-1 if no leader */
    pthread_mutex_t 		lock;
    struct mhub_topic_t *   topic;
    ism_transport_t *       transport;   /* The data connection for this partition */
    kafka_produce_msg_t	* 	kafka_msg_first;
    kafka_produce_msg_t	* 	kafka_msg_last;
    int						kafka_msg_count;
    int                     resvi;
    ism_time_t				kafka_msg_first_time;
    uint8_t    				hasackwait;
    kafka_produce_msg_t	* 	kafka_ackwait_msg_first;
    kafka_produce_msg_t	* 	kafka_ackwait_msg_last;
    double	 				lastProduceTime;
    int						produceErrorCount;
    int						produceErrorTotalCount;
    int						produceLastRC;
    ism_time_t				produceErrorFirstTimeInNanos;
} mhub_part_t;


/*
 * The MesageHub topic object.
 *
 * There is a topic object created for each topic referenced by a rule.
 * This object is filled in using the metadata after a connection is made.
 */
typedef struct mhub_topic_t {
    const char *    name;
    uint8_t         valid;            /* The metadata is valid */
    uint8_t         flags[3];
    uint32_t        partcount;
    mhub_part_t     partitions [1];   /* Allocated to size required */
} mhub_topic_t;


/*
 * The parsed forwarding rule.
 *
 * The actual rule content is in the bytes following this header.
 * A rule of the same length and some content is equal.  The other
 * header fields do not affect equality.
 */
typedef struct mhub_rule_t {
    uint16_t   rulelen;      /* Length of the rule not including the header */
    int16_t    topic_num;    /* The topic index */
    uint8_t    enabled;      /* 0=disabled, 1=enabled, 2=admindisabled */
    uint8_t    resv;
    uint8_t    name_alloc;   /* Allocated size of name (for alignment) */
    uint8_t    name_len;     /* Actual length of name which immediately follows */
} mhub_rule_t;


/*
 * Need update bits
 */
enum mhub_update_e {
    UPDATE_Rules    = 0x01,      /* Change forwarding rules */
    UPDATE_Brokers  = 0x02,      /* Change broker list */
    UPDATE_Auth     = 0x04,      /* Change user or password */
    UPDATE_Topics   = 0x08,      /* Change the list of topics or partitions */
    UPDATE_Status   = 0x10,      /* Change other fields */
    UPDATE_Secure   = 0x40       /* Update TLS context fields */
};

/*
 * Callback when mhub state changes
 */
typedef void (* mhubStateChange_f)(struct ism_mhub_t * mhub);

/*
 * MessageHub object
 *
 * This is the combined object defining a binding and credentials for one
 * MessageHub instance.
 */
struct ism_mhub_t {
    char             structid[8];    /* MHub */
    struct ism_mhub_t * next;
    const char *     name;           /* Always points to id */
    uint8_t          last_good;      /* 0=ipaddr, 1=backup */
    uint8_t          useTLS;         /* 0=none, 1=fast, 2=best */
    uint8_t          serverKind;     /* 0=MessageSight, 1=Kafka */
    uint8_t          disabled;
    int              port;
    pthread_spinlock_t  lock;        /* Lock for small chunks of code */
    struct ssl_ctx_st * tlsCTX;      /* TLS client context */
    ism_getAddress_f getAddress;     /* Callback to get server address */
    /* Everything before this is common to the server structure */
    char             id [48];        /* The ID of the binding */
    char             serviceid [48]; /* The service ID of the service as a UUID */
    ism_timezone_t * timezone;       /* The timezone object */
    ism_ts_t *       ts;             /* The timestamp object */
    ism_time_t       valid_until;    /* The valid unitl time */
    pthread_spinlock_t  tslock;      /* The timestamp lock */
    ism_tenant_t *   tenant;         /* The org */
    int              rulecount;      /* Number of rules */
    int              rulealloc;      /* Number of allocated rules */
    int              topiccount;     /* Number of topics */
    int              topicalloc;     /* Number of allocated topics */
    mhub_rule_t * *  rules;          /* The compiled rules */
    const char * *   rulestr;        /* Rule strings */
    const char * *   rulenames;      /* Rule names */
    mhub_topic_t * * topics;         /* The topics */
    const char *     password;
    const char *     username;
    uint8_t          enabled;        /* mhub is enabled: 1=enabled 0=disabled 2=admin_disabled */
    uint8_t          state;          /* Object state: see enum mhub_states_e */
    uint8_t          prev_state;     /* Previous state (used by stateChanged callback */
    uint8_t          need;           /* Config changes need to cause runtime changes. see enum mhub_update_e */
    uint8_t          apiVersion;     /* KafkaAPIVersion 0-2. Other version derived from this */
    uint8_t          metadataVersion;
    uint8_t          produceVersion;
    uint8_t          messageVersion;

    uint8_t			 describeConfigsVersion;
    uint8_t          resvi [3];
    uint8_t          mhubSASL;       /* 0=noSASL 1=PLAIN */
    int8_t           mhubACK;        /* ACL count (-1, 0, 1) */
    uint8_t          moreLogs;       /* Do additional logging */
    uint8_t          expectingMetadata;  /* Do not send more than one metadata request at a time */

    uint32_t         retry;          /* Retry count */
    int              notopen;        /* Data partitions not open */
    int              open;           /* Data partitions open */
    uint32_t         maxBatchMsgs;   /* Maximum batch size in msgs */
    uint32_t         maxBatchTimeMS; /* Maximum time before batch is sent */
    uint32_t         maxBatchBytes;  /* Maximum total bytes for the batch */
    uint8_t          versionKnown;   /* The kafka version is known */
    uint8_t          verProduce;     /* The kafka Produce max version */
    uint8_t          verFetch;       /* The kafka Fetch max version */
    uint8_t          verMetadata;    /* The kafka Metadta max version */
    uint8_t          verSaslHandshake;    /* The kafka SaslHandshake max version */
    uint8_t          verSaslAuthenticate; /* The kafka SaslAuthenticate max version */
    uint8_t          verInitProducerId;   /* The kafka InitProducerId max version */
    uint8_t          verDescribeConfigs;  /* The kafaka DescribeConfigs version */
    const char *     kafka_version;  /* String representing the Kafka version based on ApiVersions */
    mhubStateChange_f stateChanged;  /* Callback to indicate the state is changed */
    void *           userdata;       /* Used by creator of the object to store its private object */
    int              trybroker;      /* Next broker to try */
    int              broker_count;   /* Count of brokers */
    const char *     brokers [32];   /* Broker list */
    ism_transport_t * metadata;      /* Metadata connection */
    ism_timer_t      produce_timer;  /* Timer to check for batching */
    ism_prop_t *     props;          /* Properties for selection */
    const char *     ciphers;
    const char *     serverName;
    const char *     partitionMap;   /* The partition map (null use default) */
    int              partitionPart;  /* The partition part */
    uint8_t          resvxx[3];
    uint8_t          use_keymap;    /* 0=internal keymap, 1=specified keymap */
    const char *     keymap;
    const char *     trustCerts;
    int				isKafkaConn;		/*0=not a KafkaConnection type, 1=KafkaConnection type*/

    //SASL Mechanism
    ism_sasl_machanism_e     sasl_mechanism;  /*Support PLAIN, SCRAM-SHA-256, SCRAM-SHA-512*/

};


/* PartitionRule kinds */
#define PART_RULE_HASH      0   /* Hash a string */
#define PART_RULE_SINGLE    1   /* Chose a single part */
#define PART_RULE_ANY       2   /* Distribute between all parts */
#define PART_RULE_NUMBER    3   /* Select a part number */
#define PART_RULE_INSTANCE  4   /* Use instance as the part number */
/*
 * MessageHub states
 *
 * update mhubStateString if any are added
 */
enum mhub_states_e {
    MHS_Config      = 0x00,   /* Configured but not started */
    MHS_Failed      = 0x01,   /* Broker connection failed, need new config */
    MHS_Wait        = 0x02,   /* Waiting for a broker connection */
    MHS_Opening     = 0x03,   /* Some data connections are not open */
    MHS_Closing     = 0x04,   /* Closing connections due to shutdown or config change */
    MHS_Active      = 0x05   /* All data connections are open */
};

/* Temp array for broker list */
typedef struct mhub_broker_list_t {
    const char * broker;
    uint16_t     broker_len;
    uint16_t     port;
    uint32_t     nodeid;
} mhub_broker_list_t;

/*
 * The MesageHub Credential Request object.
 */
typedef struct mhub_cred_req_t {
    char *	orgId;
    char *	serviceId;

} mhub_cred_req_t;

/**
 * Batch Produce Job
 * @param mhub 			MHub object
 * @param mhub_topic		MHub Topic object
 * @param mhub_part		MHub Topic object
 * @param msgs			messages to be produced.
 */
typedef struct mhub_produce_async_job_t {
    ism_mhub_t *     mhub;
	mhub_part_t *	 mhub_part;
	kafka_produce_msg_t * msgs;
} mhub_produce_async_job_t;

/**
 * Submit the request to get the MessageHub Credential
 * @param orgId the organizational ID
 * @param serviceId servide ID string
 * @return 0 for success submission. Non-Zero for error.
 */
int ism_proxy_getMHubCredential(const char * orgId, const char * serviceId );


#ifdef __cplusplus
}
#endif

#endif /* IOTREST_H */
