#!/bin/bash

## TODO: Remove the helm mode (function moved to artifactoryrelease-helm.sh)
set -x

function print_usage() {
    echo "./artifactoryupload.sh <ISM_VERSION_ID> <BUILD_LABEL>"
}

if [ -z "$1" ]; then
    echo "Must provide the ISM_VERSION_ID as the first argument."
    exit 2
else
    echo "ISM_VERSION_ID: $ISM_VERSION_ID"
    ISM_VERSION_ID="$1"
fi

if [ -z "$2" ]; then
    echo "Must provide the BUILD_LABEL as the second argument."
    exit 2
else
    echo "BUILD_LABEL: $BUILD_LABEL"
    BUILD_LABEL="$2"
fi

RPM_BUILD_LABEL=$(echo $BUILD_LABEL | sed 's/-/./')

# Download jfrog and setup config file
echo "Downloading jfrog into the image ..."
curl -fL https://getcli.jfrog.io | sh && cp jfrog $HOME/build.common/bin && chmod 755 $HOME/build.common/bin/jfrog

# HEREDOC to create jfrog config file
echo "Creating jfrog config file."
mkdir "${HOME}/.jfrog" || exit 2 
cat <<JFROGCONF > $HOME/.jfrog/jfrog-cli.conf
{
  "artifactory": [
    {
      "url": "https://na.artifactory.swg-devops.com/artifactory/",
      "password": "${ARTIFACTORY_API_KEY}",
      "serverId": "wiotp",
      "isDefault": true
    }
  ],
  "Version": "1"
}
JFROGCONF

# Our files are already tagged with the release name, so set NOVERSIONTAG=1
NOVERSIONTAG=1
# declare an array of filenames we will upload
declare -a uploadfiles
uploadfiles=(   "${TRAVIS_BUILD_DIR}/ship/rpms/install-images-CENTOS-${ISM_VERSION_ID}-${BUILD_LABEL}.tar"
                "${TRAVIS_BUILD_DIR}/ship/rpms/rpms-CENTOS-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/rpms/rpms-SLES-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayServer-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayBridge-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayWebUI-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayProxy-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayServer-CENTOS-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayBridge-CENTOS-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayWebUI-CENTOS-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayProxy-CENTOS-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/buildtree-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/sdk-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/proxy-files-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/docs-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/test-tools-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship/build-logs-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship_asan/asanlibs-${ISM_VERSION_ID}-${BUILD_LABEL}.tar.gz"
                "${TRAVIS_BUILD_DIR}/ship_asan/IBMWIoTPMessageGatewayServer-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship_asan/IBMWIoTPMessageGatewayBridge-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship_asan/IBMWIoTPMessageGatewayProxy-${ISM_VERSION_ID}-${RPM_BUILD_LABEL}.asan.centos.x86_64.rpm"
                "${TRAVIS_BUILD_DIR}/ship_asan/build-logs-${ISM_VERSION_ID}-${BUILD_LABEL}.asan.tar.gz"
            )

#DIR="$(dirname $(find $HOME -name semver))"
PATH="$PATH:/home/travis/build.common/bin"

which md5sum || exit $?
which sha1sum || exit $?

NO_RELEASE_BUILD_REGEXP="\+build"
RELEASE_BRANCH_REGEXP="^(master|(0|[1-9][0-9]*)\.x)$"
HOTFIX_BRANCH_REGEXP="^(hotfix\-((0|[1-9][0-9]*)\.){3}(0|[1-9][0-9]*))$"
echo "VERSION: $(semver)"

if [[ "$(semver)" =~ $NO_RELEASE_BUILD_REGEXP ]]; then
  echo "No major, minor, or patch commit in this build"
  exit 0
fi

# The pull request number if the current job is a pull request, "false" if it's not a pull request.
if [[ "${TRAVIS_PULL_REQUEST}" != "false" ]]; then
  echo "Build is for a pull request so skip artifactory upload"
  exit 0
fi

if [[ "${TRAVIS_BRANCH}" =~ $RELEASE_BRANCH_REGEXP ]]; then
  # Upload to artifactory as a release
  VERSION=$(semver)
else
  # Upload to artifactory as a pre-release
  VERSION=$(semver)-pre.$TRAVIS_BRANCH
  # artifactory location
  artifactoryloc="wiotp-generic-release/wiotp/messagegateway/${VERSION}"
  # This is a branch build, so clean out the old stuff before we upload
  echo "Using jfrog to delete everything in $artifactoryloc."
  jfrog rt del "${artifactoryloc}/" || echo "There was an issue running jfrog ..."
fi

# Looping through uploadfiles array
for FILE_PATH in "${uploadfiles[@]}"; do

    if (( NOVERSIONTAG == 1 )); then
        OPTION=NOVERSIONTAG
    fi

    if [ ! -e $FILE_PATH ]; then
      echo "Skipping Artifactory release - $FILE_PATH does not exist"
      continue
    fi

    echo "Uploading file $FILE_PATH ..."

    FILE_NAME=$(basename $FILE_PATH)
    FILE_EXT=
    while [[ $FILE_NAME = ?*.@(bz2|gz|lzma) ]]; do
      FILE_EXT=${FILE_NAME##*.}.$FILE_EXT
      FILE_NAME=${FILE_NAME%.*}
    done
    if [[ $FILE_NAME = ?*.* ]]; then
      FILE_EXT=${FILE_NAME##*.}.$FILE_EXT
      FILE_NAME=${FILE_NAME%.*}
    fi
    FILE_EXT=${FILE_EXT%.}

    # We forked artifactoryrelease.sh from wiotp/build.common so we could upload files 
    # without having them tagged with the release (files that are already tagged)

    if [ "${OPTION}" = "NOVERSIONTAG" ]; then
      # Already versioned
      TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-release/${TRAVIS_REPO_SLUG}/${VERSION}/${FILE_NAME}.${FILE_EXT}"
      MSPROXY_TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-local/dependencies/msproxy/${VERSION}/${FILE_NAME}.${FILE_EXT}"
      MSSIGHT_TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-local/dependencies/messagesight/${VERSION}/${FILE_NAME}.${FILE_EXT}"
    else
      TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-release/${TRAVIS_REPO_SLUG}/${VERSION}/${FILE_NAME}-${VERSION}.${FILE_EXT}"
      MSPROXY_TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-local/dependencies/msproxy/${VERSION}/${FILE_NAME}-${VERSION}.${FILE_EXT}"
      MSSIGHT_TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-local/dependencies/messagesight/${VERSION}/${FILE_NAME}-${VERSION}.${FILE_EXT}"
    fi

    md5Value="`md5sum "$FILE_PATH"`"
    md5Value="${md5Value:0:32}"
    sha1Value="`sha1sum "$FILE_PATH"`"
    sha1Value="${sha1Value:0:40}"

    echo "Uploading $FILE_PATH version $VERSION to $TARGET_URL"
    curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $TARGET_URL

    # You can uncomment the following if clause and update the test for TRAVIS_BRANCH if you want msproxy/messagesight to consume a branch build
    #if [ "${TRAVIS_BRANCH}" == "bwm-upload-builds-issue5285" ]; then
    #    if [[ $FILE_PATH =~ ${TRAVIS_BUILD_DIR}/ship/proxy-files* ]]; then
    #        echo "Uploading proxy file binaries to msproxy dependencies directory ..."
    #        curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $MSPROXY_TARGET_URL
    #    fi
    #
    #        if [[ $FILE_PATH =~ ${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayServer-CENTOS* ]]; then
    #            echo "Uploading proxy file binaries to messagesight dependencies directory ..."
    #            curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $MSSIGHT_TARGET_URL
    #        fi
    #fi

    # This is deliberately only done if the branch==master because we don't want to override artifacts tagged as 'latest' with backlevel releases (eg if we built a new patch of an old release level)
    if [ "${TRAVIS_BRANCH}" == "master" ]; then
        LATEST_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-release/${TRAVIS_REPO_SLUG}/latest/${FILE_NAME}-latest.${FILE_EXT}" 
        echo "Uploading $FILE_PATH version latest to $LATEST_URL"
        curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $LATEST_URL

        if [[ $FILE_PATH =~ ${TRAVIS_BUILD_DIR}/ship/proxy-files* ]]; then
            echo "Uploading proxy file binaries to msproxy dependencies directory ..."
            curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $MSPROXY_TARGET_URL
        fi

        if [[ $FILE_PATH =~ ${TRAVIS_BUILD_DIR}/ship/rpms/IBMWIoTPMessageGatewayServer-CENTOS* ]]; then
            echo "Uploading proxy file binaries to messagesight dependencies directory ..."
            curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $MSSIGHT_TARGET_URL
        fi
    fi
done

exit 0
