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
 * CommUtils.cpp
 *
 *  Created on: May 14, 2015
 */

#include <CommUtils.h>
#include <iostream>
#include <sstream>

namespace spdr
{

CommUtils::CommUtils()
{
}

CommUtils::~CommUtils()
{
}

CommUtils::NICInfo::NICInfo() : name(), address_v4(), address_v6(), index(0), multicast(false), up(false), loopback(false) {}

std::string CommUtils::NICInfo::toString() const
{
	std::ostringstream oss;
	oss << "index=" << index
			<< ", name=" << name
			<< ", MC=" << multicast
			<< ", UP=" << up
			<< ", LB=" << loopback
			<< ", inet=" << address_v4
			<< ", inet6=" << address_v6 << ";";

	return oss.str();
}

/*-------------------------------------------------------------*/

char *mcc_strdup(const char *s)
{
	if (s)
		return strdup(s);
	return NULL;
}

size_t mcc_strlcpy(char *dst, const char *src, size_t size)
{
	size_t rc = 0;
	char *p, *q;
	const char *s;

	if (src)
	{
		s = src;
		if (dst && size > 0)
		{
			p = dst;
			q = p + (size - 1);
			while (p < q && *s)
				*p++ = *s++;
			*p = 0;
		}
		while (*s)
			s++;
		rc = (s - src);
	}
	else if (dst && size > 0)
		*dst = 0;

	return rc;
}

//----------------------------------------------------------

#define ERR_STR_LEN 256
#define ADDR_STR_LEN 64
typedef struct
{
	int errCode;
	int errLen;
	char errMsg[ERR_STR_LEN];
} errInfo;

#define SA  struct sockaddr
#define SA4 struct sockaddr_in
#define SA6 struct sockaddr_in6
#define SAl struct sockaddr_ll

typedef union
{
	SA sa;SA4 sa4;SA6 sa6;
} SAA, SAS;

#define IA4 struct in_addr
#define IA6 struct in6_addr

typedef union
{
	IA4 ia4;IA6 ia6;
} IAA;

typedef struct ipFlat_
{
	struct ipFlat_ *next;
	int length;
	int scope;
	IAA bytes[2];
	char label[IF_NAMESIZE];
} ipFlat;

#define ENSURE_LEN(x) if(((size_t)(x)->length) > sizeof((x)->bytes))(x)->length=sizeof((x)->bytes)

typedef struct nicInfo_
{
	struct nicInfo_ *next;
	char name[IF_NAMESIZE];
	int mtu;
	int flags;
	int index;
	int type;
	int naddr;
	ipFlat *addrs;
	ipFlat haddr[1];
} nicInfo;

#define DBG_PRINT(fmt...)
#define DBG_PRT(x)

/*-------------------------------------------------*/

static const char *RT_SCOPE(int scope)
{
	static char unkn[32];
	switch (scope)
	{
	case RT_SCOPE_UNIVERSE:
		return "global";
	case RT_SCOPE_SITE:
		return "site";
	case RT_SCOPE_LINK:
		return "link";
	case RT_SCOPE_HOST:
		return "host";
	case RT_SCOPE_NOWHERE:
		return "none";
	default:
	{
		snprintf(unkn, 32, "Unknown scope: %d", scope);
		return unkn;
	}
	}
}
/*-------------------------------------------------------------*/
static char *ip2str(ipFlat *ip, char *str, int len)
{
	char H[16] =
	{ '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e',
			'f' };
	char s64[ADDR_STR_LEN];
	if (ip->length == sizeof(IA4))
		inet_ntop(AF_INET, ip->bytes, s64, ADDR_STR_LEN);
	else if (ip->length == sizeof(IA6))
		inet_ntop(AF_INET6, ip->bytes, s64, ADDR_STR_LEN);
	else
	{
		char *p;
		uint8_t *b = (uint8_t *) ip->bytes;
		uint8_t *e = b + ip->length;
		for (p = s64; b < e; b++)
		{
			*p++ = H[(*b) >> 4];
			*p++ = H[(*b) & 15];
			*p++ = ':';
		}
		*(--p) = 0;
	}
	snprintf(str, len, "%s %s %s", s64, ip->label, RT_SCOPE(ip->scope));
	return str;
}
/*-------------------------------------------------------------*/


static int buildIfList_if(errInfo *ei, nicInfo **niHead)
{
	nicInfo *ifh = NULL, *ift = NULL, *ifi;
	int ret = -1;
	struct ifaddrs *ifa0, *ifa ;
	char s96[96];

	if ( getifaddrs(&ifa0) )
	{
		ei->errCode = errno;
		snprintf(ei->errMsg, ei->errLen,
				"_at_%d: system call %s() failed with error %d (%s)",
				__LINE__, "getifaddrs", ei->errCode, strerror(ei->errCode));
		goto err ;
	}

	for ( ifa=ifa0 ; ifa ; ifa=ifa->ifa_next )
	{
		if ( ifa->ifa_addr->sa_family==AF_PACKET )
		{
			struct sockaddr_ll *sal = (struct sockaddr_ll *)ifa->ifa_addr ;
			for ( ifi=ifh ; ifi && ifi->index != sal->sll_ifindex ; ifi=ifi->next ) ; // empty body
			if ( !ifi )
			{
				if (!(ifi = (nicInfo *)malloc(sizeof(nicInfo))) )
				{
					ei->errCode = errno;
					snprintf(ei->errMsg, ei->errLen,
							"_at_%d: system call %s() failed with error %d (%s)",
							__LINE__, "malloc", ei->errCode, strerror(ei->errCode));
					goto err;
				}
				memset(ifi,0,sizeof(nicInfo)) ;
				if ( ift ) ift->next = ifi ;
				else ifh = ifi ;
				ift = ifi ;
				ifi->index = sal->sll_ifindex ;
				ifi->type  = sal->sll_hatype ;
				ifi->flags = ifa->ifa_flags ;
				mcc_strlcpy(ifi->name, ifa->ifa_name, IF_NAMESIZE) ;
				ifi->haddr->length = sal->sll_halen ;
				memcpy(ifi->haddr->bytes, sal->sll_addr, ifi->haddr->length);
			}
		}
	}
	for ( ifa=ifa0 ; ifa ; ifa=ifa->ifa_next )
	{
		ipFlat ip;
		memset(&ip,0,sizeof(ip));
		if ( ifa->ifa_addr->sa_family==AF_INET )
		{
			SA4 *sa4 = (SA4 *)ifa->ifa_addr ;
			ip.length = sizeof(IA4) ;
			memcpy(ip.bytes,&sa4->sin_addr,ip.length) ;
		}
		else
			if ( ifa->ifa_addr->sa_family==AF_INET6)
			{
				SA6 *sa6 = (SA6 *)ifa->ifa_addr ;
				ip.length = sizeof(IA6) ;
				memcpy(ip.bytes,&sa6->sin6_addr,ip.length) ;
			}
		if ( ip.length )
		{
			char nm[IF_NAMESIZE+8];
			strncpy(ip.label, ifa->ifa_name, IF_NAMESIZE) ;
			strncpy(nm, ifa->ifa_name, IF_NAMESIZE) ;
			char *p = strstr(nm,":");
			if ( p ) *p=0;
			for ( ifi=ifh ; ifi && strncmp(nm,ifi->name,IF_NAMESIZE) ; ifi=ifi->next ) ; // empty body
			if ( ifi )
			{
				ipFlat *ipi, *ipp;
				for ( ipp=NULL,ipi=ifi->addrs ; ipi ; ipp=ipi,ipi=ipi->next )
					if ( ipi->length == ip.length && !memcmp(ipi->bytes,ip.bytes,ip.length) ) break ;
				if ( !ipi )
				{
					if ( !(ipi=(ipFlat *)malloc(sizeof(ipFlat))) )
					{
						ei->errCode = errno;
						snprintf(ei->errMsg, ei->errLen,
								"_at_%d: system call %s() failed with error %d (%s)",
								__LINE__, "malloc", ei->errCode, strerror(ei->errCode));
						goto err;
					}
					memcpy(ipi,&ip,sizeof(ip)) ;
					ifi->naddr++ ;
					if ( ipp ) ipp->next = ipi ;
					else ifi->addrs = ipi ;
				}
				else
				{
					std::cerr << "interface not found for address "
							<< ip2str(&ip, s96, 96) << std::endl;
				}
			}
		}
	}
	freeifaddrs(ifa0);

	*niHead = ifh;
	ret = 0;
	err:
	if (ret)
	{
		ipFlat *ipi;
		while (ifh)
		{
			ifi = ifh;
			ifh = ifi->next;
			while (ifi->addrs)
			{
				ipi = ifi->addrs;
				ifi->addrs = ipi->next;
				free(ipi);
			}
			free(ifi);
		}
	}
	return ret;
}

/*-------------------------------------------------------------*/

static int buildIfList_nl(errInfo *ei, nicInfo **niHead)
{
	nicInfo *ifh = NULL, *ift = NULL, *ifi;
	int fd = 0, rc, i, seq = 0, ret = -1;
	ssize_t len0, len1, blen = 0;
	struct sockaddr_nl nl;

	struct
	{
		struct nlmsghdr nh;
		union
		{
			struct ifinfomsg fi;
			struct ifaddrmsg fa;
		} u;
	} req;

	struct nlmsghdr *nh;
	struct ifinfomsg *fi;
	struct ifaddrmsg *fa;
	struct rtattr *rta;
	char s96[96];
	char *buff = NULL;

	fd = socket(AF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE);
	if (fd == -1)
	{
		ei->errCode = errno;
		snprintf(ei->errMsg, ei->errLen,
				"_at_%d: system call %s() failed with error %d (%s)",
				__LINE__, "socket", ei->errCode, strerror(ei->errCode));
		goto err;
	}

	memset(&nl, 0, sizeof(nl));
	nl.nl_family = AF_NETLINK;
	nl.nl_pid = 0; //getpid();
	rc = bind(fd, (SA *) &nl, sizeof(nl));
	if (rc == -1)
	{
		ei->errCode = errno;
		snprintf(ei->errMsg, ei->errLen,
				"_at_%d: system call %s() failed with error %d (%s)",
				__LINE__, "bind", ei->errCode, strerror(ei->errCode));
		goto err;
	}
	nl.nl_pid = 0; // dest

	memset(&req, 0, sizeof(req));
	req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	req.nh.nlmsg_type = RTM_GETLINK;
	req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.nh.nlmsg_pid = 0; //getpid();
	for (i = 0; i < 2; i++)
	{
		int multi = 0;
		req.nh.nlmsg_seq = seq++;
		req.u.fi.ifi_family = (i & 1) ? AF_INET6 : AF_INET;
		len0 = req.nh.nlmsg_len; //sizeof(req) ;
		//print_nlmsg(&req.nh,__LINE__);
		len1 = sendto(fd, &req, len0, 0, (SA *) &nl, sizeof(nl));
		if (len1 != len0)
		{
			ei->errCode = errno;
			snprintf(ei->errMsg, ei->errLen,
					"_at_%d: system call %s() failed with error %d (%s)",
					__LINE__, "send", ei->errCode, strerror(ei->errCode));
			goto err;
		}
		do
		{
			len1 = recv(fd, s96, 1, MSG_PEEK | MSG_TRUNC);
			if (len1 < 0)
			{
				ei->errCode = errno;
				snprintf(ei->errMsg, ei->errLen,
						"_at_%d: system call %s() failed with error %d (%s)",
						__LINE__, "recv(1)", ei->errCode, strerror(ei->errCode));
				goto err;
			}
			if (len1 > blen)
			{
				char *tmp;
				len0 = (len1 + 0xfff) & (~0xfff);
				if (!(tmp = (char*) realloc(buff, len0)))
				{
					ei->errCode = errno;
					snprintf(ei->errMsg, ei->errLen,
							"_at_%d: system call %s() failed with error %d (%s), req size: %lu",
							__LINE__, "realloc", ei->errCode, strerror(ei->errCode), len0);
					goto err;
				}
				buff = tmp;
				blen = len0;
			}
			len1 = recv(fd, buff, blen, 0);
			DBG_PRINT("_dbg_%d: recv len1=%lu, i=%d\n",__LINE__,len1,i);
			if (len1 < 0)
			{
				ei->errCode = errno;
				snprintf(ei->errMsg, ei->errLen,
						"_at_%d: system call %s() failed with error %d (%s)",
						__LINE__, "recv(2)", ei->errCode, strerror(ei->errCode));
				goto err;
			}
			nh = (struct nlmsghdr *) buff;
			if (nh->nlmsg_type == NLMSG_ERROR)
			{
				struct nlmsgerr *nlme;
				nlme = (nlmsgerr*) NLMSG_DATA(nh);
				ei->errCode = nlme->error;
				snprintf(ei->errMsg, ei->errLen, "_at_%d: Got error msg %d.",
						__LINE__, nlme->error);
				goto err;
			}

			len0 = len1;
			for (; NLMSG_OK(nh, len0); nh = NLMSG_NEXT(nh, len0))
			{
				int new_item = 0;
				multi = ((nh->nlmsg_flags & NLM_F_MULTI)
						&& nh->nlmsg_type != NLMSG_DONE);
				//print_nlmsg(nh,__LINE__);
				if ( NLMSG_PAYLOAD(nh,0) < sizeof(struct ifinfomsg))
					continue;
				fi = (struct ifinfomsg *) NLMSG_DATA(nh);
				for (ifi = ifh; ifi && ifi->index != fi->ifi_index;
						ifi = ifi->next)
					; // empty body
				if (!ifi)
				{
					if (!(ifi = (nicInfo*) malloc(sizeof(nicInfo))))
					{
						ei->errCode = errno;
						snprintf(ei->errMsg, ei->errLen,
								"_at_%d: system call %s() failed with error %d (%s)",
								__LINE__, "malloc", ei->errCode, strerror(ei->errCode));
						goto err;
					}
					new_item++;
					memset(ifi, 0, sizeof(nicInfo));
					if (ift)
						ift->next = ifi;
					else
						ifh = ifi;
					ift = ifi;
					ifi->index = fi->ifi_index;
					ifi->type = fi->ifi_type;
				}
				ifi->flags |= fi->ifi_flags;

				rta = IFLA_RTA(fi);
				len1 = IFLA_PAYLOAD(nh);
				for (; RTA_OK(rta, len1); rta = RTA_NEXT(rta, len1))
				{
					if (new_item)
					{
						if (rta->rta_type == IFLA_IFNAME)
						{
							mcc_strlcpy(ifi->name, (char *) RTA_DATA(rta),
									IF_NAMESIZE);
						}
						else if (rta->rta_type == IFLA_ADDRESS)
						{
							ifi->haddr->length = RTA_PAYLOAD(rta);
							ENSURE_LEN(ifi->haddr);
							memcpy(ifi->haddr->bytes, (char *) RTA_DATA(rta), ifi->haddr->length);
						}
						else if (rta->rta_type == IFLA_MTU)
						{
							ifi->mtu = *((int *) RTA_DATA(rta));
						}
					}
				} DBG_PRT(if ( new_item ) printf("=> new interface: %s , %d\n",ifi->name,ifi->index));
				if (nh->nlmsg_type == NLMSG_DONE)
					break;
			}
		} while (multi);
	}

	memset(&req, 0, sizeof(req));
	req.nh.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	req.nh.nlmsg_type = RTM_GETADDR;
	req.nh.nlmsg_flags = NLM_F_REQUEST | NLM_F_ROOT;
	req.nh.nlmsg_pid = 0; //getpid();

	for (i = 0; i < 2; i++)
	{
		int multi = 0;
		req.nh.nlmsg_seq = seq++;
		req.u.fa.ifa_family = (i & 1) ? AF_INET6 : AF_INET;
		len0 = req.nh.nlmsg_len; //sizeof(req) ;
		//print_nlmsg(&req.nh,__LINE__);
		len1 = sendto(fd, &req, len0, 0, (SA *) &nl, sizeof(nl));
		if (len1 != len0)
		{
			perror("send");
			ei->errCode = errno;
			snprintf(ei->errMsg, ei->errLen,
					"_at_%d: system call %s() failed with error %d (%s)",
					__LINE__, "send", ei->errCode, strerror(ei->errCode));
			goto err;
		}

		do
		{
			len1 = recv(fd, s96, 1, MSG_PEEK | MSG_TRUNC);
			if (len1 < 0)
			{
				ei->errCode = errno;
				snprintf(ei->errMsg, ei->errLen,
						"_at_%d: system call %s() failed with error %d (%s)",
						__LINE__, "recv", ei->errCode, strerror(ei->errCode));
				goto err;
			}
			if (len1 > blen)
			{
				char *tmp;
				len0 = (len1 + 0xfff) & (~0xfff);
				if (!(tmp = (char*) realloc(buff, len0)))
				{
					ei->errCode = ENOMEM;
					snprintf(ei->errMsg, ei->errLen,
							"_at_%d: realloc failed for %lu bytes",
							__LINE__, len0);
					goto err;
				}
				buff = tmp;
				blen = len0;
			}
			len1 = recv(fd, buff, blen, 0);
			DBG_PRINT("_dbg_%d: recv len1=%lu, i=%d\n",__LINE__,len1,i);
			if (len1 < 0)
			{
				ei->errCode = errno;
				snprintf(ei->errMsg, ei->errLen,
						"_at_%d: system call %s() failed with error %d (%s)",
						__LINE__, "recv", ei->errCode, strerror(ei->errCode));
				goto err;
			}
			nh = (struct nlmsghdr *) buff;
			if (nh->nlmsg_type == NLMSG_ERROR)
			{
				struct nlmsgerr *nlme;
				nlme = (nlmsgerr*) NLMSG_DATA(nh);
				ei->errCode = nlme->error;
				snprintf(ei->errMsg, ei->errLen, "_at_%d: Got error msg %d.",
						__LINE__, nlme->error);
				goto err;
			}

			len0 = len1;
			for (; NLMSG_OK(nh, len0); nh = NLMSG_NEXT(nh, len0))
			{
				ipFlat ip, iq;
				//print_nlmsg(nh,__LINE__);
				multi = ((nh->nlmsg_flags & NLM_F_MULTI)
						&& nh->nlmsg_type != NLMSG_DONE);
				if ( NLMSG_PAYLOAD(nh,0) < sizeof(struct ifaddrmsg))
					continue;
				fa = (struct ifaddrmsg *) NLMSG_DATA(nh);
				rta = (struct rtattr *) IFA_RTA(fa);

				DBG_PRINT("Interface: index= %d, family=%2.2x, pref=%2.2x, flags=%2.2x, scope=%2.2x\n",
						fa->ifa_index,fa->ifa_family,fa->ifa_prefixlen,fa->ifa_flags,fa->ifa_scope);
				for (ifi = ifh;
						ifi && ((unsigned int) ifi->index != fa->ifa_index);
						ifi = ifi->next)
					; // empty body
				memset(&ip, 0, sizeof(ip));
				memset(&iq, 0, sizeof(iq));
				ip.scope = fa->ifa_scope;

				len1 = IFA_PAYLOAD(nh);
				for (; RTA_OK(rta, len1); rta = RTA_NEXT(rta, len1))
				{
					//DBG_PRINT(" rta->rta_type= %d\n",rta->rta_type);
					if (rta->rta_type == IFA_ADDRESS)
					{
						iq.length = RTA_PAYLOAD(rta);
						ENSURE_LEN(&iq);
						memcpy(iq.bytes, RTA_DATA(rta), iq.length);
					}
					else if (rta->rta_type == IFA_LOCAL)
					{
						ip.length = RTA_PAYLOAD(rta);
						ENSURE_LEN(&ip);
						memcpy(ip.bytes, RTA_DATA(rta), ip.length);
					}
					else if (rta->rta_type == IFA_LABEL)
					{
						size_t mxl = RTA_PAYLOAD(rta) + 1;
						if (mxl > sizeof(ip.label))
							mxl = sizeof(ip.label);
						mcc_strlcpy(ip.label, (const char*) RTA_DATA(rta), mxl);
					}
				}
				if (!ip.length && iq.length)
				{
					memcpy(ip.bytes, iq.bytes, iq.length);
					ip.length = iq.length;
				}
				if (ip.length)
				{
					ipFlat *ipi, *ipp;
					if (ifi)
					{
						for (ipp = NULL, ipi = ifi->addrs; ipi; ipp = ipi, ipi =
								ipi->next)
							if (ipi->length == ip.length
									&& !memcmp(ipi->bytes, ip.bytes, ip.length))
								break;
						if (!ipi)
						{
							if (!(ipi = (ipFlat*) malloc(sizeof(ipFlat))))
							{
								ei->errCode = errno;
								snprintf(ei->errMsg, ei->errLen,
										"_at_%d: system call %s() failed with error %d (%s)",
										__LINE__, "malloc", ei->errCode, strerror(ei->errCode));
								goto err;
							}
							memcpy(ipi, &ip, sizeof(ip));
							ifi->naddr++;
							if (ipp)
								ipp->next = ipi;
							else
								ifi->addrs = ipi;
						}
					}
					else
					{
						std::cerr << "interface not found for address "
								<< ip2str(&ip, s96, 96) << std::endl;
					}
				}
			}
		} while (multi);
	}

	*niHead = ifh;
	ret = 0;
	err: if (fd)
		close(fd);
	if (buff)
		free(buff);
	if (ret)
	{
		ipFlat *ipi;
		while (ifh)
		{
			ifi = ifh;
			ifh = ifi->next;
			while (ifi->addrs)
			{
				ipi = ifi->addrs;
				ifi->addrs = ipi->next;
				free(ipi);
			}
			free(ifi);
		}
	}
	return ret;
}
/*-------------------------------------------------------------*/

static int buildIfList(errInfo *ei, nicInfo **niHead)
{
	int rc ;
	if ( (rc = buildIfList_nl(ei,niHead)) < 0 )
		rc = buildIfList_if(ei,niHead) ;
	return rc ;
}

/*-------------------------------------------------------------*/

std::string CommUtils::get_nic_addr(const char *nic)
{
	int any, isip;
	nicInfo *niHead = NULL, *ni;
	char addr[ADDR_STR_LEN];
	ipFlat *ip;
	ipFlat nip[1];
	errInfo ei[1];
	ei->errLen = sizeof(ei->errMsg);

	if (buildIfList(ei, &niHead) < 0)
		return std::string();

	if (nic && nic[0] && strcasecmp(nic, "any") && strcasecmp(nic, "all")
			&& strcasecmp(nic, "none"))
	{
		any = 0;
		if (inet_pton(AF_INET, nic, nip->bytes) == 1)
		{
			nip->length = sizeof(IA4);
			isip = 1;
		}
		else if (inet_pton(AF_INET6, nic, nip->bytes) == 1)
		{
			nip->length = sizeof(IA6);
			isip = 1;
		}
		else
			isip = 0;
	}
	else
	{
		any = 1;
		isip = 0;
	}

	addr[0] = 0;
	for (ni = niHead; ni; ni = ni->next)
	{
		if (any)
		{
			if (ni->flags & IFF_LOOPBACK)
				continue;
		}
		else if (isip)
		{
			for (ip = ni->addrs; ip; ip = ip->next)
			{
				if (ip->length != nip->length
						|| memcmp(ip->bytes, nip->bytes, ip->length))
					continue;
			}

			if (!ip)
				continue;

			if (ip->length == sizeof(IA4)
					&& inet_ntop(AF_INET, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
				break; //for ni

				if (ip->length == sizeof(IA6)
						&& inet_ntop(AF_INET6, ip->bytes, addr, ADDR_STR_LEN)
						== addr)
					break; //for ni
		}
		else
		{
			if (strncmp(nic, ni->name, IF_NAMESIZE))
				continue;
		}

		//On NIC name, search for an IPv4 or IPv6 address
		for (ip = ni->addrs; ip; ip = ip->next)
		{
			if (ip->length == sizeof(IA4)
					&& inet_ntop(AF_INET, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
				break;
		}

		if (addr[0])
			break;

		for (ip = ni->addrs; ip; ip = ip->next)
		{
			if (ip->length == sizeof(IA6)
					&& inet_ntop(AF_INET6, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
				break;
		}

		if (addr[0])
			break;
	}

	while (niHead)
	{
		ni = niHead;
		niHead = ni->next;
		while (ni->addrs)
		{
			ip = ni->addrs;
			ni->addrs = ip->next;
			free(ip);
		}
		free(ni);
	}

	if (addr[0])
		return std::string(addr);

	return std::string();
}

void CommUtils::get_all_nic_info(std::vector<CommUtils::NICInfo>& nic_vec)
{
	nicInfo *niHead = NULL, *ni;
	char addr[ADDR_STR_LEN];
	ipFlat *ip;
	errInfo ei[1];

	nic_vec.clear();
	if (buildIfList(ei, &niHead) < 0)
		return;

	for (ni = niHead; ni; ni = ni->next)
	{
		CommUtils::NICInfo nic_info;
		nic_info.name = std::string(ni->name);
		nic_info.index = ni->index;
		nic_info.multicast = (ni->flags & IFF_MULTICAST);
		nic_info.up = (ni->flags & IFF_UP);
		nic_info.loopback = (ni->flags & IFF_LOOPBACK);

		for (ip = ni->addrs; ip; ip = ip->next)
		{
			if (ip->length == sizeof(IA4)
					&& inet_ntop(AF_INET, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
			{
				nic_info.address_v4 = std::string(addr);
				break;
			}
		}

		for (ip = ni->addrs; ip; ip = ip->next)
		{
			if (ip->length == sizeof(IA6)
					&& inet_ntop(AF_INET6, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
			{
				nic_info.address_v6 = std::string(addr);
				break;
			}
		}

		nic_vec.push_back(nic_info);
	}

	while (niHead)
	{
		ni = niHead;
		niHead = ni->next;
		while (ni->addrs)
		{
			ip = ni->addrs;
			ni->addrs = ip->next;
			free(ip);
		}
		free(ni);
	}
}

bool CommUtils::get_nic_info(const char *nic, NICInfo* pNICInfo, int *errCode, std::string *errMsg)
{
	CommUtils::NICInfo nic_info;

	int isip = 0;

	nicInfo *niHead = NULL, *ni;
	char addr[ADDR_STR_LEN];
	ipFlat *ip;
	ipFlat nip[1];
	errInfo ei[1];
	ei->errLen = sizeof(ei->errMsg);

	if (buildIfList(ei, &niHead) < 0)
	{
		*errCode = ei->errCode;
		errMsg->assign(ei->errMsg);
		return false;
	}

	if (nic && nic[0])
	{
		if (inet_pton(AF_INET, nic, nip->bytes) == 1)
		{
			nip->length = sizeof(IA4);
			isip = 1;
		}
		else if (inet_pton(AF_INET6, nic, nip->bytes) == 1)
		{
			nip->length = sizeof(IA6);
			isip = 1;
		}
		else
			isip = 0;
	}
	else
	{
		*errCode = EINVAL;
		errMsg->assign("NULL argument");
		return false;
	}

	addr[0] = 0;
	for (ni = niHead; ni; ni = ni->next)
	{
		if (isip)
		{
			for (ip = ni->addrs; ip; ip = ip->next)
			{
				if (ip->length == nip->length
						&& memcmp(ip->bytes, nip->bytes, ip->length) == 0)
				{
					break;
				}
			}

			if (!ip)
				continue; //Skip the rest if we reached the end of the list
		}
		else
		{
			if (strncmp(nic, ni->name, IF_NAMESIZE))
				continue; //Skip the rest if not equal
		}

		//On a NIC that contains the same IP address, or
		//On NIC with equal name,
		// => search for an IPv4 and IPv6 address, and all the info
		nic_info.name = std::string(ni->name);
		nic_info.index = ni->index;
		nic_info.multicast = (ni->flags & IFF_MULTICAST);
		nic_info.up = (ni->flags & IFF_UP);
		nic_info.loopback = (ni->flags & IFF_LOOPBACK);

		for (ip = ni->addrs; ip; ip = ip->next)
		{
			if (ip->length == sizeof(IA4)
					&& inet_ntop(AF_INET, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
			{
				nic_info.address_v4 = std::string(addr);
				break;
			}
		}

		for (ip = ni->addrs; ip; ip = ip->next)
		{
			if (ip->length == sizeof(IA6)
					&& inet_ntop(AF_INET6, ip->bytes, addr, ADDR_STR_LEN)
					== addr)
			{
				nic_info.address_v6 = std::string(addr);
				break;
			}
		}

		if (nic_info.index > 0)
			break;
	}

	while (niHead)
	{
		ni = niHead;
		niHead = ni->next;
		while (ni->addrs)
		{
			ip = ni->addrs;
			ni->addrs = ip->next;
			free(ip);
		}
		free(ni);
	}

	*pNICInfo = nic_info;
	return true;
}

CommUtils::NICInfo CommUtils::get_nic_up_mc_v6()
{
	spdr::CommUtils::NICInfo nic_info;
	std::vector<spdr::CommUtils::NICInfo> nic_vec;
	spdr::CommUtils::get_all_nic_info(nic_vec);
	std::vector<spdr::CommUtils::NICInfo>::iterator nic_it = nic_vec.begin();
	while (nic_it != nic_vec.end())
	{
		if (nic_it->up && nic_it->multicast && !nic_it->address_v6.empty())
		{
			nic_info = *nic_it;
			break;
		}
		nic_it++;
	}

	return nic_info;
}

int set_CLOEXEC(int fd)
{
#ifdef SPDR_FD_CLOEXEC
	int fl = fcntl(fd, F_GETFD, 0);
	if ( fl < 0 )
		return fl;
	else
		return fcntl(fd,F_SETFD, (fl|FD_CLOEXEC));
#else
	return 0;
#endif
}
} /* namespace spdr */
