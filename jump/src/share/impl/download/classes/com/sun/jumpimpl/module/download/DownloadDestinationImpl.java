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
                                                                                      
package com.sun.jumpimpl.module.download;

import com.sun.jump.module.download.*;
import java.io.IOException;
import java.io.InputStream;

import java.net.URL;

public class DownloadDestinationImpl implements JUMPDownloadDestination {

        // Begin Destination implementation
        byte[] buffer;
        int bufferIndex = 0;
        JUMPDownloadDescriptor descriptor = null;
        String jarFile = null;
                                                                                      
        public DownloadDestinationImpl(JUMPDownloadDescriptor descriptor) {
            this.descriptor = descriptor;
            buffer = new byte[descriptor.getSize()];
        }
                                                                                      
        public byte[] getBuffer() {
            return buffer;
        }
                                                                                      
        public JUMPDownloadDescriptor getDescriptor() {
            return descriptor;
        }
                                                                                      
        public String getJarFile() {
            return jarFile;
        }
                                                                                      
        public void acceptMimeType(String mimeType) throws JUMPDownloadException {
            trace("saying we handle mimetype " + mimeType);
            return;
        }
                                                                                      
        public void start(URL sourceURL,
                          String mimeType) throws JUMPDownloadException, IOException {
            trace("download is about to start from " + sourceURL +
                  ", of type " + mimeType);
            return;
        }
                                                                                      
        public int receive(InputStream in, int desiredLength) throws
            JUMPDownloadException, IOException {
            trace("receiving data ");

            int numRead = in.read(buffer, bufferIndex, desiredLength);
            if (numRead > 0) {
                bufferIndex += numRead;
            }
            return numRead;
                                                                                      
        }
                                                                                      
        public URL finish() throws JUMPDownloadException, IOException {
                                                                                      
            trace("download succeeded. save the results");
                                                                                      
            return null;
        }
                                                                                      
        public void abort() {
            trace("download aborted");
            return;
        }
                                                                                      
        public int getMaxChunkSize() {
            trace("saying we'll take any chunksize");
            return 0;
        }
                                                                                      
        static void trace(String s) {
            if (false) {
                System.out.println(s);
            }
            return;
        }
        // End Destination implementation
                                                                                      
}


