/*
 *
 *
 * Copyright  1990-2009 Sun Microsystems, Inc. All Rights Reserved.
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

package com.sun.midp.lcdui;

import java.io.ByteArrayOutputStream;
import java.io.DataInput;
import java.io.DataInputStream;
import java.io.DataOutput;
import java.io.DataOutputStream;
import java.io.IOException;

import javax.microedition.lcdui.Image;

import com.sun.cldc.isolate.Isolate;
import com.sun.midp.ams.VMUtils;
import com.sun.midp.events.NativeEvent;
import com.sun.midp.main.NoticeManager;
import com.sun.midp.midlet.MIDletStateHandler;

/**
 * MVM specific extension of {@link NoticeBase}
 * 
 */
public class Notice extends NoticeBase {

    /**
     * The notice originator identifier.
     * 
     */
    int origID;

    /**
     * Creates a new Notification with the given type, label and
     * image.
     *
     * @param type  the notification type
     * @param label the notification's label
     * @param image the notification's image
     */
    public Notice(NoticeType newType, String newLabel, Image newImage){
        super(newType, newLabel, newImage);
        origID = VMUtils.getIsolateId();
    }

    /**
     * Returns notice originator ID. 
     * <p> 
     * So far the ID equals to isolate ID of originator application.
     * 
     * @return originator ID. 
     */
    public int getOriginatorID() {
        return origID;
    }

    /**
     * Posts the notice.
     * 
     * @param selectable if the notice can selected by the user.
     * @param duration 
     */
    public synchronized void post(boolean selectable, int duration) throws IOException {
        super.post(selectable, duration);
        NoticeManager.getInstance().post(this);
    }

    /**
     * Removes the notice. Called by originator application.
     * 
     */
    public synchronized void remove() {
        super.remove();
        // this will cause removed() callback, but it is OK
        // because DELETED reason is being filtered out
        NoticeManager.getInstance().remove(this, DELETED);
    }


    /* --- Notice control functions are not available for the application.--- */

    /**
     * Called by Notice visualizer when the user dismisses the 
     * notice. 
     * <p> 
     * Posts corresponding message to all running tasks.
     */
    public void dismiss() {
        // this will cause removed() callback, 
        // but caller instance has no listeners so nothing happens
        NoticeManager.getInstance().remove(this, DISMISSED);
    }

    /**
     * Called by Notice visualizer when the user selects the notice.
     * <p>
     * Posts corresponding message to all running tasks.
     */
    public void select() {
        // this will cause removed() callback, 
        // but caller instance has no listeners so nothing happens
        NoticeManager.getInstance().remove(this, SELECTED);
    }

    /**
     * Called by Notice watcher when notice duration exceeded. 
     * <p> 
     * Posts corresponding message to all running tasks. 
     * 
     */
    public void timeout() {
        // this will cause removed() callback, 
        // but caller instance has no listeners so nothing happens
        NoticeManager.getInstance().remove(this, TIMEOUT);
    }

    /**
     * Called by {@link NoticeManager} in reponse to event caused by 
     * one of {@link #dismiss},  {@link #select}, {@link #timeout} 
     * call. 
     * 
     * 
     * @param reason the reason of removing.
     */
    public void removed(int reason) {
        if (REMOVED == status) {
            return;
        }
        status = REMOVED;
        if (null != listener) {
            if (SELECTED == reason) {
                listener.noticeSelected((Notice)this);
            } else if (DISMISSED == reason) {
                listener.noticeDismissed((Notice)this);
            } else if (TIMEOUT == reason) {
                listener.noticeTimeout((Notice)this);
            }
        }
    }


    /* ------------------  SERIALIZATION section ----------------- */

    /**
     * Default constructor for serialization
     * 
     */
    public Notice() {
    }

    /**
     * Serializes internal datas into given stream. 
     * <p> 
     * Used to transfer <code>Notice</code> between tasks
     * 
     * @param out output stream 
     * @throw IOException if one of the field can be written into 
     *        the stream
     */
    public void serialize(DataOutputStream out) throws IOException {
        out.writeInt(uid);
        out.writeInt(origID);
        out.writeInt(type.getType());
        out.writeBoolean(selectable);
        out.writeLong(timeout);
        out.writeUTF(originator);
        out.writeBoolean(null != label);
        if (null != label) {
            out.writeUTF(label);
        }
        if (null != image) {
            int w = image.getWidth();
            int h = image.getHeight();
            out.writeInt(w);
            out.writeInt(h);
            if (w > 0 && h > 0) {
                int[] buf = new int[w * h];
                image.getRGB(buf, 0, w, 0, 0, w, h);
                int i = 0;
                while (i<buf.length) {
                    out.writeInt(buf[i]);
                    i++;
                }
            }
        } else {
            /* Workaround: nulls at the end of serialized bytearray are skipped during converting to a string.
               Shell be restored to original values when byte array be introduced as Event field. */
            out.writeInt(1);
            out.writeInt(1);
        }
    }

    /**
     * Restores internal fields from given stream.
     * <p> 
     * Used to transfer <code>Notice</code> between tasks 
     * @param in input stream 
     * @throw IOException if one of the field can be restored
     */
    public void deserialize(DataInputStream in) throws IOException {
        uid = in.readInt();
        origID = in.readInt();
        int typeUID = in.readInt();
        if (null == type || typeUID != type.getType()) {
            type = new NoticeType(typeUID);
        }
        selectable = in.readBoolean();
        timeout = in.readLong();
        originator = in.readUTF();
        boolean hasLabel = in.readBoolean();
        if (hasLabel) {
            label = in.readUTF();
        } else {
            label = null;
        }
        int w = in.readInt();
        int h = in.readInt();
        /* Workaround: nulls at the end of serialized bytearray are skipped during converting to a string.
           Shell be restored to original values when byte array be introduced as Event field. */
        if (w > 1 && h > 1) {
            int[] buf = new int[w * h];
            int i = 0;
            while (i<buf.length) {
                buf[i] = in.readInt();
                i++;
            }
            image = Image.createRGBImage(buf, w, h, true);
        } else {
            image = null;
        }
    }

}
