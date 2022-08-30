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

void printHelp(void) {
  printf("Syntax:\n\n");
  printf("    hasher -p <password> -e <encoding>\n");
  printf("\nCheck if the password matches the encoding\n");
  printf("\nSupports original messagesight encoding and the new SHA512 hash format\n");
  printf("\nOriginal messagesight encoding is a 32 character alphanumeric encryption\n");
  printf("\nnew SHA512 hash format is _<version>:<salt>:<hash>\n");
}

int main(int argc, char** argv)
{
  char * password = NULL;
  char * encoding = NULL;

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

  if ( password == NULL || encoding == NULL ) 
  {
    printHelp();
    return 255;
  }

  printf("checking password %s against encoding %s\n",password,encoding);

  if ( ism_config_confirmAdminUserPassword2(password,encoding) ) 
  {
    printf("Password matches\n");
    return 0;
  } 
  else 
  {
    printf("Password does not match\n");
    return 1;
  } 
}
