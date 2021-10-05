// Copyright (c) 2010-2021 Contributors to the Eclipse Foundation
// 
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
// 
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0
// 
// SPDX-License-Identifier: EPL-2.0
//
/*
 * Sha1.cpp
 *
 *  Created on: 28/01/2010
 *
 *----------------------------------------------------------------------------
 * This implementation is based on RFC3174, see:
 * http://tools.ietf.org/html/rfc3174,
 * which carries the following statement:
 *----------------------------------------------------------------------------
 * Full Copyright Statement
 *
 *  Copyright (C) The Internet Society (2001).  All Rights Reserved.
 *
 *   This document and translations of it may be copied and furnished to
 *   others, and derivative works that comment on or otherwise explain it
 *   or assist in its implementation may be prepared, copied, published
 *   and distributed, in whole or in part, without restriction of any
 *   kind, provided that the above copyright notice and this paragraph are
 *   included on all such copies and derivative works.  However, this
 *   document itself may not be modified in any way, such as by removing
 *   the copyright notice or references to the Internet Society or other
 *   Internet organizations, except as needed for the purpose of
 *   developing Internet standards in which case the procedures for
 *   copyrights defined in the Internet Standards process must be
 *   followed, or as required to translate it into languages other than
 *   English.
 *
 *   The limited permissions granted above are perpetual and will not be
 *   revoked by the Internet Society or its successors or assigns.
 *
 *   This document and the information contained herein is provided on an
 *   "AS IS" basis and THE INTERNET SOCIETY AND THE INTERNET ENGINEERING
 *   TASK FORCE DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, INCLUDING
 *   BUT NOT LIMITED TO ANY WARRANTY THAT THE USE OF THE INFORMATION
 *   HEREIN WILL NOT INFRINGE ANY RIGHTS OR ANY IMPLIED WARRANTIES OF
 *   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *----------------------------------------------------------------------------
 */

#include "SHA1.h"

namespace spdr
{
namespace util
{

/*
 * Constructor
 */
SHA1::SHA1()
{
	reset();
}

/*
 * Destructor
 */
SHA1::~SHA1()
{
	// The destructor does nothing
}

/*
 *  reset
 *
 *  Description:
 *      This function will initialize the sha1 class member variables
 *      in preparation for computing a new message digest.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1::reset()
{
	Length_Low = 0;
	Length_High = 0;
	Message_Block_Index = 0;

	H[0] = 0x67452301;
	H[1] = 0xEFCDAB89;
	H[2] = 0x98BADCFE;
	H[3] = 0x10325476;
	H[4] = 0xC3D2E1F0;

	Computed = false;
	Corrupted = false;
}

bool SHA1::digest(uint32_t *message_digest_array)
{
	int i; // Counter

	if (Corrupted)
	{
		return false;
	}

	if (!Computed)
	{
		padMessage();
		Computed = true;
	}

	for (i = 0; i < 5; i++)
	{
		message_digest_array[i] = H[i];
	}

	return true;
}

bool SHA1::digest(char *message_digest_array)
{
	int i; // Counter

	if (Corrupted)
	{
		return false;
	}

	if (!Computed)
	{
		padMessage();
		Computed = true;
	}

	for (i = 0; i < 5; i++)
	{
		message_digest_array[i]   = (H[i] >> 24) & 0xFF;
		message_digest_array[i+1] = (H[i] >> 16) & 0xFF;
		message_digest_array[i+2] = (H[i] >> 8) & 0xFF;
		message_digest_array[i+3] = (H[i]) & 0xFF;
	}

	return true;
}

/*
 *  update
 *
 *  Description:
 *      This function accepts an array of octets as the next portion of
 *      the message.
 *
 *  Parameters:
 *      message_array: [in]
 *          An array of characters representing the next portion of the
 *          message.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1::update(const unsigned char *message_array, uint32_t length)
{
	if (!length)
	{
		return;
	}

	if (Computed || Corrupted)
	{
		Corrupted = true;
		throw IllegalStateException("Corrupted SHA1 buffer, reset before further updates.");
	}

	while (length-- && !Corrupted)
	{
		Message_Block[Message_Block_Index++] = (*message_array & 0xFF);

		Length_Low += 8;
		Length_Low &= 0xFFFFFFFF; // Force it to 32 bits //done explicitly by using uint32
		if (Length_Low == 0)
		{
			Length_High++;
			Length_High &= 0xFFFFFFFF; // Force it to 32 bits
			if (Length_High == 0)
			{
				Corrupted = true; // Message is too long
			}
		}

		if (Message_Block_Index == 64)
		{
			processMessageBlock();
		}

		message_array++;
	}
}

/*
 *  update
 *
 *  Description:
 *      This function accepts an array of octets as the next portion of
 *      the message.
 *
 *  Parameters:
 *      message_array: [in]
 *          An array of characters representing the next portion of the
 *          message.
 *      length: [in]
 *          The length of the message_array
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1::update(const char *message_array, uint32_t length)
{
	update((unsigned char *) message_array, length);
}

/*
 *  update
 *
 *  Description:
 *      This function accepts a single octets as the next message element.
 *
 *  Parameters:
 *      message_element: [in]
 *          The next octet in the message.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1::update(unsigned char message_element)
{
	update(&message_element, 1);
}

/*
 *  update
 *
 *  Description:
 *      This function accepts a single octet as the next message element.
 *
 *  Parameters:
 *      message_element: [in]
 *          The next octet in the message.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1::update(char message_element)
{
	update((unsigned char *) &message_element, 1);
}

void SHA1::update(const String& message)
{
	update(message.data(), message.size());
}

/*
 *  operator<<
 *
 *  Description:
 *      This operator makes it convenient to provide character strings to
 *      the SHA1 object for processing.
 *
 *  Parameters:
 *      message_array: [in]
 *          The character array to take as input, a C-string (null terminated).
 *
 *  Returns:
 *      A reference to the SHA1 object.
 *
 *  Comments:
 *      Each character is assumed to hold 8 bits of information.
 *
 */
SHA1& SHA1::operator<<(const char *message_array)
{
	const char *p = message_array;

	while (*p)
	{
		update(*p);
		p++;
	}

	return *this;
}

SHA1& SHA1::operator<<(const unsigned char *message_array)
{
	const unsigned char *p = message_array;

	while (*p)
	{
		update(*p);
		p++;
	}

	return *this;
}

SHA1& SHA1::operator<<(const char message_element)
{
	update((unsigned char *) &message_element, 1);

	return *this;
}


SHA1& SHA1::operator<<(const unsigned char message_element)
{
	update(&message_element, 1);

	return *this;
}

std::string SHA1::digestToHexString(const uint32_t* digest_array)
{
	std::ostringstream oss;
	oss << std::hex << std::uppercase;
	for (int i=0; i<5; ++i)
	{
		oss << digest_array[i];
		if (i<4)
		{
			oss << ':';
		}
	}
	return oss.str();
}

/*
 *  processMessageBlock
 *
 *  Description:
 *      This function will process the next 512 bits of the message
 *      stored in the Message_Block array.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *      Many of the variable names in this function, especially the single
 *      character names, were used because those were the names used
 *      in the publication.
 *
 */
void SHA1::processMessageBlock()
{
	const uint32_t K[] =
	{ // Constants defined for SHA-1
			0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6 };
	int32_t t; // Loop counter
	uint32_t temp; // Temporary word value
	uint32_t W[80]; // Word sequence
	uint32_t A, B, C, D, E; // Word buffers

	/*
	 *  Initialize the first 16 words in the array W
	 */
	for (t = 0; t < 16; t++)
	{
		W[t] = ((uint32_t) Message_Block[t * 4]) << 24;
		W[t] |= ((uint32_t) Message_Block[t * 4 + 1]) << 16;
		W[t] |= ((uint32_t) Message_Block[t * 4 + 2]) << 8;
		W[t] |= ((uint32_t) Message_Block[t * 4 + 3]);
	}

	for (t = 16; t < 80; t++)
	{
		W[t] = circularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
	}

	A = H[0];
	B = H[1];
	C = H[2];
	D = H[3];
	E = H[4];

	for (t = 0; t < 20; t++)
	{
		temp = circularShift(5, A) + ((B & C) | ((~B) & D)) + E + W[t] + K[0];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circularShift(30, B);
		B = A;
		A = temp;
	}

	for (t = 20; t < 40; t++)
	{
		temp = circularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circularShift(30, B);
		B = A;
		A = temp;
	}

	for (t = 40; t < 60; t++)
	{
		temp = circularShift(5, A) + ((B & C) | (B & D) | (C & D)) + E + W[t]
				+ K[2];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circularShift(30, B);
		B = A;
		A = temp;
	}

	for (t = 60; t < 80; t++)
	{
		temp = circularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
		temp &= 0xFFFFFFFF;
		E = D;
		D = C;
		C = circularShift(30, B);
		B = A;
		A = temp;
	}

	H[0] = (H[0] + A) & 0xFFFFFFFF;
	H[1] = (H[1] + B) & 0xFFFFFFFF;
	H[2] = (H[2] + C) & 0xFFFFFFFF;
	H[3] = (H[3] + D) & 0xFFFFFFFF;
	H[4] = (H[4] + E) & 0xFFFFFFFF;

	Message_Block_Index = 0;
}

/*
 *  padMessage
 *
 *  Description:
 *      According to the standard, the message must be padded to an even
 *      512 bits.  The first padding bit must be a '1'.  The last 64 bits
 *      represent the length of the original message.  All bits in between
 *      should be 0.  This function will pad the message according to those
 *      rules by filling the message_block array accordingly.  It will also
 *      call processMessageBlock() appropriately.  When it returns, it
 *      can be assumed that the message digest has been computed.
 *
 *  Parameters:
 *      None.
 *
 *  Returns:
 *      Nothing.
 *
 *  Comments:
 *
 */
void SHA1::padMessage()
{
	/*
	 *  Check to see if the current message block is too small to hold
	 *  the initial padding bits and length.  If so, we will pad the
	 *  block, process it, and then continue padding into a second block.
	 */
	if (Message_Block_Index > 55)
	{
		Message_Block[Message_Block_Index++] = 0x80;
		while (Message_Block_Index < 64)
		{
			Message_Block[Message_Block_Index++] = 0;
		}

		processMessageBlock();

		while (Message_Block_Index < 56)
		{
			Message_Block[Message_Block_Index++] = 0;
		}
	}
	else
	{
		Message_Block[Message_Block_Index++] = 0x80;
		while (Message_Block_Index < 56)
		{
			Message_Block[Message_Block_Index++] = 0;
		}

	}

	/*
	 *  Store the message length as the last 8 octets, big endian
	 */
	Message_Block[56] = (Length_High >> 24) & 0xFF;
	Message_Block[57] = (Length_High >> 16) & 0xFF;
	Message_Block[58] = (Length_High >> 8) & 0xFF;
	Message_Block[59] = (Length_High) & 0xFF;
	Message_Block[60] = (Length_Low >> 24) & 0xFF;
	Message_Block[61] = (Length_Low >> 16) & 0xFF;
	Message_Block[62] = (Length_Low >> 8) & 0xFF;
	Message_Block[63] = (Length_Low) & 0xFF;

	processMessageBlock();
}

/*
 *  circularShift
 *
 *  Description:
 *      This member function will perform a circular shifting operation.
 *
 *  Parameters:
 *      bits: [in]
 *          The number of bits to shift (1-31)
 *      word: [in]
 *          The value to shift (assumes a 32-bit integer)
 *
 *  Returns:
 *      The shifted value.
 *
 *  Comments:
 *
 */
unsigned SHA1::circularShift(int bits, unsigned word)
{
	return ((word << bits) & 0xFFFFFFFF) | ((word & 0xFFFFFFFF) >> (32 - bits));
}

}
}
