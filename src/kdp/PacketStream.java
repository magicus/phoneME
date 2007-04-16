/*
 *
 * Copyright  1990-2007 Sun Microsystems, Inc. All Rights Reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 only, as published by the Free Software Foundation.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 2 for more details (a copy is
 * included at /legal/license.txt).
 * 
 * You should have received a copy of the GNU General Public License
 * version 2 along with this work; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 * 
 * Please contact Sun Microsystems, Inc., 4150 Network Circle, Santa
 * Clara, CA 95054 or visit www.sun.com if you need additional
 * information or have any questions.
 *
 *
 */

package kdp;
import java.util.*;
import java.io.*;

class PacketStream {
    final ProxyListener proxy;
    private int outCursor = 0;
    final Packet pkt;
    private ByteArrayOutputStream dataStream = new ByteArrayOutputStream();
    private boolean isCommitted = false;
    private int id;
    byte[] data;

    PacketStream(ProxyListener proxy, byte cmdSet, byte cmd) {
        this.proxy = proxy;
        this.pkt = new Packet();
        id = pkt.id;
        pkt.cmdSet = cmdSet;
        pkt.cmd = cmd;
    }

    PacketStream(ProxyListener proxy, int id, byte flags, short errorCode ) {
        this.pkt = new Packet();
        this.proxy = proxy;
        this.pkt.id = id;
        this.id = id;
        this.pkt.errorCode = errorCode;
        this.pkt.flags = flags;
    }

    PacketStream(ProxyListener proxy, Packet p) {
        this.pkt = p;
        this.proxy = proxy;
        this.id = p.id;
        this.data = p.data;
    }

    int id() {
        return id;
    }

    void send() throws ProxyConnectionException {
        if (!isCommitted) {
            pkt.data = dataStream.toByteArray();
            proxy.send(pkt);
            isCommitted = true;
        }
    }

    void waitForReply() throws PacketStreamException {
        if (!isCommitted) {
            throw new ProxyConnectionException("waitForReply without send");
        }
        proxy.waitForReply(pkt);

        if (pkt.errorCode != Packet.ReplyNoError) {
            throw new PacketStreamException(String.valueOf(pkt.errorCode));
        }
        data = pkt.data;
    }

    void writeBoolean(boolean data) {
        if(data) {
            dataStream.write( 1 );
        } else {
            dataStream.write( 0 );
        }
    }

    void writeByte(byte data) {
        dataStream.write( data );
    }

    void writeChar(char data) {
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeShort(short data) {
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeInt(int data) {
        dataStream.write( (byte)((data >>> 24) & 0xFF) );
        dataStream.write( (byte)((data >>> 16) & 0xFF) );
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeLong(long data) {
        dataStream.write( (byte)((data >>> 56) & 0xFF) );
        dataStream.write( (byte)((data >>> 48) & 0xFF) );
        dataStream.write( (byte)((data >>> 40) & 0xFF) );
        dataStream.write( (byte)((data >>> 32) & 0xFF) );

        dataStream.write( (byte)((data >>> 24) & 0xFF) );
        dataStream.write( (byte)((data >>> 16) & 0xFF) );
        dataStream.write( (byte)((data >>> 8) & 0xFF) );
        dataStream.write( (byte)((data >>> 0) & 0xFF) );
    }

    void writeFloat(float data) {
        writeInt(Float.floatToIntBits(data));
    }

    void writeDouble(double data) {
        writeLong(Double.doubleToLongBits(data));
    }

    void writeID(int size, long data) {
        if (size == 8) {
            writeLong(data);
        } else {
            writeInt((int)data);
        }
    }

    void writeByteArray(byte[] data) {
        dataStream.write(data, 0, data.length);
    }

    void writeString(String string) {
        try {
            byte[] stringBytes = string.getBytes("UTF8");
            writeInt(stringBytes.length);
            writeByteArray(stringBytes);
        } catch (java.io.UnsupportedEncodingException e) {
            throw new RuntimeException("Cannot convert string to UTF8 bytes");
        }
    }

    /**
     * Read byte represented as one bytes.
     */
    byte readByte() throws PacketStreamException {
        checkCursor(1);
        return (data[outCursor++]);
    }

    /**
     * Read boolean represented as one byte.
     */
    boolean readBoolean() throws PacketStreamException {
        byte ret = readByte();
        return (ret != 0);
    }

    /**
     * Read char represented as two bytes.
     */
    char readChar() throws PacketStreamException {
        int b1, b2;

        checkCursor(2);
        b1 = data[outCursor++] & 0xff;
        b2 = data[outCursor++] & 0xff;

        return (char)((b1 << 8) + b2);
    }

    /**
     * Read short represented as two bytes.
     */
    short readShort() throws PacketStreamException {
        return ((short)readChar());
    }

    /**
     * Read int represented as four bytes.
     */
    int readInt() throws PacketStreamException {
        checkCursor(4);
        return (((int)(data[outCursor++] & 0xff) << 24) +
                ((int)(data[outCursor++] & 0xff) << 16) +
                ((int)(data[outCursor++] & 0xff) <<  8) +
                ((int)(data[outCursor++] & 0xff)));
    }

    /**
     * Read long represented as eight bytes.
     */
    long readLong() throws PacketStreamException {
        return (((long)(readInt()) << 32) + (long)readInt());
    }

    /**
     * Read double represented as eight bytes.
     */
    double readDouble() throws PacketStreamException {
        return Double.longBitsToDouble(readLong());
    }

    /**
     * Read string represented as four byte length followed by
     * characters of the string.
     */
    String readString() throws PacketStreamException {
        String ret;
        int len = readInt();

        checkCursor(len);

        try {
            ret = new String(data, outCursor, len, "UTF8");
        } catch(java.io.UnsupportedEncodingException e) {
            throw new PacketStreamException();
        }
        outCursor += len;
        return ret;
    }

    private long readID(int size) throws PacketStreamException {
        if (size == 8) {
            return readLong();
        } else {  // Other sizes???
            return readInt();
        }
    }

    int skipBytes(int n) throws PacketStreamException {
        checkCursor(n);
        outCursor += n;
        return n;
    }

    byte cmd() {
        return (byte)pkt.cmd;
    }

    void checkCursor(int size) throws PacketStreamException {
        if ((outCursor + size) > data.length) {
            throw new PacketStreamException("Reading past buffer");
        }
    }
}

