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
import java.io.OutputStream;
import java.io.ByteArrayOutputStream;
import javax.microedition.media.Player;
import javax.microedition.media.PlayerListener;
import javax.microedition.media.control.RecordControl;
import javax.microedition.media.MediaException;

import com.sun.mmedia.BasicPlayer;

import com.sun.midp.security.Permissions;
import com.sun.midp.security.SecurityToken;
import com.sun.midp.security.ImplicitlyTrustedClass;
import com.sun.mmedia.PermissionAccessor;

import javax.microedition.io.*;
import com.sun.midp.io.j2me.storage.*;
import com.sun.mmedia.protocol.FileConnectionSubstitute;

/**
 * This class that implements RecordControl. It has one abstract method
 * called <code>getHeader</code>. Subclasses should implement this method
 * which returns the header data for the corresponding file format.
 * For example, if recording in content-type <code>"audio/x-wav"></code>
 * is supported, the subclass should implement <code>getHeader</code> and
 * return the header for the wav format.
 *
 * There are 2 ways of recording --- 1) recording to a locator in URL syntax,
 * an example of which is recording to a file and 2) recording to a OutputStream.
 */

public abstract class RecordCtrl implements RecordControl, ImplicitlyTrustedClass {
    
    private final boolean RECORD = true;
    private final boolean RECORD_TO_FILE = true;
    private final boolean RECORD_TO_HTTP = false;
    
    /* The string in URL format specifying where the recorded media
     * will be saved
     */
    private String locator; // null;

     /* The filename portion of the locator */
    private String fileName; // null;

    // #ifdef RECORD_TO_HTTP [
    private HttpConnection httpCon;  // null
    // #endif ]
    /* The temporary file where the recording will be stored when
     * recording to a stream
     */
    private String localFileName = null;

    private OutputStream httpOutputStream; // null

    /* handle that contains the native file pointer of the local file
       when recording to a stream
    */
    private int localFilePointer; // 0;

    private OutputStream stream; // null
    /* Temporary buffer used to transfer the recorded data from the
     * temporary file to the output stream
     */
    private byte[] localBuffer;
    /* The size of the localBuffer */
    private int CHUNK_SIZE = 8192;

    /* The player that provides the media to be recorded.
     * This is set by the base class
     */
    protected BasicPlayer player;
    /* specifies whether recording is requested via <code>startRecord</code> */
    private boolean recordRequested; // false;
    /* This is set to true when recording starts (RECORD_STARTED event
     * is posted). Is set to false when recording stops (RECORD_STOPPED event
     * is posted).
     */
    private boolean recording; // false;
    /* Size, if any, specified via <code>setRecordSizeLimit</code> method */
    private int sizeLimit = Integer.MAX_VALUE;
    /* Size of the media header */
    protected int headerSize; // 0;
    /* Size of the media data, not including the header, that was recorded. */
    private int dataSize; // 0;
    
    private FileConnectionSubstitute fileConnSubst = null;
    private FileConnectionSubstitute fileTemp = null;
    
    private static SecurityToken classSecurityToken;

    /**
     * Initializes the security token for this class, so it can
     * perform actions that a normal MIDlet Suite cannot.
     *
     * @param token security token for this class.
     */
    public final void initSecurityToken(SecurityToken token) {
        if (classSecurityToken == null) {
            classSecurityToken = token;
        }
    }
    
    /**
     * Convenience method that can be used by the base class to
     * write an int into a byte buffer
     * @param buf The byte array buffer where the integer will be written
     * @param value The integer value to be written
     * @param offset The offset in the buffer where the data is written
     * @param bigEndian boolean variable that specifies the endian
     */
    static public void writeInt(byte[] buf, int value, int offset, boolean bigEndian) {
        int start;
        int inc;

        if (!bigEndian) {
            start = 3;
            inc = -1;
        } else {
            start = 0;
            inc = 1;
        }
        start += offset;
        buf[start] = (byte)((value >>> 24) & 0xFF);
        buf[start+inc] = (byte)((value >>> 16) & 0xFF);
        buf[start+inc+inc] = (byte)((value >>>  8) & 0xFF);
        buf[start+inc+inc+inc] = (byte)((value >>>  0) & 0xFF);
    }


    /**
     * Convenience method that can be used by the base class to
     * write a short into a byte buffer
     * @param buf The byte array buffer where the short integer will be written
     * @param value The short integer value to be written
     * @param offset The offset in the buffer where the data is written
     * @param bigEndian boolean variable that specifies the endian
     */
    static public void writeShort(byte[] buf, int value, int offset, boolean bigEndian) {
        int start;
        int inc;

        if (!bigEndian) {
            start = 1;
            inc = -1;
        } else {
            start = 0;
            inc = 1;
        }
        start += offset;
        buf[start] = (byte)((value >> 8) & 0xFF);
        buf[start+inc] = (byte)((value >> 0) & 0xFF);
    }


    public final void setRecordStream(OutputStream stream)  throws SecurityException {
	
        if (recordRequested) {
            throw new IllegalStateException("setRecordStream cannot be called after startRecord is called");
        }

        if (locator != null) {
            throw new
            IllegalStateException("setRecordStream: setRecordLocation has been called and commit has not been called");
        }

        if (stream == null)
            throw new IllegalArgumentException("setRecordStream: null stream specified");

        // Check for the record permissions.
        checkPermission();

        this.stream = stream;

        if (localFileName == null) {
            localFileName = FileIO.getTempFileName();
        }
        localFilePointer = FileIO.nOpen(localFileName.getBytes(), FileIO.WRITE);
    }

    public final void setRecordLocation(String locator)
    	throws IOException, MediaException, SecurityException  {
        
        if (recordRequested) {
            throw new IllegalStateException("setRecordLocation cannot be called after startRecord is called");
        }

        if (stream != null) {
            throw new
            IllegalStateException("setRecordLocation: setRecordStream has been called and commit has not been called");
        }

        if (locator == null) {
            throw new IllegalArgumentException("setRecordLocation: null locator specified");
        }

        if (!RECORD) {
            throw new SecurityException("setRecordLocation is unsupported on this platform");
        }
        
        // Check for record permissions
        checkPermission();
        // Check for file write permissions
        checkFileWritePermission();

        FileIO.removeTempFiles();

        int colonIndex = locator.indexOf(":");
        if (colonIndex < 0) {
            throw new MediaException("setRecordLocation: invalid locator " + locator);
        }

        if (RECORD_TO_HTTP && !RECORD_TO_FILE) {
            if (!locator.startsWith("http:")) {
                throw new MediaException("Only http protocol is supported");
            }
        }
        
        if (RECORD_TO_FILE && !RECORD_TO_HTTP) {
    	    if (!locator.startsWith("file:")) {
    	        throw new MediaException("Only file protocol is supported");
            }
        }

        if (RECORD_TO_FILE || RECORD_TO_HTTP) {
            if ( !( locator.startsWith("file:") || 
                locator.startsWith("http:") )) {
                throw new MediaException("Unsupported protocol");
            }
        }

        if (RECORD_TO_HTTP) {
            try {
                httpCon = (HttpConnection)Connector.open(locator);
            } catch (IllegalArgumentException e) {
                throw new MediaException(e.getMessage());
            } catch (ConnectionNotFoundException e) {
                throw new IOException(e.getMessage());
            }
            httpCon.setRequestMethod(HttpConnection.POST);
            httpCon.setRequestProperty("Content-Type", "audio/wav");

            httpOutputStream = httpCon.openOutputStream();

            if (localFileName == null) {
                localFileName = FileIO.getTempFileName();
            }
            localFilePointer = FileIO.nOpen(localFileName.getBytes(), FileIO.WRITE);
        }

    	if (RECORD_TO_FILE) { 
            // IMPL_NOTE: get the file extension(s) from the mime type and check if a
            // valid extension was specified. For now .wav is used
            if (!locator.endsWith(".wav"))
                throw new SecurityException("can only write .wav files");

            fileName = locator.substring(colonIndex + 1);
            if (fileName.length() <= 0) // Note: length can never be less than zero
                throw new MediaException("setRecordLocation: invalid locator " + locator);

            fileName = extractFileNameFromURL(fileName);
            if (fileName == null) {
                throw new MediaException("setRecordLocation: invalid locator " + locator);
            }
            String fname = new String(fileName);
            fileName = null;

            try {
                fname = File.getStorageRoot() + fname;
                fileConnSubst = new FileConnectionSubstitute( Permissions.MM_RECORD );

                fileConnSubst.connect(fname, Connector.READ_WRITE);
                fileConnSubst.truncate(0);
                fileConnSubst.writeBytes(new byte[headerSize], 0, headerSize);
            } catch (IOException ioe) {
                throw ioe;
            }

            this.locator = locator;
        }
    }

    // Note: when writeHeader is called, the info required to create the
    // header should be available --- channels, bits/sample etc

    // Should be called only during commit
    abstract protected byte[] getHeader(int dataSize) throws IOException;

    public final void startRecord() /*  throws SecurityException */ {
        if ( (locator == null) && (stream == null) ) {
            throw new IllegalStateException("Should call setRecordLocation or setRecordStream before calling startRecord");
        }

        /*  checkPermission(); */
    
        if ( (stream != null) && (localFilePointer == 0) ) {
            player.sendEvent(PlayerListener.RECORD_ERROR,
                     "Unable to create temporary file for local buffering. Check if environment variable TEMP points to writable directory");

            return;
        }

        if (RECORD_TO_HTTP) {
            if ( (httpOutputStream != null) && (localFilePointer == 0) ) {
                player.sendEvent(PlayerListener.RECORD_ERROR,
                             "Unable to create temporary file for local buffering. Check if environment variable TEMP points to writable directory");

                return;
            }
        }

        if (recordRequested)
            return;
    	recordRequested = true;

        if (player.getState() == Player.STARTED) {
            recording = true;
            player.sendEvent(PlayerListener.RECORD_STARTED, 
                new Long(player.getMediaTime()));
        }
    }

    final public void playerStart() {
        if (recordRequested && !recording) {
            recording = true;
            player.sendEvent(PlayerListener.RECORD_STARTED, 
                new Long(player.getMediaTime()));
        }
    }
	
    public final void stopRecord() {
        if (recordRequested) {
            recordRequested = false;
            if (recording) {
                player.sendEvent(PlayerListener.RECORD_STOPPED, 
                    new Long(player.getMediaTime()));
                recording = false;
            }
        }
    }
    
    synchronized public final void commit() throws IOException {

        if ( (stream == null) &&
             ((locator == null) || 
              ( (httpOutputStream == null)
            && (fileConnSubst == null) )) ) {
            return;
        }
        
        try {
            stopRecord();
            if (dataSize == 0) {
                // Nothing was recorded
                // There is nothing to do in the setRecordStream case.
                // In the setRecordLocation case, we will have a zero
                // length file. This implementation will try to delete this file.
                return;
            }

            byte[] header = getHeader(dataSize);
            if (RECORD_TO_HTTP) {
                if ( (stream == null) && (httpOutputStream != null) ) {
                    stream = httpOutputStream;
                    CHUNK_SIZE = 100;
                }
            }
	    
            if (stream != null) {
                stream.write(header);

                stream.flush();

                // Close and reopen local file for reading
                if (!FileIO.nClose(localFilePointer))
                    throw new IOException("I/O error");
                localFilePointer = FileIO.nOpen(localFileName.getBytes(), FileIO.READ);
                if (localFilePointer == 0)
                    throw new IOException("I/O error");

                int numChunks = dataSize / CHUNK_SIZE;
                int remaining = dataSize;
                int offset = 0;
                if (localBuffer == null) {
                    if (dataSize < CHUNK_SIZE)
                    CHUNK_SIZE = dataSize;
                    localBuffer = new byte[CHUNK_SIZE];
                }
                while (remaining > 0) {
                    int toWrite = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
                    if (FileIO.nRead(localFilePointer, localBuffer, 0, toWrite) != toWrite)
                        throw new IOException("I/O error");
                    stream.write(localBuffer, 0, toWrite); // throws IOException on error
                    // Optional
                    stream.flush();

                    // offset += toWrite;
                    remaining -= toWrite;
                }

                if (RECORD_TO_HTTP) { 
                    if ( (locator != null) && (httpCon != null) ) {
                        int rescode = httpCon.getResponseCode();

                        if (rescode != HttpConnection.HTTP_OK) {
                            throw new IOException("HTTP response code: " + rescode);
                        }
                    }
                    if (stream == httpOutputStream) // recording to http via setRecordLocation
                        stream.close();
                }
    	    } else {
                fileConnSubst.commitWrite();
                fileConnSubst.setPosition(0);
                fileConnSubst.writeBytes(header, 0, headerSize);
                fileConnSubst.commitWrite();
                fileConnSubst.disconnect();
                fileConnSubst = null;
    	    }
        } catch (Exception e) {
            throw new IOException(e.getMessage());
        } finally {
            cleanup(null);
        }
    }

    public final void close() {
        localBuffer = null;
        cleanup(null);
    }

    public final int setRecordSizeLimit(int size) throws MediaException {
        if (size <= 0)
            throw new IllegalArgumentException("setRecordSizeLimit: record size limit cannot be 0 or negative");
        sizeLimit = size;
        return size;
    }
    
    public final void reset() throws IOException {
        // reset implies a stopRecord.
        try {
            stopRecord();
            if ( (stream != null) || (httpOutputStream != null) ) {
                // Truncate local file to zero length
                if (!FileIO.nClose(localFilePointer) ||
                    ( (localFilePointer = FileIO.nOpen(localFileName.getBytes(), FileIO.WRITE)) == 0) )
                    throw new IOException("I/O error");
            }
            if (fileConnSubst != null) {
                fileConnSubst.truncate(0);
                fileConnSubst.writeBytes(new byte[headerSize], 0, headerSize);
            }

            dataSize = 0;
        } catch (IOException e) {
            cleanup(null);
            throw e;
        }
    }

    public void record(byte[] buf, int offset, int length) {
        try {
            if (recordRequested) {
                int actualWritten = write(buf, offset, length);
                if (actualWritten != length) {
                    cleanup("I/O Error while recording");
                    return;
                }
                dataSize += actualWritten;
                if ( (headerSize + dataSize) >= sizeLimit ) {
                    stopRecord();
                    commit();
                }
            }
        } catch (Exception e) {
            cleanup("I/O Error while recording");
        }
    }

    private void cleanup(String errorMessage) {
        stopRecord();
        fileName = null;
        locator = null;

        if (localFilePointer != 0)
            if ( !FileIO.nClose(localFilePointer) ||
             !FileIO.nDelete(localFileName.getBytes()) ) {
                localFileName = null;
            }
        localFilePointer = 0;
        stream = null;

        if (RECORD_TO_HTTP) {
            try {
                if (httpCon != null)
                httpCon.close();
                httpCon = null;
                httpOutputStream = null;
            } catch (IOException e) {
            }
        }

        try {
            if (fileConnSubst != null) {
                fileConnSubst.disconnect();
                fileConnSubst = null;
            }
        } catch (IOException ioe) {
        }
        dataSize = 0;
        if (errorMessage != null)
            player.sendEvent(PlayerListener.RECORD_ERROR, errorMessage);
    }

    private int write(byte[] buf, int offset, int length) {
        int actualWritten = -1;
        if ( (locator != null) && (locator.startsWith("file:")) ) {
            try {
                actualWritten = fileConnSubst.writeBytes(buf, offset, length);
            } catch (IOException ioe) {
                return -1;
            }
        } else if (localFilePointer != 0)
            actualWritten =  FileIO.nWrite(localFilePointer, buf, offset, length);
        return actualWritten;
    }

    private void skipHeader(int fp, String message) throws IOException {
        if (!seek(fp, headerSize))
            throw new IOException(message);
    }

    private boolean seek(int fp, int offset) {
    	return FileIO.nSeek(fp, offset);
    }
    
    private String extractFileNameFromURL(String url) {
        // Strip out special characters on the URL syntax.
        // %xx = the ASCII represented by the hexadecimal number "xx".
        try {
            int idx = 0;
            while ((idx = url.indexOf("%", idx)) >= 0) {
                if (url.length() > idx + 2) {
                    byte [] bytes = new byte[1];
                    try {
                        bytes[0] =
                            (byte)Integer.valueOf(
                                            url.substring(idx + 1, idx + 3),
                                                 16).intValue();
                        url = url.substring(0, idx) +
                            new String(bytes) +
                            url.substring(idx + 3);
                    } catch (NumberFormatException ne) {
                    }
                }
                idx++;
            }
            // WIN2000 SPECIFIC CODE START
            int startindex = 0;
            int length = url.length();
            while ( ( (length - startindex) > 2) &&
                (url.charAt(startindex) == '/' &&
                 (url.charAt(startindex+1) == '/' || url.charAt(startindex+2) == ':'))) {
                startindex++;
            }
            // WIN2000 SPECIFIC CODE END
            return url.substring(startindex);
        } catch (Exception e) {
            return null;
        }
    }

    /**
     * Check for the multimedia record permission.
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     */
    private static void checkPermission()  throws SecurityException {
        PermissionAccessor.checkPermissions(PermissionAccessor.PERMISSION_AUDIO_RECORDING);
    }
    /**
     * Check for the file write permission.
     *
     * @exception SecurityException if the permission is not
     *            allowed by this token
     */
    private static void checkFileWritePermission()  throws SecurityException {
        PermissionAccessor.checkPermissions(PermissionAccessor.PERMISSION_FILE_WRITE);
    }
}
