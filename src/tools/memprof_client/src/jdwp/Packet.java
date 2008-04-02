/*
 *   
 *
 * Copyright  1990-2008 Sun Microsystems, Inc. All Rights Reserved.
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
 */


package com.sun.cldchi.tools.memoryprofiler.jdwp;

import java.util.Vector;

/**
 * This class encapsulates JDWP packet. It based on
 * <code>ByteBuffer</code> but adds methods for working with data
 * specific for JDWP packet. This class is not used directly, its subclasses
 * <code>Command</code> and <code>Reply</code> are used instead. For
 * information about JDWP packet see JDWP specification.
 *
 * @see jdwp.ByteBuffer
 * @see jdwp.Command
 * @see jdwp.Reply
 */
class Packet extends ByteBuffer {

    /**
     * JDWP packet flag "no flags". Indicates that the packet is a command.
     */
    public final static int flNoFlags 			= 0x0;
    
    /**
     * JDWP packet flag <code>Reply</code>. Indicates that the packet is a 
     * reply.
     */
    public final static int flReply 				= 0x80;
    
    /**
     * Offset of the "packet length" field in the JDWP packet.
     */
    public final static int LengthOffset		= 0;
    
    /**
     * Offset of the "packet ID" field in the JDWP packet.
     */
    public final static int IdOffset			= 4;
    
    /**
     * Offset of the "flags" field in the JDWP packet.
     */
    public final static int FlagsOffset			= 8;
    
    /**
     * Offset of the "command number" field in the JDWP packet.
     */
    public final static int CommandOffset		= 9;
    
    /**
     * Offset of the "error code" field in the JDWP packet.
     */
    public final static int ErrorCodeOffset		= 9;	
    
    /**
     * Size of JDWP packet's header.
     */
    public final static int PacketHeaderSize	= 11;

    /**
     * Create a new JDWP packet fills its header by zeros.
     */
    public Packet() {
	super();
	while (length() < PacketHeaderSize){
            addByte(0);
        }
    }

    /**
     * Sets "packet length" field of the JDWP packet
     */
    public void setLength() {
        try {
            putInt(LengthOffset, length());
        }
        catch (BoundException e) {};
    }

    /**
     * Returns ID of the JDWP packet.
     *
     * @return ID of the JDWP packet
     */
    public int getID() {
        int id = 0;

        try {
            id = getInt(IdOffset);
        }
        catch (BoundException e) {};
		return id;
    }
    
    /**
     * Sets ID of the JDWP packet.
     *
     * @param Id ID of the JDWP packet
     */
    public void setID(int Id) {
        try {
            putInt(IdOffset, Id);
        }
        catch (BoundException e) {};
    }

    /**
     * Returns flags of the JDWP packet.
     *
     * @return flags of the JDWP packet.
     */
    public int getFlags() {
        return bytes[FlagsOffset] & 0xff;
    }
    
    /**
     * Sets flags of the JDWP packet.
     *
     * @param Flags flags to be set
     */
    public void setFlags(int Flags) {
	bytes[FlagsOffset] = (byte) (Flags & 0xFF);
    }

    /**
     * Moves the reading marker to the beginning of packet data
     * (after the header). To learn about reading marker see
     * <code>ByteBuffer</code>.
     *
     * @see jdwp.ByteBuffer
     */
    public void resetDataParser() {
	resetParser(PacketHeaderSize);
    }
    
    /**
     * Returns size of packet's data (i.e., size of the packet
     * excluding header). 
     *
     * @return size of packet's data
     */
    public int getDataSize() {
	return length() - PacketHeaderSize;
    }

    /**
     * Adds a field ID to the end of JDWP packet.
     *
     * @param b ID to be added
     */
    public void addFieldID(long b) {
	addID(b, jdwp.fieldIDSize);
    }

    /**
     * Adds a method ID to the end of JDWP packet.
     *
     * @param b ID to be added
     */
    public void addMethodID(long b) {
	addID(b, jdwp.methodIDSize);
    }

    /**
     * Adds an object ID to the end of JDWP packet.
     *
     * @param b ID to be added
     */
    public void addObjectID(long b) {
	addID(b, jdwp.objectIDSize);
    }

    /**
     * Adds a reference type ID to the end of JDWP packet.
     *
     * @param b ID to be added
     */
    public void addReferenceTypeID(long b) {
	addID(b, jdwp.referenceTypeIDSize);
    }

    /**
     * Adds a frame ID to the end of JDWP packet.
     *
     * @param b ID to be added
     */
    public void addFrameID(long b) {
	addID(b, jdwp.frameIDSize);
    }
    
    /**
     * Tries to read next field ID from the buffer.
     * Value is read is one
     * that is pointed by reading marker. After completing the operation
     * the reading marker is incremented. 
     *     
     * @return current field ID from the buffer
     * @throws BoundException if value to be read is outside the filled area
     */
    public long getFieldID() throws BoundException {
	return getID(jdwp.fieldIDSize);
    }

    /**
     * Tries to read next method ID from the buffer.
     * Value is read is one
     * that is pointed by reading marker. After completing the operation
     * the reading marker is incremented. 
     *     
     * @return current method ID from the buffer
     * @throws BoundException if value to be read is outside the filled area
     */
    public long getMethodID() throws BoundException {
	return getID(jdwp.methodIDSize);
    }

    /**
     * Tries to read next object ID from the buffer.
     * Value is read is one
     * that is pointed by reading marker. After completing the operation
     * the reading marker is incremented. 
     *     
     * @return current object ID from the buffer
     * @throws BoundException if value to be read is outside the filled area
     */
    public long getObjectID() throws BoundException {
	return getID(jdwp.objectIDSize);
    }

    /**
     * Tries to read next reference type ID from the buffer.
     * Value is read is one
     * that is pointed by reading marker. After completing the operation
     * the reading marker is incremented. 
     *     
     * @return current reference type ID from the buffer
     * @throws BoundException if value to be read is outside the filled area
     */
    public long getReferenceTypeID() throws BoundException {
	return getID(jdwp.referenceTypeIDSize);
    }

    /**
     * Tries to read next frame ID from the buffer.
     * Value is read is one
     * that is pointed by reading marker. After completing the operation
     * the reading marker is incremented. 
     *     
     * @return current frame ID from the buffer
     * @throws BoundException if value to be read is outside the filled area
     */
    public long getFrameID() throws BoundException {
	return getID(jdwp.frameIDSize);
    }
    
    /**
     * Parses the JDWP packet according to the specified mask. The mask
     * specifies what elements are contained in JDWP packet's data. The rules
     * are as follows:
     * <ul>
     * <li> b - a byte value
     * <li> i - an int value
     * <li> S - a short value
     * <li> l - a long value
     * <li> s - a string value
     * <li> f - a field ID
     * <li> m - a method ID
     * <li> o - an object ID
     * <li> r - a reference type ID
     * <li> F - a frame ID
     * <li> v - a value. The first byte indicates type tag of the
     * variable, the second is a variable's value.
     * <li> . - a set of data in the end of packet that should not be parsed.
     * <li> i(&lt;submask&gt;) - the first integer indicates how many times
     * submask is appeared in the packet.
     * </ul>
     * For example, the mask <code>li(rm)</code> means that the first element
     * of packet's data is a <code>long</code> value, then an <code>int</code>
     * value that indicates how many times the pair "reference type ID - 
     * method ID" is appeared later.
     *
     * @param how a mask that indicates how to parse a packet.
     * @return a vector of parsed elements of the packet's data. The classes
     * that represent different types of elements are written below:
     * <ul>
     * <li> b - <code>java.lang.Integer</code>
     * <li> i - <code>java.lang.Integer</code>
     * <li> S - <code>java.lang.Integer</code>
     * <li> l - <code>java.lang.Long</code>
     * <li> s - <code>java.lang.String</code>
     * <li> f - <code>java.lang.Long</code>
     * <li> m - <code>java.lang.Long</code>
     * <li> o - <code>java.lang.Long</code>
     * <li> r - <code>java.lang.Long</code>
     * <li> F - <code>java.lang.Long</code>
     * <li> v - The tag of the value is represented by
     * <code>java.lang.Integer</code>. The value itself is represented
     * according to this table
     * </ul>
     */
    public Vector parse(String how) throws BoundException {

        boolean check;
	check = (how.indexOf('.') == -1);

        if (! check) {
		if (how.indexOf('.') != how.length() - 1)
                   	throw new BoundException();
		how = Tools.Left(how, how.length() - 1);
        }

	Vector v = new Vector();
	resetDataParser();
        doParse(how, v);

        if (check && (! isParsed()))
        	throw new BoundException();
		return v;
    }

    /**
     * Performs the parsing of the JDWP oacket in fact. The all the
     * parsing rules that described in the description of
     * <code>parse</code> method are the same besides of absence of
     * '.' symbol in the mask.
     *
     * @param how a mask that describes types of elements
     * @param v a vector for keeping results
     *
     * @see #parse(java.lang.String)
     */
    private void doParse(String how, Vector v) throws BoundException {

	int index = 0;
	char[] h = how.toCharArray();

	while (index < h.length) {
            switch (h[index]) {
                case 'b':
                    v.add(new Integer(getByte()));
                    break;
		case 'i':
                    v.add(new Integer(getInt()));
                    break;
		case 'S':
                    v.add(new Integer(getShort()));
                    break;
		case 'l':
                    v.add(new Long(getLong()));
                    break;
		case 's':
                    v.add(getString());
                    break;
		case 'f':
                    v.add(new Long(getFieldID()));
                    break;
		case 'm':
                    v.add(new Long(getMethodID()));
                    break;
                case 'o':
                    v.add(new Long(getObjectID()));
                    break;
		case 'r':
                    v.add(new Long(getReferenceTypeID()));
                    break;
		case 'F':
                    v.add(new Long(getFrameID()));
                    break;
                case 'v':
                    int vtype = getByte();
                    v.add(new Integer(vtype));
                    switch (vtype) {
			case jdwp.tagARRAY:
			case jdwp.tagOBJECT:
                            v.add(new Long(getObjectID()));
                            break;
			case jdwp.tagBYTE:
                            v.add(new Integer(getByte()));
                            break;
			case jdwp.tagINT:
                            v.add(new Integer(getInt()));
                            break;
			case jdwp.tagSHORT:
                            v.add(new Integer(getShort()));
                            break;
			case jdwp.tagVOID:
                            v.add("void value");
                            break;
			case jdwp.tagBOOLEAN:
                            v.add(new Integer(getByte()));
                            break;
			case jdwp.tagLONG:
                            v.add(new Long(getLong()));
                            break;
			default:
                            throw new BoundException();
                    }
                    break;

                case '(':
                    if (index == 0)
                        throw new BoundException();
                    
                    if (h[index - 1] != 'i')
                        throw new BoundException();

                    int n = ((Integer) v.elementAt(v.size() - 1)).intValue();

                    if (n < 0)
			throw new BoundException();
                    int pos = index + 1;
                    int cnt = 1;
                    int last = -1;                 
                    while (pos < h.length) {
			if (h[pos] == '(')
                            cnt++;

			if (h[pos] == ')') {
                            cnt--;
                            if (cnt == 0) {
				last = pos;
				break;
                            }
			}
			pos++;
                    }

                    if (last == -1)
			throw new BoundException();

                    String s = new String(h, index + 1, last - index - 1);
                    for (int i = 0; i < n; i++)
			doParse(s, v);
                    index = last;
                    break;

		default:
                    throw new BoundException();
            }
            index++;
	}
    }
}
