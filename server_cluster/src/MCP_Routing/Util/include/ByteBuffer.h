/*
 * Copyright (c) 2015-2021 Contributors to the Eclipse Foundation
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


#ifndef MCP_BYTE_BUFFER_H_
#define MCP_BYTE_BUFFER_H_


#include <boost/cstdint.hpp>
#include <boost/tuple/tuple.hpp>

#include "MCPTypes.h"
#include "MCPExceptions.h"

namespace mcp
{

/**
 * An immutable std::string.
 */
typedef const std::string ConstString;

/**
 * A set of Strings.
 */
typedef std::set<std::string> StringSet;
/**
 * A boost::shared_ptr to a String.
 */
typedef boost::shared_ptr<String> StringSPtr;
/**
 * A boost::shared_ptr to a ConstString.
 */
typedef boost::shared_ptr<ConstString> ConstStringSPtr;

/**
 * A boost::shared_ptr to a StringSet.
 */
typedef boost::shared_ptr<StringSet> StringSetSPtr;

/**
 * A map of string keys and string values.
 */
typedef std::map<std::string, std::string> StringStringMap;


/**
 * Byte buffer.
 *
 * The following class is not thread safe. If more than one thread has
 * access to byteBuffer, it is best to access only while holding a mutex.
 *
 * The class writes data into the byte array in Network-Byte-Order, that is Big-Endian.
 * Note: in Big-Endian the MSB is written first, LSB last.
 *
 */
class ByteBuffer
{

private:

	static const int sizeOF_CHAR 	= 1;
	static const int sizeOF_BOOL 	= 1;
	static const int sizeOF_SHORT 	= 2;
	static const int sizeOF_INT 	= 4;
	static const int sizeOF_FLOAT   = 4;
	static const int sizeOF_DOUBLE  = 8;
	static const int sizeOF_SIZE_T  = 8;
	static const int sizeOF_LONG    = 8;

	char*		_buffer;
	size_t      _capacity;
	bool        _readOnly;
	bool        _owner;


	void    checkSpace4Write(size_t index, size_t dataLength);
	void    checkSpace4Read(size_t index, size_t dataLength) const;
	void    writeGenObject(size_t& index, const char *source, size_t length);
	void    readGenObject(size_t& index, char *target, size_t length) const;

	/**
	 * Depricated
	 *
	 * Copy constructor, copies the the virtual buffer slice.
	 *
	 * Allocates storage to fit the virtual buffer slice.
	 *
	 * @param bb
	 *
	 * @return
	 */
	ByteBuffer(const ByteBuffer& bb);

	/**
	 * Depricated
	 *
	 * Assignment operator, assigns the virtual buffer slice
	 * from right-hand-side to left-hand-side.
	 *
	 * Reallocates storage if necessary.
	 *
	 * One cannot assign a NULL buffer.
	 *
	 * @param bb
	 * @return
	 *
	 * @throw OutOfMemoryException if allocation of storage failed.
	 * @throw NullPointerException if right-hand-side has a NULL buffer pointer.
	 */
	const ByteBuffer& operator=(const ByteBuffer& bb);


    //===============================================================

protected:
    /**
     * Next read/write position relative to the virtual base.
     */
    size_t      _position;

    /**
     * Constructor, R/W buffer, position=0.
     *
     * @param   'capacity' - initial capacity of buffer.

     * @throw  OutOfMemoryException if not enough memory.
     */
    ByteBuffer(size_t capacity);

    /**
     * Constructor, wrap or copy the source buffer.
     *
     * @param buffer the source buffer to warp or copy.
     * @param length length of source buffer
     * @param readOnly
     * @param noCopy if false reallocates and copies the source buffer, else take ownership of the buffer.
     *
     * @throw OutOfMemoryException if not enough memory.
     */
    ByteBuffer(const char* buffer, size_t length, bool readOnly, bool noCopy, bool owner = true);


//    //=== write ======================================================
//       // Note that all the following functions start writing from the position
//       // pointed by 'index'. 'index' is updated to point to next position.
//       // These functions do not change the internal buffer position pointer.
//       void writeChar(size_t& index, char c);
//       void writeBoolean(size_t& index, bool value);
//       void writeShort(size_t& index, int16_t value);
//       void writeInt(size_t& index, int32_t value);
//       void writeLong(size_t& index, int64_t value);
//       void writeSize_t(size_t& index, uint64_t value);
//       void writeFloat(size_t& index, float value);
//       void writeDouble(size_t& index, double value);
//       void writeString(size_t& index, const String &value);
//       void writeByteArray(size_t& index, const char* ba, size_t length);
//       void writeByteBuffer(size_t& index, const ByteBuffer &bb, size_t length);
//       void writeStringSet(size_t& index, const StringSet &strSet);
//
//       //=== write 2pos ====================================================
//       // Note that all the following functions start writing from the given position.
//       // The difference is that we do not update the original offset.
//       void writeChar2Pos(size_t pos, char c)              {writeChar(pos, c);}
//       void writeBoolean2Pos(size_t pos, bool b)           {writeBoolean(pos, b);}
//       void writeShort2Pos(size_t pos, int16_t s)          {writeShort(pos, s);}
//       void writeInt2Pos(size_t pos, int32_t i)  	        {writeInt(pos, i);}
//       void writeLong2Pos(size_t pos, int64_t l)       	{writeLong(pos, l);}
//       void writeSize_t2Pos(size_t pos, uint64_t st)   	{writeSize_t(pos, st);}
//       void writeFloat2Pos(size_t pos, float f)            {writeFloat(pos, f);}
//       void writeDouble2Pos(size_t pos, double d)          {writeDouble(pos, d);}
//       void writeString2Pos(size_t pos, const String &str) {writeString(pos, str);}
//       void writeStringSet2Pos(size_t pos, const StringSet &strSet)
//       	   {writeStringSet(pos, strSet);}
//       void writeByteArray2Pos(size_t pos, const char* ba, size_t length)
//       	   {writeByteArray(pos, ba, length);}
//       void writeByteBuffer2Pos(size_t pos, const ByteBuffer &bb, size_t length)
//       	   {writeByteBuffer(pos, bb, length);}

//       //=== read ======================================================
//
//       // Note that all the following functions start reading from the position
//       // pointed by 'index'. 'index' is updated to point to next position.
//       // These functions do not change the internal buffer position pointer.
//       char            	readChar(size_t& index) const;
//       bool            	readBoolean(size_t& index) const;
//       int16_t         	readShort(size_t& index) const;
//       int32_t         	readInt(size_t& index) const;
//       int64_t     		readLong(size_t& index) const;
//       uint64_t    		readSize_t(size_t& index) const;
//       float           	readFloat(size_t& index) const;
//       double          	readDouble(size_t& index) const;
//       StringSPtr      	readStringSPtr(size_t& index) const;
//       size_t          	readByteArray(size_t& index, char* ba, size_t length) const;
//       StringSetSPtr   	readStringSet(size_t& index) const;
//
//       //=== read from Pos =============================================
//       // Note that all the following functions start reading from the given position.
//       // The difference is that we do not update the original offset.
//       char            	readCharFromPos(size_t pos)         {return readChar(pos);}
//       bool            	readBooleanFromPos(size_t pos)      {return readBoolean(pos);}
//       int16_t         	readShortFromPos(size_t pos)        {return readShort(pos);}
//       int32_t         	readIntFromPos(size_t pos)          {return readInt(pos);}
//       int64_t     		readLongFromPos(size_t pos)         {return readLong(pos);}
//       uint64_t    		readSize_tFromPos(size_t pos)       {return readSize_t(pos);}
//       float           	readFloatFromPos(size_t pos)        {return readFloat(pos);}
//       double          	readDoubleFromPos(size_t pos)       {return readDouble(pos);}
//       StringSPtr      	readStringSPtrFromPos(size_t pos)   {return readStringSPtr(pos);}
//       StringSetSPtr   	readStringSetFromPos(size_t pos)    {return readStringSet(pos);}
//       size_t          	readByteArrayFromPos(size_t pos, char* ba, size_t length)
//       	   {return readByteArray(pos, ba, length);}


public:
    virtual ~ByteBuffer();

    typedef uint32_t size_type;

    //=== factory methods ===========================================
    /**
     * Create a R/W ByteBuffer.
     *
     * Buffer is R/W, internally allocates storage,
     * Slice equals full buffer, and position=0.
     *
     * @param capacity
     * @return
     */
    static boost::shared_ptr<ByteBuffer> createByteBuffer(size_type capacity);

    /**
     * Create a read-only buffer from a source buffer (copy or take ownership).
     *
     * @param buffer
     * 		source buffer
     * @param
     * 		length length of data in source buffer
     * @param takeOwnership
     * 		if false, allocates storage and copies source buffer,
     * 		else wraps source buffer and takes ownership.
     * @return
     */
    static boost::shared_ptr<ByteBuffer> createReadOnlyByteBuffer(
        		const char* buffer, size_type length, bool takeOwnership);

    /**
     * Create a R/W buffer from a source buffer (copy).
     *
     * @param buffer
     * @param length
     * @return
     */
    static boost::shared_ptr<ByteBuffer> cloneByteBuffer(const char* buffer, size_t length);

    //=== clone methods ==============================================


//    /**
//     * Clone the ByteBuffer (copy).
//     *
//     * Allocates storage.
//     * @return
//     */
//    boost::shared_ptr<ByteBuffer>     clone(void) const;
//    boost::shared_ptr<ByteBuffer>     cloneAsWriteable(void) const;



    //=== write relative ===============================================
	// Note that all the following functions start writing from the internal
	// position pointer '_position'.

    void writeChar(char c);

	void writeBoolean(bool b);

	void writeShort(int16_t s);

	void writeInt(int32_t i);

	void writeLong(int64_t l);

	void writeSize_t(uint64_t st);

//	void writeFloat(float f)
//	{
//		writeFloat(_position, f);
//	}
//
//	void writeDouble(double d)
//	{
//		writeDouble(_position, d);
//	}

	void writeString(const String &str);

	void writeStringSet(const StringSet &strSet);

	void writeByteArray(const char* ba, size_t length);

	void writeByteBuffer(const ByteBuffer &bb, size_t length);

    //==== read relative ============================================
    // Note that all the following functions start reading from the internal
    // position pointer '_position'.
    char            readChar(void);

    bool            readBoolean(void) ;

    int16_t         readShort(void);

    int32_t         readInt(void);

    int64_t     	readLong(void);

    uint64_t    	readSize_t(void);

//    float           readFloat(void)                     {return readFloat(_position);}
//
//    double          readDouble(void)                    {return readDouble(_position);}

    String		    readString(void);

    int 			skipString();

    StringSPtr      readStringSPtr(void);

    StringSetSPtr   readStringSet(void);

    size_t      	readByteArray(char* ba, size_t length);

    //=== control ===================================================

//Depricated
//    const char* getVirtualBase() const
//	{
//		return _virtualBase;
//	}

	size_t getRemaining(void) const
	{
		return _capacity - _position;
	}

	size_t getPosition(void) const
	{
		return _position;
	}

	size_t getCapacity(void) const
	{
		return _capacity;
	}

	bool isReadOnly(void) const
	{
		return _readOnly;
	}

	bool isValid() const
	{
		return (_buffer != NULL);
	}

	const char* getBuffer() const
	{
		return _buffer;
	}

//	size_t getOrigCapacity() const
//	{
//		return _origCapacity;
//	}

	/**
	 * Reset buffer.
	 *
	 * Discards slicing (restore to full slice), reset position to 0, set as R/W.
	 */
	void reset(void);

	/**
	 * Set position.
	 *
	 * Jumps to 'position' in buffer, may reallocate storage in a R/W buffer.
	 *
	 * @param   'position' - new positon to set.
	 *
	 * @throw  IndexOutOfBoundsException if trying to jump out of bound in read only.
	 * @throw  OutOfMemoryException if storage reallocation fails.
	 */
	void setPosition(size_t position);

	/**
	 * Returns size of data in the buffer.
	 *
	 * If it is a read-only - returns capacity.
	 * If it is R/W - returns position.
	 *
	 * @return size of data in the buffer.
	 */
	size_t getDataLength(void) const;

	/**
	 * String representation of the buffer.
	 *
	 * If it is a read-only - on [0, capacity).
	 * If it is R/W - on [0,position).
	 *
	 * In other words, on [0,getDataLength() )
	 *
	 * @return
	 */
	String toString() const;

	String toDiagnosticString() const;

	/**
	 * Returns a 32 bit checksum.
	 *
	 * If it is a read-only - on [0, capacity).
	 * If it is R/W - on [0,position).
	 *
	 * In other words, on [0,getDataLength() )
	 *
	 * @param skipLastNBytes skip the last N bytes on a read-only buffer,
	 * 		where the expected CRC is.
	 * @return
	 */
	uint32_t getCRCchecksum(uint32_t skipLastNbytes = 0) const;
};

typedef boost::shared_ptr<ByteBuffer> 	ByteBuffer_SPtr;

class ByteBufferReadOnlyWrapper : public ByteBuffer
{
public:
	ByteBufferReadOnlyWrapper(const char* buffer, size_t length);
	virtual ~ByteBufferReadOnlyWrapper();

	/**
	 * Create a read-only wrapper buffer from a source buffer (does not copy or take ownership).
	 *
	 * The ByteBuffer will not delete the buffer in the destructor.
	 *
	 * @param buffer
	 * 		source buffer
	 * @param
	 * 		length length of data in source buffer
	 * @return
	 */
	static boost::shared_ptr<ByteBufferReadOnlyWrapper> createByteBufferReadOnlyWrapper(
			const char* buffer, size_type length);
};
} //namespace mcp

#endif /* MCP_BYTE_BUFFER_H_ */
