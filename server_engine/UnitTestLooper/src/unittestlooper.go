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

//Build using
//    gccgo -g src/unittestlooper.go -o unittestlooper
//(or with non-default gcc e.g:
// /usr/local/gcc-10.1.0/bin/gccgo -g unittestlooper.go -o unittestlooper -Wl,-rpath,/usr/local/gcc-10.1.0/lib64
//)
 //Todo:
 // 1) Tidy up log file partitioning
 // 2) Install new lcov tool on down so we can (optionally?) do coverage runs there
 // 3) Allow attachments to the emails and attach relevant logs
 // 4) Allow running of specific modes e.g. dbgtest or specific unit tests
 // 5) Command line options to immediately clean up earlier runs (esp. shared mem)
 // 6) Periodically automatically remove old logs and old runs

package main 

import (
   "fmt"           //Printf, Sprintf and friends
   "os"            //Wrapper around Operating System
   "os/exec"       //Running programs
   "log"
   "text/template" //Generating email body
   "net/smtp"      //For sending email
   "bytes"
   "time"
   "strings"
   "path/filepath"
   "math/rand"
   "strconv"
   "io"
   "flag"
)

var PID int64 = int64(os.Getpid())

const ROOTDIR = "/looper"

const ASAN_OPTIONS = "suppressions="+ROOTDIR+"/leak_suppressions.txt"
const ASAN_NOLEAKCHECK_OPTIONS = "detect_leaks=0"

const ASYNCMODE_NOASYNC         = 0
const ASYNCMODE_FAKEASYNC       = 1
const ASYNCMODE_DISKPERSISTENCE = 2
const ASYNCMODE_NUMMODES        = 3
var AsyncMode_Fixed bool = false

const ASANMODE_NOASAN           = 0
const ASANMODE_ASAN             = 1
const ASANMODE_ASAN_NOLEAKCHECK = 2
const ASANMODE_NUMMODES         = 3
var AsanMode_Fixed bool = false

const MALLOC_WRAPPER_DISABLED   = 0
const MALLOC_WRAPPER_ENABLED    = 1
const MALLOCMODE_NUMMODES       = 2
var MallocMode_Fixed bool = false

var TESTROOTDIR string = ROOTDIR+"/testroot_"+strconv.FormatInt(PID,10)
var SROOT string = TESTROOTDIR +"/sroot"
var BROOT string = TESTROOTDIR + "/broot"
var TEST_DIR string = SROOT+"/server_engine"

var STOREROOT string = TESTROOTDIR+"/storeroot"
var EMAIL_ON_FAILURE = false

//const IMA_ICU_HOME = ROOTDIR + "/icu/usr/local"
//const IMA_BOOST_HOME = ROOTDIR + "/boost"
const IMA_ICU_HOME = "/usr/local/icu_60_2"
const IMA_BOOST_HOME = "/usr/local/boost_1_66"
const RTC_CMDLINE_BASE = ROOTDIR +"/bin/RTCcmdline"
const RTC_SCRIPT_BASE = RTC_CMDLINE_BASE + "/jazz/scmtools/eclipse/scripts"
const RTC_BINARY =  RTC_CMDLINE_BASE + "/jazz/scmtools/eclipse/scm"
const BUILD_COMMAND = ROOTDIR + "/bin/buildism"
const SANITIZER_GCC_HOME = "/usr/local/gcc-10.1.0"

const RTC_SERVER = "https://jazzserver.example.com:9443/ccm"
//const RTC_DEFAULT_STREAM = "IMADev Stream"
const RTC_DEFAULT_STREAM = "Experimental.engine.main"
const RTC_WORKSPACE_PREFIX = "Engine-AutomatedUnitTestTool-"

const GHE_URL = "git@github.com:changemeorg/changemerepo.git"

const TEST_TIMEOUT_HOURS                 = 1
const TEST_DISKPERSISTENCE_TIMEOUT_HOURS = 6

const TEST_COMMAND = "make"
const IMA_TEST_TEMP_PATH = "/var/tmp"

//Needed for SpiderCast on newer compilers:
const IMA_EXTRA_CPPFLAGS = "-std=gnu++11"


//Set from the RTC_PASSWORD env var
var rtc_passwd string
var rtc_user string

//Generated on start up to distinguish this invocation of the program
var processIdent string

//Overall number of current test run. Used in log file names:
var totalRunNumber uint64  = 0

var cmdLineWorkspace string
var cmdLineStream    string
var cmdLineMakeArgs  string
var cmdLineAsyncMode int
var activeAsyncMode  uint32 = 0
var cmdLineAsanMode  int
var activeAsanMode   uint32 = 0
var cmdLineMallocMode int
var activeMallocMode uint32 = 0
var activeDryRun     bool
var cmdLineGHEKey    string
var useRTC           bool //If not - uses GHE

func init() {
    flag.StringVar(&cmdLineWorkspace, "w", "", "RTC workspace to run against")
    flag.StringVar(&cmdLineStream,    "s", "", "GHE Branch/RTC stream to create a workspace against")
    flag.StringVar(&cmdLineGHEKey,    "g", "", "Path to the key used to log in to GHE")
    flag.IntVar(&cmdLineAsanMode,    "m", 0, 
                "0 for no ASAN, 1 for ASAN, 2 for ASAN but no leak checking, -1 for random")
    flag.IntVar(&cmdLineAsyncMode,   "a", -1, 
                "0 for shared mem, 1 for fake async shared mem, 2 for disk")
    flag.StringVar(&cmdLineMakeArgs,  "t", "", "Args to pass to make e.g. dbgfull")
    flag.IntVar(&cmdLineMallocMode,  "h", -1,
                "0 for no malloc wrapper, -1 for malloc wrapper, omit for random")
    flag.BoolVar(&activeDryRun, "d" , false ,
                "Dry run flag - no e-mail will be sent")
    flag.BoolVar(&useRTC, "r", false, "Use RTC (if not set uses GHE)")
}

func main() {
   const testLoopsPerBuild      = 15
   const testLoopsPerASANBuild  = 7
   const firstSuccessEmailAfter = 20
   const successEmailsEvery     = 150

   flag.Parse()

   processFlags()

   //Make test root and initialise logfile
   err := os.MkdirAll(TESTROOTDIR, 0770)
   if err != nil {
      dieError("Creating TESTROOTDIR", err.Error())
   }

   err = os.MkdirAll(STOREROOT, 0770)
   if err != nil {
      dieError("Creating STROREROOT", err.Error())
   }

   logf, err := os.OpenFile(TESTROOTDIR+"/initialisation", os.O_WRONLY|os.O_CREATE, 0660)
   if err != nil {
      dieError("Initial Setup", err.Error())
   }
   log.SetOutput(io.MultiWriter(logf, os.Stdout)) 

   setupEnv()

   randomValue := rand.New(rand.NewSource(time.Now().UnixNano()))
   
   //Don't include covtest or covfull as the lcov with RHEL6 can not deal with output from gcc 4.9 which
   //we use for ASAN.
   //More details:   https://bugzilla.redhat.com/show_bug.cgi?id=787502
   //                http://pkgs.fedoraproject.org/cgit/lcov.git/diff/?id=25a274d9dbd6813b160e8c8c6ab12791dff8733e
   testArgsArray := []string{"dbgtst",
                          "dbgfull",
                          "tst",
                          "full"}

   totalRunNumber++ //starting first run from this point
    
   EMAIL_ON_FAILURE = !activeDryRun;
   for {
      asanMode  := uint32(0)
      asyncMode := uint32(0)
      mallocMode := uint32(0)
      testLoops := testLoopsPerBuild

      //Decide whether to use ASAN for this test of test runs....
      if (AsanMode_Fixed) {
         asanMode = uint32(cmdLineAsanMode)
      } else if (randomValue.Uint32()%2 == 0) {
         //We'll use ASAN... we won't leak check as some tests have known leaks
         asanMode = ASANMODE_ASAN_NOLEAKCHECK
      }
      activeAsanMode = asanMode

      if (asanMode != 0) {
         testLoops = testLoopsPerASANBuild
      }

      //Decide whether to use noAsync, fakeAsync or real disk persistence
      if (AsyncMode_Fixed) {
          asyncMode = uint32(cmdLineAsyncMode)
      } else {
          asyncMode = randomValue.Uint32()%ASYNCMODE_NUMMODES
      }
      activeAsyncMode = asyncMode
      
      if (asyncMode == ASYNCMODE_DISKPERSISTENCE) {
         testLoops = 1
      }

      if (MallocMode_Fixed) {
         mallocMode = uint32(cmdLineMallocMode)
      } else {
         mallocMode = randomValue.Uint32()%MALLOCMODE_NUMMODES
      }
      activeMallocMode = mallocMode
       
      newlogf, err := os.OpenFile(TESTROOTDIR+"/"+strconv.FormatUint(totalRunNumber,10)+"-"+
                                                  strconv.FormatUint((totalRunNumber+uint64(testLoops)-1),10)+"_looperlog",
                                  os.O_WRONLY|os.O_CREATE, 0660)
      if err != nil {
         dieError("Opening logfile", err.Error())
      }
      log.SetOutput(io.MultiWriter(newlogf, os.Stdout)) 
   
      logf.Close()
      logf = newlogf

      log.Printf("About to build with asanMode: %v, asyncMode: %v, mallocMode: %v\n", 
                          asanMode, asyncMode, mallocMode)

      buildAttempt := 0

      for {
         //Clear current source and binary trees
         clearDownTrees()

      //Get new source tree
         err := getSource(buildAttempt)

         if (err == nil) {
            //Compile up the sources..
            err = buildProduct(buildAttempt, asanMode, asyncMode, mallocMode)

            if (err == nil) {
               //Done build - woo
               break
            } else {
               //Failed to build
               var emailbody string
               fmt.Sprintf( emailbody, "\n\nBuild Attempt: %v\n%v\n",
                               buildAttempt, err.Error())
               sendTestStatusEmail("Failed to build product", emailbody)
            }
         } else {
            //Failed to get source from RTC
            if (buildAttempt % 5 == 4) {
               var emailbody string
               fmt.Sprintf( emailbody, "\n\nBuild Attempt: %v\n%v\n",
                               buildAttempt, err.Error())
               sendTestStatusEmail("Failed to get source from RTC", emailbody)
            }
            log.Printf("Failed to get source from RTC (attempt %v)\n", 
                              buildAttempt)
         }

         if (err != nil) {
            buildAttempt++
            log.Printf("Waiting before retrying (next attempt is %v)\n", 
                              buildAttempt)
            time.Sleep(15 * time.Minute)
         }
      }

      // Clean up any directories / files
      testTimeout := uint32(TEST_TIMEOUT_HOURS)

      if (asyncMode == ASYNCMODE_DISKPERSISTENCE) {
          testTimeout = uint32(TEST_DISKPERSISTENCE_TIMEOUT_HOURS)
      }

      runTests("cleanupTests", 0, 0, 0, testTimeout)

      statusEmailDue := false
      
      //Loop running the unit tests
      for i := 0; i < testLoops; i++ {
         if ((totalRunNumber == firstSuccessEmailAfter) ||
             ( (totalRunNumber >= successEmailsEvery)  &&
               (totalRunNumber % successEmailsEvery) == 0)) {
            statusEmailDue = true
         }

         if (statusEmailDue && !activeDryRun) {
               subject := fmt.Sprintf("Unit Test successful. now doing loop %v", totalRunNumber)
               sentemail := sendTestStatusEmail(subject, "We rock")

               if (sentemail) {
                  statusEmailDue = false
               }
         }

         testArgs := cmdLineMakeArgs
         if (testArgs == "") { 
               testArgs = testArgsArray[randomValue.Uint32()%uint32(len(testArgsArray))]
         }

         buildVariantStr := ""

         log.Printf("Running %v %v unit test loop: %v of %v (this is test run: %v)\n", 
                          testArgs, buildVariantStr, i+1, testLoops, totalRunNumber)

      
         runTests(testArgs, asanMode, asyncMode, mallocMode ,testTimeout)
         
         // If this was a coverage test update our coverage report
         if strings.HasPrefix(testArgs,"cov") {
             runTests("covrep", 0, 0, 0, testTimeout)
         }
         
         runTests("cleanupTests", 0, 0, 0, testTimeout)
         totalRunNumber++   
      }
   }
}

func processFlags() {
   printHelp := false

   if (cmdLineWorkspace != "" && cmdLineStream != "") {
      log.Printf("Can't set both a workspace and a stream\n");
      printHelp = true
   }
   
   if (cmdLineWorkspace != "" && !useRTC) {
      log.Printf("Can't use a workspace with GHE\n");
      printHelp = true
   }
   
   if (cmdLineGHEKey == "" && !useRTC) {
      log.Printf("Need a path to the key file for logging in to GHE\n");
      printHelp = true
   }

   if (cmdLineAsyncMode < -1 || cmdLineAsyncMode >= ASYNCMODE_NUMMODES) {
      log.Printf("Invalid async mode : %v\n", cmdLineAsyncMode)
      printHelp = true
   } else if (cmdLineAsyncMode >= 0) {
       //Async mode has been set - don't vary it
       AsyncMode_Fixed = true
   }

   if (cmdLineAsanMode < -1 || cmdLineAsanMode >= ASANMODE_NUMMODES) {
      log.Printf("Invalid ASAN mode : %v\n", cmdLineAsanMode)
      printHelp = true
   } else if (cmdLineAsanMode >= 0) {
       //Asan mode has been set - don't vary it
       AsanMode_Fixed = true
   }

   if (cmdLineMallocMode < -1 || cmdLineMallocMode >= MALLOCMODE_NUMMODES) {
      log.Printf("Invalid Malloc mode : %v\n", cmdLineAsanMode)
      printHelp = true
   } else if (cmdLineMallocMode >= 0) {
       //Malloc mode has been set - don't vary it
       MallocMode_Fixed = true
   }

   if (printHelp) {
      flag.PrintDefaults()
      dieError("Invalid args", "exiting")
   }
}

func setupEnv() {
   err := os.Setenv("SROOT", SROOT)
   if err != nil {
      dieError("Setting SROOT", err.Error())
   }

   err = os.Setenv("BROOT", BROOT)
   if err != nil {
      dieError("Setting BROOT", err.Error())
   }

   err = os.Setenv("RTC_SCRIPT_BASE", RTC_SCRIPT_BASE)
   if err != nil {
      dieError("Setting RTC_SCRIPT_BASE", err.Error())
   }

   processIdent = fmt.Sprintf("Looper%v", PID)
   err = os.Setenv("QUALIFY_SHARED", processIdent)
   if err != nil {
     dieError("Setting QUALIFY_SHARED", err.Error())
   }

   err = os.Setenv("STOREROOT", STOREROOT)
   if err != nil {
     dieError("Setting STOREROOT", err.Error())
   }

   err = os.Setenv("IMA_ICU_HOME", IMA_ICU_HOME)
   if err != nil {
      dieError("Setting IMA_ICU_HOME", err.Error())
   }

   err = os.Setenv("IMA_BOOST_HOME", IMA_BOOST_HOME)
   if err != nil {
      dieError("Setting IMA_BOOST_HOME", err.Error())
   }

   err = os.Setenv("IMA_TEST_TEMP_PATH", IMA_TEST_TEMP_PATH)
   if err != nil {
      dieError("Setting IMA_TEST_TEMP_PATH", err.Error())
   }

   err = os.Setenv("IMA_EXTRA_CPPFLAGS",  IMA_EXTRA_CPPFLAGS)
   if err != nil {
      dieError("Setting  IMA_EXTRA_CPPFLAGS", err.Error())
   }
   
   if useRTC {
       rtc_passwd = os.Getenv("RTC_PASSWORD")
       if rtc_passwd == "" {
          dieError("Reading password from RTC_PASSWORD variable", 
                   "Password not set")
       }
   
       rtc_user = os.Getenv("RTC_USER")
       if rtc_user == "" {
         dieError("Reading user from RTC_USER variable",
                "User not set")
       }
   }   
}

func clearDownTrees() {
   if len(SROOT) < 6 {
      dieError("Refusing to remove suspiciously short SROOT", SROOT)
   }
   if len(BROOT) < 6 {
      dieError("Refusing to remove suspiciously short BROOT", BROOT)
   }

   log.Printf("Removing source and output trees\n")

   err := os.RemoveAll(SROOT)

   if err != nil {
      dieError("Deleting SROOT", err.Error())
   }

   err = os.RemoveAll(BROOT)

   if err != nil {
      dieError("Deleting BROOT", err.Error())
   }

   err = os.MkdirAll(SROOT, 0770)
   if err != nil {
      dieError("Creating SROOT", err.Error())
   }

   err = os.MkdirAll(BROOT, 0770)
   if err != nil {
      dieError("Creating BROOT", err.Error())
   }
}

func getSource(buildAttempt int) error {
   if useRTC {
       return getSourceRTC(buildAttempt)
   }
   
   return getSourceGHE(buildAttempt)
}

//This is utterly braindead - we delete our local repo and clone the remote one again
//rather than resetting/updating our local copy - but it is simple to implement (and
//can be improved later)
func getSourceGHE(buildAttempt int) error {
   log.Printf("Creating SROOT %v (attempt %v)\n", SROOT, buildAttempt)

   err := os.MkdirAll(SROOT, 0770)
   
   if err != nil {
      dieError("Creating SROOOT", err.Error())
   }
   
   log.Printf("Cloning %v using key file %v (attempt %v)\n", GHE_URL, cmdLineGHEKey , buildAttempt)
  
   argsSlice := []string{"clone",
                         "--depth", "1", //Only get most recent revision
                         "--single-branch"}
   
   if (cmdLineStream != "") {
        argsSlice = append(argsSlice, "--branch")
        argsSlice = append(argsSlice, cmdLineStream)
   }
   argsSlice = append(argsSlice, GHE_URL)
   argsSlice = append(argsSlice, SROOT)

   cmd := exec.Command("git", argsSlice...)
   
   if cmd.Env == nil {
       cmd.Env = os.Environ()
   }
   //Set the key location
   gitsshcmd := "GIT_SSH_COMMAND=ssh -i "
   gitsshcmd += cmdLineGHEKey
   cmd.Env = append(cmd.Env, gitsshcmd)
       
   err = RunCommand(cmd, "GHE clone", "", 15 * time.Minute, false)

   return err
}

func getSourceRTC(buildAttempt int) error {
   t         := time.Now()
   workspace := cmdLineWorkspace
   
   if workspace == "" {
       workspace = fmt.Sprintf("%s-%v", RTC_WORKSPACE_PREFIX, t.Unix())
   }
   
   log.Printf("Logging into RTC server %s (attempt %v)\n", RTC_SERVER, buildAttempt)

   cmd := exec.Command(RTC_BINARY,
                       "login", "-n", "rtc_server",
                       "-r", RTC_SERVER,
                       "-u", rtc_user,
                       "-P", rtc_passwd,
                       "-c")
   err := RunCommand(cmd, "RTC login", "", 15 * time.Minute, false)

   if (err == nil) {
      if workspace == cmdLineWorkspace {
         log.Printf("Using existing Workspace %s\n", workspace);
      } else {
          stream := RTC_DEFAULT_STREAM

          if (cmdLineStream != "") {
              stream = cmdLineStream
          }
          log.Printf("Creating Workspace  %s on stream %s\n", workspace, stream)
          cmd  = exec.Command(RTC_BINARY,
                              "create", "workspace",workspace,
                              "-r", RTC_SERVER,
                              "-s", stream)
          err = RunCommand(cmd, "RTC workspace creation", "", 15 * time.Minute, false)

          if (err != nil) {
            log.Printf("Failed creating Workspace  %s\n%v\n", workspace, err.Error())
          }
      }
   
      if (err == nil) {
         logstem  := TESTROOTDIR+"/"+strconv.FormatUint(totalRunNumber,10)+
                                "_load_"+strconv.FormatInt(int64(buildAttempt),10)
         log.Printf("Loading Workspace  %s (log %s.stdout/stderr)\n", workspace,logstem)
         cmd  = exec.Command(RTC_BINARY,
                             "load", workspace,
                             "-r", RTC_SERVER,
                             "-u", rtc_user,
                             "-P", rtc_passwd,
                             "-d", SROOT,
                             "--force", "Server",
                             "--allow")
         err = RunCommand(cmd, "RTC workspace load", logstem, 30 * time.Minute, false)

         if workspace != cmdLineWorkspace {
             log.Printf("Deleting Workspace  %s\n", workspace)
             cmd  = exec.Command(RTC_BINARY,
                                 "workspace", "delete",
                                 "-r", RTC_SERVER,
                                 "-u", rtc_user,
                                 "-P", rtc_passwd,
                                 workspace)
             RunCommand(cmd, "RTC workspace delete", "", 15 * time.Minute, false)
         }
      }
      
      log.Printf("Logging out of RTC\n")
      cmd  = exec.Command(RTC_BINARY,
                          "logout",
                          "-r", RTC_SERVER)
      RunCommand(cmd, "RTC logout", "", 15 * time.Minute, false)
   }

   return err
}


func buildProduct(buildAttempt int, asanMode uint32, asyncMode uint32, mallocMode uint32) error {
   logstem  := TESTROOTDIR+"/"+strconv.FormatUint(totalRunNumber,10)+
                    "_build_"+strconv.FormatInt(int64(buildAttempt),10)

   log.Printf("About to build product (Attempt %v) stdout=%s.stdout  stderr=%s.stderr\n", 
                          buildAttempt, logstem, logstem)

   cmd := exec.Command(BUILD_COMMAND)

   if asanMode != ASANMODE_NOASAN {
      log.Printf("Going to use ASAN\n")

      asanEnv := os.Environ()
      asanEnv = append(asanEnv, "GCC_HOME="+SANITIZER_GCC_HOME)
      asanEnv = append(asanEnv, "ASAN_BUILD=yes")

      cmd.Env = asanEnv
   } else {
      log.Printf("Not using ASAN\n")
   }

   if asyncMode == ASYNCMODE_FAKEASYNC || mallocMode == MALLOC_WRAPPER_ENABLED {
       if cmd.Env == nil {
          cmd.Env = os.Environ()
       }
       cflags := "IMA_EXTRA_CFLAGS="
       if asyncMode == ASYNCMODE_FAKEASYNC {
           cflags += "-DUSEFAKE_ASYNC_COMMIT "
           log.Printf("Using Fake Async\n")
       }
       if mallocMode == MALLOC_WRAPPER_ENABLED {
            cflags += "-DCOMMON_MALLOC_WRAPPER "
            log.Printf("Using malloc wrapper\n")
       }
       log.Printf("CFLAGS: " + cflags + "\n")
       cmd.Env = append(cmd.Env, cflags)
       
   }
   
   err := RunCommand(cmd, "build", logstem, 1 * time.Hour,false)
   
   if (err != nil) {
      log.Printf("Error: %v", err.Error())
   }
   return err
}

func runTests(testArgs string, asanMode uint32, asyncMode uint32, mallocMode uint32, testTimeOutHours uint32) {
   logstem  := TESTROOTDIR+"/"+strconv.FormatUint(totalRunNumber,10)+"_tests"
   
   log.Printf("About to run tests... output: %s.stdout/stderr\n", logstem)

   //And run the tests
   cmd := exec.Command(TEST_COMMAND, testArgs)
   cmd.Dir = TEST_DIR

   if (asanMode != ASANMODE_NOASAN) {
       if cmd.Env == nil {
           cmd.Env = os.Environ()
       }
       //At one stage we had to preload an asan library - I don't think this is necessary now
       //ldPreload := SANITIZER_GCC_HOME + "/lib64/libasan.so.2"
       //cmd.Env = append(cmd.Env, "LD_PRELOAD="+ldPreload)
       //log.Printf("Setting LD_PRELOAD to %v\n", ldPreload)
       cmd.Env = append(cmd.Env, "GCC_HOME="+SANITIZER_GCC_HOME)
       cmd.Env = append(cmd.Env, "ASAN_BUILD=yes")

       if (asanMode == ASANMODE_ASAN_NOLEAKCHECK ) {
           cmd.Env = append(cmd.Env, "ASAN_OPTIONS="+ASAN_NOLEAKCHECK_OPTIONS)
       } else {
           cmd.Env = append(cmd.Env, "ASAN_OPTIONS="+ASAN_OPTIONS)
       }
   }

   if asyncMode == ASYNCMODE_DISKPERSISTENCE {
       if cmd.Env == nil {
           cmd.Env = os.Environ()
       }
       cmd.Env = append(cmd.Env, "IMA_TEST_STORE_DISK_PERSIST=true")
       log.Printf("Using real disk persistence\n")
   }
   
   RunCommand(cmd, "tests", logstem, time.Duration(testTimeOutHours) * time.Hour, true)

   //Check there are no core files!!!
   corefiles, err := filepath.Glob(TEST_DIR+"/core*")

   if err != nil {
      dieError("Couldn't do search for core files", err.Error())
   }

   if (corefiles != nil) && (len(corefiles) != 0) {
      dieError("Found core files", strings.Join(corefiles, ", "))
   }


}

func RunCommand(cmd *exec.Cmd, action string, logstem string, timeOutDuration time.Duration, dieOnRunningFailure bool) error {
   if (logstem == "") {
      cmd.Stdout = os.Stdout
      cmd.Stderr = os.Stderr
   } else {
      stdoutf, err := os.OpenFile(logstem+".stdout", os.O_WRONLY|os.O_CREATE, 0660)
      
      if err != nil {
         dieError("Opening stdout file"+logstem+".stdout", err.Error())
      } 
      defer stdoutf.Close()

      stderrf, err := os.OpenFile(logstem+".stderr", os.O_WRONLY|os.O_CREATE, 0660)
      
      if err != nil {
         dieError("Opening stderr file"+logstem+".stderr", err.Error())
      } 
      defer stderrf.Close()

      cmd.Stdout = io.MultiWriter(stdoutf, os.Stdout)
      cmd.Stderr = io.MultiWriter(stderrf, os.Stderr)
   }

   err := cmd.Start()
   if err != nil { 
      dieError("Starting "+action, err.Error())
   }
   var timer *time.Timer = nil
   timedOut := false

   if timeOutDuration > 0 {
      timer = time.AfterFunc(timeOutDuration, func() { timedOut = true; cmd.Process.Kill() })
   }
   err = cmd.Wait()
   
   if timeOutDuration > 0 {
      timer.Stop()
   }

   if timedOut && err == nil  {
      err =  fmt.Errorf("Timed out (timeOut in Minutes: %v)", timeOutDuration/time.Minute)
   }

   if err != nil { 
      if dieOnRunningFailure {
         dieError("Running "+action, err.Error())
      } else {
         log.Printf("Surviving failure %s when running %s\n", err.Error(), action)
      }
   }

   return err 
}

type SmtpTemplateData struct {
   From      string
   To        string
   Subject   string
   Body      string
}



func sendTestStatusEmail(subject string, body string) bool {
   recipients := []string{"changeme@email.com",
                          "changeme@email.com"}

   hostname, err := os.Hostname()
   
   if err != nil {
        hostname = "UNKNOWN HOST"
   }

   from_email := strings.ToLower(rtc_user)
   if from_email == "" {
        from_email = "changeme@email.com"
   }

   var testcfg string
   var lineFormat string = "%-13s %v\n"

   var showAsyncMode string
   if (AsyncMode_Fixed) {
       showAsyncMode = fmt.Sprintf("%v", cmdLineAsyncMode)
   } else {
       showAsyncMode = fmt.Sprintf("%v (Random)", activeAsyncMode)
   }
   
   var showASANMode string
   if (AsanMode_Fixed) {
       showASANMode = fmt.Sprintf("%v", cmdLineAsanMode)
   } else {
      showASANMode = fmt.Sprintf("%v (Random)", activeAsanMode)
   }
   
   var showMallocMode string
   if (MallocMode_Fixed) {
       showMallocMode = fmt.Sprintf("%v", cmdLineMallocMode)
   } else {
      showMallocMode = fmt.Sprintf("%v (Random)", activeMallocMode)
   }

   testcfg = fmt.Sprintf("\n\nTest Configuration\n\n")   
   testcfg = testcfg + fmt.Sprintf(lineFormat, "Command Line:", strings.Join(os.Args, " "))
   testcfg = testcfg + fmt.Sprintf(lineFormat, "Async Mode:", showAsyncMode)
   testcfg = testcfg + fmt.Sprintf(lineFormat, "ASAN Mode:", showASANMode)
   testcfg = testcfg + fmt.Sprintf(lineFormat, "Malloc Mode:", showMallocMode)
   testcfg = testcfg + fmt.Sprintf(lineFormat, "BROOT:", BROOT)
   testcfg = testcfg + fmt.Sprintf(lineFormat, "SROOT:", SROOT)
   
   extendedsubject :=  "["+hostname+"-"+processIdent+"] "+subject

   return sendEmail( recipients,
                     from_email,
                     extendedsubject,
                     body+testcfg)
}

//No attachments yet... here's how though:
//https://gist.github.com/rmulley/6603544
func sendEmail( recipients []string,
                from string,
                subject string,
                body string) bool {
   var err error
   var doc bytes.Buffer

   emailSent := false

   const emailTemplate = `From: {{.From}}
To: {{.To}}
Subject: {{.Subject}}

{{.Body}}
`

   
   context := &SmtpTemplateData{
         from,
         strings.Join(recipients, ", "),
         subject,
         body}

   log.Printf("Sending email with subject %s\n", subject)


   t := template.New("emailTemplate")
   t, err = t.Parse(emailTemplate)
   if err != nil {
      log.Fatalf("error trying to parse mail template: %v\n", err)
   }

   err = t.Execute(&doc, context)
   if err != nil {
      log.Fatalf("error trying to execute mail template: %v\n", err)
   }

   // Connect to the remote SMTP server.
   c, err := smtp.Dial("smtp.server.example.com:25")
   if err != nil {
       log.Print(err)
   } else {
      // Set the sender and recipient.
      c.Mail(context.From)
      for _,recipient := range recipients {
         c.Rcpt(recipient)
      }

      // Send the email body.
      wc, err := c.Data()
      if err != nil {
         log.Print(err)
      }

      if _, err = doc.WriteTo(wc); err != nil {
         log.Print(err)
      }


      err = wc.Close()
      if err != nil {
         log.Print(err)
      }

      emailSent = true


       // Send the QUIT command and close the connection.
      err = c.Quit()
      if err != nil {
         log.Print(err)
      }
   }

   return emailSent
}

type BodyTemplateData struct {
   Action      string
   ErrorText   string
}

func dieError(action string, errorText string) {
   log.Printf("Reporting Error: %s (action was %s)\n", errorText, action)

   var err error
   var doc bytes.Buffer

   const emailBodyTemplate = `
When performing action:
{{.Action}}
The following error was encountered:
{{.ErrorText}}
`

   context := &BodyTemplateData{
         action,
         errorText}

   t := template.New("emailBodyTemplate")
   t, err = t.Parse(emailBodyTemplate)
   if err != nil {
      log.Fatalf("error trying to parse mail body template: %v\n", err)
   }

   err = t.Execute(&doc, context)
   if err != nil {
      log.Fatalf("error trying to execute mail body template: %v\n", err)
   }

   if EMAIL_ON_FAILURE {
       sentEmail := false
    
       for {
          sentEmail = sendTestStatusEmail("Fatal error in unit tests", doc.String())
    
          if sentEmail {
             break
          } else {
             log.Print("Fail to send email.... will sleep for 5 mins and retry")
             time.Sleep(5 * time.Minute)
          }
       }
   }

   os.Exit(10) 
}
