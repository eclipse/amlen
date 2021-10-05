/*
 * Copyright (c) 2007-2021 Contributors to the Eclipse Foundation
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

#ifndef  H_rmmConstans_H
#define  H_rmmConstans_H


#ifdef xUNUSED
xUNUSED static char * version_string = "RUM version_string:"
XSTR(RMMCAPI_VERSION) "_" XSTR(BUILD_ID) " " XSTR(BUILD_DATE) " " XSTR(BUILD_TIME);
#endif

#define USE_SI 1


/***  Receiver Constants  ***/

#define MIN_INSTANCE_MEMORY      20

#define RMM_GID_LEN   sizeof(rmmGlobalID_t)

#define RMM_INIT_SIG  0x55555555
#define RUM_INIT_SIG  RMM_INIT_SIG
#define RCMS_INIT_SIG (RMM_INIT_SIG+1)
#define RMM_QKEY_SIG  0x17a90000
#define RMM_MODIFY_SIG  -10101

#define  INT_SIZE              4   /* sizeof(int32_t) ; */
#define  SHORT_SIZE            2   /* sizeof(int16_t) ; */
#define  MAX_TOPICS            RUM_MAX_RX_QUEUES
#define  MAX_THREADS           512
#define  MAX_STREAMS          1024
#define  MAX_TASKS             64
#ifndef  LOG_MSG_SIZE
#define  LOG_MSG_SIZE          256
#endif
#ifndef  LINE_SIZE
#define  LINE_SIZE             256
#endif
#define  MAX_DST_ADDRS         1000
#define  MAX_SQN_PER_NAK       63
#define  GROUP_SIZE            32
#define  NACK_BUFF_SIZE        512
#define  MAX_NAK_LENGTH        NACK_BUFF_SIZE
#define  MAX_GLB_2KEEP         256
#define  NANO_PER_SEC          1000000000
#define  SQ_FLAG_PP_DATA          0x01
#define  SQ_FLAG_PP_NCF           0x02
#define  SQ_FLAG_PP_NAK           0x04
#define  SQ_FLAG_MA               0x08
#define  SQ_FLAG_NG               0x10

#define  PORT_RANGE_LO           35000
#define  PORT_RANGE_HI           60000


#define  R_CONFIG_FILE          "rmmRbasic.cfg"
#define  A_CONFIG_FILE          "rmmRadvanced.cfg"
#define  T_CONFIG_FILE          "rmmTbasic.cfg"
#define  T_ADVANCE_CONFIG_FILE  "rmmTadvanced.cfg"


#define RUM_CONFIG_FILE           "rumBasic.cfg"
#define RUM_ADVANCE_CONFIG_FILE   "rumAdvanced.cfg"

#define  NAK_DEAD_STATE           -1
#define  NAK_BACK_OFF_STATE        0
#define  NAK_WAIT_NCF_STATE        1
#define  NAK_WAIT_DATA_STATE       2

#define MAX_READY_CONNS         1024

/**   Macros  */

#define MID_INT    0x80000000
#define XltY(x,y)  (((x)-(y))&MID_INT)
#define XgtY(x,y)  (((y)-(x))&MID_INT)
#define XleY(x,y)  (!XgtY(x,y))
#define XgeY(x,y)  (!XltY(x,y))

#define  milli2nano(x)         (1000000*(x))
#define  milli2micro(x)        (1000*(x))
#define  micro2nano(x)         (1000*(x))



/***  Transmitter Constants  ***/


#define  RMM_TRUE        1
#define  RMM_FALSE       0
#define  RMM_ON          1
#define  RMM_OFF         0

#define  SHORT_SLEEP         10
#define  BASE_SLEEP          20
#define  LONG_SLEEP          1000
#define  FIREOUT_SLEEP_NANO  100000000

 #define MAX_LOCAL_INTERFACES 1

#define MAX_ANCILLARY_PARAM_SIZE 256
#define DEFAULT_CON_ESTABLISH_TIMEOUT_MILLI 60000

#define  MAX_PACKET_SIZE     65000
#define  MIN_PACKET_SIZE     300
#define  MAX_SPM_SIZE        512

#define  MAX_SCP_TRIES       2

#define  STD_PRINTS          1


/* API Thread constants */
#define  THREAD_INIT         0
#define  THREAD_RUNNING      1
#define  THREAD_KILL         2
#define  THREAD_EXIT         3

#define  CALLER_SUB_MSG      1
#define  CALLER_FO           2

/* FO Thread constants */
#define  HISTORY_INCREASE_PERC       10
#define  MAX_NCF_TO_SEND             50

#define  MIN_QUEUE_SIZE              1000
#define  CONTROL_PACKET_BUF_SIZE     512

/* PGM related Constants */

#define  PGM_HEADER_LENGTH           16
#define  ODATA_HEADER_LENGTH         24
#define  NAK_HEADER_FIX_LENGTH       20
#define  SPM_HEADER_FIX_LENGTH       28
#define  SPM_PATH_NLA_OFFSET         32
#define  NAK_SRC_NLA_OFFSET          24

#define  PGM_CHECKSUM_OFFSET         6
#define  MAX_NAK_LIST_SIZE           256
#define  RMM_MAX_AUTH_MSG_LENGTH     250 /* in bytes. must be < 254 */
#define  RMM_MAX_P2P_ID_LENGTH       64  /* in bytes.               */
#define  RMM_IB2IP_HEADER_SIZE       32  /* in bytes.               */

#define  OPT_MSG_INFO_SIZE           17  /* msg_sqn_trail(8) + msg_sqn_lead(8) + reliability(1)  */
#define  OPT_MCAST_ADDR_FIX_SIZE     22  /* multicast_group_length(1)+reliable(1)+Heartbeat_timeout(4)+opt(1)+data_port(2)+repair_port(2)+req_nak(1)+has_ordering_info(1)+msg_prop(1)+ack_req(1)+ack_interval(2+4)+fdbk_mode(1)  */
#define  OPT_UNICAST_CTRL_FIX_SIZE   25  /* status(1)+ack_req(1)+ack_interval(2+4)+rel(1)+fdbk_mode and port(1+2)+HB to(4)+dest+repair port(2+2)+addr_len(1)+p2p_len(1)+req_nak(1)+msg_prop(1)+has_ordering_info(1) */
#define  FIXED_MTL_HEADER_SIZE       4   /* Version (1) + number of messages (2) + Fragment (1) */
#define  MTL_FRAG_FIELDS_LENGTH      12
#define  MSG2PAC_MAX_ENTRIES         60
#define  MSG2PAC_OPT_SIZE            (8+(12*MSG2PAC_MAX_ENTRIES))
#define  RMM_MSG_PROP_FIX_SIZE       14  /*Total_Length(4)+type(1)+reserved(1)+code(2)+name_len(2)+val_len(4)   */
#define  RMM_MSG_PROPS_HEADER        8   /*Total_Length(4)+num_props(2)+reserved(2) */

#define  PGM_NLA4_LENGTH             sizeof(struct in_addr)
#define  PGM_NLA6_LENGTH             sizeof(struct in6_addr)
#define  FRAGMENT_FIRST              2
#define  FRAGMENT_MID                3
#define  FRAGMENT_LAST               1

#define SLOW_REC_OPT_SIZE            247 /* header(4) + sequence_number(4)+ suspend_time(4)+num_rec(1)+26*{action(1)+rec_id(8)}(238) */ 
#define SLOW_REC_RECORDS             26
#define SLOW_REC_REPEAT_SPM          5
#define SLOW_REC_REPEAT_INTERVAL     100
#define REC_ACK_RECORDS              64

#define MAX_RDATA_DESTS             16

#ifndef  RMM_MAX_TX_TOPICS
#define  RMM_MAX_TX_TOPICS           2048
#endif
#define  FIREOUT_ARRAY_SIZE          100

#define  RUM_MAX_CONNECTION_LISTENERS  32 
#define MAX_Tx_INSTANCES             RUM_MAX_INSTANCES+1
#define MAX_Rx_INSTANCES             RUM_MAX_INSTANCES+1
#define MAX_RUM_INSTANCES            RUM_MAX_INSTANCES+1

#endif
