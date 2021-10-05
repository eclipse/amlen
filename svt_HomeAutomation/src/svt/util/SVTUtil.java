/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package svt.util;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.lang.management.ManagementFactory;
import java.net.InetAddress;
import java.util.Random;

import svt.util.SVTLog.TTYPE;

public class SVTUtil {

	static public String getPrefix(String userName) {
		String prefix = "c";
		if (userName != null) {
			String name = new String(userName);
			int i = name.length() - 1;
			while (i > 0 && Character.isDigit(name.charAt(i)))
				i--;
			prefix = name.substring(0, i + 1);
		}
		return prefix;
	}

	static public int getDigits(String userName) {
		int digits = 0;
		String suffix = null;
		String prefix = null;

		if (userName != null) {
			prefix = getPrefix(userName);
			if ((prefix != null) && (prefix.length() > 0)) {
				suffix = userName.substring(prefix.length());
				if (suffix != null) {
					digits = suffix.length();
				}
			}
		}
		return digits;
	}

	static public long getIndex(String userName) {
		long index = 0;
		String suffix = null;
		String prefix = null;
		if (userName != null) {
			prefix = getPrefix(userName);
			if ((prefix != null) && (prefix.length() > 0)) {
				suffix = userName.substring(prefix.length());
				if ((suffix != null) && (suffix.length() > 0)) {
					index = Long.valueOf(suffix);
				}
			}
		}
		return index;
	}

	static public String formatName(long index) {
		return formatName("c", index, 0);
	}

	static public String formatName(String prefix) {
		return formatName(prefix, 0, 0);
	}

	static public String formatName(String prefix, long index) {
		return formatName(prefix, index, 0);
	}

	static public String formatName(String prefix, long index, int digits) {

		if (digits > 0)
			return String.format("%s%0" + digits + "d", prefix, index);
		else
			return prefix;
	}

	static public String getHostname() {
		String hostname = null;

		try {
			hostname = InetAddress.getLocalHost().getHostName();
		} catch (Throwable e) {
			hostname = null;
		}

		if (hostname == null) {
			try {
				Process p = Runtime.getRuntime().exec("hostname");
				BufferedReader br = new BufferedReader(new InputStreamReader(
						p.getInputStream()));
				p.waitFor();

				while (br.ready()) {
					hostname = br.readLine();
				}
			} catch (Throwable e3) {
				hostname = null;
			}
		}

		if (hostname == null) {
			try {
				hostname = InetAddress.getByName("127.0.0.1").getHostName();
			} catch (Throwable e2) {
				hostname = "unknown";
			}
		}
		return hostname;
	}

	static public long MyTid() {
		return Thread.currentThread().getId();
	}

	static public String MyPid() {
		String nameOfRunningVM = ManagementFactory.getRuntimeMXBean().getName();
		int p = nameOfRunningVM.indexOf('@');
		String pid = nameOfRunningVM.substring(0, p);
		return pid;
	}

	static public String MyTopicId(String appId, String clientId) {
		return "/APP/" + appId + "/CAR/" + clientId;
	}

	static public String MyClientId(TTYPE type, String prefix) {
		String pid = MyPid();
		long tid = MyTid();
		int random = new Random().nextInt(999999);

		String fullhostname = SVTUtil.getHostname();
		int dotLoc = 0;
		String pClientId = prefix + fullhostname + "_" + pid + "_" + tid;
		if (pClientId.length() > 23) {
			dotLoc = fullhostname.indexOf(".");
			String host = fullhostname;
			if (dotLoc > 0) {
				host = fullhostname.substring(0,
						fullhostname.indexOf("."));
			}
			pClientId = prefix + host + "_" + pid + "_" + tid;
			if (pClientId.length() > 23) {
				pClientId = prefix + host + pid + "_" + tid;
				if (pClientId.length() > 23) {
					pClientId = prefix + random + pid + "_" + tid;
					if (pClientId.length() > 23) {
						pClientId = prefix + random;
						if (pClientId.length() > 23) {
							SVTLog.log(type, "clientId is too long");
							pClientId = (prefix + random).substring(0, 22);
						}
					}
				}
			}
		}

		System.out.println("clientId is " + pClientId);
		SVTLog.log(type, "clientId is " + pClientId);
		return pClientId;
	}

	static public String MyClientId(TTYPE type) {
		return MyClientId(type, "");
	}

	static public String MyClientId() {
		return MyClientId(TTYPE.UTIL, "");
	}

	static boolean stop = false;

	static public synchronized boolean stop(Boolean s) {
		boolean so = stop;
		if (s != null) {
			stop = s;
		}
		return so;
	}

	// static int count = 0;
	//
	// static public synchronized int count(int i) {
	// return count += i;
	// }

	static int threadcount = 0;

	public static synchronized int threadcount(int i) {
		return threadcount += i;
	}

}
