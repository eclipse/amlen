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
package svt.home;

import java.util.Vector;

import svt.framework.scale.SVT_Engine;
import svt.framework.scale.transports.SVT_Transport_PahoThread;
import svt.framework.scale.transports.SVT_Transport_Thread;
import svt.home.device.SVTHomeControl;
import svt.home.device.SVTHomeMainOffice;
import svt.home.device.SVTHomeMobile;
import svt.home.device.SVTHomeSensor;
import svt.util.SVTConfig;
import svt.util.SVTConfig.BooleanEnum;
import svt.util.SVTConfig.IntegerEnum;
import svt.util.SVTConfig.StringEnum;
import svt.util.SVTLog;
import svt.util.SVTStats;
import svt.util.SVTUtil;
import svt.util.SVT_Params;

public class SVTHome_Engine extends SVT_Engine {
	public static void main(String[] args) {

		if (args.length == 0) {
			SVTLog.log3(TYPE, "No arguments supplied.");

			System.exit(0);
		} else {
			SVTConfig.parse(args);
		}
		SVTConfig.printSettings();

		if (SVTConfig.help() == false) {
			SVTHome_Engine stress = new SVTHome_Engine();
			stress.begin();

		}

		System.exit(0);
	}

	public void runMobile(SVT_Transport_Thread thisThread, int clientNum) {
		SVTHomeMobile thisMobile = new SVTHomeMobile(0L, clientNum,
				HomeConstants.MobileDeviceType);
		thisThread.addDevice(thisMobile);
		thisMobile.registerDevice();

	}

	public void runControl(SVT_Transport_Thread thisThread, int clientNum) {
		SVTHomeControl thisControl = new SVTHomeControl(0L, clientNum,
				HomeConstants.ControlDeviceType);
		thisThread.addDevice(thisControl);
		thisControl.registerDevice();

	}

	public void runMainOffice(SVT_Transport_Thread thisThread, int clientNum) {
		SVTHomeMainOffice thisMainOffice = new SVTHomeMainOffice(0L, clientNum,
				HomeConstants.OfficeDeviceType);
		thisThread.addDevice(thisMainOffice);
		thisMainOffice.registerDevice();

	}

	public void runSensor(SVT_Transport_Thread thisThread, int clientNum) {
		SVTHomeSensor thisSensor = new SVTHomeSensor(0L, clientNum,
				HomeConstants.SensorDeviceType);
		thisThread.addDevice(thisSensor);
		thisSensor.registerDevice();
	}

	public void begin() {

		list = new Vector<Thread>();
		statslist = new Vector<SVTStats>();
		SVTStats stats = null;
		String userPrefix = SVTUtil.getPrefix(StringEnum.userName.value);
		long userIndex = SVTUtil.getIndex(StringEnum.userName.value);
		int userIndexDigits = SVTUtil.getDigits(StringEnum.userName.value);
		String name = null;
		SVT_Params parms;
		parms = SVTConfig.getParams();

		int mobileClientNumLimit = 5;
		while (p < IntegerEnum.paho.value) {
			stats = new SVTStats(1000 * 60 / IntegerEnum.rate.value, 1);
			statslist.add(stats);
			parms.stats = stats;
			name = SVTUtil.formatName(userPrefix, userIndex, userIndexDigits);
			parms.userName="h"+p;
			SVT_Transport_PahoThread pt = new SVT_Transport_PahoThread(parms);
			switch (parms.clientType) {
			case HomeConstants.OfficeClientType:
				runMainOffice(pt, p);
				break;
			case HomeConstants.SensorClientType:
				runSensor(pt, p);
				if (mobileClientNumLimit >= 5) {
					p++;
					mobileClientNumLimit = 0;
				} else {
					mobileClientNumLimit++;
				}
				break;
			case HomeConstants.ControlClientType:
				runControl(pt, p);
				if (mobileClientNumLimit >= 5) {
					p++;
					mobileClientNumLimit = 0;
				} else {
					mobileClientNumLimit++;
				}
				break;
			case HomeConstants.MobileClientType:
				runMobile(pt, p);
				p++;
				break;
			}
			Thread t = new Thread(pt);
			t.setName(name);
			list.add(t);
			userIndex++;
			p++;
		}

		long starttime = System.currentTimeMillis();
		try {
			for (Thread t : list) {
				SVTUtil.threadcount(1);
				t.start();
				Thread.sleep(10L);
			}

			long lasttime = System.currentTimeMillis();

			while (activethreads(list)) {
				for (Thread t : list) {
					while (t.isAlive()) {
						t.join(60000);
						lasttime = printStats(list, lasttime,
								StringEnum.server.value,
								StringEnum.server2.value,
								IntegerEnum.port.value, hostname,
								BooleanEnum.stats.value, BooleanEnum.ssl.value,
								t.getName(), StringEnum.password.value);
					}
				}
			}
		} catch (Throwable e) {
			e.printStackTrace();
			System.exit(1);
		}

		long count = printFinalStats(starttime);
		if ((count > 0) && (SVTUtil.stop(null) == false)) {
			SVTLog.log3(TYPE, "SVTHomeEngine Success!!!");
		} else {
			SVTLog.log3(TYPE, "SVTHomeEngine Failed!!!");
		}
	}
}
