/*
 * Copyright (c) 2021 Contributors to the Eclipse Foundation
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
package svt.jms.audio;

import java.io.File;
import java.io.IOException;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.DeliveryMode;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageProducer;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class JMSSendAudio extends Thread {
    Session session = null;
    MessageProducer producer = null;
    public Connection connection = null;
    StreamMessage msg = null;
    static String file = "C:/rka/tools/rtc/jazz/client/eclipse/workspace/svt_JMSSamples/src/svt/jms/audio/bell.wav";
    AudioInputStream audioInputStream = null;
    static int bytesPerFrame = 0;

    public static void main(String[] args) {
        new JMSSendAudio().run();
    }

    public void run() {
        audioInputStream = getAudioStream(file);

        connect();
        // send(audioInputStream.getFormat());
        send(audioInputStream);
        close();
    }

    public static AudioFormat getAudioFormat() {
        return getAudioStream(file).getFormat();
    }

    void connect() {
        try {
            ConnectionFactory connectionFactory = null;
            connectionFactory = ImaJmsFactory.createConnectionFactory();

            ((ImaProperties) connectionFactory).put("Server", "9.3.177.157");
            ((ImaProperties) connectionFactory).put("Port", "16102");

            ((ImaProperties) connectionFactory).put("Protocol", "tcp");

            ImaJmsException[] exceptions = ((ImaProperties) connectionFactory).validate(false);
            if (exceptions != null) {
                for (ImaJmsException e : exceptions)
                    System.out.println(e.getMessage());
                return;
            }

            connection = connectionFactory.createConnection();
            connection.setClientID("rka");

            session = connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);
            Destination dest = null;
            dest = session.createTopic("/topic");
            if (dest == null) {
                System.err.println("ERROR:  Unable to create topic:  /topic");
                System.exit(1);
            }

            // producer = session.createProducer(dest);
            producer.setDeliveryMode(DeliveryMode.PERSISTENT);
            msg = session.createStreamMessage();

        } catch (Throwable e) {
            e.printStackTrace();
            return;
        }
    }

    // void send(AudioFormat af) {
    // try {
    // ObjectMessage om = session.createObjectMessage();
    // om.setObject((Serializable) af);
    //
    // producer.send(om);
    // } catch (JMSException e) {
    // e.printStackTrace();
    // }
    // }

    void send(AudioInputStream audioInputStream) {
        try {
            byte[] buffer = new byte[1024 * bytesPerFrame];
            int counter;
            while ((counter = audioInputStream.read(buffer)) != -1) {
                if (counter > 0) {
                    msg.clearBody();
                    msg.writeBytes(buffer, 0, counter);
                    producer.send(msg);
                    System.out.println("sent audio buffer of " + buffer.length + " bytes.");
                }
            }
            audioInputStream.close();
        } catch (IOException e) {
            e.printStackTrace();
        } catch (JMSException e) {
            e.printStackTrace();
        }
    }

    void close() {
        try {
            connection.close();
        } catch (JMSException e) {
            e.printStackTrace();
        }

        return;
    }

    public static AudioInputStream getAudioStream(String fileName) {
        File soundFile = new File(fileName);
        AudioInputStream audioInputStream = null;

        try {
            audioInputStream = AudioSystem.getAudioInputStream(soundFile);
            bytesPerFrame = audioInputStream.getFormat().getFrameSize();
            if ((bytesPerFrame == AudioSystem.NOT_SPECIFIED) || (bytesPerFrame == 0)) {
                bytesPerFrame = 1;
            }
        } catch (Exception e) {
            System.out.println("Problem with file " + fileName + ":");
            e.printStackTrace();
        }
        return audioInputStream;
    }

}
