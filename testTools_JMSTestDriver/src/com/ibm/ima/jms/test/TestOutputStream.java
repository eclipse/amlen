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
package com.ibm.ima.jms.test;

import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.ObjectOutput;
import java.io.ObjectOutputStream;
import java.io.OutputStream;

class TestOutputStream implements DataOutput, ObjectOutput {
    private final OutputStream _stream;
    
    TestOutputStream(OutputStream stream){
        _stream = stream; 
    }
    //@Override
     public void writeObject(Object obj) throws IOException {
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeObject(obj);
            return;
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void flush() throws IOException {
        _stream.flush();
    }

    //@Override
    public void close() throws IOException {
        _stream.close();
    }

    //@Override
    public void write(int b) throws IOException {
        _stream.write(b);
    }

    //@Override
    public void write(byte[] b) throws IOException {
        _stream.write(b);
    }

    //@Override
    public void write(byte[] b, int off, int len) throws IOException {
        _stream.write(b,off,len);
    }

    //@Override
    public void writeBoolean(boolean v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeBoolean(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeBoolean(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeByte(int v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeByte(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeByte(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeShort(int v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeShort(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeShort(v);
            return;
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeChar(int v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeChar(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeChar(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeInt(int v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeInt(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeInt(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeLong(long v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeLong(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeLong(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeFloat(float v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeFloat(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeFloat(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeDouble(double v) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeDouble(v);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeDouble(v);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeBytes(String s) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeBytes(s);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeBytes(s);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeChars(String s) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeChars(s);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeChars(s);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void writeUTF(String s) throws IOException {
        if (_stream instanceof DataOutputStream) {
            DataOutputStream st = (DataOutputStream) _stream;
            st.writeUTF(s);
            return;
        }
        if (_stream instanceof ObjectOutputStream) {
            ObjectOutputStream oos = (ObjectOutputStream) _stream;
            oos.writeUTF(s);
            return;
        }
        throw new IOException("Invalid stream type.");
    }

}
