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
 * AttributeTable.cpp
 *
 *  Created on: 09/06/2010
 */

#include "AttributeTable.h"

namespace spdr
{

ScTraceComponent* AttributeTable::tc_ = ScTr::enroll(
		trace::ScTrConstants::ScTr_Component_Name,
		trace::ScTrConstants::ScTr_SubComponent_Mem,
		trace::ScTrConstants::Layer_ID_Membership,
		"AttributeTable",
		trace::ScTrConstants::ScTr_ResourceBundle_Name);

const std::string AttributeTable::rebuttalKey_ = ".rebuttal";

AttributeTable::AttributeTable() :
	ScTraceContext(tc_,"",""),
	version_(0),
	version_sent_(0),
	version_pending_(0),
	pending_target_(),
	version_last_notify_(0),
	version_last_notify_internal_(0)
{

}

AttributeTable::~AttributeTable()
{

}

event::AttributeMap_SPtr AttributeTable::getAttributeMap4Notification()
{
	event::AttributeMap_SPtr att_map_sptr;

	if (!map_.empty())
	{
		att_map_sptr.reset(new event::AttributeMap);

		for (AttributeTableMap::iterator it = map_.begin(); it != map_.end(); ++it)
		{
			if (it->second.length > 0)
			{
				(*att_map_sptr)[it->first] = event::AttributeValue( it->second.length, it->second.bufferSPtr);
			}
			else
			{
				(*att_map_sptr)[it->first] = event::AttributeValue(	it->second.length, NULL);
			}

		}
	}

	return att_map_sptr;
}

void AttributeTable::pruneDeathCertificates(boost::posix_time::ptime olderThan)
{
	DeathCertificateMap::iterator it = deathCertificateMap_.begin();
	while (it != deathCertificateMap_.end())
	{
		if (it->second.first < olderThan)
		{
			//prevent the invalidation of the iterator
			DeathCertificateMap::iterator pos = it;
			++it;
			deathCertificateMap_.erase(pos);
		}
		else
		{
			++it;
		}
	}
}

int32_t AttributeTable::writeEntriesToMessage(uint64_t newerThan, SCMessage_SPtr outReply) const
{
	ByteBuffer& buffer = *(outReply->getBuffer());
	size_t pos_num_entries = buffer.getPosition();
	int32_t num_entries = 0;
	buffer.writeInt(num_entries);

	std::size_t num_keys = map_.size();
	std::size_t num_keys_written = 0;
	std::size_t val_size = 0;
	std::size_t val_size_written = 0;

	//write entries
	for (AttributeTableMap::const_iterator it = map_.begin();
			it != map_.end(); ++it)
	{
		val_size += (it->first.size() + (it->second.length>0 ? it->second.length : 0));

		if (it->second.version > newerThan)
		{
			buffer.writeString(it->first); //key
			buffer.writeSize_t(it->second.version); //version
			buffer.writeInt(it->second.length); //length
			if (it->second.length > 0) //value
			{
				buffer.writeByteArray(it->second.bufferSPtr.get(), it->second.length);
			}
			++num_entries;
			++num_keys_written;
			val_size_written += (it->first.size() + (it->second.length>0 ? it->second.length : 0));

			//Extended Trace
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "writeEntriesToMessage()");
				buffer->addProperty<int>("entry", num_entries);
				buffer->addProperty("key", it->first);
				buffer->addProperty<uint64_t>("ver", it->second.version);
				buffer->addProperty<int32_t>("len", it->second.length);
				buffer->invoke();
			}
		}
	}

	TRACE_DEBUG10(this, "writeEntriesToMessage()", "map",
			"#keys", spdr::stringValueOf(num_keys), "#keys-wr", spdr::stringValueOf(num_keys_written),
			"val-size", spdr::stringValueOf(val_size), "val-size-wr", spdr::stringValueOf(val_size_written));

	num_keys = deathCertificateMap_.size();
	num_keys_written = 0;
	val_size = 0;
	val_size_written = 0;

	//write death certificates
	for (DeathCertificateMap::const_iterator it = deathCertificateMap_.begin();
			it != deathCertificateMap_.end(); ++it)
	{
		val_size += (it->first.size() + 16);

		if (it->second.second > newerThan) //version
		{
			buffer.writeString(it->first); //key
			buffer.writeSize_t(it->second.second); //version
			buffer.writeInt(-1); //length, missing
			++num_entries;
			++num_keys_written;

			val_size_written += (it->first.size() + 16);
			//Extended Trace
			if (ScTraceBuffer::isDumpEnabled(tc_))
			{
				ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "writeEntriesToMessage()");
				buffer->addProperty<int>("entry", num_entries);
				buffer->addProperty("key", it->first);
				buffer->addProperty<uint64_t>("ver", it->second.second);
				buffer->addProperty<int32_t>("len", -1);
				buffer->invoke();
			}
		}
	}

	TRACE_DEBUG10(this, "writeEntriesToMessage()", "deathCertificateMap",
				"#keys", spdr::stringValueOf(num_keys), "#keys-wr", spdr::stringValueOf(num_keys_written),
				"val-size", spdr::stringValueOf(val_size), "val-size-wr", spdr::stringValueOf(val_size_written));

	size_t last_pos = buffer.getPosition();
	buffer.setPosition(pos_num_entries);
	buffer.writeInt(num_entries);
	buffer.setPosition(last_pos);
	//buffer.writeInt2Pos(pos_num_entries, num_entries);

	return num_entries;
}

int32_t AttributeTable::mergeEntriesFromMessage(SCMessage_SPtr inReply)
{

	//TODO do not set notify if the change includes only the rebuttal key

	ByteBuffer& buffer = *(inReply->getBuffer());
	int32_t num_entries = buffer.readInt();

	//Extended Trace
	if (ScTraceBuffer::isEntryEnabled(tc_))
	{
		ScTraceBufferAPtr buffer = ScTraceBuffer::entry(this, "mergeEntriesFromMessage()");
		buffer->addProperty<int>("#entries", num_entries);
		buffer->invoke();
	}

	uint64_t max_version = 0;

	for (int32_t i=0; i<num_entries; ++i)
	{
		String key = buffer.readString();
		AttributeTableValue value;
		value.version = buffer.readSize_t();
		value.length = buffer.readInt();

		//Extended Trace
		if (ScTraceBuffer::isDumpEnabled(tc_))
		{
			ScTraceBufferAPtr buffer = ScTraceBuffer::dump(this, "mergeEntriesFromMessage()");
			buffer->addProperty<int>("entry", i);
			buffer->addProperty("key", key);
			buffer->addProperty<uint64_t>("ver", value.version);
			buffer->addProperty<int32_t>("len", value.length);
			buffer->addProperty<int32_t>("T-ver", version_);
			buffer->invoke();
		}

		if (value.length > 0)
		{
			value.bufferSPtr.reset(
					allocateAndCopy(value.length, buffer));
			//TODO just skip in case (value.version <= version_)
		}

		if (value.version <= version_)
		{
			continue;
		}

		if (max_version < value.version)
		{
			max_version = value.version;
		}

		//Note: the two maps are mutually exclusive
		AttributeTableMap::iterator map_entry = map_.find(key);
		DeathCertificateMap::iterator dcm_entry = deathCertificateMap_.find(key);

		if (value.length >= 0) //an entry
		{
			if (map_entry != map_.end())
			{
				map_entry->second = value;
			}
			else  if (dcm_entry != deathCertificateMap_.end())
			{
				deathCertificateMap_.erase(dcm_entry);
				map_[key] = value;
			}
			else //in neither
			{
				map_[key] = value;
			}
		}
		else //a death certificate
		{
			if (map_entry != map_.end())
			{
				map_.erase(map_entry);
				deathCertificateMap_[key] = DeathCertificate(
						boost::posix_time::microsec_clock::universal_time(), value.version);
			}
			else  if (dcm_entry != deathCertificateMap_.end())
			{
				dcm_entry->second.first = boost::posix_time::microsec_clock::universal_time();
				dcm_entry->second.second = value.version;
			}
			else //in neither
			{
				deathCertificateMap_[key] = DeathCertificate(
						boost::posix_time::microsec_clock::universal_time(), value.version);
			}
		}
	} //for

	if (max_version > version_)
	{
		version_ = max_version;
	}

	return num_entries;
}

int32_t AttributeTable::skipEntriesFromMessage(SCMessage_SPtr inReply)
{
	ByteBuffer& buffer = *(inReply->getBuffer());
	int32_t num_entries = buffer.readInt();

	for (int32_t i=0; i<num_entries; ++i)
	{
		buffer.readString(); //key
		buffer.readSize_t(); //version
		int32_t length = buffer.readInt(); //value length

		if (length > 0)
		{
			buffer.setPosition(buffer.getPosition()+length); //skip value
		}
	}

	return num_entries;
}

//===
bool AttributeTable::set(const String& key,
		std::pair<const int32_t, const char*> value)
{
	//add to the map, remove from deathCertificateMap

	if (key.length() == 0)
		throw SpiderCastRuntimeError("Empty keys are not allowed");

	AttributeTableValue tableVal;
	tableVal.version = ++version_;
	tableVal.length = value.first;
	if (value.first > 0)
	{
		tableVal.bufferSPtr.reset(clone(value.first, value.second));
	}

	std::pair<AttributeTableMap::iterator, bool> result = map_.insert(
			AttributeTableMap::value_type(key, tableVal));

	if (!result.second)
	{
		result.first->second = tableVal;
	}

	deathCertificateMap_.erase(key);

	return result.second;
}

/**
 * Get the value mapped to a key.
 *
 * The buffer returned is a copy of the value, the pointer is the responsibility of the caller.
 *
 * @param key
 * @return A copy of the value, or length is (-1) if not found.
 */
std::pair<event::AttributeValue,bool> AttributeTable::get(const String& key)
{
	AttributeTableMap::const_iterator it = map_.find(key);
	if (it != map_.end())
	{
		return std::make_pair(event::AttributeValue(it->second.length, it->second.bufferSPtr), true);
		//char* buff = clone(it->second.length, it->second.bufferSPtr.get());
		//return std::make_pair(it->second.length, buff);
	}
	else
	{
		return std::make_pair(event::AttributeValue(), false);
		//return MembershipService::MissingAttributeValue;
	}
}

/**
 * Remove a key-value pair, put a death-certificate.
 *
 * @param key
 * @return true if removed, map size changed
 */
bool AttributeTable::remove(const String& key)
{
	//the map and death certificate map are mutually exclusive.
	int count = map_.erase(key);
	if (count > 0)
	{
		deathCertificateMap_[key] = DeathCertificate(
				boost::posix_time::microsec_clock::universal_time(), ++version_);
	}

	return (count > 0);
}


void AttributeTable::writeRebuttalKey()
{
	DeathCertificateMap::iterator pos = deathCertificateMap_.find(rebuttalKey_);
	if (pos != deathCertificateMap_.end())
	{
		pos->second = DeathCertificate(
				boost::posix_time::microsec_clock::universal_time(), ++version_);
	}
	else
	{
		deathCertificateMap_[rebuttalKey_] = DeathCertificate(
				boost::posix_time::microsec_clock::universal_time(), ++version_);
	}
}

/**
 * Contains a key?
 *
 * @param key
 * @return true if the map contains the key
 */
bool AttributeTable::contains(const String& key) const
{
	return (map_.count(key) > 0);
}

/**
 * Clear all key-value attributes.
 */
void AttributeTable::clear()
{
	StringSet keySet = getKeySet();

	for (StringSet::iterator it = keySet.begin();
			it != keySet.end(); ++it)
	{
		remove(*it);
	}
}

void AttributeTable::clearPrefix(const char prefix)
{
	StringSet keySet = getKeySet();

	for (StringSet::iterator it = keySet.begin();
			it != keySet.end(); ++it)
	{
		if (it->size()>0 && (*it)[0] == prefix)
		{
			remove(*it);
		}
	}
}

void AttributeTable::clearNoPrefix(const char prefix)
{
	StringSet keySet = getKeySet();

	for (StringSet::iterator it = keySet.begin();
			it != keySet.end(); ++it)
	{
		if (it->size()==0 || (it->size() > 0 && (*it)[0] != prefix))
		{
			remove(*it);
		}
	}
}

/**
 * Is the attribute map empty?
 * @return true if attribute map is empty
 */
bool AttributeTable::isEmpty() const
{
	return (map_.empty());
}

/**
 * Size of attribute map.
 * @return size of attribute map
 */
size_t AttributeTable::size() const
{
	return (map_.size());
}

/**
 * Get the attribute key-set.
 * @return
 */
StringSet AttributeTable::getKeySet() const
{
	StringSet keySet;
	for (AttributeTableMap::const_iterator it = map_.begin(); it != map_.end(); ++it)
	{
		keySet.insert(it->first);
	}
	return keySet;
}

void AttributeTable::writeMapEntriesToMessage(SCMessage_SPtr outReply) const
{
	ByteBuffer& buffer = *(outReply->getBuffer());
	int32_t num_entries = map_.size();
	buffer.writeInt(num_entries);

	//write entries
	for (AttributeTableMap::const_iterator it = map_.begin();
			it != map_.end(); ++it)
	{
			buffer.writeString(it->first); //key
			buffer.writeInt(it->second.length); //length
			if (it->second.length > 0) //value
			{
				buffer.writeByteArray(it->second.bufferSPtr.get(), it->second.length);
			}
	}
}

void AttributeTable::getSizeSummary(std::size_t *array7) const
{
	memset(array7,0,sizeof(std::size_t[7]));

	*array7 = map_.size();
	std::size_t keys_sz = 0;
	std::size_t val_sz = 0;
	std::size_t val_max = 0;
	for (AttributeTableMap::const_iterator it = map_.begin();
			it != map_.end(); ++it)
	{
		keys_sz += it->first.size();
		std::size_t val_len = (it->second.length > 0 ? it->second.length : 0);
		val_sz += val_len;
		val_max = std::max(val_max,val_len);
	}
	*(array7+1) = keys_sz;
	*(array7+2) = val_sz;
	*(array7+3) = val_max;
	*(array7+4) = deathCertificateMap_.size();
	keys_sz = 0;
	val_sz = 0;
	for (DeathCertificateMap::const_iterator it = deathCertificateMap_.begin();
			it != deathCertificateMap_.end(); ++it)
	{
		keys_sz += it->first.size();
		val_sz += sizeof(DeathCertificate);
	}
	*(array7+5) = keys_sz;
	*(array7+6) = val_sz;

}

String AttributeTable::toStringSummary() const
{
	std::size_t array[7];
	getSizeSummary(array);

	std::ostringstream oss;
	oss << "Map={#keys=" << array[0] << " keys-sz=" << array[1] << " val-sz=" << array[2] << " val-max=" << array[3] << "} ";
	oss << "DCM={#keys=" << array[4] << " keys-sz=" << array[5] << " val-sz=" << array[6] << "}";

	return oss.str();
}

//String AttributeTable::toStringSummary() const
//{
//	std::size_t keys_sz = 0;
//	std::size_t val_sz = 0;
//	std::size_t val_max = 0;
//	for (AttributeTableMap::const_iterator it = map_.begin();
//			it != map_.end(); ++it)
//	{
//		keys_sz += it->first.size();
//		std::size_t val_len = (it->second.length > 0 ? it->second.length : 0);
//		val_sz += val_len;
//		val_max = std::max(val_max,val_len);
//	}
//	std::ostringstream oss;
//	oss << "Map={#keys=" << map_.size() << " keys-sz=" << keys_sz << " val-sz=" << val_sz << " val-max=" << val_max << "} ";
//
//	keys_sz = 0;
//	val_sz = 0;
//	for (DeathCertificateMap::const_iterator it = deathCertificateMap_.begin();
//			it != deathCertificateMap_.end(); ++it)
//	{
//		keys_sz += it->first.size();
//		val_sz += sizeof(DeathCertificate);
//	}
//	oss << "DCM={#keys=" << deathCertificateMap_.size() << " keys-sz=" << keys_sz << " val-sz=" << val_sz << "}";
//
//	return oss.str();
//}

//=== private methods ===

char* AttributeTable::clone(const int size, const char* buffer)
{
	if (size <= 0)
	{
		return (char*) 0;
	}
	else if (buffer == NULL)
	{
		throw NullPointerException(
				"NullPointerException: AttributeTable trying to clone a null buffer with positive size");
	}

	char* copy = new char[size];
	if (copy == NULL)
	{
		std::ostringstream oss;
		oss << "OutOfMemoryException: AttributeTable trying to clone() "
				<< size << " bytes; source=" << buffer;
		throw OutOfMemoryException(oss.str());
	}
	memcpy(copy, buffer, size);
	return copy;
}

char* AttributeTable::allocateAndCopy(int length, ByteBuffer& buffer)
{
	char* copy = new char[length];
	if (copy == NULL)
	{
		std::ostringstream oss;
		oss << "OutOfMemoryException: AttributeTable trying to allocate " << length << " bytes";
		throw OutOfMemoryException(oss.str());
	}
	buffer.readByteArray(copy, length);
	return copy;
}

}
