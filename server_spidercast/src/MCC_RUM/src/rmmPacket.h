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

#ifndef  H_rmmPacket_H
#define  H_rmmPacket_H

#ifndef IPPROTO_PGM
#define IPPROTO_PGM            113   /* PGM IP protocol no */
#endif

/*option types*/
#define PGM_OPT_LENGTH          0x00 /*Option's Length*/
                           
#define PGM_OPT_FRAGMENT        0x01 /*Fragmentation*/
                           
#define PGM_OPT_NAK_LIST        0x02 /*List of NAK entries*/
                           
#define PGM_OPT_JOIN            0x03 /*Late Joining*/
                           
#define PGM_OPT_REDIRECT        0x07 /*Redirect*/
                           
#define PGM_OPT_SYN             0x0D /*Synchronization*/
                           
#define PGM_OPT_FIN             0x0E /*Session Fin receivers, conventional feedbackish*/
                           
#define PGM_OPT_RST             0x0F /*Session Reset*/
                           
#define PGM_OPT_PARITY_PRM      0x08 /*Forward Error Correction Parameters*/
                           
#define PGM_OPT_PARITY_GRP      0x09 /*Forward Error Correction Group Number*/
                           
#define PGM_OPT_CURR_TGSIZE     0x0A /*Forward Error Correction Group Size*/
                           
#define PGM_OPT_CR              0x10 /*Congestion Report*/
                           
#define PGM_OPT_CRQST           0x11 /*Congestion Report Request*/

#define PGM_OPT_NAK_BO_IVL      0x04 /*NAK Back-Off Interval*/
                           
#define PGM_OPT_NAK_BO_RNG      0x05 /*NAK Back-Off Range*/
                           
#define PGM_OPT_NBR_UNREACH     0x0B /*Neighbor Unreachable*/
                           
#define PGM_OPT_PATH_NLA        0x0C /*Path NLA*/
                           
#define PGM_OPT_INVALID         0x7F /*Option invalidated*/

#define PGM_OPT_END             0x80 /*This is set in the last option in the options list*/

/*the bit masks for the 'options' field in the pgm header:*/

#define PGM_OPT_PRESENT_CSC     0x01 /*if set, there are options fields in the packet*/
#define PGM_OPT_NET_SIG_CSC     0x02 /*if set, there are network significant options*/

#define PGM_OPT_PRESENT_RFC     0x80 /*if set, there are options fields in the packet*/
#define PGM_OPT_NET_SIG_RFC     0x40 /*if set, there are network significant options*/


#define PGM_OPT_VAR_PKTLEN      0x02 /*Packet is a parity packet for a transmission group of variable sized packets*/
#define PGM_OPT_PARITY          0x01 /*Packet is a parity packet*/

/* PGM packet types  */
#define  PGM_TYPE_SPM       0x00
#define  PGM_TYPE_ODATA     0x04
#define  PGM_TYPE_RDATA     0x05
#define  PGM_TYPE_NAK       0x08
#define  PGM_TYPE_NNAK      0x09
#define  PGM_TYPE_NCF       0x0A
#define  RMM_TYPE_ACK       0x0B

#define PGM_TYPE_POLL       0x01
#define PGM_TYPE_POLR       0x02
#define PGM_TYPE_NNAK       0x09
#define PGM_TYPE_SPMR       0x0C


/*other constants*/
#define PGM_OPT_HDR_LEN 4 /*length of the options header*/
#define PGM_OPT_LST_PRESENT 0x80
#define PGM_OPT_LIST_END 
/*------------------------------------------------*/

#define RUM_TYPE_SCP            0x0D
#define RUM_CFP_REQ_SHM         0x0F
#define RUM_CFP_REQ             0x10
#define RUM_CFP_REP             0x11
#define RUM_CFP_ACK             0x12
#define RUM_CON_HB              0x13
#define RUM_TX_STR_REP          0x14
#define RUM_RX_STR_REP          0x15
                                    
/*------------------------------------------------*/
#define RMM_OPT_MSG_INFO        0x20
#define RMM_OPT_MCAST_ADDR      0x21
#define RMM_OPT_TOPIC_NAME      0x22
#define RUM_OPT_QUEUE_NAME      0x22
#define RMM_OPT_MSG_TO_PAC      0x23
#define RMM_OPT_SRC_ADDR        0x24
#define RMM_OPT_FAST_ACK        0x25
#define RMM_OPT_UNICAST_INFO    0x28
#define RUM_OPT_CONTROL         0x28
#define RMM_OPT_CONNECT_MSG     0x29
#define RUM_OPT_CONNECTION_MSG  0x29
#define RMM_OPT_TIMESTAMP       0x30
#define RMM_OPT_RECEIVER_INFO   0x31
#define RMM_OPT_SLOW_REC        0x32

#define NLA_AFI                 1
#define NLA_AFI6                2
#define NLA_AFI_SHM             3
#define NLA_AFI_SIZE            4
/* 4 + 1 + 1 + 2 + 4 + 1 + 1 + 2 + 4 + 2 + 2 + 1 + (4|16) */
#define RMM_OPT_UNI_LEN         25

#define RMM_TIMESTAMP_OFFSET    5
#define RMM_TIMESTAMP_MASK      0x04

/*------------------------------------------------*/
#define RMM_STREAM_STATUS_HANDSHAKE   1  
#define RMM_STREAM_STATUS_REJECTED    2
#define RMM_STREAM_STATUS_ACTIVE      3
/*------------------------------------------------*/

typedef struct 
{
        uint16_t  srv_port;
        uint16_t  lcl_port;
        uint8_t   type;
        uint8_t   options;
        uint16_t  checksum;
        uint32_t  conn_id_ip ; 
        uint16_t  conn_id_port ; 
        uint16_t  conn_id_count ; 
        uint32_t  cip_sqn;
        uint32_t  conn_hbto ; 
        uint16_t  nla_afi;
        uint16_t  reserved ; 
 struct in6_addr  conn_src_addr ; 
}rumHeaderCCP;

/*
   pgm headers
*/

typedef struct 
{
        uint16_t  src_port;
        uint16_t  dest_port;
        uint8_t   type;
        uint8_t   options;
        uint16_t  checksum;
        uint32_t  gsid_high;
        uint16_t  gsid_low;
        uint16_t  tsdu_length;
}pgmHeaderCommon;

typedef struct 
{
        uint16_t  src_port;
        uint16_t  dest_port;
        uint8_t   type;
        uint8_t   options;
        uint16_t  checksum;
        uint32_t  gsid_high;
        uint16_t  gsid_low;
        uint16_t  tsdu_length;
        uint32_t  spm_sqn;
        uint32_t  trailing_sqn;
        uint32_t  leading_sqn;
        uint16_t  nla_afi;
        uint16_t  reserved ; 
        uint32_t  path_nla ;
}pgmHeaderSPM;

typedef struct 
{
        uint16_t  src_port;
        uint16_t  dest_port;
        uint8_t   type;
        uint8_t   options;
        uint16_t  checksum;
        uint32_t  gsid_high;
        uint16_t  gsid_low;
        uint16_t  tsdu_length;
        uint32_t  data_sqn;
        uint32_t  trailing_sqn;
}pgmHeaderData;

typedef struct
{
        uint16_t  src_port;
        uint16_t  dest_port;
        uint8_t   type;
        uint8_t   options;
        uint16_t  checksum;
        uint32_t  gsid_high;
        uint16_t  gsid_low;
        uint16_t  tsdu_length;
        uint32_t  nak_sqn;
        uint16_t  src_nla_afi;
        uint16_t  reserved_1;
        uint32_t  src_nla;
        uint16_t  mcast_nla_afi;
        uint16_t  reserved_2;
        uint32_t  mcast_nla;

} pgmHeaderNAK;

typedef struct 
{
        uint8_t   option_type;
        uint8_t   option_len;
        uint16_t  option_other;
} pgmOptHeader;

typedef struct
{
   uint16_t          PackOff ; 
   uint16_t          ConnInd ; 
   uint32_t          PackLen ; 
   uint32_t          PackSqn ; 
   uint32_t          Pad ; 
   rumConnectionID_t ConnId  ;
} rmmHeader ; 

typedef rmmHeader rumHeader ; 

#endif           
