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
    This script is used to deploy and cleanup docker msserver.

"""

import sys
import os
#import requests
import json
import time
import subprocess
import zipfile
import glob
import datetime
import shutil

if sys.version_info[0] == 3:
    import configparser
    config = configparser.RawConfigParser()
else:
    import ConfigParser
    config = ConfigParser.ConfigParser()

############################################################################
# Section: Global variables.

config_section = "options"

# setup args dictionary and default settings

cl_args        = {'clean_all':'False',
		  'build_type':'prod',
		  'build':'None',
                  'build_src':'seed',
		  'skip_warnings':'False',
		  'server_types':'server',}

# setup docker command dictionary

docker_cmds ={ 'containers':["docker","ps","-aq"],
	       'images':["docker","images","-q"],
	       'remove_container':["docker","rm"],
	       'remove_image':["docker","rmi","-f",],
	       'load_image':["docker","load","-i"],
	       'stop_container':["docker","stop"],}

# supported server types dictionary
server_types={ 'server':{'zip_pat':'msserver-*-docker.zip','tar_pat':'msserver-*-dockerImage.tar','seed_tar_pat':'msserver-*-DockerImage.tar',},
	       'webui':{'zip_pat':'mswebui-*-docker.zip','tar_pat':'mswebui-*-dockerImage.tar','seed_tar_pat':'mswebui-*-DockerImage.tar'},}


build_root="release"
build_prod="production"
build_dev="development"
build_ship="appliance"

msserver_zip_pat="msserver-*-docker.zip"
msserver_tar_pat="msserver-*-dockerImage.tar"
mswebui_zip_pat="mswebui-*-docker.zip"
mswebui_tar_pat="mswebui-*-dockerImage.tar"
image_path=None

############################################################################
# Section: Usage

def usage():
    """
    Display the usage for this script.  The usage itself will be dependent
    upon the action which is currently being executed.
    """

    print("""
Usage: deploy_msserver.py [options]

Options:
    --help                          Show this help message and exit
    --publish_dir=DIRECTORY  	    'gsacache' directory location where docker builds reside. Typically a NFS mount.

    --tmp_dir=DIRECTORY      	    Top directory used to unpackage docker build
    --build_ver=VERSION      	    Version of build to retrive from (ex. IMA14a  IMA15a)
    --build_type=[prod|dev]  	    Retrive production or development (Default is prod)
    --build=BUILD_NAME              Name of build to deploy. Use "LATEST" to grab 'latest' symlink build.  Use "LATEST_DATED" to grab
    --build_src=[offical|seed]      Source type either official build or svtpvt seed server
				    latest dated build (ie '%Y%m%d-%H%M' format). If build is not provided, script will list available 
				    dated builds and prompt for build name
    --server_types=[webui|server|all]  Deploy mswebui, msserver or all (Default is server)

    --clean_all=[True|False]        Cleanup all docker containers and images before deployment. (Default is False)
    --skip_warnings=[True|False]    Skip all prompted warnings (ie clean_all will warn and confirm operation) (Default is False)
    --registry_server=SERVER 	    The name or IP address of the docker registry server  where msserver image resides(future work).

""")
    sys.exit(1)

def process_help():
    """
    Check to see if the help command line option has been added and if so
    we want to display the help text.
    """

    if "help" in cl_args:
        usage()
        sys.exit(0)

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

    # TODO: experimental: adding deploy options to a configparser, which can be dumped into a file
    # for replay. Then use created config file to rerun at a later time
    config.add_section(config_section)
    config.set(config_section, "build", build)

    return build

def ask_warning_cleanall(prompt="Are you ready for clean all operation? > "):
    """ 
    Ask user and warn all containers and images will be removed

    """
    while 1: 
       print("Clean ALL option has been requested:\n\n")
       print("WARNING: All containers will be stopped.\n")
       print("WARNING: All containers will be removed.\n")
       print("WARNING: All images will be removed.\n")
       
       # Prompt for the answer yea or nay.
       if prompt != "":
          if sys.version_info[0] == 3:
              ans = input(prompt + " ")
          else:
              ans = raw_input(prompt + " ")

       if ans in ('y','Yes','YES'):
	  print("Confirmed. Will be wiping all containers and images\n")
	  cl_args['clean_all']='True'
	  break
       elif ans in ('n','No','NO'):
	  print("Changed mind. Will not perform clean all.\n")
	  print("Resuming deployment")
	  cl_args['clean_all']='False'
	  break
       else:
	  print("Please enter: 'y','Yes','YES','n','No', or 'NO'\n")

############################################################################
# Section: Parameter Management

def process_argv():
    """
    Process arguments which have been passed on the command line,
    """

    if not len(sys.argv[1:]):
	usage()

    for arg in sys.argv[1:]:

        if arg[:2] != "--":
            print("Error> invalid options were supplied!")
            sys.exit(1)

        if '=' in arg:
            (name, value) = arg[2:].split('=', 1)
        else:
            name  = arg[2:]
            value = ""
        cl_args[name] = value

	
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

def build_options():
    """
    From arg inputs, validate and build neccessary options
    """
    # declare global var usage
    global image_path
    global cl_args

    # validate options exist
    if not len(cl_args):
	usage()

    for option,value in cl_args.iteritems():
	if option == "publish_dir":
	   if not os.path.exists(value):
		print("Error: Publish build tree directory invalid")
		sys.exit(1)
	elif option == "tmp_dir":
	   if not os.path.exists(value):
		print("Error: temporary directory invalid")
		sys.exit(1)
	elif option == "build_type":
	   if not value in ('prod','dev',):
		print("Error: build type %s invalid" % value)
		sys.exit(1)
	   else:
		if value == 'prod':
		    build_type = build_prod
		else:
		    build_type = build_dev
	elif option == "clean_all":
	   if not value in ('True','False'):
		print("Error: clean_all option %s invalid" % value)
		sys.exit(1)
	elif option == "skip_warnings":
	   if not value in ('True','False'):
		print("Error: skip_warnings option %s invalid" % value)
		sys.exit(1)
	elif option == "server_types":

	   if not value in "all" and not value in server_types:
		print("Error: server_types option %s invalid" % value)
		sys.exit(1)
	   elif value == "all":
		cl_args['server_types']=server_types
	   else:
		cl_args['server_types']={value:server_types[value]}

	   print("Deploying %s docker images" % value)

	elif option == "registry_server":
	   print("Error: registry option not implemented")
	   sys.exit(1)
        elif option == "build_src":
           if not value in ('seed','official'):
               print("Error: invalid build_src use 'seed' or 'official'")
               sys.exit(1)
           else:
               build_src=value
               cl_args['build_src']=value

	else:
	   if not option in ('build_ver','build',):
		print("Error: unknown option: %s" % option)
	   	sys.exit(1)

    # build global image_path to docker zip file

    image_path = cl_args['publish_dir'] + "/" + build_root + "/" + cl_args['build_ver']


    if not os.path.exists(image_path):
	print("Error:Build version %s invalid using path %s" % (cl_args['build_ver'] ,image_path))
        sys.exit(1) 
    else:
	#build full path to specific build
    	image_path = image_path + "/" + build_type
	if cl_args['build'] == 'None' or cl_args['build'] == '':
	   build = ask_build(image_path)
	elif cl_args['build'] == 'LATEST_DATED':
	   print("Requested LATEST_DATED build. Scanning...")
	   build_list = retrieve_builds(image_path,True)

	   # get last element which is latest
	   build = build_list[-1]
	   print("Found LATEST build: %s" % build)
	elif cl_args['build'] == 'LATEST':
	   print("Requested LATEST build. Setting to known 'latest' symlink")

	   # get last element which is latest
	   build = "latest"
	else:
	   build = cl_args['build']
		

    	image_path = image_path +"/" + build
	if not os.path.exists(image_path):
	    print ("Error: Build %s invalid" % image_path)
 	    sys.exit(1) 

############################################################################
# Section: run_command

def run_command(cmd=[],split_ch=' '):
    """
    run a command within subprocess and return a list output using split character

    """
    #init output_list to empty
    output_list=[]

    print("Running command:%s with split:<char>%s</char>" % (cmd,split_ch))
    proc = subprocess.Popen(cmd,stdout=subprocess.PIPE)

    output = proc.communicate()[0]

    if not output == '':
	output.rstrip("\n")
	output.strip()

	# use [x for x in s.split(delim) if x] to get rid of delimeters residue
	# delimeters exists due to split with char leaves '' behind to rebuild
	# original string. so below essentially traverses each word and only includes
	# if word is not ""
	output_list=[ word for word in output.split(split_ch) if word ]

    print("Finish command with output: %s output_list:%s" % (output,output_list,))

    return output_list

############################################################################
# Section: clean_docker

def clean_docker():
    """
    Cleanup all images and containers within local docker install.
    
    Example of commands:

	[root@svtdockerperfbm ~]# docker ps -aq
	19bb9fe4a87c
	[root@svtdockerperfbm ~]# docker rm 19bb9fe4a87c
	19bb9fe4a87c
	[root@svtdockerperfbm ~]# docker ps -aq
	[root@svtdockerperfbm ~]# docker images -q
	096c049acd91
	[root@svtdockerperfbm ~]# docker rmi 096c049acd91
	Deleted: 096c049acd91b1ace786d6725607f33d6b0a4bb9d9006ff920183e4ebd994170
	Deleted: 5d2cf2a2f3ce61046da66d99b019c08e6fb93805648a6f1b4e41678e27a0964f
	Deleted: 025da92908ef4a4921c383f4fbbc8edd2e00bba3fca8e26f797bea4f3869b982
	Deleted: 7197d2d15c1851ba091eef0f3d8bede9140978f4efdc902b3739e6d61d42e20f
	Deleted: 65c4fe01568ccce5f9e34093633414c9af457b015f08849f5c4f5206a6e9c4d8
	Deleted: d6d109dab436d4b0011407e96a47e0b60da81f9820a40c78a4782ff061d9a7fb
	Deleted: 34943839435dfb2ee646b692eebb06af13823a680ace00c0adc232c437c4f90c
	Deleted: 5b12ef8fd57065237a6833039acc0e7f68e363c15d8abb5cacce7143a1f7de8a
	Deleted: 511136ea3c5a64f264b78b5433614aec563103b4d4702f3ba7d4d2698e22c158
    """

    containers = run_command(docker_cmds['containers'],'\n')
    print("Found containers: %s" % containers)
    images = run_command(docker_cmds['images'],'\n')
    print("Found images: %s" % images)

    for container in containers[:]:

	#build stop container command. use [:] to copy list
    	stop_container = docker_cmds['stop_container'][:]
	stop_container.append(container)

        output = run_command(stop_container,'\n')
	print("stop container:%s" % output)

	#build rm container command. use [:] to copy list
    	rm_container = docker_cmds['remove_container'][:]
	rm_container.append(container)

        output = run_command(rm_container,'\n')
	print("removed container:%s" % output)

    for image in images[:]:

	#build rm image command. use [:] to copy list
    	rm_image = docker_cmds['remove_image'][:]
	rm_image.append(image)

        output = run_command(rm_image,'\n')
	print("removed image:%s" % output)

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

############################################################################
# Section: deploy validate docker build

def build_exists(file,tmp_dir=None):
    """
    check existenance of build and validate docker image by un-zipping file
    into temp directory
    """

    if not os.path.exists(file) or not  os.path.exists(tmp_dir):
        print("Error %s file or tmp dir %s does not exist" % (file,tmp_dir))
    else:
        if cl_args['build_src'] == 'official':

            try:
                zip = zipfile.ZipFile(file)
                zip.extractall(tmp_dir)
                zip.close()
            except Exception:
                print("Error validating docker zip file: %s",zip_file)
                return False
            return True
        elif cl_args['build_src'] == 'seed':
            if copy_file_pattern(file,tmp_dir):
                return True
            else:
                return False
        else:
            return False
	
def deploy_image(path=None,pattern=None,tmp_dir=None):
    """
    Deploy docker image build from path. First copy build into temp_dir.
    Then unzip and add image to docker
    """

    if build_exists(path,tmp_dir):

	# look for extracted tar  file
        docker_tar_file = glob.glob(tmp_dir+"/"+pattern)
	print("Found docker tar: %s" % docker_tar_file[0])

	# deploy msserver image
	cmd = docker_cmds['load_image'][:]
	cmd.append(docker_tar_file[0])
	print("Deploying:%s" % cmd)
	output = run_command(cmd)

    else:
 	print("Error: invalid build %s" % path)

############################################################################
# Section: Main Line

process_argv()
process_help()
build_options()

if cl_args['clean_all'] == 'True':
    if cl_args['skip_warnings'] == 'False':
	ask_warning_cleanall()

    # clean_all before deploying
    if cl_args['clean_all'] == 'True':
    	clean_docker()

#deploy servers
print("Using final image path:%s" % image_path)
print("Deploying server_types: %s" % cl_args['server_types'])

for deploy_type,pattern in cl_args['server_types'].items():
     print("Deploying %s using %s pattern" % (deploy_type,pattern))
     if cl_args['build_src'] == 'official':
         print("Deploying from official builds")
         server_image_files=glob.glob(image_path+"/"+build_ship+"/"+pattern['zip_pat'])
         print("Server files found: %s" % server_image_files)
         deploy_image(server_image_files[0],pattern['tar_pat'],cl_args['tmp_dir'])
     elif cl_args['build_src'] == 'seed':
         build_ship='docker-svtperf'
         print("Deploying files within seed server: %s" % image_path+"/"+build_ship+"/"+pattern['seed_tar_pat'])
         server_image_files=glob.glob(image_path+"/"+build_ship+"/"+pattern['seed_tar_pat'])
         print("Server files found: %s" % server_image_files)
         deploy_image(server_image_files[0],pattern['seed_tar_pat'],cl_args['tmp_dir'])



print("Successful.")

sys.exit(0)

