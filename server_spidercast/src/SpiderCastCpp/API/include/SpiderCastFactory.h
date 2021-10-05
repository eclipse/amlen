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
 * SpiderCastFactory.h
 *
 *  Created on: 28/01/2010
 */

#ifndef SPDR_SPIDERCASTFACTORY_H_
#define SPDR_SPIDERCASTFACTORY_H_


#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>

#include "SpiderCast.h"
#include "SpiderCastConfig.h"
#include "SpiderCastEventListener.h"
#include "SpiderCastLogicError.h"
#include "PropertyMap.h"
#include "NodeID.h"
#include "LogListener.h"

namespace boost
{
	struct once_flag;
}

#include "SpiderCastDocumentation.h"

/**
 * The top level SpiderCast name-space.
 *
 * All SpiderCast classes are under this namespace.
 */
namespace spdr
{

/**
 * A factory for SpiderCast instances, configuration objects, and other utilities.
 *
 * A singleton, thread safe.
 *
 */
class SpiderCastFactory //: public ScTraceContext
{

private:
	//disallow copy construction
	SpiderCastFactory(const SpiderCastFactory&);
	//disallow assignment
	SpiderCastFactory& operator=(SpiderCastFactory&);

	static boost::scoped_ptr<SpiderCastFactory> instanceScopedPtr;
	static boost::once_flag flag;

	static void init();

protected:
	SpiderCastFactory();

public:

	/**
	 * Destructor.
	 *
	 * @return
	 */
	virtual ~SpiderCastFactory();

	/**
	 * Get the singleton factory instance.
	 *
	 * Thread safe.
	 *
	 * @return the factory instance
	 */
	static SpiderCastFactory& getInstance();

	/**
	 * Create a SpiderCast instance.
	 *
	 * A limited number of SpiderCast instances can be created in the same process.
	 * The current limit is 10 (hard coded).
	 *
	 * @param config SpiderCast configuration object
	 * @param eventListener an event listener for handling life-cycle events
	 *
	 * @return A new instance of SpiderCast at the SpiderCast#NodeState#Init state.
	 *
	 * @throw SpiderCastRuntimeError if creation fails
	 *
	 */
	virtual SpiderCast_SPtr createSpiderCast(
			SpiderCastConfig& config,
			SpiderCastEventListener& eventListener)
			throw (SpiderCastRuntimeError) = 0;

	/**
	 * Create a SpiderCast configuration object.
	 *
	 * The method checks the properties for validity, consistency, and existence of mandatory properties.
	 * An IllegalConfigException is thrown if errors are found.
	 *
	 * The bootstrap set is a vector of NodeID objects, representing bootstrap nodes on the same bus.
	 * The bootstrap set can be empty. In that case the node will wait to be discovered,
	 * meaning it must be included in another node's bootstrap set.
	 *
	 * The bootstrap may contain "nameless" nodes, for which the name is the label "$ANY" and
	 * only the end-points are given. These nodes will only be used for discovery and will retrieve
	 * the target server name during the discovery process. Note that a valid node cannot be named "$ANY".
	 *
	 * Same as calling
	 * SpiderCastFactory#createSpiderCastConfig(
	 *		const PropertyMap& properties,
	 *		const std::vector<NodeID_SPtr >& bootstrapSet,
	 *		const std::vector<NodeID_SPtr >& supervisorBootstrapSet)
	 * with an empty supervisorBootstrapSet.
	 *
	 * @param properties a map of key-value configuration properties
	 * @param bootstrapSet a vector of NodeID objects, representing bootstrap nodes on the same bus
	 *
	 * @return A new SpiderCastConfig object
	 *
	 * @throw IllegalConfigException if configuration parameters are wrong, or missing
	 */
	virtual SpiderCastConfig_SPtr createSpiderCastConfig(
			const PropertyMap& properties,
			const std::vector<NodeID_SPtr >& bootstrapSet)
			throw (SpiderCastLogicError) = 0;

	/**
	 * Create a SpiderCast configuration object, with supervisor bootstrap.
	 *
	 * The method checks the properties for validity, consistency, and existence of mandatory properties.
	 * An IllegalConfigException is thrown if errors are found.
	 *
	 * The bootstrap set is a vector of NodeID objects, representing bootstrap nodes on the same bus.
	 * The bootstrap set can be empty. In that case the node will wait to be discovered,
	 * meaning it must be included in another node's bootstrap set.
	 *
	 * The supervisor bootstrap set is a vector of NodeID objects, representing bootstrap nodes on
	 * a bus one above the node's bus. It is used by a node in a base zone (L2) to locate supervisors
	 * in the management zone (L1). It can be empty.
	 *
	 * The bootstrap may contain "nameless" nodes, for which the name is the label "$ANY" and
	 * only the end-points are given. These nodes will only be used for discovery and will retrieve
	 * the target server name during the discovery process. Note that a valid node cannot be named "$ANY".
	 *
	 * @param properties a map of key-value configuration properties
	 * @param bootstrapSet a vector of NodeID objects, representing bootstrap nodes on the same bus
	 * @param supervisorBootstrapSet a vector of NodeID objects, representing bootstrap nodes on the supervisor bus
	 *
	 * @return A new SpiderCastConfig object
	 *
	 * @throw IllegalConfigException if configuration parameters are wrong, or missing
	 */
	virtual SpiderCastConfig_SPtr createSpiderCastConfig(
			const PropertyMap& properties,
			const std::vector<NodeID_SPtr >& bootstrapSet,
			const std::vector<NodeID_SPtr >& supervisorBootstrapSet)
			throw (SpiderCastLogicError) = 0;

	/**
	 * Create a NodeID implementation from a string.
	 *
	 * The string must have the following format:
	 *
	 * Node-Name, [IP-Address, Address-scope,]+ Port
	 *
	 * NodeName, IP-Address, and Address-scope must not contain white-space or ',' (comma).
	 * The pair [IP-Address, Address-scope,] must appear once or more.
	 * NodeName, at least one IP-Address, and the Port are mandatory.
	 * The Address-scope can be an empty string.
	 *
	 * Example:
	 *
	 * Node1, 1.2.3.4, scopeX, 4.3.2.1, , 34567
	 *
	 * @param idstr a string describing the NodeID
	 * @return a boost::shared_pointer to a NodeID implementation
	 */
	virtual NodeID_SPtr createNodeID_SPtr(const String& idstr) throw (SpiderCastLogicError) = 0;

	/**
	 * Load the bootstrap-set from an input stream.
	 *
	 * For example from a file.
	 * In the input stream each line describes a node.
	 * Each line contains a NodeID string representation in the format described in the method:
	 * 		NodeID SpiderCastFactory::createNodeID_SPtr()
	 *
	 * @param inputStream
	 * @return a new vector with the respective NodeID's.
	 *
	 * @throw std::ios_base::failure if I/O failure occurred while reading from the input stream (badbit set)
	 * @throw SpiderCastLogicError if the format of the NodeID strings is wrong
	 */
	virtual std::vector<NodeID_SPtr > loadBootstrapSet(std::istream& inputStream)
			throw(std::ios_base::failure, SpiderCastLogicError) = 0;

	/**
	 * Load the bootstrap-set from a simple line.
	 *
	 * In the input line each triplet {name, address, port,...} describes a node (with a global scope).
	 * For example: "node1, 10.10.10.10, 11111, node2, 10.10.10.11, 11112"
	 *
	 * Each triplet contains a NodeID string in a simplified representation, which can be converted by the method:
	 * 		NodeID_SPtr SpiderCastFactory::createNodeID_SPtr("name, address, , port");
	 *
	 * @param line a C-string with the comma separated triplets
	 * @return a new vector with the respective NodeID's.
	 *
	 * @throw std::ios_base::failure if I/O failure occurred while reading from the input stream (badbit set)
	 * @throw SpiderCastLogicError if the format of the NodeID strings is wrong
	 */
	virtual std::vector<NodeID_SPtr > loadBootstrapSetSimpleLine(const char* line)
						throw(SpiderCastLogicError) = 0;

	/**
	 * Load the bootstrap-set from a string containing URIs.
	 *
	 * In the input string each node is describes by a URI: name@address:port (with a global scope).
	 * UIRs are separated by commas. Leading and trailing white space is ignored.
	 *
	 * For example: "node1@10.10.10.10:11111, node2@10.10.10.11:11112"
	 *
	 * Each URI contains a NodeID string in a simplified representation, which can be converted by the method:
	 * 		NodeID_SPtr SpiderCastFactory::createNodeID_SPtr("name, address, , port");
	 *
	 * IPv6 addresses are represented like so:
	 * name@[address_v6]:port
	 * For example: "node1@[1234::1]:11111, node2@[2345::1]:11112"
	 *
	 * Multiple URIs can have the same name.
	 *
	 * @param line a C-string with the comma separated URIs
	 * @return a new vector with the respective NodeID's.
	 * @throw SpiderCastLogicError in case parsing the line failed
	 */
	virtual std::vector<NodeID_SPtr > loadBootstrapSetURIs(const char* line)
								throw(SpiderCastLogicError) = 0;

	/**
	 * Register a log listener to receive internal log events.
	 *
	 * @param logListener
	 * @param log_level
	 */
	virtual void registerLogListener(log::LogListener * const logListener, log::Level log_level) = 0;

	/**
	 * Change the log level.
	 *
	 * This should only be called after a LogListener had been registered, otherwise it has no effect.
	 *
	 * @param log_level
	 */
	virtual void changeLogLevel(log::Level log_level) = 0;

	/**
	 * Change the log level on a specific component.
	 *
	 * This should only be called after a LogListener had been registered, otherwise it has no effect.
	 * The SpiderCast trace component name is defined in: spdr::trace::ScTrConstants
	 *
	 * @param log_level
	 * @param componentName
	 * @param subComponentName
	 */
	virtual void changeLogLevel(log::Level log_level, const std::string& componentName, const std::string& subComponentName = "") = 0;

	/**
	 * Change the RUM log level.
	 *
	 * This should only be called after a LogListener had been registered, otherwise it has no effect.
	 *
	 * RUM log levels range from 0 (none) to 9 (most detailed). Default is 5 (Info).
	 *
	 * @param log_level
	 */
	virtual void changeRUMLogLevel(int log_level) = 0;
};

} //namespace spdr

#endif /* SPDR_SPIDERCASTFACTORY_H_ */
