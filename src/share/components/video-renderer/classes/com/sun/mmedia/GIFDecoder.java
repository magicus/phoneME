/*
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

package com.sun.mmedia;

import java.io.IOException;
import java.util.Vector;
import javax.microedition.media.MediaException;
import javax.microedition.media.Player;
import javax.microedition.media.protocol.SourceStream;

public class GIFDecoder {
    /* Single image decoder */
    private GIFImageDecoder imageDecoder;
    
    private final SourceStream stream;

    /* the seek type of the stream: either <code>NOT_SEEKABLE</code>,
     * <code>SEEKABLE_TO_START</code> or <code>RANDOM_ACCESSIBLE</code>
     */
    private int seekType;

    /* the width of a video frame */
    private int videoWidth;

    /* the height of a video frame */
    private int videoHeight;

    /* the position in the source stream directly after the GIF header */
    private long firstFramePos;

    /* the frame count, shows number of rendered frames, and index of next frame to render  */
    private int frameCount;

    /* image data */
    private byte[] imageData;
    private int imageDataLength;
    private int lzwCodeSize;

    /* Last frame duration while scanning frames */
    private int scanFrameTime;

    /* a table of frame durations (default rate) */
    private Vector frameTimes;

    /* the duration of the movie in microseconds (default rate) */
    private long duration;


    public GIFDecoder( SourceStream stream )
    {
        this.stream = stream;
        seekType = stream.getSeekType();
        frameCount = 0;
        duration = Player.TIME_UNKNOWN;

    }

    public int getWidth()
    {
        return videoWidth;
    }

    public int getHeight()
    {
        return videoHeight;
    }

    /**
     * Parses the GIF header.
     *
     * @return    true, if the header was parsed successfully
     *            and the the GIF signature and version are
     *            correct,
     *            otherwise false.
     */
    public boolean parseHeader() {
        //System.out.println("parseHeader at pos " + stream.tell());

        byte [] header = new byte[6];

        try {
            stream.read(header, 0, 6);
        } catch (IOException e) {
            return false;
        }

        // check that signature spells GIF
        if (header[0] != 'G' || header[1] != 'I' || header[2] != 'F')
            return false;

        // check that version spells either 87a or 89a
        if (header[3] != '8' || header[4] != '7' && header[4] != '9' ||
            header[5] != 'a')
            return false;

        return parseLogicalScreenDescriptor();
    }

    /**
     *  Description of the Method
     *
     * @param  bin  Description of the Parameter
     */
    private boolean parseLogicalScreenDescriptor() {
        //System.out.println("parseLogicalScreenDescriptor at pos " + stream.tell());

        byte [] logicalScreenDescriptor = new byte[7];
        byte [] globalColorTable = null;

        try {
            stream.read(logicalScreenDescriptor, 0, 7);
        } catch (IOException e) {
            return false;
        }

        // logical screen width
        videoWidth = readShort(logicalScreenDescriptor, 0);

        // logical screen height
        videoHeight = readShort(logicalScreenDescriptor, 2);

        // flags
        int flags = logicalScreenDescriptor[4];

        // global color table flag
        boolean globalTable = ((flags >> 7) & 0x01) == 1;

        // color resolution
        int resolution = ((flags >> 4) & 0x07) + 1;

        // sort flag: not used in player
        //int sortFlag = (flags >> 3) & 0x01;

        // global color table depth
        int tableDepth = (flags & 0x07) + 1;

        // background color index
        int index = logicalScreenDescriptor[5] & 0xff;

        // pixel aspect ratio: not used inplayer
        //int pixelAspectRatio = logicalScreenDescriptor[6];

        imageDecoder = new GIFImageDecoder(videoWidth, videoHeight, resolution);

        if (globalTable) {
            int size = 3 * (1 << tableDepth);
            globalColorTable = new byte[size];

            try {
                stream.read(globalColorTable, 0, size);
            } catch (IOException e) {
            }

            imageDecoder.setGlobalPalette(tableDepth, globalColorTable, index);
        }

        firstFramePos = stream.tell();

        return true;
    }

    /**
     * Reads a 16-bit unsigned short value from data starting
     * at the specified offset.
     *
     * @param data   the byte array
     * @param offset offset into the byte array
     * @return       the short value
     */
    private int readShort(byte data[], int offset) {
        int lo = data[offset] & 0xff;
        int hi = data[offset + 1] & 0xff;

        return lo + (hi << 8);
    }

    /**
     * Reads a 16-bit unsigned short value from the source stream.
     *
     * @return       the short value
     */
    private int readShort() {
        int val = 0;

        try {
            int lo = readUnsignedByte();
            int hi = readUnsignedByte();

            val = lo + (hi << 8);
        } catch (IOException e) {
        }

        return val;
    }

    /**
     * A byte array designed to hold one byte of data.
     * see: readUnsignedByte().
     */
    private byte[] oneByte = new byte[1];

    /**
     * Reads one byte from the source stream.
     */
    private int readUnsignedByte() throws IOException {
        if (stream.read(oneByte, 0, 1) == -1)
            throw new IOException();

        return oneByte[0] & 0xff;
    }

    /*
     * Rewinds the stream to the position of the first frame
     * to be able to read it again
     */
    void seekFirstFrame() throws IOException {
        frameCount = 0;
        if (seekType == SourceStream.RANDOM_ACCESSIBLE) {
            // seek to the beginning of the first frame
            stream.seek(firstFramePos);
        } else { // SEEKABLE_TO_START
            // seek to the start of stream and parse the header
            stream.seek(0);
            parseHeader();
        }
        imageDecoder.clearImage();
    }

    void decodeFrame( int [] output ) {
        if (imageData != null && imageDecoder != null && output != null)
            imageDecoder.decodeImage(lzwCodeSize, imageDataLength, imageData, output);
    }

    /**
     * Reads data from the stream object and constructs a GIF frame.
     *
     * @return  true if the frame was read successfully,
     *          otherwise false.
     */
    private boolean getFrame() {
        //System.out.println("getFrame at pos " + stream.tell());

        if (stream.tell() == 0)
            parseHeader();

        boolean eos = false;

        imageData = null;

        do {
            int id;

            try {
                id = readUnsignedByte();
                //System.out.println("getFrame: id=" + id);
            } catch (IOException e) {
                id = 0x3b;
            }

            if (id == 0x21) {
                parseControlExtension(false);
            } else if (id == 0x2c) {
                parseImageDescriptor(false);
            } else if (id == 0x3b) {
                eos = true;
            } else {
                eos = true;
            }
        } while (!eos && imageData == null);

        if (imageData != null) {
            frameCount++;
            return true;
        }

        return false;
    }

    /**
     * Parses the Control Extension.
     *
     */
    private void parseControlExtension(boolean scan) {
        //System.out.println("parseControlExtension at pos " + stream.tell());
        try {
            int label = readUnsignedByte();

            if (label == 0xff) {
                parseApplicationExtension();
            } else if (label == 0xfe) {
                parseCommentExtension();
            } else if (label == 0xf9) {
                parseGraphicControlExtension(scan);
            } else if (label == 0x01) {
                parsePlainTextExtension();
            } else {
                // unkown control extension
            }
        } catch (IOException e) {
        }
    }

    /**
     * Parses the Image Descriptor.
     *
     * Each image in the Data Stream is composed of an Image Descriptor,
     * an optional Local Color Table, and the image data. Each image must
     * fit within the boundaries of the Logical Screen, as defined in the
     * Logical Screen Descriptor.
     */
    private void parseImageDescriptor(boolean scan) {
        //System.out.println("parseImageDescriptor at pos " + stream.tell());
        byte [] imageDescriptor = new byte[9];
        byte [] localColorTable = null;

        try {
            stream.read(imageDescriptor, 0, 9);
        } catch (IOException e) {
        }

        // packed fields
        int flags = imageDescriptor[8];

        // local color table flag
        boolean localTable = ((flags >> 7) & 1) == 1;

        int tableDepth = (flags & 0x07) + 1;

        if (localTable) {
            int size = 3 * (1 << tableDepth);

            localColorTable = new byte[size];

            try {
                stream.read(localColorTable, 0, size);
            } catch (IOException e) {
            }
        }

        if (!scan) {
            // image left position
            int leftPos = readShort(imageDescriptor, 0);

            // image top position
            int topPos = readShort(imageDescriptor, 2);

            // image width
            int width = readShort(imageDescriptor, 4);

            // image height
            int height = readShort(imageDescriptor, 6);

            // interlace flag
            boolean interlaceFlag = ((flags >> 6) & 0x01) == 1;

            // sort flag: not used in player
            //int sortFlag = (flags >> 5) & 0x01;

            imageDecoder.newFrame(leftPos, topPos, width, height, interlaceFlag);

            // local color table size
            if (localTable)
                imageDecoder.setLocalPalette(tableDepth, localColorTable);
        }

        parseImageData();
    }

    /**
     * Parses the Image Data.
     *
     * The image data for a table based image consists of a sequence of
     * sub-blocks, of size at most 255 bytes each, containing an index
     * into the active color table, for each pixel in the image. Pixel
     * indices are in order of left to right and from top to bottom. Each
     * index must be within the range of the size of the active color
     * table, starting at 0. The sequence of indices is encoded using the
     * LZW Algorithm with variable-length code.
     */
    private void parseImageData() {
        //System.out.println("parseImageData at pos " + stream.tell());
        int idx = 0;

        try {
            lzwCodeSize = readUnsignedByte();

            if (imageData == null)
                imageData = new byte[1024];

            int size;

            do {
                size = readUnsignedByte();

                if (imageData.length < idx + size) {
                    // increase image data buffer
                    byte[] data = new byte[ idx + ((size>1024)?size:1024) ];
                    System.arraycopy(imageData, 0, data, 0, idx);
                    imageData = data;
                }

                if (size > 0)
                    idx += stream.read(imageData, idx, size);

            } while (size != 0);

            //imageDataLength = idx;
        } catch (IOException e) {
            //imageDataLength = 0;
        }
        // Supporting unfinished GIFs
        imageDataLength = idx;
        //System.out.println("parsed image data bytes: " + idx);
    }

    /**
     * Parses the Plain Text Extension.
     *
     */
    private void parsePlainTextExtension() {
        try {
            // block size
            int size = readUnsignedByte();
            if (size != 12) {
                // ERROR
            }

            // text grid left position
            int leftPos = readShort();

            // text grid top position
            int topPos = readShort();

            // text grid width
            int width = readShort();

            // text grid height
            int height = readShort();

            // character cell width
            int cellWidth = readUnsignedByte();

            // character cell height
            int cellHeight = readUnsignedByte();

            // text foreground color index
            int fgIndex = readUnsignedByte();

            // text background color index
            int bgIndex = readUnsignedByte();

            // plain text data
            do {
                size = readUnsignedByte();

                if (size > 0) {
                    byte[] data = new byte[size];

                    stream.read(data, 0, size);
                }
            } while (size != 0);
        } catch (IOException e) {
        }
    }

    /**
     * Parses the Application Extension.
     *
     */
    private void parseApplicationExtension() {
        //System.out.println("parseApplicationExtension at pos " + stream.tell());
        try {
            // block size
            int size = readUnsignedByte();

            if (size != 11) {
                // System.out.println("ERROR");
            }

            // application identifier
            byte[] data = new byte[8];
            stream.read(data, 0, 8);

            // application authentication code
            data = new byte[3];
            stream.read(data, 0, 3);

            do {
                size = readUnsignedByte();

                if (size > 0) {
                    data = new byte[size];

                    stream.read(data, 0, size);
                }
            } while (size != 0);
        } catch (IOException e) {
        }
    }

    /**
     * Parses the Comment Extension.
     *
     */
    private void parseCommentExtension() {
        //System.out.println("parseCommentExtension at pos " + stream.tell());
        try {
            int size;

            do {
                size = readUnsignedByte();

                if (size > 0) {
                    byte[] data = new byte[size];

                    stream.read(data, 0, size);
                }
            } while (size != 0);
        } catch (IOException e) {
        }
    }

    /**
     * Parses the Graphic Control Extension.
     *
     */
    private void parseGraphicControlExtension(boolean scan) {
        //System.out.println("parseGraphicControlExtension at pos " + stream.tell());

        byte [] graphicControl = new byte[6];

        try {
            stream.read(graphicControl, 0, 6);
        } catch (IOException e) {
        }

        // block size: not used in player - validation only
        //int size = graphicControl[0] & 0xff;

        //if (size != 4) {
            // ERROR: invalid block size in graphic control
        //}

        if (scan) {
            // delay time
            scanFrameTime = readShort(graphicControl, 2) * 10000;
        } else {
            // packed field
            int flags = graphicControl[1] & 0xff;

            // transparency flag
            boolean transparencyFlag = (flags & 0x01) == 1;

            // user input: not used in player
            //int userInput = (flags & 0x02) == 2;

            // undraw mode
            int undrawMode = (flags >> 2) & 0x07;

            int transparencyColorIndex = -1;

            if (transparencyFlag)
                // transparent color index
                transparencyColorIndex = graphicControl[4] & 0xff;

            imageDecoder.setGraphicsControl(undrawMode, transparencyColorIndex);
        }
        // block terminator: shoud be 0
        //int terminator = graphicControl[5] & 0xff;
    }

    /**
     * Scans the input stream for GIF frames and builds a table
     * of frame durations.
     */
    private void scanFrames() throws MediaException {
        //System.out.println("scanFrames at pos " + stream.tell());
        frameCount = 0;
        scanFrameTime = 0;
        duration = 0;

        frameTimes = new Vector();

        boolean eos = false;

        do {
            int id;

            try {
                id = readUnsignedByte();
                //System.out.println("scanFrames: id=" + id);
            } catch (IOException e) {
                id = 0x3b;
            }

            if (id == 0x21) {
                parseControlExtension(true);
            } else if (id == 0x2c) {
                parseImageDescriptor(true);
                frameCount++;
                frameTimes.addElement(new Long(scanFrameTime));
                duration += scanFrameTime;
                scanFrameTime = 0; // ?? reset to zero
            } else if (id == 0x3b) {
                eos = true;
            } else {
                eos = true;
            }
        } while (!eos);

        // reset the frame counter
        frameCount = 0;

        try {
            seekFirstFrame();
        } catch (IOException e) {
            throw new MediaException(e.getMessage());
        }
    }

    public long getDuration()
    {
        return duration;
    }
}
