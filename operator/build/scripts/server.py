#!/usr/bin/python
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

import json
import logging
import os
import sys
import time
import requests
from requests.auth import HTTPBasicAuth

from requests.packages.urllib3.exceptions import InsecureRequestWarning # pylint: disable=E0401
requests.packages.urllib3.disable_warnings(InsecureRequestWarning) #pylint: disable=E1101

# Constants
MESSAGESIGHT_SHUTDOWN_GRACE_PERIOD = 60
DEFAULT_HEARTBEAT = 10

# Files with username/password
PASSWORD_FILE="/secrets/admin/adminPassword"

# Certificates
CA_PUBLIC_CERT = "/secrets/internal_certs/ca.crt"
MSSERVER_CERT  = "/secrets/internal_certs/tls.crt"
MSSERVER_KEY   = "/secrets/internal_certs/tls.key"

# Configuration
MSSERVER_CONFIG="/etc/msserver/iotmsserverconfig.json"

RESTART_ITERATIONS = 60
WAIT_FOR_MODE_ITERATIONS = 60
DELAY = 5

def get_logger(name):
    log = logging.getLogger(name)
    log.setLevel(logging.INFO)

    # Only add the handlers once!
    if log.handlers == []:
        handler = logging.StreamHandler()
        handler.setLevel(logging.INFO)

        formatter = logging.Formatter(
           '%(asctime)-25s %(name)-50s %(threadName)-16s %(levelname)-8s %(message)s'
        )
        handler.setFormatter(formatter)

        log.addHandler(handler)

    return log

def check_for_pause(filename,logger):
    modificationtime = None
    paused = False

    try:
        modificationtime = os.path.getmtime(filename)
    except OSError as exc:
        logger.debug("Doing normal check as %s doesn't exist: %s", filename , exc)
    except Exception as exc:
        logger.error('Error looking for for %s modification time: %s', filename, exc)
        logger.error("Doing normal check as couldn't read modification time for %s", filename)
        modificationtime = None

    if  modificationtime is not None:
        now              = time.time()

        if now - modificationtime > 24*60*60:
            logger.warning("Old Pause file found ignored - Should %s be deleted? ", filename)
        else:
            paused=True
            logger.warning(
                "Found %s - will test/log but report ok to avoid container restart", filename
            )
    return paused

def get_username():
    return "sysadmin"

def get_password():
    with open(PASSWORD_FILE, encoding="utf-8") as password_file:
        return password_file.readline().rstrip()

class Server:
    'Server instance'

    def __init__(self, server, logger, password=None):
        self.server_name = server
        self.files= []
        self.config = None

        self.logger = logger

        self.logger.info("Created MessageGateway object with name %s" , self.server_name)

        self.group_name = "amlen-k8s"

        self.username = get_username()
        if password is None:
            self.password = get_password()
        else:
            self.password = password

    def update_password(self, password):
        self.password = password

    def restart(self, clean_store=False, maintenance=None):

        server_name = self.server_name

        payload = None
        if clean_store:
            payload = json.dumps({"Service":"Server", "CleanStore":clean_store})
            ms_config_url = f"https://{server_name}:9089/ima/v1/service/restart"
            self.logger.info("* Restarting %s (cleanstore = %s)" , server_name, clean_store)
        elif maintenance is not None:
            payload = json.dumps({"Service":"Server", "Maintenance" : maintenance})
            ms_config_url = f"https://{server_name}:9089/ima/v1/service/restart"
            self.logger.info("* Restarting %s (maintenance = %s)" , server_name, maintenance)
        else:
            payload = json.dumps({"Service":"Server"})
            ms_config_url = f"https://{server_name}:9089/ima/v1/service/stop"
            self.logger.info("* Restarting %s gracefully" , server_name)

        old_status = self.get_status()

        # Try to restart 10 times, with 5-second delay between attempts
        restart_initiated = False
        for _ in range(0, 10):
            try:
                response = requests.post(url=ms_config_url,
                                         data=payload,
                                         auth=HTTPBasicAuth(self.username, self.password),
                                         verify=False)
                if response.status_code == 200:
                    restart_initiated = True
                    break

                self.logger.info(
                    "* Restart attempt failed with HTTP code %s, retrying in %s seconds" ,
                    response.status_code,
                    DELAY
                )
            except Exception as ex:
                self.logger.info("* Restart attempt failed, retrying in %s seconds (%s)", DELAY, ex)

            time.sleep(DELAY)

        if not restart_initiated:
            error_text = f"Server {server_name} could not be restarted"
            self.logger.critical(error_text)
            raise Exception(error_text)

        self.logger.info("* Restart requested for server %s" , server_name)

        self.wait_for_restart(initial_uptime=old_status["Server"]["UpTimeSeconds"], delay=DELAY)

    def switch_to_maint_mode(self):
        status = self.get_status()
        if status is None:
            error_text = f"Server {self.server_name} is down"
            self.logger.info(error_text)
            raise Exception(error_text)

        maint_mode = (status["Server"]["State"] == 9)

        if not maint_mode:
            self.restart(maintenance="start")

    def switch_to_production_mode(self):
        status = self.get_status()
        if status is None:
            error_text = f'{self.server_name} is down. Check for "Unable to bind socket" in the server trace file.'
            self.logger.info(error_text)
            raise Exception(error_text)

        maint_mode = (status["Server"]["State"] == 9)

        if maint_mode:
            self.logger.info("Moving server %s from maintenance to production" , self.server_name)
            self.restart(maintenance="stop")
        else:
            self.logger.info("Server %s is not in maintenance mode (%s - %s)" ,
                self.server_name, status["Server"]["State"], status["Server"]["Status"])

    def clean_store(self):
        self.restart(clean_store=True)

    def configure_stand_alone_ha(self, enable_ha=False):
        restart = False

        input_data = {
            "HighAvailability": {
                "Group": self.group_name,
                "RemoteDiscoveryNIC": "None",
                "LocalDiscoveryNIC": self.server_name,
                "LocalReplicationNIC": self.server_name,
                "StartupMode": "StandAlone"
            }
        }

        # If the current state doesn't match desired state, update it
        is_ha, state = self.get_ha_state(wait=True)
        if is_ha != enable_ha:
            input_data['HighAvailability']['EnableHA'] = enable_ha
            restart = True
            self.logger.info("Changing high availability status to %s, will restart server %s",
                enable_ha, self.server_name)
        elif state in ("UNSYNC", "UNSYNC_ERROR", "HADISABLED"):
            restart = True
            self.logger.info("Force restart for %s to correct HA state %s",
                self.server_name, state)

        status_code, response = self.post_configuration_request(input_data)
        if status_code != 200:
            error_text = f"Unexpected response code switching server {self.server_name} to StartupMode=StandAlone: {status_code}"
            self.logger.error(error_text)
            self.logger.error(response)
            raise Exception(error_text)

        if restart:
            self.restart()
            _, state = self.get_ha_state(wait=True)
            if state in ("UNSYNC", "UNSYNC_ERROR"):
                error_text = f"Could not configure HA on server {self.server_name}: {state}"
                self.logger.critical(error_text)
                raise Exception(error_text)

    def get_ha_startup_mode(self):
        jconfig = self.get_configuration("HighAvailability", False)
        return jconfig["HighAvailability"]["StartupMode"]

    def get_ha_sync_nodes(self):
        jstatus = self.get_status(False)
        if 'HighAvailability' not in jstatus:
            return 0
        if 'Enabled' not in jstatus['HighAvailability']:
            return 0
        if not jstatus["HighAvailability"]["Enabled"]:
            return 0
        return jstatus["HighAvailability"]["SyncNodes"]

    def get_ha_state(self, wait=False):
        is_ha = False

        state = "None"
        # Wait for up to 5 minutes (15 iterations, 20 seconds each)
        for _ in range(0, 15):
            jstatus = self.get_status(wait)
            if jstatus is not None:
                if 'HighAvailability' not in jstatus:
                    is_ha = False
                    break
                if 'Enabled' not in jstatus['HighAvailability']:
                    is_ha = False
                    break
                if jstatus["HighAvailability"]["Enabled"]:
                    is_ha = True
                    state = jstatus["HighAvailability"]["NewRole"]
                    if state in ("UNKNOWN", "UNSYNC"):
                        server_errorcode = jstatus["Server"]["ErrorCode"]
                        if server_errorcode == 0:
                            self.logger.info("Server %s is in transition state %s",
                                             self.server_name, state)
                        else:
                            self.logger.info("Server %s is in error state %s (%s)",
                                             self.server_name, state, server_errorcode)
                            break
                    else:
                        break
                else:
                    is_ha = False
                    break

            if not wait:
                break

            time.sleep(20)

        self.logger.info("HA enabled for %s: %s - %s" , self.server_name, str(is_ha), state)

        return is_ha, state

    def get_object(self, object, wait=True):

        return_object = None
        retries = 10
        for _ in range(0, retries):
            url = f"https://{self.server_name}:9089/ima/v1/{object}"
            try:
                response = requests.get(url=url,
                                        auth=HTTPBasicAuth(self.username, self.password),
                                        timeout=10,
                                        verify=False)
                if response.status_code == 200:
                    text = response.content
                    self.logger.debug(response.content)
                    return_object = json.loads(text)
                    break
                if response.status_code in (400, 401):
                    print (f"Accessing {url} with {self.username} failed")
                    raise Exception(f"Error accessing {self.server_name} - {response.status_code}")
            except requests.exceptions.Timeout:
                self.logger.info("Timed out waiting to connect to %s, retrying" , url)
            except requests.exceptions.RequestException:
                self.logger.info("Failed to connect to %s, retrying" , url)

            if not wait:
                break

            time.sleep(5)

        return return_object

    def get_configuration(self, configuration_type, wait=True):
        return self.get_object(f"configuration/HighAvailability", wait)

    def get_status(self, wait=True):
        return self.get_object(f"service/status", wait)


    def check_ha_status(self):
        okay = False
        state = "None"
        for _ in range(0, 600, 10):
            jstatus = self.get_status()
            if jstatus is not None:
                if 'NewRole' in jstatus["HighAvailability"]:
                    state = jstatus["HighAvailability"]["NewRole"]
                    if jstatus["HighAvailability"]["NewRole"] == "UNSYNC_ERROR":
                        okay = False
                        break
                    if jstatus["HighAvailability"]["NewRole"] != "UNSYNC":
                        okay = True
                        break
                else:
                    okay = False
                    break
            time.sleep(10)

        self.logger.info("HA state for %s: %s" , self.server_name, state)

        return okay

    def is_standby_node(self, wait=True):
        state = self.get_ha_state(wait)[1]
        return state == "STANDBY"

    def configure_admin_endpoint(self):
        self.logger.info("Configuring admin endpoint")

        self.logger.info("First wait for admin endpoint to come up")
        self.get_status(wait=True)

        admin_sec_profile_input = {
            "AdminUserID": self.username,
            "AdminUserPassword": self.password
        }

        status_code, response_text = self.post_configuration_request(admin_sec_profile_input,
                                                                     hide=True)
        if status_code != 200:
            error_text = f"Unexpected response configuring admin credentials for {self.server_name}: {status_code}"
            self.logger.error(error_text)
            self.logger.error(response_text)
            raise Exception(error_text)

        self.logger.info("Username and password have been set")

        if self.is_standby_node(wait=False):
            self.logger.info("Don't configure admin endpoint on standby node")
            return

        admin_sec_profile_input = {
            "SecurityProfile":  {
                "AdminDefaultSecProfile": {
                    "CertificateProfile" : "IoTCertificate",
                    "TLSEnabled" : True,
                    "UsePasswordAuthentication" : True
                }
            },
            "AdminEndpoint":  { "SecurityProfile" : "AdminDefaultSecProfile" }
        }

        status_code, response_text = self.post_configuration_request(admin_sec_profile_input)
        if status_code != 200:
            error_text = f"Unexpected response configuring Admin Endpoint for {self.server_name}: {status_code}"
            self.logger.error(error_text)
            self.logger.error(response_text)
            raise Exception(error_text)

        self.logger.info("Admin endpoint has been configured")

    def reconfigure_admin_endpoint(self, newpassword):
        self.logger.info("Re-Configuring admin endpoint")

        admin_sec_profile_input = {
            "AdminUserID": self.username,
            "AdminUserPassword": newpassword
        }

        status_code, response_text = self.post_configuration_request(admin_sec_profile_input,
                                                                     hide=True)
        if status_code != 200:
            error_text = f"Unexpected response when reconfiguring admin credentials for {self.server_name}: {status_code}"
            self.logger.error(error_text)
            self.logger.error(response_text)
            return False

        self.logger.info("Password has been updated")

        self.password = newpassword
        return True

    def load_file(self, name, contents):
        self.files.append({'file': (name,contents)})

    def read_files(self):
        for filename in [CA_PUBLIC_CERT, MSSERVER_CERT, MSSERVER_KEY]:
            name = os.path.basename(filename)
            self.files.append ({'file': (name, open(filename, 'rb')) })

    def upload_file(self, file):
        name = file['file'][0]

        auth = HTTPBasicAuth(self.username, self.password)

        url = f"https://{self.server_name}:9089/ima/v1/file/{name}"
        response = requests.put(url=url, files=file, auth=auth, verify=False)
        return response.status_code, response.content

    def set_config(self,config):
        self.config = config

    def configure_iotf_policy(self):
        self.logger.info("Setting up default WIoTP configuration for %s (%s)",
                         self.server_name,
                         MSSERVER_CONFIG)

        if self.is_standby_node(wait=False):
            self.logger.info("* Skipping standby node %s", self.server_name)
            return

        if self.config is None:
            # Pick up the "base" configuration from the config file.
            try:
                with open(MSSERVER_CONFIG,encoding="utf-8") as config_file:
                    config_dict = json.loads(config_file.read())
                    self.config = config_dict['base']
                    self.logger.info("* Retrieved base configuration information for %s",
                                 self.server_name)
            except Exception as ex:
                self.logger.critical("* Base configuration file for %s could not be read",
                                     self.server_name)
                error_text = f"Error {sys.exc_info()[0]}: {ex}"
                self.logger.critical(error_text)
                raise Exception(error_text) from ex

        # Override the base configuration with any specific values for this environment and server
        try:
            specific_qualifiers = [
                # Build from service name and domainQualifier
                "amlen"
            ]

            # Go through each of the possible qualifiers and pick up values from them.
            for specific_qualifier in specific_qualifiers:
                if specific_qualifier in config_dict:
                    self.logger.info("* Overriding configuration for %s with qualifier %s",
                                     self.server_name,
                                     specific_qualifier)
                    specific_config = config_dict[specific_qualifier]
                    for config_type in specific_config:
                        specific_config_type_info = specific_config[config_type]
                        if config_type in self.config:
                            config_type_info = self.config[config_type]
                            for specific_config_item in specific_config_type_info:
                                config_type_info[specific_config_item] = specific_config_type_info[specific_config_item]
                        else:
                            self.config[config_type] = specific_config_type_info
                else:
                    self.logger.info("* No override found for %s with qualifier %s",
                                     self.server_name,
                                     specific_qualifier)
        except Exception as ex:
            self.logger.warn("* Failed to determine specificQualifier for %s : %s",
                             self.server_name,
                             ex)

        try:
            # Upload certificates
            if len(self.files) == 0:
                self.read_files()
            for file in self.files:
                status_code, response = self.upload_file(file)
                if status_code in [200, 201, 204]:
                    self.logger.info("* Successfully uploaded %s" , file)
                else:
                    self.logger.info("* Failed to upload %s: %s (%s)" , file, status_code, response)
                    error_text = "* Could not upload all necessary certificates"
                    self.logger.error(error_text)
                    raise Exception(error_text)

            status_code, response = self.post_configuration_request(self.config)
            if status_code == 200:
                self.logger.info("* Configuration for %s updated successfully" , self.server_name)
            else:
                error_text = f"* Configuration for {self.server_name} was not updated: {status_code} ({response})"
                self.logger.error(error_text)
                raise Exception(error_text)

        except Exception as ex:
            error_text = f"Error ({sys.exc_info()[0]}): ex"
            self.logger.error(error_text)
            raise Exception(error_text) from ex

    def post_configuration_request(self, request, hide=False):
        auth = HTTPBasicAuth(self.username, self.password)
        headers={'content-type': 'application/json'}

        url = f"https://{self.server_name}:9089/ima/v1/configuration/"

        data = request if not hide else "hidden payload"
        self.logger.info("Attempting to connect to %s to post %s" , url, data)
        response = requests.post(url=url,
                                 data=json.dumps(request),
                                 headers=headers,
                                 auth=auth,
                                 verify=False)
        return response.status_code, response.content

    def configure_username_password(self):
        self.logger.info("Configuring admin endpoint")

        self.logger.info("First wait for admin endpoint to come up")
        status = self.get_status(wait=True)

        admin_sec_profile_input = {
            "AdminUserID": self.username,
            "AdminUserPassword": self.password
        }

        status_code, response_text = self.post_configuration_request(admin_sec_profile_input, hide=True)
        if status_code != 200:
            error_text = "Unexpected response code when configuring admin credentials for Amlen server %s: %s" % (self.server_name, status_code)
            self.logger.error(error_text)
            self.logger.error(response_text)
            raise Exception(error_text)

        self.logger.info("Username and password have been set")

    def cleanup_demo_entries(self):
        auth = HTTPBasicAuth(self.username, self.password)
        if self.is_standby_node(wait=False):
            return

        self.logger.info("Remove demo entries for MessageGateway server: %s" , self.server_name)

        try:
            url = f"https://{self.server_name}:9089/ima/v1/configuration/Endpoint/DemoEndpoint"
            requests.delete(url, auth=auth, verify=False)

            url = f"https://{self.server_name}:9089/ima/v1/configuration/Endpoint/DemoMqttEndpoint"
            requests.delete(url, auth=auth, verify=False)

            url = f"https://{self.server_name}:9089/ima/v1/configuration/MessageHub/DemoHub"
            requests.delete(url, auth=auth, verify=False)

            url = f"https://{self.server_name}:9089/ima/v1/configuration/ConnectionPolicy/DemoConnectionPolicy"
            requests.delete(url, auth=auth, verify=False)

            url = f"https://{self.server_name}:9089/ima/v1/configuration/TopicPolicy/DemoTopicPolicy"
            requests.delete(url, auth=auth, verify=False)

            url = f"https://{self.server_name}:9089/ima/v1/configuration/SubscriptionPolicy/DemoSubscriptionPolicy"
            requests.delete(url, auth=auth, verify=False)

            url = f"https://{self.server_name}:9089/ima/v1/configuration/Endpoint/IoTEndpoint0"
            requests.delete(url, auth=auth, verify=False)

        except:
            pass

    def configure_file_logging(self, path):
        common_file_name = os.getenv("LOG_FILE", f"{path}diag/logs/imaserver.log")

        syslog_input = {"Syslog":  {}}
        syslog_input["Syslog"]={
            "Host"    : "127.0.0.1",
            "Port"    : 514,
            "Protocol": "tcp",
            "Enabled" : False
        }
        file_location_input = {"LogLocation":  {}}
        file_location_input["LogLocation"]["DefaultLog"]={
            "LocationType" : "file",
            "Destination" : common_file_name
        }
        file_location_input["LogLocation"]["ConnectionLog"]={
            "LocationType" : "file",
            "Destination" : common_file_name
        }
        file_location_input["LogLocation"]["SecurityLog"]={
            "LocationType" : "file",
            "Destination" : common_file_name
        }
        file_location_input["LogLocation"]["AdminLog"]={
            "LocationType" : "file",
            "Destination" : common_file_name
        }
        file_location_input["LogLocation"]["MQConnectivityLog"]={
            "LocationType" : "file",
            "Destination" : common_file_name
        }

        file_enabled = False

        try:
            rc, content = self.post_configuration_request(file_location_input)
            if rc != 200:
                self.logger.warn("Unexpected response code when configuring file locations for Amlen server %s: %s (%s)" % (self.server_name, rc, content))
            else:
                rc, content = self.post_configuration_request(syslog_input)
                if rc != 200:
                    self.logger.warn("Unexpected response code when configuring file for Amlen server %s: %s (%s)" % (self.server_name, rc, content))
                else:
                    file_enabled = True
        except:
            error_text = "Unexpected exception when configuring file for Amlen server %s: %s" % (self.server_name, sys.exc_info()[0])
            self.logger.warn(error_text)

        if file_enabled:
            self.logger.info("File-based logging configured successfully for Amlen server %s" % (self.server_name))
        else:
            error_text = "File-based logging could not be configured for Amlen server %s" % (self.server_name)
            self.logger.error(error_text)
            raise Exception(error_text)

        self.cleanup_demo_entries()
        self.configure_iotf_policy()
        self.configure_admin_endpoint()

        return True

    def wait_for_restart(self, initial_uptime=0, delay=5, target_status=None):
        initial_server_start_time = 0

        if initial_uptime > 0:
            initial_server_start_time = int(time.time()) - initial_uptime
            self.logger.info("* Last server startup time was %s", initial_server_start_time)

        time.sleep(delay)

        server_name = self.server_name


        restarted = False

        # Wait for up to 5 minutes (60 iterations, 5 seconds each)
        for i in range(1, RESTART_ITERATIONS+1):
            try:
                current_status = self.get_status(wait=False)
                if current_status and current_status["Server"]["Status"] != "Stopping":
                    self.logger.info('Current status: ',json.dumps(current_status["Server"]))
                    new_server_start_time = int(time.time()) - current_status["Server"]["UpTimeSeconds"]
                    # Compare times allowing for rounding
                    if new_server_start_time > (initial_server_start_time + 1):
                        self.logger.info("* Server %s is back up (start time: %s)",
                            server_name, new_server_start_time)
                        restarted = True
                        break
            except:
                pass

            self.logger.info("* Still waiting for server %s to come up (%s/%s)",
                server_name, i, RESTART_ITERATIONS)
            time.sleep(delay)

        if restarted and target_status and current_status:
            # Wait for up to 5 minutes (60 iterations, 5 seconds each)
            for i in range(1, WAIT_FOR_MODE_ITERATIONS+1):
                try:
                    current_status = self.get_status(wait=False)
                    if current_status:
                        if target_status == current_status["Server"]["Status"]:
                            self.logger.info("* Server %s has reached status %s",
                                server_name, target_status)
                            break
                    else:
                        self.logger.info("* Current status not available (likely just rebooted)")
                        time.sleep(delay)
                        continue
                except:
                    pass

                self.logger.info(
                    '* Still waiting for server %s to reach target status - %s/%s (%s/%s)',
                    server_name,
                    target_status,
                    current_status["server"]["status"],
                    i,
                    WAIT_FOR_MODE_ITERATIONS
                )
                time.sleep(delay)

        if not restarted:
            error_text = f"Server {self.server_name} failed to restart in time"
            self.logger.error(error_text)
            raise Exception(error_text)
