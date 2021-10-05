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
 
//This simple trace formatter is maintained by Jon outside work hours.
//It's written in Go as a learning experience so the reader is warned it may not be idiomatic Go code
//
//To compile with gcc (requires gcc 4.7 or above):
//gccgo trcfmt.go -o trcfmt
//Alternatively (if you're using an older gcc) this code can be compile with gc, the standard Go compiler
//(get it from golang.org):
//go build trcfmt.go
//
// Functionality:
// At the moment INCREDIBILY basic, just indents trace lines to indicate depth in callstack


//TODO:
// * Don't recompile the regexp for every line!!!
// * Understand other variants of trace (e.g. without date)

package main

import "fmt" // Package implementing formatted I/O.
import "os"
import "io"
import "bufio"
import "regexp"
import "strings"

//This encodes as a regular expression the date format that ISM uses in its trace
const ISMTRACE_DATEFMT_REGEX = "\\d\\d\\d\\d-\\d\\d-\\d\\d \\d\\d:\\d\\d:\\d\\d\\.\\d\\d\\d(?:Z|(?:\\+\\d\\d:\\d\\d))"

//How many spaces to indent a line because of a function call
const TRACE_INDENT=3

func itfFormatTraceLine(threadIndents map[string] uint32, date string, threadName string, traceStmt string) string {
    
    var lineIndent uint32 = 0
    var prefix string = ""
    
    if threadName != "" {
        _, inMap := threadIndents[threadName]
        
        if inMap {
            //We know about this thread, use the existing indent
            lineIndent = threadIndents[threadName]
        } else {
            //New thread, start indent at 0
            threadIndents[threadName] = lineIndent
        }
    }
    
    if strings.HasPrefix(traceStmt, ">>>") { 
        //Indent the next line, doesn't affect this one
        threadIndents[threadName] = lineIndent + TRACE_INDENT
        prefix = "{"
    } else if strings.HasPrefix(traceStmt,"<<<") {
        if(lineIndent < TRACE_INDENT) {
            fmt.Printf("Unmatched function exit : %s %s: %s\n", date, threadName, traceStmt)
        } else {
            //This line represent the return from the function and needs to be outdented...
            lineIndent -= TRACE_INDENT
            threadIndents[threadName] = lineIndent
            prefix = "}"
        }
    }
    
    return date+" "+threadName+": "+strings.Repeat(" ", int(lineIndent))+prefix+traceStmt+"\n"
}

func itfParseISMTraceLine(writer *bufio.Writer, traceLine string, threadIndents map[string] uint32) {
    //First try and find date and threadname... first try assuming has date and thread
    datethreadmatcher, err := regexp.Compile("("+ISMTRACE_DATEFMT_REGEX+") ([^:]*): (.*)")
    if err != nil { panic(err) }
    
    
    matches := datethreadmatcher. FindStringSubmatch(traceLine)
    
    var outLine string
    
    if len(matches) == 4 {
        fmt.Printf("len(matches) %v     %v\n", len(matches), traceLine)
        outLine = itfFormatTraceLine(threadIndents, matches[1],matches[2],matches[3])
    } else {
        fmt.Printf("len(matches) %v     %v\n", len(matches), traceLine)
        outLine = traceLine
    }
    
    _, err = writer.WriteString(outLine)
    if err != nil { panic(err) }
}

func itfParseISMTraceFile(fileName string) bool {
    //Open the input filename
    inf, err := os.OpenFile(fileName, os.O_RDONLY, 0)
    if err != nil {
        fmt.Println("Error opening infile:", err)
        return false
    }
    defer inf.Close()

    outFileName := fileName + ".fmt"
    
    outf, err := os.OpenFile(outFileName, os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0644)
    if err != nil {
        fmt.Println("Error opening outfile:", err)
        return false
    }
    defer outf.Close()
    
    //Create a hash of threadname to current indent levels
    threadIndents := make(map[string] uint32)
    
    //Read in the file line by line processing it and writing out the parsed file 
    reader := bufio.NewReader(inf)
    writer := bufio.NewWriter(outf)
    
    defer writer.Flush()
    
    for {
        line, err := reader.ReadString('\n')
        
        if err == io.EOF {
            break
        }
        if err != nil { panic(err)}
        
        itfParseISMTraceLine(writer, line, threadIndents) 
    }

    
    for threadName, finalIndent := range threadIndents {
        if finalIndent == 0 {
            fmt.Printf("Thread %s processed\n", threadName)
        } else {
            fmt.Printf("ERROR: Thread %s processed but had final indent of %v. Mismatched function entry/exit\n", threadName, finalIndent)
        }
    }
 
    return true
}
    

func main() {
    if len(os.Args) < 2 {
        fmt.Printf("trcfmt <unformatted trace files>\n")
    } else {
        for i := 1; i < len(os.Args); i++ {
            fmt.Printf("Parsing %s...\n",os.Args[i]);
            success := itfParseISMTraceFile(os.Args[i])
            
            if success {
                fmt.Printf("Parsed %s successfully\n", os.Args[i] );
            } else {
                //errors will have been printed
                fmt.Printf("Parsing %s failed\n", os.Args[i] );
            }
        }
    }
}
