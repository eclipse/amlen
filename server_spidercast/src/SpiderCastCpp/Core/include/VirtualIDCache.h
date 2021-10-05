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
 * VirtualIDCache.h
 *
 *  Created on: Apr 17, 2011
 */

#ifndef VIRTUALIDCACHE_H_
#define VIRTUALIDCACHE_H_

#include <boost/utility.hpp>
#include <boost/unordered_map.hpp>
#include <boost/thread/mutex.hpp>

#include "SpiderCastRuntimeError.h"
#include "VirtualID.h"

namespace spdr
{

/**
 * The VirtualIDCache keeps the translation of strings to VirtualID,
 * in order to save the computation of SHA1.
 *
 * It is also used as a factory for VirtualID objects,
 * for strings that are not cached.
 *
 * The implementation is thread safe.
 *
 */
class VirtualIDCache : boost::noncopyable
{
public:
	typedef uint32_t size_type;

	/**
	 * @param target_size number of keys to keep.
	 * target_size must be >0.
	 * @return
	 */
	VirtualIDCache(size_type target_size = 4096);

	virtual ~VirtualIDCache();

	/**
	 * Get the VirtualID, cache it.
	 *
	 * If not in the cache, creates it and places it in the cache.
	 *
	 * Thread safe.
	 *
	 * @param key
	 * @return a shared pointer to a VirtualID.
	 */
	util::VirtualID_SPtr get(const String& key);

	/**
	 * Get the cache size.
	 *
	 * @return
	 */
	size_type size() const;

	/**
	 * Get the cache hit rate.
	 *
	 * @return
	 */
	double getHitRate() const;

	/**
	 * Create the VirtualID.
	 *
	 * Does NOT cache it.
	 *
	 * Thread safe.
	 *
	 * @param key
	 * @return a shared pointer to a VirtualID.
	 */
	static util::VirtualID_SPtr create(const String& key);

	//util::VirtualID get2(const String& key);

private:
	static boost::mutex create_mutex;

	mutable boost::mutex get_mutex_;

	util::SHA1 sha1_;

	size_type target_size_;

	typedef boost::unordered_map<String, util::VirtualID_SPtr > Key2VIDmap;
	Key2VIDmap key2VIDmap_;

	uint64_t n_access_;
	uint64_t n_hit_;

};

}

#endif /* VIRTUALIDCACHE_H_ */
