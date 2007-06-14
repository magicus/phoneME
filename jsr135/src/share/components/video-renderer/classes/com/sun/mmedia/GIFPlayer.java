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
import java.io.ByteArrayOutputStream;
import java.util.Vector;

import javax.microedition.media.Control;
import javax.microedition.media.MediaException;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.control.VideoControl;
import javax.microedition.media.control.FramePositioningControl;
import javax.microedition.media.control.RateControl;
import javax.microedition.media.control.StopTimeControl;

import com.sun.mmedia.Configuration;
import com.sun.mmedia.VideoRenderer;

/**
 * A player for the GIF89a.
 *
 * @created    January 30, 2004
 */
final public class GIFPlayer extends BasicPlayer implements Runnable {
    /* Single image decoder */
    private GIFImageDecoder imageDecoder;
    
    /* the width of a video frame */
    private int videoWidth;

    /* the height of a video frame */
    private int videoHeight;

    /* a full GIF frame, also called the reference frame */
    private int[] referenceFrame = null;

    /* the play thread */
    private Thread playThread; // default is null

    /* flag indicating whether to end the play thread.
     * done is set to true upon stopping or closing the player
     * or when the frame count progresses to the total number
     * of frames in this movie.
     */
    private boolean done;

    /* the start time in milliseconds.
     * startTime is initialized upon start of the play thread.
     */
    private long startTime;

    /* early threshold value for the display time in milliseconds. */
    private long EARLY_THRESHOLD = 100;

    /* minimum wait time */
    private final long MIN_WAIT = 50;

    /* For zero duration GIFs (e.g. non-animated) wait time between STARTED and END_OF_MEDIA */
    private final long ZERO_DURATION_WAIT = 50;

    /* a table of frame durations */    
    private Vector frameTimes;

    /* the frame count, shows number of rendered frames, and index of next frame to render  */
    private int frameCount;

    /* Last frame duration while scanning frames */
    private int scanFrameTime;

    /* elapsed media time since start of stream */
    private long mediaTimeOffset;

    /* the display time of the last read & created frame  */
    private long displayTime; // default is 0

    /* the video renderer object for the GIF Player */
    private VideoRenderer videoRenderer;

    /* the video control object for the GIF Player */
    private VideoControl videoControl;
    
    /* the frame positioning control object for the GIF Player */
    private FramePosCtrl framePosControl;

    /* the rate control object for the GIF Player */
    private RateCtrl rateControl;

    /* the duration of the movie in microseconds */
    private long duration;

    /* the seek type of the stream: either <code>NOT_SEEKABLE</code>, 
     * <code>SEEKABLE_TO_START</code> or <code>RANDOM_ACCESSIBLE</code>
     */
    private int seekType;

    /* the position in the source stream directly after the GIF header */
    private long firstFramePos;

    /* stopped flag */
    private boolean stopped;

    /* The lock object of play thread */
    private Object playLock = new Object();

    /* image data */
    private byte[] imageData;
    private int imageDataLength;
    private int lzwCodeSize;

    /**
     * Retrieves the specified control object for the
     * GIF Player. The following controls are currently
     * implemented: VideoControl, FramePositioningControl
     * and RateControl and StopTimeControl.
     *
     * @param  type       the requested control type.
     * @return            the control object if available,
     *                    otherwise null.
     */
    protected Control doGetControl(String type) {
        if (type.startsWith(BasicPlayer.pkgName)) {
            
            type = type.substring(BasicPlayer.pkgName.length());
            
            if (type.equals(BasicPlayer.vicName) || 
                type.equals(BasicPlayer.guiName)) {
                // video control
                return videoControl;
            } else if (type.equals(BasicPlayer.fpcName)) { 
                // frame positioning control
                return framePosControl;
            } else if (type.equals(BasicPlayer.racName)) { 
                // rate control
                return rateControl;
            } else if (type.equals(BasicPlayer.stcName)) {
                // stop time control

                // StopTimeControl is implemented BasicPlayer,
                // the parent class of GIF Player
                return this;
            }
        }
        return null;
    }

    /**
     * Retrieves the duration of the GIF movie.
     *
     * @return    the duration in microseconds.
     */
    protected long doGetDuration() {
        return duration;
    }

    /**
     * Retrieves the current media time.
     *
     * @return    the media time in microseconds.
     */
    protected long doGetMediaTime() {
        long mediaTime;

        if (getState() < STARTED) {
            mediaTime = mediaTimeOffset;
        } else {
            mediaTime = ((System.currentTimeMillis() - startTime) * 1000) + mediaTimeOffset;
            mediaTime *= (rateControl.getRate() / 100000.0);
        }

        if (mediaTime >= duration) {
            return duration;
        }

        return mediaTime;       
    }

    /**
     * Sets the media time of the GIF Player.
     *
     * @param  now the new media time.
     * @exception MediaException thrown if setMediaTime fails.
     * @return the new media time in microseconds.
     */
    protected long doSetMediaTime(long now) throws MediaException {
        if (seekType == NOT_SEEKABLE)
            throw new MediaException("stream not seekable");

        if (state == STARTED)
            doStop();

        if (now > duration)
            now = duration;

        mediaTimeOffset = now;

        try {
            int count = framePosControl.mapTimeToFrame(now);
            //System.out.println("SetMediaTime to " + now + " (frame = " + count + "), frameCount=" + frameCount);

            if (count + 1 < frameCount) {
                // rewind to beginning
                frameCount = 0;
                seekFirstFrame();         
            }

            // skip frames
            while (frameCount <= count && getFrame())
                // We need to decode all frames to have the correct pixels
                // for frames with transparent color
                decodeFrame();

            displayTime = getDuration(frameCount) / 1000;
            //System.out.println("SetMediaTime: displayTime = " + displayTime + "; frameCount=" + frameCount);

            renderFrame();

            if (state == STARTED)
                // restart the player
                doStart();
        } catch (IOException e) {
            throw new MediaException(e.getMessage());
        }

        return now;
    }

    /**
     * Realizes the GIF Player.
     * 
     * @see                      Player#realize()
     * @exception MediaException Thrown if the <code>Player</code> cannot
     *                           be realized.
     */
    protected void doRealize() throws MediaException {          
        duration = TIME_UNKNOWN;
        frameCount = 0;
        mediaTimeOffset = 0;

        seekType = stream.getSeekType();

        // parse GIF header
        if (parseHeader()) {
            scanFrames();

            // initialize video control
            videoRenderer = Configuration.getConfiguration().getVideoRenderer(
                                              this, videoWidth, videoHeight);
            videoControl = (VideoControl)videoRenderer.getVideoControl();
            videoRenderer.initRendering(VideoRenderer.XBGR888 | 
                                        VideoRenderer.USE_ALPHA,
                                        videoWidth, videoHeight);

            // initialize frame positioning control
            framePosControl = new FramePosCtrl();

            // initialize rate control
            rateControl = new RateCtrl();

            referenceFrame = null;

        } else
            throw new MediaException("invalid GIF header");

    }

    /**
     * Prefetches the GIF Player.
     * 
     * @see                      Player#prefetch()
     * @exception MediaException Thrown if the <code>Player</code> cannot
     *                           be prefetched.
     */
    protected void doPrefetch() throws MediaException {
        if (referenceFrame == null)
            referenceFrame = new int[videoWidth * videoHeight];

        try { 
            frameCount = 0;
            seekFirstFrame();         

            // get first frame
            if (!getFrame())
                throw new MediaException("can't get first frame");

            decodeFrame();

            // If duration is 0 prepare the last frame once.
            if (duration == 0) {
                while (getFrame())
                    decodeFrame();
                renderFrame();
            }

        } catch (IOException e) {
            throw new MediaException("can't seek first frame");
        }
    }
    
    /**
     * Starts the GIF Player.
     * 
     * @see       Player#start()
     * @return    true, if the player was started successfully,
     *            otherwise false.
     */
    protected boolean doStart() {
        if (duration == 0) { // e.g. for non-animated GIFs
            new Thread(new Runnable() {
                synchronized public void run() {
                    try {
                        wait(ZERO_DURATION_WAIT);
                    } catch (InterruptedException ie) { }
                    sendEvent(PlayerListener.END_OF_MEDIA, new Long(0));
                }
            }).start();
        } else {
            startTime = System.currentTimeMillis(); 

            if (stopped) {
                // wake up existing play thread
                stopped = false;
            
                synchronized (playLock) {
                    playLock.notifyAll();
                }
            } else {
                displayTime = getFrameInterval(frameCount) / 1000;
                
                // Ensure that previous thread has finished
                playThreadFinished();

                synchronized (playLock) {
                    if (playThread == null) {
                        // Check for null is a protection against several
                        // simultaneous doStart()'s trying to create a new thread.
                        // But if playThreadFinished() failed to terminate
                        // playThread, we can have a problem

                        // create a new play thread
                        playThread = new Thread(this);
                        playThread.start();
                    }
                }
            }
        }
        return true;
    }

    /**
     * Stops the GIF Player.
     * 
     * @see                      Player#stop()
     * @exception MediaException Thrown if the <code>Player</code> cannot
     *                           be stoppped.     
     */
    protected void doStop() throws MediaException {
        if (stopped) return;    

        synchronized (playLock) {
            try {
                if (playThread != null) {
                    stopped = true;
                    playLock.notifyAll();               
                    mediaTimeOffset = doGetMediaTime();
                    startTime = 0;
                    playLock.wait();
                }
            } catch (InterruptedException ie) {
                //do nothing
            }
        }
    }

    /**
     * Deallocates the GIF Player.
     * 
     * @see   Player#deallocate()
     */
    protected void doDeallocate() {
        playThreadFinished();
        
        stopped = false;
        referenceFrame = null;
    }

    /**
     * Closes the GIF Player.
     * 
     * @see   Player#close()
     */
    protected void doClose() {
        done = true;

        if (videoRenderer != null) {
            videoRenderer.close();
            videoRenderer = null;
        }

        frameTimes = null;
        imageDecoder = null;
        imageData = null;
    }

    /**
     * The run method driving the play thread.
     */
    public void run() {
        done = false;
        
        while (!done) {
            if (!stopped)
                processFrame();

            if (stopped) {
                synchronized (playLock) {
                    playLock.notifyAll();
                
                    try {
                        playLock.wait();
                    } catch (InterruptedException e) {
                        // nothing to do
                    }
                }
            }
        }

        if (!stopped && !framePosControl.isActive()) {
            // the run loop may have terminated prematurely, possibly
            // due to an I/O error...
            // In this case, the duration needs to be updated.
            if (frameCount < frameTimes.size()) {
                duration = getDuration(frameCount);

                sendEvent(PlayerListener.DURATION_UPDATED, new Long(duration));
            }

            // send an end-of-media if the player was not stopped
            // and the run loop terminates because the end of media
            // was reached.
            mediaTimeOffset = doGetMediaTime(); 
            startTime = 0;

            sendEvent(PlayerListener.END_OF_MEDIA, new Long(mediaTimeOffset));
        }

        synchronized (playLock) {
            playThread = null;
            playLock.notifyAll();
        }
    }


    /**
     * Stops the GIF player when the stop time has been reached.
     */
    private void stopTimeReached() {
        // stop the player
        mediaTimeOffset = doGetMediaTime();
        stopped = true;
        startTime = 0;        
        // send STOPPED_AT_TIME event
        satev();
    }

    /**
     * Ensures that playThread dies
     */
    private void playThreadFinished() {
        synchronized (playLock) {
            // stop the playThread if it was created and started
            if (playThread != null) {
                done = true;
                
                // wake up the play thread if it was stopped
                playLock.notifyAll();
                
                // wait for the play thread to terminate gracefully
                try {
                    // set maximum wait limit in case anything goes wrong.
                    playLock.wait(5000);
                } catch (InterruptedException e) {
                    // nothing to do.
                }
            }
        }
    }
    
    /**
     * Returns the duration in microseconds.
     */
    private long getDuration(int frameCount) {
        long duration = 0;
         
        for (int i = 0; i < frameCount; i++) {
            duration += ((Long)frameTimes.elementAt(i)).longValue();
        }
                    
        return duration;    
    }

    private long getFrameInterval(int frameCount) {
        long interval = 0;
         
        if (frameCount > 0 && frameCount <= frameTimes.size()) {
            interval = ((Long)frameTimes.elementAt(frameCount - 1)).longValue();
        }

        return interval;    
    }

    /**
     * Maps media time to the corresponding frame.
     *
     * Returns the frame number.
     */
    private int timeToFrame(long mediaTime) {
        int frame = 0;

        long elapsedTime = 0;

        for (int i = 0; i < frameTimes.size(); i++) {
            long interval = ((Long)frameTimes.elementAt(i)).longValue();

            elapsedTime += interval;

            if (elapsedTime <= mediaTime)
                frame++;
            else
                break;
        }
        
        return frame;
    }

    /**
     * Maps a frame to the corresponding media time.
     *
     * Returns the time in microseconds.
     */
    private long frameToTime(int frameNumber) {
        long elapsedTime = 0;

        for (int i = 0; i < frameTimes.size(); i++) {
            long interval = ((Long)frameTimes.elementAt(i)).longValue();
            
            if (i < frameNumber)
                elapsedTime += interval;
            else
                break;
        }

        return elapsedTime;
    }

    /**
     * Decodes and renders a GIF Frame.
     */
    private void processFrame() {
        // the media time in milliseconds
        long mediaTime = doGetMediaTime() / 1000;

        // frame interval in milliseconds
        long frameInterval = getFrameInterval(frameCount) / 1000;
        //System.out.println("Frame: " + frameCount + ", length: " + frameInterval + ", at: " + mediaTime + ", displayTime: " + displayTime);

        if (mediaTime + EARLY_THRESHOLD > displayTime) {
            // get the next frame
            if (!getFrame()) {
                // wait until end of last frame
                synchronized (playLock) {
                    try {
                        long waitTime = displayTime - mediaTime;

                        if (waitTime > 0)
                            playLock.wait(waitTime);
                                    
                    } catch (InterruptedException e) {
                        // nothing to do
                    }               
                }
                done = true;
                return;
            }
            decodeFrame();

            // frame interval in milliseconds
            frameInterval = getFrameInterval(frameCount) / 1000;

            // move display time to end of frame
            displayTime += frameInterval;
        }

        // render last read frame
        renderFrame();

        // report that stop time has been reached if
        // the mediaTime is greater or equal to stop time.      
        if (stopTime != StopTimeControl.RESET && 
            doGetMediaTime() >= stopTime) {
            stopTimeReached();
        }
       
        if (!stopped) {
            // threshold levels in milliseconds
            // It makes playback falter if frame intervals differ
            //EARLY_THRESHOLD = 250;
            //if (frameInterval > 0 && frameInterval < EARLY_THRESHOLD)
            //    EARLY_THRESHOLD = frameInterval / 2;
                        
            mediaTime = doGetMediaTime() / 1000;

            if (mediaTime + EARLY_THRESHOLD <= displayTime) {
                // wait for a bit
                synchronized (playLock) {
                    try {
                        if (!done) {
                            mediaTime = doGetMediaTime() / 1000;

                            long waitTime = displayTime - EARLY_THRESHOLD - mediaTime;
                                
                            while (!stopped && waitTime > 0) {
                                if (waitTime > MIN_WAIT) {
                                    playLock.wait(MIN_WAIT);
                                    waitTime -= MIN_WAIT;
                                } else {
                                    playLock.wait(waitTime);
                                    waitTime = 0;
                                }
                                    
                                if (stopTime != StopTimeControl.RESET && 
                                    doGetMediaTime() >= stopTime) {
                                    stopTimeReached();
                                }
                            }
                        }
                    } catch (InterruptedException e) {
                        // nothing to do
                    }           
                }
            }
        }
    }
    
    /*
     * Rewinds the stream to the position of the first frame
     * to be able to read it again
     */
    private void seekFirstFrame() throws IOException {
        if (seekType == RANDOM_ACCESSIBLE) {
            // seek to the beginning of the first frame
            stream.seek(firstFramePos);
        } else { // SEEKABLE_TO_START           
            // seek to the start of stream and parse the header
            stream.seek(0);
            parseHeader();
        }
        imageDecoder.clearImage();
    }
    
    private void decodeFrame() {
        if (imageData != null && imageDecoder != null && referenceFrame != null)
            imageDecoder.decodeImage(lzwCodeSize, imageDataLength, imageData, referenceFrame);
    }

    /**
     * Renders a frame.
     */
    private void renderFrame() {
        if (referenceFrame != null)
            videoRenderer.render(referenceFrame);
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
     * Parses the GIF header.
     *
     * @return    true, if the header was parsed successfully
     *            and the the GIF signature and version are
     *            correct,
     *            otherwise false.
     */
    private boolean parseHeader() {
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
                    byte data[] = new byte[idx + size];
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

    /**
     * Inner class implementing the frame positioning control interface.
     * 
     */
    class FramePosCtrl implements FramePositioningControl {
        /**
         * indicates whether the frame positioning control
         * is actively engaged.
         */
        private boolean active;

        /**
         * The constructor of FramePosCtrl.
         */
        FramePosCtrl() {
            active = false;
        }

        /**
         * Seek to a given video frame.
         * The media time of the <code>Player</code> will be updated
         * to reflect the new position set.
         * <p>
         * This method can be called on a stopped or started
         * <code>Player</code>.
         * If the <code>Player</code> is
         * in the <i>Started</i> state, this method may cause the
         * <code>Player</code> to change states.  If that happens, the
         * appropriate transition events will be posted by
         * the <code>Player</code> when its state changes.
         * <p>
         * If the given frame number is less than the first or larger
         * than the last frame number in the media, <code>seek</code>
         * will jump to either the first or the last frame respectively.
         *
         * @param frameNumber the frame to seek to.
         * @return the actual frame that the Player has seeked to.
         */
        public int seek(int frameNumber) {
            active = true;

            // clear the End-of-media flag to ensure that
            // a consecutive start call will start the player
            // from the seek position and not from the first
            // frame.
            EOM = false;

            if (frameNumber < 0) {
                frameNumber = 0;
            } else if (frameNumber >= frameTimes.size()) {
                frameNumber = frameTimes.size() - 1;
            }

            long time = mapFrameToTime(frameNumber);

            try {
                doSetMediaTime(time);
            } catch (MediaException e) {
                // nothing to do
            }

            active = false;

            return frameNumber;
        }


        /**
         * Skip a given number of frames from the current position.
         * The media time of the <code>Player</code> will be updated to
         * reflect the new position set.
         * <p>
         * This method can be called on a stopped or started 
         * <code>Player</code>.
         *
         * If the <code>Player</code> is in the <i>Started</i> state,
         * the current position is changing.  Hence,
         * the frame actually skipped to will not be exact.
         * <p>
         * If the <code>Player</code> is
         * in the <i>Started</i> state, this method may cause the
         * <code>Player</code> to change states.  If that happens, the
         * appropriate transition events will be posted.
         * <p>
         * If the given <code>framesToSkip</code> will cause the position to
         * extend beyond the first or last frame, <code>skip</code> will
         * jump to the first or last frame respectively.
         *
         * @param framesToSkip the number of frames to skip from the current
         * position.  If framesToSkip is positive, it will seek forward
         * by framesToSkip number of frames.  If framesToSkip is negative,
         * it will seek backward by framesToSkip number of frames.
         * e.g. skip(-1) will seek backward one frame.
         * @return the actual number of frames skipped.
         */
        public int skip(int framesToSkip) {
            active = true;

            // clear the End-of-media flag to ensure that
            // a consecutive start call will start the player
            // from the seek position and not from the first
            // frame.
            EOM = false;

            int frames_skipped = 0;

            int oldFrame = frameCount - 1;

            if (oldFrame < 0) {
                oldFrame = 0;
            } else if (oldFrame >= frameTimes.size()) {
                oldFrame = frameTimes.size() - 1;
            } 

            long newFrame = (long)oldFrame + framesToSkip;

            if (newFrame < 0) {
                newFrame = 0;
            } else if (newFrame >= frameTimes.size()) {
                newFrame = frameTimes.size() - 1;
            } 
                        
            long time = mapFrameToTime((int)newFrame);

            try {
                doSetMediaTime(time);

                frames_skipped = (int) (newFrame  - oldFrame);
            } catch (MediaException e) {
                // nothing to do
            }

            active = false;

            return frames_skipped;
        }

        /**
         * Converts the given frame number to the corresponding media time.
         * The method only performs the calculations. It does not
         * position the media to the given frame.
         *
         * @param frameNumber the input frame number for the conversion.
         * @return the converted media time in microseconds for the 
         * given frame. If the conversion fails, -1 is returned.
         */
        public long mapFrameToTime(int frameNumber) {
            if (frameNumber < 0 || frameNumber >= frameTimes.size()) {
                return -1;
            }

            return (long) (frameToTime(frameNumber) * rateControl.getRate() / 100000L);
        }

        /**
         * Converts the given media time to the corresponding frame number.
         * The method only performs the calculations.  It does not
         * position the media to the given media time.
         * <p>
         * The frame returned is the nearest frame that has a media time
         * less than or equal to the given media time.
         * <p>
         * <code>mapTimeToFrame(0)</code> must not fail and must
         * return the frame number of the first frame.
         *
         * @param mediaTime the input media time for the
         * conversion in microseconds.
         * @return the converted frame number for the given media time.
         * If the conversion fails, -1 is returned.
         */
        public int mapTimeToFrame(long mediaTime) {         
            if (mediaTime < 0 || mediaTime > duration) {
                return -1;
            }

            long time = mediaTime * rateControl.getRate() / 100000;

            return (int) timeToFrame(time);
        }

        public boolean isActive() {
            return active;
        }
    }

    /**
     * Inner class implementing the rate control interface.
     * 
     */
    class RateCtrl implements RateControl {
        /* the playback rate in 1000 times the percentage of the
         * actual rate.
         */
        private int rate;

        /* the minimum playback rate */
        private final int MIN_PLAYBACK_RATE = 10000; // 10%

        /* the maximum playback rate */
        private final int MAX_PLAYBACK_RATE = 200000; // 200%


        /**
         * The constructor of RateCtrl.
         */
        RateCtrl() {
            rate = 100000; // normal speed, 100%
        }

        /**
         * Sets the playback rate.
         *
         * The specified rate is 1000 times the percentage of the
         * actual rate. For example, to play back at twice the speed, specify
         * a rate of 200'000.<p>
         *
         * The <code>setRate</code> method returns the actual rate set by the
         * <code>Player</code>.  <code>Player</code> should set their rate
         * as close to the requested
         * value as possible, but are not required to set the rate to the exact
         * value of any argument other than 100'000. A <code>Player</code>
         * is only guaranteed to set
         * its rate exactly to 100'000.
         * If the given rate is less than <code>getMinRate</code>
         * or greater than <code>getMaxRate</code>,
         * the rate will be adjusted to the minimum or maximum
         * supported rate respectively.
         * <p>
         * If the <code>Player</code> is already
         * started, <code>setRate</code> will immediately take effect.
         *
         * @param millirate The playback rate to set. The rate is given in
         *        a &quot;milli-percentage&quot; value.
         * @return The actual rate set in &quot;milli-percentage&quot;.
         * @see #getRate
         */
        public int setRate(int millirate) {         
            if (millirate < MIN_PLAYBACK_RATE) {
                rate = MIN_PLAYBACK_RATE;
            } else if (millirate > MAX_PLAYBACK_RATE) {
                rate = MAX_PLAYBACK_RATE;
            } else {
                rate = millirate;
            }

            return rate;
        }

        /**
         * Gets the current playback rate.
         *
         * @return the current playback rate in &quot;milli-percentage&quot;.
         * @see #setRate
         */
        public int getRate() {
            return rate;
        }

        /**
         * Gets the maximum playback rate supported by the <code>Player</code>.
         *
         * @return the maximum rate in &quot;milli-percentage&quot;.
         */     
        public int getMaxRate() {
            return MAX_PLAYBACK_RATE;
        }
        
        /**
         * Gets the minimum playback rate supported by the <code>Player</code>.
         *
         * @return the minimum rate in &quot;milli-percentage&quot;.
         */
        public int getMinRate() {
            return MIN_PLAYBACK_RATE;
        }
    }
    
}

