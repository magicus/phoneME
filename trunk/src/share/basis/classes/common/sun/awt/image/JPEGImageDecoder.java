/*
 * @(#)JPEGImageDecoder.java	1.22 06/10/10
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
 *
 */
/*-
 *	Reads JPEG images from an InputStream and reports the
 *	image data to an InputStreamImageSource object.
 *
 * The native implementation of the JPEG image decoder was adapted from
 * release 6 of the free JPEG software from the Independent JPEG Group.
 */
package sun.awt.image;

import java.util.Vector;
import java.util.Hashtable;
import java.io.InputStream;
import java.io.IOException;
import java.awt.image.*;

/**
 * JPEG Image converter
 *
 * @version 1.15 08/19/02
 * @author Jim Graham
 */
public class JPEGImageDecoder extends ImageDecoder {
    private static ColorModel RGBcolormodel;
    private static ColorModel Graycolormodel;
    private static final Class InputStreamClass = InputStream.class;
    private static native void initIDs(Class InputStreamClass);
    static {
        java.security.AccessController.doPrivileged(
            new sun.security.action.LoadLibraryAction("awtjpeg"));
        initIDs(InputStreamClass);
        RGBcolormodel = new DirectColorModel(24, 0xff0000, 0xff00, 0xff);
        byte g[] = new byte[256];
        for (int i = 0; i < 256; i++) {
            g[i] = (byte) i;
        }
        Graycolormodel = new IndexColorModel(8, 256, g, g, g);
    }

    private native void readImage(InputStream is, byte buf[])
	throws ImageFormatException, IOException;

    Hashtable props = new Hashtable();

    public JPEGImageDecoder(InputStreamImageSource src, InputStream is) {
	super(src, is);
    }

    /**
     * An error has occurred. Throw an exception.
     */
    private static void error(String s1) throws ImageFormatException {
	throw new ImageFormatException(s1);
    }

    public boolean sendHeaderInfo(int width, int height,
        boolean gray, boolean multipass) {
        setDimensions(width, height);
        setProperties(props);
        ColorModel colormodel = gray ? Graycolormodel : RGBcolormodel;
        setColorModel(colormodel);
        int flags = hintflags;
        if (!multipass) {
            flags |= ImageConsumer.SINGLEPASS;
        }
        setHints(flags);
        headerComplete();
        return true;
    }

    public boolean sendPixels(int pixels[], int y) {
        int count = setPixels(0, y, pixels.length, 1, RGBcolormodel,
                pixels, 0, pixels.length);
        if (count <= 0) {
            aborted = true;
        }
        return !aborted;
    }

    public boolean sendPixels(byte pixels[], int y) {
        int count = setPixels(0, y, pixels.length, 1, Graycolormodel,
                pixels, 0, pixels.length);
        if (count <= 0) {
            aborted = true;
        }
        return !aborted;
    }

    /**
     * produce an image from the stream.
     */
    public void produceImage() throws IOException, ImageFormatException {
	try {
	    readImage(input, new byte[1024]);
	    if (!aborted) {
		imageComplete(ImageConsumer.STATICIMAGEDONE, true);
	    }
	} catch (IOException e) {
	    if (!aborted) {
		throw e;
	    }
	} finally {
	    close();
	}
    }

    /**
     * The ImageConsumer hints flag for a JPEG image.
     */
    private static final int hintflags =
	ImageConsumer.TOPDOWNLEFTRIGHT | ImageConsumer.COMPLETESCANLINES |
	ImageConsumer.SINGLEFRAME;
}
