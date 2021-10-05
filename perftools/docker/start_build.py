#!/usr/bin/python
# Copyright (c) 2021 Contributors to the Eclipse Foundation
# 
# See the NOTICE file(s) distributed with this work for additional
# information regarding copyright ownership.
# 
# This program and the accompanying materials are made available under the
# terms of the Eclipse Public License 2.0 which is available at
# http://www.eclipse.org/legal/epl-2.0
# 
# SPDX-License-Identifier: EPL-2.0
#

"""
Script to invoke auto_build.py
"""

import auto_build
import build_msserver
import datetime
import cStringIO
import sys
import os
import shutil
import errno
import smtplib
import signal
import pwd
import traceback
import time

########################################################################
# Global variables

build_timestamp=datetime.datetime.utcnow()

#save this script output to a stream, so we can publish stdout to logs
start_build_output=cStringIO.StringIO()

start_foreground = False

email_notify_addrs = []

email_failed_subject = "Build %s Failed"
email_from="builder@seed"

email_smtp="smtp.sendgrid.net"
#use '/bin/sudo -S -i  -u tester' to fully become tester user
start_tests_cmd=['/bin/sudo','-S','-i','-u','tester','/home/tester/tests/bin/start_tests.rb','--build_id='+auto_build.src_latest]

#########################################################################
# Exception class
class start_build(Exception):
    def __init__(self,msg):
        self.msg=msg

#########################################################################
# Function: publish build logs
def publish_logs(log_output=None,pub_file=None):

    """
    publish the build logs
    """

    if log_output == None or pub_file == None:
        raise start_build("publish_logs: args invalid")

    try:
        #create directory minus filename
        dist_path,logfile=os.path.split(pub_file)
        os.makedirs(dist_path)
        start_build_output.write("Creating directory: %s\n" % dist_path)

    except OSError as e:
        if e.errno == errno.EEXIST:
            start_build_output.write("Warning: build logs directory already exist. Continuing\n")
        else:
            raise e

    try:
        with open(pub_file,'wb') as f:
            log_output.seek(0)
            f.flush()
            shutil.copyfileobj(log_output,f)
	if start_foreground:
	   print("start_output: %s" % log_output.getvalue())	
    except OSError as e:
        if e.errno == errno.EEXIST:
            raise build_start("Error: publish copy failed %s\n" % msg)
        else:
            raise e
    if start_foreground:
        print("start_build: published logs in %s\n" % pub_file)

#######################################################################
# Function: send email notifications
def notify_error():

    """
    notify interested parties on build failure
    """

    
    sendgrid = smtplib.SMTP(email_smtp,587)
    sendgrid.ehlo()
    sendgrid.starttls()
    sendgrid.ehlo()
    sendgrid.login("builder",builderpass)

    email_header = 'To:' + ';'.join(email_notify_addrs) + '\n' + 'From: '+email_from + '\n'+'Subject:'+email_failed_subject % auto_build.src_latest + '\n'

    email_body="\nBuild %s has failed. Logs are located in %s\n" % (auto_build.src_latest,auto_build.seed_publish+"/"+auto_build.src_latest)

    email_msg=email_header+email_body

    sendgrid.sendmail(email_from,email_notify_addrs,email_msg)
    sendgrid.close()

#######################################################################
# startup automated tests
def start_tests():
    
    #startup tests
    start_build_output.write("Info: start_build_output: kicking off automated tests\n")
    #sudo to tester and run its start_tests
#    start_build_output.write("start_tests rc: %s" % os.execl('/bin/sudo','/bin/sudo','-u','tester','sh','-c','/home/tester/tests/bin/start_tests.rb','--build_id='+auto_build.src_latest))

    start_build_output.write("start_tests command output: %s" % build_msserver.run_command(start_tests_cmd))
#    time.sleep(5)


#######################################################################
# Main:
def main(argv=None):


    if argv is None:
        argv=sys.argv

    #initialize module rc to successful
    rc=0

    try:
        start_build_output.write("Starting build at %s\n" % build_timestamp)

        # capture/save stdout,stderr of auto_build for build_logs repository
        old_stdout=sys.stdout
        old_stderr=sys.stderr
    
        build_output=cStringIO.StringIO()
        sys.stdout=build_output
        sys.stderr=build_output

        #invoke build. stdout from auto_build and build_msserver modules
        # will be captured into output_stream
        rc = auto_build.main()

        #reset stdout,stderr
        sys.stdout=old_stdout
        sys.stderr=old_stderr

    except Exception,e:
        #catch all excpetions
        start_build_output.write("Error: auto_build General exception: %s" % e)
        type_, value_, traceback_ = sys.exc_info()
        start_build_output.write("%s \n%s \n%s \n" % ('\n'.join(traceback.format_tb(traceback_)),value_,type_))
        #set rc to non-zero for failure
        rc=1

    #setup path/filenames to publish build logs
    build_logs=auto_build.seed_publish+"/"+auto_build.src_latest+"/build_logs"
    auto_build_log=build_logs+"/auto_build.log"
    start_build_log=build_logs+"/start_build.log"

    try:
        # uncommet if no build logs present print("output: %s" % output_stream.getvalue())
        publish_logs(build_output,auto_build_log)

    except publish_logs, msg:
        start_build_output.write("Error: publish_logs occured\n")
    except Exception,e:
        start_build_output.write("General Exception:build_output: publish_logs occured\n")
        type_, value_, traceback_ = sys.exc_info()
        start_build_output.write("%s \n%s \n%s \n" % ('\n'.join(traceback.format_tb(traceback_)),value_,type_))



    if rc:
        #build did not complete
        start_build_output.write("Error: auto_build returned error. Examine auto_build.log for details\n")
        notify_error()
    else:
        try:
            # capture/save stdout,stderr of auto tests
            old_stdout=sys.stdout
            old_stderr=sys.stderr
            

            sys.stdout=start_build_output
            sys.stderr=start_build_output

            start_tests()
            #reset stdout,stderr
            sys.stdout=old_stdout
            sys.stderr=old_stderr

        except Exception,e:
            start_build_output.write("General Exception: start_tests error occured\n")
            type_, value_, traceback_ = sys.exc_info()
            start_build_output.write("%s \n%s \n%s \n" % ('\n'.join(traceback.format_tb(traceback_)),value_,type_))
            #we failed
            rc=1


    start_build_output.write("start_build: Done with rc: %s\n" % rc)
    
    try:
        #save our output also
        publish_logs(start_build_output,start_build_log)
    except publish_logs, msg:
        print("Error: publish_logs occured\n")
    except Exception,e:
        print("General Exception: start_build_output: publish_logs occured\n")
        type_, value_, traceback_ = sys.exc_info()
        print("%s \n%s \n%s \n" % ('\n'.join(traceback.format_tb(traceback_)),value_,type_))


    return rc

##########################################################
# standalone main    

if __name__ == '__main__':
    print("start_build: invoke as standalone\n\n")


    #for standalone just check for --foreground
    for arg in sys.argv:
        if arg == "--foreground":
            start_foreground=True
            
    if not start_foreground:
        print("Forking as daemon. Build executing as child process")
        try:
            # Fork a child process so the parent can exit and caller can continue
            pid = os.fork()
        except OSError, e:
            raise Exception, "%s [%d]" % (e.strerror, e.errno)

        if (pid == 0):
            # I am  child.
            os.setsid()
            
            try:
                #close parent's stdio fd's. Using sys.std(out|in|err).close() does not
                #fully close the fd's for some reason. Use os.close() C call instead
                #
                #Closing standard io's are neccessary for 'ssh' invocation of this script
                #which uses fork(). Seems 'ssh' hangs due to waiting on fd's that are used
                #by the tty. Therefore, below closes allow immediate control (ie async) back
                #to shell
                new_stdout=os.dup(1)
                new_stderr=os.dup(2)

                os.close(0)
                os.close(1)
                os.close(2)

                sys.stdout = os.fdopen(new_stdout,'w',0)
                sys.stderr = os.fdopen(new_stderr,'w',0)
            except OSError:
                pass

            #invoke main as child
            sys.exit(main())
        else:
            #ignore child's exit signal
            signal.signal(signal.SIGCHLD,signal.SIG_IGN)

            print("I am parent so exiting!")

            try:
                #close me/parent stdio fd's
                os.close(0)
                os.close(1)
                os.close(2)
            except OSError:
                pass

            os._exit(0)
    else:
        rc=main()
        print("start_build rc: %s  output: %s" % (rc,start_build_output.getvalue()))
        sys.exit(rc)
