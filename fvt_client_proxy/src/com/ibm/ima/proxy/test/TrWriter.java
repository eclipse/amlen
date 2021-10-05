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
/*-----------------------------------------------------------------
 * System Name : MQ Low Latency Messaging
 * File        : TrWriter.java
 * Author      : 
 *-----------------------------------------------------------------
 */

package com.ibm.ima.proxy.test;

import java.io.FileNotFoundException;
import java.io.PrintStream;
import java.io.UnsupportedEncodingException;
import java.text.MessageFormat;
import java.text.SimpleDateFormat;
import java.util.Date;
import java.util.HashMap;
import java.util.TimeZone;

/**
 * This is an utility class responsible for logging.
 * <p>
 * 
 */

public final class TrWriter {
    private static HashMap<String, TrWriter> trWriters = new HashMap<String, TrWriter>();
    private static final SimpleDateFormat DATE_FORMAT = new SimpleDateFormat(
            "[dd/MM/yy HH:mm:ss:SSS z]");
    static {
        DATE_FORMAT.setTimeZone(TimeZone.getDefault());
    }
    // Just for optimization
    private static ThreadLocal<String> _threadID = new ThreadLocal<String>() {
        protected synchronized String initialValue() {
            Long tid = new Long(Thread.currentThread().getId());
            return String.format("%06x", new Object[] { tid });
        }
    };

    final PrintStream _writer;
    LogLevel _allowedLogLevel;
    private boolean _closed;
    private final StringBuffer _sb = new StringBuffer(10240);
    private final Date _date = new Date();
    String _dateFormat;

    TrWriter(String fileName) {
        this(fileName, LogLevel.LOGLEV_INFO);
    }

    public TrWriter(String fileName, LogLevel allowedLogLevel) {
        synchronized (this) {
            if (trWriters.isEmpty()) {
                trWriters.put("DEFAULT", this);
            }
            try {
                if (fileName == null) {
                    _writer = new PrintStream(System.out, true, "UTF-8");
                    trWriters.put("stdout", this);
                } else {
                    _writer = new PrintStream(fileName, "UTF-8");
                    trWriters.put(fileName, this);
                }
            } catch (FileNotFoundException e) {
                throw new RuntimeException(e);
            } catch (UnsupportedEncodingException e) {
                throw new RuntimeException(e);
            }
            _allowedLogLevel = allowedLogLevel;
            _closed = false;
            _dateFormat = DATE_FORMAT.format(_date);
        }

    }
    
    public static TrWriter get(String fileName) {
        return trWriters.get(fileName);
    }

    public synchronized void writeTraceLine(LogLevel logLevel, String msgKey,
            String msg, Object... params) {
        if (_closed) {
            return;
        }
        if (logLevel.compareTo(_allowedLogLevel) > 0)
            return;
        String tid = _threadID.get();
        long time = System.currentTimeMillis();
        String trLine = (params.length > 0) ? MessageFormat.format(msg, params)
                : msg;
        writeTraceLine(time, tid, logLevel, msgKey, null,trLine);
    }

    public synchronized void writeTraceLine(long timestamp, long tid,
            LogLevel logLevel, String msgKey, String instance, String msg) {
        if (_closed) {
            return;
        }
        String tidStr = String.format("%06x", tid);
        writeTraceLine(timestamp, tidStr, logLevel, msgKey, instance, msg);
    }

    private final synchronized void writeTraceLine(long timestamp, String tid,
            LogLevel logLevel, String msgKey, String instance, String trLine) {
        if (_date.getTime() != timestamp) {
            _date.setTime(timestamp);
            _dateFormat = DATE_FORMAT.format(_date);
        }
        _sb.append(_dateFormat).append(' ').append(tid).append(' ')
                .append(logLevel.toString()).append(' ').append(msgKey)
                .append(": ");
        if(instance != null){
            _sb.append(instance).append(": ");
        }
        _sb.append(trLine);
        _writer.println(_sb);
        _writer.flush();
        _sb.delete(0, _sb.length());
    }

    public synchronized void writeException(Throwable t) {
        _writer.println(t.getMessage());
        t.printStackTrace(_writer);
    }

    synchronized void setAllowedLogLevel(LogLevel logLevel) {
        _allowedLogLevel = logLevel;
    }

    synchronized void close() {
        _writer.flush();
        _writer.close();
        _closed = true;
    }

    public enum LogLevel {
        LOGLEV_STATUS {
            public String toString() {
                return "S";
            }
        }, /* S = Status */
        LOGLEV_FATAL {
            public String toString() {
                return "F";
            }
        }, /* F = Fatal */
        LOGLEV_ERROR {
            public String toString() {
                return "E";
            }
        }, /* E = Error */
        LOGLEV_WARN {
            public String toString() {
                return "W";
            }
        }, /* W = Warning */
        LOGLEV_INFO {
            public String toString() {
                return "I";
            }
        }, /* I = Informational */
        LOGLEV_EVENT {
            public String toString() {
                return "V";
            }
        }, /* V = eVent */
        LOGLEV_DEBUG {
            public String toString() {
                return "D";
            }
        }, /* D = Debug */
        LOGLEV_TRACE {
            public String toString() {
                return "T";
            }
        }, /* T = Trace */
        LOGLEV_XTRACE {
            public String toString() {
                return "X";
            }
        }; /* X = eXtended trace */
        public static LogLevel valueOf(int i) {
            switch (i) {
            case 1:
                return LOGLEV_STATUS;
            case 2:
                return LOGLEV_FATAL;
            case 3:
                return LOGLEV_ERROR;
            case 4:
                return LOGLEV_WARN;
            case 5:
                return LOGLEV_INFO;
            case 6:
                return LOGLEV_EVENT;
            case 7:
                return LOGLEV_DEBUG;
            case 8:
                return LOGLEV_TRACE;
            case 9:
                return LOGLEV_XTRACE;
            }
            throw new RuntimeException("Inlvalid LogLevel value: " + i);
        }

    }
}
