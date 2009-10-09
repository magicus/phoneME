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

public class Notice extends NoticeBase {

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
     * Default constructor for serialization
     * 
     */
    public Notice() {
    }

    public int getOriginatorID() {
        return origID;
    }

    public synchronized void post(boolean selectable, int duration) throws IOException {
        super.post(selectable, duration);
        NoticeManager.getInstance().post(this);
    }

    public synchronized void remove() {
        super.remove();
        // this will cause removed() callback, but it is OK
        // because DELETED reason is being filtered out
        NoticeManager.getInstance().remove(this, DELETED);
    }


    public void dismiss() {
        // this will cause removed() callback, 
        // but caller instance has no listeners so nothing happens
        NoticeManager.getInstance().remove(this, DISMISSED);
    }

    public void select() {
        // this will cause removed() callback, 
        // but caller instance has no listeners so nothing happens
        NoticeManager.getInstance().remove(this, SELECTED);
    }

    public void timeout() {
        // this will cause removed() callback, 
        // but caller instance has no listeners so nothing happens
        NoticeManager.getInstance().remove(this, TIMEOUT);
    }

    public void serialize(DataOutputStream out) throws IOException {
System.out.println("1");
        out.writeInt(origID);
System.out.println("2");
        out.writeInt(type.getType());
System.out.println("3");
        out.writeBoolean(selectable);
System.out.println("4");
        out.writeLong(timeout);
System.out.println("5");
        out.writeUTF(originator);
System.out.println("6");
        out.writeBoolean(null != label);
        if (null != label) {
System.out.println("7");
            out.writeUTF(label);
        }
        if (null != image) {
            int w = image.getWidth();
            int h = image.getHeight();
System.out.println("8:" + w);
            out.writeInt(w);
System.out.println("9:" + h);
            out.writeInt(h);
            if (w > 0 && h > 0) {
                int[] buf = new int[image.getWidth() * image.getHeight()];
                image.getRGB(buf, 0, 0, 0, image.getWidth(), image.getWidth(), image.getHeight());
                int i = 0;
                while (i++<buf.length) {
                    out.writeInt(buf[i]);
                }
            }
        } else {
System.out.println("8");
            out.writeInt(1);
System.out.println("9");
            out.writeInt(1);
        }
    }

    public void deserialize(DataInputStream in) throws IOException {
System.out.println("11");
        origID = in.readInt();
System.out.println("21");
        int typeUID = in.readInt();
        if (null == type || typeUID != type.getType()) {
            type = new NoticeType(typeUID);
        }
System.out.println("31");
        selectable = in.readBoolean();
System.out.println("41");
        timeout = in.readLong();
System.out.println("51");
        originator = in.readUTF();
System.out.println("61");
        boolean hasLabel = in.readBoolean();
        if (hasLabel) {
System.out.println("71");
            label = in.readUTF();
        } else {
            label = null;
        }
System.out.println("81");
        int w = in.readInt();
System.out.println("81:" + w);
System.out.println("91");
        int h = in.readInt();
System.out.println("91:"+h);
        if (w > 1 && h > 1) {
            int[] buf = new int[image.getWidth() * image.getHeight()];
            int i = 0;
            while (i++<buf.length) {
                buf[i] = in.readInt();
            }
            image = Image.createRGBImage(buf, w, h, true);
        } else {
            image = null;
        }
    }
}
