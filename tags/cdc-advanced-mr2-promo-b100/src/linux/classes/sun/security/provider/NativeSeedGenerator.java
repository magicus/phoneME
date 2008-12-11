/*
 * @(#)NativeSeedGenerator.java	1.5 06/10/10
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

package sun.security.provider;

import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.URL;
import sun.security.util.Debug;

/**
 * Native seed generator for Linix systems. Inherit everything from
 * URLSeedGenerator.
 *
 * @version 1.2, 01/23/03
 */
class NativeSeedGenerator extends SeedGenerator.URLSeedGenerator {

    private static final Debug debug = Debug.getInstance("provider");

    private String deviceName;
    private BufferedInputStream devRandom;

    // For CDC/FP 1.1.1 we provide a fallback mechanism to deal with
    // the fact that on some devices the entropy gathering device
    // can block. The ThreadedSeedGenerator approach is not an ideal
    // solution for the range of devices which CDC/FP addresses, so
    // we will use /dev/urandom, which will not block.
    final static String URL_FALLBACK = "file:/dev/urandom";

    // Allow the fallback mechanism to be disabled if desired. This
    // could mean that an application's main threat will hang,
    // but it allows for stronger random seed generation.
    private final static String PROP_NOFALLBACK = 
        "microedition.securerandom.nofallback";
    private static boolean createFallback = true;

    // The actual fallback InputStream from which we read.
    private BufferedInputStream fallback;

    // An object to sit between the main thread and the
    // entropy generation device, to avoid blocking an
    // application if there is not sufficient entropy.
    private static RandomReader ranReader = null;

    NativeSeedGenerator() throws IOException {
        this(SeedGenerator.URL_DEV_RANDOM);
    }

    NativeSeedGenerator(String egdurl) throws IOException {
        if (egdurl == null) {
            throw new IOException("No random source specified");
        }
        this.deviceName = egdurl;
        // Because of the problems of blocking I/O on smaller
        // devices, we do not read the random device directly,
        // but rather do so in a separate thread from which we
        // can retrieve data when it is available.
        if ( ranReader == null ) {
            ranReader = new RandomReader( egdurl );
            if (ranReader == null) {
                throw new InternalError( "RandomReader thread creation failure" );
            }
        }
        // Check to see whether the fallback mechanism is disabled.
        Boolean noFallback =
            (Boolean)java.security.AccessController.doPrivileged
            (new java.security.PrivilegedAction() {
                    public Object run() {
                        String prop = System.getProperty(PROP_NOFALLBACK,
                                                         "false");
                        return Boolean.valueOf(prop);
                    }
                });
        createFallback = ! noFallback.booleanValue();
    }

    // For CDC/FP, if the fallback mechanism is not disabled
    // and we are going to block on the default entropy generation
    // device, we'll open the fallback URL instead.
    private void createFallback() throws IOException {
        if (debug != null) {
            debug.println("Creating fallback seed generator");
        }
        final URL fallbackDevice = new URL(this.URL_FALLBACK);
        fallback =
            (BufferedInputStream)java.security.AccessController.doPrivileged
            (new java.security.PrivilegedAction() {
                    public Object run() {
                        try {
                            return new BufferedInputStream(fallbackDevice.openStream());
                        } catch ( IOException ioe ) {
                            return null;
                        }
                    }
                });
        if (fallback == null) {
            throw new IOException( "failed to open " +
                                   this.URL_FALLBACK );
        }
        return;
    }

    synchronized byte getSeedByte() {
        byte b[] = new byte[1];
        int stat;
        // If we would block trying to read from the entropy
        // generation device, and we haven't been instructed
        // not to, use a fallback alternative which will not
        // block.
        if (ranReader.available() == 0 && createFallback == true) {
            try {

                if (fallback == null) {
                    createFallback();
                }

                // Do the actual read. If we get a byte of
                // data, give it to whomever wanted it.
                stat = fallback.read(b, 0, b.length);
                if (stat == b.length) {
                    return b[0];
                }

            } catch (IOException ioe) {
                throw new InternalError("NativeSeedGenerator " + 
                                        this.URL_FALLBACK +
                                        " generated exception: " +
                                        ioe.getMessage());
            } catch (Exception e) {
                throw new InternalError("NativeSeedGenerator " + 
                                        this.URL_FALLBACK +
                                        " generated exception: " +
                                        e.getMessage());
            }
            // The read didn't generate an exception but gave us
            // bad data in any case.
            throw new InternalError("NativeSeedGenerator " +
                                    this.URL_FALLBACK +
                                    " failed read: " + stat );
        }
        // Read from the main entropy device. This happens if data is
        // available *or* if the property has been set which
        // instructs us not to use the fallback device no matter
        // what.
        return ranReader.getSeedByte();
    }

    /*
     * CR 6260366. Because of varying methods for generating
     * entropy in the default random device (/dev/random, generally),
     * we will read from it in a separate thread. This will allow
     * reads to block until the device has generated sufficient
     * entropy. If the device will block, we will instead default
     * to file:/dev/urandom.
     */
    private static class RandomReader implements Runnable {
        private String deviceName;
        private BufferedInputStream randomDevReader;
        private Thread readerThread;

        byte[] buf = new byte[ 20 ];
        int start = 0;
        int end = 0;

        RandomReader(String egdurl) throws IOException {
            if (egdurl == null) {
                throw new IOException("No random source specified");
            }
            deviceName = egdurl;
            init();
        }           

        // Initialize ourselves. Open the specified device and
        // start the run() method. 
        void init() throws IOException {
            final URL device = new URL(deviceName);
            randomDevReader =
                (BufferedInputStream)java.security.AccessController.doPrivileged
                (new java.security.PrivilegedAction() {
                        public Object run() {
                            try {
                                return new BufferedInputStream(device.openStream(), 1);
                            } catch (IOException ioe) {
                                return null;
                            }
                        }
                    });

            // Oops. Check for failure.
            if (randomDevReader == null) {
                throw new IOException("failed to open " + device);
            }
            
            // Create a thread which will do the actual reading.
            readerThread = (Thread)java.security.AccessController.doPrivileged
                (new java.security.PrivilegedAction() {
                        public Object run() {
                            Thread newT = new Thread(Thread.currentThread().getThreadGroup(),
                                                     RandomReader.this,
                                             "Random Device Reader Thread");
                            newT.setPriority(Thread.MIN_PRIORITY);
                            newT.setDaemon(true);
                            return newT;
                        }
                    });
            if (readerThread == null) {
                throw new InternalError("Cannot create RandomReader thread for " + deviceName);
            }
            readerThread.start();
            return;
        }

        // The real work of RandomReader is done here. Read from the
        // random device (which may block in some implementations).
        // Retrieved bytes will be taken from our  buffer by the
        // getSeedByte() method.
        public void run() {
            while (true) {
                try {
                    // If we don't need to read any data, wait until
                    // the buffer is empty.
                    synchronized(this) {
                        while (start < end) {
                            wait();
                        }
                    }
                    // Read to fill the buffer
                    end = randomDevReader.read( buf, 0, buf.length );
                    if (end < 0) {
                        throw new InternalError("RandomReader read failed on " + deviceName);
                    }
                    start = 0;
                    // Notify anyone looking for data that the
                    // buffer is full.
                    synchronized(this) {
                        notify();
                    }
                } catch (Exception e) {
                    throw new InternalError("RandomReader thread generated exception: " +
                                             e.getMessage());
                }
            }
        }

        // Is there anything available in the buffer to consume?
        int available() {
            return end - start;
        }

        // Take a byte from the buffer.
        byte getSeedByte() {
            byte b = 0;
            try {
                synchronized(this) {
                    // If we have no data, wait until it becomes
                    // available. The buffer will be populated by
                    // the run() method.
                    if (start == end) {
                        wait();
                    }
                    b = buf[start++];
                    notifyAll();
                }
            } catch (Exception e) {
                throw new InternalError("RandomReader thread failed getSeedByte");
            }
            return b;
        }
    }

}
