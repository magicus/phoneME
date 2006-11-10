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


import java.util.Hashtable;


/* IMPL_NOTE:
 * - move ownership of RateControl, VolumeControl, etc. to
 *   the audio renderer. That will enable sharing them with
 *   other players that use the renderer.
 */

/**
 * This class implements the wav (audio/x-wav) audio player.
 */
public class WavPlayer extends BasicPlayer implements Runnable {
    /**
     * the duration of this player
     */
    long duration = TIME_UNKNOWN;

    /**
     * Audio format parameters: sampleRate, channel number, sample size
     * Package private for WavRecordCtrl.
     */
    int sampleRate, channels, sampleSizeInBits;

    boolean isBigEndian = false;

    boolean isSigned;

    /**
     * Extended audio format parameters
     */
    private int encoding, samplesPerBlock;

    /**
     * byte rate and alignment
     */
    private int bytesPerSecond, blockAlign;

    /**
     * the audio renderer
     */
    private PCMAudioOut renderer;
    private PCMAudioOut altRenderer;

    /**
     * waveout lock obj
     */
    private Object waveLock = new Object();

    /**
     * play thread obj
     */
    private Thread playThread;

    /**
     * a status flag indicating whether this player is started
     */
    private boolean started;

    /**
     * a status flag indicating whether this player has been deallocated
     */
    private boolean interrupted;

    /**
     * a play lock obj
     */
    private Object playLock = new Object();

    /**
     * In source stream, the start position of pcm data
     */
    long startpt;

    /**
     * where the pcm data ends in source stream
     */
    private long endpt = Long.MAX_VALUE;

    /**
     * Where the media time was last set.
     * IMPL_NOTE: make this private. currently needed by derived class AMRPlayer
     */
    long mediaTimeCache;        // 0 in microseconds.

    /**
     * The buffer length to read from source stream every time.
     */
    private int bufLen;

    Buffer bufferObject;

    int sleepTime;

    /**
     * a pause lock obj
     */
    final Object pauseLock = new Object();

    /**
     * A lock object to guard reading and seeking at the same time.
     */
    final Object readLock = new Object();

    /**
     * a flag indicating if this player could be paused
     */
    private boolean canPause = true;

    /**
     * Volume control
     */
    private VolCtrl vc;

    
    private long contentLength;
    private boolean fmtSeen;
    private boolean dataSeen;
    private static Hashtable mapHash;
    private MetaCtrl meta;
    
    private RecordCtrl recordControl;    
    
    private WavRateCtrl rateControl;
    
    /** if this is a player created with capture://audio locator */
    private boolean isCapturePlayer = false;
    
    /**
     * Return the contentype.
     *
     * @return the wav content type.
     */
    public String getContentType() {
        chkClosed(true);
        return DefaultConfiguration.MIME_AUDIO_WAV;
    }

    /**
     * Parse the input, realize the player
     */
    protected void doRealize() throws MediaException {
        
        contentLength = stream.getContentLength();
        if (contentLength == -1) {
            contentLength = Long.MAX_VALUE;
        }
        if ( (source != null) && (source.getLocator() != null) ) {
            isCapturePlayer = source.getLocator().startsWith("capture:");
        }            
        try {            
            source.start();                
            readHeader();
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
        } catch (IOException e) {
            throw new MediaException("Failed to realize Player: " + e.getMessage());
        }
        vc = new VolCtrl(this);
    }

    /*
     * Subclasses may return 0 if the required size is allocated
     * by the decoder
     */
    int getBufferSize() {
        return (bytesPerSecond / 32) & ~3; //1/32 second
    }

    // JAVADOC COMMENT ELIDED
    public long doGetDuration() {
        return duration;
    }

    static String parseErr = "Malformed wave media";
    
    /**
     * Parse the Wave file header.  This is a more elaborate
     * parser to extract the wave meta data.
     */
    void readHeader() throws IOException {
        if (readInt() != 0x46464952 /*RIFF*/) {
            throw new IOException(parseErr + ": expected 'RIFF'");
        }
        int length = readInt();
    
        if (readInt() != 0x45564157 /*WAVE*/) {
            throw new IOException(parseErr + ": expected 'WAVE'");
        }
        length += 8; // Add 4 bytes for RIFF and WAVE fields
    
        // The required chunks 'fmt ' and 'data' are supported.
        // The optional INFO List Chunk is supported
        // There are no restrictions upon the order of the chunks within a WAVE file,
        // with the exception that the Format chunk must precede the Data chunk.
    
        // Note if calling tell() is expensive, we can keep track of
        // of the currentPos
        int fourcc;
    
        try {
            long currentPos;
            while ((currentPos = stream.tell()) < contentLength) {
                fourcc = readInt();
                int size = readInt();        
                if (fourcc == 0x20746D66 /*FMT*/) {
                    handleFMT(size);
                } else if (fourcc == 0x61746164 /*DATA*/) {
                    if (!fmtSeen) {
                        throw new IOException(parseErr + 
                                              ": data chunk before fmt chunk");
                    }
                    startpt = stream.tell(); // start of data
                    dataSeen = true;
                    if (size > 0) { // Non-live data
                        endpt = startpt + size;
                        duration = (size*1000000L)/bytesPerSecond;
                        if ( (endpt == contentLength) ||
                             (getSeekType() == NOT_SEEKABLE) ||
                             (endpt == length) ) {
                            // Parsing is done
                            // No need to skip past the DATA chunk because:
                            // 1) There are no more chunks after this DATA chunk;
                            // or
                            // 2) The stream is not seekable and therefore
                            //    if we seek past the data chunk we won't be
                            //    able to play the media.
                            return;
                        } else {
                            skipFully(size);
                            if ( (size & 0x1) > 0) {
                                // if data chunk size is odd, skip 1 byte to
                                // word align
                                skipFully(1);
                            }
                        }
                    } else {
                        return; // live data, header parsing done
                    }
                } else if (fourcc == 0x5453494C/*LIST*/) {
                    handleLIST(size);
                } else {
                    skipFully(size);
                }
            }
        } catch (IOException e) {
            // Shouldn't get IOException if content length is known
            // and the file is properly formatted.
            if (!(fmtSeen && dataSeen)) {
                throw new IOException(parseErr + ": no FMT and/or Data");
            }
        }
    
        // stream's seekType is either SEEKABLE_TO_START or RANDOM_ACCESSIBLE
        if (getSeekType() == RANDOM_ACCESSIBLE) {
            seekStrm(startpt);
        } else { // SEEKABLE_TO_START
            seekStrm(0); // seek to start
            skipFully((int)startpt);
        }
    
        // return true if manadatory chunks are present and in the right order
        if (!(fmtSeen && dataSeen)) {
            throw new IOException(parseErr + ": no FMT and/or Data");
        }
    }


    /**
     * Parses the FMT chunk.
     */
    private void handleFMT(int formatSize) throws IOException {
        // Handle Format chunk 'fmt '
        int remFormatSize = formatSize;
        encoding = readShort();
        channels = readShort();
        sampleRate = readInt();
        bytesPerSecond = readInt();
        blockAlign = readShort();
        sampleSizeInBits = readShort();
        if ( encoding != 0x0001/*WF_PCM*/
                  || (sampleSizeInBits != 8 && sampleSizeInBits != 16)
                  || (channels != 1 && channels != 2)) {
                 throw new IOException(
                        "only supports PCM 8/16-bit mono/stereo");
             }
    
        if (sampleRate > 48000 || sampleRate < 500) {
            throw new IOException("unsupported sample rate");
        }
        isSigned = sampleSizeInBits == 16;
        samplesPerBlock = -1;
        remFormatSize -= 16;
    
        if (remFormatSize > 0) {
            skipFully(remFormatSize);
        }
        // bytesPerSecond and blockAlign in the wave file is not accurate,
        // just a hint need to be recalculated.
        bytesPerSecond = sampleRate * channels * sampleSizeInBits / 8;
        blockAlign = (sampleSizeInBits / 8) * channels;
    
        fmtSeen = true;
        // bufLen = (bytesPerSecond / 32) & ~3; //1/32 second
        // buffer = new byte[bufLen];
    }


    /**
     * IART: Author
     * ICOP: Copyright
     * INAM: Title
     * ICRD: Date
     * ICMT: Comments
     */
    private void handleLIST(int size) throws IOException {
        int listtype = readInt();
        if (listtype != 0x4F464E49/*INFO*/) { // Only LIST INFO chunks are supported
            skipFully(size - 4);
            return;
        }
        size -= 4;
    
        // create MetaCtrl
        if (meta==null) {
            if (mapHash == null) {
            mapHash = new Hashtable(4);
            mapHash.put("IART",  MetaCtrl.AUTHOR_KEY);
            mapHash.put("ICOP",  MetaCtrl.COPYRIGHT_KEY);
            mapHash.put("ICRD",  MetaCtrl.DATE_KEY);
            mapHash.put("INAM",  MetaCtrl.TITLE_KEY);
            }
            meta=new MetaCtrl(mapHash);
        }
    
        int remainingSize = size;
        int length;
        String key;
        byte[] metabuffer = null;
        while (remainingSize > 0) {
            readFully(intArray, 0, 4); // Read the key
            key = new String(intArray);
            length = readInt();
            if ( (metabuffer == null) || (metabuffer.length < length) ) {
                metabuffer = new byte[length];
            }
            readFully(metabuffer, 0, length); // Add 1 to read the null terminator
            // metabuffer contains value
            meta.put(key, new String(metabuffer, 0, length));
    
            remainingSize -= (4 + 4 + length);
            if ( (length & 1) > 0 ) { // odd
                skipFully(1);
                remainingSize -= 1;
            }
        }
        metabuffer = null;
    }

    /**
     * Get the resources ready.
     */
    protected void doPrefetch() throws MediaException {
        // Open the audio device.
        synchronized (waveLock) {
            if (renderer == null) {
                renderer = (altRenderer == null) 
                    ? Configuration.getConfiguration().getAudioRenderer() 
                    : altRenderer; 

                if (!renderer.open(sampleRate, sampleSizeInBits, channels,
                          isSigned, isBigEndian)) {
                    throw new MediaException("audio device");
                }
                if ( vc.getLevel() == -1 ) {
                    vc.setLevel(renderer.getVolume());
                } else {
                    renderer.setVolume(vc.getLevel());
                }
                renderer.setMediaTime(mediaTimeCache/1000);                
                if (rateControl != null) {
                    rateControl.setRate(rateControl.getRate());
                }
            }
        }
    
    	doSetLevel( vc.isMuted() ? 0 : vc.getLevel() );
    }


    /**
     * Start the playback.
     * @return the status if the player has been successfully started.
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
                // set to almost MAX_PRIORITY
                playThread.setPriority(Thread.NORM_PRIORITY
                                       + ((Thread.MAX_PRIORITY
                                          - Thread.NORM_PRIORITY) * 4) / 5);
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
        if (!started) {
            return;
        }        
        // If it's a custom DataSource, stop may fail.
        // We'll rethrow a MediaException to the app.
        try {
            source.stop();
        } catch (IOException e) {
            throw new MediaException("Failed to stop Player: " + e.getMessage());
        }            
        started = false;
        synchronized (pauseLock) {
            while (!canPause) {
                try {
                    pauseLock.wait();
                } catch (InterruptedException ie) { 
                }
            }
            renderer.pause();
            pauseLock.notifyAll();
        }
    }

    private void closeDevice() {
        synchronized(waveLock) {
            if ( renderer != null) {
                renderer.flush();
                mediaTimeCache = renderer.getMediaTime() * 1000;
                renderer.close();
                renderer = null;
                return;
            }
        }
    }

    /**
     * Deallocate the exclusive resource.
     */
    protected void doDeallocate() {
        // Interrupt the playback loop.    
        
        if(playThread == null)
            return;
        
        synchronized (playLock) {
            interrupted = true;    
            // Wake up the run loop if it was stopped.
            playLock.notifyAll();    
            // Wait for the playback loop to completely stop before
            // returning.  There's a maximum wait limit set here in
            // case anything goes wrong.
            while (playThread != null) {
                try {
                    playLock.wait(100);
                } catch (InterruptedException ie) {
                    break;
                }
            }
            if (renderer != null)
                renderer.flush();
        }     
    }


    /**
     * Close the player.
     */
    protected void doClose() {
        if(state != CLOSED)
            closeDevice();
        if (bufferObject != null) {
            bufferObject.data = null;
            bufferObject = null;
        }
        if (recordControl != null) {
            recordControl.close();
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
        long ret = now;
        try {
            /* byte position in the stream. */
            long pp = (bytesPerSecond * now / 1000000L / blockAlign) *
            blockAlign + startpt;
            if (getState() == STARTED) {
                doStop();
            }
            synchronized (readLock) {
                if (getSeekType() == RANDOM_ACCESSIBLE) {
                    pp = seekStrm(pp);
                } else if (getSeekType() == SEEKABLE_TO_START &&
                                                               pp == startpt) {
                    seekStrm(0); // seek to start
                    skipFully((int)pp);
                } else {
                    throw new MediaException("stream is not seekable");
                }
        
            }
            /* convert pp to microseconds */
            ret = (pp - startpt) * 1000000 / bytesPerSecond;
    
            if (getState() >= PREFETCHED) {
                renderer.flush();
                renderer.setMediaTime(ret/1000);
            }
            if (getState() == STARTED) {
                doStart();
            }    
        } catch (Exception e) {
            throw new MediaException("Failed to set media time: " + e.getMessage());
        }
        mediaTimeCache = ret;
        return ret;
    }

    // JAVADOC COMMENT ELIDED
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
            
            if (type.equals(BasicPlayer.vocName)) {
                return vc;
            } else if (!isCapturePlayer && 
                    type.equals(BasicPlayer.racName)) {
                // don't provide RateControl for capture -- would need
                // to change speed of captured audio...
                if (rateControl == null) {
                    rateControl = new WavRateCtrl();
                }
                return rateControl;
            } else if (type.equals(BasicPlayer.stcName)) {
                return this;
            } else if (type.equals(BasicPlayer.mdcName)) {
                // this may return null - but only in cases where there is
                // no meta data present.
                return meta;
            } else if (type.equals(BasicPlayer.recName)) {
                // Check the media prefs.
                if (recordControl == null) {
                    recordControl = new WavRecordCtrl(this);
                }
                return recordControl;
            }
        }
        return null;
    }
    
    /**
     * error string
     */
    private String runErr = null;

    int readDecodedBytes(Buffer b) throws IOException {
        return readBytes(b.data, b.offset, b.length);
    }

    /**
     * Read the data from the source and write them to the waveout in native.
     * @return the status.
     */
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
                    } catch (InterruptedException ie) {
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
            synchronized (pauseLock) {
                canPause = false;
                do {
                    wlen = renderer.write(bufferObject.data, bufferObject.offset, len);
                    
                    if(wlen == 0)
                    {
                        try {
                            // pauseLock.wait(16); // sleep half of the bufLen
                            pauseLock.wait(sleepTime); // sleep half of the bufLen
                        } catch  (Exception ex) {
                        }
                    }
                } while(wlen == 0);
                 
                // wlen = renderer.write(buffer, 0, len);
                canPause = true;
                pauseLock.notifyAll();
            }        
            if (wlen == -1) {
                return false;
            }
            Thread.yield();
            if (recordControl != null) {
                // recordControl.record(buffer, 0, len);
                recordControl.record(bufferObject.data, bufferObject.offset, len);
            }

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
                if (interrupted || !statusOK) {
                    break;
                }    
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
            if (source != null) {
                source.disconnect();    
            }
            sendEvent(PlayerListener.ERROR, runErr);
        }
    }


    /**
     * ====================================
     * Read calls to read from SourceStream
     * ====================================
     */

    /**
     * Read bytes from source stream.
     * @param array the byte array to hold the data
     * @param offset the offset in the byte array
     * @param num the number of bytes to be read
     * @return the actual number of bytes has been read
     */

    

    /**
     * Read a byte from source stream
     * @return the byte read
     * @throws IOException if there is an error
     */
    byte readByte() throws IOException {
        stream.read(intArray, 0, 1);
        return intArray[0];
    }

    int readBytes(byte[] array, int offset, int num) throws IOException {
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

    /**
     * temporary buffer
     */
    byte [] intArray = new byte[4];

    /**
     * Read an integer from source stream
     * @return the integer read.
     * @throws IOExeption if there is an error
     *
     * IMPL_NOTE: add endian parameter or have separate functions
     * for little-endian and big-endian, and make it package-private.
     */
    private int readInt() throws IOException {
        if (readFully(intArray, 0, 4) < 4) {
            throw new IOException("malformed wave data");
        }
        return ((intArray[3] & 0xFF) << 24) |
            ((intArray[2] & 0xFF) << 16) |
            ((intArray[1] & 0xFF) << 8) |
            (intArray[0] & 0xFF);
    }

    /**
     * Read a short from source stream
     * @return the short read
     *
     * IMPL_NOTE: add endian parameter or have separate functions
     * for little-endian and big-endian, and make it package-private.
     * @throws IOException if there is an error
     */
    private short readShort() throws IOException {
        if (readFully(intArray, 0, 2) < 2){
            throw new IOException("malformed wave data");
        }
        return (short) (((intArray[1] & 0xFF) << 8) |
            (intArray[0] & 0xFF));
    }


    /**
     * ==========================
     * Methods for VolumeControl.
     * ==========================
     */
    /**
     * The worker method to actually obtain the control.
     *
     * @param vol the volume level to be set.
     * @return the actual level has been set.
     */
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
        return (Object) ((altRenderer == null) ? renderer : altRenderer); 
    }

    /**
     *  ==================
     *  WavRateCtrl 
     *  ==================
     */
    
    class WavRateCtrl implements RateControl {
        private int playRateMP = 100000;
        private int minRate = 10000;
        private int maxRate = 200000;
    
        public int getRate() {
            return playRateMP;
        }
    
        public int setRate(int milliRate) {
            int state = getState();
            if (milliRate < minRate) {
                milliRate = minRate;
            }
            if (milliRate > maxRate) {
                milliRate = maxRate;
            }
            playRateMP = milliRate;
            /* 
             * (renderer == null) if (state<=REALIZED), 
             * however RateControl can be requested in (state==REALIZED) too ! 
             */
            if (renderer != null)
                renderer.setRate(playRateMP);
            return playRateMP;
        }
    
        public int getMinRate() {
            return minRate;
        }
    
        public int getMaxRate() {
            return maxRate;
        }
    }
}


