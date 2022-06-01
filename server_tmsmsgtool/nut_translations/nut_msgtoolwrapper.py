# Copyright (c) 2022 Contributors to the Eclipse Foundation
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

import shutil
import glob
import subprocess
import shlex
import datetime

import nut_parse

def runMsgTool(logger, langliststr, replace_filename_vars, input, outputbasedir,
               msgtoolbindir, msgtoolclasspath, msgtoolclass, msgtoolargs):
    """For each lang:
       1) Copy all the input files to {msgtoolbindir}
       2) Run java -cp {msgtoolclasspath}  {msgtoolclassname} {msgtoolargs} 
       3) Copy {msgtoolbindir}/ism.xml to {outputbasedir}"""

    langlist = nut_parse.parseLanguageLists(langliststr)

    for lang in langlist:
        #Copy all the inputs to the msgtoolbindir
        for input_arg in input:
            if replace_filename_vars:
                input_varsreplaced  = nut_parse.parseFileNameForLangVars(input_arg, lang)

    
            try:
                input_list = glob.glob(input_varsreplaced)

                for inputfile in input_list:
                    timestr = datetime.datetime.now().strftime("%H:%M:%S on %d %B %Y")
                    logger.info("Copy "+inputfile+" to msgtool working dir: "+msgtoolbindir+" at "+timestr)
                    shutil.copy(inputfile, msgtoolbindir)
            except Exception as e:
                logger.error("Failed to copy file: %s to %s - error %s" %
                             (inputfile, msgtoolbindir, e))
                raise e 

        #Run the msgtool
        extraargs = msgtoolargs.strip()
        if extraargs.startswith('"'):
            extraargs = extraargs.strip('"')

        passedinargs = shlex.split(extraargs)
        msgtoolarglist = ['java', '-cp', msgtoolclasspath, msgtoolclass] + passedinargs

        try:
            output = subprocess.run(msgtoolarglist, 
                                    cwd=msgtoolbindir,
                                    capture_output=True)
        except Exception as e:
            logger.error("Failed to run %s - error %s" %
                             (shlex.join(msgtoolarglist), e))
            raise e

        logger.info("Output from running: "+shlex.join(msgtoolarglist)+" is: "+output.stdout.decode('utf-8')+" and stderr: "+output.stderr.decode('utf-8'))

        #Copy msgtoolbindir/ism.xml to outputbasedir
        outputbasedir_varsreplaced = nut_parse.parseFileNameForLangVars(outputbasedir, lang)
        try:
            filetocopy = msgtoolbindir+'/ism.xml'
            timestr = datetime.datetime.now().strftime("%H:%M:S on %d %B %Y")
            logger.info("Copy "+filetocopy+" to output dir: "+outputbasedir_varsreplaced+" at "+timestr)
            shutil.copy(filetocopy, outputbasedir_varsreplaced)
        except Exception as e:
            logger.error("Failed to copy file: %s to %s - error %s" %
                             (inputfile, msgtoolbindir, e))
            raise e 

