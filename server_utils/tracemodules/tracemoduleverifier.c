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
/// @file  tracemoduleverifier.c
/// @brief Code for verifying trace modules were signed by IBM
//****************************************************************************

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>

#define EXPECTED_SIGNATURE_LENGTH 512

//#define DEBUG

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

	char *pubkeyfilename      = NULL;
	char *signedtracemodulefilename = NULL;
	char *outputfilename      = NULL;

	if( argc < 4) {
		fprintf(stderr, "Usage: %s <pem-pubkeyfile> <signedtracemodule> <outputfilename>\n", argv[0]);
		return 10;
	}
	pubkeyfilename            = *++argv;
	signedtracemodulefilename = *++argv;
	outputfilename            = *++argv;

	OpenSSL_add_all_algorithms();
	OpenSSL_add_all_digests();

	EVP_MD_CTX *mdctx= NULL;
	mdctx = EVP_MD_CTX_create();

	if (mdctx == NULL) {
		fprintf(stderr, "Could not create digest context\n");
		ERR_print_errors_fp(stderr);
		return 20;
	}

	//Open the public key file...
	FILE *publickeyfp = NULL;

	if(!(publickeyfp = fopen(pubkeyfilename, "r"))) {
		fprintf(stderr, "Can't open key file %s for reading.\n", pubkeyfilename);
		return 30;
	}

	//Read the public key from the file
	EVP_PKEY *pub_key = NULL;
	if (!PEM_read_PUBKEY(publickeyfp, &pub_key, NULL, NULL )) {
		fprintf(stderr, "Error parsing Public Key File.\n");
		ERR_print_errors_fp(stderr);
		return 40;
	}

	//Setup the digesting&signing operation...
	if (1 != EVP_DigestVerifyInit(mdctx, NULL, EVP_sha512(), NULL, pub_key)) {
		fprintf(stderr, "Error initialising verify operation\n");
		ERR_print_errors_fp(stderr);
		return 50;
	}

	//Open the signed trace module for reading...
	FILE *signedtracemodulefp = NULL;
	if(!(signedtracemodulefp = fopen(signedtracemodulefilename, "r"))) {
		fprintf(stderr, "Can't open signed trace module %s for reading.\n", signedtracemodulefilename);
		return 60;
	}

	//Read the signed module header:
	SignedFileHeader fileHeader = {0};

	if (1 != fread(&fileHeader, sizeof(SignedFileHeader), 1, signedtracemodulefp)) {
		fprintf(stderr, "Can't open signed trace module %s for reading.\n", signedtracemodulefilename);
	    return 70;
	}

	if(memcmp(fileHeader.StrucID, SIGNEDFILEHEADER_STRUCTID, 4) != 0) {
		fprintf(stderr, "File %s does not appear to be a signed trace module.\n", signedtracemodulefilename);
		return 80;
	}

	if(fileHeader.Version != 1) {
		fprintf(stderr, "File %s appears to be a version %u  signed trace module and we only support v1.\n",
				                                 signedtracemodulefilename, fileHeader.Version );
		return 90;
	}

	if(fileHeader.signatureLen != EXPECTED_SIGNATURE_LENGTH) {
		fprintf(stderr, "File %s appears have a signature of length %u, expected %u\n",
						   signedtracemodulefilename, fileHeader.signatureLen, EXPECTED_SIGNATURE_LENGTH );
				return 90;
	}

	//Read the signature...
	unsigned char *signature = ism_common_malloc(ISM_MEM_PROBE(ism_memory_utils_misc,1),fileHeader.signatureLen);

	if (signature == NULL) {
		fprintf(stderr, "Couldn't allocate %u bytes for signature.", fileHeader.signatureLen);
		return 100;
	}

	fseek(signedtracemodulefp, fileHeader.signatureOffset, SEEK_SET);
	if (1 != fread(signature, fileHeader.signatureLen, 1, signedtracemodulefp)) {
		fprintf(stderr, "Can't read signature (%u bytes) from %s.\n", fileHeader.signatureLen, signedtracemodulefilename);
		return 110;
	}

#ifdef DEBUG
	//print out signature
	printf("Signature (length %u): ", fileHeader.signatureLen);
    int sigchar;
	for(sigchar = 0; sigchar < fileHeader.signatureLen; sigchar++) printf("%02x", signature[sigchar]);
	printf("\n");
#endif


	//Read the header of the signed content
	SignedContentHeader contentHeader = {0};
	fseek(signedtracemodulefp, fileHeader.signedOffset, SEEK_SET);

	if (1 != fread(&contentHeader, sizeof(SignedContentHeader), 1, signedtracemodulefp)) {
		fprintf(stderr, "Can't read content header from %s.\n",  signedtracemodulefilename);
		return 120;
	}

#ifdef DEBUG
	//print out contentHeader:
	printf("ContentHeader: structid: %.*s version: %u, installbyepoch: %lu, appliance: %s, binaryLength: %lu binaryoffset: %lu\n",
			 4, contentHeader.StrucID, contentHeader.Version, contentHeader.installDeadline, contentHeader.SerialNumber, contentHeader.binaryLength, contentHeader.binaryOffset);
	uint printedBytes=0;
#endif
	//digest needs to include the content header
	if (1 != EVP_DigestVerifyUpdate(mdctx, &contentHeader, sizeof(SignedContentHeader))) {
		fprintf(stderr, "Error digesting/verifying signed content header)\n");
		ERR_print_errors_fp(stderr);
		return 130;
	}

	//Read the signed library
	fseek(signedtracemodulefp, contentHeader.binaryOffset, SEEK_SET);
	size_t totalBytesToRead = contentHeader.binaryLength;

	while(totalBytesToRead > 0) {
		//Read file 4KiB at a time
		size_t bufsize = 4*1024;
		char buff[bufsize];

		size_t chunkSize = bufsize;

		if(totalBytesToRead < bufsize) {
			chunkSize = totalBytesToRead;
		}

		if (1 != fread(buff, chunkSize, 1, signedtracemodulefp)) {
			fprintf(stderr, "Can't read binary data from %s.\n",  signedtracemodulefilename);
			return 120;
		}

#ifdef DEBUG
		if (printedBytes == 0) {
			uint bytesToPrint = 64;
			if (bytesToPrint > chunkSize) {
				bytesToPrint = chunkSize;
			}
			printf("First %u bytes of binary: ", bytesToPrint);
			uint curpos;
			for(curpos = 0; curpos < bytesToPrint; curpos++) printf("%02x", buff[curpos]);
			printf("\n");
			printedBytes += chunkSize;
		}
#endif

		//Update the digest with the latest bytes
		if (1 != EVP_DigestVerifyUpdate(mdctx, buff, chunkSize)) {
			fprintf(stderr, "Error digesting/verifying (with %u bytes to go)\n", totalBytesToRead);
			ERR_print_errors_fp(stderr);
			return 130;
		}
		totalBytesToRead -= chunkSize;
	}

	if (1 != EVP_DigestVerifyFinal(mdctx, signature, fileHeader.signatureLen)) {
		fprintf(stderr, "Signature doesn't match binary or signed with a different key...\n");
		ERR_print_errors_fp(stderr);
		return 140;
	}

     EVP_PKEY_free(pub_key);
	 EVP_MD_CTX_destroy(mdctx);
	 //From here on we can trust the contentHeader and binary contents...


	 //If the serial number or datestamp checks are present do them now...
	 if ((contentHeader.installDeadline != 0) && (contentHeader.installDeadline <= time(NULL))) {
#ifdef DEBUG
         printf("Module expired at %lu, time now %lu\n",contentHeader.installDeadline, time(NULL));
#endif
		 fprintf(stderr, "This module has expired and is no longer allowed to be installed\n");
		 return 150;
	 }
	 if (strlen(contentHeader.SerialNumber) != 0) {
		 fprintf(stderr, "This module can only be used on appliance with serial number: %s and this is not an appliance.\n", contentHeader.SerialNumber);
		 return 160;
	 }
	 //Open the output file...
	 FILE *outputfp = NULL;
	 if(!(outputfp = fopen(outputfilename, "w"))) {
		 fprintf(stderr, "Can't open output file %s for writing.\n", outputfilename);
		 return 150;
	 }


	 //Copy the shared object into the file...
	 fseek(signedtracemodulefp, contentHeader.binaryOffset, SEEK_SET);
	 size_t bytesToCopy = contentHeader.binaryLength; //We need to copy the number of bytes we signed...

	 while (bytesToCopy > 0) {
		size_t chunkSize = 4 *1024;
		char chunkBuf[chunkSize];

		if(bytesToCopy < chunkSize) {
			chunkSize = bytesToCopy;
		}

		if (1 != fread(chunkBuf, chunkSize, 1, signedtracemodulefp)) {
			fprintf(stderr, "Failed to read tracemodule during copying (though we just read it during signature verification.\n");
			return 160;
		}

		if (1 != fwrite(chunkBuf, chunkSize, 1, outputfp)) {
			fprintf(stderr, "Failed to write tracemodule to %s.\n", outputfilename);
			return 170;
		}

		bytesToCopy -= chunkSize;
	 }

	 fclose(signedtracemodulefp);
	 fclose(outputfp);
	 
	 printf("Successfully verified file and wrote output to %s\n", outputfilename);
	 return 0;
}
