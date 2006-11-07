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

import javax.microedition.media.*;
import javax.microedition.media.control.*;
import java.io.IOException;
import com.sun.mmedia.Configuration;

import java.util.Hashtable;

/**
 * This class implements the amr (audio/amr) audio player.
 */
public class AMRPlayer extends BasicPlayer implements Runnable {
    //private static final boolean debug = false;

    private String fourcc_encoding;
    private Buffer inBuffer;
    private Buffer bufferObject;    
    private PCMAudioOut renderer;
    private PCMAudioOut altRenderer;
    private AMRDecoder decoder;

    // private boolean notSeekable;

    private String contentType;

    private static final int header_pound_bang_A_M = 0x2321414d;
    private static final short header_R_newline = 0x520A;
    private static final short header_R_hyphen = 0x522D;
    private static final short header_WB = 0x5742;
    private static final byte header_newline = 0xA;
    private static final short header_R_underscore = 0x525F;
    private static final int header_MC1_period = 0x4D43312E;
    private static final short header_0_newline = 0x300A;
    private static final byte header_underscore = 0x5F;


    // Frame type 0-7 are speech bits = [95,103,118,134,148,159,204,244]
    // Frame type 8 = AMR Comfort Noise Frame, 39 (Class A) bits
    // Frame type 9 = GSM-EFR Comfort Noise Frame, 43 (Class A) bits
    // Frame type 10 = IS0641 Comfort Noise Frame, 38 (Class A) bits
    // Frame type 11 = PDC-EFR Comfort Noise Frame, 37 (Class A) bits
    // Frame type 12-14: For future use
    // Frame type 15: No Data

    private static final int[]
	framesizes_nb = {
	    13, /*  4.75 kbit/sec */
	    14, /*  5.15 kbit/sec */
	    16, /*  5.90 kbit/sec */
	    18, /*  6.70 kbit/sec */
	    20, /*  7.40 kbit/sec */
	    21, /*  7.95 kbit/sec */
	    27, /* 10.20 kbit/sec */
	    32, /* 12.20 kbit/sec */

	     6, /* AMR Comfort Noise */
	     7, /* GSM-EFR Comfort Noise */
	     6, /* ISO641 Comfort Noise */
	     6, /* PDC-EFR Comfort Noise */

	     0, 0, 0, /* For future use */
	     1  /* No data */
	};

    private static final int[]
	framesizes_wb = {
	    18, /*  6.60 kbit/sec */
	    24, /*  8.85 kbit/sec */
	    33, /* 12.65 kbit/sec */
	    37, /* 14.25 kbit/sec */
	    41, /* 15.85 kbit/sec */
	    47, /* 18.25 kbit/sec */
	    51, /* 19.85 kbit/sec */
	    59, /* 23.05 kbit/sec */
	    61, /* 23.85 kbit/sec */
	     6, /* Comfort Noise */
	     0, 0, 0, 0, /* For future use */
 	     1, /* Speech Lost */
	     1  /* No data */
	};

    private int[] framesizes = framesizes_nb;
    /* corresponds to the highest bitrate */
    private int maxFrameSize = framesizes_nb[7];
    // private int aFrameSize = 0;
    // private boolean constantFrameSize = false;
    private int numFrames = -1;
    // Each frame has a 20 msec duration
    private static final int FRAME_DURATION = 20;
    private static final int FRAME_DURATION_USEC = 20000;
    private static final int numOfFramesToDecode = 1;
    private static int sleepTime =
	(numOfFramesToDecode * (FRAME_DURATION / 2)); // half buf length in milliseconds
    private int maxFrameNumber = -1;
    private int currentFrameNumber = 0;
    private int currentMaxFrameNumber = 0;

    private long contentLength;
    private int seekType;

    private VolCtrl vc;
    private RecordCtrl recordControl;
    private MetaCtrl meta;

    private int sampleRate; 
    private int channels;
    private int sampleSizeInBits;
    private long lastPos; // 0;
    private long origin; // 0;
 
    private boolean started;
    private boolean interrupted;
    private Thread playThread;
    private Object playLock = new Object();
    private long startpt;
    private long endpt = Long.MAX_VALUE;
    private boolean isBigEndian = false;
    private boolean isSigned;

    private int bufLen;
    private byte [] buffer;

    private long duration = TIME_UNKNOWN;
    private Object pauseLock = new Object();
    private boolean canPause = true;

    private Object waveLock = new Object();

    private boolean isCapturePlayer = false;

    /**
     * AMR NB mime type.
     */
    private final String AMR = "audio/amr";


    /**
     * AMR WB mime type.
     */
    private final String AMR_WB = "audio/amr-wb";

    /**
     * A lock object to guard reading and seeking at the same time.
     */
    private final Object readLock = new Object();

    /**
     * Where the media time was last set.
     * IMPL_NOTE: make this private. currently needed by derived class AMRPlayer
     */
    private long mediaTimeCache;	// 0 in microseconds.

    /**
     * Return the contentype.
     *
     * @return the wav content type.
     */
    public String getContentType() {
        chkClosed(true);
        return contentType;
    }


    int getBufferSize() {
    	return 0;
    }


    /**
     * Parse the input, realize the player
     */
    protected void doRealize() throws MediaException {
        seekType = stream.getSeekType();
        contentLength = stream.getContentLength();

        if (contentLength == -1)
            contentLength = Long.MAX_VALUE;
        try {
            source.start();

            if (!readHeader()) {
                throw new MediaException("malformed amr media");
            }

            source.stop();
        } catch (IOException e) {
            throw new MediaException(e.getMessage());
        }

        vc = new VolCtrl(this);

        sleepTime = (numOfFramesToDecode * (FRAME_DURATION / 2)); // half buf length in milliseconds
        decoder = new AMRDecoder(fourcc_encoding, sampleRate, channels);

        inBuffer = new Buffer();
        inBuffer.data = new byte[maxFrameSize * channels * numOfFramesToDecode];
        // outBuffer = new Buffer();

        bufferObject = new Buffer();
        // Update audiocap.c:AUDIOCAP_BUFFER_SIZE if you modify bufLen
        // bufLen = (bytesPerSecond / 32) & ~3; //1/32 second
        int bufferSize = getBufferSize();
        if (bufferSize > 0) {
            bufferObject.data = new byte[bufferSize];
            bufferObject.length = bufferSize;
        }
        bufferObject.offset = 0;
        sleepTime = 16; // half of bufLen

        /*
        if (debug) {
            System.out.println("encoding " + fourcc_encoding);
            System.out.println("sampleRate " + sampleRate);
            System.out.println("channels " + channels);
            System.out.println("input buffer size is " +
                       maxFrameSize * channels * numOfFramesToDecode);
        }
         */
    }

    public long doGetDuration() {
    /*
     * TBD: 
     * Current version of AMR player is not able to get duration of its media
     * from header, decore or whatever else before start().
     * It == -1 (TIME_UNKNOWN) until end of media, and only then it is updated.
     */
        return duration;
    }

    static String parseErr = "Malformed amr media";

    /**
     * Parse the amr header
     */
    // Include support for AMR-WB also here
    private boolean readHeader() throws IOException {
        sampleRate = 8000;
        channels = 1;

        /*
          "#!AMR\n"  [#!AM, R\n  ==> 0x2321414d, 0x520a)

          "#!AMR_MC1.0\n" [#!AM, R_, MC1., 0\n" ==> 0x2321414d, 525F, 4D43312E, 300a]

          "#!AMR-WB\n" [#!AM, R-, WB, \n ==> 0x2321414d, 0x522d, 0x5742, 0a]

          "#!AMR-WB_MC1.0\n" [#!AM, R-, WB, _, MC1., 0\n ==>
                              0x2321414d, 0x522d, 0x5742, 0x5F, 0x4D43312E, 0x300a]
        */

        int header_32 = readInt();
        if ( header_32 != header_pound_bang_A_M) // '#!AM'
            throw new IOException(parseErr);

        short header_16 = readShort();
        if (header_16 == header_R_newline) { // 'R\n'
            // AMR narrow band mono: sampleRate:8000, channels:1
            fourcc_encoding = "amr";
        } else if (header_16 == header_R_underscore) {
            // Could be AMR narrow band, multi channel OR
            // Could be Illegal AMR header
            int header_32a = readInt();
            short header_16a = readShort();
            if ( (header_32a != header_MC1_period) ||
                 (header_16a != header_0_newline) )
                throw new IOException(parseErr);
            // Is AMR narrow band, multi channel
            fourcc_encoding = "amr";
            channels = readInt() & 0xF;
        } else if (header_16 == header_R_hyphen) {
            // Could be AMR wide band, single channel OR
            // Could be AMR wide band, multi channel OR
            // Could be Illegal AMR header
            short header_16b = readShort();
            if (header_16b != header_WB)
                throw new IOException(parseErr);
            byte header_8 = readByte();
            if (header_8 == (byte) header_newline) {
                // Is AMR wide band mono: sampleRate:16000, channels:1
            } else if (header_8 == (byte) header_underscore) {
                // Could be AMR wide band, multi channel OR
                // Could be Illegal AMR header
                int header_32c = readInt();
                short header_16c = readShort();
                if ( (header_32c != header_MC1_period) ||
                     (header_16c != header_0_newline) )
                    throw new IOException(parseErr);
                // Is AMR wide band multichannel: sampleRate:16000
                channels = readInt() & 0xF;
            } else {
                throw new IOException(parseErr);
            }
            fourcc_encoding = "amr-wb";
            sampleRate = 16000;
            framesizes = framesizes_wb;
            /* corresponds to the highest bitrate */
            maxFrameSize = framesizes_wb[8];
        } else {
            throw new IOException(parseErr);
        }

        startpt = stream.tell(); // start of data

        contentType = (fourcc_encoding.equals("amr")) ? AMR : AMR_WB;

        return true;
    }


    /**
     * Get the resources ready.
     */
    protected void doPrefetch() throws MediaException {
        sampleSizeInBits = decoder.getBitsPerSample();
        isBigEndian = decoder.isBigEndian();
        isSigned = decoder.isSigned();

        // Open the audio device.
        synchronized (waveLock) {
            if (renderer == null) {
                renderer = (altRenderer == null) 
                    ? Configuration.getConfiguration().getAudioRenderer() 
                    : altRenderer; 

                if (!renderer.open(sampleRate, sampleSizeInBits, channels, 
                                isSigned, isBigEndian)) {
                    throw new MediaException("cannot open audio device");
                }

                if ( vc.getLevel() == -1 ) {
                    vc.setLevel(renderer.getVolume());
                } else {
                    renderer.setVolume(vc.getLevel());
                }
            }
        }

    	doSetLevel( vc.isMuted() ? 0 : vc.getLevel() );
    }


    /**
     * Start the playback.
     */
    protected boolean doStart() {
        if (started) {
            return true;
        }

        started = true;

        try {
            source.start();
        } catch (IOException ioe) {
            return false;
        }

        // Start the playback loop.
        synchronized (playLock) {
            if (playThread == null) {
                playThread = new Thread(this);
                playThread.start();
            } else {
                playLock.notifyAll();
            }
        }

        if (recordControl != null) {
            recordControl.playerStart();
        }

        renderer.resume();

        return true;
    }

    /**
     * Stop the playback loop.
     */
    protected void doStop() throws MediaException {
        if (!started)
            return;

        // If it's a custom DataSource, stop may fail.
        // We'll rethrow a MediaException to the app.
        try {
            source.stop();
        } catch (IOException ioex) {
            throw new MediaException(ioex.getMessage());
        }

        started = false;
        synchronized (pauseLock) {
            while (!canPause)
                try {
                    pauseLock.wait();
                } catch (InterruptedException ie){
                }
            renderer.pause();
            pauseLock.notifyAll();
        }
    }

    /**
     * Deallocate the exclusing resource.
     */
    protected void doDeallocate() {
        // If the playThread had not been started, we'll need
        // to explicitly close the device.
        if (state == PREFETCHED && playThread == null) {
            renderer.close();
            return;
        }

        // Interrupt the playback loop.
        synchronized (playLock) {
            interrupted = true;
            // Wake up the run loop if it was stopped.
            playLock.notifyAll();

            // Wait for the playback loop to completely stop before 
            // returning.  There's a maximum wait limit set here in 
            // case anything goes wrong.
            if (playThread != null) {
                try {
                    playLock.wait(5000);
                } catch (InterruptedException ie) {
                }
            }
        }
    }

    /**
     * Close the player.
     */
    protected void doClose() {
        // Deallocate would have been called before this.
        // So all the resources should have been released.
        if (recordControl != null) {
            recordControl.close();
        }

        cleanup();
    }

    private void cleanup() {
        if (decoder != null) {
            decoder.close();
            decoder = null;
        }

        if (inBuffer != null) {
            inBuffer.data = null;
            inBuffer = null;
        }
    }

    private String errMsg = null;

    private boolean doProcess() {
        int len = 0, wlen = 0;

        synchronized (readLock) {
            try {
                len = readDecodedBytes(bufferObject);
            } catch (IOException ioe) {
                runErr = ioe.getMessage();
                return false;
            }
        }

        if (len < 1) {
            synchronized (pauseLock) {
                canPause = false;
                while (renderer.drain() == 0) {
                    /**
                     * Check stop time.
                     * On some systems, Zaurus in particular,
                     * a large amount of data are buffered in
                     * the audio device even after EOM.  So we
                     * need to test the stop time here to make
                     * sure that we report STOPPED_AT_TIME event
                     * even after we are done with the input.
                     * A number of TCK tests demand this.
                     *
                     * There is a bug here, however.  If stop
                     * time is reached here, restarting the
                     * Player will not resume from where it
                     * left off.  Rather, it will start from EOM.
                     */
                    if ((stopTime != StopTimeControl.RESET) &&
                        (stopTime < doGetMediaTime()) ) {
                        canPause = true;
                        try {
                            doStop();
                        } catch (MediaException e) {
                            // Not much we can do here.
                        }
                        satev();
                        pauseLock.notifyAll();
                        return true;
                    }

                    try {
                        // Give a little time for drain.
                        pauseLock.wait(20);
                    } catch (InterruptedException e) {
                    }
                }

                canPause = true;
                pauseLock.notifyAll();
            }

            Thread.yield();
            started = false;
            long time = getMediaTime();
            if (duration == TIME_UNKNOWN) {
                duration = time;
                // send duration_updated event
                sendEvent(PlayerListener.DURATION_UPDATED, new Long(duration));
            }
            sendEvent(PlayerListener.END_OF_MEDIA, new Long(time));
            return true;

        } else {

            if (!isCapturePlayer) {
                synchronized (pauseLock) {
                    canPause = false;
                    while ((wlen = renderer.write(bufferObject.data,
                                     bufferObject.offset, len)) == 0 ) {
                        try {
                        // pauseLock.wait(16); // sleep half of the bufLen
                        pauseLock.wait(sleepTime); // sleep half of the bufLen
                        } catch  (Exception ex) {}
                    }
                    canPause = true;
                    pauseLock.notifyAll();
                }
            } else {
                wlen = len;
            }

            if (wlen == -1)
                return false;

            Thread.yield();

            if ((stopTime != StopTimeControl.RESET) && 
                (stopTime < doGetMediaTime()) ) {
                try {
                    doStop();
                } catch (MediaException e) {
                    // Not much we can do here.
                }
                satev();
            }
            return true;
        }
    }
        

    /**
     * Main process loop driving the media flow.
     */
    public void run() {
        boolean statusOK = true;

        while (true) {
            while (!interrupted && started && statusOK) {
                statusOK = doProcess();
                Thread.yield();
            }

            synchronized (playLock) {
                if (interrupted || !statusOK)
                    break;

                try {
                    playLock.wait();
                } catch (InterruptedException ie) {
                }
            }
        } // end of while (true)

        // Control will come here if player is deallocated (interrupted is true)
        // or if a non-recoverable error occurs (statusOK is false)

        if (!statusOK) {
            // non-recoverable Error occured
            try {
                doStop();
            } catch (MediaException me) {
            }
            closeDevice();
            doClose();
            state = CLOSED;
        }
        interrupted = started = false;

        synchronized (playLock) {
            playThread = null;
            // Notify the blocking deallocate that we are done with
            // the process loop.
            playLock.notifyAll();
        }

        if (!statusOK) {
            if (source != null)
            source.disconnect();

            sendEvent(PlayerListener.ERROR, runErr);
        }
    }

    private void closeDevice() {
        synchronized(waveLock) {
            if (renderer != null) {
            renderer.flush();
            mediaTimeCache = renderer.getMediaTime() * 1000;
            renderer.close();
                    renderer = null;
            return;
            }
        }
    }

    /**
     * The worker method to actually set player's media time.
     *
     * @param now The new media time in microseconds.
     * @return The actual media time set in microseconds.
     * @exception MediaException Thrown if an error occurs
     * while setting the media time.
     */
    protected long doSetMediaTime(long now) throws MediaException {
        // 	if (notSeekable)
        // 	    throw new MediaException("stream is not seekable");

        long ret;
        int frameNumberToSeek = (int) (now / FRAME_DURATION_USEC);

        try {
            synchronized (readLock) {
            if ( maxFrameNumber != -1 ) {
                // maxFrameNumber is known
                if (frameNumberToSeek >= maxFrameNumber) {
                throw new MediaException("cannot set media time past end of media");
                }
            } else if (frameNumberToSeek > currentMaxFrameNumber)
                // maxFrameNumber is unknown, cannot seek forward to
                // frames that have not been played
                throw new MediaException("cannot set media time");

            if (getState() == STARTED)
                doStop();

            if (frameNumberToSeek != currentFrameNumber)
                doSeek(frameNumberToSeek);
            ret = frameNumberToSeek * FRAME_DURATION_USEC;
            currentFrameNumber = frameNumberToSeek;
            }
            if (getState() >= PREFETCHED) {
                renderer.flush();
                renderer.setMediaTime(ret/1000);
            }
            if (getState() == STARTED)
            doStart();

        } catch (Exception e) {
            throw new MediaException(e.getMessage());
        }

        mediaTimeCache = ret;
        return ret;
    }


    private void doSeek(int frameNumberToSeek) throws IOException {
        synchronized (readLock) {
            if (frameNumberToSeek > currentFrameNumber) {
                // forward seek
                skipFrame(frameNumberToSeek - currentFrameNumber);
            } else {
                // backward seek
                seekStrm(0); // seek to start
                seekStrm(startpt);
                skipFrame(frameNumberToSeek);
            }
        }
    }

    private void  skipFrame(int framesToSkip) throws IOException {
        for (int i = 0; i < framesToSkip; i++) {
            int len;
            int type;
            int size;

            for (int j = 0; j < channels;) {
                len = readBytes(intArray, 0, 1);
                if (len < 1)
                    throw new IOException("Error setting media time");
                type = (intArray[0] >> 3) & 0xf;
                size = framesizes[type];
                if (size > 0) {
                    skipFully(size-1);
                    j++;
                }
            }
        }
    }

    public long doGetMediaTime() {
        long mt = 0; // in microseconds
        synchronized (waveLock) {
            if (sampleRate == 0)
                return 0;
            
            mt = (state >= PREFETCHED) 
                ? ((long) renderer.getMediaTime()) * 1000
                : mediaTimeCache;
        }
        // Media time is in micro-seconds
        return mt;
    }

    /**
     * The worker method to actually obtain the control.
     *
     * @param type the class name of the <code>Control</code>.
     * @return <code>Control</code> for the class or interface
     * name.
     */
    protected Control doGetControl(String type) {
        if ((getState() != UNREALIZED) && 
            type.startsWith(BasicPlayer.pkgName)) {
            
            type = type.substring(BasicPlayer.pkgName.length());
            
            if (type.equals(BasicPlayer.recName)) {
                return null; // RecordControl is not supported
            } else if (type.equals(BasicPlayer.stcName)) {
                return this;
            } else if (type.equals(BasicPlayer.vocName)) {
                return vc;
            } else if (type.equals(BasicPlayer.mdcName)) {
                // this may return null - but only in cases where there is 
                // no meta data present.
                return meta;
            }
        }
    	return null;
    }


    /**
     * error string
     */
    private String runErr = null;

    private byte [] intArray = new byte[4];

    byte readByte() throws IOException {
        stream.read(intArray, 0, 1);
        return intArray[0];
    }

    private int readBytes(byte[] array, int offset, int num) 
        throws IOException {
        long available = endpt - stream.tell();
        if (available <= 0) {
            return -1;
        }

        if (num > available) {
            num = (int) available;
        }

        // be fault-tolerant here: do not use readFully
        return stream.read(array, offset, (int) num);
    }    

    int readDecodedBytes(Buffer outBuffer) throws IOException {
        int len = 0, wlen = 0;

        int dataSize = 0;

        try {
            int offset = 0;
            for (int i = 0; i < numOfFramesToDecode; i++) {
            int ch = 0;
            for (; ch < channels; ) {
                len = readBytes(inBuffer.data, offset, 1);
                if (len < 1) {
                    break;
                }
                if ( (inBuffer.data[offset] & 0x83) > 0 ) {
                    // Malformed AMR data.
                    throw new IOException(parseErr + 
                            ": padding bits should be zero");
                }
                // If bit 4 is not 1 (SPEECH_BAD), the data is corrupted
                // and the decoder may try to use it for error concealment
                int type = (inBuffer.data[offset] >> 3) & 0xf;

                /*
                if (debug) {
                    System.out.println("frame byte is " +
                       Integer.toHexString(
                       (int) (inBuffer.data[offset] & 0xFF)));
                    System.out.println("type is " + type);
                }
                 */
                offset++;
                int size = framesizes[type];
                size--; // Subtract 1 byte for frame type
                if (size > 0) {
                    len = readBytes(inBuffer.data, offset, size);
                    // Handle size != len
                    if (len != size) {
                        // Error
                        System.err.println("ERROR: asked, got " + size + 
                                " , " + len);
                        throw new IOException("Error playing amr media");
                    }
                    offset += size;
                    dataSize += (1 + size); // add the header byte
                    ch++;
                } else if (size == 0) {
                    // No Data or Speech lost
                } else {
                    // Malformed AMR data.
                    System.err.println(parseErr);
                    throw new IOException(parseErr);
                }
            }
            // It is an error if you got partial channels of the frame.
            if ( (ch != 0) && (ch != channels) ) {
                System.err.println(parseErr);
                throw new IOException(parseErr);
            }
            currentFrameNumber++;
            if (currentFrameNumber > currentMaxFrameNumber)
                currentMaxFrameNumber = currentFrameNumber;
            }
        } catch (Exception e) {
            runErr = e.getMessage();
            cleanup();
            throw new IOException(runErr);
        }


        if ( (len < 1) && (dataSize == 0) ) {
            maxFrameNumber = currentFrameNumber;
            return len;
        }

        inBuffer.offset = 0;
        inBuffer.length = dataSize;
        try {
            decoder.process(inBuffer, outBuffer);
        } catch (MediaException e) {
            cleanup();
            throw new IOException(e.getMessage());
        }
        return outBuffer.length;
    }


    /**
     * Read a big-endian integer from source stream
     * @return the integer read.
     * @throws IOExeption if there is an error
     * Would be nice to consolidate little endian (for wav) and big endian
     * for amr in one method.
     */
    private int readInt() throws IOException {
        if (readFully(intArray, 0, 4) < 4)
            throw new IOException(parseErr);
        return ((intArray[0] & 0xFF) << 24) |
            ((intArray[1] & 0xFF) << 16) |
            ((intArray[2] & 0xFF) << 8) |
            (intArray[3] & 0xFF);
    }

    /**
     * Read a big-endian short from source stream
     * @return the short read
     * @throws IOException if there is an error
     */
    private short readShort() throws IOException {
        if (readFully(intArray, 0, 2) < 2)
            throw new IOException(parseErr);
        return (short) (((intArray[0] & 0xFF) << 8) |
            (intArray[1] & 0xFF));
    }

    public int doSetLevel(int vol) {
        // 0 <= vol <= 100
        synchronized (waveLock) {
            if (state >= PREFETCHED) {
            // renderer's setVolume takes care of converting 100 to 65535
            renderer.setVolume(vol);
            }
        }
        return vol;
    }
    
    public int getAudioType() { 
    	return AUDIO_PCM; 
    }
    
    public void setOutput(Object output) {
        altRenderer = (PCMAudioOut)output;
    }
    
    public Object getOutput() {
        return (Object) (altRenderer == null ? renderer : altRenderer); 
    }
}
