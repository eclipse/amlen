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
package svt.mqtt.mq;

/**
 * Class to encapsulate the ISM topic subscribe logic.
 * This class contains the doSubscribe method for subscribing to ISM topic messages. It also defines the necessary MQTT
 * callbacks for asynchronous subscriptions.
 * 
 */
public class MqttSampleOrderCheck2 {
    //
    // public class data {
    // // LinkedList<byte[]> rstack;
    // LinkedList<byte[]> ostack;
    // // ReentrantLock rlock;
    // ReentrantLock olock;
    //
    // public data() {
    // // this.rstack = new LinkedList<byte[]>();
    // this.ostack = new LinkedList<byte[]>();
    // // this.rlock = new ReentrantLock();
    // this.olock = new ReentrantLock();
    // }
    //
    // // long rstackPush(byte[] data) {
    // // rlock.lock();
    // // rstack.add(data);
    // // rlock.unlock();
    // // return receivedCount(1);
    // // }
    // //
    // // byte[] rstackPop() {
    // // byte[] b = null;
    // // rlock.lock();
    // // b = rstack.removeFirst();
    // // rlock.unlock();
    // // return b;
    // // }
    // //
    // // byte[] rstackPeek() {
    // // byte[] b = null;
    // // rlock.lock();
    // // b = rstack.peek();
    // // rlock.unlock();
    // // return b;
    // // }
    // //
    // // int rstackSize() {
    // // int i = 0;
    // // rlock.lock();
    // // i = rstack.size();
    // // rlock.unlock();
    // // return i;
    // // }
    // //
    // // boolean rstackisEmpty() {
    // // boolean i = true;
    // // rlock.lock();
    // // i = rstack.isEmpty();
    // // rlock.unlock();
    // // return i;
    // // }
    //
    // long ostackPush(byte[] data) {
    // olock.lock();
    // ostack.add(data);
    // olock.unlock();
    // return receivedCount(1);
    // }
    //
    // byte[] ostackPop() {
    // byte[] b = null;
    // olock.lock();
    // b = ostack.removeFirst();
    // olock.unlock();
    // return b;
    // }
    //
    // byte[] ostackPeek() {
    // byte[] b = null;
    // olock.lock();
    // b = ostack.peek();
    // olock.unlock();
    // return b;
    // }
    //
    // int ostackSize() {
    // int i = 0;
    // olock.lock();
    // i = ostack.size();
    // olock.unlock();
    // return i;
    // }
    //
    // boolean ostackisEmpty() {
    // boolean i = true;
    // olock.lock();
    // i = ostack.isEmpty();
    // olock.unlock();
    // return i;
    // }
    //
    // boolean isEmpty() {
    // boolean b = false;
    // olock.lock();
    // b = (ostack.isEmpty());
    // olock.unlock();
    // return b;
    // }
    // }
    //
    // Hashtable<Integer, data> stacks = new Hashtable<Integer, data>();
    // boolean done = false;
    // boolean complete = false;
    // long receivedcount = 0;
    // Long batch = 100000L;
    // MqttSample config = null;
    // Thread orderMessage = null;
    // boolean pass = false;
    //
    // // vdata vdata = new vdata();
    //
    // public MqttSampleOrderCheck2(MqttSample config) {
    // this.config = config;
    // done = false;
    // complete = false;
    //
    // Thread batchMessage = new Thread(new batchMessage());
    // batchMessage.start();
    //
    // if (config.order == true) {
    // orderMessage = new Thread(new globalOrderMessage());
    // orderMessage.start();
    // }
    //
    // }
    //
    // public void orderMessageJoin() {
    // try {
    // orderMessage.join();
    // } catch (InterruptedException e) {
    // // TODO Auto-generated catch block
    // e.printStackTrace();
    // }
    // }
    //
    // public void batchNotify() {
    // batch.notify();
    // }
    //
    // public boolean dataIsEmpty() {
    // boolean empty = true;
    // for (Integer id : stacks.keySet()) {
    // data data = stacks.get(id);
    // empty = empty && data.isEmpty();
    // // config.log.println("empty is "+empty);
    // }
    // return empty;
    // }
    //
    // /**
    // * Synchronized method for indicating all messages received.
    // *
    // * @param setDone
    // * optional Boolean parameter indicating all messages received.
    // *
    // * @return Boolean flag
    // */
    // synchronized public boolean isDone(Boolean setDone) {
    // if (setDone != null) {
    // this.done = setDone;
    // }
    // return this.done;
    // }
    //
    // synchronized public boolean isComplete(Boolean setComplete) {
    // if (setComplete != null) {
    // this.complete = setComplete;
    // }
    // return this.complete;
    // }
    //
    // synchronized public boolean isPass(Boolean pass) {
    // if (pass != null) {
    // this.pass = pass;
    // }
    // return this.pass;
    // }
    //
    // synchronized public long receivedCount(int i) {
    // receivedcount += i;
    // return receivedcount;
    // }
    //
    // class batchMessage implements Runnable {
    // long nowTime = 0;
    // long prevTime = 0;
    // long nowCount = 0;
    // long prevCount = 0;
    // long rate = 0;
    //
    // public void run() {
    // while (isDone(null) == false) {
    // synchronized (batch) {
    // try {
    // batch.wait(10000);
    // } catch (InterruptedException e) {
    // }
    // }
    // nowCount = receivedCount(0);
    // nowTime = System.currentTimeMillis();
    //
    // if ((prevTime > 0) && (isDone(null) == false)) {
    // rate = ((nowCount - prevCount) * 1000L) / (nowTime - prevTime);
    // config.log
    // .println(" Received " + (nowCount - prevCount) + " msgs at the rate " + rate
    // + " msg/sec");
    // }
    // prevTime = nowTime;
    // prevCount = nowCount;
    // }
    // return;
    // }
    // }
    //
    // class globalOrderMessage implements Runnable {
    // Hashtable<String, Long> table = new Hashtable<String, Long>();
    // Long previous = 0L;
    // Long incoming = 0L;
    // String[] words = null;
    // boolean sleep = true;
    // boolean pass = true;
    // String pubId = null;
    // int qos = 0;
    // int incomingqos = 0;
    // data data = null;
    // boolean force = false;
    // boolean next = false;
    // boolean found = false;
    // Hashtable<Integer, String[]> top = new Hashtable<Integer, String[]>();
    // Hashtable<String, Long> least = new Hashtable<String, Long>();
    // Hashtable<String, Integer> leastId = new Hashtable<String, Integer>();
    // Hashtable<String, String[]> leastWords = new Hashtable<String, String[]>();
    // Long l = null;
    // String word = null;
    // byte[] b = null;
    //
    // public globalOrderMessage() {
    // }
    //
    // public void run() {
    // while ((isDone(null) == false) || (dataIsEmpty() == false)) {
    // top.clear();
    // least.clear();
    // leastId.clear();
    // leastWords.clear();
    // sleep = true;
    // // while ((top.size() < config.shared)&&((config.count - receivedCount(0))>config.shared)) {
    // for (Integer id : stacks.keySet()) {
    // data = stacks.get(id);
    // if (data.ostackisEmpty() == false) {
    // b = data.ostackPeek();
    // word = new String(b);
    // words = word.split(":");
    // pubId = new String(words[1]);
    // incoming = Long.parseLong(words[3]);
    // top.put(id, words);
    // l = least.get(pubId);
    // if ((l == null) || (l > incoming)) {
    // least.put(pubId, incoming);
    // leastId.put(pubId, id);
    // leastWords.put(pubId, words.clone());
    // sleep = false;
    // }
    // }
    // }
    //
    // // if (top.size() < config.shared) {
    // // try {
    // // Thread.sleep(100);
    // // } catch (InterruptedException e) {
    // // // TODO Auto-generated catch block
    // // e.printStackTrace();
    // // }
    // // }
    // // }
    //
    // if (sleep == false) {
    // found = false;
    // for (String pubId : least.keySet()) {
    // words = leastWords.get(pubId);
    // if ("ORDER".equals(words[0])) {
    // // pubId = words[1];
    // previous = table.get(pubId);
    // incomingqos = Integer.parseInt(words[2]);
    // // incoming = Long.parseLong(words[3]);
    // incoming = least.get(pubId);
    // Integer id = leastId.get(pubId);
    // qos = Math.min(config.qos, incomingqos);
    //
    // if (qos == 0) {
    // stacks.get(id).ostackPop();
    // table.put(pubId, incoming);
    // found = true;
    // break;
    // } else if ((qos == 1) && ((previous == null) && (incoming == 0))) {
    // stacks.get(id).ostackPop();
    // table.put(pubId, incoming);
    // found = true;
    // break;
    // } else if ((qos == 1)
    // && ((previous != null) && ((incoming == previous) || (incoming == previous + 1)))) {
    // stacks.get(id).ostackPop();
    // table.put(pubId, incoming);
    // found = true;
    // break;
    // } else if ((qos == 2) && ((previous == null) && (incoming == 0))) {
    // stacks.get(id).ostackPop();
    // table.put(pubId, incoming);
    // found = true;
    // break;
    // } else if ((qos == 2) && ((previous != null) && (incoming == previous + 1))) {
    // stacks.get(id).ostackPop();
    // table.put(pubId, incoming);
    // found = true;
    // break;
    // }
    // }
    // }
    // if ((found == false) && ((top.size() == config.shared) || (isDone(null) == true))) {
    // for (String pubId : least.keySet()) {
    // incoming = least.get(pubId);
    // Integer id = leastId.get(pubId);
    // stacks.get(id).ostackPop();
    // Long index = table.get(pubId);
    // config.log.printErr("Order Error for publisher " + pubId + " and receiver " + id + " Qos "
    // + qos + ", index " + incoming
    // + " is incorrect.  Previous message index was " + index);
    // table.put(pubId, incoming);
    // isPass(false);
    // }
    // System.exit(1);
    // } else {
    // // String message = "";
    // // for (Integer id : stacks.keySet()) {
    // // data = stacks.get(id);
    // // message = message + " " + id + ":" + data.ostackSize();
    // // }
    // // config.log.println(message);
    // }
    // } else {
    // try {
    // Thread.sleep(500);
    // } catch (InterruptedException e) {
    // e.printStackTrace();
    // }
    // }
    // }
    //
    // if (isPass(null) == true) {
    // config.log.println("Order check SUCCESS");
    // } else {
    // config.log.println("Order check FAILED");
    // }
    //
    // return;
    // }
    // }
    //
    // public data register(Integer id) {
    // data data = new data();
    // stacks.put(id, data);
    //
    // // if (config.order == true) {
    // // orderMessage = new Thread(new orderMessage(data));
    // // orderMessage.start();
    // // }
    //
    // return data;
    // }

}
