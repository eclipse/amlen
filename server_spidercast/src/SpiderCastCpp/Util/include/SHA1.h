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
 * Sha1.h
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

#ifndef SPDR_UTIL_SHA1_H_
#define SPDR_UTIL_SHA1_H_

#include <string>
#include <sstream>

#include <boost/cstdint.hpp>

#include "SpiderCastLogicError.h"

namespace spdr
{
namespace util
{

/**
 * This class implements the Secure Hashing Standard as defined
 * in FIPS PUB 180-1 published April 17, 1995.
 *
 *      The Secure Hashing Standard, which uses the Secure Hashing
 *      Algorithm (SHA), produces a 160-bit message digest for a
 *      given data stream.  In theory, it is highly improbable that
 *      two messages will produce the same message digest.  Therefore,
 *      this algorithm can serve as a means of providing a "fingerprint"
 *      for a message.
 *
 *  Portability Issues:
 *      SHA-1 is defined in terms of 32-bit "words".  This code was
 *      written with the expectation that the processor has at least
 *      a 32-bit machine word size.  If the machine word size is larger,
 *      the code should still function properly.  One caveat to that
 *      is that the input functions taking characters and character arrays
 *      assume that only 8 bits of information are stored in each character.
 *
 *  Caveats:
 *      SHA-1 is designed to work with messages less than 2^64 bits long.
 *      Although SHA-1 allows a message digest to be generated for
 *      messages of any number of bits less than 2^64, this implementation
 *      only works with messages with a length that is a multiple of 8
 *      bits.
 *
 *  Usage:
 *  	Create an object, update() with data, and call digest() to get the message digest.
 *  	Before another use call reset().
 *
 */
class SHA1
{
public:
	/**
	 * Constructor for the SHA1 class.
	 */
	SHA1();

	/**
	 * Destructor for the SHA1 class.
	 */
	virtual ~SHA1();

	/**
	 *  Re-initialize the class
	 */
	void reset();

	/**
	 * Returns the 160-bit message digest into the word array provided.
	 *
	 * Big endian representation, H[0],H[1],H[2],H[3],H[4]; where H[0] is MSB.
	 *
	 *
	 * @param message_digest_array
	 * 		This is an array of five unsigned integers (32 bit) which
	 * 		will be filled with the message digest that has been computed (output).
	 *
	 * @return True if successful, false if it failed.
	 */
	bool digest(uint32_t *message_digest_array);

	/**
	 * Returns the 160-bit message digest into the char array provided.
	 *
	 * Big-endian representation, H[0],H[1],H[2],H[3],H[4]; where H[0] is MSB;
	 * Each integer is written into the buffer in big-endian as well.
	 *
	 * @param message_digest_array
	 * 		This is an array of (at least) 20 char (8 bit) which
	 * 		will be filled with the message digest that has been computed (output).
	 *
	 * @return True if successful, false if it failed.
	 */
	bool digest(char *message_digest_array);

	/**
	 * This function accepts an array of octets as the next portion of
	 * the message.
	 *
	 * @param message_array
	 * 		An array of characters representing the next portion of the
	 * 		message (input).
	 * @param length
	 * 		Length of the array.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	void update(const unsigned char *message_array, uint32_t length);

	/**
	 * This function accepts an array of octets as the next portion of
	 * the message.
	 *
	 * @param message_array
	 * 		An array of characters representing the next portion of the
	 * 		message (input).
	 * @param length
	 * 		Length of the array.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	void update(const char *message_array, uint32_t length);

	/**
	 * This function accepts a single octet as the next message element.
	 *
	 * @param message_element
	 * 		The next octet in the message.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	void update(unsigned char message_element);

	/**
	 * This function accepts a single octet as the next message element.
	 *
	 * @param message_element
	 * 		The next octet in the message.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	void update(char message_element);

	/**
	 * This function accepts a string as the next message element.
	 *
	 * @param message
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	void update(const String& message);

	/**
	 * Accept a a null terminated string as the next message element.
	 *
	 * This operator makes it convenient to provide character strings to
	 * the SHA1 object for processing.
	 *
	 * @param message_array
	 * 		A null terminated string, the next element of the message for processing.
	 *
	 * @return A reference to the SHA1 object.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	SHA1& operator<<(const char *message_array);
	/**
	 * Accept a a null terminated string as the next message element.
	 *
	 * This operator makes it convenient to provide character strings to
	 * the SHA1 object for processing.
	 *
	 * @param message_array
	 * 		A null terminated string, the next element of the message for processing.
	 *
	 * @return A reference to the SHA1 object.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	SHA1& operator<<(const unsigned char *message_array);

	/**
	 * This function provides the next octet in the message.
	 *
	 * The character is assumed to hold 8 bits of information.Provide input to SHA1.
	 *
	 * @param message_element
	 * 		The next octet in the message.
	 * @return A reference to the SHA1 object.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	SHA1& operator<<(const char message_element);

	/**
	 * This function provides the next octet in the message.
	 *
	 * The character is assumed to hold 8 bits of information.Provide input to SHA1.
	 *
	 * @param message_element
	 * 		The next octet in the message.
	 * @return A reference to the SHA1 object.
	 *
	 * @throw IllegalStateException if invoked when digest already computed
	 */
	SHA1& operator<<(const unsigned char message_element);

	/**
	 * A Hex string representation of the 160 bit digest.
	 *
	 * AAAAAAAA:BBBBBBBB:CCCCCCCC:DDDDDDDD:EEEEEEEE
	 *
	 * @param digest_array
	 * 		This is an array of five unsigned integers (32 bit) which
	 * 		contains the message digest.
	 *
	 * @return
	 */
	static std::string digestToHexString(const uint32_t* digest_array);

private:

	/*
	 *  Process the next 512 bits of the message
	 */
	void processMessageBlock();

	/*
	 *  Pads the current message block to 512 bits
	 */
	void padMessage();

	/*
	 *  Performs a circular left shift operation
	 */
	inline uint32_t circularShift(int32_t bits, uint32_t word);

	uint32_t H[5]; // Message digest buffers

	uint32_t Length_Low; // Message length in bits
	uint32_t Length_High; // Message length in bits

	unsigned char Message_Block[64]; // 512-bit message blocks
	int32_t Message_Block_Index; // Index into message block array

	bool Computed; // Is the digest computed?
	bool Corrupted; // Is the message digest corruped?
};

}
}

#endif /* SPDR_UTIL_SHA1_H_ */
