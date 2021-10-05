#!/bin/bash

# DNS entry below can be found in the WIoTP LoadTest IBM Cloud Account under Classic Infrastructure > DNS > Forward Zones
WEBHOOK_URL=https://afharness.loadtest.internetofthings.ibmcloud.com:8443/notifications

#if [ "${TRAVIS_PULL_REQUEST}" != "false" ]; then
#    echo "Build is a Pull Request.  No build notification is required for PRs"
#    exit 0
#fi

if [ -n "$1" ]; then
    export BUILD_LABEL="$1"
else
    echo "Create fake build label for AFharness ..."
    export BUILD_LABEL=$(date +%Y%m%d-%H%M)
fi

if [ -n "$2" ]; then
    export ISM_VERSION_ID="$2"
else
    echo "No ISM_VERSION_ID passed in. Defaulting to 5.0.0.3."
    export ISM_VERSION_ID="5.0.0.3"
fi

if [ -n "$3" ]; then
    export buildproduct="$3"
else
    echo "No BUILDPRODUCT passed in. Defaulting to ibm."
    export buildproduct="ibm"
fi

# branch builds get a BVT AF run by default
#   If BUILD_LABEL does not contain a string we assume it is a master build and we do a full PROD run
if [[ $BUILD_LABEL =~ pre ]]; then
    export AFTYPE="BVT"
    echo "Requesting a BVT run for a branch build."
elif [ "$TRAVIS_BRANCH" = "master" ]; then
    # Default for master builds is PROD run
    export AFTYPE="PROD"
    echo "Requesting a PROD run for a stable build."
else
    export AFTYPE="BVT"
    echo "Unexpected BUILD_LABEL: $BUILD_LABEL"
    echo "Setting AFTYPE as BVT."
fi

# Check the git commit message to see if developer requested a specific AF test run (i.e. a non-default type of AF run)
# choices: "NONE", "PROD", "DEV", "BVT"
if [[ $TRAVIS_COMMIT_MESSAGE == *" AFTYPE="* || $TRAVIS_COMMIT_MESSAGE == *" aftype="* ]]; then
    AFTYPE=$(echo $TRAVIS_COMMIT_MESSAGE | grep -ioP "aftype=\K\w+") # grab the AFTYPE from the commit message
    export AFTYPE=$(echo $AFTYPE | tr a-z A-Z | awk '{print $1}') # uppercase AFTYPE
    if [ "$AFTYPE" != "PROD" ] && [ "$AFTYPE" != "DEV" ] && [ "$AFTYPE" != "BVT" ] && [ "$AFTYPE" != "NONE" ]; then
        echo "The AFTYPE provided in the git commit was not valid. Valid options are aftype=none, aftype=prod, aftype=dev or aftype=bvt"
        export AFTYPE="NONE"
    fi
fi

if [ "$AFTYPE" = "NONE" ]; then
    echo "AFTYPE = NONE. Not requesting an AF test run."
    exit 0
else
    echo "Submitting a job of $AFTYPE type to AF, BUILD_LABEL: $BUILD_LABEL"
fi

GIT_RELEASE=$($HOME/build.common/bin/semver)
# Temporarily setting STREAM to MGPROD while co-existing with RTC
STREAM="MGPROD"

# See: https://stackoverflow.com/questions/39681509/travis-ci-how-to-get-committer-email-author-name-within-after-script-command
export COMMITTER_EMAIL="$(git log -1 $TRAVIS_COMMIT --pretty="%cE")"
echo "COMMITTER_EMAIL=${COMMITTER_EMAIL}"

if [ -n "$SLACK_TOKEN" ]; then
    echo "SLACK_TOKEN is set."
else
    echo "Setting SLACK_TOKEN to empty."
    export SLACK_TOKEN="empty"
fi

if [ -n "$ARTIFACTORY_API_KEY" ]; then
    echo "ARTIFACTORY_API_KEY is set."
else
    echo "Setting ARTIFACTORY_API_KEY to empty."
    export ARTIFACTORY_API_KEY="empty"
fi


# To be able to upload logs to artifactory, we need the Artifactory API token

# Do we want to add a check in here to not do a full run for branch build
#     ie, if GIT_RELEASE include the string "pre"?
POST_DATA_TEMPLATE='{"repo": "%s", "branch": "%s", "release": "%s", "stream": "%s", "build_label": "%s", "aftype": "%s", "username": "%s", "version": "%s", "product": "%s"}'
POST_DATA=$(printf "$POST_DATA_TEMPLATE" "${TRAVIS_REPO_SLUG}" "${TRAVIS_BRANCH}" "${GIT_RELEASE}" "${STREAM}" "${BUILD_LABEL}" "${AFTYPE}" "${COMMITTER_EMAIL}" "${ISM_VERSION_ID}" "${buildproduct}")

# Send POST request to AFHarness HTTP server to notify AF of build
KEY=$(echo -n "etymolog minor notebook neatly daybed" | base64)
curl -ik \
     -H "Content-Type:application/json" \
     -H "Authorization: Basic ${KEY}" \
     -X POST -d "${POST_DATA}" $WEBHOOK_URL
