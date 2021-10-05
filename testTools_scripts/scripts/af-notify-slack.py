#!/usr/bin/env python3
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

import os
import json
import re
import sys
import requests
import argparse
from pprint import pprint
from slack import WebClient

stfile = None
artbaseurl = "https://na.artifactory.swg-devops.com/artifactory/wiotp-generic-release/wiotp/messagegateway"
useremail = ""
logfile = None
message = None
token = None
slackToken = None
slackUser = None
userList = []
branch = None

parser = argparse.ArgumentParser()
parser.add_argument("-a", "--attachments", help="Provide comma-separated list of attachments, if any.")
parser.add_argument("-b", "--branch", help="Provide the github branch that is being tested in AF.")
parser.add_argument("-c", "--channel", help="Provide the channel in slack (if there is one) to notify.")
parser.add_argument("-l", "--logfile", help="Provide the URL to the logfile (in Artifactory).")
parser.add_argument("-m", "--message", help="Provide the message to be sent to slack.")
parser.add_argument("-s", "--stfile", default="/root/.stfile", help="File containing slack token.")
parser.add_argument("-t", "--token", help="Slack token.")
parser.add_argument("-u", "--user", help="Provide the user email of the committer.")

args = parser.parse_args()

attachments_temp = args.attachments
branch = args.branch
slackChannel = args.channel
message = args.message
stfile = args.stfile
useremail = args.user
logfile = args.logfile
token = args.token
slackUserList = []

def lookupUserFromEmailAddress(uemail):
    try:
        r = requests.get('https://slack.com/api/auth.findUser?team=T056XAKT3&email=' + str(uemail) + '&token=' + str(slackToken))
    except requests.exceptions.RequestException as e:
        raise SystemExit(e)

    userData = r.json()
    if userData["ok"] == True and userData["found"] == True:
        return userData["user_id"]
    else:
        return None


# Try and get slack token
slackToken = os.getenv("SLACK_TOKEN")

if token is not None:
    slackToken = token
else:
    if stfile is not None:
        if os.path.isfile(stfile):
            with open(stfile, 'r') as file:
                slackToken = file.read().replace('\n', '')

if slackToken is None:
    print("WARNING: SLACK_TOKEN is not defined, exiting")
    sys.exit(0)

if useremail == "NA":
    useremail = "USEREMAIL"

# slug should always be the same
slug = "wiotp/messagegateway"
slackChannel = "#messagegateway"

if useremail is not None:
    slackUser = lookupUserFromEmailAddress(useremail)
    slackUserList.append(slackUser)
if useremail == "all":
    for u in userList:
        slackUserList.append(lookupUserFromEmailAddress(u))
else:
    for u in userList:
        slackUserList.append(lookupUserFromEmailAddress(u))

if slackUser is None:
    slackUser = lookupUserFromEmailAddress('USEREMAIL')
    slackUserList.append(slackUser)

print("slackUser: " + slackUser)

if branch is None:
    if slackUser is None:
        branch = "Unknown-branch-unknown-user"
    else:
        branch = "Unknown-branch-" + useremail

if message is None:
    if logfile is not None:
        message = "AF Notification: Log file " + artbaseurl + "/" + branch + "/test_logs/" + logfile + " for tests for release " + branch + " was uploaded to Artifactory.\n"
    else:
        message = "This is an empty default message. There must have been a problem building the message.\n"

sc = WebClient(slackToken)

attachments = []

if re.match('^(master|(0|[1-9]\d*)\.x)$', branch):
    # If branch is release branch then we want to send the update to #continuous-build
    #print("Posting message " + message + " to " + slackChannel + " in Slack")
    try:
        response = sc.chat_postMessage(channel=slackChannel, text=message, attachments=attachments, as_user=False )
    except:
        pass

    pprint(response)
else:
    if not slackUserList:
        print("No one in the user notification list to slack!")
    else:
        for sUser in slackUserList:
            # Tell the person who submitted the build too/instead
            slackChannel = '@' + str(sUser)
            #print("Posting message " + message + " to @" + sUser + " in Slack")
            try:
                response = sc.chat_postMessage(channel=slackChannel, text=message, attachments=attachments, as_user=True )
            except:
                sys.exit(0)

            pprint(response)
            slackChannel = None

