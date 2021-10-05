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
//build: 
//gccgo -g trchistfmt.go -o trchistfmt
// 
//This program is used to format trace history buffers dumped by
//using the dumptracehistory.py script in gdb on a core file (or 
//attached to a running process)
//e.g.:
//./trchistfmt /tmp/dumpedhistory ../src/*.c

package main

import "fmt" // Package implementing formatted I/O.
import "os"
import "bufio"
import "io"
import "path/filepath"
import "regexp"
import "log"
import "strconv"
import "sort"

type parseState int
const (
        ParseStateNone parseState = iota
        ParseStateFoundEntry
        ParseStateInEngineFunc
)

//Once initialised will determine which subFunctions we scan for in the trace
type subFunctionDefinition struct {
    name            string
    startEyeCatcher string
    stopEyeCatcher  string
}

//Data about subfunctions that have been seen in the trace
type subFunctionData struct {
    avgNumCalls        float64
    totalTime          uint64
}

//Store data about timings  
type functionData struct {
    numCalls               uint64
    totalTime              uint64
    avgDuration            float64
    fastestDuration        uint64
    longestDuration        uint64
    longestDurationLine    uint64
    subFuncs               map[string] *subFunctionData
}

//Data about subfunctions of a call we are currently parsing
type subFunctionContext struct {
    startTime    uint64
    numCalls     uint64
    totalTime    uint64
}

type threadContext struct {
    state                 parseState
    currEntryTime         uint64
    currFunc              string
    firstTime             uint64   // First entry time seen in this thread
    lastTime              uint64   // last exit time seen in this thread
    totalEngineTime       uint64   // total engine time in this thread
    lineNum               uint64   // during parsing, line we're on
    funcData              map[string] *functionData  //funcName -> funcData
    subFuncContext        map[string] *subFunctionContext 
}

type perfData struct {
    threadData       map[string] *threadContext //trcFileName -> threadData
}

var entryTimeRegExp *regexp.Regexp = regexp.MustCompile(".*:\\d*\\s*entryTime\\s*=\\s*(\\d*)")

func findEntryTime(srcLine string) uint64 {
    var entryTime uint64
    entryTime = 0

    matches := entryTimeRegExp.FindStringSubmatch(srcLine)
        
    if len(matches) == 2 {
        parsedTime, err := strconv.ParseUint(matches[1], 10, 64) 

        if err != nil {
            log.Fatalf("Failed to parse int %s\n", matches[1])
        }
        entryTime = parsedTime        
    }
    return entryTime
}

var exitTimeRegExp *regexp.Regexp = regexp.MustCompile(".*:\\d*\\s*exitTime\\s*=\\s*(\\d*)")

func findExitTime(srcLine string) uint64 {
    var exitTime uint64
    exitTime = 0

    matches := exitTimeRegExp.FindStringSubmatch(srcLine)
        
    if len(matches) == 2 {
        parsedTime, err := strconv.ParseUint(matches[1], 10, 64) 

        if err != nil {
            log.Fatalf("Failed to parse int %s\n", matches[1])
        }
        exitTime = parsedTime        
    }
    return exitTime
}

var funcNameRegExp *regexp.Regexp = regexp.MustCompile(".*:\\d*\\s*[^=]*\\s*=\\s*\\d*\\s*\\([^)]*\\)\\s*\\{\\s*(.*)")

func findFuncName(srcLine string) string {
    funcName := ""

    matches := funcNameRegExp.FindStringSubmatch(srcLine)
        
    if len(matches) == 2 {
        funcName = matches[1]
    }

    return funcName
}

var subFuncEyeCatcherRegExp *regexp.Regexp = regexp.MustCompile(".*:\\d*\\s*TTS_([^_]*)_([^\\s]*)\\s*=\\s*(\\d*)")

func trackSpecialFunctions(srcLine string, context *threadContext) {
    
    matches := subFuncEyeCatcherRegExp.FindStringSubmatch(srcLine)

    if len(matches) == 4 {
        foundStart := false
        foundStop := false

        if matches[1] == "Start" {
            foundStart = true
        } else if matches[1] == "Stop" {
            foundStop = true
        }

        if foundStart == foundStop {
            log.Fatalf("Malformed TTS eyecatcher: %s (line %v)\n", matches[1], context.lineNum)
        }

        subFuncName := matches[2]
        subFuncCtxt, foundSubFuncCtxt := context.subFuncContext[subFuncName]
        
        if !foundSubFuncCtxt {
            subFuncCtxt = &subFunctionContext{ 
                                    startTime: 0,
                                    numCalls:  0,
                                    totalTime: 0}
            context.subFuncContext[subFuncName] = subFuncCtxt
        }
            
        parsedTime, err := strconv.ParseUint(matches[3], 10, 64) 

        if err != nil {
            log.Fatalf("Failed to parse int %s (line %v)\n", matches[3], context.lineNum)
        }

        if foundStart {
            if subFuncCtxt.startTime != 0 {
                fmt.Printf("Found start of %s whilst already inside it (line %v)\n", 
                                                      subFuncName, context.lineNum)
            } else {
                subFuncCtxt.startTime = parsedTime
            }
        } else {
            if subFuncCtxt.startTime == 0 {
                fmt.Printf("Found end of %s whilst not already inside it (line %v)\n", 
                                                      subFuncName, context.lineNum)
            } else {
                duration := parsedTime - subFuncCtxt.startTime
                subFuncCtxt.numCalls++
                subFuncCtxt.totalTime += duration
                subFuncCtxt.startTime = 0 //No longer in subfunction
            }
        }
    }
}

func updateSpecialFunctions(funcData *functionData, context *threadContext) {
    for subFuncName, subFuncCurrent := range context.subFuncContext {

        if subFuncCurrent.startTime != 0 {
            //We've ended an outer function whilst we still think we are in a subfunction
            fmt.Printf("line %v: Ended func but still in sub func %s\n",
                              context.lineNum, subFuncName) 
        }

        subFuncData, foundSubFuncData := funcData.subFuncs[subFuncName]
            
        if !foundSubFuncData {
            subFuncData = &subFunctionData{ 
                                        avgNumCalls:  0,
                                        totalTime: 0}
            funcData.subFuncs[subFuncName] = subFuncData
        }


        //The numCalls for the function already includes the call that these special functions were found in
        //So the avgnum calls = total number subfunccalls/ number Outer Func calls
        //                    = ((oldAvg * oldNumOuterFuncCall)+numExtraSubfuncCalls)/newNumOuterFuncCalls
        subFuncData.avgNumCalls = ((subFuncData.avgNumCalls * float64(funcData.numCalls-1))+
                                                          float64(subFuncCurrent.numCalls))/
                                       float64(funcData.numCalls)
        subFuncData.totalTime += subFuncCurrent.totalTime

        subFuncCurrent.startTime = 0
        subFuncCurrent.numCalls  = 0
        subFuncCurrent.totalTime = 0
    }
}

func parseTraceLine(srcLine string, context *threadContext) {
    switch(context.state) {
    case ParseStateNone:
        //Try and find an entry time
        entryTime := findEntryTime(srcLine)

        if entryTime != 0 {
           context.currEntryTime = entryTime
           context.state    = ParseStateFoundEntry

            if context.firstTime == 0 {
                context.firstTime = entryTime
            }
        }

    case ParseStateFoundEntry:
        funcName := findFuncName(srcLine)

        if funcName != "" {
            context.currFunc = funcName
            context.state = ParseStateInEngineFunc
        } else {
            log.Fatalf("Failed to read function name from: %s\n", srcLine)
        }

    case ParseStateInEngineFunc:
        //Look to see if we can see any of the special functions (store commit, checkWaiters) we are tracking
        trackSpecialFunctions(srcLine, context)

        //Look for the exitTime that signifies the end of the function
        exitTime := findExitTime(srcLine)

        if exitTime != 0 {
            duration := exitTime - context.currEntryTime

            funcData, foundFunc := context.funcData[context.currFunc]
           
            if foundFunc {
                funcData.totalTime += duration
                funcData.avgDuration = ((funcData.avgDuration * float64(funcData.numCalls)) + float64(duration))/ float64(1+funcData.numCalls)
                funcData.numCalls += 1

                if funcData.fastestDuration == 0 || duration < funcData.fastestDuration {
                    funcData.fastestDuration = duration
                }
                if funcData.longestDuration == 0 || duration > funcData.longestDuration {
                    funcData.longestDuration     = duration
                    funcData.longestDurationLine = context.lineNum 
                }
            } else {
                funcData =  &functionData{ numCalls: 1,
                                           totalTime: duration,
                                           avgDuration: float64(duration),
                                           fastestDuration: duration,
                                           longestDurationLine: context.lineNum}
                funcData.subFuncs = make(map[string] *subFunctionData)
                context.funcData[context.currFunc] = funcData
            }

            //Now we have a complete function... update the stats for the special functions we are tracking
            updateSpecialFunctions(funcData, context)

            context.totalEngineTime += duration

            //Record we are no longer in a function
            context.currEntryTime    = 0
            context.currFunc         = ""
            context.state            = ParseStateNone

            context.lastTime         = exitTime
        }
    }
}

func parseTraceFile(fileName string, data *perfData) bool {
    
    shortFileName := filepath.Base(fileName)
    fmt.Printf("Parsing %s...\n", shortFileName)

    //Open the input filename
    inf, err := os.OpenFile(fileName, os.O_RDONLY, 0)
    if err != nil {
        fmt.Println("Error opening infile:", err)
        return false
    }
    defer inf.Close()
    
    //Read in the file line by line processing it and writing out the parsed file 
    reader := bufio.NewReader(inf)

    context := threadContext{state: ParseStateNone,
                             currEntryTime: 0,
                             currFunc: "",
                             firstTime: 0,
                             lastTime: 0,
                             lineNum: 0,
                             funcData: make(map[string] *functionData)}

    context.subFuncContext = make(map[string] *subFunctionContext)

    for {
        context.lineNum++

        srcline, err := reader.ReadString('\n')

        if err == io.EOF {
            break
        }
        if err != nil { panic(err)}

        parseTraceLine(srcline, &context)
    }

    data.threadData[shortFileName] = &context
    return true
}

func sortTotalTime(funcHash map[string] *functionData) []string {
    funcNames := make([]string, 0, 100)
    funcTimes := make([]uint64, 0, 100)

    for funcName, funcData := range funcHash {
        funcTime := funcData.totalTime
        inserted := false

        for i:=0 ; i<len(funcTimes); i++ {
            if( funcTime > funcTimes[i]) {
                //New func is more important (we spent more time in it)... insert it into the list
                funcNames = append(funcNames, "")
                funcTimes = append(funcTimes, 0)
                copy(funcNames[i+1:], funcNames[i:])
                copy(funcTimes[i+1:], funcTimes[i:])
                funcNames[i] = funcName
                funcTimes[i] = funcTime
                inserted = true
                break
            }
        }
        if !inserted {
            funcNames = append(funcNames, funcName)
            funcTimes = append(funcTimes, funcTime)
        }
    }

    return funcNames
}

func printFuncData(funcName string, funcData *functionData) {
    fmt.Printf("%s (%v calls): totalTime: %v avg: %v (%v to %v",
                        funcName, funcData.numCalls, funcData.totalTime, uint64(funcData.avgDuration),
                        funcData.fastestDuration, funcData.longestDuration)

    if funcData.longestDurationLine != 0 {
        fmt.Printf(" (on line %v))\n",
                         funcData.longestDurationLine)
    } else {
        fmt.Printf(")\n")
     }

    for subFuncName, subFuncData := range(funcData.subFuncs) {
        if (subFuncData.totalTime > 0) {
            fmt.Printf("   %s: avg number: %.1f, avg duration %v total time %v\n",
                subFuncName,
                subFuncData.avgNumCalls,
                uint64(float64(subFuncData.totalTime)/ (subFuncData.avgNumCalls*float64(funcData.numCalls))),
                subFuncData.totalTime)
        }
    }
}

func printPerfReport(data *perfData) {
    //Build up whole program data for each engine call by combining the data for each thread
    allFuncs   := make(map[string] *functionData)

    for _,tData := range data.threadData {
        for funcName, threadFuncData := range tData.funcData {
            allFuncData, foundFunc := allFuncs[funcName]
           
            if !foundFunc {
                allFuncData = &functionData{}
                allFuncData.subFuncs  = make(map[string] *subFunctionData)
                allFuncs[funcName] = allFuncData
            }

            //Merge the subfunction data for this thread, with our culumative data
            for subFuncName, subFuncData := range (threadFuncData.subFuncs) {
                allThreadSubFunc, foundAllThread := allFuncData.subFuncs[subFuncName]

                if !foundAllThread {
                    allThreadSubFunc = &subFunctionData {
                                                avgNumCalls: 0,
                                                totalTime:   0 }

                    allFuncData.subFuncs[subFuncName] = allThreadSubFunc
                }
                allThreadSubFunc.avgNumCalls = ((allThreadSubFunc.avgNumCalls * float64(allFuncData.numCalls) )+
                                                (subFuncData.avgNumCalls      * float64(threadFuncData.numCalls)))/
                                                       float64(allFuncData.numCalls + threadFuncData.numCalls)

                allThreadSubFunc.totalTime += subFuncData.totalTime
            }

            allFuncData.totalTime += threadFuncData.totalTime
            allFuncData.avgDuration = (  ( (allFuncData.avgDuration * float64(allFuncData.numCalls))+ 
                                               (threadFuncData.avgDuration * float64(threadFuncData.numCalls))) /
                                            float64(threadFuncData.numCalls+allFuncData.numCalls))

            allFuncData.numCalls += threadFuncData.numCalls

            if allFuncData.fastestDuration == 0 || threadFuncData.fastestDuration < allFuncData.fastestDuration {
                allFuncData.fastestDuration = threadFuncData.fastestDuration
            }
            if allFuncData.longestDuration == 0 || threadFuncData.longestDuration > allFuncData.longestDuration {
                allFuncData.longestDuration = threadFuncData.longestDuration
                allFuncData.longestDurationLine = 0 //for the whole program, we don't show a line as we don't know which file 
            }
        }
    }

    // Get the filenames in a consistent order for comparison
    fileNames := make([]string, len(data.threadData))
    i := 0
    for fileName, _ := range data.threadData {
        fileNames[i] = fileName
        i++
    }
    sort.Strings(fileNames)

    fmt.Printf("Per Thread Stats\n==================\n\n")
    for _, fileName := range fileNames {
        threadData := data.threadData[fileName]
        traceDuration := threadData.lastTime - threadData.firstTime //We use per-thread first & last times,
                                                                    //If a thread is half way through a long call when trace starts.... 
                                                                    //can't count time until next call starts

        fmt.Printf("%s: In engine %v out of %v (%v %%)\n",
                        fileName, threadData.totalEngineTime, traceDuration,
                        (float64(threadData.totalEngineTime)/float64(traceDuration))*float64(100))

        sortedFuncs := sortTotalTime(threadData.funcData)
        for _,funcName := range sortedFuncs {
            printFuncData(funcName, threadData.funcData[funcName])
        }
        fmt.Printf("\n\n")
    }

    fmt.Printf("Whole Program Stats\n======================\n\n")
    sortedFuncs := sortTotalTime(allFuncs)
    for _,funcName := range sortedFuncs {
        printFuncData(funcName, allFuncs[funcName])
    }
}


func main() {
    if len(os.Args) < 2 {
        fmt.Printf("trcperfreport <formatted fast trace files>\n")
    } else {
        data := perfData{}

        data.threadData =  make(map[string] *threadContext)

        for i := 1; i < len(os.Args); i++ {
            parseTraceFile(os.Args[i], &data)            
        }
        printPerfReport(&data)
    }
}
