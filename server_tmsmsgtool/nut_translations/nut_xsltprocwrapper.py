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

import nut_utils

def runXSLTProc(logger, langliststr, replace_filename_vars, input, output,
                   xsl, extraargs):
    """For each lang:
        Run xsltproc [extraargs] -output {output}  {xsl} {inputfiles} """

    langlist = nut_utils.parseLanguageLists(langliststr)

    for lang in langlist:
        input_args = []

        for input_arg in input:
            if replace_filename_vars:
                input_varsreplaced  = nut_utils.parseFileNameForLangVars(input_arg, lang)
                input_args.append(input_varsreplaced)
            else:
                input_args.append(input)

        output_args = ['--output']

        if replace_filename_vars:
            output_args.append(nut_utils.parseFileNameForLangVars(output, lang))
        else:
            output_args.append(output)
        
        extraargs = extraargs.strip()
        if extraargs.startswith('"'):
            extraargs = extraargs.strip('"')

        passedinargs = shlex.split(extraargs)

        xsl_args = [xsl]

        xsltprocarglist = ['xsltproc'] + passedinargs + output_args + xsl_args + input_args

        try:
            procoutput = subprocess.run(xsltprocarglist, 
                                    capture_output=True)
        except Exception as e:
            logger.error("Failed to run %s - error %s" %
                             (shlex.join(xsltprocarglist), e))
            raise e

        logger.info("Output from running: "+shlex.join(xsltprocarglist)+" is: "+procoutput.stdout.decode('utf-8')+" and stderr: "+procoutput.stderr.decode('utf-8'))
