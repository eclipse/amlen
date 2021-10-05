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

   This script is used to automate builds for SVTPERF on seed

"""
import build_msserver
import datetime
import os
import errno
import shutil
import glob
import sys
import traceback
import subprocess
import re

###############################################################################
# Section: global variables

seed_top="/seed/builds"
seed_root=seed_top+"/"+"release"
seed_release="CURREL"
seed_prod="production"
seed_ship="svtperf"


seed_workspace=seed_top+"/workspace"
seed_publish=seed_root +"/"+seed_release+"/"+seed_prod

src_name='src'
ship_name='ship'
seed_workspace_ship=seed_workspace+"/"+ship_name
seed_workspace_src=seed_workspace+"/"+src_name

svtperf_docker_publish="docker-svtperf"

logs_dir="build_logs"
nfs_pub_mnt="/mnt/pub"

which_build="latest_dated"

#publish files

pub_file_pat={ship_name:('*tar',),
              src_name:('*env','*rpm',),}

#build location of templates off pub perftool tree. Always use latest
templates_pub=nfs_pub_mnt+"/"+build_msserver.build_root+"/"+seed_release+"/"+build_msserver.build_test+"/latest/"+build_msserver.build_perftools+"/docker"

#publish location of official docker sources
#always use CURREL
src_publish_prod=nfs_pub_mnt+"/"+build_msserver.build_root+"/"+seed_release+"/"+build_msserver.build_prod

"""
template files

NOTE: change these patterns when template file target change
"""
msserver_template="docker_imaserver.template"
mswebui_template="docker_imawebui.template"

msserver_temp_file=templates_pub+"/"+msserver_template
mswebui_temp_file=templates_pub+"/"+mswebui_template

"""
build targets

NOTE: change these patterns when build targets change
"""
msserver_target="imaserver"
mswebui_target="imawebui"

#yum repo dir for publishing rpms for yum install
yumrepo_dir="/seed/msrepo"
yumrepo_publish_dir=yumrepo_dir+"/"+"Packages"
#yum cmd to rebuild repo
yumrepo_pub_cmd=["createrepo","--update","-v",yumrepo_dir]

"""
yum target

NOTE: change this pattern when rpm name changes
"""
#current format is 'IBMWIoTPMessageGatewayServer-2.0-20151105.1904.x86_64.rpm'
yumrepo_rpm_regexpat=["IBMWIoTPMessageGateway.*-.*-(.*).x86_64.rpm"]


#add new '--root_dir' arguement to build_msserver.py for template macor %ROOT_DIR%
macro_perftools_rootdir=nfs_pub_mnt+"/"+build_msserver.build_root+"/"+seed_release+"/"+build_msserver.build_test+"/latest/"+build_msserver.build_perftools

#docker command dictionary
docker_cmds ={ 'containers':["docker","ps","-aq"],
	       'images_id':["docker","images","-q"],
               'images_detail':["docker","images"],
	       'remove_container':["docker","rm"],
	       'remove_image':["docker","rmi"],
	       'load_image':["docker","load","-i"],
	       'stop_container':["docker","stop"],}
#supported OS's for cleanup
os_set = ('centos','redhat')

#For args being 'None' indicates no value exist for key. A null
#string "" indicates a string value is expected before passing to 
#build_msserver.py module

#config paramers

config_args = {"config":None,
               "--type":"",
               "--src":nfs_pub_mnt,
               "--src_type":"ima_pub",
               "--build_type":"prod",
               "--workspace":seed_workspace,
               "--template":"",
               "--src_build":which_build,}
#dont use root_dir for now
#               "--root_dir":macro_perf_rootdir}

#build args
build_args = {"build":None,
             "--target":"",
              "--workspace":seed_workspace,
              "--tags":""}

#clean args
clean_args = {"clean":None,
              "--all":None,
              "--workspace":seed_workspace,}

#gather list of current builds
#format is in timestamp: '%Y%m%d-%H%M'
svtperf_builds=build_msserver.retrieve_builds(seed_publish,True)
if svtperf_builds != []:
    svtperf_latest=svtperf_builds[-1]
    svtperf_oldest=svtperf_builds[0]
    svtperf_latest_dated=datetime.datetime.strptime(svtperf_latest,'%Y%m%d-%H%M')
    svtperf_oldest_dated=datetime.datetime.strptime(svtperf_oldest,'%Y%m%d-%H%M')
else:
    #handle when no svtperf builds exist
    svtperf_latest_dated=[]
    svtperf_oldest_dated=[]
    svtperf_latest=None
    svtperf_oldest=None

src_builds=build_msserver.retrieve_builds(src_publish_prod,True)
src_latest=src_builds[-1]
src_oldest=src_builds[0]

src_latest_dated=datetime.datetime.strptime(src_latest,'%Y%m%d-%H%M')
src_oldest_dated=datetime.datetime.strptime(src_oldest,'%Y%m%d-%H%M')


###########################################################################
# Exception class
class auto_build(Exception):
    def __init__(self,msg):
        self.msg=msg

class msserver_build(Exception):
    def __init__(self,msg):
        self.msg=msg

class mswebui_build(Exception):
    def __init__(self,msg):
        self.msg=msg

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
# Section: clean_docker

def clean_docker():

    """
    Cleanup containers. Seems docker build creates OS containers (ie centos or redhat) 
    depending on which OS build requires. These containers prevent images from being removed. Therefore,
    stop and remove all docker containers
    """

    containers = run_command(docker_cmds['containers'],'\n')
    print("Found containers: %s" % containers)

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

    """
    Cleanup all built images within local docker install.
    Do NOT remove 'centos' or other OS images which docker pulls
    down during build process.
    """

    images = run_command(docker_cmds['images_detail'],'\n')
    print("Found images: %s" % images)

    rm_images=filter(lambda old_image:  not any(os_image in old_image for os_image in os_set),images)

    image_ids=[line.split()[2] for line in rm_images]
    print("list of image ids to remove: %s" % image_ids)

    for image in image_ids:

	#build rm image command. use [:] to copy list
    	rm_image = docker_cmds['remove_image'][:]
	rm_image.append(image)

        output = run_command(rm_image,'\n')
	print("removed image:%s" % output)



#############################################################################
#Function: clean
# cleanup build artifacts, including docker artificates

def clean():

    """
    cleanup build artifacts
    """

    #cleanup docker build images
    clean_docker()

############################################################################
#Function: prune publish tree to only 2 weeks of builds
def prune_pubtree():

    """
    prune old builds upto 2 weeks old
    """

    print("\nPrune published builds from seed publish tree...\n")
    if os.listdir(seed_publish) == [] or svtperf_latest_dated == [] or svtperf_oldest_dated == []:
        #no pruning to be done
        print("No builds found or publish dir is empty... skipping prunning process")
        return
    else:

        time_delta=svtperf_latest_dated - svtperf_oldest_dated
        two_week_delta=datetime.timedelta(days=14)
        print("Existing build time span:%s" % time_delta)
        print("Comparing against 2 week delta:%s" % two_week_delta)

        if time_delta > two_week_delta:
            #prune builds older than 2 weeks
            print("found builds > 2 weeks ... starting pruning process")
            #copy and filter out < 2 weeks build for removal
            rm_build=filter(lambda dir_date: (svtperf_latest_dated-datetime.datetime.strptime(dir_date,'%Y%m%d-%H%M')) > two_week_delta,svtperf_builds)

            #lets first remove oldest then proceed
            for old_build in rm_build:
                shutil.rmtree(seed_publish+"/"+old_build)
                print("Pruned old build: %s" % seed_publish+"/"+old_build)

                
        else:
            print("no builds > 2 weeks found ... skipping pruning process")


##############################################################################
#Function: prune_yumrpms
def prune_yumrpms():

    """
    prune old yum rpms builds upto 2 weeks old
    """
    
    print("\nPrune published yum rpms...\n")
    #NOTE: retrive_yumrpms returns an array of tuples of (rpmfile,itsdate) elements
    svtperf_yumrpms=retrieve_yumrpms(yumrepo_publish_dir,True)

    if svtperf_yumrpms != []:
        svtperf_latest_rpm=svtperf_yumrpms[-1]
        svtperf_oldest_rpm=svtperf_yumrpms[0]
        #reference the date element of tuple which is the second element
        #NOTE: rpm uses '.' instead of '-' used for published build dir dates
        svtperf_latest_rpmdated=datetime.datetime.strptime(svtperf_latest_rpm[1],'%Y%m%d.%H%M')
        svtperf_oldest_rpmdated=datetime.datetime.strptime(svtperf_oldest_rpm[1],'%Y%m%d.%H%M')
    else:
        #handle when no svtperf builds exist
        svtperf_latest_rpmdated=[]
        svtperf_oldest_rpmdated=[]
        svtperf_latest_rpm=None
        svtperf_oldest_rpm=None

    if os.listdir(yumrepo_publish_dir) == [] or svtperf_latest_rpmdated == [] or svtperf_oldest_rpmdated == []:
        #no pruning to be done
        print("No yum rpms found or publish dir is empty... skipping prunning process")
        return
    else:

        time_delta=svtperf_latest_rpmdated - svtperf_oldest_rpmdated
        two_week_delta=datetime.timedelta(days=14)
        print("Existing yum rpm time span:%s" % time_delta)
        print("Comparing against 2 week delta:%s" % two_week_delta)

        if time_delta > two_week_delta:
            #prune builds older than 2 weeks
            print("found yum rpm builds > 2 weeks ... starting pruning process")
            #copy and filter out < 2 weeks build for removal
            rm_rpm=filter(lambda rpm_date: (svtperf_latest_rpmdated-datetime.datetime.strptime(rpm_date[1],'%Y%m%d.%H%M')) > two_week_delta,svtperf_yumrpms)

            #lets first remove oldest then proceed
            for oldrpm,rpmdate in rm_rpm:
                os.remove(yumrepo_publish_dir+"/"+oldrpm)
                print("Pruned old yum rpm: %s" % yumrepo_publish_dir+"/"+oldrpm)

                
        else:
            print("no yum rpms > 2 weeks found ... skipping pruning process")


    print("Done yum rpm pruning... we can now update Yum repository with latest rpms")
    output = run_command(yumrepo_pub_cmd,'\n')

    if output == []:
        print("Warning:Yum update repo failed with empty output for some reason!!!\n")
    else:
        print("Finished updated yum repository with:%s\n\n-------------------\nEnd createrepo output\n-------------------\n" % ('\n'.join(output)))

#############################################################################
#Function: retrive_yumrpms          
def retrieve_yumrpms(path,remove_nondated=False):

    """
    Get and return a sorted list of available rpm builds
    """

    output = os.listdir(path)
    non_dated=[]
    rpm_dated=[]

    #iterate thru contents of yum repo
    for rpmfile in output[:]:
        for regexpat in yumrepo_rpm_regexpat[:]:
            try:
                #rpm date format is different from build tree. It uses '.' seperator for month-date,hour-min
                #find the date string according to regex pattern defined in yumrpm_rpm_regexpat
                arpmdate=re.findall(regexpat,rpmfile)
                #there should be only one find from findall, use firt element below
                if arpmdate:
                    is_valid=datetime.datetime.strptime(arpmdate[0],'%Y%m%d.%H%M')
                    print("stripped yum rpm date: %s pushing (file: %s,datetime: %s) tuple into rpm_dated array" % (arpmdate,rpmfile,is_valid))
                    rpm_dated.append((rpmfile,arpmdate[0]))

            except ValueError:
                print("Found non-dated/file/rpm:%s Removing from build list" % rpmfile)
                output.remove(rpmfile)
                #save non dated to restore later
                non_dated.append((rpmfile,None))



    # sorting using build timestamp format '20150331-2001'

    sorted_output = sorted(rpm_dated,key=lambda arpm_tuple: datetime.datetime.strptime(arpm_tuple[1], '%Y%m%d.%H%M'))

    #add back removed known non-dated builds, if requested:
    if not remove_nondated:
        for afile in non_dated:
            sorted_output.append((afile,None))
                          
    print("\n retrive_yumrpms returning sorted tuple output:%s" % sorted_output)
    return sorted_output

    
############################################################################
#Function: publish
def publish():

    """
    1) move ship artifacts to publish tree
    2) move rpms to yum repo
    """
    
    #created neccessary dirs
    new_publish_dir=seed_publish+"/"+src_latest+"/"+svtperf_docker_publish
    print("Creating new publish dir:%s" % new_publish_dir)

    try:
        os.makedirs(new_publish_dir)
    except OSError as e:
        if e.errno == errno.EEXIST:
            print("WARNING: publish dir already exist ... ignoring")
        else:
            raise e

    #copy all tar files from workspace ship into publish

    published_files=[]
    for dir_key,set_list in pub_file_pat.items():
        for file_pat in set_list:
            #build path with pattern
            pub_files = seed_workspace+"/"+dir_key+"/"+file_pat
            print("Publishing files using: %s" % pub_files)
            for pub_file in glob.iglob(pub_files):
                #first publish file into ship tree
                try:
                    #use copy to just copy file and not its perms
                    shutil.copy(pub_file,new_publish_dir)
                    print("\n\nPublished: %s to dir: %s" % (pub_file,new_publish_dir))
                    #save published file list for logs
                    published_files.append(pub_file)
                except OSError as e:
                    if e.errno == errno.EEXIST:
                        print("WARNING: file already exist ... ignoring")
                    else:
                        raise e

                #second publish rpms into yum repo dir

                #check if current pub file is '.rpm' type, then copy
                if re.search('\.rpm',pub_file):
                    try:
                        #use copy to just copy file and not its perms
                        shutil.copy(pub_file,yumrepo_publish_dir)
                        print("\n\nYum published: %s to dir: %s" % (pub_file,yumrepo_publish_dir))
                        #save published file list for logs
                    except OSError as e:
                        if e.errno == errno.EEXIST:
                            print("WARNING: file already exist ... ignoring")
                        else:
                            raise e



    print("Published build with the following files: %s \n" % published_files)
    print("Creating 'latest' link")

    try:
        os.unlink(seed_publish+"/latest")
    except OSError as e:
        if e.errno == errno.ENOENT:
            print("WARNING: 'latest' link does not exist ... ignoring")
        else:
            raise e

    try:
        os.symlink("./"+src_latest,seed_publish+"/latest")
    except OSError as e:
        if e.errno == errno.EEXIST:
            print("WARNING: link already exist ... ignoring")
        else:
            raise e
###########################################################################
#Function: clean_docker_setup
def clean_docker_context():

    """
    cleanup old workspace which contains old docker build context for ADD files
    """

    #remove all files from workspace src
    #copy perftool dir tree into workspace
    try:
        shutil.rmtree(seed_workspace_src)
    except OSError as e:
        print("Error: exception during cleanup/remvoal of workspace src")
        raise e

###########################################################################
#Function: setup_docker_context
def setup_docker_context():

    """
    setup build context workspace for ADD dockerfile operation
    """

    #copy perftool dir tree into workspace
    try:
        shutil.copytree(macro_perftools_rootdir,seed_workspace_src,False,None)
    except OSError as e:
        print("Error: exception occured trying to copy perftools to workspace src\n")
        raise e


###########################################################################
#Function: build_all
def build_all():

    """
    build all msserver
    """
    
    #clean all
    arg_list=[]
    for arg,value in clean_args.items():
        print("build args:%s with value:%s" % (arg,value))
        if not value==None:
            full_arg=arg+"="+value
        else:
            full_arg=arg
        arg_list.insert(0,full_arg)

    #add dummy first element which should be name of module
    arg_list.insert(0,"msserver_build_module")
    print("passing arg_list: %s" % arg_list)
    rc = build_msserver.main(arg_list)

    if rc:
        #error occured
        raise auto_build("Error: msserver clean all failed using args: %s" % arg_list)

    #config msserver first
    arg_list=[]
    for arg,value in config_args.items():
        print("build args:%s with value:%s" % (arg,value))
        if not value==None:
            if arg == "--type":
                full_arg=arg+"="+msserver_target
            elif arg == "--template":
                full_arg=arg+"="+msserver_temp_file
            else:
                full_arg=arg+"="+value
        else:
            full_arg=arg
        arg_list.insert(0,full_arg)

    #add dummy first element which should be name of module
    arg_list.insert(0,"msserver_build_module")
    print("passing arg_list: %s" % arg_list)
    rc = build_msserver.main(arg_list)

    if rc:
        #error occured
        raise msserver_build("Error: msserver config failed using args: %s" % arg_list)


    #now build msserver
    arg_list=[]
    #chef requires a known tag to perform 'docker run' for some reasone. Adding additional tag
    repository=msserver_target+"-"+seed_ship
    first_tag=repository+":"+src_latest
    second_tag=repository+":latest"

    for arg,value in build_args.items():
        print("build args:%s with value:%s" % (arg,value))
        if not value==None:
            if arg == "--target":
                full_arg=arg+"="+msserver_target
            elif arg == "--tags":
                full_arg=arg+"="+first_tag+"|"+second_tag
            else:
                full_arg=arg+"="+value
        else:
            full_arg=arg
        arg_list.insert(0,full_arg)

    #add dummy first element which should be name of module
    arg_list.insert(0,"msserver_build_module")
    print("passing arg_list: %s" % arg_list)
    rc = build_msserver.main(arg_list)

    if rc:
        #error occured
        raise msserver_build("Error: msserver build failed using args: %s" % arg_list)


    #config mswebui first
    arg_list=[]
    for arg,value in config_args.items():
        print("build args:%s with value:%s" % (arg,value))
        if not value==None:
            if arg == "--type":
                full_arg=arg+"="+mswebui_target
            elif arg == "--template":
                full_arg=arg+"="+mswebui_temp_file
            else:
                full_arg=arg+"="+value
        else:
            full_arg=arg
        arg_list.insert(0,full_arg)

    #add dummy first element which should be name of module
    arg_list.insert(0,"msserver_build_module")
    print("passing arg_list: %s" % arg_list)
    rc = build_msserver.main(arg_list)

    if rc:
        #error occured
        raise mswebui_build("Error: mswebui config failed using args: %s" % arg_list)

    #now build mswebui
    arg_list=[]
    #chef requires a known tag to perform 'docker run' for some reasone. Adding additional tag
    repository=mswebui_target+"-"+seed_ship
    first_tag=repository+":"+src_latest
    second_tag=repository+":latest"

    for arg,value in build_args.items():
        print("build args:%s with value:%s" % (arg,value))
        if not value==None:
            if arg == "--target":
                full_arg=arg+"="+mswebui_target
            elif arg == "--tags":
                full_arg=arg+"="+first_tag+"|"+second_tag
            else:
                full_arg=arg+"="+value
        else:
            full_arg=arg
        arg_list.insert(0,full_arg)

    #add dummy first element which should be name of module
    arg_list.insert(0,"msserver_build_module")
    print("passing arg_list: %s" % arg_list)
    rc = build_msserver.main(arg_list)

    if rc:
        #error occured
        raise mswebui_build("Error: mswebui build failed using args: %s" % arg_list)



###############################################################################
#Function: main()

def main(argv=None):

    print("current published svtperf builds: %s" % svtperf_builds)
    print("oldest dated: %s latest dated: %s" % (svtperf_oldest_dated,svtperf_latest_dated))

    print("current source's in publish tree: %s" % src_builds)
    print("oldest dated: %s latest dated: %s" % (src_oldest_dated,src_latest_dated))

    if argv is None:
        argv=sys.argv

    try:
        clean_docker_context()
        setup_docker_context()
        build_all()
    except auto_build, err:
        print("Error: auto_build exception: %s" % err.msg)
        print("Exiting with error")
        return 1
    except msserver_build,err:
        print("Error: msserver_build exception: %s" % err.msg)
        print("Exiting with error")
        return 1
    except mswebui_build,err:
        print("Warning: mswebui_build exception: %s" % err.msg)
        print("continuing with warning. publishing will not include mswebui image")
    except Exception,e:
        print("Error: auto_build General exception: %s" % e)
        type_, value_, traceback_ = sys.exc_info()
        print("%s \n%s \n%s \n" % ('\n'.join(traceback.format_tb(traceback_)),value_,type_))
        print("Exiting with error")
        return 1

    try:
        publish()
        prune_pubtree()
        prune_yumrpms()
        clean()
    except auto_build, err:
        print("Error: auto_build exception: %s" % err.msg)
        print("Exiting with error")
        return 1
    except Exception,e:
        print("Error: auto_build General exception: %s" % e)
        type_, value_, traceback_ = sys.exc_info()
        print("%s \n%s \n%s \n" % ('\n'.join(traceback.format_tb(traceback_)),value_,type_))
        print("Exiting with error")
        return 1

    print("Build and publish SUCCESS!")
    return 0


if __name__ == '__main__':
    print("auto_build: invoke as standalone\n\n")
    sys.exit(main())
