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

import java.sql.Timestamp;

import svt.util.SVTConfig.BooleanEnum;

public class SVTLog {
   long t = 0;

    public static synchronized void logtry(TTYPE type, String text) {
        if (BooleanEnum.vverbose.value)
            log(STATE.TRY, type, text, 0, null);
    }

    public static synchronized void logtry(TTYPE type, String text, long n) {
        if (BooleanEnum.vverbose.value)
            log(STATE.TRY, type, text, n, null);
    }

    public static synchronized void logretry(TTYPE type, String text, long n) {
        if (BooleanEnum.vverbose.value)
            log(STATE.RETRY, type, text, n, null);
    }

    public static synchronized void logretry(TTYPE type, String text, Throwable e, long n) {
        if (BooleanEnum.vverbose.value) {
            String output = text + " (" + e.getMessage();
            if (e.getCause() != null) {
                output += " caused by " + e.getCause().getMessage();
            }
            output += ")";
            log(STATE.RETRY, type, output, n, null);
        }
    }

    public static synchronized void logdone(TTYPE type, String text) {
        if (BooleanEnum.vverbose.value)
            log(STATE.DONE, type, text, 0, null);
    }

    public static synchronized void logdone(TTYPE type, String text, String text2) {
        if (BooleanEnum.vverbose.value)
            log(STATE.DONE2, type, text2, 0, null);
    }

    public static synchronized void logdone(TTYPE type, String text, long n) {
        if (BooleanEnum.vverbose.value)
            log(STATE.DONE, type, text, n, null);
    }

    public static synchronized void log(TTYPE type, String text) {
        if (BooleanEnum.vverbose.value)
            log(STATE.MSG, type, text, 0, null);
    }

    public static synchronized void lognr(TTYPE type, String text) {
        if (BooleanEnum.vverbose.value)
            log(STATE.NRET, type, text, 0, null);
    }

    public static synchronized void logv(TTYPE type, String text) {
        if (BooleanEnum.verbose.value)
            log(STATE.MSG, type, text, 0, null);
    }

    public static synchronized void log3(TTYPE type, String text) {
        log(STATE.MSG, type, text, 0, null);
    }

    public static synchronized void logex2(TTYPE type, String text, Throwable e) {
       String output = text + " exception (" + e.getMessage();
       if (e.getCause() != null) {
           output += " caused by " + e.getCause().getMessage();
       }
       output += ")";
       log(STATE.EX, type, output, 0, e);
    }
    
    public static synchronized void logex(TTYPE type, String text, Throwable e) {
        String output = text + " failed with ex (" + e.getMessage();
        if (e.getCause() != null) {
            output += " caused by " + e.getCause().getMessage();
        }
        output += ")";
        log(STATE.EX, type, output, 0, e);
    }

    private static synchronized void log(STATE state, TTYPE type, String text, long n, Throwable e) {
        long tid = Thread.currentThread().getId();
        java.util.Date date = new java.util.Date();
        String time = new Timestamp(date.getTime()).toString();

        if (state == STATE.MSG) {
            System.out.format("\n%-24s %06d %-7s %6d: " + text + "\n", time, SVTUtil.threadcount(0), type, tid);

        } else if (state == STATE.NRET) {
            System.out.format("\n%-24s %06d %-7s %6d: " + text, time, SVTUtil.threadcount(0), type, tid);

        } else if (state == STATE.EX) {
            System.out.format("\n%-24s %06d %-7s %6d: " + text + "\n", time, SVTUtil.threadcount(0), type, tid);
            e.printStackTrace(System.err);

        } else {
            if (state == STATE.TRY) {
                if (n == 0) {
                    System.out.print(", try_" + text);
                } else {
                    System.out.print(", try_" + text + "#" + n);
                }
            } else if (state == STATE.RETRY) {
                System.out.print("..retry#" + n);
            } else if (state == STATE.DONE) {
                System.out.print("..complete");
            } else if (state == STATE.DONE2) {
                System.out.println("..complete" + text);
            }
        }
    }

    public enum STATE {
        TRY, RETRY, DONE, MSG, NRET, EX, DONE2
    }

    public enum TTYPE {
        SCALE, MQTT, PAHO, ISMJMS, UTIL, CONFIG;

        @Override
        public String toString() {
            String s = super.toString();
            return s.toLowerCase();
        }
    }

}
