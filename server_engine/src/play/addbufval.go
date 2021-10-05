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
 
//This program was used to add an extra argument to to a function


//TODO:
// * Don't recompile the regexp for every line!!!

//    gccgo -g src/addbufval.go -o addbufval
//(or with non-default gcc e.g:
// /opt/gcc4.9/bin/gccgo -g addbufval.go -o addbufval -Wl,-rpath,/opt/gcc4.9/lib64

package main

import "fmt" // Package implementing formatted I/O.
import "os"
import "io"
import "bufio"
import "regexp"

const outdir = "/tmp/parsed"

func parseSourceLine(writer *bufio.Writer, sourceLine string) {
    //First try and find matching function calls
    linematcher, err := regexp.Compile("(.*)ieutTRACEL(.*)pThreadData,(.*)__func__,([^,)]+)(.+)")
    if err != nil { panic(err) }
    
    
    matches := linematcher.FindStringSubmatch(sourceLine)
    
    var outLine string
    
    if len(matches) == 6 {
        fmt.Printf("len(matches) %v   %v  %v\n", len(matches), matches[4], sourceLine)
        outLine = fmt.Sprintf("%vieutTRACEL_BV%vpThreadData,%v, %v__func__,%v%v\n", 
                              matches[1], matches[2], matches[4], matches[3],matches[4],matches[5])
    } else {
         outLine = sourceLine
    }
    
    _, err = writer.WriteString(outLine)
    if err != nil { panic(err) }
}

func parseSourceFile(fileName string) bool {
    //Open the input filename
    inf, err := os.OpenFile(fileName, os.O_RDONLY, 0)
    if err != nil {
        fmt.Println("Error opening infile:", err)
        return false
    }
    defer inf.Close()

    outFileName := outdir+"/"+fileName
    
    outf, err := os.OpenFile(outFileName, os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0644)
    if err != nil {
        fmt.Println("Error opening outfile:", err)
        return false
    }
    defer outf.Close()
      
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
        
        parseSourceLine(writer, line)
    }
 
    return true
}
    

func main() {
    if len(os.Args) < 2 {
        fmt.Printf("addbufval <source files>\n")
    } else {
        for i := 1; i < len(os.Args); i++ {
            fmt.Printf("Parsing %s...\n",os.Args[i]);
            success := parseSourceFile(os.Args[i])
            
            if success {
                fmt.Printf("Parsed %s successfully\n", os.Args[i] );
            } else {
                //errors will have been printed
                fmt.Printf("Parsing %s failed\n", os.Args[i] );
            }
        }
    }
}
