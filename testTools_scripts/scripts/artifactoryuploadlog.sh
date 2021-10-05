#!/bin/bash

## script: artifactoryuploadlog.sh
## usage: ./artifactoryuploadlog.sh <LOGFILE> <RELEASE_BRANCH>
## example: ./artifactoryuploadlog.sh ismClient_logs_A1AFMGbvtCOD10aA2AFMGbvtCOD10bM1AFMGbvt10.zip 1.0.18-pre.bwm5452-af-updates
## This will use jfrog to upload the log files to:
##   https://na.artifactory.swg-devops.com/artifactory/list/wiotp-generic-release/wiotp/messagegateway/1.0.18-pre.bwm5452-af-updates/test_logs/
#
#set -x

AULOGFILE="/root/artifactoryuploadlog.log"

function print_usage() {
    echo "./artifactoryupload.sh <LOGFILE> <TARGETFILE> <RELEASE_BRANCH> <API_KEY>"
}

if [ -z "$1" ]; then
    echo "Must provide a LOGFILE as the first argument." | tee -a "$AULOGFILE"
    exit 2
else
    echo "LOGFILE: $1" | tee -a "$AULOGFILE"
    LOGFILE="$1"
fi

if [ -z "$2" ]; then
    echo "Must provide a value for the targetFile; eg: softlayer/fvt_prod/logfiles/20201007-1539/ismClient_logs_A1AFcciCOD01iM1AFfvt01iM2AFfvt02iP1AFproxy01i.zip"
    echo "See the postLogFilesArtifactory function in commonFunc.xml on the AFharness machine for details."
    print_usage
    exit 2
else
    echo "TARGET_FILE: $2" | tee -a "$AULOGFILE"
    TARGET_FILE="$2"
fi

if [ -z "$3" ]; then
    echo "Must provide a value for the releaseBranch; eg: 1.0.18-pre.bwm5452-af-updates"
    print_usage
    exit 2
else
    echo "RELEASE_BRANCH: $3" | tee -a "$AULOGFILE"
    RELEASE_BRANCH="$3"
fi

if [ -z "$4" ]; then
    echo "Must provide a value for the artifactory api key."
    print_usage
    exit 2
else
    #echo "ARTIFACTORY_API_KEY: $3"
    ARTIFACTORY_API_KEY="$4"
fi


ARTIFACTORY_URL="https://na.artifactory.swg-devops.com/artifactory/list/wiotp-generic-release/wiotp/messagegateway"
LOGFILE_DIR="${ARTIFACTORY_URL}/${RELEASE_BRANCH}/test_logs"

which md5sum || exit $?
which sha1sum || exit $?

md5Value="`md5sum "$LOGFILE"`"
md5Value="${md5Value:0:32}"
sha1Value="`sha1sum "$LOGFILE"`"
sha1Value="${sha1Value:0:40}"

BASE_LOGFILE="$(basename $LOGFILE)"
DIR_LOGFILE="$(dirname $LOGFILE)"
TARGET_URL="${LOGFILE_DIR}/${TARGET_FILE}/${LOGFILE}"

echo "Uploading $LOGFILE to $TARGET_URL" | tee -a "$AULOGFILE"
curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $LOGFILE $TARGET_URL | tee -a "$AULOGFILE"

exit 0
