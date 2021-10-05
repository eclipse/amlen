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
 * EnumCounter.h
 *
 *  Created on: Dec 2, 2010
 */

#ifndef ENUMCOUNTER_H_
#define ENUMCOUNTER_H_

#include <vector>
#include <string>
#include <sstream>
#include <boost/cstdint.hpp>

#include "SpiderCastLogicError.h"

namespace spdr
{

/**
 * An array of numeric counters, each for one enum value,
 * assuming the enum is in the range [0,max], and that '0' and 'max' are just range guards.
 * That is, legal values are [1,max-1].
 *
 * Default assignment and copy-constructor.
 *
 * Ntype can be any integer type (int, short, etc), double, or float.
 */
template<class Etype, class Ntype = int32_t>
class EnumCounter
{
public:

	EnumCounter() :
			max_(1)
	{
	}

	/**
	 * @param max The maximal value of the enum
	 * @param labels an array of enum labels, valid for labels[0] till labels[max]
	 * @return
	 */
	EnumCounter(Etype max, const char* const* labels = NULL) :
		max_((int) max)
	{
		for (int i = 0; i < max_; ++i)
		{
			counters_.push_back(0);
		}

		if (labels == NULL)
		{
			for (int i = 0; i < max_; ++i)
			{
				std::ostringstream oss;
				oss << typeid(Etype).name() << "_" << i;
				labels_.push_back( oss.str() );
			}
		}
		else
		{
			for (int i = 0; i < max_; ++i)
			{
				labels_.push_back(std::string(labels[i]));
			}
		}

	}

	~EnumCounter()
	{
	}

	void increment(Etype t, Ntype value)
	{
		testRange(t);
		counters_[(int) t] += value;
	}

	void increment(Etype t)
	{
		testRange(t);
		counters_[(int) t] += 1;
	}

	void set(Etype t, Ntype value)
	{
		testRange(t);
		counters_[(int) t] = value;
	}

	void set2max(Etype t, Ntype value)
	{
		testRange(t);
		if (value > counters_[(int) t])
			counters_[(int) t] = value;
	}

	Ntype get(Etype t)
	{
		testRange(t);
		return counters_[(int) t];
	}

	void reset()
	{
		for (int i = 1; i < max_; ++i)
			counters_[i] = 0;
	}

	void add(const EnumCounter<Etype, Ntype>& other)
	{
		for (int i = 1; i < max_; ++i)
		{
			counters_[i] += other.counters_[i];
		}
	}

	void sub(const EnumCounter<Etype, Ntype>& other)
	{
		for (int i = 1; i < max_; ++i)
		{
			counters_[i] -= other.counters_[i];
		}
	}

	std::string toCounterString()
	{
		std::ostringstream oss;
		for (int i = 1; i < (int) counters_.size(); ++i)
		{
			oss << (counters_[i]);
			if (i < max_-1)
			{
				oss << ", ";
			}
		}

		return oss.str();
	}

	std::string toLabelString()
	{
		std::ostringstream oss;
		for (int i = 1; i < (int) labels_.size(); ++i)
		{
			oss << (labels_[i]);
			if (i < max_-1)
			{
				oss << ", ";
			}
		}

		return oss.str();
	}

	std::string toString() const
	{
		std::ostringstream oss;
		for (int i = 1; i < (int) counters_.size(); ++i)
		{
			oss << (labels_[i]) << "=" << (counters_[i]);
			if (i < max_-1)
			{
				oss << ", ";
			}
		}

		return oss.str();
	}

	/**
	 * Size of legal values, |[1,max-1]| == (max-1)
	 * @return
	 */
	int getSize()
	{
		return (max_ - 1);
	}

private:
	int max_;
	std::vector<Ntype> counters_;
	std::vector<std::string> labels_;

	void testRange(Etype t) throw (IndexOutOfBoundsException)
	{
		if ((int) t <= 0 || (int) t >= max_)
		{
			std::ostringstream oss;
			oss << "EnumCounter::increment(Etype t), t must be in (0," << max_
					<< "), t=" << t;
			throw IndexOutOfBoundsException(oss.str());
		}
	}
};

}

#endif /* ENUMCOUNTER_H_ */
