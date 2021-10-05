#!/bin/bash

# If we are passed an argument, we upload the file we are passed
#    But if no args are passed, we try and upload all logs we can
#    find.

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

if [ -n "$3" ]; then
    FILE_PATH="$3"
else
    ls -l "${TRAVIS_BUILD_DIR}/ship/"
    FILE_PATH=$(find ${TRAVIS_BUILD_DIR}/ship/ -name 'build-logs*.tar*')
    echo "FILE_PATH: $FILE_PATH"
fi

# Add directory to path so we can find semver
PATH="$PATH:/home/travis/build.common/bin"

if [ -f "$FILE_PATH" ]; then
    echo "File $FILE_PATH exists and will be uploaded."
    FILE_NAME=$(basename $FILE_PATH)

    NO_RELEASE_BUILD_REGEXP="\+build"
    RELEASE_BRANCH_REGEXP="^(master|(0|[1-9][0-9]*)\.x)$"

    echo "VERSION: $(semver)"

    if [[ "${TRAVIS_BRANCH}" =~ $RELEASE_BRANCH_REGEXP ]]; then
        # Upload to artifactory as a release
        VERSION=$(semver)
    else
        # Upload to artifactory as a pre-release
        VERSION=$(semver)-pre.$TRAVIS_BRANCH
    fi

    TARGET_URL="https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-release/${TRAVIS_REPO_SLUG}/buildlogs/${VERSION}/${TRAVIS_BUILD_NUMBER}/$FILE_NAME"

    md5Value="`md5sum "$FILE_PATH"`"
    md5Value="${md5Value:0:32}"
    sha1Value="`sha1sum "$FILE_PATH"`"
    sha1Value="${sha1Value:0:40}"

    echo "Uploading build log files $FILE_PATH to $TARGET_URL"
    if curl -H "X-JFrog-Art-Api:$ARTIFACTORY_API_KEY"  -H "X-Checksum-Md5: $md5Value" -H "X-Checksum-Sha1: $sha1Value" -T $FILE_PATH $TARGET_URL; then
        echo "The build tarball $FILE_NAME was uploaded to $TARGET_URL."
    else
        echo "There was a problem uploading $FILE_NAME to $TARGET_URL."
        exit 1
    fi
else
    echo "Did not find any log file tarball to upload."
fi
