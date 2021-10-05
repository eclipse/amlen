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
 * AttributeTable.h
 *
 *  Created on: 09/06/2010
 */

#ifndef ATTRIBUTETABLE_H_
#define ATTRIBUTETABLE_H_

#include <boost/shared_array.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/unordered_map.hpp>

#include "MembershipService.h"
#include "NodeIDImpl.h"
#include "SCMessage.h"
#include "MetaData.h"

#include "ScTr.h"
#include "ScTraceBuffer.h"

#ifdef RUM_UNIX
#include <cstring>
#endif

namespace spdr
{

class AttributeTableValue
{
public:
	uint64_t version;
	int32_t length;
	boost::shared_array<const char> bufferSPtr;

	AttributeTableValue() :
		version(0), length(0), bufferSPtr()
	{
	}

	virtual ~AttributeTableValue()
	{
	}
};



class AttributeTable : public ScTraceContext
{

private:
	static ScTraceComponent* tc_;

	const static String rebuttalKey_;

	uint64_t version_;
	uint64_t version_sent_;

	uint64_t version_pending_;
	NodeIDImpl_SPtr pending_target_;
	boost::posix_time::ptime pending_time_;

	uint64_t version_last_notify_; //external
	uint64_t version_last_notify_internal_;

	//typedef std::map<String, AttributeTableValue> AttributeTableMap;
	typedef boost::unordered_map<String, AttributeTableValue> AttributeTableMap;

	//Holds valid attributes.
	//The attribute map and death certificate map are kept mutually exclusive.
	AttributeTableMap map_;

	typedef std::pair<boost::posix_time::ptime, uint64_t>	DeathCertificate;
	//typedef std::map<String,DeathCertificate> 				DeathCertificateMap;
	typedef boost::unordered_map<String,DeathCertificate> 	DeathCertificateMap;

	//Holds death certificates.
	DeathCertificateMap deathCertificateMap_;

	//=== methods ===
	char* clone(const int size, const char* buffer);
	char* allocateAndCopy(int length, ByteBuffer& buffer);

public:
	AttributeTable();
	virtual ~AttributeTable();

	bool isUpdateNeeded() const
	{
		return version_ > version_sent_;
	}

	bool isNotifyNeeded() const
	{
		return version_ > version_last_notify_;
	}

	bool isNotifyInternalNeeded() const
	{
		return version_ > version_last_notify_internal_;
	}

	/**
	 * Get a copy of the table in the format ready for notification.
	 * @return
	 */
	event::AttributeMap_SPtr getAttributeMap4Notification();

	void markLastNotify()
	{
		version_last_notify_ = version_;
	}

	void markLastNotifyInternal()
	{
		version_last_notify_internal_ = version_;
	}

	uint64_t getVersion() const
	{
		return version_;
	}

	void markVersionSent()
	{
		version_sent_ = version_;
	}

	void markVersionSent(uint64_t version_sent)
	{
		version_sent_ = version_sent;
	}

	void resetVersionSent()
	{
		version_sent_ = 0;
	}


	//TODO clear pending when the reply is received
	//TODO check when neighbor leaves
	void markPending(uint64_t version, NodeIDImpl_SPtr target)
	{
		if (version <= version_)
		{
			throw IllegalArgumentException(
					"IllegalArgumentException: markPending version must be bigger then internal version");
		}

		version_pending_ = version;
		pending_target_ = target;
		pending_time_ = boost::posix_time::microsec_clock::universal_time();
	}

	bool isPending() const
	{
			return (version_pending_ > version_);
	}

	void clearPending()
	{
		version_pending_ = version_;
		pending_target_.reset();
	}

	bool isPending(NodeIDImpl_SPtr target) const
	{
		if (isPending() && pending_target_ && (*target == *pending_target_))
			return true;
		else
			return false;
	}

	NodeIDImpl_SPtr getPendingTarget()
	{
		return pending_target_;
	}

	uint64_t getVersionPending()
	{
		return version_pending_;
	}

	/**
	 * Write entries to the (Reply) message.
	 * Writes the entries with version larger than "newerThan".
	 *
	 * Format:
	 * #entries 		(int32_t)
	 * EntryFormat  	(x #entries)
	 * - key			(String)
	 * - version		(uint64_t)
	 * - length			(int32_t)
	 * - value			(byte array of size 'length', if length>0)
	 *
	 * @param newerThan start version (not including)
	 * @param msg
	 * @return the number of entries written
	 */
	int32_t writeEntriesToMessage(uint64_t newerThan, SCMessage_SPtr outReply) const;

	/**
	 * Read entries to the (Reply) message.
	 * Read the entries and merges them into the table.
	 *
	 * Expected format: see writeEntriesToMessage().
	 *
	 * @param inReply
	 * @return the number of entries read
	 */
	int32_t mergeEntriesFromMessage(SCMessage_SPtr inReply);

	/**
	 * Skip the entries of a single node from the message.
	 * Used when the target node is not in view and we need to skip its entries.
	 *
	 * Expected format: see writeEntriesToMessage().
	 *
	 * @param inReply
	 * @return the number of entries skipped
	 */
	static int32_t skipEntriesFromMessage(SCMessage_SPtr inReply);

	void pruneDeathCertificates(boost::posix_time::ptime olderThan);

	/**
	 * Write the rebuttal key.
	 *
	 * Puts a new version on the the rebuttal key in the death-certificate map, forcing
	 * a new version to be sent. Used for rebutting suspicions.
	 *
	 */
	void writeRebuttalKey();

	//=== writing original values ===

	/**
	 * Set (and overwrite) a key-value pair.
	 * The buffer is copied, so the pointer remains under responsibility of the caller.
	 *
	 * @param key Key must be a non empty string, without leading and trailing whitespace.
	 * @param value
	 * @return true if the key is new, map size changed
	 */
	bool set(const String& key, std::pair<const int32_t,const char*> value);

	/**
	 * Get the value mapped to a key.
	 *
	 * The buffer returned is a copy of the value, the pointer is the responsibility of the caller.
	 *
	 * @param key
	 * @return A copy of the value, or length is (-1) if not found.
	 */
	std::pair<event::AttributeValue,bool> get(const String& key);
	//std::pair<int32_t,char*> get(const String& key);


	/**
	 * Remove a key-value pair.
	 *
	 * @param key
	 * @return true if removed, map size changed
	 */
	bool remove(const String& key);

	/**
	 * Contains a key?
	 *
	 * @param key
	 * @return true if the map contains the key
	 */
	bool contains(const String& key) const;

	/**
	 * Clear all key-value attributes.
	 */
	void clear();

	/**
	 * Clear all key-value attributes with key starting with prefix.
	 * @param prefix
	 */
	void clearPrefix(const char prefix);

	/**
	 * Clear all key-value attributes with key NOT starting with prefix.
	 * @param prefix
	 */
	void clearNoPrefix(const char prefix);

	/**
	 * Is the attribute map empty?
	 * @return true if attribute map is empty
	 */
	bool isEmpty() const;

	/**
	 * Size of attribute map.
	 * @return size of attribute map
	 */
	size_t size() const;

	/**
	 * Get the attribute key-set.
	 * @return
	 */
	StringSet getKeySet() const;

	/**
	 * Write map to the (ViewUpdate for supervisor) message.
	 * Writes all the entries without version numbers.
	 *
	 * Format:
	 * #entries 		(int32_t)
	 * EntryFormat  	(x #entries)
	 * - key			(String)
	 * - length			(int32_t)
	 * - value			(byte array of size 'length', if length>0)
	 *
	 * @param msg
	 */
	void writeMapEntriesToMessage(SCMessage_SPtr outReply) const;

	/**
	 * An array of size 7:
	 * {#keys, key-sz, val-sz, val-max, dcm-#keys, dcm-key-sz, dcm-val-sz}
	 */
	void getSizeSummary(std::size_t *array7) const;

	String toStringSummary() const;
};

typedef boost::shared_ptr<AttributeTable> AttributeTable_SPtr;

}

#endif /* ATTRIBUTETABLE_H_ */
