/*
 *
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
import javax.microedition.media.Player;
import com.sun.j2me.app.AppModel;
import java.lang.*;
import java.lang.reflect.*;

/**
 * Abstraction for the video renderer for the particular application model
 */
public class ModelVideoRenderer{
     
    /** Guard from 'new' operator */
    private ModelVideoRenderer()
    {
    }
    
    public static VideoRenderer getVideoRenderer(BasicPlayer player, 
                                          int sourceWidth, 
                                          int sourceHeight) {
        int appModel = AppModel.getAppModel();
        String className;
        VideoRenderer ret = null;

        if (appModel == AppModel.XLET)
        {
            className = "com.sun.mmedia.AWTVideoRenderer";
        } else if (appModel == AppModel.MIDLET) {
            className = "com.sun.mmedia.MIDPVideoRenderer";
        } else {
            return null;
        }
        try {
            Class clazz = Class.forName(className);
            Integer w = new Integer(sourceWidth);            
            Integer h = new Integer(sourceHeight);            
            /*
            Constructor constructor = clazz.getConstructor(new Class[] {Player.class, int.class, int.class} );
            ret = (VideoRenderer)constructor.newInstance( new Object[] {player, w, h});
             */
            Constructor constructor[] = clazz.getConstructors();
            ret = (VideoRenderer)constructor[0].newInstance( new Object[] {player, w, h});
        } catch ( Exception ex ) {
            ex.printStackTrace();
            return null;
        }
        return ret;
    }

 }
