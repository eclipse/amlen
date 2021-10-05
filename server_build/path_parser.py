#!/usr/bin/python3
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


#This script can be run standalone and controlled by command line vars
#or imported and parseString() used.

import argparse
import os
import pathlib
import logging
import logging.handlers

#By default we'll look in the same directory as this script for the file called:
defaultPathVarsFile='paths.properties'
LOG_FILENAME = "/tmp/path_parser.log"

logger = logging.getLogger("path_parser")
logger.setLevel(logging.DEBUG)

def findPathVarsFile():   
    filepath = os.getenv('IMA_PATH_PROPERTIES')

    if filepath is None:
        filepath = os.path.join(os.path.dirname(os.path.realpath(__file__)),defaultPathVarsFile)

    logger.info("Using file {} as path properties".format(filepath))

    return filepath

def parsePathVars(varsfname):

    with open(varsfname) as fp:
        pathVarsDict = {}

        for cnt, line in enumerate(fp):
            logger.debug("Read line {} as {}".format(cnt,line))
            stripline = line.strip()

            if len(stripline) >0 and stripline[0] != '#':
                splitline = stripline.split('=', 1)
            
                if len(splitline) == 2:
                    key = splitline[0]
                    val = splitline[1]
                    
                    #If the val is surround by double quotes, remove them
                    trimmedval=val.strip()
                    if trimmedval[0] == '"' and trimmedval[-1] == '"':
                        val = trimmedval[1:-1]

                    logger.debug("Path var {}={}".format(key,val))
                    pathVarsDict[key] = val

    return pathVarsDict
    
def generate_defines(pathvars, var_prefixlist):
    definestr = ""

    for key in pathvars:
        matchedvar = False
        
        if var_prefixlist is None:
            matchedvar = True
        else:
            for var_prefix in var_prefixlist:
                if key.startswith(var_prefix):
                    matchedvar = True
                    break #We don't need to check more prefixes

        if matchedvar: 
            if definestr != "":
                definestr += " "

            #We want the quotes around the variable to be escaped so the shell
            #doesn't eat them and they look like a string constant to the compiler
            definestr += '"-D{}=\\"{}\\""'.format(key, pathvars[key])

    print(definestr)
                
def output_value(pathvars, var_prefixlist):
    var_prefix=var_prefixlist[0] #already checked there is exactly one prefix
    for key in pathvars:
        if key.startswith(var_prefix):
            print(pathvars[key])
            #Can only print a single value in this mode - we're done
            break

def file_replace(pathvars, inputfilename, outputfilename):

    logger.debug("Going to process {} to {}".format(inputfilename, outputfilename))
    tmpoutfilename = outputfilename+".pptmp"
    
    pathlib.Path(os.path.dirname(tmpoutfilename)).mkdir(parents=True, exist_ok=True)
    
    lineschanged = 0

    try:
        with open(inputfilename, encoding='utf-8') as infp:
            with open(tmpoutfilename, "w", encoding='utf8') as outfp:
                for linenum, inline in enumerate(infp):
                    currline = inline

                    for key in pathvars:
                        targetstring="${"+key+"}"

                        alteredline = currline.replace(targetstring, pathvars[key])

                        if currline != alteredline:
                            lineschanged += 1 
                            logger.debug("Replaced {}  at {} line {}".format(
                                          key, inputfilename, linenum))

                        currline = alteredline

                    outfp.write(currline)

        #Now we've successfully written the processed file with a temporary name..
        #..atomically move it to the outfilename
        os.rename(tmpoutfilename, outputfilename)
    except UnicodeDecodeError as e:
        logger.warning("Skipping "+inputfilename+" as it doesn't look like a utf-8 text file")
        os.unlink(tmpoutfilename)
        lineschanged = -1
        
    
    return lineschanged
    
    

def dir_replace(pathvars, inputdir, outputdir):
    lineschanged = 0
    fileschanged = 0
    totalfiles   = 0
    
    for (root, dirs, files) in os.walk(inputdir):
        for fname in files:
            infile = os.path.join(root, fname)

            inpath = pathlib.PurePosixPath(infile)
            relativeinpath = inpath.relative_to(inputdir)

            outdirpath = pathlib.PurePosixPath(outputdir)
            outfilepath = outdirpath.joinpath(relativeinpath)

            filelineschanged = file_replace(pathvars, infile, str(outfilepath))
            
            if filelineschanged > 0:
                lineschanged += filelineschanged
                fileschanged += 1

            totalfiles += 1
 
    logger.info("Changed {} lines in {} files (out of {} files processed)".format(lineschanged, fileschanged, totalfiles))

#This function is a little different from the rest of this file, normally the parsing is doine
def parseString(instring):
    """This function is a little different from the rest of this file, normally the parsing is done by running this script standalone
       but this function allows the script to be imported and run by other python programs on the supplied string."""
    varsfname = findPathVarsFile()
    pathvars = parsePathVars(varsfname)

    currstring = instring

    for key in pathvars:
        targetstring="${"+key+"}"

        currstring = currstring.replace(targetstring, pathvars[key])

    return currstring

def parseArguments():
    parser = argparse.ArgumentParser()
    
    args_ok = True
    
    parser.add_argument('-p', '--prefix', required=False, action='append', help="Only variables that match this prefix are included")
    parser.add_argument('-m', '--mode', type=str, default='CFLAGS', help="Output format", choices=['CFLAGS','value','filereplace','dirreplace'])
    parser.add_argument('-i', '--input', required=False, help="Source file (in filereplace mode) Source directory in dirreplace")
    parser.add_argument('-o', '--output', required=False, help="Destination file (in filereplace mode) Destination directory in dirreplace")
    parser.add_argument('-s', '--screenloglevel', required=False, help="Log to the screen", choices=['error','warning','info','debug'])
    parser.add_argument('-l', '--fileloglevel', required=False, help="Log level to log file", choices=['error','warning','info','debug'])    
    
    args, unknown = parser.parse_known_args()

    if args.screenloglevel:
        consoleHandler = logging.StreamHandler()
        
        if args.screenloglevel == 'error':
            consoleHandler.setLevel(logging.INFO)
        elif args.screenloglevel == 'warning':
            consoleHandler.setLevel(logging.WARN)
        elif args.screenloglevel == 'info':
            consoleHandler.setLevel(logging.INFO)
        else:
            consoleHandler.setLevel(logging.DEBUG)

        consoleformatter = logging.Formatter('%(asctime)-25s %(levelname)-8s %(message)s')
        consoleHandler.setFormatter(consoleformatter)
        logger.addHandler(consoleHandler)

    if args.fileloglevel:
        logHandler = logging.handlers.RotatingFileHandler(LOG_FILENAME, maxBytes=10485760, backupCount=1) # max size of 10Mb and 1 backup

        
        if args.fileloglevel == 'error':
            logHandler.setLevel(logging.INFO)
        elif args.screenloglevel == 'warning':
            logHandler.setLevel(logging.WARN)
        elif args.screenloglevel == 'info':
            logHandler.setLevel(logging.INFO)
        else:
            logHandler.setLevel(logging.DEBUG)
            
        logfileformatter = logging.Formatter('%(asctime)-25s %(name)-30s ' + ' %(levelname)-8s %(message)s')
        logHandler.setFormatter(logfileformatter)
        logger.addHandler(logHandler)


    if unknown:
        for badarg in unknown:
            logger.error("Unknown arg: "+badarg)
        args_ok = False

    if args.mode == 'value':
        if not args.prefix:
            logger.error("-p <prefix> must be set  in value mode to select the value to print")
            args_ok = False
        elif len(args.prefix) != 1:
            logger.error("-p <prefix> must be set only once in value mode to select the value to print")
            args_ok = False
    elif args.mode == 'filereplace' or args.mode == 'dirreplace':
        if not args.input:
            logger.error("-i <input> must be set in {} mode".format(args.mode))
            args_ok = False
        if not args.output:
            logger.error("-i <output> must be set in {} mode".format(args.mode))
            args_ok = False
    

    if not args_ok:
        logger.fatal("Invalid arguments - exiting")
        exit(10)

    return args


if __name__ == "__main__":
    
    args = parseArguments()
    
    varsfname = findPathVarsFile()
    pathvars = parsePathVars(varsfname)

    if args.mode == 'CFLAGS':
        generate_defines(pathvars, args.prefix)
    elif args.mode == 'value':
        output_value(pathvars, args.prefix)
    elif args.mode == 'filereplace':
        file_replace(pathvars, args.input, args.output)
    elif args.mode == 'dirreplace':
        dir_replace(pathvars, args.input, args.output)
    else:
        print("Mode not known: "+args.mode)
        exit(10)

    exit(0)
