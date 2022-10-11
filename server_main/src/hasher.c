/*
 * Copyright (c) 2015-2022 Contributors to the Eclipse Foundation
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
/**
 * This application will compare a password against the encoding stored
 * in the config which is of the form _<version>:<salt>:<hash>
 */

#include <string.h>

#include <stdio.h>
#include <ismutil.h>
#include <config.h>
#include <sasl_scram.h>
#include <openssl/rand.h>

#include <stdlib.h>
#include <sys/time.h>

extern bool ism_config_confirmAdminUserPassword2(char * password,char * encoding);
extern char * ism_security_createAdminUserPasswordHash(const char * password);

void printHelp(void) {
  printf("Syntax:\n\n");
  printf("    hasher -p <password> -e <encoding> [-s]\n");
  printf("\nCheck if the password matches the encoding\n");
  printf("\nSupports original messagesight encoding and the new SHA512 hash format\n");
  printf("\nOriginal messagesight encoding is a 32 character alphanumeric encryption\n");
  printf("\nnew SHA512 hash format is _<version>:<salt>:<hash>\n");
  printf("\nif running silent (-s) then nothing is printed rc=0 is match, 1 is mismatch, 255 is error\n");
  printf("    hasher -c <newpassword> [-s]\n");
  printf("\nCreate a new encoding for the given password\n");
  printf("\nif running silent (-s) then only the new encoding will be printed\n");
}

int main(int argc, char** argv)
{
  char * password = NULL;
  char * encoding = NULL;
  char * newpassword = NULL;
  bool silent = false;

  int i=0;

  // Parse the arguments -
  for (i=1; i<argc; i++)
  {
    // Check this is a valid argument
    if (strlen(argv[i]) == 2 && argv[i][0] == '-')
    {
      char arg = argv[i][1];
      if (arg == 'h' || arg == '?')
      {
        printHelp();
        return 255;
      }

      if (arg == 's')
      {
        silent = true;
        continue;
      }

      // Validate there is a value associated with the argument
      if (i == argc - 1 || argv[i+1][0] == '-')
      {
        printf("Missing value for argument: %s\n", argv[i]);
        printHelp();
        return 255;
      }

      switch(arg)
      {
        case 'p': password = argv[++i]; break;
        case 'e': encoding = argv[++i]; break;
        case 'c': newpassword = argv[++i]; break;
        default:
          printf("Unrecognised argument: %s\n", argv[i]);
          printHelp();
          return 255;
      }
    }
    else
    {
      printf("Unrecognised argument: %s\n", argv[i]);
      printHelp();
      return 255;
    }
  }

  if ( newpassword == NULL ) {
    if ( password == NULL || encoding == NULL ) 
    {
      printHelp();
      return 255;
    }

    if (!silent) {printf("checking password %s against encoding %s\n",password,encoding);}

    if ( ism_config_confirmAdminUserPassword2(password,encoding) ) 
    {
      if (!silent) {printf("Password matches\n");}
      return 0;
    } 
    else 
    {
      if (!silent) {printf("Password does not match\n");}
      return 1;
    } 
  }
  else {
    if ( password != NULL || encoding != NULL )
    {
      printHelp();
      return 255;
    }

    if (!silent) {printf("Creating password encoding:\n");}

    char * pass = ism_security_createAdminUserPasswordHash(newpassword);
    printf("%s\n",pass);
    return 0;
  }
}
