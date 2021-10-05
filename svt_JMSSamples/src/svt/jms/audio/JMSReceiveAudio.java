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

import java.io.ByteArrayInputStream;
import java.io.IOException;
import java.io.InputStream;

import javax.jms.Connection;
import javax.jms.ConnectionFactory;
import javax.jms.Destination;
import javax.jms.JMSException;
import javax.jms.MessageConsumer;
import javax.jms.Session;
import javax.jms.StreamMessage;
import javax.jms.Topic;
import javax.sound.sampled.AudioFormat;
import javax.sound.sampled.AudioInputStream;
import javax.sound.sampled.AudioSystem;
import javax.sound.sampled.DataLine;
import javax.sound.sampled.FloatControl;
import javax.sound.sampled.LineUnavailableException;
import javax.sound.sampled.SourceDataLine;

import com.ibm.ima.jms.ImaJmsException;
import com.ibm.ima.jms.ImaJmsFactory;
import com.ibm.ima.jms.ImaProperties;

public class JMSReceiveAudio extends Thread {

    public Connection connection = null;
    Session session = null;
    MessageConsumer consumer = null;
    StreamMessage msg = null;
    AudioFormat af = null;

    public static void main(String[] args) {
        new JMSReceiveAudio().run();
    }

    public void run() {
        connect();
        getAudioFormat();
        listen();
        close();
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
            connection.setClientID("rkar");
        } catch (Throwable e) {
            e.printStackTrace();
            return;
        }

        try {
            session = connection.createSession(false, Session.DUPS_OK_ACKNOWLEDGE);

            Destination dest = session.createTopic("/topic");
            if (dest == null) {
                System.err.println("ERROR:  Unable to create topic:  /topic");
                System.exit(1);
            }

            // consumer = session.createDurableSubscriber((Topic) dest, "rkac");
            consumer = session.createConsumer((Topic) dest, "rkac");
            connection.start();

        } catch (JMSException e) {
            e.printStackTrace();
        }
    }

    // need to send format data as stream. tried sending as Object message
    // but AudioFormat object is not serializable.
    void getAudioFormat() {
        // while (af == null) {
        // try {
        // ObjectMessage om = (ObjectMessage) consumer.receive();
        // af = (AudioFormat) om.getObject();
        // } catch (JMSException e) {
        // e.printStackTrace();
        // break;
        // }
        // }

        af = JMSSendAudio.getAudioFormat();
    }

    void listen() {
        byte[] buffer = new byte[4096];
        int i = 0;

        try {
            msg = (StreamMessage) consumer.receive();
            while (msg != null) {
                i = msg.readBytes(buffer);
                stream(buffer, i);
                msg = (StreamMessage) consumer.receive();
            }
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

    void stream(byte[] buffer, int i) {

        InputStream is = new ByteArrayInputStream(buffer, 0, i);
        AudioInputStream ais = new AudioInputStream(is, af, i);
        playAudioStream(ais);
    }

    public static void playAudioStream(AudioInputStream audioInputStream) {
        // Audio format provides information like sample rate, size, channels.
        AudioFormat audioFormat = audioInputStream.getFormat();
        System.out.println("Play input audio format=" + audioFormat);

        // Convert compressed audio data to uncompressed PCM format.
        if (audioFormat.getEncoding() != AudioFormat.Encoding.PCM_SIGNED) {
            // if ((audioFormat.getEncoding() != AudioFormat.Encoding.PCM) ||
            // (audioFormat.getEncoding() == AudioFormat.Encoding.ALAW) ||
            // (audioFormat.getEncoding() == AudioFormat.Encoding.MP3)) {
            AudioFormat newFormat = new AudioFormat(AudioFormat.Encoding.PCM_SIGNED, audioFormat.getSampleRate(), 16,
                            audioFormat.getChannels(), audioFormat.getChannels() * 2, audioFormat.getSampleRate(),
                            false);
            System.out.println("Converting audio format to " + newFormat);
            AudioInputStream newStream = AudioSystem.getAudioInputStream(newFormat, audioInputStream);
            audioFormat = newFormat;
            audioInputStream = newStream;

        }

        // Open a data line to play our type of sampled audio.
        // Use SourceDataLine for play and TargetDataLine for record.
        DataLine.Info info = new DataLine.Info(SourceDataLine.class, audioFormat);
        if (!AudioSystem.isLineSupported(info)) {
            System.out.println("Play.playAudioStream does not handle this type of audio on this system.");
            return;
        }

        try {
            // Create a SourceDataLine for play back (throws
            // LineUnavailableException).
            SourceDataLine dataLine = (SourceDataLine) AudioSystem.getLine(info);
            // System.out.println("SourceDataLine class=" + dataLine.getClass());

            // The line acquires system resources (throws LineAvailableException).
            dataLine.open(audioFormat);

            // Adjust the volume on the output line.
            if (dataLine.isControlSupported(FloatControl.Type.MASTER_GAIN)) {
                FloatControl volume = (FloatControl) dataLine.getControl(FloatControl.Type.MASTER_GAIN);
                volume.setValue(6F);
            }

            // Allows the line to move data in and out to a port.
            dataLine.start();

            // Create a buffer for moving data from the audio stream to the line.
            int bufferSize = (int) audioFormat.getSampleRate() * audioFormat.getFrameSize();
            byte[] buffer = new byte[bufferSize];

            // Move the data until done or there is an error.
            try {
                int bytesRead = 0;
                while (bytesRead >= 0) {
                    bytesRead = audioInputStream.read(buffer, 0, buffer.length);
                    if (bytesRead >= 0) {
                        // System.out.println("Play.playAudioStream bytes read=" + bytesRead
                        // +
                        // ", frame size=" + audioFormat.getFrameSize() + ", frames read=" +
                        // bytesRead / audioFormat.getFrameSize());
                        // Odd sized sounds throw an exception if we don't write the same
                        // amount.
                        dataLine.write(buffer, 0, bytesRead);
                    }
                } // while
            } catch (IOException e) {
                e.printStackTrace();
            }

            System.out.println("Play.playAudioStream draining line.");
            // Continues data line I/O until its buffer is drained.
            dataLine.drain();

            System.out.println("Play.playAudioStream closing line.");
            // Closes the data line, freeing any resources such as the audio device.
            dataLine.close();
        } catch (LineUnavailableException e) {
            e.printStackTrace();
        }
    }

}
