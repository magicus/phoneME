 /*
 * 
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

import com.sun.mmedia.DefaultConfiguration;
import com.sun.j2me.app.AppIsolate;

import javax.microedition.media.*;
import java.io.IOException;
import java.io.ByteArrayOutputStream;
import javax.microedition.media.control.*;
import java.util.*;

import com.sun.mmedia.DefaultConfiguration;
import com.sun.j2me.log.Logging;
import com.sun.j2me.log.LogChannels;
import com.sun.mmedia.control.DualToneControl; 
/**
 * Java Tone Sequence Player
 * it implements ToneControl
 */
public final class DirectTone extends DirectPlayer {

    /**
     * It does not need data source
     */
    public DirectTone() {
        super();
        hasDataSource = false;
    }

    /**
     * the worker method to realize the player
     *
     * @exception  MediaException  Description of the Exception
     */
    protected void doRealize() throws MediaException {

        // Get current isolate ID to support MVM
        int isolateId = AppIsolate.getIsolateId();        
        // Init native library
        if (this.source == null) {
            hNative = nInit(isolateId, pID, Manager.TONE_DEVICE_LOCATOR, Manager.TONE_DEVICE_LOCATOR, -1);
        } else {
            hNative = nInit(isolateId, pID, DefaultConfiguration.MIME_AUDIO_TONE, source.getLocator(), -1);
        }
        
        if (hNative == 0) {
            throw new MediaException("Unable to realize tone player");
        }

        // if no source stream, player is created from TONE_DEVICE_LOCATOR
        // simply return it.
        if (stream == null) {
            return;
        }

        // read the whole sequence from the source stream
        int chunksize = 128;
        byte[] tmpseqs = new byte[chunksize];
        byte[] seqs = null;
        // make use of BAOS, since it takes care of growing buffer
        ByteArrayOutputStream baos = new ByteArrayOutputStream(chunksize);

        try {
            int read;

            while ((read = stream.read(tmpseqs, 0, chunksize)) != -1) {
                baos.write(tmpseqs, 0, read);
            }

            seqs = baos.toByteArray();
            baos.close();
            tmpseqs = null;
            System.gc();

        } catch (IOException ex) {
            throw new MediaException("unable to realize: fail to read from source");
        }
        
        try {
            ToneControl tControl = new DirectToneControl(this, false);
            tControl.setSequence(seqs);
        } catch (Exception e) {
            throw new MediaException("unable to realize: " + e.getMessage());
        }
    }

    /**
     * The worker method to actually obtain the control.
     *
     * @param  type  the class name of the <code>Control</code>.
     * @return       <code>Control</code> for the class or interface
     * name.
     */
    protected Control doGetControl(String type) {
        Control c = super.doGetControl(type);
        if (c != null) {
            return c;
        }

        if (getState() >= REALIZED) {
            if (type.equals("javax.microedition.media.control.ToneControl") ||
                type.equals("ToneControl")) {
                return new DirectToneControl(this, false);
            }
            if (type.equals("com.sun.mmedia.control.DualToneControl")) {
                return new DirectToneControl(this, true);
            }
        }

        return null;
    }


    /**
     * Override getContentType from BasicPlayer
     * Always return DefaultConfiguration.TONE content type
     */
    public String getContentType() {
        chkClosed(true);
        return DefaultConfiguration.MIME_AUDIO_TONE;
    }

    /**
     * Sets the tone sequence.<p>
     * 
     * @param sequence The sequence to set.
     * @exception IllegalArgumentException Thrown if the sequence is 
     * <code>null</code> or invalid.
     * @exception IllegalStateException Thrown if the <code>Player</code>
     * that this control belongs to is in the <i>PREFETCHED</i> or
     * <i>STARTED</i> state.
     */
    void setSequence(byte[] sequence)
    {
        if (this.getState() >= Player.PREFETCHED)
            throw new IllegalStateException("cannot set seq after prefetched");
       
        nFlushBuffer(hNative);

        nBuffering(hNative, sequence, sequence.length);
        
        if(-1 == nBuffering(hNative, sequence, -1))
            throw new IllegalArgumentException("invalid sequence");

        hasToneSequenceSet = true;
    }

    /**
     * Java Tone Sequence Control
     * it implements ToneControl
     */
    public final class DirectToneControl implements ToneControl, DualToneControl {
        private DirectTone _player;
        private boolean dualTone;

        DirectToneControl(DirectTone player, boolean dualTone) {
            _player = player;
            this.dualTone = dualTone;
        }

        private boolean checkSequence( byte[] seq )
        {
            int t;
            int pos = 0;

            boolean blk_defined[] = new boolean[128];
            boolean res_defined = false;
            boolean tmp_defined = false;
            int  blk         = -1;
            
            if( seq.length < 2 ) return false;

            for (int i = 0; i < 128; i++) blk_defined[i] = false;

                while (pos < seq.length)
                {
                    switch (t = seq[pos++])
                    {
                        case ToneControl.VERSION:
                            if (1 != seq[pos]) return false;
                            pos++;
                            break;
                        case ToneControl.TEMPO:
                            if (tmp_defined) return false;
                            if (seq[pos] < 5 || seq[pos] > 127) return false;
                            tmp_defined = true;
                            pos++;
                            break;
                        case ToneControl.RESOLUTION:
                            if (res_defined) return false;
                            if (seq[pos] < 1 || seq[pos] > 127) return false;
                            res_defined = true;
                            pos++;
                            break;
                        case ToneControl.BLOCK_START:
                            if (seq[pos] < 0 || seq[pos] > 127) return false;
                            blk = seq[pos++];
                            break;
                        case ToneControl.BLOCK_END:
                            if (-1 == blk) return false;
                            if (seq[pos++] != blk) return true;
                            blk_defined[blk] = true;
                            blk = -1;
                            break;
                        case ToneControl.PLAY_BLOCK:
                            if (seq[pos] < 0 || seq[pos] > 127) return false;
                            if (!blk_defined[seq[pos]]) return false;
                            pos++;
                            break;
                        case ToneControl.SET_VOLUME:
                            if (seq[pos] < 0 || seq[pos] > 100) return false;
                            pos++;
                            break;
                        case ToneControl.REPEAT:
                            if (seq[pos] < 2 || seq[pos] > 127) return false;
                            pos++;
                            break;
                        case ToneControl.SILENCE:
                            if (seq[pos] < 1 || seq[pos] > 127) return false;
                            pos++;
                            break;
                        case DualToneControl.DUALTONE:
                            if (!dualTone) return false;
                            if (seq[pos] < 0 || seq[pos] > 127) return false;
                            pos++;
                            if (seq[pos] < 0 || seq[pos] > 127) return false;
                            pos++;
                            if (seq[pos] < 0 || seq[pos] > 127) return false;
                            pos++;
                            break;
                        default: // note
                            if (t < 0 || t > 127) return false;
                            if (seq[pos] < 1 || seq[pos] > 127) return false;
                            pos++;
                    }
                }

            return true;
        }

        /**
         * Sets the tone sequence.<p>
         * 
         * @param sequence The sequence to set.
         * @exception IllegalArgumentException Thrown if the sequence is 
         * <code>null</code> or invalid.
         * @exception IllegalStateException Thrown if the <code>Player</code>
         * that this control belongs to is in the <i>PREFETCHED</i> or
         * <i>STARTED</i> state.
         */
        public void setSequence(byte[] sequence)
        {
            if (sequence == null) 
                throw new IllegalArgumentException("null sequence");
            if (!checkSequence(sequence)) 
                throw new IllegalArgumentException("invalid sequence");
            _player.setSequence(sequence);
        }
    }
}
