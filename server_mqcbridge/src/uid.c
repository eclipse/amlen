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

#include <mqcInternal.h>

#include <ifaddrs.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <openssl/rand.h>

int mqc_getObjectIdentifier(char * buffer)
{
  int rc, fd;
  struct ifaddrs * ifap, *ifa;
  struct ifreq ifr;
  char * bp = buffer;
  int startPos = 0;

  TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

  /* Assume the worst */
  rc = RC_MQC_RECONNECT;

  /* Based on xcsGetMacAddress */

  /* Create socket for use with ioctl() */
  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) != -1)
  {
    /* Create list of network interfaces */
    if (getifaddrs(&ifap) == 0)
    {
      /* Step through list */
      for (ifa = ifap; ifa; ifa = ifa -> ifa_next)
      {
        if (ifa->ifa_addr)
        {
          TRACE(MQC_NORMAL_TRACE, "ifa_name<%s> sa_family(%d) ifa_flags(0X%8.8X)\n",
                ifa -> ifa_name, ifa -> ifa_addr -> sa_family, ifa -> ifa_flags);

          /* Find non-loopback AF_INET network interface */
          if ((ifa -> ifa_addr -> sa_family == AF_INET) &&
             !(ifa -> ifa_flags & IFF_LOOPBACK))
          {
            memset(&ifr, 0, sizeof(struct ifreq));

            ifr.ifr_addr.sa_family = AF_INET;
            strcpy(ifr.ifr_name, ifa -> ifa_name);

           /* Determine hardware address */
            if (ioctl(fd, SIOCGIFHWADDR, &ifr) != -1)
            {
              static char base36 [36] = {
                      '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
                      'G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V',
                      'W','X','Y','Z'
              };

              TRACE(MQC_NORMAL_TRACE, "MAC address %02X:%02X:%02X:%02X:%02X:%02X\n",
                    (unsigned char)ifr.ifr_hwaddr.sa_data[0],
                    (unsigned char)ifr.ifr_hwaddr.sa_data[1],
                    (unsigned char)ifr.ifr_hwaddr.sa_data[2],
                    (unsigned char)ifr.ifr_hwaddr.sa_data[3],
                    (unsigned char)ifr.ifr_hwaddr.sa_data[4],
                    (unsigned char)ifr.ifr_hwaddr.sa_data[5]);

              for (; startPos<6; startPos++)
              {
                *bp++ = base36[(unsigned char)ifr.ifr_hwaddr.sa_data[startPos]%36];
              }

              /* Fill up with random characters */
              uint64_t rval;
              uint8_t * randbuf = (uint8_t *)&rval;

              RAND_bytes(randbuf, 8);
              for (; startPos<12; startPos++) {
                  *bp++ = base36[(int)(rval%36)];
                  rval /= 36;
              }

              TRACE(MQC_NORMAL_TRACE, "Object Identifier '%.12s'\n", buffer);

              rc = 0;
              break;
            }
            else
            {
              TRACE(MQC_NORMAL_TRACE, "ioctl(SIOCGIFHWADDR) errno(%d)\n", errno);
            }
          }
        }
        else
        {
            TRACE(MQC_NORMAL_TRACE, "Skipping ifa_name<%s> with null ifa_addr. ifa_flags(0X%8.8X)\n",
                  ifa -> ifa_name, ifa -> ifa_flags);
        }
      }

      freeifaddrs(ifap);
    }
    else
    {
      TRACE(MQC_NORMAL_TRACE, "getifaddrs() errno(%d)\n", errno);
    }

    close(fd);
  }
  else
  {
    TRACE(MQC_NORMAL_TRACE, "socket() errno(%d)\n", errno);
  }

  TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
  return rc;
}

/* When MQ Connectivity starts, write into the trace all the durable  */
/* subscriptions that we find in the server ie the ones that we are   */
/* likely to resume.                                                  */

int mqc_displayRecoveredSubscriptions(void)
{
    int rc = 0;
    int exitrc = 0;
    int i = 0;

    int subscriptionCount = 0;
    ismc_durablesub_t *pDurableSubscriptions = NULL;

    ismc_session_t *pSess = NULL;
    int transacted = 0;
    int ackmode = SESSION_CLIENT_ACKNOWLEDGE;

    char subNamePattern[] = "__MQConnectivity.*.SUB";

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession <%p>\n", pSess);
    if (!pSess)
    {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    rc = ismc_listDurableSubscriptions(pSess,
                                       &pDurableSubscriptions,
                                       &subscriptionCount);

    i = (subscriptionCount - 1);

    while (i >= 0)
    {
        /* If this is an MQ Connectivity subscription then put its    */
        /* name in the trace.                                         */
        if (ism_common_match(pDurableSubscriptions[i].subscriptionName,
                             subNamePattern))
        {
            TRACE(MQC_ISMC_TRACE,
                  "Discovered subscription: %s\n",
                  pDurableSubscriptions[i].subscriptionName);
        }
        i--;
    }

    ismc_freeDurableSubscriptionList(pDurableSubscriptions);

MOD_EXIT:

    if (pSess != NULL)
    {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);
        if (exitrc)
        {
            mqc_ISMCError("ismc_closeSession");
        }
        else
        {
            exitrc = ismc_free(pSess);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);

    return rc;
}

int mqc_setObjectIdentifier(void)
{
    int exitrc, rc = 0;
    int i = 0;

    ismc_session_t *pSess = NULL;
    int transacted = 0;
    int ackmode = SESSION_CLIENT_ACKNOWLEDGE;

    mqcManagerRecord_t mgrRecord;
    ismc_manrec_t pTempMan;

    TRACE(MQC_FNC_TRACE, FUNCTION_ENTRY "\n", __func__);

    if (mqcMQConnectivity.objectIdentifier[0])
    {
        TRACE(MQC_NORMAL_TRACE,
              "Using objectIdentifier %s from server store\n",
              mqcMQConnectivity.objectIdentifier);
        goto MOD_EXIT;
    }

    pSess = ismc_createSession(mqcMQConnectivity.ima.pConn, transacted, ackmode);
    TRACE(MQC_ISMC_TRACE, "ismc_createSession <%p>\n", pSess);
    if (!pSess)
    {
        rc = mqc_ISMCError("ismc_createSession");
        goto MOD_EXIT;
    }

    /* Get the Object Identifier from MAC address of underlying machine */
    for (i = 0; i < 30; i++)
    {
        rc = mqc_getObjectIdentifier(mqcMQConnectivity.objectIdentifier);
        if (rc == 0)
        {
            TRACE(MQC_NORMAL_TRACE,
                  "Obtained Object Identifier %s after %d attempt(s)\n",
                  mqcMQConnectivity.objectIdentifier, i + 1);
            break;
        }
        ism_common_sleep(1 * 1000 * 1000);
    }
    if (rc)
    {
        TRACE(5, "Failed to obtain Object Identifier after %d attempts\n", i);
        goto MOD_EXIT;
    }

    /* Store the result in the server store.                      */
    memset(&mgrRecord, 0, sizeof(mqcManagerRecord_t));
    mgrRecord.eyecatcher = MANAGER_RECORD_EYECATCHER;
    mgrRecord.version = MANAGER_RECORD_VERSION;
    mgrRecord.size = offsetof(mqcManagerRecord_t, object) +
                     sizeof(mgrRecord.object.objectIdentifier);
    mgrRecord.type = MRTYPE_OBJECTIDENTIFIER;
    memcpy(mgrRecord.object.objectIdentifier, mqcMQConnectivity.objectIdentifier, MQC_OBJECT_IDENTIFIER_LENGTH);

    pTempMan = ismc_createManagerRecord(pSess,
                                        (const void *) &mgrRecord,
                                        mgrRecord.size);
    if (!pTempMan)
    {
        rc = mqc_ISMCError("ismc_createManagerRecord");
        goto MOD_EXIT;
    }

    ism_common_free(ism_memory_mqcbridge_misc,pTempMan);
    pTempMan = NULL;

    TRACEDATA(MQC_MQAPI_TRACE,
              "Created manager record for objectIdentifier: ",
              0,
              &mgrRecord,
              mgrRecord.size,
              mgrRecord.size);

MOD_EXIT:

    if (pSess != NULL)
    {
        exitrc = ismc_closeSession(pSess);
        TRACE(MQC_ISMC_TRACE, "ismc_closeSession rc(%d)\n", exitrc);
        if (exitrc)
        {
            mqc_ISMCError("ismc_closeSession");
        }
        else
        {
            exitrc = ismc_free(pSess);
            TRACE(MQC_ISMC_TRACE, "ismc_free rc(%d)\n", exitrc);
        }
    }

    TRACE(MQC_FNC_TRACE, FUNCTION_EXIT "rc=%d\n", __func__, rc);
    return rc;
}

