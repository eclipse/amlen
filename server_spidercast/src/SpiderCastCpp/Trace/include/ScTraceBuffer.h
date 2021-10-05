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

/*-----------------------------------------------------------------
 * File        : ScTraceBuffer.h
 * Version     : $Revision: 1.15 $
 * Last updated: $Date: 2016/11/27 11:28:33 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTraceBuffer.h,v $
 * Revision 1.15  2016/11/27 11:28:33 
 * macros fro trace
 *
 * Revision 1.14  2015/12/22 11:56:40 
 * print typeid in catch std::exception; fix toString of sons of AbstractTask.
 *
 * Revision 1.13  2015/11/18 08:37:00 
 * Update copyright headers
 *
 * Revision 1.12  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.11  2015/11/03 14:36:44 
 * trace
 *
 * Revision 1.10  2015/08/31 11:54:26 
 * *** empty log message ***
 *
 * Revision 1.9  2015/07/29 09:26:11 
 * split brain
 *
 * Revision 1.8  2015/07/16 07:34:33 
 * config incarnation number
 *
 * Revision 1.7  2015/07/13 08:26:21 
 * *** empty log message ***
 *
 * Revision 1.6  2015/07/06 12:20:30 
 * improve trace
 *
 * Revision 1.5  2015/07/02 10:29:12 
 * clean trace, byte-buffer
 *
 * Revision 1.4  2015/06/30 13:26:45 
 * Trace 0-8 levels, trace levels by component
 *
 * Revision 1.3  2015/06/09 10:18:50 
 * trace method
 *
 * Revision 1.2  2015/03/22 14:43:20 
 * Copyright - MessageSight
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.15  2012/10/25 10:11:16 
 * Added copyright and proprietary notice
 *
 * Revision 1.14  2012/03/05 15:50:39 
 * refactoring, no functional change
 *
 * Revision 1.13  2011/06/29 07:39:21 
 * *** empty log message ***
 *
 * Revision 1.12  2011/03/10 09:38:23 
 * *** empty log message ***
 *
 * Revision 1.11  2011/02/14 08:57:27 
 * Foreign Zone Events
 *
 * Revision 1.10  2011/01/10 12:27:09 
 * No functional chage; remove double semi-colon
 *
 * Revision 1.9  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.8  2010/11/29 14:10:07 
 * change std::map to boost::unordered_map
 *
 * Revision 1.7  2010/08/16 13:38:26 
 * changed Properties to vector
 *
 * Revision 1.6  2010/07/13 13:22:12 
 * *** empty log message ***
 *
 * Revision 1.5  2010/06/27 12:38:46 
 * *** empty log message ***
 *
 * Revision 1.4  2010/06/21 11:31:32 
 * *** empty log message ***
 *
 * Revision 1.3  2010/06/17 14:07:56 
 * node attributes implementation
 *
 * Revision 1.2  2010/06/15 06:32:52 
 * *** empty log message ***
 *
 * Revision 1.1  2010/05/16 07:13:34 
 * Tracer - first version
 *
 *
 * Function: Trace buffer declaration
 */
#ifndef SCTRACE_BUFFER_H
#define SCTRACE_BUFFER_H

#include "Definitions.h"
#include "ScTrConstants.h"
#include "ScTraceable.h"
#include "ScTraceContext.h"

//#include <map>
#include <string>
#include <vector>
#include <exception>
#include <sstream>

#include <boost/lexical_cast.hpp>

namespace spdr
{

typedef std::auto_ptr<class ScTraceBuffer> ScTraceBufferAPtr;

static const char* SC_EMPTY_STRING = "";

typedef enum SC_TR_EVENT_TYPE
{
	SC_TR_PROPERTY_LIST = -1,

	SC_TR_NONE = 0,

	SC_TR_ERROR = 1,
	SC_TR_WARNING = 2,
	SC_TR_INFO = 3,
	SC_TR_CONFIG = 4,

	SC_TR_EVENT = 5,
	SC_TR_DEBUG = 6,
	SC_TR_ENTRY = 7,
	SC_TR_EXIT = 8,
	SC_TR_DUMP = 9,

	SC_TR_NUM_OF_EVENT_TYPES = 10
} ScTrEventType;

class  ScTraceInvokable
{
public:
	virtual ~ScTraceInvokable()
	{}
	/**
	 */
	virtual void invoke(void) const = 0;
	/**
	 * @param TrEventType
	 */
	virtual void invoke(ScTrEventType scTrEventType) const = 0;
};

/**
 * Trace utility.
 * Provides unified formatting for trace entries.<br>
 * Some of the arguments SHOULD have a particular format. 
 * This format is by convention and not enforced.
 */
class ScTraceBuffer : public ScTraceable, public ScTraceInvokable, public ScTraceContext
{
private:
	//typedef std::multimap<std::string, std::string> Properties;
	typedef std::vector< std::pair<std::string, std::string> > Properties; //print order is insertion order

	static const ScTraceComponent* TC;
	static const ScTraceContextImpl* PROPERTIES_TRACE_CONTEXT;

	static const int INITIAL_BUF_SIZE = 240;
	static const int FOCUS_COLUMN_WIDTH = 21;

	static const std::string AT;
	static const std::string PREFIX_SEP;
	static const std::string PREFIX_END;
	static const std::string SEP;
	static const std::string FOCUS_SEP;
	static const std::string ARRAY_START;
	static const std::string ARRAY_SEP;
	static const std::string ARRAY_END;
	static const std::string BLANK_COLUMN;
	static const std::string PROPERTY_START;
	static const std::string PROPERTY_END;
	static const std::string PROPERTY_SEP;
	static const size_t PROPERTY_END_LENGTH;
	static const size_t PROPERTY_SEP_LENGTH;

	static const std::string PROPERTY_RELATION;
	static const std::string NULL_PROPERTY_KEY;
	static const std::string TRACE_NAME;

	// Event Types
	const static std::string _eventTypeNames[SC_TR_NUM_OF_EVENT_TYPES];

	const ScTrEventType _eventType;
	std::string _traceName;
	const ScTraceContext* _ctxt;
	const std::string _methodName;
	const std::string _focusEntry;
	const std::string _description;
	Properties _properties;
	const std::string _errorCause;

	std::ostream& writeMessage(std::ostream &out) const;
	std::ostream& writeProperties(std::ostream &out) const;

protected:
	ScTraceBuffer(ScTrEventType eventType, const ScTraceContext *ctxt, const std::string &description = SC_EMPTY_STRING,
			const std::string& methodName = SC_EMPTY_STRING, const std::string& errorCause = SC_EMPTY_STRING);

	ScTraceBuffer(const ScTraceContext *ctxt, const std::string& methodName, const std::vector<std::string> &focusColumns,
			const std::string &description = SC_EMPTY_STRING);

	std::string formatFocusColumns(const std::vector<std::string> &focusColumns);

public:
	virtual ~ScTraceBuffer()
	{
	}
	template <typename T> static std::string stringValueOf(const T& value);
	template <typename T> static std::string stringValueOf(const T* array, int arrayLength);
	static std::string stringValueOf(const spdr::Exception& ex);
	static std::string stringValueOf(const std::string& str)
	{	return str;}

	static const std::string& stringValueOf(bool value);
	std::string getPropertiesString(void);
	std::vector<std::string> getPropertiesStringVector(void);
	void mergePropertyList(const ScTraceBuffer *tb);
	void addProperty(const ScTraceable &dt);
	void addProperty(const std::string &dt);
	void addProperty(const std::string &prefix, const ScTraceable &dt);

	template <typename T> void addProperty(const std::string &key, const T& value);
	template <typename T> void addProperty(const std::string &prefix,
			const std::string &key, const T& value);
	void addProperty(const std::string &key, const std::string &value);
	void addProperty(const std::string &prefix, const std::string &key, const std::string &value);
	template <typename T> void addProperty(const std::string &key, const T* array, int arrayLength);
	template <typename T> void addProperty(const std::string &prefix, const std::string &key,
			const T* array, int arrayLength);

	void addProperty(const spdr::Exception &ex);

	void addProperty(std::string prefix, const spdr::Exception &ex);

	virtual std::string toString(void) const;

	static bool isInternalInfoEnabled(const ScTraceComponent*)
	{
		return true;
	}

	static bool isInternalWarningEnabled(const ScTraceComponent*)
	{
		return true;
	}

	static bool isErrorEnabled(const ScTraceComponent *tc)
	{
		return tc->isErrorEnabeled();
	}

	static bool isWarningEnabled(const ScTraceComponent *tc)
	{
		return tc->isWarningEnabeled();
	}

	static bool isInfoEnabled(const ScTraceComponent *tc)
	{
		return tc->isInfoEnabeled();
	}

	static bool isConfigEnabled(const ScTraceComponent *tc)
	{
		return tc->isConfigEnabeled();
	}

	static bool isEventEnabled(const ScTraceComponent *tc)
	{
		return tc->isEventEnabled();
	}

	static bool isDebugEnabled(const ScTraceComponent *tc)
	{
		return tc->isDebugEnabled();
	}

	static bool isDumpEnabled(const ScTraceComponent *tc)
	{
		return tc->isDumpEnabled();
	}

	static bool isEntryEnabled(const ScTraceComponent *tc)
	{
		return tc->isEntryEnabled();
	}

	static bool isExitEnabled(const ScTraceComponent *tc)
	{
		return tc->isEntryEnabled();
	}

	static ScTraceBufferAPtr propertyList(const std::string &traceName = SC_EMPTY_STRING);

//	static ScTraceBufferAPtr focusEvent(const ScTraceContext *ctxt, const std::string &methodName,
//			const std::string &focusEvent, const std::string &description);
//	ScTraceBufferAPtr toFocusEvent(const std::string focusEvent,
//			const std::string description = SC_EMPTY_STRING);
//
//	static ScTraceBufferAPtr focusEvent(const ScTraceContext *ctxt, const std::string &methodName,
//			const std::string &column1, const std::string &column2,
//			const std::string &description);
//	ScTraceBufferAPtr toFocusEvent(const std::string &column1, const std::string &column2,
//			const std::string &description);
//
//	static ScTraceBufferAPtr focusEvent(const ScTraceContext *ctxt, const std::string &methodName,
//			const std::string &column1, const std::string &column2,
//			const std::string &column3, const std::string &description);
//	ScTraceBufferAPtr toFocusEvent(const std::string &column1, const std::string &column2,
//			const std::string &column3, const std::string &description);

	static ScTraceBufferAPtr error(const ScTraceContext *ctxt, const std::string &methodName,
				const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr error(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description, const spdr::Exception &cause);

	static ScTraceBufferAPtr warning(const ScTraceContext *ctxt, const std::string &methodName,
				const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr warning(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description, const spdr::Exception &cause);

	static ScTraceBufferAPtr info(const ScTraceContext *ctxt, const std::string &methodName,
				const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr config(const ScTraceContext *ctxt, const std::string &methodName,
				const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr event(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description = SC_EMPTY_STRING);
	ScTraceBufferAPtr toEvent(const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr debug(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description = SC_EMPTY_STRING);
	ScTraceBufferAPtr toDebug(const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr dump(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description = SC_EMPTY_STRING);
	ScTraceBufferAPtr toDump(const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr entry(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description = SC_EMPTY_STRING);
	ScTraceBufferAPtr toEntry(const std::string &description = SC_EMPTY_STRING);

	static ScTraceBufferAPtr exit(const ScTraceContext *ctxt, const std::string &methodName,
			const std::string &description = SC_EMPTY_STRING);
	ScTraceBufferAPtr toExit(const std::string &description = SC_EMPTY_STRING);

	void setTraceName(std::string newName)
	{	_traceName = (newName.empty()) ? TRACE_NAME : newName;}
	virtual const std::string& getTraceName(void) const
	{	return _traceName;}
	const ScTraceComponent* getTraceComponent() const
	{	return _ctxt->getTraceComponent();}
	const std::string& getInstanceID() const
	{	return _ctxt->getInstanceID();}
	const std::string& getMemberName() const
	{	return _ctxt->getMemberName();}
	const std::string& getDescription() const
	{	return _description;}
	const std::string& getMethodName() const
	{	return _methodName;}
	const Properties& getProperties() const
	{	return _properties;}

	void invoke(ScTrEventType eventType) const;
	void invoke() const;

	static void setStaticVariables(bool init); // Internal service function.
};

/*****************************************************************************/
template<typename T> std::string ScTraceBuffer::stringValueOf(const T& value)
{
	std::ostringstream strBuff;
	strBuff << value;
	return strBuff.str();
}

template<typename T> std::string ScTraceBuffer::stringValueOf(const T* array,
		int arrayLength)
{
	if (array == NULL)
		return SC_EMPTY_STRING;
	std::ostringstream strBuff;
	strBuff << ARRAY_START << ARRAY_SEP;
	for (int i = 0; i < arrayLength; i++)
	{
		strBuff << array[i] << ARRAY_SEP;
	}
	strBuff << ARRAY_END;
	return strBuff.str();
}

template<typename T> void ScTraceBuffer::addProperty(const std::string &key,
		const T& value)
{
	addProperty(SC_EMPTY_STRING, key, value);
}

template<typename T> void ScTraceBuffer::addProperty(const std::string &prefix,
		const std::string &key, const T& value)
{
	addProperty(prefix, key, stringValueOf(value));
}

template<typename T> void ScTraceBuffer::addProperty(const std::string &key,
		const T* array, int arrayLength)
{
	addProperty(SC_EMPTY_STRING, key, array, arrayLength);
}

template<typename T> void ScTraceBuffer::addProperty(const std::string &prefix,
		const std::string &key, const T* array, int arrayLength)
{
	addProperty(prefix, key, stringValueOf(array, arrayLength));
}

/*****************************************************************************/
// GLOBAL ENTRY/EXIT FUNCTIONS

//Trace Exit
template <typename T>
void Trace_Exit(const ScTraceContext *ctxt, const std::string &methodName,
		const T& returnCode)
{
	if (ScTraceBuffer::isExitEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::exit(ctxt, methodName);
		buffer->addProperty(ScTraceable::RETURN_CODE, returnCode);
		buffer->invoke();
	}
}

//Trace Exit 0/3
void Trace_Exit(const ScTraceContext *ctxt, const std::string &methodName,
		const ScTraceable &dt);
//Trace Exit 1/3
void Trace_Exit(const ScTraceContext *ctxt, const std::string &methodName);
//Trace Exit 2/3
void Trace_Exit(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix, const std::string &str);
//Trace Exit 3/3
void Trace_Exit(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description);

//Trace Debug 
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description,
		const std::string &prefix, const ScTraceable &dt);
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description,
		const std::string &prefix, const std::string &str);
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix, const ScTraceable &dt);
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description);
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3);
void Trace_Debug(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4);

#define TRACE_DEBUG2(ctxt, a, b)                                          \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b);}}

#define TRACE_DEBUG3(ctxt, a, b, c)                                       \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b, c);}}

#define TRACE_DEBUG4(ctxt, a, b, c, d)                                    \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b, c, d);}}

#define TRACE_DEBUG5(ctxt, a, b, c, d, e)                                 \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b, c, d, e);}}

#define TRACE_DEBUG6(ctxt, a, b, c, d, e, f)                              \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b, c, d, e, f);}}

#define TRACE_DEBUG8(ctxt, a, b, c, d, e, f, g, h)                        \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b, c, d, e, f, g, h);}}

#define TRACE_DEBUG10(ctxt, a, b, c, d, e, f, g, h, i, j)                 \
        {if (ScTraceBuffer::isDebugEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Debug((ctxt), a, b, c, d, e, f, g, h, i, j);}}

//=== Trace Entry ===

template <typename T>
void Trace_Entry(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string& prefix, const T& data)
{
	if (ScTraceBuffer::isEntryEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(ctxt, methodName);
		buffer->addProperty(prefix, data);
		buffer->invoke();
	}
}

//1
void  Trace_Entry(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string& description = "");
//2
void  Trace_Entry(const ScTraceContext *ctxt, const std::string &methodName,
		const ScTraceable &dt);
//3
void  Trace_Entry(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix, const std::string &str);
//5
void  Trace_Entry(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);
//7
void  Trace_Entry(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3);


#define TRACE_ENTRY1(ctxt, a)                                          \
        {if (ScTraceBuffer::isEntryEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Entry((ctxt), a);}}
#define TRACE_ENTRY2(ctxt, a, b)                                       \
        {if (ScTraceBuffer::isEntryEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Entry((ctxt), a, b);}}
#define TRACE_ENTRY3(ctxt, a, b, c)                                    \
        {if (ScTraceBuffer::isEntryEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Entry((ctxt), a, b, c);}}
#define TRACE_ENTRY5(ctxt, a, b, c, d, e)                              \
        {if (ScTraceBuffer::isEntryEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Entry((ctxt), a, b, c, d, e);}}
#define TRACE_ENTRY7(ctxt, a, b, c, d, e, f, g)                        \
        {if (ScTraceBuffer::isEntryEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Entry((ctxt), a, b, c, d, e, f, g);}}

//=== Trace Dump ===

//1
void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description = "");

void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1);

void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);

void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3);

void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4);

//=== Trace Event ===

//1
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description = "");
//2 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const ScTraceable &dt);
//3 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix, const std::string &str);
//4
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix, const std::string &str); //4
//5 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2); //5
//6 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const ScTraceable &dt,
		const std::string &prefix2,	const std::string &str2);
//7 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix1, const std::string &prefix11, const std::string &str1,
		const std::string &prefix2,	const std::string &prefix22, const std::string &str2);
//8 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4);
//9 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const ScTraceable &dt,
		const std::string &prefix2, const std::string &str2);
//10 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const ScTraceable &dt,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);
//11 -
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);
//12
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3);
//13
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4);

//14
void Trace_Event(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4,
		const std::string &prefix5, const std::string &str5);

//15
template <typename T>
void  Trace_Event(const ScTraceContext *ctxt, const std::string &methodName, const std::string &description,
		const std::string &prefix, const T& data)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName, description);
		buffer->addProperty(prefix, data);
		buffer->invoke();
	}
}

#define TRACE_EVENT2(ctxt, a, b)                                          \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b);}}
#define TRACE_EVENT3(ctxt, a, b, c)                                       \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c);}}
#define TRACE_EVENT4(ctxt, a, b, c, d)                                       \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c, d);}}
#define TRACE_EVENT5(ctxt, a, b, c, d, e)                                 \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c, d, e);}}
#define TRACE_EVENT6(ctxt, a, b, c, d, e, f)                              \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c, d, e, f);}}
#define TRACE_EVENT8(ctxt, a, b, c, d, e, f, g, h)                        \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c, d, e, f, g, h);}}
#define TRACE_EVENT10(ctxt, a, b, c, d, e, f, g, h, i, j)                 \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c, d, e, f, g, h, i, j);}}
#define TRACE_EVENT12(ctxt, a, b, c, d, e, f, g, h, i, j, k, l)           \
        {if (ScTraceBuffer::isEventEnabled((ctxt)->getTraceComponent()))  \
            {Trace_Event((ctxt), a, b, c, d, e, f, g, h, i, j, k, l);}}

//=== Trace Error ===
//1
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description = "");
//2
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1);
//3
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);
//4
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3);
//5
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const int rc);
//6
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const int rc);
//7
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4);

//=== Trace Warning ===
//1
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description = "");
//2
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1);
//3
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);
//4
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3);

//=== Trace Info ===
//1
void Trace_Info(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description = "");
//2
void Trace_Info(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1);

//=== Trace Config ===
//1
void Trace_Config(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description = "");
//2
void Trace_Config(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1);
//3
void Trace_Config(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2);

String toHexString(uint64_t l, bool uppercase = false);

template <class T> std::string stringValueOf(const T& value)
{
	std::ostringstream strBuff;
	strBuff << value;
	return strBuff.str();
}

template <> inline std::string stringValueOf<int16_t>(const int16_t& value)
{
	return boost::lexical_cast<std::string>(value);
}
template <> inline std::string stringValueOf<uint16_t>(const uint16_t& value)
{
	return boost::lexical_cast<std::string>(value);
}
template <> inline std::string stringValueOf<int32_t>(const int32_t& value)
{
	return boost::lexical_cast<std::string>(value);
}
template <> inline std::string stringValueOf<uint32_t>(const uint32_t& value)
{
	return boost::lexical_cast<std::string>(value);
}
template <> inline std::string stringValueOf<int64_t>(const int64_t& value)
{
	return boost::lexical_cast<std::string>(value);
}
template <> inline std::string stringValueOf<uint64_t>(const uint64_t& value)
{
	return boost::lexical_cast<std::string>(value);
}

} //namespace spdr

#endif //SCTRACE_BUFFER_H
