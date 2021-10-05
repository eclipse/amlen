/*
 * Copyright (c) 2014-2021 Contributors to the Eclipse Foundation
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

// build: 
//
//     gccgo -g trchistfmt.go -o trchistfmt
// 
// This program is used to format trace history buffers dumped using the
// dumptracehistory.py script in gdb on a core file (or attached to a running
// process):
// 
//     ./trchistfmt -t /tmp/dumpedhistory -s server_engine/src
//
// Alternatively, golang can be used to interpret trchistfmt.go:
//
//     go run ./trchistfmt.go -t /tmp/dumpedhistory -s server_engine/src
//
// If the -t flag is not specified, the program reads from STDIN.
//
// Multiple -s flags can be supplied giving the paths to source directory (from which c
// source and header files are parsed).
//
// Trace dumps written by ietd_dumpHistories can be processed too, they are encrypted so
// showCrypto needs to be used with the 'trcDump' display type... putting it all together:
//
//     showCrypto <ENCRYPTED_FILE> <PASSWORD> trcDump | trchistfmt -s server_engine/src
//
// Will write Trace_activeX.fmt files of the active threads in the dump. 

package main

import "fmt" // Package implementing formatted I/O.
import "os"
import "io"
import "bytes"
import "bufio"
import "encoding/binary"
import "crypto/md5"
import "path/filepath"
import "strings"
import "log"
import "regexp"
import "strconv"
import "flag"
import "math"

//Set up some trace point types... entry exits etc...
type TracePointType int

const (
        TracePointEntry TracePointType = iota
        TracePointExit
        TracePointIdent
        TracePointHistoryBuffer
        TracePointOther
)

type TracePointData struct {
   File      string
   Line      uint32
   Function  string
   Value     string
   Type      TracePointType
}

type TraceContextData struct {
    BaseTime     uint64
    LowCycles    uint64
    FileIdents   map[uint32] string
    TracePoints  map[uint64] *TracePointData
    ColumnWidths map[string] uint32
}

type SourceParseContext struct {
    CurrentFunction string
    CurlyDepth    int32
}

type TraceParseContext struct {
    InFunc            string      //Name of Function we are currently in
    FuncEntry         bool        //At the entrypoint to a function
    FuncEntryCycles   uint64
    FuncExitCycles    uint64
    CyclesToMillis    float64
    CyclesToNanos     float64
    VolatileTrace     string
}

var cpuGHz=2.9


func getFmtTraceFileName(outDir       string,
                         filePrefix   string,
                         threadStatus string, //active or saved or the empty string
                         threadNum    uint64) string {

    outFileName := ""

    if (threadStatus == "") {
        outFileName = outDir + filePrefix + strconv.FormatUint(threadNum, 10)+".fmt"
    } else {
        outFileName = outDir + filePrefix + "_" + strings.Trim(threadStatus, " ") + strconv.FormatUint(threadNum, 10)+".fmt"
    }

    return outFileName
}

func writeFmttedTraceFileHeader(writer        *bufio.Writer,
                                threadAddress string) {
    outLine := fmt.Sprintf("pThreadData: %v\nTimings based on a CPU speed: %vGHz\n--------------------\n",
                         threadAddress,float64(cpuGHz))

    _, err := writer.WriteString(outLine)

    if err != nil {
        log.Fatalf("Couldn't write formatted trace: %v\n", err) 
    }
}

func outputTracePoint(writer *bufio.Writer,
                      traceIdent uint64,
                      traceValue uint64,
                      traceContext *TraceContextData,
                      traceParsingContext *TraceParseContext,
                      findLowCycles bool) {

    if traceIdent != 0 {
        tracePoint, foundTracePt := traceContext.TracePoints[traceIdent]
        outLine := ""

        if foundTracePt {
            trcTypeIndicator := " ";
            if tracePoint.Type == TracePointEntry {
                trcTypeIndicator = "{"
            } else if tracePoint.Type == TracePointExit {
                trcTypeIndicator = "}"
            } else if tracePoint.Type == TracePointIdent {
                trcTypeIndicator = "="
            } else if tracePoint.Type == TracePointHistoryBuffer {
                trcTypeIndicator = "|"
            }

            if (tracePoint.Value != "") {
                perfInfo := ""

                if (traceParsingContext.FuncEntry) {
                    traceParsingContext.InFunc = tracePoint.Function
                    traceParsingContext.FuncEntry = false
                }

                // Calculate performance from the engine entry / exit times
                if (tracePoint.Value == "exitTime") {
                    traceParsingContext.FuncExitCycles = uint64(traceValue)
                    if (findLowCycles) {
                        if (traceParsingContext.FuncExitCycles < traceContext.LowCycles) {
                            traceContext.LowCycles = traceParsingContext.FuncExitCycles
                        }
                    } else {
                        if (traceContext.BaseTime != 0) {
                            startDeltaCycles := traceParsingContext.FuncExitCycles-traceContext.LowCycles
                            timeNanos := traceContext.BaseTime + uint64(float64(startDeltaCycles) * traceParsingContext.CyclesToNanos)
                            perfInfo += fmt.Sprintf("[%vns] ", timeNanos)
                        }

                        if (traceParsingContext.InFunc != "") {
                            funcDeltaCycles := uint64(traceParsingContext.FuncExitCycles - traceParsingContext.FuncEntryCycles)
                            funcDeltaMillis := float64(funcDeltaCycles) * traceParsingContext.CyclesToMillis

                            perfInfo += fmt.Sprintf("%s took %.3fms (%d cycles)", 
                                                    traceParsingContext.InFunc, funcDeltaMillis, funcDeltaCycles)
                            traceParsingContext.InFunc = ""
                        }
                    }
                } else {
                    if (tracePoint.Value == "entryTime") {
                        traceParsingContext.FuncEntry = true
                        traceParsingContext.FuncEntryCycles = uint64(traceValue)

                        if (findLowCycles) {
                            if (traceParsingContext.FuncEntryCycles < traceContext.LowCycles) {
                                traceContext.LowCycles = traceParsingContext.FuncEntryCycles
                            }
                        } else {
                            if (traceContext.BaseTime != 0) {
                                startDeltaCycles := traceParsingContext.FuncEntryCycles-traceContext.LowCycles
                                timeNanos := traceContext.BaseTime + uint64(float64(startDeltaCycles) * traceParsingContext.CyclesToNanos)
                                perfInfo += fmt.Sprintf("[%vns] ", timeNanos)
                            }

                            if (traceParsingContext.FuncExitCycles != 0) {
                                funcDeltaCycles := uint64(traceParsingContext.FuncEntryCycles-traceParsingContext.FuncExitCycles)
                                funcDeltaMillis := float64(funcDeltaCycles) * traceParsingContext.CyclesToMillis

                                perfInfo += fmt.Sprintf("delay %.3fms (%d cycles)",
                                                        funcDeltaMillis, funcDeltaCycles)
                            }
                        }
                    }
                }

                outLine = fmt.Sprintf("%*s:%-*v %*s = %15d (0x%-16x)      %s %s%s %s\n", 
                                      int(traceContext.ColumnWidths["File"]),
                                      filepath.Base(tracePoint.File),
                                      int(traceContext.ColumnWidths["Line"]),
                                      tracePoint.Line,
                                      int(traceContext.ColumnWidths["Value"]),
                                      tracePoint.Value,
                                      traceValue, traceValue,
                                      trcTypeIndicator,
                                      tracePoint.Function,
                                      traceParsingContext.VolatileTrace,
                                      perfInfo)
            } else {
                outLine = fmt.Sprintf("%*s:%-*v %*s                                             %s %s%s\n", 
                                        int(traceContext.ColumnWidths["File"]),
                                        filepath.Base(tracePoint.File),
                                        int(traceContext.ColumnWidths["Line"]),
                                        tracePoint.Line,
                                        int(traceContext.ColumnWidths["Value"]),
                                        "",
                                        trcTypeIndicator,
                                        tracePoint.Function,
                                        traceParsingContext.VolatileTrace)
            }

        } else {
            fileIdent := uint32(traceIdent >> 32)
            lineNum   := uint32(traceIdent)

            fileName, knownFile  := traceContext.FileIdents[fileIdent]

            if knownFile {
                outLine = fmt.Sprintf("!!!Unexpected TracePoint: %s:%v%s\n",
                                              fileName, lineNum, traceParsingContext.VolatileTrace)
                fmt.Print(outLine)
            } else {
                outLine = fmt.Sprintf("!!!Unexpected TracePoint: line %v of unknown file with ident %v%s\n",
                                              lineNum, fileIdent, traceParsingContext.VolatileTrace)
                fmt.Print(outLine)
            }
        }

        if (writer != nil) {
            _, err := writer.WriteString(outLine)
            if err != nil {
                log.Fatalf("Couldn't write formatted trace: %v\n", err) 
            }
        }
    }
}


var tracePointRegExp *regexp.Regexp =  regexp.MustCompile("TP\\[\\s*([^,]+),\\s*(\\d+)\\s*\\]:\\s*(\\d+)\\s*(\\d+)\\s*(\\**)")
var endThreadRegExp *regexp.Regexp = regexp.MustCompile("ENDTHREAD:\\s*(.*)")
func parseThreadData(reader *bufio.Reader, 
                     outFileName string,
                     threadAddress string, 
                     traceContext *TraceContextData,
                     findLowCycles bool) {

    var writer *bufio.Writer = nil

    // 2.9GHz == 290000000 cycles per second == 290000 cycles per millisecond
    freqGHz := float64(cpuGHz)
    cyclesToMillis := float64(1)/(freqGHz*float64(1000000))
    cyclesToNanos := float64(1)/(freqGHz*float64(1000))
 
    parseContext := TraceParseContext{"", false, uint64(0), uint64(0), cyclesToMillis, cyclesToNanos, ""}

    for {
        line, err := reader.ReadString('\n')

        if err == io.EOF {
            break
        }

        matches := endThreadRegExp.FindStringSubmatch(line)
        
        if len(matches) == 2 {
            if matches[1] != threadAddress {
                log.Fatalf("Found an ENDTHREAD for %s expected %s\n", matches[1], threadAddress)
            }
            break
        }

        matches = tracePointRegExp.FindStringSubmatch(line)

        if len(matches) == 6 {
            if (writer == nil && !findLowCycles) {
                    outf, err := os.OpenFile(outFileName, os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0644)
                    if err != nil {
                        log.Fatalf("Error opening outfile: %v\n", err)
                    }
                    defer outf.Close()

                    writer = bufio.NewWriter(outf)
                    defer writer.Flush()
                    
                    writeFmttedTraceFileHeader(writer, threadAddress)
            }

            if matches[1] != threadAddress {
                log.Fatalf("Found a TP for %s expected %s\n", matches[1], threadAddress)
            }
            
            traceIdent, err := strconv.ParseUint(matches[3], 10, 64)

            if err != nil {
                log.Fatalf("Error parsing traceIdent: %s: %v", matches[1], err)
            }

            traceValue, err := strconv.ParseUint(matches[4], 10, 64)

            if err != nil {
                log.Fatalf("Error parsing traceValue: %s: %v", matches[2], err)
            }

            if matches[5] == "*" {
               parseContext.VolatileTrace = " *VOLATILE*"
            } else {
               parseContext.VolatileTrace = ""
            }

            outputTracePoint(writer,
                             traceIdent,
                             traceValue,
                             traceContext,
                             &parseContext,
                             findLowCycles)
        }
    }
}

var newThreadRegExp *regexp.Regexp = regexp.MustCompile("NEWTHREAD:\\s*(.*)")
var newThreadPrefixRegExp *regexp.Regexp = regexp.MustCompile("NEWTHREAD-([^:]*):\\s*(.*)")
func parseTraceFile(fileName string, outDir string, traceContext *TraceContextData, findLowCycles bool) {
    inf := os.Stdin

    //Open the input filename
    if fileName != "" {
        var err error
        inf, err = os.OpenFile(fileName, os.O_RDONLY, 0)
        if err != nil {
            log.Fatalf("Error opening infile: %v\n", err)
        }
    } else {
        fileName = "Trace"
    }

    if (outDir != "") {
        outDir += "/"
        os.MkdirAll(outDir, 0777);
    }

    //Read in the file line by line processing it and writing out the parsed file 
    reader := bufio.NewReader(inf)
    
    var threadNum uint64 = 0

    for {
        line, err := reader.ReadString('\n')
        
        if err == io.EOF {
            break
        }
        if err != nil {
            log.Fatalf("Couldn't read line from trace file: %v\n", err)
        }
        
        matches := newThreadRegExp.FindStringSubmatch(line)
        
        if len(matches) == 2 {
            outFileName := getFmtTraceFileName(outDir, fileName, "", threadNum)
            parseThreadData(reader, outFileName, matches[1], traceContext, findLowCycles)
            threadNum++
        } else {
            matches := newThreadPrefixRegExp.FindStringSubmatch(line)

            if len(matches) == 3 {
               outFileName := getFmtTraceFileName(outDir, fileName, matches[1], threadNum)
               parseThreadData(reader, outFileName, matches[2], traceContext, findLowCycles)
               threadNum++
            }
        }
    }

    if inf != os.Stdin {
        inf.Close()
    }
}
var RawConcatNameFmt *regexp.Regexp =  regexp.MustCompile("thread_([^_]*)_([^_]*)_([^_]*)_([^_]*).rawconcattrc")

func parseRawConcatTraceFile(rawConcatFilePath string,
                             threadNum         uint64,
                             findLowCycles     bool,
                             outDir            string,
                             traceContext *TraceContextData) bool {
    fmt.Println("raw file:", rawConcatFilePath)
    matches := RawConcatNameFmt.FindStringSubmatch(rawConcatFilePath)
    
    if len(matches) < 5 {
        log.Fatalf("Couldn't parse raw concat trace file name: %v\n", rawConcatFilePath)
    }
    thrdDataAddrStr   := matches[1]
    thrdDataStatusStr := matches[2] // active or saved
    
    bufCurPos, err := strconv.ParseUint(matches[3], 0, 64)
    if err != nil {
        log.Fatalf("Couldn't parse raw concat trace file name. Buf Cur Pos was %v. Err %v\n", matches[3], err)
    }

    bufSize, err := strconv.ParseUint(matches[4], 0, 64)
    if err != nil {
        log.Fatalf("Couldn't parse raw concat trace file name. Bufsize was %v. Err %v\n", matches[4], err)
    }

    //Open the input filename
    inf, err := os.OpenFile(rawConcatFilePath, os.O_RDONLY, 0)
    if err != nil {
        log.Fatalf("Error opening infile: %v\n", err)
    }
    defer inf.Close()

    traceIdents := make([]uint64, bufSize)
    traceValues := make([]uint64, bufSize)

    for i:= uint64(0); i < bufSize; i++ {
        var thenum uint64 //
        err = binary.Read(inf, binary.LittleEndian, &thenum)
        
        if err != nil {
            log.Fatalf("binary.Read failed: %v\n", err)
        }
        //Place numbers into the slice so that the bufCurPos is the first (oldest) entry:
        if i >= bufCurPos {
            traceIdents[i - bufCurPos] = thenum
        } else {
            traceIdents[i + bufSize - bufCurPos] = thenum
        }
    }
    for i:= uint64(0); i < bufSize; i++ {
        var thenum uint64 //
        err = binary.Read(inf, binary.LittleEndian, &thenum)
        
        if err != nil {
            log.Fatalf("binary.Read failed: %v\n", err)
        }
        //Place numbers into the slice so that the bufCurPos is the first (oldest) entry:
        if i >= bufCurPos {
            traceValues[i - bufCurPos] = thenum
        } else {
            traceValues[i + bufSize - bufCurPos] = thenum
        }
    }

    if (outDir != "") {
        outDir += "/"
        os.MkdirAll(outDir, 0777);
    }

    var writer *bufio.Writer = nil

    if (!findLowCycles) {
        outFileName := getFmtTraceFileName(outDir, "fromRaw", thrdDataStatusStr, threadNum)

        outf, err := os.OpenFile(outFileName, os.O_WRONLY|os.O_TRUNC|os.O_CREATE, 0644)
        if err != nil {
            log.Fatalf("Error opening outfile: %v\n", err)
        }
        defer outf.Close()

        writer = bufio.NewWriter(outf)
        defer writer.Flush()

        writeFmttedTraceFileHeader(writer,
                                   thrdDataAddrStr)
    }

    // 2.9GHz == 290000000 cycles per second == 290000 cycles per millisecond
    freqGHz := float64(cpuGHz)
    cyclesToMillis := float64(1)/(freqGHz*float64(1000000))
    cyclesToNanos := float64(1)/(freqGHz*float64(1000))

    parseContext := TraceParseContext{"", false, uint64(0), uint64(0), cyclesToMillis, cyclesToNanos, ""}

    for i:= uint64(0); i < bufSize; i++ {
        outputTracePoint(writer,
                         traceIdents[i],
                         traceValues[i],
                         traceContext,
                         &parseContext,
                         findLowCycles)
    }
    return true
}

//Function name is consider to be second word (first is return type) on line that isn't in reserved list (e.g. static, inline etc..)
var funcNameSplitter *regexp.Regexp =  regexp.MustCompile("[(\\s]+")
func getFuncName(srcline string) string {
    words := funcNameSplitter.Split(srcline, -1)

    skipped := 0
    funcName := "Unknown"

    for i:= 0; i < len(words); i++ {
        if (words[i] != "static" && words[i] != "inline" &&  words[i] != "XAPI" && words[i] != "WARN_CHECKRC") {

            if (skipped > 0) {
                //Remove any * we've accidentally captured as it returns a pointer
                funcName = strings.Replace(words[i],"*","",1)
                break
            } else {
                skipped++
            }
        }
    }

    return funcName
}

//1st return is tracedata... 
//2nd is whether line is a complete statement.. if false this line should be preprepended onto next line...
var traditionalCommentRemover *regexp.Regexp =  regexp.MustCompile("/\\*.*\\*/")
var cppCommentRemover  *regexp.Regexp =  regexp.MustCompile("//.*")
var conditionalRemover *regexp.Regexp = regexp.MustCompile("#if.*")
var ieutTRACELFinder *regexp.Regexp =  regexp.MustCompile("ieutTRACEL\\s*\\([^,]*,([^,]*)")
var ieutTRACE_HISTORYBUFFinder *regexp.Regexp =  regexp.MustCompile("ieutTRACE_HISTORYBUF\\s*\\([^,]*,([^)]*)")
var ieutTRACE_HISTORYBUF_IDENTFinder *regexp.Regexp =  regexp.MustCompile("ieutTRACE_HISTORYBUF_IDENT\\s*\\([^,]*,[^,]*,([^)]*)")

func parseSourceLine(fileName string, lineNum uint32, srcline string, parseContext *SourceParseContext) (*TracePointData,bool, string) {
    foundTracePt := false
    lineComplete := true

    //Remove comments and conditionals from src line
    srcLineNoComments := traditionalCommentRemover.ReplaceAllLiteralString(srcline, "")
    srcLineNoComments  = cppCommentRemover.ReplaceAllLiteralString(srcLineNoComments, "")
    srcLineNoComments  = conditionalRemover.ReplaceAllLiteralString(srcLineNoComments, "")

    oldCurlyDepth := parseContext.CurlyDepth

    openCurly  := strings.Count(srcLineNoComments, "{")
    closeCurly := strings.Count(srcLineNoComments, "}")
    openRound  := strings.Count(srcLineNoComments, "(")

    parseContext.CurlyDepth = parseContext.CurlyDepth + int32(openCurly) - int32(closeCurly)
   

    if (parseContext.CurlyDepth < 0) {
        log.Fatalf("Negative num of brackets on line %v of file %s\n", lineNum, fileName)
    }

    if oldCurlyDepth == 0 && openRound != 0 {
        //We're starting a function
        parseContext.CurrentFunction = getFuncName(srcLineNoComments)
        //fmt.Printf("%s %v\n", parseContext.CurrentFunction, lineNum)
    } else if oldCurlyDepth != 0 && parseContext.CurlyDepth == 0 {
        //We've left a function
        parseContext.CurrentFunction = ""
    }

    if (strings.Contains(srcLineNoComments, "ieutTRACE")) {
        foundTracePt = true

        if (!strings.Contains(srcLineNoComments, ";")) {
            lineComplete = false         
        }
    }

    var returnedData *TracePointData = nil

    if foundTracePt && lineComplete {
        //Get Variable text for thing that output it:
        valueText := ""
        matches := ieutTRACELFinder.FindStringSubmatch(srcLineNoComments)

        trcType := TracePointOther

        if len(matches) == 2 {
            valueText = strings.Trim(matches[1], " ")

            //Work out type of trace line, entry, exit etc...
            if (strings.Contains(srcLineNoComments,"FUNCTION_ENTRY")) {
                trcType = TracePointEntry
            } else if (strings.Contains(srcLineNoComments,"FUNCTION_EXIT")) {
                trcType = TracePointExit
            } else if (strings.Contains(srcLineNoComments,"FUNCTION_IDENT")) {
                trcType = TracePointIdent
            } 
        } else {
            matches = ieutTRACE_HISTORYBUFFinder.FindStringSubmatch(srcLineNoComments)

            if len(matches) == 2 {
                valueText = strings.Trim(matches[1], " ")
                trcType = TracePointHistoryBuffer
            } else {
                matches = ieutTRACE_HISTORYBUF_IDENTFinder.FindStringSubmatch(srcLineNoComments)

                if len(matches) == 2 {
                    valueText = strings.Trim(matches[1], " ")
                    trcType = TracePointHistoryBuffer
                }
            }

        }

        returnedData = &TracePointData{ fileName, lineNum,
                                        parseContext.CurrentFunction,
                                        valueText, trcType}
    }

    return returnedData, lineComplete, srcLineNoComments
}

func parseSourceFile(fileName string, traceContext *TraceContextData) bool {
    //Work out the hash of the file name (because it's used as first 4 bytes of tracepoint identifier)
    h := md5.New()
    io.WriteString(h, filepath.Base(fileName))
    md5hash :=  h.Sum(nil)

    buf := bytes.NewBuffer(md5hash)

    var partialmd5 uint32 //first 4 bytes are first 4 bytes of md5sum hash

    err := binary.Read(buf, binary.BigEndian, &partialmd5)
    if err != nil {
        fmt.Println("binary.Read failed:", err)
    }
    fmt.Printf("Parsing %s %x...\n", filepath.Base(fileName), partialmd5)

    //Add the identifier for this file...
    traceContext.FileIdents[partialmd5] = fileName

    //Open the input filename
    inf, err := os.OpenFile(fileName, os.O_RDONLY, 0)
    if err != nil {
        fmt.Println("Error opening infile:", err)
        return false
    }
    defer inf.Close()

    //Read in the file line by line processing it and writing out the parsed file 
    reader := bufio.NewReader(inf)

    parseContext := SourceParseContext{"", 0}

    var lineNum uint32 = 0

    incompleteStatement := ""

    for {
        lineNum++

        srcline, err := reader.ReadString('\n')

        if err == io.EOF {
            break
        }
        if err != nil { panic(err)}

        var statement string

        if incompleteStatement != "" {
            statement = incompleteStatement + " " + strings.TrimLeft(srcline, " \n\r")
        } else {
            statement = srcline
        }

        trcPtData, lastLineComplete, modifiedSrcLine := parseSourceLine(fileName, lineNum, statement, &parseContext)

        if lastLineComplete {
            incompleteStatement = ""

            if trcPtData != nil {
                //Work out tracepoint identifier
                var identifier uint64
                identifier = (uint64(partialmd5) << 32) | uint64(lineNum)

                traceContext.TracePoints[identifier] = trcPtData
            }
        } else {
            incompleteStatement = strings.TrimRight(modifiedSrcLine, " \n\r")
        }
    }
 
    return true
}

var walkTraceContextData *TraceContextData

var SourceFileFinder *regexp.Regexp =  regexp.MustCompile("(^/.+\\.(c|h|cpp)$)")
func sourceDirWalkVisit(path string, fileInfo os.FileInfo, err error) error {
    if fileInfo.IsDir() == false {
        matches := SourceFileFinder.FindStringSubmatch(path)
        if len(matches) == 3 {
            sourceFilePath := matches[1]
            parseSourceFile(sourceFilePath, walkTraceContextData)
        } 
    }
    return nil
}

func parseSourceDir(dirName string, traceContext *TraceContextData) bool {
    walkTraceContextData = traceContext

    dirPath,err := filepath.Abs(dirName)

    if err != nil {
        fmt.Printf("filepath.Abs() returned error %v\n", err)
    } else {
        err = filepath.Walk(dirPath, sourceDirWalkVisit)

        if err != nil {
            fmt.Printf("filepath.Walk() returned error %v\n", err)
        }
    }

    return true
}

var RawWalkTraceContextData *TraceContextData
var RawWalkOutDir string
var RawWalkFindLowCycles bool
var RawWalkThreadNum uint64

var RawConcatFileFinder *regexp.Regexp =  regexp.MustCompile("(^/.+\\.rawconcattrc$)")
func rawTraceDirWalkVisit(path string, fileInfo os.FileInfo, err error) error {
    if fileInfo.IsDir() == false {
        matches := RawConcatFileFinder.FindStringSubmatch(path)
        if len(matches) == 2 {
            rawConcatFilePath := matches[1]
            parseRawConcatTraceFile(rawConcatFilePath,
                                    RawWalkThreadNum, 
                                    RawWalkFindLowCycles,
                                    RawWalkOutDir,
                                    RawWalkTraceContextData)
            RawWalkThreadNum++
        } 
    }
    return nil
}
func parseRawTraceDir(dirName string,
                      outDir string,
                      traceContext *TraceContextData,
                      findLowCycles bool) bool {
    RawWalkOutDir           = outDir
    RawWalkTraceContextData = traceContext
    RawWalkFindLowCycles    = findLowCycles
    RawWalkThreadNum        = uint64(0)

    dirPath,err := filepath.Abs(dirName)

    if err != nil {
        fmt.Printf("filepath.Abs() returned error %v\n", err)
    } else {
        err = filepath.Walk(dirPath, rawTraceDirWalkVisit)

        if err != nil {
            fmt.Printf("filepath.Walk() returned error %v\n", err)
        }
    }

    return true
}

func setColumnWidths(traceContext *TraceContextData) {
    traceContext.ColumnWidths["File"] = 0
    traceContext.ColumnWidths["Line"] = 0
    traceContext.ColumnWidths["Value"] = 0

    for _, v := range traceContext.TracePoints {
        length := uint32(len(filepath.Base(v.File)))
        if length > traceContext.ColumnWidths["File"] {
            traceContext.ColumnWidths["File"] = length
        }
        length = uint32(len(strconv.FormatUint(uint64(v.Line),10))) 
        if length > traceContext.ColumnWidths["Line"] {
            traceContext.ColumnWidths["Line"] = length
        }
        length = uint32(len(v.Value))
        if length > traceContext.ColumnWidths["Value"] {
            traceContext.ColumnWidths["Value"] = length
        }
    }
}

type arrayFlags []string

var srcDirs arrayFlags
var rawTraceDirs arrayFlags

func (i *arrayFlags) String() string {
    return "my string representation"
}

func (i *arrayFlags) Set(value string) error {
    *i = append(*i, value)
    return nil
}

func main() {
    if len(os.Args) < 2 {
        fmt.Printf("trcfmt -t <unformatted trace file> -s <sourceDir> [-s <sourceDir>] [-o <outDir>]\n")
    } else {
        //Create a hash of tracepoint identifier to data about the tracepoint
        tracePoints := make(map[uint64] *TracePointData)
        //Create a hash of tracefile identifier to file name
        fileIdents := make(map[uint32] string)
        //Create a hash of field names to column widths
        columnWidths := make(map[string] uint32)

        outDir := flag.String("o","","Output directory to use")
        fileName := flag.String("t","", "Filename to parse")
        baseTime := flag.Uint64("b", 0, "Base time to use")
        flag.Var(&rawTraceDirs, "r", "rawTraceDirs")
        flag.Var(&srcDirs, "s", "Source diretory.")

        flag.Parse()

        if len(rawTraceDirs) == 0 {
            if *fileName == "" {
                fmt.Printf("Reading from STDIN\n"); 
            } else { 
                if _, err := os.Stat(*fileName); err == nil {
                   fmt.Printf("Parsing trace file %s...\n", *fileName)
                } else {
                   fmt.Printf("File %s doesn't exist\n", *fileName)
                   os.Exit(1)
                }
            }
        }

        traceContext := TraceContextData{ *baseTime, math.MaxUint64, fileIdents, tracePoints, columnWidths}
 
        for i := 0; i < len(srcDirs); i++ {
            parseSourceDir(srcDirs[i], &traceContext)
        }

        fmt.Printf("Setting column widths...\n")
        setColumnWidths(&traceContext)

        // Actually produce the formatted trace histories
        if len(rawTraceDirs) > 0 {
            // Find the lowest cycle count we have across all trace histories if we've
            // been given a baseTime (WARNING so far not tested/used for raw trace files!)
            if (*baseTime != 0) {
                for i := 0; i < len(rawTraceDirs); i++ {
                    parseRawTraceDir(rawTraceDirs[i], "", &traceContext, true)
                }
            }
            for i := 0; i < len(rawTraceDirs); i++ {
                parseRawTraceDir(rawTraceDirs[i], *outDir, &traceContext, false)
            }
        } else {
            // Find the lowest cycle count we have across all trace histories if we've
            // been given a baseTime.
            if (*baseTime != 0) {
                parseTraceFile(*fileName, "", &traceContext, true)
            }
            parseTraceFile(*fileName, *outDir, &traceContext, false)
        }
    }
}
