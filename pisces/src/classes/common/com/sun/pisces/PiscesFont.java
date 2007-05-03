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

import java.io.DataInputStream;
import java.io.EOFException;
import java.io.IOException;
import java.io.InputStream;
import java.util.Hashtable;

class Face {

    PathStore[] paths = new PathStore[256];
    int[] minX = new int[256];
    int[] minY = new int[256];
    int[] width = new int[256];
    int[] height = new int[256];
    double scale;

    public Face(InputStream in) throws IOException {
        GZIPInputStream gin = new GZIPInputStream(in);
	DataInputStream dis = new DataInputStream(gin);
	String name = dis.readUTF();
	String style = dis.readUTF();

        this.scale = 65536.0/dis.readDouble();

	while (true) {
	    char glyph;
	    try {
		glyph = dis.readChar();
	    } catch (EOFException eof) {
		return;
	    }
	    int gx = dis.readInt();
	    int gy = dis.readInt();
	    int gwidth = dis.readInt();
	    int gheight = dis.readInt();
	    int numEntries = dis.readInt();

	    PathStore ps = new PathStore(numEntries);

            int[] x = new int[4];
            int[] y = new int[4];
	    int sx0 = 0, sy0 = 0, xp = 0, yp = 0;

            boolean prevIsQuad = false;
            boolean prevIsCubic = false;
                
	    while (true) {
		char tok = dis.readChar();
		if (tok == 'Z') {
		    ps.close();
                    ps.end();
                    break;
                } else if (tok == 'E') {
		    ps.end();
		    break;
		}

                int x0 = x[0];
                int y0 = y[0];
		
		switch (tok) {
		case 'M':
                    // readInts(x, 1);
                    // readInts(y, 1);
		    x[0] = dis.readInt();
		    y[0] = dis.readInt();
		    break;

		case 'm':
                    // readShorts(x, 1);
                    // readShorts(y, 1);
		    x[0] += dis.readShort();
		    y[0] += dis.readShort();
		    break;

		case 'n':
                    // readBytes(x, 1);
                    // readBytes(y, 1);
		    x[0] += dis.readByte();
		    y[0] += dis.readByte();
		    break;

		case 'H':
                    // readInts(x, 1);
		    x[0] = dis.readInt();
		    break;

		case 'h':
                    // readShorts(x, 1);
		    x[0] += dis.readShort();
		    break;

		case 'i':
                    // readBytes(x, 1);
		    x[0] += dis.readByte();
		    break;

		case 'V':
                    // readInts(y, 1);
		    y[0] = dis.readInt();
		    break;

		case 'v':
                    // readShorts(y, 1);
		    y[0] += dis.readShort();
		    break;

		case 'w':
                    // readBytes(y, 1);
		    y[0] += dis.readByte();
		    break;

		case 'L':
                    // readInts(x, 1);
                    // readInts(y, 1);
		    x[0] = dis.readInt();
		    y[0] = dis.readInt();
		    break;

		case 'l':
                    // readShorts(x, 1);
                    // readShorts(y, 1);
		    x[0] += dis.readShort();
		    y[0] += dis.readShort();
		    break;

		case 'k':
                    // readBytes(x, 1);
                    // readBytes(y, 1);
		    x[0] += dis.readByte();
		    y[0] += dis.readByte();
		    break;

		case 'Q':
                    // readInts(x, 2);
                    // readInts(y, 2);
		    x[0] = dis.readInt();
		    y[0] = dis.readInt();
		    x[1] = dis.readInt();
		    y[1] = dis.readInt();
		    break;

		case 'q':
                    // readShorts(x, 2);
                    // readShorts(y, 2);
		    x[0] = x0 + dis.readShort();
		    y[0] = y0 + dis.readShort();
		    x[1] = x0 + dis.readShort();
		    y[1] = y0 + dis.readShort();
		    break;

		case 'r':
                    // readBytes(x, 2);
                    // readBytes(y, 2);
		    x[0] = x0 + dis.readByte();
		    y[0] = y0 + dis.readByte();
		    x[1] = x0 + dis.readByte();
		    y[1] = y0 + dis.readByte();
		    break;

                case 'T':
                    x[0] = x0 + (prevIsQuad ? (x0 - xp) : 0);
                    y[0] = y0 + (prevIsQuad ? (y0 - yp) : 0);
                    x[1] = dis.readInt();
                    y[1] = dis.readInt();
                    break;

                case 't':
                    x[0] = x0 + (prevIsQuad ? (x0 - xp) : 0);
                    y[0] = y0 + (prevIsQuad ? (y0 - yp) : 0);
                    x[1] = x0 + dis.readShort();
                    y[1] = y0 + dis.readShort();
                    break;

                case 'u':
                    x[0] = x0 + (prevIsQuad ? (x0 - xp) : 0);
                    y[0] = y0 + (prevIsQuad ? (y0 - yp) : 0);
                    x[1] = x0 + dis.readByte();
                    y[1] = y0 + dis.readByte();
                    break;

		case 'C':
                    // readInts(x, 3);
                    // readInts(y, 3);
		    x[0] = dis.readInt();
		    y[0] = dis.readInt();
		    x[1] = dis.readInt();
		    y[1] = dis.readInt();
		    x[2] = dis.readInt();
		    y[2] = dis.readInt();
		    break;

		case 'c':
		    x[0] = x0 + dis.readShort();
		    y[0] = y0 + dis.readShort();
		    x[1] = x0 + dis.readShort();
		    y[1] = y0 + dis.readShort();
		    x[2] = x0 + dis.readShort();
		    y[2] = y0 + dis.readShort();
		    break;

		case 'd':
		    x[0] = x0 + dis.readByte();
		    y[0] = y0 + dis.readByte();
		    x[1] = x0 + dis.readByte();
		    y[1] = y0 + dis.readByte();
		    x[2] = x0 + dis.readByte();
		    y[2] = y0 + dis.readByte();
		    break;

                case 'S':
                    x[0] = x0 + (prevIsCubic ? (x0 - xp) : 0);
                    y[0] = y0 + (prevIsCubic ? (y0 - yp) : 0);
                    x[1] = dis.readInt();
                    y[1] = dis.readInt();
                    x[2] = dis.readInt();
                    y[2] = dis.readInt();
                    break;

                case 's':
                    x[0] = x0 + (prevIsCubic ? (x0 - xp) : 0);
                    y[0] = y0 + (prevIsCubic ? (y0 - yp) : 0);
                    x[1] = x0 + dis.readShort();
                    y[1] = y0 + dis.readShort();
                    x[2] = x0 + dis.readShort();
                    y[2] = y0 + dis.readShort();
                    break;
                    
                case 'p':
                    x[0] = x0 + (prevIsCubic ? (x0 - xp) : 0);
                    y[0] = y0 + (prevIsCubic ? (y0 - yp) : 0);
                    x[1] = x0 + dis.readByte();
                    y[1] = y0 + dis.readByte();
                    x[2] = x0 + dis.readByte();
                    y[2] = y0 + dis.readByte();
                    break;
                }

                switch (tok) {
                case 'M': case 'm': case 'n':
		    ps.moveTo(x[0], y[0]);
                    sx0 = x[0];
                    sy0 = y[0];
                    prevIsQuad = prevIsCubic = false;
                    break;

                case 'H': case 'h': case 'i':
                case 'V': case 'v': case 'w':
                case 'L': case 'l': case 'k':
		    ps.lineTo(x[0], y[0]);
                    prevIsQuad = prevIsCubic = false;
                    break;

                case 'Q': case 'q': case 'r':
                case 'T': case 't': case 'u':
		    ps.quadTo(x[0], y[0], x[1], y[1]);
                    xp = x[0];
                    yp = y[0];
                    x[0] = x[1];
                    y[0] = y[1];
                    prevIsQuad = true;
                    prevIsCubic = false;
                    break;

                case 'C': case 'c': case 'd':
                case 'S': case 's': case 'p':
                    ps.cubicTo(x[0], y[0], x[1], y[1], x[2], y[2]);
                    xp = x[1];
                    yp = y[1];
                    x[0] = x[2];
                    y[0] = y[2];
                    prevIsQuad = false;
                    prevIsCubic = true;
                    break;
                    
		case 'z':
		    ps.close();
		    x[0] = sx0;
		    y[0] = sy0;
                    prevIsQuad = prevIsCubic = false;
                    break;
                }
	    }

	    int idx = glyph;
	    paths[idx] = ps;
	    minX[idx] = gx;
	    minY[idx] = gy;
	    width[idx] = gwidth;
	    height[idx] = gheight;
	}
    }
}

public class PiscesFont {

    public static final int PLAIN = 0;
    public static final int BOLD = 1;
    public static final int ITALIC = 2;

    private static final String[] styles = {
	"PLAIN", "BOLD", "ITALIC", "BOLD+ITALIC"
    };

    private static Hashtable faces = new Hashtable();
    
    String name;
    int style;
    int size;

    Face face;

    // size is S15.16
    public PiscesFont(String name, int style, int size) throws IOException {
	this.face = getFace(name, style);
        this.name = name;
        this.style = style;
        this.size = size;
    }

    private static Face getFace(String name, int style) throws IOException {
        String fname = "/" + name + "_" + styles[style] + ".fnt.gz";
        Face face = (Face)faces.get(fname);
        if (face == null) {
            InputStream in = (PiscesFont.class).getResourceAsStream(fname);
            if (in == null && style != PLAIN) {
                return getFace(name, PLAIN);
            }
            face = new Face(in);
            faces.put(fname, face);
        }
        return face;
    }

    public String getName() {
	return name;
    }

    public int getStyle() {
	return style;
    }

    public int getSize() {
	return size;
    }

    public void getBounds(String s, int[] bounds) {
//         int size = (int)(this.size*face.scale);

        int c = (int)s.charAt(0);
        int minX = face.minX[c];
        int minY = face.minY[c];
        int width = 0;
        int height = 0;
	for (int i = 0; i < s.length(); i++) {
	    c = (int)s.charAt(i);
            width += (int)((long)face.width[c]*size >> 16);
            if (height < face.height[c]) {
                height = face.height[c];
            }
        }

        bounds[0] = minX;
        bounds[1] = minY;
        bounds[2] = width;
        bounds[3] = height;
    }
    
    public void produce(PathSink consumer, String s, int x, int y) {
        int size2 = (int)(this.size*face.scale);

	for (int i = 0; i < s.length(); i++) {
	    int c = (int)s.charAt(i);
	    PathStore glyph = face.paths[c];
	    int width = (int)((long)face.width[c]*size >> 16);

            Transform6 transform = new Transform6(size2, 0,
                                                  0, size2,
                                                  x, y);
	    Transformer pt = new Transformer(consumer, transform);

	    glyph.produce(pt);
	    x += width;
	}
    }
}
