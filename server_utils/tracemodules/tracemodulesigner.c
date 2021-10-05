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

//****************************************************************************
/// @file  tracemodulesigner.c
/// @brief Code for signing trace modules
//****************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <openssl/evp.h>

#define PRIV_KEY_PEM_PASSWORD "WhoopsADaisy"
#define EXPECTED_SIGNATURE_LENGTH 512

//#define DEBUG

//Gen keys with:
// openssl genrsa -des3 -out /tmp/privkey.pem 4096
// openssl rsa -in /tmp/privkey.pem -out /tmp/pubkey.pem -pubout -outform PEM

//Compile with gcc tracemodulesigner.c -o tracemodulesigner -lcrypto



//****************************************************************************
/// @brief Header at start of signed binary. Includes signature, signature
/// does not sign this header.
//****************************************************************************

#define SIGNEDFILEHEADER_STRUCTID "MSSB"
typedef struct tagSignedFileHeader {
	char    StrucID[4];                        // Will be MSSB (MessageSight Signed Binary)
	uint32_t Version;                          // Will be v1 at the mo - only version at least for now
	uint64_t signatureOffset;                  //Offset in file of start of signature
	uint64_t signatureLen;                     //Length of signature in bytes
	uint64_t signedOffset;                     //Offset in file of start of signed content
	uint64_t signedLength;                     //Length of signed content in bytes
}SignedFileHeader;

//****************************************************************************
/// @brief Header inside signed content. These fields are validated by the
/// signature. Binary follows this header and is the entire remainder of signed
//  content.
//****************************************************************************
#define SIGNEDCONTENTHEADER_STRUCTID "MSSC"
typedef struct tagSignedContentHeader {
	char    StrucID[4];                        //Will be MSSC (MessageSight Signed Content)
	uint32_t Version;                          //Will be v1 at the mo - only version at least for now
	char     SerialNumber[64];                 //Null-terminated serial number
	uint64_t installDeadline;                  //Epoch time the module must be installed by
	uint64_t binaryOffset;                     //Offset in file of start of binary
	uint64_t binaryLength;                     //Length of binary in bytes.
}SignedContentHeader;

int main(int argc, char **argv)
{
	int rc =0;

	char *privkeyfilename     = NULL;
	char *tracemodulefilename = NULL;
	char *outputfilename      = NULL;
	char *applianceserial    = "";
	uint64_t installByTime    = 0;

	if( argc < 4) {
		fprintf(stderr, "Usage: %s <pem-privkeyfile> <tracemodule> <outputfilename> [serial number of appliance] [days before the module must be installed]\n", argv[0]);
		return 10;
	}
	privkeyfilename     = *++argv;
	tracemodulefilename = *++argv;
	outputfilename      = *++argv;

	if (argc >= 5) {
		//User has requested this binary is only run on a certain appliance (serial number as shown by "show version" command
		applianceserial = *++argv;

		if (strlen(applianceserial) >= 64) {
			fprintf(stderr, "The serial number %s is too long for this program to store in the signed binary. (It must correspond to serial number shown in \"show version\" command)\n",applianceserial);
			return 20;
		}
	}

	if (argc >= 6) {
		//User has requested we include a timestamp that prevents installation after this time...
		uint64_t numDays = strtol(*++argv, NULL, 10);

		if (errno != 0 || numDays == 0) {
			fprintf(stderr, "Invalid number of days: %s\n", *argv);
			return 20;
		}
		installByTime = time(NULL) + (24 *60 * 60 * numDays);
	}

	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_digests();

	EVP_MD_CTX *mdctx= NULL;
	mdctx = EVP_MD_CTX_create();

	if (mdctx == NULL) {
		fprintf(stderr, "Could not create digest context\n");
		ERR_print_errors_fp(stderr);
		return 20;
	}

	//Open the private key file...
	FILE *privkeyfp = NULL;

	if(!(privkeyfp = fopen(privkeyfilename, "r"))) {
		fprintf(stderr, "Can't open key file %s for reading.\n", privkeyfilename);
		return 30;
	}

	//Read the private key from the file
	EVP_PKEY *priv_key = NULL;
	if (!PEM_read_PrivateKey(privkeyfp, &priv_key, NULL, PRIV_KEY_PEM_PASSWORD )) {
		fprintf(stderr, "Error parsing Private Key File.\n");
		ERR_print_errors_fp(stderr);
		return 40;
	}

	//Setup the digesting&signing operation...
	if (1 != EVP_DigestSignInit(mdctx, NULL, EVP_sha512(), NULL, priv_key)) {
		fprintf(stderr, "Error initialising signing operation\n");
		ERR_print_errors_fp(stderr);
		return 50;
	}



	//Open the trace module for reading...
	FILE *tracemodulefp = NULL;
	if(!(tracemodulefp = fopen(tracemodulefilename, "r"))) {
		fprintf(stderr, "Can't open trace module %s for reading.\n", tracemodulefilename);
		return 60;
	}

	//Work out length of trace module...
	fseek(tracemodulefp, 0, SEEK_END);
	long binaryLength = ftell(tracemodulefp);
	fseek(tracemodulefp, 0, SEEK_SET);

	//Setup the Header at start of signed content:
	SignedContentHeader contentHeader = {SIGNEDCONTENTHEADER_STRUCTID, 1};
	contentHeader.installDeadline = installByTime;
	strcpy(contentHeader.SerialNumber, applianceserial);
	contentHeader.binaryOffset = sizeof(SignedFileHeader) + EXPECTED_SIGNATURE_LENGTH + sizeof(SignedContentHeader);
	contentHeader.binaryLength = binaryLength;

	if (1 != EVP_DigestSignUpdate(mdctx, &contentHeader,sizeof(SignedContentHeader))) {
		fprintf(stderr, "Error digesting/signing signed content header\n");
		ERR_print_errors_fp(stderr);
		return 70;
	}

	size_t totalBytesRead = 0;

	while(!feof(tracemodulefp)) {
		//Read & digest file 4KiB at a time
		size_t bufsize = 4*1024;
		char buff[bufsize];
		size_t bytesRead = fread(buff, 1, bufsize, tracemodulefp);

		//Update the digest with the latest bytes
		if (1 != EVP_DigestSignUpdate(mdctx, buff,bytesRead)) {
			fprintf(stderr, "Error digesting/signing (after %u bytes done successfully)\n", totalBytesRead);
			ERR_print_errors_fp(stderr);
			return 70;
		}
		totalBytesRead += bytesRead;
	}

	if (totalBytesRead != binaryLength) {
		fprintf(stderr, "Internal error: Could only read %u bytes from %s but initially we calculated size as was %u bytes)\n", totalBytesRead, tracemodulefilename, binaryLength);
		return 80;
	}


	size_t siglenbufsize = 10240;
	unsigned char sigbuf[siglenbufsize];
	size_t siglen = siglenbufsize;

	if (1 != EVP_DigestSignFinal(mdctx, sigbuf, &siglen)) {
		fprintf(stderr, "Error finalising signature...\n");
		ERR_print_errors_fp(stderr);
		return 90;
	}

#ifdef DEBUG
	printf("Signature (length %u): ", siglen);
	uint sigchar;
	for(sigchar = 0; sigchar < siglen; sigchar++) printf("%02x", sigbuf[sigchar]);
	printf("\n");
#endif

	if(siglen != EXPECTED_SIGNATURE_LENGTH) {
		fprintf(stderr, "Error signature is not expected size for sha512\n");
		return 100;
	}

     EVP_PKEY_free(priv_key);
	 EVP_MD_CTX_destroy(mdctx);

	 //Open the output file...
	 FILE *outputfp = NULL;
	 if(!(outputfp = fopen(outputfilename, "w"))) {
		 fprintf(stderr, "Can't open output file %s for writing.\n", outputfilename);
		 return 100;
	 }

	 //We'll write the output file with the header, signature, binarybytes consecutively in that order so create the header to say so...

	 SignedFileHeader fileHeader = {SIGNEDFILEHEADER_STRUCTID,            //Identifier that this is a MessageSight signed binary file
			                        1,                                    //Version 1 of this header type,
			                        sizeof(SignedFileHeader),             //signature starts after the fileheader,
			                        siglen,                               //length of the signature,
			                        sizeof(SignedFileHeader) + siglen,    //signed content starts after header and signature
			                        sizeof(SignedContentHeader) + totalBytesRead  //Content header + Number of bytes in the shared object that we read during signing
	                                };
	 if (1 != fwrite((void *)&fileHeader, sizeof(fileHeader), 1, outputfp)) {
		 fprintf(stderr, "Failed to write signed file header to %s.\n", outputfilename);
		 return 110;
	 }

	 if (1 != fwrite(sigbuf, siglen, 1, outputfp)) {
		 fprintf(stderr, "Failed to write signature (length %u)  to %s.\n", siglen, outputfilename);
		 return 120;
	 }

	 if (1 != fwrite(&contentHeader, sizeof(SignedContentHeader), 1, outputfp)) {
		 fprintf(stderr, "Failed to write signed content header (length %u)  to %s.\n", sizeof(SignedContentHeader), outputfilename);
		 return 120;
	 }
#ifdef DEBUG
	//print out contentHeader:
	printf("ContentHeader: structid: %.*s version: %u, installbyepoch: %lu, appliance: %s, binaryLength: %lu binaryoffset: %lu\n",
			 4, contentHeader.StrucID, contentHeader.Version, contentHeader.installDeadline, contentHeader.SerialNumber, contentHeader.binaryLength, contentHeader.binaryOffset);
	uint printedBytes=0;
#endif

	 //Copy the shared object into the file...
	 fseek(tracemodulefp, 0L, SEEK_SET);
	 size_t bytesToCopy = totalBytesRead; //We need to copy the number of bytes we signed...

	 while (bytesToCopy > 0) {
		size_t chunkSize = 4 *1024;
		char chunkBuf[chunkSize];

		if(bytesToCopy < chunkSize) {
			chunkSize = bytesToCopy;
		}

		if (1 != fread(chunkBuf, chunkSize, 1, tracemodulefp)) {
			fprintf(stderr, "Failed to read tracemodule during copying (though we just read it during signing.\n");
			return 130;
		}
#ifdef DEBUG
		if (printedBytes == 0) {
			uint bytesToPrint = 64;
			if (bytesToPrint > chunkSize) {
				bytesToPrint = chunkSize;
			}
			printf("First %u bytes of binary: ", bytesToPrint);
			uint curpos;
			for(curpos = 0; curpos < bytesToPrint; curpos++) printf("%02x", chunkBuf[curpos]);
			printf("\n");
			printedBytes += chunkSize;
		}
#endif

		if (1 != fwrite(chunkBuf, chunkSize, 1, outputfp)) {
			fprintf(stderr, "Failed to write tracemodule to %s.\n", outputfilename);
			return 140;
		}

		bytesToCopy -= chunkSize;
	 }

	 fclose(tracemodulefp);
	 fclose(outputfp);

	 printf("Successfully created signed file: %s\n", outputfilename);
	 return 0;
}
