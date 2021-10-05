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
 * File        : ScTracebuffer.cpp
 * Author      :
 * Version     : $Revision: 1.16 $
 * Last updated: $Date: 2015/12/22 11:56:40 $
 * Lab         : Haifa Research Lab, IBM Israel
 *-----------------------------------------------------------------
 * $Log: ScTraceBuffer.cpp,v $
 * Revision 1.16  2015/12/22 11:56:40 
 * print typeid in catch std::exception; fix toString of sons of AbstractTask.
 *
 * Revision 1.15  2015/11/18 08:37:04 
 * Update copyright headers
 *
 * Revision 1.14  2015/11/11 14:41:14 
 * handle EVENT_STREAM_NOT_PRESENT from RUM
 *
 * Revision 1.13  2015/11/03 14:36:44 
 * trace
 *
 * Revision 1.12  2015/08/31 11:54:26 
 * *** empty log message ***
 *
 * Revision 1.11  2015/08/27 10:53:53 
 * change RUM log level dynamically
 *
 * Revision 1.10  2015/08/09 09:42:51 
 * fix compilation with g++ 4.8.2 and -std=gnu++11
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
 * Revision 1.2  2015/05/20 11:09:31 
 * *** empty log message ***
 *
 * Revision 1.1  2015/03/22 12:29:19 
 * First version in GPFS 22/3/2015
 *
 * Revision 1.13  2012/10/25 10:11:19 
 * Added copyright and proprietary notice
 *
 * Revision 1.12  2011/06/29 07:39:21 
 * *** empty log message ***
 *
 * Revision 1.11  2011/03/10 09:38:23 
 * *** empty log message ***
 *
 * Revision 1.10  2011/01/09 09:12:58 
 * Hierarchy branch merged in to HEAD
 *
 * Revision 1.9.4.1  2010/11/08 07:58:56 
 * *** empty log message ***
 *
 * Revision 1.9  2010/08/16 13:38:25 
 * changed Properties to vector
 *
 * Revision 1.8  2010/08/15 11:48:45 
 * Eliminate compilation warnings, no functional change
 *
 * Revision 1.7  2010/08/03 14:27:01 
 * disconnected neighbor task
 *
 * Revision 1.6  2010/07/13 13:22:12 
 * *** empty log message ***
 *
 * Revision 1.5  2010/07/06 08:59:23 
 * *** empty log message ***
 *
 * Revision 1.4  2010/06/27 12:38:47 
 * *** empty log message ***
 *
 * Revision 1.3  2010/06/21 11:31:32 
 * *** empty log message ***
 *
 * Revision 1.2  2010/06/15 06:32:53 
 * *** empty log message ***
 *
 * Revision 1.1  2010/05/16 07:13:33 
 * Tracer - first version
 *
 *
 * Function: Trace buffer implementation
 */
#include "ScTraceBuffer.h"
//#include "Thread.h"
#include <string>

#include <iostream>


namespace spdr
{

using namespace std;

const string ScTraceBuffer::AT = " at ";
const string ScTraceBuffer::PREFIX_SEP = ".";
const string ScTraceBuffer::PREFIX_END = ":";

const string ScTraceBuffer::SEP = " ";
const string ScTraceBuffer::ARRAY_START = "[";
const string ScTraceBuffer::ARRAY_SEP = " ";
const string ScTraceBuffer::ARRAY_END = "]";
const string ScTraceBuffer::FOCUS_SEP = ", ";
const string ScTraceBuffer::BLANK_COLUMN(FOCUS_COLUMN_WIDTH,' ');
const string ScTraceBuffer::PROPERTY_START = " {";
const string ScTraceBuffer::PROPERTY_END = "} ";
const string ScTraceBuffer::PROPERTY_SEP = ", ";

const size_t ScTraceBuffer::PROPERTY_END_LENGTH = PROPERTY_END.length();
const size_t ScTraceBuffer::PROPERTY_SEP_LENGTH = PROPERTY_SEP.length();

const string ScTraceBuffer::PROPERTY_RELATION = "=";
const string ScTraceBuffer::NULL_PROPERTY_KEY = "NullPropertyKey";
const string ScTraceBuffer::TRACE_NAME = "ScTraceBuffer";

const string ScTraceBuffer::_eventTypeNames[SC_TR_NUM_OF_EVENT_TYPES] =
            {"NONE","ERROR","WARNING","INFO","CONFIG","EVENT","DEBUG","ENTRY","EXIT","DUMP"};

const ScTraceContextImpl*  ScTraceBuffer::PROPERTIES_TRACE_CONTEXT;
const ScTraceComponent*    ScTraceBuffer::TC;

ostream& ScTraceBuffer::writeMessage(ostream &out) const

{
	switch (_eventType)
	{
	case SC_TR_PROPERTY_LIST:
		return out;

	default:
		out << _ctxt->getTraceComponent()->getTracePrefix();
		if (!_methodName.empty())
		{
			out << PREFIX_SEP << _methodName;
			if (_eventType == SC_TR_ENTRY)
			{
				out << PREFIX_SEP << "Entry";
			}
			if (_eventType == SC_TR_EXIT)
			{
				out << PREFIX_SEP << "Exit";
			}

		}

		if (!((_ctxt->getInstanceID()).empty()))
		{
			out << SEP << _ctxt->getInstanceID() << PREFIX_END;
			//out << SEP << _ctxt->getInstanceID();
		}

		if (!((_ctxt->getMemberName()).empty()))
		{
			out << SEP << _ctxt->getMemberName() << PREFIX_END;
			//out << AT << _ctxt->getMemberName();
		}

		//            if (_eventType == TR_FOCUS_EVENT) {
		//                out << _focusEntry;
		//            }
	}
	if (!_description.empty())
	{
		out << SEP << '"' << _description << '"' << ' ';
	}
	return out;
}

ostream& ScTraceBuffer::writeProperties(ostream& out) const
{
	if (_properties.size() == 0)
	{
		if (_eventType == SC_TR_PROPERTY_LIST)
		{
			out << PROPERTY_START << " " << PROPERTY_END;
		}
		return out;
	}

	bool firstProperty = true;
	out << PROPERTY_START;
	Properties::iterator pos;
	Properties* props = const_cast<Properties*> (&_properties);
	for (pos = props->begin(); pos != props->end(); ++pos)
	{
		if (firstProperty)
		{
			firstProperty = false;
		}
		else
		{
			out << PROPERTY_SEP;
		}
		out << pos->first << PROPERTY_RELATION << pos->second;
	}
	out << PROPERTY_END;
	return out;
}

void ScTraceBuffer::invoke(ScTrEventType eventType) const {
    switch (eventType)
    {

    case SC_TR_ERROR :
    	ScTr::error(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_WARNING :
    	ScTr::warning(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_INFO :
    	ScTr::info(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_CONFIG :
    	ScTr::config(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_EVENT :
    	ScTr::event(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_DEBUG :
    	ScTr::debug(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_ENTRY :
    	ScTr::entry(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_EXIT :
    	ScTr::exit(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_DUMP :
    	ScTr::dump(_ctxt->getTraceComponent(), toString());
    	break;

    case SC_TR_PROPERTY_LIST :
    	// TODO should we do anything here ?

    default :
    	break;
    }
}

/*****************************************************************************/

ScTraceBuffer::ScTraceBuffer(ScTrEventType eventType, const ScTraceContext *ctxt, const string &description,
        const string& methodName, const string& errorCause)
    : ScTraceContext(ctxt->getTraceComponent(), ctxt->getInstanceID(), ctxt->getMemberName()),
    _eventType(eventType),
    _traceName(TRACE_NAME),
    _ctxt(ctxt),
    _methodName(methodName),
    _focusEntry(),
    _description(description),
    _properties(),
    _errorCause(errorCause)
{
		//addProperty("tid", Thread::getCurrentThreadId());
}

ScTraceBuffer::ScTraceBuffer(const ScTraceContext *ctxt, const string& methodName,
        const std::vector<string> &focusColumns,
        const string &description)
    : ScTraceContext(ctxt->getTraceComponent(), ctxt->getInstanceID(), ctxt->getMemberName()),
    _eventType(SC_TR_EVENT),
    _traceName(TRACE_NAME),
    _ctxt(ctxt),
    _methodName(methodName),
    _focusEntry(formatFocusColumns(focusColumns)),
    _description(description),
    _properties(),
    _errorCause()
    {
}

string ScTraceBuffer::formatFocusColumns(const std::vector<string> &focusColumns) {
    if (focusColumns.empty()) return SC_EMPTY_STRING;
    size_t numOfColumns = focusColumns.size();

    ostringstream   strBuff;
    for (size_t i=0, col = 0; i<numOfColumns; i++) {
        strBuff << FOCUS_SEP << focusColumns[i];
        col += FOCUS_COLUMN_WIDTH;
        size_t size = strBuff.str().size();//TODO: Consider to remove padding
        // sysAssert (col >= size);         !!!!
        if (col > size) {
            size_t padLen = col - size;
            strBuff << BLANK_COLUMN.substr(0, padLen);
        } else { 
            // Just for sake of runtime robustness. 
            col = size; 
        } 
    }
    return strBuff.str();
}

const string& ScTraceBuffer::stringValueOf(bool value){
    if(value){
        return ScTraceable::TRUE_STR;
    }else{
        return ScTraceable::FALSE_STR;
    }
}

string ScTraceBuffer::getPropertiesString(void) {
    ostringstream   strBuff;
    writeProperties(strBuff);
    return strBuff.str();
}

std::vector<string> ScTraceBuffer::getPropertiesStringVector(void)
{
	std::vector<string> props(_properties.size());
	Properties::iterator pos;
	int i;
	for (i = 0, pos = _properties.begin(); pos != _properties.end(); ++pos, ++i)
	{
		props[i] = pos->first + PROPERTY_RELATION + pos->second;
	}
	return props;
}

void ScTraceBuffer::mergePropertyList(const ScTraceBuffer *tb)
{
	const Properties& props = tb->getProperties();
	Properties::const_iterator pos;
	for (pos = props.begin(); pos != props.end(); ++pos)
	{
		//_properties.insert(*pos);
		_properties.push_back(*pos);
	}
}

/**
    * Append a property to the trace using the default trace name.
    * Format is dt.getTraceName()+PROPERTY_RELATION+dt.tostring().
    * 
    * @param dt property object to add
    * @return a reference to this ScTraceBuffer
    */
void ScTraceBuffer::addProperty(const ScTraceable &dt) {
    addProperty(SC_EMPTY_STRING, dt);
}

void ScTraceBuffer::addProperty(const string &dt) {
    addProperty(SC_EMPTY_STRING, dt);
}

/**
    * Append a property to the trace string using a prefixed default trace name
    * Format is prefix+":"+dt.getTraceName()+PROPERTY_RELATION+dt.tostd::string().
    * This form appends a prefix to the default name allowing different property names 
    * for objects of the same type.
    *   
    * @param prefix prefix to be added before default name
    * @param dt property object to add
    * @return a reference to this DCSScTraceBuffer
    */
void ScTraceBuffer::addProperty(const string &prefix, const ScTraceable &dt) {
	if (&dt == this){
        addProperty(prefix, dt.getTraceName(), "this");
    } else {
        addProperty(prefix, dt.getTraceName(), dt.toString());
    }
}

/**
    * Append a Throwable property to the trace string.
    * Format is class+PROPERTY_RELATION+{@link #throwableTostd::string(Throwable)}.
    * 
    * @param aThrowable a throwable
    * @return a reference to this DCSScTraceBuffer
    */
void ScTraceBuffer::addProperty(const spdr::Exception &ex) {
    addProperty(SC_EMPTY_STRING, ex);
}

/**
    * Append a Throwable property to the trace string.
    * Format is prefix+":"+class+PROPERTY_RELATION+{@link #throwableTostd::string(Throwable)}.
    * This form appends a prefix to the class name allowing different property names 
    * for throwables of the same type.
    * 
    * @param prefix prefix to be added before class name
    * @param aThrowable a throwable
    * @return a reference to this DCSScTraceBuffer
    */
void ScTraceBuffer::addProperty(string prefix, const spdr::Exception &ex) {
    addProperty(prefix,stringValueOf(ex));
}

/**
 * @name    ScTraceBuffer::stringValueOf
 * @dscr    
 */
string ScTraceBuffer::stringValueOf(const spdr::Exception& ex){
    std::ostringstream  strBuff;
    strBuff << ScTraceable::EXCEPTION << ":" << ex.what();
    return strBuff.str();
}

/**
 * @name    ScTraceBuffer::stringValueOf
 * @dscr    
 */
void ScTraceBuffer::addProperty(const string &key, const string &value)
{
	if (!key.empty())
	{
		//_properties.insert(make_pair<const string, string> (key, value));
		_properties.push_back(make_pair(key, value));
	}
	else
	{
		//_properties.insert(make_pair<const string, string> (NULL_PROPERTY_KEY, value));
		_properties.push_back(make_pair(NULL_PROPERTY_KEY, value));
	}
}

/**
 * @name    ScTraceBuffer::stringValueOf
 * @dscr    
 */
void ScTraceBuffer::addProperty(const string &prefix, const string &key,
		const string &value)
{
	string mapKey;
	if (!prefix.empty())
	{
		mapKey = prefix + PREFIX_SEP + key;
	}
	if (!key.empty())
	{
		mapKey += key;
	}
	else
	{
		mapKey += NULL_PROPERTY_KEY;
	}
	//_properties.insert(make_pair<const string, string> (mapKey, value));
	//_properties.push_back(make_pair<const string, string> (mapKey, value));
	_properties.push_back(make_pair(mapKey, value));
}

string ScTraceBuffer::toString(void) const {
    ostringstream   strBuff;
    writeMessage(strBuff);
    writeProperties(strBuff);
    return strBuff.str();
}

/**
    * Create an empty trace buffer with a given trace name.
    * This is a utility call to generate a formatted property list.
    * Use the addProperty() functions and then call {@link #tostd::string()}.
    */
ScTraceBufferAPtr ScTraceBuffer::propertyList(const string &traceName) {
    ScTraceBufferAPtr tb = ScTraceBufferAPtr(
            new ScTraceBuffer(SC_TR_PROPERTY_LIST, PROPERTIES_TRACE_CONTEXT));
    if(!traceName.empty()){
        tb->setTraceName(traceName);
    }
    return tb;
}

///**
//    * Create a trace formatted for a focus event.
//    * Each entry indicates a high level protocol event.
//    * Details should be added through properties.
//    *
//    * @param dtc
//    * @param methodName
//    * @param focusEvent
//    * @param description
//    * @return new DCSScTraceBuffer with formatted event
//    */
//ScTraceBufferAPtr ScTraceBuffer::focusEvent(const ScTraceContext *ctxt, const string &methodName,
//                                    const string &focusEvent, const string &description) {
//    std::vector<string> focusColumns(1);
//    focusColumns[0] = focusEvent;
//    return ScTraceBufferAPtr(new ScTraceBuffer(ctxt,methodName,focusColumns, description));
//}
//
//ScTraceBufferAPtr ScTraceBuffer::toFocusEvent(const string focusEvent, const string description) {
//    std::vector<string> focusColumns(1);
//    focusColumns[0] = focusEvent;
//    return ScTraceBufferAPtr(new ScTraceBuffer(_ctxt,_methodName, focusColumns,description));
//}
//
///**
//    * Create a trace formatted for a focus table entry.
//    * Each entry indicates a state and and event.
//    * These should be the high level protocol states.
//    * Details should be added through properties.
//    *
//    * @param dtc
//    * @param methodName
//    * @param focusEvent
//    * @param description
//    * @return new DCSScTraceBuffer with formatted event
//    */
//ScTraceBufferAPtr ScTraceBuffer::focusEvent(const ScTraceContext *ctxt, const string &methodName,
//                                    const string &column1, const string &column2,
//                                    const string &description) {
//    std::vector<string> focusColumns(2);
//    focusColumns[0] = column1;
//    focusColumns[1] = column2;
//    return ScTraceBufferAPtr(new ScTraceBuffer(ctxt,methodName,focusColumns, description));
//}
//
//ScTraceBufferAPtr ScTraceBuffer::toFocusEvent(const string &column1, const string &column2,
//                         const string &description) {
//    std::vector<string> focusColumns(2);
//    focusColumns[0] = column1;
//    focusColumns[1] = column2;
//    return ScTraceBufferAPtr(new ScTraceBuffer(_ctxt,_methodName, focusColumns,description));
//}
//
///**
//    * Create a trace formatted for a focus table entry.
//    * Each entry indicates a state and and event.
//    * These should be the high level protocol states.
//    * Details should be added through properties.
//    *
//    * @param dtc
//    * @param methodName
//    * @param focusEvent
//    * @param description
//    * @return new DCSScTraceBuffer with formatted event
//    */
//ScTraceBufferAPtr ScTraceBuffer::focusEvent(const ScTraceContext *ctxt, const string &methodName,
//                                    const string &column1, const string &column2,
//                                    const string &column3, const string &description) {
//    std::vector<string> focusColumns(3);
//    focusColumns[0] = column1;
//    focusColumns[1] = column2;
//    focusColumns[2] = column3;
//    return ScTraceBufferAPtr(new ScTraceBuffer(ctxt,methodName,focusColumns, description));
//}
//
//ScTraceBufferAPtr ScTraceBuffer::toFocusEvent(const string &column1, const string &column2,
//                                      const string &column3, const string &description) {
//    std::vector<string> focusColumns(3);
//    focusColumns[0] = column1;
//    focusColumns[1] = column2;
//    focusColumns[2] = column3;
//    return ScTraceBufferAPtr(new ScTraceBuffer(_ctxt,_methodName, focusColumns,description));
//}

ScTraceBufferAPtr ScTraceBuffer::error(const ScTraceContext *ctxt, const string &methodName,
                               const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_ERROR,ctxt, description, methodName));
}


ScTraceBufferAPtr ScTraceBuffer::error(const ScTraceContext *ctxt, const string &methodName,
		const string &description, const spdr::Exception &cause)
{
	return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_ERROR, ctxt,
			description, methodName, string(cause.what())));
}

ScTraceBufferAPtr ScTraceBuffer::warning(const ScTraceContext *ctxt, const string &methodName,
                               const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_WARNING,ctxt, description, methodName));
}

ScTraceBufferAPtr ScTraceBuffer::warning(const ScTraceContext *ctxt, const string &methodName,
		const string &description, const spdr::Exception &cause)
{
	return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_WARNING, ctxt,
			description, methodName, string(cause.what())));
}


ScTraceBufferAPtr ScTraceBuffer::info(const ScTraceContext *ctxt, const string &methodName,
                               const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_INFO,ctxt, description, methodName));
}

ScTraceBufferAPtr ScTraceBuffer::config(const ScTraceContext *ctxt, const string &methodName,
                               const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_CONFIG,ctxt, description, methodName));
}

/**
    * Create a trace formatted for a generic event.
    * 
    * @param dtc 
    * @param methodName
    * @param focusEvent
    * @param description
    * @return new DCSScTraceBuffer with formatted event
    */
ScTraceBufferAPtr ScTraceBuffer::event(const ScTraceContext *ctxt, const string &methodName,
                               const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_EVENT,ctxt, description, methodName));
}

ScTraceBufferAPtr ScTraceBuffer::toEvent(const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_EVENT, _ctxt, description, _methodName));
}
    
/**
    * Create a trace formatted for a debug trace event
    * 
    * @param dtc 
    * @param methodName
    * @param description
    * @return new DCSScTraceBuffer with formatted event
    */
ScTraceBufferAPtr ScTraceBuffer::debug(const ScTraceContext *ctxt, const string &methodName,
                               const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_DEBUG,ctxt, description, methodName));
}

ScTraceBufferAPtr ScTraceBuffer::toDebug(const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_DEBUG, _ctxt, description, _methodName));
}
/**
    * Create a trace formatted for a dump trace event
    * 
    * @param dtc 
    * @param methodName
    * @param focusEvent
    * @param description
    * @return new DCSScTraceBuffer with formatted event
    */

ScTraceBufferAPtr ScTraceBuffer::dump(const ScTraceContext *ctxt, const string &methodName,
                              const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_DUMP,ctxt, description, methodName));
}

ScTraceBufferAPtr ScTraceBuffer::toDump(const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_DUMP, _ctxt, description, _methodName));
}

/**
    * Create a trace formatted for an entry trace event
    * 
    * @param dtc 
    * @param methodName
    * @param focusEvent
    * @param description
    * @return new DCSScTraceBuffer with formatted event
    */
ScTraceBufferAPtr ScTraceBuffer::entry(const ScTraceContext *ctxt, const string &methodName,
                  const string &description) {
    ScTraceBufferAPtr buffer = ScTraceBufferAPtr(
            new ScTraceBuffer(SC_TR_ENTRY, ctxt, description, methodName));
    return buffer;
}

ScTraceBufferAPtr ScTraceBuffer::toEntry(const string &description) {
        return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_ENTRY, _ctxt, description, _methodName));
}

/**
    * Create a trace formatted for an exit trace event
    * 
    * @param dtc 
    * @param methodName
    * @param focusEvent
    * @param description
    * @return new DCSScTraceBuffer with formatted event
    */
ScTraceBufferAPtr ScTraceBuffer::exit(const ScTraceContext *ctxt, const string &methodName,
                                  const string &description) {
    ScTraceBufferAPtr buffer = ScTraceBufferAPtr(
            new ScTraceBuffer(SC_TR_EXIT,ctxt, description, methodName));
    return buffer;
}

ScTraceBufferAPtr ScTraceBuffer::toExit(const string &description) {
    return ScTraceBufferAPtr(new ScTraceBuffer(SC_TR_EXIT, _ctxt, description, _methodName));
}

/**
     * Call the TraceComponent to actually log the event.
     * Maps DCS events to RAS events.
     * 
     * @see com.ibm.ejs.ras.TraceComponent
     */
void ScTraceBuffer::invoke() const {
    invoke(_eventType);
}

/**
 * @name    ScTraceBuffer::setStaticVariables
 * @param   init - whether it is initialization or destruction.  
 * @dscr    Activates initialization of static variables if 'init' is true. 
 * @dscr    Activates destruction of static variables if 'init' is false. 
 */
void ScTraceBuffer::setStaticVariables(bool init)
{
	if (init)
	{
		TC = ScTr::enroll(
				spdr::trace::ScTrConstants::ScTr_Component_Name,
				spdr::trace::ScTrConstants::ScTr_SubComponent_Trace,
				spdr::trace::ScTrConstants::Layer_ID_Trace, "ScTraceBuffer", "");
		PROPERTIES_TRACE_CONTEXT = new ScTraceContextImpl(TC, "", "");
	}
	else
	{
		delete PROPERTIES_TRACE_CONTEXT;
	}
} 

/*****************************************************************************/

//Trace Exit 0/3
//void Trace_Exit(const ScTraceContext *ctxt, const string &methodName, const Traceable &dt) {
void Trace_Exit(const ScTraceContext *ctxt, const string &methodName, const ScTraceable& dt) {
        if (ScTraceBuffer::isExitEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::exit(ctxt, methodName);
        buffer->addProperty(dt);
        buffer->invoke(); 
    }
}

//Trace Exit 1/3
void Trace_Exit(const ScTraceContext *ctxt, const string &methodName) {
        if (ScTraceBuffer::isExitEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::exit(ctxt, methodName);
        buffer->invoke(); 
    }
}

//Trace Exit 2/3
void Trace_Exit(const ScTraceContext *ctxt, const string &methodName,
                const string &prefix, const string &str) {
        if (ScTraceBuffer::isExitEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::exit(ctxt, methodName);
        buffer->addProperty(prefix, str);
        buffer->invoke();
    }
}
//Trace Exit 3/3
void Trace_Exit(const ScTraceContext *ctxt, const string &methodName,
                const string &description) {
        if (ScTraceBuffer::isExitEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::exit(ctxt, methodName,description);
        buffer->invoke(); 
    }
}

//Trace Debug 1
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName, const string &description,
                 const string &prefix, const ScTraceable &dt) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName, description);
        buffer->addProperty(prefix, dt);
        buffer->invoke(); 
    }
}

//Trace Debug 2
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName, const string &description,
                 const string &prefix, const string &str) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName, description);
        buffer->addProperty(prefix, str);
        buffer->invoke(); 
    }
}

//Trace Debug 3
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix, const ScTraceable &dt) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName);
        buffer->addProperty(prefix, dt);
        buffer->invoke(); 
    }
}

//Trace Debug 4
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName, const string &description,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName, description);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->invoke(); 
    }
}

//Trace Debug 5
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName, const string &description) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName, description);
        buffer->invoke();
    }
}

//Trace Debug 6
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName, const string &description,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2,
                 const string &prefix3, const string &str3) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName, description);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->addProperty(prefix3, str3);
        buffer->invoke();
    }
}

//Trace Debug 7/7
void Trace_Debug(const ScTraceContext *ctxt, const string &methodName, const string &description,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2,
                 const string &prefix3, const string &str3,
                 const string &prefix4, const string &str4) {
    if (ScTraceBuffer::isDebugEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::debug(ctxt, methodName, description);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->addProperty(prefix3, str3);
        buffer->addProperty(prefix4, str4);
        buffer->invoke();
    }
}
//=== Trace_Entry ===

//Trace Entry 1
void Trace_Entry(const ScTraceContext *ctxt, const string &methodName,
		const string& description) {
    if (ScTraceBuffer::isEntryEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(ctxt, methodName, description);
        buffer->invoke(); 
    }
}

//Trace Entry 2
void Trace_Entry(const ScTraceContext *ctxt, const string &methodName,
		const ScTraceable &dt) {
    if (ScTraceBuffer::isEntryEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(ctxt, methodName);
        buffer->addProperty(dt);
        buffer->invoke(); 
    }
}

//Trace Entry 3
void Trace_Entry(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix, const string &str) {
    if (ScTraceBuffer::isEntryEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(ctxt, methodName);
        buffer->addProperty(prefix, str);
        buffer->invoke(); 
    }
}

//Trace Entry 4
void Trace_Entry(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2) {
    if (ScTraceBuffer::isEntryEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(ctxt, methodName);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->invoke();
    }
}

//Trace Entry 5
void Trace_Entry(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2,
                 const string &prefix3, const string &str3) {
    if (ScTraceBuffer::isEntryEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::entry(ctxt, methodName);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->addProperty(prefix3, str3);
        buffer->invoke();
    }
}


//Trace Dump 1
void Trace_Dump(const ScTraceContext *ctxt, const string &methodName, const string& description)
{
	if (ScTraceBuffer::isDumpEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::dump(ctxt, methodName,
				description);
		buffer->invoke();
	}
}

//Trace Dump 2
void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1)
{
	if (ScTraceBuffer::isDumpEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::dump(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->invoke();
	}
}

//Trace Dump 3
void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2)
{
	if (ScTraceBuffer::isDumpEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::dump(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}

//Trace Dump 4
void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3)
{
	if (ScTraceBuffer::isDumpEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::dump(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->invoke();
	}
}


//Trace Dump 5
void Trace_Dump(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4)
{
	if (ScTraceBuffer::isDumpEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::dump(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->addProperty(prefix4, str4);
		buffer->invoke();
	}
}
//Trace Event ===

//Trace Event 1
void Trace_Event(const ScTraceContext *ctxt, const string &methodName, const string& description)
{
    if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName, description);
        buffer->invoke();
    }
}

//Trace Event 2
void Trace_Event(const ScTraceContext *ctxt, const string &methodName, const string &description,
		const ScTraceable &dt)
{
    if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName, description);
        buffer->addProperty(dt);
        buffer->invoke();
    }
}

//Trace Event 3
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix, const string &str) {
    if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName);
        buffer->addProperty(prefix, str);
        buffer->invoke();
    }
}
//Trace Event 4
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const string &prefix, const string &str)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName,
				description);
		buffer->addProperty(prefix, str);
		buffer->invoke();
	}
}

//Trace Entry 5
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2) {
    if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->invoke();
    }
}

//Trace Event 6
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const ScTraceable &dt,
		const string &prefix2, const string &str2)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName);
		buffer->addProperty(dt);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}

//Trace Event 7
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &prefix1, const string &prefix11, const string &str1,
		const string &prefix2, const string &prefix22, const string &str2)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName);
		buffer->addProperty(prefix1, prefix11, str1);
		buffer->addProperty(prefix2, prefix22, str2);
		buffer->invoke();
	}
}

//Trace Event 8
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
                 const string &prefix1, const string &str1,
                 const string &prefix2, const string &str2,
                 const string &prefix3, const string &str3,
                 const string &prefix4, const string &str4) {

    if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent())) {
        ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName);
        buffer->addProperty(prefix1, str1);
        buffer->addProperty(prefix2, str2);
        buffer->addProperty(prefix3, str3);
        buffer->addProperty(prefix3, str4);
        buffer->invoke();
    }
}

//Trace Event 9
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const ScTraceable &dt,
		const string &prefix2, const string &str2)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName,
				description);
		buffer->addProperty(dt);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}


//Trace Event 10
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const ScTraceable &dt,
		const string &prefix1, const string &str1,
		const string &prefix2, const string &str2)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName,
				description);
		buffer->addProperty(dt);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}

//Trace Event 11
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const string &prefix1, const string &str1,
		const string &prefix2, const string &str2)
{
	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName, description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}

//Trace Event 12
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const string &prefix1, const string &str1,
		const string &prefix2, const string &str2,
		const string &prefix3, const string &str3)
{

	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->invoke();
	}
}

//Trace Event 13
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const string &prefix1, const string &str1,
		const string &prefix2, const string &str2,
		const string &prefix3, const string &str3,
		const string &prefix4, const string &str4)
{

	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->addProperty(prefix4, str4);
		buffer->invoke();
	}
}


//Trace Event 14
void Trace_Event(const ScTraceContext *ctxt, const string &methodName,
		const string &description,
		const string &prefix1, const string &str1,
		const string &prefix2, const string &str2,
		const string &prefix3, const string &str3,
		const string &prefix4, const string &str4,
		const string &prefix5, const string &str5)
{

	if (ScTraceBuffer::isEventEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::event(ctxt, methodName,
				description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->addProperty(prefix4, str4);
		buffer->addProperty(prefix5, str5);
		buffer->invoke();
	}
}

//=== Trace Error ===
//1
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->invoke();
	}
}
//2
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->addProperty(prefix1, str1);
		buffer->invoke();
	}
}
//3
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}
//4
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->invoke();
	}
}
//5
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const int rc)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->addProperty<int>(prefix1, rc);
		buffer->invoke();
	}
}
//5
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const int rc)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty<int>(prefix2, rc);
		buffer->invoke();
	}
}
//7
void Trace_Error(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3,
		const std::string &prefix4, const std::string &str4)
{
	if (ScTraceBuffer::isErrorEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::error(ctxt, methodName, description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->addProperty(prefix4, str4);
		buffer->invoke();
	}
}

//=== Trace Warning ===
//1
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description)
{
	if (ScTraceBuffer::isWarningEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::warning(ctxt, methodName,	description);
		buffer->invoke();
	}
}
//2
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1)
{
	if (ScTraceBuffer::isWarningEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::warning(ctxt, methodName,	description);
		buffer->addProperty(prefix1, str1);
		buffer->invoke();
	}
}
//3
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2)
{
	if (ScTraceBuffer::isWarningEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::warning(ctxt, methodName,	description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}
//4
void Trace_Warning(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2,
		const std::string &prefix3, const std::string &str3)
{
	if (ScTraceBuffer::isWarningEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::warning(ctxt, methodName,	description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->addProperty(prefix3, str3);
		buffer->invoke();
	}
}

//=== Trace Info ===
//1
void Trace_Info(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description)
{
	if (ScTraceBuffer::isInfoEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::info(ctxt, methodName,	description);
		buffer->invoke();
	}
}
//2
void Trace_Info(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1)
{
	if (ScTraceBuffer::isInfoEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::info(ctxt, methodName,	description);
		buffer->addProperty(prefix1, str1);
		buffer->invoke();
	}
}

//=== Trace Config ===
//1
void Trace_Config(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description)
{
	if (ScTraceBuffer::isConfigEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::config(ctxt, methodName,	description);
		buffer->invoke();
	}
}
//2
void Trace_Config(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1)
{
	if (ScTraceBuffer::isConfigEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::config(ctxt, methodName,	description);
		buffer->addProperty(prefix1, str1);
		buffer->invoke();
	}
}

//3
void Trace_Config(const ScTraceContext *ctxt, const std::string &methodName,
		const std::string &description,
		const std::string &prefix1, const std::string &str1,
		const std::string &prefix2, const std::string &str2)
{
	if (ScTraceBuffer::isConfigEnabled(ctxt->getTraceComponent()))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::config(ctxt, methodName,	description);
		buffer->addProperty(prefix1, str1);
		buffer->addProperty(prefix2, str2);
		buffer->invoke();
	}
}

String toHexString(uint64_t l, bool uppercase)
{
	std::ostringstream oss;
	oss << std::hex << (uppercase ? std::uppercase : std::nouppercase) << l;
	return oss.str();
}

}
