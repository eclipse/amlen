package com.ibm.mqst.mqxr.scada;
import java.io.*;
import java.text.*;
import java.util.*;

/*
 * Copyright (c) 2002-2021 Contributors to the Eclipse Foundation
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
/**
 * @version 1.0
 */

/*
 * This class represents a logfile used to record results and message
 * information during testing
 */

public class LogFile
{

    String name;

    File log;

    PrintWriter out;

    // Constructor - attempts to open/create file named in parameter
    public LogFile(String logFile) throws IOException
    {

        log = new File(logFile);
        name = logFile;
        out = new PrintWriter(new FileWriter(log));
    }

    // Prints a string/object to the file with a timestamp and newline
    public void println(Object data)
    {
        Date today;
        String dateOut;
        Locale currentLocale = Locale.UK;
        SimpleDateFormat timeFormatter = new SimpleDateFormat("dd/MM/yy HH:mm:ss:SSS", currentLocale);

        today = new Date();
        dateOut = timeFormatter.format(today);

        out.println(dateOut + " " + data);
        out.flush();
    }

    // Prints a string to the file no newline
    public void print(Object data)
    {

        out.print(data);
        out.flush();
    }

    // Prints an integer to the file with no newline
    public void print(int data)
    {
        out.print(data);
        out.flush();
    }

    // Prints a newline to the file
    public void newLine()
    {
        out.println();
        out.flush();
    }

    // Prints a String encompassed by a border
    public void printTitle(String s)
    {
        out.println("*******************************************************************************");
        out.println("**	" + s);
        out.println("*******************************************************************************");
        out.flush();
    }

}
