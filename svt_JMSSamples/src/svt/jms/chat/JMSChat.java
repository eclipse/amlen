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

package svt.jms.chat;

import svt.jms.audio.JMSReceiveAudio;

public class JMSChat implements Runnable {
    String[] args = null;

    public static void main(String[] args) {
        new JMSChat(args).run();
    }

    public JMSChat(String[] args) {
        this.args = args;
    }

    public void run() {
        try {
            if ((args.length == 0) || (args[0].equals("-receive"))) {
                new JMSReceiveAudio().start();
            }
            if ((args.length == 0) || (args[0].equals("-send"))) {
                new JMSSendThread().start();
            }
        } catch (Throwable e) {
            e.printStackTrace();
        }
        return;
    }

}
