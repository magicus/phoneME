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
 */

package com.sun.pisces;

public class PathStore extends PathSink implements PathSource {

    private static final int DEFAULT_INITIAL_CAPACITY = 1000;

// M - moveTo II 
// m - rel moveTo ss
// n - rel moveTo bb

// H - horiz moveTo I
// h - rel horiz moveTo s
// i - rel horiz moveTo b

// V - vert moveTo I
// v - rel vert moveTo s
// w - rel vert moveTo b

// L - abs lineTo II
// l - rel lineTo ss
// k - rel lineTo bb

// Q - abs quadTo IIII
// q - rel quadTo ssss
// r - rel quadTo bbbb

// T - abs short quadTo II
// t - rel short quadTo ss
// u - rel short quadTo bb

// C - abs cubicTo IIIIII
// c - rel cubicTo ssssss
// d - rel cubicTo bbbbbb

// S - abs short cubicTo IIII
// s - rel short cubicTo ssss 
// p - rel short cubicTo bbbb 

// z - close
// Z - close and end

    static final byte MOVE_TO            = (byte)'M';
    static final byte LINE_JOIN          = (byte)'J';
    static final byte ABS_LINE_TO        = (byte)'L';
    static final byte REL_LINE_TO_SHORT  = (byte)'l';
    static final byte ABS_QUAD_TO        = (byte)'Q';
    static final byte REL_QUAD_TO_SHORT  = (byte)'q';
    static final byte CUBIC_TO           = (byte)'C';
    static final byte CLOSE              = (byte)'z';
    static final byte END                = (byte)'E';

    int numSegments = 0;

    int[] pathData;
    int dindex = 0;

    byte[] pathTypes;
    int tindex = 0;

    int x0, y0, sx0, sy0, xp, yp;

    public PathStore() {
	this(DEFAULT_INITIAL_CAPACITY);
    }

    public PathStore(int initialCapacity) {
	this.pathData = new int[initialCapacity];
	this.pathTypes = new byte[initialCapacity];
    }

    private void ensureCapacity(int elements) {
	if (dindex + elements > pathData.length) {
	    int[] newPathData = new int[pathData.length + 512];
	    System.arraycopy(pathData, 0, newPathData, 0, pathData.length);
	    this.pathData = newPathData;
        }

	if (tindex + 1 > pathTypes.length) {
	    byte[] newPathTypes = new byte[pathTypes.length + 512];
	    System.arraycopy(pathTypes, 0, newPathTypes, 0, pathTypes.length);
	    this.pathTypes = newPathTypes;
	}
    }

    private static boolean isShort(int x) {
	return x >= -32768 && x <= 32767;
    }

    public void moveTo(int x0, int y0) {
	ensureCapacity(2);
	pathTypes[tindex++] = MOVE_TO;
	pathData[dindex++] = x0;
	pathData[dindex++] = y0;
        
        this.sx0 = this.x0 = x0;
        this.sy0 = this.y0 = y0;
    }

    private int packShorts(int hi, int lo) {
        return (hi << 16) | (lo & 0xffff);
    }

    public void lineJoin() {
	ensureCapacity(0);
	pathTypes[tindex++] = LINE_JOIN;
    }

    public void lineTo(int x1, int y1) {
        int dx = x1 - x0;
        int dy = y1 - y0;
        if (isShort(dx) && isShort(dy)) {
            ensureCapacity(1);
            pathTypes[tindex++] = REL_LINE_TO_SHORT;
            pathData[dindex++] = packShorts(dx, dy);
        } else {
            ensureCapacity(2);
            pathTypes[tindex++] = ABS_LINE_TO;
            pathData[dindex++] = x1;
            pathData[dindex++] = y1;
        }

        this.x0 = x1;
        this.y0 = y1;
    }

    public void quadTo(int x1, int y1, int x2, int y2) {
        int dx1 = x1 - x0;
        int dy1 = y1 - y0;
        int dx2 = x2 - x0;
        int dy2 = y2 - y0;

        if (isShort(dx1) && isShort(dy1) && isShort(dx2) && isShort(dy2)) {
            ensureCapacity(2);
            pathTypes[tindex++] = REL_QUAD_TO_SHORT;
            pathData[dindex++] = packShorts(dx1, dy1);
            pathData[dindex++] = packShorts(dx2, dy2);
        } else {
            ensureCapacity(4);
            pathTypes[tindex++] = ABS_QUAD_TO;
            pathData[dindex++] = x1;
            pathData[dindex++] = y1;
            pathData[dindex++] = x2;
            pathData[dindex++] = y2;
        }

        this.xp = x1;
        this.yp = y1;
        this.x0 = x2;
        this.y0 = y2;
    }

    public void cubicTo(int x1, int y1, int x2, int y2, int x3, int y3) {
	ensureCapacity(6);
	pathTypes[tindex++] = CUBIC_TO;
	pathData[dindex++] = x1;
	pathData[dindex++] = y1;
	pathData[dindex++] = x2;
	pathData[dindex++] = y2;
	pathData[dindex++] = x3;
	pathData[dindex++] = y3;

        this.x0 = x3;
        this.y0 = y3;
    }

    public void close() {
	ensureCapacity(0);
	pathTypes[tindex++] = CLOSE;

        this.x0 = sx0;
        this.y0 = sy0;
    }

    public void end() {
	ensureCapacity(0);
	pathTypes[tindex++] = END;

        this.x0 = 0;
        this.y0 = 0;
    }

    public void produce(PathSink consumer) {
	int tidx = 0;
        int didx = 0;
        int x0 = 0, y0 = 0, sx0 = 0, sy0 = 0;

	while (tidx < tindex) {
	    switch (pathTypes[tidx++]) {
	    case MOVE_TO:
                sx0 = x0 = pathData[didx++];
                sy0 = y0 = pathData[didx++];
                consumer.moveTo(x0, y0);
		break;

            case LINE_JOIN:
                consumer.lineJoin();
                break;

	    case ABS_LINE_TO:
                consumer.lineTo(x0 = pathData[didx++], y0 = pathData[didx++]);
		break;

	    case REL_LINE_TO_SHORT:
                int dxdy = pathData[didx++];
                x0 += dxdy >> 16;
                y0 += (dxdy << 16) >> 16;
		consumer.lineTo(x0, y0);
		break;

	    case ABS_QUAD_TO:
		consumer.quadTo(pathData[didx++], pathData[didx++],
				x0 = pathData[didx++], y0 = pathData[didx++]);
                break;

	    case REL_QUAD_TO_SHORT:
                int dxdy1 = pathData[didx++];
                int dxdy2 = pathData[didx++];
                int x1 = x0 + (dxdy1 >> 16);
                int y1 = y0 + ((dxdy1 << 16) >> 16);
                int x2 = x0 + (dxdy2 >> 16);
                int y2 = y0 + ((dxdy2 << 16) >> 16);
		consumer.quadTo(x1, y1, x2, y2);
                x0 = x2;
                y0 = y2;
		break;

	    case CUBIC_TO:
		consumer.cubicTo(pathData[didx++], pathData[didx++],
				 pathData[didx++], pathData[didx++],
				 x0 = pathData[didx++], y0 = pathData[didx++]);
		break;

	    case CLOSE:
		consumer.close();
                x0 = sx0;
                y0 = sy0;
		break;

	    case END:
		consumer.end();
                x0 = 0;
                y0 = 0;
		break;
	    }
	}
    }
}

