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
 * ByteBuffer.cpp
 *
 *  Created on: 15/03/2010
 */


#include <iostream>
#include <sstream>
#include <exception>
#include <cassert>
#include <memory.h>

#include <boost/crc.hpp>

#ifdef SPDR_LINUX
#include <arpa/inet.h> //for hton<l/s> ntoh<l/s>
#endif

#ifdef SPDR_WINDOWS
#include <winsock2.h> //for hton<l/s> ntoh<l/s>
#endif

#include "ByteBuffer.h"

namespace spdr
{

using namespace std;

static bool isLittleEndian()
{
   static short word = 0x0001;
   static char* byte = (char*)&word;
   static bool  res = (byte[0]) ? true : false;
   return res;
}

/* @name    orderConvert
 * @param   'target' 
 * @param   'source' 
 * @param   'length' - size of object to convert. 
 * @dscr    This function translates basic data types (such as integers and 
 * @dscr    floats) to char array, independently of little endian or big 
 * @dscr    endian order. 
 * @dscr    This function can both convert an object to char array or reconstruct  
 * @dscr    the object from the char array. 
 * @note    We do not need two functions here. One translates well both ways. 
 */
inline static void orderConvert(char* target, const char *source, size_t length)
{
    static const bool changeOrder = isLittleEndian(); //we are network-order == big-endian
    if(changeOrder){
            source += length-1; 
        while (length) {
            *target = *source;
            source--;
            target++;
            length--;
        }
    } else { 
        while (length) {
            *target = *source;
            target++;
            source++;
            length--;
        }
    }

    //TODO use htonl(), htons(), ntohs(), ntohl() for 16 and 32 bit
} 

/*****************************************************************************/
/* @name    ByteBuffer::~ByteBuffer
 * @dscr    Destructor. 
 */
ByteBuffer::~ByteBuffer() { 
    //CLASS_INIT_SPY("ByteBuffer", CLASS_INIT_SPY_EXIT);
    delete []_buffer;
    _buffer = NULL; 
} 

///* @name    ByteBuffer::writeStringSet
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input string set 'strSet' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeStringSet(size_t& index, const StringSet &strSet) {
//    writeInt(index, (int)(strSet.size()));
//    StringSet::const_iterator from = strSet.begin();
//    StringSet::const_iterator to = strSet.end();
//    while (from != to) {
//        // Note that we apply a virtual function here.
//        writeString(index, *from);
//        from++;
//    }
//}

void ByteBuffer::writeStringSet(const StringSet &strSet) {
    writeInt((int)(strSet.size()));
    StringSet::const_iterator from = strSet.begin();
    StringSet::const_iterator to = strSet.end();
    while (from != to)
    {
        writeString(*from);
        from++;
    }
}


///* @name    ByteBuffer::readStringSet
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a string set from buffer at position 'index'.
// * @return  StringSet - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//StringSetSPtr ByteBuffer::readStringSet(size_t& index) const {
//    int size = readInt(index);
//    StringSetSPtr retStrSet =
//        StringSetSPtr(new(StringSet));
//    for (int i=0; i<size; i++) {
//        // Note that we apply a virtual function here.
//        StringSPtr curStr = readStringSPtr(index);
//        retStrSet->insert(*curStr);
//    }
//    return retStrSet;
//}

StringSetSPtr ByteBuffer::readStringSet() {
    int size = readInt();
    StringSetSPtr retStrSet =
        StringSetSPtr(new(StringSet));
    for (int i=0; i<size; i++) {
        StringSPtr curStr = readStringSPtr();
        retStrSet->insert(*curStr);
    }
    return retStrSet;
}


ByteBuffer::ByteBuffer(size_t capacity) 
        : _buffer(new char[capacity]) { 
    //CLASS_INIT_SPY("ByteBuffer", CLASS_INIT_SPY_ENTRY);
    if(_buffer == NULL) { 
        delete this; 
        _capacity = 0; 
        throw OutOfMemoryException(); 
    } 

    _capacity = capacity; 
    //_origCapacity = _capacity;
    _position = 0; 
    _readOnly = false; 
    //_virtualBase = _buffer;
} 

ByteBuffer::ByteBuffer(const char* buffer, size_t length, bool readOnly, bool wrap)
        : _buffer(NULL) { 
    //CLASS_INIT_SPY("ByteBuffer", CLASS_INIT_SPY_ENTRY);
	if(!wrap) //copy
	{
		_buffer = new char[length];
		if(_buffer == NULL) { 
			_capacity = 0; 
			throw OutOfMemoryException(); 
		} 
		memcpy(_buffer, buffer, length);
	}
	else
	{
		_buffer = const_cast<char*>(buffer);
	}
    //_virtualBase = _buffer;
    _capacity = length; 
    //_origCapacity = _capacity;
    _position = 0; 
    _readOnly = readOnly;                                                   // ibm@AVI006
} 

/* @name    ByteBuffer::checkSpace4Write
 * @param   'index' - location to start writing from. 
 * @param   'dataLength' - length of write object. 
 * @dscr    verifies that buffer has enough space for next write. 
 * @dscr    If not, then expands the buffer. 
 * @throws  'BufferNotWriteableException' if buffer is read only.
 */
void ByteBuffer::checkSpace4Write(size_t index, size_t dataLength)
{
	if (_readOnly)
		throw BufferNotWriteableException();

	if (_buffer == NULL)
		throw NullPointerException("ByteBuffer::checkSpace4Write _buffer is NULL");

	//assert (_buffer != NULL);

	if ((index + dataLength) <= _capacity)
	{
		//There is enough space in the buffer
		return;
	}
	//Need to realloc

	int rem = ((index+dataLength) % 1024 > 0 ? 1 : 0);
	size_t newCapacity = (((index+dataLength)/1024) + rem)*1024;

	//Allocate new buffer
	char *newBuffer = new char[newCapacity];
	if (newBuffer == NULL)
	{
		throw OutOfMemoryException();
	}

	//Copy the data
	memcpy(newBuffer, _buffer, _capacity);

	_capacity = newCapacity;
	//_origCapacity = _capacity; //YT
	delete[] _buffer;
	_buffer = newBuffer;
	//_virtualBase = _buffer;
	return;
}

/* @name    ByteBuffer::checkSpace4Read
 * @dscr    verifies that buffer has enough space for next read.   
 * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
 */
void ByteBuffer::checkSpace4Read(size_t index, size_t dataLength) const
{
	if ((index + dataLength) <= _capacity)
	{
		//There is enough space in the buffer
		return;
	}
	else
	{
		ostringstream errorMsg;
		errorMsg << "IndexOutOfBoundsException: Trying to read " << (unsigned int) dataLength;
		errorMsg << " bytes from " << (unsigned int) index;
		errorMsg << " while remaining data from this place is ";
		errorMsg << (unsigned int) getRemaining();
		errorMsg << " bytes." << endl;
		errorMsg << this->toString();
		errorMsg << this->toDiagnosticString();
		throw IndexOutOfBoundsException(errorMsg.str());
	}
}

/* @name    ByteBuffer::writeGenObject
 * @param   '&index' - position to write to. Updated to reflect new position. 
 * @param   'length' - size of object to write. 
 * @dscr    This method writes the object pointed by 'source' to the buffer 
 * @dscr    pointed by 'target'.   
 * @throws  'BufferNotWriteableException'. 
 */
void ByteBuffer::writeGenObject(size_t& index,  
                                const char *source, size_t length) { 
    checkSpace4Write(index, length);
    orderConvert(_buffer+index, source, length);
    index += length;
}

///* @name    ByteBuffer::writeChar
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    writes input char 'c' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeChar(size_t& index, char c) {
//    writeGenObject(index, (char *)&c, sizeOF_CHAR);
//}

void ByteBuffer::writeChar(char c)
{
	checkSpace4Write(_position, sizeOF_CHAR);
	char* target = _buffer + _position;
	*target = c;
	_position += sizeOF_CHAR;
}

///* @name    ByteBuffer::writeBoolean
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    writes input boolean 'b' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeBoolean(size_t& index, bool b) {
//    if (b) {
//        writeChar(index, 1);
//    } else {
//        writeChar(index, 0);
//    }
//}

void ByteBuffer::writeBoolean(bool b)
{
    if (b)
    {
        writeChar(1);
    }
    else
    {
        writeChar(0);
    }
}


///* @name    ByteBuffer::writeShort
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input short 's' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeShort(size_t& index, int16_t s) {
//    writeGenObject(index, (char *)&s, sizeOF_SHORT);
//}

void ByteBuffer::writeShort(int16_t s)
{
	checkSpace4Write(_position, sizeOF_SHORT);
	int16_t s_be = htons(s);
	memcpy(_buffer+_position,&s_be,sizeOF_SHORT);
	_position += sizeOF_SHORT;
}

///* @name    ByteBuffer::writeInt
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input integer 'i' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeInt(size_t& index, int32_t i)
//{
//	//<<
//	writeGenObject(index, (char *)&i, sizeOF_INT);
//	//>>
//}

void ByteBuffer::writeInt(int32_t i)
{
    checkSpace4Write(_position, sizeOF_INT);
    int32_t i_be = htonl(i);
    memcpy(_buffer+_position,&i_be,sizeOF_INT);
    _position += sizeOF_INT;
}


///* @name    ByteBuffer::writeLong
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input long 'l' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeLong(size_t& index, int64_t l) {
//    writeGenObject(index, (char *)&l, sizeOF_LONG);
//}

void ByteBuffer::writeLong(int64_t l)
{
    checkSpace4Write(_position, sizeOF_LONG);
    int32_t i_be = htonl((int32_t)(l >> 32)); //High
    memcpy(_buffer+_position, &i_be, sizeOF_INT);
    i_be = htonl((int32_t)l); //Low
    memcpy(_buffer+_position+sizeOF_INT, &i_be, sizeOF_INT);
    _position += sizeOF_LONG;
}

///* @name    ByteBuffer::writeSize_t
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input size_t 'st' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeSize_t(size_t& index, uint64_t st) {
//    writeGenObject(index, (char *)&st, sizeOF_SIZE_T);
//}

void ByteBuffer::writeSize_t(uint64_t st)
{
	checkSpace4Write(_position, sizeOF_SIZE_T);
	uint32_t st_be = htonl((uint32_t)(st >> 32)); //MSB
	memcpy(_buffer+_position, &st_be, sizeOF_INT);
	st_be = htonl((uint32_t)st); //LSB
	memcpy(_buffer+_position+sizeOF_INT, &st_be, sizeOF_INT);
	_position += sizeOF_SIZE_T;
}

///* @name    ByteBuffer::writeFloat
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input float 'f' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeFloat(size_t& index, float f) {
//    writeGenObject(index, (char *)&f, sizeOF_FLOAT);
//}

///* @name    ByteBuffer::writeDouble
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input float 'd' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeDouble(size_t& index, double d) {
//    writeGenObject(index, (char *)&d, sizeOF_DOUBLE);
//}

///* @name    ByteBuffer::writeString
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @dscr    write input string 'str' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeString(size_t& index, const String &str) {
//    int length = (int)str.size(); //length();
//    writeInt(index, length);
//    checkSpace4Write(index, length);
//    memcpy(_buffer+index, str.data(), length);
//    index += length;
//}

void ByteBuffer::writeString(const String &str)
{
    int length = (int)str.size(); //length();
    checkSpace4Write(_position, sizeOF_INT + length);
    writeInt(length);
    memcpy(_buffer+_position, str.data(), length);
    _position += length;
}

///* @name    ByteBuffer::writeByteArray
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @param   'length' - length of input char array.
// * @dscr    write input char array 'ba' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeByteArray(size_t& index, const char* ba, size_t length) {
//    checkSpace4Write(index,length);
//    memcpy(_buffer+index, ba, length);
//    index += length;
//}

void ByteBuffer::writeByteArray(const char* ba, size_t length)
{
    checkSpace4Write(_position,length);
    memcpy(_buffer+_position, ba, length);
    _position += length;
}

///* @name    ByteBuffer::writeByteBuffer
// * @param   '&index' - position to write to. Updated to reflect new position.
// * @param   'length' - length of input char array.
// * @dscr    write input char array 'ba' to current buffer at position 'index'.
// * @throws  'BufferNotWriteableException'.
// */
//void ByteBuffer::writeByteBuffer(size_t& index, const ByteBuffer &bb, size_t length) {
//    //writeByteArray(index, bb.getVirtualBase(), length);
//	writeByteArray(index, bb.getBuffer(), length);
//}

void ByteBuffer::writeByteBuffer(const ByteBuffer &bb, size_t length)
{
	writeByteArray(bb.getBuffer(), length);
}

void ByteBuffer::writeNodeVersion(const NodeVersion& version)
{
	writeLong(version.getIncarnationNumber());
	writeLong(version.getMinorVersion());
}

void ByteBuffer::writeNodeID(const NodeIDImpl_SPtr& nodeID)
{
	using namespace std;

	writeString(nodeID->getNodeName());
	size_t num_address = nodeID->getNetworkEndpoints().getNumAddresses();
	if (num_address > 255)
	{
		throw IllegalArgumentException(
				"Number of addresses exceeds 255, violates wire-format");
	}
	writeChar((uint8_t) num_address);

	vector<pair<string, string> >::const_iterator it =
			nodeID->getNetworkEndpoints().getAddresses().begin();
	vector<pair<string, string> >::const_iterator it_end =
			nodeID->getNetworkEndpoints().getAddresses().end();
	while (it != it_end)
	{
		writeString(it->first);
		writeString(it->second);
		++it;
	}
	writeShort(nodeID->getNetworkEndpoints().getPort());
}

void ByteBuffer::writeVirtualID(const util::VirtualID& vid)
{
	checkSpace4Write(_position,util::VirtualID::HashSize);
	vid.copyTo((_buffer+_position));
	_position += util::VirtualID::HashSize;
}

void ByteBuffer::writeStreamID(const messaging::StreamIDImpl& streamID)
{
	writeSize_t(streamID.getPrefix());
	writeSize_t(streamID.getSuffix());
}

//=== read ================================================================

/* @name    ByteBuffer::readGenObject
 * @param   '&index' - position to read from. Updated to reflect new position. 
 * @param   'length' - size of object to read. 
 * @dscr    This method reads from the buffer pointed by 'source' into an 
 * @dscr    object pointed by 'source'.   
 * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
 */
void ByteBuffer::readGenObject(size_t& index, char *dest, size_t length) const { 
    checkSpace4Read(index, length);
    orderConvert(dest, _buffer+index, length);
    index += length;
}

///* @name    ByteBuffer::readChar
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a character from buffer at position 'index'.
// * @return  character - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//char ByteBuffer::readChar(size_t& index) const {
//    char ch;
//    readGenObject(index, (char *)&ch, sizeOF_CHAR);
//    return ch;
//}

char ByteBuffer::readChar()
{
	checkSpace4Read(_position, sizeOF_CHAR);
	char* source = (_buffer+_position);
	_position += sizeOF_CHAR;
    return (*source);
}


///* @name    ByteBuffer::readBoolean
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a boolean from buffer at position 'index'.
// * @return  boolean - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//bool ByteBuffer::readBoolean(size_t& index) const {
//    char ch;
//    ch = readChar(index);
//    bool retVal = (ch == 1);
//    return retVal;
//}

bool ByteBuffer::readBoolean()
{
    char ch = readChar();
    bool retVal = (ch == 1);
    return retVal;
}

///* @name    ByteBuffer::readShort
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a short from buffer at position 'index'.
// * @return  short - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//short ByteBuffer::readShort(size_t& index) const {
//	int16_t var;
//    readGenObject(index, (char *)&var, sizeOF_SHORT);
//    return var;
//}

short ByteBuffer::readShort()
{
	checkSpace4Read(_position, sizeOF_SHORT);
	int16_t s_be;
	memcpy(&s_be, _buffer+_position, sizeOF_SHORT);
	_position += sizeOF_SHORT;
	return ntohs(s_be);
}


///* @name    ByteBuffer::readInt
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads an integer from buffer at position 'index'.
// * @return  integer - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//int32_t ByteBuffer::readInt(size_t& index) const {
//	int32_t var;
//    readGenObject(index, (char *)&var, sizeOF_INT);
//    return var;
//}

int32_t ByteBuffer::readInt()
{
	checkSpace4Read(_position, sizeOF_INT);
	int32_t i_be;
	memcpy(&i_be, _buffer+_position, sizeOF_INT);
	_position += sizeOF_INT;
	return ntohl(i_be);
}

///* @name    ByteBuffer::readLong
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a long from buffer at position 'index'.
// * @return  long - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//int64_t ByteBuffer::readLong(size_t& index) const {
//    int64_t var;
//    readGenObject(index, (char *)&var, sizeOF_LONG);
//    return var;
//}

int64_t ByteBuffer::readLong()
{
	checkSpace4Read(_position, sizeOF_LONG);
	int32_t i_be;
	memcpy(&i_be, _buffer+_position, sizeOF_INT);
	int64_t var = ntohl(i_be);
	var = var << 32;
	memcpy(&i_be, _buffer+_position+sizeOF_INT, sizeOF_INT);
	var = var | (int64_t)ntohl(i_be);
	_position += sizeOF_LONG;
	return var;
}

///* @name    ByteBuffer::readSize_t
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a size_t from buffer at position 'index'.
// * @return  size_t - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//uint64_t ByteBuffer::readSize_t(size_t& index) const {
//    uint64_t var;
//    readGenObject(index, (char *)&var, sizeOF_SIZE_T);
//    return var;
//}

uint64_t ByteBuffer::readSize_t()
{
	checkSpace4Read(_position, sizeOF_SIZE_T);
	uint32_t i_be;
	memcpy(&i_be, _buffer+_position, sizeOF_INT);
	uint64_t var = ntohl(i_be);
	var = var << 32;
	memcpy(&i_be, _buffer+_position+sizeOF_INT, sizeOF_INT);
	var = var | (uint64_t)ntohl(i_be);
	_position += sizeOF_SIZE_T;
	return var;
}


///* @name    ByteBuffer::readFloat
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a float from buffer at position 'index'.
// * @return  float - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//float ByteBuffer::readFloat(size_t& index) const {
//    float var;
//    readGenObject(index, (char *)&var, sizeOF_FLOAT);
//    return var;
//}

///* @name    ByteBuffer::readDouble
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a double from buffer at position 'index'.
// * @return  double - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//double ByteBuffer::readDouble(size_t& index) const {
//    double var;
//    readGenObject(index, (char *)&var, sizeOF_DOUBLE);
//    return var;
//}

///* @name    ByteBuffer::readString
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @dscr    reads a string from buffer at position 'index'.
// * @return  String - read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//StringSPtr ByteBuffer::readStringSPtr(size_t& index) const {
//	int length = readInt(index);
//    checkSpace4Read(index,length);
//    StringSPtr retStr(new String(_buffer+index, length));
//    index += length;
//    return retStr;
//}

StringSPtr ByteBuffer::readStringSPtr()
{
	int length = readInt();
    checkSpace4Read(_position,length);
    StringSPtr retStr(new String(_buffer+_position, length));
    _position += length;
    return retStr;
}

String ByteBuffer::readString(void)
{
	int length = readInt();
	checkSpace4Read(_position,length);
	size_t pos = _position;
    _position += length;
	return String(_buffer+pos, length);
}

int ByteBuffer::skipString(void)
{
	int length = readInt();
	checkSpace4Read(_position,length);
    _position += length;
	return length;
}

///* @name    ByteBuffer::readStringSet
// * @param   '&index' - position to read from. Updated to reflect new position.
// * @param   'ba' - destination byte array.
// * @param   'length' - number of bytes to copy.
// * @dscr    reads a character set from buffer at position 'index'.
// * @return  size_t - number of bytes actually read from buffer.
// * @throws  'IndexOutOfBoundsException' if trying to read out of bound.
// */
//size_t ByteBuffer::readByteArray(size_t& index, char* ba, size_t length) const {
//    assert (_capacity >= index);
//    size_t rc = (_capacity - index);
//    if(rc > length){
//        rc = length;
//    }
//    memcpy(ba, _buffer+index, rc);
//    index += rc;
//    return rc;
//}

size_t ByteBuffer::readByteArray(char* ba, size_t length)
{
	if (_position > _capacity)
	{
		throw SpiderCastRuntimeError("ByteBuffer::readByteArray _position > _capacity");
	}

    size_t rc = (_capacity - _position);
    if(rc > length){
        rc = length;
    }
    memcpy(ba, _buffer+_position, rc);
    _position += rc;
    return rc;
} 

NodeVersion ByteBuffer::readNodeVersion()
{
	int64_t incarnationNum = readLong();
	int64_t minorVer = readLong();
	return NodeVersion(incarnationNum, minorVer);
}

NodeIDImpl_SPtr ByteBuffer::readNodeID()
{
	String name = readString();
	uint8_t num_address = (uint8_t) readChar();
	std::vector<std::pair<String, String> > addresses;
	for (uint8_t i = 0; i < num_address; ++i)
	{
		String address = readString();
		String scope = readString();
		addresses.push_back(std::make_pair(address, scope));
	}
	uint16_t port = readShort();

	return NodeIDImpl_SPtr (new NodeIDImpl(name, addresses, port));
}

util::VirtualID ByteBuffer::readVirtualID()
{
	checkSpace4Read(_position,util::VirtualID::HashSize);
	size_t p = _position;
	_position += util::VirtualID::HashSize;
	return util::VirtualID(_buffer+p);
}

util::VirtualID_SPtr ByteBuffer::readVirtualID_SPtr()
{
	checkSpace4Read(_position,util::VirtualID::HashSize);
	size_t p = _position;
	_position += util::VirtualID::HashSize;
	return util::VirtualID_SPtr(new util::VirtualID(_buffer+p));
}

messaging::StreamIDImpl  ByteBuffer::readStreamID()
{
	using namespace messaging;
	uint64_t prefix = readSize_t();
	uint64_t suffix = readSize_t();
	return StreamIDImpl(prefix,suffix);
}

messaging::StreamID_SPtr  ByteBuffer::readStreamID_SPtr()
{
	using namespace messaging;
	uint64_t prefix = readSize_t();
	uint64_t suffix = readSize_t();
	return StreamID_SPtr(new StreamIDImpl(prefix,suffix));
}

//=== control =======================================================

void ByteBuffer::reset(void) {
    //_capacity = _origCapacity;
    _position = 0; 
    //_virtualBase = _buffer;
    _readOnly = false; 
}

void ByteBuffer::setPosition(size_t pos)
{
	if (_readOnly)
	{
		checkSpace4Read(pos, 0);
	}
	else
	{
		checkSpace4Write(pos, 0);
	}
	_position = pos;
}


//Depricated
//void ByteBuffer::slice(size_t start, size_t length)
//{
//	if ( (start<0) || (length<0) )
//	{
//		throw IllegalArgumentException("In ByteBuffer::slice");
//	}
//	else if ((start+length > _capacity))
//	{
//		throw IndexOutOfBoundsException("In ByteBuffer::slice");
//	}
//
//    _virtualBase += start;
//    _capacity = length;
//    _position = 0;
//}

/* @name    ByteBuffer::ByteBuffer
 * @dscr    Copy constructor. 
 */
//ByteBuffer::ByteBuffer(const ByteBuffer& bb)
//{
//    //CLASS_INIT_SPY("ByteBuffer", CLASS_INIT_SPY_ENTRY);
//    _buffer = NULL;
//    *this = bb;
//}


//const ByteBuffer& ByteBuffer::operator=(const ByteBuffer& bb)
//{
//	if (&bb == this)
//		return *this;
//
//	if (bb._buffer == NULL)
//		throw NullPointerException("Cannot assign a null ByteBuffer");
//
//	//why reallocate if LHS is big enough?
//	if (_buffer)
//	{
//		if (bb._capacity > _capacity)
//		{
//			delete[] _buffer;
//			_buffer = new char[bb._capacity];
//			_capacity = bb._capacity;
//		}
//	}
//	else
//	{
//		_buffer = new char[bb._capacity];
//		_capacity = bb._capacity;
//	}
//
//	if (_buffer == NULL)
//	{
//		throw OutOfMemoryException();
//	}
//
//	_capacity = bb._capacity;
//	_readOnly = bb._readOnly;
//	_position = bb._position;
//	//_virtualBase = _buffer;
//
//	memcpy(_buffer, bb._buffer, _capacity);
//	return *this;
//}

//Depricated
//char* ByteBuffer::extractBufferSlice()
//{
//	char* result;
//	if (_virtualBase == _buffer)
//	{
//		result = _buffer;
//		_buffer = NULL;
//		_virtualBase = NULL;
//		_capacity = 0;
//		_origCapacity = 0;
//		_position = 0;
//	}
//	else
//	{
//		result = new char[_capacity];
//		memcpy(result, _virtualBase, _capacity);
//		delete[] _buffer;
//		_buffer = NULL;
//		_virtualBase = NULL;
//		_capacity = 0;
//		_origCapacity = 0;
//		_position = 0;
//	}
//	return result;
//}

//Depricated
//boost::tuples::tuple<char*,size_t,size_t,size_t> ByteBuffer::extractBuffer()
//{
//	boost::tuples::tuple<char*,size_t,size_t,size_t> result = boost::tuples::make_tuple(
//			_buffer,_origCapacity,(size_t)(_virtualBase-_buffer),_capacity);
//
//	_buffer = _virtualBase = NULL;
//	_capacity = 0;
//	_origCapacity = 0;
//	_position = 0;
//
//	return result;
//}


size_t ByteBuffer::getDataLength() const
{
	if (_readOnly)
		return _capacity;
	return _position;
}

ByteBuffer_SPtr ByteBuffer::createByteBuffer(size_type capacity)
{
	ByteBuffer_SPtr retBuffer(new ByteBuffer(capacity));
	return retBuffer;
}

ByteBuffer_SPtr ByteBuffer::createReadOnlyByteBuffer(const char* buffer,
		size_type length, bool wrap)
{
	ByteBuffer_SPtr retBuffer(new ByteBuffer(buffer, length, true, wrap));
	return retBuffer;
}

/**
 * @name    ByteBuffer::cloneByteBuffer
 * @dscr    Creates a new byte buffer, holding my initial 'bufferSize' bytes.
 * @throws  DCSException if I do not hold 'bufferSize' bytes
 * @note    Added by ibm@AVI006
 */
ByteBuffer_SPtr ByteBuffer::cloneByteBuffer(const char* buffer, size_t length)
{
	ByteBuffer_SPtr retBuffer(new ByteBuffer(buffer, length, false));
	return retBuffer;
}

// Depricated
//
//ByteBuffer_SPtr ByteBuffer::clone() const
//{
//	return ByteBuffer_SPtr(new ByteBuffer(*this));
//}
//
//ByteBuffer_SPtr ByteBuffer::cloneAsWriteable(void) const
//{
//	if (!_readOnly)
//		return clone();
//	ByteBuffer_SPtr rc = clone();
//	rc->_readOnly = false;
//	rc->_position = _capacity;
//	return rc;
//}

String ByteBuffer::toString() const
{
	using namespace std;

	ostringstream oss;
	oss << (&_buffer) << dec << " c:" << _capacity << " p:" << _position;
	oss << " r" << (_readOnly ? "o " : "w " );

	if (_buffer != NULL)
	{
		size_t len = (_readOnly ? _capacity : _position);
		oss << " b: "<< hex;
		for (size_t i=0; i< len; ++i)
		{
			oss << (unsigned short)(*(_buffer+i));
			if (i < len-1)
			{
				oss << ',';
			}
		}
		oss << endl;
	}

	return oss.str();
}

String ByteBuffer::toDiagnosticString() const
{
	using namespace std;

	ostringstream oss;

	size_t indx1 = (_position-50 >=0 ? _position-50 : 0);
	size_t indx2 = (_position+50 < _capacity ? _position+50 : _capacity);

	if (_buffer != NULL)
	{
		oss << "d: " << hex;
		for (size_t i=indx1; i<indx2; ++i)
		{
			if (i==_position)
			{
				oss << " P-> ";
			}

			oss << (unsigned short)(*(_buffer+i));

			if (i < indx2-1)
			{
				oss << ',';
			}
		}
		oss << endl;
	}

	return oss.str();
}

uint32_t ByteBuffer::getCRCchecksum(uint32_t skipLastNbytes) const
{
	boost::crc_32_type crcCode;
	int len = 0;
	if (_readOnly)
	{
		len = _capacity - skipLastNbytes;
		if (len < 0)
			throw SpiderCastRuntimeError("CRC with skipLastNbytes, on a buffer smaller then N bytes");
	}
	else
	{
		len = _position;
	}

	crcCode.process_bytes(_buffer, (size_t)len);
	return crcCode.checksum();
}

}
