/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */

#include <ismutil.h>
#include <signal.h>
#include <transport.h>
static char * server = NULL;
static char * ports = NULL;
static char * trcfile = NULL;
static int    trclvl = -1;
static void process_args(int argc, char * * argv);


static int ism_process_adminRequest(const char *inpbuf, int inpbuflen, const char * locale, concat_alloc_t *output_buffer, int *rc){
	fprintf(stderr,"%s\n", inpbuf);
	ism_common_allocBufferCopy(output_buffer, "{\"rc\": 0, \"reply\": \"OK\"}");
	*rc = 0;
	return 0;
}
extern void ism_common_initTrace();
void ism_engine_threadInit(uint8_t isStoreCrit)
{
}
int ism_process_monitoring_action(ism_transport_t *transport, const char *inpbuf, int buflen, concat_alloc_t *output_buffer, int *rc)
{
	return 0;
}
int main(int argc, char * * argv) {
    int    port;
    ism_simpleServer_t configServer;
    process_args(argc, argv);
    if (!server)
        server = "127.0.0.1";
    if (!ports)
        ports = "9086";
    port = atoi(ports);
    if (!port)
        port = 9086;

    if (!strcmpi(server, "any"))
        server = NULL;

    /* Init utils */
    ism_common_initUtil();

    /* Override trace level */
    if (trclvl != -1) {
        ism_field_t f;
        f.type = VT_Integer;
        f.val.i = trclvl;
        ism_common_setProperty( ism_common_getConfigProperties(), "TraceLevel", &f);
    }

    /* Override trace file */
    if (trcfile) {
        ism_field_t f;
        f.type = VT_String;
        f.val.s = trcfile;
        ism_common_setProperty( ism_common_getConfigProperties(), "TraceFile", &f);
    }

    ism_common_initTrace();
    TRACE(1, "Initialize the server\n");
    TRACE(2, "version   = %s %s %s\n", ism_common_getVersion(), ism_common_getBuildLabel(), ism_common_getBuildTime());
    TRACE(2, "platform  = %s %s\n", ism_common_getPlatformInfo(), ism_common_getKernelInfo());
    TRACE(2, "processor = %s\n", ism_common_getProcessorInfo());


    /*Ignore SIGPIPE signals for broken TCP connections.*/
    sigignore(SIGPIPE);

    configServer = ism_common_simpleServer_start("/tmp/.com.ibm.ima.mqcAdmin", ism_process_adminRequest, NULL, NULL);
    if (configServer)
        pause();
}


/*
 * Process args
 */
static void process_args(int argc, char * * argv) {
    xUNUSED int config_found = 1;
    int  i;

    for (i=1; i<argc; i++) {
        char * argp = argv[i];
        if (argp[0]=='-') {
            /* -0 to -9 test trace level */
            if (argp[1] >='0' && argp[1]<='9') {
                trclvl = argp[1]-'0';
            }

            /* -h -H -? = syntax help */
            else if (argp[1] == 'h' || argp[1]=='H' || argp[1]=='?' ) {
                config_found = 0;
                break;
            }

            /* -t = trace file */
            else if (argp[1] == 't') {
                if (i+1 < argc) {
                    argp = argv[++i];
                    trcfile = argp;
                }
            }

            /* Anything else is an error */
            else {
                fprintf(stderr, "Unknown switch found: %s\n", argp);
                config_found = 0;
                break;
            }
        } else {
            /* The first positional arg is the config file */
            if (server == NULL) {
                server = argp;
            } else {
                if (ports == NULL) {
                    ports = argp;
                } else {
                    fprintf(stderr, "Extra parameter found: %s\n", argp);
                }
            }
        }
    }
}

