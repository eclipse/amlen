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

import java.io.DataInput;
import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.io.ObjectInput;
import java.io.ObjectInputStream;

class TestInputStream implements DataInput, ObjectInput {
    private final InputStream _stream;
    
    TestInputStream(InputStream stream) {
        _stream = stream;
    }
    //@Override
    public Object readObject() throws IOException {
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            try {
                return ois.readObject();
            } catch (ClassNotFoundException e) {
                IOException ioe = new IOException("Failed to read object");
                ioe.initCause(e);
                throw ioe;
            }            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public int read() throws IOException {
        return _stream.read();
    }

    //@Override
    public int read(byte[] b) throws IOException {
        return _stream.read(b);
    }

    //@Override
    public int read(byte[] b, int off, int len) throws IOException {
        int i = _stream.read(b, off, len);
        if(i != len)
            throw new EOFException();
        return i;
    }

    //@Override
    public long skip(long n) throws IOException {
        return _stream.skip(n);
    }

    //@Override
    public int available() throws IOException {
        return _stream.available();
    }

    //@Override
    public void close() throws IOException {
        _stream.close();
    }

    //@Override
    public void readFully(byte[] b) throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            st.readFully(b);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public void readFully(byte[] b, int off, int len) throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            st.readFully(b,off,len);            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public int skipBytes(int n) throws IOException {
        return (int) _stream.skip(n);
    }

    //@Override
    public boolean readBoolean() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readBoolean();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readBoolean();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public byte readByte() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readByte();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readByte();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public int readUnsignedByte() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readUnsignedByte();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readUnsignedByte();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public short readShort() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readShort();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readShort();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public int readUnsignedShort() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readUnsignedShort();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readUnsignedShort();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public char readChar() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readChar();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readChar();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public int readInt() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readInt();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readInt();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public long readLong() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readLong();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readLong();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public float readFloat() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readFloat();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readFloat();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public double readDouble() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readDouble();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readDouble();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    @Deprecated
    public String readLine() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readLine();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readLine();            
        }
        throw new IOException("Invalid stream type.");
    }

    //@Override
    public String readUTF() throws IOException {
        if (_stream instanceof DataInputStream) {
            DataInputStream st = (DataInputStream) _stream;
            return st.readUTF();            
        }
        if (_stream instanceof ObjectInputStream) {
            ObjectInputStream ois = (ObjectInputStream) _stream;
            return ois.readUTF();            
        }
        throw new IOException("Invalid stream type.");
    }

}
