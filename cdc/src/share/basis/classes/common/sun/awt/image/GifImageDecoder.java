/*
 * @(#)GifImageDecoder.java	1.55 06/10/24
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

/*-
 *	Reads GIF images from an InputStream and reports the
 *	image data to an InputStreamImageSource object.
 */
package sun.awt.image;

import java.util.Vector;
import java.util.Hashtable;
import java.io.InputStream;
import java.io.IOException;
import java.awt.image.*;

/**
 * Gif Image converter
 *
 * @version 1.41 01/05/01
 * @author Arthur van Hoff
 * @author Jim Graham
 */
public class GifImageDecoder extends ImageDecoder {
    private static final boolean verbose = false;

    private static final int IMAGESEP 		= 0x2c;
    private static final int EXBLOCK 		= 0x21;
    private static final int EX_GRAPHICS_CONTROL= 0xf9;
    private static final int EX_COMMENT 	= 0xfe;
    private static final int EX_APPLICATION 	= 0xff;
    private static final int TERMINATOR 	= 0x3b;
    private static final int TRANSPARENCYMASK 	= 0x01;
    private static final int INTERLACEMASK 	= 0x40;
    private static final int COLORMAPMASK 	= 0x80;

    int num_global_colors;
    byte[] global_colormap;
    int trans_pixel = -1;
    IndexColorModel global_model;

    Hashtable props = new Hashtable();

    byte[] saved_image;
    IndexColorModel saved_model;

    int global_width;
    int global_height;
    int global_bgpixel;

    GifFrame curframe;

    public GifImageDecoder(InputStreamImageSource src, InputStream is) {
	super(src, is);
    }

    /**
     * An error has occurred. Throw an exception.
     */
    private static void error(String s1) throws ImageFormatException {
	throw new ImageFormatException(s1);
    }

    /**
     * Read a number of bytes into a buffer.
     * @return number of bytes that were not read due to EOF or error
     */
    private int readBytes(byte buf[], int off, int len) {
	while (len > 0) {
	    try {
		int n = input.read(buf, off, len);
		if (n < 0) {
		    break;
		}
		off += n;
		len -= n;
	    } catch (IOException e) {
		break;
	    }
	}
	return len;
    }

    private static final int ExtractByte(byte buf[], int off) {
	return (buf[off] & 0xFF);
    }

    private static final int ExtractWord(byte buf[], int off) {
	return (buf[off] & 0xFF) | ((buf[off + 1] & 0xFF) << 8);
    }

    /**
     * produce an image from the stream.
     */
    public void produceImage() throws IOException, ImageFormatException {
	try {
	    readHeader();

	    int totalframes = 0;
	    int frameno = 0;
	    int nloops = -1;
	    int disposal_method = 0;
	    int delay = -1;
	    boolean loopsRead = false;
	    boolean isAnimation = false;

	    while (!aborted) {
		int code;

		if (!ImageProductionMonitor.allowsProduction()) {
		    abort();
		    break;
		}

		switch (code = input.read()) {
		  case EXBLOCK:
		    switch (code = input.read()) {
		      case EX_GRAPHICS_CONTROL: {
			byte buf[] = new byte[6];
			if (readBytes(buf, 0, 6) != 0) {
			    return;//error("corrupt GIF file");
			}
			if ((buf[0] != 4) || (buf[5] != 0)) {
			    return;//error("corrupt GIF file (GCE size)");
			}
			// Get the index of the transparent color
			delay = ExtractWord(buf, 2) * 10;
			if (delay > 0 && !isAnimation) {
			    isAnimation = true;
			    ImageFetcher.startingAnimation();
			}
			disposal_method = (buf[1] >> 2) & 7;
			if ((buf[1] & TRANSPARENCYMASK) != 0) {
			    trans_pixel = ExtractByte(buf, 4);
			} else {
			    trans_pixel = -1;
			}
			break;
		      }

		      case EX_COMMENT:
		      case EX_APPLICATION:
		      default:
			boolean loop_tag = false;
			String comment = "";
			while (true) {
			    int n = input.read();
			    if (n <= 0) {
				break;
			    }
			    byte buf[] = new byte[n];
			    if (readBytes(buf, 0, n) != 0) {
				return;//error("corrupt GIF file");
			    }
			    if (code == EX_COMMENT) {
				comment += new String(buf);
			    } else if (code == EX_APPLICATION) {
				if (loop_tag) {
				    if (n == 3 && buf[0] == 1) {
				        if (loopsRead) {
					    ExtractWord(buf, 1);
					}
					else {
					    nloops = ExtractWord(buf, 1);
					    loopsRead = true;
					}
				    } else {
					loop_tag = false;
				    }
				}
				if ("NETSCAPE2.0".equals(new String(buf))) {
				    loop_tag = true;
				}
			    }
			}
			if (code == EX_COMMENT) {
			    props.put("comment", comment);
			}
			if (loop_tag && !isAnimation) {
			    isAnimation = true;
			    ImageFetcher.startingAnimation();
			}
			break;

		      case -1:
			return; //error("corrupt GIF file");
		    }
		    break;

		  case IMAGESEP:
		    if (!isAnimation) {
		        input.mark(0); // we don't need the mark buffer
		    }
		    try {
			if (!readImage(totalframes == 0,
				       disposal_method,
				       delay)) {
			    return;
			}
		    } catch (Exception e) {
			if (verbose) {
			    e.printStackTrace();
			}
			return;
		    }
		    frameno++;
		    totalframes++;
		    break;

		  default:
		  case -1:
		    if (verbose) {
			if (code == -1) {
			    System.err.println("Premature EOF in GIF file," +
					       " frame " + frameno);
			} else {
			    System.err.println("corrupt GIF file (parse) ["
					       + code + "].");
			}
		    }
		    if (frameno == 0) {
			return;
		    }
		    // NOBREAK

		  case TERMINATOR:
		    if (nloops == 0 || nloops-- >= 0) {
			try {
			    if (curframe != null) {
				curframe.dispose();
				curframe = null;
			    }
			    input.reset();
			    saved_image = null;
			    saved_model = null;
			    frameno = 0;
			    break;
			} catch (IOException e) {
			    return; // Unable to reset input buffer
			}
		    }
		    if (verbose && frameno != 1) {
			System.out.println("processing GIF terminator,"
					   + " frames: " + frameno
					   + " total: " + totalframes);
		    }
		    imageComplete(ImageConsumer.STATICIMAGEDONE, true);
		    return;
		}
	    }
	} finally {
	    close();
	}
    }

    /**
     * Read Image header
     */
    private void readHeader() throws IOException, ImageFormatException {
	// Create a buffer
	byte buf[] = new byte[13];

	// Read the header
	if (readBytes(buf, 0, 13) != 0) {
	    throw new IOException();
	}

	// Check header
	if ((buf[0] != 'G') || (buf[1] != 'I') || (buf[2] != 'F')) {
	    error("not a GIF file.");
	}

	// Global width&height
	global_width = ExtractWord(buf, 6);
	global_height = ExtractWord(buf, 8);

	// colormap info
	int ch = ExtractByte(buf, 10);
	if ((ch & COLORMAPMASK) == 0) {
	    error("no global colormap in GIF file.");
	}
	num_global_colors = 1 << ((ch & 0x7) + 1);

	global_bgpixel = ExtractByte(buf, 11);

	if (buf[12] != 0) {
	    props.put("aspectratio", ""+((ExtractByte(buf, 12) + 15) / 64.0));
	}

	// Read colors
	global_colormap = new byte[num_global_colors * 3];
	if (readBytes(global_colormap, 0, num_global_colors * 3) != 0) {
	    throw new IOException();
	}

        input.mark(1000000); // set this mark in case this is an animated GIF
    }

    /**
     * The ImageConsumer hints flag for a non-interlaced GIF image.
     */
    private static final int normalflags =
	ImageConsumer.TOPDOWNLEFTRIGHT | ImageConsumer.COMPLETESCANLINES |
	ImageConsumer.SINGLEPASS | ImageConsumer.SINGLEFRAME;

    /**
     * The ImageConsumer hints flag for an interlaced GIF image.
     */
    private static final int interlaceflags =
	ImageConsumer.RANDOMPIXELORDER | ImageConsumer.COMPLETESCANLINES |
	ImageConsumer.SINGLEPASS | ImageConsumer.SINGLEFRAME;

    private short prefix[]  = new short[4096];
    private byte  suffix[]  = new byte[4096];
    private byte  outCode[] = new byte[4097];

    //private native boolean parseImage(int x, int y, int width, int height,
    //				      boolean interlace, int initCodeSize,
    //				      byte block[], byte rasline[],
    //				      IndexColorModel model);

    /* 
     * This is converted from the native version. The java version is 
     * much faster when we have JIT.
     */
    private boolean parseImage(int relx, int rely, int width, int height,
                               boolean interlace, int initCodeSize,
                               byte block[], byte rasline[],
                               IndexColorModel model) {
        int OUTCODELENGTH = 4097;

        int clearCode = (1 << initCodeSize);
        int eofCode = clearCode + 1;
        int bitMask;
        int curCode;
        int outCount = OUTCODELENGTH;

        /* Variables used to form reading data */
        boolean blockEnd = false;
        int remain = 0;
        int byteoff = 0;
        int accumbits = 0;
        int accumdata = 0;

        /* Variables used to decompress the data */
        int codeSize = initCodeSize + 1;
        int maxCode = 1 << codeSize;
        int codeMask = maxCode - 1;
        int freeCode = clearCode + 2;
        int code = 0;
        int oldCode = 0;
        char prevChar = 0;

        //int blockLength = 0;
        int blockLength = 0;

        /* Variables used for writing pixels */
        int x = width;
        int y = 0;
        int off = 0;
        int passinc = interlace ? 8 : 1;
        int passht = passinc;
        int len;

        bitMask = model.getMapSize() - 1;

        /* Read codes until the eofCode is encountered */
        for (;;) {
            if (accumbits < codeSize) {
                boolean lastByte = false;
                /* fill the buffer if needed */
                while (remain < 2) {
                    if (blockEnd) {
                        /* Sometimes we have one last byte to process... */
                        if (remain == 1 && accumbits + 8 >= codeSize) {
                            break;
                        }

                        if (off > 0) {
//                             sendPixels(relx, rely+y, width, passht, rasline, model);
                             sendPixels(relx, rely+y, width, 1, rasline, model);
                        }
                        /* quietly accept truncated GIF images */
                        return false;
                    }
                    /* move remaining bytes to the beginning of the buffer */
                    block[0] = block[byteoff];
                    byteoff = 0;

                    /* fill the block */
                    len = readBytes(block, remain, blockLength + 1);

                    remain += blockLength;
                    if (len > 0) {
                        remain -= (len - 1);
                        blockLength = 0;
                    } else {
                        blockLength = (block[remain] & 0xff);
                    }
                    if (blockLength == 0) {
                        blockEnd = true;
                    }
                }

                /* 2 bytes at a time saves checking for accumbits < codeSize.
                 * We know we'll get enough and also that we can't overflow
                 * since codeSize <= 12.
                 */
                if (!lastByte) {
                    remain -= 2;
                    accumdata += (block[byteoff++] & 0xff) << accumbits;
                    accumbits += 8;
                    accumdata += (block[byteoff++] & 0xff) << accumbits;
                    accumbits += 8;
                } else {
                    remain--;
                    accumdata += (block[byteoff++] & 0xff) << accumbits;
                    accumbits += 8;
                }
            }

            /* Compute the code */
            code = accumdata & codeMask;
            accumdata >>= codeSize;
            accumbits -= codeSize;

            /*
             * Interpret the code
             */
            if (code == clearCode) {
                /* Clear code sets everything back to its initial value, then
                 * reads the immediately subsequent code as uncompressed data.
                 */

                /* Note that freeCode is one less than it is supposed to be,
                 * this is because it will be incremented next time round the 
                 * loop
                 */
                freeCode = clearCode + 1;
                codeSize = initCodeSize + 1;
                maxCode = 1 << codeSize;
                codeMask = maxCode - 1;

                /* Continue if we've NOT reached the end, some Gif images
                 * contain bogus codes after the last clear code.
                 */
                if (y < height) {
                    continue;
                }

                /* pretend we've reached the end of the data */
                code = eofCode;
            }

            if (code == eofCode) {
                /* make sure we read the whole block of pixels. */
                while (!blockEnd) {
                    if (readBytes(block, 0, blockLength + 1) != 0) {
                        /* quietly accept truncated GIF images */
                        return false;
                    }
                    blockLength = block[blockLength];
                    blockEnd = (blockLength == 0);
                }
                return true;
            }

            /* It must be data: save code in CurCode */
            curCode = code;
            /* Whenever it gets here outCount is always equal to 
               OUTCODELENGTH, so no need to reset outCount. */
            //outCount = OUTCODELENGTH;

            /* If greater or equal to freeCode, not in the hash table
             * yet; repeat the last character decoded
             */
            if (curCode >= freeCode) {
                if (curCode > freeCode) {
                    /*
                     * if we get a code too far outside our range, it
                     * could case the parser to start traversing parts
                     * of our data structure that are out of range...
                     */
                    /*In native version: goto flushit;*/
                    while (!blockEnd) {
                        if (readBytes(block, 0, blockLength + 1) != 0) {
                            /* quietly accept truncated GIF images */
                            return false;
                        }
                        blockLength = block[blockLength];
                        blockEnd = (blockLength == 0);
                    }
                    return true;
                }
                curCode = oldCode;
                outCode[--outCount] = (byte)prevChar;
            }

            /* Unless this code is raw data, pursue the chain pointed
             * to by curCode through the hash table to its end; each
             * code in the chain puts its associated output code on
             * the output queue.
             */
             while (curCode > bitMask) {
                 outCode[--outCount] = suffix[curCode];
                 if (outCount == 0) {
                     /*
                      * In theory this should never happen since our
                      * prefix and suffix arrays are monotonically
                      * decreasing and so outCode will only be filled
                      * as much as those arrays, but I don't want to
                      * take that chance and the test is probably
                      * cheap compared to the read and write operations.
                      * If we ever do overflow the array, we will just
                      * flush the rest of the data and quietly accept
                      * the GIF as truncated here.
                      */
                     //In native version: goto flushit;
                     while (!blockEnd) {
                        if (readBytes(block, 0, blockLength + 1) != 0) {
                            /* quietly accept truncated GIF images */
                            return false;
                        }
                        blockLength = block[blockLength];
                        blockEnd = (blockLength == 0);
                    }
                    return true;
                 }
                 curCode = prefix[curCode];
             }

            /* The last code in the chain is treated as raw data. */
            prevChar = (char)curCode;
            outCode[--outCount] = (byte)prevChar;

            /* Now we put the data out to the Output routine. It's
             * been stacked LIFO, so deal with it that way...
             */
            len = OUTCODELENGTH - outCount; /* This is why I commented out
                                               the code that resets outCount. */
            while (--len >= 0) {
                rasline[off++] = outCode[outCount++];

                /* Update the X-coordinate, and if it overflows, update the
                 * Y-coordinate
                 */
                if (--x == 0) {
                    int count;

                    /* If a non-interlaced picture, just increment y to the next
                     * scan line.  If it's interlaced, deal with the interlace as
                     * described in the GIF spec.  Put the decoded scan line out
                     * to the screen if we haven't gone past the bottom of it
                     */
//                    count = sendPixels(relx, rely+y, width, passht, rasline, model);
                    count = sendPixels(relx, rely+y, width, 1, rasline, model);

                    if (count <= 0) {
                        /* Nobody is listening any more. */
                        return false;
                    }

                    x = width;
                    off = 0;
                    /*  pass        inc     ht      ystart */
                    /*   0           8      8          0   */
                    /*   1           8      4          4   */
                    /*   2           4      2          2   */
                    /*   3           2      1          1   */
                    y += passinc;
                    while (y >= height) {
                        passinc = passht;
                        passht >>= 1;
                        y = passht;
                        if (passht == 0) {
                            //In native version: goto flushit;
                            while (!blockEnd) {
                                if (readBytes(block, 0, blockLength + 1) != 0) {
                                    /* quietly accept truncated GIF images */
                                    return false;
                                }
                                blockLength = block[blockLength] & 0xff;
                                blockEnd = (blockLength == 0);
                            }
                            return true;
                        }
                    }
                }
            }

            /* Build the hash table on-the-fly. No table is stored in the file. */
            prefix[freeCode] = (short)oldCode;
            suffix[freeCode] = (byte)prevChar;
            oldCode = code;
            /* Point to the next slot in the table.  If we exceed the
             * maxCode, increment the code size unless
             * it's already 12.  If it is, do nothing: the next code
             * decompressed better be CLEAR
             */
            if (++freeCode >= maxCode) {
                if (codeSize < 12) {
                    codeSize++;
                    maxCode <<= 1;
                    codeMask = maxCode - 1;
                } else {
                    /* Just in case */
                    freeCode = maxCode - 1;
                }
            }
        }
    }

    private int sendPixels(int x, int y, int width, int height, byte rasline[], ColorModel model) {
	int rasbeg, rasend, x2;
	if (y < 0) {
	    height += y;
	    y = 0;
	}
	if (y + height > global_height) {
	    height = global_height - y;
	}
	if (height <= 0) {
	    return 1;
	}
	// rasline[0]     == pixel at coordinate (x,y)
	// rasline[width] == pixel at coordinate (x+width, y)
	if (x < 0) {
	    rasbeg = -x;
	    width += x;		// same as (width -= rasbeg)
	    x2 = 0;		// same as (x2     = x + rasbeg)
	} else {
	    rasbeg = 0;
	    // width -= 0;	// same as (width -= rasbeg)
	    x2 = x;		// same as (x2     = x + rasbeg)
	}
	// rasline[rasbeg]          == pixel at coordinate (x2,y)
	// rasline[width]           == pixel at coordinate (x+width, y)
	// rasline[rasbeg + width]  == pixel at coordinate (x2+width, y)
	if (x2 + width > global_width) {
	    width = global_width - x2;
	}
	if (width <= 0) {
	    return 1;
	}
	rasend = rasbeg + width;
	// rasline[rasbeg] == pixel at coordinate (x2,y)
	// rasline[rasend] == pixel at coordinate (x2+width, y)
	int off = y * global_width + x2;
	boolean save = (curframe.disposal_method == GifFrame.DISPOSAL_SAVE);
	if (trans_pixel >= 0 && !curframe.initialframe) {
	    if (saved_image != null && saved_model == model) {
		for (int i = rasbeg; i < rasend; i++, off++) {
		    byte pixel = rasline[i];
		    if ((pixel & 0xff) == trans_pixel) {
			rasline[i] = saved_image[off];
		    } else if (save) {
			saved_image[off] = pixel;
		    }
		}
	    } else {
		// We have to do this the hard way - only transmit
		// the non-transparent sections of the line...
		int runstart = -1;
		int count = 1;
		for (int i = rasbeg; i < rasend; i++, off++) {
		    byte pixel = rasline[i];
		    if ((pixel & 0xff) == trans_pixel) {
			if (runstart >= 0) {
//			    count = setPixels(x + runstart, y, i - runstart, height, model, rasline, runstart, 0);
			    count = setPixels(x + runstart, y, i - runstart, height, model, rasline, runstart, i - runstart);
			    if (count == 0) {
				break;
			    }
			}
			runstart = -1;
		    } else {
			if (runstart < 0) {
			    runstart = i;
			}
			if (save) {
			    saved_image[off] = pixel;
			}
		    }
		}
		if (runstart >= 0) {
//		    count = setPixels(x + runstart, y, rasend - runstart, height, model, rasline, runstart, 0);
		    count = setPixels(x + runstart, y, rasend - runstart, height, model, rasline, runstart, rasend - runstart);
		}
		// Since (saved_model != model), store must be null...
		return count;
	    }
	} else if (save) {
	    System.arraycopy(rasline, rasbeg, saved_image, off, width);
	}
//	int count = setPixels(x2, y, width, height, model, rasline, rasbeg, 0);
	int count = setPixels(x2, y, width, height, model, rasline, rasbeg, width);
	return count;
    }

    /**
     * Read Image data
     */
    private boolean readImage(boolean first, int disposal_method, int delay)
	throws IOException
    {
	if (curframe != null && !curframe.dispose()) {
	    abort();
	    return false;
	}

	long tm = 0;

	if (verbose) {
	    tm = System.currentTimeMillis();
	}

	// Allocate the buffer
	byte block[] = new byte[256 + 3];

	// Read the image descriptor
	if (readBytes(block, 0, 10) != 0) {
	    throw new IOException();
	}
	int x = ExtractWord(block, 0);
	int y = ExtractWord(block, 2);
	int width = ExtractWord(block, 4);
	int height = ExtractWord(block, 6);
	boolean interlace = (block[8] & INTERLACEMASK) != 0;

	IndexColorModel model = global_model;

	if ((block[8] & COLORMAPMASK) != 0) {
	    // We read one extra byte above so now when we must
	    // transfer that byte as the first colormap byte
	    // and manually read the code size when we are done
	    int num_local_colors = 1 << ((block[8] & 0x7) + 1);

	    // Read local colors
	    byte[] local_colormap = new byte[num_local_colors * 3];
	    local_colormap[0] = block[9];
	    if (readBytes(local_colormap, 1, num_local_colors * 3 - 1) != 0) {
		throw new IOException();
	    }

	    // Now read the "real" code size byte which follows
	    // the local color table
	    if (readBytes(block, 9, 1) != 0) {
		throw new IOException();
	    }
	    model = new IndexColorModel(8, num_local_colors, local_colormap,
					0, false, trans_pixel);
	} else if (model == null
		   || trans_pixel != model.getTransparentPixel()) {
	    model = new IndexColorModel(8, num_global_colors, global_colormap,
					0, false, trans_pixel);
	    global_model = model;
	}

	// Notify the consumers
	if (first) {
            if (global_width == 0) global_width = width;
            if (global_height == 0) global_height = height;

	    setDimensions(global_width, global_height);
	    setProperties(props);
	    setColorModel(model);
	    headerComplete();
	}

	if (disposal_method == GifFrame.DISPOSAL_SAVE && saved_image == null) {
	    saved_image = new byte[global_width * global_height];
	}

	int hints = (interlace ? interlaceflags : normalflags);
	setHints(hints);

	curframe = new GifFrame(this, disposal_method, delay,
				(curframe == null), model,
				x, y, width, height);

	// allocate the raster data
	byte rasline[] = new byte[width];

	if (verbose) {
	    System.out.print("Reading a " + width + " by " + height + " " +
		      (interlace ? "" : "non-") + "interlaced image...");
	}

	boolean ret = parseImage(x, y, width, height,
				 interlace, ExtractByte(block, 9),
				 block, rasline, model);

	if (!ret) {
	    abort();
	}

	if (verbose) {
	    System.out.println("done in "
			       + (System.currentTimeMillis() - tm)
			       + "ms");
	}

	return ret;
    }


    /* 
     * Since I translated the JNI version of parseImage() into Java,
     * we no longer need initIDs().
     */
    /* // if we're using JNI, we need to find the method and field IDs.
     * private static native void initIDs();
     * static {
     * 	 initIDs();
     * }
     */
}

class GifFrame {
    private static final boolean verbose = false;
    private static IndexColorModel trans_model;

    static final int DISPOSAL_NONE      = 0x00;
    static final int DISPOSAL_SAVE      = 0x01;
    static final int DISPOSAL_BGCOLOR   = 0x02;
    static final int DISPOSAL_PREVIOUS  = 0x03;

    GifImageDecoder decoder;

    int disposal_method;
    int delay;

    IndexColorModel model;

    int x;
    int y;
    int width;
    int height;

    boolean initialframe;

    public GifFrame(GifImageDecoder id, int dm, int dl, boolean init,
		    IndexColorModel cm, int x, int y, int w, int h) {
	this.decoder = id;
	this.disposal_method = dm;
	this.delay = dl;
	this.model = cm;
	this.initialframe = init;
	this.x = x;
	this.y = y;
	this.width = w;
	this.height = h;
    }

    private void setPixels(int x, int y, int w, int h,
			   ColorModel cm, byte[] pix, int off, int scan) {
	decoder.setPixels(x, y, w, h, cm, pix, off, scan);
    }

    public boolean dispose() {
	if (decoder.imageComplete(ImageConsumer.SINGLEFRAMEDONE, false) == 0) {
	    return false;
	} else {
	    if (delay > 0) {
		try {
		    if (verbose) {
			System.out.println("sleeping: "+delay);
		    }
		    Thread.sleep(delay);
		} catch (InterruptedException e) {
		    return false;
		}
	    } else {
		Thread.yield();
	    }
	    
	    if (verbose && disposal_method != 0) {
		System.out.println("disposal method: "+disposal_method);
	    }

	    int global_width = decoder.global_width;
	    int global_height = decoder.global_height;

	    if (x < 0) {
		width += x;
		x = 0;
	    }
	    if (x + width > global_width) {
		width = global_width - x;
	    }
	    if (width <= 0) {
		disposal_method = DISPOSAL_NONE;
	    } else {
		if (y < 0) {
		    height += y;
		    y = 0;
		}
		if (y + height > global_height) {
		    height = global_height - y;
		}
		if (height <= 0) {
		    disposal_method = DISPOSAL_NONE;
		}
	    }

	    switch (disposal_method) {
	    case DISPOSAL_PREVIOUS:
		byte[] saved_image = decoder.saved_image;
		IndexColorModel saved_model = decoder.saved_model;
		if (saved_image != null) {
		    setPixels(x, y, width, height,
			      saved_model, saved_image,
			      y * global_width + x, global_width);
		}
		break;
	    case DISPOSAL_BGCOLOR:
		byte tpix;
		if (model.getTransparentPixel() < 0) {
		    model = trans_model;
		    if (model == null) {
			model = new IndexColorModel(8, 1,
						    new byte[4], 0, true);
			trans_model = model;
		    }
		    tpix = 0;
		} else {
		    tpix = (byte) model.getTransparentPixel();
		}

		int rassize = width * height; 
		byte[] rasfill = new byte[rassize];
		if (tpix != 0) {
		    for (int i = 0; i < rassize; i++) {
			rasfill[i] = tpix;
		    }
		}
//		setPixels(x, y, width, height, model, rasfill, 0, 0);
		setPixels(x, y, width, height, model, rasfill, 0, width);
		break;
	    case DISPOSAL_SAVE:
		decoder.saved_model = model;
		break;
	    }
	}
	return true;
    }
}
