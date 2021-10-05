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
 * AttributeControl.h
 *
 *  Created on: Oct 4, 2010
 */

#ifndef ATTRIBUTECONTROL_H_
#define ATTRIBUTECONTROL_H_


#include "Definitions.h"
#include "AttributeValue.h"

namespace spdr
{

/**
 * The interface for the control of the node's own attribute table.
 */
class AttributeControl
{
public:
	AttributeControl();
	virtual ~AttributeControl();


	/**
	 * Set (and overwrite) a key-value pair.
	 * The buffer is copied, so the pointer remains under responsibility of the caller.
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @param key
	 * @param value
	 * @return true if the key is new, map size changed
	 */
	virtual bool setAttribute(const String& key, std::pair<const int32_t,const char*> value) = 0;

	/**
	 * Get the value mapped to a key.
	 *
	 * The buffer returned is a pointer to the internal value.
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @param key
	 * @return The value (or NULL), and an indication if it was found in the map.
	 */
	virtual std::pair<event::AttributeValue,bool> getAttribute(const String& key) = 0;

	/**
	 * Remove a key-value pair.
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @param key
	 * @return true if removed, map size changed
	 */
	virtual bool removeAttribute(const String& key) = 0;

	/**
	 * Contains a key?
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @param key
	 * @return true if the map contains the key
	 */
	virtual bool containsAttributeKey(const String& key) = 0;

	/**
	 * Clear all key-value attributes.
	 *
	 * MemTopoThread, App.-thread
	 *
	 */
	virtual void clearAttributeMap() = 0;

	/**
	 * Clear all external key-value attributes.
	 *
	 * MemTopoThread, App.-thread
	 *
	 */
	virtual void clearExternalAttributeMap() = 0;

	/**
	 * Clear all internal key-value attributes.
	 *
	 * MemTopoThread, App.-thread,
	 *
	 */
	virtual void clearInternalAttributeMap() = 0;

	/**
	 * Is the attribute map empty?
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @return true if attribute map is empty
	 */
	virtual bool isEmptyAttributeMap() = 0;

	/**
	 * Size of attribute map.
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @return size of attribute map
	 */
	virtual size_t sizeOfAttributeMap() = 0;

	/**
	 * Get the attribute key-set.
	 *
	 * MemTopoThread, App.-thread
	 *
	 * @return
	 */
	virtual StringSet getAttributeKeySet() = 0;
};

}

#endif /* ATTRIBUTECONTROL_H_ */
