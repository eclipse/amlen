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
 * SpiderCastDocumentation.h
 *
 *  Created on: Mar 15, 2012
 */

#ifndef SPIDERCASTDOCUMENTATION_H_
#define SPIDERCASTDOCUMENTATION_H_

//--- Page documentation ------------------------------------------------------
/**
 * \mainpage SpiderCast (C++) User Manual
 *
 * \section intro_sec Introduction
 *
 * SpiderCast is a distributed system that provides membership and monitoring services,
 * accompanied by scalable communication services. SpiderCast is based on overlay and
 * peer-to-peer technologies.
 *
 * SpiderCast services are:

 *\li Membership service.
 *\li Attribute replication service.
 *\li Light-weight topic-based publish/subscribe messaging.
 *\li Leader election service.
 *
 *  The **membership service** enable the discovery of active nodes, as well as
 *  the detection of node failures and departures. The membership service provides
 *  a set of active nodes and supplies event notifications when the set changes due to
 *  nodes joining or leaving the overlay.
 *
 *  The **attribute service** allows each node to declare a set of attributes of itself,
 *	using a per node map-like API - putAttribute, getAttribute, etc. The attribute
 *	map of each node is efficiently replicated to every other node. This service allows
 *	a node to efficiently propagate slowly changing state information (node attributes),
 *	and can be used to disseminate and discover deployed services, supported protocols,
 *	server role, etc.
 *
 *  The **topic-based publish/subscribe** messaging service allows each node to send messages
 *  to a group (topic), and to receive all messages sent to a group (topic) by other
 *  nodes. This service is also known as Application Level Multicast (ALM). It allow
 *  data replication, data dissemination, load monitoring, implementation of shared state, etc.
 *
 *  The **leader election service** allows a user selected set of SpiderCast candidate
 *  nodes to elect a leader among themselves. It also allows a set of observer nodes to be
 *  informed of the election results, but remain outside the election process.
 *
 * <p>
 * <B>Additional information</B>
 *
 * The following sections provide additional information:
 *
 * <UL>
 * 	<LI> \subpage the_node_page
 *  <LI> \subpage mem_service_page
 *  <LI> \subpage attr_service_page
 *  <LI> \subpage pubsub_service_page
 *  <LI> \subpage leader_election_service_page
 *  <LI> \subpage contact_info_page
 * </UL>
 *
 * <HR>
 * \ref the_node_page "Next" The SpiderCast Node
 *
 * <HR>
 *
 * \section copyright_sec Copyright Notice
 *
 * SpiderCast (C++).
 *
 *
 */

//-----------------------------------------------------------------------------

/**
 * \page 	the_node_page The SpiderCast Node
 * \section the_node_sec Creation and configuration
 *
 * SpiderCast is centered around the concept of a node. A node is a unique entity in the overlay,
 * that has a unique name and network endpoints. \link spdr::SpiderCast SpiderCast\endlink is the main class that represents a node.
 * A node is created using the \link spdr::SpiderCastFactory SpiderCastFactory\endlink which is the main factory class.
 * In order to properly create a node some mandatory configuration parameters need to be supplied.
 * Those are encapsulated in the \link spdr::SpiderCastConfig SpiderCastConfig \endlink class, which is also created using
 * the SpiderCastFactory. The SpiderCastConfig class also allows additional optional
 * configuration parameters to be supplied.
 *
 * When the created instance of a SpiderCast node is started, it starts a phase in which it discovers
 * overlay peers and connects to a subset of them. Peers to which a node is connected to are called
 * neighbors. The node then proceeds with a gossip protocol that implements the membership and attribute
 * services, and establishes an internal representation of the overlay node membership.
 *
 * The services that a SpiderCast node provides, like the membership service, are all created by
 * factory methods from the SpiderCast instance.
 *
 * \section node_example_sec Example
 *
 * Here is a simple example of how to create a SpiderCast node and start it.
 * \code
 * SpiderCastFactory& factory = SpiderCastFactory::getInstance();
 *
 * //create a properties object with mandatory properties
 * PropertyMap prop;
 * prop.setProperty(config::NodeName_PROP_KEY,"node1");
 * prop.setProperty(config::BusName_PROP_KEY,"/test-bus");
 * prop.setProperty(config::NetworkInterface_PROP_KEY,"10.0.0.5, Scope1");
 * prop.setProperty(config::TCPReceiverPort_PROP_KEY,"34561");
 *
 * //create a bootstrap set with two other nodes
 * std::vector<NodeID_SPtr > bootstrap_set;
 * NodeID_SPtr id_SPtr = factory.createNodeID_SPtr(
 *    "node2, 10.0.0.6, Scope1, 34561");
 * bootstrap_set.push_back(id_SPtr);
 * id_SPtr = factory.createNodeID_SPtr(
 *    "node3, 10.0.0.7, Scope1, 34561");
 * bootstrap_set.push_back(id_SPtr);
 *
 * //create the configuration object
 * SpiderCastConfig_SPtr spidercastConfig_SPtr =
 *    factory.createSpiderCastConfig(prop, bootstrap_set);
 *
 * //create a simple event listener implementation
 * SpiderCastEventListenerPrintStub eventListener;
 *
 * //create the SpiderCast node
 * SpiderCast_SPtr spidercast_SPtr = factory.createSpiderCast(
 *    *spidercastConfig_SPtr,eventListener);
 * //and start it
 * spidercast_SPtr->start();
 * //the node now joins the overlay, let it run for a while
 * boost::this_thread::sleep(boost::posix_time::seconds(60));
 * //and close it
 * spidercast_SPtr->close(true);
 * \endcode
 *
 * The simple implementation of the SpiderCastEventListener is the following.
 *
 * \code
 * class SpiderCastEventListenerPrintStub : public SpiderCastEventListener {
 * public:
 *   SpiderCastEventListenerPrintStub() {}
 *   ~SpiderCastEventListenerPrintStub() {}
 *   //the event handler just prints
 *   void onEvent(const boost::shared_ptr<event::SpiderCastEvent> event) {
 *     if (event) {
 *       std::cout << event->toString() << std::endl;
 *       switch (event->getType()) {
 *         case event::Fatal_Error:
 *         std::cout << "Opps... fatal error..." << std::endl;
 *         break;
 *
 *         default:
 *         std::cout << "Some other event..." << std::endl;
 *       }
 *     } else {
 *       std::cerr << "event is null, this can't be good..." << std::endl;
 *     }
 *   }
 * };
 * \endcode
 *
 * Note that these code snippets avoid some boiler-plate code and error paths.
 *
 * <HR>
 * \ref mem_service_page "Next": The Membership Service
 */

//-----------------------------------------------------------------------------

/**
 * \page 	mem_service_page The Membership Service
 *
 * \section mem_service_sec Function
 *
 * The access point to the membership service is the \link spdr::MembershipService MembershipService \endlink interface.
 * An implementation of this interface is created by calling
 * \link spdr::SpiderCast#createMembershipService() SpiderCast#createMembershipService() \endlink.
 * In the creation process the user essentially registers an event listener (an implementation
 * of the \link spdr::MembershipListener MembershipListener \endlink interface),
 * which is then used by SpiderCast to deliver membership events to the user.
 *
 * The membership events first deliver the full membership view, by
 * a \link spdr::event::ViewChangeEvent ViewChangeEvent \endlink, followed by differential updates
 * to the membership. That is, when a new node starts and joins the overlay,
 * a \link spdr::event::NodeJoinEvent NodeJoinEvent \endlink is delivered.
 * When a node closes or fails,
 * a \link spdr::event::NodeLeaveEvent NodeLeaveEvent \endlink is delivered.
 * These events carry the respective node identifier (\link spdr::NodeID NodeID \endlink).
 *
 * \section mebership_example_sec Example
 *
 * The following code snippet demonstrates how to create the membership service.
 * Creation is allowed only after the node has been started.
 *
 * \code
 * //create and start the node (see previous example)
 * spidercast_SPtr->start();
 * //create a simple membership listener (see below)
 * ViewKeeper memListener(spidercast_SPtr->getNodeID());
 * //create the membership service
 * PropertyMap prop;
 * MembershipService_SPtr memService_SPtr =
 *   spidercast_SPtr->createMembershipService(prop,memListener);
 * //from this moment on, membership events start flowing through the listener
 * //let it work for a while
 * boost::this_thread::sleep(boost::posix_time::seconds(60));
 * //close the membership service
 * memService_SPtr->close();
 * //close the node
 * spidercast_SPtr->close(true);
 * \endcode
 *
 * Here is a simple example of a membership listener implementation.
 * This class maintains an internal set of live nodes (a view), and allows
 * another thread to get a copy of the view. This code is thread safe.
 *
 * \code
 *
 * namespace spdr {
 * class ViewKeeper: public MembershipListener {
 * private:
 *   //the view, a set of NodeID (shared-pointer)
 *   std::set<NodeID_SPtr> view_;
 *   NodeID_SPtr myNodeID_;
 *   mutable boost::recursive_mutex mutex_;
 *
 * public:
 *   ViewKeeper(NodeID_SPtr myNodeID) : myNodeID_(myNodeID) {}
 *
 *   virtual ~ViewKeeper() {}
 *
 *   //process membership events
 *   void onMembershipEvent(event::MembershipEvent_SPtr event) {
 *     if (event) {
 *       std::cout << myNodeID_->toString() << " : "
 *         << event->toString() << std::endl;
 *
 *       switch (event->getType())
 *       {
 *          case event::View_Change:
 *          { //the first view
 *            boost::recursive_mutex::scoped_lock lock(mutex_);
 *            event::ViewChangeEvent_SPtr vcp = boost::static_pointer_cast<
 *              event::ViewChangeEvent>(event);
 *            for (event::ViewMap::const_iterator it =
 *              vcp->getView()->begin(); it != vcp->getView()->end(); ++it) {
 *                view_.insert(it->first);
 *            }
 *          }
 *          break;
 *
 *         case event::Node_Join:
 *         { //insert to the view
 *           boost::recursive_mutex::scoped_lock lock(mutex_);
 *           event::NodeJoinEvent_SPtr njp = boost::static_pointer_cast<
 *             event::NodeJoinEvent>(event);
 *           view_.insert(njp->getNodeID());
 *          }
 *          break;
 *
 *          case event::Node_Leave:
 *          { //remove from view
 *            boost::recursive_mutex::scoped_lock lock(mutex_);
 *            event::NodeLeaveEvent_SPtr nlp = boost::static_pointer_cast<
 *              event::NodeLeaveEvent>(event);
 *            view_.erase(nlp->getNodeID());
 *          }
 *          break;
 *
 *          case event::Change_of_Metadata:
 *          {
 *            //update meta-data, ignore (see next section)
 *          }
 *          break;
 *        }
 *      }
 *   }
 *
 *   //allow another thread to get a copy of the view
 *   std::set<NodeID_SPtr> getView() {
 *     std::set<NodeID_SPtr> copy;
 *     {
 *       boost::recursive_mutex::scoped_lock lock(mutex_);
 *       copy->insert(view_.begin(), view_.end());
 *     }
 *     return copy;
 *   }
 * };
 * }
 * \endcode
 *
 * Note that these code snippets avoid some boiler-plate code and error paths.
 *
 * <HR>
 * \ref attr_service_page "Next": The Attribute Service
 */

//-----------------------------------------------------------------------------

/**
 * \page 	attr_service_page The Attribute Replication Service
 *
 * \section attr_service_sec Function
 *
 * The attribute service is integrated with the membership service.
 * Creating the MembershipService object provides access to the attribute service as well.
 *
 * Each node has an internal map of attributes only it can write.
 * A single attribute is a key-value pair, where keys are Strings and the values are byte buffers.
 * This map is replicated to all other nodes in the overlay.
 *
 * The MembershipService interface has methods for manipulating the node's (self) attribute map, for example:
 * <UL>
 * 	<LI> \link spdr::MembershipService::setAttribute() MembershipService::setAttribute() \endlink
 * 	<LI> \link spdr::MembershipService::removeAttribute() MembershipService::removeAttribute() \endlink
 * 	<LI> \link spdr::MembershipService::getAttribute() MembershipService::getAttribute() \endlink
 * </UL>
 *
 * Membership events carry, along with each node identifier, an object that encapsulates meta-data about the node.
 * The attribute map of a node is delivered as a member of the \link spdr::event::MetaData MetaData \endlink object.
 * Thus, when a node joins the overlay, the NodeJoinEvent also carries that node's
 * \link spdr::event::AttributeMap AttributeMap. \endlink
 *
 * When an existing node changes its attribute map, the change is propagated and subsequently delivered to all other nodes
 * with a \link spdr::event::ChangeOfMetaDataEvent ChangeOfMetaDataEvent \endlink event, that carries the updated attribute
 * map and the respective node identifier.
 *
 * \section Example
 *
 * This code snippet show a membership listener that accumulates the node membership view together with the meta-data,
 * in an internal map. The node also allows another thread to get a copy of the augmented view.
 *
 * \code
 *
 * namespace spdr {
 * class ViewKeeper2: public MembershipListener {
 * private:
 *   //the view, a map of NodeID (shared-pointer) and MetaData (shared-pointer) pairs
 *   //this is a shared-pointer to the map
 *   event::ViewMap_SPtr view_SPtr;
 *   NodeID_SPtr myNodeID_;
 *   mutable boost::recursive_mutex mutex_;
 *
 * public:
 *   SCViewKeeper2(NodeID_SPtr myNodeID) : myNodeID_(myNodeID) {}
 *
 *   virtual ~SCViewKeeper2() {}
 *
 *   //process membership events
 *   void onMembershipEvent(event::MembershipEvent_SPtr event) {
 *     if (event) {
 *       std::cout << myNodeID_->toString() << " : "
 *         << event->toString() << std::endl;
 *
 *       switch (event->getType())
 *       {
 *          case event::View_Change:
 *          { //the first view
 *            boost::recursive_mutex::scoped_lock lock(mutex_);
 *            event::ViewChangeEvent_SPtr vcp = boost::static_pointer_cast<
 *              event::ViewChangeEvent>(event);
 *            view_SPtr = vcp->getView();
 *          }
 *          break;
 *
 *         case event::Node_Join:
 *         { //insert to the view
 *           boost::recursive_mutex::scoped_lock lock(mutex_);
 *           event::NodeJoinEvent_SPtr njp = boost::static_pointer_cast<
 *             event::NodeJoinEvent>(event);
 *           view_SPtr->insert(event::ViewMap::value_type(
 *             njp->getNodeID(), njp->getMetaData())); //keep meta-data
 *          }
 *          break;
 *
 *          case event::Node_Leave:
 *          { //remove from view
 *            boost::recursive_mutex::scoped_lock lock(mutex_);
 *            event::NodeLeaveEvent_SPtr nlp = boost::static_pointer_cast<
 *              event::NodeLeaveEvent>(event);
 *            view_SPtr->erase(nlp->getNodeID());
 *          }
 *          break;
 *
 *          case event::Change_of_Metadata:
 *          { //update meta-data
 *            boost::recursive_mutex::scoped_lock lock(mutex_);
 *            event::ChangeOfMetaDataEvent_SPtr cmp = boost::static_pointer_cast<
 *              event::ChangeOfMetaDataEvent>(event);
 *            event::ViewMap_SPtr partial_view_SPtr = cmp->getView();
 *
 *            for (event::ViewMap::const_iterator it =
 *              partial_view_SPtr->begin(); it != partial_view_SPtr->end(); ++it) {
 *                //update only nodes that have changed their meta-data
 *                event::ViewMap::iterator res = view_SPtr->find(it->first);
 *                res->second = it->second;
 *            }
 *          }
 *          break;
 *        }
 *      }
 *   }
 *
 *   //allow another thread to get a copy of the view
 *   event::ViewMap_SPtr getView() {
 *     event::ViewMap_SPtr copy;
 *     {
 *       boost::recursive_mutex::scoped_lock lock(mutex_);
 *       if (view_SPtr) {
 *         copy.reset(new event::ViewMap);
 *         copy->insert(view_SPtr->begin(), view_SPtr->end());
 *       }
 *     }
 *     return copy;
 *   }
 * };
 * }
 * \endcode
 *
 * Note that these code snippets avoid some boiler-plate code and error paths.
 *
 * <HR>
 * \ref pubsub_service_page "Next": The Publish-Subscribe Service
 */

//-----------------------------------------------------------------------------

/**
 * \page pubsub_service_page The Publish-Subscribe Service
 *
 * Under construction.
 *
 * *Partialy implemented in this version*.
 *
 * <HR>
 * \ref leader_election_service_page "Next": The Leader Election Service
 */

//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------

/**
 * \page leader_election_service_page The Leader Election Service
 *
 * Under construction.
 *
 * <HR>
 * \ref contact_info_page "Next": Contact Information
 */

//--- END ---------------------------------------------------------------------


#endif /* SPIDERCASTDOCUMENTATION_H_ */
