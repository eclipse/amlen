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
    This script is used to build docker msserver.

"""

import sys
import os
import rpm
#import requests
import json
import time
import subprocess
import zipfile
import glob
import datetime
import tarfile
import re
import shutil

if sys.version_info[0] == 3:
    import configparser
    config = configparser.RawConfigParser()
else:
    import ConfigParser
    config = ConfigParser.ConfigParser()

############################################################################
# Section: Global variables.

#current action
current_action=None

#config action vars
os_type=""
work_path=""
temp_file=""
src_path=""
src_type=""
src_rel=""
src_build=""
src_type=""
build_type=""
config_type=""
clean_all=False

#build action vars
build_target=None

#global setup settings
workspace_src=""
workspace_ship=""

# default arg settings
def_build_latest="latest"
def_rel="CURREL"
def_ostype="centos:latest"
def_clean_all=False

#setup action args dictionary
cl_args = {'clean':{'all':def_clean_all,},'config':{'os_type':def_ostype,'src_type':'ima_pub','src_rel':def_rel,'src_build':def_build_latest,},'build':{'tags':None,},}

#build_date=lambda build_date: datetime.datetime.strptime(build_date,'%Y%m%d-%H%M')
build_datetime=datetime.datetime.utcnow().strftime('%Y%m%d-%H%M')

#setup template macro and filename dictionaries
macro_template={'%OS_BUILD%':"",'%WORKSPACE_DIR%':"",'%ROOT_DIR%':""}
macro_rootdir=""

#setup docker cmd dictionary
docker_cmds ={ 'containers':["docker","ps","-aq"],
	       'images':["docker","images","-q"],
	       'remove_container':["docker","rm"],
	       'remove_image':["docker","rmi"],
	       'load_image':["docker","load","-i"],
	       'stop_container':["docker","stop"],
               'build':["docker","build","-f","%DOCKERFILE%","-t","%TAG%","%BUILDPATH%"],
               'save':["docker","save","-o","%DOCKERTAR%","%TAG%"],
               'tagit':["docker","tag"],}

#default docker strings,macros. default will combine with build_target
docker_tag_suffix="-svtperf"

macro_docker_cmds={ "%DOCKERFILE%":"",
                    "%TAG%":"",
                    "%BUILDPATH%":"",
                    "%DOCKERTAR%":"",}



#setup template file output
build_filename=""

#setup known build tree path nodes
build_root="release"
build_prod="production"
build_dev="development"
build_ship="appliance"
build_test="test"
build_perftools="/pvt/pvt_perftools"
      

#setup file patterns

#docker build file patterns
docker_build_glbpat="Docker-*.build"

#target patterns
target_suffix="-SVTPERF-DockerImage.tar"
target_glbpat='*'+target_suffix


"""
msserver patterns

NOTE: change these patterns when file sources change
"""
# not using zip pat
# msserver_zip_pat="msserver-*-docker.zip"
msserver_zip_globpat=""
# not using tar pat for now
#msserver_tar_pat="msserver-*-dockerImage.tar"
msserver_tar_globpat=""
msserver_rpm_regxpat=r'.*IBMWIoTPMessageGatewayServer-(.*)x86_64\.rpm'
msserver_rpm_globpat='*IBMWIoTPMessageGatewayServer-*x86_64.rpm'
msserver_env_regxpat='.*imaserver-docker\.env'
msserver_env_globpat='*imaserver-docker.env'
msserver_targz_globpat='IBMWIoTPMessageGatewayServer-*tz'
msserver_bin_regxpat=r'.*IBMWIoTPMessageGatewayServer'
msserver_src_regxpat=r'.*IBMWIoTPMessageGatewayServer(.*)\.src\.rpm'

"""
mswebui patterns

NOTE: change these patterns when file sources change
"""

# not using zip pat
#mswebui_zip_pat="mswebui-*-docker.zip"
mswebui_zip_globpat=""
# not using tar pat
#mswebui_tar_pat="mswebui-*-dockerImage.tar"
mswebui_tar_globpat=""
mswebui_rpm_regxpat=r'.*IBMWIoTPMessageGatewayWebUI-(.*)x86_64\.rpm'
mswebui_rpm_globpat='*IBMWIoTPMessageGatewayWebUI-*x86_64.rpm'
mswebui_env_regxpat='.*imawebui-docker\.env'
mswebui_env_globpat='*imawebui-docker.env'
mswebui_targz_globpat='*IBMWIoTPMessageGatewayWebUI-*.tz'
mswebui_bin_regxpat=r'.*IBMWIoTPMessageGatewayWebUI'
mswebui_src_regxpat=r'.*IBMWIoTPMessageGatewayWebUI(.*)\.src\.rpm'

"""
setup file patterns dictionary for server and mswebui

NOTE: if target changes modify key value below
"""

server_targets={ 'imaserver':{'zip_glbpat':msserver_zip_globpat,
                           'tar_glbpat':msserver_tar_globpat,
                           'rpm_rgxpat':msserver_rpm_regxpat,
                           'rpm_glbpat':msserver_rpm_globpat,
                           'env_rgxpat':msserver_env_regxpat,
                           'env_glbpat':msserver_env_globpat,
                           'tz_glbpat':msserver_targz_globpat,
                           'bin_rgxpat':msserver_bin_regxpat,
                           'src_rgxpat':msserver_src_regxpat},
	       'imawebui':{'zip_glbpat':mswebui_zip_globpat,
                          'tar_glbpat':mswebui_tar_globpat,
                          'rpm_rgxpat':mswebui_rpm_regxpat,
                          'rpm_glbpat':mswebui_rpm_globpat,
                          'env_rgxpat':mswebui_env_regxpat,
                          'env_glbpat':mswebui_env_globpat,
                          'tz_glbpat':mswebui_targz_globpat,
                          'bin_rgxpat':mswebui_bin_regxpat,
                          'src_rgxpat':mswebui_src_regxpat,}}


###########################################################################
# Exception class
class build(Exception):
    def __init__(self,msg):
        self.msg=msg

############################################################################
# Section: Usage

def usage():
    """
    Display the usage for this script.  The usage itself will be dependent
    upon the action which is currently being executed.
    """

    print("""
Usage: build_msserver.py [action][action options]

Actions: build config clean help

Action Options:
    help                          Show this help message and exit
    config:
         --os_type=TYPE           OS used to build docker image. 
                                  Default "centos:latest"

         --workspace=PATH         Path to a working workspace directory 
                                  where build artifacts will reside

                                  Required

         --template=FILE          Full path to template file containing 
                                  Docker build commands. Parses template and
                                  generates 'Docker-<type>.build' file
                                  
                                  Optional

         --type=[msserver|mswebui] Type of server to config

                                  Required

         --src=PATH               Top IMA publishing directory or direct path to
                                  the source zip,tars or rpm's
                                  
                                  Required

         --src_type=[ima_pub|direct] Type of source path. 

                                  'ima_pub' indicates top of IMA publish tree 
                                  (ie ../release/CURREL/production/latest). 

                                  'direct' indicates direct path to 
                                  rpm,tar, or zip sources

                                  Keywords: 'ima_pub', 'direct'
    

         --src_rel=RELEASE        Source product release. 

                                  Valid only for 'ima_pub' src_type and required.

                                  Default: 'CURREL'

         --src_build=LABEL        Build label to retrive sources. 

                                  Valid only for 'ima_pub' src_type
                                
                                  Default: list available, prompt user

                                  Keywords: 'latest' grab from 'latest' link (Default)
                                            'latest_dated' grab latest dated
                                            'ask' choose from latest build
                                                 
                                           

         --build_type=[dev|prod]  Source build type. 

                                  Valid only for 'ima_pub' type

                                  Default: 'prod'

         --root_dir=PATH          Root directory path for %ROOT_DIR% docker template macro
                                  
                                  Optional

    build:

         --target=[imaserver|imawebui] Build docker container against generated 
                                     'Docker-<target>.build' file

                                  Required

         --workspace=PATH         Top directory where 'src' and 'ship were created 
                                  by 'config' action

                                  Required

         --tags                   Tag names given to image file. To support mutliple tag names
                                  use '|' seperator 

                                  Example: 'svtperf:20140501-0890|svtperf:fixpack1|svtperf:latest')

                                  The repository name must the exactly same string for all tags. Otherwise,
                                  build will fail with error when parsing tag string.

                                  Default: svtperf

    clean:
         --workspace=PATH         Workspace path to cleanup

                                  Required

         --all                    Remove rpm,Dockerfile,docker tar


""")
    raise build("")

def process_help():
    """
    Check to see if the help command line option has been added and if so
    we want to display the help text.
    """

    if "help" in cl_args:
        usage()
        raise build("")

############################################################################
# Section: prompt user for variuos inputs

def ask_build(path, prompt="Retrieve Build> "):
    """ 
    Ask user for build name
    """

    while 1: 
       print("Choose build from list (do not include single quotes):\n")
       build_list=retrieve_builds(path)
       print("%s" % build_list)

       # Prompt for the build.
       if prompt != "":
          if sys.version_info[0] == 3:
              build = input(prompt + " ")
          else:
              build = raw_input(prompt + " ")
       if build in build_list:
	  break

    return build


############################################################################
# Section: Parameter Management

def process_argv(args=None):
    """
    Process arguments which have been passed on the command line,
    """

    global cl_args
    global current_action
    temp_dict={}

#    if not len(sys.argv[1:]):
    if not len(args) or len(args) == 1:
	usage()

    print("processing input args: %s" % args)
#    for arg in sys.argv[1:]:
    for arg in args[1:]:


        # we only allow one action at one time
        if current_action != None  and arg[:2] != "--":
            print("Error: invalid options were supplied!")
            
        if '=' in arg:
            #we have an action option
            (name, value) = arg[2:].split('=', 1)
            print("action: %s name:%s value:%s" % (current_action,name,value))
         
            temp_dict[name]=str(value)
        elif arg[:2] == "--":
            print("boolean option: %s" % arg)
        else:
            #we have an action
            print("found action:%s" % arg)
            if current_action==None:
                current_action=arg
            else:
                print("Error: only one action allowed")
                raise build("Error: only one action allowed")

    #at this point setup cl_args since we now should have current_action

    for arg,value in temp_dict.items():

        if arg[:2] == "--":
            print("boolean option: %s" % arg)
            cl_args[current_action][arg]=None
        else:
            cl_args[current_action][arg]=str(value)

        

def retrieve_builds(path=None,remove_nondated=False):
    """
    Get and return a sorted list list of available builds
    """

    output = os.listdir(path)
    non_dated=[]

    #remove known non-dated builds from list at this point 
    #so we can sort build base
    #on date stamps
    #'latest' points to latest build so remove before sorting
    #add additional below, non-dated builds to remove:
    for timestamp_dir in output[:]:
        try:
            is_valid=datetime.datetime.strptime(timestamp_dir,'%Y%m%d-%H%M')
        except ValueError:
            print("Found non-dated/invalid directory:%s Removing from build list" % timestamp_dir)
            output.remove(timestamp_dir)
            #save non dated to restore later
            non_dated.append(timestamp_dir)


    # sorting using build timestamp format '20150331-2001'

    sorted_output = sorted(output,key=lambda build_date: datetime.datetime.strptime(build_date, '%Y%m%d-%H%M'))

    #add back removed known non-dated builds, if requested:
    if not remove_nondated:
        for directory in non_dated:
            sorted_output.append(directory)

    return sorted_output

######################################################################
# Section: prompt user for variuos inputs
def ask_build(path, prompt="Retrieve Build> "):
    """ 
    Ask user for build name
    """

    while 1: 
       print("Choose build from list (do not include single quotes):\n")
       build_list=retrieve_builds(path)
       print("%s" % build_list)

       # Prompt for the build.
       if prompt != "":
          if sys.version_info[0] == 3:
              build = input(prompt + " ")
          else:
              build = raw_input(prompt + " ")
       if build in build_list:
	  break

    return build

def build_options():
    """
    From arg inputs, validate and build neccessary options
    """
    #
    #declare global var usage
    #

    #config action vars
    global src_path
    global src_rel
    global src_build
    global src_type
    global os_type
    global work_path
    global temp_file
    global build_type

    # build action vars
    global build_target

    #clean action vars
    global clean_all

    #general vars
    global cl_args
    global current_action


    #workspace directories

    global macro_template
    global macro_rootdir
    global macro_docker_cmds

    # global runtime strings
    global workspace_src
    global workspace_ship
    global build_filename

    # validate options exist
    if current_action == "help" or len(cl_args) == 1:
	usage()

    for option,value in cl_args[current_action].iteritems():

        print("Processing action:%s with options:%s" % (current_action,(option,value)))
	if current_action == "config":
                #loop thru config options
                if option == "os_type":
                    os_type=value
                elif option == "workspace":
                    work_path=value
                elif option == "template":
                    temp_file=value
                elif option == "src":
                    src_path=value
                elif option == "src_type":
                    if value == "ima_pub":
                        if "build_type" not in cl_args[current_action]:
                            print("Error:'ima_pub' source type requires 'src_rel' options")
                            raise build("Error:'ima_pub' source type requires 'src_rel' options")
                        else:
                            src_type=value
                    elif value == "direct":
                        src_type=value
                    else:
                        print("Error: Unknown src_type:'%s' Only 'ima_pub' and 'direct' supported" % value)
                        raise build("Error: Unknown src_type: '%s' Only 'ima_pub' and 'direct' support" % value)
                elif option == "src_rel":
                    src_rel=value
                elif option == "src_build":
                    if not value == "latest" and not value == "latest_dated" and not value == "ask":
                        print("Error: Unknown source build option %s" % value)
                        raise build("Error: Unknown source build option %s" % value)

                    else:
                        src_build=value
                elif option == "build_type":
                    if value == "prod":
                        build_type=build_prod
                    elif value == "dev":
                        build_type=build_dev
                    else:
                        print("Error: Unknown build_type:%s Only 'dev','prod' supported",value)
		#
                #setup type of build to configure
		#	
		#NOTE: change/add type names below of new/changed values
		#
                elif option == "type":
                    if value == "imaserver" or value == "imawebui":
                        build_target=value
                elif option == "root_dir":
                    macro_rootdir=value
                else:
                    print("Error: Unknown config option: %s" % option)
                    raise build("Error: Unknown config option: %s" % option)
                    
	elif current_action == "build":
                if "workspace" and "target" not in cl_args[current_action]:
                    print("Error: build action requires 'workspace' and 'target' options")
                    raise build("Error: build action requires 'workspace' and 'target options")
		#
                #check thru build options
		#	
		#NOTE: change/add target names below of new/changed values
		# 
                if option == "target":
                    if value != "imaserver" and value != "imawebui":
                        print ("Error: unknown build target: %s" % value)
                    else:
                        build_target=value
                elif option == "workspace":
                    if not os.path.exists(value):
                        print ("Error: %s workspace path  does not exist" % value)
                    else:
                        work_path=value
                elif option == "tags":
                    tag_value=value
                    if not value:
                        raise build("Error: tag value empty")
                else:
                    print("Error: unknown build option: %s" % option)
                    raise build("Error: unknown build option: %s" % option)

        elif current_action == "clean":
                #check thru clean options
                if option == "all":
                    if "workspace" not in cl_args[current_action]:
                        print("Error: action clean requires workspace")
                        raise build("Error: action clean requires workspace")
                    else:
                        clean_all=True
                elif option == "workspace":
                    work_path=value
                else:
                    print("Error: unknown clean option: %s" % option)
                    raise build("Error: unknown clean option: %s" %option)
        else:
            print("Error: unknown action: %s" % option)
            raise build("Error: unknown action: %s" %  option)

    #initialize runtime global strings and dictionaries

    workspace_src=work_path + "/src"
    workspace_ship=work_path + "/ship"

    #action initialization 

    if current_action == "build" or current_action == "config":
        #setup common initializations for 'build' and 'config'

        #setup template file output,input for 'config' and 'build'
        build_filename="Docker-"+build_target+".build"

    if current_action == "config":
        #workspace directories
        macro_template['%OS_BUILD%']=os_type
        macro_template['%WORKSPACE_DIR%']=workspace_src
        macro_template['%ROOT_DIR%']=macro_rootdir
    elif current_action == "build":
        #check existence of workspace tree
        if not os.path.exists(workspace_src) or not os.path.exists(workspace_ship):
           print("Error: Both 'src' and 'ship' directories must exist for build")
           raise build("Error: Both 'src' and 'ship' directories must exist for build")
        #fill in docker macors
     
        macro_docker_cmds['%DOCKERFILE%']=workspace_src +"/"+build_filename
        macro_docker_cmds['%BUILDPATH%']=workspace_src
        macro_docker_cmds['%DOCKERTAR%']=workspace_ship+"/"+build_target+target_suffix
        if tag_value==None:
           macro_docker_cmds['%TAG%']=build_target+docker_tag_suffix
        else:
           macro_docker_cmds['%TAG%']=tag_value


############################################################################
# Section: run_command
# 

def run_command(cmd=[],split_ch=' '):
    """
    run a command within subprocess and return a list output using split character

    """
    #init output_list to empty
    output_list=[]

    #Ensure cmd is list type
    if type(cmd) is not list:
        print("run_command requires command to be list type (ex: ['ls','-l'])")
        return output_list


    print("Running command:%s with split:<char>%s</char>" % (cmd,split_ch))
    
    try:
        proc = subprocess.Popen(cmd,stdout=subprocess.PIPE)
    except Exception, e :
        print("Exception caught running command. Returning empty output:")
        print e
        return output_list

    output = proc.communicate()[0]

    if not output == '':
	output.rstrip("\n")
	output.strip()

	# use [x for x in s.split(delim) if x] to get rid of delimeters residue
	# delimeters exists due to split with char leaves '' behind to rebuild
	# original string. so below essentially traverses each word and only includes
	# if word is not ""
	output_list=[ word for word in output.split(split_ch) if word ]

    #uncomment next line to debug
    #print("Finish command with output: %s output_list:%s" % (output,output_list,))

    return output_list



############################################################################
# Section: extract helper functions

def copy_file_pattern(src,dst,file_pattern=None):
    """
    For some reason python does not seem to have a file copy using regular
    file patterns (ie '*' '?'). Bogus!
    glob handles shell-style wildcards.

    If file_pattern is None, then assume pattern is appended to src input

    return list of files copy. empty if error occurs from start, or nothing found,
    otherwise will return list of successful file copies
    """

    file_list=[]

    if file_pattern:
        src = src +"/"+file_pattern

    for afile in glob.iglob(src):
        try:
            shutil.copy2(afile,dst)
            file_list.append(afile)

        except Exception, e:
            print("copy_file_pattern: Exception caught either glob/shutil.copy2:")
            print e
            print("Error:copy_file_pattern: src: %s dst: %s file_pattern: %s current afile:%s" % (src,dst,file_pattern,afile))

    if not file_list:
            print("copy_file_pattern: no successful copies found")

    return file_list


    
def zip_extract(zip_file=None,pattern=None,tmp_dir=None):
    """
    Extract files according to pattern into tmp_dir
    """

    #first check file type
    if not re.search('*zip',zip_file):
        print("Error> input zip_file %s does not have correct file type", zip_file)
        return False

    #extract files
    try:
        zip = zipfile.ZipFile(zip_file,'r')

        for afile in zip.namelist():
               print("Checking for  %s file patters within zip %s" % (pattern,afile))

               if re.search(pattern,afile):
                   print("extracting %s" % afile,zip_file)
                               
                   zip.extract(afile,tmp_path)

        zip.close()
        return True
    except Exception:
        print("Error validating Zip file: %s",zip_file)
        return False



def tar_extract(tar_file=None,pattern=None,tmp_path=None):
    """
    Extract from tar file according to pattern  into tmp dir
    """

    #first check file type
    if re.search('\.tar',tar_file):
        tar_mode='r'
    elif re.search('\.tz',tar_file):
        tar_mode='r:gz'
    else:
        print("build_msserver:tar_extract:Error: input tar_file %s does not have correct file type (.tar,.tz)", tar_file)
        return False


    print("tar_extract:Using:%s with pattern:%s" , tar_mode,str(pattern))

    #extract files
    try:
        tar=tarfile.open(tar_file,tar_mode)

        for afile in tar.getnames():
            print("tar_extract:Checking for %s file patterns within tar" % [pattern,afile])

            #if file matches patter lets extract

            if re.search(pattern,afile):
               print("tar_extract:extracting %s" , [afile,tar_file])
               tar.extract(afile,tmp_path)
        tar.close()
        return True
    except Exception,e:
        print("tar_extract:Error:validating Tar file: %s" % tar_file)
        print e
        return False

def validate_rpm(file_path_pattern=None,validate_pattern=[]):
    """
    find specific rpm located within path,validate rpm and
    return validate rpm. Return None if no valid rpm was found
    """

    #Ensure cmd is list type
    if type(validate_pattern) is not list:
        print("validate_rpm requires command to be list type (ex: ['ls','-l'])")
        return []
    else:
        print("validate_rpm: using file_path_pattern: %s \n validate_pattern: %s" % (file_path_pattern, validate_pattern))

    #valid rpm files
    valid=True
    valid_rpms=[]

    #first check rpms within src_path
    for rpm_file in glob.iglob(file_path_pattern):

        output=run_command(['rpm','-qip',rpm_file],'\n')

        #loop thru validate_patterns
        for pattern in validate_pattern:

            if not re.findall(pattern,''.join(output).strip()):
                #found a pattern that did not match
                valid=False
                break

        #add valid rpm to list
        if valid:
            valid_rpms.append(rpm_file)

        # reset not_valid
        valid=True

    print("Returning RPM header for pattern: %s" % valid_rpms)

    return valid_rpms

############################################################################
# Function: find supported archive files types and extract rpm into output
# path

def find_and_extract(input_path,output_path,file_glb_pattern=None,file_regx_pattern=None):
    """
    Look for a built rpm within tar,targz,zip or plain rpm files. Validate the rpm
    to ensure the rpm is valid
    """

    #we now have a full path patterns to sources. These are used for glob
    input_files = input_path + "/" + file_glb_pattern
    zip_files = input_path + "/" + server_targets[build_target]['zip_glbpat']
    tar_files = input_path + "/" + server_targets[build_target]['tar_glbpat']
    tz_files = input_path + "/" + server_targets[build_target]['tz_glbpat']

    #workspace rpm file pattern
    output_files = output_path + "/" + file_glb_pattern
    

    done_rpm=False
    done_zip=False
    done_tar=False
    done_tz=False

    print("\n\nfind_and_extract: Searching using: %s with glb pat: %s regx pat: %s\n\n" % ([file_glb_pattern,zip_files,tar_files,tz_files], file_glb_pattern,file_regx_pattern))
    if glob.glob(output_files):
        print("\n\nFound existing matching files in output directory")
        print("\n\nWarning: output directory files will be removed")

        for afile in glob.iglob(output_files):
            print("Removing file: %s" % afile)
            os.remove(afile)
        
    while 1:
        if not done_rpm and (glob.glob(input_files) or glob.glob(output_files)):

            print ("Found  files")

            file_list=copy_file_pattern(input_files,output_path)

            if len(file_list):
                return file_list

            if done_zip and done_tar and done_tz:
                print("\n\nWe have searched all possible archives. Done")
                #return list of output files
                file_list = glob.glob(output_files)
                return file_list

        elif not done_zip:


            print ("Searching zip files: %s" % zip_files)

            #check zips within src_path
            for zip_file in glob.iglob(zip_files):

                output=run_command(['unzip', '-l',zip_file],'\n')

                if re.findall(file_regx_pattern,''.join(output).strip()):
                    print("Zip contains a match %s" % zip_file)
                    print("extracting file into src_path %s" % src_path)

                    if not zip_extract(zip_file,file_regx_pattern,output_path):
                        print("No files in %s" % zip_file)


            done_zip=True
            print("zip done: %s" % str(done_zip))
        elif not done_tar:
           
            print ("Searching tar files: %s" % tar_files)

            #check zips within src_path
            for tar_file in glob.iglob(tar_files):

                output=run_command(['tar','-tvf',tar_file],'\n')

                if re.findall(file_regx_pattern,''.join(output).strip()):
                    print("Tar contains a match: %s" % tar_file)
                    print("extracting rpm into src_path %s" % src_path)

                    if not tar_extract(tar_file,file_regx_pattern,output_path):
                       print("No files in %s" % tar_file)

            done_tar=True
            print("tar done: %s" % str(done_tar))

        elif not done_tz:

            print ("Searching targz files: %s" % tz_files)

            #check tarz within src_path
            for tz_file in glob.iglob(tz_files):

                output=run_command(['tar','-ztvf',tz_file],'\n')

                if re.findall(file_regx_pattern, ''.join(output).strip()):
                    print("GzTar contains a match: %s" % tz_file)
                    print("extracting file into path %s" % output_path)
                    print("pattern: %s" % file_regx_pattern)

                    if not tar_extract(tz_file,file_regx_pattern,output_path):
                       print("Error:extracting file from %s failed" % tz_file)
                else:
                    print("No files in %s" % tz_file)

            done_tz=True
            print("tz done: %s" % str(done_tz))

        else:
            print("We should never get here but indicates no rpms,tar,targz nor zip files exist to process")
            print("returning from find_rpms() None")
            return None
        
   
    return None

###########################################################################
# Function: process the template file
def process_template(template_file=None,os_type=None,output_path=None):
    
    """
    Process template by subsituting macros base on command args
    Currently supported macros are defined in macro_template[] dictionary
    """

    if not os.path.exists(template_file):
        print("template file %s not found. Nothing to process" % template_file)
        return False
    else:
        print("Processing template %s with macro_template dictionary: %s" % (template_file,macro_template))
        with open(template_file, 'r') as f, open(output_path + "/"+build_filename, 'w') as o:
            for line in f:
               if len(line.strip()):
                  if not line.startswith('#'):  # This will skip comments

                     #process macros
                     tokens = re.findall('%(.*)%', line.strip())
                     for match in tokens:
                        print("Found match:%s tokens: %s" % (match,tokens))
                        line = re.sub('%'+match+'%',macro_template['%'+match+'%'],line,0) 
                     print("printing line:%s" % line)
                     o.write(line) 
                     
               else:
                  o.write(line) # write out the comments back        
    return True
#########################################################################
#
# Function: process docker file
#


def process_docker(docker_file=None,input_src=None,output_path=None):

    """
    Process docker file.
    Currently handle 'ADD' commands by copying from src_path
    """
    #check/validate inputs
    if not os.path.isfile(docker_file) or not os.path.exists(input_src) or not os.path.exists(output_path):

        print("input args invalid docker_file:%s input_src:%s output_path:%s" % (docker_file,input_src,output_path))
        return False
    else:
        #args should be good
        print("Processing docker_file %s" % docker_file)
        with open(docker_file, 'r') as f:
              for line in f:
                  if len(line.strip()):
                     if not line.startswith('#'):  # This will skip comments

                        #process ADD
                        if re.search('^ADD', line):
                              print("Found ADD cmd match:%s" % line.split())
                              src_file = input_src+"/"+line.split()[1]
                              if not os.path.isfile(src_file):
                                  print("\n\nWarning: src file within docker file %s does not exist in input src tree ... continuing\n\n" % src_file)
                                  #
                                  #print("Returning False")
                                  #return False
                                  #
                                  #lets not fail, just continue to next ADD line
                                  continue
                              print("Copy src file: %s to output:%s" % (src_file,output_path)) 
                              try:
                                  shutil.copy2(src_file,output_path)
                              except shutil.error,msg:
                                  raise build(msg)
                        
                              print("printing line:%s" % line)
    return True

############################################################################
# Section: process config action
def process_config():

    """
    Process config option
    """

    print("Processing Config action")


    #################################################################
    # setup various paths depending on inputs
    ################################################################


    # build path to rpm,zip or tar
    if src_type == "ima_pub":
        full_path = src_path + "/" + build_root + "/" + src_rel

        if not os.path.exists(full_path):
            print("Error:Build version %s invalid using path %s" % (src_rel ,full_path))
            raise build("Error:Build version %s invalide using path %s" % (src_rel,fullpath))

        else:
            #build full path to specific build
            buildtype_path = full_path + "/" + build_type
            # test path is seperate so need another var
            testtype_path = full_path + "/" + build_test

            #figure out the build label from which to retrieve image files
            if src_build == 'ask':
                print ("Asking for specific build source and test source\n\n")
                docker_build = ask_build(buildtype_path)
                test_build=ask_build(testtype_path,"Retrieve Test Build>")
            elif src_build == 'latest_dated':
                print("Requested latest_dated build. Scanning...")
                build_list = retrieve_builds(buildtype_path,True)

	        # get last element which is latest
                docker_build = build_list[-1]
                print("Found latest dated build: %s" % build)
                
                print("Requested latest_dated test build. Scanning...")
                testbuild_list = retrieve_builds(testtype_path,True)
                test_build=testbuild_list[-1]

            elif src_build == 'latest':
               print("Requested LATEST build. Setting to known 'latest' symlink")
	       # get last element which is latest
               docker_build = def_build_latest
               test_build=def_build_latest
            else:
               docker_build = src_build
               print("Error: specific source build not supported at this time, due to test tree dependecies for template processing. please use 'ask' instead")
               raise build("Error: specific source build not supported at this time due to test tree dependeies for template processing. please use 'ask' instead")

            # we can build entier path which contains appliance images
            buildimgs_path = buildtype_path +"/" + docker_build
            testimgs_path = testtype_path + "/" + test_build

            if not os.path.exists(buildimgs_path) or not os.path.exists(testimgs_path):
               print("Error: Build %s or test %s invalid" % (buildimgs_path,testimgs_path))
               raise build("Error: Build %s or test %s invalid" % (buildimgs_path,testimgs_path))

    #user provided direct path where images reside
    elif src_type == "direct":

        if not os.path.exists(src_path):
            print("Error:direct src_path  %s invalid" % (src_path))
            raise build("Error: direct src_path %s invalid" % src_path)
        else:
            buildimgs_path=src_path

    #create workspace dir
    if not os.path.exists(work_path):
        os.mkdir(work_path)
    print("workspace path:%s" % work_path)

    if not os.path.exists(workspace_src):
        os.mkdir(workspace_src)
    print("workspace source path:%s" % workspace_src)

    if not os.path.exists(workspace_ship):
        os.mkdir(workspace_ship)
    print("workspace ship path:%s" % workspace_ship)


    ###########################################################
    # extract and validate rpm and env files
    ###########################################################


    #setup glob pattern of rpm files
    rpm_glb_files = server_targets[build_target]['rpm_glbpat']
    rpm_rgx_files = server_targets[build_target]['rpm_rgxpat']
    #rpm server patterns that should reside in rpm info/headers
    #to validate rpm
    rpm_header_regx = [server_targets[build_target]['bin_rgxpat'],
                       server_targets[build_target]['src_rgxpat']]
    #env server patterns
    env_glb_files = server_targets[build_target]['env_glbpat']
    env_rgx_files = server_targets[build_target]['env_rgxpat']


    #look for rpm and copy rpm into workspace path, if found and validated
    if src_type ==  "ima_pub":
        lookin_path=buildimgs_path+"/"+build_ship
    elif src_type == "direct":
        lookin_path=buildimgs_path
    else:
        print("Error:Unknown src_type:%s" % src_type)

    src_rpm=find_and_extract(lookin_path,workspace_src,rpm_glb_files,rpm_rgx_files)
    print("config: found rpms: %s" % src_rpm)

    is_rpmvalid=validate_rpm(workspace_src+"/"+rpm_glb_files, rpm_header_regx)

    #lets also extract env files but do not validate (no reason at this point)
    src_env=find_and_extract(lookin_path,workspace_src,env_glb_files,env_rgx_files)

    if not src_rpm or not is_rpmvalid or not src_env:
        print("No rpm valid source or env found. Require rpm and env for build!")
        raise build("Error: process_config: could not process rpms images path:%s workspace src: %s is_rpmvalid:%s" % (lookin_path,workspace_src,str(is_rpmvalid)))


    #source files was found, lets next process docker template file

    #Process template file if requested
    print("looking for template: %s\n" % temp_file)
    if temp_file == "" or not os.path.isfile(temp_file):
        print("Template file option not provided or provided template file does not exist")

    elif not process_template(temp_file,os_type,workspace_src):
        print ("Cannot process template file %s" % temp_file)

    #determine if a build template file was created
    if os.path.isfile(workspace_src+"/"+build_filename):

        # build path to perftools
        if src_type == "ima_pub":
            svtperf_tools=full_path + "/" + build_test + "/" + test_build+ "/" + build_perftools
        elif src_type == "direct":
            #docker ADD files in direct path
            svtperf_tools=full_path

        #process docker commands
        print("Copy ADD docker sources: %s  into workspace %s" % (svtperf_tools,workspace_src))

        if not process_docker(workspace_src+"/"+build_filename,svtperf_tools,workspace_src):
            print("\nWarning:could not process docker template file\n")
            print("Warning:Please check previous messages for details\n")
            print("Warning:not all ADD action for template completed successfully\n\n")
    else:
        
        print("Created workspace  %s" % work_path)

    print("Created workspace %s with %s build file and associated source rpm files" % (work_path,build_filename))
    print("Config action done")

############################################################################
# Section: process build action
def process_build():
    """
    Process build action
    """

    #check for source images
    print("Processing Build action")

    #minimum validate is to check for rpms and build file
    #build rpm server patterns that should reside in rpm info/headers
    rpm_patterns=[server_targets[build_target]['bin_rgxpat'],
                  server_targets[build_target]['src_rgxpat']]

    #first check rpms within src_path 
    src_rpm=validate_rpm(workspace_src+"/"+server_targets[build_target]['rpm_glbpat'],rpm_patterns)

    if not src_rpm or not os.path.exists(workspace_src+"/"+build_filename):
        print("Error: Source RPM or %s not found or invalid %s" % (src_rpm,workspace_src+"/"+build_filename))
        raise build("Error: Source RPM or %s not found or invalid %s" %(src_rpm,workspace_src+"/"+build_filename))
    else:
        #build sources within workspace

        # modify docker cmds to reflect args
        # first copy build and save list's into strings
        raw_build_cmd=' '.join(docker_cmds['build'][:])
        raw_save_cmd=' '.join(docker_cmds['save'][:])
        
        #search and replace
        print("macro_docker_cmds:%s" % macro_docker_cmds)

        #init tags to None to indicate no multi tag
        tags=None
        repository=None
        
        #process build macros
        for macro,value in macro_docker_cmds.iteritems():
            if macro == "%TAG%":
                #check for multiple tags
                if re.search('|',value):
                    print("Found multiple tags ... processing")
                    tags=value.split('|')

                    #extract the repository name and ensure all repository names are the same
                    # otherwise fail
                    repository=tags[0].split(':')[0]
                    for tag in tags[1:]:
                        check_repository=tag.split(':')[0]
                        if check_repository != repository:
                            print("Error: all multiple tags must contain same repository name original: %s found: %s" % (repository,check_repository))
                            raise build("Error: all multiple tags must contain same repository name original: %s found: %s" % (repository,check_repository))

                    #setup first tag for build and repository for save
                    raw_build_cmd=re.sub(macro,tags[0],raw_build_cmd,0)
                    raw_save_cmd=re.sub(macro,repository,raw_save_cmd,0)

                else:
                    #single tag lets sub
                    raw_build_cmd=re.sub(macro,value,raw_build_cmd,0)
                    raw_save_cmd=re.sub(macro,value,raw_save_cmd,0)

            else:
                raw_build_cmd=re.sub(macro,value,raw_build_cmd,0)
                raw_save_cmd=re.sub(macro,value,raw_save_cmd,0)

        print("Using build cmd:%s\n\n save cmd:%s\n\n" % (raw_build_cmd.split(),raw_save_cmd.split()))

        #run build command
        build_output=run_command(raw_build_cmd.split(),'\n')
        

        if not build_output:

           print("Error: Docker build failed")
           raise build("Error: Docker build failed")

        #check for 'Successfully built'
        #use list search
        hits=[msg[len('Successfully built'):] for msg in build_output if msg.startswith('Successfully built')]

        if not hits:
            print("Error: Docker build output did not contain 'Successfully built'")
            print("Error------------build output---------------------------------\n")
            print("%s\n" % '\n'.join(build_output))
            print("End Error-----------build output------------------------------\n")
            raise build("Error:Docker build output did not contain 'Successfully built'")

        if tags:
            #we have multiple tags request. Need to run tag command for each tag
            raw_tag_cmd=docker_cmds['tagit'][:]

            src_tag=tags[0]
            for tag in tags[1:]:
                #use first tag (used in build cmd above) as the src tag
                raw_tag_cmd.extend([src_tag,tag])
                # run tag command, for each requested tag
                tag_output=run_command(raw_tag_cmd,'\n')

                if tag_output:
                    print("Error: docker tag command failed")
                    print("Error------tag output--------------------------\n")
                    print("%s\n" % '\n'.join(tag_output))
                    print("End Error-------------------tag output --------------\n")
                    raise build("Error: docker tag command failed")

        #run save command
        save_output=run_command(raw_save_cmd.split(),'\n')
    
        #docker save command does not stdout|stderr if successful
        if save_output:
           print("Error: Docker save failed")
           print("Error----------------------------------save output---------------------\n")
           print("%s\n" % '\n'.join(save_output))
           print("End Error--------------------------------------save output------------------\n")
           raise build("Error: Docker save failed")

        #we get this far we succeeded and are done
        print("Build done")

    
############################################################################
# Section: process clean action
def process_clean():

    """
    Process clean action
    """

    print("Processing Clean action")
    if not os.path.exists(workspace_src) and not os.path.exists(workspace_ship) and not clean_all:
        print("Error: either workspace '%s' src,ship do not exist or didnt ask to cleanall" % work_path)
    else:
        #only clean peacemeal specifics. do not remove all files
        for target,patterns in server_targets.iteritems():
            #check workspace src
            #remove rpm's
            src_rpm_pat=workspace_src+"/"+patterns['rpm_glbpat']
            print("using glob pat:%s" % src_rpm_pat)
            for file in glob.iglob(src_rpm_pat):
                print("Removing file: %s" % file)
                os.remove(file)

            src_env_pat=workspace_src+"/"+patterns['env_glbpat']
            for file in glob.iglob(src_env_pat):
                print("Removing file: %s" % file)
                os.remove(file)

            #remove docke file ADD's
            src_dockerfile=workspace_src+"/"+docker_build_glbpat
            for file in glob.iglob(src_dockerfile):
                with open(file,'r') as f:
                    for line in f:
                        if len(line.strip()):
                            if not line.startswith('#'):
                                if re.search('^ADD',line):
                                    print("Found ADD cmd:%s" % line.split())
                                    src_file=workspace_src+"/"+line.split()[1]
                                    if not os.path.isfile(src_file):
                                        print ("src_file %s not found, so not deleted" % src_file)
                                    else:
                                        print("removing %s" % src_file)
                                        os.remove(src_file)
                                    
                print("Remove docker build file %s" % file)
                os.remove(file)

            
            target_pat=workspace_ship+"/"+target_glbpat
            print("using glob pat:%s" % target_pat)
            for file in glob.iglob(target_pat):
                print("Removing file: %s" % file)
                os.remove(file)
            

    print("Clean done")

    
############################################################################
# Section: Process action
def process_action():
    """
    Execute the provided action
    """

    if current_action=="config":
        process_config()
    elif current_action=="build":
        process_build()
    elif current_action=="clean":
        process_clean()
    else:
        print("Error: Unsuported action")
        raise build("Unsupported action")
  
############################################################################
# Section: Main Line

def main(argv=None):

    #for module usage, cleanup/reset current_action
    global current_action

    if argv is None:
        argv=sys.argv

    try:
        process_argv(argv)
        process_help()
        build_options()
        process_action()

        #for module cleanup action to process next action
        current_action=None
    except build, err:
        print("Exception:%s" % err.msg)
        return 1

    #deploy servers

    print("\n\nSuccessful.\n")

    return 0

######################################################################
#standalone main

if __name__== '__main__':
    print("build_msserver: invoke as standalone\n\n")
    sys.exit(main())

