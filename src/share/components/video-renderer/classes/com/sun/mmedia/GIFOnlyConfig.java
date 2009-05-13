/*
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

public class GIFOnlyConfig extends Configuration {

    public String[] getSupportedContentTypes(String protocol) {
        String [] ret = null;
        if( null == protocol || protocol.equals("http") || protocol.equals("file") )
        {
            ret = new String [] { "image/gif" };
        }
        return ret;
    }

    public String[] getSupportedProtocols(String ctype) {
        String [] ret = null;
        if( null == ctype || ctype.equals("image/gif") )
        {
            ret = new String [] { "file", "http" };
        }
        return ret;
    }

    public String getProperty(String key) {
        String value = (String) properties.get(key);

        return value;
    }

    public void setProperty(String key, String value) {
        properties.put(key, value);
    }

    public ImageAccess getImageAccessor() {
        return null;
    }

    public VideoRenderer getVideoRenderer(HighLevelPlayer player) {
        // workaround for demo only
        return ModelVideoRenderer.getVideoRenderer(player);
        //return new MIDPVideoRenderer(player);
    }


    public TonePlayer getTonePlayer() {
        return null;
    }

}
