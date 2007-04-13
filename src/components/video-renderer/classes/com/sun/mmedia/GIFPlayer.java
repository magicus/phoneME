/*
 * Copyright  1990-2006 Sun Microsystems, Inc. All Rights Reserved.
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

    protected void doDeallocate() {
        playThreadFinished();
        
        stopped = false;
        referenceFrame = null;
    }

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

    private void renderFrame() {
        if (referenceFrame != null)
            videoRenderer.render(referenceFrame);
    }
    
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

    private int readShort(byte data[], int offset) {
        int lo = data[offset] & 0xff;
        int hi = data[offset + 1] & 0xff;
        
        return lo + (hi << 8);
    }

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

    private byte[] oneByte = new byte[1];

    private int readUnsignedByte() throws IOException {
        if (stream.read(oneByte, 0, 1) == -1)
            throw new IOException();

        return oneByte[0] & 0xff;
    }

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

        public int seek(int frameNumber)
        {
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

        public long mapFrameToTime(int frameNumber) {
            if (frameNumber < 0 || frameNumber >= frameTimes.size()) {
                return -1;
            }

            return (long) (frameToTime(frameNumber) * rateControl.getRate() / 100000L);
        }

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

    class RateCtrl implements RateControl {
        /* the playback rate in 1000 times the percentage of the
         * actual rate.
         */
        private int rate;

        /* the minimum playback rate */
        private final int MIN_PLAYBACK_RATE = 10000; // 10%

        /* the maximum playback rate */
        private final int MAX_PLAYBACK_RATE = 200000; // 200%

        RateCtrl() {
            rate = 100000; // normal speed, 100%
        }

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

        public int getRate() {
            return rate;
        }

        public int getMaxRate() {
            return MAX_PLAYBACK_RATE;
        }
        
        public int getMinRate() {
            return MIN_PLAYBACK_RATE;
        }
    }
    
}

