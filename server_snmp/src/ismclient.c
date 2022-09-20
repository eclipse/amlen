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
 * Websockets Client for ISM Server
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <openssl/sha.h>
#include <ismutil.h>
#include <ismrc.h>
#include <ismclient.h>



#define htonll(x) __builtin_bswap64(x)
#define ntohll(x) htonll(x)

static char * ws_server_guid = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

#define WEBSOCKET_CLIENT_REQUEST_HEADER "\
GET %s HTTP/1.1\r\n\
Upgrade: WebSocket\r\n\
Connection: Upgrade\r\n\
Host: %s\r\n\
Sec-WebSocket-Version: 13\r\n\
Sec-WebSocket-Origin: %s\r\n\
Sec-WebSocket-Protocol: %s\r\n\
Sec-WebSocket-Key: %s\r\n\r\n"

static const char *server_addr = "127.0.0.1";
static const char *client_addr = "127.0.0.1";
static int serverport = 9089;
static int ismServerport = 9089;
static int mqServerport = 9086;
static int traceport = 0;
static int snmpport = 0;
static __thread char *rcvbuf = NULL;
static __thread int recvSize = 65536;
static __thread char *payload = NULL;
static __thread int maxPayloadSize = 256;
static __thread char *clientKey = NULL;
static __thread char *clientAcceptStr = NULL;
static __thread int clientFd = 0;
static const char *resource = "/";
static int serverAddr_isSpecified = 0;

static int ism_cli_maskJSONField(const char * line, char * fieldName, char * retValue)
{

        size_t n;
        if(line==NULL || retValue==NULL) return 1;

        char * ptr = strstr(line,fieldName);
        if(ptr == NULL){

            return 1;
        }
        ptr+=strlen(fieldName)+3;
        n = ptr-line;
        ptr = strchr(ptr,'\"');
        memcpy(retValue,line,n);
        sprintf(retValue+n,"******%s",ptr);

        return 0;
}

/* Encode string */
static char * ism_base64_encode( char *src , long nbytes ) {
    char *dest,*results;

    /* the Base64 printable encoding characters */
    static char ENC[64] = {
        'A','B','C','D','E','F','G','H','I','J','K','L','M',
        'N','O','P','Q','R','S','T','U','V','W','X','Y','Z',
        'a','b','c','d','e','f','g','h','i','j','k','l','m',
        'n','o','p','q','r','s','t','u','v','w','x','y','z',
        '0','1','2','3','4','5','6','7','8','9','+','/'
    };

    long i, j;                   /* counters */
    unsigned char byte_array[3]; /* input byte array */

    if (nbytes <= 0)
        return NULL;

    /* Malloc a buffer for the output string.  Add one byte for null termination */
    results = dest = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,2),((nbytes / 3) + 1 ) * 4 + 1);
    for (i = 0; i < nbytes; i+= 3) {
        /* check to make sure we don't run off the end of input string */
        if (i + 3 < nbytes)
            j = i + 3;
        else
            j = nbytes;

        memset(byte_array,0,3);
        memcpy(byte_array,&src[i],j-i);

        /* convert the three bytes into four Base64 characters */
        *dest++ = ENC[byte_array[0] >> 2];
        *dest++ = ENC[((byte_array[0] << 4) & 0x30) | ((byte_array[1] >> 4) & 0x0F)];

        if ( j - i == 3) {
            *dest++ = ENC[((byte_array[1] << 2) & 0x3C) | ((byte_array[2] >> 6) & 0x03)];
            *dest++ = ENC[byte_array[2] & 0x3F];
        } else if ( j - i == 2) {
            *dest++ = ENC[((byte_array[1] << 2) & 0x3C) | ((byte_array[2] >> 6) & 0x03)];
            *dest++ = '=';
        } else {
            *dest++ = '=';
            *dest++ = '=';
        }

    } /* end for loop */

    *dest = 0;
    return results;
}

/* Set websockets accept string for client */
static int ismcli_setClientAcceptStr(void) {
    int sz;
    char *input;
    char *clientSecretKey = "ISM-SECRETK-0f3D";
    // unsigned char *inputSha1;
    unsigned char obuf[20];
    int mdLen;

    clientKey = (char *)ism_base64_encode(clientSecretKey, 16);
    if (clientKey == NULL) {
        // printf("ClientKey returned as null: ");
        return -1;
    }

    sz = strlen(clientKey) + strlen(ws_server_guid);
    input = (char*) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,3),sz + 1);
    strcpy(input, clientKey);
    strcat(input, ws_server_guid);
    mdLen = ism_ssl_SHA1(input, sz, obuf);
    clientAcceptStr = ism_base64_encode((char *) obuf, mdLen);
    if (clientAcceptStr == NULL) {
        // printf("ClientAcceptStr returned as null: ");
        ism_common_free(ism_memory_snmp_misc,input);
        ism_common_free(ism_memory_snmp_misc,clientKey);
        return -1;
    }
    ism_common_free(ism_memory_snmp_misc,input);
    return 0;
}

/* Get element values from received frame */
static char * ismcli_getReqElementValue(const char *token, const char * inpBuffer, int *rc) {
    char *retBuffer = NULL;
    const char *tmpBuffer = inpBuffer;
    int len = 0;

    *rc = 0;

    /* move pointer after token */
    tmpBuffer += strlen(token);
    /* check if line has some data */
    len = strstr(tmpBuffer, "\r\n") - tmpBuffer + 1;
    if (len == 0) {
        *rc = ENODATA;
        return NULL;
    }

    /* allocate memory for return buffer - caller should free memory */
    retBuffer = (char *) ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,7),len);
    if (retBuffer == NULL) {
        *rc = errno;
        return NULL;
    }

    /* copy rest of the line content to return buffer */
    memcpy(retBuffer, tmpBuffer, len - 1);
    retBuffer[len - 1] = 0;

    return retBuffer;
}

/* Verify server response */
static int ismcli_verify_server_response(char *buffer, int len) {
    int statusCode;
    char *startPtr = buffer;
    char *endPtr = buffer + len;
    int rc = -1;

    if (len < 32) {
        // printf("Not enough data in server response\n");
        return -1;
    }
    /* get HTTP status code */
    if (sscanf(startPtr, "HTTP/1.1 %d Switching Protocols\r\n", &statusCode) != 1) {
        // printf("Received incorrect server handshake: code=%d\n", statusCode);
        return -1;
    }

    // printf("Received server handshake: code=%d\n", statusCode);
    startPtr = strstr(startPtr, "\r\n") + 2;

    /* loop thru received tokens and verify accept string */
    while (startPtr < endPtr && startPtr[0] != '\r' && startPtr[1] != '\n') {
        if ((memcmp(startPtr, "Sec-WebSocket-Accept: ", 22)) == 0) {
            char *value = NULL;
            value = ismcli_getReqElementValue("Sec-WebSocket-Accept: ", startPtr, &rc);
            if ( value && strcmp(value, clientAcceptStr) == 0 )
                rc = 0;
            if (value)  ism_common_free(ism_memory_snmp_misc,value);
            break;
        }
        /* move start pointer to end of line */
        startPtr = strstr(startPtr, "\r\n") + 2;
    }
    return rc;
}

/* Create websockets send frame */
static int ismcli_create_send_frame(const char *message, int message_len, char **frame_buffer, int *frame_len) {
    int len = 0;
    char * frame = (char *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_snmp_misc,9),1, message_len + 14);
    if (frame == NULL) {
        return -1;
    }

    frame[0] = '\x81';
    if (message_len <= 125) {
        frame[1] = 128 + message_len;
        frame[2] = '\x00';
        frame[3] = '\x00';
        frame[4] = '\x00';
        frame[5] = '\x00';
        memcpy(frame+6, message, message_len);
        len = message_len + 6;
    } else if (message_len > 125 && message_len <= 65536) {
        frame[1] = (char)(126|0x80);
        frame[2] = (char)(message_len>>8);
        frame[3] = (char)message_len;
        frame[4] = '\x00';
        frame[5] = '\x00';
        memcpy(frame+8, message, message_len);
        len = message_len + 8;
    } else {
        uint64_t len64 = (uint64_t) htonll((uint64_t)message_len);
        frame[1] = 127;
        memcpy(frame+2, &len64, 8);
        memcpy(frame+14, message, message_len);
        len = message_len + 10;
    }

    *frame_buffer = frame;
    *frame_len = len;

    return 0;
}

/* Parse websocket receive frame from server */
static int ismcli_parse_receive_frame(char *frame_buffer, int frame_len, char **pl_buffer, int *pl_len, int *frame_type, int *rc) {
    uint64_t len;
    const char *p;
    unsigned char mask[4];
    int i;
    int maskBitSet;
    xUNUSED int resvBitSet;
    int opCode;
    int finBit;
    int received = 0;
    int remain = 0;

    *rc = 0;

    if (frame_len < 6) {
        *rc = 2;
        return frame_len;
    }

    /* get mask, FIN bit, opCode and payload length */
    maskBitSet = frame_buffer[1] & 0x80 ? 1 : 0;
    finBit = frame_buffer[0] & 0x80 ? 1 : 0;
    opCode = frame_buffer[0] & 0x0f;
    len = frame_buffer[1] & 0x7f;
    resvBitSet = frame_buffer[0] & 0x70 ? 1 : 0;

    /* Return the opcode */
    if (frame_type)
        *frame_type = opCode;

    TRACE(9, "frameLen=%d opCode=%d finBit=%d msgLen=%ld\n", frame_len, opCode, finBit, len);

    /* get payload length */
    if (len <= 125) {
        p = frame_buffer + 2 + (maskBitSet ? 4 : 0);
        if (maskBitSet)
            memcpy(&mask, frame_buffer + 2, sizeof(mask));
    } else if (len == 126) {
        uint16_t sz16;
        memcpy(&sz16, frame_buffer + 2, sizeof(uint16_t));
        len = ntohs(sz16);
        p = frame_buffer + 4 + (maskBitSet ? 4 : 0);
        if (maskBitSet)
            memcpy(&mask, frame_buffer + 4, sizeof(mask));
    } else if (len == 127) {
	    uint64_t sz64;
	    memcpy(&sz64,(frame_buffer+2),sizeof(uint64_t));
        len = ntohll(sz64);
        p = frame_buffer + 10 + (maskBitSet ? 4 : 0);
        if (maskBitSet)
            memcpy(&mask, frame_buffer + 10, sizeof(mask));
    }

    received = frame_len - (p - frame_buffer);
    if (len > received) { /* more data to come */
        TRACE(9, "More data to come: frameLen=%d msglen=%ld received=%d\n", frame_len, len, received);
        *rc = 2;
        recvSize = len + 64;
        rcvbuf = ism_common_realloc(ISM_MEM_PROBE(ism_memory_snmp_misc,10),rcvbuf, recvSize);
        return frame_len;
    }

    /* add data to frame payload */
    if (len > maxPayloadSize) {
        maxPayloadSize += len;
        payload = ism_common_realloc(ISM_MEM_PROBE(ism_memory_snmp_misc,11),payload, maxPayloadSize);
    }
    memcpy(payload, p, len);

    /* apply mask if set */
    if ( maskBitSet ) {
        for (i = 0; i < len; ++i) {
            payload[i] = (unsigned char) p[i] ^ mask[i % 4];
        }
    }

    *pl_buffer = payload;
    *pl_len = len;

    p += len;
    remain = frame_len - (p - frame_buffer);

    return remain;
}

/* COnnect to the server, create websockets frame, send and receive message from ISM server */
static char * ismcli_connectAndSendMessage(const char *protocol, const char *messageToSend, int *rc) {
    struct sockaddr_in clientAddr, serverAddr;
    int nBytes;
    char *frame = NULL;
    int framelen = 0;
    char *client_shake;
    int client_shake_len;
    char server_buffer[1024];
    int  bytes_read;
    int  pos;
    char * ptr;
    int fragpos = 0;
    char *result = NULL;
    int32_t flags;

    clientFd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if ( clientFd < 0 ) {
        TRACE(2, "socket failed: %s\n", strerror(errno));
        *rc = ISMRC_Error;
        return result;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = inet_addr(server_addr);
    serverAddr.sin_port = htons(serverport);

    flags = 1;
    setsockopt(clientFd, SOL_SOCKET, SO_REUSEADDR, (const char *) &flags, sizeof flags);


    /* bind socket to client address (any port) if specified */
    if (client_addr != NULL && serverAddr_isSpecified == 0) {
        clientAddr.sin_family = AF_INET;
        clientAddr.sin_addr.s_addr = inet_addr(client_addr);
        clientAddr.sin_port = htons(0);

        if (bind(clientFd, (struct sockaddr *)&clientAddr, sizeof(clientAddr)) < 0) {
            TRACE(2, "bind failed: %s\n", strerror(errno));
            *rc = ISMRC_Error;
            close(clientFd);
            return result;
        }
    }

    if (connect(clientFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0) {
        TRACE(2, "connect failed: %s\n", strerror(errno));
        *rc = ISMRC_UnableToConnect;
        close(clientFd);
        return result;
    }

    /* start handshake */
    if ( clientKey == NULL && ismcli_setClientAcceptStr() != 0 ) {
        TRACE(2, "failed to client accept string\n");
        *rc = ISMRC_Error;
        close(clientFd);
        return result;
    }

    /* set origin */
    char origin[32];

    sprintf(origin, "http://%s:%d", server_addr, serverport);
    client_shake_len = strlen(WEBSOCKET_CLIENT_REQUEST_HEADER)
        + strlen(clientKey)
        + strlen(origin) + strlen(resource)
        + strlen(protocol) + strlen(server_addr) - 10;
    client_shake = (char *) alloca(client_shake_len + 1);
    sprintf(client_shake, WEBSOCKET_CLIENT_REQUEST_HEADER,
        resource, server_addr, origin, protocol, clientKey);

    // TRACE(9, "Send handshake: len=%d\n%s \n", client_shake_len, client_shake);
    TRACE(9, "Send handshake: len=%d\n", client_shake_len);

    nBytes = send(clientFd, client_shake, client_shake_len, 0);
    if ( nBytes < 0 ) {
        TRACE(2, "write failed: %s\n", strerror(errno));
        *rc = ISMRC_NetworkError;
        close(clientFd);
        return result;
    }

    TRACE(9, "Client handshake frame sent\n");
    bzero(server_buffer, 1024);

    nBytes = read(clientFd, server_buffer, 1024);
    if (nBytes < 0) {
        TRACE(2, "read data from server failed: %s\n", strerror(errno));
        *rc = ISMRC_NetworkError;
        close(clientFd);
        return result;
    }

    server_buffer[nBytes] = 0;

    /* verify handshake */
    if ( ismcli_verify_server_response(server_buffer, nBytes) < 0 ) {
        TRACE(2, "Verify server response failed\n");
        *rc = ISMRC_Failure;
        close(clientFd);
        return result;
    }

    TRACE(9, "Handshake is OK\n");

    if ( ismcli_create_send_frame(messageToSend, strlen(messageToSend), &frame, &framelen) < 0 ) {
        // printf("Error in send frame: %d\n", rc);
    }

    nBytes = 0;
    while (nBytes == 0) {
        nBytes = send(clientFd, frame, framelen, 0);
        if ( nBytes < 0 ) {
            TRACE(2, "write failed: %s\n", strerror(errno));
            *rc = ISMRC_NetworkError;
            close(clientFd);
            ism_common_free(ism_memory_snmp_misc,frame);
            return result;
        }
    }
    ism_common_free(ism_memory_snmp_misc,frame);

    // printf("Wait for server response\n");

    int done = 0;
    int rc1 = 0;
    for (;;) {
        pos = fragpos;
        ptr = rcvbuf + pos;
        bytes_read = recv(clientFd, ptr, recvSize-pos, 0);
        // printf("bytes_read = %d\n", bytes_read);
        if (bytes_read <= 0) {
            if ( errno == EAGAIN || errno == EWOULDBLOCK ) {
                continue;
            }
            TRACE(2, "recv failed from server: %s\n", strerror(errno));
            *rc = ISMRC_NetworkError;
            close(clientFd);
            return result;
        }

        int bytes_processed = 0;
        int bytes_remaining = bytes_read + pos;
        fragpos = 0;

        for (;;) {
            char *pload = NULL;
            int plen = 0;
            int ret = 0;

            ret = ismcli_parse_receive_frame(rcvbuf + bytes_processed,
                          bytes_remaining, &pload, &plen, NULL, &rc1);
            if ( ret == 0 || (ret != 0 && rc1 == 0)) {
                int cl = 0;

                /* check if connection is closing */
                if ( rc1 == 0x8 )
                    cl = 1;

                /* check if frame has any payload */
                if ( plen ) {
                    result = (char *)ism_common_calloc(ISM_MEM_PROBE(ism_memory_snmp_misc,14),1, plen + 1);
                    if ( result != NULL )
                        memcpy(result, pload, plen);
                    // printf("%s\n", result);
                    done = 1;
                    break;
                }

                if ( cl ) {
                    close(clientFd);
                    return result;
                }


                if (ret != 0 && rc1 == 0) {
                    /* more data to process */
                    // printf("Process remaining packet: size:%d remain:%d\n", ret, bytes_remaining);
                    bytes_processed += bytes_remaining - ret;
                    bytes_remaining = ret;
                    continue;
                }
                if ( ret != 0 ) {
                    // printf("Discard remaining packet: %d\n", ret);
                    continue;
                }
                break;
            } else if ( ret == -1 ) {
                TRACE(2, "parse_receive_frame failed.\n");
                *rc = ISMRC_BadClientData;
                close(clientFd);
                return result;
            } else if ( ret > 0 && rc1 == 2 ) {
                // printf("Fragmented Message\n");
                fragpos = (bytes_read + pos) - bytes_processed;
                memmove(rcvbuf, rcvbuf + bytes_processed, fragpos);
                break;
            }
        }
        if (done == 1)
            break;
    }

    close(clientFd);
    return result;
}

/*
 * caller should free return buffer
 */
XAPI char * ismcli_ISMClient(char *user, char *protocol, char *command, int proctype) {
    char *result;
    char *s_addr = NULL;
    int rc = ISMRC_OK;

    if ( !command || *command == '\0' )
        return NULL;

    /* Get server IP address from Environment variable IMA_SERVER_ADDRESS and IMA_SERVER_PORT */
    char *saddress = getenv("IMA_SERVER_ADDRESS");
    if (saddress ) {
        TRACE(2, "IMA_SERVER_ADDRESS is specified: %s\n", saddress);
        /* Validate saddress */
        struct addrinfo hints, *res;
        char *ipAddr = saddress;
        int status;
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = 0;
        if ((status = getaddrinfo(ipAddr, NULL, &hints, &res)) == 0) {
            /* Use specified address */
            s_addr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_snmp_misc,1000),saddress);
            serverAddr_isSpecified = 1;
            freeaddrinfo(res);
        } else {
            s_addr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_snmp_misc,1000),"127.0.0.1");
        }
    } else {
            s_addr = ism_common_strdup(ISM_MEM_PROBE(ism_memory_snmp_misc,1000),"127.0.0.1");
    }

    server_addr = s_addr;

    char *aport = getenv("IMA_SERVER_PORT");
    if (aport) {
        int sport = atoi(aport);
        TRACE(2, "IMA_SERVER_PORT is specified: %s\n", aport);
        if ( sport >= 0 )
            ismServerport = sport;
    }


    if ( proctype == PROCTYPE_MQC )
    	serverport = mqServerport;
    else if (proctype == PROCTYPE_TRACE) {
    	if (traceport == 0) {
    		traceport = ism_common_getIntConfig("TraceProcessorPort", 9085);
    	}
    	serverport = traceport;
    } else if (proctype == PROCTYPE_SNMP) {
    	if (snmpport == 0) {
    		snmpport = ism_common_getIntConfig("SnmpAgentPort", 9065);
    	}
    	serverport = snmpport;
    }
    else
    	serverport = ismServerport;

    TRACE(2, "User=%s protocol=%s CMD: %s\n", user, protocol, command);

    rcvbuf = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,15),recvSize);
    payload = (char *)ism_common_malloc(ISM_MEM_PROBE(ism_memory_snmp_misc,16),maxPayloadSize);
    result = ismcli_connectAndSendMessage(protocol, command, &rc);
    if ( result == NULL ) {
        TRACE(2, "Didn't receive any response from server. RC=%d\n", rc);
        /* create JSON error string and add to result */
        char rbuf[512];
        char buf[256];
        char *errstr = NULL;
        errstr = (char *)ism_common_getErrorStringByLocale(rc, ism_common_getLocale(), buf, 256);
        if ( errstr )
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"%s\" }", rc, errstr);
        else
            sprintf(rbuf, "{ \"RC\":\"%d\", \"ErrorString\":\"Unknown\" }", rc);
        result = ism_common_strdup(ISM_MEM_PROBE(ism_memory_snmp_misc,1000),rbuf);
    } else {
    	char * res_logBuffer=result;
    	char maskBuffer[2048];
    	if(strstr(result, "BindPassword")!=NULL){
	    	res_logBuffer=maskBuffer;
	    	ism_cli_maskJSONField(result, "BindPassword", res_logBuffer);
	    }
        TRACE(2, "RES: %s\n", res_logBuffer);
    }
    ism_common_free(ism_memory_snmp_misc,payload);
    payload = NULL;
    ism_common_free(ism_memory_snmp_misc,rcvbuf);
    rcvbuf = NULL;
    ism_common_free(ism_memory_snmp_misc,s_addr);
    s_addr = NULL;

    return result;
}

