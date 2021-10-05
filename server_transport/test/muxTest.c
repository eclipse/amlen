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
 * File: muxTest.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#define RX_BUFF_SIZE 1024
typedef union {
    uint32_t    iValue;
    struct {
        uint16_t    stream;
        uint8_t     cmd;
        uint8_t     rsrv;
    } hdr;
} ism_muxHdr_t;
enum muxcmd_e {
    MuxData              = 1,
    MuxCreateStream      = 2,
    MuxCloseStream       = 3,
    MuxCreatePhysical    = 'M',
    MuxCreatePhysicalAck = 4,
    MuxClosePhysical     = 5
};

static int sendToServer(int socket, uint8_t cmd, uint16_t stream, const char * s1, const char * s2) {
    int rc;
    char * buff = alloca(RX_BUFF_SIZE);
    char * pBuf = buff + 4;
    int mlen = 3;
    pBuf[0] = cmd;
    stream = htons(stream);
    pBuf++;
    memcpy(pBuf,&stream,2);
    pBuf += 2;
    if(cmd == MuxCreatePhysical) {
        *pBuf = 1;
        pBuf++;
    }
    if(s1) {
        uint16_t len = htons(strlen(s1));
        memcpy(pBuf,&len,2);
        pBuf += 2;
        strcpy(pBuf,s1);
        pBuf += strlen(s1);
    }
    if(s2) {
        uint16_t len = htons(strlen(s2));
        memcpy(pBuf,&len,2);
        pBuf += 2;
        strcpy(pBuf,s2);
        pBuf += strlen(s2);
    }
    mlen = pBuf-buff-4;
    mlen = htonl(mlen);
    memcpy(buff, &mlen,4);
    mlen = pBuf-buff;
    pBuf = buff;
    do {
        rc = send(socket, pBuf, mlen, 0);
        if(rc < 0) {
            perror("send failed");
            exit(1);
        }
        mlen -= rc;
        pBuf += rc;
    } while(mlen > 0);
    return 0;
}

static int readFromServer(int socket) {
    char * buff = alloca(RX_BUFF_SIZE);
    uint32_t biglen;
    uint16_t len = 0;
    ism_muxHdr_t hdr = {0};
    memset(buff, 0, RX_BUFF_SIZE);
    do {
        int rc = recv(socket, buff+len, RX_BUFF_SIZE-len, 0);
        if(rc < 0) {
            perror("recv failed");
            exit(1);
        }
        if(rc == 0) {
            fprintf(stderr, "Connection closed.\n");
            exit(1);
        }
        len += rc;
        if(len < 7)
            continue;
        memcpy(&biglen, buff, 4);
        biglen = ntohl(biglen);
        if((len-4) < biglen)
            continue;
        break;
    } while(1);
    buff += 4;
    hdr.hdr.cmd = buff[0];
    buff++;
    memcpy(&hdr.hdr.stream, buff, 2);
    buff += 2;
    hdr.hdr.stream = ntohs(hdr.hdr.stream);
    if(hdr.hdr.cmd == MuxCreatePhysicalAck) {
        int serverVersion = *buff;
        buff++;
        memcpy(&len,buff,2);
        len = ntohs(len);
        buff += 2;
        fprintf(stderr, "MuxCreatePhysicalAck received: stream=%u version=%d infoLength=%u info=%s\n",hdr.hdr.stream, serverVersion, len, buff);
    } else {
        if(hdr.hdr.cmd == MuxCloseStream) {
            int rc;
            memcpy(&rc, buff, sizeof(rc));
            rc = ntohl(rc);
            fprintf(stderr, "MuxCloseStream received: stream=%u rc=%d\n",hdr.hdr.stream, rc);
        } else {
            fprintf(stderr,"Unexpected command %d\n", hdr.hdr.cmd);
            exit(1);
        }
    }
    return 0;
}

int main(int argc, char * argv[] ){
  int clientSocket;
  struct sockaddr_in serverAddr;
  socklen_t addrLen;
  const char * serverHost = "127.0.0.1";
  int serverPort = 16101;
  int rc;
  uint16_t stream = 0x5558;
  clientSocket = socket(AF_INET, SOCK_STREAM, 0);
  if(argc > 1)
        serverHost = argv[1];
  if(argc > 2)
        serverPort = atoi(argv[2]);

  serverAddr.sin_family = AF_INET;
  serverAddr.sin_port = htons(serverPort);
  serverAddr.sin_addr.s_addr = inet_addr(serverHost);
  memset(serverAddr.sin_zero, '\0', sizeof serverAddr.sin_zero);

  addrLen = sizeof(serverAddr);
  rc = connect(clientSocket, (struct sockaddr *) &serverAddr, addrLen);
  if(rc) {
      perror("Connect to server failed");
      exit(1);
  }
  sendToServer(clientSocket, MuxCreatePhysical, stream, "MuxTestClient", "TestVersion");
  readFromServer(clientSocket);
  sendToServer(clientSocket, MuxCreateStream, 7, NULL, NULL);
  sendToServer(clientSocket, MuxCreateStream, 17, NULL, NULL);
  sendToServer(clientSocket, MuxCloseStream, 17, NULL, NULL);
  sendToServer(clientSocket, MuxCreateStream, 27, NULL, NULL);
  sendToServer(clientSocket, MuxClosePhysical, 27, NULL, NULL);
  sleep(10);
  close(clientSocket);
  return 0;
}
