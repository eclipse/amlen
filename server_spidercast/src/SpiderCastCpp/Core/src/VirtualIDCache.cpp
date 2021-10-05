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
 * VirtualIDCache.cpp
 *
 *  Created on: Apr 17, 2011
 */

#include "VirtualIDCache.h"

namespace spdr
{

boost::mutex VirtualIDCache::create_mutex;

VirtualIDCache::VirtualIDCache(size_type size) :
		sha1_(),
		target_size_(size),
		key2VIDmap_(),
		n_access_(0),
		n_hit_(0)
{
	if (target_size_ == 0)
	{
		throw IllegalArgumentException("target_size = 0");
	}
}

VirtualIDCache::~VirtualIDCache()
{
}

VirtualIDCache::size_type VirtualIDCache::size() const
{
	boost::mutex::scoped_lock lock(get_mutex_);
	return key2VIDmap_.size();
}

double VirtualIDCache::getHitRate() const
{
	boost::mutex::scoped_lock lock(get_mutex_);
	if (n_access_==0)
	{
		return 0;
	}
	else
	{
		return (double)n_hit_ / (double)n_access_;
	}
}


util::VirtualID_SPtr VirtualIDCache::get(const String& key)
{
	boost::mutex::scoped_lock lock(get_mutex_);

	++n_access_;

	Key2VIDmap::iterator pos = key2VIDmap_.find(key);
	if (pos != key2VIDmap_.end())
	{
		++n_hit_;
		return pos->second;
	}
	else
	{
		sha1_.reset();
		sha1_.update(key);
		util::VirtualID_SPtr val = util::VirtualID_SPtr(new util::VirtualID(sha1_));

		std::pair<Key2VIDmap::iterator,bool> res = key2VIDmap_.insert(std::make_pair(key,val));

		if (res.second)
		{
			if (key2VIDmap_.size() > target_size_)
			{
				if (++(res.first) != key2VIDmap_.end())
				{
					key2VIDmap_.erase(res.first);
				}
				else
				{
					key2VIDmap_.erase(key2VIDmap_.begin());
				}
			}

			return val;
		}
		else
		{
			throw SpiderCastRuntimeError("Failed to insert <String,util::VirtualID_SPtr into Key2VIDmap");
		}
	}
}
//
//util::VirtualID VirtualIDCache::get2(const String& key)
//{
//	boost::mutex::scoped_lock lock(mutex_);
//
//	Key2VIDmap2::iterator pos = key2VIDmap2_.find(key);
//	if (pos != key2VIDmap2_.end())
//	{
//		return pos->second;
//	}
//	else
//	{
//		sha1_.reset();
//		sha1_.update(key);
//		util::VirtualID val(sha1_);
//		std::pair<Key2VIDmap2::iterator,bool> res = key2VIDmap2_.insert(std::make_pair(key,val));
//
//		if (res.second)
//		{
//			return val;
//		}
//		else
//		{
//			throw SpiderCastRuntimeError("Failed to insert <String,util::VirtualID_SPtr into Key2VIDmap");
//		}
//	}
//}

util::VirtualID_SPtr VirtualIDCache::create(const String& key)
{
	{
		boost::mutex::scoped_lock lock(create_mutex);
		static util::SHA1 sha1;
		sha1.reset();
		sha1.update(key);
		return util::VirtualID_SPtr(new util::VirtualID(sha1));
	}
}

}
