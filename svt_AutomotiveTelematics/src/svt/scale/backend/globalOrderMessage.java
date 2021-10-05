/*
 * Copyright (c) 2012-2021 Contributors to the Eclipse Foundation
 * 
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 * 
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0
 * 
 * SPDX-License-Identifier: EPL-2.0
 */
package svt.scale.backend;

import java.util.Collections;
import java.util.HashMap;
import java.util.Hashtable;
import java.util.LinkedList;
import java.util.List;
import java.util.Map.Entry;
import java.util.concurrent.Semaphore;

import svt.scale.backend.ATBackendEngine;

/**
 * Class to encapsulate the ISM topic subscribe logic.
 * 
 * This class contains the doSubscribe method for subscribing to ISM topic messages. It also defines the necessary MQTT
 * callbacks for asynchronous subscriptions.
 * 
 */
class globalOrderMessage implements Runnable {
    HashMap<String, Long> publishersIndexTable = new HashMap<String, Long>();
    Long newMessage = 100000L;

    private final Semaphore mutex = new Semaphore(1);
    boolean pass = true;
    String publisherId = null;
    Long l = null;

    private List<String> failureList = new LinkedList<String>();
    private HashMap<String, Long> subscriberCounts = new HashMap<String, Long>();
    private HashMap<String, Long> publisherCounts = new HashMap<String, Long>();
    Hashtable<String, List<MessageStrings>> sortedBySubscriber = new Hashtable<String, List<MessageStrings>>();
    Hashtable<String, List<MessageStrings>> sortedByPublisher = new Hashtable<String, List<MessageStrings>>();
    private HashMap<String, Long> publisherFinalIndexes = new HashMap<String, Long>();
    List<MessageStrings> failList = new LinkedList<MessageStrings>();

    String failure = null;

    long verifiedCount = 0;
    long failCount = 0;
    long removeCount = 0;
    long privSortCount = 0;
    long missingCount = 0;
    long dupCount = 0;
    long outOfOrderCount = 0;

    synchronized long sortCount(Integer inc) {
        if (inc != null)
            privSortCount += inc;
        return privSortCount;
    }

    private boolean privSortingIsDoneFlag = false;

    synchronized public boolean sortingIsDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            // if (gatherer.config.debug)
            gatherer.config.log.println(message);
            privSortingIsDoneFlag = done;
        }
        return privSortingIsDoneFlag;
    }

    private boolean privVerifyIsDoneFlag = false;

    synchronized public boolean verifyIsDone(Boolean done, String message) {
        if ((done != null) && (done == true)) {
            if (gatherer.config.debug)
                gatherer.config.log.println(message);
            privVerifyIsDoneFlag = done;
        }
        return privVerifyIsDoneFlag;
    }

    // boolean allSubscribersHaveAMessage() {
    // return ((sortedBySubscriber.size() == gatherer.config.sharedSubscribers) && (minSubscribersDepth() > 0));
    // }

    class MessageStrings {
        byte[] b = null;
        String string = null;
        String[] strings = null;

        public MessageStrings(byte[] b) {
            this.b = b;
            this.strings = new String(b).split(":");
        }

        private MessageStrings(String lString) {
            this.string = lString;
            this.strings = string.split(":");
            if (this.strings.length < 6) {
                gatherer.config.log.printErr("Message '" + this.string + "' only contained " + this.strings.length
                                + " parts");
            }
        }

        public String getSharedSubscriber() {
            return strings[0];
        }

        public String getKeyWord() {
            return new String(strings[1]);
        }

        public String getPublisherId() {
            return strings[2];
        }

        public Integer getIncomingQoS() {
            return Integer.parseInt(strings[3]);
        }

        public Long getIndex() {
            return Long.parseLong(strings[4]);
        }

        public Long getFinalIndex() {
            return Long.parseLong(strings[5]);
        }
    }

    boolean publisherPerSubscriber(String publisherId) {
        boolean found = false;
        MessageStrings ms = null;
        if (sortedBySubscriber.size() == gatherer.config.sharedSubscribers) {
            found = true;
            for (Entry<String, List<MessageStrings>> subscriberQueue : sortedBySubscriber.entrySet()) {
                ms = getFirst(subscriberQueue.getValue(), publisherId);
                if (ms == null) {
                    found = false;
                }
            }
        }
        return found;
    }

    MessageStrings leastFirstOnAnyList(String publisherId) {
        MessageStrings least = null;
        MessageStrings first = null;
        for (Entry<String, List<MessageStrings>> entry : sortedBySubscriber.entrySet()) {
            first = getFirst(entry.getValue(), publisherId);
            if (first != null) {
                if ((least == null) || (first.getIndex() < least.getIndex()))
                    least = first;
            }
        }
        return least;
    }

    MessageStrings getFirst(List<MessageStrings> list, String publisherId) {
        MessageStrings ms = null;

        if (list != null)
            synchronized (list) {
                if ((list == null) || (list.size() == 0)) {
                    ms = null;
                } else {
                    for (MessageStrings m : list) {
                        if (m.getPublisherId().equals(publisherId)) {
                            ms = m;
                            break;
                        }
                    }
                }
            }
        return ms;
    }

    boolean remove(MessageStrings ms) {
        boolean done = false;
        List<MessageStrings> subscriberList = sortedBySubscriber.get(ms.getSharedSubscriber());
        List<MessageStrings> publisherList = sortedByPublisher.get(ms.getPublisherId());
        synchronized (subscriberList) {
            done = subscriberList.remove(ms);
            done = publisherList.remove(ms) && done;
            removeCount++;
        }
        return done;
    }

    boolean done = false;
    long privReceivedcount = 0;
    private ATBackendEngine gatherer = null;

    public globalOrderMessage(ATBackendEngine subscriber) {
        this.gatherer = subscriber;
        done = false;
    }

    public int maxSubscribersDepth() {
        Integer size = 0;

        for (Entry<String, List<MessageStrings>> entry : sortedBySubscriber.entrySet()) {
            if (entry.getValue().size() > size)
                size = entry.getValue().size();
        }

        return size;
    }

    public long totalListSize() {
        long size = 0;

        for (Entry<String, List<MessageStrings>> entry : sortedBySubscriber.entrySet()) {
            size += entry.getValue().size();
        }
        return size;
    }

    /**
     * Synchronized method for indicating all messages received.
     * 
     * @param setDone
     *            optional Boolean parameter indicating all messages received.
     * 
     * @return Boolean flag
     */
    synchronized public boolean verifyIsDone(Boolean setDone) {
        if (setDone != null) {
            this.done = setDone;
        }
        return this.done;
    }

    synchronized public boolean isPass(Boolean pass) {
        if (pass != null) {
            this.pass = pass;
        }
        return this.pass;
    }

    synchronized public long receivedCount(int i) {
        privReceivedcount += i;
        return privReceivedcount;
    }

    public void run() {

        if (gatherer.config.debug) {
            gatherer.config.log.println("gobalOrderMessage.run() starting");
        }

        Thread sortIncoming = new Thread(new sortIncoming(gatherer));
        sortIncoming.start();

        boolean search = false;
        MessageStrings parsedMessage = null;
        int max = 0;
        while (!verifyIsDone(null, null)) {
            parsedMessage = null;

            mutex.acquireUninterruptibly();
            max = maxSubscribersDepth();
            mutex.release();

            if (max == 0) {
                synchronized (newMessage) {
                    try {
                        newMessage.wait(1000);
                    } catch (InterruptedException e) {
                    }
                }
            }

            mutex.acquireUninterruptibly();

            search = false;
            for (Entry<String, List<MessageStrings>> publisherEntry : sortedByPublisher.entrySet()) {
                publisherId = publisherEntry.getKey();
                boolean sortingIsDone = sortingIsDone(null, null);
                boolean publisherPerSubscriber = publisherPerSubscriber(publisherId);
                parsedMessage = leastFirstOnAnyList(publisherId);
                if (parsedMessage != null) {
                    if (sortingIsDone || publisherPerSubscriber) {
                        search = true;
                        if (isNext(parsedMessage)) {
                            record(parsedMessage);
                        } else {
                            fail(parsedMessage);
                        }
                        remove(parsedMessage);
                    }
                }
            }

            if ((search == false) && (!sortingIsDone(null, null))) {
                mutex.release();
                synchronized (newMessage) {
                    try {
                        newMessage.wait(2000);
                    } catch (InterruptedException e) {
                    }
                }
                mutex.acquireUninterruptibly();
            }

            if ((sortingIsDone(null, null) && (removeCount == sortCount(null)) && (maxSubscribersDepth() == 0))) {
                verifyIsDone(true, "verifyIsDone set true because sorting is done and removeCount equals sortCount of "
                                + sortCount(null) + " and 0 messages remain to be sorted");
            }
            mutex.release();

        }

        if ((gatherer.config.discard == false) && (removeCount < gatherer.config.count)) {
            isPass(false);
            for (Entry<String, Long> entry : publishersIndexTable.entrySet()) {
                long delta = publisherFinalIndexes.get(entry.getKey()).longValue() - entry.getValue().longValue();
                if (delta == 1) {
                    failCount++;
                    String failure = "Order Error for publisher " + entry.getKey() + ", index "
                                    + publisherFinalIndexes.get(entry.getKey())
                                    + " is missing.  Previous message index was " + entry.getValue();
                    failureList.add(failure);
                    gatherer.config.log.printErr(failure);
                } else if (delta > 1) {
                    failCount++;
                    String failure = "Order Error for publisher " + entry.getKey() + ", index "
                                    + (entry.getValue() + 1) + " to " + publisherFinalIndexes.get(entry.getKey())
                                    + " are missing.  Previous message index was " + entry.getValue();
                    failureList.add(failure);
                    gatherer.config.log.printErr(failure);
                }
            }
        }

        if (isPass(null)) {
            gatherer.config.log.println("Order check SUCCESS -- verified " + verifiedCount + " messages.");
            gatherer.config.log.println("failCount is " + failCount);
            gatherer.config.log.println("Messages remaining to be verified is " + totalListSize());
            if (totalListSize() > 0) {
                gatherer.config.log.println("--------------");
                for (Entry<String, List<MessageStrings>> entry : sortedBySubscriber.entrySet()) {
                    gatherer.config.log.println("subscriber " + entry.getKey() + " has " + entry.getValue().size()
                                    + " messages remaining");
                }
            }
            gatherer.config.log.println("--------------");
            gatherer.config.log.println("-  Number of publishers: " + sortedByPublisher.size());
            long total = 0;
            if (publishersIndexTable.size() > 0) {
                for (Entry<String, Long> entry : publisherCounts.entrySet()) {
                    gatherer.config.log.println("publisherID: " + entry.getKey() + ", count " + entry.getValue());
                    total += entry.getValue();
                }
                gatherer.config.log.println("                     Total:  " + total);
            }
            gatherer.config.log.println("--------------");
            total = 0;
            gatherer.config.log.println("-  Number of subscribers: " + subscriberCounts.size());
            if (subscriberCounts.size() > 0) {
                for (Entry<String, Long> entry : subscriberCounts.entrySet()) {
                    gatherer.config.log.println("subscriberId: " + entry.getKey() + ", count " + entry.getValue());
                    total += entry.getValue();
                }
                gatherer.config.log.println("                      Total:  " + total);
            }
            gatherer.config.log.println("--------------");
        } else {
            gatherer.config.log.println("\n");
            gatherer.config.log.println("Missed Message Count " + missingCount);
            gatherer.config.log.println("Duplicate Message Count " + dupCount);
            gatherer.config.log.println("Out of Order Message Count " + outOfOrderCount);
            gatherer.config.log.println("\n");
            gatherer.config.log.println("Failure Report");
            gatherer.config.log.println("--------------");
            gatherer.config.log.println("-  Pass: " + pass);
            gatherer.config.log.println("\n");
            gatherer.config.log.println("-  Number of publishers: " + publisherCounts.size());
            if (publishersIndexTable.size() > 0) {
                gatherer.config.log.println("-  List of publishers:");
                for (Entry<String, Long> entry : publisherCounts.entrySet()) {
                    gatherer.config.log.println("publisherID: " + entry.getKey() + ", count " + entry.getValue());
                }
            }
            gatherer.config.log.println("-  Number of subscribers: " + subscriberCounts.size());
            if (subscriberCounts.size() > 0) {
                gatherer.config.log.println("-  List of subscribers:");
                for (Entry<String, Long> entry : subscriberCounts.entrySet()) {
                    gatherer.config.log.println("subscriberId: " + entry.getKey() + ", count " + entry.getValue());
                }
            }
            gatherer.config.log.println("\n");
            gatherer.config.log.println("-  Number of failures:" + failureList.size());
            if (failureList.size() > 0) {
                gatherer.config.log.println("-  List of failures:");
                while (failureList.isEmpty() == false) {
                    gatherer.config.log.println("-  " + failureList.remove(0));
                }
            }
            gatherer.config.log.println("--------------\n");
        }

        try {
            sortIncoming.join();
        } catch (InterruptedException e) {
            // TODO Auto-generated catch block
            e.printStackTrace();
        }

        if (gatherer.config.debug)
            gatherer.config.log.println("gobalOrderMessage.run() return");

        return;
    }

    int getQoS(MessageStrings parsedMessage) {
        int qos = 0;
        if (gatherer.config.discard) {
            qos = 0;
        } else {
            qos = Math.min(gatherer.config.qos, parsedMessage.getIncomingQoS());
        }

        return qos;
    }

    boolean isNext(MessageStrings parsedMessage) {
        boolean found = false;
        if ("ORDER".equals(parsedMessage.getKeyWord())) {
            String pubId = parsedMessage.getPublisherId();
            Long previousRecordedIndex = publishersIndexTable.get(pubId);
            Long incomingIndex = parsedMessage.getIndex();
            int qos = getQoS(parsedMessage);

            if ((qos == 0) && ((previousRecordedIndex == null))) {
                found = true;
                if (incomingIndex != 0) {
                    gatherer.config.log.printErr("WARNING:  initial message index for " + pubId + " and "
                                    + parsedMessage.getSharedSubscriber() + " is " + incomingIndex);
                }
            } else if ((qos == 0) && ((previousRecordedIndex != null) && (incomingIndex > previousRecordedIndex))) {
                found = true;
                if (incomingIndex != (previousRecordedIndex + 1)) {
                    gatherer.config.log.printErr("WARNING:  message index for " + pubId + " and "
                                    + parsedMessage.getSharedSubscriber() + " is " + incomingIndex + " instead of "
                                    + (previousRecordedIndex + 1));
                }
            } else if ((qos == 1) && ((previousRecordedIndex == null) && (incomingIndex == 0))) {
                found = true;
            } else if ((qos == 1)
                            && ((previousRecordedIndex != null) && ((incomingIndex == previousRecordedIndex) || (incomingIndex == previousRecordedIndex + 1)))) {
                found = true;
            } else if ((qos == 2) && ((previousRecordedIndex == null) && (incomingIndex == 0))) {
                found = true;
            } else if ((qos == 2) && ((previousRecordedIndex != null) && (incomingIndex == previousRecordedIndex + 1))) {
                found = true;
            }
        }
        return found;

    }

    public void record(MessageStrings parsedMessage) {
        int qos = Math.min(gatherer.config.qos, parsedMessage.getIncomingQoS());
        if (gatherer.config.debug)
            gatherer.config.log.println("publishersIndexTable.put(" + parsedMessage.getPublisherId() + ", "
                            + parsedMessage.getIndex() + ")");
        publishersIndexTable.put(parsedMessage.getPublisherId(), parsedMessage.getIndex());
        verifiedCount++;
        gatherer.config.log.println(parsedMessage.getSharedSubscriber() + "(subscriberID),  "
                        + parsedMessage.getPublisherId() + ":" + qos + ":" + parsedMessage.getIndex() + " -- verified "
                        + verifiedCount + "/" + gatherer.config.count);
    }

    public void fail(MessageStrings parsedMessage) {
        Long previousRecordedIndex = publishersIndexTable.get(parsedMessage.getPublisherId());
        long index = parsedMessage.getIndex();
        int qos = Math.min(gatherer.config.qos, parsedMessage.getIncomingQoS());

        failCount++;

        if (gatherer.config.debug)
            gatherer.config.log.println("publishersIndexTable.get(" + parsedMessage.getPublisherId() + ") returned "
                            + publishersIndexTable.get(parsedMessage.getPublisherId()));
        String failure = "Order Error for publisher " + parsedMessage.getPublisherId() + " and shared subscriber "
                        + parsedMessage.getSharedSubscriber() + " Qos " + qos + ", index " + index
                        + " is incorrect.  Previous message index was " + previousRecordedIndex;

        if (previousRecordedIndex == null) {
            missingCount = index;
        } else {
            if (index > previousRecordedIndex)
                missingCount += (index - previousRecordedIndex - 1);
            if (index == previousRecordedIndex)
                dupCount++;
            if (index < previousRecordedIndex) {
                outOfOrderCount++;
                missingCount -= 2;
            }
        }

        failureList.add(failure);
        gatherer.config.log.printErr(failure);
        publishersIndexTable.put(parsedMessage.getPublisherId(), parsedMessage.getIndex());
        isPass(false);
    }

    public class sortIncoming implements Runnable {
   	 ATBackendEngine gatherer = null;
        public List<String> sortList = Collections.synchronizedList(new LinkedList<String>());

        sortIncoming(ATBackendEngine gatherer) {
            this.gatherer = gatherer;
        }

        int messagesWaitingToBeSorted() {
            return gatherer.orderCheckMessageList.size();
        }

        public void run() {

            while ((!gatherer.subscriberIsDone(null, null)) || (messagesWaitingToBeSorted() > 0)) {
                mws = messagesWaitingToBeSorted();
                if (mws == 0) {
                    try {
                        Thread.sleep(200);
                    } catch (InterruptedException e) {
                        // TODO Auto-generated catch block
                        e.printStackTrace();
                    }
                } else {
                    sort();
                }
            }

            sortingIsDone(true, "sortingIsDone set true since subscriberIsDone is "
                            + gatherer.subscriberIsDone(null, null) + " and " + messagesWaitingToBeSorted()
                            + " messages remain to sort");

            synchronized (newMessage) {
                newMessage.notify();
            }

            return;
        }

        int mws = 0;
        int size = 0;

        private void sort() {
            mutex.acquireUninterruptibly();
            size = Math.min(gatherer.orderCheckMessageList.size(), 2000);
            for (int i = 0; i < size; i++) {
                sortCount(1);
                sortBySubscriber(sortedBySubscriber, gatherer.orderCheckMessageList.remove(0).getMessageText());
            }
            mutex.release();
        }

        private void sortBySubscriber(Hashtable<String, List<MessageStrings>> sortedBySubscriber, String message) {
            Long i = 0L;
            boolean newList = false;
            MessageStrings messageStrings = new MessageStrings(message);
            publisherFinalIndexes.put(messageStrings.getPublisherId(), messageStrings.getFinalIndex());
            String sharedSubscriber = messageStrings.getSharedSubscriber();
            if (subscriberCounts.containsKey(sharedSubscriber)) {
                i = subscriberCounts.get(sharedSubscriber);
                subscriberCounts.put(sharedSubscriber, ++i);
            } else {
                subscriberCounts.put(sharedSubscriber, 1L);
            }
            String publisherID = messageStrings.getPublisherId();
            if (publisherCounts.containsKey(publisherID)) {
                i = publisherCounts.get(publisherID);
                publisherCounts.put(publisherID, ++i);
            } else {
                publisherCounts.put(publisherID, 1L);
            }
            List<MessageStrings> subscriberList = sortedBySubscriber.get(sharedSubscriber);

            if ((subscriberList == null) || (subscriberList.size() == 0)) {
                newList = true;
            }

            if (subscriberList == null) {
                subscriberList = new LinkedList<MessageStrings>();
                subscriberList.add(messageStrings);
                sortedBySubscriber.put(sharedSubscriber, subscriberList);

            } else {
                subscriberList.add(messageStrings);
            }
            List<MessageStrings> publisherList = sortedByPublisher.get(publisherID);
            if (publisherList == null) {
                publisherList = new LinkedList<MessageStrings>();
                publisherList.add(messageStrings);
                sortedByPublisher.put(publisherID, publisherList);
            } else {
                publisherList.add(messageStrings);
            }

            if (newList) {
                synchronized (newMessage) {
                    newMessage.notify();
                }
            }
        }

    }

}
