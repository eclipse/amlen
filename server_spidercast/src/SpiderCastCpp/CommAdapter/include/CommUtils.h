// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//

/*
 * CommUtils.h
 *
 *  Created on: May 14, 2015
 */

#ifndef SPDR_COMMUTILS_H_
#define SPDR_COMMUTILS_H_


#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <errno.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
//#include <linux/if_arp.h>
#include <netpacket/packet.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ifaddrs.h>

#include <vector>
#include <string>

#ifndef IF_NAMESIZE
#define IF_NAMESIZE IFNAMSIZ
#endif

namespace spdr
{

class CommUtils
{
public:

  struct NICInfo
  {
    NICInfo();

    std::string name;
    std::string address_v4;
    std::string address_v6;
    uint32_t index;
    bool multicast;
    bool up;
    bool loopback;

    std::string toString() const;
  };


  /**
   * Get an address from NIC name, or address, or ANY/ALL/NONE.
   *
   * @param nic
   * @return
   */
  static std::string get_nic_addr(const char *nic);


  /**
   * Get the NIC info all all the NICs.
   *
   * @param nic_vec fill this vector with info on all the NICs
   * @return
   */
  static void get_all_nic_info(std::vector<NICInfo>& nic_vec);


  /**
   * Get the NIC info from NIC name or IP address.
   *
   * @param nic input
   * @param nicInfo output, when no error
   * @param errCode output, when error
   * @param errMsg output, when error
   * @return true if success
   */
  static bool get_nic_info(const char *nic, NICInfo* pNICInfo, int *errCode, std::string *errMsg);

  static NICInfo get_nic_up_mc_v6();

private:
  CommUtils();
  virtual ~CommUtils();


};


int set_CLOEXEC(int fd);

} /* namespace spdr */

#endif /* SPDR_COMMUTILS_H_ */
